#include "pch.h"


void _append_crash_entity(buffer& /*_buf_dest*/, const DUMP_INFO& /*_crash_info*/)
{
	//// 1) id_thread
	//_buf_dest.append_text<char>("<td>");
	//_buf_dest.append_text<char>(reinterpret_cast<uint64_t>(_crash_info.id_thread));
	//_buf_dest.append_text<char>("</td>");

	//// 2) count_executed
	//_buf_dest.append_text<char>("<td>");
	//_buf_dest.append_text<char>(_crash_info.count_executed);
	//_buf_dest.append_text<char>("</td>");

	//// 3) count_error
	//_buf_dest.append_text<char>("<td>");
	//_buf_dest.append_text<char>(_crash_info.count_error);
	//_buf_dest.append_text<char>("</td>");

	//// 4) tick_last_execute
	////_buf_dest.append_text<char>("<td>");
	////_buf_dest.append_text<char>(_crash_info.count_error);
	////_buf_dest.append_text<char>("</td>");

	//// 5) is_executing
	//_buf_dest.append_text<char>("<td>");
	//_buf_dest.append_text<char>((_crash_info.is_executing) ? "executing" : "");
	//_buf_dest.append_text<char>("</td>");
}

shared_buffer _make_webmessage_crash_info(const DUMP_INFO& _crash_info)
{
	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_THREAD_LIST);

	// 4) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_crash_entity(buf_text, _crash_info);

		// - append command (A:add)
		buf_result.append<char>('A');

		// - tr element id
		auto tr_element_id = NFMT::format("cr{}", _crash_info.id);

		// - append element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append length
		buf_result.append_text<char>(buf_text.size());
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_crash_update(const DUMP_INFO& _crash_info, uint64_t /*_flag*/)
{
	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_THREAD_LIST);

	// 4) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_crash_entity(buf_text, _crash_info);

		// - append command (U:update)
		buf_result.append<char>('U');

		// - th element id
		auto tr_element_id = NFMT::format("cr{}", _crash_info.id);

		// - append element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append length
		buf_result.append_text<char>(buf_text.size());
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_crash_remove(const DUMP_INFO& _crash_info)
{
	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_THREAD_LIST);

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('D');

		// - th node name
		auto tr_element_id = NFMT::format("cr{}", _crash_info.id);

		// - append tr_element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

observable_websocket_peer_crash_info::observable_websocket_peer_crash_info(const std::string_view _name)
{
	set_name(_name);

	m_webpage_str_target = "webpage_server_crash_info.htm";
	m_webpage_index = 'F';
}

observable_websocket_peer_crash_info::~observable_websocket_peer_crash_info()
{
}

result_code observable_websocket_peer_crash_info::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// 1) register schedulable
	if (empty())
	{
		execute_interval(2s);
		system_executor::register_schedulable(this);
	}

	// return) 
	return eRESULT::SUCCESS;
}

void observable_websocket_peer_crash_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// 2) send factory list
	_pobserver->send(make_webmessage_crash_info_list_add());
}

uintptr_t observable_websocket_peer_crash_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// 3) unregister schedulable
	if (observer_count() == 1)
	{
		system_executor::unregister_schedulable(this);
	}

	// return) 
	return 0;
}

void observable_websocket_peer_crash_info::on_observer_unregistered(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

uintptr_t observable_websocket_peer_crash_info::on_observer_data_updating(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return 0;
}

void observable_websocket_peer_crash_info::on_observer_data_update(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

void observable_websocket_peer_crash_info::on_execute()
{
	// 1) send factory list
	//send(make_webmessaged_list_update());
}

shared_buffer observable_websocket_peer_crash_info::make_webmessage_crash_info_list_add()
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_THREAD_LIST);

	// 4) get time_event amanger
	auto vector_status_snap = dump_manager::instance()->status_snap();

	// 5) append message 
	for (const auto& iter : vector_status_snap)
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("cr{}", iter.id);

		// - append tr data
		_append_crash_entity(buf_text, iter);

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

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer observable_websocket_peer_crash_info::make_webmessage_crash_info_list_update()
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_THREAD_LIST);

	// 4) get time_event amanger
	auto vector_crash_info = dump_manager::instance()->status_snap();

	// 5) append message 
	for (const auto& iter : vector_crash_info)
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = NFMT::format("cr{}", iter.id);

		// - append tr data
		_append_crash_entity(buf_text, iter);

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

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}
