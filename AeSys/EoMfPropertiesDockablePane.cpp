#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"
#include "EoMfPropertiesDockablePane.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
std::vector<const wchar_t*> EoMfPropertiesDockablePane::ms_TabsStyles {L"None", L"Standard", L"Grouped"};
std::vector<const wchar_t*> EoMfPropertiesDockablePane::ms_TabsLocations {L"On Bottom", L"On Top"};
#pragma warning(push)
#pragma warning(disable : 4191) // (level 3) 'operator': unsafe conversion from 'type_of_expression' to 'type_required'
BEGIN_MESSAGE_MAP(EoMfPropertiesDockablePane, CDockablePane)
		ON_WM_CREATE()
		ON_WM_SETFOCUS()
		ON_WM_SETTINGCHANGE()
		ON_WM_SIZE()
		ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
		ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
		ON_COMMAND(ID_PROPERTIES1, OnProperties1)
		ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
		ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
		ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
		ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
END_MESSAGE_MAP()
#pragma warning (pop)
void EoMfPropertiesDockablePane::AdjustLayout() {
	if (GetSafeHwnd() == nullptr || AfxGetMainWnd() != nullptr && AfxGetMainWnd()->IsIconic() != 0) {
		return;
	}
	CRect ClientRectangle;
	CRect ComboRectangle;
	GetClientRect(ClientRectangle);
	m_ComboBox.GetWindowRect(&ComboRectangle);
	const int cyTlb = m_PropertiesToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	m_ComboBox.SetWindowPos(nullptr, ClientRectangle.left, ClientRectangle.top, ClientRectangle.Width(), m_ComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	m_PropertiesToolBar.SetWindowPos(nullptr, ClientRectangle.left, ClientRectangle.top + m_ComboHeight, ClientRectangle.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_PropertyGrid.SetWindowPos(nullptr, ClientRectangle.left, ClientRectangle.top + m_ComboHeight + cyTlb, ClientRectangle.Width(), ClientRectangle.Height() - (m_ComboHeight + cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int EoMfPropertiesDockablePane::OnCreate(const LPCREATESTRUCT createStructure) {
	if (CDockablePane::OnCreate(createStructure) == -1) {
		return -1;
	}
	CRect EmptyRectangle;
	EmptyRectangle.SetRectEmpty();
	if (m_ComboBox.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, EmptyRectangle, this, 1) == 0) {
		TRACE0("Failed to create Properties Combo\n");
		return -1;
	}
	m_ComboBox.AddString(L"Application");
	m_ComboBox.AddString(L"Persistent");
	m_ComboBox.SetFont(CFont::FromHandle(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))), TRUE);
	m_ComboBox.SetCurSel(0);
	CRect ComboRectangle;
	m_ComboBox.GetClientRect(&ComboRectangle);
	m_ComboHeight = ComboRectangle.Height();
	if (m_PropertyGrid.Create(WS_VISIBLE | WS_CHILD, EmptyRectangle, this, 2) == 0) {
		TRACE0("Failed to create Properties Grid \n");
		return -1;
	}
	InitializePropertyGrid();
	SetWorkspaceTabsSubItemsState();
	m_PropertiesToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_PropertiesToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE);
	m_PropertiesToolBar.CleanUpLockedImages();
	m_PropertiesToolBar.LoadBitmapW(static_cast<unsigned>(theApp.HighColorMode() ? IDB_PROPERTIES_HC : IDR_PROPERTIES), 0, 0, TRUE);
	m_PropertiesToolBar.SetPaneStyle(m_PropertiesToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_PropertiesToolBar.SetPaneStyle(m_PropertiesToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_PropertiesToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_PropertiesToolBar.SetRouteCommandsViaFrame(FALSE);
	AdjustLayout();
	return 0;
}

void EoMfPropertiesDockablePane::OnSize(const unsigned type, const int cx, const int cy) {
	CDockablePane::OnSize(type, cx, cy);
	AdjustLayout();
}

void EoMfPropertiesDockablePane::OnExpandAllProperties() {
	m_PropertyGrid.ExpandAll();
}

void EoMfPropertiesDockablePane::OnUpdateExpandAllProperties(CCmdUI* /*commandUserInterface*/) noexcept {}

void EoMfPropertiesDockablePane::OnSortProperties() {
	m_PropertyGrid.SetAlphabeticMode(static_cast<BOOL>(m_PropertyGrid.IsAlphabeticMode() == 0));
}

void EoMfPropertiesDockablePane::OnUpdateSortProperties(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(m_PropertyGrid.IsAlphabeticMode());
}

void EoMfPropertiesDockablePane::OnProperties1() noexcept {
	// TODO: Add your command handler code here
}

void EoMfPropertiesDockablePane::OnUpdateProperties1(CCmdUI* /*commandUserInterface*/) noexcept {
	// TODO: Add your command update UI handler code here
}

void EoMfPropertiesDockablePane::InitializePropertyGrid() {
	SetPropertyGridFont();
	m_PropertyGrid.EnableHeaderCtrl(FALSE, L"Property", L"Value");
	m_PropertyGrid.EnableDescriptionArea();
	m_PropertyGrid.SetVSDotNetLook();
	m_PropertyGrid.SetGroupNameFullWidth();
	m_PropertyGrid.MarkModifiedProperties();
	auto WorkspaceTabsGroup {new CMFCPropertyGridProperty(L"Workspace Tabs")};
	auto TabsStyle {new CMFCPropertyGridProperty(L"Tabs Style", L"", L"Set the Tabs Style to None, Standard, or Grouped", kTabsStyle)};
	TabsStyle->AddOption(ms_TabsStyles[0]);
	TabsStyle->AddOption(ms_TabsStyles[1]);
	TabsStyle->AddOption(ms_TabsStyles[2]);
	TabsStyle->SetValue(ms_TabsStyles.at(theApp.applicationOptions.tabsStyle));
	TabsStyle->AllowEdit(FALSE);
	WorkspaceTabsGroup->AddSubItem(TabsStyle);
	auto TabLocation {new CMFCPropertyGridProperty(L"Tab Location", L"", L"Set the Tab Location to Top or Bottom", kTabLocation)};
	TabLocation->AddOption(ms_TabsLocations[0]);
	TabLocation->AddOption(ms_TabsLocations[1]);
	TabLocation->SetValue(ms_TabsLocations.at(theApp.applicationOptions.mdiTabInfo.m_tabLocation));
	TabLocation->AllowEdit(FALSE);
	WorkspaceTabsGroup->AddSubItem(TabLocation);
	COleVariant TabsAutoColor(static_cast<short>(theApp.applicationOptions.mdiTabInfo.m_bAutoColor == TRUE), VT_BOOL);
	WorkspaceTabsGroup->AddSubItem(new CMFCPropertyGridProperty(L"Tabs auto-color", TabsAutoColor, L"Set Workspace Tabs to use automatic color", kTabsAutoColor));
	COleVariant TabIcons(static_cast<short>(theApp.applicationOptions.mdiTabInfo.m_bTabIcons == TRUE), VT_BOOL);
	WorkspaceTabsGroup->AddSubItem(new CMFCPropertyGridProperty(L"Tab icons", TabIcons, L"Show document icons on Workspace Tabs", kTabIcons));
	COleVariant TabBorderSize(static_cast<short>(theApp.applicationOptions.mdiTabInfo.m_nTabBorderSize), VT_I2);
	auto BorderSize {new CMFCPropertyGridProperty(L"Border Size", TabBorderSize, L"Set Workspace border size from 0 to 8 pixels", kTabBorderSize)};
	BorderSize->EnableSpinControl(TRUE, 0, 8);
	BorderSize->AllowEdit(FALSE);
	WorkspaceTabsGroup->AddSubItem(BorderSize);
	m_PropertyGrid.AddProperty(WorkspaceTabsGroup);
	auto ActiveView {AeSysView::GetActiveView()};
	const auto Scale {ActiveView != nullptr ? ActiveView->WorldScale() : 1.0};
	auto ActiveViewGroup {new CMFCPropertyGridProperty(L"Active View")};
	auto WorldScaleProperty {new CMFCPropertyGridProperty(L"World Scale", static_cast<_variant_t>(Scale), L"Specifies the world scale used in the Active View", kActiveViewScale)};
	ActiveViewGroup->AddSubItem(WorldScaleProperty);
	ActiveViewGroup->AddSubItem(new CMFCPropertyGridProperty(L"Use True Type fonts", static_cast<_variant_t>(true), L"Specifies that the Active View uses True Type fonts"));
	m_PropertyGrid.AddProperty(ActiveViewGroup);
	WorldScaleProperty->Enable(static_cast<BOOL>(ActiveView != nullptr));
	auto AppearanceGroup {new CMFCPropertyGridProperty(L"Appearance")};
	AppearanceGroup->AddSubItem(new CMFCPropertyGridProperty(L"3D Look", static_cast<_variant_t>(false), L"Specifies the window's font will be non-bold and controls will have a 3D border"));
	auto LengthUnits {new CMFCPropertyGridProperty(L"Length Units", L"Engineering", L"Specifies the units used to display lengths")};
	LengthUnits->AddOption(L"Architectural");
	LengthUnits->AddOption(L"Engineering");
	LengthUnits->AddOption(L"Feet");
	LengthUnits->AddOption(L"Inches");
	LengthUnits->AddOption(L"Meters");
	LengthUnits->AddOption(L"Millimeters");
	LengthUnits->AddOption(L"Centimeters");
	LengthUnits->AddOption(L"Decimeters");
	LengthUnits->AddOption(L"Kilometers");
	LengthUnits->AllowEdit(FALSE);
	AppearanceGroup->AddSubItem(LengthUnits);
	auto LengthPrecision {new CMFCPropertyGridProperty(L"Length Precision", static_cast<_variant_t>(8l), L"Specifies the precision used to display lengths")};
	LengthPrecision->EnableSpinControl(TRUE, 0, 256);
	AppearanceGroup->AddSubItem(LengthPrecision);
	AppearanceGroup->AddSubItem(new CMFCPropertyGridProperty(L"Caption", static_cast<_variant_t>(L"About"), L"Specifies the text that will be displayed in the window's title bar"));
	m_PropertyGrid.AddProperty(AppearanceGroup);
	auto PointGrid {new CMFCPropertyGridProperty(L"Point Grid", 0, TRUE)};
	auto pProp {new CMFCPropertyGridProperty(L"X", static_cast<_variant_t>(3.0), L"Specifies the point grid x spacing")};
	PointGrid->AddSubItem(pProp);
	pProp = new CMFCPropertyGridProperty(L"Y", static_cast<_variant_t>(3.0), L"Specifies the point grid y spacing");
	PointGrid->AddSubItem(pProp);
	pProp = new CMFCPropertyGridProperty(L"Z", static_cast<_variant_t>(0.0), L"Specifies the point grid z spacing");
	PointGrid->AddSubItem(pProp);
	m_PropertyGrid.AddProperty(PointGrid);
	auto NoteGroup {new CMFCPropertyGridProperty(L"Note")};
	auto Font {CFont::FromHandle(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)))};
	LOGFONT FontAttributes;
	Font->GetLogFont(&FontAttributes);
	wcscpy_s(FontAttributes.lfFaceName, LF_FACESIZE, L"Arial");
	NoteGroup->AddSubItem(new CMFCPropertyGridFontProperty(L"Font", FontAttributes, CF_EFFECTS | CF_SCREENFONTS, L"Specifies the default font for the window"));
	NoteGroup->AddSubItem(new CMFCPropertyGridProperty(L"Use System Font", static_cast<_variant_t>(true), L"Specifies that the window uses MS Shell Dlg font"));
	auto HorizontalAlignment {new CMFCPropertyGridProperty(L"Horizontal Alignment", L"Left", L"Specifies the horizontal alignment used for new notes")};
	HorizontalAlignment->AddOption(L"Left");
	HorizontalAlignment->AddOption(L"Center");
	HorizontalAlignment->AddOption(L"Right");
	HorizontalAlignment->AllowEdit(FALSE);
	NoteGroup->AddSubItem(HorizontalAlignment);
	auto VerticalAlignment {new CMFCPropertyGridProperty(L"Vertical Alignment", L"Bottom", L"Specifies the vertical alignment used for new notes")};
	VerticalAlignment->AddOption(L"Bottom");
	VerticalAlignment->AddOption(L"Middle");
	VerticalAlignment->AddOption(L"Top");
	VerticalAlignment->AllowEdit(FALSE);
	NoteGroup->AddSubItem(VerticalAlignment);
	auto Path {new CMFCPropertyGridProperty(L"Path", L"Right", L"Specifies the text path used for new notes")};
	Path->AddOption(L"Right");
	Path->AddOption(L"Left");
	Path->AddOption(L"Up");
	Path->AddOption(L"Down");
	Path->AllowEdit(FALSE);
	NoteGroup->AddSubItem(Path);
	m_PropertyGrid.AddProperty(NoteGroup);
	auto MiscGroup {new CMFCPropertyGridProperty(L"Misc")};
	pProp = new CMFCPropertyGridProperty(L"(Name)", L"Application");
	pProp->Enable(FALSE);
	MiscGroup->AddSubItem(pProp);
	auto ColorProperty {new CMFCPropertyGridColorProperty(L"Window Color", RGB(210, 192, 254), nullptr, L"Specifies the default window color")};
	ColorProperty->EnableOtherButton(L"Other...");
	ColorProperty->EnableAutomaticButton(L"Default", GetSysColor(COLOR_3DFACE));
	MiscGroup->AddSubItem(ColorProperty);
	static wchar_t BASED_CODE szFilter[] = L"Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||";
	MiscGroup->AddSubItem(new CMFCPropertyGridFileProperty(L"Icon", TRUE, L"", L"ico", 0, szFilter, L"Specifies the window icon"));
	MiscGroup->AddSubItem(new CMFCPropertyGridFileProperty(L"Shadow Folder Path", theApp.ShadowFolderPath(), 0));
	m_PropertyGrid.AddProperty(MiscGroup);
}

void EoMfPropertiesDockablePane::OnSetFocus(CWnd* oldWindow) {
	CDockablePane::OnSetFocus(oldWindow);
	m_PropertyGrid.SetFocus();
}

void EoMfPropertiesDockablePane::OnSettingChange(const unsigned flags, const wchar_t* section) {
	CDockablePane::OnSettingChange(flags, section);
	SetPropertyGridFont();
}

void EoMfPropertiesDockablePane::SetPropertyGridFont() {
	DeleteObject(m_PropertyGridFont.Detach());
	LOGFONT FontAttributes;
	afxGlobalData.fontRegular.GetLogFont(&FontAttributes);
	NONCLIENTMETRICS Info;
	Info.cbSize = sizeof Info;
	afxGlobalData.GetNonClientMetrics(Info);
	FontAttributes.lfHeight = Info.lfMenuFont.lfHeight;
	FontAttributes.lfWeight = Info.lfMenuFont.lfWeight;
	FontAttributes.lfItalic = Info.lfMenuFont.lfItalic;
	m_PropertyGridFont.CreateFontIndirectW(&FontAttributes);
	m_PropertyGrid.SetFont(&m_PropertyGridFont);
}

LRESULT EoMfPropertiesDockablePane::OnPropertyChanged(WPARAM, const LPARAM lparam) {
	const auto Property {reinterpret_cast<CMFCPropertyGridProperty*>(lparam)};
	auto ResetMDIChild {FALSE};
	switch (Property->GetData()) {
		case kTabsStyle: {
			const CString TabStyle {Property->GetValue().bstrVal};
			ResetMDIChild = TRUE;
			for (auto TabStylesIterator = ms_TabsStyles.begin(); TabStylesIterator != ms_TabsStyles.end(); TabStylesIterator++) {
				if (TabStyle == *TabStylesIterator) {
					if (*TabStylesIterator == ms_TabsStyles.at(0)) {
						theApp.applicationOptions.tabsStyle = EoApOptions::kNone;
					} else if (*TabStylesIterator == ms_TabsStyles.at(1)) {
						theApp.applicationOptions.tabsStyle = EoApOptions::kStandard;
					} else {
						theApp.applicationOptions.tabsStyle = EoApOptions::kGrouped;
					}
				}
			}
			SetWorkspaceTabsSubItemsState();
			break;
		}
		case kTabLocation: {
			const CString TabLocation {Property->GetValue().bstrVal};
			theApp.applicationOptions.mdiTabInfo.m_tabLocation = TabLocation == ms_TabsLocations.at(0) ? CMFCTabCtrl::LOCATION_BOTTOM : CMFCTabCtrl::LOCATION_TOP;
			break;
		}
		case kTabsAutoColor:
			theApp.applicationOptions.mdiTabInfo.m_bAutoColor = static_cast<BOOL>(Property->GetValue().boolVal == VARIANT_TRUE);
			break;
		case kTabIcons:
			theApp.applicationOptions.mdiTabInfo.m_bTabIcons = static_cast<BOOL>(Property->GetValue().boolVal == VARIANT_TRUE);
			break;
		case kTabBorderSize: {
			const int Border {Property->GetValue().iVal};
			theApp.applicationOptions.mdiTabInfo.m_nTabBorderSize = min(8, max(0, Border));
			break;
		}
		case kActiveViewScale: {
			auto ActiveView {AeSysView::GetActiveView()};
			ActiveView->SetWorldScale(Property->GetValue().dblVal);
			ActiveView->UpdateStateInformation(AeSysView::kScale);
			return 0;
		}
		default: ;
	}
	AeSys::UpdateMdiTabs(ResetMDIChild);
	return 0;
}

void EoMfPropertiesDockablePane::SetWorkspaceTabsSubItemsState() {
	for (auto i = 0; i < m_PropertyGrid.GetPropertyCount(); i++) {
		const auto PropertyGridProperty {m_PropertyGrid.GetProperty(i)};
		ASSERT_VALID(PropertyGridProperty);
		if (wcscmp(PropertyGridProperty->GetName(), L"Workspace Tabs") == 0) {
			for (auto SubItemIndex = 1; SubItemIndex < PropertyGridProperty->GetSubItemsCount(); SubItemIndex++) {
				auto SubProperty {PropertyGridProperty->GetSubItem(SubItemIndex)};
				ASSERT_VALID(SubProperty);
				SubProperty->Enable(static_cast<BOOL>(theApp.applicationOptions.tabsStyle != EoApOptions::kNone));
			}
		}
	}
	if (m_PropertyGrid.GetSafeHwnd() != nullptr) {
		m_PropertyGrid.RedrawWindow();
	}
}
