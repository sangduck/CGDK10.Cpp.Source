#include "pch.h"

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

namespace CGDK
{

CGSERVERQUERYMFCCONTROL_EXT_API CChild_querier_list* WINAPI create_child_querier_list(query::manager_querier* _pquerier_manager, CWnd* _pparent_wnd)
{
	// 1) Wnd를 할당한다.
	CChild_querier_list*	pWnd	 = new CChild_querier_list(_pquerier_manager, _pparent_wnd);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_DBQUERIER_LIST, _pparent_wnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Return) 생성된 윈도우 리턴~
	return	pWnd;
}


}