#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;


net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::~QUEUE_SEND()
{
	if (this->allocated != nullptr)
	{
		::free(this->allocated);
		this->allocated = nullptr;
	}
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_prepare()
{
	// 1) reset buffer
	this->buffer_reset();

	// declare) 
	static const size_t MIN_CAPACITY = 4;

	// 2) shrink buffer size
	if (this->capacity_node != MIN_CAPACITY)
	{
		// check)
		CGASSERT_ERROR(this->count_node == 0);

		// - alloc new buffer
		auto allocated_new = reinterpret_cast<char*>(::malloc((sizeof(WSABUF) + sizeof(NODE*)) * (MIN_CAPACITY + 2)));

		// check)
		if(allocated_new == nullptr)
			throw throwable::failure(eRESULT::BAD_ALLOC);

		// - free allocated buffer
		if (this->allocated != nullptr)
			::free(this->allocated);

		// - set buffer
		this->allocated = allocated_new;
		this->array_wsabuffer = reinterpret_cast<WSABUF*>(allocated_new);
		this->array_node = reinterpret_cast<NODE*>(allocated_new + (sizeof(WSABUF) * (MIN_CAPACITY + 2)));
		this->capacity_node = MIN_CAPACITY;
	}
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_expand()
{
	// 주의) 할당개수를 _count에 최소 1의 마진을 줘야 한다. 최대 _count + 1 까지 억세스를 할 수 있기 때문이다.

	// check)
	CGASSERT_ERROR(this->capacity_node != 0);
	CGASSERT_ERROR(this->allocated != nullptr);

	// 1) capacity x 4
	this->capacity_node <<= 2;

	// 1) alloc buffer
	auto allocated_new = reinterpret_cast<char*>(::malloc((sizeof(WSABUF) + sizeof(NODE*)) * (capacity_node + 2)));

	// check)
	if(allocated_new == nullptr)
		throw throwable::failure(eRESULT::BAD_ALLOC);

	auto array_buffer_new = reinterpret_cast<WSABUF*>(allocated_new);
	auto array_node_new = reinterpret_cast<NODE*>(allocated_new + (sizeof(WSABUF) * (capacity_node + 2)));

	// 2) copy
	if (count_node != 0)
	{
		memcpy(array_buffer_new, this->array_wsabuffer, sizeof(WSABUF) * this->count_node);
		memcpy(array_node_new, this->array_node, sizeof(NODE*) * this->count_node);
	}

	// 3) free 
	if (this->allocated != nullptr)
	{
		::free(this->allocated);
	}

	// 4) apply
	this->allocated = allocated_new;
	this->array_wsabuffer = array_buffer_new;
	this->array_node = array_node_new;
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_free() noexcept
{
	// 1) reset value
	this->buffer_reset();

	// 2) reset buffer & values
	if (this->allocated != nullptr)
	{
		::free(this->allocated);
	}
	this->allocated = nullptr;
	this->array_wsabuffer = nullptr;
	this->array_node = nullptr;
	this->capacity_node = 0;
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_reset() noexcept
{
	// 1) release source
	for (auto piter_node = this->array_node, piter_node_end = this->array_node + this->count_node; piter_node != piter_node_end; ++piter_node)
	{
		// check)
		CGASSERT_FATAL(piter_node->psource != nullptr);

		piter_node->psource->release();
		piter_node->psource = nullptr;
	}

	// 2) reset values
	this->count_node = 0;
	this->bytes_data = 0;
	this->count_message = 0;
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_reset_and_swap(QUEUE_SEND&& _move) noexcept
{
	// check)
	CGASSERT_ERROR(allocated != nullptr);

	// 1) swap buffer info
	SWAP(this->allocated, _move.allocated);
	SWAP(this->array_wsabuffer, _move.array_wsabuffer);
	SWAP(this->array_node, _move.array_node);
	SWAP(this->capacity_node, _move.capacity_node);

	// 2 ) move and reset
	this->count_node = _move.count_node; _move.count_node = 0;
	this->bytes_data = _move.bytes_data; _move.bytes_data = 0;
	this->count_message = _move.count_message; _move.count_message = 0;
}

void net::io::sendable::Ntcp_async_gather_raw::QUEUE_SEND::buffer_push_back(const buffer_view& _buffer, Ireferenceable* _psource, std::size_t _count_messages)
{
	// check)
	CGASSERT_FATAL(_buffer.data() != nullptr);
	CGASSERT_FATAL(this->allocated != nullptr);
	CGASSERT_FATAL(this->array_node != nullptr);

	// - get imex
	auto index = this->count_node;

	if (index != 0)
	{
		// - get pbuffer and pnode
		auto* pbuffer_now = array_wsabuffer + index - 1;

		if ((pbuffer_now->buf + pbuffer_now->len) == _buffer.data())
		{
			// - expand only
			auto size_added = _buffer.size();
			pbuffer_now->len += static_cast<ULONG>(size_added);
			this->bytes_data += size_added;
			this->count_message += _count_messages;

			// return) 
			return;
		}
	}

	// - set buf data
	this->array_wsabuffer[index] = { _buffer.size<ULONG>(), _buffer.data()};
	this->array_node[index].psource = _psource;
	if (_psource != nullptr)
		_psource->add_ref();
	++this->count_node;

	this->bytes_data += _buffer.size();
	this->count_message += _count_messages;
}

bool net::io::sendable::Ntcp_async_gather_raw::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer->data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);
	if(_buffer.data() == nullptr)
		return false;

	// check) _buffer->size()이 0이 아닌가?
	CGASSERT_ERROR(_buffer.size() != 0);
	if(_buffer.size() == 0)
		return false;

	// declare)
	CGSOCKET handle_socket;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 
		if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
			return false;

		// 1) get socket native handle
		handle_socket = this->native_handle();

		// check) 
		if(handle_socket == INVALID_SOCKET)
			return false;

		// check) 
		if (this->m_flag_sending == true)
		{
			// check) queue overflowed?
			if ((this->m_queue_pending.count_node >= m_max_depth_of_send_buffer) || (m_queue_pending.bytes_data >= m_bytes_max_queued) )
			{
				// statistics)
				this->statistics_on_error_send();

				// return)
				return false;
			}

			// - alloc buffer if empty
			if (this->m_queue_pending.is_full())
			{
				this->m_queue_pending.buffer_expand();
			}

			// - queuing to pending queue
			this->m_queue_pending.buffer_push_back(_buffer, _powner, _count_messages);

			// return) 
			return true;
		}

		// 2) queueing
		this->m_flag_sending = true;
	}

	// 3) async send and return result
	return this->process_sendable_async(handle_socket, _buffer, _powner, _count_messages);
}

bool net::io::sendable::Ntcp_async_gather_raw::process_sendable_async(CGSOCKET _handle_socket, const buffer_view& _buffer, Ireferenceable* _psource, std::size_t _count_messages)
{
	// 1) prepare executable
	{
		CGASSERT_ERROR(this->m_queue_sending.count_node == 0);
		CGASSERT_ERROR(this->m_queue_sending.capacity_node != 0);
		CGASSERT_ERROR(this->m_queue_sending.allocated != nullptr);

		this->m_queue_sending.count_node = 1;
		*this->m_queue_sending.array_wsabuffer = { _buffer.size<ULONG>(), _buffer.data() };
		this->m_queue_sending.array_node->psource = _psource;
		this->m_queue_sending.bytes_data = _buffer.size();
		this->m_queue_sending.count_message = _count_messages;

		if (_psource != nullptr)
			_psource->add_ref();
	}

	// 2) 
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 3) Reference count_node를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// declare) 
	DWORD bytes_sent = 0;

	// 4) send~
	auto result = api::socket::send_async(_handle_socket, this->m_queue_sending.array_wsabuffer, 1, &bytes_sent, this->m_pexecutable_send_buffer_list.get());

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

std::size_t net::io::sendable::Ntcp_async_gather_raw::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather_raw::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// check)
	if(this->m_queue_sending.count_message == 0)
		return;

	// declare)
	CGSOCKET handle_socket;

	SCOPED_LOCK_NAMED(this->m_lockable_sending, cs)
	{
		// check) 
		CGASSERT_ERROR(this->m_queue_sending.array_node != nullptr);

		// 1) clear m_queue_sending
		this->m_queue_sending.buffer_reset();

		// declare)
		handle_socket = this->native_handle();

		// check) 
		if (handle_socket == INVALID_SOCKET)
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
			SCOPED_LOCK_LEAVE(cs)

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - linger Option을 Graceful disconnect로 설정한다.
				api::socket::set_linger_option(handle_socket, 0);

				// - 접속을 종료한다.
				api::socket::shutdown(handle_socket, SD_SEND);
			}

			// return) 
			return;
		}

		// 2) move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.buffer_reset_and_swap(std::move(this->m_queue_pending));
	}

	// 4) executable을 준비한다.
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 5) Reference count_node를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// 6) send Overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// declare) 
	DWORD bytes_sent = 0;

	// 7) send~
	auto result = api::socket::send_async(handle_socket, this->m_queue_sending.array_wsabuffer, static_cast<uint32_t>(this->m_queue_sending.count_node), &bytes_sent, this->m_pexecutable_send_buffer_list.get());

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
	this->statistics_on_send_async();
}

void net::io::sendable::Ntcp_async_gather_raw::process_close_sender()
{
	// check)
	bool do_shutdown;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		do_shutdown = this->m_queue_sending.count_node == 0;
	}

	if (do_shutdown)
	{
		auto handle_socket = this->native_handle();

		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(handle_socket, 0);

		// - 접속을 종료한다.
		api::socket::shutdown(handle_socket, SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather_raw::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - reset send buffer
		this->_process_prepare_send_buffers();

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

void net::io::sendable::Ntcp_async_gather_raw::_process_prepare_send_buffers()
{
	this->m_queue_pending.buffer_prepare();
	this->m_queue_sending.buffer_prepare();
}

#endif