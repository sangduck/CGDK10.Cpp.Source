#include "pch.h"

using namespace CGDK;


bool net::io::binder::Nudp::process_request_bind( const net::sockaddr& _socketaddr)
{
	//  check) 이미 bind된 상태인지를 검사한다.
	RETURN_IF(this->socket_state() >= eSOCKET_STATE::BINDED, false);

	// declare) 
	bool result = true;

	try
	{
		// 1) sockaddr
		auto socketaddr = _socketaddr;

		// check) 
		if (socketaddr.u_v4.sin_family == AF_UNSPEC)
			socketaddr.u_v4.sin_family = AF_INET6;

		// 2) socket reset(ProcessResetSocket()함수를 호출한다.)
		this->prepare_socket(socketaddr.u_v4.sin_family);

		// 3) socket 핸들을 얻는다.
		CGSOCKET handle_socket = this->native_handle();

		// 4) bind한다.
		int result_bind = api::socket::bind(handle_socket, reinterpret_cast<const ::sockaddr*>(&socketaddr), socketaddr.length);

		// check) result_bind가 false이면 bind 실패다!!!
		if(result_bind != SOCKET_ERROR)
		{
			// trace) 
			LOG_INFO << "(info) CGDK.net.socket: success to binding udp socket (" << to_string<char>(local_endpoint()) << ")";

			// 5) socket reset(ProcessResetSocket()함수를 호출한다.)
			this->prepare_receive();

			// 6) on_bind함수를 호출한다.
			this->on_bind();
		}
		else
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// trace) 
			LOG_DEBUG << "(err ) CGDK.net.socket: fail to binding on " << static_cast<int>(htons(socketaddr.u_v4.sin_port)) << " port(socket:" << static_cast<int64_t>(handle_socket) << "/ error_code:" << api::get_last_error() << ") " __CGFUNCTION__;

			// - Return값을 false로 설정한다.
			result = false;
		}
	}
	catch(std::exception& e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - socket을 닫는 처리 등을 한다.(socket Handle을 닫는다.)
		this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - Return값을 false로 설정한다.
		result = false;
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - socket을 닫는 처리 등을 한다.(socket Handle을 닫는다.)
		this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - Return값을 false로 설정한다.
		result = false;
	}

	// return) 성공!!!
	return result;
}
