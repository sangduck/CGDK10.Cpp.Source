#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;


net::io::receivable::Ndatagram::Ndatagram() noexcept :
	m_message_buffer_size(DEFAULT_UDP_MESSAGE_BUFFER_SIZE)
#if defined(_WINSOCK2API_)
	,m_prepare_receive_count(DEFAULT_UDP_COUNT_RECEIVE_BUFFER)
#endif
{
}

void net::io::receivable::Ndatagram::set_message_size(std::size_t _message_size)
{
	// check) 최소 Packet Size가 너무 작아도 안된다.
	CGASSERT_ERROR(_message_size >= MIN_UDP_MESSAGE_SIZE);

	// check) 최소 Packet Size가 너무 커도 안된다.
	CGASSERT_ERROR(_message_size <= MAX_UDP_MESSAGE_SIZE);

	// check)
	RETURN_IF(this->m_message_buffer_size == _message_size);

	// 1) set message size
	this->m_message_buffer_size = _message_size;
}

std::size_t net::io::receivable::Ndatagram::get_prepare_receive_count() const
{
	return 0;
}

void net::io::receivable::Ndatagram::set_prepare_receive_count([[maybe_unused]] std::size_t _count)
{
	// check) 최소 _count가 1보다 작으면 안된다.
	CGASSERT_ERROR(_count > 0);
}

void net::io::receivable::Ndatagram::suspend_receive()
{
}

void net::io::receivable::Ndatagram::resume_receive()
{
}

bool net::io::receivable::Ndatagram::prepare_receive()
{
	// check)
	ERROR_RETURN_IF_INVALID_SOCKET(false, LOG_DEBUG << "(err ) CGDK.net.socket: socket handle is 'INVALID_HANDLE' " __CGFUNCTION__ ""sv)

	// 1) socket Status를 바꾼다.
	eSOCKET_STATE socket_state_old = this->socket_state(eSOCKET_STATE::BINDED);

	// check) 
	RETURN_IF(socket_state_old >= eSOCKET_STATE::BINDED, false);

	// trace) 최대 message크기를 출력한다.
	LOG_INFO << "(info) CGDK.net.socket: udp max message size is "sv << api::socket::get_max_message_size(native_handle());

	// trace) 버퍼의 크기를 출력한다.
	LOG_INFO << "(info) CGDK.net.socket: udp message buffer size is "sv << get_message_size();

	// 2) get io_context
	auto pio_context = this->io_context();

	// check) io_context가 없으면 잘못된 것이다.
	CGASSERT_ERROR(pio_context.exist());

	// 3) Event를 설정한다.
	return pio_context->set_event(native_handle(), EPOLLET | EPOLLIN | EPOLLERR, this->m_pexecutable_socket);

	// return)
	return true;
}


intptr_t net::io::receivable::Ndatagram::process_complete_receivable( uintptr_t /*_result*/, std::size_t /*_transferred*/, shared_buffer& /*_buffer*/, const net::sockaddr& /*_address*/)
{
	// check) 
	CGASSERT(socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// 1) get socket handle
	auto handle_socket = this->native_handle();

	// 2) get message
	while (true)
	{
		// - get next pended message size
		std::size_t size_pended = api::socket::get_pending_read_buffer(native_handle());

		// check)
		CGASSERT_ERROR(size_pended <= static_cast<std::size_t>(api::socket::get_max_message_size(handle_socket)));

		// check)
		BREAK_IF(size_pended == 0);

		// - alloc shared buffer
		auto buffer_receive = alloc_shared_buffer(size_pended);

		// declare) 
		net::sockaddr address_peer;

		// - set sockaddr length
		address_peer.length = sizeof(net::sockaddr);

		// - receive datagram data
		auto bytes_transfered = ::recvfrom(handle_socket, buffer_receive.data(), this->m_message_buffer_size, MSG_DONTWAIT, reinterpret_cast<::sockaddr*>(&address_peer), reinterpret_cast<::socklen_t*>(&address_peer.length));

		// check) ...
		BREAK_IF(bytes_transfered <= 0);

		// - set length
		buffer_receive.set_size(bytes_transfered);

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// - call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
		this->process_on_receive(buffer_receive, address_peer);

		// - process message
		//    - 여기서 Return해 오는 값은 다음 message를 받기위해 대음에 필요로 하는 메모리 용량이다.
		//    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
		//      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
		//    - Exception이 발생가능하다.
		this->process_packet(buffer_receive, address_peer);
	}

	// return)
	return EPOLLIN;
}


#endif