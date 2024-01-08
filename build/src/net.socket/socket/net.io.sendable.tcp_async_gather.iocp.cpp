#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

bool net::io::sendable::Ntcp_async_gather::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_async_gather::process_sendable')"sv));

	// declare)
	CGSOCKET handle_socket;

	scoped_lock(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

		// 1) get socket native handle
		handle_socket = this->native_handle();

		// check) if invalid socket, return false 
		RETURN_IF(handle_socket == INVALID_SOCKET, false);

		// check) 
		if (this->m_flag_sending == true)
		{
			// check) quque overflowed?
			if ((this->m_queue_pending.array_buffer.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= m_bytes_max_queued))
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				THROW_IF(this->exception_on_send_overflow(), throwable::failure(eRESULT::OVERFLOW_ERROR, 0, "occure send overflow"sv));

				// return)
				return false;
			}

			// - queuing to pending queue
			this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

			// return) 
			return true;
		}

		// 2) queueing
		this->m_flag_sending = true;
	}

	// 3) async send and return result
	return this->process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_messages);
}

bool net::io::sendable::Ntcp_async_gather::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	// check)
	CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty());
	CGASSERT_ERROR(this->m_queue_sending.array_node.empty());
	CGASSERT_ERROR(this->m_queue_sending.bytes_data == 0);
	CGASSERT_ERROR(this->m_queue_sending.count_message == 0);

	// 1) queuing _buffer for sending
	{
		this->m_queue_sending.array_buffer.emplace_back(_buffer.size<ULONG>(), _buffer.data());
		this->m_queue_sending.array_node.emplace_back(std::move(_powner));
		this->m_queue_sending.bytes_data = _buffer.size();
		this->m_queue_sending.count_message = _count_messages;
	}

	// 2) 
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 3) Reference count_node를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// 4) send~
	auto result = api::socket::send_async(_handle_socket, &this->m_queue_sending.array_buffer.front(), 1, nullptr, this->m_pexecutable_send_buffer_list.get());
		
	// check)
	if(result == false)
	{
		// statistics)
		this->statistics_on_error_send();

		// statistics)
		this->statistics_on_decrease_async_sending();

		// - reset 
		this->m_pexecutable_send_buffer_list->m_psendable.reset();

		// - rollback (release)
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

std::size_t net::io::sendable::Ntcp_async_gather::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// check) 이미 destroy 된경우 더이상 처리하지 않고 그냥 끝낸다. (destroy되어 clear 처리된 경우 bytes_data가 0일 것이다.)
	RETURN_IF(this->m_queue_sending.bytes_data == 0);

	scoped_lock_named(this->m_lockable_sending, cs)
	{
		// 1) clear m_queue_sending
		this->m_queue_sending.clear();

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

			// - lock leave
			scoped_lock_leave(cs)

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - linger Option을 Graceful disconnect로 설정한다.
				api::socket::set_linger_option(this->native_handle(), 0);

				// - 접속을 종료한다.
				api::socket::shutdown(this->native_handle(), SD_SEND);
			}

			// return) 
			return;
		}

		// 2) move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.reset_and_swap(std::move(this->m_queue_pending));
	}

	// 4) executable을 준비한다.
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 5) Reference count_node를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// 6) send Overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// 7) send async
	auto result = api::socket::send_async(this->native_handle(), &this->m_queue_sending.array_buffer.front(), static_cast<DWORD>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

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

		// - process reset sendable
		this->process_reset_sendable();

		// return) fail to send
		return;
	}

	// statistics) 
	statistics_on_send_async();
}

void net::io::sendable::Ntcp_async_gather::process_close_sender()
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
		api::socket::shutdown(native_handle(), SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather::process_reset_sendable() noexcept
{
	// 1) ...
	scoped_lock(this->m_lockable_sending)
	{
		// - reset send buffer
		this->m_queue_pending.clear();
		this->m_queue_sending.clear();

		// - reset flag
		this->m_flag_sending = false;
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