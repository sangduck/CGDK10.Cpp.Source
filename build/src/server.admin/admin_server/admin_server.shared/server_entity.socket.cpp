#include "pch.h"


server_entity::socket::socket() noexcept :
	m_pid(0),
	m_idx_list(-1)
{
}

server_entity::socket::~socket() noexcept
{
}

void server_entity::socket::request_service_start(const std::string& _str_setting)
{
	// 1) start 신호를 전송한다.
	send(server_admin::message_SERVER_ADMIN_REQUEST_START_SERVICE(m_info_server_setting.hid, m_info_server_setting.id, _str_setting));

	// LOG)
	LOG_PROGRESS << "> request 'START' SERVER['" << m_info_server_setting.name << "':" << m_info_server_setting.id.key << "]";
}

void server_entity::socket::request_service_stop()
{
	// 1) start 신호를 전송한다.
	send(server_admin::message_SERVER_ADMIN_REQUEST_STOP_SERVICE(m_info_server_setting.hid, m_info_server_setting.id));

	// LOG)
	LOG_PROGRESS << "> request 'STOP' SERVER['" << m_info_server_setting.name << "':" << m_info_server_setting.id.key << "]";
}

void server_entity::socket::request_service_destroy()
{
	// 1) start 신호를 전송한다.
	send(server_admin::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(m_info_server_setting.hid, m_info_server_setting.id));

	// LOG)
	LOG_PROGRESS << "> request 'DESTORY' SERVER['" << m_info_server_setting.name << "':" << m_info_server_setting.id.key << "]";
}

void server_entity::socket::on_request_connect(const net::sockaddr& /*_paddr_peer*/)
{
}

void server_entity::socket::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	// 1) Reset한다.
	m_pid		 = 0;
	m_info_server.clear();
	m_pserver_entity.reset();
	m_idx_list	 = -1;

	// check) 아직 Service가 RUNNING 상태가 아니면 강제로 접속을 끝난다.
	if(g_pservice->get_object_state() != eOBJECT_STATE::RUNNING)
	{
		// - 접속을 강제로 끊는다.
		closesocket();

		// return)
		return;
	}

	// 2) Immigration Group에 Enter시킨다.
	g_pservice->m_pgroup_server_entity_immigration->enter(this);
}

void server_entity::socket::on_fail_connect(net::io::Iconnective* /*_pconnective*/, uint64_t /*_reason*/)
{
}

void server_entity::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) Group에서 빠진다.
	leave_group();
}

result_code server_entity::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) set source
		_msg.set_source(this);

		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);

		// error) message isn't handled
		_error_unhandled_message(_msg);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::BYPASS;
}
