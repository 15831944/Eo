#include "stdafx.h"
#include <DbSymUtl.h>
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbFile.h"
#include "EoDbBlockReference.h"
#include "EoDbPolyline.h"
EoDbPrimitive* EoDbGroup::sm_PrimitiveToIgnore = static_cast<EoDbPrimitive*>(nullptr);

EoDbGroup::EoDbGroup(const EoDbBlock& block) {
	auto Database {AeSysDoc::GetDoc()->m_DatabasePtr};
	const OdDbBlockTableRecordPtr BlockTableRecord {Database->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	auto Position {block.GetHeadPosition()};
	while (Position != nullptr) {
		AddTail(block.GetNext(Position)->Clone(BlockTableRecord));
	}
}

EoDbGroup::EoDbGroup(const EoDbGroup& group) {
	auto Database {AeSysDoc::GetDoc()->m_DatabasePtr};
	const OdDbBlockTableRecordPtr BlockTableRecord {Database->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	auto PrimitivePosition {group.GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		AddTail(group.GetNext(PrimitivePosition)->Clone(BlockTableRecord));
	}
}

void EoDbGroup::AddPrimitivesToTreeViewControl(const HWND tree, const HTREEITEM parent) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->AddToTreeViewControl(tree, parent);
	}
}

HTREEITEM EoDbGroup::AddToTreeViewControl(const HWND tree, const HTREEITEM parent) {
	const auto TreeItem {CMainFrame::InsertTreeViewControlItem(tree, parent, L"<Group>", this)};
	AddPrimitivesToTreeViewControl(tree, TreeItem);
	return TreeItem;
}

void EoDbGroup::BreakPolylines() {
	auto Database {AeSysDoc::GetDoc()->m_DatabasePtr};
	const OdDbBlockTableRecordPtr BlockTableRecord {Database->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	auto Position {GetHeadPosition()};
	while (Position != nullptr) {
		const auto PrimitivePosition {Position};
		const auto Primitive {GetNext(Position)};
		if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbPolyline)) != 0) {
			const auto Polyline {dynamic_cast<EoDbPolyline*>(Primitive)};
			OdGePoint3dArray Points;
			Polyline->GetAllPoints(Points);
			for (unsigned w = 0; w < Points.size() - 1; w++) {
				auto Line {EoDbLine::Create(BlockTableRecord, Points[w], Points[w + 1])};
				Line->setColorIndex(static_cast<unsigned short>(Primitive->ColorIndex()));
				Line->setLinetype(EoDbPrimitive::LinetypeObjectFromIndex(Primitive->LinetypeIndex()));
				CObList::InsertBefore(PrimitivePosition, EoDbLine::Create(Line));
			}
			if (Polyline->IsClosed()) {
				auto Line {EoDbLine::Create(BlockTableRecord, Points[Points.size() - 1], Points[0])};
				Line->setColorIndex(static_cast<unsigned short>(Primitive->ColorIndex()));
				Line->setLinetype(EoDbPrimitive::LinetypeObjectFromIndex(Primitive->LinetypeIndex()));
				CObList::InsertBefore(PrimitivePosition, EoDbLine::Create(Line));
			}
			this->RemoveAt(PrimitivePosition);
			delete Primitive;
		} else if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbBlockReference)) != 0) {
			EoDbBlock* Block;
			if (AeSysDoc::GetDoc()->LookupBlock(dynamic_cast<EoDbBlockReference*>(Primitive)->Name(), Block)) {
				Block->BreakPolylines();
			}
		}
	}
}

void EoDbGroup::BreakSegRefs() {
	int BlockReferences;
	do {
		BlockReferences = 0;
		auto Position {GetHeadPosition()};
		while (Position != nullptr) {
			const auto PrimitivePosition {Position};
			const auto Primitive {GetNext(Position)};
			if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbBlockReference)) != 0) {
				BlockReferences++;
				EoDbBlock* Block;
				if (AeSysDoc::GetDoc()->LookupBlock(dynamic_cast<EoDbBlockReference*>(Primitive)->Name(), Block)) {
					auto BlockCopy {new EoDbGroup(*Block)};
					auto BlockTransform {dynamic_cast<EoDbBlockReference*>(Primitive)->BlockTransformMatrix(Block->BasePoint())};
					BlockCopy->TransformBy(BlockTransform);
					this->InsertBefore(PrimitivePosition, BlockCopy);
					this->RemoveAt(PrimitivePosition);
					delete Primitive;
					BlockCopy->RemoveAll();
					delete BlockCopy;
				}
			}
		}
	} while (BlockReferences != 0);
}

void EoDbGroup::DeletePrimitivesAndRemoveAll() {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		const auto EntityObjectId {Primitive->EntityObjectId()};
		if (!EntityObjectId.isNull()) {
			OdDbEntityPtr Entity {EntityObjectId.safeOpenObject(OdDb::kForWrite)};
			Entity->erase();
		}
		delete Primitive;
	}
	RemoveAll();
}

void EoDbGroup::Display(AeSysView* view, CDC* deviceContext) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->Display(view, deviceContext);
	}
}

void EoDbGroup::Erase() const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		const auto EntityObjectId {Primitive->EntityObjectId()};
		if (!EntityObjectId.isNull()) {
			OdDbEntityPtr Entity {EntityObjectId.safeOpenObject(OdDb::kForWrite)};
			Entity->erase();
		}
	}
}

void EoDbGroup::UndoErase() const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		const auto EntityObjectId {Primitive->EntityObjectId()};
		if (!EntityObjectId.isNull()) {
			OdDbEntityPtr Entity {EntityObjectId.safeOpenObject(OdDb::kForWrite, true)};
			Entity->erase(false);
		}
	}
}

POSITION EoDbGroup::FindAndRemovePrimitive(EoDbPrimitive* primitive) {
	const auto PrimitivePosition {Find(primitive)};
	if (PrimitivePosition != nullptr) {
		RemoveAt(PrimitivePosition);
	}
	return PrimitivePosition;
}

EoDbPrimitive* EoDbGroup::GetAt(const POSITION position) {
	return dynamic_cast<EoDbPrimitive*>(CObList::GetAt(position));
}

int EoDbGroup::GetBlockReferenceCount(const CString& name) const {
	auto Count {0};
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbBlockReference)) != 0) {
			if (dynamic_cast<EoDbBlockReference*>(Primitive)->Name() == name) {
				Count++;
			}
		}
	}
	return Count;
}

void EoDbGroup::GetExtents_(AeSysView* view, OdGeExtents3d& extents) const {
	OdGeExtents3d GroupExtents;
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->GetExtents(view, GroupExtents);
	}
	if (GroupExtents.isValidExtents()) {
		GroupExtents.transformBy(view->ModelToWorldTransform());
		extents.addExt(GroupExtents);
	}
}

EoDbPoint* EoDbGroup::GetFirstDifferentPoint(EoDbPoint* pointPrimitive) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive != pointPrimitive && Primitive->IsKindOf(RUNTIME_CLASS(EoDbPoint)) != 0) {
			return dynamic_cast<EoDbPoint*>(Primitive);
		}
	}
	return nullptr;
}

int EoDbGroup::GetLinetypeIndexRefCount(const short linetypeIndex) const {
	auto Count {0};
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		if (GetNext(PrimitivePosition)->LinetypeIndex() == linetypeIndex) {
			Count++;
		}
	}
	return Count;
}

EoDbPrimitive* EoDbGroup::GetNext(POSITION& position) const {
	return (EoDbPrimitive*)CObList::GetNext(position);
}

void EoDbGroup::InsertBefore(const POSITION position, EoDbGroup* group) {
	auto PrimitivePosition {group->GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {group->GetNext(PrimitivePosition)};
		CObList::InsertBefore(position, Primitive);
	}
}

bool EoDbGroup::IsInView(AeSysView* view) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive != nullptr && Primitive->IsInView(view)) {
			return true;
		}
	}
	return false;
}

bool EoDbGroup::IsOn(const EoGePoint4d& point, AeSysView* view) const {
	OdGePoint3d Point;
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		if (GetNext(PrimitivePosition)->SelectUsingPoint(point, view, Point)) {
			return true;
		}
	}
	return false;
}

void EoDbGroup::ModifyColorIndex(const short colorIndex) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->SetColorIndex2(colorIndex);
	}
}

void EoDbGroup::ModifyLinetypeIndex(const short linetypeIndex) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->SetLinetypeIndex2(linetypeIndex);
	}
}

void EoDbGroup::ModifyNotes(const EoDbFontDefinition& fontDefinition, EoDbCharacterCellDefinition& characterCellDefinition, const int iAtt) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive != nullptr && Primitive->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0) {
			dynamic_cast<EoDbText*>(Primitive)->ModifyNotes(fontDefinition, characterCellDefinition, iAtt);
		}
	}
}

void EoDbGroup::PenTranslation(const unsigned numberOfColors, std::vector<int>& newColors, std::vector<int>& pCol) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		auto Primitive {GetNext(PrimitivePosition)};
		for (unsigned ColorIndex = 0; ColorIndex < numberOfColors; ColorIndex++) {
			if (Primitive->ColorIndex() == pCol.at(ColorIndex)) {
				Primitive->SetColorIndex2(static_cast<short>(newColors.at(ColorIndex)));
				break;
			}
		}
	}
}

// <tas="Is no longer working!"</tas>
void EoDbGroup::RemoveDuplicatePrimitives() {
	auto BasePosition {GetHeadPosition()};
	while (BasePosition != nullptr) {
		const auto BasePrimitive {GetNext(BasePosition)};
		auto TestPosition {BasePosition};
		while (TestPosition != nullptr) {
			const auto TestPositionSave {TestPosition};
			const auto TestPrimitive {GetNext(TestPosition)};
			if (BasePrimitive->IsEqualTo(TestPrimitive)) {
				RemoveAt(TestPositionSave);
				delete TestPrimitive;
			}
		}
	}
}

int EoDbGroup::RemoveEmptyNotesAndDelete() {
	auto Count {0};
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto posPrev {PrimitivePosition};
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0) {
			if (dynamic_cast<EoDbText*>(Primitive)->Text().GetLength() == 0) {
				RemoveAt(posPrev);
				delete Primitive;
				Count++;
			}
		}
	}
	return Count;
}

bool EoDbGroup::SelectUsingLineSeg(const EoGeLineSeg3d& lineSeg, AeSysView* view) const {
	OdGePoint3dArray Intersections;
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		if (GetNext(PrimitivePosition)->SelectUsingLineSeg(lineSeg, view, Intersections)) {
			return true;
		}
	}
	return false;
}

bool EoDbGroup::SelectUsingRectangle(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		if (GetNext(PrimitivePosition)->SelectUsingRectangle(lowerLeftCorner, upperRightCorner, view)) {
			return true;
		}
	}
	return false;
}

EoDbPrimitive* EoDbGroup::SelectControlPointBy(const EoGePoint4d& point, AeSysView* view, OdGePoint3d* ptCtrl) const {
	EoDbPrimitive* EngagedPrimitive {nullptr};
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive == sm_PrimitiveToIgnore) {
			continue;
		}
		const auto pt {Primitive->SelectAtControlPoint(view, point)};
		if (EoDbPrimitive::ControlPointIndex() != SIZE_T_MAX) {
			EngagedPrimitive = Primitive;
			EoGePoint4d ptView4(pt, 1.0);
			view->ModelViewTransformPoint(ptView4);
			*ptCtrl = ptView4.Convert3d();
		}
	}
	return EngagedPrimitive;
}

EoDbPrimitive* EoDbGroup::SelectPrimitiveUsingPoint(const EoGePoint4d& point, AeSysView* view, double& pickAperture, OdGePoint3d& pDetPt) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive->SelectUsingPoint(point, view, pDetPt)) {
			pickAperture = point.DistanceToPointXY(EoGePoint4d(pDetPt, 1.0));
			return Primitive;
		}
	}
	return nullptr;
}

void EoDbGroup::SetPrimitiveToIgnore(EoDbPrimitive* primitive) noexcept {
	sm_PrimitiveToIgnore = primitive;
}

void EoDbGroup::SortTextOnY() {
	int iT;
	auto Count {static_cast<int>(GetCount())};
	do {
		iT = 0;
		auto Position {GetHeadPosition()};
		for (auto i = 1; i < Count; i++) {
			auto pos1 {Position};
			const auto pPrim1 {GetNext(pos1)};
			auto pos2 {pos1};
			const auto pPrim2 {GetNext(pos2)};
			if (pPrim1->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0 && pPrim2->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0) {
				const auto dY1 {dynamic_cast<EoDbText*>(pPrim1)->Position().y};
				const auto dY2 {dynamic_cast<EoDbText*>(pPrim2)->Position().y};
				if (dY1 < dY2) {
					SetAt(Position, pPrim2);
					SetAt(pos1, pPrim1);
					iT = i;
				}
			} else if (pPrim1->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0 || pPrim2->IsKindOf(RUNTIME_CLASS(EoDbText)) != 0) {
				SetAt(Position, pPrim2);
				SetAt(pos1, pPrim1);
				iT = i;
			}
			Position = pos1;
		}
		Count = iT;
	} while (iT != 0);
}

void EoDbGroup::Square(AeSysView* view) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		const auto Primitive {GetNext(PrimitivePosition)};
		if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbLine)) != 0) {
			dynamic_cast<EoDbLine*>(Primitive)->Square(view);
		}
	}
}

void EoDbGroup::TransformBy(const EoGeMatrix3d& transformMatrix) const {
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		auto Primitive {GetNext(PrimitivePosition)};
		const auto EntityObjectId {Primitive->EntityObjectId()};
		if (!EntityObjectId.isNull()) {
			OdDbEntityPtr Entity {EntityObjectId.safeOpenObject(OdDb::kForWrite)};
			Entity->transformBy(transformMatrix);
		}
		Primitive->TransformBy(transformMatrix);
	}
}

void EoDbGroup::Write(EoDbFile& file) const {
	file.WriteUInt16(static_cast<unsigned short>(GetCount()));
	for (auto PrimitivePosition = GetHeadPosition(); PrimitivePosition != nullptr;) {
		GetNext(PrimitivePosition)->Write(file);
	}
}

void EoDbGroup::Write(CFile& file, unsigned char* buffer) const {
	// group flags
	buffer[0] = 0;
	// number of primitives in group
	*reinterpret_cast<short*>(& buffer[1]) = short(GetCount());
	auto PrimitivePosition {GetHeadPosition()};
	while (PrimitivePosition != nullptr) {
		GetNext(PrimitivePosition)->Write(file, buffer);
	}
}

std::pair<EoDbGroup*, OdDbGroupPtr> EoDbGroup::Create(OdDbDatabasePtr database) {
	OdDbDictionaryPtr GroupDictionary {database->getGroupDictionaryId().safeOpenObject(OdDb::kForWrite)};
	auto Group {OdDbGroup::createObject()}; // do not attempt to add entries to the newly created group before adding the group to the group dictionary.
	GroupDictionary->setAt(L"*", Group);
	Group->setSelectable(true);
	Group->setAnonymous();
	return {new EoDbGroup, Group};
}
