#include "pch.h"


group_server_immigration::group_server_immigration(const std::string_view _name) :
	Nnameable(_name)
{
}

group_server_immigration::~group_server_immigration()
{
}

void group_server_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " -create GROUP_IMMIGRATION Server";

	// 1) Register Mediator한다.
	register_messageable(service::get_instance());
}

void group_server_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " -destroy GROUP_IMMIGRATION Server";

	// 1) Messageable을 unregister한다.
	unregister_messageable(service::get_instance());
}

void group_server_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	enable_member_enter();
}

void group_server_immigration::on_stop()
{
}

result_code group_server_immigration::on_member_entering(socket_server* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	return	eRESULT::SUCCESS;
}

void group_server_immigration::on_member_entered(socket_server* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_server_immigration::on_member_leaving(socket_server* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return 0;
}

void group_server_immigration::on_member_leaved(socket_server* _pmember, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);
}

BEGIN_CGMESSAGE_MAP(group_server_immigration)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_SERVER_INFO,		on_message_SERVER_ADMIN_SEND_SERVER_INFO,		sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code group_server_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message를 실행...
	EXECUTE_CGMESSAGE_MAP(_msg);
	
	// return) 
	return eRESULT::BYPASS;
}

result_code group_server_immigration::on_message_SERVER_ADMIN_SEND_SERVER_INFO(sMESSAGE_NETWORK& _msg)
{
	// check) psource가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_server>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto* psource = _msg.get_source<socket_server>();

	try
	{
		// 2) Server Socket을 GroupServer에 붙인다.
		auto result = service::get_instance()->m_pgroup_server->enter(psource, _msg);

		// check) 결과값이 eRESULT::SUCCESS가 아니면 그냥 끝낸다.
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result));
	}
	catch(...)
	{
		// - 접속을 종료한다.
		psource->closesocket();
	}

	// return) 성공적 메시지 처리!
	return eRESULT::DONE;
}

