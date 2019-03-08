#pragma once

#include "EoCtrlColorsButton.h"

class EoDlgSetupColor : public CDialog {
	DECLARE_DYNAMIC(EoDlgSetupColor)

public:

	EoDlgSetupColor(CWnd* pParent = NULL);
	virtual ~EoDlgSetupColor();

// Dialog Data
	enum { IDD = IDD_SETUP_COLOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* result);

	EoCtrlColorsButton m_EvenColorsButton;
	EoCtrlColorsButton m_OddColorsButton;
	EoCtrlColorsButton m_NamedColorsButton;
	EoCtrlColorsButton m_GraysButton;
	EoCtrlColorsButton m_SelectionButton;

	CEdit m_ColorEditControl;

	void DrawSelectionInformation(EoUInt16 index);

public:
	EoUInt16 m_ColorIndex;

	afx_msg void OnBnClickedByblockButton();
	afx_msg void OnBnClickedBylayerButton();
	afx_msg void OnClickedEvenColors();
	afx_msg void OnClickedGrays();
	afx_msg void OnClickedNamedColors();
	afx_msg void OnClickedOddColors();
	afx_msg void OnChangeColorEdit();

protected:
	DECLARE_MESSAGE_MAP()
};