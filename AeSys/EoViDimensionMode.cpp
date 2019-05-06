#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "DbDimStyleTable.h"
#include "DbDimStyleTableRecord.h"

#include "DbAlignedDimension.h"

#include "EoDbDimension.h"

double DimensionModePickTolerance = .05;
OdGePoint3d PreviousDimensionPosition;
OdUInt16 PreviousDimensionCommand = 0;

OdGePoint3d ProjPtToLn(const OdGePoint3d& point) {
	const auto Document {AeSysDoc::GetDoc()};

	EoGeLineSeg3d ln;
	OdGePoint3d ptProj;

	double Relationship;

	POSITION GroupPosition = Document->GetFirstWorkLayerGroupPosition();
	while (GroupPosition != 0) {
		EoDbGroup* Group = Document->GetNextWorkLayerGroup(GroupPosition);

		POSITION PrimitivePosition = Group->GetHeadPosition();
		while (PrimitivePosition != 0) {
			EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

			if (Primitive->Is(EoDb::kLinePrimitive))
				dynamic_cast<EoDbLine*>(Primitive)->GetLine(ln);
			else if (Primitive->Is(EoDb::kDimensionPrimitive))
				ln = dynamic_cast<EoDbDimension*>(Primitive)->Line();
			else
				continue;

			if (ln.IsSelectedBy_xy(point, DimensionModePickTolerance, ptProj, Relationship))
				return (Relationship <= .5) ? ln.startPoint() : ln.endPoint();
		}
	}
	return (point);
}

void AeSysView::OnDimensionModeOptions(void) {
	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
	PreviousDimensionPosition = GetCursorPosition();
}

void AeSysView::OnDimensionModeArrow(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d CurrentPnt = GetCursorPosition();

	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
	EoGeLineSeg3d TestLine;
	POSITION GroupPosition = GetFirstVisibleGroupPosition();
	while (GroupPosition != 0) {
		EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);

		POSITION PrimitivePosition = Group->GetHeadPosition();
		while (PrimitivePosition != 0) {
			EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
			if (Primitive->Is(EoDb::kLinePrimitive)) {
				EoDbLine* LinePrimitive = dynamic_cast<EoDbLine*>(Primitive);
				LinePrimitive->GetLine(TestLine);
			} else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
				EoDbDimension* DimensionPrimitive = dynamic_cast<EoDbDimension*>(Primitive);
				TestLine = DimensionPrimitive->Line();
			} else {
				continue;
			}
			OdGePoint3d ptProj;
			double dRel;

			if (TestLine.IsSelectedBy_xy(CurrentPnt, DimensionModePickTolerance, ptProj, dRel)) {
				OdGePoint3d pt;

				EoDbGroup* NewGroup = new EoDbGroup;
				if (dRel <= .5) {
					GenerateLineEndItem(1, .1, TestLine.endPoint(), TestLine.startPoint(), NewGroup);
					pt = TestLine.startPoint();
				} else {
					GenerateLineEndItem(1, .1, TestLine.startPoint(), TestLine.endPoint(), NewGroup);
					pt = TestLine.endPoint();
				}
				Document->AddWorkLayerGroup(NewGroup);
				Document->UpdateGroupInAllViews(EoDb::kGroupSafe, NewGroup);

				SetCursorPosition(pt);
				PreviousDimensionPosition = pt;
				return;
			}
		}
	}
	PreviousDimensionPosition = CurrentPnt;
}

#include "DbAlignedDimension.h"
#include "DbRotatedDimension.h"

void AeSysView::OnDimensionModeLine() {
	auto CurrentPnt {GetCursorPosition()};
	RubberBandingDisable();
	if (PreviousDimensionCommand != ID_OP2) {
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP2);
		PreviousDimensionPosition = CurrentPnt;
	} else {
		CurrentPnt = SnapPointToAxis(PreviousDimensionPosition, CurrentPnt);
		if (PreviousDimensionPosition != CurrentPnt) {
			OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
			
			auto Line {EoDbLine::Create(BlockTableRecord, PreviousDimensionPosition, CurrentPnt)};
			Line->setColorIndex(1);
			Line->setLinetype(L"Continuous");

			auto Group {new EoDbGroup};
			Group->AddTail(EoDbLine::Create(Line));

			auto Document {GetDocument()};
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);
		}
		PreviousDimensionPosition = CurrentPnt;
	}
	RubberBandingStartAtEnable(CurrentPnt, Lines);
}

void AeSysView::OnDimensionModeDLine(void) {
	const auto CurrentPnt {GetCursorPosition()};
	if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
		RubberBandingDisable();
		if (PreviousDimensionPosition != CurrentPnt) {
			auto Group {new EoDbGroup};

			if (PreviousDimensionCommand == ID_OP4) {
				GenerateLineEndItem(1, .1, CurrentPnt, PreviousDimensionPosition, Group);
				ModeLineUnhighlightOp(PreviousDimensionCommand);
				PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
			}
			OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);

			auto AlignedDimension {EoDbDimension::Create(BlockTableRecord)};
			AlignedDimension->setXLine1Point(PreviousDimensionPosition);
			AlignedDimension->setXLine2Point(CurrentPnt);
			AlignedDimension->setDimLinePoint(CurrentPnt);
			AlignedDimension->measurement(); // initial compute of the measurement

			OdDbDimStyleTablePtr DimStyleTable = Database()->getDimStyleTableId().safeOpenObject(OdDb::kForRead);
			auto DimStyleRecord {DimStyleTable->getAt(L"EoStandard")};
			AlignedDimension->setDimensionStyle(DimStyleRecord);
			AlignedDimension->downgradeOpen();

			Group->AddTail(EoDbDimension::Create(AlignedDimension));

			GetDocument()->AddWorkLayerGroup(Group);
			GetDocument()->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionPosition = CurrentPnt;
		}
	} else {
		if (PreviousDimensionCommand != 0) {
			RubberBandingDisable();
			ModeLineUnhighlightOp(PreviousDimensionCommand);
		}
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
		PreviousDimensionPosition = CurrentPnt;
	}
	SetCursorPosition(CurrentPnt);
	RubberBandingStartAtEnable(CurrentPnt, Lines);
}

void AeSysView::OnDimensionModeDLine2(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d CurrentPnt = GetCursorPosition();
	if (PreviousDimensionCommand == 0) {
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
		PreviousDimensionPosition = CurrentPnt;
	} else if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
		RubberBandingDisable();
		if (PreviousDimensionPosition != CurrentPnt) {
			EoDbGroup* Group = new EoDbGroup;
			if (PreviousDimensionCommand == ID_OP4)
				GenerateLineEndItem(1, .1, CurrentPnt, PreviousDimensionPosition, Group);
			else {
				ModeLineUnhighlightOp(PreviousDimensionCommand);
				PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
			}
			OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);

			auto AlignedDimension {EoDbDimension::Create(BlockTableRecord)};
			AlignedDimension->setXLine1Point(PreviousDimensionPosition);
			AlignedDimension->setXLine2Point(CurrentPnt);
			AlignedDimension->setDimLinePoint(CurrentPnt);
			AlignedDimension->measurement(); // initial compute of the measurement

			OdDbDimStyleTablePtr DimStyleTable = Database()->getDimStyleTableId().safeOpenObject(OdDb::kForRead);
			auto DimStyleRecord {DimStyleTable->getAt(L"EoStandard")};
			AlignedDimension->setDimensionStyle(DimStyleRecord);
			AlignedDimension->downgradeOpen();

			Group->AddTail(EoDbDimension::Create(AlignedDimension));

			GenerateLineEndItem(1, .1, PreviousDimensionPosition, CurrentPnt, Group);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionPosition = CurrentPnt;
		} else
			theApp.AddModeInformationToMessageList();
	} else {
		// error finish prior op first
	}
	SetCursorPosition(CurrentPnt);
	RubberBandingStartAtEnable(CurrentPnt, Lines);
}
void AeSysView::OnDimensionModeExten(void) {
	auto Document {GetDocument()};
	auto CurrentPnt {GetCursorPosition()};
	if (PreviousDimensionCommand != ID_OP5) {
		RubberBandingDisable();
		PreviousDimensionPosition = ProjPtToLn(CurrentPnt);
		ModeLineUnhighlightOp(PreviousDimensionCommand);
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP5);
	} else {
		CurrentPnt = ProjPtToLn(CurrentPnt);
		if (PreviousDimensionPosition != CurrentPnt) {
			CurrentPnt = ProjectToward(CurrentPnt, PreviousDimensionPosition, -.1875);
			PreviousDimensionPosition = ProjectToward(PreviousDimensionPosition, CurrentPnt, .0625);
			OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
			auto Group {new EoDbGroup};

			auto Line {EoDbLine::Create(BlockTableRecord, PreviousDimensionPosition, CurrentPnt)};
			Line->setColorIndex(1);
			Line->setLinetype(L"Continuous");
			Group->AddTail(EoDbLine::Create(Line));

			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);
		}
		PreviousDimensionPosition = CurrentPnt;
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
}
void AeSysView::OnDimensionModeRadius(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d CurrentPnt = GetCursorPosition();

	if (SelectGroupAndPrimitive(CurrentPnt) != nullptr) {
		const OdGePoint3d ptEnd = DetPt();

		if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
			EoDbEllipse* pArc = dynamic_cast<EoDbEllipse*>(EngagedPrimitive());

			const OdGePoint3d ptBeg = pArc->Center();

			EoDbGroup* Group = new EoDbGroup;

			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex2(1);
			DimensionPrimitive->SetLinetypeIndex2(1);
			DimensionPrimitive->SetStartPoint(ptBeg);
			DimensionPrimitive->SetEndPoint(ptEnd);
			DimensionPrimitive->SetTextColorIndex(5);
			DimensionPrimitive->SetTextHorizontalAlignment(EoDb::kAlignCenter);
			DimensionPrimitive->SetTextVerticalAlignment(EoDb::kAlignMiddle);
			DimensionPrimitive->SetDefaultNote();
			DimensionPrimitive->SetText(L"R" + DimensionPrimitive->Text());
			DimensionPrimitive->SetDefaultNote();
			Group->AddTail(DimensionPrimitive);

			GenerateLineEndItem(1, .1, ptBeg, ptEnd, Group);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionPosition = ptEnd;
		}
	} else { // error arc not identified
		PreviousDimensionPosition = CurrentPnt;
	}
}
void AeSysView::OnDimensionModeDiameter() {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d CurrentPnt = GetCursorPosition();

	if (SelectGroupAndPrimitive(CurrentPnt) != nullptr) {
		const OdGePoint3d ptEnd = DetPt();

		if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
			EoDbEllipse* pArc = dynamic_cast<EoDbEllipse*>(EngagedPrimitive());

			const OdGePoint3d ptBeg = ProjectToward(ptEnd, pArc->Center(), 2. * pArc->MajorAxis().length());

			EoDbGroup* Group = new EoDbGroup;

			GenerateLineEndItem(1, .1, ptEnd, ptBeg, Group);

			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex2(1);
			DimensionPrimitive->SetLinetypeIndex2(1);
			DimensionPrimitive->SetStartPoint(ptBeg);
			DimensionPrimitive->SetEndPoint(ptEnd);
			DimensionPrimitive->SetTextColorIndex(5);
			DimensionPrimitive->SetTextHorizontalAlignment(EoDb::kAlignCenter);
			DimensionPrimitive->SetTextVerticalAlignment(EoDb::kAlignMiddle);
			DimensionPrimitive->SetDefaultNote();
			DimensionPrimitive->SetText(L"D" + DimensionPrimitive->Text());
			DimensionPrimitive->SetDefaultNote();
			Group->AddTail(DimensionPrimitive);

			GenerateLineEndItem(1, .1, ptBeg, ptEnd, Group);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionPosition = ptEnd;
		}
	} else {
		PreviousDimensionPosition = CurrentPnt;
	}
}
void AeSysView::OnDimensionModeAngle(void) {
	CDC* DeviceContext = GetDC();

	AeSysDoc* Document = GetDocument();
	const OdGePoint3d CurrentPnt = GetCursorPosition();

	static OdGePoint3d rProjPt[2];
	static OdGePoint3d CenterPoint;
	static int iLns;
	static EoGeLineSeg3d ln;

	if (PreviousDimensionCommand != ID_OP8) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);

		if (SelectLineBy(CurrentPnt) != 0) {
			EoDbLine* pLine = dynamic_cast<EoDbLine*>(EngagedPrimitive());

			rProjPt[0] = DetPt();
			pLine->GetLine(ln);
			PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
			theApp.AddStringToMessageList(L"Select the second line.");
			iLns = 1;
		}
	} else {
		if (iLns == 1) {
			if (SelectLineBy(CurrentPnt) != 0) {
				EoDbLine* pLine = dynamic_cast<EoDbLine*>(EngagedPrimitive());

				rProjPt[1] = DetPt();
				if (ln.intersectWith(pLine->Line(), CenterPoint)) {
					iLns++;
					theApp.AddStringToMessageList(L"Specify the location for the dimension arc.");
				}
			}
		} else {
			double Angle;

			const OdGeVector3d vCenterToProjPt = OdGeVector3d(rProjPt[0] - CenterPoint);
			const OdGeVector3d vCenterToCur = OdGeVector3d(CurrentPnt - CenterPoint);
			OdGeVector3d PlaneNormal = vCenterToProjPt.crossProduct(vCenterToCur);
			PlaneNormal.normalize();
			if (pFndSwpAngGivPlnAnd3Lns(PlaneNormal, rProjPt[0], CurrentPnt, rProjPt[1], CenterPoint, Angle)) {
				const double dRad = OdGeVector3d(CurrentPnt - CenterPoint).length();

				ln.set(ProjectToward(CenterPoint, rProjPt[0], dRad), ln.startPoint());
				ln.endPoint().rotateBy(Angle, PlaneNormal, CenterPoint);
				const OdGeVector3d MajorAxis = OdGeVector3d(ln.startPoint() - CenterPoint);
				OdGePoint3d ptRot = ln.startPoint();
				ptRot.rotateBy(HALF_PI, PlaneNormal, CenterPoint);
				const OdGeVector3d MinorAxis = OdGeVector3d(ptRot - CenterPoint);
				OdGePoint3d ptArrow = ln.startPoint();
				ptArrow.rotateBy(RADIAN, PlaneNormal, CenterPoint);
				EoDbGroup * Group = new EoDbGroup;
				// <tas> GenerateLineEndItem(1, .1, ptArrow, ln.startPoint(), Group);

				OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
				EoDbEllipse * Ellipse = EoDbEllipse::Create0(BlockTableRecord);
				Ellipse->SetTo2(CenterPoint, MajorAxis, MinorAxis, Angle);
				Ellipse->SetColorIndex2(1);
				Ellipse->SetLinetypeIndex2(1);
				Group->AddTail(Ellipse);

				ptArrow = ln.startPoint();
				ptArrow.rotateBy(Angle - RADIAN, PlaneNormal, CenterPoint);
				// <tas="This LineEndItem is wrong"</tas>
				// <tas> GenerateLineEndItem(1, .1, ptArrow, ln.endPoint(), Group);
				const int PrimitiveState = pstate.Save();

				EoDbFontDefinition FontDefinition = pstate.FontDefinition();
				FontDefinition.SetHorizontalAlignment(EoDb::kAlignCenter);
				FontDefinition.SetVerticalAlignment(EoDb::kAlignMiddle);
				pstate.SetFontDefinition(DeviceContext, FontDefinition);

				EoDbCharacterCellDefinition CharacterCellDefinition = pstate.CharacterCellDefinition();
				CharacterCellDefinition.SetRotationAngle(0.);
				CharacterCellDefinition.SetHeight(.1);
				pstate.SetCharacterCellDefinition(CharacterCellDefinition);

				const OdGePoint3d ptPvt = ProjectToward(CurrentPnt, CenterPoint, -.25);

				EoGeReferenceSystem ReferenceSystem(ptPvt, PlaneNormal, CharacterCellDefinition);

				OdDbTextPtr Text = EoDbText::Create(BlockTableRecord, ReferenceSystem.Origin(), (LPCWSTR) theApp.FormatAngle(Angle));

				Text->setNormal(PlaneNormal);
				Text->setRotation(ReferenceSystem.Rotation());
				Text->setHeight(ReferenceSystem.YDirection().length());
				Text->setAlignmentPoint(ReferenceSystem.Origin());
				Text->setHorizontalMode(OdDb::kTextCenter);
				Text->setVerticalMode(OdDb::kTextVertMid);

				Group->AddTail(EoDbText::Create(Text));

				Document->AddWorkLayerGroup(Group);
				Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);
				pstate.Restore(DeviceContext, PrimitiveState);
			}
			ModeLineUnhighlightOp(PreviousDimensionCommand);
			theApp.AddModeInformationToMessageList();
		}
	}
}
void AeSysView::OnDimensionModeConvert(void) {
	const auto CurrentPnt {GetCursorPosition()};
	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}

	EoDbGroup* Group;
	EoDbPrimitive* Primitive;
	OdGePoint3d ptProj;

	POSITION posPrimCur;

	EoGePoint4d ptView(CurrentPnt, 1.);
	ModelViewTransformPoint(ptView);

	POSITION GroupPosition = GetFirstVisibleGroupPosition();
	while (GroupPosition != 0) {
		Group = GetNextVisibleGroup(GroupPosition);

		POSITION PrimitivePosition = Group->GetHeadPosition();
		while (PrimitivePosition != 0) {
			posPrimCur = PrimitivePosition;
			Primitive = Group->GetNext(PrimitivePosition);
			if (Primitive->SelectBy(ptView, this, ptProj)) {
				if (Primitive->Is(EoDb::kLinePrimitive)) {
					auto LinePrimitive {dynamic_cast<EoDbLine*>(Primitive)};
					auto DimensionPrimitive {new EoDbDimension()};
					DimensionPrimitive->SetColorIndex2(LinePrimitive->ColorIndex());
					DimensionPrimitive->SetLinetypeIndex2(LinePrimitive->LinetypeIndex());
					DimensionPrimitive->SetStartPoint(LinePrimitive->StartPoint());
					DimensionPrimitive->SetEndPoint(LinePrimitive->EndPoint());
					DimensionPrimitive->SetFontDefinition(pstate.FontDefinition());

					DimensionPrimitive->SetTextColorIndex(5);
					DimensionPrimitive->SetTextHorizontalAlignment(EoDb::kAlignCenter);
					DimensionPrimitive->SetTextVerticalAlignment(EoDb::kAlignMiddle);
					DimensionPrimitive->SetDefaultNote();
					Group->InsertAfter(posPrimCur, DimensionPrimitive);
					Group->RemoveAt(posPrimCur);
					delete Primitive;
					PreviousDimensionPosition = ptProj;
					return;
				} else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
					auto DimensionPrimitive {dynamic_cast<EoDbDimension*>(Primitive)};
					EoGeReferenceSystem ReferenceSystem;
					ReferenceSystem = DimensionPrimitive->ReferenceSystem();
					OdGeVector3d PlaneNormal;
					ReferenceSystem.GetUnitNormal(PlaneNormal);

					OdDbBlockTableRecordPtr BlockTableRecord = Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite);

					auto Line {EoDbLine::Create(BlockTableRecord, DimensionPrimitive->Line().startPoint(), DimensionPrimitive->Line().endPoint())};
					Line->setColorIndex(DimensionPrimitive->ColorIndex());
					Line->setLinetype(EoDbPrimitive::LinetypeObjectFromIndex(DimensionPrimitive->LinetypeIndex()));
					auto LinePrimitive {EoDbLine::Create(Line)};

					auto Text {EoDbText::Create(BlockTableRecord, ReferenceSystem.Origin(), (LPCWSTR) DimensionPrimitive->Text())};

					Text->setNormal(PlaneNormal);
					Text->setRotation(ReferenceSystem.Rotation());
					Text->setHeight(ReferenceSystem.YDirection().length());
					Text->setAlignmentPoint(ReferenceSystem.Origin());
					Text->setHorizontalMode(EoDbText::ConvertHorizontalMode(DimensionPrimitive->FontDef().HorizontalAlignment()));
					Text->setVerticalMode(EoDbText::ConvertVerticalMode(DimensionPrimitive->FontDef().VerticalAlignment()));
					Text->setColorIndex(DimensionPrimitive->TextColorIndex());

					auto TextPrimitive = EoDbText::Create(Text);

					Group->InsertAfter(posPrimCur, LinePrimitive);
					Group->InsertAfter(posPrimCur, TextPrimitive);
					Group->RemoveAt(posPrimCur);
					delete Primitive;
					PreviousDimensionPosition = ptProj;
					return;
				}
			}
		}
	}
	PreviousDimensionPosition = CurrentPnt;
}
void AeSysView::OnDimensionModeReturn(void) {
	const OdGePoint3d CurrentPnt = GetCursorPosition();
	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
	PreviousDimensionPosition = CurrentPnt;
}
void AeSysView::OnDimensionModeEscape(void) {
	const OdGePoint3d CurrentPnt = GetCursorPosition();

	RubberBandingDisable();
	ModeLineUnhighlightOp(PreviousDimensionCommand);
}
