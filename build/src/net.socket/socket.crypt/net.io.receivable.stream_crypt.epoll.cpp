#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


intptr_t net::io::receivable::Nstream_crypt::process_complete_receivable(intptr_t /*_result*/, std::size_t /*_transferred*/)
{
	// check) 
	CGASSERT_ERROR(this->socket_state() != eSOCKET_STATE::SYN);

	// declare)
	intptr_t result_return = EPOLLIN;

	try
	{
		// 1) get socket handle
		auto handle_socket = this->native_handle();

		// 2) get pended receive bytes
		auto bytes_receive_pended = api::socket::get_pending_read_buffer(handle_socket);

		// check) return if there are no pended receive data.
		if(bytes_receive_pended == 0)
			return result_return;

		// declare) 
		shared_buffer buffer_receive;

		// 3) if bytes_receive_peneded size is less than min_size, set as min_size
		auto buffer_size = __select_max(static_cast<std::size_t&>(g_bytes_message_buffer_default_min_size), bytes_receive_pended);

		// 4) alloc buffer 
		{
			// 설명) 256과 4*n승 크기로 saturation한다. 그렇지 않으면 너무 다양한 크기의 pool에서 메모리를 할당 받아 pool의 효율이 떨어진다.

			// - if bytes_receive_peneded size is less than min_size, set as min_size
			auto required_bytes = this->m_buffer_received.size() + buffer_size;

			// - saturate size
			std::size_t temp_size = 256; while (temp_size < required_bytes) (temp_size <<= 2);

			// - alloc shared buffer
			buffer_receive = alloc_shared_buffer(temp_size);
		}

		// 5) receive pended  data (offset for stored data)
		int bytes_transfered = ::recv(handle_socket, buffer_receive.data() + this->m_buffer_received.size(), static_cast<int>(buffer_size), MSG_DONTWAIT);

		// check) 
		if (bytes_transfered == 0)
			return result_return;

		// check) 
		if (bytes_transfered < 0 && api::get_last_error() != EAGAIN)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// return)
			return EPOLLHUP;
		}

		// check) EAGAIN (nothing to receive now : it went on with -1 bytes)
		if (bytes_transfered < 0)
			return result_return;

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// declare) the received bytes were taken as plain (no cryptor)
		bool is_plain = false;

		// 6) copy stored data to front of new buffer
		if (this->m_buffer_received.size() != 0)
			buffer_receive.append(this->m_buffer_received.size(), this->m_buffer_received.data());

		// ex) 복호화 처리
		{
			// - receive 받은 shared_buffer를 얕은 복사한다. (막 받은 buffer)
			//   (the received bytes are after the stored data : it was 'buffer_receive ^ bytes_transfered', the stored data (decrypted already)
			//   decrypted again and the tail of the received bytes not decrypted when data was stored)
			shared_buffer temp_source = buffer_receive ^ buffer_view(buffer_receive.data() + buffer_receive.size(), static_cast<std::size_t>(bytes_transfered));

			// - no cryptor : taken as plain (the cryptor may be set while they are processed : process_decrypt_received_after)
			is_plain = this->get_cryptor().empty();

			// - 복호화한다. (receive Encrypt를 수행하는 함수.)
			auto buf_decrypt = this->process_decryptor(temp_source);

			// - get decrypted size
			bytes_transfered = static_cast<int>(buf_decrypt.size());

			// - 복호화된 데이터 크기가 남음 buffer보다 크면 새로 생성 후 복사.
			//   (the stored data is copied to the new buffer : it was dropped, and the size came from the unused parameter _transferred)
			if (buffer_receive.get_source()->size() < (buffer_receive.size() + buf_decrypt.size()))
			{
				auto buf_new = alloc_shared_buffer(buffer_receive.size() + buf_decrypt.size());
				buf_new.append(buffer_receive.size(), buffer_receive.data());
				buffer_receive = std::move(buf_new);
			}

			// - 복호화된 데이터를 버퍼에 복사한다. (no cryptor : the bytes are there already)
			if (buf_decrypt.data() == buffer_receive.data() + buffer_receive.size())
				buffer_receive.add_size(buf_decrypt.size());
			else
				buffer_receive += buf_decrypt;
		}

		//  debug)
#if !defined(NDEBUG) || defined(_DEBUG)
		if (this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(handle_socket, bytes_transfered, buffer_receive.size());
#endif

		// declare)
		std::size_t required_bytes;
		net::sockaddr address_peer;

		// 10) call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
		//     (the received bytes : after the stored data. it was 'buffer_receive ^ bytes_transfered' : from the stored data)
		char* preceived = buffer_receive.data() + buffer_receive.size() - bytes_transfered;
		this->process_on_receive(buffer_view{ preceived, static_cast<std::size_t>(bytes_transfered) }, address_peer);

		// - the bytes taken as plain (process_decrypt_received_after)
		this->m_preceived_plain_begin = is_plain ? preceived : nullptr;
		this->m_preceived_plain_end = is_plain ? preceived + bytes_transfered : nullptr;

		// 11) process message
		//    - 여기서 Return해 오는 값은 다음 message를 받기 위해 다음에 필요로 하는 메모리 용량이다.
		//    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
		//      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
		//    - Exception이 발생가능하다.
		required_bytes = this->process_packet(buffer_receive, address_peer);

		// - done
		this->m_preceived_plain_begin = nullptr;
		this->m_preceived_plain_end = nullptr;

		// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
		if(required_bytes > this->m_maximum_message_buffer_size)
		{
			throwable::failure e(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH);
			LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string();
			throw e;
		}

		// 12) 처리하고 남은 Bytes들을 저장해 놓는다.
		if (buffer_receive.size() != 0)
			this->m_buffer_received = buffer_receive;
		else
			this->m_buffer_received.clear();
	}
	catch (throwable::failure& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 Exception이 발생가능하다.

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

		// - set result
		result_return = EPOLLHUP;
	}
	catch (std::exception& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (std::exception) " << __CGFUNCTION__;
		LOG_DEBUG << "       - " << _e.what();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - set result
		result_return = EPOLLHUP;
	}
	catch (...)
	{
		// exception) 위의 exception과 동일한 설명

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

		// - set result
		result_return = EPOLLHUP;
	}

	// return)
	return result_return;
}

#endif