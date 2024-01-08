#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChildDialog, CGDIALOG)

CChildDialog::CChildDialog(UINT nIDTemplate, CWnd* pnode_parent)
	: CGDIALOG(nIDTemplate, pnode_parent)
{

}

CChildDialog::~CChildDialog()
{
}


BEGIN_MESSAGE_MAP(CChildDialog, CGDIALOG)
	ON_BN_CLICKED(IDOK, &CChildDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CChildDialog::OnBnClickedCancel)
END_MESSAGE_MAP()

void CChildDialog::OnBnClickedOk()
{
}

void CChildDialog::OnBnClickedCancel()
{
}

void CChildDialog::MoveToCenter(CWnd* _pwnd, int _off_x, int _off_y)
{
	// declare) 
	CRect rectDest;
	CRect rectSrc;

	// 1) Wnd의 위치를 구한다.
	_pwnd->GetWindowRect(rectDest);

	// 2) 현재 Window의 Window Rect를 구한다.
	GetWindowRect(rectSrc);

	// 3) 이동할 X, Y 좌표를 구한다.
	int moveX = rectDest.left-rectSrc.left + (rectDest.Width()-rectSrc.Width())/2+_off_x;
	int moveY = rectDest.top-rectSrc.top + (rectDest.Height()-rectSrc.Height())/2+_off_y;

	rectSrc.MoveToXY(moveX, moveY);

	SetWindowPos(nullptr, rectSrc.left, rectSrc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// CChildDialog message handlers
void CChildDialog::Move(int p_X, int p_Y)
{
	// declare) 
	CRect rectSrc;

	// 1) Wnd의 위치를 구한다.
	GetWindowRect(rectSrc);
	rectSrc.MoveToXY(p_X, p_Y);

	SetWindowPos(nullptr, rectSrc.left, rectSrc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void CChildDialog::Move(int _off_x, int _off_y, int _origin, CWnd* _pwnd)
{
	// declare) 
	CRect rectDest;
	CRect rectSrc;

	// 1) Wnd의 위치를 구한다.
	_pwnd->GetWindowRect(rectDest);

	// 2) 현재 Window의 Window Rect를 구한다.
	GetWindowRect(rectSrc);

	// 3) 이동할 X, Y 좌표를 구한다.
	int moveX = 0;
	int moveY = 0;

	if(_pwnd != nullptr)
	{
		switch(_origin & 0x0f)
		{
		case	ORIGIN_CENTER_X:
				moveX = rectDest.left - rectSrc.left + (rectDest.Width()-rectSrc.Width())/2+_off_x;
				break;

		case	ORIGIN_LEFT:
				moveX = rectDest.left - rectSrc.left + _off_x;
				break;

		case	ORIGIN_RIGHT:
				moveX = rectDest.right - rectSrc.right + _off_x;
				break;

		case	ORIGIN_FRONT:
				moveX = rectDest.left - rectSrc.right + _off_x;
				break;

		case	ORIGIN_REAR:
				moveX = rectDest.right - rectSrc.left + _off_x;
				break;

		default:
				moveX = _off_x;
		};
	}
	else
	{
		moveX = _off_x;
	}

	if(_pwnd!=nullptr)
	{
		switch(_origin & 0xf0)
		{
		case	ORIGIN_CENTER_Y:
				moveY = rectDest.top - rectSrc.top + (rectDest.Height()-rectSrc.Height())/2+_off_y;
				break;

		case	ORIGIN_TOP:
				moveY = rectDest.top- rectSrc.top + _off_y;
				break;

		case	ORIGIN_BOTTOM:
				moveY = rectDest.bottom - rectSrc.bottom + _off_y;
				break;

		case	ORIGIN_UPON:
				moveY = rectDest.top - rectSrc.bottom + _off_y;
				break;

		case	ORIGIN_BELOW:
				moveY = rectDest.bottom - rectSrc.top + _off_y;
				break;

		default:
				moveY = _off_y;
		};
	}
	else
	{
		moveY = _off_y;
	}

	rectSrc.MoveToXY(moveX, moveY);

	SetWindowPos(nullptr, rectSrc.left, rectSrc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void CChildDialog::Resize(int _off_x, int _off_y)
{
	// declare) 
	CRect rectSrc;

	// 1) Wnd의 위치를 구한다.
	GetWindowRect(rectSrc);
	rectSrc.right	+= _off_x;
	rectSrc.bottom	+= _off_y;

	SetWindowPos(nullptr, 0, 0, rectSrc.Width(), rectSrc.Height(), SWP_NOMOVE | SWP_NOZORDER);
}


void CChildDialog::AddTargetWnd(CWnd* _pwnd)
{
	circular_list<CWnd*>::iterator iter = std::find(m_list_wnd_target.begin(), m_list_wnd_target.end(), _pwnd);

	// check)
	RETURN_IF(iter != m_list_wnd_target.end());

	m_list_wnd_target.push_back(_pwnd);

}

void CChildDialog::RemoveTargetWnd(CWnd* _pwnd)
{
	circular_list<CWnd*>::iterator iter	 = std::find(m_list_wnd_target.begin(), m_list_wnd_target.end(), _pwnd);

	// check)
	RETURN_IF(iter==m_list_wnd_target.end());

	m_list_wnd_target.erase(iter);
}

void CChildDialog::SendMessageToTarget(WPARAM _message, LPARAM _lParam)
{
	for(auto iter:m_list_wnd_target)
	{
		iter->SendMessage(eMESSAGE::WINDOWS::NOTIFY, _message, _lParam);
	}
}

void CChildDialog::SetUpdateInterval(int _interval)
{
	switch(_interval)
	{
	// No Update
	case	0:
			KillTimer(0);
			break;

	// 1sec
	case	1:
			SetTimer(0, 1000, nullptr);
			break;

	// 2sec
	case	2:
			SetTimer(0, 2000, nullptr);
			break;

	// 5sec
	case	3:
			SetTimer(0, 5000, nullptr);
			break;

	// 10sec
	case	4:
			SetTimer(0, 10000, nullptr);
			break;

	// 30sec
	case	5:
			SetTimer(0, 30000, nullptr);
			break;

	// 1min
	case	6:
			SetTimer(0, 60000, nullptr);
			break;
	};
}


}