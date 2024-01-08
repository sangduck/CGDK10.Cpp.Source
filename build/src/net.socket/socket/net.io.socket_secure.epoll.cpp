#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

extern size_t	g_bytes_message_buffer_default_min_size;
extern size_t	g_bytes_message_buffer_default_max_size;


void net::io::Nsocket_secure::process_reset_receivable()
{
	// 1) SSL관련 reset
	this->m_buffer_decrypt_receiving.clear();
	this->m_buffer_decrypt_received.clear();

	// 2) reset Crypt
	this->m_pcrypt.reset();

	// 3) reset
	this->net::io::receivable::tcp::Ndefault::process_reset_receivable();
}

void net::io::Nsocket_secure::process_reset_sendable() noexcept
{
	this->net::io::sendable::tcp::Ndefault::process_reset_sendable();
}

void net::io::Nsocket_secure::process_on_receive ( const buffer_view& _buffer, const net::sockaddr& _address)
{
	this->on_receive(_buffer, _address);
}

intptr_t net::io::Nsocket_secure::process_complete_receivable( intptr_t /*_result*/, std::size_t /*_transferred*/)
{
	// check) 
	CGASSERT(this->socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// declare)
	intptr_t result_return = EPOLLIN;

	try
	{
		// declare) peneded receive bytes
		int bytes_receive_pended = 0;

		// 1) get socket handle
		auto handle_socket = this->native_handle();

		// 2) get pended receive bytes
		::ioctl(handle_socket, FIONREAD, &bytes_receive_pended);

		// check) return if there are no pended receive data.
		RETURN_IF(bytes_receive_pended == 0, result_return);

		// declare) 
		shared_buffer buffer_receive;

		// 3) if bytes_receive_peneded size is less than min_size, set as min_size
		auto buffer_size = __select_max(static_cast<int>(g_bytes_message_buffer_default_min_size), bytes_receive_pended);

		// 4) alloc shared buffer
		buffer_receive = alloc_shared_buffer(m_buffer_received.size() + buffer_size);

		// 5) receive pended  data (offset for stored data)
		const int bytes_transfered = ::recv(handle_socket, buffer_receive.data() + m_buffer_received.size(), buffer_size, MSG_DONTWAIT);

		// check) 
		RETURN_IF(bytes_transfered <= 0, result_return);

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// 6) copy stored data to front of new buffer
		if (this->m_buffer_received.size() != 0)
		{
			buffer_receive.append(this->m_buffer_received.size(), this->m_buffer_received.data());
		}

		// 7) add size 
		buffer_receive.add_size(bytes_transfered);

		// declare)
		std::size_t required_bytes = MINIMUM_REMAINED_RECEIVE_BUFFER;
		net::sockaddr address_peer;

		// 8) decrypt
		if (m_pcrypt.exist())
		{
			std::size_t size_added = bytes_transfered + 256;	// transfered + 256

			// - Decrypt 함수를 확인한다.
			if (this->m_buffer_decrypt_receiving.size() < size_added)
			{
				// - 
				auto buffer_size_new = 65536 + size_added + this->m_buffer_decrypt_received.size();

				// - 새로운 버퍼를 할당한다.
				auto buf_new = alloc_shared_buffer(buffer_size_new) ^ buffer_size_new;

				// - 기존에 받아 높은 데이터를 복사한다.
				if (this->m_buffer_decrypt_received.size() != 0)
				{
					memcpy(buf_new.data(), this->m_buffer_decrypt_received.data(), this->m_buffer_decrypt_received.size());
				}

				// - 설정한다.
				this->m_buffer_decrypt_receiving = buf_new + offset(this->m_buffer_decrypt_received.size());
				this->m_buffer_decrypt_received = buf_new ^ this->m_buffer_decrypt_received.size();
			}

			// declare)
			buffer_view	buffder_decrypt_receiving = this->m_buffer_decrypt_receiving;

			// - Decrypt한다.
			auto result = this->m_pcrypt->decode(buffder_decrypt_receiving, reinterpret_cast<const_buffer_view*>(&this->m_buffer_received), 1);

			// check) result가 false면 
			if (result == true)
			{
				// check) 
				CGASSERT_ERROR(buffder_decrypt_receiving.size() > 0)

				// - 길이를 더한다.
				this->m_buffer_decrypt_received.add_size(buffder_decrypt_receiving.size());

				// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
				this->process_on_receive(m_buffer_decrypt_received ^ m_buffer_decrypt_receiving, address_peer);

				// - Pakcet을 처리한다.
				required_bytes = this->process_packet(m_buffer_decrypt_received, address_peer);

				// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
				THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << m_maximum_message_buffer_size << "bytes, ip:"sv << remote_endpoint().to_string<char>());

				// - 길이를 더해준다.
				this->m_buffer_decrypt_receiving += offset(buffder_decrypt_receiving.size());
			}
		}
		else
		{
			// 10) call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
			this->process_on_receive(buffer_receive ^ bytes_transfered, address_peer);

			// 11) process message
			//    - 여기서 Return해 오는 값은 다음 message를 받기위해 대음에 필요로 하는 메모리 용량이다.
			//    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
			//      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
			//    - Exception이 발생가능하다.
			required_bytes = this->process_packet(buffer_receive, address_peer);

			// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
			THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << this->m_maximum_message_buffer_size << "bytes, ip:"sv << remote_endpoint().to_string<char>());

			// 11) 처리하고 남은 Bytes들을 저장해 놓는다.
			if (buffer_receive.size() != 0)
			{
				this->m_buffer_received = buffer_receive;
			}
			else
			{
				this->m_buffer_received.clear();
			}
		}
	}
	catch (std::exception & e)
	{
		//-----------------------------------------------------------------
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Definine함수이므로 이 함수 수행중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행중 Exception이 발생가능하다.
		//-----------------------------------------------------------------
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

		// return)
		return result_return;
	}
	catch (...)
	{
		//-----------------------------------------------------------------
		// exception) 위의 Exception과 동일한 설명
		//-----------------------------------------------------------------
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed " __CGFUNCTION__ ""sv;

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

		// return)
		return result_return;
	}

	// return)
	return result_return;
}

bool net::io::Nsocket_secure::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& /*_powner*/, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_sync_buffered::process_sendable')"sv));

	// check) if socket is not connected, return false 
	RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

	// 1) send할  WSABuffer
	shared_buffer buffer_crypt;

	if(this->m_pcrypt.exist())
	{
		// - get buffer size
		uint32_t bytes_buffer = _buffer.size();

		// - Encryption과정에서 크기가 커질 수 있으므로일단 2배로 + Header(32)로 할당한다.
		bytes_buffer = bytes_buffer * 2 + 32;

		// declare) 
		buffer_crypt = alloc_shared_buffer(bytes_buffer);

		// - 전체를 SSL send를 한다.
		auto temp_dest= buffer_view(buffer_crypt.data(), bytes_buffer);

		__zero_memory(temp_dest.data(), temp_dest.size());

		// - encode를 한다.
		this->m_pcrypt->encode(temp_dest, reinterpret_cast<const const_buffer_view*>(&_buffer), 1);

		// - 최종...
		buffer_crypt.set_size(temp_dest.size());
	}
	else
	{
		static_cast<buffer_view&>(buffer_crypt) = _buffer;
	}

	// 2) send 한다.
	auto bytes_sended = ::send(native_handle(), buffer_crypt.data(), buffer_crypt.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		
	// check) bytes_sended가 0보다 작으면 전송에 실패한 것이다.
	if(bytes_sended >= 0)
	{
		this->process_complete_sendable(0, static_cast<uint32_t>(bytes_sended), _count_message);
	}
	else
	{
		// - get last error code
		auto error_code = api::get_last_error();

		// check)
		if (error_code == EWOULDBLOCK)
		{
			// statistics) 
			this->statistics_on_increase_async_sending();

			// - set EPOLLOUT event flag
			this->post_epoll_event_EPOLLOUT();
		}
		else
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics) 
			this->statistics_on_error_send();

			// - process_complete_sendable을 호출한다.(실패이므로 _result값은 1로, _Transfered는 0으로 전달한다.)
			this->process_complete_sendable(1, 0, 0);

			// return) 전송에 실패했다.
			return false;
		}
	}

	// return) 성공!!
	return true;
}

void net::io::Nsocket_secure::process_complete_sendable(uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	if(m_pcrypt.exist())
	{
		// statistics) 
		this->statistics_on_send_bytes(_transferred_bytes);
		this->statistics_on_send_message(_transferred_message);
	}
	else
	{
		this->net::io::sendable::tcp::Ndefault::process_complete_sendable(_result, _transferred_bytes, _transferred_message);
	}
}


#endif