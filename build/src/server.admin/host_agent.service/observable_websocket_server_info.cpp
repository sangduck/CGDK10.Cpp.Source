#include "pch.h"

void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::string_view _str_value);
void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::size_t _value);

void _append_server_info_service_instance(buffer& _buf_dest, server_admin::instance_Iservice* _pinstance_service)
{
	// check)
	CGASSERT_ERROR(_pinstance_service != nullptr);

	// check)
	THROW_IF(_pinstance_service == nullptr, throwable::failure(eRESULT::EXCEPTION));

	// 1) get info
	const auto& setting = _pinstance_service->m_instance.setting;
	const auto& status = _pinstance_service->m_instance.status;

	// 2) 
	_buf_dest.append_text<char>("<td>");

	// 3) "name"
	_buf_dest.append_text<char>("<span style='font-size:15px;font-weight:bold'><i class='fa fa-server'></i> ");
	_buf_dest.append_text<char>(setting.nickname);
	_buf_dest.append_text<char>("</span><br>");

	// 4) TYPE/UIDs
	_buf_dest.append_text<char>(setting.rank.role);
	_buf_dest.append_text<char>("<span style='font-size:11px'>");
	_buf_dest.append_text<char>(setting.iid);
	_buf_dest.append_text<char>("</span>");

	// 5) STATUS
	//     <td>START</td>
	if (status.state != eOBJECT_STATE::NONE)
	{
		_buf_dest.append_text<char>(to_string<char>(status.state));
	}
	_buf_dest.append_text<char>("<br>");

	// 6) 
	_buf_dest.append_text<char>("<canvas class='table_server_list_canvas' Id='si_cv1{}'>", reinterpret_cast<uint64_t>(static_cast<server_admin::Iinstance*>(_pinstance_service)));


	_buf_dest.append_text<char>("</canvas>");
	if(status.vector_process_status.empty() == false)
		_buf_dest.append_text<char>(to_string_scaled_byte<char>(status.vector_process_status.back().bytes_memory_usage));
	else
		_buf_dest.append_text<char>("0"sv);

	// 7) 
	_buf_dest.append_text<char>("</td>");
}

//-----------------------------------------------------------------
// 설명) service instance ADD
// 
// 1) _make_webmessage_server_info_service_instance_add
// 
//    'SELI' + 'A' + [id] + '\n' + [ctx_length] + [ctx] + '\n' => Append 
// 
//-----------------------------------------------------------------
shared_buffer _make_webmessage_server_info_service_instance_add(server_admin::Iinstance* _pinstance)
{

	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_INFO_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_server_info_service_instance(buf_text, dynamic_cast<server_admin::instance_Iservice*>(_pinstance));

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('A');

		// - tr element id
		auto tr_element_id = NFMT::format("tr{}", reinterpret_cast<uint64_t>(_pinstance));

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

//-----------------------------------------------------------------
// 설명) service instance UPDATE
// 
//   _make_webmessage_server_info_service_instance_update
// 
//    'SELI' + 'U' + [id] + '\n' + [ctx_length] + [ctx] + '\n' => Update
// 
//-----------------------------------------------------------------
shared_buffer _make_webmessage_server_info_service_instance_update(server_admin::Iinstance* _pinstance, uint64_t /*_flag*/)
{
	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_INFO_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_server_info_service_instance(buf_text, dynamic_cast<server_admin::instance_Iservice*>(_pinstance));

		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = NFMT::format("tr{}", reinterpret_cast<uint64_t>(_pinstance));

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

//-----------------------------------------------------------------
// 설명) service instance REMOVE
// 
//   _make_webmessage_server_info_service_instance_remove
// 
//    'SELI' + 'D' + [id] + '\n' => Delete
// 
//-----------------------------------------------------------------
shared_buffer _make_webmessage_server_info_service_instance_remove(server_admin::Iinstance* _pinstance)
{
	// 1) get instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_INFO_SERVER_LIST); // "SELI"

	// 4) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('D');

		// - tr node name
		auto tr_element_id = NFMT::format("tr{}", reinterpret_cast<uint64_t>(_pinstance));

		// - append tr_element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');
	}

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer make_webmessage_server_info_server_list()
{
	// 1) get instance 
	auto pmanager_instance = service::pmanager_instance.get_object_ptr();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SERVER_INFO_SERVER_LIST); // "SELI"

	// 4) append message 
	scoped_lock(pmanager_instance->m_cs_instance)
	{
		for (const auto& iter : pmanager_instance->m_map_instance)
		{
			// - temporary buffer
			buffer buf_text = buf_temp;

			// check)
			CONTINUE_IF(iter.second->get_instance_type() != server_admin::eINSTANCE_TYPE::SERVICE);

			// - append tr data
			_append_server_info_service_instance(buf_text, dynamic_pointer_cast<server_admin::instance_Iservice>(iter.second));

			// - append command
			buf_result.append<char>('A');

			// - tr node name
			auto tr_element_id = NFMT::format("tr{}", reinterpret_cast<uint64_t>(iter.second.get()));

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

	// 5) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer make_web_message_event_list();
shared_buffer _make_web_message_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record);






























observable_websocket_peer_info::observable_websocket_peer_info(const std::string_view _name)
{
	// 1) set name
	this->set_name(_name);

	this->m_webpage_str_target = "webpage_server_info.htm";
}

observable_websocket_peer_info::~observable_websocket_peer_info()
{
}

void observable_websocket_peer_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// 2) send server list
	_pobserver->send(make_webmessage_server_info_server_list());
}

uintptr_t observable_websocket_peer_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);
	_pobserver->unsubscribe(OCID_FRAME2);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

result_code observable_websocket_peer_info::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message 
	sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 3) get message head
	buffer_view buf_temp = msg.buf_message;

	auto message_type = buf_temp.extract_text<char>(size(4));
	auto pmember = msg.get_source<socket_websocket>();

	if (message_type[0] == 'B')
	{
		if (message_type == MESSAGE_WEB_SEVER_INFO_SELECT)
		{
			// - skip 2byte;
			buf_temp.extract(size(2));

			// - get id
			void* pobject = reinterpret_cast<void*>(__from_string<uint64_t>(buf_temp.to_string<char>()));

			// - find service_entity
			auto pservice_entity = service::pmanager_instance->find_instance_by_pvoid(pobject);

			// check)
			RETURN_IF(pservice_entity.empty(), eRESULT::DONE);

			// - register to service_entity
			pservice_entity->subscribe(pmember, OCID_FRAME2);
		}
	}

	// return) 
	return eRESULT::BYPASS;
}

void observable_websocket_peer_info::notify_to_web_controls_INSTANCE_ENTER(server_admin::Iinstance* _pinstance)
{
	// declare) 
	shared_buffer buf_send;

	// 1) make buffer
	scoped_lock(_pinstance->m_cs_instance)
	{
		buf_send = _make_webmessage_server_info_service_instance_add(_pinstance);
	}

	// 2) send
	this->send(buf_send);
}
void observable_websocket_peer_info::notify_to_web_controls_INSTANCE_LEAVE(server_admin::Iinstance* _pinstance)
{
	// declare) 
	shared_buffer buf_send;

	// 1) make buffer
	scoped_lock(_pinstance->m_cs_instance)
	{
		buf_send = _make_webmessage_server_info_service_instance_remove(_pinstance);
	}

	// 2) send
	this->send(buf_send);
}

void observable_websocket_peer_info::notify_to_web_controls_INSTANCE_STATUS_UPDATE(server_admin::Iinstance* _pinstance)
{
	// declare) 
	shared_buffer buf_send;

	// 1) make buffer
	scoped_lock(_pinstance->m_cs_instance)
	{
		buf_send = _make_webmessage_server_info_service_instance_update(_pinstance, 0xffffffff);
	}

	// 2) send
	this->send(buf_send);
}

void observable_websocket_peer_info::notify_to_web_controls_INSTANCE_STATUS_UPDATE()
{
	//this->send(_make_webmessage_server_info_service_instance_update(_pinstance, 0xffffffff));
}
