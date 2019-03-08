#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"
#include "EoDlgNamedViews.h"
#include "DbLayout.h"
#include "DbViewport.h"

#include "DbViewTable.h"
#include "DbViewTableRecord.h"
#include "DbViewportTable.h"
#include "DbViewportTableRecord.h"
#include "DbBlockTableRecord.h"
#include "EoDlgNewView.h"
#include "DbLayerState.h"
#include "DbUCSTableRecord.h"
#include "DbSymUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

OdDbObjectId CNamedViewListCtrl::viewId(int nItem) const {
	return OdDbObjectId(reinterpret_cast<OdDbStub*>(GetItemData(nItem)));
}
void CNamedViewListCtrl::setViewId(int nItem, const OdDbObjectId& id) {
	SetItemData(nItem, reinterpret_cast<DWORD>(static_cast<OdDbStub*>(id)));
}
OdDbViewTableRecordPtr CNamedViewListCtrl::view(int nItem) {
	return viewId(nItem).safeOpenObject(OdDb::kForWrite);
}
void CNamedViewListCtrl::setView(int nItem, const OdDbViewTableRecord* pView) {
	setViewId(nItem, pView->objectId());
}
OdString ucsString(const OdDbObject* pViewObj) {
	OdString res;
	OdDbAbstractViewportDataPtr pViewPE(pViewObj);
	switch (pViewPE->orthoUcs(pViewObj)) {
	case OdDb::kTopView:
		res = L"Top";
		break;
	case OdDb::kBottomView:
		res = L"Bottom";
		break;
	case OdDb::kFrontView:
		res = L"Front";
		break;
	case OdDb::kBackView:
		res = L"Back";
		break;
	case OdDb::kLeftView:
		res = L"Left";
		break;
	case OdDb::kRightView:
		res = L"Right";
		break;
	default: {
			OdDbUCSTableRecordPtr pUCS = pViewPE->ucsName(pViewObj).openObject();
			if (pUCS.get()) {
				res = pUCS->getName();
			}
			else {
				OdGePoint3d origin;
				OdGeVector3d xAxis, yAxis;
				pViewPE->getUcs(pViewObj, origin, xAxis, yAxis);
				if (origin==OdGePoint3d::kOrigin && xAxis==OdGeVector3d::kXAxis && yAxis==OdGeVector3d::kYAxis) {
					res = L"World";
				}
				else {
					res = L"Unnamed";
				}
			}
		}
		break;
	}
	return res;
}
void CNamedViewListCtrl::InsertItem(int i, const OdDbViewTableRecord* pView) {
	CListCtrl::InsertItem(i, pView->getName());
	setView(i, pView);
	SetItemText(i, 1, pView->getCategoryName());
	SetItemText(i, 2, pView->isPaperspaceView() ? L"Layout" : L"Model");
	SetItemText(i, 3, pView->isViewAssociatedToViewport() ? L"True" : L"");
	SetItemText(i, 4, pView->getLayerState().isEmpty() ? L"" : L"Saved");
	if (pView->isUcsAssociatedToView()) {
		SetItemText(i, 5, ucsString(pView));
	}
	SetItemText(i, 6, pView->perspectiveEnabled() ? L"On" : L"Off");
}
OdDbViewTableRecordPtr CNamedViewListCtrl::selectedView() {
	int nSelectionMark = GetSelectionMark();
	if (nSelectionMark > - 1) {
		return view(nSelectionMark);
	}
	return OdDbViewTableRecordPtr();
}

EoDlgNamedViews::EoDlgNamedViews(AeSysDoc* pDoc, CWnd* pParent /*=NULL*/) : 
	CDialog(EoDlgNamedViews::IDD, pParent) {
	m_pDoc = pDoc;
}

OdDbDatabase* EoDlgNamedViews::database() {
	return document()->m_DatabasePtr; 
}
void EoDlgNamedViews::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_NAMEDVIEWS, m_views);
}

BEGIN_MESSAGE_MAP(EoDlgNamedViews, CDialog)
	ON_BN_CLICKED(IDC_SETCURRENT_BUTTON, OnSetcurrentButton)
	ON_NOTIFY(NM_DBLCLK, IDC_NAMEDVIEWS, OnDblclkNamedviews)
	ON_BN_CLICKED(IDC_NEW_BUTTON, OnNewButton)
	ON_BN_CLICKED(IDC_UPDATE_LAYERS_BUTTON, OnUpdateLayersButton)
	ON_BN_CLICKED(IDC_DELETE_BUTTON, OnDeleteButton)

END_MESSAGE_MAP()

BOOL EoDlgNamedViews::OnInitDialog() {
	CDialog::OnInitDialog();

	m_views.InsertColumn(0, L"Name", LVCFMT_LEFT, 100);
	m_views.InsertColumn(1, L"Category", LVCFMT_LEFT, 60);
	m_views.InsertColumn(2, L"Location", LVCFMT_LEFT, 50);
	m_views.InsertColumn(3, L"VP", LVCFMT_LEFT, 40);
	m_views.InsertColumn(4, L"Layers", LVCFMT_LEFT, 50);
	m_views.InsertColumn(5, L"UCS", LVCFMT_LEFT, 60);
	m_views.InsertColumn(6, L"Perspective", LVCFMT_LEFT, 30);

	try {
		OdDbDatabase* Database = m_pDoc->m_DatabasePtr;
		OdDbViewTablePtr ViewTable = Database->getViewTableId().safeOpenObject();
		int Index(0);
		OdDbSymbolTableIteratorPtr ViewTableIterator;
		for (ViewTableIterator = ViewTable->newIterator(); !ViewTableIterator->done(); ViewTableIterator->step()) {
			OdDbViewTableRecordPtr ViewTableRecord = ViewTableIterator->getRecordId().openObject();
			m_views.InsertItem(Index++, ViewTableRecord);
		}
	}
	catch(const OdError& Error) {
		theApp.reportError(L"Error creating Named Views dialog", Error);
		EndDialog(IDCANCEL);
		return FALSE;
	}
	return TRUE;
}
void EoDlgNamedViews::OnSetcurrentButton() {
	OdDbViewTableRecordPtr NamedView = m_views.selectedView();
	if (NamedView.get()) {
		OdDbDatabase* pDb = m_pDoc->m_DatabasePtr;
		OdDbObjectPtr ActiveViewportObject = pDb->activeViewportId().safeOpenObject(OdDb::kForWrite);
		OdDbAbstractViewportDataPtr pVpPE(ActiveViewportObject);
		pVpPE->setView(ActiveViewportObject, NamedView);
		pVpPE->setUcs(ActiveViewportObject, NamedView);
		pVpPE->setProps(ActiveViewportObject, NamedView);
		OdString sLSName = NamedView->getLayerState();
		if (!sLSName.isEmpty()) {
			OdDbLayerState::restore(pDb, sLSName, OdDbLayerState::kUndefDoNothing, OdDbLayerState::kOn|OdDbLayerState::kFrozen);
		}
	}
}
void EoDlgNamedViews::OnDblclkNamedviews(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/) {
	OnSetcurrentButton();
}
void deleteLayerState(OdDbViewTableRecord* pNamedView) {
	OdString sLSName = pNamedView->getLayerState();
	if(!sLSName.isEmpty()) {
		OdDbLayerState::remove(pNamedView->database(), sLSName);
		pNamedView->setLayerState(L"");
	}
}
void updateLayerState(OdDbViewTableRecord* pNamedView) {
	OdString sLSName = pNamedView->getLayerState();
	OdDbDatabase* pDb = pNamedView->database();
	if(sLSName.isEmpty()) {
		OdString name;
		name.format(OD_T("ACAD_VIEWS_%s"), pNamedView->getName().c_str());
		sLSName = name;
		int i = 1;
		while (OdDbLayerState::has(pDb, sLSName)) {
			sLSName.format(OD_T("%s(%d)"), name.c_str(), ++i);
		}
		pNamedView->setLayerState(sLSName);
	}
	OdDbLayerState::save(pDb, sLSName, OdDbLayerState::kHidden|OdDbLayerState::kCurrentViewport);
}
void EoDlgNamedViews::OnNewButton() {
	EoDlgNewView newDlg(this);
	OdDbViewTableRecordPtr pNamedView;
	OdDbDatabase* pDb = m_pDoc->m_DatabasePtr;
	while(newDlg.DoModal() == IDOK) {
		LVFINDINFO lvfi = {LVFI_STRING, newDlg.m_sViewName, 0, {0,0}, 0};
		int i = m_views.FindItem(&lvfi);
		if (i >= 0) {
			if (AfxMessageBox(newDlg.m_sViewName + L" already exists.\nDo you want to replace it?", MB_YESNOCANCEL) != IDYES)
				continue;
			pNamedView = m_views.view(i);
			m_views.DeleteItem(i);
		}
		else {
			OdDbViewTablePtr pViewTable = pDb->getViewTableId().safeOpenObject(OdDb::kForWrite);
			pNamedView = OdDbViewTableRecord::createObject();
			pNamedView->setName(OdString(newDlg.m_sViewName));
			pViewTable->add(pNamedView);

			i = m_views.GetItemCount();
		}
		OdDbObjectPtr ActiveViewportObject = pDb->activeViewportId().safeOpenObject();
		OdDbAbstractViewportDataPtr pViewPE(pNamedView);
		pViewPE->setView(pNamedView, ActiveViewportObject);

		if (newDlg.m_bSaveUCS) {
			if (newDlg.m_sUcsName == L"Unnamed")
				pViewPE->setUcs(pNamedView, ActiveViewportObject);
			else if (newDlg.m_sUcsName == L"World")
				pNamedView->setUcsToWorld();
			else
				pNamedView->setUcs(OdDbSymUtil::getUCSId(OdString(newDlg.m_sUcsName), pDb));
		}
		else
			pNamedView->disassociateUcsFromView();

		pViewPE->setProps(pNamedView, ActiveViewportObject);

		pNamedView->setCategoryName(OdString(newDlg.m_sViewCategory));

		if (newDlg.m_bStoreLS)
			updateLayerState(pNamedView);
		else
			deleteLayerState(pNamedView);

		m_views.InsertItem(i, pNamedView);
		break;
	}
}
void EoDlgNamedViews::OnUpdateLayersButton() {
	int nSelectionMark = m_views.GetSelectionMark();
	if (nSelectionMark > - 1) {
		updateLayerState(m_views.selectedView());
		m_views.SetItemText(nSelectionMark, 4, L"Saved");
	}
}
void EoDlgNamedViews::OnDeleteButton() {
	int nSelectionMark = m_views.GetSelectionMark();
	if (nSelectionMark > - 1) {
		m_views.view(nSelectionMark)->erase();
		m_views.DeleteItem(nSelectionMark);
	}
}
