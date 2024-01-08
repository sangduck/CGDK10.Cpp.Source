#include "pch.h"


client::app::socket::socket() noexcept :
	m_idx_list(-1)
{
}

client::app::socket::~socket() noexcept
{
}

void client::app::socket::on_request_connect(const net::sockaddr& /*_paddr_peer*/)
{
}

void client::app::socket::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	// 1) Reset한다.
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
	g_pservice->m_pgroup_client_immigration->enter(this);
}

void client::app::socket::on_fail_connect(net::io::Iconnective* /*_pconnective*/, uint64_t /*_reason*/)
{
}

void client::app::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) Group에서 빠진다.
	leave_group();
}

result_code client::app::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// error)
	_error_unhandled_message(_msg);

	// return)
	return eRESULT::BYPASS;
}
