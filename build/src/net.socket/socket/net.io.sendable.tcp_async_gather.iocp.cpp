#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

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
	if (_buffer.data() == nullptr)
		return false;

	// - send defer : 이 thread가 그룹 락 안에서 보내는 중이다. (바로 보내지 않고 모아 두었다가 가장 바깥 그룹 락이 풀릴 때 한번에 보낸다)
	const auto ptoken = send_defer::token();
	const bool is_deferring = send_defer::is_deferring(static_cast<const Isocket*>(this));

	// declare) 이 thread가 보내는 thread가 되어 lock 밖에서 m_queue_sending을 보낸다.
	//          - is_send_own : 이 송신 하나만 보낸다. (실패하면 false를 돌려준다)
	bool is_send_now = false;
	bool is_send_own = false;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
			return false;

		// check) if invalid socket, return false (보낼 때 m_lockable_send_call 안에서 다시 확인한다)
		if(this->native_handle() == INVALID_SOCKET)
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
			{
				this->_process_send_deferred();
				is_send_now = true;
			}
		}

		// case) 다른 thread가 보내는 중이다. (m_queue_pending에 쌓는다)
		else if (this->m_flag_sending == true)
		{
			// check) queue overflowed?
			if ((this->m_queue_pending.array_buffer.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= m_bytes_max_queued))
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				this->process_on_send_overflow();

				// return)
				return false;
			}

			// - queuing to pending queue
			this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);
		}

		// case) send defer : 처음 모은다. (WSASend하지 않고 m_queue_sending에 넣어 둔다)
		//       - 이 thread가 보내는 thread가 된다. (그 사이 다른 thread가 보내는 것은 m_flag_sending을 보고 m_queue_pending에 쌓는다)
		//       - 등록하면 가장 바깥 그룹 락이 풀릴 때 process_flush_deferred_send()로 보낸다. (등록에 실패하면 바로 보낸다)
		//       - 한번에 모을 크기보다 큰 message는 모으지 않는다. (바로 보낸다)
		else if (is_deferring && _buffer.size() < SEND_DEFER_MAX_BYTES)
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
			{
				this->_process_send_deferred();
				is_send_now = true;
			}
		}
		else
		{
			// check)
			CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty());

			// 2) 이 thread가 보내는 thread가 된다. (lock 밖에서 보낸다 : 그 사이 다른 thread는 m_queue_pending에 쌓는다)
			this->m_flag_sending = true;
			this->m_queue_sending.push_back(_buffer, std::move(_powner), _count_messages);

			is_send_now = true;
			is_send_own = true;
		}
	}

	// check)
	if (is_send_now == false)
		return true;

	// 3) async send (lock 밖)
	const auto result = this->_process_send_queue_sending();

	// return)
	return (is_send_own) ? result : true;
}

bool net::io::sendable::Ntcp_async_gather::_is_send_defer_full() const noexcept
{
	// - m_lockable_sending 안에서 부른다. (모은 것은 m_queue_sending과 그 뒤 m_queue_pending에 있다)
	//   queue 한도의 절반을 넘지 않게 한다. (모으느라 overflow가 나지 않는다)
	const auto count = this->m_queue_sending.array_buffer.size() + this->m_queue_pending.array_buffer.size();
	const auto bytes = this->m_queue_sending.bytes_data + this->m_queue_pending.bytes_data;

	return count >= SEND_DEFER_MAX_COUNT || bytes >= SEND_DEFER_MAX_BYTES || (count * 2) >= this->m_max_depth_of_send_buffer || (bytes * 2) >= this->m_bytes_max_queued;
}

void net::io::sendable::Ntcp_async_gather::process_flush_deferred_send() noexcept
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

		// - 모으기를 끝낸다.
		this->_process_send_deferred();
	}

	// - send (lock 밖)
	this->_process_send_queue_sending();
}

void net::io::sendable::Ntcp_async_gather::process_flush_deferred_send_on_close() noexcept
{
	// 설명) socket을 닫기 전에 부른다. 어느 thread가 모아 둔 것이든 지금 보낸다.
	//       (그룹 락 안에서 send한 뒤 다른 thread가 closesocket해도 보낸 것이 먼저 간다)
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 모아 둔 것이 없다.
		if(this->m_pdefer_owner.load(std::memory_order_relaxed) == nullptr)
			return;

		// - 모으기를 끝낸다. (이 thread가 보내는 thread가 된다)
		this->_process_send_deferred();
	}

	// - send (lock 밖. close는 이것이 돌아온 뒤에 handle을 바꾼다)
	this->_process_send_queue_sending();
}

void net::io::sendable::Ntcp_async_gather::_process_send_deferred() noexcept
{
	// - m_lockable_sending 안에서 부른다. 모아 둔 것(m_queue_sending)은 부른 thread가 lock 밖에서 보낸다. (_process_send_queue_sending)

	// 1) 모으기를 끝낸다. (보내는 thread는 그대로다 : m_flag_sending)
	this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);

	// check)
	CGASSERT_ERROR(this->m_flag_sending == true);
	CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty() == false);
}

bool net::io::sendable::Ntcp_async_gather::_process_send_queue_sending() noexcept
{
	//-----------------------------------------------------------------
	// 설명) 보내는 thread(m_flag_sending)가 m_queue_sending을 보낸다.
	//
	//  - m_lockable_sending 밖에서 부른다. (WSASend 동안 다른 thread는 기다리지 않고 m_queue_pending에 쌓는다)
	//    m_queue_sending은 보내는 thread만 쓴다. (완료 처리가 지우고 m_queue_pending을 가져온다)
	//  - handle은 m_lockable_send_call 안에서 읽고 WSASend도 그 안에서 한다.
	//    (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
	//  - 실패하면 보내려던 것은 버린다. (process_reset_sendable)
	//-----------------------------------------------------------------
	bool result = false;

	SCOPED_LOCK(this->m_lockable_send_call)
	{
		// 1) get socket native handle
		const auto handle_socket = this->native_handle();

		// check) 그 사이 socket이 닫혔다 => 보내려던 것은 버린다.
		if (handle_socket == INVALID_SOCKET)
		{
			result = false;
		}
		else
		{
			// 2) executable을 준비한다.
			this->m_pexecutable_send_buffer_list->m_psendable = this;

			// 3) Reference count_node를 증가시킨다.
			this->m_pexecutable_send_buffer_list->add_ref();

			// statistics) send overlapped 카운트를 하나 증가시킨다.
			this->statistics_on_increase_async_sending();

			// - the messages of this WSASend (counted at its completion : process_reset_sendable() may clear m_queue_sending before it comes)
			this->m_count_message_sending.store(this->m_queue_sending.count_message, std::memory_order_release);

			// 4) send async
			result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), static_cast<DWORD>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

			// check)
			if (result == false)
			{
				// statistics)
				this->statistics_on_error_send();

				// statistics)
				this->statistics_on_decrease_async_sending();

				// - reset
				this->m_pexecutable_send_buffer_list->m_psendable.reset();

				// - rollback (release)
				this->m_pexecutable_send_buffer_list->release();
			}
		}
	}

	// check) fail to send
	if (result == false)
	{
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

std::size_t net::io::sendable::Ntcp_async_gather::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_count_message_sending.load(std::memory_order_acquire) : 0);

	// statistics) the overlapped send completed (it was never decreased : statistics_get_now_async_io() of a socket that had sent stayed > 0)
	this->statistics_on_decrease_async_sending();

	// check) 이미 destroy 된 경우 더 이상 처리하지 않고 그냥 끝낸다. (destroy되어 clear 처리된 경우 bytes_data가 0일 것이다.)
	if(this->m_queue_sending.bytes_data == 0)
		return;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 1) clear m_queue_sending
		//    - 보낸 thread가 WSASend에서 돌아온 뒤에 지운다. (m_lockable_send_call : 완료가 WSASend가 돌아오기 전에 올 수 있다)
		SCOPED_LOCK(this->m_lockable_send_call)
		{
			this->m_queue_sending.clear();
		}

		// check) 
		if (this->native_handle() == INVALID_SOCKET)
		{
			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// check)
		if (this->m_queue_pending.array_buffer.empty())
		{
			// - reset is_queued
			this->m_flag_sending = false;

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - handle은 m_lockable_send_call 안에서 읽는다. (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다)
				SCOPED_LOCK(this->m_lockable_send_call)
				{
					const auto handle_socket = this->native_handle();

					if (handle_socket != INVALID_SOCKET)
					{
						// - linger Option을 Graceful disconnect로 설정한다.
						api::socket::set_linger_option(handle_socket, 0);

						// - 접속을 종료한다.
						api::socket::shutdown(handle_socket, SD_SEND);
					}
				}
			}

			// return) 
			return;
		}

		// 2) move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.reset_and_swap(std::move(this->m_queue_pending));
	}

	// 3) send async (lock 밖 : 이 thread가 보내는 thread다)
	this->_process_send_queue_sending();
}

void net::io::sendable::Ntcp_async_gather::process_close_sender()
{
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 보내는 중이면 다 보낸 뒤(process_complete_sendable) shutdown한다.
		if(this->m_queue_sending.array_buffer.empty() == false)
			return;

		// - handle은 m_lockable_send_call 안에서 읽는다. (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다)
		SCOPED_LOCK(this->m_lockable_send_call)
		{
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
}

void net::io::sendable::Ntcp_async_gather::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - reset send buffer (m_queue_sending : 보내는 thread가 WSASend에서 돌아온 뒤에 지운다)
		this->m_queue_pending.clear();
		SCOPED_LOCK(this->m_lockable_send_call)
		{
			this->m_queue_sending.clear();
		}

		// - reset flag
		this->m_flag_sending = false;

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
