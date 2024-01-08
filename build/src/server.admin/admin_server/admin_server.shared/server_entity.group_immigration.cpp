#include "pch.h"


server_entity::group_immigration::group_immigration(std::string_view _name) :
	Nnameable(_name)
{
}

server_entity::group_immigration::~group_immigration()
{
}

void server_entity::group_immigration::on_initializing(context& /*_context*/)
{
}

void server_entity::group_immigration::on_initialize(context& /*_context*/)
{
}

void server_entity::group_immigration::on_destroying()
{
}

void server_entity::group_immigration::on_destroy()
{
}

void server_entity::group_immigration::on_starting(context& /*_context*/)
{
}

void server_entity::group_immigration::on_start(context& /*_context*/)
{
	// 1) 입장을 받기 시작~
	enable_member_enter(true);
}

void server_entity::group_immigration::on_stopping()
{
	// 1) 실행 등록 해제한다.
	system_executor::unregister_schedulable(this);

	// 2) 입장을 금지
	disable_member_enter();

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](server_entity::socket* _pmember)->bool
	{
		_pmember->closesocket();
		return true;
	});

	// 4) Leavel All Memberl
	leave_all();
}

void server_entity::group_immigration::on_stop()
{
}

result_code server_entity::group_immigration::on_member_entering(server_entity::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void server_entity::group_immigration::on_member_entered(server_entity::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

	// 2) register schedulable
	if (get_member_count() == 1)
	{
		// - interval = 10s
		execute_interval(10s);

		// - register
		system_executor::register_schedulable(this);
	}
}

uintptr_t server_entity::group_immigration::on_member_leaving(server_entity::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) unregister schedulable
	if (get_member_count() == 1)
	{
		system_executor::unregister_schedulable(this);
	}

	// 2) unregister messageable
	_pmember->unregister_messageable(this);

	// return) 
	return	0;
}

void server_entity::group_immigration::on_member_leaved(server_entity::socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

void server_entity::group_immigration::on_execute()
{
}

result_code server_entity::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

// Message Map)
BEGIN_CGMESSAGE_MAP(server_entity::group_immigration)
	// 1) from Server
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<eMESSAGE_TYPE>(4))
		ON_CGMESSAGE(eMESSAGE_TYPE::CERTIFY_SERVER_REQUEST,	on_message_CERTIFY_SERVER_REQUEST,		sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code server_entity::group_immigration::on_message_CERTIFY_SERVER_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::socket>();

	// check) psource이 nullptr이면 안됀다.
	CGASSERT_ERROR(psource != nullptr);

	// 2) 
	auto buf_recv			 = _msg.buf_message + offset(sizeof(MSG_HEADER));
	auto info_server_setting = buf_recv.extract<server_admin::sSERVER_SETTING_INFO>();
	auto infoServer			 = buf_recv.extract<shared_buffer>();
	/*auto pid				 =*/ buf_recv.extract<uint64_t>();

	try
	{
		//// declare)
		//object_ptr<server_entity::Igroup> pgroup;

		//// 3) 접속한 Server의 종료에 따라 전송한다.
		//switch(static_cast<eSERVER_TYPE>(info_server_setting.rank.role))
		//{
		//case	eSERVER_TYPE::SESSION:
		//		pgroup	 = g_pservice->m_pgroup_server_session;
		//		break;

		//case	eSERVER_TYPE::LOGIN:
		//		pgroup	 = g_pservice->m_pgroup_server_login;
		//		break;

		//case	eSERVER_TYPE::TRADE:
		//		pgroup	 = g_pservice->m_pgroup_server_trade;
		//		break;

		//case	eSERVER_TYPE::QUOT:
		//		pgroup	 = g_pservice->m_pgroup_server_quot;
		//		break;

		//case	eSERVER_TYPE::ACCOUNT:
		//		pgroup	 = g_pservice->m_pgroup_server_account;
		//		break;

		//case	eSERVER_TYPE::CHATTING_MANAGER:
		//		pgroup	 = g_pservice->m_pgroup_server_chatting_manager;
		//		break;

		//case	eSERVER_TYPE::CHATTING_CHANNEL:
		//		pgroup	 = g_pservice->m_pgroup_server_chatting_channel;
		//		break;

		//case	eSERVER_TYPE::NOTIFY:
		//		pgroup	 = g_pservice->m_pgroup_server_notify;
		//		break;

		//case	eSERVER_TYPE::ADMIN_SERVER:
		//		CGASSERT_ERROR(false);
		//		break;

		//case	eSERVER_TYPE::CUSTOMER_SERVICE:
		//		CGASSERT_ERROR(false);
		//		break;
		//default:
		//		break;
		//}

		//// check)
		//THROW_IF(pgroup.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));
	
		//// 4) server_entity info를 저장한다.
		//scoped_lock(psource->m_lockable_server_setting)
		//{
		//	psource->m_info_server_setting = info_server_setting;
		//	psource->m_info_server		 = infoServer;
		//	psource->m_pid				 = pid;
		//}

		//// 5) Enter한다
		//auto result = pgroup->enter(psource);

		//// check) Enter에 실패했으면 Exception을 던진다.
		//THROW_IF(result!=eRESULT::SUCCESS, throwable::failure(result));
	}
	catch(throwable::failure& _e)
	{
		// Error 메시지를 전송한다.
		psource->send(message_CERTIFY_SERVER_RESPONSE(_e.m_reason, server_admin::sID()));

		// - 접속종료한다.
		psource->disconnect();
	}
	catch(...)
	{
		// Error 메시지를 전송한다.
		psource->send(message_CERTIFY_SERVER_RESPONSE(eRESULT::EXCEPTION, server_admin::sID()));

		// - 접속종료한다.
		psource->disconnect();
	}
	
	// return) 
	return eRESULT::DONE;
}
