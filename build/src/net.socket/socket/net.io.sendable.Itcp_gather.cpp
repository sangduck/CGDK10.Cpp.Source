#include "pch.h"

using namespace CGDK;

std::size_t net::io::sendable::Itcp_gather::m_s_default_max_depth_of_send_buffer = DEFAULT_MAX_DEPTH_OF_MESSAGE_SEND_QUEUE;
std::size_t net::io::sendable::Itcp_gather::m_s_default_max_queued_bytes = DEFAULT_BYTES_MAX_MESSAGE_SEND_QUEUE;
std::size_t net::io::sendable::Itcp_gather::m_s_default_max_queued_bytes_accepted = DEFAULT_BYTES_MAX_MESSAGE_SEND_QUEUE_ACCEPTED;

net::io::sendable::Itcp_gather::Itcp_gather() noexcept :
	m_bytes_max_queued{ net::io::sendable::Itcp_gather::default_maximum_queued_bytes() },
	m_max_depth_of_send_buffer{ net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer() }
{
}

void net::io::sendable::Itcp_gather::maximum_depth_of_send_buffer(std::size_t _depth) noexcept
{
	// check) 
	RETURN_IF(this->m_max_depth_of_send_buffer == _depth);

	// 1) 
	this->m_max_depth_of_send_buffer = _depth;
}

void net::io::sendable::Itcp_gather::maximum_depth_of_send_buffer_bytes(std::size_t _size) noexcept
{
	// check) 
	RETURN_IF(this->m_bytes_max_queued == _size);

	// 1) 
	this->m_bytes_max_queued = _size;
}

void net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer(std::size_t _max) noexcept
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_max == m_s_default_max_depth_of_send_buffer);

	// check) 최소 4 최대 16384로 한다.
	SATURATE(_max, 4, (4096 * 4));

	// 1) 값을 설정한다.
	m_s_default_max_depth_of_send_buffer = _max;

	// log) 
	LOG_INFO << log_level_t::important() << "(info) set " << m_s_default_max_depth_of_send_buffer << " as DEFAULT MAX SENDBUFFER DEPTH of net::io::sendable::Itcp_gather.";
}

std::size_t net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer() noexcept
{
	return m_s_default_max_depth_of_send_buffer;
}

void net::io::sendable::Itcp_gather::default_maximum_queued_bytes(std::size_t _max) noexcept
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_max == m_s_default_max_queued_bytes);

	// check) 최소 256K로 설정한다.
	SATURATE_MIN(_max, (256 * 1024));

	// 1) 값을 설정한다.
	m_s_default_max_queued_bytes = _max;

	// log) 
	LOG_INFO << log_level_t::important() << "(info) set " << m_s_default_max_queued_bytes << " bytes as DEFAULT MAX SUSTAIN QUEUED BYTE of net::io::sendable::Itcp_gather.";
}

std::size_t net::io::sendable::Itcp_gather::default_maximum_queued_bytes() noexcept
{
	return m_s_default_max_queued_bytes;
}

void net::io::sendable::Itcp_gather::default_maximum_queued_bytes_accepted(std::size_t _max) noexcept
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_max == m_s_default_max_queued_bytes_accepted);

	// check) 최소 256K로 설정한다.
	SATURATE_MIN(_max, (256 * 1024));

	// 1) 값을 설정한다.
	m_s_default_max_queued_bytes_accepted = _max;

	// log) 
	LOG_INFO << log_level_t::important() << "(info) set " << m_s_default_max_queued_bytes_accepted << " bytes as DEFAULT MAX SUSTAIN QUEUED BYTE (accepted) of net::io::sendable::Itcp_gather.";
}

std::size_t net::io::sendable::Itcp_gather::default_maximum_queued_bytes_accepted() noexcept
{
	return m_s_default_max_queued_bytes_accepted;
}

#if defined(_SYS_EPOLL_H)
//-----------------------------------------------------------------------------
// MSG_ZEROCOPY (the gather sendables : Ntcp_async_gather, Ntcp_async_gather_buffered)
//
//  - used only when the io_context of the socket is the io_uring backend (executor::Iio_context::backend()). epoll : copied as before.
//    (a zerocopy send adds a notification event per send. on epoll that is one more epoll_wait/epoll_ctl round per large message,
//     on io_uring the notifications ride the batched completions)
//  - SO_ZEROCOPY is set on the first large send of a socket (AF_INET/AF_INET6 only). a kernel without it => copied as before.
//  - a send with MSG_ZEROCOPY is counted by the kernel (0, 1, 2 ...). the pages of the buffer are pinned until the data is acked :
//    the owner of the buffer is held in m_queue_zerocopy with the count.
//  - the kernel notifies the completion of a range [ee_info, ee_data] on the error queue (recvmsg MSG_ERRQUEUE) : EPOLLERR.
//    socket::Ntcp::process_execute_epoll() calls process_complete_error_queue() first : the notifications release the owners,
//    anything else on the error queue (or no notification : a socket error) is an error as before.
//  - a send that fails (EAGAIN ...) is not counted by the kernel. a partial send is counted : the part sent is held.
//  - the sender holds the owner after send() returns : another thread can read the notification of that send (EPOLLERR) first.
//    the ids of a notification not held yet are remembered (m_zerocopy_id_notified_end) and their hold releases the owner at once.
//  - ENOBUFS : the kernel could not pin the pages or charge the notification. the pinned pages count against RLIMIT_MEMLOCK of the
//    user : shared by all its processes (8MB by default : two sockets with their socket buffers of large sends in flight reach it,
//    the other processes of the user pin too) and the notifications against optmem. MSG_ZEROCOPY is a hint : the send goes again as a
//    copy (_zerocopy_send) and zerocopy pauses on the socket for ZEROCOPY_PAUSE_AFTER_ENOBUFS (_zerocopy_pause). closed before.
//-----------------------------------------------------------------------------
bool net::io::sendable::Itcp_gather::_zerocopy_prepare(CGSOCKET _handle_socket) noexcept
{
	// check) TCP over IP only (a UDS does not support it)
	if (this->address_family() != AF_INET && this->address_family() != AF_INET6)
	{
		this->m_zerocopy_state.store(-1, std::memory_order_relaxed);
		return false;
	}

	// check) the io_uring backend only
	auto pio_context = this->io_context();

	if (pio_context.empty() || pio_context->backend() != executor::io_backend::io_uring)
	{
		this->m_zerocopy_state.store(-1, std::memory_order_relaxed);
		return false;
	}

	// 1) SO_ZEROCOPY (fails on a kernel without it)
	int enable = 1;

	if (::setsockopt(_handle_socket, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) != 0)
	{
		this->m_zerocopy_state.store(-1, std::memory_order_relaxed);
		return false;
	}

	// 2) enabled
	this->m_zerocopy_state.store(1, std::memory_order_relaxed);
	return true;
}

int net::io::sendable::Itcp_gather::_zerocopy_send_flags(CGSOCKET _handle_socket, std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) noexcept
{
	// check) enabled, large enough and an owner that keeps the buffer alive (not paused by an ENOBUFS)
	if (_bytes < s_zerocopy_threshold.load(std::memory_order_relaxed) || _powner.empty() || s_enable_zerocopy.load(std::memory_order_relaxed) == false || this->_is_zerocopy_paused())
		return 0;

	// check) available on this socket? (checked once)
	if (this->m_zerocopy_state.load(std::memory_order_relaxed) == 0)
		this->_zerocopy_prepare(_handle_socket);

	// return)
	return (this->m_zerocopy_state.load(std::memory_order_relaxed) > 0) ? MSG_ZEROCOPY : 0;
}

void net::io::sendable::Itcp_gather::_zerocopy_hold(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner) noexcept
{
	// - the kernel counts the zerocopy sends from 0 : the id is the count of this send
	std::lock_guard lock(this->m_lockable_zerocopy);

	// check) already notified (read by another thread before this hold) => the kernel is done with the buffer
	if (this->m_zerocopy_is_notified_early && static_cast<int32_t>(this->m_zerocopy_id_notified_end - this->m_zerocopy_id_next) > 0)
	{
		if (++this->m_zerocopy_id_next == this->m_zerocopy_id_notified_end)
			this->m_zerocopy_is_notified_early = false;

		_powner.reset();
		this->m_zerocopy_count_sent.fetch_add(1, std::memory_order_relaxed);
		return;
	}

	try
	{
		this->m_queue_zerocopy.push_back(ZEROCOPY_NODE{ this->m_zerocopy_id_next++, _buffer, std::move(_powner) });
	}
	catch (...)
	{
		// - out of memory : the owner is released now (the pages stay pinned by the kernel until the ack : the buffer must not be reused meanwhile. not expected)
		++this->m_zerocopy_id_next;
	}

	this->m_zerocopy_count_sent.fetch_add(1, std::memory_order_relaxed);
}

void net::io::sendable::Itcp_gather::_zerocopy_reset() noexcept
{
	// - the socket is closed or reset : the kernel does not notify any more (the pages are released with the socket)
	std::lock_guard lock(this->m_lockable_zerocopy);

	this->m_queue_zerocopy.clear();
	this->m_zerocopy_id_next = 0;
	this->m_zerocopy_id_notified_end = 0;
	this->m_zerocopy_is_notified_early = false;
	this->m_zerocopy_state.store(0, std::memory_order_relaxed);
	this->m_zerocopy_paused_until.store(0, std::memory_order_relaxed);
}

ssize_t net::io::sendable::Itcp_gather::_zerocopy_send(CGSOCKET _handle_socket, const void* _data, std::size_t _bytes, int& _flags) noexcept
{
	// 1) send
	auto result = ::send(_handle_socket, _data, _bytes, _flags);

	// check) a zerocopy send refused (ENOBUFS : the pages could not be pinned) => sent again as a copy (_flags : the flags of the send done)
	if (result < 0 && (_flags & MSG_ZEROCOPY) != 0 && errno == ENOBUFS)
	{
		this->_zerocopy_pause();

		_flags &= ~MSG_ZEROCOPY;
		result = ::send(_handle_socket, _data, _bytes, _flags);
	}

	// return)
	return result;
}

void net::io::sendable::Itcp_gather::_zerocopy_pause() noexcept
{
	// - the memory to pin is short (the limit of the user, the other processes ...) : copied for a while, tried again after
	const auto until = std::chrono::steady_clock::now() + ZEROCOPY_PAUSE_AFTER_ENOBUFS;

	this->m_zerocopy_paused_until.store(std::max<int64_t>(until.time_since_epoch().count(), 1), std::memory_order_relaxed);
	this->m_zerocopy_count_fallback.fetch_add(1, std::memory_order_relaxed);
}

bool net::io::sendable::Itcp_gather::process_complete_error_queue() noexcept
{
	// check) no zerocopy send on this socket => the EPOLLERR is an error
	RETURN_IF(this->m_zerocopy_state.load(std::memory_order_relaxed) <= 0, false);

	// 1) get socket handle
	const auto handle_socket = this->native_handle();

	// check)
	RETURN_IF(handle_socket == INVALID_SOCKET, false);

	// declare)
	bool is_notified = false;

	// 2) read the error queue
	for (;;)
	{
		::msghdr msg{};
		alignas(::cmsghdr) char buffer_control[128];

		msg.msg_control = buffer_control;
		msg.msg_controllen = sizeof(buffer_control);

		// - a notification has no data
		const auto result = ::recvmsg(handle_socket, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);

		// check) empty (EAGAIN) or an error
		if (result < 0)
			break;

		for (auto pcmsg = CMSG_FIRSTHDR(&msg); pcmsg != nullptr; pcmsg = CMSG_NXTHDR(&msg, pcmsg))
		{
			// check) IP_RECVERR (SOL_IP) / IPV6_RECVERR (SOL_IPV6)
			const bool is_error_v4 = (pcmsg->cmsg_level == SOL_IP && pcmsg->cmsg_type == IP_RECVERR);
			const bool is_error_v6 = (pcmsg->cmsg_level == SOL_IPV6 && pcmsg->cmsg_type == IPV6_RECVERR);

			CONTINUE_IF(is_error_v4 == false && is_error_v6 == false);

			// - get extended error
			const auto* perror = reinterpret_cast<const ::sock_extended_err*>(CMSG_DATA(pcmsg));

			// check) not a zerocopy notification => an error of the socket (ICMP ...) : the caller closes
			RETURN_IF(perror->ee_origin != SO_EE_ORIGIN_ZEROCOPY, false);

			// 3) the range [ee_info, ee_data] is done : release the owners (the ids wrap at 2^32 : compared by the distance)
			const uint32_t id_begin = perror->ee_info;
			const uint32_t length_range = perror->ee_data - perror->ee_info;

			{
				std::lock_guard lock(this->m_lockable_zerocopy);

				for (auto iter = this->m_queue_zerocopy.begin(); iter != this->m_queue_zerocopy.end(); )
				{
					if (static_cast<uint32_t>(iter->id - id_begin) <= length_range)
						iter = this->m_queue_zerocopy.erase(iter);
					else
						++iter;
				}

				// - the range reaches a send not held yet (its sender is between send() and _zerocopy_hold()) : its hold releases the owner
				if (static_cast<int32_t>(perror->ee_data - this->m_zerocopy_id_next) >= 0)
				{
					this->m_zerocopy_id_notified_end = perror->ee_data + 1;
					this->m_zerocopy_is_notified_early = true;
				}
			}

			is_notified = true;
		}
	}

	// check) no notification => an error of the socket (EPOLLERR without a notification) : the caller closes
	RETURN_IF(is_notified == false, false);

	// 4) notifications and an error at the same time? (SO_ERROR : the caller closes)
	int error_socket = 0;
	socklen_t length_error = sizeof(error_socket);

	if (::getsockopt(handle_socket, SOL_SOCKET, SO_ERROR, &error_socket, &length_error) == 0 && error_socket != 0)
	{
		this->m_last_socket_error = error_socket;
		return false;
	}

	// return) handled
	return true;
}
#endif
