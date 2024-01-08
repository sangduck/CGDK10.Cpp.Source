#include "pch.h"


querier_server::querier_server(std::string_view _name) :
	query::rdb::querier(_name),
	m_uid_host(0),
	m_uid_server(0),
	m_uid_market(0),
	m_uid_command(0)
{
}

querier_server::~querier_server()
{
}

void querier_server::request_host_certify(server_entity::host::socket* /*_psocket*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// check)
	//CGASSERT_ERROR(_psocket != nullptr);

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::host_certify>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_host;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_psocket				 = _psocket;

	//// 3) 할등한다.
	//_psocket->m_info_server_setting.id.code = pquery->m_uid;

	//// 4) querier를 요청한다.
	//m_pquerier_wal_host_certify->execute_query(pquery);
}

void querier_server::request_host_close(server_entity::host::socket* /*_psocket*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// check)
	//CGASSERT_ERROR(_psocket != nullptr);

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::host_close>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_host;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_psocket				 = _psocket;

	//// 4) querier를 요청한다.
	//m_pquerier_wal_host_close->execute_query(pquery);
}

void querier_server::request_server_certify(server_entity::socket* /*_psocket*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// check)
	//CGASSERT_ERROR(_psocket != nullptr);

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::server_certify>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_server;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_psocket				 = _psocket;

	//// 3) 할등한다.
	//_psocket->m_info_server_setting.id.code = pquery->m_uid;

	//// 4) querier를 요청한다.
	//m_pquerier_wal_server_certify->execute_query(pquery);
}

void querier_server::request_server_close(server_entity::socket* /*_psocket*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// check)
	//CGASSERT_ERROR(_psocket != nullptr);

	//// check)
	//CGASSERT_ERROR(_psocket->m_info_server_setting.id.key != 0);

	//// check)
	//CGASSERT_ERROR(_psocket->m_info_server_setting.id.code != 0);

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::server_close>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_server;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_psocket				 = _psocket;

	//// 4) querier를 요청한다.
	//m_pquerier_wal_server_close->execute_query(pquery);
}

void querier_server::request_server_state(server_entity::socket* /*_psocket*/, eOBJECT_STATE /*_state*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// check)
	//CGASSERT_ERROR(_psocket != nullptr);

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::server_state>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_server;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_state					 = _state;
	//pquery->m_psocket				 = _psocket;

	//// 4) querier를 요청한다.
	//m_pquerier_wal_server_state->execute_query(pquery);
}


void querier_server::request_market_state(server_entity::socket* /*_psocket*/, eOBJECT_STATE /*_state*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY), );

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::market_state>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_server;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_state					 = _state;
	//pquery->m_psocket				 = _psocket;

	//// 3) ...
	//m_pquerier_wal_market_state->execute_query(pquery);
}

void querier_server::request_market_status(server_entity::socket* /*_psocket*/)
{
	//// check) 현재 상태가 RUNNING 상태일때만 처리한다. (CHECKIN과 ORDER는 RUNNING 상태일 때만 받아들인다.)
	//CGASSERT_ERROR(get_object_state() == eOBJECT_STATE::RUNNING);

	//// check) RUNNING 상태가 아니면 Error를 리턴한다.
	//THROW_IF(get_object_state() != eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::NOT_READY));

	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::market_status>();

	//// 2) Server의 code를 할당한다.
	//pquery->m_uid					 = ++m_uid_server;
	//pquery->m_info_server_setting	 = _psocket->m_info_server_setting;
	//pquery->m_peer_address			 = _psocket->get_peer_address();
	//pquery->m_psocket				 = _psocket;

	//// 3) ...
	//m_pquerier_wal_market_status->execute_query(pquery);
}

void querier_server::request_register_time_event(time_event_server::operation* /*_pserver_operation*/)
{
	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::time_event_register>();

	//// 3) 값을 설정한다.
	//pquery->m_pserver_operation = _pserver_operation;

	//// 4) query를 요청한다.
	//m_pquerier_wal_time_event_register->execute_query(pquery);
}

void querier_server::request_unregister_time_event(time_event_server::operation* /*_pserver_operation*/)
{
	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::time_event_unregister>();

	//// 3) 값을 설정한다.
	//pquery->m_pserver_operation = _pserver_operation;

	//// 4) query를 요청한다.
	//m_pquerier_wal_time_event_unregister->execute_query(pquery);
}

void querier_server::request_store_time_event_result(time_event_server::server_operation* /*_pserver_operation*/, uint64_t /*_id_event*/)
{
	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::time_event_process>();

	//// 2) 값을 설정한다.
	//pquery->m_id_event			 = _id_event;
	//pquery->m_pserver_operation	 = _pserver_operation;

	//// 3) query를 요청한다.
	//m_pquerier_wal_time_event_process->execute_query(pquery);
}

void querier_server::request_done_time_event_result(time_event_server::operation* /*_pserver_operation*/, uint64_t /*_id_event*/)
{
	//// 1) query 객체를 생성한다.
	//auto pquery = make_object<query_event::time_event_done>();

	//// 2) 값을 설정한다.
	//pquery->m_id_event			 = _id_event;
	//pquery->m_pserver_operation	 = _pserver_operation;

	//// 3) query를 요청한다.
	//m_pquerier_wal_time_event_done->execute_query(pquery);
}

void querier_server::request_update_time_event(time_event_server::operation* /*_pserver_operation*/, uint64_t /*_id_event*/)
{
	//// 1) query 객체를 생성한다.
	//auto	pquery		 = make_object<query_event::time_event_update>();

	//// 2) id_event값들을 설정한다.
	//_ProcessAllocEventID_Update(_pserver_operation);

	//// 3) 값을 설정한다.
	//pquery->m_pserver_operation	 = _pserver_operation;

	//// 4) query를 요청한다.
	//post_execute_query(pquery);

	//m_pquerier_wal_time_event_update->execute_query(pquery);
}


void querier_server::on_initializing(context& /*_context*/)
{
}

void querier_server::on_initialize(context& /*_context*/)
{

}

void querier_server::on_destroying()
{

}

void querier_server::on_destroy()
{

}

void querier_server::on_starting(context& /*_context*/)
{
}

void querier_server::on_start(context& /*_context*/)
{
	//// 1) wal file start~
	//auto pwal_file_host_certify = make_own<wal::querier<query_event::host_certify>>();
	//pwal_file_host_certify->set_target(this);
	//pwal_file_host_certify->set_querier(this);
	//pwal_file_host_certify->start(std::string(g_pservice->get_name()) + "_host_certify.jnl");

	//auto pwal_file_host_close = make_own<wal::querier<query_event::host_close>>();
	//pwal_file_host_close->set_target(this);
	//pwal_file_host_close->set_querier(this);
	//pwal_file_host_close->start(std::string(g_pservice->get_name()) + "_host_close.jnl");

	//auto pwal_file_server_certify = make_own<wal::querier<query_event::server_certify>>();
	//pwal_file_server_certify->set_target(this);
	//pwal_file_server_certify->set_querier(this);
	//pwal_file_server_certify->start(std::string(g_pservice->get_name()) + "_server_certify.jnl");

	//auto pwal_file_server_close = make_own<wal::querier<query_event::server_close>>();
	//pwal_file_server_close->set_target(this);
	//pwal_file_server_close->set_querier(this);
	//pwal_file_server_close->start(std::string(g_pservice->get_name()) + "_server_close.jnl");

	//auto pwal_file_server_state = make_own<wal::querier<query_event::server_state>>();
	//pwal_file_server_state->set_target(this);
	//pwal_file_server_state->set_querier(this);
	//pwal_file_server_state->start(std::string(g_pservice->get_name()) + "_server_state.jnl");

	//auto pwal_file_market_state = make_own<wal::querier<query_event::market_state>>();
	//pwal_file_market_state->set_target(this);
	//pwal_file_market_state->set_querier(this);
	//pwal_file_market_state->start(std::string(g_pservice->get_name()) + "_market_state.jnl");

	//auto pwal_file_market_status = make_own<wal::querier<query_event::market_status>>();
	//pwal_file_market_status->set_target(this);
	//pwal_file_market_status->set_querier(this);
	//pwal_file_market_status->start(std::string(g_pservice->get_name()) + "_market_status.jnl");

	//auto pwal_file_time_event_register = make_own<wal::querier<query_event::time_event_register>>();
	//pwal_file_time_event_register->set_target(this);
	//pwal_file_time_event_register->set_querier(this);
	//pwal_file_time_event_register->start(std::string(g_pservice->get_name()) + "_time_event_register.jnl");

	//auto pwal_file_time_event_unregister = make_own<wal::querier<query_event::time_event_unregister>>();
	//pwal_file_time_event_unregister->set_target(this);
	//pwal_file_time_event_unregister->set_querier(this);
	//pwal_file_time_event_unregister->start(std::string(g_pservice->get_name()) + "_time_event_unregister.jnl");

	//auto pwal_file_time_event_process = make_own<wal::querier<query_event::time_event_process>>();
	//pwal_file_time_event_process->set_target(this);
	//pwal_file_time_event_process->set_querier(this);
	//pwal_file_time_event_process->start(std::string(g_pservice->get_name()) + "_time_event_process.jnl");

	//auto pwal_file_time_event_done = make_own<wal::querier<query_event::time_event_done>>();
	//pwal_file_time_event_done->set_target(this);
	//pwal_file_time_event_done->set_querier(this);
	//pwal_file_time_event_done->start(std::string(g_pservice->get_name()) + "_time_event_done.jnl");

	//auto pwal_file_time_event_update = make_own<wal::querier<query_event::time_event_update>>();
	//pwal_file_time_event_update->set_target(this);
	//pwal_file_time_event_update->set_querier(this);
	//pwal_file_time_event_update->start(std::string(g_pservice->get_name()) + "_time_event_update.jnl");

	//m_pquerier_wal_host_certify			 = std::move(pwal_file_host_certify);
	//m_pquerier_wal_host_close			 = std::move(pwal_file_host_close);
	//m_pquerier_wal_server_certify		 = std::move(pwal_file_server_certify);
	//m_pquerier_wal_server_close			 = std::move(pwal_file_server_close);
	//m_pquerier_wal_server_state			 = std::move(pwal_file_server_state);
	//m_pquerier_wal_market_state			 = std::move(pwal_file_market_state);
	//m_pquerier_wal_market_status		 = std::move(pwal_file_market_status);
	//m_pquerier_wal_time_event_register	 = std::move(pwal_file_time_event_register);
	//m_pquerier_wal_time_event_unregister = std::move(pwal_file_time_event_unregister);
	//m_pquerier_wal_time_event_process	 = std::move(pwal_file_time_event_process);
	//m_pquerier_wal_time_event_done		 = std::move(pwal_file_time_event_done);
	//m_pquerier_wal_time_event_update	 = std::move(pwal_file_time_event_update);

	// 2) oid, ciid, poid 값을 읽어온다.
	{
		// - query를 요청한다.
		auto pquery = execute_query(std::string_view("{ CALL Get_last_uid }"));

		// - 계좌 정보를 읽어온다.
		for (auto& iter : *pquery)
		{
			m_uid_command = iter.get_value<uint64_t>(1);
			m_uid_market = iter.get_value<uint64_t>(2);
			m_uid_server = iter.get_value<uint64_t>(3);
			m_uid_host	 = iter.get_value<uint64_t>(4);
		}

		// - query를 Close한다.
		pquery->close_query();
	}
}

void querier_server::on_stopping()
{
	//// 1) wal destroy
	//m_pquerier_wal_host_certify.destroy();
	//m_pquerier_wal_host_close.destroy();
	//m_pquerier_wal_server_certify.destroy();
	//m_pquerier_wal_server_close.destroy();
	//m_pquerier_wal_server_state.destroy();
	//m_pquerier_wal_market_state.destroy();
	//m_pquerier_wal_market_status.destroy();
	//m_pquerier_wal_time_event_register.destroy();
	//m_pquerier_wal_time_event_unregister.destroy();
	//m_pquerier_wal_time_event_process.destroy();
	//m_pquerier_wal_time_event_done.destroy();
	//m_pquerier_wal_time_event_update.destroy();
}

void querier_server::on_stop()
{
}

//void querier_server::process_wal_restore(query_event::host_certify* _query)
//{
//	_query->m_check_duplicate = true;
//}

//void querier_server::process_wal_appended(query_event::host_certify* _query)
//{
//	// declare)
//	sSERVER_EVENT_LOG* pevent_server_log = new sSERVER_EVENT_LOG;
//
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role) ;
//	pevent_server_log->event_type	 = eEVENT_TYPE::SERVER_CONNECT;
//	pevent_server_log->state		 = eOBJECT_STATE::NONE;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server = _query->m_peer_address;
//
//	// log) 
//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_LOG_ADD, reinterpret_cast<uintptr_t>(pevent_server_log)));
//	g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_LOG_ADD::MAKE_BUFFER(*pevent_server_log));
//
//	// check) 
//	if(_query->m_psocket.exist())
//	{
//		// declare) 
//		sMESSAGE_NETWORK msg_send(CGNOTIFY_SERVER_INIT_COMPLETE, _query->m_psocket.get());
//
//		// - 전송한다.
//		_query->m_psocket->transmit_message(msg_send);
//	}
//}
//
//void querier_server::process_wal_restore(query_event::host_close* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::host_close* _query)
//{
//	// 1) event 객체를 생성한다.
//	auto pevent_server_log = make_object<server_event_log>();
//
//	// 2) event 객체의 값을 설정한다.
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role);
//	pevent_server_log->event_type	 = eEVENT_TYPE::SERVER_DISCONNECT;
//	pevent_server_log->state		 = eOBJECT_STATE::NONE;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server	 = _query->m_peer_address;
//
//	// 3) event log를 추가한다.
//	g_pservice->add_event_log(pevent_server_log);
//}
//
//void querier_server::process_wal_restore(query_event::server_certify* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::server_certify* _query)
//{
//	// 1) event 객체를 생성한다.
//	auto pevent_server_log = make_object<server_event_log>();
//
//	// 2) event 객체의 값을 설정한다.
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role);
//	pevent_server_log->event_type	 = eEVENT_TYPE::SERVER_CONNECT;
//	pevent_server_log->state		 = eOBJECT_STATE::NONE;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server	 = _query->m_peer_address;
//
//	// 3) event log를 추가한다.
//	g_pservice->add_event_log(pevent_server_log);
//
//	if (_query->m_psocket.exist())
//	{
//		// declare) 
//		sMESSAGE_NETWORK msg_send(CGNOTIFY_SERVER_INIT_COMPLETE, _query->m_psocket.get());
//
//		// - 전송한다.
//		_query->m_psocket->transmit_message(msg_send);
//	}
//}
//
//void querier_server::process_wal_restore(query_event::server_close* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::server_close* _query)
//{
//	// 1) event 객체를 생성한다.
//	auto pevent_server_log = make_object<server_event_log>();
//
//	// 2) event 객체의 값을 설정한다.
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role) ;
//	pevent_server_log->event_type	 = eEVENT_TYPE::SERVER_DISCONNECT;
//	pevent_server_log->state		 = eOBJECT_STATE::NONE;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server	 = _query->m_peer_address;
//
//	// 3) event log를 추가한다.
//	g_pservice->add_event_log(pevent_server_log);
//}
//
//void querier_server::process_wal_restore(query_event::server_state* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::server_state* _query)
//{
//	// 1) event 객체를 생성한다.
//	auto pevent_server_log = make_object<server_event_log>();
//
//	// 2) event 객체의 값을 설정한다.
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role) ;
//	pevent_server_log->event_type	 = eEVENT_TYPE::SERVER_STATE;
//	pevent_server_log->state		 = _query->m_state;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server	 = _query->m_peer_address;
//
//	// 3) event log를 추가한다.
//	g_pservice->add_event_log(pevent_server_log);
//}
//
//void querier_server::process_wal_restore(query_event::market_state* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::market_state* _query)
//{
//	// 1) event 객체를 생성한다.
//	auto pevent_server_log = new sSERVER_EVENT_LOG;
//
//	// 2) event 객체의 값을 설정한다.
//	pevent_server_log->time_occure	 = chrono::time::utc::now();
//	pevent_server_log->eServerType	 = static_cast<eSERVER_TYPE>(_query->m_info_server_setting.rank.role);
//	pevent_server_log->event_type	 = eEVENT_TYPE::MARKET_STATE;
//	pevent_server_log->state		 = _query->m_state;
//	pevent_server_log->id_server	 = _query->m_info_server_setting.id;
//	pevent_server_log->address_server	 = _query->m_peer_address;
//
//	// log) 
//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_LOG_ADD, reinterpret_cast<uintptr_t>(pevent_server_log)));
//	g_pservice->m_pgroup_admin_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_LOG_ADD::MAKE_BUFFER(*pevent_server_log));
//}
//
//void querier_server::process_wal_restore(query_event::market_status* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::market_status* /*_query*/)
//{
//}
//
//void querier_server::process_wal_restore(query_event::time_event_register* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::time_event_register* _query)
//{
//	g_pservice->m_pmanager_time_event->_ProcessCompleteTimeEventRegister(_query->m_pserver_operation, eRESULT::SUCCESS);
//}
//
//void querier_server::process_wal_restore(query_event::time_event_unregister* /*_query*/)
//{
//
//}
//
//void querier_server::process_wal_appended(query_event::time_event_unregister* _query)
//{
//	g_pservice->m_pmanager_time_event->_ProcessCompleteTimeEventUnregister(_query->m_pserver_operation);
//}
//
//void querier_server::process_wal_restore(query_event::time_event_process* /*_query*/)
//{
//
//}
//
//void querier_server::process_wal_appended(query_event::time_event_process* /*_query*/)
//{
//}
//
//void querier_server::process_wal_restore(query_event::time_event_done* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::time_event_done* /*_query*/)
//{
//}
//
//void querier_server::process_wal_restore(query_event::time_event_update* _query)
//{
//	_query->m_check_duplicate = true;
//}
//
//void querier_server::process_wal_appended(query_event::time_event_update* _query)
//{
//	g_pservice->m_pmanager_time_event->_process_complete_time_event_update(_query->m_pserver_operation);
//}
