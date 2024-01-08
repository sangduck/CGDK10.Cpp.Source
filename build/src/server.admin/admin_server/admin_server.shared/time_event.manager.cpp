#include "pch.h"


time_event_server::manager::manager(std::string_view _name) :
	Nnameable(_name),
	m_idEvent_Last(0)
{
}
time_event_server::manager::~manager()
{
}

void time_event_server::manager::register_time_event(time_event_server::operation* /*_ptime_event*/)
{
	//// 1) id_event값들을 설정한다.
	//_ProcessAllocEventID_New(_ptime_event);
	//_ptime_event->set_setup_time(chrono::time::utc::now());

	//// 2) 요청한다.
	//g_pservice->m_pquerier_server->request_register_time_event(_ptime_event);
}
void time_event_server::manager::unregister_time_event(time_event_server::operation* _ptime_event) noexcept
{
	// 1) id_event값들을 설정한다.
	_ProcessAllocEventID_Update(_ptime_event);

	// 2) 요청한다.
	g_pservice->m_pquerier_server->request_unregister_time_event(_ptime_event);
}
void time_event_server::manager::update_time_event(time_event_server::operation* /*_pserver_operation*/)
{
	//g_pservice->m_pquerier_server->request_update_time_event(_pserver_operation, _process_alloc_event_id());
}
void time_event_server::manager::store_time_event_result(server_operation* _pserver_operation)
{
	g_pservice->m_pquerier_server->request_store_time_event_result(_pserver_operation, _process_alloc_event_id());
}

void time_event_server::manager::done_time_event_result(operation* _ptime_event)
{
	g_pservice->m_pquerier_server->request_done_time_event_result(_ptime_event, _process_alloc_event_id());
}

object_ptr<time_event_server::operation> time_event_server::manager::find_time_event(uint64_t /*_uid*/) const noexcept
{
	// declare) 
	object_ptr<time_event_server::operation> ptime_event;

	//// 1) Event Manager를 얻는다.
	//auto pmanager_event = time_event::entity_manager::get_instance();

	//// 2) 해당 Event를 찾는다.
	//scoped_lock(*pmanager_event)
	//{
	//	for(auto& iter: pmanager_event->m_set_event)
	//	{
	//		// - Casting...
	//		auto poperation = dynamic_pointer_cast<time_event_server::operation>(iter);

	//		// check)
	//		CONTINUE_IF(poperation.empty());

	//		// - 얻는다.
	//		if(poperation->get_time_event_id() == static_cast<int64_t>(_uid))
	//		{
	//			ptime_event = poperation;
	//			break;
	//		}
	//	}
	//}

	// return)
	return ptime_event;
}

void time_event_server::manager::on_initializing(context& /*_context*/)
{
}
void time_event_server::manager::on_initialize(context& /*_context*/)
{
}
void time_event_server::manager::on_destroying()
{
}
void time_event_server::manager::on_destroy()
{
}

void time_event_server::manager::on_starting(context& /*_context*/)
{
	// 1) Notify등록한다.
	time_event::entity_manager::get_instance()->register_messageable(this);

}
void time_event_server::manager::on_start(context& /*_context*/)
{
	//{
	//	// 1) query를 요청한다.
	//	auto	pquery = g_pservice->m_pquerier_server->execute_query(std::string_view("{ CALL get_time_event_server }"));

	//	// declare) 
	//	std::map<uint64_t, object_ptr<time_event_server::operation>> map_time_event_server;

	//	// 2) time event 정보를 읽어온다.
	//	for (auto& iter : *pquery)
	//	{
	//		// declare) 
	//		auto id_event			 = iter.get_value<uint64_t>(1);
	//		//auto id_source			 = iter.get_value<uint64_t>(2);
	//		auto id_group			 = iter.get_value<uint64_t>(3);
	//		auto timeSetup			 = iter.get_value<chrono::time::utc::time_point>(4);
	//		auto Name				 = iter.get_value<std::string_view>(5);
	//		auto eTimeEventType		 = iter.get_value<time_event::eTYPE>(6);
	//		auto eTimeEventSetter	 = iter.get_value<time_event::eSETTER>(7);
	//		auto TimeEventLevel		 = iter.get_value<int>(8);
	//		auto time_next			 = iter.get_value<chrono::time::utc::time_point>(9);
	//		//auto status				 = iter.get_value<int32_t>(10);
	//		//auto operation			 = iter.get_value<eEVENT_OPERATION>(11);
	//		auto hid					 = iter.get_value<uint64_t>(12);
	//		auto id_server			 = iter.get_value<uint64_t>(13);
	//		auto setting			 = iter.get_value<std::string_view>(14);
	//		auto times				 = iter.get_value<int>(15);

	//		// declare) 
	//		auto pevent_operation = make_object<time_event_server::operation>();

	//		// - 먼저 해당 id_group이 존재하는지 찾는다.
	//		auto iter_find = map_time_event_server.find(id_group);

	//		// - 있으면 그것을 사용한다.
	//		if(iter_find != map_time_event_server.end())
	//		{
	//			pevent_operation = iter_find->second;
	//		}
	//		else
	//		{
	//			// -신규 TimeEventOperation을 생성한다.
	//			pevent_operation = make_object<time_event_server::operation>();

	//			// - 추가한다.
	//			map_time_event_server.insert({ id_group, pevent_operation });
	//		}

	//		// - Name이 존재할 경우 pevent_operation에 바로 쓰기...
	//		if(Name.empty() == false)
	//		{
	//			// - 값을 설정한다.
	//			pevent_operation->set_name				(Name);
	//			pevent_operation->set_time_event_id		(id_event);
	//			pevent_operation->set_time_event_type	(eTimeEventType);
	//			pevent_operation->set_time_event_setter	(eTimeEventSetter);
	//			pevent_operation->set_event_level		(TimeEventLevel);
	//			pevent_operation->set_setup_time		(timeSetup);
	//		}
	//		// - Name이 존재하지 않는 경우 ServerOperation을 생성하기.
	//		else
	//		{
	//			// - 신규 ServerEvent를 생성한다.
	//			auto	pserver_operation = make_object<time_event_server::server_operation>();

	//			// - 값을 설정한다.
	//			pserver_operation->m_operation.id_event			 = id_event;
	//			pserver_operation->m_operation.id_group			 = id_group;
	//			pserver_operation->m_operation.hid.mac.sll_mac	 = hid;
	//			pserver_operation->m_operation.id_server.key	 = id_server;
	//			pserver_operation->m_operation.setting			 = std::move(setting);
	//			pserver_operation->time_event_next				 = time_next;
	//			pserver_operation->count_times					 = times;
	//			pserver_operation->count_executed				 = 0;

	//			// - 추가한다.
	//			pevent_operation->add_schedule(pserver_operation);
	//		}
	//	}

	//	// check) 다음 Result를 얻어온다.
	//	THROW_IF(pquery->get_next_result() != eRESULT::SUCCESS, throwable::failure(eRESULT::FAIL));

	//	// 2) uid_last와 idEvent_last를 얻어온다.
	//	for (auto& iter : *pquery)
	//	{
	//		m_idEvent_Last = iter.get_value<uint64_t>(1);
	//	}

	//	// - query를 Close한다.
	//	pquery->close_query();

	//	// 3) 모든 ServerEvent를 등록한다.
	//	for(auto& iter: map_time_event_server)
	//	{
	//		iter.second->register_event();
	//	}
	//}
}
void time_event_server::manager::on_stopping()
{
}
void time_event_server::manager::on_stop()
{
	// 1) 모든 time_event를 제거한다.
	time_event::entity_manager::get_instance()->unregister_event_all();

	// 2) Notify 등록해제한다.
	time_event::entity_manager::get_instance()->register_messageable(this);
}

result_code time_event_server::manager::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) Message Map
		EXECUTE_CGMESSAGE_MAP(_msg);
	}
	catch (...)
	{
	}

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

BEGIN_CGMESSAGE_MAP(time_event_server::manager)
	// 1) Message from Socket Chatting Server
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,		on_message_SERVER_TIME_EVENT_REGISTER_EVENT,sMESSAGE_NETWORK);
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,	on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,	sMESSAGE_NETWORK);
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE,		on_message_SERVER_TIME_EVENT_EVENT_EXECUTE,	sMESSAGE_NETWORK);
	ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE,		on_message_SERVER_TIME_EVENT_EVENT_UPDATE,	sMESSAGE_NETWORK);
END_CGMESSAGE_MAP


result_code time_event_server::manager::on_message_SERVER_TIME_EVENT_REGISTER_EVENT(sMESSAGE_NETWORK& /*_msg*/)
{
//	// 1) psource를 얻는다.
//	auto psource = _msg.get_source<time_event::Ientity>();
//
//	// 2) Casting한다.
//	auto pserver_time_event = dynamic_cast<time_event_server::operation*>(psource);
//
//	// check)
//	RETURN_IF(pserver_time_event == nullptr, 1);
//
//	// notify) 
//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_REGISTER, reinterpret_cast<uintptr_t>(pserver_time_event)));
//	vector<sDATA_SERVER_OPERATION> v; for (auto& iter : pserver_time_event->m_queue_event) { auto x= dynamic_cast<time_event_server::server_operation*>(iter.get()); v.push_back({*(EVENT_SCHEDULE*)x, x->m_operation});}
//	g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_ADD::MAKE_BUFFER(pserver_time_event->get_time_event_info(), v));
//
//	// log) ...
//	LOG_INFO << " > SERVER_TIME_EVENT '" << pserver_time_event->get_name<char>() << "' registered";

	// return) 
	return eRESULT::DONE;
}

result_code time_event_server::manager::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
//	// 1) psource를 얻는다.
//	auto psource = _msg.get_source<time_event::Ientity>();
//
//	// 2) Casting한다.
//	auto pserver_time_event = dynamic_cast<time_event_server::operation*>(psource);
//
//	// check)
//	RETURN_IF(pserver_time_event == nullptr, 1);
//
//	// notify) 
//	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_time_event)));
//	g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_REMOVE::MAKE_BUFFER(pserver_time_event->get_time_event_id()));
//
//	// log) ...
//	LOG_INFO << " < SERVER_TIME_EVENT '" << pserver_time_event->get_name<char>() << "' unregistered";

	// return) 
	return eRESULT::DONE;
}
result_code time_event_server::manager::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) psource를 얻는다.
	//auto psource = _msg.get_source<time_event::Ientity>();

	//// 2) Casting한다.
	//auto pserver_time_event = dynamic_cast<time_event_server::operation*>(psource);

	//// check)
	//RETURN_IF(pserver_time_event == nullptr, 1);

	//// notify) 
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_UPDATE, reinterpret_cast<uintptr_t>(pserver_time_event)));

	//vector<sDATA_SERVER_OPERATION> v; for (auto& iter : pserver_time_event->m_queue_event) { auto x = dynamic_cast<time_event_server::server_operation*>(iter.get()); v.push_back({ *(EVENT_SCHEDULE*)x, x->m_operation }); }
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_UPDATE::MAKE_BUFFER(pserver_time_event->get_time_event_info(), v));

	// return) 
	return eRESULT::DONE;
}
result_code time_event_server::manager::on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	// return) 
	return eRESULT::DONE;
}

shared_buffer time_event_server::manager::message_SERVER_ADMIN_SERVER_TIME_EVENT()
{
	// declare) 
	vector<sADMIN_SERVER_TIME_EVENT> vector_admin_server_event;

	//// 1) 메시지 내용을 써넣는다.
	//auto& set_event = time_event::entity_manager::get_instance()->m_set_event;

	//scoped_lock(set_event)
	//{
	//	// - Reverve한다.
	//	vector_admin_server_event.reserve(set_event.size());

	//	// - 데이터를 추가한다.
	//	for (auto& iter : set_event)
	//	{
	//		// declare)
	//		sADMIN_SERVER_TIME_EVENT	temp;

	//		auto poperation = dynamic_pointer_cast<time_event_server::operation>(iter);

	//		// check) operation 객체가 아니면 다음으로 넘어간다.
	//		CONTINUE_IF(poperation.empty());

	//		temp.infoEvent = poperation->get_time_event_info();
	//		temp.vectorServerOperation.reserve(poperation->m_queue_event.size());

	//		for(auto& iter_nest: poperation->m_queue_event)
	//		{
	//			// - Casting한다.
	//			auto pserver_operation = dynamic_pointer_cast<time_event_server::server_operation>(iter_nest);

	//			// check)
	//			CGASSERT_ERROR(pserver_operation.exist());

	//			// - 추가한다.
	//			sDATA_SERVER_OPERATION tempoperation;
	//			tempoperation.eventSchedule = *(EVENT_SCHEDULE*)pserver_operation;
	//			tempoperation.ServerOperation = pserver_operation->m_operation;

	//			temp.vectorServerOperation.push_back(tempoperation);
	//		}

	//		// - 추가한다.
	//		vector_admin_server_event.push_back(std::move(temp));
	//	}
	//}
	
	// 2) 버퍼를 할당받는다.
	shared_buffer buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(vector_admin_server_event));

	// 3) 메시지 헤더를 써넣는다.
	buf_temp.append<uint32_t>();
	buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_SERVER_TIME_EVENT_LIST);

	// 4) 메시지 내용을 써넣는다.
	buf_temp.append(vector_admin_server_event);

	// 5) 메시지 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

void time_event_server::manager::_ProcessAllocEventID_New(time_event_server::operation* /*_ptime_event*/) noexcept
{
	//scoped_lock(m_csid)
	//{
	//	// - Event ID를 설정한다.
	//	_ptime_event->set_time_event_id(++m_idEvent_Last);

	//	for (auto& iter : _ptime_event->m_queue_event)
	//	{
	//		auto poperation = dynamic_pointer_cast<time_event_server::server_operation>(iter);

	//		// check)
	//		CGASSERT_ERROR(poperation.exist());

	//		// - id_event를 써넣는다.
	//		poperation->m_operation.id_event = ++m_idEvent_Last;
	//		poperation->m_operation.id_group = _ptime_event->get_time_event_id();
	//	}
	//}
}
void time_event_server::manager::_ProcessAllocEventID_Update(time_event_server::operation* /*_ptime_event*/) noexcept
{
	//// Update용은 기존의 id_event값이 필요하기 ㄸ때문에 이전의 id_event값을 id_source로 옮겨놓는 작업을 수행한다.

	//scoped_lock(m_csid)
	//{
	//	// - Event ID를 설정한다.
	//	_ptime_event->m_idSource = _ptime_event->get_time_event_id();
	//	_ptime_event->set_time_event_id(++m_idEvent_Last);

	//	for (auto& iter : _ptime_event->m_queue_event)
	//	{
	//		auto poperation = dynamic_pointer_cast<time_event_server::server_operation>(iter);

	//		// check)
	//		CGASSERT_ERROR(poperation.exist());

	//		// - id_event를 써넣는다.
	//		poperation->m_operation.id_source = poperation->m_operation.id_event;
	//		poperation->m_operation.id_event = ++m_idEvent_Last;
	//		poperation->m_operation.id_group = _ptime_event->get_time_event_id();
	//	}
	//}
}

uint64_t time_event_server::manager::_process_alloc_event_id() noexcept
{
	scoped_lock(m_csid)
	{
		return ++m_idEvent_Last;
	}
}
void time_event_server::manager::_ProcessCompleteTimeEventRegister(time_event_server::operation* /*_ptime_event*/, eRESULT /*_result*/)
{
	//try
	//{
	//	// check) _ptime_event가 nullptr이면 아됀다.
	//	CGASSERT_ERROR(_ptime_event != nullptr);

	//	// check) _ptime_event가 nullptr이면 아됀다.
	//	THROW_IF(_ptime_event == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	//	// check) query에 실패했으면 Exception을 던진다.
	//	CGASSERT_ERROR(_result == eRESULT::SUCCESS);

	//	// check) query에 실패했으면 Exception을 던진다.
	//	THROW_IF(_result != eRESULT::SUCCESS, throwable::failure(_result));

	//	// 1) time_event_server를 등록한다.
	//	auto result = _ptime_event->register_event();

	//	// check) 등록에 실패했을 경우 Exception을 던진다.
	//	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
	//}
	//catch (...)
	//{
	//	// log) ...
	//	LOG_ERROR(nullptr, "Fail to register SERVER_TIME_EVENT '%s'", _ptime_event->get_name<char>().c_str());
	//}
}

void time_event_server::manager::_ProcessCompleteTimeEventUnregister(time_event_server::operation* _ptime_event)
{
	// check) _ptime_event가 nullptr이면 아됀다.
	CGASSERT_ERROR(_ptime_event != nullptr);

	// check) _ptime_event가 nullptr이면 아됀다.
	RETURN_IF(_ptime_event == nullptr);

	// 3) unregister한다.
	_ptime_event->unregister_event();
}

void time_event_server::manager::_process_complete_time_event_update(time_event_server::operation* /*_pserver_time_event*/)
{
	//// check) _pserver_time_event가 nullptr이면 아됀다.
	//CGASSERT_ERROR(_pserver_time_event != nullptr);

	//// check) _pserver_time_event가 nullptr이면 아됀다.
	//RETURN_IF(_pserver_time_event == nullptr);

	//// notify) 
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_SERVER_TIME_EVENT_UPDATE, reinterpret_cast<uintptr_t>(_pserver_time_event)));

	//vector<sDATA_SERVER_OPERATION> v; for (auto& iter : _pserver_time_event->m_queue_event) { auto x = dynamic_cast<time_event_server::server_operation*>(iter.get()); v.push_back({ *(EVENT_SCHEDULE*)x, x->m_operation }); }
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_TIME_EVENT_UPDATE::MAKE_BUFFER(_pserver_time_event->get_time_event_info(), v));
}
