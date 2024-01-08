#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include "net.io.sendable.executable_send_buffer.h"

using namespace CGDK;


bool net::io::sendable::Nudp::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& /*_powner*/, std::size_t _count_message, const net::sockaddr& _paddress)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_sync_buffered::process_sendable')"sv));

	// check) Invalid socket인가?
	ERROR_RETURN_IF_INVALID_SOCKET(false, LOG_DEBUG << "(err ) CGDK.net.socket: socket handle is INVALID_HANDLE " __CGFUNCTION__)

	// declare)
	bool result;

	// 1) send 한다.
	auto bytes_sended = ::sendto(this->native_handle(), _buffer.data(), _buffer.size(), MSG_DONTWAIT,  reinterpret_cast<::sockaddr*>(const_cast<net::sockaddr*>(&_paddress)), _paddress.length);

	// check) Result가 false면 send 못한거다..
	if (bytes_sended >= 0)
	{
		// - 
		this->process_complete_sendable(0, bytes_sended, _count_message);

		// - set result false
		result = true;
	}
	else
	{
		// statistics) 
		this->statistics_on_error_send();

		// trace) 
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to call 'send_to' function " __CGFUNCTION__;

		// - 
		this->process_complete_sendable(1, 0, 0);

		// - set result false
		result = false;
	}

	// return) 성공!!
	return result;
}

#if defined(_MSC_VER)
	#pragma warning(default:4100)
#endif

void net::io::sendable::Nudp::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics)  
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

uint32_t net::io::sendable::Nudp::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	return 0;
}

void net::io::sendable::Nudp::process_reset_sendable() noexcept
{
}


#endif