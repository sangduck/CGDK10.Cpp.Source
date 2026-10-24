#include "pch.h"

using namespace CGDK;


net::io::socket::Ndummy::Ndummy()
{
	native_handle(1);
}

net::io::socket::Ndummy::~Ndummy()
{
}

void net::io::socket::Ndummy::process_prepare_socket(ADDRESS_FAMILY _address_family)
{
	// 1) socket handle을 dummy값 아무거나로 설정한다. (단 INVALID_SOCKET_HANDLE만 아니면 된다.)
	if (native_handle() == INVALID_SOCKET || _address_family != m_address_family)
	{
		// - address family를 설정한다.
		address_family(_address_family);
	}

	// 2) socket을 Closed로 한다.
	socket_state(eSOCKET_STATE::CLOSED);

#if defined(_SYS_EPOLL_H)
	// 3) reset epoll
	reset_epoll_event_flag();
#endif

	// 4) reason을 reset한다.
	m_disconnect_reason = 0;

	// 9) Interfaces setting
	if (m_pconnectable == nullptr)
	{
		m_pbinder = dynamic_cast<Ibinder*>(this);
		m_pconnect_requestable = dynamic_cast<Iconnect_requestable*>(this);
		m_pconnectable = dynamic_cast<Iconnectable*>(this);
		m_psendable = dynamic_cast<Isendable*>(this);
		m_psend_request = dynamic_cast<Isend_request*>(this);
		m_preceivable_stream = dynamic_cast<Ireceivable_stream*>(this);
		m_preceivable_datagram = dynamic_cast<Ireceivable_datagram*>(this);
		m_ppacketable = dynamic_cast<Ipacketable*>(this);
	}

	// 9) Ibindable을 reset한다.
	if (m_pbinder != nullptr)
	{
		m_pbinder->process_reset_binder();
	}

	// 10) Iconnector을 reset한다.
	if (m_pconnect_requestable != nullptr)
	{
		m_pconnect_requestable->process_reset_connector();
	}

	// 11) Iconnectable을 reset한다.
	if (m_pconnectable != nullptr)
	{
		m_pconnectable->process_reset_connectable();
	}

	// 12) Isendable을 reset한다.
	if (m_psendable != nullptr)
	{
		m_psendable->process_reset_sendable();
	}

	// 13) Isend_request을 reset한다.
	if (m_psend_request != nullptr)
	{
		m_psend_request->reset_send_request();
	}

	// 14) Ireceivable_stream을 reset한다.
	if (m_preceivable_stream != nullptr)
	{
		m_preceivable_stream->process_reset_receivable();
	}

	// 15) Ireceivable_datagram을 reset한다.
	if (m_preceivable_datagram != nullptr)
	{
		m_preceivable_datagram->process_reset_receivable();
	}

	// 16) Ipacketable에 붙은 messageable을 reset한다.
	if (m_ppacketable != nullptr)
	{
		m_ppacketable->process_reset_packetable();
	}

	// 17) reset socket
	on_prepare_socket();
}


bool net::io::socket::Ndummy::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	// 1) 먼저 socket_handle을 INVALID_SOCKET으로 바꾸고 이전 socket_handle을 받아온다.
	auto handle_socket = native_handle(INVALID_SOCKET);

	// check) 
	RETURN_IF(handle_socket == INVALID_SOCKET, false);

	// 2) reason을 설정한다.
	m_disconnect_reason |= _disconnect_reason;

	// 3) reset executor
	reset_io_context();

	// return)
	return true;
}
