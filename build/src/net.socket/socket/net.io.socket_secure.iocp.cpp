#include "pch.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


void net::io::Nsocket_secure::process_reset_receivable()
{
	// 1) SSL buffer reset
	this->m_buffer_decrypt_receiving.clear();
	this->m_buffer_decrypt_received.clear();

	// 2) clear crypt object
	this->m_pcrypt.reset();

	// 3) reset
	net::io::receivable::tcp::Ndefault::process_reset_receivable();
}

void net::io::Nsocket_secure::process_reset_sendable() noexcept
{
	this->net::io::sendable::tcp::Ndefault::process_reset_sendable();
}

void net::io::Nsocket_secure::process_on_receive (const buffer_view& _buffer, const net::sockaddr& _address)
{
	this->on_receive(_buffer, _address);
}

intptr_t net::io::Nsocket_secure::process_complete_receivable (intptr_t _result, std::size_t _transferred)
{
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

		// 1) decrypt
		if (m_pcrypt.exist())
		{
			std::size_t size_added = _transferred + 256;	// transfered + 256

			// - Decrypt 함수를 확인한다.
			if(this->m_buffer_decrypt_receiving.size() < size_added)
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
			auto result = this->m_pcrypt->decode(buffder_decrypt_receiving, reinterpret_cast<const_buffer_view*>(&m_buffer_received), 1);

			// - succeeded
			if(result == true)
			{
				// check) 
				CGASSERT_ERROR(buffder_decrypt_receiving.size() > 0)

				// - 길이를 더한다.
				this->m_buffer_decrypt_received.add_size(buffder_decrypt_receiving.size());

				// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
				this->process_on_receive(this->m_buffer_decrypt_received ^ this->m_buffer_decrypt_receiving, address_peer);

				// - Pakcet을 처리한다.
				required_bytes = this->process_packet(this->m_buffer_decrypt_received, address_peer);

				// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
				THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << this->m_maximum_message_buffer_size << "bytes, ip:"sv << to_string<char>(remote_endpoint()));

				// - 길이를 더해준다.
				this->m_buffer_decrypt_receiving += buffder_decrypt_receiving.size();
			}
		}
		else
		{
			// - 일단 on_receive한다.(일반적으로 여기서 하는 역할은 거의 없다.)
			this->process_on_receive(this->m_buffer_received ^ buffer_view{ this->m_wsabuf_receiving.buf, _transferred }, address_peer);

			// - message를 처리한다.
			required_bytes = this->process_packet(this->m_buffer_received, address_peer);

			// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
			THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:"sv << required_bytes << "bytes/max_buffer_size:"sv << this->m_maximum_message_buffer_size << "bytes, ip:"sv << to_string<char>(remote_endpoint()));
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
			std::size_t size_prepare = this->m_minimum_mesage_buffer_size; while (size_compare>size_prepare) size_prepare *= 8;

			// - [최소_TCP_수신_버퍼_크기]보다는 크게 한다.
			SATURATE_MAX(size_prepare, this->m_maximum_message_buffer_size);

			// declare)
			auto buffer_received_size = this->m_buffer_received.size();

			// - 최소 [요구_버퍼_크기]+[이미_수신한_데이터_크기]보다는 크야 한다.
			size_prepare = __select_max(size_prepare, required_bytes + buffer_received_size);

			// - [새_수신_버퍼]를 [준비할_버퍼량] 크기만큼 할당한다.
			//   exception) Memory 할당을 받지 못했을 경우라면... Error처리를 위해 Throw한다.
			//   * Size가 Pool이 할당할 수 있는 최대 크기(일반적으로 256Kbytes)이상일 경우 발생한다.
			//   * 메모리 부족으로 인해 더이상 할당받지 못했을 경우에 뜬다.
			auto buf_new = alloc_shared_buffer(size_prepare);

			// - [이미_수신한_데이터]를 [새_수신_버퍼]의 앞쪽에 복사한다.
			// - 새로 할당된 buf_shared에 처리되지 않고 남은 Byte를 복사해 준다.
			if(buffer_received_size != 0)
			{
				memcpy(buf_new.data(), this->m_buffer_received.data(), buffer_received_size);
			}

			// - [새_수신_버퍼]를 [수신_버퍼](m_wsabuf_receiving)에 설정한다.
			this->m_wsabuf_receiving = WSABUF{ static_cast<ULONG>(size_prepare - buffer_received_size), buf_new.data() + buffer_received_size };

			// - [이미_수신한_데이터_버퍼]에 버퍼를 설정한다.
			this->m_buffer_received = buf_new ^ buffer_received_size;
		}

		// check) 
		CGASSERT_ERROR(this->m_wsabuf_receiving.len != 0);
	}
	catch(std::exception& e)
	{
		//-----------------------------------------------------------------
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Definine함수이므로 이 함수 수행중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행중 다양한 Exception이 발생가능하다.
		// * alloc_shared_buffer 수행 중 Exception이 발생가능하다.
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
		return intptr_t(1);
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed " __CGFUNCTION__;

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
	{
		// - disconnect를 수행한다.(실질적으로 Overlapped로 DisconnectEX를 건다.)
		this->process_complete_disconnect();
	}

	// return) 
	return intptr_t(0);
}

// declare) 
class SSL_BUFFER : virtual public Ireferenceable
{
public:
	shared_buffer					buf_shared;
	object_ptr<Ireferenceable>		powner;
};

bool net::io::Nsocket_secure::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress)
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

	if(this->m_pcrypt.exist())
	{
		// 1) Encryption과정에서 크기가 커질 수 있으므로일단 2배로 + Header(32)로 할당한다.
		std::size_t bytes_buffer = _buffer.size() * 2 + 32;

		// declare) 
		auto ssl_buffer = make_object<SSL_BUFFER>();
		ssl_buffer->buf_shared = alloc_shared_buffer(bytes_buffer);
		ssl_buffer->powner = std::move(_powner);

		// declare)
		auto temp_dest = buffer_view(ssl_buffer->buf_shared.data(), bytes_buffer);

		__zero_memory(temp_dest.data(), temp_dest.size());

		// 3) encode
		this->m_pcrypt->encode(temp_dest, reinterpret_cast<const const_buffer_view*>(&_buffer), 1);

		// 4) set buffer size
		ssl_buffer->buf_shared.set_size(temp_dest.size());

		// 5) send
		this->net::io::sendable::tcp::Ndefault::process_sendable(ssl_buffer->buf_shared, ssl_buffer, _count_message, _paddress);
	}
	else
	{
		this->net::io::sendable::tcp::Ndefault::process_sendable(_buffer, std::move(_powner), _count_message, _paddress);
	}

	// return) 
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