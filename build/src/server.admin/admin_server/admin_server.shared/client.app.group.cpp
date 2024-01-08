#include "pch.h"


client::app::group::group(std::string_view _name) :
	Nnameable(_name)
{
}
client::app::group::~group()
{
}

void client::app::group::on_initializing(context& /*_context*/)
{
}
void client::app::group::on_initialize(context& /*_context*/)
{
}
void client::app::group::on_destroying()
{
}
void client::app::group::on_destroy()
{
}

void client::app::group::on_starting(context& /*_context*/)
{
}
void client::app::group::on_start(context& /*_context*/)
{
	// 1) 10초마다 한번씩 호출되도록 간격을 설정한다.
	execute_interval(10s);

	// 2) Scheduler에  등록한다.
	system_executor::register_schedulable(this);

	// 3) 입장을 받기 시작~
	enable_member_enter(true);
}
void client::app::group::on_stopping()
{
	// 1) 입장을 금지
	disable_member_enter();

	// 2) 실행 등록 해제한다.
	system_executor::unregister_schedulable(this);

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](client::app::socket* _pmember)->bool
	{
		_pmember->closesocket(); 
		return true;
	});

	// 4) Leavel All Member
	leave_all();
}
void client::app::group::on_stop()
{
}

result_code client::app::group::on_member_entering(client::app::socket* _pmember, sMESSAGE& /*_msg*/)
{
	//// 2) host List를 전송한다.
	//_pmember->send(g_pservice->m_pmanager_server_instance->message_SERVER_ADMIN_HOST_SOCKET_LIST());

	//// 3) Server setting을 전송한다.
	//_pmember->send(g_pservice->m_pmanager_server_instance->message_SERVER_ADMIN_SERVER_SETTING_LIST());

	//// 4) Makret 정보를 전송한다.
	//_pmember->send(g_pservice->m_pgroup_server_trade->message_SERVER_ADMIN_MARKET_LIST());

	//// 5) Confirm 정보를 전송앟ㄴ다.
	//_pmember->send(g_pservice->m_pgroup_server_trade->message_SERVER_ADMIN_CONFIRM());

	// 6) Server Event 정보를 전송한다.
	_pmember->send(g_pservice->m_pmanager_time_event->message_SERVER_ADMIN_SERVER_TIME_EVENT());

	//// 7) Event Log 정보를 전송한다.
	//_pmember->send(g_pservice->message_SERVER_ADMIN_SERVER_TIME_EVENT_LOG_LIST());

	// 8) Statistics 정보를 전송한다.
	//_pmember->send();


	// return) 
	return	eRESULT::SUCCESS;
}

void client::app::group::on_member_entered(client::app::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

}

uintptr_t client::app::group::on_member_leaving(client::app::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// return) 
	return	0;
}

void client::app::group::on_member_leaved(client::app::socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

void client::app::group::on_execute()
{
}

result_code client::app::group::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) Message Map
		EXECUTE_CGMESSAGE_MAP(_msg);
	}
	catch (...)
	{
		// - Exception을 처리한다.
	}

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

BEGIN_CGMESSAGE_MAP(client::app::group)
	// 1) Message from Socket Chatting client::app
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<eMESSAGE_TYPE>(4))
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_LIST_REQUEST,					on_message_SERVER_LIST_REQUEST,								sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_INSTANCE_UPDATE,					on_message_SERVER_SETTING_UPDATE,							sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_INFO_UPDATE,						on_message_SERVER_INFO_UPDATE,								sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_STATE_UPDATE,					on_message_SERVER_STATE_UPDATE,								sMESSAGE_NETWORK);

		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SERVER_INIT_REQUEST,		on_message_SERVER_ADMIN_CONTROL_SERVER_INIT_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SERVER_DESTROY_REQUEST,	on_message_SERVER_ADMIN_CONTROL_SERVER_DESTROY_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SERVER_START_REQUEST,		on_message_SERVER_ADMIN_CONTROL_SERVER_START_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SERVER_STOP_REQUEST,		on_message_SERVER_ADMIN_CONTROL_SERVER_STOP_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SERVER_KILL_REQUEST,		on_message_SERVER_ADMIN_CONTROL_SERVER_KILL_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST,on_message_SERVER_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST,	sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_MARKET_START_REQUEST,		on_message_SERVER_ADMIN_CONTROL_MARKET_START_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_MARKET_STOP_REQUEST,		on_message_SERVER_ADMIN_CONTROL_MARKET_STOP_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_MARKET_DESTROY_REQUEST,	on_message_SERVER_ADMIN_CONTROL_MARKET_DESTROY_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_CONFIRM_RESPONSE,			on_message_SERVER_ADMIN_CONTROL_CONFIRM_RESPONSE,			sMESSAGE_NETWORK);

		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_EVENT_REGISTER_REQUEST,	on_message_SERVER_ADMIN_CONTROL_EVENT_REGISTER_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST,	on_message_SERVER_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST,	sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_EVENT_UPDATE_REQUEST,		on_message_SERVER_ADMIN_CONTROL_EVENT_UPDATE_REQUEST,		sMESSAGE_NETWORK);

		ON_CGMESSAGE(eMESSAGE_TYPE::ADMIN_CONTROL_SETTING_REFRESH_REQUEST,	on_message_SERVER_ADMIN_CONTROL_SETTING_REFRESH_REQUEST,	sMESSAGE_NETWORK);

		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_LOG,					on_message_SERVER_ADMIN_REQUEST_LOG,						sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP

	// 2) Message from Socket Game client::app
	ON_CGMESSAGE(CGNOTIFY_SERVER_INIT_COMPLETE,								on_message_NOTIFY_SERVER_INIT_COMPLETE,					sMESSAGE_NETWORK);
END_CGMESSAGE_MAP

result_code client::app::group::on_message_NOTIFY_SERVER_INIT_COMPLETE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto	psource = _msg.get_source<client::app::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);


	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_LIST_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<client::app::socket>();

	// check) psource이 nullptr이면 안됀다.
	CGASSERT_ERROR(psource != nullptr);

	try
	{
	}
	catch (...)
	{
		// - 문제...
		CGASSERT_ERROR(false);

		// - 접속을 종료해 버린다.
		psource->closesocket();

		// Reraise)
		throw;
	}

	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_SETTING_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto	psource	 = _msg.get_source<client::app::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);


	// return) 
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_INFO_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto	psource = _msg.get_source<server::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);


	// return) 
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_STATE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto	psource = _msg.get_source<server::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);


	// return)
	return eRESULT::DONE;
}


result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_SERVER_INIT_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_SERVER_INIT_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_SERVER_INIT_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_SERVER_INIT_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pserver_entity = g_pservice->m_pmanager_server_instance->find_instance(msg_recv.id);

	//// check) pserver_entity가 nullptr이면 안된다.
	//CGASSERT_ERROR(pserver_entity.exist());

	//// check) pserver_entity가 nullptr이면 끝낸다.
	//RETURN_IF(pserver_entity.empty(), 1);

	//// 2) 실행한다.
	//pserver_entity->request_service_initialize();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_SERVER_DESTROY_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_SERVER_DESTROY_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_SERVER_DESTROY_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_SERVER_DESTROY_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pserver_entity = g_pservice->m_pmanager_server_instance->find_instance(msg_recv.id);

	//// check) pserver_entity가 nullptr이면 안된다.
	//CGASSERT_ERROR(pserver_entity.exist());

	//// check) pserver_entity가 nullptr이면 끝낸다.
	//RETURN_IF(pserver_entity.empty(), 1);

	//// 3) 실행한다.
	//pserver_entity->request_service_destroy();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_SERVER_START_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() > sizeof(MSG_HEADER) + sizeof(uint64_t));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() <= sizeof(MSG_HEADER) + sizeof(uint64_t), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_SERVER_START_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pserver_entity = g_pservice->m_pmanager_server_instance->find_instance(msg_recv.id);

	//// check) pserver_entity가 nullptr이면 안된다.
	//CGASSERT_ERROR(pserver_entity.exist());

	//// check) pserver_entity가 nullptr이면 끝낸다.
	//RETURN_IF(pserver_entity.empty(), 1);

	//// 2) 실행한다.
	//pserver_entity->request_service_start(msg_recv.str_setting);

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_SERVER_STOP_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_SERVER_STOP_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_SERVER_STOP_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_SERVER_STOP_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pserver_entity = g_pservice->m_pmanager_server_instance->find_instance(msg_recv.id);

	//// check) pserver_entity가 nullptr이면 안된다.
	//CGASSERT_ERROR(pserver_entity.exist());

	//// check) pserver_entity가 nullptr이면 끝낸다.
	//RETURN_IF(pserver_entity.empty(), 1);

	//// 2) 실행한다.
	//pserver_entity->request_service_stop();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_SERVER_KILL_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_SERVER_KILL_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_SERVER_KILL_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_SERVER_KILL_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pserver_entity = g_pservice->m_pmanager_server_instance->find_instance(msg_recv.id);

	//// check) pserver_entity가 nullptr이면 안된다.
	//CGASSERT_ERROR(pserver_entity.exist());

	//// check) pserver_entity가 nullptr이면 끝낸다.
	//RETURN_IF(pserver_entity.empty(), 1);

	//// 2) 실행한다.
	//pserver_entity->request_service_kill();

	// return)
	return eRESULT::DONE;
}

result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pMarket = g_pservice->m_pgroup_server_trade->find_market(msg_recv.mid_serverid);

	//// check) pMarket이 nullptr이면 안됀다.
	//CGASSERT_ERROR(pMarket.exist());

	//// check) pMarket가 nullptr이면 끝낸다.
	//RETURN_IF(pMarket.empty(), 1);

	//// 3) 실행한다.
	//pMarket->request_market_control_initialize();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_MARKET_START_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_MARKET_START_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_MARKET_START_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_MARKET_START_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pMarket = g_pservice->m_pgroup_server_trade->find_market(msg_recv.mid_serverid);

	//// check) pMarket이 nullptr이면 안됀다.
	//CGASSERT_ERROR(pMarket.exist());

	//// check) pMarket가 nullptr이면 끝낸다.
	//RETURN_IF(pMarket.empty(), 1);

	//// 3) 실행한다.
	//pMarket->request_market_control_start();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_MARKET_STOP_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_MARKET_STOP_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_MARKET_STOP_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_MARKET_STOP_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pMarket = g_pservice->m_pgroup_server_trade->find_market(msg_recv.mid_serverid);

	//// check) pMarket이 nullptr이면 안됀다.
	//CGASSERT_ERROR(pMarket.exist());

	//// check) pMarket가 nullptr이면 끝낸다.
	//RETURN_IF(pMarket.empty(), 1);

	//// 3) 실행한다.
	//pMarket->request_market_control_stop();

	// return)
	return eRESULT::DONE;
}
result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_MARKET_DESTROY_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_MARKET_DESTROY_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_MARKET_DESTROY_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_MARKET_DESTROY_REQUEST>();

	//// 2) Register Info를 얻는다.
	//auto pMarket = g_pservice->m_pgroup_server_trade->find_market(msg_recv.mid_serverid);

	//// check) pMarket이 nullptr이면 안됀다.
	//CGASSERT_ERROR(pMarket.exist());

	//// check) pMarket가 nullptr이면 끝낸다.
	//RETURN_IF(pMarket.empty(), 1);

	//// 3) 실행한다.
	//pMarket->request_market_control_destroy();

	// return)
	return eRESULT::DONE;
}

result_code	client::app::group::on_message_SERVER_ADMIN_CONTROL_CONFIRM_RESPONSE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_CONFIRM_RESPONSE));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_CONFIRM_RESPONSE), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_CONFIRM_RESPONSE>();

	//// 1) Register Info를 얻는다.
	//auto pconfirm_request = g_pservice->m_pgroup_server_trade->FindConfirmRequest(msg_recv.idConfirm);

	//// check) 
	//CGASSERT_ERROR(pconfirm_request.exist());

	//// check) 
	//RETURN_IF(pconfirm_request.empty(), 1);

	//// check) 
	//RETURN_IF(pconfirm_request->m_trade_event.status != eEVENT_STATUS::SUCCESS, 1);

	//// 2) 실행한다.
	//if(msg_recv.bAccept)
	//{
	//	g_pservice->m_pgroup_server_trade->AcceptTransferConfirm(pconfirm_request);
	//}
	//else
	//{
	//	g_pservice->m_pgroup_server_trade->RejectTransferConfirm(pconfirm_request);
	//}

	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_ADMIN_CONTROL_EVENT_REGISTER_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_EVENT_REGISTER_REQUEST>();

	//// 2) Operation객체를 생성한다.
	//auto ptime_event_operation = make_object<time_event_server::operation>();

	////// 3) 값을 설정한다.
	////ptime_event_operation->set_time_event_info(msg_recv.infoEvent);

	//// 4) ...
	//for(auto& iter: msg_recv.vectorOperation)
	//{
	//	// - 객체를 생성한다.
	//	auto pserver_operation = make_object<time_event_server::server_operation>();

	//	// - 값을...
	//	*(EVENT_SCHEDULE*)pserver_operation	 = iter.eventSchedule;
	//	pserver_operation->m_operation		 = iter.ServerOperation;

	//	// - 추가한다.
	//	ptime_event_operation->add_schedule(pserver_operation);
	//}

	//// 3) 등록한다.
	//g_pservice->m_pmanager_time_event->register_time_event(ptime_event_operation);

	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 크기를 확인한다.
	//CGASSERT_ERROR(_msg.buf_message.size() == sizeof(MESSAGE_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST));

	//// check) 크기를 확인한다.
	//THROW_IF(_msg.buf_message.size() != sizeof(MESSAGE_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH));

	//// 1) 메시지를 읽어들인다.
	//auto& msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST>();

	//// 2) 등록해제한다.
	//auto pserver_time_event = g_pservice->m_pmanager_time_event->find_time_event(msg_recv.id_event);

	//// check) 찾지 못했으면 끝낸다
	//RETURN_IF(pserver_time_event.empty(), 1);

	//// 3) 삭제처리
	//g_pservice->m_pmanager_time_event->unregister_time_event(pserver_time_event);

	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_ADMIN_CONTROL_EVENT_UPDATE_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) 메시지를 읽어들인다.
	//auto&	msg_recv = _msg.buf_message.front<MESSAGE_ADMIN_CONTROL_EVENT_UPDATE_REQUEST>();
	
	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_ADMIN_CONTROL_SETTING_REFRESH_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code client::app::group::on_message_SERVER_ADMIN_REQUEST_LOG(sMESSAGE_NETWORK& _msg)
{
	// 1) Log File을 얻는다.
	object_ptr<logger_hub_archive> plog = g_plogger;

	// check) logger_file이 없으면 1을 리턴한다.
	RETURN_IF(plog.empty(), 1);

	// 2) Source를 얻는다.
	auto* psource = _msg.get_source<socket>();

	try
	{
		// 3) Log의 Position을 얻는다.
		auto position = _msg.buf_message.front<uint64_t>(8);

		// 4) Position의 차이가 많이 나면 Position값을 수정한다.
		if(position>plog->get_log_count())
		{
			psource->send(message_SERVER_ADMIN_REQUEST_LOG(0));
			position = 0;
		}
		else if((plog->get_log_count()-position)>512)
		{
			position = plog->get_log_count()-512;
			psource->send(message_SERVER_ADMIN_REQUEST_LOG(position-1));
		}

		// declare) 
		std::vector<object_ptr<LOG_RECORD>>	vector_log_record;

		// 4) Position 이후의 Log정보를 읽는다.
		vector_log_record = g_plogger->get_stored_log(position);

		// check) 아무 버퍼가 없으면 끝낸다.
		RETURN_IF(vector_log_record.empty(), 1);

		// 5) 모두 전송한다.
		auto iter = vector_log_record.begin();
		auto iterEnd = vector_log_record.end();

		for(; iter!=iterEnd; ++iter)
		{
			psource->send(server_admin::message_SYSTEM_LOG(*iter));
		}
	}
	catch (throwable::failure& /*_e*/)
	{

	}
	catch (...)
	{

	}

	// return) 
	return eRESULT::DONE;
}


shared_buffer client::app::group::message_SERVER_ADMIN_REQUEST_LOG(uint64_t _Index) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + sizeof(server_admin::sHOST_ID) + sizeof(server_admin::sID) + sizeof(_Index);

	// 1) 버퍼를 할당한다.
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 2) Message를 작성한다.
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_LOG);
	temp_buffer.append<server_admin::sHOST_ID>();	// hid
	temp_buffer.append<server_admin::sID>();	// id_server
	temp_buffer.append<uint64_t>(_Index);

	// 3) Message 길이를 써넣는다.
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}


