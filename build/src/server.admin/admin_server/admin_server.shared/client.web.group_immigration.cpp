#include "pch.h"


client::web::group_immigration::group_immigration(const std::string_view _name) :
	Nnameable(_name)
{
}

client::web::group_immigration::~group_immigration()
{
}

void client::web::group_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " -create GROUP_IMMIGRATION Server";

	// 1) Register Mediator한다.
	register_messageable(g_pservice);
}

void client::web::group_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " -destroy GROUP_IMMIGRATION Server";

	// 1) Messageable을 unregister한다.
	unregister_messageable(g_pservice);
}

void client::web::group_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	enable_member_enter();
}

void client::web::group_immigration::on_stop()
{
}

result_code client::web::group_immigration::on_member_entering(socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	return eRESULT::SUCCESS;
}

void client::web::group_immigration::on_member_entered(socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);

	// 2) set initial page
	_pmember->send(_pmember->make_webmessage_set_main_page("ASCF", "webpage_host_info.htm"));
}

uintptr_t client::web::group_immigration::on_member_leaving(socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return 0;
}

void client::web::group_immigration::on_member_leaved(socket* _pmember, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);
}

result_code client::web::group_immigration::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return) 
	return eRESULT::BYPASS;
}
