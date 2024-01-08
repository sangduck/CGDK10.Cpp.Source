#include "pch.h"


void _append_executor_entity_add(buffer& _buf_dest, const server_admin::executor_snap& _statistics_executor)
{
	// 1) executor "NAME"
	_buf_dest.append_text<char>("<span style='font-size:14px;font-weight:bold;overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_statistics_executor.name);
	_buf_dest.append_text<char>("\">");
	switch(_statistics_executor.type)
	{
	case	eEXECUTOR_TYPE::SYSTEM:
			_buf_dest.append_text<char>("<i class='fa fa-cogs'></i> ");
			break;

	default:
			_buf_dest.append_text<char>("&nbsp;&nbsp;&nbsp;");
			break;
	}
	_buf_dest.append_text<char>(_statistics_executor.name);
	_buf_dest.append_text<char>("</span>");

	// 2) threads
	if (_statistics_executor.updatable.vector_threads.empty() == false)
	{
		_buf_dest.append_text<char>("<ul style='padding:5px'>");

		int index = 0;
		for (auto& iter : _statistics_executor.updatable.vector_threads)
		{
			_buf_dest.append_text<char>("<li>");

			_buf_dest.append_text<char>("<table width='400px' cellspacing='4'><tbody><tr id='");
			auto er_element_id = NFMT::format("xt{}", iter.id_thread);
			_buf_dest.append_text<char>(er_element_id);
			_buf_dest.append_text<char>("' align='left'><td width='140px' align='right'>thread#");
			_buf_dest.append_text<char>(index);
			_buf_dest.append_text<char>("</td><td width='40px' class='texecutor_thread'>");
			_buf_dest.append_text<char>(iter.id_thread);
			_buf_dest.append_text<char>("</td><td width='80px' class='texecutor_thread'>");
			_buf_dest.append_text<char>(iter.count_executed);
			_buf_dest.append_text<char>("</td><td width='80px' class='texecutor_thread'>");
			_buf_dest.append_text<char>(iter.count_error);
			_buf_dest.append_text<char>("</td></tr></tbody></table>");

			_buf_dest.append_text<char>("</li>");

			++index;
		}

		_buf_dest.append_text<char>("</ul>");
	}
}

void _append_executor_entity_update(buffer& _buf_dest, const server_admin::executor_snap& _statistics_executor)
{
	// 1) executor "name"
	_buf_dest.append<char>(' ');
	_buf_dest.append_text<char>(_statistics_executor.name);
	_buf_dest.append_text<char>("\n");

	// 2) count
	_buf_dest.append_text<char>(_statistics_executor.updatable.vector_threads.size());
	_buf_dest.append_text<char>("\n");

	// 3) threads
	for (auto& iter : _statistics_executor.updatable.vector_threads)
	{
		auto er_element_id = NFMT::format("xt{}", iter.id_thread);
		_buf_dest.append_text<char>(er_element_id);
		_buf_dest.append_text<char>("\n");

		_buf_dest.append_text<char>(iter.count_executed);
		_buf_dest.append_text<char>("\n");
		_buf_dest.append_text<char>(iter.count_error);
		_buf_dest.append_text<char>("\n");
	}
}

void _append_executable_entity_add(buffer& _buf_dest, const server_admin::factory_snap& _executable_snap)
{
	// 1) "name"
	_buf_dest.append_text<char>("<td style='overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_executable_snap.name);
	_buf_dest.append_text<char>("\">");

	switch(_executable_snap.type)
	{
	case	eFACTORY_TYPE::POOL_MEMORYBLOCK:
			_buf_dest.append_text<char>("<i class='fa fa-th'></i> ");
			break;

	case	eFACTORY_TYPE::POOL_SYSTEM:
	case	eFACTORY_TYPE::ALLOCATOR_SYSTEM:
			_buf_dest.append_text<char>("<i class='fa fa-cogs'></i> ");
			break;

	case	eFACTORY_TYPE::ALLOCATOR_USER:
	case	eFACTORY_TYPE::POOL_USER:
			_buf_dest.append_text<char>("&nbsp;&nbsp;&nbsp; ");
			break;

	default:
			_buf_dest.append_text<char>("&nbsp;&nbsp;&nbsp; ");
			break;
	}

	_buf_dest.append_text<char>(_executable_snap.name);
	_buf_dest.append_text<char>("</td>");

	// 2) TIME.NEXT
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_executable_snap.updatable.now.extra);
	_buf_dest.append_text<char>("</td>");
}

void _append_executable_entity_update(buffer& _buf_dest, const server_admin::factory_snap& _executable_snap)
{
	// 1) EXECUTED
	_buf_dest.append_text<char>(_executable_snap.updatable.now.extra);
	_buf_dest.append_text<char>("\n");
}

void _append_schedulable_entity_add(buffer& _buf_dest, const server_admin::schedulable_snap& _schedulable_snap)
{
	// 1) "name"
	_buf_dest.append_text<char>("<td style='overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_schedulable_snap.name);
	_buf_dest.append_text<char>("\">");

	switch(_schedulable_snap.type)
	{
	case	eSCHEDULABLE_TYPE::SYSTEM:
			_buf_dest.append_text<char>("<i class='fa fa-cogs'></i> ");
			break;

	case	eSCHEDULABLE_TYPE::USER:
			_buf_dest.append_text<char>("&nbsp;&nbsp;&nbsp; ");
			break;

	default:
			_buf_dest.append_text<char>("&nbsp;&nbsp;&nbsp; ");
			break;
	}
	_buf_dest.append_text<char>(_schedulable_snap.name);
	_buf_dest.append_text<char>("</td>");

	// 2) INTERVAL
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(to_string(_schedulable_snap.updatable.duration_interval));
	_buf_dest.append_text<char>("</td>");

	// 2) EXECUTED
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_schedulable_snap.updatable.count_execute);
	_buf_dest.append_text<char>("</td>");
}

void _append_schedulable_entity_update(buffer& _buf_dest, const server_admin::schedulable_snap& _schedulable_snap)
{
	// 1) INTERVAL
	_buf_dest.append_text<char>(to_string(_schedulable_snap.updatable.duration_interval));
	_buf_dest.append_text<char>("\n");

	// 2) EXECUTED
	_buf_dest.append_text<char>(_schedulable_snap.updatable.count_execute);
	_buf_dest.append_text<char>("\n");
}

observable_websocket_server_executor_info::observable_websocket_server_executor_info(const std::string_view _name)
{
	set_name(_name);

	m_webpage_str_target = "webpage_server_executor_info.htm";
	m_webpage_index = 'C';
}

observable_websocket_server_executor_info::~observable_websocket_server_executor_info()
{
}

void observable_websocket_server_executor_info::request_update_webpage_executor(uint64_t /*_flag*/)
{
	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) make messsages
	scoped_lock(this->m_lockable_observable)
	{
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver , make_webmessage_executor_list_update(this->m_pservice_entity->m_instance.status.vector_executor, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send messages
	send_queue.send();
}

void observable_websocket_server_executor_info::request_update_webpage_schedulable(uint64_t /*_flag*/)
{
	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) make messsages
	scoped_lock(this->m_lockable_observable)
	{
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver , make_webmessage_schedulable_list_update(this->m_pservice_entity->m_instance.status.vector_schedulable, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send messages
	send_queue.send();
}

void observable_websocket_server_executor_info::request_update_webpage_executable(uint64_t /*_flag*/)
{
	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) make messsages
	scoped_lock(this->m_lockable_observable)
	{
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver , make_webmessage_executable_list_update(this->m_pservice_entity->m_instance.vector_executable, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send messages
	send_queue.send();
}


result_code observable_websocket_server_executor_info::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void observable_websocket_server_executor_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// check)
	CGASSERT_ERROR(this->m_pservice_entity != nullptr);

	// declare)
	std::array<shared_buffer, 3> array_buf;

	// 2) make messages
	scoped_lock(this->m_pservice_entity->m_cs_instance)
	{
		array_buf[0] = make_webmessage_executor_list(this->m_pservice_entity->m_instance.status.vector_executor, _pobserver->m_flag_set);
		array_buf[1] = make_webmessage_schedulable_list(this->m_pservice_entity->m_instance.status.vector_schedulable, _pobserver->m_flag_set);
		array_buf[2] = make_webmessage_executable_list(this->m_pservice_entity->m_instance.vector_executable, _pobserver->m_flag_set);
	}

	// 2) send messages
	for(auto& iter: array_buf)
		_pobserver->send(iter);
}

uintptr_t observable_websocket_server_executor_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

void observable_websocket_server_executor_info::on_observer_unregistered(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

uintptr_t observable_websocket_server_executor_info::on_observer_data_updating(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return 0;
}

void observable_websocket_server_executor_info::on_observer_data_update(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

result_code observable_websocket_server_executor_info::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message
	auto& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get message head
	auto str_message = msg.buf_message.front_text<char>(size(4));

	// 3) process message by dest
	if (str_message == "CVAD")
	{
		return on_message_server_button_toggle(msg);
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code observable_websocket_server_executor_info::on_message_server_button_toggle(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) get source
	auto pmember = _msg.get_source<socket_websocket>();

	// 2) get id
	_msg.buf_message.extract(size(4));

	// 3) 나머지 모두를 string으로
	std::string_view str_id = _msg.buf_message.to_string<char>();

	// 4) 
	if (str_id == "button_executor_system_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x10;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_executor_list(this->m_pservice_entity->m_instance.status.vector_executor, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (str_id == "button_executor_user_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x20;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_executor_list(this->m_pservice_entity->m_instance.status.vector_executor, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}

	else if (str_id == "button_schedulable_system_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x40;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_schedulable_list(this->m_pservice_entity->m_instance.status.vector_schedulable, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (str_id == "button_schedulable_user_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x80;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_schedulable_list(this->m_pservice_entity->m_instance.status.vector_schedulable, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (str_id == "button_executable_system_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x100;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_executable_list(this->m_pservice_entity->m_instance.vector_executable, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (str_id == "button_executable_user_v")
	{
		// check)
		CGASSERT_ERROR(this->m_pservice_entity != nullptr);

		pmember->m_flag_set ^= 0x200;

		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_executable_list(this->m_pservice_entity->m_instance.vector_executable, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}

	// return) 
	return eRESULT::DONE;
}

shared_buffer observable_websocket_server_executor_info::make_webmessage_executor_list(const std::vector<server_admin::executor_snap>& _vector_executor_snap, uint64_t _flag)
{
	// 2) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_EXECUTOR_LIST);

	// 4) append command
	buf_result.append<char>('L');

	// declare)
	bool is_visiable_system = _flag & 0x010;
	bool is_visiable_user = _flag & 0x020;

	// 5) append flag
	buf_result.append<char>(is_visiable_system ? 'C' : 'U');
	buf_result.append<char>(is_visiable_user ? 'C' : 'U');

	// 6) append message 
	for (const auto& iter : _vector_executor_snap)
	{
		// check)
		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::SYSTEM && is_visiable_system == false);
		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::USER && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("ex{}", iter.updatable.id);

		// - append tr data
		_append_executor_entity_add(buf_text, iter);

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - button status

		// - append length
		buf_result.append_text<char>(buf_text.size());
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}
	buf_result.append('\n');

	// 7) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer observable_websocket_server_executor_info::make_webmessage_schedulable_list(const std::vector<server_admin::schedulable_snap>& _vector_schedulable_info, uint64_t _flag)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 3) append command
	buf_result.append<char>('L');

	// declare)
	bool is_visiable_system = _flag & 0x040;
	bool is_visiable_user = _flag & 0x080;

	// 4) append flag
	buf_result.append<char>(is_visiable_system ? 'C' : 'U');
	buf_result.append<char>(is_visiable_user ? 'C' : 'U');

	// 5) append message 
	for (const auto& iter : _vector_schedulable_info)
	{
		// check)
		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::SYSTEM && is_visiable_system == false);
		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::USER && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("es{}", iter.updatable.id);

		// - append tr data
		_append_schedulable_entity_add(buf_text, iter);

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append length
		buf_result.append_text<char>(buf_text.size());
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}
	buf_result.append('\n');

	// 6) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer observable_websocket_server_executor_info::make_webmessage_executable_list(const std::vector<server_admin::factory_snap>& _vector_executable_info, uint64_t _flag)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_EXECUTABLE_LIST);

	// 3) append command
	buf_result.append<char>('L');

	// declare)
	bool is_visiable_system = _flag & 0x0100;
	bool is_visiable_user = _flag & 0x0200;

	// 4) append flag
	buf_result.append<char>(is_visiable_system ? 'C' : 'U');
	buf_result.append<char>(is_visiable_user ? 'C' : 'U');

	// 5) append message 
	for (const auto& iter : _vector_executable_info)
	{
		// check)
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_SYSTEM || iter.type == eFACTORY_TYPE::POOL_SYSTEM) && is_visiable_system == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_USER || iter.type == eFACTORY_TYPE::POOL_USER) && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("eb{}", iter.updatable.id);

		// - append tr data
		_append_executable_entity_add(buf_text, iter);

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append length
		buf_result.append_text<char>(buf_text.size());
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}
	buf_result.append('\n');

	// 7) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

//shared_buffer observable_websocket_server_executor_info::make_webmessage_executor_list_add(uint64_t _flag)
//{
//	// 1) 
//	object_ptr<service> pservice = service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(65536 * 4);
//	auto buf_temp = alloc_shared_buffer(65536 * 4);
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_EXECUTOR_LIST);
//
//	// declare)
//	bool is_visiable_system = _flag & 0x010;
//	bool is_visiable_user = _flag & 0x020;
//
//	// 4) get time_event amanger
//	auto vector_statistiscs_executor = server_admin::get_now_snap(executor::manager::instance());
//
//	// 5) append message 
//	for (const auto& iter : vector_statistiscs_executor)
//	{
//		// check)
//		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::SYSTEM && is_visiable_system == false);
//		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::USER && is_visiable_user == false);
//
//		// - temporary buffer
//		buffer buf_text = buf_temp;
//
//		// - tr element id
//		auto tr_element_id = NFMT::format("ex{}", iter.updatable.id);
//
//		// - append tr data
//		_append_executor_entity_add(buf_text, iter);
//
//		// - append command
//		buf_result.append<char>('A');
//
//		// - append element id
//		buf_result.append_text<char>(tr_element_id);
//		buf_result.append<char>('\n');
//
//		// - button status
//
//		// - append length
//		buf_result.append_text<char>(buf_text.size());
//		buf_result.append<char>('\n');
//
//		// - append data
//		buf_result.append(buf_text.size(), buf_text.data());
//	}
//
//	// 5) make message
//	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
//}

shared_buffer observable_websocket_server_executor_info::make_webmessage_executor_list_update(const std::vector<server_admin::executor_snap>& _vector_executor_snap, uint64_t _flag)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_EXECUTOR_LIST);

	// declare)
	bool is_visiable_system = _flag & 0x010;
	bool is_visiable_user = _flag & 0x020;

	// 3) append message 
	for (const auto& iter : _vector_executor_snap)
	{
		// check)
		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::SYSTEM && is_visiable_system == false);
		CONTINUE_IF(iter.type == eEXECUTOR_TYPE::USER && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("ex{}", iter.updatable.id);

		// - append tr data
		_append_executor_entity_update(buf_text, iter);

		// - append command
		buf_result.append<char>('U');

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

//shared_buffer observable_websocket_server_executor_info::make_webmessage_schedulable_list_add(uint64_t _flag)
//{
//	// 1) 
//	object_ptr<service> pservice = service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(65536 * 4);
//	auto buf_temp = alloc_shared_buffer(65536 * 4
// 
//	// declare)
//	bool is_visiable_system = _flag & 0x040;
//	bool is_visiable_user = _flag & 0x080;
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);
//
//	// 4) snap executable statistics
//	auto vector_schedulable_snap = server_admin::get_snap(system_executor::get_scheduler());
//		
//	// 5) append message 
//	for (const auto& iter : vector_schedulable_snap)
//	{
//		// check)
//		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::SYSTEM && is_visiable_system == false);
//		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::USER && is_visiable_user == false);
//
//		// - temporary buffer
//		buffer buf_text = buf_temp;
//
//		// - tr element id
//		auto tr_element_id = NFMT::format("es{}", iter.updatable.id);
//
//		// - append tr data
//		_append_schedulable_entity_add(buf_text, iter);
//
//		// - append command
//		buf_result.append<char>('A');
//
//		// - append element id
//		buf_result.append_text<char>(tr_element_id);
//		buf_result.append<char>('\n');
//
//		// - append length
//		buf_result.append_text<char>(buf_text.size());
//		buf_result.append<char>('\n');
//
//		// - append data
//		buf_result.append(buf_text.size(), buf_text.data());
//	}
//
//	// 5) make message
//	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
//}

shared_buffer observable_websocket_server_executor_info::make_webmessage_schedulable_list_update(const std::vector<server_admin::schedulable_snap>& _vector_schedulable, uint64_t _flag)
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// declare)
	bool is_visiable_system = _flag & 0x040;
	bool is_visiable_user = _flag & 0x080;

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 4) append message 
	for (const auto& iter : _vector_schedulable)
	{
		// check)
		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::SYSTEM && is_visiable_system == false);
		CONTINUE_IF(iter.type == eSCHEDULABLE_TYPE::USER && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("es{}", iter.updatable.id);

		// - append tr data
		_append_schedulable_entity_update(buf_text, iter);

		// - append command
		buf_result.append<char>('U');

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

//shared_buffer observable_websocket_server_executor_info::make_webmessage_executable_list_add(uint64_t _flag)
//{
//	// 1) 
//	object_ptr<service> pservice = service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(65536 * 4);
//	auto buf_temp = alloc_shared_buffer(65536 * 4);
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_EXECUTABLE_LIST);
//
//	// declare)
//	bool is_visiable_system = _flag & 0x0100;
//	bool is_visiable_user = _flag & 0x0200;
//
//	// 4) snap executable statistics
//	auto vector_executable_statistics_snap = server_admin::get_now_snap(factory::manager::instance(), eOBJECT_TYPE::EXECUTABLE);
//
//	// 5) append message 
//	for (const auto& iter : vector_executable_statistics_snap)
//	{
//		// check)
//		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_SYSTEM || iter.type == eFACTORY_TYPE::POOL_SYSTEM) && is_visiable_system == false);
//		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_USER || iter.type == eFACTORY_TYPE::POOL_USER) && is_visiable_user == false);
//
//		// - temporary buffer
//		buffer buf_text = buf_temp;
//
//		// - tr element id
//		auto tr_element_id = NFMT::format("eb{}", iter.updatable.id);
//
//		// - append tr data
//		_append_executable_entity_add(buf_text, iter);
//
//		// - append command
//		buf_result.append<char>('A');
//
//		// - append element id
//		buf_result.append_text<char>(tr_element_id);
//		buf_result.append<char>('\n');
//
//		// - append length
//		buf_result.append_text<char>(buf_text.size());
//		buf_result.append<char>('\n');
//
//		// - append data
//		buf_result.append(buf_text.size(), buf_text.data());
//	}
//
//	// 5) make message
//	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
//}

shared_buffer observable_websocket_server_executor_info::make_webmessage_executable_list_update(const std::vector<server_admin::factory_snap>& _vector_schedulable_snap, uint64_t _flag)
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(65536 * 4);
	auto buf_temp = alloc_shared_buffer(65536 * 4);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_EXECUTABLE_LIST);

	// declare)
	bool is_visiable_system = _flag & 0x0100;
	bool is_visiable_user = _flag & 0x0200;

	// 4) append message 
	for (const auto& iter : _vector_schedulable_snap)
	{
		// check)
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_SYSTEM || iter.type == eFACTORY_TYPE::POOL_SYSTEM) && is_visiable_system == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_USER || iter.type == eFACTORY_TYPE::POOL_USER) && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("eb{}", iter.updatable.id);

		// - append tr data
		_append_executable_entity_update(buf_text, iter);

		// - append command
		buf_result.append<char>('U');

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
