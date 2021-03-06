#pragma once
class EoDlgEditProperties final : public CDialog {
DECLARE_DYNAMIC(EoDlgEditProperties)

	EoDlgEditProperties(OdDbObjectId& id, CWnd* parent = nullptr);

	virtual ~EoDlgEditProperties();

	enum { IDD = IDD_PROPERTIES };

protected:
	void DoDataExchange(CDataExchange* dataExchange) final;

	BOOL OnInitDialog() final;

	OdDbObjectId& m_pObjectId;
	OdResBufPtr m_ResourceBuffer;
	int m_CurrentItem;
	CButton m_SetValue;
	CListCtrl m_propList;
	CString m_sValue;

	void OnSetFocusValue();

	void OnButton();

	void OnClickProplist(NMHDR* notifyStructure, LRESULT* result);

	void OnKeydownProplist(NMHDR* notifyStructure, LRESULT* result);

DECLARE_MESSAGE_MAP()
};
