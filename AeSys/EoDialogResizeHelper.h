#pragma once
#include <list>

class EoDialogResizeHelper {
public:
	// fix horizontal dimension/position
	enum EHFix { kNoHFix = 0, kWidth = 1, kLeft = 2, kRight = 4, kWidthLeft = 3, kWidthRight = 5, kLeftRight = 6 };
	// fix vertical dimension/position
	enum EVFix { kNoVFix = 0, kHeight = 1, kTop = 2, kBottom = 4, kHeightTop = 3, kHeightBottom = 5, kTopBottom = 6 };

	// initialize with parent window, all child windows must already have their original position/size
	void Init(HWND parent);

	// explicitly add a window to the list of child windows (e.g. a sibling window)
	// Note: you've got to call Init() before you can add a window
	void Add(HWND a_hWnd);

	// fix position/dimension for a child window, determine child by...
	// ...HWND...
	BOOL Fix(HWND a_hCtrl, EHFix a_hFix, EVFix a_vFix);
	// ...item ID (if it's a dialog item)...
	BOOL Fix(int a_itemId, EHFix a_hFix, EVFix a_vFix);
	// ...all child windows with a common class name (e.g. "Edit")
	unsigned Fix(const wchar_t* a_pszClassName, EHFix a_hFix, EVFix a_vFix);
	// ...or all registered windows
	BOOL Fix(EHFix a_hFix, EVFix a_vFix);

	// resize child windows according to changes of parent window and fix attributes
	void OnSize();

private:
	struct CtrlSize {
		CRect m_origSize;
		HWND m_hCtrl {nullptr};
		EHFix m_hFix {kNoHFix};
		EVFix m_vFix {kNoVFix};

		CtrlSize() = default;
	};

	using CtrlCont_t = std::list<CtrlSize>;
	CtrlCont_t m_Controls;
	HWND m_ParentWindow;
	CRect m_OriginalParentSize;
};
