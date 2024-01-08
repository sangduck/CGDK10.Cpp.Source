#include "pch.h"


void _append_querier_entity(buffer& _buf_dest, time_event::Ientity* _querier_info)
{
	// 1) get entity info
	const auto& entity_setting = _querier_info->entity_setting();
	const auto& entity_status = _querier_info->entity_status();

	// 2) "Name"
	_buf_dest.append_text<char>("<td><span style='font-size:15px;font-weight:bold'><i class='xi-alarm-o'></i> "sv);
	_buf_dest.append_text<char>(entity_setting.name);
	_buf_dest.append_text<char>("</span></td>"sv);

	// 3) TYPE (ONCE/ITERATION/...)
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string<char>(static_cast<time_event::eTYPE>(entity_setting.type)));
	_buf_dest.append_text<char>("</td>"sv);

	// 4) STATUS
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(entity_status.state));
	_buf_dest.append_text<char>("</td>"sv);

	// 5) SETTER
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(entity_setting.setter));
	_buf_dest.append_text<char>("</td>"sv);

	// 6) TIME.SET
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(entity_status.time_setup, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	_buf_dest.append_text<char>("</td>"sv);

	// 7) TIME.NEXT
	_buf_dest.append_text<char>("<td>"sv);
	_buf_dest.append_text<char>(to_string(entity_status.time_next, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	_buf_dest.append_text<char>("</td>"sv);

	// 8) TIME.LAST.TRY
	_buf_dest.append_text<char>("<td>"sv);
	if (entity_status.count_try != 0)
	{
		_buf_dest.append_text<char>(to_string(entity_status.time_last_try, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 9) TRY
	_buf_dest.append_text<char>("<td>"sv);
	if (entity_status.count_try != 0)
	{
		_buf_dest.append_text<char>(to_string(entity_status.count_try));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 10) TIME.LAST_SUCCESS
	_buf_dest.append_text<char>("<td>"sv);
	if (entity_status.count_succeeded != 0)
	{
		_buf_dest.append_text<char>(to_string(entity_status.time_last_succeeded, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 11) SUCCESS
	_buf_dest.append_text<char>("<td>"sv);
	if (entity_status.count_succeeded != 0)
	{
		_buf_dest.append_text<char>(to_string(entity_status.count_succeeded));
	}
	_buf_dest.append_text<char>("</td>"sv);

	// 12) FAILED
	_buf_dest.append_text<char>("<td>"sv);
	if (entity_status.count_failed != 0)
	{
		_buf_dest.append_text<char>(to_string(entity_status.count_failed));
	}
	_buf_dest.append_text<char>("</td>"sv);
}

shared_buffer _make_webmessage_querier_entity_add(time_event::Ientity* _querier_info)
{
	// 1) get service instance 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 4) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 5) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_querier_entity(buf_text, _querier_info);

		// - append command
		buf_result.append<char>('A');

		// - tr element id
		auto tr_element_id = NFMT::format("sc{}", reinterpret_cast<uint64_t>(_querier_info));

		// - append element_id
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

shared_buffer _make_webmessage_querier_entity_update(time_event::Ientity* _querier_info, uint64_t /*_flag*/)
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 4) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 5) append message 
	{
		// - temporary buffer
		buffer buf_text = buf_temp;

		// - append tr data
		_append_querier_entity(buf_text, _querier_info);

		// - append command
		buf_result.append<char>('U');

		// - tr element id
		auto tr_element_id = NFMT::format("sc{}", reinterpret_cast<uint64_t>(_querier_info));

		// - <tr>
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

shared_buffer _make_webmessage_querier_entity_remove(time_event::Ientity* _querier_info)
{
	// 1) 
	object_ptr<service> pservice = service::instance();

	// 2) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 3) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 4) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 5) append message 
	{
		// - append command (A:Add, D:delete, U:update)
		buf_result.append<char>('D');

		// - tr element id
		auto tr_element_id = NFMT::format("sc{}", reinterpret_cast<uint64_t>(_querier_info));

		// - append tr_element_id
		buf_result.append_text<char>(tr_element_id);
		buf_result.append<char>('\n');
	}

	// 5) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));
}

shared_buffer _make_webmessage_querier_list_add()
{
	// 1) buffer
	auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	// 3) get time_event amanger
	auto ptime_event_entity_manager = time_event::entity_manager::instance();

	// 4) append message 
	scoped_lock(ptime_event_entity_manager->get_lockable())
	{
		for (const auto& iter : *ptime_event_entity_manager)
		{
			// - temporary buffer
			buffer buf_text = buf_temp;

			// - tr element id
			auto tr_element_id = NFMT::format("sc{}", reinterpret_cast<uint64_t>(iter.get()));

			// - append tr data
			_append_querier_entity(buf_text, iter);

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

shared_buffer make_webmessage_querier_list_update()
{
	//// 1) buffer
	//auto buf_result = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);
	//auto buf_temp = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	//// 2) append message head
	//buf_result.append_text<char>(MESSAGE_WEB_SCHEDULABLE_LIST);

	//// 3) get time_event amanger
	//auto ptime_event_entity_manager = time_event::entity_manager::instance();

	//// 4) append message 
	//scoped_lock(ptime_event_entity_manager->get_lockable())
	//{
	//	for (const auto& iter : ptime_event_entity_manager->m_vector_entity)
	//	{
	//		// - temporary buffer
	//		buffer buf_text = buf_temp;

	//		// - tr element id
	//		auto tr_element_id = NFMT::format("sc{}", reinterpret_cast<uint64_t>(iter.get()));

	//		// - append tr data
	//		_append_executor_entity(buf_text, iter);

	//		// - append command
	//		buf_result.append<char>('A');

	//		// - append element id
	//		buf_result.append_text<char>(tr_element_id);
	//		buf_result.append<char>('\n');

	//		// - append length
	//		buf_result.append_text<char>(buf_text.size());
	//		buf_result.append<char>('\n');

	//		// - append data
	//		buf_result.append(buf_text.size(), buf_text.data());
	//	}
	//}

	//// 5) make message
	//return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_result.to_string<char>()));

	return shared_buffer();
}



observable_websocket_server_querier_info::observable_websocket_server_querier_info(const std::string_view _name)
{
	this->set_name(_name);

	this->m_webpage_str_target = "webpage_server_querier_info.htm"sv;
	this->m_webpage_index = 'E';
}

observable_websocket_server_querier_info::~observable_websocket_server_querier_info()
{
}

void observable_websocket_server_querier_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// 2) send factory list
	_pobserver->send(_make_webmessage_querier_list_add());
}

uintptr_t observable_websocket_server_querier_info::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) unregister to observable
	_pobserver->unsubscribe(OCID_FRAME3);

	// 2) uregister _pmenber
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

