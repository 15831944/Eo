#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include <DbLayerTable.h>
#include <DbViewport.h>
#include <DbLinetypeTable.h>
#include <DbLinetypeTableRecord.h>
#include "EoDb.h"
#include "EoDlgFileManage.h"
#include "EoDlgLineWeight.h"
#include "EoDlgSetupColor.h"
#include "EoDlgSetupLinetype.h"
#include "Preview.h"
IMPLEMENT_DYNAMIC(EoDlgFileManage, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191) // (level 3) 'operator': unsafe conversion from 'type_of_expression' to 'type_required'
BEGIN_MESSAGE_MAP(EoDlgFileManage, CDialog)
		ON_WM_DRAWITEM()
		ON_BN_CLICKED(IDC_FUSE, &EoDlgFileManage::OnBnClickedFuse)
		ON_BN_CLICKED(IDC_MELT, &EoDlgFileManage::OnBnClickedMelt)
		ON_BN_CLICKED(IDC_NEWLAYER, &EoDlgFileManage::OnBnClickedNewLayer)
		ON_BN_CLICKED(IDC_SETCURRENT, &EoDlgFileManage::OnBnClickedSetCurrent)
		ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgFileManage::OnLbnSelectionChangeBlocksList)
		ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNmClickLayersListControl)
		ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNmDoubleClickLayersListControl)
		ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnItemChangedLayersListControl)
		ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnLvnEndlabeleditLayersListControl)
		ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnLvnBeginLabelEditLayersListControl)
		ON_NOTIFY(LVN_KEYDOWN, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnLvnKeydownLayersListControl)
END_MESSAGE_MAP()
#pragma warning (pop)
EoDlgFileManage::EoDlgFileManage(CWnd* parent)
	: CDialog(IDD, parent) {}

EoDlgFileManage::EoDlgFileManage(AeSysDoc* document, OdDbDatabasePtr database, CWnd* parent)
	: CDialog(IDD, parent)
	, m_Document(document)
	, m_Database(database) {}

void EoDlgFileManage::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_LAYERS_LIST_CONTROL, m_LayersList);
	DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_BlocksList);
	DDX_Control(dataExchange, IDC_GROUPS, m_Groups);
}

void EoDlgFileManage::DrawItem(CDC& deviceContext, const int itemId, const int labelIndex, const RECT& itemRectangle) {
	const EoDbLayer* Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(itemId))};
	auto LayerTableRecord {Layer->TableRecord()};
	OdString ItemName;
	switch (labelIndex) {
		case kStatus:
			if (Layer->IsCurrent()) {
				m_StateImages.Draw(&deviceContext, 8, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			} else if (LayerTableRecord->isInUse()) {
				m_StateImages.Draw(&deviceContext, 9, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			} else if (!Layer->IsInternal()) { // <tas="Unconventional usage of status image to flag Tracing files"</tas>
				m_StateImages.Draw(&deviceContext, 10, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			}
			break;
		case kName:
			ItemName = Layer->Name();
			deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, ItemName, static_cast<unsigned>(ItemName.getLength()), nullptr);
			break;
		case kOn:
			m_StateImages.Draw(&deviceContext, Layer->IsOff() ? 3 : 2, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			break;
		case kFreeze:
			m_StateImages.Draw(&deviceContext, LayerTableRecord->isFrozen() ? 4 : 5, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			break;
		case kLock:
			m_StateImages.Draw(&deviceContext, LayerTableRecord->isLocked() ? 0 : 1, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			break;
		case kColor:
			CMainFrame::DrawColorBox(deviceContext, itemRectangle, LayerTableRecord->color());
			break;
		case kLinetype:
			ItemName = OdDbSymUtil::getSymbolName(LayerTableRecord->linetypeObjectId());
			deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, ItemName, static_cast<unsigned>(ItemName.getLength()), nullptr);
			break;
		case kLineweight:
			CMainFrame::DrawLineWeight(deviceContext, itemRectangle, LayerTableRecord->lineWeight());
			break;
		case kPlotStyle:
			ItemName = LayerTableRecord->plotStyleName();
			CMainFrame::DrawPlotStyle(deviceContext, itemRectangle, ItemName, m_Database);
			break;
		case kPlot:
			m_StateImages.Draw(&deviceContext, LayerTableRecord->isPlottable() ? 6 : 7, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
			break;
		case kVpFreeze:
			if (labelIndex != m_Description) {
				auto Viewport {OdDbViewport::cast(LayerTableRecord->database()->activeViewportId().safeOpenObject())};
				if (Viewport.get() != nullptr) {
					//				m_StateImages.Draw(&deviceContext, Viewport->isLayerFrozenInViewport(ItemData) ? 4 : 5, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
				}
			} else {
				ItemName = LayerTableRecord->description();
				deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, ItemName, static_cast<unsigned>(ItemName.getLength()), nullptr);
			}
			break;
		case kVpColor:
			CMainFrame::DrawColorBox(deviceContext, itemRectangle, LayerTableRecord->color(m_ActiveViewport));
			break;
		case kVpLinetype:
			ItemName = OdDbSymUtil::getSymbolName(LayerTableRecord->linetypeObjectId(m_ActiveViewport));
			deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, ItemName, static_cast<unsigned>(ItemName.getLength()), nullptr);
			break;
		case kVpLineweight:
			CMainFrame::DrawLineWeight(deviceContext, itemRectangle, LayerTableRecord->lineWeight(m_ActiveViewport));
			break;
		case kVpPlotStyle:
			ItemName = LayerTableRecord->plotStyleName(m_ActiveViewport);
			CMainFrame::DrawPlotStyle(deviceContext, itemRectangle, ItemName, m_Database);
			break;
		default: ;
	}
}

void EoDlgFileManage::OnBnClickedFuse() {
	const auto SelectionMark {m_LayersList.GetSelectionMark()};
	if (SelectionMark > -1) {
		const EoDbLayer* Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(SelectionMark))};
		auto Name {Layer->Name()};
		if (Layer->IsInternal()) {
			theApp.AddStringToMessageList(L"Selection <%s> already an internal layer.\n", Name);
		} else {
			m_Document->TracingFuse(Name);
			m_LayersList.SetItemText(SelectionMark, 0, Name);
			theApp.AddStringToMessageList(IDS_MSG_TRACING_CONVERTED_TO_LAYER, Layer->Name());
		}
	}
}

void EoDlgFileManage::OnBnClickedMelt() {
	const auto SelectionMark {m_LayersList.GetSelectionMark()};
	if (SelectionMark > -1) {
		const EoDbLayer* Layer = reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(SelectionMark));
		auto Name {Layer->Name()};
		if (!Layer->IsInternal()) {
			theApp.AddStringToMessageList(L"Selection <%s> already a tracing.\n", Name);
		} else {
			if (m_Document->LayerMelt(Name)) {
				m_LayersList.SetItemText(SelectionMark, 0, Name);
				theApp.AddStringToMessageList(IDS_MSG_LAYER_CONVERTED_TO_TRACING, Layer->Name());
			}
		}
	}
}

void EoDlgFileManage::OnBnClickedNewLayer() {
	auto Layers {m_Document->LayerTable(OdDb::kForWrite)};
	OdString Name;
	auto Suffix {1};
	do {
		Name.format(L"Layer%d", Suffix++);
	} while (Layers->has(Name));
	auto LayerTableRecord {OdDbLayerTableRecord::createObject()};
	const auto Layer {new EoDbLayer(LayerTableRecord)};
	LayerTableRecord->setName(Name);
	m_Document->AddLayerTo(Layers, Layer);
	const auto ItemCount {m_LayersList.GetItemCount()};
	m_LayersList.InsertItem(ItemCount, Name);
	m_LayersList.SetItemData(ItemCount, DWORD_PTR(m_Document->GetLayerAt(Name)));
}

void EoDlgFileManage::OnBnClickedSetCurrent() {
	const auto SelectionMark {m_LayersList.GetSelectionMark()};
	if (SelectionMark > -1) {
		const auto Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(SelectionMark))};
		auto LayerTableRecord {Layer->TableRecord()};
		LayerTableRecord->upgradeOpen();
		if (!LayerTableRecord.isNull()) {
			OdDbLayerTableRecordPtr PreviousLayer {m_Database->getCLAYER().safeOpenObject()};
			const auto PreviousLayerName {PreviousLayer->getName()};
			m_Document->GetLayerAt(PreviousLayerName)->MakeActive();
			theApp.AddStringToMessageList(L"Status of layer <%s> has changed to active\n", PreviousLayerName);
			m_Database->setCLAYER(LayerTableRecord->objectId());
			theApp.AddStringToMessageList(L"Layer <%s> is now the current working layer\n", LayerTableRecord->getName());
			m_Document->SetCurrentLayer(LayerTableRecord);
			UpdateCurrentLayerInfoField();
			m_LayersList.RedrawWindow();
		}
		LayerTableRecord->downgradeOpen();
	}
}

void EoDlgFileManage::OnDrawItem(const int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct) {
	if (controlIdentifier == IDC_LAYERS_LIST_CONTROL) {
		switch (drawItemStruct->itemAction) {
			case ODA_DRAWENTIRE: {
				CRect rcItem(drawItemStruct->rcItem);
				CDC DeviceContext;
				const auto BackgroundColor {GetSysColor(drawItemStruct->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW)};
				DeviceContext.Attach(drawItemStruct->hDC);
				CBrush BackgroundBrush(BackgroundColor);
				DeviceContext.FillRect(rcItem, &BackgroundBrush);
				if (drawItemStruct->itemState & ODS_FOCUS) {
					DeviceContext.DrawFocusRect(rcItem);
				}
				const auto ItemID {gsl::narrow_cast<int>(drawItemStruct->itemID)};
				if (ItemID != -1) { // The text color is stored as the item data.
					const auto TextColor {drawItemStruct->itemState & ODS_SELECTED ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_WINDOWTEXT)};
					DeviceContext.SetBkColor(BackgroundColor);
					DeviceContext.SetTextColor(TextColor);
					for (auto labelIndex = 0; labelIndex < m_NumberOfColumns; ++labelIndex) {
						m_LayersList.GetSubItemRect(ItemID, labelIndex, LVIR_LABEL, rcItem);
						DrawItem(DeviceContext, ItemID, labelIndex, rcItem);
					}
				}
				DeviceContext.Detach();
			}
			break;
			case ODA_SELECT:
				InvertRect(drawItemStruct->hDC, &drawItemStruct->rcItem);
				break;
			case ODA_FOCUS:
				//::DrawFocusRect(drawItemStruct->hDC, &(drawItemStruct->rcItem));
				break;
			default: ;
		}
		return;
	}
	CDialog::OnDrawItem(controlIdentifier, drawItemStruct);
}

BOOL EoDlgFileManage::OnInitDialog() {
	CDialog::OnInitDialog();
	CString CaptionText;
	GetWindowTextW(CaptionText);
	SetWindowTextW(CaptionText + L" - " + m_Document->GetPathName());
	m_PreviewWindowHandle = GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd();
	m_LayersList.DeleteAllItems();
	m_LayersList.InsertColumn(kStatus, L"Status", LVCFMT_LEFT, 32);
	m_LayersList.InsertColumn(kName, L"Name", LVCFMT_LEFT, 96);
	m_LayersList.InsertColumn(kOn, L"On", LVCFMT_LEFT, 32);
	m_LayersList.InsertColumn(kFreeze, L"Freeze in all VP", LVCFMT_LEFT, 32);
	m_LayersList.InsertColumn(kLock, L"Lock", LVCFMT_LEFT, 32);
	m_LayersList.InsertColumn(kColor, L"Color", LVCFMT_LEFT, 96);
	m_LayersList.InsertColumn(kLinetype, L"Linetype", LVCFMT_LEFT, 96);
	m_LayersList.InsertColumn(kLineweight, L"Lineweight", LVCFMT_LEFT, 96);
	m_LayersList.InsertColumn(kPlotStyle, L"Plot Style", LVCFMT_LEFT, 64);
	m_NumberOfColumns = m_LayersList.InsertColumn(kPlot, L"Plot", LVCFMT_LEFT, 32);
	if (!m_Database->getTILEMODE()) { // Layout (not Model) tab is active
		m_ActiveViewport = m_Database->activeViewportId();
		m_LayersList.InsertColumn(kVpFreeze, L"VP Freeze", LVCFMT_LEFT, 32);
		m_LayersList.InsertColumn(kVpColor, L"VP Color", LVCFMT_LEFT, 96);
		m_LayersList.InsertColumn(kVpLinetype, L"VP Linetype", LVCFMT_LEFT, 96);
		m_LayersList.InsertColumn(kVpLineweight, L"VP Lineweight", LVCFMT_LEFT, 96);
		m_NumberOfColumns = m_LayersList.InsertColumn(kVpPlotStyle, L"Plot Style", LVCFMT_LEFT, 64);
	}
	m_Description = m_LayersList.InsertColumn(++m_NumberOfColumns, L"Description", LVCFMT_LEFT, 96);
	m_NumberOfColumns++;
	auto Layers = m_Document->LayerTable(OdDb::kForRead);
	for (auto LayerIndex = 0; LayerIndex < m_Document->GetLayerTableSize(); LayerIndex++) {
		const auto Layer {m_Document->GetLayerAt(LayerIndex)};
		m_LayersList.InsertItem(LayerIndex, Layer->Name());
		m_LayersList.SetItemData(LayerIndex, DWORD_PTR(Layer));
	}
	UpdateCurrentLayerInfoField();
	m_BlocksList.SetHorizontalExtent(512);
	CString BlockName;
	EoDbBlock* Block;
	auto Position {m_Document->GetFirstBlockPosition()};
	while (Position != nullptr) {
		m_Document->GetNextBlock(Position, BlockName, Block);
		if (!Block->IsAnonymous()) {
			const auto ItemIndex {m_BlocksList.AddString(BlockName)};
			m_BlocksList.SetItemData(ItemIndex, DWORD_PTR(Block));
		}
	}
	CBitmap Bitmap;
	Bitmap.LoadBitmapW(IDB_LAYER_STATES_HC);
	m_StateImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1);
	m_StateImages.Add(&Bitmap, RGB(0, 0, 128));
	WndProcPreviewClear(m_PreviewWindowHandle);
	return TRUE;
}

void EoDlgFileManage::OnItemChangedLayersListControl(NMHDR* notifyStructure, LRESULT* result) {
	const auto ListViewNotificationMessage = reinterpret_cast<tagNMLISTVIEW*>(notifyStructure);
	if ((ListViewNotificationMessage->uNewState & LVIS_FOCUSED) == LVFIS_FOCUSED) {
		const auto Item {ListViewNotificationMessage->iItem};
		auto Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(Item))};
		CString NumberOfGroups;
		NumberOfGroups.Format(L"%-4i", Layer->GetCount());
		m_Groups.SetWindowTextW(NumberOfGroups);
		EoDbPrimitive::SetLayerColorIndex(Layer->ColorIndex());
		EoDbPrimitive::SetLayerLinetypeIndex(Layer->LinetypeIndex());
		_WndProcPreviewUpdate(m_PreviewWindowHandle, m_Document->GetLayerAt(Layer->Name()));
	}
	*result = 0;
}

void EoDlgFileManage::OnLbnSelectionChangeBlocksList() {
	const auto CurrentSelection {m_BlocksList.GetCurSel()};
	if (CurrentSelection != LB_ERR) {
		if (m_BlocksList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString BlockName;
			m_BlocksList.GetText(CurrentSelection, BlockName);
			const auto Block {reinterpret_cast<EoDbBlock*>(m_BlocksList.GetItemData(CurrentSelection))};
			m_Groups.SetDlgItemInt(IDC_GROUPS, static_cast<unsigned>(Block->GetCount()), FALSE);
			WndProcPreviewUpdate(m_PreviewWindowHandle, Block);
		}
	}
}

void EoDlgFileManage::OnNmClickLayersListControl(NMHDR* notifyStructure, LRESULT* result) {
	const auto pNMItemActivate {reinterpret_cast<tagNMITEMACTIVATE*>(notifyStructure)};
	const auto Item {pNMItemActivate->iItem};
	const auto SubItem {pNMItemActivate->iSubItem};
	auto Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(Item))};
	auto LayerTableRecord {Layer->TableRecord()};
	m_ClickToColumnStatus = false;
	switch (SubItem) {
		case kStatus:
			m_ClickToColumnStatus = true;
			break;
		case kName:
			break;
		case kOn:
			if (Layer->IsCurrent()) {
				AeSys::WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, LayerTableRecord->getName());
			} else {
				Layer->SetIsOff(!Layer->IsOff());
			}
			break;
		case kFreeze:
			Layer->SetIsFrozen(!LayerTableRecord->isFrozen());
			break;
		case kLock:
			if (Layer->IsCurrent()) {
				AeSys::WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, LayerTableRecord->getName());
			} else {
				Layer->SetIsLocked(!Layer->IsLocked());
			}
			break;
		case kColor: {
			EoDlgSetupColor SetupColorDialog;
			SetupColorDialog.colorIndex = static_cast<unsigned short>(LayerTableRecord->colorIndex());
			if (SetupColorDialog.DoModal() == IDOK) {
				Layer->SetColorIndex(static_cast<short>(SetupColorDialog.colorIndex));
			}
			break;
		}
		case kLinetype: {
			OdDbLinetypeTablePtr Linetypes {m_Database->getLinetypeTableId().safeOpenObject()};
			EoDlgSetupLinetype Dialog(Linetypes);
			if (Dialog.DoModal() == IDOK) {
				Layer->SetLinetype(Dialog.linetype->objectId());
			}
			break;
		}
		case kLineweight: {
			EoDlgLineWeight Dialog(LayerTableRecord->lineWeight());
			if (Dialog.DoModal() == IDOK) {
				LayerTableRecord->upgradeOpen();
				LayerTableRecord->setLineWeight(Dialog.lineWeight);
				LayerTableRecord->downgradeOpen();
			}
			break;
		}
		case kPlotStyle:
			break;
		case kPlot:
			LayerTableRecord->upgradeOpen();
			LayerTableRecord->setIsPlottable(!LayerTableRecord->isPlottable());
			LayerTableRecord->downgradeOpen();
			break;
		case kVpFreeze: case kDescr:
			if (SubItem != m_Description) {
				auto pVp {OdDbViewport::cast(LayerTableRecord->database()->activeViewportId().safeOpenObject(OdDb::kForWrite))};
				//			if (pVp.get()) {
				//				OdDbObjectIdArray ids(1);
				//				ids.append(ItemData);
				//				if (pVp->isLayerFrozenInViewport(ItemData)) {
				//					pVp->thawLayersInViewport(ids);
				//				}
				//				else {
				//					pVp->freezeLayersInViewport(ids);
				//				}
				//			}
			} else { }
			break;
		case kVpColor:
			break;
		case kVpLinetype:
			break;
		case kVpLineweight: {
			EoDlgLineWeight dlg(LayerTableRecord->lineWeight());
			if (IDOK == dlg.DoModal()) {
				LayerTableRecord->upgradeOpen();
				LayerTableRecord->setLineWeight(dlg.lineWeight, m_ActiveViewport);
				LayerTableRecord->downgradeOpen();
			}
			break;
		}
		case kVpPlotStyle:
			break;
		default: ;
	}
	m_LayersList.Invalidate();
	*result = 0;
}

void EoDlgFileManage::OnNmDoubleClickLayersListControl(NMHDR* /*notifyStructure*/, LRESULT* result) {
	if (m_ClickToColumnStatus) {
		OnBnClickedSetCurrent();
	}
	*result = 0;
}

void EoDlgFileManage::UpdateCurrentLayerInfoField() {
	const auto LayerName {OdDbSymUtil::getSymbolName(m_Database->getCLAYER())};
	GetDlgItem(IDC_STATIC_CURRENT_LAYER)->SetWindowTextW(L"Current Layer: " + LayerName);
}

void EoDlgFileManage::OnLvnBeginLabelEditLayersListControl(NMHDR* const notifyStructure, LRESULT* result) {
	const NMLVDISPINFO* ListViewNotificationDisplayInfo = reinterpret_cast<NMLVDISPINFO*>(notifyStructure);
	const auto Item {ListViewNotificationDisplayInfo->item};
	const EoDbLayer* Layer = reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(Item.iItem));
	auto LayerTableRecord {Layer->TableRecord()};
	// <tas="Layer0 should be culled here instead of the EndlabeleditLayers."</tas>
	result = nullptr;
}

void EoDlgFileManage::OnLvnEndlabeleditLayersListControl(NMHDR* const notifyStructure, LRESULT* result) {
	const NMLVDISPINFO* ListViewNotificationDisplayInfo = reinterpret_cast<NMLVDISPINFO*>(notifyStructure);
	const auto Item {ListViewNotificationDisplayInfo->item};
	auto Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(Item.iItem))};
	auto LayerTableRecord {Layer->TableRecord()};
	const OdString NewName(Item.pszText);
	if (!NewName.isEmpty()) {
		auto Layers {m_Document->LayerTable(OdDb::kForWrite)};
		if (LayerTableRecord->objectId() == m_Database->getLayerZeroId()) {
			AeSys::WarningMessageBox(IDS_MSG_LAYER_NO_RENAME_0);
		} else {
			if (Layers->getAt(NewName).isNull()) {
				Layer->SetName(NewName);
				if (LayerTableRecord->objectId() == m_Database->getCLAYER()) {
					m_Document->SetCurrentLayer(LayerTableRecord);
					UpdateCurrentLayerInfoField();
				}
				m_LayersList.SetItemText(Item.iItem, 0, NewName);
			}
		}
	}
	result = nullptr;
}

void EoDlgFileManage::OnLvnKeydownLayersListControl(NMHDR* const notifyStructure, LRESULT* result) {
	const auto pLVKeyDow {reinterpret_cast<tagLVKEYDOWN*>(notifyStructure)};
	if (pLVKeyDow->wVKey == VK_DELETE) {
		const auto SelectionMark {m_LayersList.GetSelectionMark()};
		const auto Layer {reinterpret_cast<EoDbLayer*>(m_LayersList.GetItemData(SelectionMark))};
		auto LayerTableRecord {Layer->TableRecord()};
		const auto Name {Layer->Name()};
		const auto Result {LayerTableRecord->erase(true)};
		if (Result != 0) {
			auto ErrorDescription {m_Database->appServices()->getErrorDescription(Result)};
			ErrorDescription += L": <%s> layer can not be deleted";
			theApp.AddStringToMessageList(ErrorDescription, Name);
		} else {
			m_Document->UpdateLayerInAllViews(EoDb::kLayerErase, Layer);
			const auto LayerIndex {m_Document->FindLayerAt(Name)};
			m_Document->RemoveLayerAt(LayerIndex);
			m_LayersList.DeleteItem(SelectionMark);
			theApp.AddStringToMessageList(IDS_MSG_LAYER_ERASED, Name);
		}
	}
	*result = 0;
}
