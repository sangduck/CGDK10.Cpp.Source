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

	// 3) the executable of the socket (executable_socket::process_execute => process_execute_epoll). it was never set for UDP : no event reached the socket
	CGASSERT_ERROR(this->m_pexecutable_socket.exist());
	this->m_pexecutable_socket->set_socket(this);

	// 4) set event
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

	// check)
	RETURN_IF(handle_socket == INVALID_SOCKET, EPOLLIN);

	//-----------------------------------------------------------------
	// recvmmsg : up to COUNT_RECEIVE_BATCH_MAX datagrams in one syscall. (a FIONREAD and a recvfrom per datagram before)
	//  - the batch adapts : a full batch doubles the next one, a partial one is the next size (at least COUNT_RECEIVE_BATCH_MIN).
	//    (a socket with a datagram per event allocates two buffers, a busy one up to the max)
	//  - at least 2 : a batch of 1 is always full, so a datagram per event took a second recvmmsg (EAGAIN) and two more buffers.
	//    with 2 the partial batch tells the queue is drained (one syscall, one buffer more than the datagrams)
	//  - a buffer per datagram (the pool) : a message kept by the application keeps only its own buffer.
	//  - EPOLLET : drained until there is nothing more. (the edge is lost otherwise)
	//  - an exception of a message closes nothing (UDP) : logged, the rest of the batch is processed.
	//-----------------------------------------------------------------
	const auto size_message = this->m_message_buffer_size;

	// definitions)
	constexpr int COUNT_RECEIVE_BATCH_MIN = std::min(2, COUNT_RECEIVE_BATCH_MAX);

	for (;;)
	{
		// 2) prepare the batch
		//    (read once and kept in the limit : the arrays below hold COUNT_RECEIVE_BATCH_MAX. the other threads receiving on the socket change it)
		const int count_batch = std::clamp(this->m_count_receive_batch.load(std::memory_order_relaxed), COUNT_RECEIVE_BATCH_MIN, COUNT_RECEIVE_BATCH_MAX);

		shared_buffer	array_buffer[COUNT_RECEIVE_BATCH_MAX];
		net::sockaddr	array_address[COUNT_RECEIVE_BATCH_MAX];
		::iovec			array_iovec[COUNT_RECEIVE_BATCH_MAX];
		::mmsghdr		array_message[COUNT_RECEIVE_BATCH_MAX];		// - the entries of the batch are cleared below (not all 32 : 2KB)

		for (int i = 0; i < count_batch; ++i)
		{
			array_buffer[i] = alloc_shared_buffer(size_message);
			array_iovec[i].iov_base = array_buffer[i].data();
			array_iovec[i].iov_len = size_message;
			array_message[i] = ::mmsghdr{};
			array_message[i].msg_hdr.msg_name = &array_address[i];
			array_message[i].msg_hdr.msg_namelen = static_cast<socklen_t>(sizeof(array_address[i].bytes_));
			array_message[i].msg_hdr.msg_iov = &array_iovec[i];
			array_message[i].msg_hdr.msg_iovlen = 1;
		}

		// 3) receive
		const int count_received = ::recvmmsg(handle_socket, array_message, static_cast<unsigned int>(count_batch), MSG_DONTWAIT, nullptr);

		// check) nothing more (EAGAIN) or an error (not fatal for UDP : the next event receives again)
		if (count_received <= 0)
		{
			if (count_received < 0)
			{
				const auto error_code = api::get_last_error();

				if (error_code != EAGAIN && error_code != EWOULDBLOCK && error_code != EINTR)
					this->m_last_socket_error = error_code;
			}

			// - the next event starts with a small batch
			this->m_count_receive_batch.store(COUNT_RECEIVE_BATCH_MIN, std::memory_order_relaxed);

			// break)
			break;
		}

		// 4) process the datagrams
		for (int i = 0; i < count_received; ++i)
		{
			auto& buffer_receive = array_buffer[i];
			auto& address_peer = array_address[i];
			const auto bytes_transfered = static_cast<std::size_t>(array_message[i].msg_len);

			// - set length (a datagram longer than the buffer is truncated : as before)
			buffer_receive.set_size(bytes_transfered);
			address_peer.length = static_cast<int>(array_message[i].msg_hdr.msg_namelen);

			// statistics)
			this->statistics_on_receive_bytes(bytes_transfered);

			try
			{
				// - call process_on_receive
				this->process_on_receive(buffer_receive, address_peer);

				// - process message
				this->process_packet(buffer_receive, address_peer);
			}
			catch (std::exception& e)
			{
				// trace)
				LOG_DEBUG << "(excp) CGDK.net.socket: exception on processing a datagram (dropped) : "sv << e.what() << " " __CGFUNCTION__ ""sv;

				// - call 'exception handler'
				throwable::execute_exception_handler(this, e, __FUNCTION__);
			}
			catch (...)
			{
				// trace)
				LOG_DEBUG << "(excp) CGDK.net.socket: exception on processing a datagram (dropped) " __CGFUNCTION__ ""sv;

				// declare)
				throwable::failure e(eRESULT::EXCEPTION);

				// - call 'exception handler'
				throwable::execute_exception_handler(this, e, __FUNCTION__);
			}
		}

		// 5) adapt the batch : partial => the queue is drained (the next batch is the count received)
		if (count_received < count_batch)
		{
			this->m_count_receive_batch.store(std::max(count_received, COUNT_RECEIVE_BATCH_MIN), std::memory_order_relaxed);

			// break)
			break;
		}

		// - full => a larger batch next
		this->m_count_receive_batch.store((count_batch * 2 < COUNT_RECEIVE_BATCH_MAX) ? count_batch * 2 : COUNT_RECEIVE_BATCH_MAX, std::memory_order_relaxed);
	}

	// return)
	return EPOLLIN;
}


#endif