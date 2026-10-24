#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

extern size_t	g_bytes_message_buffer_default_min_size;
extern size_t	g_bytes_message_buffer_default_max_size;

// - 복호화 버퍼의 여유 공간 (TLS record 최대 평문 크기(16KB) + 여유)
static constexpr std::size_t SSL_DECRYPT_MARGIN = 16384 + 2048;

// - 암호화된 송신 buffer와 원래 송신 buffer의 owner를 함께 보관한다.
class SSL_BUFFER : virtual public Ireferenceable
{
public:
	shared_buffer					buf_shared;
	object_ptr<Ireferenceable>		powner;
};


void net::io::Nsocket_ssl::process_reset_receivable()
{
	// 1) SSL관련 reset
	this->m_buffer_decrypt_receiving.clear();
	this->m_buffer_decrypt_received.clear();

	// 2) reset Crypt
	this->reset_ssl_cryptor();

	// 3) reset
	this->net::io::receivable::tcp::Ndefault::process_reset_receivable();
}

void net::io::Nsocket_ssl::process_reset_sendable() noexcept
{
	this->net::io::sendable::tcp::Ndefault::process_reset_sendable();
}

void net::io::Nsocket_ssl::process_on_receive ( const buffer_view& _buffer, const net::sockaddr& _address)
{
	this->on_receive(_buffer, _address);
}

intptr_t net::io::Nsocket_ssl::process_complete_receivable( intptr_t _result, std::size_t _transferred)
{
	// check) 
	CGASSERT(this->socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// - SSL을 쓰지 않으면 기본 수신 처리(receivable::tcp::Ndefault)를 그대로 쓴다.
	//   (평문 수신 코드를 여기에 따로 두지 않는다. 같은 코드를 두 벌 두면 한쪽만 고쳐지는 문제가 생긴다.)
	if (this->get_ssl_cryptor().empty())
		return net::io::receivable::tcp::Ndefault::process_complete_receivable(_result, _transferred);

	// declare)
	intptr_t result_return = EPOLLIN;

	try
	{
		// declare) pended receive bytes
		int bytes_receive_pended = 0;

		// 1) get socket handle
		auto handle_socket = this->native_handle();

		// 2) get pended receive bytes
		::ioctl(handle_socket, FIONREAD, &bytes_receive_pended);

		// check) return if there are no pended receive data.
		if(bytes_receive_pended == 0)
			return result_return;

		// declare) 
		shared_buffer buffer_receive;

		// 3) if bytes_receive_peneded size is less than min_size, set as min_size
		auto buffer_size = __select_max(static_cast<int>(g_bytes_message_buffer_default_min_size), bytes_receive_pended);

		// 4) alloc shared buffer
		buffer_receive = alloc_shared_buffer(m_buffer_received.size() + buffer_size);

		// 5) receive pended  data (offset for stored data)
		const int bytes_transfered = ::recv(handle_socket, buffer_receive.data() + m_buffer_received.size(), buffer_size, MSG_DONTWAIT);

		// check) 
		if(bytes_transfered == 0)
			return result_return;

		// check) 수신 오류 (기본 receivable과 같다.)
		if(bytes_transfered < 0)
		{
			if (api::get_last_error() == EAGAIN)
				return result_return;

			this->m_last_socket_error = api::get_last_error();
			return EPOLLHUP;
		}

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// 6) copy stored data to front of new buffer
		if (this->m_buffer_received.size() != 0)
			buffer_receive.append(this->m_buffer_received.size(), this->m_buffer_received.data());

		// 7) add size 
		buffer_receive.add_size(bytes_transfered);

		// declare)
		std::size_t required_bytes = MINIMUM_REMAINED_RECEIVE_BUFFER;
		net::sockaddr address_peer;

		// 8) get cryptor
		auto pssl_cryptor = this->get_ssl_cryptor();

		// 9) decrypt
		if (pssl_cryptor.exist())
		{
			// - 복호화 결과는 이번에 받은 데이터 + SSL에 남아 있던 미완성 record(최대 16KB)까지 나올 수 있다.
			std::size_t size_added = static_cast<std::size_t>(bytes_transfered) + SSL_DECRYPT_MARGIN;

			// - Decrypt 함수를 확인한다.
			if (this->m_buffer_decrypt_receiving.size() < size_added)
			{
				// - 
				auto buffer_size_new = 65536 + size_added + this->m_buffer_decrypt_received.size();

				// - 새로운 버퍼를 할당한다.
				auto buf_new = alloc_shared_buffer(buffer_size_new) ^ buffer_size_new;

				// - 기존에 받아 높은 데이터를 복사한다.
				if (this->m_buffer_decrypt_received.size() != 0)
					memcpy(buf_new.data(), this->m_buffer_decrypt_received.data(), this->m_buffer_decrypt_received.size());

				// - 설정한다.
				this->m_buffer_decrypt_receiving = buf_new + offset(this->m_buffer_decrypt_received.size());
				buf_new ^= this->m_buffer_decrypt_received.size();
				this->m_buffer_decrypt_received = std::move(buf_new);
			}

			// declare)
			buffer_view	buffer_decrypt_receiving = this->m_buffer_decrypt_receiving;

			// - Decrypt한다.
			//   (받은 데이터는 buffer_receive에 있다. decode는 받은 데이터를 모두 SSL(BIO)에 넣는다.)
			bool result = false;
			{
				std::lock_guard cs(this->m_lockable_ssl);

				shared_buffer buf_send;
				const_buffer_view buffer_receive_view{ buffer_receive.data(), buffer_receive.size() };
				result = pssl_cryptor->process_receive(buffer_decrypt_receiving, buffer_receive_view, buf_send);

				// - handshake 응답 등 보낼 데이터는 순서가 바뀌지 않도록 lock 안에서 전송한다.
				if (buf_send.size() != 0)
					this->_ssl_send_raw(std::move(buf_send), 0);
			}

			// - 남은 암호화 데이터는 없다. (모두 SSL(BIO)에 들어갔다.)
			this->m_buffer_received.clear();

			// check) result가 false면 
			if (result == true)
			{
				// check) 
				CGASSERT_ERROR(buffer_decrypt_receiving.size() > 0)

				// - 길이를 더한다.
				this->m_buffer_decrypt_received.add_size(buffer_decrypt_receiving.size());

				// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
				this->process_on_receive(m_buffer_decrypt_received ^ m_buffer_decrypt_receiving, address_peer);

				// - Packet을 처리한다.
				required_bytes = this->process_packet(m_buffer_decrypt_received, address_peer);

				// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
				if(required_bytes > this->m_maximum_message_buffer_size)
					throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, 0, std::string("(excp) CGDK.net.socket: message size too BIG. required size:") + std::to_string(required_bytes) + "bytes/max_buffer_size:" + std::to_string(this->m_maximum_message_buffer_size) + "bytes, ip:" + remote_endpoint().to_string());

				// - 길이를 더해준다.
				this->m_buffer_decrypt_receiving += offset(buffer_decrypt_receiving.size());
			}
		}
		else
		{
			// 10) call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
			this->process_on_receive(buffer_receive ^ bytes_transfered, address_peer);

			// 11) process message
			//    - 여기서 Return해 오는 값은 다음 message를 받기 위해 다음에 필요로 하는 메모리 용량이다.
			//    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
			//      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
			//    - Exception이 발생가능하다.
			required_bytes = this->process_packet(buffer_receive, address_peer);

			// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
			if(required_bytes > this->m_maximum_message_buffer_size)
				throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, 0, std::string("(excp) CGDK.net.socket: message size too BIG. required size:") + std::to_string(required_bytes) + "bytes/max_buffer_size:" + std::to_string(this->m_maximum_message_buffer_size) + "bytes, ip:" + remote_endpoint().to_string());

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
	catch (throwable::failure& _e)
	{
		// trace) (ssl handshake/인증서 검증 실패 등의 이유를 남긴다.)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (throwable) " << __CGFUNCTION__;
		if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// return) 접속 종료 처리는 EPOLLHUP으로 event 처리에서 한다.
		return EPOLLHUP;
	}
	catch (std::exception & e)
	{
		//-----------------------------------------------------------------
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 Exception이 발생가능하다.
		//-----------------------------------------------------------------
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// return) 접속 종료 처리(process_complete_disconnect)는 EPOLLHUP으로 event 처리에서 한다. (기본 receivable과 같다.)
		//         (여기서 직접 호출하면 HUP event 처리와 겹쳐 종료 처리가 두 번 수행된다.)
		return EPOLLHUP;
	}
	catch (...)
	{
		//-----------------------------------------------------------------
		// exception) 위의 Exception과 동일한 설명
		//-----------------------------------------------------------------
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure	e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// return) 접속 종료 처리(process_complete_disconnect)는 EPOLLHUP으로 event 처리에서 한다. (기본 receivable과 같다.)
		//         (여기서 직접 호출하면 HUP event 처리와 겹쳐 종료 처리가 두 번 수행된다.)
		return EPOLLHUP;
	}

	// return)
	return result_return;
}

bool net::io::Nsocket_ssl::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT_FATAL(_buffer.size() != 0);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// check) if socket is not connected, return false 
	if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// 설명) 암호화(SSL_write)와 송신 queue에 넣는 것을 하나의 lock 안에서 처리한다.
	//       - SSL 객체는 thread safe하지 않다. (수신 thread의 decode와도 같은 lock을 사용한다.)
	//       - 일부만 전송되거나 EWOULDBLOCK인 경우의 처리는 기본 sendable(queue + EPOLLOUT)이 한다.
	//         (암호화된 데이터를 버리면 TLS stream이 깨진다.)
	std::unique_lock cs(this->m_lockable_ssl);

	// 1) get cryptor
	auto pssl_cryptor = this->m_pssl_cryptor;

	// check) ssl을 사용하지 않으면 그대로 전송한다.
	if(pssl_cryptor.empty())
	{
		cs.unlock();
		return this->net::io::sendable::tcp::Ndefault::process_sendable(_buffer, std::move(_powner), _count_message, _paddress);
	}

	// 2) encode (암호화한 결과를 크기에 맞는 buffer로 받는다. 실패하면 exception)
	//   - handshake 중이면 평문은 보관되고 handshake 데이터만(또는 없음) 돌아온다.
	auto buf_encrypted = pssl_cryptor->encode_buffer(_buffer);

	// 3) send (송신 queue에 넣는 것까지 lock 안에서 처리한다.)
	return this->_ssl_send_raw(std::move(buf_encrypted), _count_message);
}

bool net::io::Nsocket_ssl::_ssl_send_raw(shared_buffer&& _buffer, std::size_t _count_message)
{
	// check) 보낼 것이 없으면 성공 (handshake 중 보관만 된 경우)
	if (_buffer.size() == 0)
		return true;

	// 1) buffer를 보관할 객체
	auto ssl_buffer = make_object<SSL_BUFFER>();
	ssl_buffer->buf_shared = std::move(_buffer);

	// declare)
	buffer_view buffer_send = ssl_buffer->buf_shared;

	// 2) send
	return this->net::io::sendable::tcp::Ndefault::process_sendable(buffer_send, std::move(ssl_buffer), _count_message, net::sockaddr());
}

bool net::io::Nsocket_ssl::ssl_start_handshake()
{
	// lock)
	std::lock_guard cs(this->m_lockable_ssl);

	// 1) get cryptor
	auto pcryptor = this->m_pssl_cryptor;

	// check) ssl을 사용하지 않거나 이미 handshake가 끝났으면 할 것이 없다.
	if (pcryptor.empty() || pcryptor->is_handshake_completed())
		return true;

	// 2) handshake를 시작하고 ClientHello를 보낸다. (이미 보냈으면 보낼 것이 없다.)
	return this->_ssl_send_raw(pcryptor->start_handshake(), 0);
}

void net::io::Nsocket_ssl::start_ssl_handshake_timer(chrono::tick::duration _timeout)
{
	// check)
	if (_timeout <= chrono::tick::duration::zero())
		return;

	// 1) 현재 SSL 객체
	//    - socket 객체는 다른 접속에 재사용될 수 있으므로 SSL 객체가 같은지로 같은 접속인지 확인한다.
	auto pssl = this->get_ssl_cryptor();

	// check)
	if (pssl.empty())
		return;

	// 2) 시간이 지나면 handshake가 끝났는지 확인하고 끝나지 않았으면 접속을 끊는다.
	auto pexecutable = make_executable([psocket = object_ptr<net::io::Nsocket_ssl>(this), pssl = std::move(pssl)]()
	{
		{
			std::lock_guard cs(psocket->m_lockable_ssl);

			// check) 다른 접속이거나 handshake가 끝났으면 할 것이 없다.
			if (psocket->m_pssl_cryptor.get() != pssl.get() || pssl->is_handshake_completed())
				return;
		}

		// log)
		LOG_DEBUG << "(info) CGDK.net.socket: ssl handshake timeout. socket closed"sv;

		// - 접속을 끊는다.
		psocket->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
	});

	// 3) post
	system_executor::post(std::move(pexecutable), chrono::tick::now() + _timeout);
}

void net::io::Nsocket_ssl::process_complete_sendable(uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// 설명) ssl을 사용해도 송신 완료 처리(송신 queue 정리, 대기 중인 데이터 전송)는 같다.
	this->net::io::sendable::tcp::Ndefault::process_complete_sendable(_result, _transferred_bytes, _transferred_message);
}


#endif
