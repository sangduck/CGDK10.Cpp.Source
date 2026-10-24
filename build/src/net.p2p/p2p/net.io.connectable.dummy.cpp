#include "pch.h"

using namespace CGDK;


void net::io::connectable::Ndummy::process_reset_connectable() noexcept
{
	// 1) Reason을 reset한다.
	CGASSERT_ERROR((m_disconnect_reason & DISCONNECT_REASON_MASK) == 0);
}

bool net::io::connectable::Ndummy::disconnect(uint64_t _disconnect_reason) noexcept
{
	// 1) set socket send_disconnect
	auto result = set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::SEND_DISCONNECTED);

	// check) 
	RETURN_IF(result == false, false);

	// check)
	CGASSERT_ERROR((_disconnect_reason & DISCONNECT_REASON_MASK) == 0);

	// 2) add DISCONNECT_ACTIVE flag to disconnect_reason 
	_disconnect_reason &= ~DISCONNECT_REASON_MASK;
	_disconnect_reason |= DISCONNECT_REASON_ACTIVE;

	// 3) add disconnect reason flag
	m_disconnect_reason |= _disconnect_reason;

	// 4) get sender_closer interface
	auto psender_close = dynamic_cast<net::io::Isender_closser*>(this);

	// 5) close
	if(psender_close != nullptr)
	{
		psender_close->process_close_sender();
	}
	else
	{
		process_close_socket(DISCONNECT_REASON_ACTIVE);
	}

	// return)
	return true; 
}

void net::io::connectable::Ndummy::process_request_connect (const net::sockaddr& _address)
{
	on_request_connect(_address);
}

bool net::io::connectable::Ndummy::process_complete_connect (Iconnective* _pconnective, uintptr_t _result)
{
	//-----------------------------------------------------------------
	// 1. connect에 실패한 경우.
	//-----------------------------------------------------------------
	// 1) _result가 0이 아니면 connect에 실패한 것이다.
	if(_result != 0)
	{
		// - socket Status를 disconnect로
		socket_state(eSOCKET_STATE::CLOSED);

		// - on_fail_connect()함수를 호출한다.
		try
		{
			on_fail_connect(_pconnective, DISCONNECT_REASON_FAIL);
		}
		catch(std::exception& e)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}
		catch(...)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'			
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}

		// return) 실패!
		return false;
	}


	//-----------------------------------------------------------------
	// 2. socket의 상태를 바꾼다.
	//-----------------------------------------------------------------
	// 1) 접속된 socket의 상태를 CONNECTED된 것으로 설정한다.
	//    - 여기서 eSOCKET_STATE::SYN이 아닐 수도 있다 왜냐면 그 사이에 disconnect를 했을 수도 있기 때문이다.
	//    - 따라서 반드시  SOCKET_STTE_SYN일 때만 ESTABLISHED로 설정해 주어야 한다.
	//      아니면 disconnect가 진행되는 것이기 때문이다.
	set_socket_state_if(eSOCKET_STATE::SYN, eSOCKET_STATE::ESTABLISHED);

	// statistics) 
	statistics_on_connect();

	// declare)
	bool result = true;

	try
	{
		// 2) on_connect를 호출한다.(virtual함수인 on_connect를 호출한다.)
		// 
		// 주의1) on_connect를 반드시 prepare_receive보다 먼저 수행해야 한다.
		//   만약 on_connect를 prepare_receive이전에 호출하게 되면 Multi-Thread로
		//   처리되므로 on_connect()수행 중 receive를 받아 처리할 수가 있기
		//   때문에 순차성에 문제가 생길 수 있으므로 이점을 반드시 유의해야 한다.
		//   (혹시 추후 Code 수정시 이점 정말 주의!!!)
		//
		// 주의2) on_connect함수 호출 시 disconnect()를 호출할 수 있다. 이럴 경우
		//   receive가 호출실패가 될 수 있으므로 주의해야 한다.
		//
		process_on_connect(_pconnective);

		// 3) prepare_receive함수를 호출한다.
		//    - receive를 걸게 된다. 따라서 이 함수 호출 이후로 receive를 
		//      받을 수 있게 된다. 
		//    - 실질적으로 이후로는 접속종료 시 disconnect를 수행해야 한다.
		if(prepare_receive()==false)
		{
			// - process_complete_disconnect한다.
			process_complete_disconnect();

			// - prepare단계에서 실패된 socket을 앗사리 닫아 버린다.
			process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL_TO_SOCKET_PREPARE);

			// - Result를 false로...
			result = false;
		}
	}

	// exception) 
	catch(std::exception& e)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		statistics_set_error_disconnect();

		// E1) process_close_socket한다.
		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) 접속해제과정을 진행한다.
		if (m_pconnective != nullptr)
		{
			m_pconnective->process_connective_disconnect(this);
		}

		// - Result를 false로...
		result = false;
	}
	catch(...)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - OnException을 호출한다.
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		statistics_set_error_disconnect();

		// E1) process_close_socket한다.
		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) 접속해제과정을 진행한다.
		if (m_pconnective != nullptr)
		{
			m_pconnective->process_connective_disconnect(this);
		}

		// - Result를 false로...
		result = false;
	}

	// return) 
	return result;
}


bool net::io::connectable::Ndummy::process_complete_disconnect()
{
	//-----------------------------------------------------------------
	// 1. SocketStatus를 설정한다.
	//
	//    - socket_state()가 atomic으로 동작을 하게 함으로써 동시에
	//      두 개의 Thread가 접근더라도 하나의 Thread에서만 실행되게 함은
	//      물론 두 번 수행되는 것을 방지하기 위해 이전 State를 검사한다.
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
	// 1) on_disconnect를 부른다.
	try
	{
		process_on_disconnect(m_disconnect_reason, socket_state_old);
	}
	catch(std::exception& e)
	{
		// trace) prepare receive에 실패했다!
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}
	catch(...)
	{
		// trace) prepare receive에 실패했다!
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}

	// statistics) 접속 종료시간을 쓴다.
	statistics_on_disconnect();

	// 2) 접속 종료 처리한다.
	process_close_socket();

	// 3) process_connective_disconnect를 호출한다.
	if (m_pconnective != nullptr)
	{
		m_pconnective->process_connective_disconnect(this);
	}

	// return) 성공~
	return true;
}
