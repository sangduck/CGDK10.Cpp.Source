#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

bool net::io::sendable::Ntcp_async_gather_buffered::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
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

	// - send defer : 이 thread가 그룹 락 안에서 보내는 중이다. (바로 보내지 않고 모아 두었다가 가장 바깥 그룹 락이 풀릴 때 한번에 보낸다)
	const auto ptoken = send_defer::token();
	const bool is_deferring = send_defer::is_deferring(static_cast<const Isocket*>(this));

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

		// case) send defer : 이 thread가 모으고 있다. (아직 WSASend하지 않았다 : m_queue_sending은 이 thread만 쓴다)
		//       - 그 사이 다른 thread가 보낸 것(m_queue_pending)이 없으면 m_queue_sending 뒤에 넣는다.
		//         있으면 순서를 지키기 위해 그 뒤(m_queue_pending)에 넣는다.
		if (this->m_pdefer_owner.load(std::memory_order_relaxed) == ptoken)
		{
			if (this->m_queue_pending.array_buffer.empty())
				this->m_queue_sending.push_back(_buffer, std::move(_powner), _count_messages);
			else
				this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

			// - 모은 것이 많으면 지금 보낸다. (overflow 되지 않게)
			if (this->_is_send_defer_full())
				this->_process_send_deferred();

			// return)
			return true;
		}

		// 2) 전송 처리 중(m_flag_sending이 signal된 상태(true))라면 queuing 처리만 하고 끝낸다.
		if (this->m_flag_sending == true)
		{
			// check) queue overflowed?
			if ((this->m_queue_pending.array_buffer.size() >= m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= m_bytes_max_queued))
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				this->process_on_send_overflow();

				// return)
				return false;
			}

			// - queuing to pending_queue
			this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

			// return) 
			return true;
		}

		// case) send defer : 처음 모은다. (WSASend하지 않고 m_queue_sending에 넣어 둔다)
		//       - 이 thread가 보내는 thread가 된다. (그 사이 다른 thread가 보내는 것은 m_flag_sending을 보고 m_queue_pending에 쌓는다)
		//       - 등록하면 가장 바깥 그룹 락이 풀릴 때 process_flush_deferred_send()로 보낸다. (등록에 실패하면 바로 보낸다)
		//       - 한번에 모을 크기보다 큰 message는 모으지 않는다. (바로 보낸다)
		if (is_deferring && _buffer.size() < SEND_DEFER_MAX_BYTES)
		{
			// check) 보내는 thread가 없으면 m_queue_sending은 비어 있다.
			CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty());

			// - queueing
			this->m_queue_sending.push_back(_buffer, std::move(_powner), _count_messages);

			// - 이 thread가 보낸다. (모은다)
			this->m_flag_sending = true;
			this->m_pdefer_owner.store(ptoken, std::memory_order_relaxed);

			// - 등록한다. (모은 것이 많으면 지금 보낸다 : queue 한도가 아주 작을 때)
			if (send_defer::push(this, object_ptr<Ireferenceable>(this)) == false || this->_is_send_defer_full())
				this->_process_send_deferred();

			// return)
			return true;
		}

		// 3) set flag_sending true
		this->m_flag_sending = true;

		// - handle은 lock 안에서 읽고 send까지 lock 안에서 한다.
		//   (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
		// 4) queuing to sending buffer
		{
			CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty());
			CGASSERT_ERROR(this->m_queue_sending.array_node.empty());

			this->m_queue_sending.array_buffer.emplace_back(_buffer.size<ULONG>(), _buffer.data());
			this->m_queue_sending.array_node.emplace_back(std::move(_powner));
			this->m_queue_sending.bytes_data = _buffer.size();
			this->m_queue_sending.count_message = _count_messages;
		}

		// 5) prepare executable
		this->m_pexecutable_send_buffer_list->m_psendable = this;

		// 6) reference count를 증가시킨다.
		this->m_pexecutable_send_buffer_list->add_ref();

		// statistics) send overlapped 카운트를 하나 증가시킨다.
		this->statistics_on_increase_async_sending();

		// 7) send async
		auto result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), 1, nullptr, this->m_pexecutable_send_buffer_list.get());

		// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
		if (result == false && api::get_last_error() != WSA_IO_PENDING)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// statistics)
			this->statistics_on_error_send();

			// - send overlapped 카운트를 하나 줄인다.
			this->statistics_on_decrease_async_sending();

			// - reset 
			this->m_pexecutable_send_buffer_list->m_psendable.reset();

			// - release 'm_pexecutable_send_buffer_list'
			this->m_pexecutable_send_buffer_list->release();

			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return false;
		}

		// statistics) 
		this->statistics_on_send_async();

		// return)
		return true;
	}

	// return) (not reached)
	return false;
}

bool net::io::sendable::Ntcp_async_gather_buffered::_is_send_defer_full() const noexcept
{
	// - m_lockable_sending 안에서 부른다. (모은 것은 m_queue_sending과 그 뒤 m_queue_pending에 있다)
	//   queue 한도의 절반을 넘지 않게 한다. (모으느라 overflow가 나지 않는다)
	const auto count = this->m_queue_sending.array_buffer.size() + this->m_queue_pending.array_buffer.size();
	const auto bytes = this->m_queue_sending.bytes_data + this->m_queue_pending.bytes_data;

	return count >= SEND_DEFER_MAX_COUNT || bytes >= SEND_DEFER_MAX_BYTES || (count * 2) >= this->m_max_depth_of_send_buffer || (bytes * 2) >= this->m_bytes_max_queued;
}

void net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send() noexcept
{
	//-----------------------------------------------------------------
	// 설명) send defer로 이 thread가 모아 둔 것을 보낸다. (send_defer::flush가 부른다)
	//
	//  - 처음 모을 때 이 thread가 보내는 thread(m_flag_sending)가 되어 m_queue_sending에 모았다. (WSASend는 하지 않았다)
	//    그 사이 다른 thread가 보낸 것은 그 뒤(m_queue_pending)에 쌓여 있다. (완료 처리가 이어서 보낸다)
	//  - 이미 보냈거나(조기 flush) 다른 thread가 가져갔으면(closesocket) 아무것도 하지 않는다.
	//-----------------------------------------------------------------
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 이 thread가 모은 것이 아니다.
		if(this->m_pdefer_owner.load(std::memory_order_relaxed) != send_defer::token())
			return;

		// - send
		this->_process_send_deferred();
	}
}

void net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send_on_close() noexcept
{
	// 설명) socket을 닫기 전에 부른다. 어느 thread가 모아 둔 것이든 지금 보낸다.
	//       (그룹 락 안에서 send한 뒤 다른 thread가 closesocket해도 보낸 것이 먼저 간다)
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 모아 둔 것이 없다.
		if(this->m_pdefer_owner.load(std::memory_order_relaxed) == nullptr)
			return;

		// - send
		this->_process_send_deferred();
	}
}

void net::io::sendable::Ntcp_async_gather_buffered::_process_send_deferred() noexcept
{
	// - m_lockable_sending 안에서 부른다.

	// 1) 모으기를 끝낸다. (보내는 thread는 그대로다 : m_flag_sending)
	this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);

	// check)
	CGASSERT_ERROR(this->m_flag_sending == true);
	CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty() == false);

	// 2) get socket native handle
	const auto handle_socket = this->native_handle();

	// check) 그 사이 socket이 닫혔다 => 모아 둔 것은 버린다.
	if (handle_socket == INVALID_SOCKET)
	{
		// - process reset sendable
		this->process_reset_sendable();

		// return)
		return;
	}

	// 3) prepare executable
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 4) reference count를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// 5) send async (모아 둔 것을 한번에)
	auto result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), static_cast<uint32_t>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

	// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
	if (result == false && api::get_last_error() != WSA_IO_PENDING)
	{
		// - set last error
		this->m_last_socket_error = api::get_last_error();

		// statistics)
		this->statistics_on_error_send();

		// - send overlapped 카운트를 하나 줄인다.
		this->statistics_on_decrease_async_sending();

		// - reset
		this->m_pexecutable_send_buffer_list->m_psendable.reset();

		// - release 'm_pexecutable_send_buffer_list'
		this->m_pexecutable_send_buffer_list->release();

		// - process reset sendable
		this->process_reset_sendable();

		// return) fail to send
		return;
	}

	// statistics)
	this->statistics_on_send_async();
}

void net::io::sendable::Ntcp_async_gather_buffered::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// check) 이미 destroy 된 경우 더 이상 처리하지 않고 그냥 끝낸다. (destroy되어 clear 처리된 경우 bytes_data가 0일 것이다.)
	if(this->m_queue_sending.bytes_data == 0)
		return;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 1) clear seding queue
		this->m_queue_sending.clear();

		// - handle은 lock 안에서 읽고 send까지 lock 안에서 한다.
		//   (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
		const auto handle_socket = this->native_handle();

		// check) 
		if (handle_socket == INVALID_SOCKET)
		{
			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// check) return if no pended data
		if (this->m_queue_pending.array_buffer.empty())
		{
			// - reset queued
			this->m_flag_sending = false;

			// - shutdown if send_disconnected
			if (socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(handle_socket, 0);

				// - send shutdown
				api::socket::shutdown(handle_socket, SD_SEND);
			}

			// return)
			return;
		}

		// - move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.reset_and_swap(std::move(this->m_queue_pending));

		// - send (lock 안)
		// 2) prepare executable
		this->m_pexecutable_send_buffer_list->m_psendable = this;

		// 3) reference count를 증가시킨다.
		this->m_pexecutable_send_buffer_list->add_ref();

		// statistics) send overlapped 카운트를 하나 증가시킨다.
		this->statistics_on_increase_async_sending();

		// 4) send async
		auto result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), static_cast<uint32_t>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

		// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
		if (result == false && api::get_last_error() != WSA_IO_PENDING)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// statistics)
			this->statistics_on_error_send();

			// - send overlapped 카운트를 하나 줄인다.
			this->statistics_on_decrease_async_sending();

			// - reset 
			this->m_pexecutable_send_buffer_list->m_psendable.reset();

			// - release 'm_pexecutable_send_buffer_list'
			this->m_pexecutable_send_buffer_list->release();

			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// statistics) 
		this->statistics_on_send_async();
	}
}

std::size_t net::io::sendable::Ntcp_async_gather_buffered::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather_buffered::process_close_sender()
{
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 보내는 중이면 다 보낸 뒤(process_complete_sendable) shutdown한다.
		if(this->m_queue_sending.array_buffer.empty() == false)
			return;

		// - handle은 lock 안에서 읽는다. (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다)
		const auto handle_socket = this->native_handle();

		// check)
		if(handle_socket == INVALID_SOCKET)
			return;

		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(handle_socket, 0);

		// - 접속을 종료한다.
		api::socket::shutdown(handle_socket, SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather_buffered::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		this->m_flag_sending = false;
		this->m_queue_pending.clear();
		this->m_queue_sending.clear();

		// - send defer : 모으던 thread의 flush는 아무것도 하지 않는다.
		this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());

	// 3) reset executable
	if (this->m_pexecutable_send_buffer_list.empty())
	{
		this->m_pexecutable_send_buffer_list = make_object<executable_send_buffer_list>();
	}
}

#endif
