#include "pch.h"

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

namespace CGDK
{

CGNETSOCKETMFCCONTROL_EXT_API CChild_acceptor_control* WINAPI create_child_acceptor_control(net::io::connective::Iacceptor* p_pAcceptor, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_acceptor_control* pWnd = new CChild_acceptor_control(p_pAcceptor);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_ACCEPTOR_CONTROL, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_connective_list* WINAPI create_child_connective_list(net::io::connective::manager* p_pmangerConnective, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_connective_list*	pWnd = new CChild_connective_list(p_pmangerConnective);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_CONNECTIVE_LIST, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_connective_info* WINAPI create_child_connective_info(const statistics::unit_connective* _pinfo, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_connective_info*	pWnd = new CChild_connective_info(_pinfo);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_CONNECTIVE_INFO, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_connector_control_ex* WINAPI create_child_connector_control(net::io::Iconnect_requestable* _pConnector, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_connector_control_ex* pWnd = new CChild_connector_control_ex(_pConnector, pParentWnd);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_CONNECTOR_CONTROL_EX, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_traffic_info* WINAPI create_child_traffic_info(const statistics::unit_traffic* _pinfo, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_traffic_info* pWnd = new CChild_traffic_info(_pinfo);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_TRAFFIC_INFO, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_traffic_info_brief_receive* WINAPI create_child_traffic_info_brief_receive(const statistics::unit_traffic* _pinfo, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_traffic_info_brief_receive* pWnd = new CChild_traffic_info_brief_receive(_pinfo);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_TRAFFIC_INFO_BRIEF_RECEIVE, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API CChild_traffic_info_brief_send* WINAPI create_child_traffic_info_brief_send(const statistics::unit_traffic* _pinfo, CWnd* pParentWnd)
{
	// 1) Wnd를 할당한다.
	CChild_traffic_info_brief_send*	pWnd = new CChild_traffic_info_brief_send(_pinfo);

	// 2) Windows를 생성한다.
	pWnd->Create(IDD_DIALOG_TRAFFIC_INFO_BRIEF_SEND, pParentWnd);

	// 3) Show한다.
	pWnd->ShowWindow(SW_SHOW);

	// Retrun) 생성된 윈도우 리턴~
	return	pWnd;
}

CGNETSOCKETMFCCONTROL_EXT_API INT_PTR WINAPI DoModal_connective_status(net::io::connective::manager* p_pmangerConnective)
{
	// 1) Dialog를 생성한다.
	CDialog_connective_status dlg(p_pmangerConnective);

	// 2) DoModal해서 결과를 Return한다.
	return	dlg.DoModal();
}

CGNETSOCKETMFCCONTROL_EXT_API INT_PTR WINAPI DoModal_connectable_status(net::io::Iconnectable* _pconnectable)
{
	// 1) Dialog를 생성한다.
	CDialog_connectable_status dlg(_pconnectable);

	// 2) DoModal해서 결과를 Return한다.
	return	dlg.DoModal();
}


}