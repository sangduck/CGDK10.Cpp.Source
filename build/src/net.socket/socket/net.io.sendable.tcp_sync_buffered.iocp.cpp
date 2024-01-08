#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

bool net::io::sendable::Ntcp_sync_buffered::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// ----------------------------------------------------------------------------
	// 섦명) 전송을 요청한다.
	//       - 전송하려는 데이터가 0 byte면 전송을 진행하지 않고 실패를 리턴한다.
	//       - buffer에 오류가 있을 경우 전송을 진행하지 않고 실패를 리턴한다.
	//       - 접속이 종료된 상태거나 socket이 할댕해제 된 상태일 경우 전송을 진행하지 않고 실패를 리턴한다.
	//       - 이미 전송이 진행 중일 경우 전송할 데이터를 m_queue_pending에 queuing만 한 후 종료한다.
	//         queuing 버퍼의 수가 최대 버퍼 수(m_max_depth_of_send_buffer)를 초과할 경우 실패를 리턴한다.
	//       - 전송 중인 데이터가 없을 경우 새로운 비동기 전송을 요청한다.
	// ----------------------------------------------------------------------------

	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.size()이 0이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 nullptr이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 nullptr이 아닌가? (size가 0이 아닌데 data가 nullptr이라면 심각한 오류 이므로 exception을 던진다.)
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_sync_buffered::process_sendable')"sv));

	// declare)
	CGSOCKET handle_socket;

	scoped_lock(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

		// 1) get socket native handle (실패확률을 최소화 하기 위해 lock을 건 후에 native handle을 얻는다.)ㄴ
		handle_socket = this->native_handle();

		// check) aleady released socket handle?
		RETURN_IF(handle_socket == INVALID_SOCKET, false);

		// check) quque overflowed? (최대 queuing 한계를 초과했을 경우 false를 리턴한다.)
		if ((this->m_queue_pending.array_buffer.size() >= this->m_max_depth_of_send_buffer) || (this->m_bytes_max_queued >= this->m_bytes_max_queued))
		{
			// statistics)
			this->statistics_on_error_send();

			// check)
			THROW_IF(this->exception_on_send_overflow(), throwable::failure(eRESULT::OVERFLOW_ERROR, 0, "occure send overflow"sv));

			// return)
			return false;
		}

		// 2) queued (전송중일 경우 (m_flag_sending == true)라면 그냥 queuing만 하고 종료한다.
		if (this->m_flag_sending == true)
		{
			// - queuing to pending_queue
			this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_message);

			// return) 
			return true;
		}

		// 3) queueing
		this->m_flag_sending = true;
	}

	// 4-1) send sync
	if (_buffer.size() < BYTES_SEND_ASYNC_THRESHOLD)
	{
		auto transferred_bytes = api::socket::send (handle_socket, _buffer.data(), static_cast<int>(_buffer.size()));

		if (transferred_bytes != _buffer.size())
		{
			// - get last error
			auto error_code = api::get_last_error();

			// - set socket error code
			this->m_last_socket_error = error_code;

			// check) 
			if (error_code != WSAEWOULDBLOCK)
			{
				// - roll back
				scoped_lock(this->m_lockable_sending)
				{
					// - clear buffers
					this->m_queue_pending.clear();
					this->m_queue_sending.clear();

					// - set flag_seding false
					this->m_flag_sending = false;
				}

				// return) 
				return false;
			}

			// check)
			CGASSERT_ERROR(transferred_bytes <= 0);

			// - async send and return result
			return this->process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_message);
		}

		// 5) complete sendable
		this->process_complete_sendable(1, transferred_bytes, _count_message);

		// return)
		return true;
	}
	// 4-2) send async
	else
	{
		// - async send and return result
		return this->process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_message);
	}
}

bool net::io::sendable::Ntcp_sync_buffered::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message)
{
	// ----------------------------------------------------------------------------
	// 섦명) 비동기 전송 요청한다.
	//       - m_queue_sending에 전송 정보를 저장한 후 비동기 전송을 요청한다.
	//       - 실패할 경우 false를 리턴한다.
	// ----------------------------------------------------------------------------
	// 1) queuing to sending buffer
	this->m_queue_sending.array_buffer.emplace_back(_buffer.size<ULONG>(), _buffer.data());
	this->m_queue_sending.array_node.emplace_back(std::move(_powner));
	this->m_queue_sending.bytes_data = _buffer.size();
	this->m_queue_sending.count_message = _count_message;

	// 2) prepare executable
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 3) Reference count를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// 4) send~
	auto result = api::socket::send_async(_handle_socket, &this->m_queue_sending.array_buffer.front(), 1, nullptr, this->m_pexecutable_send_buffer_list.get());

	// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
	if (result == false && api::get_last_error() != WSA_IO_PENDING)
	{
		// - set last error
		this->m_last_socket_error = api::get_last_error();

		// statistics)
		this->statistics_on_error_send();

		// - send overlapped 카운트를 하나 줄인다.
		this->statistics_on_decrease_async_sending();

		// - reset executable
		this->m_pexecutable_send_buffer_list->m_psendable.reset();

		// - Release한다.
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

void net::io::sendable::Ntcp_sync_buffered::process_complete_sendable(uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _count_message)
{
	// ----------------------------------------------------------------------------
	// 섦명) send completion (전송 완료 후 처리)
	//       - 전송 완료 후 전송 통계를 처리한다.
	//       - 완료되기 전 이미 파괴했을 수 있으므로 그럴 경우 그냥 리턴시킨다.
	//       - 완료되기 전 이미 접속이 종료되었을 수 있으므로 그럴 경우 reset후 종료 처리한다.
	//       - 정성적으로 처리 완료된 경우 pending된 buffer들이 있는지 확인한 후 있을 경우 다시 전송처리한다.
	// ----------------------------------------------------------------------------
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? (this->m_queue_sending.count_message + _count_message) : 0);

	// check) 이미 destroy 된경우 더이상 처리하지 않고 그냥 끝낸다. (destroy되어 clear 처리된 경우 bytes_data가 0일 것이다.)
	RETURN_IF(this->m_queue_sending.bytes_data == 0);

	scoped_lock_named(this->m_lockable_sending, cs)
	{
		// 1) clear seding queue
		this->m_queue_sending.clear();

		// check) 
		if (this->native_handle() == INVALID_SOCKET)
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

			// - lock leave
			scoped_lock_leave(cs)

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(native_handle(), 0);

				// - send shutdown
				api::socket::shutdown(native_handle(), SD_SEND);
			}

			// return)
			return;
		}

		// - move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.reset_and_swap(std::move(this->m_queue_pending));
	}

	// 4) send queued buffers
	{
		// - executable을 준비한다.
		this->m_pexecutable_send_buffer_list->m_psendable = this;

		// - increase reference count
		this->m_pexecutable_send_buffer_list->add_ref();

		// statistics)
		this->statistics_on_increase_async_sending();

		// - send~
		auto result = api::socket::send_async(native_handle(), &this->m_queue_sending.array_buffer.front(), static_cast<uint32_t>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

		// check) result가 false이고 WSA_IO_PENDING이 아닐 때만 전송 실패이다.
		if (result == false)
		{
			// statistics)
			this->statistics_on_error_send();

			// statistics)
			this->statistics_on_decrease_async_sending();

			// - rollback (reset executable)
			this->m_pexecutable_send_buffer_list->m_psendable.reset();

			// - rollback (release reference counting)
			this->m_pexecutable_send_buffer_list->release();

			// - rollback (process reset sendable)
			this->process_reset_sendable();

			// return) fail to send
			return;
		}
	}

	// statistics) 
	this->statistics_on_send_async();
}

void net::io::sendable::Ntcp_sync_buffered::process_close_sender()
{
	// check)
	bool do_shutdown;

	scoped_lock(this->m_lockable_sending)
	{
		do_shutdown = this->m_queue_sending.array_buffer.empty();
	}

	if (do_shutdown)
	{
		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(this->native_handle(), 0);

		// - 접속을 종료한다.
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}
}

void net::io::sendable::Ntcp_sync_buffered::process_reset_sendable() noexcept
{
	// 1) prepare & reset sending status
	scoped_lock(this->m_lockable_sending)
	{
		// - prepare and reset send buffer
		this->m_queue_pending.clear();
		this->m_queue_sending.clear();

		// - reset flag 'sending'
		this->m_flag_sending = false;
	}

	// 2) reset socket option
	maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());

	// 3) prepare 'm_pexecutable_send_buffer_list'
	if (this->m_pexecutable_send_buffer_list.empty())
	{
		this->m_pexecutable_send_buffer_list = make_object<executable_send_buffer_list>();
	}
}

#endif
