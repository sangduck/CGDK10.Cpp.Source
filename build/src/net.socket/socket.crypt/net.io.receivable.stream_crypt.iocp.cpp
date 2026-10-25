#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;

extern std::size_t g_bytes_message_buffer_default_min_size;
extern std::size_t g_bytes_message_buffer_default_max_size;


intptr_t net::io::receivable::Nstream_crypt::process_complete_receivable( intptr_t _result, std::size_t _transferred)
{
	//-----------------------------------------------------------------
	// 설명) process_execute
	//
	// - receive가 완료된 경우는 Overlapped로 동작하는 곳에서는 해당
	//   socket으로부터 데이터를 전송받거나 혹은 접속이 종료되었을 때
	//	 불려진다.
	// - GetQueuedCompletionPort에서 Port가 닫치게 되면 dwReturn값이 
	//   ERROR_SUCCESS값이 return된다.
	// - dwReturn이 0이 아닌 경우는 일단 정상적인 Overlapped I/O의
	//   완료라고 할 수 있다.
	// - Return값이 ERROR_SUCCESS값이 돌아오게 되면 접속 종료로 처리를
	//   한다.
	// - 정상적인 데이터가 들어오게 되면 일단은 process_packet함수를 
	//   수행해서 message의 처리를 수행하게 된다.
	// - process_packet()이후에 새로 Overlapped receive를 걸어준다. 
	//-----------------------------------------------------------------
	// check) 
	CGASSERT(socket_state() != eSOCKET_STATE::SYN, static_cast<intptr_t>(1));

	// check) 만약 전송된 Byte수가 0Byte면 접속을 종료처리한다.
	if(_transferred == 0)
	{
		// - 비정상 접속종료일 경우 REASON_FAIL을 설정한다.
		if(_result != 0)
			this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return) 실패!!!
		return intptr_t(1);
	}

	try
	{
		//-----------------------------------------------------------------
		// 2. 전송되어 왔을 때 처리
		//    - 복화화
		//
		//    (주의) 복호화 과정에서 _transferred크기는 복호화된 데이터 크기로 변경된다.
		//-----------------------------------------------------------------
		// statistics) socket의 receive통계.
		this->statistics_on_receive_bytes(_transferred);

		// declare) the received bytes were taken as plain (no cryptor)
		bool is_plain = false;

		// 1) 복호화 처리
		{
			// - receive 받은 shared_buffer를 얕은 복사한다. (막 받은 buffer)
			shared_buffer temp_source = this->m_buffer_received ^ buffer_view(this->m_wsabuf_receiving.buf, static_cast<uint32_t>(_transferred));

			// - no cryptor : taken as plain (the cryptor may be set while they are processed : process_decrypt_received_after)
			is_plain = this->get_cryptor().empty();

			// - 복호화한다. (receive Encrypt를 수행하는 함수.)
			auto buf_decrypt = this->process_decryptor(temp_source);

			// - get decrypted size
			_transferred = buf_decrypt.size();

			// - 복호화된 데이터 크기가 남음 buffer보다 크면 새로 생성 후 복사.
			if (this->m_wsabuf_receiving.len < _transferred)
			{
				// declare)
				auto buffer_received_size = this->m_buffer_received.size();

				// - 새로 만들 buffer의 크기
				std::size_t size_prepare = buffer_received_size + _transferred;

				auto buf_new = alloc_shared_buffer(size_prepare);

				// - 새로 할당된 shared_buffer에 처리되지 않고 남은 Byte를 복사해 준다.
				if (buffer_received_size != 0)
					memcpy(buf_new.data(), this->m_buffer_received.data(), buffer_received_size);

				// - [새_수신_버퍼]를 [수신_버퍼](m_wsabuf_receiving)에 설정한다.
				this->m_wsabuf_receiving = WSABUF{ static_cast<ULONG>(_transferred), buf_new.data() + buffer_received_size };

				// - [이미_수신한_데이터_버퍼]에 버퍼를 설정한다.
				buf_new ^= buffer_received_size; this->m_buffer_received = std::move(buf_new);
			}

			// - append decrypted data to buffser
			this->m_buffer_received += buf_decrypt;
		}

	// debug)
	#if !defined(NDEBUG) || defined(_DEBUG)
		if(this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(this->native_handle(), _transferred, this->m_buffer_received.size());
	#endif

		//-----------------------------------------------------------------
		// 4. 패킷화 처리하기
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

		// 1) 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
		//    (the decrypted bytes : appended at m_wsabuf_receiving.buf. it was 'buf + len' : past the received bytes)
		this->process_on_receive(buffer_view{ this->m_wsabuf_receiving.buf, _transferred }, address_peer);

		// - the bytes taken as plain (process_decrypt_received_after)
		if (is_plain)
		{
			this->m_preceived_plain_begin = this->m_wsabuf_receiving.buf;
			this->m_preceived_plain_end = this->m_wsabuf_receiving.buf + _transferred;
		}
		else
		{
			this->m_preceived_plain_begin = nullptr;
			this->m_preceived_plain_end = nullptr;
		}

		// 2) Process message를 수행한다.
		//    - 여기서 Return해 오는 값은 다음 message를 받기 위해 다음에 필요로 하는 메모리 용량이다.
		//    - 주어지는 &m_buffer_received에 진행된 Memory과 남은 Size등이 되돌아 온다.
		//      따라서 m_buffer_received은 받은 데이터 그대로 사용하면 된다.
		//    - Exception이 발생가능하다.
		required_bytes = this->process_packet(this->m_buffer_received, address_peer);

		// - done
		this->m_preceived_plain_begin = nullptr;
		this->m_preceived_plain_end = nullptr;

		// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
		if (required_bytes > this->m_maximum_message_buffer_size)
		{
			LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << to_string(remote_endpoint());
			throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, 0);
		}


		//-----------------------------------------------------------------
		// 5. shared_buffer 체크한다.
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
			// - 새로 할당된 shared_buffer에 처리되지 않고 남은 Byte를 복사해 준다.
			if(buffer_received_size != 0)
				memcpy(buf_new.data(), this->m_buffer_received.data(), buffer_received_size);

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
	catch(std::exception& _e)
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
		// exception) 위의 Exception과 동일한 설명

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

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
		// - 비정상 접속종료일 경우 REASON_FAIL을 설정한다.
		this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();

		// return) 
		return intptr_t(1);
	}


	//-----------------------------------------------------------------
	// 3. Overlapped receive를 걸어준다.
	//    - receive에 실패했을 경우 접속을 바로 종료처리한다.
	//-----------------------------------------------------------------
	// 1) 새로 receive를 건다.
	if(this->receive()==false)
	{
		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// - 버퍼들을 할당해제한다.
		this->process_clear_buffer();
	}

	// return) 
	return intptr_t(0);
}
#endif