#include "pch.h"
#include "net.io.connectable.disconnect_manager.h"

using namespace CGDK;

#if defined(_WINSOCK2API_)
class net::io::connectable::Ntcp::executable_disconnect : 
	virtual public	Iexecutable, 
	public			Npoolable<net::io::connectable::Ntcp::executable_disconnect>,
	public			factory::_traits_system
{
public:
	virtual ~executable_disconnect() noexcept {}

public:
	virtual	intptr_t process_execute( intptr_t _result, std::size_t _param) override;
			void	set_connectable( net::io::connectable::Ntcp* _pconnectable);

	virtual	void	on_final_release() noexcept override;

public:
			object_ptr<net::io::connectable::Ntcp>	m_pconnectable_tcp;
};

void net::io::connectable::Ntcp::executable_disconnect::set_connectable( net::io::connectable::Ntcp* _pconnectable)
{
	// check) connectable이 nullptr이면 안됀다.
	CGASSERT_ERROR(_pconnectable!=nullptr);

	// 1) 설정
	this->m_pconnectable_tcp = _pconnectable;

	// 2) disconnect manager에 추가한다.
	net::io::connectable::manager_disconnect::instance()->join(_pconnectable);
}

void net::io::connectable::Ntcp::executable_disconnect::on_final_release() noexcept
{
	// 1) connectable을 제거한다.
	if(this->m_pconnectable_tcp.exist())
	{
		// - disconnect manager에서 떼어낸다.
		net::io::connectable::manager_disconnect::instance()->leave(m_pconnectable_tcp);

		// - reset한다.
		this->m_pconnectable_tcp.reset();
	}
}

intptr_t net::io::connectable::Ntcp::executable_disconnect::process_execute( intptr_t /*_result*/, std::size_t /*_param*/)
{
	// 1) Process disconnect
	this->m_pconnectable_tcp->process_disconnect(); 

	// 2) Release
	this->m_pconnectable_tcp->release();

	// return) Success!!
	return intptr_t(0);
}

object_ptr<Iexecutable> net::io::connectable::Ntcp::alloc_executable_disconnect( net::io::connectable::Ntcp* _pconnectable)
{
	// 1) 생성한다.
	auto pexecutable_disconnect = make_object<net::io::connectable::Ntcp::executable_disconnect>();

	// 2) connectable을 설정한다.
	pexecutable_disconnect->set_connectable(_pconnectable);

	// return) 
	return pexecutable_disconnect;
}
#endif

void net::io::connectable::Ntcp::process_reset_connectable() noexcept
{
	// 1) Reason을 reset한다.
	CGASSERT_ERROR((m_disconnect_reason & DISCONNECT_REASON_MASK) == 0);
}

bool net::io::connectable::Ntcp::disconnect(uint64_t _disconnect_reason) noexcept
{
	// 1) set socket send_disconnect
	auto result = this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::SEND_DISCONNECTED);

	// check) 
	RETURN_IF(result == false, false);

	// check)
	CGASSERT_ERROR((_disconnect_reason & DISCONNECT_REASON_MASK) == 0);

	// 2) add DISCONNECT_ACTIVE flag to disconnect_reason 
	_disconnect_reason &= ~DISCONNECT_REASON_MASK;
	_disconnect_reason |= DISCONNECT_REASON_ACTIVE;

	// 3) add disconnect reason flag
	this->m_disconnect_reason |= _disconnect_reason;

	// 4) get sender_closer interface
	auto psender_closer = dynamic_cast<net::io::Isender_closser*>(this);

	// 5) close
	if(psender_closer != nullptr)
	{
		psender_closer->process_close_sender();
	}
	else
	{
		// - set linger option for graceful close
		api::socket::set_linger_option(native_handle(), 0);

		// - shutdown socket
		return api::socket::shutdown(native_handle(), SD_SEND);
	}

	// return) 
	return true;
}

void net::io::connectable::Ntcp::process_request_connect (const net::sockaddr& _address)
{
	this->on_request_connect(_address);
}

bool net::io::connectable::Ntcp::process_complete_connect (uintptr_t _result)
{
	//-----------------------------------------------------------------
	// 1. connect에 실패한 경우.
	//-----------------------------------------------------------------
	// 1) _result가 0이 아니면 connect에 실패한 것이다.
	if(_result != 0)
	{
		// - socket Status를 disconnect로
		this->socket_state(eSOCKET_STATE::CLOSED);

		// - on_fail_connect()함수를 호출한다.
		try
		{
			this->on_fail_connect(DISCONNECT_REASON_FAIL);
		}
		catch(std::exception& e)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv; // static_cast<int64_t>(native_handle()));

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}
		catch(...)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv; // static_cast<int64_t>(native_handle()));

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}

		// return) 실패!
		return false;
	}


	//-----------------------------------------------------------------
	// 2. 접속된 socket의 상태를 CONNECTED로 설정한다.
	// 
	//    - 여기서 eSOCKET_STATE::SYN이 아닐 수도 있다 왜냐면 그 사이에 disconnect를 했을 수도 있기 때문이다.
	//    - 따라서 반드시  SOCKET_STTE_SYN일 때만 ESTABLISHED로 설정해 주어야 한다.
	//      아니면 disconnect가 진행되는 것이기 때문이다.
	//-----------------------------------------------------------------
	this->set_socket_state_if(eSOCKET_STATE::SYN, eSOCKET_STATE::ESTABLISHED);

	// statistics) 
	this->statistics_on_connect();


	//-----------------------------------------------------------------
	// 3. socket Option설정.
	//-----------------------------------------------------------------
	// 1) Linger Option을 사용하지 않는다. 기본적인 Graceful shutdown을 
	//    수행한다.
	api::socket::set_linger_option(this->native_handle(), 0);

	// 2) send shared_buffer Size와 receive BufferSize를  설정한다.
	api::socket::set_send_buffer_size(this->native_handle(), maximum_send_buffer_size());
	api::socket::set_receive_buffer_size(this->native_handle(), maximum_receive_buffer_size());
	api::socket::set_tcp_nodelay(this->native_handle(), this->m_enable_tcp_nodelay);


	//-----------------------------------------------------------------
	// 4. receive를 수행한후 on_connect를 호출 한다.
	//
	// - prepare_receive()를 호출하여 Overlapped receive를 걸어주게 된다.
	// - TCP의 경우 무조건 1번에 한번씩만 receive를 건다.
	// - 접속후 바로 끊기는 경우나 접속 처리가 지연되는 경우 prepare_receive()
	//   과정중에도 얼마든지 Error가 날수 있기 때문에 여기서 Error처리를 
	//   반드시 해주어야 한다.(아니면 Ghost처럼 남을 수 있다.)
	//-----------------------------------------------------------------
	// 1) reference count를 증가시킨다.
#if defined(WINAPI_FAMILY)
	add_ref();
#elif defined(_SYS_EPOLL_H)
	this->m_pexecutable_socket->set_socket(this);

	// check)
	CGASSERT_ERROR(this->m_epoll_event_flag == 0);
#endif

	// declare)
	bool result = true;

	try
	{
		// debug)
	#if !defined(NDEBUG) || defined(_DEBUG)
		if(this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_connected(this->native_handle(), this->remote_endpoint());
	#endif

		// 2) process_on_connect를 호출한다.
		// 
		// 주의1) 반드시 prepare_receive보다 먼저 수행해야 한다.
		//   만약 process_on_connect를 prepare_receive이전에 호출하게 되면
		//   Multi-Thread로 처리되므로 on_connect()수행 중 receive를 받아 처리할
		//   수가 있기 때문에 순차성에 문제가 생길수 있으므로 이점을 반드시 
		//   유의해야 한다.
		//   (혹시 추후 Code 수정시 이점 정말 주의!!!)
		//
		// 주의2) on_connect함수 호출시 disconnect()를 호출할 수 있다. 이럴 경우
		//   receive가 호출실패가 될수 있으므로 주의해야 한다.
		//
		this->process_on_connect();

		// 3) prepare_receive함수를 호출한다.
		//    - receive를 걸게 된다. 따라서 이 함수 호출 이후로 receive를 
		//      받을 수 있게 된다. 
		//    - 실질적으로 이후로는 접속종료시 disconnect를 수행해야 한다.
		if(this->prepare_receive() == false)
		{
			// - prepare단계에서 실패된 socket을 앗사리 닫아 버린다.
			this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL_TO_SOCKET_PREPARE);

			// - process_complete_disconnect한다.
			this->process_complete_disconnect();

			// - Result를 false로...
			result = false;
		}
	}
	// exception) 
	catch(std::exception& e)
	{
		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics)
		this->statistics_set_error_disconnect();

		// E1) process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) process_disconnect를 호출한다.
		this->process_disconnect();

		// - release
	#if defined(WINAPI_FAMILY)
		this->release();
	#elif defined(_SYS_EPOLL_H)
		this->process_reset_executable_socket();
	#endif

		// - Result를 false로...
		result = false;
	}
	catch(...)
	{
		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// E1) process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) process_disconnect를 호출한다.
		this->process_disconnect();

		// - release
	#if defined(WINAPI_FAMILY)
		this->release();
	#elif defined(_SYS_EPOLL_H)
		this->process_reset_executable_socket();
	#endif

		// - Result를 false로...
		result = false;
	}

	// return) 
	return result;
}

bool net::io::connectable::Ntcp::process_complete_disconnect()
{
	//-----------------------------------------------------------------
	// 1. SocketStatus를 설정한다.
	//
	//    - socket_state()가 atomic으로 동작을 하게 함으로써 동시에
	//      두 개의 Thread가 접근더라도 하나의 Thread에서만 실행되게 함은
	//      물론 두번 수행되는 것을 방지하기 위해 이전 State를 검사한다.
	//      socket_state()함수를 수행하게 되면 atomic함수를 사용
	//      하여 해당 상태로 바꿈과 동시에 이전 상태 값을 돌려준다. 
	//      만약 이전 상태 값이 eSOCKET_STATE::CLOSED라면 그것은 이미 
	//      disconnect 처리가 된 상태나 혹은 접속된 상태를 의미하므로 
	//      처리를 하지 않고 바로 Return한다.
	//
	//-----------------------------------------------------------------
	// 1) Closed상태로 socket State를 바꾸고 이전 State를 얻어온다.
	eSOCKET_STATE socket_state_old = socket_state(eSOCKET_STATE::CLOSED);

	// check) socket이 이미 Closed상태라면 돌려보낸다.
	RETURN_IF(socket_state_old == eSOCKET_STATE::CLOSED, false);


	//-----------------------------------------------------------------
	// 2. process_complete_disconnect를 수행한다.
	//    - 이 함수 내에서 통계처리와 on_disconnect()함수를 호출한다.
	//-----------------------------------------------------------------
	// declare) 
	auto socket_disconnect_reason = this->m_disconnect_reason;

	//  debug)
#if !defined(NDEBUG) || defined(_DEBUG)
	if(this->is_socket_debug_logging_enabled())
		net::io::_debug_log_socket_disconnected(this->native_handle(), socket_disconnect_reason);
#endif

	// 1) process_on_disconnect를 호출한다.
	try
	{
		this->process_on_disconnect(socket_disconnect_reason, socket_state_old);
	}
	catch(std::exception& e)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}
	catch(...)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}

	// statistics)
	this->statistics_on_disconnect();


#if defined(_WINSOCK2API_)
	if ((socket_disconnect_reason & DISCONNECT_REASON_NODISCONNECTEX) == 0)
	{
		//-----------------------------------------------------------------
		// 3. shutdown 한다.
		//    - 이 순간 상대 peer에게 FIN신호를 날리게 된다.
		//-----------------------------------------------------------------
		api::socket::shutdown(native_handle(), SD_SEND);


		//-----------------------------------------------------------------
		// 4. disconnect_async를 수행한다.
		//
		// - disconnect_async함수를 사용해서 Overlapped로 socket을 종료한다.
		// - 상대가 정상적인 접속상태라면 socket을 정리후 바로 완료를 통보
		//   하지만 
		// - 만약 전송할 내용이 남아 있어 전송할 내용이 있거나 상대 peer가
		//   비정상 종료로 인해 물리적으로 종료된 상황이라 TIME_WAIT상태에
		//   물리게 되면 최대 2MSL만큼의 시간이 걸릴 수 있다.
		// - 이럴 경우 일정시간(보통 4분)동안 접속종료신호가 도착하지 않을 
		//   경우 강제 접속종료처리를 하고 disconnect_async의 완료를 통보하여 
		//   Release되며 완전 종료된다.
		// - Reuse옵션을 사용해 닫으므로 재사용한다.
		// - disconnect시에는 Overlapped로 수행되더라도 add_reference를 
		//   수행하지 않는다.
		//   connect시 Reference가 증가된 것이 Release되지 않고 유지되기 
		//   때문이며 이 값을 disconnect과정에서 감소시킬 것이다.
		//-----------------------------------------------------------------
		// declare) 
		object_ptr<Iexecutable>	pexecutable_disconnect;

		// 1) executable disconnect 객체를 할당한다.
		try
		{
			pexecutable_disconnect = alloc_executable_disconnect(this);
		}
		catch (...)
		{
			// - 강제 종료를 진행한다.
			this->process_close_socket();

			// - 바로 disconnect Completion을 처리해 준다.
			this->process_disconnect();

			// - disconnect처리 시....
			this->release();

			// return) 
			return false;
		}

		// 2) Reference count를 1 증가시킨다.(executor에서 Completion후 ReferenceCount를 1 감소시키는 것 때문...)
		pexecutable_disconnect->add_ref();

		// 4) disconnect_async한다.
		bool result = api::socket::disconnect_async(native_handle(), pexecutable_disconnect);

		// check) disconnect_async가 제대로 수행되었는지 검사합니다.
		if(result == false)
		{
			// - 위의 (3)항에서 add_ref했던 것!
			pexecutable_disconnect->release();

			// - 강제 종료를 진행한다.
			this->process_close_socket();

			// - 바로 disconnect Completion을 처리해 준다.
			this->process_disconnect();

			// - disconnect처리시....
			this->release();

			// return) 
			return false;
		}
	}
	else
	{
		// - 접속 종료 처리한다.
		this->process_close_socket();

		// - process_disconnect를 호출한다.
		this->process_disconnect();

		// - Release한다.(process_complete_connect때 add_ref()해 준 것)
		this->release();
	}
#else
	// - 접속 종료 처리한다. (closesocket 처리를 한다.)
	this->process_complete_closesocket();

	// - process_disconnect를 호출한다.(connectable manager에서 빼낸다.)
	this->process_disconnect();

	// - Release한다.(process_complete_connect때 add_ref()해 준 것)
	this->process_reset_executable_socket();
#endif

	// return) 성공~
	return true;
}

void net::io::connectable::Ntcp::process_disconnect() noexcept
{
	// 1) connective
	if(this->m_pconnectable_notify != nullptr)
	{
		// - ...
		this->m_pconnectable_notify->process_connective_disconnect(this);

		// - 
		CGASSERT_ERROR(this->m_pconnectable_notify == nullptr);
	}

	// 2) call 'process_disconnect_socket'
	this->process_dispose_socket();

	// statistics) 
	net::io::statistics::Nconnective::statistics_total_on_disconnect();
}

#if defined(_SYS_EPOLL_H)
void net::io::connectable::Ntcp::process_complete_closesocket() noexcept
{
	// check)
	CGASSERT_ERROR(native_handle() == INVALID_SOCKET);

	// 1) detach epoll handle
	auto epoll_handle = detach_epoll_handle();

	// check)
	RETURN_IF(epoll_handle == INVALID_SOCKET);

	// 2.A) closesocket
	if (this->m_disconnect_reason & (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE))
	{
		api::socket::abortive_closesocket(epoll_handle);
	}
	// 2.B) graceful disconnect
	else
	{
		api::socket::graceful_closesocket(epoll_handle);
	}
}
#endif
