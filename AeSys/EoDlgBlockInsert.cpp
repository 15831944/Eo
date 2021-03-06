#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "Preview.h"
#include "EoDb.h"
#include "EoDbBlockReference.h"
#include "EoDlgBlockInsert.h"
IMPLEMENT_DYNAMIC(EoDlgBlockInsert, CDialog)

BEGIN_MESSAGE_MAP(EoDlgBlockInsert, CDialog)
		ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgBlockInsert::OnLbnSelectionChangeBlocksList)
		ON_BN_CLICKED(IDC_PURGE, &EoDlgBlockInsert::OnBnClickedPurge)
		ON_BN_CLICKED(IDCANCEL, &EoDlgBlockInsert::OnBnClickedCancel)
END_MESSAGE_MAP()
OdGePoint3d EoDlgBlockInsert::ms_InsertionPoint;

EoDlgBlockInsert::EoDlgBlockInsert(CWnd* parent)
	: CDialog(IDD, parent) {}

EoDlgBlockInsert::EoDlgBlockInsert(AeSysDoc* document, CWnd* parent)
	: CDialog(IDD, parent)
	, m_Document(document) {}

void EoDlgBlockInsert::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_BlocksListBoxControl);
	DDX_Control(dataExchange, IDC_INSERTION_POINT_ONSCREEN, m_InsertionPointOnscreen);
	DDX_Control(dataExchange, IDC_INSERTION_POINT_X, m_InsertionPointX);
	DDX_Control(dataExchange, IDC_INSERTION_POINT_Y, m_InsertionPointY);
	DDX_Control(dataExchange, IDC_INSERTION_POINT_Z, m_InsertionPointZ);
	DDX_Control(dataExchange, IDC_SCALE_ONSCREEN, m_ScaleOnscreen);
	DDX_Control(dataExchange, IDC_SCALE_X, m_ScaleX);
	DDX_Control(dataExchange, IDC_SCALE_Y, m_ScaleY);
	DDX_Control(dataExchange, IDC_SCALE_Z, m_ScaleZ);
	DDX_Control(dataExchange, IDC_ROTATION_ONSCREEN, m_RotationOnscreen);
	DDX_Control(dataExchange, IDC_ROTATION_ANGLE, m_RotationAngle);
	DDX_Control(dataExchange, IDC_EXPLODE, m_Explode);
}

BOOL EoDlgBlockInsert::OnInitDialog() {
	CDialog::OnInitDialog();
	ms_InsertionPoint = AeSys::GetCursorPosition();
	CString BlockName;
	EoDbBlock* Block;
	auto BlockPosition {m_Document->GetFirstBlockPosition()};
	while (BlockPosition != nullptr) {
		m_Document->GetNextBlock(BlockPosition, BlockName, Block);
		if (!Block->IsAnonymous()) {
			m_BlocksListBoxControl.AddString(BlockName);
		}
	}
	m_BlocksListBoxControl.SetCurSel(0);
	if (m_Document->BlockTableIsEmpty()) {
		WndProcPreviewClear(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd());
	} else {
		BlockPosition = m_Document->GetFirstBlockPosition();
		m_Document->GetNextBlock(BlockPosition, BlockName, Block);
		SetDlgItemInt(IDC_GROUPS, gsl::narrow_cast<unsigned>(Block->GetCount()), FALSE);
		SetDlgItemInt(IDC_REFERENCES, static_cast<unsigned>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
		WndProcPreviewUpdate(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
	}
	return TRUE;
}

void EoDlgBlockInsert::OnOK() {
	const auto CurrentSelection {m_BlocksListBoxControl.GetCurSel()};
	if (CurrentSelection != LB_ERR) {
		CString BlockName;
		m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);
		auto BlockReference {new EoDbBlockReference()};
		BlockReference->SetName(BlockName);
		BlockReference->SetPosition2(ms_InsertionPoint);
		auto Group {new EoDbGroup};
		Group->AddTail(BlockReference);
		m_Document->AddWorkLayerGroup(Group);
		m_Document->UpdateGroupInAllViews(EoDb::kGroup, Group);
	}
	CDialog::OnOK();
}

void EoDlgBlockInsert::OnLbnSelectionChangeBlocksList() {
	const auto CurrentSelection {m_BlocksListBoxControl.GetCurSel()};
	if (CurrentSelection != LB_ERR) {
		CString BlockName;
		m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);
		EoDbBlock* Block;
		m_Document->LookupBlock(BlockName, Block);
		SetDlgItemInt(IDC_GROUPS, gsl::narrow_cast<unsigned>(Block->GetCount()), FALSE);
		SetDlgItemInt(IDC_REFERENCES, static_cast<unsigned>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
		WndProcPreviewUpdate(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
	}
}

void EoDlgBlockInsert::OnBnClickedPurge() {
	m_Document->PurgeUnreferencedBlocks();
	CDialog::OnOK();
}

void EoDlgBlockInsert::OnBnClickedCancel() {
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}
