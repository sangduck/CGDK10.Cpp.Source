#include "pch.h"

#if defined(_WINSOCK2API_)

#include "net.io.receivable.executable_receive.h"

using namespace CGDK;

// - NTSTATUS of a receive completed by the ICMP port unreachable of a datagram sent before (SIO_UDP_CONNRESET on : WSAECONNRESET)
static constexpr LONG STATUS_PORT_UNREACHABLE_VALUE = static_cast<LONG>(0xC000023FUL);

net::io::receivable::Ndatagram::Ndatagram() noexcept :
	m_message_buffer_size(DEFAULT_UDP_MESSAGE_BUFFER_SIZE),
	m_prepare_receive_count(DEFAULT_UDP_COUNT_RECEIVE_BUFFER)
{
}

void net::io::receivable::Ndatagram::set_message_size(std::size_t _message_size)
{
	// check) 최소 Packet Size가 너무 작아도 안된다.
	CGASSERT_ERROR(_message_size >= MIN_UDP_MESSAGE_SIZE);

	// check) 최소 Packet Size가 너무 커도 안된다.
	CGASSERT_ERROR(_message_size <= MAX_UDP_MESSAGE_SIZE);

	// 1) set message size
	this->m_message_buffer_size = _message_size;
}

std::size_t net::io::receivable::Ndatagram::get_prepare_receive_count() const
{
	return this->m_prepare_receive_count;
}

void net::io::receivable::Ndatagram::set_prepare_receive_count( std::size_t _count)
{
	// check) 최소 _count가 1보다 작으면 안된다.
	CGASSERT_ERROR(_count > 0);

	// 1) set prepare receive count
	this->m_prepare_receive_count = _count;
}

void net::io::receivable::Ndatagram::suspend_receive()
{
}

void net::io::receivable::Ndatagram::resume_receive()
{
}

bool net::io::receivable::Ndatagram::prepare_receive()
{
	//-----------------------------------------------------------------
	// 설명)
	//   UDP의 receive는 TCP와 다르게 동시에 동시에 여러 개의 receive를
	//   걸게 된다.
	//   이유는 UDP의 경우 하나의 socket이 여러 개의 peer에서 Data를
	//   receive하고 또 그런 Data들의 순차성이 어차피 보장되지 않기
	//   때문에 TCP와 같이 한꺼번에 하나씩의 I/O를 걸어서 순차성을 보장
	//   받을 필요가 없기 때문이다.
	//-----------------------------------------------------------------

	// check) Invalid socket인가?
	ERROR_RETURN_IF_INVALID_SOCKET(false, LOG_DEBUG << "(err ) CGDK.net.socket: socket handle is 'INVALID_HANDLE' " __CGFUNCTION__ ""sv)

	// 1) socket 상태를 BINEDED로 바꾼다.
	eSOCKET_STATE socket_state_old = this->socket_state(eSOCKET_STATE::BINDED);

	// check) 이미 Connected 상태인지를 검사한다.
	RETURN_IF(socket_state_old >= eSOCKET_STATE::BINDED, false);


	// 2) buffer의 크기를 확인한다. (message용 buffer의 크기를 구한다.)
	size_t message_size = this->get_message_size();

	// trace) 최대 message크기를 출력한다.
	LOG_INFO << "(info) CGDK.net.socket: max message size of udp socket is "sv << api::socket::get_max_message_size(this->native_handle());

	// trace) 버퍼의 크기를 출력한다.
	LOG_INFO << "(info) CGDK.net.socket: message buffer size for udp receive is "sv << this->get_message_size();


	// 3) UDP용 Overlapped receive를 걸어준다.
	//    * prepare_receive만큼 receive를 준비한다.
	// declare) 
	size_t must_prepare = this->get_prepare_receive_count();
	size_t prepared;

	for(prepared = this->statistics_get_async_receiving(); prepared < must_prepare; ++prepared)
	{	
		// declare) 
		object_ptr<executable_receive> pexecutable;

		//-----------------------------------------------------------------
		// 1) add_reference처리.
		//
		//     receive를 걸기 전에 반드시 먼저 add_ref()를 해줘야 한다.
		//     왜냐하면 receive가 다른 Thread에서 완료를 할 수도 있기 때문에
		//     그렇게 되면 Reference count가 증가되기 전에 감소해서 할당
		//     해제 될 수도 있기 때문에 Error 발생 시 다시 release()하는
		//     한이 있더라도 먼저 add_reference를 해야 한다.
		//-----------------------------------------------------------------
		add_ref();

		try
		{
			// 1) CGExecutable::CReceive를 생성한다.
			pexecutable = make_object<executable_receive>();

			// 2) CGExecutable::CReceive를 설정한다.
			pexecutable->set_receivable(this); // receivable을 설정한다. this로...
			pexecutable->set_receive_buffer(alloc_shared_buffer(message_size) ^ message_size); // receive를 받을 buffer를 설정한다.

			// 3) Address의 length를 다시 reset함.
			pexecutable->set_length_of_address(sizeof(net::sockaddr));

			// 4) Reference count를 증가시킨다.
			pexecutable->add_ref();

			// check) Reference count가 1이어야 한다.
			CGASSERT_ERROR(pexecutable->reference_count() == 2);

			// 5) Overlapped receive를 하나 늘인다.
			this->statistics_on_increase_async_receiveing();

			// 6) receive한다.
			while(false == this->_call_receiving_shared([&](CGSOCKET _handle_socket) { return api::socket::recv_from_async(	// - the handle is read under the receiving lock (close changes it under the same lock)
				_handle_socket,									// - socket Handle
				pexecutable->get_wsabuf(),						// - WSABUF
				1,												// - buffer count
				pexecutable->reset_and_get_ptr_bytes_reiced(),	// - Received Byte
				pexecutable->reset_and_get_ptr_flags(),			// - Flag
				pexecutable->sockaddr_pointer(),				// - Address
				pexecutable->sockaddr_length(),					// - Address length
				pexecutable); }))									// - LPWSAOVERLAPPED
			{
				// declare)
				int	error_code = api::get_last_error();

				// check) connect reset이 아닐 경우 완전한 Error닷!!!
			#if defined(_WINSOCK2API_)
				if(error_code != WSAECONNRESET)
			#elif defined(_SYS_EPOLL_H)
				if (error_code != EWOULDBLOCK)
			#endif
				{
					// - set socket error code
					this->m_last_socket_error = error_code;

					// statistic) receive Error통계처리.
					this->statistics_on_error_receive();

					// - Overlapped receive를 다시 줄인다.
					this->statistics_on_decrease_async_receiveing();

					// trace) 
					LOG_DEBUG << "(excp) CGDK.net.socket: error on receiving datagram error_code:"sv << error_code << " " __CGFUNCTION__ ""sv;

					// Throw) 
					throw std::runtime_error("CGDK.net.socket: error on receiving datagram");
				}

				// - connect reset이 떴음을 Notify한다.(connect reset일 경우 buffer가 nullptr이 된다.)
				shared_buffer buf_temp;
				this->process_packet(buf_temp, pexecutable->sockaddr());
			}
		}
		//-----------------------------------------------------------------
		// exception) receive중 Exception이 발생한 경우.
		//
		// * new CGExecutable::CReceive() 중 Exception발생가능성이 있다.
		// * temp_executable->receive_buffer().alloc_object(message_size); 중 alloc_object()함수 수행 중 Exception발생가능성 있다.
		// * recv_from()함수 호출 실패 시 Throw된다.
		// * process_packet 호출 과정 중 Exception발생 가능성 있다.
		//-----------------------------------------------------------------
		catch(std::exception& e)
		{
			// trace)
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - release한다.
			pexecutable->release();

			// - Release한다.
			this->release();
		}
		catch(...)
		{
			// trace)
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - release한다.
			pexecutable->release();

			// - release한다.
			this->release();
		}

		// statistics) 
		this->statistics_on_receive_async();
	}

	return true;
}


intptr_t net::io::receivable::Ndatagram::process_complete_receivable( uintptr_t _result, std::size_t _transferred, shared_buffer& _buffer, const net::sockaddr& _address)
{
	// check) _buffer.get_source()은 nullptr이면 안된다.
	CGASSERT_ERROR(_buffer.get_source() != nullptr);

	// - the executable of this receive (_result)
	auto pexecutable = reinterpret_cast<executable_receive*>(_result);

	// check) temp_executable은 nullptr이면 안된다.
	CGASSERT(pexecutable != nullptr, 0);

	// - the status of this receive (NTSTATUS, not the last error : the completion was taken by GetQueuedCompletionStatusEx)
	const LONG io_status = pexecutable->io_status();


	//-----------------------------------------------------------------
	//
	// 1. receive 처리를 하는 부분
	//
	//-----------------------------------------------------------------
	try
	{
		// 1) a datagram (a zero-length datagram is one too : succeeded with 0 bytes. a truncated one (STATUS_BUFFER_OVERFLOW) is passed as before)
		if(_transferred != 0 || io_status >= 0)
		{
			// 통계) socket의 receive통계.
			this->statistics_on_receive_bytes(_transferred);

			// - Received한 용량을 _pBuffer에 값에 설정한다.
			_buffer.set_size( _transferred );

			// - process_on_receive()함수를 호출한다.
			this->process_on_receive(_buffer, _address);

			// - process_packet을 수행한다.(UDP에서는 Return Value가 의미없어서 받지 않는다.)
			this->process_packet(_buffer, _address);
		}
		// 2) Error발생 시 on_fail_receive를 수행함.
		// - 이런 경우는 거의 대부분 connect reset일 때 발생한다.
		// - 따라서 이부분은 connect reset을 Report하는 Option을 켰을 때에만 동작한다.
		else
		{
			// - on_fail_receive함수를 호출한다.
			this->on_fail_receive(0, _buffer, _address);

			// check) ERROR_PORT_UNREACHABLE이 떴을 경우 Error사항에 대해서 전달해주어야 한다.
			//        Port Unreachable은 해당 Port로 전송을 할 수 없을 때 뜨는 Error이다.
			//        Port Unreachable은 WSAECONNRESET 옵션을 켰을 때만 동작한다.
			//        - the status of the receive (STATUS_PORT_UNREACHABLE : ERROR_PORT_UNREACHABLE / WSAECONNRESET). api::get_last_error() was read here :
			//          not set by GetQueuedCompletionStatusEx (and on_fail_receive() runs before) => the report never came
			if(io_status == STATUS_PORT_UNREACHABLE_VALUE)
			{
				// - connect reset이 떴음을 Notify한다.
				shared_buffer buf_temp;
				this->process_packet(buf_temp, _address);
			}
		}
	}
	//---------------------------------------------------------
	// On Exceptions)
	//   on_receive나 process_packet과정에서 예외가
	//   발생했을 경우 UDP는 해당 Packet만 폐기처분하고 계속 
	//   처리한다. 왜냐면.. 하나의 peer만 존재하는 것이 아니기
	//   때문이다.
	//   따라서.. Exception에 대한 처리만 할뿐 아무일 안한다~ 
	//    모든 Exception은 무효화다.
	//   Debug Mode에서는 ASSERT를 걸어준다.
	//
	//   * 대략 다음의 경우에서 Exception이 발생할 수 있다.
	//     - OnReceiv()함수 수행 시 Exception~
	//     - PrecessPacket수행 시 Exception~
	//     - OnFailPacket 수행 시 Exception~
	//---------------------------------------------------------
	catch(std::exception& e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// assert) Critical Error를 낸다!
		CGASSERT_ERROR_CRITICAL;
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// assert) Critical Error를 낸다!!!(무조건)
		CGASSERT_ERROR_CRITICAL;
	}


	//-----------------------------------------------------------------
	//
	// 2. 새로운 Overlapped receive를 거는 부분.
	//
	//-----------------------------------------------------------------
	// 1) add_ref한다.
	pexecutable->add_ref();

	// 2) buffer를 재설정한다.
	//     - the same buffer again when the message was not kept (prepare_receive_buffer)
	//     -------------------------------------------------------
	//     Exceptions) Packet을 할당 중 Exception이 발생했을 경우.
	//     - 메모리가 부족할 경우 주로 발생한다.
	//     - 혹은 Packet Size가 너무 클 경우도 발생한다.
	//     -------------------------------------------------------
	pexecutable->prepare_receive_buffer(get_message_size());

	// 3) receive한다.
	//    - shared : the receives of this socket are posted by many threads at once (the close changes the handle under the exclusive lock)
	while(false == this->_call_receiving_shared([&](CGSOCKET _handle_socket) { return api::socket::recv_from_async(	// - the handle is read under the receiving lock (close changes it under the same lock)
		_handle_socket,									// - socket Handle
		pexecutable->get_wsabuf(),						// - WSABUF
		1,												// - buffer count
		pexecutable->reset_and_get_ptr_bytes_reiced(),	// - Received Byte
		pexecutable->reset_and_get_ptr_flags(),			// - Flag
		(::sockaddr*)&_address,							// - Address
		pexecutable->sockaddr_length(),					// - Address length
		pexecutable); }))									// - LPWSAOVERLAPPED
	{
		// declare)
		int	error_code = api::get_last_error();

		// Exceptions) recv_from이 실패한 경우 Throw한다.
		if(error_code != WSAECONNRESET)
		{
			// statistics) 
			this->statistics_on_error_receive();

			// statistics) 
			this->statistics_on_decrease_async_receiveing();

			// trace) 
			LOG_DEBUG << "(err ) CGDK.net.socket: fail to trying UDP receive[socket:{}/ error:" << static_cast<unsigned int>(native_handle()) << "] (" << error_code << ")";

			// check) 만약 OVerlapped Receiveing이 0라면 문제 심각하다~~~!!! 
			//        완전 receive를 못받게 된  상황을 말하는 것이다.
			if(this->statistics_get_async_receiving()==0)
			{
				this->on_receive_exhausted();
			}

			// - Release한다.
			pexecutable->release();

			// - Release한다.
			this->release();

			// return) 
			return 0;
		}

		#if defined(_MSC_VER)
			#pragma warning(disable:6102)
		#endif

		// - connect reset이 떴음을 Notify한다.
		shared_buffer buf_temp;
		this->process_packet(buf_temp, _address);

		#if defined(_MSC_VER)
			#pragma  warning(default:6102)
		#endif
	}

	// statistics) 
	this->statistics_on_receive_async();

	// return)
	return 0;
}


#endif