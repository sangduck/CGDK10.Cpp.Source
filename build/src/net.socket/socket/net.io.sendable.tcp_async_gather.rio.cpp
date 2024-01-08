#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;


void net::io::sendable::Ntcp_async_gather_rio::QUEUE_SEND::buffer_alloc(std::size_t _count)
{
	// 1) get alloc count - 
	//    주의) 할당갯수를 _count에 최소 1의 마진을 줘야 한다. 최대 _count + 1 까지 억세스를 할수 있기 때문이다.
	auto count_alloc = _count + 2;

	// 2) alloc buffer
	this->allocated = alloc_shared_buffer((sizeof(WSABUF) + sizeof(NODE)) * count_alloc);

	// 3) set value
	this->array_buffer = allocated.data<WSABUF>();
	this->array_node = allocated.data<NODE>(sizeof(WSABUF) * count_alloc);
	this->count_node = 0;

	// 3) reset value
	this->bytes_data = 0;
	this->count_message = 0;
}

void net::io::sendable::Ntcp_async_gather_rio::QUEUE_SEND::buffer_free() noexcept
{
	// 1) release source
	for (auto piter_node = this->array_node, piter_node_end = this->array_node + this->count_node; piter_node != piter_node_end; ++piter_node)
	{
		// check)
		CGASSERT_ERROR(piter_node->powner.exist());
		piter_node->powner.reset();
	}

	// 2) reset buffer & values
	this->allocated.clear();
	this->array_buffer = nullptr;
	this->array_node = nullptr;
	this->count_node = 0;
	this->bytes_data = 0;
	this->count_message = 0;
}

void net::io::sendable::Ntcp_async_gather_rio::QUEUE_SEND::buffer_move(QUEUE_SEND&& _move) noexcept
{
	// check)
	CGASSERT_ERROR(this->allocated.empty());

	this->allocated = std::move(_move.allocated);
	this->array_buffer = _move.array_buffer; _move.array_buffer = nullptr;
	this->array_node = _move.array_node; _move.array_node = nullptr;
	this->count_node = _move.count_node; _move.count_node = 0;
	this->bytes_data = _move.bytes_data; _move.bytes_data = 0;
	this->count_message = _move.count_message; _move.count_message = 0;
}

bool net::io::sendable::Ntcp_async_gather_rio::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_async_gather_rio::process_sendable')"sv));

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
			if ((this->m_queue_pending.count_node >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= this->m_bytes_max_queued) )
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				THROW_IF(this->exception_on_send_overflow(), throwable::failure(eRESULT::OVERFLOW_ERROR, 0, "occure send overflow"sv));

				// return)
				return false;
			}

			// - queuing to pending queue
			{
				if (this->m_ptr_queued_buffer_tail == _buffer.data())
				{
					// - expand only
					auto index = this->m_queue_pending.count_node - 1;
					auto size_added = _buffer.size();
					this->m_queue_pending.array_buffer[index].len += static_cast<ULONG>(size_added);
					this->m_queue_pending.bytes_data += size_added;
					this->m_queue_pending.count_message += _count_message;
					this->m_ptr_queued_buffer_tail += size_added;

					// return) 
					return true;
				}

				// - get imex
				auto index = this->m_queue_pending.count_node;

				// - create new if empty
				if (index == 0)
				{
					this->m_queue_pending.buffer_alloc(this->m_max_depth_of_send_buffer);
				}

				// check)
				CGASSERT_ERROR(this->m_queue_pending.array_buffer != nullptr);

				// - set buf data
				this->m_queue_pending.array_buffer[index] = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };
				this->m_queue_pending.array_node[index].powner = std::move(_powner);
				++this->m_queue_pending.count_node;
				this->m_queue_pending.bytes_data = _buffer.size();
				this->m_queue_pending.count_message = _count_message;
				this->m_ptr_queued_buffer_tail = _buffer.back();
			}

			// return) 
			return true;
		}

		// 2) queueing
		this->m_flag_sending = true;
	}

	// 3) async send and return result
	return process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_message);
}

bool net::io::sendable::Ntcp_async_gather_rio::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message)
{
	// 1) alloc 'executable_send_buffer_list' object
	auto pexecutable = make_object<executable_send_buffer_list>();

	// 2) queuing to sending buffer
	{
		this->m_queue_sending.count_node = 1;

		this->m_queue_sending.allocated.clear();
		this->m_queue_sending.array_buffer = &pexecutable->m_wsabuf;
		pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };
		this->m_queue_sending.array_node = &this->m_temp_queued_send;

		this->m_queue_sending.bytes_data = _buffer.size();
		this->m_queue_sending.count_message = _count_message;
		this->m_temp_queued_send.powner = std::move(_powner);
	}

	// 3) 
	pexecutable->m_psendable = this;

	// 4) Reference count_node를 증가시킨다.
	pexecutable->add_ref();

	// 5) set buffer (for reserve during send operation)
	pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// 6) send~
	auto result = api::socket::send_async(_handle_socket, &pexecutable->m_wsabuf, 1, nullptr, pexecutable.get());
		
	// check)
	if(result == false)
	{
		// statistics)
		this->statistics_on_error_send();

		// statistics)
		this->statistics_on_decrease_async_sending();

		// - rollback (release)
		pexecutable->release();

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

std::size_t net::io::sendable::Ntcp_async_gather_rio::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather_rio::_process_prepare_send_buffers()
{
	this->m_queue_pending.buffer_free();
	this->m_queue_sending.buffer_free();
	this->m_ptr_queued_buffer_tail = nullptr;

	this->m_temp_queued_send.powner.reset();
}

void net::io::sendable::Ntcp_async_gather_rio::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_message*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// 1) clear m_queue_sending
	this->m_queue_sending.buffer_free();

	scoped_lock_named(this->m_lockable_sending, cs)
	{
		// check) 
		if (this->native_handle() == INVALID_SOCKET)
		{
			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// check)
		if (this->m_queue_pending.count_node == 0)
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

		// 2) move buffer
		this->m_queue_sending.buffer_move(std::move(m_queue_pending));

		// 3) Queueing 전송 관련 변수들을 초기화한다.
		this->m_ptr_queued_buffer_tail = nullptr;
	}

	{
		// 4) alloc 'executable_send_buffer_list' object
		auto pexecutable = make_object<executable_send_buffer_list>();

		// 5) executable을 준비한다.
		pexecutable->m_psendable = this;

		// 6) Reference count_node를 증가시킨다.
		pexecutable->add_ref();

		// 7) send Overlapped 카운트를 하나 증가시킨다.
		this->statistics_on_increase_async_sending();

		// 8) send~
		auto result = api::socket::send_async(this->native_handle(), this->m_queue_sending.array_buffer, static_cast<DWORD>(this->m_queue_sending.count_node), nullptr, pexecutable.get());

		// check)
		if (result == false)
		{
			// statistics)
			this->statistics_on_error_send();

			// statistics)
			this->statistics_on_decrease_async_sending();

			// - rollback (release)
			pexecutable->release();

			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}
	}

	// statistics) 
	statistics_on_send_async();
}

void net::io::sendable::Ntcp_async_gather_rio::process_close_sender()
{
	// check)
	bool do_shutdown;

	scoped_lock(this->m_lockable_sending)
	{
		do_shutdown = this->m_queue_sending.count_node == 0;
	}

	if (do_shutdown)
	{
		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(this->native_handle(), 0);

		// - 접속을 종료한다.
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather_rio::process_reset_sendable() noexcept
{
	// 1) prepare & reset sending status
	scoped_lock(this->m_lockable_sending)
	{
		// - prepare and reset send buffer
		this->_process_prepare_send_buffers();

		// - reset send flag
		this->m_flag_sending = false;
	}

	// 2) reset socket option
	maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
}

#endif
