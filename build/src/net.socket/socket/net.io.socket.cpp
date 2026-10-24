#include "pch.h"

using namespace CGDK;


void net::io::socket::Nbase::process_prepare_socket(ADDRESS_FAMILY _address_family)
{
	// 1) socket을 새로 만든다.
	if(this->native_handle() == INVALID_SOCKET || _address_family != m_address_family)
	{
		// - socket을 새로 만든다.(기존 socket은 자동으로 파괴한다.)
		this->process_create_socket(_address_family);
	}

	// 2) socket을 Closed로 한다.
	this->socket_state(eSOCKET_STATE::CLOSED);

	// 3) reset values
	this->m_last_socket_error = 0;
	this->m_disconnect_reason = 0;

	// 5) set 'is client socket' false
	this->m_is_client_socket = false;

#if defined(_SYS_EPOLL_H)
	// 6) reset epoll
	this->reset_epoll_event_flag();

	// 7) reset shutdown handle
	this->set_epoll_handle(native_handle());

	// 8) reset io_uring completion (decided again in process_start_io. no op of the last connection is in flight : they held the socket)
	CGASSERT_ERROR(this->m_count_io_inflight.load(std::memory_order_acquire) == 0);
	this->m_is_io_completion = false;
	this->m_is_receiving_op = false;
#endif

	// 9)
	this->m_is_exception_on_send_overflow = false;
	this->m_is_send_overflow_closing = false;


	// 10) 통계정보를 reset한다.
	this->reset_statistics();

	// 11) reset socket을 수행한다.
	this->on_preparing_socket();

	// 12) Interfaces setting
	if (this->m_pconnectable == nullptr)
	{
		this->m_pbinder = dynamic_cast<Ibinder*>(this);
		this->m_pconnect_requestable = dynamic_cast<Iconnect_requestable*>(this);
		this->m_pconnectable = dynamic_cast<Iconnectable*>(this);
		this->m_psendable = dynamic_cast<Isendable*>(this);
		this->m_psend_request = dynamic_cast<Isend_request*>(this);
		this->m_preceivable_stream = dynamic_cast<Ireceivable_stream*>(this);
		this->m_preceivable_datagram = dynamic_cast<Ireceivable_datagram*>(this);
		this->m_ppacketable = dynamic_cast<Ipacketable*>(this);
	}

	// 13) Ibindable을 reset한다.
	if(this->m_pbinder != nullptr)
	{
		this->m_pbinder->process_reset_binder();
	}

	// 14) Iconnector을 reset한다.
	if(this->m_pconnect_requestable != nullptr)
	{
		this->m_pconnect_requestable->process_reset_connector();
	}

	// 15) Iconnectable을 reset한다.
	if(this->m_pconnectable != nullptr)
	{
		this->m_pconnectable->process_reset_connectable();
	}

	// 16) Isendable을 reset한다.
	if(this->m_psendable != nullptr)
	{
		this->m_psendable->process_reset_sendable();
	}

	// 17) Isend_request을 reset한다.
	if(this->m_psend_request != nullptr)
	{
		this->m_psend_request->reset_send_request();
	}

	// 18) Ireceivable_stream을 reset한다.
	if(this->m_preceivable_stream != nullptr)
	{
		this->m_preceivable_stream->process_reset_receivable();
	}

	// 19) Ireceivable_datagram을 reset한다.
	if (this->m_preceivable_datagram != nullptr)
	{
		this->m_preceivable_datagram->process_reset_receivable();
	}

	// 20) Ipacketable에 붙은 messageable을 reset한다.
	if(this->m_ppacketable != nullptr)
	{
		this->m_ppacketable->process_reset_packetable();
	}

	// 20.1) Icryptable : drop the cryptor left by the previous session.
	//       (the socket object is reused from the pool without re-construction,
	//        cryption_type is kept : the acceptor sets it again on every accept.)
	if(auto pcryptable = dynamic_cast<Icryptable*>(this); pcryptable != nullptr)
	{
		pcryptable->process_reset_cryptor();
	}

	// 21) reset socket을 수행한다.
	this->on_prepare_socket();
}
