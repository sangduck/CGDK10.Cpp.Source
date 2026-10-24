#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include <climits>
#include <cstdlib>
#include <algorithm>

// - the CQE flags (linux/io_uring.h : IORING_CQE_F_MORE, IORING_CQE_F_NOTIF)
static constexpr std::size_t CGDK_CQE_FLAG_MORE = 1u << 1;
static constexpr std::size_t CGDK_CQE_FLAG_NOTIF = 1u << 3;

#if defined(IOV_MAX)
	static constexpr std::size_t CGDK_SEND_OP_MAX_IOV = IOV_MAX;
#else
	static constexpr std::size_t CGDK_SEND_OP_MAX_IOV = 1024;
#endif

using namespace CGDK;

//-----------------------------------------------------------------------------
// io_uring completion (executor::io_backend::io_uring_completion) : the send of Ntcp_async_gather with the SEND ops
//
//  - the caller thread sends directly (send/sendmsg MSG_DONTWAIT) like epoll. optional (CGDK_IO_URING_SEND_OP=1,
//    enable_send_on_owner_ring()) : the thread that owns the ring of the socket (the worker that runs its receive completions : the
//    replies of on_message) sends with a SENDMSG op on that ring (_is_send_on_owner_ring) : written into its SQ and submitted with
//    the enter it makes after its batch anyway (no send syscall). off by default : the reply waits for that enter and costs a CQE
//    (-8% with a window of 16 on mistral, neutral with one message in flight). the other threads always send directly (an op of
//    theirs would be staged for the owner and cost a wake).
//  - the op replaces the EPOLLOUT wait too : the socket buffer full (EWOULDBLOCK, a partial send, EINPROGRESS of a TCP Fast Open
//    connect) posts a SENDMSG op of the rest (m_queue_sending from m_index_sending : up to IOV_MAX buffers) and the op is the sending
//    thread (m_flag_sending) till it completes. its completion consumes the bytes sent and continues with _process_send_queued()
//    (direct sends again, or another op on the owner. the buffer full again => another op).
//  - a single large buffer (>= zerocopy_threshold(), AF_INET/AF_INET6, with an owner) goes as a SEND_ZC op, even when the socket
//    is writable (the direct send is skipped : process_sendable, _process_send_queued). the first CQE (F_MORE) gives the bytes,
//    the notification CQE (F_NOTIF) says the kernel is done with the buffer : the owner is held in the executable till then.
//    no SO_ZEROCOPY / error queue (the readiness zerocopy, _zerocopy_send_flags, is not used on a completion socket).
//    -ENOBUFS (the pages could not be pinned : RLIMIT_MEMLOCK of the user) => the same buffers again as a SENDMSG, zerocopy paused.
//  - the executable holds the socket (add_ref) while an op is in flight. a socket reset while an op is in flight
//    (process_close_socket cancels the op, process_dispose_socket clears the queues) moves m_queue_sending to the executable
//    (m_queue_hold) : the buffers stay till the kernel is done with the op.
//  - a failure of the op (-ECONNRESET, -EPIPE ...) closes the socket (DISCONNECT_REASON_FAIL) like the epoll failure path.
//    -ECANCELED : closed by process_close_socket() meanwhile (the cleanup only), or the exit of the thread that submitted the op
//    (the kernel cancels the requests of an exiting thread) : posted again when the handle is still open.
//-----------------------------------------------------------------------------
intptr_t net::io::sendable::Ntcp_async_gather::executable_sending::process_execute(intptr_t _result, std::size_t _param)
{
	// 1) the sendable (held by the post : released at the end of the op)
	auto* psendable = this->m_psendable;

	// check)
	RETURN_IF(psendable == nullptr, 0);

	// 2) the result CQE (not the notification of a zerocopy send)
	if ((_param & CGDK_CQE_FLAG_NOTIF) == 0)
	{
		if (psendable->_process_complete_send_op(_result, _param))
			this->m_is_reposted = true;

		// - more CQEs follow (the notification) : the op is not done yet
		RETURN_IF((_param & CGDK_CQE_FLAG_MORE) != 0, 0);
	}

	// 3) the final CQE of the op : the kernel is done with the buffers
	//    - the owner of the zerocopy buffer (the notifications come in the order of the sends)
	//    - the queue of a socket reset while the op was in flight (when no op is in flight any more : a repost shares the queue)
	QUEUE_SEND queue_release;
	const bool is_reposted = this->m_is_reposted;
	this->m_is_reposted = false;

	if (this->m_owners_zerocopy.empty() == false)
		this->m_owners_zerocopy.erase(this->m_owners_zerocopy.begin());

	SCOPED_LOCK(psendable->m_lockable_sending)
	{
		if (--psendable->m_count_sending_op == 0)
			queue_release.swap(this->m_queue_hold);
	}

	queue_release.clear();

	// statistics) the wait for the socket buffer is over (counted once per wait : a repost continues it)
	if (is_reposted == false)
		psendable->statistics_on_decrease_async_sending();

	// 4) the op is done (the reference of the post)
	psendable->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
	psendable->release();

	// return)
	return 0;
}

namespace
{
	// - the sends of the ring owner as ops : OFF by default (CGDK_IO_URING_SEND_OP=1 or enable_send_on_owner_ring() : on). measured on
	//   mistral (kernel 7.0, 8 threads) : neutral with one message in flight per connection, -8% with a window of 16 (the reply waits
	//   for the next enter and costs a CQE of its own : more than the send syscall it saves)
	std::atomic<int> s_send_on_owner_ring{ -1 };

	bool _read_send_on_owner_ring() noexcept
	{
		int value = s_send_on_owner_ring.load(std::memory_order_relaxed);

		if (value < 0)
		{
			const char* penv = std::getenv("CGDK_IO_URING_SEND_OP");
			value = (penv != nullptr && (penv[0] == '1' || penv[0] == 't' || penv[0] == 'T' || penv[0] == 'y' || penv[0] == 'Y')) ? 1 : 0;
			s_send_on_owner_ring.store(value, std::memory_order_relaxed);
		}

		return value != 0;
	}
}

void net::io::sendable::Ntcp_async_gather::enable_send_on_owner_ring(bool _enable) noexcept
{
	s_send_on_owner_ring.store(_enable ? 1 : 0, std::memory_order_relaxed);
}

bool net::io::sendable::Ntcp_async_gather::is_send_on_owner_ring_enabled() noexcept
{
	return _read_send_on_owner_ring();
}

bool net::io::sendable::Ntcp_async_gather::_is_send_on_owner_ring(CGSOCKET _handle_socket) const noexcept
{
	//-----------------------------------------------------------------
	// the sends of the ring owner go as ops (the worker that owns the ring of this socket : the receive completions of this socket run on
	// it, so the replies sent in on_message do)
	//
	//  - the op is written into the SQ of the ring and submitted with the io_uring_enter the owner makes after its batch anyway : the
	//    sends of a batch cost no syscall (a direct send is a syscall per flush). the completion comes in the next batch.
	//  - another thread sends directly as before (an op staged for the owner would cost a wake : a nop and a syscall)
	//-----------------------------------------------------------------
	RETURN_IF(this->is_io_completion() == false || _handle_socket == INVALID_SOCKET || _read_send_on_owner_ring() == false, false);

	auto* pio_context = this->m_pio_context.get();

	return pio_context != nullptr && pio_context->is_ring_owner(static_cast<CGHANDLE>(_handle_socket));
}

bool net::io::sendable::Ntcp_async_gather::_is_zerocopy_op(std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) const noexcept
{
	// - a large buffer with an owner that keeps it alive, TCP over IP only (a UDS does not support it), not paused by an ENOBUFS
	RETURN_IF(_bytes < s_zerocopy_threshold.load(std::memory_order_relaxed) || _powner.empty() || s_enable_zerocopy.load(std::memory_order_relaxed) == false || this->_is_zerocopy_paused(), false);

	return this->address_family() == AF_INET || this->address_family() == AF_INET6;
}

bool net::io::sendable::Ntcp_async_gather::_process_post_send_op() noexcept
{
	//-----------------------------------------------------------------
	// posts the SEND op of m_queue_sending from m_index_sending (under m_lockable_sending : the handle is read and the op is
	// submitted under it. process_close_socket() swaps the handle under the same lock and cancels the ops after)
	//
	//  - false : the socket is closed or the submit failed (the queue is cleared, m_flag_sending is reset, the socket is closed
	//    by a post : process_close_socket() takes this lock)
	//-----------------------------------------------------------------
	// check)
	CGASSERT_ERROR(this->m_flag_sending == true);
	CGASSERT_ERROR(this->m_index_sending < this->m_queue_sending.array_node.size());

	// 1) the handle (closed meanwhile => the queued sends are dropped)
	const auto handle_socket = this->native_handle();
	auto* pio_context = this->m_pio_context.get();

	if (handle_socket == INVALID_SOCKET || pio_context == nullptr || this->m_index_sending >= this->m_queue_sending.array_node.size())
	{
		this->_process_clear_send_buffers();
		this->m_flag_sending = false;
		return false;
	}

	// 2) the executable (pooled : allocated at the first use of this socket)
	if (this->m_pexecutable_sending.empty())
	{
		try
		{
			this->m_pexecutable_sending = make_object<executable_sending>();
		}
		catch (...)
		{
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
			return false;
		}
	}

	auto* pexecutable = this->m_pexecutable_sending.get();
	pexecutable->m_psendable = this;

	// 3) the buffers (up to IOV_MAX)
	const auto count_buffer = std::min<std::size_t>(this->m_queue_sending.array_node.size() - this->m_index_sending, CGDK_SEND_OP_MAX_IOV);
	auto* pbuffer_send = this->m_queue_sending.array_buffer.data() + this->m_index_sending;
	auto& node_first = this->m_queue_sending.array_node[this->m_index_sending];
	const bool is_zerocopy = (count_buffer == 1) && this->_is_zerocopy_op(pbuffer_send->iov_len, node_first.powner);

	// 4) post (the socket is held by the op : released at its end)
	this->add_ref();
	this->m_count_io_inflight.fetch_add(1, std::memory_order_acq_rel);
	++this->m_count_sending_op;

	bool result;

	if (is_zerocopy)
	{
		try
		{
			pexecutable->m_owners_zerocopy.push_back(node_first.powner);
		}
		catch (...)
		{
			--this->m_count_sending_op;
			this->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
			this->release();
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
			return false;
		}

		result = pio_context->submit_send(static_cast<CGHANDLE>(handle_socket), pbuffer_send->iov_base, pbuffer_send->iov_len, MSG_NOSIGNAL, pexecutable, true);

		if (result)
			this->m_zerocopy_count_sent.fetch_add(1, std::memory_order_relaxed);
		else
			pexecutable->m_owners_zerocopy.pop_back();
	}
	else
	{
		pexecutable->m_msg = ::msghdr{};
		pexecutable->m_msg.msg_iov = pbuffer_send;
		pexecutable->m_msg.msg_iovlen = count_buffer;

		result = pio_context->submit_sendmsg(static_cast<CGHANDLE>(handle_socket), &pexecutable->m_msg, MSG_NOSIGNAL, pexecutable, false);
	}

	// check) the submit failed => the socket is closed (by a post : process_close_socket() takes the sending lock held here)
	if (result == false)
	{
		--this->m_count_sending_op;
		this->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
		this->release();

		this->m_last_socket_error = api::get_last_error();
		this->statistics_on_error_send();
		this->_process_clear_send_buffers();
		this->m_flag_sending = false;

		try
		{
			system_executor::post(make_executable([psocket = object_ptr<net::io::Isocket>(this), handle_socket]()
			{
				if (psocket->native_handle() == handle_socket)
					psocket->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			}));
		}
		catch (...)
		{
		}

		return false;
	}

	// statistics)
	this->statistics_on_send_async();

	// return)
	return true;
}

bool net::io::sendable::Ntcp_async_gather::_process_complete_send_op(intptr_t _result, std::size_t /*_flags*/) noexcept
{
	//-----------------------------------------------------------------
	// the result CQE of the SEND op (the op is the sending thread : m_flag_sending)
	//
	//  - the bytes sent are consumed from m_queue_sending and the sending continues (_process_send_queued : direct sends).
	//    the socket buffer full again => another op (true : reposted)
	//  - -EINPROGRESS (the first send of a TCP Fast Open connect without a cookie : the SYN is sent) => posted again (the op waits
	//    for the handshake)
	//  - a failure => the socket is closed (DISCONNECT_REASON_FAIL). -ECANCELED / a closed handle => the cleanup only
	//-----------------------------------------------------------------
	// 1) the handle (under the lock : closed meanwhile => the queue is dropped. process_close_socket() cancelled this op)
	CGSOCKET handle_socket;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		handle_socket = this->native_handle();

		if (handle_socket == INVALID_SOCKET)
		{
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
			return false;
		}

		// 2) the op again : TCP Fast Open (-EINPROGRESS : the SYN went, the data waits for the handshake), -ECANCELED with the handle
		//    still open (the thread that submitted the op exited : the kernel cancels its requests)
		if (_result == -EINPROGRESS || _result == -ECANCELED)
			return this->_process_post_send_op();

		//    -ENOBUFS of a SEND_ZC op (the pages could not be pinned : RLIMIT_MEMLOCK of the user ...) : zerocopy pauses and the same
		//    buffers go again as a SENDMSG (copied). an ENOBUFS while paused (a SENDMSG) is a failure
		if (_result == -ENOBUFS && this->_is_zerocopy_paused() == false)
		{
			this->_zerocopy_pause();
			return this->_process_post_send_op();
		}

		// - the wait for the socket buffer is over (like process_recover_sendable : parked again below if the buffer is full again.
		//   left as it was, sending_queued_bytes_now() kept the bytes of the wait after all of them were sent)
		this->m_count_sending_parked = 0;
		this->m_bytes_sending_parked = 0;
	}

	// 3) a failure (0 bytes : the socket is not writable any more)
	if (_result <= 0)
	{
		// - set last error
		this->m_last_socket_error = (_result < 0) ? static_cast<int>(-_result) : EPIPE;

		// statistics)
		this->statistics_on_error_send();

		// - closesocket (abortive close)
		this->process_close_socket(DISCONNECT_REASON_FAIL);

		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
		}

		return false;
	}

	// 4) the bytes sent (the op covered up to IOV_MAX buffers from m_index_sending)
	std::size_t bytes_completed = 0;
	std::size_t count_completed = 0;
	auto bytes_left = static_cast<std::size_t>(_result);
	const auto index_end = this->m_index_sending + std::min<std::size_t>(this->m_queue_sending.array_node.size() - this->m_index_sending, CGDK_SEND_OP_MAX_IOV);

	for (; this->m_index_sending < index_end; ++this->m_index_sending)
	{
		auto& buffer_now = this->m_queue_sending.array_buffer[this->m_index_sending];

		// - a part of it => the rest from there
		if (bytes_left < buffer_now.iov_len)
		{
			buffer_now.iov_base = static_cast<char*>(buffer_now.iov_base) + bytes_left;
			buffer_now.iov_len -= bytes_left;
			break;
		}

		// - all of it
		bytes_left -= buffer_now.iov_len;

		const auto& node_now = this->m_queue_sending.array_node[this->m_index_sending];
		bytes_completed += node_now.bytes_data;
		count_completed += node_now.count_message;
	}

	this->m_queue_sending.bytes_data -= std::min<std::size_t>(this->m_queue_sending.bytes_data, static_cast<std::size_t>(_result));

	// 5) continue (direct sends till the queues are empty. the socket buffer full again => another op)
	const auto result = this->_process_send_queued(handle_socket, bytes_completed, count_completed);

	// check) all sent (m_flag_sending is reset) or failed (the socket is closed)
	RETURN_IF(result != EPOLLOUT, false);

	SCOPED_LOCK(this->m_lockable_sending)
	{
		return this->_process_post_send_op();
	}

	// return) (not reached)
	return false;
}

#endif
