#pragma once

// EoDlgSetScale dialog
class EoDlgSetScale : public CDialog {
DECLARE_DYNAMIC(EoDlgSetScale)
	EoDlgSetScale(CWnd* parent = nullptr);
	virtual ~EoDlgSetScale();

	// Dialog Data
	enum { IDD = IDD_SET_SCALE };

protected:
	void DoDataExchange(CDataExchange* pDX) final;
public:
	double m_Scale;
protected:
DECLARE_MESSAGE_MAP()
};
