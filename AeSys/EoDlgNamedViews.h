#pragma once
class AeSysDoc;
class OdDbViewTableRecord;
using OdDbViewTableRecordPtr = OdSmartPtr<OdDbViewTableRecord>;

class CNamedViewListCtrl final : public CListCtrl {
	void setViewId(int item, const OdDbObjectId& id);

	void setView(int item, const OdDbViewTableRecord* view);

public:
	[[nodiscard]] OdDbObjectId viewId(int item) const;

	OdDbViewTableRecordPtr view(int item) const;

	OdDbViewTableRecordPtr selectedView();

	void InsertItem(int i, const OdDbViewTableRecord* pView);
};

class EoDlgNamedViews final : public CDialog {
	AeSysDoc* m_pDoc;
public:
	EoDlgNamedViews(AeSysDoc* pDoc, CWnd* parent = nullptr);

	enum { kUnchangedItem = 0, kNewItem = 1, kReplace = 2 };

	AeSysDoc* document() const noexcept {
		return m_pDoc;
	}

	OdDbDatabase* database();

	enum { IDD = IDD_DIALOG_NAMED_VIEWS };

	CNamedViewListCtrl m_views;
protected:
	void DoDataExchange(CDataExchange* dataExchange) final;

	BOOL OnInitDialog() final;

	void OnSetcurrentButton();

	void OnDoubleClickNamedviews(NMHDR* notifyStructure, LRESULT* pResult);

	void OnNewButton();

	void OnUpdateLayersButton();

	void OnDeleteButton();

DECLARE_MESSAGE_MAP()
};
