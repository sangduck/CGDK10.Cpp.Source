#include "pch.h"


void _append_factory_info_add(buffer& _buf_dest, const server_admin::factory_snap& _factory_info)
{
	// 1) NAME
	_buf_dest.append_text<char>("<td style='overflow-x:hidden;' title=\"");
	_buf_dest.append_text<char>(_factory_info.name);
	_buf_dest.append_text<char>("\"><span style='font-size:14px'>");
	switch(_factory_info.type)
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
			_buf_dest.append_text<char>("&nbsp;&nbsp; ");
			break;
	}
	_buf_dest.append_text<char>(_factory_info.name);
	_buf_dest.append_text<char>("</span></td>");

	// 3) IN_USING
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_factory_info.updatable.now.in_using);
	_buf_dest.append_text<char>("</td>");

	// 4) EXISTING
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_factory_info.updatable.now.existing);
	_buf_dest.append_text<char>("</td>");

	// 5) REQUESTED
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_factory_info.updatable.now.alloc_create + _factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls);
	_buf_dest.append_text<char>("</td>");

	// 6) CREATED
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_factory_info.updatable.now.alloc_create);
	_buf_dest.append_text<char>("</td>");

	// 7) DELETED
	_buf_dest.append_text<char>("<td align='right'>");
	_buf_dest.append_text<char>(_factory_info.updatable.now.free_delete);
	_buf_dest.append_text<char>("</td>");

	// 8) STATUS
	_buf_dest.append_text<char>("<td align='center'>");
	_buf_dest.append_text<char>("sustain");
	_buf_dest.append_text<char>("</td>");

	// 9) STACKED
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER)
	{
		_buf_dest.append_text<char>("<td align='right'>");
		_buf_dest.append_text<char>(_factory_info.updatable.stacked);
		_buf_dest.append_text<char>("</td>");
	}
	else
	{
		_buf_dest.append_text<char>("<td></td>");
	}

	// 10) MAX
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER)
	{
		if (_factory_info.updatable.now.existing_limits != INT64_MAX)
		{
			_buf_dest.append_text<char>("<td align='right'>");
			_buf_dest.append_text<char>(_factory_info.updatable.now.existing_limits);
			_buf_dest.append_text<char>("</td>");
		}
		else
		{
			_buf_dest.append_text<char>("<td align='left'>Unlimited</td>");
		}
	}
	else
	{
		_buf_dest.append_text<char>("<td></td>");
	}

	// 11) REUSE rate & DESTROY/CREQTE ratio
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER)
	{
		// - REUSE ratio
		int64_t	fallocated_total = _factory_info.updatable.now.alloc_create + _factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls;

		if (fallocated_total)
		{
			_buf_dest.append_text<char>("<td align='right'>{:6.3f}%</td>", ((float)_factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls) / (float)fallocated_total * 100.0f);
		}
		else
		{
			_buf_dest.append_text<char>("<td align='right'> - </td>");
		}

		// - D/C ratio
		int64_t	fallocated_create = _factory_info.updatable.now.alloc_create;

		if (fallocated_create != 0)
		{
			_buf_dest.append_text<char>("<td align='right'>{:6.3f}%</td>", (float)_factory_info.updatable.now.free_delete / fallocated_create * 100.0f);
		}
		else
		{
			_buf_dest.append_text<char>("<td align='right'> - </td>");
		}
	}
	else
	{
		// - REUSE ratio
		_buf_dest.append_text<char>("<td></td>");

		// - D/C ratio
		_buf_dest.append_text<char>("<td></td>");
	}
}

void _append_factory_info_update(buffer& _buf_dest, const server_admin::factory_snap& _factory_info)
{
	// 0) NAME
	// 1) ID
	// 2) SIZE

	// 3) IN_USING
	_buf_dest.append_text<char>(_factory_info.updatable.now.in_using);
	_buf_dest.append_text<char>("\n");

	// 4) EXISTING
	_buf_dest.append_text<char>(_factory_info.updatable.now.existing);
	_buf_dest.append_text<char>("\n");

	// 5) REQUESTED
	_buf_dest.append_text<char>(_factory_info.updatable.now.alloc_create + _factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls);
	_buf_dest.append_text<char>("\n");

	// 6) CREATED
	_buf_dest.append_text<char>(_factory_info.updatable.now.alloc_create);
	_buf_dest.append_text<char>("\n");

	// 7) DELETED
	_buf_dest.append_text<char>(_factory_info.updatable.now.free_delete);
	_buf_dest.append_text<char>("\n");

	// 8) STATUS
	_buf_dest.append_text<char>("sustain\n");

	// 9) STACKED
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER)
	{
		_buf_dest.append_text<char>(_factory_info.updatable.stacked);
	}
	_buf_dest.append_text<char>("\n");

	// 10) MAX
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER
	 && _factory_info.updatable.now.existing_limits != INT64_MAX)
	{
		_buf_dest.append_text<char>(_factory_info.updatable.now.existing_limits);
	}
	_buf_dest.append_text<char>("\n");

	// 11,12) REUSE rate & DESTROY/CREQTE ratio
	if (_factory_info.type <= eFACTORY_TYPE::POOL_USER)
	{
		// - REUSE ratio
		int64_t	fallocated_total = _factory_info.updatable.now.alloc_create + _factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls;

		if (fallocated_total)
		{
			_buf_dest.append_text<char>("{:6.3f}%\n", ((float)_factory_info.updatable.now.alloc_stack + _factory_info.updatable.now.alloc_stack_tls) / (float)fallocated_total * 100.0f);
		}
		else
		{
			_buf_dest.append_text<char>(" - \n");
		}

		// - D/C ratio
		int64_t	fallocated_create = _factory_info.updatable.now.alloc_create;

		if (fallocated_create != 0)
		{
			_buf_dest.append_text<char>("{:6.3f}%\n", (float)_factory_info.updatable.now.free_delete / fallocated_create * 100.0f);
		}
		else
		{
			_buf_dest.append_text<char>(" - \n");
		}
	}
	else
	{
		// - REUSE ratio
		_buf_dest.append_text<char>("\n");

		// - D/C ratio
		_buf_dest.append_text<char>("\n");
	}
}

shared_buffer make_webmessage_factory_list_add(const std::vector<server_admin::factory_snap>& _vector_factory_info, uint64_t _flag)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(65536);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_FACTORY_LIST_ADD); // "CAGA"

	// declare)
	bool is_visiable_memoryblock = _flag & 0x01;
	bool is_visiable_system = _flag & 0x02;
	bool is_visiable_user = _flag & 0x04;

	// 3) append flag
	buf_result.append<char>(is_visiable_memoryblock ? 'C' : 'U');
	buf_result.append<char>(is_visiable_system ? 'C' : 'U');
	buf_result.append<char>(is_visiable_user ? 'C' : 'U');

	// 4) append factory list info 
	for (const auto& iter : _vector_factory_info)
	{
		// check)
		CONTINUE_IF(iter.type == eFACTORY_TYPE::POOL_MEMORYBLOCK && is_visiable_memoryblock == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_SYSTEM || iter.type == eFACTORY_TYPE::POOL_SYSTEM) && is_visiable_system == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_USER || iter.type == eFACTORY_TYPE::POOL_USER) && is_visiable_user == false);

		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_factory_info_add(buf_text, iter);

		// - tr node name
		auto tr_element_id = NFMT::format("tr{}", iter.updatable.id);

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

shared_buffer make_webmessage_factory_list_update(const std::vector<server_admin::factory_snap>& _vector_factory_info, uint64_t _flag)
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(65536);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_FACTORY_LIST_UPDATE);// "CAGU"

	// declare)
	bool is_visiable_memoryblock = _flag & 0x01;
	bool is_visiable_system = _flag & 0x02;
	bool is_visiable_user = _flag & 0x04;

	// 3) append message 
	for (const auto& iter : _vector_factory_info)
	{
		// check) 
		CONTINUE_IF(iter.type == eFACTORY_TYPE::POOL_MEMORYBLOCK && is_visiable_memoryblock == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_SYSTEM || iter.type == eFACTORY_TYPE::POOL_SYSTEM) && is_visiable_system == false);
		CONTINUE_IF((iter.type == eFACTORY_TYPE::ALLOCATOR_USER || iter.type == eFACTORY_TYPE::POOL_USER) && is_visiable_user == false);

		// - tr node name
		auto tr_element_id = NFMT::format("tr{}", iter.updatable.id);

		// - append element id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');

		// - append tr data
		_append_factory_info_update(buf_result, iter);
	}

	// 4) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

observable_websocket_peer_factory_info::observable_websocket_peer_factory_info(const std::string_view _name)
{
	set_name(_name);

	m_webpage_str_target = "webpage_server_factory_info.htm";
	m_webpage_index = 'B';
}

observable_websocket_peer_factory_info::~observable_websocket_peer_factory_info()
{
}

void observable_websocket_peer_factory_info::request_update_webpage_factory()
{
	// check)
	CGASSERT_ERROR(m_pservice_entity != nullptr);

	// declare)
	net::local_send_queue send_queue;

	// 1) 모두에게 전송한다.
	scoped_lock(this->m_lockable_observable)
	{
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			for (auto& iter : this->m_container_observer)
			{
				send_queue.enqueue({ iter.pobserver, make_webmessage_factory_list_update(this->m_pservice_entity->m_instance.status.vector_factory, iter.pobserver->m_flag_set) });
			}
		}
	}

	// 2) send
	send_queue.send();
}

result_code observable_websocket_peer_factory_info::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void observable_websocket_peer_factory_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// check)
	CGASSERT_ERROR(m_pservice_entity != nullptr);

	// 1) register mediator
	_pobserver->register_messageable(this);

	// declare) 
	shared_buffer buf_send;

	scoped_lock(this->m_pservice_entity->m_cs_instance)
	{
		buf_send = make_webmessage_factory_list_add(this->m_pservice_entity->m_instance.status.vector_factory, _pobserver->m_flag_set);
	}

	// 2) send factory list
	_pobserver->send(buf_send);
}

uintptr_t observable_websocket_peer_factory_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

void observable_websocket_peer_factory_info::on_observer_unregistered(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

uintptr_t observable_websocket_peer_factory_info::on_observer_data_updating(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return 0;
}

void observable_websocket_peer_factory_info::on_observer_data_update(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

result_code observable_websocket_peer_factory_info::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message 
	sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get message head
	auto message_type = msg.buf_message.front_text<char>(size(4));
	auto pmember = msg.get_source<socket_websocket>();

	// 3) ...
	RETURN_IF(message_type[0] != 'C', eRESULT::BYPASS);

	if (message_type == MESSAGE_WEB_FACTORY_FILTER_MEMORY) // "CFGA"
	{
		// check)
		CGASSERT_ERROR(m_pservice_entity != nullptr);

		// - get commnad
		auto command = msg.buf_message[4];

		if (command == 'C')
		{
			pmember->m_flag_set |= (uint64_t)0x01;

		}
		else if (command == 'U')
		{
			pmember->m_flag_set &= (~(uint64_t)0x01);
		}

		// - send message
		shared_buffer buf_send;
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_factory_list_add(this->m_pservice_entity->m_instance.status.vector_factory, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (message_type == MESSAGE_WEB_FACTORY_FILTER_SYSTEM) // "CFGB"
	{
		// check)
		CGASSERT_ERROR(m_pservice_entity != nullptr);

		// - get commnad
		auto command = msg.buf_message[4];

		if (command == 'C')
		{
			pmember->m_flag_set |= (uint64_t)0x02;

		}
		else if (command == 'U')
		{
			pmember->m_flag_set &= (~(uint64_t)0x02);
		}

		// declare) 
		shared_buffer buf_send;

		// - send message
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_factory_list_add(this->m_pservice_entity->m_instance.status.vector_factory, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}
	else if (message_type == MESSAGE_WEB_FACTORY_FILTER_USER) // "CFGC"
	{
		// check)
		CGASSERT_ERROR(m_pservice_entity != nullptr);

		// - get commnad
		auto command = msg.buf_message[4];

		if (command == 'C')
		{
			pmember->m_flag_set |= (uint64_t)0x04;

		}
		else if (command == 'U')
		{
			pmember->m_flag_set &= (~(uint64_t)0x04);

		}

		// declare) 
		shared_buffer buf_send;

		// - send message
		scoped_lock(this->m_pservice_entity->m_cs_instance)
		{
			buf_send = make_webmessage_factory_list_add(this->m_pservice_entity->m_instance.status.vector_factory, pmember->m_flag_set);
		}
		pmember->send(buf_send);
	}

	// return) 
	return eRESULT::BYPASS;
}
