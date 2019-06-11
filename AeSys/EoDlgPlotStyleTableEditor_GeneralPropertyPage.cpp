#include "stdafx.h"

#include "Shlwapi.h"
#include "EoDlgPlotStyleTableEditor_GeneralPropertyPage.h"

IMPLEMENT_DYNCREATE(EoDlgPlotStyleEditor_GeneralPropertyPage, CPropertyPage)

EoDlgPlotStyleEditor_GeneralPropertyPage::EoDlgPlotStyleEditor_GeneralPropertyPage() :
	CPropertyPage(EoDlgPlotStyleEditor_GeneralPropertyPage::IDD), m_pPlotStyleTable(nullptr) {
}

EoDlgPlotStyleEditor_GeneralPropertyPage::~EoDlgPlotStyleEditor_GeneralPropertyPage() {
}

void EoDlgPlotStyleEditor_GeneralPropertyPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PS_GENERAL_EDIT_DESCRIPTION, m_editDescription);
	DDX_Control(pDX, IDC_PS_GENERAL_EDIT_SCALE_FACTOR, m_editScalefactor);
	DDX_Control(pDX, IDC_PS_GENERAL_CHECK_SCALE_FACTOR, m_checkScalefactor);
	DDX_Control(pDX, IDC_PS_GENERAL_STATIC_FILEPATH, m_staticFilepath);
	DDX_Control(pDX, IDC_PS_GENERAL_STATIC_FILE_NAME, m_staticFilename);
	DDX_Control(pDX, IDC_PS_GENERAL_STATIC_BITMAP, m_staticBitmap);
	DDX_Control(pDX, IDC_PS_GENERAL_STATIC_REGULAR, m_staticRegular);
}

BEGIN_MESSAGE_MAP(EoDlgPlotStyleEditor_GeneralPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_PS_GENERAL_EDIT_DESCRIPTION, OnChangeEditDescription)
	ON_BN_CLICKED(IDC_PS_GENERAL_CHECK_SCALE_FACTOR, OnCheckScalefactor)
	ON_EN_CHANGE(IDC_PS_GENERAL_EDIT_SCALE_FACTOR, OnEditScalefactor)

END_MESSAGE_MAP()

void DrawTransparentBitmap(HDC hdc, HBITMAP bitmap, short xStart, short yStart, COLORREF transparentColor) noexcept {
	auto hdcTemp {CreateCompatibleDC(hdc)};

	SelectObject(hdcTemp, bitmap);

	BITMAP bm;

	GetObjectW(bitmap, sizeof(BITMAP), (LPSTR) & bm);
	POINT ptSize {bm.bmWidth, bm.bmHeight};
	DPtoLP(hdcTemp, &ptSize, 1);

	auto hdcBack {CreateCompatibleDC(hdc)};
	auto hdcObject {CreateCompatibleDC(hdc)};
	auto hdcMem {CreateCompatibleDC(hdc)};
	auto hdcSave {CreateCompatibleDC(hdc)};

	auto bmAndBack {CreateBitmap(ptSize.x, ptSize.y, 1, 1, nullptr)};
	auto bmAndObject {CreateBitmap(ptSize.x, ptSize.y, 1, 1, nullptr)};
	auto bmAndMem {CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y)};
	auto bmSave {CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y)};

	auto bmBackOld {static_cast<HBITMAP>(SelectObject(hdcBack, bmAndBack))};
	auto bmObjectOld {static_cast<HBITMAP>(SelectObject(hdcObject, bmAndObject))};
	auto bmMemOld {static_cast<HBITMAP>(SelectObject(hdcMem, bmAndMem))};
	auto bmSaveOld {static_cast<HBITMAP>(SelectObject(hdcSave, bmSave))};

	SetMapMode(hdcTemp, GetMapMode(hdc));

	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	const auto BackgroundColor {SetBkColor(hdcTemp, transparentColor)};

	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);
	SetBkColor(hdcTemp, BackgroundColor);
	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart, SRCCOPY);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
	BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(hdcBack, bmBackOld));
	DeleteObject(SelectObject(hdcObject, bmObjectOld));
	DeleteObject(SelectObject(hdcMem, bmMemOld));
	DeleteObject(SelectObject(hdcSave, bmSaveOld));

	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
}

const bool EoDlgPlotStyleEditor_GeneralPropertyPage::SetPlotStyleTable(OdPsPlotStyleTable* pPlotStyleTable) noexcept {
	if (!pPlotStyleTable) {
		return false;
	}
	m_pPlotStyleTable = pPlotStyleTable;
	return true;
}

void WinPathToDos(wchar_t* str) {
	CString pStr = str;
	CString sNewStr;
	int pos = 0;
	while (pos >= 0) {

		pos = pStr.Find('\\');
		CString s;
		if (pos < 0) {
			s = pStr;
		} else {
			s = pStr.Left(pos);
			if (s.GetLength() > 8) {
				s = s.Left(6);
				s.MakeUpper();
				s += L"~1";
		//        s += "\\";
			}
			s += L"\\";
			pStr = pStr.Right(pStr.GetLength() - pos - 1);
		}

		sNewStr += s;
	}

	wcscpy(str, sNewStr.GetBuffer(sNewStr.GetLength()));
	sNewStr.ReleaseBuffer();
}

BOOL EoDlgPlotStyleEditor_GeneralPropertyPage::OnInitDialog() {
	CPropertyPage::OnInitDialog();

	if (!m_pPlotStyleTable) { return FALSE; }

	auto description {m_pPlotStyleTable->description()};
	m_editDescription.SetWindowTextW(description);

	const bool check {m_pPlotStyleTable->isApplyScaleFactor()};
	m_checkScalefactor.SetCheck(check);
	m_editScalefactor.EnableWindow(check);
	OdString sScaleFactor;
	sScaleFactor.format(L"%.1f", m_pPlotStyleTable->scaleFactor());
	m_editScalefactor.SetWindowTextW(sScaleFactor);

	auto editDC {::GetDC(m_staticFilepath.m_hWnd)};
  //  CRect rect;
  //  m_staticFilepath.GetClientRect(&rect);
	wchar_t buffer[MAX_PATH];
	wcscpy(buffer, m_sFileBufPath);
	WinPathToDos(buffer);
	wchar_t* lpStr = buffer;
	PathCompactPathW(editDC, lpStr, 630/*rect.right*/);
	m_staticFilepath.SetWindowTextW(lpStr);

	auto sFileName {m_sFileBufPath.right(m_sFileBufPath.getLength() - m_sFileBufPath.reverseFind('\\') - 1)};
	m_staticFilename.SetWindowTextW(sFileName);

	if (m_pPlotStyleTable->isAciTableAvailable()) { m_staticRegular.SetWindowTextW(L"Legacy (can be used to import old DWGs)"); }

	auto BitmapHandle {static_cast<HBITMAP>(::LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(m_pPlotStyleTable->isAciTableAvailable() ? IDB_PS_BITMAP_GENERAL_CTB : IDB_PS_BITMAP_GENERAL_STB), IMAGE_BITMAP, 32, 32, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS))};

	CClientDC ClientDeviceContext(&m_staticBitmap);
	DrawTransparentBitmap(ClientDeviceContext.m_hDC, BitmapHandle, 0, 0, 0x00FFFFFF);
	m_staticBitmap.SetBitmap(BitmapHandle);

	return TRUE;

}

void EoDlgPlotStyleEditor_GeneralPropertyPage::SetFileBufPath(const OdString sFilePath) {
	m_sFileBufPath = sFilePath;
}

void EoDlgPlotStyleEditor_GeneralPropertyPage::OnChangeEditDescription() {
	CString pVal;
	m_editDescription.GetWindowText(pVal);
	m_pPlotStyleTable->setDescription(OdString(pVal));
}

void EoDlgPlotStyleEditor_GeneralPropertyPage::OnCheckScalefactor() {
	const int check = m_checkScalefactor.GetCheck();
	m_pPlotStyleTable->setApplyScaleFactor(check ? true : false);
	m_editScalefactor.EnableWindow(check);
}

void EoDlgPlotStyleEditor_GeneralPropertyPage::OnEditScalefactor() {
	CString pVal;
	m_editScalefactor.GetWindowText(pVal);
	double scaleFactor;
	swscanf(pVal, L"%lf", &scaleFactor);
	
	if (scaleFactor <= 0 || scaleFactor > PS_EDIT_MAX_SCALEFACTOR) {
		scaleFactor = 0.01;
		m_editScalefactor.SetWindowTextW(L"0.01");
	}
	m_pPlotStyleTable->setScaleFactor(scaleFactor);
}

void EoDlgPlotStyleEditor_GeneralPropertyPage::OnOK() {
	CPropertyPage::OnOK();
}
