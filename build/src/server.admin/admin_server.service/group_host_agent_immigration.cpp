#include "pch.h"


group_host_agent_immigration::group_host_agent_immigration(const std::string_view _name) :
	Nnameable(_name)
{
}

group_host_agent_immigration::~group_host_agent_immigration()
{
}

void group_host_agent_immigration::on_initializing(context& /*_context*/)
{
}

void group_host_agent_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize GROUP['" << name() << "']";

	// 1) Register Mediator한다.
	register_messageable(service::get_instance());
}

void group_host_agent_immigration::on_destroying()
{
}

void group_host_agent_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy GROUP['" << name() << "']";

	// 1) Register Mediator한다.
	unregister_messageable(service::get_instance());
}

void group_host_agent_immigration::on_starting(context& /*_context*/)
{
}

void group_host_agent_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	enable_member_enter();
}

void group_host_agent_immigration::on_stopping()
{
}

void group_host_agent_immigration::on_stop()
{
	// 1) Enter를 Disable한다.
	disable_member_enter();
}

result_code group_host_agent_immigration::on_member_entering(Isocket_host_agent* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return	eRESULT::SUCCESS;
}

void group_host_agent_immigration::on_member_entered(Isocket_host_agent* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_host_agent_immigration::on_member_leaving(Isocket_host_agent* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);

	// return) 
	return	0;
}

void group_host_agent_immigration::on_member_leaved(Isocket_host_agent* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

eRESULT group_host_agent_immigration::process_resuest_login(std::string_view /*_str_id*/, std::string_view /*_str_password*/)
{
	return eRESULT::SUCCESS;
}

BEGIN_CGMESSAGE_MAP(group_host_agent_immigration)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_HOST_INFO,		on_message_SERVER_ADMIN_SEND_HOST_INFO,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_CONTROL_INFO,	on_message_SERVER_ADMIN_SEND_CONTROL_INFO,	sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code group_host_agent_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message를 실행...
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code group_host_agent_immigration::on_message_SERVER_ADMIN_SEND_HOST_INFO(sMESSAGE_NETWORK& _msg)
{
	// check) psource가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_host_agent>() != nullptr);

	// 1) Isocket_host_agent을 얻는다.
	auto psource = _msg.get_source<socket_host_agent>();

	// declare)
	eRESULT result;

	// 2) get message buffer
	buffer_view buf_message = _msg.buf_message + offset(8);

	try
	{
		// 3) extract id/password
		std::string str_id = buf_message.extract<std::string>();;
		std::string str_password = buf_message.extract<std::string>();

		// 4) request login
		result = process_resuest_login(str_id, str_password);
	}
	catch (...)
	{
		result = eRESULT::EXCEPTION;
	}

	// check)
	RETURN_IF(result == eRESULT::PENDED, 1);

	// 5) send result
	psource->send(message_SERVER_ADMIN_LOGIN_ACCEPT(result));

	// return)
	return eRESULT::DONE;
}

result_code group_host_agent_immigration::on_message_SERVER_ADMIN_SEND_CONTROL_INFO(sMESSAGE_NETWORK& _msg)
{
	// check) psource가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_host_agent>() != nullptr);

	// 1) Isocket_host_agent을 얻는다.
	auto psource = _msg.get_source<socket_host_agent>();

	try
	{
		// 2) Server Socket을 GroupServer에 붙인다.
		auto result = service::get_instance()->m_pgroup_host_agent->enter(psource, _msg);

		// check) 결과값이 FALSE이면 그냥 끝낸다.
		RETURN_IF(result != eRESULT::SUCCESS, 1);
	}
	catch(...)
	{
	}

	// return)
	return eRESULT::DONE;
}

shared_buffer group_host_agent_immigration::message_SERVER_ADMIN_LOGIN_ACCEPT(eRESULT _result)
{

}
