#include "pch.h"


client::app::group_immigration::group_immigration(std::string_view _name) :
	Nnameable(_name)
{
}

client::app::group_immigration::~group_immigration()
{
}

void client::app::group_immigration::on_initializing(context& /*_context*/)
{
}

void client::app::group_immigration::on_initialize(context& /*_context*/)
{
}

void client::app::group_immigration::on_destroying()
{
}

void client::app::group_immigration::on_destroy()
{
}

void client::app::group_immigration::on_starting(context& /*_context*/)
{
}

void client::app::group_immigration::on_start(context& /*_context*/)
{
	// 1) 10초마다 한번씩 호출된다.
	execute_interval(10s);

	// 2) 실행 등록한다.
	system_executor::register_schedulable(this);

	// 3) 입장을 받기 시작~
	enable_member_enter(true);
}

void client::app::group_immigration::on_stopping()
{
	// 1) 입장을 금지
	disable_member_enter();

	// 2) 실행 등록 해제한다.
	system_executor::unregister_schedulable(this);

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](socket* _pmember)->bool
	{
		_pmember->closesocket();
		return true;
	});

	// 4) Leavel All Memberl
	leave_all();
}

void client::app::group_immigration::on_stop()
{
}

result_code client::app::group_immigration::on_member_entering(socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return	eRESULT::SUCCESS;
}

void client::app::group_immigration::on_member_entered(socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);
}

uintptr_t client::app::group_immigration::on_member_leaving(socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void client::app::group_immigration::on_member_leaved(socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

void client::app::group_immigration::on_execute()
{
}

result_code client::app::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

// Message Map)
BEGIN_CGMESSAGE_MAP(client::app::group_immigration)
	// 1) from client::app::app
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<eMESSAGE_TYPE>(4))
		ON_CGMESSAGE(eMESSAGE_TYPE::CERTIFY_CLIENT_REQUEST,	on_message_CERTIFY_CLIENT_REQUEST,		sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code client::app::group_immigration::on_message_CERTIFY_CLIENT_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//// 2) CERTIFY_CLIENT_RESPONSE 메시지를 전송함.
	//psource->send(MESSAGE_CERTIFY_CLIENT_RESPONSE::MAKE_BUFFER(eRESULT::SUCCESS));

	//// 3) Enter시켜버림...
	//g_pservice->m_pgroup_client->enter(psource);

	// return) 
	return eRESULT::DONE;
}
