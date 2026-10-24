#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;


net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::~QUEUE_SEND()
{
	if (this->allocated != nullptr)
	{
		::free(this->allocated);
		this->allocated = nullptr;
	}
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_prepare()
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

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_expand()
{
	// 주의) 할당개수를 _count에 최소 1의 마진을 줘야 한다. 최대 _count + 1 까지 억세스를 할 수 있기 때문이다.

	// check)
	CGASSERT_FATAL(this->capacity_node != 0);

	// check)
	CGASSERT_FATAL(this->allocated != nullptr);

	// 1) capacity x 4
	this->capacity_node <<= 2;

	// 2) alloc buffer
	auto allocated_new = reinterpret_cast<char*>(::malloc((sizeof(WSABUF) + sizeof(NODE*)) * (this->capacity_node + 2)));

	// check)
	if(allocated_new == nullptr)
		throw throwable::failure(eRESULT::BAD_ALLOC);

	auto array_buffer_new = reinterpret_cast<WSABUF*>(allocated_new);
	auto array_node_new = reinterpret_cast<NODE*>(allocated_new + (sizeof(WSABUF) * (this->capacity_node + 2)));

	// 3) copy
	if (this->count_node != 0)
	{
		memcpy(array_buffer_new, this->array_wsabuffer, sizeof(WSABUF) * this->count_node);
		memcpy(array_node_new, this->array_node, sizeof(NODE*) * this->count_node);
	}

	// 4) free 
	if (this->allocated != nullptr)
		::free(this->allocated);

	// 5) apply
	this->allocated = allocated_new;
	this->array_wsabuffer = array_buffer_new;
	this->array_node = array_node_new;
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_free() noexcept
{
	// 1) reset value
	this->buffer_reset();

	// 2) reset buffer & values
	if (allocated != nullptr)
		::free(this->allocated);

	this->allocated = nullptr;
	this->array_wsabuffer = nullptr;
	this->array_node = nullptr;
	this->capacity_node = 0;
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_reset() noexcept
{
	// 1) release source
	for (auto piter_node = array_node, piter_node_end = array_node + count_node; piter_node != piter_node_end; ++piter_node)
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

	// 3) ...
	this->bytes_remained = 0;
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_reset_and_swap(QUEUE_SEND&& _move) noexcept
{
	// check)
	CGASSERT_FATAL(allocated != nullptr);

	// 1) swap buffer info
	SWAP(this->allocated, _move.allocated);
	SWAP(this->array_wsabuffer, _move.array_wsabuffer);
	SWAP(this->array_node, _move.array_node);
	SWAP(this->capacity_node, _move.capacity_node);

	// 2 ) move and reset
	this->count_node = _move.count_node; _move.count_node = 0;
	this->bytes_data = _move.bytes_data; _move.bytes_data = 0;
	this->count_message = _move.count_message; _move.count_message = 0;

	// 3) move values
	this->bytes_remained = _move.bytes_remained; _move.bytes_remained = 0;
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::QUEUE_SEND::buffer_push_back(const buffer_view& _buffer, Ireferenceable* _powner, std::size_t _count_messages)
{
	// check)
	CGASSERT_FATAL(this->allocated != nullptr);

	// check)
	CGASSERT_FATAL(this->array_wsabuffer != nullptr);

	// check)
	CGASSERT_FATAL(this->array_node != nullptr);

	// 1) ...
	auto bytes_requested = _buffer.size<ULONG>();

	if (this->count_node > COUNT_SEND_GATHER_START_DEPTH && bytes_requested <= BYTES_SEND_GATHER_BUFFER_MIN)
	{
		// - get pbuffer and pnode
		auto* pbuffer_now = array_wsabuffer + this->count_node - 1;

		// - is buffer enough to copy (no => create new)
		if (this->bytes_remained < bytes_requested)
		{
			// - shift count
			auto shift_count = static_cast<uint32_t>(this->count_node - COUNT_SEND_GATHER_START_DEPTH - 1) * 2;

			// - saturate_max
			SATURATE_MAX(shift_count, BYTES_SEND_GATHER_SHIFT_MAX);

			// - get buffer size
			auto size_buffer = (BYTES_SEND_GATHER_BUFFER_MIN << shift_count);

			// check)
			CGASSERT_FATAL(size_buffer <= BYTES_SEND_GATHER_BUFFER_MAX);

			// - alloc new buffer
			auto buf_new = alloc_shared_buffer(size_buffer);

			// - queue allocated buffer
			*(this->array_wsabuffer + this->count_node) = { 0, buf_new.data() };
			*(this->array_node + this->count_node) = NODE{ buf_new.get_source() };
			buf_new.get_source()->add_ref();
			++this->count_node;
			++pbuffer_now;
			this->bytes_remained = size_buffer;
		}

		// check)
		CGASSERT_ERROR(this->bytes_remained >= bytes_requested)

		// - copy~
		memcpy(pbuffer_now->buf + pbuffer_now->len, _buffer.data(), bytes_requested);
		pbuffer_now->len += static_cast<ULONG>(bytes_requested);
		this->bytes_remained -= bytes_requested;
	}
	// - BYTES_SEND_GATHER_BUFFER_MIN보다 크면 버퍼 통째로 추가한다.
	else
	{
		// - check 
		if (this->count_node != 0)
		{
			// - get pbuffer and pnode
			auto* pbuffer_now = this->array_wsabuffer + this->count_node - 1;
			auto* pnode_now = this->array_node + this->count_node - 1;

			if (pnode_now->psource == _powner && (pbuffer_now->buf + pbuffer_now->len) == _buffer.data())
			{
				// - expand only
				pbuffer_now->len += bytes_requested;
				this->bytes_data += bytes_requested;
				this->count_message += _count_messages;

				// check)
				CGASSERT_FATAL(this->bytes_remained == 0);

				// return) 
				return;
			}
		}

		// - ...
		*(array_wsabuffer + this->count_node) = { _buffer.size<ULONG>(), _buffer.data() };
		*(array_node + this->count_node) = NODE{ _powner };
		if (_powner != nullptr)
			_powner->add_ref();
		++this->count_node;
		this->bytes_remained = 0;
	}

	// - ...
	this->bytes_data += bytes_requested;
	this->count_message += _count_messages;
}

bool net::io::sendable::Ntcp_async_gather_buffered_raw::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_FATAL(_buffer.data() != nullptr);

	// check) _buffer.size()이 0이 아닌가?
	CGASSERT_FATAL(_buffer.size() != 0);

	// check) data가 nullptr이거나 size가 0이면 보내지 않는다. (Release)
	if(_buffer.data() == nullptr || _buffer.size() == 0)
		return false;

	// check) 
	if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// 1) get socket native handle
	auto handle_socket = this->native_handle();

	// check) 
	if(handle_socket == INVALID_SOCKET)
		return false;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 2) 전송 처리 중(m_flag_sending이 signal된 상태(true))라면 queuing 처리만 하고 끝낸다.
		if (this->m_flag_sending == true)
		{
			// check) queue overflowed?
			if ((this->m_queue_pending.count_node >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= this->m_bytes_max_queued))
			{
				// statistics)
				this->statistics_on_error_send();

				// return)
				return false;
			}

			// - alloc buffer if empty
			if (this->m_queue_pending.is_full())
				this->m_queue_pending.buffer_expand();

			// - queuing to pending_queue
			this->m_queue_pending.buffer_push_back(_buffer, _powner, _count_messages);

			// return) 
			return true;
		}

		// 3) set flag_sending true
		this->m_flag_sending = true;
	}

	// 1) queuing to sending buffer
	{
		// check) 
		CGASSERT_FATAL(this->m_queue_sending.count_node == 0);
		CGASSERT_FATAL(this->m_queue_sending.capacity_node != 0);
		CGASSERT_FATAL(this->m_queue_sending.allocated != nullptr);

		this->m_queue_sending.count_node = 1;
		*this->m_queue_sending.array_wsabuffer = { _buffer.size<ULONG>(), _buffer.data()};
		this->m_queue_sending.array_node->psource = _powner;
		this->m_queue_sending.bytes_data = _buffer.size();
		this->m_queue_sending.count_message = _count_messages;

		if (_powner != nullptr)
			_powner->add_ref();
	}

	// 4) async send and return result
	return this->process_sendable_async(handle_socket, this->m_queue_sending.array_wsabuffer, 1);
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// check)
	if(this->m_queue_sending.count_message == 0)
		return;

	// check) 
	CGASSERT_FATAL(this->m_queue_sending.array_node != nullptr);

	SCOPED_LOCK_NAMED(this->m_lockable_sending, cs)
	{
		// 1) clear seding queue
		this->m_queue_sending.buffer_reset();

		// declare)
		auto handle = this->native_handle();

		// check) 
		if (handle == INVALID_SOCKET)
		{
			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// check) return if no pended data
		if (this->m_queue_pending.count_node == 0)
		{
			// - reset queued
			this->m_flag_sending = false;

			// - lock leave
			SCOPED_LOCK_LEAVE(cs)

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(handle, 0);

				// - send shutdown
				api::socket::shutdown(handle, SD_SEND);
			}

			// return)
			return;
		}

		// - move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.buffer_reset_and_swap(std::move(this->m_queue_pending));
	}

	// 3) send queued buffers
	this->process_sendable_async(native_handle(), m_queue_sending.array_wsabuffer, static_cast<uint32_t>(m_queue_sending.count_node));
}

bool net::io::sendable::Ntcp_async_gather_buffered_raw::process_sendable_async(CGSOCKET _handle_socket, LPWSABUF _array_wsabuffer, uint32_t _count_buffer)
{
	// 1) prepare executable
	this->m_pexecutable_send_buffer_list->m_psendable = this;

	// 2) reference count를 증가시킨다.
	this->m_pexecutable_send_buffer_list->add_ref();

	// statistics) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// declare) 
	DWORD bytes_sent = 0;

	// 3) send~
	auto result = api::socket::send_async(_handle_socket, _array_wsabuffer, _count_buffer, &bytes_sent, this->m_pexecutable_send_buffer_list.get());

	// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
	if (result == false && api::get_last_error() != WSA_IO_PENDING)
	{
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

std::size_t net::io::sendable::Ntcp_async_gather_buffered_raw::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::process_close_sender()
{
	// check)
	bool do_shutdown;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		do_shutdown = this->m_queue_sending.count_node == 0;
	}

	if (do_shutdown)
	{
		// - get handle
		auto handle = this->native_handle();

		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(handle, 0);

		// - 접속을 종료한다.
		api::socket::shutdown(handle, SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		this->_process_prepare_send_buffers();
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());

	// 3) reset executable
	if (this->m_pexecutable_send_buffer_list.empty())
		this->m_pexecutable_send_buffer_list = make_object<executable_send_buffer_list>();
}

void net::io::sendable::Ntcp_async_gather_buffered_raw::_process_prepare_send_buffers()
{
	this->m_flag_sending = false;

	this->m_queue_pending.buffer_prepare();
	this->m_queue_sending.buffer_prepare();
}

#endif
