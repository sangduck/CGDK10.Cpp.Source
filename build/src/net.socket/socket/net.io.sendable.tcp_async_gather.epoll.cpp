#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include <climits>
#include <algorithm>

// - sendmsg의 최대 buffer 수
#if defined(IOV_MAX)
	static constexpr std::size_t CGDK_SEND_MAX_IOV = IOV_MAX;
#else
	static constexpr std::size_t CGDK_SEND_MAX_IOV = 1024;
#endif

using namespace CGDK;

// - send cork : 이 thread가 처음 모은 시간 (가장 오래 기다린 송신)
static thread_local chrono::tick::time_point t_tick_corked;

// - send cork : 오래 기다렸는지는 시각을 읽어 확인한다. (작은 message를 많이 보낼 때 송신마다 읽으면 그 비용이 크다)
//   확인할 때마다 직전 간격의 송신 속도로 다음 간격(송신 수)을 정한다 : SEND_CORK_DURATION/8 (남은 시간이 더 짧으면 남은 시간) 동안의 송신 수.
//   (빠른 송신은 SEND_CORK_CHECK_MAX번마다, 오래 걸리는 처리 사이의 송신은 매번 확인한다. 모으기 시작하면 다음 송신부터 확인한다)
static thread_local chrono::tick::time_point t_tick_cork_checked;
static thread_local std::size_t t_count_cork_interval = 1;	// - 확인 간격 (송신 수)
static thread_local std::size_t t_count_cork_check = 1;		// - 다음 확인까지 남은 송신 수

static void _cork_begin() noexcept
{
	const auto tick_now = chrono::tick::now();

	t_tick_corked = tick_now;
	t_tick_cork_checked = tick_now;
	t_count_cork_interval = 1;
	t_count_cork_check = 1;
}

static bool _is_cork_expired(chrono::tick::duration _duration, std::size_t _count_check_max) noexcept
{
	// check) 확인할 차례가 아니다.
	if (t_count_cork_check > 1)
	{
		--t_count_cork_check;
		return false;
	}

	// 1) 가장 먼저 모은 것이 기다린 시간
	const auto tick_now = chrono::tick::now();
	const auto tick_waited = tick_now - t_tick_corked;

	// check) 오래 기다렸다.
	if (tick_waited >= _duration)
		return true;

	// 2) 다음 확인 간격 = 지금 간격 x (min(_duration/8, 남은 시간) / 지금 간격에 걸린 시간) (1 ~ _count_check_max)
	const auto ticks_budget = static_cast<int64_t>((std::min)(_duration / 8, _duration - tick_waited).count());
	const auto ticks_elapsed = static_cast<int64_t>((tick_now - t_tick_cork_checked).count());
	const auto count_interval = static_cast<int64_t>(t_count_cork_interval);
	const auto count_max = static_cast<int64_t>(_count_check_max);

	auto count_next = (ticks_elapsed * count_max <= count_interval * ticks_budget) ? count_max : (count_interval * ticks_budget / ticks_elapsed);
	if (count_next < 1)
		count_next = 1;

	t_tick_cork_checked = tick_now;
	t_count_cork_interval = static_cast<std::size_t>(count_next);
	t_count_cork_check = t_count_cork_interval;

	// return)
	return false;
}


void net::io::sendable::Ntcp_async_gather::QUEUE_SEND::push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _bytes_data, std::size_t _count_message)
{
	// 1) node를 넣는다.
	this->array_node.push_back(NODE{ std::move(_powner), _bytes_data, _count_message });

	// 2) buffer를 넣는다. (실패하면 node도 빼서 두 배열의 크기를 맞춘다)
	try
	{
		this->array_buffer.push_back(::iovec{ const_cast<void*>(static_cast<const void*>(_buffer.data())), _buffer.size() });
	}
	catch (...)
	{
		this->array_node.pop_back();
		throw;
	}
}

void net::io::sendable::Ntcp_async_gather::QUEUE_SEND::push_back_or_expand(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message)
{
	// 1) 바로 앞 buffer에 이어진 메모리이고 owner가 같으면 늘리기만 한다.
	if (this->array_node.empty() == false)
	{
		auto& buffer_back = this->array_buffer.back();
		auto& node_back = this->array_node.back();

		if (static_cast<const char*>(buffer_back.iov_base) + buffer_back.iov_len == static_cast<const char*>(_buffer.data())
			&& node_back.powner == _powner)
		{
			// - expand only
			buffer_back.iov_len += _buffer.size();
			node_back.bytes_data += _buffer.size();
			node_back.count_message += _count_message;
			this->bytes_data += _buffer.size();

			// return)
			return;
		}
	}

	// 2) queueing buffers
	this->push_back(_buffer, std::move(_powner), _buffer.size(), _count_message);
	this->bytes_data += _buffer.size();
}

bool net::io::sendable::Ntcp_async_gather::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT_ERROR(_buffer.size() != 0);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// declare)
	CGSOCKET handle_socket;

	// - send defer : 이 thread가 그룹 락 안에서 보내거나(send_defer) 이 socket이 받은 message를 처리하는 중이다(send cork).
	//   (바로 보내지 않고 모아 두었다가 한번에 보낸다)
	const auto ptoken = send_defer::token();
	const bool is_deferring = send_defer::is_deferring(static_cast<const Isocket*>(this));

	// declare)
	bool is_cork_queued = false;
	bool is_cork_first = false;
	bool is_cork_full = false;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
			return false;

		// 1) get socket native handle
		handle_socket = this->native_handle();

		// check) if invalid socket, return false
		if(handle_socket == INVALID_SOCKET)
			return false;

		// check) m_flag_sending이 signal된 상태(true)라면 보내고 있는 thread가 있다. (queue에 넣기만 한다)
		//        (send cork로 이 thread가 모으고 있는 중일 수도 있다)
		if (this->m_flag_sending == true)
		{
			// case) send cork : 이 thread가 모으고 있다. (이 thread가 보내는 thread다)
			//       - 그 사이 다른 thread가 보낸 것(m_queue_pending)이 없으면 m_queue_sending 뒤에 바로 이어 넣는다.
			//         있으면 순서를 지키기 위해 그 뒤(m_queue_pending)에 넣는다.
			if (this->m_pdefer_owner.load(std::memory_order_relaxed) == ptoken)
			{
				if (this->m_queue_pending.array_node.empty())
					this->m_queue_sending.push_back_or_expand(_buffer, std::move(_powner), _count_messages);
				else
					this->m_queue_pending.push_back_or_expand(_buffer, std::move(_powner), _count_messages);

				// - 모은 것이 많은가? (큰 message를 넣었으면 바로 보낸다)
				is_cork_queued = true;
				is_cork_full = this->_is_send_defer_full();
			}
			else
			{
				// check) queue overflowed? (보내는 thread가 가져간 m_queue_sending은 세지 않는다 : EPOLLOUT을 기다리며 남은 것 포함. iocp의 WSASend 중인 것과 같다)
				if ((this->m_queue_pending.array_node.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= this->m_bytes_max_queued) )
				{
					// statistics)
					this->statistics_on_error_send();

					// check)
					this->process_on_send_overflow();

					// return)
					return false;
				}

				// - queueing buffers (바로 앞 buffer에 이어진 메모리이고 owner가 같으면 늘리기만 한다)
				this->m_queue_pending.push_back_or_expand(_buffer, std::move(_powner), _count_messages);

				// return)
				return true;
			}
		}
		else
		{
			// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
			CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

			// check) 보내는 thread가 없으면 m_queue_sending은 비어 있다.
			CGASSERT_ERROR(this->m_index_sending == this->m_queue_sending.array_node.size());

			// - send cork : 바로 보내지 않고 m_queue_sending에 넣어 둔다. (처음 모은다)
			//   - 이 thread가 보내는 thread가 된다. (m_queue_sending은 보내는 thread만 쓴다 : flush 때 swap 없이 바로 보낸다)
			//   - 그 사이 다른 thread가 보내는 것은 m_flag_sending을 보고 그 뒤(m_queue_pending)에 쌓는다.
			//   - 한번에 모을 크기보다 큰 message는 모으지 않는다. (바로 보낸다)
			if (is_deferring && _buffer.size() < SEND_DEFER_MAX_BYTES)
			{
				this->m_queue_sending.push_back(_buffer, std::move(_powner), _buffer.size(), _count_messages);
				this->m_queue_sending.bytes_data += _buffer.size();

				// - 이 thread가 모은다. (등록은 lock 밖에서) 모은 것이 많은가? (queue 한도가 아주 작을 때)
				this->m_pdefer_owner.store(ptoken, std::memory_order_relaxed);
				is_cork_queued = true;
				is_cork_first = true;
				is_cork_full = this->_is_send_defer_full();
			}

			// - set flag_sending (이 thread가 보낸다)
			this->m_flag_sending = true;
		}
	}

	// case) send defer (그룹 락 안 / send cork)
	if (is_cork_queued)
	{
		// - 처음 모았다 => 등록한다. (가장 바깥 그룹 락이 풀릴 때 / send_cork의 범위가 끝날 때 process_flush_deferred_send()로 보낸다)
		//   등록에 실패하면 바로 보낸다.
		if (is_cork_first)
		{
			if (send_defer::push(this, object_ptr<Ireferenceable>(this)) == false)
				is_cork_full = true;

			if (send_defer::is_cork_age_checked())
				_cork_begin();
		}
		// - send cork : 가장 먼저 모은 것이 오래 기다렸다 => 지금 보낸다. (그룹 락 안에서는 그룹 락이 풀릴 때 보낸다)
		else if (is_cork_full == false && send_defer::is_cork_age_checked() && _is_cork_expired(SEND_CORK_DURATION, SEND_CORK_CHECK_MAX))
		{
			is_cork_full = true;
		}

		// - 모은 것이 많거나 오래됐으면 범위가 끝나기 전에 보낸다.
		if (is_cork_full)
		{
			this->process_flush_deferred_send();
		}

		// return)
		return true;
	}

	// check)
	CGASSERT_ERROR(_buffer.size() != 0);

	// 2) 바로 보낸다.
	const auto bytes_requested = _buffer.size();

	// - io_uring completion : a large buffer goes as a SEND_ZC op, and the sends of the thread that owns the ring of this socket go as a
	//   SENDMSG op on it (submitted with its next enter : no send syscall). not sent directly : the op is the sending thread till it completes
	if (this->is_io_completion() && (this->_is_zerocopy_op(bytes_requested, _powner) || this->_is_send_on_owner_ring(handle_socket)))
	{
		SCOPED_LOCK(this->m_lockable_send_call)
		{
			this->m_queue_sending.push_back(_buffer, std::move(_powner), bytes_requested, _count_messages);
			this->m_queue_sending.bytes_data = bytes_requested;
		}

		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->m_count_sending_parked = 1;
			this->m_bytes_sending_parked = bytes_requested;

			// statistics)
			this->statistics_on_increase_async_sending();

			// - post the op
			this->_process_post_send_op();
		}

		// return)
		return true;
	}

	// - MSG_ZEROCOPY for a large buffer (the io_uring backend : the owner is held until the kernel notifies the completion)
	int flags_send = MSG_DONTWAIT | MSG_NOSIGNAL | this->_zerocopy_send_flags(handle_socket, bytes_requested, _powner);
	ssize_t bytes_result = 0;
	bool is_closed = false;

	// - the handle is checked and sent under m_lockable_send_call (closed meanwhile : the close swapped it under the lock)
	SCOPED_LOCK(this->m_lockable_send_call)
	{
		is_closed = (handle_socket == INVALID_SOCKET || this->native_handle() != handle_socket);

		if (is_closed == false)
			bytes_result = this->_zerocopy_send(handle_socket, _buffer.data(), bytes_requested, flags_send);
	}

	// check) closed meanwhile => the queued sends are dropped
	if (is_closed)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
		}

		// return)
		return false;
	}

	// case) send completed~
	if (bytes_result == static_cast<ssize_t>(bytes_requested))
	{
		// - the buffer is not used any more (zerocopy : held until the kernel notifies)
		if ((flags_send & MSG_ZEROCOPY) != 0)
			this->_zerocopy_hold(_buffer, std::move(_powner));
		else
			_powner.reset();

		// - 그 사이 queue에 쌓인 것을 이어서 보낸다.
		//   (EPOLLOUT을 기다리지 않는다 : queue가 빌 때까지 보내고, socket 송신 버퍼가 찼을 때(EAGAIN)만 EPOLLOUT을 건다)
		const auto result = this->_process_send_queued(handle_socket, bytes_requested, _count_messages);

		// - EPOLLOUT을 기다린다.
		if (result == EPOLLOUT)
		{
			SCOPED_LOCK(this->m_lockable_sending)
			{
				// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
				CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

				// statistics)
				this->statistics_on_increase_async_sending();

				// - set EPOLLOUT event flag
				this->_process_post_send_wait();
			}
		}

		// return) 이 buffer는 보냈다.
		return true;
	}

	// declare)
	std::size_t bytes_sent = 0;

	// case) send partial
	if (bytes_result >= 0)
	{
		// check)
		CGASSERT_ERROR(static_cast<std::size_t>(bytes_result) < bytes_requested);

		// - 보낸 bytes
		bytes_sent = static_cast<std::size_t>(bytes_result);

		// - zerocopy : the part sent is held until the kernel notifies (the rest below shares the owner)
		if ((flags_send & MSG_ZEROCOPY) != 0 && bytes_sent != 0)
			this->_zerocopy_hold(buffer_view{ _buffer.data(), bytes_sent }, object_ptr<Ireferenceable>(_powner));
	}
	else
	{
		// get error code
		const auto error_code = api::get_last_error();

		// case) fail to send (EWOULDBLOCK이면 EPOLLOUT을 기다린다)
		if (error_code != EWOULDBLOCK && error_code != EINPROGRESS)	// - EINPROGRESS : a TCP Fast Open connect (TCP_FASTOPEN_CONNECT) without a cookie. the SYN is sent, the data waits for EPOLLOUT
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics)
			this->statistics_on_error_send();

			// - closesocket
			this->process_close_socket(DISCONNECT_REASON_FAIL);

			SCOPED_LOCK(this->m_lockable_sending)
			{
				// - clear send queue
				this->_process_clear_send_buffers();

				// - reset flag
				this->m_flag_sending = false;
			}

			// return)
			return false;
		}
	}

	// 3) 남은 것은 m_queue_sending에 넣고 EPOLLOUT을 기다린다. (socket 송신 버퍼가 찼다 : 바로 다시 보내도 EAGAIN이다)
	//    - m_queue_sending은 보내는 thread(m_flag_sending)만 쓴다. 지금은 비어 있다.
	//    (m_lockable_send_call 안 : process_reset_sendable()이 그 안에서 m_queue_sending을 비운다)
	SCOPED_LOCK(this->m_lockable_send_call)
	{
		auto buf_remained = _buffer;
		buf_remained += offset(static_cast<int64_t>(bytes_sent));

		this->m_queue_sending.push_back(buf_remained, std::move(_powner), bytes_requested, _count_messages);
		this->m_queue_sending.bytes_data = buf_remained.size();
	}

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - EPOLLOUT을 기다리는 것을 남긴다. (중복 EPOLLOUT 판별, sending_queued_bytes_now() : queue 한도에는 세지 않는다)
		this->m_count_sending_parked = 1;
		this->m_bytes_sending_parked = this->m_queue_sending.bytes_data;

		// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// statistics)
		this->statistics_on_increase_async_sending();

		// - set EPOLLOUT event flag
		this->_process_post_send_wait();
	}

	// return)
	return true;
}

uint32_t net::io::sendable::Ntcp_async_gather::_process_send_queued(CGSOCKET _handle_socket, std::size_t _bytes_completed, std::size_t _count_completed)
{
	//-----------------------------------------------------------------
	// 설명) 보내는 thread(m_flag_sending == true)만 호출한다.
	//
	//  - m_queue_sending이 비면 m_queue_pending과 swap해 (할당/복사 없이) 가져와 sendmsg로 보낸다.
	//    queue가 모두 빌 때까지 반복한다.
	//  - return
	//    0                  : 모두 보냈다. (m_flag_sending = false)
	//    EPOLLOUT           : socket 송신 버퍼가 찼다. 남은 것은 m_queue_sending에 두고 EPOLLOUT을 기다린다.
	//    SEND_QUEUED_FAILED : 실패해 접속을 끊었다. (m_flag_sending = false)
	//  - 통계(process_complete_sendable)는 호출당 한번만 한다.
	//-----------------------------------------------------------------

	// declare)
	uint32_t result = 0;
	bool is_shutdown = false;
	bool is_closed = false;
	msghdr msg_send{};

	for (;;)
	{
		// 1) 보낼 것이 없으면 m_queue_pending에서 가져온다.
		bool is_sent_all;

		SCOPED_LOCK(this->m_lockable_send_call)
		{
			is_sent_all = (this->m_index_sending == this->m_queue_sending.array_node.size());

			// - 보낸 buffer들의 owner를 release한다. (m_lockable_sending 밖에서. 용량은 남는다)
			//   (m_lockable_send_call 안 : close 뒤 process_reset_sendable()이 그 안에서 m_queue_sending을 비운다)
			if (is_sent_all)
			{
				this->m_queue_sending.clear();
				this->m_index_sending = 0;
			}
		}

		if (is_sent_all)
		{
			// declare)
			bool is_empty;

			SCOPED_LOCK(this->m_lockable_sending)
			{
				// - get
				is_empty = this->m_queue_pending.array_node.empty();

				// - 보낼 것이 없다 => 송신을 끝낸다. (이후 m_queue_sending은 다음 보내는 thread가 쓴다)
				if (is_empty)
				{
					is_shutdown = (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED);
					this->m_flag_sending = false;
				}
				// - swap (m_queue_pending은 비어 있던 m_queue_sending과 바뀐다)
				else
				{
					this->m_queue_sending.swap(this->m_queue_pending);
				}
			}

			// break) 모두 보냈다.
			if (is_empty)
				break;
		}

		// 2) send / 3) the part sent : m_lockable_send_call 안에서 한다. (close가 그 안에서 handle을 바꾸고, 그 뒤 process_reset_sendable()이
		//    그 안에서 m_queue_sending을 비운다 : 닫힌 뒤 다시 만들어진 같은 번호의 handle로 보내지 않고, 비워진 queue를 읽지 않는다)
		//    - SCOPED_LOCK은 if 문이다 : 안의 break / continue는 for 문에 걸리고 lock은 if 문을 빠져나갈 때 풀린다.
		int error_code = 0;

		SCOPED_LOCK(this->m_lockable_send_call)
		{
			// check) 그 사이 닫혔다. (close가 handle을 바꿨다 / process_reset_sendable()이 m_queue_sending을 비웠다)
			if (_handle_socket == INVALID_SOCKET || this->native_handle() != _handle_socket || this->m_index_sending >= this->m_queue_sending.array_node.size())
			{
				is_closed = true;
				break;
			}

			// 2) send (sendmsg는 한번에 IOV_MAX개의 buffer까지만 보낼 수 있다)
			const auto index_end = this->m_index_sending + std::min<std::size_t>(this->m_queue_sending.array_node.size() - this->m_index_sending, CGDK_SEND_MAX_IOV);

			const auto pbuffer_send = this->m_queue_sending.array_buffer.data() + this->m_index_sending;
			const auto count_buffer = index_end - this->m_index_sending;

			// - buffer가 하나면 send로 보낸다. (sendmsg보다 가볍다)
			ssize_t bytes_result;
			bool is_zerocopy = false;

			// - io_uring completion : the thread that owns the ring of this socket sends with an op (the caller posts it like a full socket buffer)
			if (this->is_io_completion() && this->_is_send_on_owner_ring(_handle_socket))
			{
				result = EPOLLOUT;
				break;
			}

			if (count_buffer == 1)
			{
				// - io_uring completion : a large buffer goes as a SEND_ZC op (not sent directly : the caller posts the op like a full socket buffer)
				if (this->is_io_completion() && this->_is_zerocopy_op(pbuffer_send->iov_len, this->m_queue_sending.array_node[this->m_index_sending].powner))
				{
					result = EPOLLOUT;
					break;
				}

				// - a large buffer : MSG_ZEROCOPY (the io_uring backend)
				int flags_send = MSG_DONTWAIT | MSG_NOSIGNAL | this->_zerocopy_send_flags(_handle_socket, pbuffer_send->iov_len, this->m_queue_sending.array_node[this->m_index_sending].powner);

				bytes_result = this->_zerocopy_send(_handle_socket, pbuffer_send->iov_base, pbuffer_send->iov_len, flags_send);
				is_zerocopy = (bytes_result > 0) && (flags_send & MSG_ZEROCOPY) != 0;
			}
			else
			{
				msg_send.msg_iov = pbuffer_send;
				msg_send.msg_iovlen = count_buffer;

				bytes_result = ::sendmsg(_handle_socket, &msg_send, MSG_DONTWAIT | MSG_NOSIGNAL);
			}

			// 3) the part sent
			if (bytes_result > 0)
			{
				// - zerocopy : the part sent is held until the kernel notifies
				if (is_zerocopy)
					this->_zerocopy_hold(buffer_view{ static_cast<char*>(pbuffer_send->iov_base), static_cast<std::size_t>(bytes_result) }, object_ptr<Ireferenceable>(this->m_queue_sending.array_node[this->m_index_sending].powner));

				// declare)
				auto bytes_left = static_cast<std::size_t>(bytes_result);

				for (; this->m_index_sending < index_end; ++this->m_index_sending)
				{
					auto& buffer_now = this->m_queue_sending.array_buffer[this->m_index_sending];

					// - 일부만 보냈다 => 남은 위치로 옮긴다.
					if (bytes_left < buffer_now.iov_len)
					{
						buffer_now.iov_base = static_cast<char*>(buffer_now.iov_base) + bytes_left;
						buffer_now.iov_len -= bytes_left;
						break;
					}

					// - 다 보냈다.
					bytes_left -= buffer_now.iov_len;

					const auto& node_now = this->m_queue_sending.array_node[this->m_index_sending];
					_bytes_completed += node_now.bytes_data;
					_count_completed += node_now.count_message;
				}

				// - 남은 bytes
				this->m_queue_sending.bytes_data -= static_cast<std::size_t>(bytes_result);

				// continue) 보내려던 것을 다 보냈으면 이어서 보낸다.
				if(this->m_index_sending == index_end)
					continue;

				// - 덜 보냈다 => socket 송신 버퍼가 찼다. EPOLLOUT을 기다린다.
				result = EPOLLOUT;
				break;
			}

			// get last error
			error_code = api::get_last_error();
		}

		// 4-1) EWOULDBLOCK => EPOLLOUT을 기다린다.
		if (error_code == EWOULDBLOCK || error_code == EINPROGRESS)	// - EINPROGRESS : a TCP Fast Open connect (TCP_FASTOPEN_CONNECT) without a cookie. the SYN is sent, the data waits for EPOLLOUT
		{
			result = EPOLLOUT;
			break;
		}

		// 4-2) EINTR => 다시 보낸다.
		if(error_code == EINTR)
			continue;

		// 4-3) failure
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics)
			this->statistics_on_error_send();

			// - closesocket (abortive close)
			this->process_close_socket(DISCONNECT_REASON_FAIL);

			SCOPED_LOCK(this->m_lockable_sending)
			{
				// - clear send queue
				this->_process_clear_send_buffers();

				// - reset flag
				this->m_flag_sending = false;
			}

			// - set result
			result = SEND_QUEUED_FAILED;
		}

		// break)
		break;
	}

	// check) 그 사이 닫혔다 => 남은 것은 버린다. (닫은 쪽이 접속 종료를 처리한다)
	if (is_closed)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->_process_clear_send_buffers();
			this->m_flag_sending = false;
		}

		// - set result
		result = SEND_QUEUED_FAILED;
	}

	// 5) EPOLLOUT을 기다리며 남은 것을 남긴다. (중복 EPOLLOUT 판별, sending_queued_bytes_now() : queue 한도에는 세지 않는다)
	if (result == EPOLLOUT)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->m_count_sending_parked = this->m_queue_sending.array_node.size() - this->m_index_sending;
			this->m_bytes_sending_parked = this->m_queue_sending.bytes_data;
		}
	}

	// statistics) 보낸 것들의 통계 (한번에)
	if (_bytes_completed != 0 || _count_completed != 0)
	{
		this->process_complete_sendable(0, _bytes_completed, _count_completed);
	}

	// - shutdown if send_disconnected (모두 보냈을 때)
	if (is_shutdown)
		this->_process_shutdown_send(_handle_socket);

	// return)
	return result;
}

void net::io::sendable::Ntcp_async_gather::_process_shutdown_send(CGSOCKET _handle_socket) noexcept
{
	// - m_lockable_send_call 안에서 handle을 확인한다. (그 사이 닫혔으면 하지 않는다 : 같은 번호로 다시 만들어진 handle일 수 있다)
	SCOPED_LOCK(this->m_lockable_send_call)
	{
		// check)
		RETURN_IF(_handle_socket == INVALID_SOCKET || this->native_handle() != _handle_socket);

		// - set linger Option for graceful disconnect
		api::socket::set_linger_option(_handle_socket, 0);

		// - send shutdown
		api::socket::shutdown(_handle_socket, SD_SEND);
	}
}

uint32_t net::io::sendable::Ntcp_async_gather::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	// check) EPOLLOUT이 설정되어 있어야 한다.
	CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

	// 1) EPOLLOUT을 기다리던 것을 이 thread가 이어서 보낸다.
	SCOPED_LOCK(this->m_lockable_sending)
	{
		//
		// 설명) EPOLLOUT 이벤트는 중복 실행될 수 있으므로 적절한 처리가 필요하다.
		//  보낼 것(EPOLLOUT을 기다리며 m_queue_sending에 남은 것, m_queue_pending)이 없으면 바로 리턴한다.
		//  (이때 m_flag_sending은 건드리지 않는다 : 다른 thread가 바로 보내는 중일 수 있다)
		//
		//  중복 이벤트가 발생하더라도 절대로 발생해서는 안 되는 상황을 설명하자면...
		//
		//    - m_queue_pending.array_node가 empty()가 아닌데 m_flag_sending가 false인 상황
		//    - EPOLLOUT을 기다리는 것(m_count_sending_parked != 0)이 있는데 m_flag_sending가 false인 상황
		//

		// check)
		if (this->m_count_sending_parked == 0 && this->m_queue_pending.array_node.empty())
		{
			// statistics)
			this->statistics_on_decrease_async_sending();

			// check)
			CGASSERT_ERROR(this->m_queue_pending.bytes_data == 0);

			// return)
			return 0;
		}

		// check)
		CGASSERT_ERROR(this->m_flag_sending == true);

		// - 이 thread가 보낸다. (EPOLLOUT을 기다리는 것이 없다 : 그 사이 중복 EPOLLOUT은 바로 돌아간다)
		this->m_count_sending_parked = 0;
		this->m_bytes_sending_parked = 0;
	}

	// 2) queue가 빌 때까지 보낸다.
	const auto result = this->_process_send_queued(this->native_handle(), 0, 0);

	// check) 아직 남았다 => 다시 EPOLLOUT을 기다린다.
	if(result == EPOLLOUT)
		return EPOLLOUT;

	// statistics) 모두 보냈거나 실패했다. (송신 끝)
	this->statistics_on_decrease_async_sending();

	// return)
	return 0;
}

bool net::io::sendable::Ntcp_async_gather::_is_send_defer_full() const noexcept
{
	// - m_lockable_sending 안에서 부른다. (모은 것은 m_queue_sending과 그 뒤 m_queue_pending에 있다)
	//   queue 한도의 절반을 넘지 않게 한다. (모으느라 overflow가 나지 않는다)
	const auto count = (this->m_queue_sending.array_node.size() - this->m_index_sending) + this->m_queue_pending.array_node.size();
	const auto bytes = this->m_queue_sending.bytes_data + this->m_queue_pending.bytes_data;

	return count >= SEND_DEFER_MAX_COUNT || bytes >= SEND_DEFER_MAX_BYTES || (count * 2) >= this->m_max_depth_of_send_buffer || (bytes * 2) >= this->m_bytes_max_queued;
}

void net::io::sendable::Ntcp_async_gather::process_flush_deferred_send() noexcept
{
	// 설명) send defer로 이 thread가 모아 둔 것을 보낸다. (send_defer::flush가 부른다)
	//       이미 보냈거나(조기 flush) 다른 thread가 가져갔으면(closesocket) 아무것도 하지 않는다.
	//       (모으기를 끝내는 것은 이 thread와 closesocket(lock 안)뿐이다 : 한쪽만 보낸다)
	auto ptoken = send_defer::token();

	// check) 이 thread가 모은 것이 아니다.
	if(this->m_pdefer_owner.compare_exchange_strong(ptoken, nullptr, std::memory_order_acq_rel) == false)
		return;

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Ntcp_async_gather::process_flush_deferred_send_on_close() noexcept
{
	// 설명) socket을 닫기 전에 부른다. 어느 thread가 모아 둔 것이든 지금 보낸다.
	//       (그룹 락 안에서 send한 뒤 다른 thread가 closesocket해도 보낸 것이 먼저 간다)
	//       - lock 안에서 가져온다. (모으는 thread가 lock 안에서 m_queue_sending에 넣는 중일 수 있다 : 그 뒤 그 thread는 m_queue_pending에 넣는다)
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 모아 둔 것이 없다.
		if(this->m_pdefer_owner.exchange(nullptr, std::memory_order_acq_rel) == nullptr)
			return;
	}

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Ntcp_async_gather::_process_send_deferred() noexcept
{
	//-----------------------------------------------------------------
	// 설명) send defer로 모아 둔 것을 보낸다. (m_pdefer_owner를 가져온 thread가 부른다)
	//
	//  - 처음 모을 때 이 thread가 보내는 thread(m_flag_sending)가 되어 m_queue_sending에 모았다.
	//    그 사이 다른 thread가 보낸 것은 그 뒤(m_queue_pending)에 쌓여 있다. (그 뒤에 이 thread가 보낸 것도)
	//  - 바로 보낼 때와 같이 queue가 빌 때까지 보내고, socket 송신 버퍼가 찼을 때만 EPOLLOUT을 기다린다.
	//-----------------------------------------------------------------

	// 1) get socket native handle
	const auto handle_socket = this->native_handle();

	// check) 그 사이 다른 thread가 socket을 닫았다 => 모아 둔 것은 버린다.
	if (handle_socket == INVALID_SOCKET)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			// - clear send queue
			this->_process_clear_send_buffers();

			// - reset flag
			this->m_flag_sending = false;
		}

		// return)
		return;
	}

	// 2) queue가 빌 때까지 보낸다.
	const auto result = this->_process_send_queued(handle_socket, 0, 0);

	// 3) EPOLLOUT을 기다린다.
	if (result == EPOLLOUT)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
			CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

			// statistics)
			this->statistics_on_increase_async_sending();

			// - set EPOLLOUT event flag
			this->_process_post_send_wait();
		}
	}
}

std::size_t net::io::sendable::Ntcp_async_gather::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data + this->m_bytes_sending_parked;
}

void net::io::sendable::Ntcp_async_gather::_process_clear_send_buffers()
{
	// statistics)
	if (this->m_queue_pending.array_node.empty() == false)
	{
		this->process_complete_sendable(1, 0, 0);
	}

	this->m_queue_pending.clear();

	// - m_queue_sending : under m_lockable_send_call (the sending thread sends and consumes it under the lock out of m_lockable_sending)
	SCOPED_LOCK(this->m_lockable_send_call)
	{
		// - io_uring completion : a SEND op in flight uses m_queue_sending (its iovecs, its buffers) => moved to the executable till the op ends
		if (this->m_count_sending_op != 0 && this->m_pexecutable_sending.exist())
		{
			auto& queue_hold = this->m_pexecutable_sending->m_queue_hold;

			if (queue_hold.array_node.empty())
				queue_hold.swap(this->m_queue_sending);
			else
				try { queue_hold.array_node.insert(queue_hold.array_node.end(), std::make_move_iterator(this->m_queue_sending.array_node.begin()), std::make_move_iterator(this->m_queue_sending.array_node.end())); } catch (...) {}
		}

		this->m_queue_sending.clear();
		this->m_index_sending = 0;
	}
	this->m_count_sending_parked = 0;
	this->m_bytes_sending_parked = 0;

	// - zerocopy : the buffers held for the notifications
	this->_zerocopy_reset();
}

void net::io::sendable::Ntcp_async_gather::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics)
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

void net::io::sendable::Ntcp_async_gather::process_close_sender()
{
	// check)
	bool do_shutdown;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		do_shutdown = !this->m_flag_sending;
	}

	// - linger Option을 Graceful disconnect로 설정하고 접속을 종료한다.
	if (do_shutdown)
		this->_process_shutdown_send(this->native_handle());
}

void net::io::sendable::Ntcp_async_gather::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 1) event_out flag
		this->m_flag_sending = false;

		// 2)
		this->_process_clear_send_buffers();

		// 3) send defer : 모으던 thread의 flush는 아무것도 하지 않는다.
		this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
}

#endif
