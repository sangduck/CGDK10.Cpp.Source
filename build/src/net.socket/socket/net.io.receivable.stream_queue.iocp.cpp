#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


intptr_t net::io::receivable::Nstream_queue::executable_receiving::process_execute( intptr_t _result, std::size_t _param)
{
	// 1) process_complete_receiveable을 호출한다.
	auto result = this->m_preceivable_stream->process_complete_receivable(_result, _param);

	// statistics) receive가 완료되었으므로 걸려있는 Overlapped I/O의 갯수를 줄인다.
	this->m_preceivable_stream->statistics_on_decrease_async_receiveing();

	// 2) release한다.
	this->m_preceivable_stream->release();

	// return) 
	return result;
}

void net::io::receivable::Nstream_queue::executable_receiving::on_final_release() noexcept
{
}

intptr_t net::io::receivable::Nstream_queue::executable_received::process_execute( intptr_t /*_result*/, std::size_t /*_param*/)
{
	// 1) process_complete_receiveable을 호출한다.
	this->m_preceivable_stream->process_received_buffer();
	
	// statistics) receive가 완료되었으므로 걸려있는 Overlapped I/O의 갯수를 줄인다.
	this->m_preceivable_stream->statistics_on_decrease_async_receiveing();

	// 2) release한다.
	this->m_preceivable_stream->release();

	// return) 
	return intptr_t(0);
}

net::io::receivable::Nstream_queue::Nstream_queue() :
	m_buffer_receiving(alloc_shared_buffer(g_bytes_message_buffer_default_min_size)),
	m_lockable_queue_received(), 
	m_queue_received(), 
	m_byte_queued_receive(0),
	m_buf_processing(),
	m_bytes_required(0),
	m_max_received_bytes_queued(DEFAULT_BYTES_MAX_MESSAGE_RECEIVE_QUEUE),
	m_minimum_mesage_buffer_size(g_bytes_message_buffer_default_min_size),
	m_maximum_message_buffer_size(g_bytes_message_buffer_default_max_size)
{
	// check) 최소 MINIMUM_REMAINED_RECEIVE_BUFFER보다는 크거나 같아야 한다.
	CGASSERT_ERROR(this->m_minimum_mesage_buffer_size >= MINIMUM_REMAINED_RECEIVE_BUFFER);

	// check) MAX_TCP_MESSAGE_SIZE보다는 같거나 작아야 한다.
	CGASSERT_ERROR(this->m_maximum_message_buffer_size <= MAX_TCP_MESSAGE_SIZE);

	// 1) executable Receiving 객첼를 생성한다.
	this->m_pexecutable_receiving = make_object<executable_receiving>();
	this->m_pexecutable_receiving->m_preceivable_stream = this;

	// 2) executable Received 객첼를 생성한다.
	this->m_pexecutable_received = make_object<executable_received>();
	this->m_pexecutable_received->m_preceivable_stream = this;
}

net::io::receivable::Nstream_queue::~Nstream_queue() noexcept
{
	this->m_pexecutable_receiving.reset();
	this->m_pexecutable_received.reset();
}

std::size_t net::io::receivable::Nstream_queue::minimum_message_buffer_size(std::size_t _size) noexcept
{
	// 1) 기존의 Min receive buffer 크기를 저장해 놓는다.
	std::size_t	old	= this->m_minimum_mesage_buffer_size;

	// 2) 최하크기 이하일 경우 최하크기로 맞춘다.
	SATURATE_MIN(_size, MINIMUM_REMAINED_RECEIVE_BUFFER)

	// check) 이전의 값과 동일하면 그냥 끝낸다.
	RETURN_IF(old == _size, old);

	// 3) 값을 변경한다.
	this->m_minimum_mesage_buffer_size = _size;

	// 4) 
	if (this->m_minimum_mesage_buffer_size > this->m_maximum_message_buffer_size)
	{
		this->m_maximum_message_buffer_size = this->m_minimum_mesage_buffer_size;
	}

	// return) 
	return old;
}

std::size_t net::io::receivable::Nstream_queue::minimum_message_buffer_size() noexcept
{
	return this->m_minimum_mesage_buffer_size;
}

std::size_t net::io::receivable::Nstream_queue::maximum_message_buffer_size(std::size_t _size) noexcept
{
	// 1) 기존의 Min receive buffer 크기를 저장해 놓는다.
	std::size_t	old = this->m_maximum_message_buffer_size;

	// check) 이전의 값과 동일하면 그냥 끝낸다.
	RETURN_IF(old == _size, old);

	// 3) 값을 변경한다.
	this->m_maximum_message_buffer_size = _size;

	// 4) 
	if (this->m_minimum_mesage_buffer_size > this->m_maximum_message_buffer_size)
	{
		this->m_minimum_mesage_buffer_size = this->m_maximum_message_buffer_size;
	}

	// return) 
	return old;
}

std::size_t net::io::receivable::Nstream_queue::maximum_message_buffer_size() noexcept
{
	return this->m_maximum_message_buffer_size;
}

void net::io::receivable::Nstream_queue::process_reset_receivable()
{
	// 1) size를 얻는다.
	auto size_alloc = this->minimum_message_buffer_size();

	// 2) buffer를 setting한다.
	this->m_buffer_receiving = alloc_shared_buffer(size_alloc) ^ size_alloc;

	// 3) Received Queue를 clear한다.
	scoped_lock(this->m_lockable_queue_received)
	{
		this->m_queue_received.clear();
		this->m_byte_queued_receive = 0;
	}

	// 5) Processing buffer를 reset한다.
	this->m_buf_processing = this->m_buffer_receiving ^ 0 ;
	this->m_bytes_required = 0;
}

bool net::io::receivable::Nstream_queue::receive()
{
	// check) socket상태가 연결된 상태인가?
	CGASSERT(this->m_buffer_receiving.data() != nullptr, false)

	// check) m_buffer_receiving.size_가 0이어서는 안된다.
	CGASSERT(this->m_buffer_receiving.size() != 0, false)

	// check) Invalid socket인가?( release에서는 만약 socket Handle이 잘못되었으면 receive함수가 자동 실패한다.)
	RETURN_IF(this->native_handle() == INVALID_SOCKET, false)

	// 1) reference count를 증가시킨다.
	this->add_ref();

	// 2) m_pexecutable_receiving의 reference count를 증가시킨다.
	this->m_pexecutable_receiving->add_ref();

	// 3) overlapped receive를 하나 늘인다.
	this->statistics_on_increase_async_receiveing();

	// 4) set WSABUF
	this->m_wsabuf_receiving = WSABUF{ this->m_buffer_receiving.size<ULONG>(), this->m_buffer_receiving.data() };
	this->m_wsa_bytes_received = 0;
	this->m_wsa_flag = 0;

	// 5) 보낸다.
	if(api::socket::recv_async(this->native_handle(), &this->m_wsabuf_receiving, &this->m_wsa_bytes_received, &m_wsa_flag, this->m_pexecutable_receiving) == false)
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

bool net::io::receivable::Nstream_queue::prepare_receive()
{
	// check)
	CGASSERT_ERROR(this->m_buf_processing.size() == 0);

	// 1) Reference count를 증가시킨다.
	this->add_ref();

	// 2) receive한다.
	auto result = this->receive();

	// check) receive가 실패했다면 Roll back~
	if (result == false)
	{
		// - 앞의 1)에서 add_ref했던 것 release
		this->release();
	}

	// return) 
	return result;
}

intptr_t net::io::receivable::Nstream_queue::process_complete_receivable( intptr_t _result, std::size_t _transferred)
{
	//-----------------------------------------------------------------
	// 설명) 데이터를 수신을 받았을 경우
	//       1) 과도한 처리되지 않는 receive 데이터가 큐잉되어 있는지 확인해 
	//          과도한 receive를 받았을 경우 종료 처리를 한다.
	//       2) 접속이 종료된 것인지를 확인해 접속 종료되었다면 그 처리를 해준다.
	//       3) 일단 신규로 받은 데이터를 queuing한다.
	//       4) 남아 있는 receive buffer를 확인 후 receive를 받을 buffer를 준비한다.
	//       5) 다시 receive를 걸어 준다.
	//  
	//-----------------------------------------------------------------

	// check)
	CGASSERT(this->socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// 1) 너무 많은 처리 되지 않는 receive 데이터가 누적되었을 시 접속 종료 처리한다. (sustain을 위한 접속종료)
	if(this->m_byte_queued_receive > this->m_max_received_bytes_queued)
	{
		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - 종료 메시지를 넣는다.
		this->queue_close_message();

		// return) 
		return intptr_t(1);
	}

	// 2) 만약 receive가 실패했거나 전송된 바이트수가 0이면 종료처리한다.
	if (_result != 0)
	{
		// - Reason을 Abortive로 설정한다.
		this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

		// - Transferred 된 것이 있으면 해당 message를 Queueing한다.
		if (_transferred != 0)
		{
			this->queue_received_message(m_buffer_receiving ^ _transferred);
		}

		// - 종료 메시지를 넣는다.
		this->queue_close_message();

		// return) 
		return intptr_t(1);
	}
	else if (_transferred == 0)
	{
		// - 종료 메시지를 넣는다.
		this->queue_close_message();

		// return) 
		return intptr_t(1);
	}

	// 3) Queuing한다.
	this->queue_received_message(m_buffer_receiving ^ _transferred);

	// 4) receive를 받을 메모리를 다시 준비한다.
	{
		// - receiving buffer를 계산함.
		this->m_buffer_receiving += offset(_transferred);

		// debug)
#if !defined(NDEBUG) || defined(_DEBUG)
		if (this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(this->native_handle(), _transferred, this->m_buffer_receiving.size());
#endif

		// - Required bytes를 얻어 놓는다.
		auto bytes_required = this->m_bytes_required;

		// - 다시 receive를 하기 위해 버퍼의 크기를 검사한다.
		//    - 버퍼를 완전히 다 채웠다면 새 버퍼를 할당받아 receive를 건다.
		if (this->m_buffer_receiving.size() < MINIMUM_REMAINED_RECEIVE_BUFFER || this->m_buffer_receiving.size() < bytes_required)
		{
			// - [필요_버퍼크기]를 구한다.(receive받은 크기 + 이미 받아 놓은 데이터 크기)
			const std::size_t size_compare = api::socket::get_pending_read_buffer(native_handle()) + _transferred;

			// - [준비할_버퍼_크기]의 [필요_버퍼크기]의 최소 8의 승수로  만든다.(8,16,128, 1024, 8192, 64k, 256k, ...)
			//   (이렇게 8의 배수로 하는 이유는... 너무 단계를 많이 하게 될 경우 오히리 사용하는 메모리 버퍼 풀의 종류가 많아져
			//   각 풀에 재고가 너무 많이 남아 있어 메모리 사용량을 늘릴 수 있으므로 단계를 줄이기 위해서 8의 승수로
			//   단계를 줄여 사용하는 풀의 종류를 줄인다.)
			std::size_t size_prepare = this->m_minimum_mesage_buffer_size; while (size_compare > size_prepare) size_prepare *= 8;

			// - [최소_TCP_수신_버퍼_크기]보다는 크게 한다.
			SATURATE_MAX(size_prepare, this->m_maximum_message_buffer_size);

			// - 최소 [요구_버퍼_크기]+[이미_수신한_데이터_크기]보다는 크야 한다.
			size_prepare = __select_max(size_prepare, bytes_required);

			// - [새_수신_버퍼]를 [준비할_버퍼량] 크기만큼 할당한다.
			//   exception) Memory 할당을 받지 못했을 경우라면... Error처리를 위해 Throw한다.
			//   * Size가 Pool이 할당할 수 있는 최대 크기(일반적으로 256Kbytes)이상일 경우 발생한다.
			//   * 메모리 부족으로 인해 더이상 할당받지 못했을 경우에 뜬다.
			//   [새_수신_버퍼]를 [준비할_버퍼량] 크기만큼 할당해 [수신_버퍼](m_buffer_receiving)에 설정한다.
			this->m_buffer_receiving = alloc_shared_buffer(size_prepare) ^ size_prepare;
		}
	}

	// 5) 새로 receive를 건다.
	if(this->receive() == false)
	{
		// - process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - post close message
		this->queue_close_message();
	}

	// return)
	return intptr_t(0);
}

bool net::io::receivable::Nstream_queue::post_received_buffer()
{
	// 1) add_ref
	this->add_ref();

	// 3) Overlapped receive를 하나 늘인다.
	this->statistics_on_increase_async_receiveing();

	// 4) io_context를 얻는다.
	auto pio_context = this->io_context();

	// 5) io_context가 null이면 기본 io_context를 얻는다.
	if(pio_context.empty())
	{
		pio_context = system_executor::executor();
	}

	// 6) ProcessReceived를 건다.
	if(pio_context->post(this->m_pexecutable_received) == false)
	{
		// - PostReceived가 실패했을 경우 여기서 바로 실행을 한다.
		this->process_received_buffer();

		// - release
		this->release();

		// return) 
		return false;	
	}

	// statistics) 
	this->statistics_on_receive_async();

	// return)
	return true;
}

void net::io::receivable::Nstream_queue::queue_close_message()
{
	//-----------------------------------------------------------------
	// 설명) 접속 종료시 처리
	//    - 여기서 바로 접속 종료처리를 해서는 안됀다.
	//      (receive 데이터를 처리하는 쪽에서 접속종료처리를 하도록 한다.)
	//    - 따라서 buffer(0,0,0)을 queuing하고 끝낸다.
	//    - 그러면 receive 데이터를 처리하는 쪽에서 접속종료처리를 한다.
	//-----------------------------------------------------------------
	// declare) 
	bool is_post = false;

	scoped_lock(this->m_lockable_queue_received)
	{
		// 1) Empty엿으면 새로 PostReceive를 걸어야 한다.
		is_post = this->m_queue_received.empty();

		// declare)
		size_t cancled_bytes = 0;

		// 2) Abortive close일 경우 최대 1개만 남기고 모두 취소한다.
		if((this->m_disconnect_reason & DISCONNECT_REASON_ABORTIVE) != 0)
		{
			while (this->m_queue_received.size() > 1)
			{
				// 3) queueing된 버퍼의 크기를 계산한다.
				cancled_bytes += this->m_queue_received.back().size();
				this->m_queue_received.pop_back();
			}
		}

		// 3) 취소된 bytes들을 빼준다.
		this->m_byte_queued_receive -= cancled_bytes;

		// 4) queuing한다.
		this->m_queue_received.emplace_back();
	}

	// 5) execute를 건다.
	if(is_post == true)
	{
		this->post_received_buffer();
	}
}

void net::io::receivable::Nstream_queue::queue_received_message( const shared_buffer& _buffer)
{
	//-----------------------------------------------------------------
	// 설명) receive 데이터 queuing
	//    - receive받은 데이터를 queuing한 후에 다른 쓰레드에서 처리하도록 한다.
	//    - 일단 this->m_queue_received에 추가한 다음.
	//    - post_received_buffer()함수를 호출해 다른 쓰레드에서 
	//      receive 데이터를 처리하도록 한다.
	//-----------------------------------------------------------------
	// check) 
	RETURN_IF(_buffer.size() == 0);

	// declare) 
	bool is_post = false;

	scoped_lock(this->m_lockable_queue_received)
	{
		// 1) bPrePost인지 확인한다.(Empty라면 새로 Post해야 한다.)
		is_post = this->m_queue_received.empty();

		// 2) Queuing한다.
		if(this->m_queue_received.size() <= 1 || this->m_queue_received.back().back() != _buffer.data())
		{
			this->m_queue_received.push_back(_buffer);
		}
		else
		{
			this->m_queue_received.back().add_size(_buffer.size());
		}

		// 3) queueing된 버퍼의 크기를 계산한다.
		this->m_byte_queued_receive += _buffer.size();
	}

	// 4) execute를 건다.
	if(is_post)
	{
		this->post_received_buffer();
	}
}

void net::io::receivable::Nstream_queue::process_received_buffer()
{
	//-----------------------------------------------------------------
	// 설명) qeueing된 receive 데이터 처리
	//    - queuing된 receive 데이터를 이 함수에서 처리한다.
	//    - queue에서 receive 데이터를 하나 꺼낸 후 process_packet()
	//      함수를 호출해 message를 처리한다. 
	//    - 문제는 message가 여러 buffer에 나누어 저장된 경우의 처리인데...
	//      나뉘어진 경우 하나의 버프로 합쳐 준 후 처리해 준다.
	//-----------------------------------------------------------------
	// declare)
	shared_buffer buf_buffer;
	net::sockaddr address_peer;

	scoped_lock(this->m_lockable_queue_received)
	{
		// check) 만약 정상적인 접속 종료가 아니라 오류로 접속이 죵료되었다면 recset되었을 수 있으므로 그럴 경우 바로 리턴한다.
		RETURN_IF(this->m_queue_received.empty(),);

		// 1) 연속된 버퍼일 경우 연속되도록 Loop를 돌며 연결한다.
		buf_buffer = std::move(this->m_queue_received.front());

		// check) 
		CGASSERT_ERROR(this->m_byte_queued_receive >= buf_buffer.size());

		// 2) Queued된 량을 줄인다.
		this->m_byte_queued_receive -= buf_buffer.size();
	}

	// 4) 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
	this->process_on_receive(buf_buffer, address_peer);

	// statistics) socket의 receive통계.
	this->statistics_on_receive_bytes(buf_buffer.size());

	// declare) 
	shared_buffer buf_processing = this->m_buf_processing;

	try
	{
		// CASE 5-1) 0일 경우 -> 종료처리
		if(buf_buffer.size() == 0)
		{
			// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
			this->process_complete_disconnect();

			// - release=
			this->release();

			// return) 그냥 끝낸다!
			return;
		}
		// Case 5-1) buffer가 연결된 경우
		else if((buf_processing.data() + buf_processing.size()) == buf_buffer.data())
		{
			// - buffer를 늘려주기만 하면 된다.
			buf_processing.add_size(buf_buffer.size());

			// - Process Packet한다.
			auto bytes_required = this->process_packet(buf_processing, address_peer);

			// check)
			CGASSERT_ERROR(bytes_required > buf_processing.size());

			// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
			CGASSERT_ERROR(bytes_required <= m_maximum_message_buffer_size);

			// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
			THROW_IF(bytes_required > m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);

			// - m_bytes_required를 저장한다.
			this->m_bytes_required = bytes_required;
		}
		// Case 5-2) 기본 buffer가 완전히 빈경우.
		else if(buf_processing.size() == 0)
		{
			// - 그대로 덥어 쓴다.
			buf_processing = buf_buffer;

			// - Process Packet한다.
			auto bytes_required = this->process_packet(buf_processing, address_peer);

			// check)
			CGASSERT_ERROR(bytes_required > buf_processing.size());

			// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
			CGASSERT_ERROR(bytes_required <= this->m_maximum_message_buffer_size);

			// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
			THROW_IF(bytes_required > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);

			// - m_bytes_required를 저장한다.
			this->m_bytes_required = bytes_required;
		}
		else
		{
			// - 조각난 연결부위를 잇는다.
			for(;;)
			{
				// - 임시 버퍼를 할당한다.
				auto temp_buffer = alloc_shared_buffer(m_bytes_required + buf_processing.size());

				// - 이미 받아 놓은 데이터를 먼저 복사해 넣는다.
				temp_buffer.append(buf_processing.size(), buf_processing.data());

				// check) 
				CGASSERT_ERROR(this->m_bytes_required>buf_processing.size());

				// - 더 복사해 넣어야할 량을 계산한다.
				auto more_needed = this->m_bytes_required - buf_processing.size();

				// - 나머지 데이터는 새로 받은 데이터에서 복사해 넣는다.
				if(buf_buffer.size() < more_needed)
				{
					temp_buffer.append(buf_buffer.size(), buf_buffer.data());
					buf_processing = temp_buffer;
					break;
				}

				// - 패킷을 완성한다.
				temp_buffer.append(more_needed, buf_buffer.data());

				// - Process Packet한다.
				auto bytes_required = this->process_packet(temp_buffer, address_peer);

				// check) 요구된 바이트수가 현재 버퍼에 남아 있는 바이트수보다 반드시 커야 한다.
				CGASSERT_ERROR(bytes_required > temp_buffer.size());

				// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
				CGASSERT_ERROR(bytes_required <= this->m_maximum_message_buffer_size);

				// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
				THROW_IF(bytes_required > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);

				// - m_bytes_required를 저장한다.
				this->m_bytes_required = bytes_required;

				// check) socket의 접속이 종료되었으면 그냥 끝낸다.
				BREAK_IF(this->native_handle()==INVALID_SOCKET);

				// check) temp_buffer.lne이 0이 아니라면 패킷의 길이가 모자라 처리를 하지 못했다는 의미이다.
				//        이런 경우는 m_bytes_required가 Header의 크기였을 경우 발생할 수 있다.
				CONTINUE_IF(temp_buffer.size() != 0);

				// - 패킷이 처리되었다.(temp_buffer.size_이 0라면 패킷처리가 진행되었다는 의미이다.)
				buf_processing = buf_buffer + offset(more_needed);

				// - process_packet한다.
				if(buf_processing.size() != 0)
				{
					bytes_required = this->process_packet(buf_processing, address_peer);

					// check) 요구된 바이트수가 현재 버퍼에 남아 있는 바이트수보다 반드시 커야 한다.
					CGASSERT_ERROR(bytes_required > buf_processing.size());

					// check) 요구한 message의 크기가 최대 message buffer 크기보다 작아야 한다.
					CGASSERT_ERROR(bytes_required <= this->m_maximum_message_buffer_size);

					// check) 요구한 message의 크기가 최대 message buffer 크기보다 크면 Exception을 던진다.
					THROW_IF(bytes_required > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, bytes_required, "message is too long lengnth"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message size is too BIG!"sv);

					// - m_bytes_required를 저장한다.
					this->m_bytes_required = bytes_required;
				}

				// - Break!
				break;
			}
		}

		// - 처리된 값 저장.
		 this->m_buf_processing = std::move(buf_processing);
	}
	// exception) 해당 socket을 끊고 끝낸다.
	catch (throwable::failure& _e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed (throwable) " << __CGFUNCTION__;
		LOG_DEBUG << "       - reason: " << NFMT::format("0x{:x}", _e.reason<uint64_t>()) << " [" << to_string<char>(_e.reason<eRESULT>()) << "]";
		if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - release
		this->release();

		// return)
		return;
	}
	// exception) 해당 socket을 끊고 끝낸다.
	catch(std::exception& _e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed (std::exception) " << __CGFUNCTION__;
		LOG_DEBUG << "       - " << _e.what();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - release
		this->release();

		// return)
		return;
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - release
		this->release();

		// return)
		return;
	}

	// declare) 
	bool is_post = false;

	// 6) 비어 있는지 확인한다.
	scoped_lock(this->m_lockable_queue_received)
	{
		// - Pop front한다.
		this->m_queue_received.pop_front();

		// - 비어 있는지를 확인한다.
		is_post = this->m_queue_received.empty();
	}

	// 7) 다시 execute를 건다.
	if(is_post == false)
	{
		this->post_received_buffer();
	}
}

#endif