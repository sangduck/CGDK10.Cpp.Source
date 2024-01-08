#include "pch.h"

// SSL Include files
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/buffer.h> 
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <cmath>

using namespace CGDK;

void net::io::Nhttp_server::process_on_connect()
{
}

void net::io::Nhttp_server::process_on_fail_connect(uint64_t _disconnect_reason)
{
	on_fail_connect(_disconnect_reason);
}
void net::io::Nhttp_server::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
	{
		on_disconnect(_disconnect_reason);
	}
}

shared_buffer message_web_error(uint32_t _error_code, const std::string_view _error_message)
{
	// 1) shared_buffer를 할당받는다.
	auto temp_buffer = alloc_shared_buffer(4096);

	// 2) Response head를 작성한다.
	temp_buffer.append_text("HTTP/1.1 {} {}\r\n"sv, _error_message, _error_code);	// Protocol 버전 및 응답코드
	temp_buffer.append_text("Server: CGCII/10.0.0\r\n"sv);		// Web Server Bannder 정보
	temp_buffer.append_text("Content-type:text/html\r\n"sv);	// MIME 타입
	temp_buffer.append_text("Content-length:107\r\n"sv);		// HTTP Body 크기

	// 3) Response Body (HTML) 만들기
	temp_buffer.append_text("<html>"sv);						// HTML 시작
	temp_buffer.append_text("<head></head>"sv);					// head 정보
	temp_buffer.append_text("<title>Http Protocol</title>"sv);	// Title
	temp_buffer.append_text("<body>"sv);						// Body 시작
	temp_buffer.append_text("</body>"sv);						// Body 끝
	temp_buffer.append_text("</html>\r\n"sv);					// HTML 끝

	// return) 
	return	temp_buffer;
}

shared_buffer message_web_socket_handshake(const std::string_view _websocket_protocol, const std::string_view _websocket_accept, const std::string_view _websocket_location = "ws://127.0.0.1")
{
	// 1) shared_buffer를 할당받는다.
	auto temp_buffer = alloc_shared_buffer(4096);

	// 2) 메시지를 작성한다.
	temp_buffer.append_text("HTTP/1.1 101 Switching Protocols\r\n"sv);
	temp_buffer.append_text("Upgrade: websocket\r\n"sv);
	temp_buffer.append_text("Connection: Upgrade\r\n"sv);
	temp_buffer.append_text("Sec-WebSocket-Location: {}\r\n"sv, _websocket_location);
	if (_websocket_protocol.empty() == false)
	{
		temp_buffer.append_text("Sec-WebSocket-Protocol: {}\r\n"sv, _websocket_protocol);
	}
	temp_buffer.append_text("Sec-WebSocket-Accept: {}\r\n"sv, _websocket_accept);
	temp_buffer.append_text("Server: CGCII/10.0.0\r\n"sv);
	temp_buffer.append_text("\r\n"sv);

	// return)
	return	temp_buffer;
}

char get_next_char(char*& _pnow, const char* _pend)
{
	// check) End면 끝낸다.
	RETURN_IF(_pnow == _pend, 0);

	if (*_pnow == '%')
	{
		// check) 길이가 2Byte 이하면 Exception을 던진다.
		THROW_IF((_pend-_pnow) < 3, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		char* pNow = _pnow+1;

		// - %로 시작할 경우 Binary값이므로 변환한다.
		char vaule1 = (char)*pNow++;
		char vaule2 = (char)*pNow++;

		// - 그 다음.
		_pnow = pNow;

		vaule1 = (vaule1 >= '0' && vaule1 <= '9') ? (vaule1 - '0') : (vaule1 >= 'A' && vaule1 <= 'F') ? (vaule1 - 'A' + 10) : (vaule1 >= 'a' && vaule1 <= 'z') ? (vaule1 - 'a' + 10) : 0;
		vaule2 = (vaule2 >= '0' && vaule2 <= '9') ? (vaule2 - '0') : (vaule2 >= 'A' && vaule2 <= 'F') ? (vaule2 - 'A' + 10) : (vaule2 >= 'a' && vaule2 <= 'z') ? (vaule2 - 'a' + 10) : 0;

		return	(vaule1 << 4) | vaule2;
	}
	else
	{
		return	*_pnow++;
	}
}

char get_next_code_utf8(char*& _pnow, const char* _pend)
{
	// 1) Character 값을 읽어온다.
	char ch_read = (char)get_next_char(_pnow, _pend);

	// check)
	 RETURN_IF(ch_read == 0, 0);

	// Case-1) 0x80보다 작으면 그냥 그 값을 Return한다.(일반적인 값임)
	if ((ch_read & 0x80) == 0)
	{
		return ch_read;
	}
	// Case-2) 110xxxxx 10xxxxxx
	else if ((ch_read & 0xe0) == 0xc0)
	{
		// - Character 읽기
		char wch_next = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		return	((ch_read & 0x1f) << 6) | (wch_next & 0x3f);
	}
	// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
	else if ((ch_read & 0xf0) == 0xe0)
	{
		// - 1번째 Character
		char wch_next1 = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next1 == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		// - 2번째 Character
		char wch_next2 = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next2 == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		return ((ch_read & 0x0f)<<12) | ((wch_next1 & 0x3f)<<6) | (wch_next2 & 0x3f);
	}
	// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((ch_read & 0xf8) == 0xf0)
	{
		//CGASSERT_ERROR(false);

		//// check) NULL이 돌아오면 Exception을 던진다.
		//throw	throwable::failure(400, "Invalid Request"sv);

		// - 1번째 Character
		char wch_next1 = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next1 == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		// - 2번째 Character
		char wch_next2 = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next2 == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		// - 3번째 Character
		char wch_next3 = (char)get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		THROW_IF(wch_next3 == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv));

		return ((ch_read & 0x07) << 18) | ((ch_read & 0x3f) << 12) | ((wch_next1 & 0x3f) << 6) | (wch_next2 & 0x3f);
	}

	// return) Error...
	return	0;
}

size_t net::io::Nhttp_server::process_packet_websocket( shared_buffer& _buffer, const net::sockaddr& /*_address*/)
{
	// declare)
	buffer_view	buf_recv  = _buffer;

	try
	{
		for(;;)
		{
			// check) uint64_t보다 길이가 작으면 여기서 끝낸다.
			BREAK_IF(buf_recv.size() < sizeof(uint16_t));

			// 1) 
			buffer_view	buf_apply = buf_recv;

			// 2) Head를 얻는다.
			uint16_t header = buf_apply.extract<uint16_t>();

			// 3) 첫번째 메시지인가 아닌가.
			bool  is_fin = (header & 0x0080)!=0;
			uint16_t Opcode = (header & 0x000f); // opcode 0:continue, 1:text, 2:binary 8:close

			// 4) message length
			bool bMasked = (header & 0x8000) != 0;
			uint16_t length = (header & 0x7f00)>>8;

			// 5) length가 126일 경우 길이용 바이트로 2Byte 추가.
			if (length == 126)
			{
				// - 길이를 0으로 리셋
				length = 0;

				// - 2Byte 읽어 뒤집어 사용한다.
				for(int i=0;i<2; ++i)
				{
					length += static_cast<size_t>(buf_apply.extract<uint8_t>()) << (8 * (2 - 1 - i));
				}
			}
			// 6) length가 127일 경우 길이용 바이트로 8Byte 추가.
			else if(length == 127)
			{
				// - 길이를 0으로 리셋
				length = 0;

				// - 8Byte 추가
				for (int i = 0; i<8; ++i)
				{
					length += static_cast<size_t>(buf_apply.extract<uint8_t>()) << (8 * (8 - 1 - i));
				}
			}

			// 7) Masked일 경우 Masked 처리를 한다.
			if(bMasked)
			{
				// check) length보다 길이가 작으면 여기서 끝낸다.
				//        주의!!) 이 BREAK문은 반드시 여기서 BREAK를 해야한다. 이유는 중복 Masking 처리를 방지하기 위해서이다.
				BREAK_IF(buf_apply.size() - sizeof(uint32_t) < length);

				// declare) 
				uint8_t	mask[4];
		
				// - Mask를 읽어낸다
				*(uint32_t*)mask = buf_apply.extract<uint32_t>();

				// - Read/ReadEnd...
				auto read_pos_now = buf_apply.data<char>();
				auto read_pos_end = buf_apply.data<char>() + length;

				// - Mask값을 XOR처리한다.
				for (int c = 0; read_pos_now != read_pos_end; ++read_pos_now, ++c)
				{
					*read_pos_now = *read_pos_now ^ mask[c % 4];
				}
			}
			else
			{
				// check) length보다 길이가 작으면 여기서 끝낸다.
				BREAK_IF(buf_apply.size() < length);
			}
		
			// 8) shared_buffer를 설정한다.
			auto buf_payload_data = buffer_view(buf_apply.data(), length);

			// 9) 길이를 진행
			buf_apply += offset(length);
			buf_recv = buf_apply;

			// 10) Opcode 처리하기.
			//	*  0x0			Denotes a continuation frame
			//	*  0x1			Denotes a text frame
			//	*  0x2			Denotes a binary frame
			//	*  0x3 ~ 0x7	Reserved for further non - control frames
			//	*  0x8			Denotes a connection close
			//	*  0x9			Denotes a ping
			//	*  0xA			Denotes a pong
			//	*  0xB ~ 0xF	Reserved for further control frames
			switch(Opcode)
			{
			// CASE) continue
			case	0:
					{

					}
					break;

			// CASE) Text
			case	1:
					{
						// - 전달용 message를 설정한다.
						sMESSAGE_HTTP_WEBSOCKET msg;

						msg.method = eHTTP_METHOD::TEXT_;
						msg.buf_message = _buffer ^ buf_payload_data;

						// - message를 Imessageable로 전달하여 처리한다.
						process_message(msg);

						// - Paket수를 증가시킨다.
						statistics_on_receive_message();
					}
					break;

			// CASE) Binary
			case	2:
					{
						// declare) 
						sMESSAGE_HTTP_WEBSOCKET msg;

						// - 전달용 message를 설정한다.
						msg.method = eHTTP_METHOD::BINARY_;
						msg.buf_message = _buffer ^ buf_payload_data;

						// - message를 Imessageable로 전달하여 처리한다.
						process_message(msg);

						// - Paket수를 증가시킨다.
						statistics_on_receive_message();
					}
					break;

			// CASE) close
			case	8:
					{
						disconnect();
					}
					break;

			// CASE) PING 요청시
			case	9:
					{
						// - Pong을 전송한다.
						//   Pong은 Opcode가 0x0a로 변경해서 전달받은 Ping과 완전히 동일한 데이터를 Echo 전송한다.
						_buffer.front<uint8_t>() = (_buffer.front<uint8_t>() & 0xf0) | 0x0a;

						// - 전송한다.
						process_send(shared_buffer(_buffer), 1, 0);
					}
					break;

			// CASE) PONG
			case	10:
					{
						header = 1;
					}
					break;

			// Default) ...
			default:
					{
						header = 2;
					}
					break;
			};

			// 11) 메시지 끝일 경우...
			if (is_fin == true)
			{
				// - 접속 종료시
				//int status = 0;
				//if(length >= 2)
				//{
				//	unsigned char byte1 = message->get();
				//	unsigned char byte2 = message->get();
				//	status = (byte1 << 8) + byte2;
				//}

				//auto reason = message->string();
				//connection->send_close(status, reason);
				//connection_close(connection, endpoint, status, reason);
				//disconnect();
			}
		}

		// 12) 처리된 메시지 버퍼를 업데이트한다.
		_buffer  = buf_recv;
	}
	catch(...)
	{
		// - 처리된 메시지 버퍼를 업데이트한다.
		_buffer  = buf_recv;

		// reraise)
		throw;
	}

	// return)
	return 0;
}

std::string sha1(const std::string_view _input, size_t iterations = 1) noexcept
{
	std::string hash;

	hash.resize(160 / 8);
	SHA1(reinterpret_cast<const unsigned char *>(_input.data()), _input.size(), reinterpret_cast<unsigned char*>(&hash[0]));

	for (size_t c = 1; c < iterations; ++c)
		SHA1(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char*>(&hash[0]));

	return hash;
}

std::string base64_encode(const std::string_view _ascii) noexcept
{
	std::string base64;

	BUF_MEM*	bptr = BUF_MEM_new();
	BIO*		b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO*		bio = BIO_new(BIO_s_mem());
	BIO_push(b64, bio);
	BIO_set_mem_buf(b64, bptr, BIO_CLOSE);

	// write directly to base64-shared_buffer to avoid copy
	auto base64_length = static_cast<size_t>(round(4 * ceil(static_cast<double>(_ascii.size()) / 3.0)));
	base64.resize(base64_length);
	bptr->length = 0;
	bptr->max = base64_length + 1;
	bptr->data = &base64[0];

	if (BIO_write(b64, &_ascii[0], static_cast<int>(_ascii.size())) <= 0 || BIO_flush(b64) <= 0)
	{
		base64.clear();
	}

	// To keep &base64[0] through BIO_free_all(b64)
	bptr->length = 0;
	bptr->max = 0;
	bptr->data = nullptr;

	BIO_free_all(b64);

	// return) 
	return base64;
}

bool base64_encode(char* _dest, size_t _dest_size, const std::string_view _source) noexcept
{
	BUF_MEM*	bptr = BUF_MEM_new();
	BIO*		b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO*		bio = BIO_new(BIO_s_mem());
	BIO_push(b64, bio);
	BIO_set_mem_buf(b64, bptr, BIO_CLOSE);

	// write directly to base64-shared_buffer to avoid copy
	auto base64_length = static_cast<size_t>(round(4 * ceil(static_cast<double>(_source.size()) / 3.0)));

	// check)
	RETURN_IF(base64_length >= _dest_size, false);

	bptr->length = 0;
	bptr->max = base64_length + 1;
	bptr->data = _dest;

	if (BIO_write(b64, _source.data(), static_cast<int>(_source.size())) <= 0 || BIO_flush(b64) <= 0)
	{
		return	false;
	}

	// - NULL을 추가한다.
	_dest[base64_length] = 0;

	// To keep &base64[0] through BIO_free_all(b64)
	bptr->length = 0;
	bptr->max = 0;
	bptr->data = nullptr;

	BIO_free_all(b64);

	// return) 
	return true;
}

void encode_web_socket_key(char* _dest, const std::string_view _key)
{
	// check)
	CGASSERT_ERROR(_key.size() == 24);

	// declare) 
	char str_source[256];

	// 1) convert string
	memcpy(str_source, _key.data(), _key.size());

	// 2) string 붙이기
	memcpy(str_source +24, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 37);

	// 3) SHA1로 Encoding
	char str_sha1[256];
	SHA1(reinterpret_cast<const unsigned char*>(str_source), 60, reinterpret_cast<unsigned char*>(str_sha1));

	// 4) Base64로 다시 Encoding
	auto result = base64_encode(_dest, 256, std::string_view(str_sha1, 20));

	// check) ...
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
}

bool check_web_socket_key(const std::string_view _compare, const std::string_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_source.size() == 24);

	// check)
	CGASSERT_ERROR(_compare.size() == 28);

	// declare) 
	char str_source[256];

	// 1) convert string
	memcpy(str_source, _source.data(), _source.size());

	// 2) string 붙이기
	memcpy(str_source + 24, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 37);

	// 3) SHA1로 Encoding
	char str_sha1[256];
	SHA1(reinterpret_cast<const unsigned char*>(str_source), 60, reinterpret_cast<unsigned char*>(str_sha1));

	// decalre) 
	char str_result[256];

	// 4) Base64로 다시 Encoding
	auto result = base64_encode(str_result, 256, std::string_view(str_sha1, 20));

	// check) ...
	RETURN_IF(result == false, false);

	// 5) compare keyward
	return _compare == std::string_view(str_result, 28);
}

size_t net::io::Nhttp_server::process_packet_http( shared_buffer& _buffer, const net::sockaddr& /*_address*/)
{
	// 1) message parsing
	auto size_process = process_parsing_http(_buffer);

	// 2) 처리된 buffer에서 뺀다.
	_buffer += offset(size_process);

	// 3) completed되면 여기서 끝낸다.
	if (m_msg_http_request.is_completed)
	{
		// - WebSocket이면...
		if (is_certified() == false)
		{
			if (m_msg_http_request.get_header("Upgrade"sv) == "websocket"sv)
			{
				// - WebSocket-Key를 얻는다.
				auto str_web_socket_key = m_msg_http_request.get_header("Sec-WebSocket-Key"sv);

				// check)
				CGASSERT_ERROR(str_web_socket_key.size() == 24);

				// check) WebSocketKey가 없으면 Exception을 던진다.
				THROW_IF(str_web_socket_key.size() != 24, throwable::failure(eRESULT::FAIL_NO_WEBSOCKET_KEY))

				// declare)
				uint32_t version = HTTP_VERSION(1,1);

				// - Version을 얻는다.
				auto str_web_socket_version = m_msg_http_request.get_header("Sec-WebSocket-Version"sv);

				// check) Version 정보를 얻느다.
				if (str_web_socket_version.empty() == false)
				{
					version = __from_string<int>(str_web_socket_version);
				}


				// declare)
				char web_socket_accept_y[64];

				// "2iO4T28+b5mJns7lddxJFA==258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
				encode_web_socket_key(web_socket_accept_y, "2iO4T28+b5mJns7lddxJFA=="sv);
				// "mBGsZJil65RYPs2rsG/9RE+x1o4="

				// declare)
				char web_socket_accept[64];

				// - WebSocket-Accept를 얻는다.
				encode_web_socket_key(web_socket_accept, str_web_socket_key);

				// - WebRequest 정보를 설정한다.
				m_msg_http_request.protocol = eHTTP_PROTOCOL::WEBSOCKET_;
				m_msg_http_request.version  = version;

				// - Acceptor를 얻는다.
				auto pacceptor = dynamic_cast<net::io::connective::acceptor::Ntcp*>(this->m_pconnectable_notify);

				// check)
				CGASSERT_ERROR(pacceptor != nullptr);

				{
					// declare)
					std::string_view str_address;
					char temp_string[256]{ 0 };

					// - accept address를 얻는다.
					if (pacceptor != nullptr)
					{
						str_address = CGDK::to_string(temp_string, 256, pacceptor->get_accept_endpoint().address());
					}
					else
					{
						str_address = "ws://localhost"sv;
					}

					// 주의) message_web_socket_handshake 메시지는 반드시 on_connect 호출 전에 클라이언트에거 전송되어야 한다.
					//       그렇지 않으면 on_connect 중 전송되는 send에 의해 websocket 인증이 완료 되지 않았는데 메시지를 전송할 수 있다.

					// declare) 
					std::string_view str_protocol;

					// - Handshaking 메시지를 전송
					process_send(message_web_socket_handshake(str_protocol, std::string_view(web_socket_accept, 28), str_address), 1, 0);
				}
							   
				try
				{
					// - set object_state
					socket_state(eSOCKET_STATE::CERTIFIED);

					// - call 'on_connect'
					on_connect();
				}
				catch (...)
				{
					// - restore socket status
					socket_state(eSOCKET_STATE::ESTABLISHED);

					// reraise)
					throw;
				}

				// return) 
				return	0;
			}
			else
			{
				try
				{
					// - set object_state
					socket_state(eSOCKET_STATE::CERTIFIED);

					// - call 'on_connect'
					on_connect();
				}
				catch (...)
				{
					// - restore socket status
					socket_state(eSOCKET_STATE::ESTABLISHED);

					// reraise)
					throw;
				}
			}
		}

		// - 훅함수 process_pre_message()를 호출한다.
		process_pre_message(m_msg_http_request);

		// - message을 Imessageable로 전달하여 처리한다.
		process_message(m_msg_http_request);

		// statistics) Paket수를 증가시킨다.
		statistics_on_receive_message();

		// - reset message
		m_msg_http_request.reset();

		// return)
		return 0;
	}

	// 3) chunked면 추가 처리해 준다.
	if (m_msg_http_request.is_chunked)
	{
		while (m_msg_http_request.content_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			process_pre_message(m_msg_http_request);

			// - message를 Imessageable로 전달하여 처리한다.
			process_message(m_msg_http_request);

			// - m_msg_http_request를 reset
			m_msg_http_request.content_completed = false;

			// - 다시 parsing 처리 함.
			auto size_process_nested = process_parsing_http(_buffer);

			// - 처리된 만큼 뺀다.
			_buffer += offset(size_process_nested);
		}

		// - complete면...
		if (m_msg_http_request.is_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			process_pre_message(m_msg_http_request);

			// - message를 Imessageable로 전달하여 처리한다.
			process_message(m_msg_http_request);

			// statistics) 
			statistics_on_receive_message();

			// - reset message
			m_msg_http_request.reset();
		}
	}

	// return)
	return m_msg_http_request.buf_message.size();
}

std::size_t net::io::Nhttp_server::process_packet( shared_buffer& _buffer, const net::sockaddr& _socket_address)
{
	// check) _buffer.data_가 0이어서는 안된다.
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_DEBUG << "(err ) _buffer.data_ is nullptr. _buffer.size_: "sv << _buffer.size() << " " __CGFUNCTION__ ""sv)

	// check) 최하 4Byte보다 작을 경우 그냥 끝낸다.
	RETURN_IF(_buffer.size() < 4, 0);

	// 1) message 처리
	switch (m_msg_http_request.protocol)
	{
	case	eHTTP_PROTOCOL::WEBSOCKET_:
			process_packet_websocket(_buffer, _socket_address);
			break;

	default:
			process_packet_http(_buffer, _socket_address);
			break;
	}

	// return) 
	return 8192;
}

std::string_view	find_paragraph(std::string_view& _string);
std::string_view	find_tok(std::string_view& _string, char _tok);
uint32_t			string_to_uint_hex(const std::string_view _string);
char*				str_find_terminal(char* _str_begin, char* _str_end) noexcept;

size_t net::io::Nhttp_server::process_parsing_http(shared_buffer _buffer)
{
	// declare) 
	size_t processed = 0;

	// check) 
	RETURN_IF(_buffer.size() <= 4, processed);

	// declare) 
	char* pos_now = _buffer.data();
	char* pos_end = _buffer.data() + _buffer.size();

	// 1) http (non-chunk)
	if(m_msg_http_request.is_chunked == false)
	{
		// - http_request head
		if(m_msg_http_request.content_length_now == 0)
		{
			// - 처리된 만큼을 건너띈다.
			pos_now += m_msg_http_request.buf_head.size();

			// - TERINAL_STRING(\n\r\n\r)을 찾는다.
			pos_now = str_find_terminal(pos_now, (pos_end - 3));

			// check) 해당 문자열을 찾지 못했으므로 끝낸다.
			if (pos_now == (pos_end - 3))
			{
				// - process를 저장한다.
				m_msg_http_request.buf_head.set_size( pos_now - _buffer.data() );

				// return) 0
				return	processed;
			}

			// - TERMINAML_STRING만큼을 이동...
			pos_now += 4;

			// - process를 저장한다.
			processed = pos_now - _buffer.data();

			// - http_resquest_head를 저장한다. (길이 +2는 마지막 \n\r\n\r을 넣기 위해서다.)
			m_msg_http_request.buf_head = _buffer ^ (pos_now - _buffer.data());

			// - Parsing [http_head]
			process_parsing_http_head(m_msg_http_request.buf_head);

			// - context가 없으면 여기서 종료한다.
			if (m_msg_http_request.is_chunked == false && m_msg_http_request.content_length_now == 0)
			{
				// - buf_message를 써넣는다. 
				m_msg_http_request.buf_message = _buffer;

				// - complete~
				m_msg_http_request.is_completed = true;
			}
		}

		// - http_request context (chunked)
		if (m_msg_http_request.content_length_now != 0)
		{
			// - 남은 길이를 얻는다.
			size_t length = pos_end - pos_now;

			// - 남은 길이가 buf_content.size_보다 크면 끝낸다.
			if ((m_msg_http_request.content_length_now) <= length)
			{
				// check)
				CGASSERT_ERROR(m_msg_http_request.contents.empty())

				// - shared_buffer를 설정한다.
				m_msg_http_request.contents.emplace_back(_buffer ^ buffer_view{pos_now, m_msg_http_request.content_length_now});
				m_msg_http_request.buf_message = m_msg_http_request.contents[0];

				// - completed를 증가시킨다.
				processed += m_msg_http_request.content_length_now;
				m_msg_http_request.content_completed = true;

				// - complete~
				m_msg_http_request.is_completed	 = true;
			}
		}
	}

	// 2) chunk
	if(m_msg_http_request.is_chunked == true)
	{
		// - 얻기...
		std::string_view temp_str_pos = std::string_view(pos_now, static_cast<uint32_t>(pos_end - pos_now));

		// - 첫줄을 읽는다.
		if (m_msg_http_request.content_length_now == 0)
		{
			// - 첫번째 줄을 얻는다.
			auto temp_str_first = find_paragraph(temp_str_pos);

			// - 첫줄을 읽어들였으면 진행
			if (temp_str_first.empty() == false)
			{
				// - chunk length
				auto chunk_length = string_to_uint_hex(temp_str_first);

				// - 첫줄이 '0'이 아니면 => chunk 진행
				if (chunk_length != 0)
				{
					// - context 길이를 써넣는다.
					m_msg_http_request.content_length_now = chunk_length;

					// - 진행 처리...
					processed += (temp_str_pos.data() - temp_str_first.data());
				}
				// - 첫줄이 '0'이면 => 종료.
				else
				{
					// - clear
					m_msg_http_request.content_length_now = 0;

					// - complete~
					m_msg_http_request.is_completed = true;
				}
			}
		}

		// - chunk를 읽는다.
		if(m_msg_http_request.content_length_now != 0)
		{
			// - shared_buffer를 설정한다.
			if((m_msg_http_request.content_length_now) <= temp_str_pos.size())
			{
				// - append exist data
				m_msg_http_request.contents[0].reserve(m_msg_http_request.contents[0].size() + m_msg_http_request.content_length_now);

				// - ...
				auto ptr_old = m_msg_http_request.contents[0].get_back_ptr();

				// - 설정한다.
				m_msg_http_request.contents[0] += buffer_view { const_cast<char*>(temp_str_pos.data()), m_msg_http_request.content_length_now };
				m_msg_http_request.buf_message  = m_msg_http_request.contents[0] ^ buffer_view{ ptr_old, m_msg_http_request.content_length_now };
				m_msg_http_request.content_completed = true;

				// - 진행 처리...
				processed += m_msg_http_request.content_length_now;
			}
		}
	}

	// return)
	return	processed;
}

std::u8string_view splite_path(std::u8string_view& _string)
{
	// declare)
	std::u8string_view str_path;

	// 1) begin & end
	auto pos_begin = _string.data();
	auto pos_end = pos_begin + _string.size();
	auto pos_now = pos_begin;

	// 2) 분리점을 찾는다.
	while(pos_now < pos_end && *pos_now != '?' && *pos_now != '#')
	{
		++pos_now;
	}

	// 3) ...
	_string	 = std::u8string_view(pos_now, pos_end - pos_now);
	str_path = std::u8string_view(pos_begin, pos_now - pos_begin);

	// return) 
	return	str_path;
}

sHTTP_PATH_ITEM splite_option(std::u8string_view& _string)
{
	// declare) 
	sHTTP_PATH_ITEM	temp_path_item;

	// check) 한자도 없으면 끝낸다.
	RETURN_IF(_string.empty(), temp_path_item);

	auto pos_begin = _string.data();
	auto pos_end = pos_begin + _string.size();
	auto pos_now = pos_begin + 1; // 주의) 첫자('/' '?' '#')는 비교하지 않는다.

	// 1) splite을 찾는다.
	while(pos_now < pos_end  && *pos_now != '?' && *pos_now != '#' && *pos_now != '&')
	{
		++pos_now;
	}

	// 2) 남은 string 적용
	_string	 = std::u8string_view(pos_now, pos_end - pos_now);

	// 3) splite된 부분 작성
	temp_path_item.type	 = *pos_begin;
	temp_path_item.value = std::string_view(reinterpret_cast<const char*>(pos_begin + 1), pos_now - pos_begin -1);
	temp_path_item.key	 = find_tok(temp_path_item.value, '=');

	// return) 
	return temp_path_item;
}

uint32_t parsing_http_version(std::string_view _str_http_protocol)
{
	if (_str_http_protocol == "HTTP/1.1"sv)
	{
		return	HTTP_VERSION(1, 1);
	}
	else if (_str_http_protocol == "HTTP/2.0"sv)
	{
		return	HTTP_VERSION(2, 0);
	}
	else
	{
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	}
}

std::string_view find_paragraph(std::string_view& _string);
std::string_view find_tok(std::string_view& _string, char _tok);

// - url decoding을 수행한다.
std::u8string url_encoding(std::u8string_view _url)
{
	// declare) 
	std::u8string output_string;

	// 1) ...
	output_string.reserve(_url.size() * 3);

	// 2) 
	auto iter = _url.begin();
	auto iter_end = _url.end();

	for (; iter != iter_end; ++iter)
	{
		// - get character
		auto ch = *iter;

		if ((ch >= '0' && ch <= '9')
		 || (ch >= 'a' && ch <= 'z')
		 || (ch >= 'A' && ch <= 'Z')
		 ||  ch == '$'
		 ||  ch == '-'
		 ||  ch == '_'
		 ||  ch == '.'
		 ||  ch == '+'
		 ||  ch == '!'
		 ||  ch == '*'
		 ||  ch == '\''
		 ||  ch == '('
		 ||  ch == ')'
		 ||  ch == ','
		 )
		{
			output_string.push_back(ch);
		}
		else
		{
			char temp[8]{0};
			__sprintf(temp, "%02x"sv, ch);
			output_string.push_back('%');
			output_string.push_back(temp[0]);
			output_string.push_back(temp[1]);
		}
	}

	// return) 
	return output_string;
}

void url_decoding(std::u8string_view& _url)
{
	// check) 
	RETURN_IF(_url.empty());

	std::string_view std_temp (reinterpret_cast<const char*>(_url.data()), _url.size());

	// 1) ...
	auto iter_src = _url.begin();
	auto iter_src_end = _url.end();
	char8_t* iter_dest = const_cast<char8_t*>(&_url.front());
	auto remained = _url.size();

	while(iter_src != iter_src_end)
	{
		if (*iter_src != '%')
		{
			*iter_dest++ = *iter_src++;
		}
		else
		{
			// check) 길이가 2Byte 이하면 Exception을 던진다.
			THROW_IF(remained < 3, throwable::failure(eRESULT::LENGTH_ERROR));

			// - remove '%'
			++iter_src;

			// - %로 시작할 경우 Binary값이므로 변환한다.
			char vaule1 = *iter_src++;
			char vaule2 = *iter_src++;

			vaule1 = (vaule1 >= '0' && vaule1 <= '9') ? (vaule1 - '0') : (vaule1 >= 'A' && vaule1 <= 'F') ? (vaule1 - 'A' + 10) : (vaule1 >= 'a' && vaule1 <= 'z') ? (vaule1 - 'a' + 10) : 0;
			vaule2 = (vaule2 >= '0' && vaule2 <= '9') ? (vaule2 - '0') : (vaule2 >= 'A' && vaule2 <= 'F') ? (vaule2 - 'A' + 10) : (vaule2 >= 'a' && vaule2 <= 'z') ? (vaule2 - 'a' + 10) : 0;

			*iter_dest++ = (vaule1 << 4) | vaule2;
		}
	}
	*iter_dest = 0;

	auto size_s = static_cast<int>(iter_dest - &_url.front());
	std::string_view str_url( reinterpret_cast<const char*>(&_url.front()), size_s );

	// 2) set...
	_url = std::u8string_view( &_url.front(), iter_dest - &_url.front() );
}

void net::io::Nhttp_server::process_parsing_http_head(shared_buffer _buffer)
{
	// check)
	CGASSERT_ERROR(m_msg_http_request.headers.empty());
	CGASSERT_ERROR(m_msg_http_request.contents.empty());
	CGASSERT_ERROR(m_msg_http_request.method == eHTTP_METHOD::NONE_);
	CGASSERT_ERROR(m_msg_http_request.protocol == eHTTP_PROTOCOL::NOT_DEFINED_);
	CGASSERT_ERROR(m_msg_http_request.version == 0);
	CGASSERT_ERROR(m_msg_http_request.path.empty());

	// 1) decoding처리를 한다.

	// 2) parsing string
	std::string_view temp_str_pos = _buffer;
	
	// 3) request line (첫번째줄)
	{
		// - 첫번째 줄을 얻는다.
		auto temp_str_first = find_paragraph(temp_str_pos);

		// check) 없으면 exception을 던진다.
		THROW_IF(temp_str_first.empty(), throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv))

		// - methoid를 확인한다.
		switch (temp_str_first[0])
		{
		// "GET"
		case	'G':
				{
					THROW_IF(temp_str_first[1] != 'E' || temp_str_first[2] != 'T' || temp_str_first[3] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(4);

					m_msg_http_request.method = eHTTP_METHOD::GET_;
				}
				break;

		// "POST" or "PUT" or "PATCH" or "PURGE" or "PROPFIND"
		case	'P':
				{
					switch(temp_str_first[1])
					{
					// "POST"
					case	'O':
							{
								THROW_IF(temp_str_first[2] != 'S' || temp_str_first[3] != 'T' || temp_str_first[4] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(5);

								m_msg_http_request.method = eHTTP_METHOD::POST_;
							}
							break;

					// "PUT" or "PURGE"
					case	'U':
							{
								switch(temp_str_first[2])
								{
								// "PUT"
								case	'T':
										{
											THROW_IF(temp_str_first[3] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
											temp_str_first.remove_prefix(4);

											m_msg_http_request.method = eHTTP_METHOD::PUT_;
										}
										break;

								// "PURGE"
								case	'U':
										{
											THROW_IF(temp_str_first[3] != 'R' || temp_str_first[4] != 'G' || temp_str_first[5] != 'E' || temp_str_first[6] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
											temp_str_first.remove_prefix(6);

											m_msg_http_request.method = eHTTP_METHOD::PURGE_;
										}
										break;

								default:
										throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);
								}
							}
							break;

					// "PATCH"
					case	'A':
							{
								THROW_IF(temp_str_first[2] != 'T' || temp_str_first[3] != 'C' || temp_str_first[4] != 'H' || temp_str_first[5] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(6);

								m_msg_http_request.method = eHTTP_METHOD::PATCH_;
							}
							break;

					// "PROPFIND"
					case	'R':
							{
								THROW_IF(temp_str_first[2] != 'O' || temp_str_first[3] != 'P' || temp_str_first[4] != 'F' || temp_str_first[5] != 'I' || temp_str_first[6] != 'N' || temp_str_first[7] != 'D' || temp_str_first[8] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(9);

								m_msg_http_request.method = eHTTP_METHOD::PROPFIND_;
							}
							break;

					default:
							throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);
					}
				}
				break;

		// COPY
		case	'C':
				{
					THROW_IF(temp_str_first[1] != 'O' || temp_str_first[2] != 'P' || temp_str_first[3] != 'Y' || temp_str_first[6] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(5);

					m_msg_http_request.method = eHTTP_METHOD::COPY_;
				}
				break;

		// DELETE
		case	'D':
				{
					THROW_IF(temp_str_first[1] != 'E' || temp_str_first[2] != 'L' || temp_str_first[3] != 'E' || temp_str_first[4] != 'T' || temp_str_first[5] != 'E' || temp_str_first[6] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(7);

					m_msg_http_request.method = eHTTP_METHOD::DELETE_;
				}
				break;

		// HEAD
		case	'H':
				{
					THROW_IF(temp_str_first[1] != 'E' || temp_str_first[2] != 'A' || temp_str_first[3] != 'D' || temp_str_first[4] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(5);

					m_msg_http_request.method = eHTTP_METHOD::HEAD_;
				}
				break;

		// OPTIONS
		case	'O':
				{
					THROW_IF(temp_str_first[1] != 'P' || temp_str_first[2] != 'T' || temp_str_first[3] != 'I' || temp_str_first[4] != 'O' || temp_str_first[5] != 'N' || temp_str_first[6] != 'S' || temp_str_first[7] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(8);

					m_msg_http_request.method = eHTTP_METHOD::OPTIONS_;
				}
				break;

		// LINK or LOCK
		case	'L':
				{
					switch(temp_str_first[1])
					{
					// "LINK"
					case	'I':
							{
								THROW_IF(temp_str_first[2] != 'N' || temp_str_first[3] != 'K' || temp_str_first[4] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(5);

								m_msg_http_request.method = eHTTP_METHOD::LINK_;
							}
							break;

					// "LOCK"
					case	'O':
							{
								THROW_IF(temp_str_first[2] != 'C' || temp_str_first[3] != 'K' || temp_str_first[4] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(5);

								m_msg_http_request.method = eHTTP_METHOD::LOCK_;
							}
							break;

					default:
							throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);
					}
				}
				break;

		// UNLINK or UNLOCK
		case	'U':
				{
					THROW_IF(temp_str_first[1] != 'N' || temp_str_first[2] != 'L', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));

					switch (temp_str_first[3])
					{
					// "UNLINK"
					case	'I':
							{
								THROW_IF(temp_str_first[4] != 'N' || temp_str_first[5] != 'K' || temp_str_first[6] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(7);

								m_msg_http_request.method = eHTTP_METHOD::UNLINK_;
							}
							break;

					// "UNLOCK"
					case	'O':
							{
								THROW_IF(temp_str_first[4] != 'C' || temp_str_first[5] != 'K' || temp_str_first[6] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
								temp_str_first.remove_prefix(7);

								m_msg_http_request.method = eHTTP_METHOD::UNLOCK_;
							}
							break;

					default:
							throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);
					}
				}
				break;

		// VIEW
		case	'V':
				{
					THROW_IF(temp_str_first[1] != 'I' || temp_str_first[2] != 'E' || temp_str_first[3] != 'W' || temp_str_first[4] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(5);

					m_msg_http_request.method = eHTTP_METHOD::VIEW_;
				}
				break;

		// TRACE
		case	'T':
				{
					THROW_IF(temp_str_first[1] != 'R' || temp_str_first[2] != 'A' || temp_str_first[3] != 'C' || temp_str_first[4] != 'E' || temp_str_first[5] != ' ', throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv));
					temp_str_first.remove_prefix(6);

					m_msg_http_request.method = eHTTP_METHOD::TRACE_;
				}
				break;

		default:
				throw throwable::failure(eRESULT::FAIL);
		}

		// - trim (remove blank)
		temp_str_first.remove_prefix(std::min(temp_str_first.find_first_not_of(" "sv), temp_str_first.size()));

		// - path를 얻는다.
		auto result_path = find_tok(temp_str_first, ' ');
		m_msg_http_request.path = std::u8string_view(reinterpret_cast<const char8_t*>(result_path.data()), result_path.size());

		// check) Path가 없으면 exception을 던진다.
		THROW_IF(m_msg_http_request.path.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid path"sv));

		// check) Path가아 니면 Exception을 던진다.(path는 무조건 '/'나 '*'로 시작한다.)
		THROW_IF(m_msg_http_request.path[0] != '/' && m_msg_http_request.path[0] != '*', throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "Invalid path"sv));

		if(m_msg_http_request.method == eHTTP_METHOD::GET_ && m_msg_http_request.path[0] != '*')
		{
			// declare) 
			std::u8string_view temp_path = m_msg_http_request.path;

			// - 복사한다.
			m_msg_http_request.path = splite_path(temp_path);

			// - char
			char type = 0;

			// - 다른 옵션들을 읽어들인다. ('#' 혹은 '?')
			for(;;)
			{
				// - splite해낸다.
				sHTTP_PATH_ITEM	temp_path_item = splite_option(temp_path);

				// check) 없으면 여기서 끝낸다.
				BREAK_IF(temp_path_item.type == 0);

				// - type을 설정한다.
				if (temp_path_item.type != '&')
				{
					type = temp_path_item.type;
				}
				else
				{
					temp_path_item.type = type;
				}

				// - 추가하기
				m_msg_http_request.parameters.push_back(temp_path_item);
			}
		}

		// - trim (remove blank)
		temp_str_first.remove_prefix(std::min(temp_str_first.find_first_not_of(" "sv), temp_str_first.size()));

		// - protocol을 써넣는다.
		if (temp_str_first.size() > 1)
		{
			// - protocol을 얻는다.
			m_msg_http_request.protocol = eHTTP_PROTOCOL::HTTP_;
			m_msg_http_request.version  = parsing_http_version(temp_str_first);
		}
	}

	// 4) request header (둘째줄부터~끝)
	{
		// declare) 
		std::string_view temp_str_now;

		while ((temp_str_now = find_paragraph(temp_str_pos)).empty() == false)
		{
			// declare) 
			sHTTP_HEADER_ITEM temp_param;

			// - ...
			temp_param.key = find_tok(temp_str_now, ':');
			temp_param.value = temp_str_now;

			// - "Content-Length"인지 확인한다.
			if (temp_param.key == "Content-Length")
			{
				m_msg_http_request.content_length_now = __from_string<uint32_t>(temp_param.value);
			}
			else if (temp_param.key == "Transfer-Encoding"sv)
			{
				//if(str_now == "chunked")
				{
					m_msg_http_request.is_chunked = true;
				}
			}

			// - 추가한다.
			m_msg_http_request.headers.push_back(temp_param);
		}
	}

	// 5) buf_head를 설정한다.
	m_msg_http_request.buf_head = _buffer;
}

result_code net::io::Nhttp_server::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code result = eRESULT::BYPASS;

	// 1) message fuction
	if ((_msg.message & eMESSAGE::SYSTEM::NETWORK_MASK) == eMESSAGE::SYSTEM::NETWORK)
	{
		switch (_msg.message)
		{
		case	eMESSAGE::SYSTEM::HTTP_REQUEST:
				result = on_message(static_cast<sMESSAGE_HTTP_REQUEST&>(_msg));
				break;

		case	eMESSAGE::SYSTEM::HTTP_RESPONSE:
				CGASSERT_ERROR(false);
				break;

		case	eMESSAGE::SYSTEM::WEBSOCKET:
				result = on_message(static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg));
				break;

		default:
				result = on_message(static_cast<sMESSAGE_NETWORK&>(_msg));
				break;
		}
	}

	// return)
	return result;
}
