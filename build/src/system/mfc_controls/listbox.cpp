#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{
// CCGListBox
IMPLEMENT_DYNAMIC(CCGListBox, CListBox)

CCGListBox::CCGListBox() noexcept
{
}

CCGListBox::~CCGListBox() noexcept
{
}


BEGIN_MESSAGE_MAP(CCGListBox, CListBox)
END_MESSAGE_MAP()

BOOL CCGListBox::PreCreateWindow(CREATESTRUCT& cs)
{
	// 1) HasString과 OwnerdrawVariable을 추가한다.
	cs.style	|= (LBS_HASSTRINGS | LBS_OWNERDRAWVARIABLE | LBS_EXTENDEDSEL);

	// 2) CListBox의 원래 PreCreateWindow
	return CListBox::PreCreateWindow(cs);
}

void CCGListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	// check) 
	RETURN_IF(static_cast<int>(lpDIS->itemID) < 0);

	// 1) DC를 얻는다.
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// 2) ...
	COLORREF crNorm = (COLORREF)lpDIS->itemData;


	//-----------------------------------------------------------------
	// 1. 배경을 그린다.
	//-----------------------------------------------------------------
	// 1) 선택된 경우 배경을 칠한다.
	if ((lpDIS->itemState & ODS_SELECTED) && (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		//CBrush brush(crNorm);
		CBrush brush(::GetSysColor(COLOR_HIGHLIGHT));		
		pDC->FillRect(&lpDIS->rcItem, &brush);
	}

	// 2) 선택되지 않았을 경우 윈도우 기본 색으로 채운다.
	if (!(lpDIS->itemState & ODS_SELECTED) &&	(lpDIS->itemAction & ODA_SELECT))
	{
		CBrush brush(::GetSysColor(COLOR_WINDOW));
		pDC->FillRect(&lpDIS->rcItem, &brush);
	}	 	


	//-----------------------------------------------------------------
	// 2. 사각을 그린다.
	//-----------------------------------------------------------------
	// 1) 현재 Focus된 상태라면 사각 외곽선을 그린다.
	if ((lpDIS->itemAction & ODA_FOCUS) && (lpDIS->itemState & ODS_FOCUS))
	{
		pDC->DrawFocusRect(&lpDIS->rcItem); 
	}

	// 2) Focus가 되지 않았을 경우 사각 외곽선을 지운다.
	if ((lpDIS->itemAction & ODA_FOCUS) &&	!(lpDIS->itemState & ODS_FOCUS))
	{
		pDC->DrawFocusRect(&lpDIS->rcItem); 
	}


	//-----------------------------------------------------------------
	// 3. Text를 그린다.
	//-----------------------------------------------------------------
	// declare) 
	COLORREF	crText	 = 0;
	CString		sText;

	// 1) 색상~
	//COLORREF crHilite = RGB(255-GetRValue(crNorm), 255-GetGValue(crNorm), 255-GetBValue(crNorm));

	// 2) Text의 배경을 투명하도록 한다. (그리고 이전 배경색을 저장한다.)
	int		nBkMode = pDC->SetBkMode(TRANSPARENT);

	// 3A) ItemData가 0이 아닐 경우...
	if (lpDIS->itemData)		
	{
		if (lpDIS->itemState & ODS_SELECTED)
			crText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		else if (lpDIS->itemState & ODS_DISABLED)
			crText = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		else
			crText = pDC->SetTextColor(crNorm);
	}
	// 3B) ItemData가 0일 경우...
	else
	{
		if (lpDIS->itemState & ODS_SELECTED)
			crText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		else if (lpDIS->itemState & ODS_DISABLED)
			crText = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		else
			crText = pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	}
	
	// 4) 현재 Display되어 있는 Text를 얻는다.
	GetText(lpDIS->itemID, sText);
	CRect rect = lpDIS->rcItem;

	// 5) Setup the text format.
	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	if (GetStyle() & LBS_USETABSTOPS)
	{
		nFormat |= DT_EXPANDTABS;
	}

	// 6) Text를 그리기 전에 먼저 영역을 계산한다.
	pDC->DrawText(sText, &rect, nFormat | DT_CALCRECT);

	// 7) 최종적으로 그린다.
	pDC->DrawText(sText, &rect, nFormat);

	// 8) 색상을 되돌린다.
	pDC->SetTextColor(crText); 
	pDC->SetBkMode(nBkMode);
}

void CCGListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemHeight = ::GetSystemMetrics(SM_CYMENUCHECK);
}

int CCGListBox::AddString(LPCTSTR lpszItem)
{
	return __super::AddString(lpszItem);
}

int CCGListBox::AddString(LPCTSTR lpszItem, COLORREF rgb)
{
	int nItem = AddString(lpszItem);

	if (nItem >= 0)
		SetItemData(nItem, rgb);

	return nItem;
}

int CCGListBox::InsertString(int nIndex, LPCTSTR lpszItem)
{
	return __super::InsertString(nIndex, lpszItem);
}

int CCGListBox::InsertString(int nIndex, LPCTSTR lpszItem, COLORREF rgb)
{
	int nItem = __super::InsertString(nIndex,lpszItem);

	if (nItem >= 0)
		SetItemData(nItem, rgb);

	return nItem;
}

void CCGListBox::SetItemColor(int nIndex, COLORREF rgb)
{
	SetItemData(nIndex, rgb);

	RedrawWindow();
}


}