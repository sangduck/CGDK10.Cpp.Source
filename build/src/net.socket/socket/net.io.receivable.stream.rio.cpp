#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


intptr_t net::io::receivable::Nstream_rio::executable_receiving::process_execute( intptr_t _result, std::size_t _param)
{
	// 1) process_complete_receiveable을 호출한다.
	auto result = this->m_preceivable_stream->process_complete_receivable(_result, _param);

	// statistics) receive가 완료되었으므로 걸려있는 Overlapped I/O의 개수를 줄인다.
	this->m_preceivable_stream->statistics_on_decrease_async_receiveing();

	// 2) release한다.
	this->m_preceivable_stream->release();

	// return) 
	return result;
}

net::io::receivable::Nstream_rio::Nstream_rio() :
	m_minimum_message_buffer_size(g_bytes_message_buffer_default_min_size),
	m_maximum_message_buffer_size(g_bytes_message_buffer_default_max_size),
	m_buffer_received(alloc_shared_buffer(g_bytes_message_buffer_default_min_size)),
	m_wsabuf_receiving(static_cast<ULONG>(g_bytes_message_buffer_default_min_size), m_buffer_received.data())
{
	// check) 최소 MINIMUM_REMAINED_RECEIVE_BUFFER보다는 크거나 같아야 한다.
	CGASSERT_ERROR(this->m_minimum_message_buffer_size >= MINIMUM_REMAINED_RECEIVE_BUFFER);

	// check) MAX_TCP_MESSAGE_SIZE보다는 같거나 작아야 한다.
	CGASSERT_ERROR(this->m_maximum_message_buffer_size <= MAX_TCP_MESSAGE_SIZE);

	// check) m_minimum_message_buffer_size보다 m_maximum_message_buffer_size가 커야한다.
	CGASSERT_ERROR(this->m_minimum_message_buffer_size <= this->m_maximum_message_buffer_size);

	// check) m_maximum_message_buffer_size가  풀의 최고 할당 가능 메모리(MEMORYPOOL_MAX_SIZE_OF_MEMORY_BLOCK)보다 크면 경고!
	EXECUTE_IF(this->m_maximum_message_buffer_size > MEMORYPOOL_MAX_SIZE_OF_MEMORY_BLOCK, LOG_WARNING << "(!) Max receive buffer size is bigger than max memory size of memory block pool  (" << this->m_maximum_message_buffer_size << "bytes)");

	// - executable을 생성한다.
	this->m_pexecutable_receiving = make_object<executable_receiving>();
	this->m_pexecutable_receiving->m_preceivable_stream = this;
}

net::io::receivable::Nstream_rio::~Nstream_rio() noexcept
{
}

void net::io::receivable::Nstream_rio::process_reset_receivable()
{
	// 1) 새로 buffer를 할당받는다.
	this->m_buffer_received = alloc_shared_buffer(minimum_message_buffer_size());

	// 2) buffer를 setting한다.
	this->m_wsabuf_receiving = WSABUF(static_cast<ULONG>(minimum_message_buffer_size()), this->m_buffer_received.data());
}

bool net::io::receivable::Nstream_rio::receive()
{
	// check) socket상태가 연결된 상태인가?
	CGASSERT(this->m_wsabuf_receiving.buf != nullptr, false)

	// check) m_wsabuf_receiving->size_가 0이어서는 안된다.
	CGASSERT(this->m_wsabuf_receiving.len != 0, false)

	// check) Invalid socket인가?( release에서는 만약 socket Handle이 잘못되었으면 receive함수가 자동 실패한다.)
	RETURN_IF(this->native_handle() == INVALID_SOCKET, false)

	// 1) reference count를 증가시킨다.
	this->add_ref();

	// 2) m_pexecutable_receiving의 reference count를 증가시킨다.
	this->m_pexecutable_receiving->add_ref();

	// 3) overlapped receive를 하나 늘인다.
	this->statistics_on_increase_async_receiveing();

	// 4) reset params
	this->m_wsa_bytes_received = 0;
	this->m_wsa_flag = 0; // MSG_PUSH_IMMEDIATE

	// 5) 보낸다.
	if(this->_call_receiving([&](CGSOCKET _handle_socket) { return api::socket::recv_async(_handle_socket, &this->m_wsabuf_receiving, &this->m_wsa_bytes_received, &this->m_wsa_flag, this->m_pexecutable_receiving); }) == false)
	{
		// statistics) 
		this->statistics_on_error_receive();

		// - overlapped receive를 하나 줄인다.
		this->statistics_on_decrease_async_receiveing();

		// - release한다.
		this->m_pexecutable_receiving->release();

		// - release한다.
		this->release();

		// return) 실패~
		return false;
	}

	// statistics) 
	this->statistics_on_receive_async();

	// return) 성공!!
	return true;
}

bool net::io::receivable::Nstream_rio::prepare_receive()
{
	return this->receive();
}

intptr_t net::io::receivable::Nstream_rio::process_complete_receivable( intptr_t _result, std::size_t _transferred)
{
	//-----------------------------------------------------------------
	// 설명) process_complete_receivable
	//
	// - receive가 완료된 경우는 Overlapped로 동작하는 곳에서는 해당
	//   socket으로부터 데이터를 전송받거나 혹은 접속이 종료되었을 때
	//	 불려진다.
	//	 
	// - _result값
	//   I/O 처리를 한 결과를 리턴한다.
	//    _result값이 0인 경우 경우 일단 정상 송수신을 의미한다.
	//    _result값이 0이 아닌 경우 비정상 처리를 의미한다.
	//
	// - _transferred값
	//   I/O가 일어난 바이트 수를 전달한다.
	//   일반적으로 I/O가 0이면 접속 종료를 의미한다.
	//
	// - 파라메터 값에 따른 처리
	//
	//   Case1) _result==0 && _transferred!=0
	//          정상적인 receive를 의미하며 받은 Packet 처리 절차를
	//          수행한다.
	//
	//   Case1) _result==0 && _transferred==0
	//          정상적인 접속 종료를 의미한다.
	//
	//   Case2) _result!=0
	//          비정상적인 접속 종료를 의미한다.
	//
	//-----------------------------------------------------------------
	// check) 
	CGASSERT(this->socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// check) 만약 전송된 Byte수가 0Byte면 접속을 종료처리한다.
	if(_transferred == 0)
	{
		// - 비정상 접속종료일 경우 REASON_FAIL을 설정한다.
		if(_result != 0)
		{
			this->m_disconnect_reason	|= DISCONNECT_REASON_FAIL;
		}

		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return) 
		return intptr_t(1);
	}


	//-----------------------------------------------------------------
	// 1. 전송된 데이터 처리
	//-----------------------------------------------------------------
	// statistics)
	this->statistics_on_receive_bytes(_transferred);

	// 1) add size
	this->m_buffer_received.add_size( _transferred);

	try
	{
		// debug)
	#if !defined(NDEBUG) || defined(_DEBUG)
		if(this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(this->native_handle(), _transferred, this->m_buffer_received.size());
	#endif

		//-----------------------------------------------------------------
		// 2. 패킷화 처리하기
		//
		// exception) on_receive나 혹은 process_packet()의 과정에서
		//   예외가 발생할 수 있다.
		//    process_packet과정에서 I/O에 관련되 Exception이 발생할 수
		//   있는 데 그것은 잘못된 message의 크기 같은 때이다. 
		//    이런 Exception이 발생하면 해당 socket은 끊어버리는 것이
		//   일반적이다. 따라서 OnException함수를 호출 후 끊어버린다.
		//-----------------------------------------------------------------
		// declare) 
		std::size_t required_bytes;
		net::sockaddr address_peer;

		// 1) 일단 on_receive한다.(사용자 정의)
		this->process_on_receive(buffer_view{ this->m_wsabuf_receiving.buf, _transferred }, address_peer);

		// 2) Process message를 수행한다.
		//    - 여기서 Return해 오는 값은 다음 message를 받기 위해 다음에 필요로 하는 메모리 용량이다.
		//    - 주어지는 &m_buffer_received에 진행된 Memory과 남은 Size등이 되돌아 온다.
		//      따라서 m_buffer_received은 받은 데이터 그대로 사용하면 된다.
		//    - Exception이 발생가능하다.
		required_bytes = this->process_packet(this->m_buffer_received, address_peer);

		// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
		THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size too BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << to_string(remote_endpoint()));


		//-----------------------------------------------------------------
		// 3. buffer 체크한다.
		//    1) required_bytes보다 남아 있는 receive 버퍼의 크기가 작다면
		//       버퍼를 새로 할당한다.
		//    2) 기존의 버퍼를 버리고 새로운 버퍼를 할당받는다.
		//    3) 또 기존 receive 버퍼에 처리되지 않은 데이터가 있다면
		//       새로 할당받은 버퍼에 복사를 해 넣는다.
		//-----------------------------------------------------------------
		// 1) m_wsabuf_receiving를 계산함.
		this->m_wsabuf_receiving.buf += static_cast<ULONG>(_transferred);
		this->m_wsabuf_receiving.len -= static_cast<ULONG>(_transferred);

		// 2) 받고자하는 message의 크기보다 message의 미수신 크기가 많으므로...(메모리를 새로 할당받는다.)
		if(this->m_wsabuf_receiving.len < MINIMUM_REMAINED_RECEIVE_BUFFER || (this->m_buffer_received.size() + this->m_wsabuf_receiving.len) < required_bytes)
		{
			// - [Receiving buffer]의 원 크기를얻는다.
			auto size_source = this->m_buffer_received.get_source()->size();

			// declare) 
			std::size_t size_prepare;

			// - [Receiving buffer]와 _transferred 크기 차이에 따라 할당할 버퍼 크기를 결정한다.
			//   [Receiving buffer]의 할당 크기를 한번에 받은 경우 => 버퍼 크기를 8배로 늘린다.
			if (_transferred == size_source)
			{
				size_prepare = _transferred * 8;
			}
			// [Receiving buffer]의 할당 크기의 절반 이상을 받았을 경우 => 이전 버퍼 크기를 유지한다.
			else if (_transferred >= (size_source >> 1))
			{
				size_prepare = size_source;
			}
			// [Receiving buffer]의 할당 크기의 절반 이하일 경우 => _transferred 크기보다 크게만 할당한다.
			else
			{
				size_prepare = this->m_minimum_message_buffer_size;
				while (_transferred > size_prepare) size_prepare *= 8;
			}

			// - [최소_TCP_수신_버퍼_크기]보다는 크게 한다.
			SATURATE_MAX(size_prepare, this->m_maximum_message_buffer_size);

			// declare)
			auto buffer_received_size = this->m_buffer_received.size();

			// - 최소 [요구_버퍼_크기]+[이미_수신한_데이터_크기]보다는 커야 한다.
			size_prepare = __select_max(size_prepare, required_bytes + buffer_received_size);

			// - [새_수신_버퍼]를 [준비할_버퍼량] 크기만큼 할당한다.
			//   exception) Memory 할당을 받지 못했을 경우라면... Error처리를 위해 Throw한다.
			//   * Size가 Pool이 할당할 수 있는 최대 크기(일반적으로 256Kbytes)이상일 경우 발생한다.
			//   * 메모리 부족으로 인해 더 이상 할당받지 못했을 경우에 뜬다.
			auto buf_new = alloc_shared_buffer(size_prepare);

			// - [이미_수신한_데이터]를 [새_수신_버퍼]의 앞쪽에 복사한다.
			// - 새로 할당된 buffer에 처리되지 않고 남은 Byte를 복사해 준다.
			if(buffer_received_size != 0)
			{
				memcpy(buf_new.data(), this->m_buffer_received.data(), buffer_received_size);
			}

			// - [새_수신_버퍼]를 [수신_버퍼](m_wsabuf_receiving)에 설정한다.
			this->m_wsabuf_receiving = WSABUF{ static_cast<ULONG>(size_prepare - buffer_received_size), buf_new.data() + buffer_received_size };

			// - [이미_수신한_데이터_버퍼]에 버퍼를 설정한다.
			buf_new ^= buffer_received_size; this->m_buffer_received = std::move(buf_new);
		}

		// check) 
		CGASSERT_ERROR(this->m_wsabuf_receiving.len != 0);
	}
	catch (throwable::failure& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 다양한 Exception이 발생가능하다.
		// * alloc_shared_buffer 수행 중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (throwable) " << __CGFUNCTION__;
		LOG_DEBUG << "       - reason: " << CGDK::stream_hex(_e.reason<uint64_t>(), 0, false, true) << " [" << to_string(_e.reason<eRESULT>()) << "]";
		if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return)
		return intptr_t(1);
	}
	catch (std::exception& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 다양한 Exception이 발생가능하다.
		// * alloc_shared_buffer 수행 중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (std::exception) " << __CGFUNCTION__;
		LOG_DEBUG << "       - " << _e.what();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return)
		return intptr_t(1);
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__;

		// declare)
		throwable::failure	e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return)
		return intptr_t(1);
	}

	// check) 만약 I/O의 결과로 Error가 돌아 왔다면 접속을 종료시킨다.
	if(_result != 0)
	{
		// - Abortive로 설정
		this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return) 
		return intptr_t(1);
	}


	//-----------------------------------------------------------------
	// 3. overlapped receive를 걸어준다.
	//    - receive에 실패했을 경우 접속을 바로 종료처리한다.
	//-----------------------------------------------------------------
	// 1) receive를 건다.
	if(receive() == false)
	{
		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();
	}

	// return) 
	return intptr_t(0);
}

void net::io::receivable::Nstream_rio::process_clear_buffer() noexcept
{
	// 1) 버퍼를 할당 해제한다.
	this->m_buffer_received.clear();

	// 2) 리셋한다.
	this->m_wsabuf_receiving = WSABUF{ 0,nullptr };
}
#endif
