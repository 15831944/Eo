#pragma once

// EoCtrlColorsButton

class EoCtrlColorsButton : public CMFCButton {
	DECLARE_DYNAMIC(EoCtrlColorsButton)

	static COLORREF* m_Palette;
	static EoUInt16 m_CurrentIndex;
	static EoUInt16 m_SelectedIndex;

	enum Layouts {
		SimpleSingleRow,
		GridDown5RowsOddOnly,
		GridUp5RowsEvenOnly
	};
	Layouts m_Layout;
	CSize m_CellSize;
	CSize m_CellSpacing;
	CSize m_Margins;
	EoUInt16 m_BeginIndex;
	EoUInt16 m_EndIndex;
	EoUInt16 m_SubItem;

	void DrawCell(CDC* deviceContext, EoUInt16 index, COLORREF color);
	EoUInt16 SubItemByPoint(const CPoint& point);
	void SubItemRectangleByIndex(EoUInt16 index, CRect& rectangle);

public:

	EoCtrlColorsButton();

	virtual ~EoCtrlColorsButton();

	static void SetCurrentIndex(const EoUInt16 index) {
		m_CurrentIndex = index;
	}
	static void SetPalette(COLORREF* palette) {
		m_Palette = palette;
	}
	void SetLayout(Layouts layout, const CSize& cellSize) {
		m_Layout = layout;
		m_CellSize = cellSize;
	}
	void SetSequenceRange(const EoUInt16 beginIndex, const EoUInt16 endIndex) {
		m_BeginIndex = beginIndex;
		m_EndIndex = endIndex;
	}

	virtual void OnDraw(CDC* deviceContext, const CRect& rectangle, UINT state);
	virtual CSize SizeToContent(BOOL calculateOnly = FALSE);

	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT keyCode, UINT repeatCount, UINT flags);
	afx_msg void OnLButtonUp(UINT flags, CPoint point);
	afx_msg void OnMouseMove(UINT flags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* oldWindow);

protected:

	DECLARE_MESSAGE_MAP()
};
