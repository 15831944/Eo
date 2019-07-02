#include "stdafx.h"
#include "AeSysDoc.h"
#include "PrimState.h"
#include "EoDbHatch.h"
#include "EoDlgTrapModify.h"
IMPLEMENT_DYNAMIC(EoDlgTrapModify, CDialog)

BEGIN_MESSAGE_MAP(EoDlgTrapModify, CDialog)
END_MESSAGE_MAP()

EoDlgTrapModify::EoDlgTrapModify(CWnd* parent) noexcept
	: CDialog(IDD, parent) {
}

EoDlgTrapModify::EoDlgTrapModify(AeSysDoc* document, CWnd* parent)
	: CDialog(IDD, parent)
	, m_Document(document) {
}

void EoDlgTrapModify::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
}

void EoDlgTrapModify::OnOK() {
	if (IsDlgButtonChecked(IDC_MOD_PEN)) {
		m_Document->ModifyTrappedGroupsColorIndex(g_PrimitiveState.ColorIndex());
	}
	if (IsDlgButtonChecked(IDC_MOD_LINE)) {
		m_Document->ModifyTrappedGroupsLinetypeIndex(g_PrimitiveState.LinetypeIndex());
	}
	if (IsDlgButtonChecked(IDC_MOD_FILL)) {
		ModifyPolygons();
	}
	auto CharacterCellDefinition {g_PrimitiveState.CharacterCellDefinition()};
	auto FontDefinition {g_PrimitiveState.FontDefinition()};
	if (IsDlgButtonChecked(IDC_MOD_NOTE)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(FontDefinition, CharacterCellDefinition, gc_TrapModifyTextAll);
	} else if (IsDlgButtonChecked(IDC_FONT)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(FontDefinition, CharacterCellDefinition, gc_TrapModifyTextFont);
	} else if (IsDlgButtonChecked(IDC_HEIGHT)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(FontDefinition, CharacterCellDefinition, gc_TrapModifyTextHeight);
	}
	CDialog::OnOK();
}

void EoDlgTrapModify::ModifyPolygons() {
	auto Position {m_Document->GetFirstTrappedGroupPosition()};
	while (Position != nullptr) {
		const auto Group {m_Document->GetNextTrappedGroup(Position)};
		auto PrimitivePosition {Group->GetHeadPosition()};
		while (PrimitivePosition != nullptr) {
			const auto Primitive {Group->GetNext(PrimitivePosition)};
			if (Primitive->IsKindOf(RUNTIME_CLASS(EoDbHatch))) {
				auto Polygon {dynamic_cast<EoDbHatch*>(Primitive)};
				Polygon->SetInteriorStyle(g_PrimitiveState.HatchInteriorStyle());
				Polygon->SetInteriorStyleIndex2(g_PrimitiveState.HatchInteriorStyleIndex());
				Polygon->SetHatchReferenceAxes(EoDbHatch::patternAngle, EoDbHatch::patternScaleX, EoDbHatch::patternScaleY);
			}
		}
	}
}
