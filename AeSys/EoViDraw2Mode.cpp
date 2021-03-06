#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "PrimState.h"
#include "EoDlgSetLength.h"

void AeSysView::OnDraw2ModeOptions() {
	EoDlgSetLength SetLengthDialog;
	SetLengthDialog.title = L"Set Distance Between Lines";
	SetLengthDialog.length = m_DistanceBetweenLines;
	if (SetLengthDialog.DoModal() == IDOK) {
		m_DistanceBetweenLines = SetLengthDialog.length;
	}
}

void AeSysView::OnDraw2ModeJoin() {
	auto CurrentPnt {GetCursorPosition()};
	CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
	auto Selection {SelectLineUsingPoint(CurrentPnt)};
	const auto Group {std::get<tGroup>(Selection)};
	if (Group != nullptr) {
		const auto Line {std::get<1>(Selection)};
		CurrentPnt = Line->ProjPt_(CurrentPnt);
		if (m_PreviousOp == 0) { // Starting at existing wall
			m_BeginSectionGroup = Group;
			m_BeginSectionLine = Line;
			m_PreviousPnt = CurrentPnt;
			m_PreviousOp = ID_OP1;
		} else { // Ending at existing wall
			m_EndSectionGroup = Group;
			m_EndSectionLine = Line;
			OnDraw2ModeWall();
			OnDraw2ModeEscape();
		}
		SetCursorPosition(CurrentPnt);
	}
}

void AeSysView::OnDraw2ModeWall() {
	auto CurrentPnt {GetCursorPosition()};
	OdDbBlockTableRecordPtr BlockTableRecord {Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	const auto ColorIndex {g_PrimitiveState.ColorIndex()};
	const auto Linetype {EoDbPrimitive::LinetypeObjectFromIndex(g_PrimitiveState.LinetypeIndex())};
	if (m_PreviousOp != 0) {
		GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, &m_PreviewGroup);
		m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	}
	if (m_EndSectionGroup == nullptr) {
		if (m_PreviousOp != 0) {
			CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
			m_CurrentReferenceLine = EoGeLineSeg3d(m_PreviousPnt, CurrentPnt);
			m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine, m_CurrentRightLine);
			if (m_ContinueCorner) {
				CleanPreviousLines();
			} else if (m_BeginSectionGroup != nullptr) {
				StartAssemblyFromLine();
			} else if (m_PreviousOp == ID_OP2) {
				m_AssemblyGroup = new EoDbGroup;
				GetDocument()->AddWorkLayerGroup(m_AssemblyGroup);
				auto Line {EoDbLine::Create(BlockTableRecord, m_CurrentLeftLine.startPoint(), m_CurrentRightLine.startPoint())};
				Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
				Line->setLinetype(Linetype);
				m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
			}
			auto Line = EoDbLine::Create(BlockTableRecord, m_CurrentLeftLine.startPoint(), m_CurrentLeftLine.endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(Linetype);
			m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
			Line = EoDbLine::Create(BlockTableRecord, m_CurrentRightLine.startPoint(), m_CurrentRightLine.endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(Linetype);
			m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
			Line = EoDbLine::Create(BlockTableRecord, m_CurrentRightLine.endPoint(), m_CurrentLeftLine.endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(Linetype);
			m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
			GetDocument()->UpdateGroupInAllViews(EoDb::kGroupSafe, m_AssemblyGroup);
			m_ContinueCorner = true;
			m_PreviousReferenceLine = m_CurrentReferenceLine;
		}
		m_PreviousOp = ID_OP2;
		m_PreviousPnt = CurrentPnt;
		SetCursorPosition(m_PreviousPnt);
	} else {
		m_CurrentReferenceLine = EoGeLineSeg3d(m_PreviousPnt, CurrentPnt);
		m_CurrentReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, m_CurrentLeftLine, m_CurrentRightLine);
		if (m_ContinueCorner) {
			CleanPreviousLines();
		} else if (m_BeginSectionGroup != nullptr) {
			StartAssemblyFromLine();
		} else if (m_PreviousOp == ID_OP2) {
			m_AssemblyGroup = new EoDbGroup;
			GetDocument()->AddWorkLayerGroup(m_AssemblyGroup);
			auto Line {EoDbLine::Create(BlockTableRecord, m_CurrentLeftLine.startPoint(), m_CurrentRightLine.startPoint())};
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(Linetype);
			m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
		}
		GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, m_EndSectionGroup);
		const auto StartPoint {m_EndSectionLine->StartPoint()};
		auto Line {EoDbLine::Create(BlockTableRecord, m_CurrentLeftLine.startPoint(), m_CurrentLeftLine.endPoint())};
		Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
		Line->setLinetype(Linetype);
		m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
		Line = EoDbLine::Create(BlockTableRecord, m_CurrentRightLine.startPoint(), m_CurrentRightLine.endPoint());
		Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
		Line->setLinetype(Linetype);
		m_AssemblyGroup->AddTail(EoDbLine::Create(Line));
		GetDocument()->UpdateGroupInAllViews(EoDb::kGroupSafe, m_AssemblyGroup);
		OdDbLinePtr LineEntity {m_EndSectionLine->EntityObjectId().safeOpenObject()->clone()};
		BlockTableRecord->appendOdDbEntity(LineEntity);
		auto LinePrimitive {EoDbLine::Create(LineEntity)};
		if (EoGeLineSeg3d(m_PreviousPnt, CurrentPnt).DirectedRelationshipOf(StartPoint) < 0) {
			m_EndSectionLine->SetEndPoint(m_CurrentRightLine.endPoint());
			LinePrimitive->SetStartPoint(m_CurrentLeftLine.endPoint());
		} else {
			m_EndSectionLine->SetEndPoint(m_CurrentLeftLine.endPoint());
			LinePrimitive->SetStartPoint(m_CurrentRightLine.endPoint());
		}
		m_EndSectionGroup->AddTail(LinePrimitive);
		GetDocument()->UpdateGroupInAllViews(EoDb::kGroupSafe, m_EndSectionGroup);
		m_EndSectionGroup = nullptr;
		ModeLineUnhighlightOp(m_PreviousOp);
		m_ContinueCorner = false;
	}
	m_PreviousPnt = CurrentPnt;
}

void AeSysView::OnDraw2ModeReturn() {
	if (m_PreviousOp != 0) {
		OnDraw2ModeEscape();
	}
	m_PreviousPnt = GetCursorPosition();
}

void AeSysView::OnDraw2ModeEscape() {
	GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, &m_PreviewGroup);
	m_PreviewGroup.DeletePrimitivesAndRemoveAll();
	// <tas="ModeLineUnhighlightOp does not set commandId to 0 in some cases"</tas>
	ModeLineUnhighlightOp(m_PreviousOp);
	m_PreviousOp = 0;
	m_AssemblyGroup = nullptr;
	m_ContinueCorner = false;
	m_BeginSectionGroup = nullptr;
	m_BeginSectionLine = nullptr;
	m_EndSectionGroup = nullptr;
	m_EndSectionLine = nullptr;
}

bool AeSysView::CleanPreviousLines() {
	const auto ParallelLines {m_PreviousReferenceLine.isParallelTo(m_CurrentReferenceLine)};
	if (ParallelLines) {
		return false;
	}
	OdGePoint3d ptInt;
	EoGeLineSeg3d PreviousLeftLine;
	EoGeLineSeg3d PreviousRightLine;
	m_PreviousReferenceLine.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviousLeftLine, PreviousRightLine);
	PreviousLeftLine.IntersectWith_xy(m_CurrentLeftLine, ptInt);
	PreviousLeftLine.SetEndPoint(ptInt);
	m_CurrentLeftLine.SetStartPoint(ptInt);
	PreviousRightLine.IntersectWith_xy(m_CurrentRightLine, ptInt);
	PreviousRightLine.SetEndPoint(ptInt);
	m_CurrentRightLine.SetStartPoint(ptInt);
	GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, m_AssemblyGroup);
	const auto Primitive {dynamic_cast<EoDbPrimitive*>(m_AssemblyGroup->RemoveTail())};
	Primitive->EntityObjectId().safeOpenObject(OdDb::kForWrite)->erase();
	delete Primitive;
	auto Position {m_AssemblyGroup->GetTailPosition()};
	dynamic_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(Position))->SetEndPoint(PreviousRightLine.endPoint());
	dynamic_cast<EoDbLine*>(m_AssemblyGroup->GetPrev(Position))->SetEndPoint(PreviousLeftLine.endPoint());
	return true;
}

bool AeSysView::StartAssemblyFromLine() {
	const auto Line {m_BeginSectionLine->LineSeg()};
	const auto ParallelLines {Line.isParallelTo(m_CurrentReferenceLine)};
	if (ParallelLines) {
		return false;
	}
	OdGePoint3d ptInt;
	m_AssemblyGroup = m_BeginSectionGroup;
	GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, m_AssemblyGroup);
	Line.IntersectWith_xy(m_CurrentLeftLine, ptInt);
	m_CurrentLeftLine.SetStartPoint(ptInt);
	Line.IntersectWith_xy(m_CurrentRightLine, ptInt);
	m_CurrentRightLine.SetStartPoint(ptInt);
	OdDbBlockTableRecordPtr BlockTableRecord {Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	OdDbLinePtr LineEntity {m_BeginSectionLine->EntityObjectId().safeOpenObject()->clone()};
	BlockTableRecord->appendOdDbEntity(LineEntity);
	auto LinePrimitive {EoDbLine::Create(LineEntity)};
	if (OdGeVector3d(m_CurrentLeftLine.startPoint() - Line.startPoint()).length() > OdGeVector3d(m_CurrentRightLine.startPoint() - Line.startPoint()).length()) {
		m_BeginSectionLine->SetEndPoint(m_CurrentRightLine.startPoint());
		LinePrimitive->SetStartPoint(m_CurrentLeftLine.startPoint());
	} else {
		m_BeginSectionLine->SetEndPoint(m_CurrentLeftLine.startPoint());
		LinePrimitive->SetStartPoint(m_CurrentRightLine.startPoint());
	}
	m_AssemblyGroup->AddTail(LinePrimitive);
	m_BeginSectionLine = nullptr;
	return true;
}

void AeSysView::DoDraw2ModeMouseMove() {
	static auto CurrentPnt {OdGePoint3d()};
	const OdDbBlockTableRecordPtr BlockTableRecord {Database()->getModelSpaceId().safeOpenObject(OdDb::kForWrite)};
	if (m_PreviousOp == 0) {
		CurrentPnt = GetCursorPosition();
	} else if (m_PreviousOp == ID_OP1 || m_PreviousOp == ID_OP2) {
		CurrentPnt = GetCursorPosition();
		if (!CurrentPnt.isEqualTo(m_PreviousPnt)) {
			GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, &m_PreviewGroup);
			m_PreviewGroup.DeletePrimitivesAndRemoveAll();
			CurrentPnt = SnapPointToAxis(m_PreviousPnt, CurrentPnt);
			EoGeLineSeg3d PreviewLines[2];
			const EoGeLineSeg3d ln(m_PreviousPnt, CurrentPnt);
			ln.GetParallels(m_DistanceBetweenLines, m_CenterLineEccentricity, PreviewLines[0], PreviewLines[1]);
			const auto ColorIndex {g_PrimitiveState.ColorIndex()};
			const auto LinetypeId {EoDbPrimitive::LinetypeObjectFromIndex(g_PrimitiveState.LinetypeIndex())};
			auto Line {EoDbLine::Create(BlockTableRecord, PreviewLines[0].startPoint(), PreviewLines[1].startPoint())};
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(LinetypeId);
			m_PreviewGroup.AddTail(EoDbLine::Create(Line));
			Line = EoDbLine::Create(BlockTableRecord, PreviewLines[0].startPoint(), PreviewLines[0].endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(LinetypeId);
			m_PreviewGroup.AddTail(EoDbLine::Create(Line));
			Line = EoDbLine::Create(BlockTableRecord, PreviewLines[1].startPoint(), PreviewLines[1].endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(LinetypeId);
			m_PreviewGroup.AddTail(EoDbLine::Create(Line));
			Line = EoDbLine::Create(BlockTableRecord, PreviewLines[1].endPoint(), PreviewLines[0].endPoint());
			Line->setColorIndex(static_cast<unsigned short>(ColorIndex));
			Line->setLinetype(LinetypeId);
			m_PreviewGroup.AddTail(EoDbLine::Create(Line));
			GetDocument()->UpdateGroupInAllViews(EoDb::kGroupEraseSafe, &m_PreviewGroup);
		}
	}
}
