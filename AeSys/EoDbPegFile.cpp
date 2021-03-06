#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include <DbLayerTable.h>
#include <DbBlockTable.h>
#include <DbBlockTableRecord.h>
#include <DbLinetypeTable.h>
#include <DbLinetypeTableRecord.h>
#include "EoDbPegFile.h"

EoDbPegFile::EoDbPegFile(OdDbDatabasePtr database)
	: EoDbFile(database) {}

void EoDbPegFile::Load(AeSysDoc* document) {
	try {
		ReadHeaderSection(document);
		ReadTablesSection(document);
		ReadBlocksSection(document);
		ReadGroupsSection(document);
	} catch (const wchar_t* Message) {
		MessageBoxW(nullptr, Message, L"EoDbPegFile", MB_ICONWARNING | MB_OK);
	} catch (const OdError& Error) {
		theApp.ErrorMessageBox(L"", Error);
	}
}

void EoDbPegFile::ReadHeaderSection(AeSysDoc* /*document*/) {
	if (ReadUInt16() != kHeaderSection) {
		throw L"Exception ReadHeaderSection: Expecting sentinel kHeaderSection.";
	}
	// <tas="All database settings can be set here ??"</tas>
	m_Database->setPDMODE(64); // Point display mode - square
	m_Database->setPDSIZE(.015625);
	if (ReadUInt16() != kEndOfSection) {
		throw L"Exception ReadHeaderSection: Expecting sentinel kEndOfSection.";
	}
}

void EoDbPegFile::ReadTablesSection(AeSysDoc* document) {
	if (ReadUInt16() != kTablesSection) {
		throw L"Exception ReadTablesSection: Expecting sentinel kTablesSection.";
	}
	ReadViewportTable(document);
	ReadLinetypesTable();
	ReadLayerTable(document);
	if (ReadUInt16() != kEndOfSection) {
		throw L"Exception ReadTablesSection: Expecting sentinel kEndOfSection.";
	}
}

void EoDbPegFile::ReadViewportTable(AeSysDoc* /*document*/) {
	if (ReadUInt16() != kViewPortTable) {
		throw L"Exception ReadViewportTable: Expecting sentinel kViewPortTable.";
	}
	ReadUInt16();
	if (ReadUInt16() != kEndOfTable) {
		throw L"Exception ReadViewportTable: Expecting sentinel kEndOfTable.";
	}
}

void EoDbPegFile::ReadLinetypesTable() {
	if (ReadUInt16() != kLinetypeTable) {
		throw L"Exception ReadLinetypesTable: Expecting sentinel kLinetypeTable.";
	}
	OdDbLinetypeTablePtr Linetypes {m_Database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite)};
	const auto NumberOfLinetypes {ReadUInt16()};
	const auto DashLength {new double[32]};
	for (unsigned LinetypeIndex = 0; LinetypeIndex < NumberOfLinetypes; LinetypeIndex++) {
		OdString Name;
		ReadString(Name); /* unsigned short Flags = */
		ReadUInt16();
		OdString Comments;
		ReadString(Comments);
		const auto NumberOfDashes {ReadUInt16()};
		const auto PatternLength {ReadDouble()};
		for (auto DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
			DashLength[DashIndex] = ReadDouble();
		}
		if (Linetypes->getAt(Name).isNull()) {
			auto Linetype = OdDbLinetypeTableRecord::createObject();
			Linetype->setName(Name);
			Linetype->setComments(Comments);
			Linetype->setNumDashes(NumberOfDashes);
			Linetype->setPatternLength(PatternLength);
			if (NumberOfDashes > 0) {
				for (auto DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
					Linetype->setDashLengthAt(DashIndex, DashLength[DashIndex]);
					Linetype->setShapeStyleAt(DashIndex, OdDbObjectId::kNull);
					Linetype->setShapeNumberAt(DashIndex, 0);
					Linetype->setTextAt(DashIndex, L" ");
					Linetype->setShapeScaleAt(DashIndex, 1.0);
					Linetype->setShapeOffsetAt(DashIndex, OdGeVector2d(0.0, 0.0));
					Linetype->setShapeRotationAt(DashIndex, 0.0);
					Linetype->setShapeIsUcsOrientedAt(DashIndex, false);
				}
			}
			Linetypes->add(Linetype);
		}
	}
	delete [] DashLength;
	if (ReadUInt16() != kEndOfTable) {
		throw L"Exception ReadLinetypesTable: Expecting sentinel kEndOfTable.";
	}
}

void EoDbPegFile::ReadLayerTable(AeSysDoc* document) {
	if (ReadUInt16() != kLayerTable) {
		throw L"Exception ReadLayerTable: Expecting sentinel kLayerTable.";
	}
	auto Layers {document->LayerTable(OdDb::kForWrite)};
	const auto NumberOfLayers {ReadUInt16()};
	for (unsigned LayerIndex = 0; LayerIndex < NumberOfLayers; LayerIndex++) {
		OdString Name;
		ReadString(Name); /* unsigned short TracingFlags = */
		ReadUInt16();
		auto StateFlags {ReadUInt16()};
		StateFlags |= EoDbLayer::kIsResident;
		if ((StateFlags & EoDbLayer::kIsInternal) != EoDbLayer::kIsInternal) {
			if (Name.find('.') == - 1) {
				Name += L".jb1";
			}
		}
		const auto ColorIndex {ReadInt16()};
		OdString LinetypeName;
		ReadString(LinetypeName);
		EoDbLayer* Layer;
		OdDbLayerTableRecordPtr LayerTableRecord;
		if (Layers->has(Name)) { // should be the default layer (0) on a load
			LayerTableRecord = Layers->getAt(Name).safeOpenObject(OdDb::kForWrite);
			Layer = document->GetLayerAt(Name);
		} else {
			LayerTableRecord = OdDbLayerTableRecord::createObject();
			Layer = new EoDbLayer(LayerTableRecord);
			LayerTableRecord->setName(Name);
			LayerTableRecord = document->AddLayerTo(Layers, Layer).safeOpenObject(OdDb::kForWrite);
		}
		Layer->SetStateFlags(StateFlags);
		if ((StateFlags & EoDbLayer::kIsCurrent) == EoDbLayer::kIsCurrent) {
			document->SetCurrentLayer(LayerTableRecord);
		}
		if ((StateFlags & EoDbLayer::kIsLocked) == EoDbLayer::kIsLocked) {
			Layer->SetIsLocked(true);
		}
		Layer->SetColorIndex(ColorIndex);
		OdDbObjectId Linetype;
		if (LinetypeName.iCompare(L"Continuous") == 0) {
			Linetype = m_Database->getLinetypeContinuousId();
		} else {
			OdDbLinetypeTablePtr Linetypes {m_Database->getLinetypeTableId().safeOpenObject(OdDb::kForRead)};
			Linetype = Linetypes->getAt(LinetypeName);
		}
		Layer->SetLinetype(Linetype);
	}
	if (ReadUInt16() != kEndOfTable) {
		throw L"Exception ReadLayerTable: Expecting sentinel kEndOfTable.";
	}
}

void EoDbPegFile::ReadBlocksSection(AeSysDoc* document) {
	if (ReadUInt16() != kBlocksSection) {
		throw L"Exception ReadBlocksSection: Expecting sentinel kBlocksSection.";
	}
	OdDbBlockTablePtr BlockTable {m_Database->getBlockTableId().safeOpenObject(OdDb::kForWrite)};
	OdString Name;
	const OdString PathName;
	const auto NumberOfBlocks {ReadUInt16()};
	for (unsigned BlockIndex = 0; BlockIndex < NumberOfBlocks; BlockIndex++) {
		const auto NumberOfPrimitives {ReadUInt16()};
		ReadString(Name);
		const auto BlockTypeFlags {ReadUInt16()};
		const auto BasePoint {ReadPoint3d()};
		auto Block {new EoDbBlock(BlockTypeFlags, BasePoint, PathName)};
		document->InsertBlock(Name, Block);
		OdDbBlockTableRecordPtr BlockTableRecord {m_Database->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
		if (BlockTable->getAt(Name).isNull()) {
			BlockTableRecord = OdDbBlockTableRecord::createObject();
			BlockTableRecord->setName(Name);
			BlockTableRecord->setOrigin(BasePoint);
			BlockTableRecord->setPathName(PathName);
			BlockTable->add(BlockTableRecord);
		}
		const auto LayoutBlock {BlockTableRecord->isLayout()};
		for (unsigned PrimitiveIndex = 0; PrimitiveIndex < NumberOfPrimitives; PrimitiveIndex++) {
			const auto Primitive {ReadPrimitive(BlockTableRecord)};
			Block->AddTail(Primitive);
		}
	}
	if (ReadUInt16() != kEndOfSection) {
		throw L"Exception ReadBlocksSection: Expecting sentinel kEndOfSection.";
	}
}

void EoDbPegFile::ReadGroupsSection(AeSysDoc* document) {
	if (ReadUInt16() != kGroupsSection) {
		throw L"Exception ReadGroupsSection: Expecting sentinel kGroupsSection.";
	}
	const OdDbBlockTableRecordPtr ModelSpaceBlock {m_Database->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	const auto CurrentLayerObjectId {m_Database->getCLAYER()};
	auto Layers {document->LayerTable(OdDb::kForRead)};
	const auto NumberOfLayers {static_cast<int>(ReadUInt16())};
	for (auto LayerIndex = 0; LayerIndex < NumberOfLayers; LayerIndex++) {
		auto Layer {document->GetLayerAt(LayerIndex)};
		if (Layer == nullptr) {
			return;
		}
		auto LayerName {Layer->Name()};
		const auto LayerObjectId {Layers->getAt(LayerName)};
		m_Database->setCLAYER(LayerObjectId);
		const auto NumberOfGroups {ReadUInt16()};
		if (Layer->IsInternal()) {
			for (unsigned GroupIndex = 0; GroupIndex < NumberOfGroups; GroupIndex++) {
				const auto NumberOfPrimitives {ReadUInt16()};
				auto Group {new EoDbGroup};
				for (unsigned PrimitiveIndex = 0; PrimitiveIndex < NumberOfPrimitives; PrimitiveIndex++) {
					const auto Primitive {ReadPrimitive(ModelSpaceBlock)};
					Group->AddTail(Primitive);
				}
				Layer->AddTail(Group);
			}
		} else {
			OdString PathName {GetFilePath().GetString()};
			PathName.replace(GetFileName(), Layer->Name());
			document->TracingLoadLayer(PathName, Layer);
		}
	}
	m_Database->setCLAYER(CurrentLayerObjectId);
	if (ReadUInt16() != kEndOfSection) {
		throw L"Exception ReadGroupsSection: Expecting sentinel kEndOfSection.";
	}
}

void EoDbPegFile::Unload(AeSysDoc* document) {
	CFile::SetLength(0);
	SeekToBegin();
	WriteHeaderSection(document);
	WriteTablesSection(document);
	WriteBlocksSection(document);
	WriteEntitiesSection(document);
	WriteString(OdString(L"EOF"));
	CFile::Flush();
}

void EoDbPegFile::WriteHeaderSection(AeSysDoc* /*document*/) {
	WriteUInt16(kHeaderSection);

	// header variable items go here
	WriteUInt16(kEndOfSection);
}

void EoDbPegFile::WriteTablesSection(AeSysDoc* document) {
	WriteUInt16(kTablesSection);
	WriteVPortTable(document);
	WriteLinetypeTable(document);
	WriteLayerTable(document);
	WriteUInt16(kEndOfSection);
}

void EoDbPegFile::WriteVPortTable(AeSysDoc* /*document*/) {
	WriteUInt16(kViewPortTable);
	WriteUInt16(0);
	WriteUInt16(kEndOfTable);
}

void EoDbPegFile::WriteLinetypeTable(AeSysDoc* /*document*/) {
	WriteUInt16(kLinetypeTable);
	OdDbLinetypeTablePtr Linetypes {m_Database->getLinetypeTableId().safeOpenObject(OdDb::kForRead)};
	auto Iterator {Linetypes->newIterator()};
	unsigned short NumberOfLinetypes = 0;
	for (Iterator->start(); !Iterator->done(); Iterator->step()) {
		NumberOfLinetypes++;
	}
	WriteUInt16(NumberOfLinetypes);
	Iterator = Linetypes->newIterator();
	for (Iterator->start(); !Iterator->done(); Iterator->step()) {
		OdDbLinetypeTableRecordPtr Linetype {Iterator->getRecordId().safeOpenObject(OdDb::kForRead)};
		WriteString(Linetype->getName());
		WriteUInt16(0);
		WriteString(Linetype->comments());
		const auto DefinitionLength {gsl::narrow_cast<unsigned short>(Linetype->numDashes())};
		WriteUInt16(DefinitionLength);
		const auto PatternLength {Linetype->patternLength()};
		WriteDouble(PatternLength);
		for (auto DashIndex = 0; DashIndex < DefinitionLength; DashIndex++) {
			WriteDouble(Linetype->dashLengthAt(DashIndex));
		}
	}
	WriteUInt16(kEndOfTable);
}

void EoDbPegFile::WriteLayerTable(AeSysDoc* document) {
	auto NumberOfLayers {document->GetLayerTableSize()};
	WriteUInt16(kLayerTable);
	const auto SavedFilePosition {static_cast<long long>(CFile::GetPosition())};
	WriteUInt16(static_cast<unsigned short>(NumberOfLayers));
	for (auto LayerIndex = 0; LayerIndex < document->GetLayerTableSize(); LayerIndex++) {
		auto Layer {document->GetLayerAt(LayerIndex)};
		if (Layer->IsResident()) {
			WriteString(Layer->Name());
			WriteUInt16(static_cast<unsigned short>(Layer->StateFlags() & 0x003c)); // used to be separate set of state flags for tracings (only used bits 3-6)
			WriteUInt16(Layer->StateFlags());
			WriteInt16(Layer->ColorIndex());
			WriteString(Layer->LinetypeName());
		} else {
			NumberOfLayers--;
		}
	}
	WriteUInt16(kEndOfTable);
	if (NumberOfLayers != document->GetLayerTableSize()) {
		const auto CurrentFilePosition {static_cast<long long>(CFile::GetPosition())};
		CFile::Seek(SavedFilePosition, begin);
		WriteUInt16(static_cast<unsigned short>(NumberOfLayers));
		CFile::Seek(CurrentFilePosition, begin);
	}
}

void EoDbPegFile::WriteBlocksSection(AeSysDoc* document) {
	WriteUInt16(kBlocksSection);
	const auto NumberOfBlocks {document->BlockTableSize()};
	WriteUInt16(NumberOfBlocks);
	CString Name;
	EoDbBlock* Block;
	auto Position {document->GetFirstBlockPosition()};
	while (Position != nullptr) {
		document->GetNextBlock(Position, Name, Block);
		const auto SavedFilePosition {static_cast<long long>(CFile::GetPosition())};
		WriteUInt16(0);
		unsigned short NumberOfPrimitives {0};
		WriteString(Name);
		WriteUInt16(Block->GetTypeFlags());
		WritePoint3d(Block->BasePoint());
		auto PrimitivePosition {Block->GetHeadPosition()};
		while (PrimitivePosition != nullptr) {
			const auto Primitive {Block->GetNext(PrimitivePosition)};
			if (Primitive->Write(*this)) {
				NumberOfPrimitives++;
			}
		}
		const auto CurrentFilePosition {static_cast<long long>(CFile::GetPosition())};
		CFile::Seek(SavedFilePosition, begin);
		WriteUInt16(NumberOfPrimitives);
		CFile::Seek(CurrentFilePosition, begin);
	}
	WriteUInt16(kEndOfSection);
}

void EoDbPegFile::WriteEntitiesSection(AeSysDoc* document) {
	WriteUInt16(kGroupsSection);
	const auto NumberOfLayers {document->GetLayerTableSize()};
	WriteUInt16(static_cast<unsigned short>(NumberOfLayers));
	for (auto LayerIndex = 0; LayerIndex < NumberOfLayers; LayerIndex++) {
		auto Layer {document->GetLayerAt(LayerIndex)};
		if (Layer->IsInternal()) {
			WriteUInt16(static_cast<unsigned short>(Layer->GetCount()));
			auto Position {Layer->GetHeadPosition()};
			while (Position != nullptr) {
				auto Group {Layer->GetNext(Position)};
				Group->Write(*this);
			}
		} else {
			WriteUInt16(0);
		}
	}
	WriteUInt16(kEndOfSection);
}
