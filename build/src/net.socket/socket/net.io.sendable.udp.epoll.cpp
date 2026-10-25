#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include "net.io.sendable.executable_send_buffer.h"

using namespace CGDK;

//-----------------------------------------------------------------------------
// UDP sendable (epoll)
//
//  - a datagram is sent at once (sendto) by the thread that sends while no other thread sends.
//  - sendmmsg : the datagrams queued meanwhile (other threads, send_defer in a group lock / send cork,
//    the socket buffer full : EPOLLOUT) are sent in one syscall, SEND_BATCH_MAX at a time.
//  - the socket buffer full (EWOULDBLOCK) : the rest waits for EPOLLOUT (process_recover_sendable). (dropped before)
//  - a send error of a datagram (ICMP unreachable, EMSGSIZE ...) drops that datagram only. UDP : the socket is not closed.
//-----------------------------------------------------------------------------

bool net::io::sendable::Nudp::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress)
{
	// check) _buffer.size() must not be 0
	CGASSERT_FATAL(_buffer.size() != 0);

	// check)
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data() must not be nullptr
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check)
	if(_buffer.data() == nullptr)
		return false;

	// 1) get socket handle
	const auto handle_socket = this->native_handle();

	// check) invalid socket?
	if(handle_socket == INVALID_SOCKET)
	{
		LOG_DEBUG << "(err ) CGDK.net.socket: socket handle is INVALID_HANDLE " __CGFUNCTION__;
		return false;
	}

	// 2) send defer : this thread defers its sends (a group lock / send cork)
	const auto ptoken = send_defer::token();
	const bool is_deferring = send_defer::is_deferring(static_cast<const Isocket*>(this));

	// declare)
	bool is_queued = false;
	bool is_defer_first = false;
	bool is_defer_full = false;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		if (this->m_flag_sending || is_deferring)
		{
			// - a thread is sending (or EPOLLOUT is awaited) or this thread defers : queue (the order is kept). the sending thread sends it.
			if (this->m_flag_sending == false)
			{
				// - nobody sends : this thread becomes the sending thread and the owner of the deferred sends
				this->m_flag_sending = true;
				this->m_pdefer_owner.store(ptoken, std::memory_order_relaxed);
				is_defer_first = true;
			}

			this->m_queue_pending.push_back(NODE{ _buffer, std::move(_powner), _paddress, _count_message });
			this->m_bytes_pending += _buffer.size();
			is_queued = true;

			// - the owner of the deferred sends sends them early when they are many
			if (is_deferring && this->m_pdefer_owner.load(std::memory_order_relaxed) == ptoken)
				is_defer_full = (this->m_queue_pending.size() >= SEND_DEFER_MAX_COUNT || this->m_bytes_pending >= SEND_DEFER_MAX_BYTES);
		}
		else
		{
			// - this thread sends now
			this->m_flag_sending = true;
		}
	}

	// case) queued
	if (is_queued)
	{
		// - registers the deferred sends of this thread (sent at the end of the group lock / cork). failed => sent now
		if (is_defer_first && send_defer::push(this, object_ptr<Ireferenceable>(this)) == false)
			is_defer_full = true;

		if (is_defer_full)
			this->process_flush_deferred_send();

		// return)
		return true;
	}

	// 3) send at once (EINTR : again)
	ssize_t bytes_sent;

	for (;;)
	{
		bytes_sent = ::sendto(handle_socket, _buffer.data(), _buffer.size(), MSG_DONTWAIT | MSG_NOSIGNAL, reinterpret_cast<const ::sockaddr*>(&_paddress), static_cast<socklen_t>(_paddress.length));

		if (bytes_sent >= 0 || api::get_last_error() != EINTR)
			break;
	}

	// declare)
	bool result = true;

	if (bytes_sent >= 0)
	{
		// - sent
		this->process_complete_sendable(0, static_cast<std::size_t>(bytes_sent), _count_message);
		_powner.reset();
	}
	else
	{
		// - get last error
		const auto error_code = api::get_last_error();

		// case) the socket buffer is full : queue at the head and wait for EPOLLOUT (m_flag_sending stays : the sends meanwhile are queued)
		if (error_code == EWOULDBLOCK || error_code == EAGAIN)
		{
			SCOPED_LOCK(this->m_lockable_sending)
			{
				this->m_queue_pending.push_front(NODE{ _buffer, std::move(_powner), _paddress, _count_message });
				this->m_bytes_pending += _buffer.size();
			}

			// - wait for EPOLLOUT
			this->_process_set_epollout(true);

			// return)
			return true;
		}

		// case) failed : dropped (UDP)
		this->m_last_socket_error = error_code;

		// statistics)
		this->statistics_on_error_send();

		// trace)
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to call 'sendto' error_code:"sv << error_code << " " __CGFUNCTION__;

		// - complete (failed)
		this->process_complete_sendable(1, 0, 0);

		// - set result false
		result = false;
	}

	// 4) the datagrams queued meanwhile by the other threads (sendmmsg). the socket buffer full => EPOLLOUT
	if (this->_process_send_pending(handle_socket) == EPOLLOUT)
		this->_process_set_epollout(true);

	// return)
	return result;
}

uint32_t net::io::sendable::Nudp::_process_send_pending(CGSOCKET _handle_socket)
{
	//-----------------------------------------------------------------
	// the sending thread (m_flag_sending) sends the queued datagrams with sendmmsg until the queue is empty.
	//
	//  return 0        : all sent (m_flag_sending = false)
	//         EPOLLOUT : the socket buffer is full. the rest is at the head of the queue (m_flag_sending stays)
	//  - an error of a datagram (sendmmsg returns -1 : the first one failed) drops that datagram and goes on.
	//    sent partially (0 < n < count) : the error of the next one is reported by the next call (retried).
	//-----------------------------------------------------------------
	// declare)
	// - the datagrams of a batch : list nodes moved from the queue by splice (no allocation, no copy. a vector of SEND_BATCH_MAX nodes was
	//   4.6KB from malloc at every call : beyond the thread cache)
	std::list<NODE> list_send;
	NODE* array_node[SEND_BATCH_MAX];
	::mmsghdr array_message[SEND_BATCH_MAX];
	::iovec array_iovec[SEND_BATCH_MAX];

	for (;;)
	{
		// 1) take up to SEND_BATCH_MAX datagrams (the batch before is released : the owners of the sent datagrams)
		list_send.clear();

		SCOPED_LOCK(this->m_lockable_sending)
		{
			// - nothing left => this thread stops sending
			if (this->m_queue_pending.empty())
			{
				this->m_flag_sending = false;
				return 0;
			}

			auto iter_end = this->m_queue_pending.begin();

			for (std::size_t count = 0; count < SEND_BATCH_MAX && iter_end != this->m_queue_pending.end(); ++count, ++iter_end)
				this->m_bytes_pending -= iter_end->buf_send.size();

			list_send.splice(list_send.end(), this->m_queue_pending, this->m_queue_pending.begin(), iter_end);
		}

		// 2) sendmmsg
		std::size_t count_send = 0;

		for (auto& node : list_send)
		{
			const auto i = count_send++;

			array_node[i] = &node;

			array_iovec[i].iov_base = const_cast<char*>(node.buf_send.data());
			array_iovec[i].iov_len = node.buf_send.size();
			array_message[i] = ::mmsghdr{};
			array_message[i].msg_hdr.msg_name = &node.address;
			array_message[i].msg_hdr.msg_namelen = static_cast<socklen_t>(node.address.length);
			array_message[i].msg_hdr.msg_iov = &array_iovec[i];
			array_message[i].msg_hdr.msg_iovlen = 1;
		}

		int count_sent;

		for (;;)
		{
			count_sent = ::sendmmsg(_handle_socket, array_message, static_cast<unsigned int>(count_send), MSG_DONTWAIT | MSG_NOSIGNAL);

			if (count_sent >= 0 || api::get_last_error() != EINTR)
				break;
		}

		// 3) the sent ones : statistics (the owners are released with list_send)
		const std::size_t count_ok = (count_sent > 0) ? static_cast<std::size_t>(count_sent) : 0;

		for (std::size_t i = 0; i < count_ok; ++i)
			this->process_complete_sendable(0, static_cast<std::size_t>(array_message[i].msg_len), array_node[i]->count_message);

		// check) all sent => the next batch
		if (count_ok == count_send)
			continue;

		// 4) the first one not sent : EAGAIN => EPOLLOUT. an error => dropped. (sent partially => retried by the next call)
		std::size_t index_rest = count_ok;
		bool is_full = false;

		if (count_sent < 0)
		{
			const auto error_code = api::get_last_error();

			if (error_code == EWOULDBLOCK || error_code == EAGAIN)
			{
				is_full = true;
			}
			else
			{
				// - dropped
				this->m_last_socket_error = error_code;

				// statistics)
				this->statistics_on_error_send();

				// trace)
				LOG_DEBUG << "(err ) CGDK.net.socket: fail to call 'sendmmsg' error_code:"sv << error_code << " (a datagram dropped) " __CGFUNCTION__;

				// - complete (failed)
				this->process_complete_sendable(1, 0, 0);

				index_rest = 1;
			}
		}

		// 5) the rest back to the head of the queue (in order)
		if (index_rest < count_send)
		{
			auto iter_rest = list_send.begin();
			std::advance(iter_rest, index_rest);

			SCOPED_LOCK(this->m_lockable_sending)
			{
				for (auto iter = iter_rest; iter != list_send.end(); ++iter)
					this->m_bytes_pending += iter->buf_send.size();

				this->m_queue_pending.splice(this->m_queue_pending.begin(), list_send, iter_rest, list_send.end());
			}
		}

		// check) the socket buffer is full => wait for EPOLLOUT
		if (is_full)
			return EPOLLOUT;
	}
}

void net::io::sendable::Nudp::_process_set_epollout(bool _enable) noexcept
{
	// - the UDP socket is registered with EPOLLET | EPOLLIN | EPOLLERR (receivable::Ndatagram::prepare_receive).
	//   EPOLLOUT is added while the sending thread waits for the socket buffer (m_flag_epollout : no thread sends, the queue waits)
	//   and removed when a thread takes the queue over (process_recover_sendable).
	// - the flag and the registered events change together under m_lockable_sending : set_event() outside the lock let the
	//   'off' of a thread that had sent everything land after the 'on' of the next sending thread (EAGAIN) => EPOLLOUT was not
	//   registered while the queue waited for it and the socket sent nothing any more (the sends after it were only queued).
	SCOPED_LOCK(this->m_lockable_sending)
	{
		RETURN_IF(this->m_flag_epollout == _enable);

		this->m_flag_epollout = _enable;

		// - get io_context
		auto pio_context = this->io_context();

		// check)
		RETURN_IF(pio_context.empty());

		// - get socket handle
		const auto handle_socket = this->native_handle();

		// check)
		RETURN_IF(handle_socket == INVALID_SOCKET);

		// - set event (the executable is kept : nullptr)
		pio_context->set_event(static_cast<CGHANDLE>(handle_socket), static_cast<uint32_t>(EPOLLET | EPOLLIN | EPOLLERR) | (_enable ? static_cast<uint32_t>(EPOLLOUT) : 0u), nullptr);
	}
}

uint32_t net::io::sendable::Nudp::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	// 1) get socket handle
	const auto handle_socket = this->native_handle();

	// check)
	RETURN_IF(handle_socket == INVALID_SOCKET, 0);

	// 2) take the queue over : EPOLLOUT off under the lock. not waiting => nothing (EPOLLET : an event carries EPOLLOUT while it is
	//    registered. another thread took the queue over already, or the queue was sent and a thread sends now : it is the sending thread)
	SCOPED_LOCK(this->m_lockable_sending)
	{
		RETURN_IF(this->m_flag_epollout == false, 0);

		this->_process_set_epollout(false);
	}

	// 3) send the queue (this thread is the sending thread now). the socket buffer full again => EPOLLOUT
	if (this->_process_send_pending(handle_socket) == EPOLLOUT)
		this->_process_set_epollout(true);

	// return)
	return 0;
}

void net::io::sendable::Nudp::process_flush_deferred_send() noexcept
{
	// - the owner of the deferred sends (send_defer) sends them. already sent (an earlier flush) or taken by a close : nothing
	auto ptoken = send_defer::token();

	// check) not the owner
	if(this->m_pdefer_owner.compare_exchange_strong(ptoken, nullptr, std::memory_order_acq_rel) == false)
		return;

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Nudp::process_flush_deferred_send_on_close() noexcept
{
	// - called before the socket is closed : sends what any thread deferred
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) nothing deferred
		if(this->m_pdefer_owner.exchange(nullptr, std::memory_order_acq_rel) == nullptr)
			return;
	}

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Nudp::_process_send_deferred() noexcept
{
	// 1) get socket handle
	const auto handle_socket = this->native_handle();

	// check) closed meanwhile => drop (m_flag_sending = false)
	if (handle_socket == INVALID_SOCKET)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->_process_clear_send_buffers();
		}

		// return)
		return;
	}

	// 2) send the queue. the socket buffer full => EPOLLOUT
	if (this->_process_send_pending(handle_socket) == EPOLLOUT)
		this->_process_set_epollout(true);
}

void net::io::sendable::Nudp::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics)
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

void net::io::sendable::Nudp::process_reset_sendable() noexcept
{
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - drop the queue
		this->_process_clear_send_buffers();

		// - send defer : the deferring thread flushes nothing
		this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);
		this->m_flag_epollout = false;
	}
}

void net::io::sendable::Nudp::_process_clear_send_buffers()
{
	// - under m_lockable_sending
	for ([[maybe_unused]] auto& iter : this->m_queue_pending)
		this->process_complete_sendable(1, 0, 0);

	this->m_queue_pending.clear();
	this->m_bytes_pending = 0;
	this->m_flag_sending = false;
}


#endif
