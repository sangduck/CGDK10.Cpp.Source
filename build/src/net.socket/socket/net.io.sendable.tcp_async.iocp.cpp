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
	CGASSERT_FATAL(_buffer.size() != 0);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// check) if socket is not connected, return false
	if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// 1) executable send_buffer를 생성한다. (lock 밖)
	auto pexecutable_send = make_object<executable_send_buffer>();

	// 2) set executalbe_send
	pexecutable_send->m_psendable = this;
	pexecutable_send->m_powner = std::move(_powner);
	pexecutable_send->m_count_message = _count_message;
	pexecutable_send->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };

	SCOPED_LOCK(m_lockable_sending)
	{
		// - handle은 lock 안에서 읽고 send까지 lock 안에서 한다.
		//   (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
		const auto handle_socket = this->native_handle();

		// check) if invalid socket, return false
		if (handle_socket == INVALID_SOCKET)
		{
			pexecutable_send->m_psendable.reset();
			return false;
		}

		// check) queue overflowed?
		if ((this->m_count_pended >= this->m_max_depth_of_send_buffer) || (this->m_bytes_pended >= this->m_bytes_max_queued))
		{
			// - reset executable
			pexecutable_send->m_psendable.reset();

			// statistics)
			this->statistics_on_error_send();

			// check)
			this->process_on_send_overflow();

			// return)
			return false;
		}

		// 3) count pended
		++this->m_count_pended;
		this->m_bytes_pended += _buffer.size();

		// 4) increase reference count
		pexecutable_send->add_ref();

		// statistics)
		this->statistics_on_increase_async_sending();

		// 5) send async
		auto result = api::socket::send_async(handle_socket, &pexecutable_send->m_wsabuf, 1, nullptr, pexecutable_send.get());

		// check)
		if (result == false)
		{
			// - rollback pended (이전에는 되돌리지 않아 실패가 쌓이면 overflow로 처리되었다)
			--this->m_count_pended;
			this->m_bytes_pended -= _buffer.size();

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
	}

	// return)
	return true;
}

void net::io::sendable::Ntcp_async::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _count_message)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? _count_message : 0);

	// - 
	SCOPED_LOCK(this->m_lockable_sending)
	{
		--this->m_count_pended;
		this->m_bytes_pended -= _transferred_bytes;
	}

	// statistics) the overlapped send completed (statistics_on_send_async() was called here : the sends counted twice and
	//             statistics_get_async_sending() never went back to 0 : statistics_get_now_async_io() of the socket stayed > 0)
	this->statistics_on_decrease_async_sending();
}

void net::io::sendable::Ntcp_async::process_close_sender()
{
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - handle은 lock 안에서 읽는다. (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다)
		const auto handle_socket = this->native_handle();

		// check)
		RETURN_IF(handle_socket == INVALID_SOCKET);

		// 1) linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(handle_socket, 0);

		// 2) 접속을 종료한다.
		api::socket::shutdown(handle_socket, SD_SEND);
	}
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
