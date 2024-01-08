#include "pch.h"


group_websocket_immigration::group_websocket_immigration(const std::string_view _name) :
	Nnameable(_name)
{
}

group_websocket_immigration::~group_websocket_immigration()
{
}

void group_websocket_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " -create GROUP_IMMIGRATION Server";

	// 1) Register Mediator한다.
	register_messageable(service::get_instance());
}

void group_websocket_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " -destroy GROUP_IMMIGRATION Server";

	// 1) Messageable을 unregister한다.
	unregister_messageable(service::get_instance());
}

void group_websocket_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	enable_member_enter();
}

void group_websocket_immigration::on_stop()
{
}

result_code group_websocket_immigration::on_member_entering(socket_websocket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	return	eRESULT::SUCCESS;
}

void group_websocket_immigration::on_member_entered(socket_websocket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);

	// 2) set initial page
	_pmember->send(_pmember->make_webmessage_set_main_page("ASCF", "webpage_host_info.htm"));
}

uintptr_t group_websocket_immigration::on_member_leaving(socket_websocket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return 0;
}

void group_websocket_immigration::on_member_leaved(socket_websocket* _pmember, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);
}

result_code group_websocket_immigration::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return) 
	return eRESULT_BYPASS;
}
