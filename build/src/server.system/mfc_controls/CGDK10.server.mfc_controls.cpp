#include "pch.h"

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

namespace CGDK
{

CGSERVERMFCCONTROL_EXT_API CChild_event_list* WINAPI create_child_event_list(time_event::entity_manager* _pentity_manager, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_event_list* pWnd = new CChild_event_list(_pentity_manager, pParentWnd);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_EVENT_LIST, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Return) 생성된 윈도우 리턴~
	return	pWnd;
}

CGSERVERMFCCONTROL_EXT_API CChild_logger_view_ex* WINAPI create_child_log_view(CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_logger_view_ex* pWnd = new CChild_logger_view_ex(pParentWnd);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_LOGVIEW, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Return) 생성된 윈도우 리턴~
	return	pWnd;
}


}