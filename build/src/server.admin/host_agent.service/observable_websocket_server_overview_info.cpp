#include "pch.h"


object_ptr<time_event::Ientity> find_time_event_entity(uint64_t _id);
void _append_event_object(buffer& _buf_dest, const server_admin::time_event_snap& _pentity);
void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::string_view _str_value);
void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::size_t _value);
void _append_log_info(shared_buffer& _buf_out, const LOG_RECORD* _plog_record);

shared_buffer _make_webmessage_event_list(const std::vector<server_admin::time_event_snap>& _vector_time_event)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_EVENT_LIST_OBJECT_INFO); // "CVLI"

	// 3) append message 
	{
		for (const auto& iter : _vector_time_event)
		{
			// - temporary buffer
			buffer buf_text = buf_temp;

			// - tr element id
			auto tr_element_id = NFMT::format("tr{}", iter.updatable.id);

			// - append tr data
			_append_event_object(buf_text, iter);

			// - append command
			buf_result.append<char>('A');

			// - append element id
			buf_result.append_text<char>(tr_element_id);
			buf_result.append<char>('\n');

			// - append length
			buf_result.append_text<char>(buf_text.size());
			buf_result.append<char>('\n');

			// - append data
			buf_result.append(buf_text.size(), buf_text.data());
		}
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_log_count(std::array<size_t, MAX_LOG_TYPE>& _array_count_last)
{
	// declare) 
	size_t log_count_total = 0;
	std::array<size_t, MAX_LOG_TYPE> array_log_count;

	// 1) get logger
	auto plogger = service::get_logger();

	// 2) get loc count
	scoped_lock(plogger->get_count_lock())
	{
		log_count_total = plogger->get_log_count();
		array_log_count = plogger->_log_count();
		_array_count_last = array_log_count;
	}

	// 3) alloc buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 4) append message head
	buf_text.append_text<char>(MESSAGE_WEB_GENERAL_OBJECT_INFO); // "CGNR"

	// 5) append log_counts
	::_append_key_value(buf_text, "log_count_total"sv, log_count_total);
	::_append_key_value(buf_text, "log_count_info"sv, array_log_count[static_cast<int>(log_type_t::info)]); // 0
	::_append_key_value(buf_text, "log_count_progress"sv, array_log_count[static_cast<int>(log_type_t::progress)]); // 1
	::_append_key_value(buf_text, "log_count_debug"sv, array_log_count[static_cast<int>(log_type_t::debug)]); // 2
	::_append_key_value(buf_text, "log_count_fatal"sv, array_log_count[static_cast<int>(log_type_t::fatal)]); // 3
	::_append_key_value(buf_text, "log_count_error"sv, array_log_count[static_cast<int>(log_type_t::error)]); // 4
	::_append_key_value(buf_text, "log_count_warning"sv, array_log_count[static_cast<int>(log_type_t::warning)]); // 5
	::_append_key_value(buf_text, "log_count_user"sv, array_log_count[static_cast<int>(log_type_t::user)]); // 6
	::_append_key_value(buf_text, "log_count_system"sv, array_log_count[static_cast<int>(log_type_t::system)]); // 7

	// return) make shared buffer and return
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));;
}

template<class TITER>
shared_buffer _make_webmessage_log(std::string_view _command, TITER _begin, TITER _end)
{
	// 1) calculate memory size
	size_t size_buffer = 4;
	for (auto iter = _begin; iter != _end; ++iter)
	{
		size_buffer += (*iter)->buf_message.size() * 4 + 256;
		size_buffer += (*iter)->get_line_count() * 256;
		size_buffer += 1024;
	}

	// 2) alloc buffer
	auto buf_text = alloc_shared_buffer(size_buffer);

	// 3) append message head
	buf_text.append_text<char>(_command); // "CLOG"

	// 4) append message
	for (auto iter = _begin; iter != _end; ++iter)
	{
		// - append log
		_append_log_info(buf_text, *iter);

		// - append text
		buf_text.append_text<char>("<br>"sv);
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));
}

shared_buffer _make_web_message_log(std::string_view _command, const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	return _make_webmessage_log(_command, _vector_log_record.begin(), _vector_log_record.end());
}

shared_buffer _make_web_message_log(std::string_view _command, circular_list<object_ptr<LOG_RECORD>>& _list_plog_record)
{
	return _make_webmessage_log(_command, _list_plog_record.begin(), _list_plog_record.end());
}

observable_websocket_peer_overview_info::observable_websocket_peer_overview_info(const std::string_view _name) :
	m_is_count_log_count_updating(false),
	m_array_log_count_last{ 0,0,0,0,0,0,0,0 }
{
	set_name(_name);

	m_webpage_str_target = "webpage_server_overview_info.htm";
	m_webpage_index = 'A';
}

observable_websocket_peer_overview_info::~observable_websocket_peer_overview_info()
{
}

void observable_websocket_peer_overview_info::request_update_webpage_time_event()
{
	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare) 
	shared_buffer buf_send;

	// 1) make message
	scoped_lock(this->m_pservice_entity->m_cs_instance)
	{
		buf_send = _make_webmessage_event_list(this->m_pservice_entity->m_instance.status.vector_time_event);
	}

	// 2) 모두에게 전송한다.
	scoped_lock(this->m_lockable_observable)
	{
		for (auto& iter : this->m_container_observer)
		{
			iter.pobserver->send(buf_send);
		}
	}
}

void observable_websocket_peer_overview_info::add_log_records(const circular_list<object_ptr<LOG_RECORD>>& _list_log_record)
{
	// 1) send log message
	this->send(_make_webmessage_log(MESSAGE_WEB_LOG_OBJECT_INFO, _list_log_record.begin(), _list_log_record.end()));

	//// 2) send updating log count message
	//scoped_lock(service::get_logger()->get_count_lock())
	//{
	//	// check)
	//	RETURN_IF(this->m_is_count_log_count_updating == true);

	//	// - flag set true
	//	this->m_is_count_log_count_updating = true;
	//}

	//// 3) request
	//system_executor::post(make_executable(this, &observable_websocket_peer_overview_info::process_send_webmessage_log_count_update));
}

void observable_websocket_peer_overview_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare) 
	std::array<shared_buffer, 3> array_buffer;

	scoped_lock(this->m_pservice_entity->m_cs_instance)
	{
		// 2) make server list/event list
		array_buffer[0]  = _make_webmessage_event_list(this->m_pservice_entity->m_instance.status.vector_time_event);

		// 3) make stored log
		array_buffer[1] = _make_webmessage_log_count(this->m_array_log_count_last);
		array_buffer[2] = _make_web_message_log(MESSAGE_WEB_LOG_OBJECT_INFO, this->m_pservice_entity->m_instance.status.vector_log_record);
	}

	// 4) send all
	for (auto& iter : array_buffer)
		_pobserver->send(iter);
}

uintptr_t observable_websocket_peer_overview_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}


//template<class TITER>
//shared_buffer _make_webmessage_log(TITER _begin, TITER _end)
//{
//	// 1) calculate memory size
//	size_t size_buffer = 4;
//	for (auto iter = _begin; iter != _end; ++iter)
//	{
//		size_buffer += (*iter)->buf_message.size() * 4 + 64 + 64;
//	}
//
//	// 2) alloc buffer
//	auto buf_text = alloc_shared_buffer(size_buffer);
//
//	// 3) append message head
//	buf_text.append_text<char>(MESSAGE_WEB_LOG_OBJECT_INFO); // "CLOG"
//
//	// 4) append message
//	for (auto iter = _begin; iter != _end; ++iter)
//	{
//		// - append log
//		_append_log_info(buf_text, *iter);
//
//		// - append text
//		buf_text.append_text<char>("<br>"sv);
//	}
//
//	// 5) send message
//	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));
//}
//shared_buffer _make_webmessage_log_count(std::array<size_t, MAX_LOG_TYPE>& _array_count_last);

//_make_webmessage_log(list_log_record.begin(), list_log_record.end());
//_make_webmessage_log_count(std::array<size_t, MAX_LOG_TYPE>& _array_count_last);

result_code observable_websocket_peer_overview_info::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message
	auto& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get message head
	auto message_dest = msg.buf_message.data<char>()[0];

	// 3) process message by dest
	switch(message_dest)
	{
	// case) message - time_event 
	case	'E':
			return on_message_time_event(msg);
			break;

	// case) message - sever
	case	'S':
			break;

	default:
			break;
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code observable_websocket_peer_overview_info::on_message_time_event(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) get message type
	auto message_type = _msg.buf_message.extract_text<char>(size(4));

	// 2) get target id
	auto element_id = __from_string<uint64_t>((_msg.buf_message + offset(2)).to_string<char>());

	// 3) find time event object
	object_ptr<time_event::Ientity> ptime_event_entity = find_time_event_entity(element_id);

	// check)
	if (ptime_event_entity.empty())
	{
		// return)
		return eRESULT::DONE;
	}

	// 4) time_event add
	if (message_type == "EVAD")
	{
		//send(make_shared_buffer(sWEBSOCKET_MESSAGE(buf_binary)));

		// return)
		return eRESULT::DONE;
	}
	// 5) time_event remove
	else if (message_type == "EVRV")
	{
		ptime_event_entity->destroy();

		// return)
		return eRESULT::DONE;
	}
	// 6) time_event start
	else if (message_type == "EVST")
	{
		ptime_event_entity->start();

		// return)
		return eRESULT::DONE;
	}
	// 7) time_event pause
	else if (message_type == "EVPU")
	{
		if (ptime_event_entity->get_object_state() == eOBJECT_STATE::PAUSED
			|| ptime_event_entity->get_object_state() == eOBJECT_STATE::PAUSE_PENDING)
		{
			ptime_event_entity->resume();
		}
		else if (ptime_event_entity->get_object_state() == eOBJECT_STATE::START_PENDING
			|| ptime_event_entity->get_object_state() == eOBJECT_STATE::RUNNING)
		{
			ptime_event_entity->pause();
		}

		// return)
		return eRESULT::DONE;
	}
	// 8) time_event stop
	else if (message_type == "EVSP")
	{
		ptime_event_entity->stop();

		// return)
		return eRESULT::DONE;
	}

	// return)
	return eRESULT::BYPASS;
}

void observable_websocket_peer_overview_info::process_send_webmessage_log_count_update()
{
	// declare) 
	size_t log_count_total = 0;
	std::array<size_t, MAX_LOG_TYPE> array_log_count;

	// 1) get logger
	auto plogger = service::get_logger();

	// 2) get loc count
	scoped_lock(plogger->get_count_lock())
	{
		// check)
		CGASSERT_ERROR(m_is_count_log_count_updating == true);

		// - check) 
		log_count_total = plogger->get_log_count();
		array_log_count = plogger->_log_count();
	}

	// 3) alloc buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 4) append message head
	buf_text.append_text<char>(MESSAGE_WEB_GENERAL_OBJECT_INFO); // "GNRL"

	// 5) append log_count.total
	::_append_key_value(buf_text, "log_count_total"sv, log_count_total);

	// 6) append log_count.total
	if(this->m_array_log_count_last.at(0) != array_log_count.at(0))
		::_append_key_value(buf_text, "log_count_info"sv, array_log_count[static_cast<int>(log_type_t::info)]);			// 0

	if (this->m_array_log_count_last.at(1) != array_log_count.at(1))
		::_append_key_value(buf_text, "log_count_progress"sv, array_log_count[static_cast<int>(log_type_t::progress)]);	// 1

	if (this->m_array_log_count_last.at(2) != array_log_count.at(2))
		::_append_key_value(buf_text, "log_count_debug"sv, array_log_count[static_cast<int>(log_type_t::debug)]);		// 2

	if (this->m_array_log_count_last.at(3) != array_log_count.at(3))
		::_append_key_value(buf_text, "log_count_fatal"sv, array_log_count[static_cast<int>(log_type_t::fatal)]);		// 3

	if (this->m_array_log_count_last.at(4) != array_log_count.at(4))
		::_append_key_value(buf_text, "log_count_error"sv, array_log_count[static_cast<int>(log_type_t::error)]);		// 4

	if (this->m_array_log_count_last.at(5) != array_log_count.at(5))
		::_append_key_value(buf_text, "log_count_warning"sv, array_log_count[static_cast<int>(log_type_t::warning)]);	// 5

	if (this->m_array_log_count_last.at(6) != array_log_count.at(6))
		::_append_key_value(buf_text, "log_count_user"sv, array_log_count[static_cast<int>(log_type_t::user)]);			// 6

	if (this->m_array_log_count_last.at(7) != array_log_count.at(7))
		::_append_key_value(buf_text, "log_count_system"sv, array_log_count[static_cast<int>(log_type_t::system)]);		// 7

	// 7) send
	this->send(make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>())));

	// 8) re-request or stop
	scoped_lock(plogger->get_count_lock())
	{
		// - store last log count
		this->m_array_log_count_last = array_log_count;

		// - stop if no-log posted
		if (log_count_total == plogger->get_log_count())
		{
			this->m_is_count_log_count_updating = false;

			// return)
			return;
		}
	}

	// 9) request
	system_executor::post(make_executable(this, &observable_websocket_peer_overview_info::process_send_webmessage_log_count_update));
}