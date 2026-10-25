#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include <climits>
#include <algorithm>

// - the CQE flags (linux/io_uring.h : IORING_CQE_F_MORE, IORING_CQE_F_NOTIF)
static constexpr std::size_t CGDK_CQE_FLAG_MORE = 1u << 1;
static constexpr std::size_t CGDK_CQE_FLAG_NOTIF = 1u << 3;

// - the res of the notification CQE of a SEND_ZC with IORING_SEND_ZC_REPORT_USAGE (6.2+) : IORING_NOTIF_USAGE_ZC_COPIED, the kernel copied the data
static constexpr uint32_t CGDK_NOTIF_USAGE_ZC_COPIED = 1u << 31;

#if defined(IOV_MAX)
	static constexpr std::size_t CGDK_SEND_OP_MAX_IOV = IOV_MAX;
#else
	static constexpr std::size_t CGDK_SEND_OP_MAX_IOV = 1024;
#endif

using namespace CGDK;

//-----------------------------------------------------------------------------
// io_uring completion (executor::io_backend::io_uring_completion) : the send of Ntcp_async_gather_buffered with the SEND ops
//
//  - the sending thread sends directly (send/sendmsg MSG_DONTWAIT) like epoll : the small messages are copied into the gather
//    buffers of m_queue_pending (QUEUE_SEND::push_back) and the queue is sent with sendmsg by the sending thread (m_flag_sending).
//  - the op replaces the EPOLLOUT wait : the socket buffer full (EWOULDBLOCK, a partial send, EINPROGRESS of a TCP Fast Open
//    connect) puts the rest back at the front of m_queue_pending (as epoll does) and posts a SENDMSG op of the front of the queue :
//    the nodes (the rest and what was queued behind it, up to IOV_MAX) move into m_queue_sending, which only the op uses till it
//    completes (the iovecs point into them). the op is the sending thread meanwhile. its completion consumes the bytes sent : the
//    rest of the op is posted again, else the sending continues with _process_send_pending() (direct sends again, the buffer full
//    again => another op).
//  - the send queue limit does not count the nodes of the op (the batch being sent, like the WSASend in progress on windows and the
//    rest waiting for EPOLLOUT on epoll). sending_queued_bytes_now() includes them (m_bytes_sending_op).
//  - a single large buffer of the user (>= zerocopy_threshold(), AF_INET/AF_INET6, not a gather buffer) goes as a SEND_ZC op (the
//    direct send is skipped : process_sendable_async, _process_send_pending). the notification CQE (F_NOTIF) says the kernel is
//    done with the buffer : the owner is held in the executable till then. no SO_ZEROCOPY / error queue on a completion socket.
//  - the executable holds the socket (add_ref) while an op is in flight. a socket reset while an op is in flight
//    (process_close_socket cancels the op, process_reset_sendable clears the queues) moves m_queue_sending to the executable
//    (m_queue_hold) : the buffers stay till the kernel is done with the op.
//  - a failure of the op (-ECONNRESET, -EPIPE ...) closes the socket (DISCONNECT_REASON_FAIL) like the epoll failure path.
//    -ECANCELED : closed by process_close_socket() meanwhile (the cleanup only), or the exit of the thread that submitted the op :
//    posted again when the handle is still open. -ENOBUFS of a SEND_ZC op (RLIMIT_MEMLOCK) : the same buffers again as a SENDMSG.
//-----------------------------------------------------------------------------
intptr_t net::io::sendable::Ntcp_async_gather_buffered::executable_sending::process_execute(intptr_t _result, std::size_t _param)
{
	// 1) the sendable (held by the post : released at the end of the op)
	auto* psendable = this->m_psendable;

	// check)
	RETURN_IF(psendable == nullptr, 0);

	// declare) the owner released out of the lock
	object_ptr<Ireferenceable> powner_release;

	// 2) the result CQE (not the notification of a zerocopy send)
	if ((_param & CGDK_CQE_FLAG_NOTIF) == 0)
	{
		// - the entry of this op in m_owners_zerocopy (the last one : one op waits for its result at a time). no notification
		//   follows (a SENDMSG, a SEND_ZC that ended without F_MORE) => removed now. a SEND_ZC with F_MORE keeps its owner till
		//   its notification. before the completion below : it may post the next op (another entry)
		//   a SEND_ZC that failed with F_MORE (-ENOBUFS / -ENOMEM : sent again as a copy) : its notification still comes, reported copied
		//   (nothing was sent from the pages) => not counted (as copied : ZEROCOPY_COPIED_TO_STOP of them turned zerocopy off for good. as sent)
		SCOPED_LOCK(psendable->m_lockable_sending)
		{
			if (this->m_owners_zerocopy.empty() == false)
			{
				if ((_param & CGDK_CQE_FLAG_MORE) == 0)
				{
					powner_release = std::move(this->m_owners_zerocopy.back().powner);
					this->m_owners_zerocopy.pop_back();
				}
				else if (_result < 0 && this->m_owners_zerocopy.back().is_failed == false)
				{
					this->m_owners_zerocopy.back().is_failed = true;
					psendable->m_zerocopy_count_sent.fetch_sub(1, std::memory_order_relaxed);
				}
			}
		}

		if (psendable->_process_complete_send_op(_result))
			this->m_is_reposted = true;

		// - more CQEs follow (the notification) : the op is not done yet
		RETURN_IF((_param & CGDK_CQE_FLAG_MORE) != 0, 0);
	}

	// 3) the final CQE of the op : the kernel is done with the buffers
	//    - the notification : the owner of the oldest zerocopy buffer (the notifications come in the order of the sends. only the
	//      SEND_ZC ops that ended with F_MORE are left in m_owners_zerocopy)
	//    - the nodes of a socket reset while the op was in flight (when no op is in flight any more : a repost shares the executable)
	container_t<QUEUE_SEND::NODE> queue_release;
	const bool is_reposted = this->m_is_reposted;
	this->m_is_reposted = false;

	bool is_failed = false;

	SCOPED_LOCK(psendable->m_lockable_sending)
	{
		if ((_param & CGDK_CQE_FLAG_NOTIF) != 0 && this->m_owners_zerocopy.empty() == false)
		{
			is_failed = this->m_owners_zerocopy.front().is_failed;
			powner_release = std::move(this->m_owners_zerocopy.front().powner);
			this->m_owners_zerocopy.erase(this->m_owners_zerocopy.begin());
		}

		if (--psendable->m_count_sending_op == 0)
			queue_release.swap(this->m_queue_hold);
	}

	queue_release.clear();
	powner_release.reset();

	// - the notification of a zerocopy send : copied by the kernel (a peer on this host : loopback, an address of this host, a local container.
	//   a NIC without scatter-gather) zerocopy only adds the pinning and the notification. copied in a row, it stops on this socket till it is reset.
	//   the notification of a SEND_ZC that failed (is_failed) : not counted
	if ((_param & CGDK_CQE_FLAG_NOTIF) != 0 && is_failed == false)
		psendable->_zerocopy_on_notified(1, (static_cast<uint32_t>(_result) & CGDK_NOTIF_USAGE_ZC_COPIED) != 0);

	// statistics) the wait for the socket buffer is over (counted once per wait : a repost continues it)
	if (is_reposted == false)
		psendable->statistics_on_decrease_async_sending();

	// 4) the op is done (the reference of the post)
	psendable->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
	psendable->release();

	// return)
	return 0;
}

bool net::io::sendable::Ntcp_async_gather_buffered::_is_zerocopy_op(std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) const noexcept
{
	// - a large buffer with an owner that keeps it alive, TCP over IP only (a UDS does not support it), not paused by an ENOBUFS
	RETURN_IF(_bytes < s_zerocopy_threshold.load(std::memory_order_relaxed) || _powner.empty() || s_enable_zerocopy.load(std::memory_order_relaxed) == false || this->_is_zerocopy_paused() || this->is_zerocopy_copied(), false);

	return this->address_family() == AF_INET || this->address_family() == AF_INET6;
}

bool net::io::sendable::Ntcp_async_gather_buffered::_process_post_send_op() noexcept
{
	//-----------------------------------------------------------------
	// posts the SEND op (under m_lockable_sending : the handle is read and the op is submitted under it. process_close_socket()
	// swaps the handle under the same lock and cancels the ops after)
	//
	//  - m_queue_sending empty : the front of m_queue_pending moves into it (the rest put back by the sending thread and what was
	//    queued behind it : up to IOV_MAX nodes, or the single large buffer of a zerocopy send). not empty : the rest of the last op
	//  - false : the socket is closed or the submit failed (the queues are cleared, m_flag_sending is reset, the socket is closed
	//    by a post : process_close_socket() takes this lock)
	//-----------------------------------------------------------------
	// check)
	CGASSERT_ERROR(this->m_flag_sending == true);

	// 1) the handle (closed meanwhile => the queued sends are dropped)
	const auto handle_socket = this->native_handle();
	auto* pio_context = this->m_pio_context.get();

	if (handle_socket == INVALID_SOCKET || pio_context == nullptr || (this->m_queue_sending.array_node.empty() && this->m_queue_pending.array_node.empty()))
	{
		this->_process_clear_send_buffers();
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
			return false;
		}
	}

	auto* pexecutable = this->m_pexecutable_sending.get();
	pexecutable->m_psendable = this;

	// 3) the nodes of the op (the front of m_queue_pending : not counted for the send queue limit any more)
	if (this->m_queue_sending.array_node.empty())
	{
		const auto& node_front = this->m_queue_pending.array_node.front();
		const bool is_zerocopy_front = (node_front.bytes_remained == 0) && this->_is_zerocopy_op(node_front.buf_send.size(), node_front.powner);
		const std::size_t count_max = is_zerocopy_front ? 1 : CGDK_SEND_OP_MAX_IOV;

		std::size_t count_moved = 0;
		std::size_t bytes_moved = 0;
		auto iter_end = this->m_queue_pending.array_node.begin();

		for (; iter_end != this->m_queue_pending.array_node.end() && count_moved < count_max; ++iter_end, ++count_moved)
			bytes_moved += iter_end->buf_send.size();

		this->m_queue_sending.array_node.splice(this->m_queue_sending.array_node.end(), this->m_queue_pending.array_node, this->m_queue_pending.array_node.begin(), iter_end);
		this->m_queue_pending.bytes_data -= std::min<std::size_t>(this->m_queue_pending.bytes_data, bytes_moved);
		this->m_bytes_sending_op = bytes_moved;

		// - the rest waiting (parked) was the front : moved with the op
		if (this->m_count_sending_parked <= count_moved)
		{
			this->m_count_sending_parked = 0;
			this->m_bytes_sending_parked = 0;
		}
		else
		{
			this->m_count_sending_parked -= count_moved;
			this->m_bytes_sending_parked -= std::min<std::size_t>(this->m_bytes_sending_parked, bytes_moved);
		}
	}

	// 4) the iovecs (in the executable : stable while the op is in flight)
	try
	{
		pexecutable->m_iovec.clear();
		pexecutable->m_iovec.reserve(this->m_queue_sending.array_node.size());

		for (const auto& iter : this->m_queue_sending.array_node)
			pexecutable->m_iovec.push_back(::iovec{ iter.buf_send.data(), iter.buf_send.size() });
	}
	catch (...)
	{
		this->_process_clear_send_buffers();
		return false;
	}

	const auto& node_first = this->m_queue_sending.array_node.front();
	const bool is_zerocopy = (pexecutable->m_iovec.size() == 1) && (node_first.bytes_remained == 0) && this->_is_zerocopy_op(pexecutable->m_iovec[0].iov_len, node_first.powner);

	// 5) post (the socket is held by the op : released at its end)
	this->add_ref();
	this->m_count_io_inflight.fetch_add(1, std::memory_order_acq_rel);
	++this->m_count_sending_op;

	//    - an entry per op in m_owners_zerocopy (the owner of a SEND_ZC, empty for a SENDMSG) : the result CQE without F_MORE removes
	//      it, a SEND_ZC with F_MORE keeps it till its notification (a repost of the same executable may complete before that)
	try
	{
		pexecutable->m_owners_zerocopy.push_back(executable_sending::ZEROCOPY_OP{ is_zerocopy ? node_first.powner : object_ptr<Ireferenceable>(), false });
	}
	catch (...)
	{
		--this->m_count_sending_op;
		this->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
		this->release();
		this->_process_clear_send_buffers();
		return false;
	}

	bool result;

	if (is_zerocopy)
	{
		result = pio_context->submit_send(static_cast<CGHANDLE>(handle_socket), pexecutable->m_iovec[0].iov_base, pexecutable->m_iovec[0].iov_len, MSG_NOSIGNAL, pexecutable, true);

		if (result)
			this->m_zerocopy_count_sent.fetch_add(1, std::memory_order_relaxed);
	}
	else
	{
		pexecutable->m_msg = ::msghdr{};
		pexecutable->m_msg.msg_iov = pexecutable->m_iovec.data();
		pexecutable->m_msg.msg_iovlen = pexecutable->m_iovec.size();

		result = pio_context->submit_sendmsg(static_cast<CGHANDLE>(handle_socket), &pexecutable->m_msg, MSG_NOSIGNAL, pexecutable, false);
	}

	// check) the submit failed => the socket is closed (by a post : process_close_socket() takes the sending lock held here)
	if (result == false)
	{
		pexecutable->m_owners_zerocopy.pop_back();
		--this->m_count_sending_op;
		this->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
		this->release();

		this->m_last_socket_error = api::get_last_error();
		this->statistics_on_error_send();
		this->_process_clear_send_buffers();

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

bool net::io::sendable::Ntcp_async_gather_buffered::_process_complete_send_op(intptr_t _result) noexcept
{
	//-----------------------------------------------------------------
	// the result CQE of the SEND op (the op is the sending thread : m_flag_sending)
	//
	//  - the bytes sent are consumed from m_queue_sending. the rest => the op again. all of it => the sending continues
	//    (_process_send_pending : direct sends. the socket buffer full again => another op) (true : reposted)
	//  - -EINPROGRESS (the first send of a TCP Fast Open connect without a cookie : the SYN is sent) => posted again (the op waits
	//    for the handshake)
	//  - a failure => the socket is closed (DISCONNECT_REASON_FAIL). -ECANCELED / a closed handle => the cleanup only
	//-----------------------------------------------------------------
	// declare)
	CGSOCKET handle_socket;
	std::size_t bytes_completed = 0;
	std::size_t count_completed = 0;
	container_t<QUEUE_SEND::NODE> queue_sent;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 1) the handle (closed meanwhile => the queues are dropped. process_close_socket() cancelled this op)
		handle_socket = this->native_handle();

		if (handle_socket == INVALID_SOCKET)
		{
			this->_process_clear_send_buffers();
			return false;
		}

		// 2) the op again : TCP Fast Open (-EINPROGRESS : the SYN went, the data waits for the handshake), -ECANCELED with the handle
		//    still open (the thread that submitted the op exited : the kernel cancels its requests)
		if (_result == -EINPROGRESS || _result == -ECANCELED)
			return this->_process_post_send_op();

		//    -ENOBUFS of a SEND_ZC op (the pages could not be pinned : RLIMIT_MEMLOCK of the user ...) : zerocopy pauses and the same
		//    buffers go again as a SENDMSG (copied). an ENOBUFS while paused (a SENDMSG) is a failure
		//    -ENOMEM the same : io_uring charges the pinned pages of a SEND_ZC to RLIMIT_MEMLOCK itself (__io_account_mem) and fails
		//    the op with -ENOMEM when it runs out (the MSG_ZEROCOPY of a socket : -ENOBUFS). closed before (errno 12)
		if ((_result == -ENOBUFS || _result == -ENOMEM) && this->_is_zerocopy_paused() == false)
		{
			this->_zerocopy_pause();
			return this->_process_post_send_op();
		}

		// check) a failure : below (out of the lock)
		if (_result > 0)
		{
			// 3) the bytes sent (the nodes sent are released out of the lock)
			auto bytes_left = static_cast<std::size_t>(_result);

			while (bytes_left != 0 && this->m_queue_sending.array_node.empty() == false)
			{
				auto& node_front = this->m_queue_sending.array_node.front();
				const auto bytes_node = node_front.buf_send.size();

				// - a part of it => the rest from there
				if (bytes_left < bytes_node)
				{
					node_front.buf_send += offset(static_cast<int64_t>(bytes_left));
					break;
				}

				// - all of it
				bytes_left -= bytes_node;
				bytes_completed += node_front.bytes_data;
				count_completed += node_front.count_message;
				queue_sent.splice(queue_sent.end(), this->m_queue_sending.array_node, this->m_queue_sending.array_node.begin());
			}

			this->m_bytes_sending_op -= std::min<std::size_t>(this->m_bytes_sending_op, static_cast<std::size_t>(_result));

			// 4) the rest of the op => the op again
			if (this->m_queue_sending.array_node.empty() == false)
			{
				const bool is_reposted = this->_process_post_send_op();

				queue_sent.clear();

				if (bytes_completed != 0 || count_completed != 0)
					this->process_complete_sendable(0, bytes_completed, count_completed);

				return is_reposted;
			}
		}
	}

	// 5) a failure (0 bytes : the socket is not writable any more)
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
		}

		return false;
	}

	// 6) continue (direct sends till m_queue_pending is empty. the socket buffer full again => another op)
	queue_sent.clear();

	const auto result = this->_process_send_pending(handle_socket, bytes_completed, count_completed);

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
