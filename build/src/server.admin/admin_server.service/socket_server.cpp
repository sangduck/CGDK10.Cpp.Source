#include "pch.h"
#include "socket_server.h"


socket_server::socket_server()
{
}

socket_server::~socket_server()
{
}


void socket_server::set_server_entity_info(server_entity* _pserver_register_info) noexcept
{
	// check)
	CGASSERT_ERROR(_pserver_register_info != nullptr);

	// 1) server_setting Info를 설정한다.
	m_pinfo_server_entity = _pserver_register_info;

	// 2) infoServer에 info_server_setting 정보를 업데이트 한다.
	if(_pserver_register_info != nullptr)
	{
		m_pinfo_server_entity->info_server_register.server_setting.id = _pserver_register_info->info_server_register.server_setting.id;
	}
}

void socket_server::reset_server_entity_info() noexcept
{
	// 1) Reset한다.
	m_pinfo_server_entity.reset();
}

void socket_server::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	// 1) set socket configure
	set_maximum_message_buffer_size(256 * 1024 * 1024);

	// 2) HostAgent group에 Enter한다.
	service::get_instance()->m_pgroup_server_immigration->enter(this);
}

void socket_server::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) 현재 붙어 있는 group에서 빠져나온다.
	leave_group();

	// 2) reset server_entity_info
	reset_server_entity_info();
}

result_code socket_server::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

shared_buffer socket_server::message_SERVER_ADMIN_SEND_SERVER_SETTING()
{
	// 1) SerserSetting 정보를 얻는다.
	auto pinfo_server_setting = get_server_entity_info();

	// check)
	CGASSERT_ERROR(pinfo_server_setting.exist());

	// declare) get size of message
	auto size_of_message = 8 + get_size_of(pinfo_server_setting->info_server_register.server_setting);

	// 2) alloc memory buffer
	auto buffer_send = alloc_shared_buffer(size_of_message);

	// 3) write message
	buffer_send.append<uint32_t>();
	buffer_send.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SEND_SERVER_SETTING);
	buffer_send.append<server_admin::sSERVER_SETTING_INFO>(pinfo_server_setting->info_server_register.server_setting);

	// 4) write message length
	buffer_send.front<uint32_t>() = buffer_send.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buffer_send.size());

	// return) 
	return buffer_send;
}

shared_buffer socket_server::message_SERVER_ADMIN_SERVER_ENTER()
{
	// 1) SerserSetting 정보를 얻는다.
	auto pinfo_server_setting = get_server_entity_info();

	// check)
	CGASSERT_ERROR(pinfo_server_setting.exist());

	// 2) 메시지를 작성해 리턴한다.
	return server_admin::message_SERVER_ADMIN_SERVER_ENTER(service::get_instance()->m_info_host.hid, pinfo_server_setting->info_server_register.server_setting.id, pinfo_server_setting->status_server);
}

shared_buffer socket_server::message_SERVER_ADMIN_SERVER_LEAVE()
{
	// 1) SerserSetting 정보를 얻는다.
	auto pinfo_server_setting = get_server_entity_info();

	// check)
	CGASSERT_ERROR(pinfo_server_setting.exist());

	// 2) 메시지를 작성해 리턴한다.
	return server_admin::message_SERVER_ADMIN_SERVER_LEAVE(service::get_instance()->m_info_host.hid, pinfo_server_setting->info_server_register.server_setting.id);
}

shared_buffer socket_server::message_SERVER_ADMIN_SERVER_UPDATE()
{
	// 1) SerserSetting 정보를 얻는다.
	auto pinfo_server_setting = get_server_entity_info();

	// check)
	CGASSERT_ERROR(pinfo_server_setting.exist());

	// 2) 메시지를 작성해 리턴한다.
	return server_admin::message_SERVER_ADMIN_SERVER_UPDATE(service::get_instance()->m_info_host.hid, pinfo_server_setting->info_server_register.server_setting.id, pinfo_server_setting->status_server);
}
