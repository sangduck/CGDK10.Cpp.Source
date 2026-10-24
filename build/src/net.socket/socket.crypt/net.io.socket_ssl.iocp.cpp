#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;

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
	// 1) SSL buffer reset
	this->m_buffer_decrypt_receiving.clear();
	this->m_buffer_decrypt_received.clear();

	// 2) clear crypt object
	this->reset_ssl_cryptor();

	// 3) reset
	net::io::receivable::tcp::Ndefault::process_reset_receivable();
}

void net::io::Nsocket_ssl::process_reset_sendable() noexcept
{
	this->net::io::sendable::tcp::Ndefault::process_reset_sendable();
}

void net::io::Nsocket_ssl::process_on_receive (const buffer_view& _buffer, const net::sockaddr& _address)
{
	this->on_receive(_buffer, _address);
}

intptr_t net::io::Nsocket_ssl::process_complete_receivable (intptr_t _result, std::size_t _transferred)
{
	// check) 
	CGASSERT(this->socket_state() != eSOCKET_STATE::SYN, intptr_t(1));

	// - SSL을 쓰지 않으면 기본 수신 처리(receivable::tcp::Ndefault)를 그대로 쓴다.
	//   (평문 수신 코드를 여기에 따로 두지 않는다. 같은 코드를 두 벌 두면 한쪽만 고쳐지는 문제가 생긴다.)
	if (this->get_ssl_cryptor().empty())
		return net::io::receivable::tcp::Ndefault::process_complete_receivable(_result, _transferred);

	// check) 만약 전송된 Byte수가 0Byte면 접속을 종료처리한다.
	if(_transferred == 0)
	{
		// - 비정상 접속종료일 경우 REASON_FAIL을 설정한다.
		if(_result != 0)
			this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();

		// return) 
		return intptr_t(1);
	}

	// statistics) socket의 receive통계.
	this->statistics_on_receive_bytes(_transferred);

	// 1) 받은 Size를 더한다.
	this->m_buffer_received.add_size(_transferred);

	try
	{
		// declare) 
		std::size_t required_bytes = MINIMUM_REMAINED_RECEIVE_BUFFER;
		net::sockaddr address_peer;

		// 1) get cryptor
		auto pssl_cryptor = this->get_ssl_cryptor();

		// 2) decrypt
		if (pssl_cryptor.exist())
		{
			// - 복호화 결과는 이번에 받은 데이터 + 이전에 받아 SSL에 남아 있던 미완성 record(최대 16KB)까지 나올 수 있다.
			std::size_t size_added = _transferred + SSL_DECRYPT_MARGIN;

			// - Decrypt 함수를 확인한다.
			if(this->m_buffer_decrypt_receiving.size() < size_added)
			{
				// - 
				auto buffer_size_new = 65536 + size_added + this->m_buffer_decrypt_received.size();

				// - 새로운 버퍼를 할당한다.
				auto buf_new = alloc_shared_buffer(buffer_size_new) ^ buffer_size_new;

				// - 기존에 받아 놓은 데이터를 복사한다.
				if (this->m_buffer_decrypt_received.size() != 0)
					memcpy(buf_new.data(), this->m_buffer_decrypt_received.data(), this->m_buffer_decrypt_received.size());

				// - 설정한다.
				this->m_buffer_decrypt_receiving = buf_new + offset(this->m_buffer_decrypt_received.size());
				buf_new ^= this->m_buffer_decrypt_received.size();
				this->m_buffer_decrypt_received = std::move(buf_new);
			}

			// declare)
			buffer_view	buffer_decrypt_receiving = this->m_buffer_decrypt_receiving;

			// - handshake/Decrypt한다. (SSL 객체는 송신 thread와 공유하므로 lock을 잡는다.)
			//   (handshake 응답 등 보낼 데이터는 순서가 바뀌지 않도록 lock 안에서 전송한다.)
			bool result = false;
			{
				std::lock_guard cs(this->m_lockable_ssl);

				shared_buffer buf_send;
				result = pssl_cryptor->process_receive(buffer_decrypt_receiving, *reinterpret_cast<const_buffer_view*>(&m_buffer_received), buf_send);

				if (buf_send.size() != 0)
					this->_ssl_send_raw(std::move(buf_send), 0);
			}

			// - succeeded
			if(result == true)
			{
				// check) 
				CGASSERT_ERROR(buffer_decrypt_receiving.size() > 0)

				// - 길이를 더한다.
				this->m_buffer_decrypt_received.add_size(buffer_decrypt_receiving.size());

				// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
				this->process_on_receive(this->m_buffer_decrypt_received ^ this->m_buffer_decrypt_receiving, address_peer);

				// - Packet을 처리한다.
				required_bytes = this->process_packet(this->m_buffer_decrypt_received, address_peer);

				// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
				THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size too BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << this->m_maximum_message_buffer_size << "bytes, ip:"sv << to_string(remote_endpoint()));

				// - 길이를 더해준다.
				this->m_buffer_decrypt_receiving += buffer_decrypt_receiving.size();
			}
		}
		else
		{
			// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
			this->process_on_receive(this->m_buffer_received ^ buffer_view{ this->m_wsabuf_receiving.buf, _transferred }, address_peer);

			// - message를 처리한다.
			required_bytes = this->process_packet(this->m_buffer_received, address_peer);

			// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
			THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size too BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << this->m_maximum_message_buffer_size << "bytes, ip:"sv << to_string(remote_endpoint()));
		}


		//-----------------------------------------------------------------
		// 3. buf_shared 체크한다.
		//    1) required_bytes보다 남아 있는 receive 버퍼의 크기가 작다면
		//       버퍼를 새로 할당한다.
		//    2) 기존의 버퍼를 버리고 새로운 버퍼를 할당받는다.
		//    3) 또 기존 receive 버퍼에 처리되지 않은 데이터가 있다면
		//       새로 할당받은 버퍼에 복사를 해 넣는다.
		//-----------------------------------------------------------------
		// 1) Receiving buf_shared를 계산함.
		this->m_wsabuf_receiving.buf += static_cast<ULONG>(_transferred);
		this->m_wsabuf_receiving.len -= static_cast<ULONG>(_transferred);

		// 2) 받고자하는 message의 크기보다 message의 미수신 크기가 많으므로...(메모리를 새로 할당받는다.)
		if(this->m_wsabuf_receiving.len < MINIMUM_REMAINED_RECEIVE_BUFFER || (this->m_buffer_received.size() + this->m_wsabuf_receiving.len) < required_bytes)
		{
			// - [필요_버퍼크기]를 구한다.(receive받은 크기 + 이미 받아 놓은 데이터 크기)
			const std::size_t size_compare = api::socket::get_pending_read_buffer(this->native_handle()) + this->m_buffer_received.size();

			// - [준비할_버퍼_크기]의 [필요_버퍼크기]의 최소 8의 승수로  만든다.(8,16,128, 1024, 8192, 64k, 256k, ...)
			//   (이렇게 8의 배수로 하는 이유는... 너무 단계를 많이 하게 될 경우 오히리 사용하는 메모리 버퍼 풀의 종류가 많아져
			//   각 풀에 재고가 너무 많이 남아 있어 메모리 사용량을 늘릴 수 있으므로 단계를 줄이기 위해서 8의 승수로
			//   단계를 줄여 사용하는 풀의 종류를 줄인다.)
			std::size_t size_prepare = this->m_minimum_message_buffer_size; while (size_compare>size_prepare) size_prepare *= 8;

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
			// - 새로 할당된 buf_shared에 처리되지 않고 남은 Byte를 복사해 준다.
			if(buffer_received_size != 0)
				memcpy(buf_new.data(), this->m_buffer_received.data(), buffer_received_size);

			// - [새_수신_버퍼]를 [수신_버퍼](m_wsabuf_receiving)에 설정한다.
			this->m_wsabuf_receiving = WSABUF{ static_cast<ULONG>(size_prepare - buffer_received_size), buf_new.data() + buffer_received_size };

			// - [이미_수신한_데이터_버퍼]에 버퍼를 설정한다.
			buf_new ^= buffer_received_size;
			this->m_buffer_received = std::move(buf_new);
		}

		// check) 
		CGASSERT_ERROR(this->m_wsabuf_receiving.len != 0);
	}
	catch(std::exception& e)
	{
		//-----------------------------------------------------------------
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 다양한 Exception이 발생가능하다.
		// * alloc_shared_buffer 수행 중 Exception이 발생가능하다.
		//-----------------------------------------------------------------
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - disconnect를 수행한다.
		this->process_complete_disconnect();

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

		// return) 
		return intptr_t(1);
	}


	//-----------------------------------------------------------------
	// 3. Overlapped receive를 걸어준다.
	//    - receive에 실패했을 경우 접속을 바로 종료처리한다.
	//-----------------------------------------------------------------
	// 1) 새로 receive를 건다.
	if(this->receive()==false)
		this->process_complete_disconnect(); // - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)

	// return) 
	return intptr_t(0);
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
	//       - 암호화한 순서와 송신 queue에 들어가는 순서가 바뀌면 TLS record 순서가 깨진다.
	std::unique_lock cs(this->m_lockable_ssl);

	// 1) get cryptor
	auto pcryptor = this->m_pssl_cryptor;

	// check) ssl을 사용하지 않으면 그대로 전송한다.
	if(pcryptor.empty())
	{
		cs.unlock();
		return this->net::io::sendable::tcp::Ndefault::process_sendable(_buffer, std::move(_powner), _count_message, _paddress);
	}

	// 2) encode (암호화한 결과를 크기에 맞는 buffer로 받는다. 실패하면 exception)
	//    - handshake 중이면 평문은 보관되고 handshake 데이터만(또는 없음) 돌아온다.
	auto buf_encrypted = pcryptor->encode_buffer(_buffer);

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
	//       (호출하지 않으면 첫 송신 이후의 데이터가 전송되지 않는다.)
	this->net::io::sendable::tcp::Ndefault::process_complete_sendable(_result, _transferred_bytes, _transferred_message);
}

#endif
