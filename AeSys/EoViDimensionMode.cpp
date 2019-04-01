#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

double DimensionModePickTolerance = .05;
OdGePoint3d PreviousDimensionCursorPosition;
OdUInt16 PreviousDimensionCommand = 0;

OdGePoint3d ProjPtToLn(const OdGePoint3d& point) {
	AeSysDoc* Document = AeSysDoc::GetDoc();

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
				static_cast<EoDbLine*>(Primitive)->GetLine(ln);
			else if (Primitive->Is(EoDb::kDimensionPrimitive))
				ln = static_cast<EoDbDimension*>(Primitive)->Line();
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
	PreviousDimensionCursorPosition = GetCursorPosition();
}

void AeSysView::OnDimensionModeArrow(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();

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
				EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);
				LinePrimitive->GetLine(TestLine);
			}
			else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
				EoDbDimension* DimensionPrimitive = static_cast<EoDbDimension*>(Primitive);
				TestLine = DimensionPrimitive->Line();
			}
			else {
				continue;
			}
			OdGePoint3d ptProj;
			double dRel;

			if (TestLine.IsSelectedBy_xy(ptCur, DimensionModePickTolerance, ptProj, dRel)) {
				OdGePoint3d pt;

				EoDbGroup* NewGroup = new EoDbGroup;
				if (dRel <= .5) {
					GenerateLineEndItem(1, .1, TestLine.endPoint(), TestLine.startPoint(), NewGroup);
					pt = TestLine.startPoint();
				}
				else {
					GenerateLineEndItem(1, .1, TestLine.startPoint(), TestLine.endPoint(), NewGroup);
					pt = TestLine.endPoint();
				}
				Document->AddWorkLayerGroup(NewGroup);
				Document->UpdateGroupInAllViews(EoDb::kGroupSafe, NewGroup);

				SetCursorPosition(pt);
				PreviousDimensionCursorPosition = pt;
				return;
			}
		}
	}
	PreviousDimensionCursorPosition = ptCur;
}

void AeSysView::OnDimensionModeLine(void) {
	AeSysDoc* Document = GetDocument();
	OdGePoint3d ptCur = GetCursorPosition();
	RubberBandingDisable();
	if (PreviousDimensionCommand != ID_OP2) {
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP2);
		PreviousDimensionCursorPosition = ptCur;
	}
	else {
		ptCur = SnapPointToAxis(PreviousDimensionCursorPosition, ptCur);
		if (PreviousDimensionCursorPosition != ptCur) {
			EoDbLine* Line = EoDbLine::Create(PreviousDimensionCursorPosition, ptCur);
			Line->SetColorIndex(1);
			Line->SetLinetypeIndex(1);
			EoDbGroup* Group = new EoDbGroup;
			Group->AddTail(Line);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);
		}
		PreviousDimensionCursorPosition = ptCur;
	}
	RubberBandingStartAtEnable(ptCur, Lines);
}
void AeSysView::OnDimensionModeDLine(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();
	if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
		RubberBandingDisable();
		if (PreviousDimensionCursorPosition != ptCur) {
			EoDbGroup* Group = new EoDbGroup;

			if (PreviousDimensionCommand == ID_OP4) {
				GenerateLineEndItem(1, .1, ptCur, PreviousDimensionCursorPosition, Group);
				ModeLineUnhighlightOp(PreviousDimensionCommand);
				PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
			}
			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex(1);
			DimensionPrimitive->SetLinetypeIndex(1);
			DimensionPrimitive->SetStartPoint(PreviousDimensionCursorPosition);
			DimensionPrimitive->SetEndPoint(ptCur);
			DimensionPrimitive->SetTextColorIndex(5);
			DimensionPrimitive->SetTextHorizontalAlignment(EoDb::kAlignCenter);
			DimensionPrimitive->SetTextVerticalAlignment(EoDb::kAlignMiddle);
			DimensionPrimitive->SetDefaultNote();
			Group->AddTail(DimensionPrimitive);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionCursorPosition = ptCur;
		}
	}
	else {
		if (PreviousDimensionCommand != 0) {
			RubberBandingDisable();
			ModeLineUnhighlightOp(PreviousDimensionCommand);
		}
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
		PreviousDimensionCursorPosition = ptCur;
	}
	SetCursorPosition(ptCur);
	RubberBandingStartAtEnable(ptCur, Lines);
}
void AeSysView::OnDimensionModeDLine2(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();
	if (PreviousDimensionCommand == 0) {
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
		PreviousDimensionCursorPosition = ptCur;
	}
	else if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
		RubberBandingDisable();
		if (PreviousDimensionCursorPosition != ptCur) {
			EoDbGroup* Group = new EoDbGroup;
			if (PreviousDimensionCommand == ID_OP4)
				GenerateLineEndItem(1, .1, ptCur, PreviousDimensionCursorPosition, Group);
			else {
				ModeLineUnhighlightOp(PreviousDimensionCommand);
				PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
			}
			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex(1);
			DimensionPrimitive->SetLinetypeIndex(1);
			DimensionPrimitive->SetStartPoint(PreviousDimensionCursorPosition);
			DimensionPrimitive->SetEndPoint(ptCur);
			DimensionPrimitive->SetTextColorIndex(5);
			DimensionPrimitive->SetTextHorizontalAlignment(EoDb::kAlignCenter);
			DimensionPrimitive->SetTextVerticalAlignment(EoDb::kAlignMiddle);
			DimensionPrimitive->SetDefaultNote();
			Group->AddTail(DimensionPrimitive);
			GenerateLineEndItem(1, .1, PreviousDimensionCursorPosition, ptCur, Group);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);

			PreviousDimensionCursorPosition = ptCur;
		}
		else
			theApp.AddModeInformationToMessageList();
	}
	else {
		// error finish prior op first
	}
	SetCursorPosition(ptCur);
	RubberBandingStartAtEnable(ptCur, Lines);
}
void AeSysView::OnDimensionModeExten(void) {
	AeSysDoc* Document = GetDocument();
	OdGePoint3d ptCur = GetCursorPosition();
	if (PreviousDimensionCommand != ID_OP5) {
		RubberBandingDisable();
		PreviousDimensionCursorPosition = ProjPtToLn(ptCur);
		ModeLineUnhighlightOp(PreviousDimensionCommand);
		PreviousDimensionCommand = ModeLineHighlightOp(ID_OP5);
	}
	else {
		ptCur = ProjPtToLn(ptCur);
		if (PreviousDimensionCursorPosition != ptCur) {
			ptCur = ProjectToward(ptCur, PreviousDimensionCursorPosition, - .1875);
			PreviousDimensionCursorPosition = ProjectToward(PreviousDimensionCursorPosition, ptCur, .0625);
			EoDbLine* Line = EoDbLine::Create(PreviousDimensionCursorPosition, ptCur);
			Line->SetColorIndex(1);
			Line->SetLinetypeIndex(1);
			EoDbGroup* Group = new EoDbGroup;
			Group->AddTail(Line);
			Document->AddWorkLayerGroup(Group);
			Document->UpdateGroupInAllViews(EoDb::kGroupSafe, Group);
		}
		PreviousDimensionCursorPosition = ptCur;
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
}
void AeSysView::OnDimensionModeRadius(void) {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();

	if (SelectGroupAndPrimitive(ptCur) != 0) {
		const OdGePoint3d ptEnd = DetPt();

		if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
			EoDbEllipse* pArc = static_cast<EoDbEllipse*>(EngagedPrimitive());

			const OdGePoint3d ptBeg = pArc->Center();

			EoDbGroup* Group = new EoDbGroup;

			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex(1);
			DimensionPrimitive->SetLinetypeIndex(1);
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

			PreviousDimensionCursorPosition = ptEnd;
		}
	}
	else { // error arc not identified
		PreviousDimensionCursorPosition = ptCur;
	}
}
void AeSysView::OnDimensionModeDiameter() {
	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();

	if (SelectGroupAndPrimitive(ptCur) != 0) {
		const OdGePoint3d ptEnd = DetPt();

		if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
			EoDbEllipse* pArc = static_cast<EoDbEllipse*>(EngagedPrimitive());

			const OdGePoint3d ptBeg = ProjectToward(ptEnd, pArc->Center(), 2. * pArc->MajorAxis().length());

			EoDbGroup* Group = new EoDbGroup;

			GenerateLineEndItem(1, .1, ptEnd, ptBeg, Group);

			EoDbDimension* DimensionPrimitive = new EoDbDimension();
			DimensionPrimitive->SetColorIndex(1);
			DimensionPrimitive->SetLinetypeIndex(1);
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

			PreviousDimensionCursorPosition = ptEnd;
		}
	}
	else {
		PreviousDimensionCursorPosition = ptCur;
	}
}
void AeSysView::OnDimensionModeAngle(void) {
	CDC* DeviceContext = GetDC();

	AeSysDoc* Document = GetDocument();
	const OdGePoint3d ptCur = GetCursorPosition();

	static OdGePoint3d rProjPt[2];
	static OdGePoint3d CenterPoint;
	static int iLns;
	static EoGeLineSeg3d ln;

	if (PreviousDimensionCommand != ID_OP8) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);

		if (SelectLineBy(ptCur) != 0) {
			EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

			rProjPt[0] = DetPt();
			pLine->GetLine(ln);
			PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
			theApp.AddStringToMessageList(L"Select the second line.");
			iLns = 1;
		}
	}
	else {
		if (iLns == 1) {
			if (SelectLineBy(ptCur) != 0) {
				EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

				rProjPt[1] = DetPt();
				if (ln.intersectWith(pLine->Line(), CenterPoint)) {
					iLns++;
					theApp.AddStringToMessageList(L"Specify the location for the dimension arc.");
				}
			}
		}
		else {
			double Angle;

			const OdGeVector3d vCenterToProjPt = OdGeVector3d(rProjPt[0] - CenterPoint);
			const OdGeVector3d vCenterToCur = OdGeVector3d(ptCur - CenterPoint);
			OdGeVector3d PlaneNormal = vCenterToProjPt.crossProduct(vCenterToCur);
			PlaneNormal.normalize();
			if (pFndSwpAngGivPlnAnd3Lns(PlaneNormal, rProjPt[0], ptCur, rProjPt[1], CenterPoint, Angle)) {
				const double dRad = OdGeVector3d(ptCur - CenterPoint).length();

				ln.set(ProjectToward(CenterPoint, rProjPt[0], dRad), ln.startPoint());
				ln.endPoint().rotateBy(Angle, PlaneNormal, CenterPoint);
				const OdGeVector3d MajorAxis = OdGeVector3d(ln.startPoint() - CenterPoint);
				OdGePoint3d ptRot = ln.startPoint();
				ptRot.rotateBy(HALF_PI, PlaneNormal, CenterPoint);
				const OdGeVector3d MinorAxis = OdGeVector3d(ptRot - CenterPoint);
				OdGePoint3d ptArrow = ln.startPoint();
				ptArrow.rotateBy(RADIAN, PlaneNormal, CenterPoint);
				EoDbGroup* Group = new EoDbGroup;
				GenerateLineEndItem(1, .1, ptArrow, ln.startPoint(), Group);

				EoDbEllipse* Ellipse = EoDbEllipse::Create(Database());
				Ellipse->SetTo(CenterPoint, MajorAxis, MinorAxis, Angle);
				Ellipse->SetColorIndex(1);
				Ellipse->SetLinetypeIndex(1);
				Group->AddTail(Ellipse);

				ptArrow = ln.startPoint();
				ptArrow.rotateBy(Angle - RADIAN, PlaneNormal, CenterPoint);
				// <tas="This LineEndItem is wrong"</tas>
				GenerateLineEndItem(1, .1, ptArrow, ln.endPoint(), Group);
				const int PrimitiveState = pstate.Save();

				EoDbFontDefinition FontDefinition = pstate.FontDefinition();
				FontDefinition.SetHorizontalAlignment(EoDb::kAlignCenter);
				FontDefinition.SetVerticalAlignment(EoDb::kAlignMiddle);
				pstate.SetFontDefinition(DeviceContext, FontDefinition);

				EoDbCharacterCellDefinition CharacterCellDefinition = pstate.CharacterCellDefinition();
				CharacterCellDefinition.SetRotationAngle(0.);
				CharacterCellDefinition.SetHeight(.1);
				pstate.SetCharacterCellDefinition(CharacterCellDefinition);

				const OdGePoint3d ptPvt = ProjectToward(ptCur, CenterPoint, - .25);

                EoGeReferenceSystem ReferenceSystem(ptPvt, PlaneNormal, CharacterCellDefinition);

				EoDbText* TextPrimitive = EoDbText::Create(Database());
				TextPrimitive->SetTo(FontDefinition, ReferenceSystem, theApp.FormatAngle(Angle));
				Group->AddTail(TextPrimitive);
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
	const OdGePoint3d ptCur = GetCursorPosition();
	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}

	EoDbGroup* Group;
	EoDbPrimitive* Primitive;
	OdGePoint3d ptProj;

	POSITION posPrimCur;

	EoGePoint4d ptView(ptCur, 1.);
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
					EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);
					EoDbDimension* DimensionPrimitive = new EoDbDimension();
					DimensionPrimitive->SetColorIndex(LinePrimitive->ColorIndex());
					DimensionPrimitive->SetLinetypeIndex(LinePrimitive->LinetypeIndex());
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
					PreviousDimensionCursorPosition = ptProj;
					return;
				}
				else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
					EoDbDimension* DimensionPrimitive = static_cast<EoDbDimension*>(Primitive);
					EoGeReferenceSystem ReferenceSystem;
					ReferenceSystem = DimensionPrimitive->ReferenceSystem();
					EoDbLine* LinePrimitive = EoDbLine::Create(DimensionPrimitive->Line().startPoint(), DimensionPrimitive->Line().endPoint());
					LinePrimitive->SetColorIndex(DimensionPrimitive->ColorIndex());
					LinePrimitive->SetLinetypeIndex(DimensionPrimitive->LinetypeIndex());
					EoDbText* TextPrimitive = EoDbText::Create(Database());
					TextPrimitive->SetTo(DimensionPrimitive->FontDef(), ReferenceSystem, DimensionPrimitive->Text());
					TextPrimitive->SetColorIndex(DimensionPrimitive->TextColorIndex());
					Group->InsertAfter(posPrimCur, LinePrimitive);
					Group->InsertAfter(posPrimCur, TextPrimitive);
					Group->RemoveAt(posPrimCur);
					delete Primitive;
					PreviousDimensionCursorPosition = ptProj;
					return;
				}
			}
		}
	}
	PreviousDimensionCursorPosition = ptCur;
}
void AeSysView::OnDimensionModeReturn(void) {
	const OdGePoint3d ptCur = GetCursorPosition();
	if (PreviousDimensionCommand != 0) {
		RubberBandingDisable();
		ModeLineUnhighlightOp(PreviousDimensionCommand);
	}
	PreviousDimensionCursorPosition = ptCur;
}
void AeSysView::OnDimensionModeEscape(void) {
	const OdGePoint3d ptCur = GetCursorPosition();

	RubberBandingDisable();
	ModeLineUnhighlightOp(PreviousDimensionCommand);
}
