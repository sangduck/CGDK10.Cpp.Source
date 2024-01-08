#include "pch.h"

#if defined(_WINSOCK2API_)

//using namespace CGDK;
//
//extern std::size_t	g_bytes_message_buffer_default_min_size;
//extern std::size_t	g_bytes_message_buffer_default_max_size;
//
//
//net::io::receivable::Nstream_queue_rio::Nstream_queue_rio() :
//	m_buffer_receiving(alloc_shared_buffer(g_bytes_message_buffer_default_min_size)),
//	m_lockable_queue_received(), 
//	m_queue_received(), 
//	m_byte_queued(0),
//	m_buf_processing(),
//	m_bytes_required(0),
//	m_max_received_bytes_queued(DEFAULT_BYTES_MAX_MESSAGE_RECEIVE_QUEUE),
//	m_minimum_mesage_buffer_size(g_bytes_message_buffer_default_min_size),
//	m_maximum_message_buffer_size(g_bytes_message_buffer_default_max_size)
//{
//	// check) 최소 MINIMUM_REMAINED_RECEIVE_BUFFER보다는 크거나 같아야 한다.
//	CGASSERT_ERROR(m_minimum_mesage_buffer_size >= MINIMUM_REMAINED_RECEIVE_BUFFER);
//
//	// check) MAX_TCP_MESSAGE_SIZE보다는 같거나 작아야 한다.
//	CGASSERT_ERROR(m_maximum_message_buffer_size <= MAX_TCP_MESSAGE_SIZE);
////#if defined(_WINSOCK2API_)
////	// 1) executable Receiving 객첼를 생성한다.
////	m_pexecutable_receiving = make_object<executable_receiving>();
////	m_pexecutable_receiving->m_preceivable = this;
////
////	// 2) executable Received 객첼를 생성한다.
////	m_pexecutable_received = make_object<executable_received>();
////	m_pexecutable_received->m_preceivable = this;
////#endif
//}
//
//net::io::receivable::Nstream_queue_rio::~Nstream_queue_rio() noexcept
//{
////#if defined(_WINSOCK2API_)
////	m_pexecutable_receiving.reset();
////	m_pexecutable_received.reset();
////#endif
//}
//
//std::size_t net::io::receivable::Nstream_queue_rio::minimum_message_buffer_size(std::size_t _size) noexcept
//{
//	// 1) 기존의 Min receive buffer 크기를 저장해 놓는다.
//	std::size_t	old = m_minimum_mesage_buffer_size;
//
//	// 2) 최하크기 이하일 경우 최하크기로 맞춘다.
//	SATURATE_MIN(_size, MINIMUM_REMAINED_RECEIVE_BUFFER)
//
//	// check) 이전의 값과 동일하면 그냥 끝낸다.
//	RETURN_IF(old == _size, old);
//
//	// 3) 값을 변경한다.
//	m_minimum_mesage_buffer_size = _size;
//
//	// 4) 
//	if (m_minimum_mesage_buffer_size > m_maximum_message_buffer_size)
//	{
//		m_maximum_message_buffer_size = m_minimum_mesage_buffer_size;
//	}
//
//	// return) 
//	return old;
//}
//
//std::size_t net::io::receivable::Nstream_queue_rio::minimum_message_buffer_size() noexcept
//{
//	return	m_minimum_mesage_buffer_size;
//}
//
//std::size_t net::io::receivable::Nstream_queue_rio::maximum_message_buffer_size(std::size_t _size) noexcept
//{
//	// 1) 기존의 Min receive buffer 크기를 저장해 놓는다.
//	std::size_t	old = m_maximum_message_buffer_size;
//
//	// check) 이전의 값과 동일하면 그냥 끝낸다.
//	RETURN_IF(old == _size, old);
//
//	// 2) 값을 변경한다.
//	m_maximum_message_buffer_size = _size;
//
//	// 3) 
//	if (m_minimum_mesage_buffer_size > m_maximum_message_buffer_size)
//	{
//		m_minimum_mesage_buffer_size = m_maximum_message_buffer_size;
//	}
//
//	// return) 
//	return old;
//}
//
//std::size_t net::io::receivable::Nstream_queue_rio::maximum_message_buffer_size() noexcept
//{
//	return	m_maximum_message_buffer_size;
//}
//
//void net::io::receivable::Nstream_queue_rio::process_reset_receivable()
//{
//	// 1) size를 얻는다.
//	auto size_alloc = minimum_message_buffer_size();
//
//	// 2) buffer를 setting한다.
//	m_buffer_receiving = alloc_shared_buffer(size_alloc) ^ size_alloc;
//
//	// 3) Received Queue를 clear한다.
//	scoped_lock(m_lockable_queue_received)
//	{
//		m_queue_received.clear();
//		m_byte_queued = 0;
//	}
//
//	// 4) Processing buffer를 reset한다.
//	m_buf_processing = m_buffer_receiving ^ 0;
//	m_bytes_required = 0;
//}
//
//bool net::io::receivable::Nstream_queue_rio::receive()
//{
//	// 1) io_context를 얻는다.
//	auto pio_context = io_context();
//
//	// check) io_context가 없으면 안됀다!
//	CGASSERT_ERROR(pio_context.exist()); 
//
//	// 2) add_ref를 해준다. 이것은 execute 처리 된이후에 release될 것이다.
//	m_pexecutable_socket->add_ref();
//
//	// 3) add_ref한다.
//	add_ref();
//
//	// 3) Event를 설정한다.
//	auto result = set_event(EPOLLONESHOT | EPOLLWAKEUP | EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
//	//auto result = set_event(EPOLLONESHOT | EPOLLWAKEUP | EPOLLIN | EPOLLERR);
//
//	// check) Event 설정에 실패했으면 Reference count를 release를 해준다.
//	if(result==false)
//	{
//		// - ...
//		process_reset_executable_socket();
//
//		// - release
//		release();
//	}
//
//	// return) 성공!!
//	return result;
//}
//
//bool net::io::receivable::Nstream::prepare_receive()
//{
//	// 1) 현 버퍼 크기를 Default Min크기로 한다.
//	m_bytes_buffer_receive = g_bytes_message_buffer_default_min_size;
//
//	// 2) 버퍼 크기를 설정한다.
//	api::socket::set_receive_buffer_size(native_handle(), m_bytes_buffer_receive);
//
//	// 3) receive를 건다.
//	return receive();
//}
//
//intptr_t net::io::receivable::Nstream::process_complete_receivable( intptr_t /*_result*/, std::size_t _transferred)
//{
//	// check)
//	CGASSERT(socket_state() != eSOCKET_STATE::SYN, intptr_t(1));
//
//	try
//	{
//		//-----------------------------------------------------------------
//		// 1. 수신 받은 데이터 Byte수를 구한다.
//		//-----------------------------------------------------------------
//		// declare) 
//		int bytes_pended = 0;
//
//		// 1) SOcket Handle을 얻는다.
//		auto handle_socket = native_handle();
//
//		// 2) 먼저 receive 받을 수 있는 Bytes수를 얻는다.
//		::ioctl (handle_socket,FIONREAD, &bytes_pended);
//
//		// declare) 
//		shared_buffer buffer_receive;
//
//		// 3) 최소 버퍼크기 이승으로 설정한다.
//		bytes_pended = __select_max(static_cast<int>(g_bytes_message_buffer_default_min_size), bytes_pended);
//		
//		// 4) shared_buffer를 할당받는다.
//		buffer_receive = alloc_shared_buffer(m_buffer_received.size() + bytes_pended);
//
//		// 5) receive한다.
//		const int bytes_transfered = ::recv(handle_socket, buffer_receive.data() + m_buffer_received.size(), bytes_pended, MSG_DONTWAIT);
//
//		// check) 전송된 Byte수가 0이거나 보다 작으면 접속 종료일 수 있다.
//		if(bytes_transfered <= 0)
//		{
//			// check) Error code가 EWOULDBLOCK이면 Error가 아니므로 바로 리턴한다.
//			RETURN_IF(bytes_transfered < 0 && api::get_last_error() == EWOULDBLOCK, 0);
//
//			// - bytes_transfered가 0보다 작으면 비정상 접속 종료이므로 REASON_FAIL을 설정한다.
//			if(bytes_transfered != 0)
//			{
//				m_disconnect_reason	|= DISCONNECT_REASON_FAIL;
//			}
//
//			// - 종료 메시지를 넣는다.
//			queue_close_message();
//
//			// return) 
//			return intptr_t(0);
//		}
//
//		// statistics) socket의 receive통계.
//		statistics_on_receive_bytes(bytes_transfered);
//
//		// 6) 만약 이전에 저장된 데이터가 있었으면 먼저 복사한다.
//		if(m_buffer_received.size() != 0)
//		{
//			buffer_receive.append(m_buffer_received.size(), m_buffer_received.data());
//		}
//
//		// 7) 받은 Size를 더한다.
//		buffer_receive.add_size(bytes_transfered);
//
//		// 8) sustain을 위한 접속종료
//		if (m_byte_queued > m_max_received_bytes_queued)
//		{
//			// statistics) Error로 인한 disconnect임을 표시한다.
//			statistics_set_error_disconnect();
//
//			// - process_disconnect한다.
//			process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
//
//			// - 종료 메시지를 넣는다.
//			queue_close_message();
//
//			// return) 
//			return intptr_t(1);
//		}
//
//		// 3) Queuing한다.
//		queue_received_message(m_buffer_receiving ^ _transferred);
//	}
//	catch(std::exception& e)
//	{
//		//-----------------------------------------------------------------
//		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
//		//
//		// * on_receive()는 User Definine함수이므로 이 함수 수행중 Exception발생가능성이 있다.
//		// * process_packet() 역시 수행중 Exception이 발생가능하다.
//		//-----------------------------------------------------------------
//		// trace) 
//		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;
//
//		// - call 'exception handler'
//		throwable::execute_exception_handler(this, e, __FUNCTION__);
//
//		// statistics) Error로 인한 disconnect임을 표시한다.
//		statistics_set_error_disconnect();
//
//		// - disconnect를 수행한다.
//		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
//
//		// - disconnect를 수행한다.
//		process_complete_disconnect();
//
//		// return)
//		return	intptr_t(0);
//	}
//	catch(...)
//	{
//		//-----------------------------------------------------------------
//		// exception) 위의 Exception과 동일한 설명
//		//-----------------------------------------------------------------
//		// trace) 
//		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;
//
//		// declare)
//		throwable::failure	e(eRESULT::EXCEPTION);
//
//		// - call 'exception handler'
//		throwable::execute_exception_handler(this, e, __FUNCTION__);
//
//		// statistics) Error로 인한 disconnect임을 표시한다.
//		statistics_set_error_disconnect();
//
//		// - disconnect를 수행한다.
//		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
//
//		// - disconnect를 수행한다.
//		process_complete_disconnect();
//
//		// return)
//		return	intptr_t(0);
//	}
//
//	// 2) 새로 receive를 건다.
//	if(receive()==false)
//	{
//		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
//		process_complete_disconnect();
//	}
//
//	// return)
//	return	intptr_t(0);
//}
//
//
//
//////-----------------------------------------------------------------
////// 4. 패킷화 처리하기
//////-----------------------------------------------------------------
////// declare)
////std::size_t required_bytes;
////net::sockaddr address_peer;
////
////// 2) 일단 process_on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
////process_on_receive(buffer_receive^ bytes_transfered, address_peer);
////
////// 3) Process message를 수행한다.
//////    - 여기서 Return해 오는 값은 다음 message를 받기위해 대음에 필요로 하는 메모리 용량이다.
//////    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
//////      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
//////    - Exception이 발생가능하다.
////required_bytes = process_packet(buffer_receive, address_peer);
////
//
////// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
////THROW_IF(required_bytes > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string<char>());
////
////// 4) 처리하고 남은 Bytes들을 저장해 놓는다.
////if (buffer_receive.size() != 0)
////{
////	m_buffer_received = buffer_receive;
////}
////else
////{
////	m_buffer_received.clear();
////}
//
//
//
//
//bool net::io::receivable::Nstream_queue_rio::post_received_buffer()
//{
//	// 1) add_ref
//	add_ref();
//
//	// 3) Overlapped receive를 하나 늘인다.
//	statistics_on_increase_async_receiveing();
//
//	// 4) io_context를 얻는다.
//	auto pio_context = io_context();
//
//	// 5) io_context가 null이면 기본 io_context를 얻는다.
//	if(pio_context.empty())
//	{
//		pio_context = system_executor::executor();
//	}
//
//	// 6) ProcessReceived를 건다.
//	if(pio_context->post(m_pexecutable_received)==false)
//	{
//		// - PostReceived가 실패했을 경우 여기서 바로 실행을 한다.
//		process_received_buffer();
//
//		// - release
//		release();
//
//		// return) 
//		return false;	
//	}
//
//	// statistics) 
//	statistics_on_receive_async();
//
//	// return)
//	return true;
//}
//
//void net::io::receivable::Nstream_queue_rio::queue_close_message()
//{
//	//-----------------------------------------------------------------
//	// 1. 접속 종료시 처리
//	//    - 접속이 종료되었을 때 buffer(0,0,0)을 Queuing하고 끝낸다.
//	//    - 버퍼를 받아 처리하는 쪽에서 만약 buffer의 size_이 0이면
//	//      접속종료처리를 하도록 처리된다.
//	//-----------------------------------------------------------------
//	// declare) 
//	bool bPost = false;
//
//	scoped_lock(m_lockable_queue_received)
//	{
//		// 1) Empty엿으면 새로 PostReceive를 걸어야 한다.
//		bPost = m_queue_received.empty();
//
//		// 2) Abortive close일 경우 최대 1개만 남기고 모두 취소한다.
//		if((this->m_disconnect_reason & DISCONNECT_REASON_ABORTIVE) != 0)
//		{
//			while (m_queue_received.size()>1)
//			{
//				m_queue_received.pop_back();
//			}
//		}
//
//		// 2) Queuing한다.
//		m_queue_received.emplace_back();
//	}
//
//	// 3) execute를 건다.
//	if(bPost == true)
//	{
//		post_received_buffer();
//	}
//}
//
//void net::io::receivable::Nstream_queue_rio::queue_received_message( const shared_buffer& _buffer)
//{
//	// check) 
//	RETURN_IF(_buffer.size() == 0);
//
//	// declare) 
//	bool bPost = false;
//
//	scoped_lock(m_lockable_queue_received)
//	{
//		// 1) bPrePost인지 확인한다.(Empty라면 새로 Post해야 한다.)
//		bPost = m_queue_received.empty();
//
//		// 1) Queuing한다.
//		if(m_queue_received.size() <= 1 || m_queue_received.back().back() != _buffer.data())
//		{
//			m_queue_received.emplace_back(_buffer);
//		}
//		else
//		{
//			m_queue_received.back().add_size(_buffer.size());
//		}
//
//		// 2) Queueing된 버퍼의 크기를 계산한다.
//		m_byte_queued+= _buffer.size();
//	}
//
//	// 4) execute를 건다.
//	if(bPost)
//	{
//		post_received_buffer();
//	}
//}
//
//void net::io::receivable::Nstream_queue_rio::process_received_buffer()
//{
//	// declare)
//	shared_buffer buf_buffer;
//	net::sockaddr address_peer;
//
//	// 주의) 여기서는 Lock을 걸필요도 없다.
//	//       m_queue_received의 front가 있다고 가정하고 이 처리를 진행하는 것이고
//	//       m_queue_received의 front는 여기서 처리하기 전에는 누구도 건드리지 않는다.
//
//	// check) Received Queue의 크기는 0이 아니어야 한다.
//	CGASSERT_ERROR(m_queue_received.size() != 0);
//
//	// 1) 연속된 버퍼일 경우 연속되도록 Loop를 돌며 연결한다.
//	buf_buffer = m_queue_received.front();
//
//	// 2) Queued된 량을 줄인다.
//	m_byte_queued -= buf_buffer.size();
//
//	// 4) 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
//	process_on_receive(buf_buffer, address_peer);
//
//	// statistics) socket의 receive통계.
//	statistics_on_receive_bytes(buf_buffer.size());
//
//	// declare) 
//	shared_buffer buf_processing = m_buf_processing;
//
//	try
//	{
//		// CASE 5-1) 0일 경우 -> 종료처리
//		if(buf_buffer.size() == 0)
//		{
//			// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
//			process_complete_disconnect();
//
//			// - release
//			release();
//
//			// return) 그냥 끝낸다!
//			return;
//		}
//		// Case 5-1) buffer가 연결된 경우
//		else if((buf_processing.data() + buf_processing.size()) == buf_buffer.data())
//		{
//			// - buffer를 늘려주기만 하면 된다.
//			buf_processing.add_size(buf_buffer.size());
//
//			// - Process Packet한다.
//			auto bytes_required = process_packet(buf_processing, address_peer);
//
//			// check)
//			CGASSERT_ERROR(bytes_required > buf_processing.size());
//
//			// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
//			CGASSERT_ERROR(bytes_required <= m_maximum_message_buffer_size);
//
//			// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
//			THROW_IF(bytes_required > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);
//
//			// - m_bytes_required를 저장한다.
//			m_bytes_required = bytes_required;
//		}
//		// Case 5-2) 기본 buffer가 완전히 빈경우.
//		else if(buf_processing.size() == 0)
//		{
//			// - 그대로 덥어 쓴다.
//			buf_processing = buf_buffer;
//
//			// - Process Packet한다.
//			auto bytes_required = process_packet(buf_processing, address_peer);
//
//			// check)
//			CGASSERT_ERROR(bytes_required > buf_processing.size());
//
//			// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
//			CGASSERT_ERROR(bytes_required <= m_maximum_message_buffer_size);
//
//			// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
//			THROW_IF(bytes_required > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);
//
//			// - m_bytes_required를 저장한다.
//			m_bytes_required = bytes_required;
//		}
//		else
//		{
//			// - 조각난 연결부위를 잇는다.
//			for(;;)
//			{
//				// - 임시 버퍼를 할당한다.
//				auto temp_buffer = alloc_shared_buffer(m_bytes_required + buf_processing.size());
//
//				// - 이미 받아 놓은 데이터를 먼저 복사해 넣는다.
//				temp_buffer.append(buf_processing.size(), buf_processing.data());
//
//				// check) 
//				CGASSERT_ERROR(m_bytes_required>buf_processing.size());
//
//				// - 더 복사해 넣어야할 량을 계산한다.
//				auto more_needed = m_bytes_required- buf_processing.size();
//
//				// - 나머지 데이터는 새로 받은 데이터에서 복사해 넣는다.
//				if(buf_buffer.size() < more_needed)
//				{
//					temp_buffer.append(buf_buffer.size(), buf_buffer.data());
//					buf_processing = temp_buffer;
//					break;
//				}
//
//				// - 패킷을 완성한다.
//				temp_buffer.append(more_needed, buf_buffer.data());
//
//				// - Process Packet한다.
//				auto bytes_required = process_packet(temp_buffer, address_peer);
//
//				// check) 요구된 바이트수가 현재 버퍼에 남아 있는 바이트수보다 반드시 커야 한다.
//				CGASSERT_ERROR(bytes_required > temp_buffer.size());
//
//				// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
//				CGASSERT_ERROR(bytes_required <= m_maximum_message_buffer_size);
//
//				// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
//				THROW_IF(bytes_required > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);
//
//				// - m_bytes_required를 저장한다.
//				m_bytes_required = bytes_required;
//
//				// check) socket의 접속이 종료되었으면 그냥 끝낸다.
//				BREAK_IF(native_handle()==INVALID_SOCKET);
//
//				// check) temp_buffer.lne이 0이 아니라면 패킷의 길이가 모자라 처리를 하지 못했다는 의미이다.
//				//        이런 경우는 m_bytes_required가 Header의 크기였을 경우 발생할 수 있다.
//				CONTINUE_IF(temp_buffer.size() != 0);
//
//				// - 패킷이 처리되었다.(temp_buffer.size_이 0라면 패킷처리가 진행되었다는 의미이다.)
//				buf_processing = buf_buffer + offset(more_needed);
//
//				// - process_packet한다.
//				if(buf_processing.size() != 0)
//				{
//					bytes_required = process_packet(buf_processing, address_peer);
//
//					// check) 요구된 바이트수가 현재 버퍼에 남아 있는 바이트수보다 반드시 커야 한다.
//					CGASSERT_ERROR(bytes_required > buf_processing.size());
//
//					// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
//					CGASSERT_ERROR(bytes_required <= m_maximum_message_buffer_size);
//
//					// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
//					THROW_IF(bytes_required > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);
//
//					// - m_bytes_required를 저장한다.
//					m_bytes_required = bytes_required;
//				}
//
//				// - Break!
//				break;
//			}
//		}
//
//		// - 처리된 값 저장.
//		 m_buf_processing = buf_processing;
//	}
//	// exception) 해당 socket을 끊고 끝낸다.
//	catch(std::exception& e)
//	{
//		// trace) 
//		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;
//
//		// - call 'exception handler'
//		throwable::execute_exception_handler(this, e, __FUNCTION__);
//
//		// statistics) Error로 인한 disconnect임을 표시한다.
//		statistics_set_error_disconnect();
//
//		// - disconnect를 수행한다.
//		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
//
//		// - disconnect를 수행한다.
//		process_complete_disconnect();
//
//		// - release
//		release();
//
//		// return)
//		return;
//	}
//	catch(...)
//	{
//		// trace) 
//		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;
//
//		// declare)
//		throwable::failure e(eRESULT::EXCEPTION);
//
//		// - call 'exception handler'
//		throwable::execute_exception_handler(this, e, __FUNCTION__);
//
//		// statistics) Error로 인한 disconnect임을 표시한다.
//		statistics_set_error_disconnect();
//
//		// - disconnect를 수행한다.
//		process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
//
//		// - disconnect를 수행한다.
//		process_complete_disconnect();
//
//		// - release
//		release();
//
//		// return)
//		return;
//	}
//
//	// declare) 
//	bool bPost = false;
//
//	// 6) 비어 있는지 확인한다.
//	scoped_lock(m_lockable_queue_received)
//	{
//		// - Pop front한다.
//		m_queue_received.pop_front();
//
//		// - 비어 있는지를 확인한다.
//		bPost = m_queue_received.empty();
//	}
//
//	// 7) 다시 execute를 건다.
//	if(bPost == false)
//	{
//		post_received_buffer();
//	}
//}

#endif