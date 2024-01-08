#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;


bool net::io::sendable::Ntcp_sync::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& /*_powner*/, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_sync::process_sendable')"sv));

	// check) if socket is not connected, return false 
	RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

	// 1) get socket native handle
	auto handle_socket = this->native_handle();

	// check) if invalid socket, return false 
	RETURN_IF(handle_socket == INVALID_SOCKET, false);

	// - temporary buffer
	auto buf_now = _buffer;
	auto bytes_requested = buf_now.size();

	// check) 
	CGASSERT_ERROR(buf_now.size() != 0);

	for(;;)
	{
		// - request send
		auto bytes_sended = ::send(handle_socket, buf_now.data(), buf_now.size(), MSG_DONTWAIT | MSG_NOSIGNAL);

		// case) send completed~
		if (bytes_sended == buf_now.size<int>())
		{
			// - call 'process_complete_sendable'
			this->process_complete_sendable(1, bytes_requested, _count_message);

			//break) 
			break;
		}
		// case) send partial - retry
		else if (bytes_sended >= 0)
		{
			// check)
			CGASSERT_ERROR(static_cast<std::size_t>(bytes_sended) < buf_now.size());

			// - 전송한 만큼 이동시킨다.
			buf_now += offset(bytes_sended);

			// - retry
			continue;
		}
		else
		{
			auto error_code = api::get_last_error();

			// case) bytes_sended < 0 && error ==  EWOULDBLOCK (wait for EPOLLOUT)
			if (error_code == EWOULDBLOCK)
			{
				// statistics) 
				this->statistics_on_error_send();

				// return) 전송에 실패했다.
				return false;
			}
			// case) fail to send
			else
			{
				// - set last error
				this->m_last_socket_error = error_code;

				// statistics) 
				this->statistics_on_error_send();

				// - closesocket
				this->process_close_socket(DISCONNECT_REASON_FAIL);

				// return)
				return false;
			}
		}
	}

	// return)
	return true;
}


void net::io::sendable::Ntcp_sync::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics)
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

void net::io::sendable::Ntcp_sync::process_reset_sendable() noexcept
{
}

uint32_t net::io::sendable::Ntcp_sync::process_recover_sendable(intptr_t /*_event*/, std::size_t /*_param*/)
{
	return 0;
}

#endif