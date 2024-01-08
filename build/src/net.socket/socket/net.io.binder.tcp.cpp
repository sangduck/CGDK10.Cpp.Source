#include "pch.h"

using namespace CGDK;


bool net::io::binder::Ntcp::process_request_bind( const net::sockaddr& _socketaddr)
{
	// check) 이미 Connected 상태인지를 검사한다.
	RETURN_IF(this->socket_state() >= eSOCKET_STATE::ESTABLISHED, false);

	try
	{
		// 1) socket reset(ProcessResetSocket()함수를 호출한다.)
		this->prepare_socket(_socketaddr.u_v4.sin_family);

		// 2) bind socket
		auto result = api::socket::bind(native_handle(), &static_cast<const ::sockaddr&>(_socketaddr), _socketaddr.length);

	#if defined(_WINSOCK2API_)
		// check)
		if (result == SOCKET_ERROR)
		{
			// - get error code
			auto error_code = api::get_last_error();

			// - set error code
			this->m_last_socket_error = error_code;

			// - re-alloc socket handle
			if (error_code == WSAEINVAL || error_code == WSAEADDRINUSE)
			{
				// - alloc new socket handle
				this->process_create_socket(address_family());

				// - try binding one more
				result = api::socket::bind(native_handle(), &static_cast<const ::sockaddr&>(_socketaddr), _socketaddr.length);
			}
		}
	#endif

		// check) 
		if (result == SOCKET_ERROR)
		{
			// - set last error
			auto error_code = api::get_last_error();

			// - set last error
			this->m_last_socket_error = error_code;

			// raise) 
			throw throwable::failure(eRESULT::API_FAILURE, error_code);
		}
	}
	catch(std::exception& e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - socket을 닫는 처리 등을 한다.(socket Handle을 닫는다.)
		this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// return) 
		return false;
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - socket을 닫는 처리 등을 한다.(socket Handle을 닫는다.)
		this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// return) 
		return false;
	}

	// return) 
	return true;
}
