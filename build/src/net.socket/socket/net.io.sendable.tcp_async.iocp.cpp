#include "pch.h"

#if defined(_WINSOCK2API_)

#include "net.io.sendable.executable_send_buffer.h"

using namespace CGDK;

net::io::sendable::Ntcp_async::Ntcp_async()
{
	this->maximum_depth_of_send_buffer(256);
}

net::io::sendable::Ntcp_async::~Ntcp_async() noexcept
{
}

bool net::io::sendable::Ntcp_async::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer->size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_async::process_sendable')"sv));

	// check) if socket is not connected, return false (after lock)
	RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

	// 1) get socket native handle
	auto handle_socket = this->native_handle();

	// check) if invalid socket, return false 
	RETURN_IF(handle_socket == INVALID_SOCKET, false);

	// 2) check ovaerlapped counting 
	scoped_lock(m_lockable_sending)
	{
		// check) quque overflowed?
		if ((this->m_count_pended >= this->m_max_depth_of_send_buffer) || (this->m_bytes_pended >= this->m_bytes_max_queued))
		{
			// statistics)
			this->statistics_on_error_send();

			// check)
			THROW_IF(this->exception_on_send_overflow(), throwable::failure(eRESULT::OVERFLOW_ERROR, 0, "occure send overflow"sv));

			// return)
			return false;
		}

		// - 
		++this->m_count_pended;
		this->m_bytes_pended += _buffer.size();
	}

	// 2) sexcutable send_buffer를 생성한다.
	auto pexecutable_send = make_object<executable_send_buffer>();

	// 3) set executalbe_send
	pexecutable_send->m_psendable = this;
	pexecutable_send->m_powner = std::move(_powner);
	pexecutable_send->m_count_message = _count_message;
	pexecutable_send->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };

	// 3) increase reference count
	pexecutable_send->add_ref();

	// statistics)
	this->statistics_on_increase_async_sending();

	// 4) send async
	auto result = api::socket::send_async(handle_socket, &pexecutable_send->m_wsabuf, 1, nullptr, pexecutable_send.get());

	// check)
	if (result == false)
	{
		// statistics)
		this->statistics_on_error_send();

		// statistics)
		this->statistics_on_decrease_async_sending();

		// - reset executable
		pexecutable_send->m_psendable.reset();

		// - rollback (release)
		pexecutable_send->release();

		// - don't execute 'process reset sendable'

		// return) fail to send
		return false;
	}

	// statistics) 
	this->statistics_on_send_async();

	// return)
	return true;
}

void net::io::sendable::Ntcp_async::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _count_message)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? _count_message : 0);

	// - 
	scoped_lock(this->m_lockable_sending)
	{
		--this->m_count_pended;
		this->m_bytes_pended -= _transferred_bytes;
	}

	// statistics) 
	this->statistics_on_send_async();
}

void net::io::sendable::Ntcp_async::process_close_sender()
{
	// 1) linger Option을 Graceful disconnect로 설정한다.
	api::socket::set_linger_option(this->native_handle(), 0);

	// 2) 접속을 종료한다.
	api::socket::shutdown(this->native_handle(), SD_SEND);
}

void net::io::sendable::Ntcp_async::process_reset_sendable() noexcept
{
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());

	this->m_count_pended = 0;
	this->m_bytes_pended = 0;
}

std::size_t net::io::sendable::Ntcp_async::process_get_sending_queued_bytes() const noexcept
{
	return this->m_bytes_pended;
}


#endif
