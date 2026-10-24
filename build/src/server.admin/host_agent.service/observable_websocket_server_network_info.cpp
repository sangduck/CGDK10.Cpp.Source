#include "pch.h"


void _append_network_connective_entity(buffer& _buf_dest, const server_system::connective_info_snap& _connective_info_snap)
{
	// 2) "Name"
	_buf_dest.append_text<char>("<td><span style='font-size:14px;font-weight:bold;overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_connective_info_snap.name);
	_buf_dest.append_text<char>("\">");
	_buf_dest.append_text<char>("<i class='fa fa-cogs'>&nbsp;</i>");
	_buf_dest.append_text<char>(_connective_info_snap.name);
	_buf_dest.append_text<char>("</span></td>");

	// 3) STATUS
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>(to_string(_connective_info_snap.status.state));
	_buf_dest.append_text<char>("</td>");

	// 4) BIND ADDRESS
	_buf_dest.append_text<char>("<td>");
	if (_connective_info_snap.status.state > eSOCKET_STATE::CLOSED)
		_buf_dest.append_text<char>(to_string(_connective_info_snap.status.addr_local));
	else
		_buf_dest.append_text<char>(" - ");
	_buf_dest.append_text<char>("</td>");

	// declare)
	const auto& statistics_now = _connective_info_snap.now;

	// 5) KEEP
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_keep);
	_buf_dest.append_text<char>("</td>");

	// 6) TRY (count)
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_try);
	_buf_dest.append_text<char>("</td>");

	// 7) TRY (/sec)
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_try);
	_buf_dest.append_text<char>("</td>");

	// 8) SUCCESS (count)
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_success_connect);
	_buf_dest.append_text<char>("</td>");

	// 9) SUCCESS (/sec)
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_success_connect);
	_buf_dest.append_text<char>("</td>");

	// 10) FAIL
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_fail_connect);
	_buf_dest.append_text<char>("</td>");

	// 11) DISCONNECT
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(statistics_now.count_disconnect);
	_buf_dest.append_text<char>("</td>");

	// 12) </tr>
	_buf_dest.append_text<char>("</td>");
}

//shared_buffer _make_webmessage_network_entity_add(time_event::Ientity* _network_info)
//{
//	// 1) get service instance 
//	object_ptr<service> pservice = ::service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);
//
//	// 4) get time_event amanger
//	auto ptime_event_entity_manager = time_event::entity_manager::instance();
//
//	// 5) append message 
//	{
//		// - temporary buffer
//		buffer buf_text = buf_temp;
//
//		// - append tr data
//		_append_network_entity(buf_text, _network_info);
//
//		// - append command
//		buf_result.append<char>('A');
//
//		// - tr element id
//		auto tr_element_id = CGDK::stream_string("cv", reinterpret_cast<uint64_t>(_network_info));
//
//		// - append element_id
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

//shared_buffer _make_webmessage_network_entity_update(time_event::Ientity* _network_info, uint64_t /*_flag*/)
//{
//	// 1) 
//	object_ptr<service> pservice = ::service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);
//
//	// 4) get time_event amanger
//	auto ptime_event_entity_manager = time_event::entity_manager::instance();
//
//	// 5) append message 
//	{
//		// - temporary buffer
//		buffer buf_text = buf_temp;
//
//		// - append tr data
//		_append_network_connective_entity(buf_text, _network_info);
//
//		// - append command
//		buf_result.append<char>('U');
//
//		// - tr element id
//		auto tr_element_id = CGDK::stream_string("cv", reinterpret_cast<uint64_t>(_network_info));
//
//		// - <tr>
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
//
//shared_buffer _make_webmessage_network_entity_remove(time_event::Ientity* _network_info)
//{
//	// 1) 
//	object_ptr<service> pservice = ::service::instance();
//
//	// 2) buffer
//	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
//
//	// 3) append message head
//	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);
//
//	// 4) get time_event amanger
//	auto ptime_event_entity_manager = time_event::entity_manager::instance();
//
//	// 5) append message 
//	{
//		// - append command (A:Add, D:delete, U:update)
//		buf_result.append<char>('D');
//
//		// - tr element id
//		auto tr_element_id = CGDK::stream_string("cv", reinterpret_cast<uint64_t>(_network_info));
//
//		// - append tr_element_id
//		buf_result.append_text<char>(tr_element_id);
//		buf_result.append<char>('\n');
//	}
//
//	// 5) make message
//	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
//}

void _append_network_binder_entity(buffer& _buf_dest, const server_system::socket_info_snap& _binder_info_snap)
{
	// 2) "Name"
	_buf_dest.append_text<char>("<td><span style='font-size:14px;font-weight:bold;overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_binder_info_snap.name);
	_buf_dest.append_text<char>("\">");
	_buf_dest.append_text<char>("<i class='fa fa-cogs'>&nbsp;</i>");
	_buf_dest.append_text<char>(_binder_info_snap.name);
	_buf_dest.append_text<char>("</span></td>");

	// 3) TYPE (TCP/UDP)
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>((_binder_info_snap.updatable.flag_socket & 0x02) ? "TCP" : "UDP");
	_buf_dest.append_text<char>((_binder_info_snap.updatable.flag_socket & 0x01) ? "(ipv6)" : "(ipv4)");
	_buf_dest.append_text<char>("</td>");

	// 4) STATUS
	_buf_dest.append_text<char>("<td>");
	_buf_dest.append_text<char>(to_string(_binder_info_snap.updatable.state));
	_buf_dest.append_text<char>("</td>");

	// 5) ADDRESS
	_buf_dest.append_text<char>("<td>");
	if(_binder_info_snap.updatable.state >= eSOCKET_STATE::BINDED)
		_buf_dest.append_text<char>(to_string(_binder_info_snap.updatable.addr_remote));
	else
		_buf_dest.append_text<char>(" - ");
	_buf_dest.append_text<char>("</td>");
	
	// 6) ESTABLIISH
	_buf_dest.append_text<char>("<td>");
	if(_binder_info_snap.updatable.state >= eSOCKET_STATE::BINDED)
		_buf_dest.append_text<char>(to_string(_binder_info_snap.updatable.time_bind, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"));
	else
		_buf_dest.append_text<char>(" -");
	_buf_dest.append_text<char>("</td>");

	// 7) SEND - messages
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(to_string_scaled(_binder_info_snap.updatable.traffic.count_sent_message));
	_buf_dest.append_text<char>("</td>");

	// 8) SEND - bytes
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(to_string_scaled_byte(_binder_info_snap.updatable.traffic.count_sent_bytes));
	_buf_dest.append_text<char>("</td>");

	// 9) RECEIVE - messages
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(to_string_scaled(_binder_info_snap.updatable.traffic.count_received_message));
	_buf_dest.append_text<char>("</td>");

	// 10) RECEIVE - bytes
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(to_string_scaled_byte(_binder_info_snap.updatable.traffic.count_received_bytes));
	_buf_dest.append_text<char>("</td>");

	// 11) ERROR
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_binder_info_snap.updatable.traffic.count_error_on_send + _binder_info_snap.updatable.traffic.count_error_on_receive);
	_buf_dest.append_text<char>("</td>");

	// 12) </td>
	_buf_dest.append_text<char>("</td>");
}

shared_buffer _make_webmessage_connective_list_add(const std::vector<server_system::connective_info_snap>& _vector_connective_info, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_CONNECTIVE_LIST);

	// 4) append message 
	for (const auto& iter : _vector_connective_info)
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = CGDK::stream_string("cv", iter.status.id);

		// - append tr data
		_append_network_connective_entity(buf_text, iter);

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

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_connective_list_update(const std::vector<server_system::connective_info_snap>& _vector_connective_info, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_CONNECTIVE_LIST);

	// 3) append message 
	for (const auto& iter : _vector_connective_info)
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = CGDK::stream_string("cv", iter.status.id);

		// - append command
		buf_result.append<char>('U');

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - "Name"
		buf_result.append_text<char>(iter.name);
		buf_result.append<char>('\n');

		// - STATUS
		buf_result.append_text<char>(to_string(iter.status.state));
		buf_result.append<char>('\n');

		// - BIND ADDRESS
		if (iter.status.state > eSOCKET_STATE::CLOSED)
			buf_result.append_text<char>(to_string(iter.status.addr_local));
		else
			buf_result.append_text<char>(" - ");
		buf_result.append<char>('\n');

		// declare)
		const auto& statistics_now = iter.now;

		// - KEEP
		buf_result.append_text<char>(statistics_now.count_keep);
		buf_result.append<char>('\n');

		// - TRY (count)
		buf_result.append_text<char>(statistics_now.count_try);
		buf_result.append<char>('\n');

		// - TRY (/sec)
		buf_result.append_text<char>(statistics_now.count_try);
		buf_result.append<char>('\n');

		// - SUCCESS (count)
		buf_result.append_text<char>(statistics_now.count_success_connect);
		buf_result.append<char>('\n');

		// - SUCCESS (/sec)
		buf_result.append_text<char>(statistics_now.count_success_connect);
		buf_result.append<char>('\n');

		// - FAIL
		buf_result.append_text<char>(statistics_now.count_fail_connect);
		buf_result.append<char>('\n');

		// - DISCONNECT
		buf_result.append_text<char>(statistics_now.count_disconnect);
		buf_result.append<char>('\n');

		// - append data
		buf_result.append(buf_text.size(), buf_text.data());
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_binder_list_add(const std::vector<server_system::socket_info_snap>& _vector_binder_info, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_BINDER_LIST);

	// 3) append message 
	for (const auto& iter : _vector_binder_info)
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - tr element id
		auto tr_element_id = CGDK::stream_string("bi", iter.updatable.id);

		// - append tr data
		_append_network_binder_entity(buf_text, iter);

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

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_binder_list_update(const std::vector<server_system::socket_info_snap>& _vector_binder_info, uint64_t /*_flag*/)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_BINDER_LIST);

	// 3) append message 
	for (const auto& iter : _vector_binder_info)
	{
		// - tr element id
		auto tr_element_id = CGDK::stream_string("bi", iter.updatable.id);

		// - append command
		buf_result.append<char>('U');

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// 2) "Name"
		buf_result.append_text<char>(iter.name);
		buf_result.append<char>('\n');

		// 3) TYPE (TCP/UDP)
		buf_result.append_text<char>((iter.updatable.flag_socket & 0x02) ? "TCP" : "UDP");
		buf_result.append_text<char>((iter.updatable.flag_socket & 0x01) ? "(ipv6)" : "(ipv4)");
		buf_result.append<char>('\n');

		// 4) STATUS
		buf_result.append_text<char>(to_string(iter.updatable.state));
		buf_result.append<char>('\n');

		// 5) ADDRESS
		if (iter.updatable.state >= eSOCKET_STATE::BINDED)
			buf_result.append_text<char>(to_string(iter.updatable.addr_remote));
		else
			buf_result.append_text<char>(" - ");
		buf_result.append<char>('\n');

		// 6) ESTABLIISH
		if (iter.updatable.state >= eSOCKET_STATE::BINDED)
			buf_result.append_text<char>(to_string(iter.updatable.time_bind, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"));
		else
			buf_result.append_text<char>(" -");
		buf_result.append<char>('\n');

		// 7) SEND - messages
		buf_result.append_text<char>(to_string_scaled(iter.updatable.traffic.count_sent_message));
		buf_result.append<char>('\n');

		// 8) SEND - bytes
		buf_result.append_text<char>(to_string_scaled_byte(iter.updatable.traffic.count_sent_bytes));
		buf_result.append<char>('\n');

		// 9) RECEIVE - messages
		buf_result.append_text<char>(to_string_scaled(iter.updatable.traffic.count_received_message));
		buf_result.append<char>('\n');

		// 10) RECEIVE - bytes
		buf_result.append_text<char>(to_string_scaled_byte(iter.updatable.traffic.count_received_bytes));
		buf_result.append<char>('\n');

		// 11) ERROR
		buf_result.append_text<char>(iter.updatable.traffic.count_error_on_send + iter.updatable.traffic.count_error_on_receive);
		buf_result.append<char>('\n');
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}








void observable_websocket_peer_network_info::request_update_webpage_connective(uint64_t /*_flag*/)
{
	// check)
	CGASSERT_FATAL(this->m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) 모두에게 전송한다.
	SCOPED_LOCK(this->m_lockable_observable)
	{
		SCOPED_LOCK(this->m_pservice_entity->m_lockable_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver, _make_webmessage_connective_list_update(this->m_pservice_entity->m_instance.info.status.vector_connective, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send
	send_queue.send();
}

void observable_websocket_peer_network_info::request_update_webpage_binder(uint64_t /*_flag*/)
{
	// check)
	CGASSERT_FATAL(m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) 모두에게 전송한다.
	SCOPED_LOCK(this->m_lockable_observable)
	{
		SCOPED_LOCK(this->m_pservice_entity->m_lockable_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver, _make_webmessage_binder_list_update(this->m_pservice_entity->m_instance.info.status.vector_binder, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send
	send_queue.send();
}

observable_websocket_peer_network_info::observable_websocket_peer_network_info()	
{
	this->m_webpage_str_target = "webpage_server_network_info.htm";
	this->m_webpage_index = 'D';
}

observable_websocket_peer_network_info::~observable_websocket_peer_network_info()
{
}

result_code observable_websocket_peer_network_info::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void observable_websocket_peer_network_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// declare) 
	shared_buffer buf_send_1;
	shared_buffer buf_send_2;

	// 2) make buffer
	SCOPED_LOCK(this->m_pservice_entity->m_lockable_instance)
	{
		buf_send_1 = _make_webmessage_connective_list_add(this->m_pservice_entity->m_instance.info.status.vector_connective, _pobserver->m_flag_set);
		buf_send_2 = _make_webmessage_binder_list_add(this->m_pservice_entity->m_instance.info.status.vector_binder, _pobserver->m_flag_set);
	}

	// 3) send factory list
	_pobserver->send(buf_send_1);
	_pobserver->send(buf_send_2);
}

uintptr_t observable_websocket_peer_network_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) unregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}
