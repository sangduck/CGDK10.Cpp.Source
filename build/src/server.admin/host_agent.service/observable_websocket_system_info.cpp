#include "pch.h"

void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::string_view _str_value);
void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::size_t _value);
//-----------------------------------------------------------------
// 설명) host system info
// 
//   _make_webmessage_host_info (host info)
// 
//    'ASGN' + [title_host] + [title_user_info] + [title_os_info] + [processor_count]
// 
//-----------------------------------------------------------------
shared_buffer _make_webmessage_host_info(socket_websocket* _psocket)
{
	// 1) buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_text.append_text<char>(MESSAGE_WEB_GENERAL); // "ASGN"

	// 3) title_host_info
	{
		std::string str_value = CGDK::stream_string("<span class=\'top_bar_tag\'><i class=\'fa fa-server\'></i> ", server_system::instance_manager::self()->m_instance.info.setting.name, "</span> <span class=\'top_bar_value\'>", api::socket::instance().get_local_address_public_first().to_string(), "</span>");
		::_append_key_value(buf_text, "title_host_info", str_value);
	}

	// 4) title_user_info
	{
		std::string str_value = CGDK::stream_string("<span class=\'top_bar_tag\'><i class=\'fa fa-user-circle\'></i> ", "SANGDUCK", "</span> <span class=\'top_bar_value\'>", to_string(_psocket->remote_endpoint()), "</span>");
		::_append_key_value(buf_text, "title_user_info", str_value);
	}

	// 5) title_os_info
	{
		// - os info
		auto& os_info = server_system::instance_manager::self()->m_instance.info.info.os_info;

		if (os_info.type == eOS_TYPE::WINDOWS_NATIVE
			|| os_info.type == eOS_TYPE::WINDOWS_UNIVERSAL)
		{
			std::string str_value = CGDK::stream_string("<i class=\'fa fa-windows\'></i> ", os_info.version.str_version);
			::_append_key_value(buf_text, "title_os_info", str_value);
		}
		else if (os_info.type == eOS_TYPE::LINUX)
		{
			std::string str_value = CGDK::stream_string("<i class=\'fa fa-linux\'></i> ", os_info.version.major, ".", os_info.version.minor, ".", os_info.version.build);
			::_append_key_value(buf_text, "title_os_info", str_value);
		}
		else
		{
			::_append_key_value(buf_text, "title_os_info", "n/a");
		}
	}

	// 6) processor_count_v
	{
		// - serialize
		std::string str_value = CGDK::stream_string("x", api::get_core_count());
		::_append_key_value(buf_text, "processor_count_v", str_value);
	}

	// 7) send message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));
}

//-----------------------------------------------------------------
// 설명) host system status
// 
//   make_webmessage_host_host_status (system status)
// 
//    'ASST' + {[process_value] + [process_meter]} + {[memory_value] + [memory_meter]} + {[network_value] + [network_meter]}
// 
//-----------------------------------------------------------------
shared_buffer _make_webmessage_host_system_status()
{
	// 1) buffer
	auto buf_text = alloc_shared_buffer(DEFAULT_BUFFER_FOR_WEB_PAGE);

	// 2) append message head
	buf_text.append_text<char>(MESSAGE_WEB_SYSTEM_STATUS); // "ASST"

	// 3) get host info
	const auto& status = server_system::instance_manager::self()->m_instance.info.status;

	// 4) get status
	auto status_now = status.vector_status.back();

	// 4) processor_usage_v
	{
		// - text_value
		std::string str_value = CGDK::stream_string(
			status_now.processor_usage / 1000,
			".",
			CGDK::stream_width(status_now.processor_usage % 1000, 2, '0'),
			"%\n");
		buf_text.append_text<char>(str_value);

		// - meter
		buf_text.append_text<char>(str_value);
	}

	// 5) mem_status_v
	{
		// - value
		buf_text.append_text<char>(to_string_scaled_byte(status_now.bytes_memory_usage));
		buf_text.append<char>('\n');
	}
	{
		// - meter
		buf_text.append_text<char>((static_cast<float>(status_now.bytes_memory_usage) * 100.0f) / static_cast<float>(status.system_info.memory.bytes_physical_total));
		buf_text.append<char>('\n');
	}

	// 6) network_staus_v
	{
		// - value
		buf_text.append_text<char>(to_string_scaled_byte(status_now.bytes_traffic));
		buf_text.append<char>('\n');
	}
	{
		// - meter
		std::string str_value = "100\n";
		buf_text.append_text<char>(str_value);
	}

	// 7) make message
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_text.to_string<char>()));
}

observable_websocket_system_info::observable_websocket_system_info()
{
}

observable_websocket_system_info::~observable_websocket_system_info()
{
}

result_code observable_websocket_system_info::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// 1) register schedulable
	if (this->empty())
	{
		this->execute_interval(5s);
		system_executor::register_schedulable(this);
	}

	// return) 
	return eRESULT::SUCCESS;
}

void observable_websocket_system_info::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// declare) 
	shared_buffer buf_send_1;
	shared_buffer buf_send_2;

	SCOPED_LOCK(server_system::instance_manager::self()->m_lockable_instance)
	{
		buf_send_1 = _make_webmessage_host_info(_pobserver);
		buf_send_2 = _make_webmessage_host_system_status();
	}

	// 1) send server status
	_pobserver->send(buf_send_1);
	_pobserver->send(buf_send_2);
}

uintptr_t observable_websocket_system_info::on_observer_unregistering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/)
{
	// 1) unregister schedulable
	if (observer_count() == 1)
	{
		system_executor::unregister_schedulable(this);
	}

	// return) 
	return 0;
}

void observable_websocket_system_info::on_observer_unregistered(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

uintptr_t observable_websocket_system_info::on_observer_data_updating(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return 0;
}

void observable_websocket_system_info::on_observer_data_update(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

void observable_websocket_system_info::on_execute()
{
	// declare) 
	shared_buffer buf_send;

	// 1) make message
	SCOPED_LOCK(server_system::instance_manager::self()->m_lockable_instance)
	{
		buf_send = _make_webmessage_host_system_status();
	}

	// 2) send buffer
	this->send(buf_send);
}
