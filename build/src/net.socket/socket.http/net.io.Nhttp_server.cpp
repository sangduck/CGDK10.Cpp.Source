#include "pch.h"
#include <charconv>

// SSL Include files
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/buffer.h> 
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <cmath>

using namespace CGDK;

void apply_websocket_mask(char* _data, std::size_t _size, const uint8_t* _mask) noexcept;

void net::io::Nhttp_server::process_on_connect()
{
}

void net::io::Nhttp_server::process_on_fail_connect(uint64_t _disconnect_reason)
{
	this->on_fail_connect(_disconnect_reason);
}
void net::io::Nhttp_server::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
		this->on_disconnect(_disconnect_reason);
}

shared_buffer message_web_socket_handshake(const std::string_view _websocket_protocol, const std::string_view _websocket_accept, const std::string_view _websocket_location = "ws://127.0.0.1")
{
	// 1) shared_buffer를 할당받는다.
	auto temp_buffer = alloc_shared_buffer(4096);

	// 2) 메시지를 작성한다.
	temp_buffer.append_text("HTTP/1.1 101 Switching Protocols\r\n"sv);
	temp_buffer.append_text("Upgrade: websocket\r\n"sv);
	temp_buffer.append_text("Connection: Upgrade\r\n"sv);
	temp_buffer.append_text(CGDK::stream_string("Sec-WebSocket-Location: ", _websocket_location, "\r\n"));
	if (_websocket_protocol.empty() == false)
	{
		temp_buffer.append_text(CGDK::stream_string("Sec-WebSocket-Protocol: ", _websocket_protocol, "\r\n"));
	}
	temp_buffer.append_text(CGDK::stream_string("Sec-WebSocket-Accept: ", _websocket_accept, "\r\n"));
	temp_buffer.append_text("Server: CGCII/10.0.0\r\n"sv);
	temp_buffer.append_text("\r\n"sv);

	// return)
	return	temp_buffer;
}

char get_next_char(char*& _pnow, const char* _pend)
{
	// check) End면 끝낸다.
	if(_pnow == _pend)
		return 0;

	if (*_pnow == '%')
	{
		// check) 길이가 2Byte 이하면 Exception을 던진다.
		if((_pend-_pnow) < 3)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

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
	char ch_read = get_next_char(_pnow, _pend);

	// check)
	if(ch_read == 0)
		return 0;

	// Case-1) 0x80보다 작으면 그냥 그 값을 Return한다.(일반적인 값임)
	if ((ch_read & 0x80) == 0)
	{
		return ch_read;
	}
	// Case-2) 110xxxxx 10xxxxxx
	else if ((ch_read & 0xe0) == 0xc0)
	{
		// - Character 읽기
		char wch_next = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		return	((ch_read & 0x1f) << 6) | (wch_next & 0x3f);
	}
	// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
	else if ((ch_read & 0xf0) == 0xe0)
	{
		// - 1번째 Character
		char wch_next1 = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next1 == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		// - 2번째 Character
		char wch_next2 = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next2 == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		return ((ch_read & 0x0f)<<12) | ((wch_next1 & 0x3f)<<6) | (wch_next2 & 0x3f);
	}
	// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((ch_read & 0xf8) == 0xf0)
	{
		//CGASSERT_ERROR(false);

		//// check) NULL이 돌아오면 Exception을 던진다.
		//throw	throwable::failure(400, "Invalid Request"sv);

		// - 1번째 Character
		char wch_next1 = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next1 == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		// - 2번째 Character
		char wch_next2 = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next2 == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		// - 3번째 Character
		char wch_next3 = get_next_char(_pnow, _pend);

		// check) NULL이 돌아오면 Exception을 던진다.
		if(wch_next3 == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid request"sv);

		return ((ch_read & 0x07) << 18) | ((wch_next1 & 0x3f) << 12) | ((wch_next2 & 0x3f) << 6) | (wch_next3 & 0x3f);
	}

	// return) Error...
	return	0;
}

void net::io::Nhttp_server::_throw_too_large(std::string_view _what, std::size_t _size, std::size_t _max) const
{
	// log)
	LOG_WARNING << "(warn) CGDK.net.socket.http: "sv << _what << " is too large ("sv << _size << " > "sv << _max << " bytes). disconnect"sv;

	// - 접속을 끊는다. (receivable에서 exception을 받으면 socket을 닫는다.)
	throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH);
}

void net::io::Nhttp_server::_process_websocket_data(uint16_t _opcode, bool _is_fin, const shared_buffer& _buffer, const buffer_view& _payload)
{
	// declare)
	auto f_dispatch = [this](uint16_t _opcode_message, shared_buffer&& _buf_message)
	{
		sMESSAGE_HTTP_WEBSOCKET msg;
		msg.method = (_opcode_message == 1) ? eHTTP_METHOD::TEXT_ : eHTTP_METHOD::BINARY_;
		msg.buf_message = std::move(_buf_message);

		// - message를 Imessageable로 전달하여 처리한다.
		this->process_message(msg);

		// statistics)
		this->statistics_on_receive_message();
	};
	auto f_append = [this](const buffer_view& _data)
	{
		if (this->m_max_body_size != 0 && this->m_ws_fragments.size() + _data.size() > this->m_max_body_size)
			this->_throw_too_large("websocket fragmented message"sv, this->m_ws_fragments.size() + _data.size(), this->m_max_body_size);
		this->m_ws_fragments.append(reinterpret_cast<const char*>(_data.data()), _data.size());
	};

	// 1) continuation (시작 frame 없이 온 것은 버린다.)
	if (_opcode == 0)
	{
		RETURN_IF(this->m_ws_fragment_opcode == 0);

		f_append(_payload);
		RETURN_IF(_is_fin == false);

		auto buf_message = alloc_shared_buffer(this->m_ws_fragments.size() + 1);
		if (this->m_ws_fragments.empty() == false)
			buf_message.append(this->m_ws_fragments.size(), this->m_ws_fragments.data());

		auto opcode_message = this->m_ws_fragment_opcode;
		this->m_ws_fragments.clear();
		this->m_ws_fragment_opcode = 0;

		f_dispatch(opcode_message, std::move(buf_message));
		return;
	}

	// 2) 새 text/binary frame (끝나지 않은 조각이 있으면 버린다.)
	this->m_ws_fragments.clear();
	this->m_ws_fragment_opcode = 0;

	// - 조각의 시작
	if (_is_fin == false)
	{
		this->m_ws_fragment_opcode = _opcode;
		f_append(_payload);
		return;
	}

	// - 한 frame
	f_dispatch(_opcode, _buffer ^ _payload);
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

			// 3) 첫 번째 메시지인가 아닌가.
			bool  is_fin = (header & 0x0080)!=0;
			uint16_t Opcode = (header & 0x000f); // opcode 0:continue, 1:text, 2:binary 8:close

			// 4) message length (126: 2 byte 길이, 127: 8 byte 길이. 길이 byte가 아직 다 오지 않았으면 기다린다.)
			bool bMasked = (header & 0x8000) != 0;
			uint64_t length = (header & 0x7f00)>>8;

			BREAK_IF(length == 126 && buf_apply.size() < 2);
			BREAK_IF(length == 127 && buf_apply.size() < 8);

			// 5) length가 126일 경우 길이용 바이트로 2Byte 추가.
			if (length == 126)
			{
				// - 길이를 0으로 리셋
				length = 0;

				// - 2Byte 읽어 뒤집어 사용한다.
				for(int i=0;i<2; ++i)
				{
					length += static_cast<uint64_t>(buf_apply.extract<uint8_t>()) << (8 * (2 - 1 - i));
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
					length += static_cast<uint64_t>(buf_apply.extract<uint8_t>()) << (8 * (8 - 1 - i));
				}
			}

			// check) frame 크기 제한
			if (this->m_max_body_size != 0 && length > this->m_max_body_size)
				this->_throw_too_large("websocket frame"sv, static_cast<std::size_t>(length), this->m_max_body_size);

			// 7) Masked일 경우 Masked 처리를 한다.
			if(bMasked)
			{
				// check) length보다 길이가 작으면 여기서 끝낸다.
				//        주의!!) 이 BREAK문은 반드시 여기서 BREAK를 해야한다. 이유는 중복 Masking 처리를 방지하기 위해서이다.
				//        (size가 mask(4 byte)보다 작을 수 있으므로 빼지 않고 더해서 비교한다.)
				//        (overflow safe : with no body limit a 64 bit length near the max would wrap 'sizeof(uint32_t) + length')
				BREAK_IF(buf_apply.size() < sizeof(uint32_t) || buf_apply.size() - sizeof(uint32_t) < length);

				// declare) 
				uint8_t	mask[4];
		
				// - Mask를 읽어낸다
				*(uint32_t*)mask = buf_apply.extract<uint32_t>();


				// - Mask값을 XOR처리한다.
				apply_websocket_mask(buf_apply.data<char>(), static_cast<std::size_t>(length), mask);
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
			// CASE) continue / Text / Binary (조각은 모아서 하나로 전달한다.)
			case	0:
			case	1:
			case	2:
					this->_process_websocket_data(Opcode, is_fin, _buffer, buf_payload_data);
					break;

			// CASE) close
			case	8:
					{
						// - close frame을 돌려주고 끊는다. (RFC 6455 5.5.1 : status code(2 byte)만 돌려준다.)
						auto buf_close = alloc_shared_buffer(4);
						buf_close.append<uint8_t>(0x88);
						if (length >= 2)
						{
							buf_close.append<uint8_t>(2);
							buf_close.append(2, buf_payload_data.data());
						}
						else
						{
							buf_close.append<uint8_t>(0);
						}
						process_send(std::move(buf_close), 1, 0);

						disconnect();
					}
					break;

			// CASE) PING 요청 시
			case	9:
					{
						// - Pong을 전송한다. (opcode 0x0a, Ping과 같은 payload. 서버는 mask하지 않는다.)
						//   (이 frame의 payload만 보낸다. control frame의 payload는 125 byte 이하다.)
						if (length <= 125)
						{
							auto buf_pong = alloc_shared_buffer(2 + static_cast<std::size_t>(length));
							buf_pong.append<uint8_t>(0x8a);
							buf_pong.append<uint8_t>(static_cast<uint8_t>(length));
							if (length != 0)
								buf_pong.append(static_cast<std::size_t>(length), buf_payload_data.data());

							process_send(std::move(buf_pong), 1, 0);
						}
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
				// - 접속 종료 시
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

bool net::io::Nhttp_server::send_websocket_ping()
{
	// check)
	if (this->is_websocket() == false)
		return false;

	// - PING (opcode 0x9, payload 없음. 서버는 mask하지 않는다.)
	auto buf_ping = alloc_shared_buffer(2);
	buf_ping.append<uint8_t>(0x89);
	buf_ping.append<uint8_t>(0x00);

	return this->process_send(std::move(buf_ping), 1, 0);
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
	// - EVP_EncodeBlock : no new line, '\0' terminated (4 * ceil(n / 3) + 1 bytes)
	std::string base64(4 * ((_ascii.size() + 2) / 3) + 1, '\0');

	const auto length = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(base64.data()), reinterpret_cast<const unsigned char*>(_ascii.data()), static_cast<int>(_ascii.size()));
	if (length < 0)
		return std::string();

	base64.resize(static_cast<size_t>(length));

	// return)
	return base64;
}

bool base64_encode(char* _dest, size_t _dest_size, const std::string_view _source) noexcept
{
	// check) the size of the result + '\0'
	const size_t base64_length = 4 * ((_source.size() + 2) / 3);
	if (_dest == nullptr || base64_length >= _dest_size)
		return false;

	// - EVP_EncodeBlock : no new line, '\0' terminated
	const auto length = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(_dest), reinterpret_cast<const unsigned char*>(_source.data()), static_cast<int>(_source.size()));

	// return)
	return length >= 0 && static_cast<size_t>(length) == base64_length;
}

void encode_web_socket_key(char* _dest, const std::string_view _key)
{
	// check)
	CGASSERT_ERROR(_key.size() == 24);
	if(_key.size() != 24)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

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
	if(result == false)
		throw throwable::failure(eRESULT::FAIL);
}

bool check_web_socket_key(const std::string_view _compare, const std::string_view _source) noexcept
{
	// check)
	CGASSERT_ERROR(_source.size() == 24);
	if(_source.size() != 24)
		return false;

	// check)
	CGASSERT_ERROR(_compare.size() == 28);
	if(_compare.size() != 28)
		return false;

	// declare) 
	char str_source[256];

	// 1) convert string
	memcpy(str_source, _source.data(), _source.size());

	// 2) string 붙이기
	memcpy(str_source + 24, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 37);

	// 3) SHA1로 Encoding
	char str_sha1[256];
	SHA1(reinterpret_cast<const unsigned char*>(str_source), 60, reinterpret_cast<unsigned char*>(str_sha1));

	// declare) 
	char str_result[256];

	// 4) Base64로 다시 Encoding
	auto result = base64_encode(str_result, 256, std::string_view(str_sha1, 20));

	// check) ...
	if(result == false)
		return false;

	// 5) compare keyword
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
				// check) upgrade (ex. Origin) : '403 Forbidden' and disconnect
				if (this->on_websocket_upgrade(m_msg_http_request) == false)
				{
					auto buf_forbidden = alloc_shared_buffer(128);
					buf_forbidden.append_text("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv);
					process_send(std::move(buf_forbidden), 1, 0);
					this->disconnect();
					return 0;
				}

				// - WebSocket-Key를 얻는다.
				auto str_web_socket_key = m_msg_http_request.get_header("Sec-WebSocket-Key"sv);

				// check)
				CGASSERT_ERROR(str_web_socket_key.size() == 24);

				// check) WebSocketKey가 없으면 Exception을 던진다.
				if(str_web_socket_key.size() != 24)
					throw throwable::failure(eRESULT::FAIL_NO_WEBSOCKET_KEY);

				// declare)
				uint32_t version = HTTP_VERSION(1,1);

				// - Version을 얻는다.
				auto str_web_socket_version = m_msg_http_request.get_header("Sec-WebSocket-Version"sv);

				// check) Version 정보를 얻느다.
				if (str_web_socket_version.empty() == false)
				{
					version = 0;
					std::from_chars(str_web_socket_version.data(), str_web_socket_version.data() + str_web_socket_version.size(), version);
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
				CGASSERT_FATAL(pacceptor != nullptr);

				{
					// declare)
					std::string_view str_address;
					std::string temp_string;

					// - accept address를 얻는다.
					if (pacceptor != nullptr)
					{
						temp_string = pacceptor->get_accept_endpoint().address().to_string();
						str_address = temp_string;
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
					// - set object_state (CAS : ESTABLISHED일 때만 CERTIFIED로 바꾸고 on_connect를 호출한다.
					//   그 사이 닫히는 중이면(오류 응답 후 disconnect 등) 상태를 덮어쓰지 않고 on_connect 없이 메시지만 전달한다)
					if (this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED))
						on_connect();
				}
				catch (...)
				{
					// - restore socket status (CAS : CERTIFIED일 때만)
					this->set_socket_state_if(eSOCKET_STATE::CERTIFIED, eSOCKET_STATE::ESTABLISHED);

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
					// - set object_state (CAS : ESTABLISHED일 때만 CERTIFIED로 바꾸고 on_connect를 호출한다.
					//   그 사이 닫히는 중이면(오류 응답 후 disconnect 등) 상태를 덮어쓰지 않고 on_connect 없이 메시지만 전달한다)
					if (this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED))
						on_connect();
				}
				catch (...)
				{
					// - restore socket status (CAS : CERTIFIED일 때만)
					this->set_socket_state_if(eSOCKET_STATE::CERTIFIED, eSOCKET_STATE::ESTABLISHED);

					// reraise)
					throw;
				}
			}
		}

		// - 훅함수 process_pre_message()를 호출한다.
		process_pre_message(m_msg_http_request);

		// - message을 Imessageable로 전달하여 처리한다.
		process_message(m_msg_http_request);

		// statistics) Packet수를 증가시킨다.
		statistics_on_receive_message();

		// - reset message
		m_msg_http_request.reset();
		this->_reset_chunk();

		// return)
		return 0;
	}

	// return) (chunked body도 process_parsing_http에서 다 모은 후 한 번에 전달한다.)
	return m_msg_http_request.buf_message.size();
}

std::size_t net::io::Nhttp_server::process_packet( shared_buffer& _buffer, const net::sockaddr& _socket_address)
{
	// check) _buffer.data_가 0이어서는 안된다.
	if (_buffer.data() == nullptr)
	{
		LOG_DEBUG << "(err ) _buffer.data_ is nullptr. _buffer.size_: "sv << _buffer.size() << " " __CGFUNCTION__ ""sv;
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	}

	// check) 최하 4Byte보다 작을 경우 그냥 끝낸다.
	if(_buffer.size() == 0)
		return 0;

	// 1) message 처리 (한 번 받은 데이터에 message가 여럿 있으면(pipelining) 모두 처리한다.)
	for (;;)
	{
		auto size_before = _buffer.size();
		auto protocol_before = m_msg_http_request.protocol;

		if (protocol_before == eHTTP_PROTOCOL::WEBSOCKET_)
			process_packet_websocket(_buffer, _socket_address);
		else
			process_packet_http(_buffer, _socket_address);

		// check) 더 처리한 것이 없다. (message가 아직 다 오지 않았다.)
		BREAK_IF(_buffer.size() == 0 || (_buffer.size() == size_before && m_msg_http_request.protocol == protocol_before));

		// check) 접속을 끊는 중이다.
		auto state = this->socket_state();
		BREAK_IF(state >= eSOCKET_STATE::SEND_DISCONNECTED && state <= eSOCKET_STATE::LAST_ACK);
	}

	// return) 
	return 8192;
}

std::string_view	find_paragraph(std::string_view& _string);
std::string_view	find_tok(std::string_view& _string, char _tok);
char*				str_find_terminal(char* _str_begin, char* _str_end) noexcept;

size_t net::io::Nhttp_server::process_parsing_http(shared_buffer _buffer)
{
	// declare) 
	size_t processed = 0;

	// check) 
	if(_buffer.size() == 0)
		return processed;

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
			//   (head가 아직 다 오지 않았을 때 찾아본 위치. buf_head는 비어 있으므로 크기를 바꾸지 않는다.)
			pos_now += this->m_size_head_scanned;

			// check) head 끝(CRLF CRLF)을 찾을 수 있을 만큼 오지 않았다. (4 byte)
			if (pos_end - pos_now < 4)
				return processed;

			// - TERINAL_STRING(\n\r\n\r)을 찾는다.
			pos_now = str_find_terminal(pos_now, (pos_end - 3));

			// check) 해당 문자열을 찾지 못했으므로 끝낸다. (header가 너무 길면 끊는다.)
			if (pos_now == (pos_end - 3))
			{
				if (this->m_max_header_size != 0 && static_cast<std::size_t>(pos_end - _buffer.data()) > this->m_max_header_size)
					this->_throw_too_large("http header"sv, static_cast<std::size_t>(pos_end - _buffer.data()), this->m_max_header_size);

				// - process를 저장한다.
				this->m_size_head_scanned = static_cast<std::size_t>(pos_now - _buffer.data());

				// return) 0
				return	processed;
			}

			// - TERMINAML_STRING만큼을 이동...
			pos_now += 4;
			this->m_size_head_scanned = 0;

			// check) header 크기 제한 (한 번에 다 받은 경우)
			if (this->m_max_header_size != 0 && static_cast<std::size_t>(pos_now - _buffer.data()) > this->m_max_header_size)
				this->_throw_too_large("http header"sv, static_cast<std::size_t>(pos_now - _buffer.data()), this->m_max_header_size);

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

	// 2) chunked body
	//    - size line(hex[;ext]) -> data + CRLF -> ... -> '0' -> trailer header들 -> 빈 줄
	//    - 다 모은 후 한 번에 전달한다. (받은 만큼 처리하고 상태(m_chunk_state)를 기억한다.)
	if (m_msg_http_request.is_chunked == true && m_msg_http_request.is_completed == false)
	{
		for (;;)
		{
			std::string_view rest(pos_now, static_cast<std::size_t>(pos_end - pos_now));

			// - size line
			if (this->m_chunk_state == 0)
			{
				auto pos_line = rest.find("\r\n"sv);
				if (pos_line == std::string_view::npos)
				{
					if (rest.size() > 1024)
						this->_throw_too_large("http chunk size line"sv, rest.size(), 1024);
					break;
				}

				auto line = rest.substr(0, pos_line);
				if (auto pos_ext = line.find(';'); pos_ext != std::string_view::npos)
					line = line.substr(0, pos_ext);
				while (line.empty() == false && (line.back() == ' ' || line.back() == '\t'))
					line.remove_suffix(1);

				std::size_t chunk_size = 0;
				auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), chunk_size, 16);
				if (line.empty() || ec != std::errc() || ptr != line.data() + line.size())
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid chunk size"sv);

				pos_now += pos_line + 2;
				processed = static_cast<size_t>(pos_now - _buffer.data());

				if (chunk_size == 0)
				{
					this->m_chunk_state = 2;
					continue;
				}

				// check) the size + CRLF must not overflow (a size near SIZE_MAX would wrap the checks below)
				if (chunk_size > (std::numeric_limits<std::size_t>::max)() - 2 - this->m_chunked_body.size())
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid chunk size"sv);

				if (this->m_max_body_size != 0 && chunk_size > this->m_max_body_size - (std::min)(this->m_chunked_body.size(), this->m_max_body_size))
					this->_throw_too_large("http chunked body"sv, this->m_chunked_body.size() + chunk_size, this->m_max_body_size);

				this->m_chunk_size = chunk_size;
				this->m_chunk_state = 1;
			}
			// - data + CRLF
			else if (this->m_chunk_state == 1)
			{
				BREAK_IF(rest.size() < this->m_chunk_size || rest.size() - this->m_chunk_size < 2);

				// check) the data must be followed by CRLF
				if (rest[this->m_chunk_size] != '\r' || rest[this->m_chunk_size + 1] != '\n')
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid chunk data"sv);

				this->m_chunked_body.append(rest.substr(0, this->m_chunk_size));
				pos_now += this->m_chunk_size + 2;
				processed = static_cast<size_t>(pos_now - _buffer.data());
				this->m_chunk_state = 0;
			}
			// - trailer (빈 줄이 나올 때까지. trailer header는 쓰지 않는다.)
			else
			{
				auto pos_line = rest.find("\r\n"sv);
				if (pos_line == std::string_view::npos)
				{
					if (this->m_max_header_size != 0 && rest.size() > this->m_max_header_size)
						this->_throw_too_large("http chunk trailer"sv, rest.size(), this->m_max_header_size);
					break;
				}

				pos_now += pos_line + 2;
				processed = static_cast<size_t>(pos_now - _buffer.data());

				// - trailer header
				if (pos_line != 0)
					continue;

				// - complete
				auto buf_body = alloc_shared_buffer(this->m_chunked_body.size() + 1);
				if (this->m_chunked_body.empty() == false)
					buf_body.append(this->m_chunked_body.size(), this->m_chunked_body.data());

				m_msg_http_request.contents.clear();
				m_msg_http_request.contents.emplace_back(std::move(buf_body));
				m_msg_http_request.buf_message = m_msg_http_request.contents[0];
				m_msg_http_request.content_completed = true;
				m_msg_http_request.is_completed = true;

				this->_reset_chunk();
				break;
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
	if(_string.empty())
		return temp_path_item;

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
			std::snprintf(temp, std::size(temp), "%02x", ch);
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
	if(_url.empty())
		return;

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
			if (remained < 3)
				throw throwable::failure(eRESULT::LENGTH_ERROR);

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
	
	// 3) request line (첫 번째 줄)
	{
		// - 첫 번째 줄을 얻는다.
		auto temp_str_first = find_paragraph(temp_str_pos);

		// check) 없으면 exception을 던진다.
		if (temp_str_first.empty())
			throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);

		// - method를 확인한다. (첫 공백까지를 표와 비교한다. 짧은 요청줄도 범위를 넘어 읽지 않는다.)
		{
			struct sMETHOD
			{
				std::string_view	name;
				eHTTP_METHOD		method;
			};
			static constexpr sMETHOD METHODS[] =
			{
				{ "GET"sv,		eHTTP_METHOD::GET_ },
				{ "POST"sv,		eHTTP_METHOD::POST_ },
				{ "PUT"sv,		eHTTP_METHOD::PUT_ },
				{ "PURGE"sv,	eHTTP_METHOD::PURGE_ },
				{ "PATCH"sv,	eHTTP_METHOD::PATCH_ },
				{ "PROPFIND"sv,	eHTTP_METHOD::PROPFIND_ },
				{ "COPY"sv,		eHTTP_METHOD::COPY_ },
				{ "DELETE"sv,	eHTTP_METHOD::DELETE_ },
				{ "HEAD"sv,		eHTTP_METHOD::HEAD_ },
				{ "OPTIONS"sv,	eHTTP_METHOD::OPTIONS_ },
				{ "LINK"sv,		eHTTP_METHOD::LINK_ },
				{ "LOCK"sv,		eHTTP_METHOD::LOCK_ },
				{ "UNLINK"sv,	eHTTP_METHOD::UNLINK_ },
				{ "UNLOCK"sv,	eHTTP_METHOD::UNLOCK_ },
				{ "VIEW"sv,		eHTTP_METHOD::VIEW_ },
				{ "TRACE"sv,	eHTTP_METHOD::TRACE_ },
			};

			// check) method 다음에 공백이 있어야 한다.
			const auto pos_space = temp_str_first.find(' ');
			if (pos_space == std::string_view::npos)
				throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);

			const auto str_method = temp_str_first.substr(0, pos_space);
			const auto iter_find = std::find_if(std::begin(METHODS), std::end(METHODS), [&](const sMETHOD& _item) { return _item.name == str_method; });

			// check)
			if (iter_find == std::end(METHODS))
				throw throwable::failure(eRESULT::FAIL, 400, "unknowned method"sv);

			m_msg_http_request.method = iter_find->method;
			temp_str_first.remove_prefix(pos_space + 1);
		}

		// - trim (remove blank)
		temp_str_first.remove_prefix(std::min(temp_str_first.find_first_not_of(" "sv), temp_str_first.size()));

		// - path를 얻는다.
		auto result_path = find_tok(temp_str_first, ' ');
		m_msg_http_request.path = std::u8string_view(reinterpret_cast<const char8_t*>(result_path.data()), result_path.size());

		// check) Path가 없으면 exception을 던진다.
		if (m_msg_http_request.path.empty())
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid path"sv);

		// check) Path가아 니면 Exception을 던진다.(path는 무조건 '/'나 '*'로 시작한다.)
		if (m_msg_http_request.path[0] != '/' && m_msg_http_request.path[0] != '*')
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "Invalid path"sv);

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
		bool is_content_length = false;

		// - trims the spaces and tabs (OWS)
		auto f_trim = [](std::string_view _value) -> std::string_view
		{
			while (_value.empty() == false && (_value.front() == ' ' || _value.front() == '\t'))
				_value.remove_prefix(1);
			while (_value.empty() == false && (_value.back() == ' ' || _value.back() == '\t'))
				_value.remove_suffix(1);
			return _value;
		};

		while ((temp_str_now = find_paragraph(temp_str_pos)).empty() == false)
		{
			// declare) 
			sHTTP_HEADER_ITEM temp_param;

			// - ...
			temp_param.key = find_tok(temp_str_now, ':');
			temp_param.value = temp_str_now;

			// - "Content-Length"인지 확인한다. (header 이름은 대소문자를 구분하지 않는다. 크기 제한)
			if (__icompare(temp_param.key, "Content-Length"sv))
			{
				// - digits only (an overflow or garbage is '400 Bad Request', not 0. the same value if repeated)
				const auto value = f_trim(temp_param.value);
				std::size_t content_length = 0;
				const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), content_length);
				if (value.empty() || ec != std::errc() || ptr != value.data() + value.size())
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "invalid content-length"sv);
				if (is_content_length && content_length != m_msg_http_request.content_length_now)
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "different content-lengths"sv);

				if (this->m_max_body_size != 0 && content_length > this->m_max_body_size)
					this->_throw_too_large("http body"sv, content_length, this->m_max_body_size);

				is_content_length = true;
				m_msg_http_request.content_length_now = content_length;
			}
			// - "Transfer-Encoding" (case insensitive) : the last coding must be "chunked" (RFC 9112 6.3 : otherwise '400 Bad Request')
			else if (__icompare(temp_param.key, "Transfer-Encoding"sv))
			{
				auto value = temp_param.value;
				if (const auto pos = value.rfind(','); pos != std::string_view::npos)
					value = value.substr(pos + 1);

				if (__icompare(f_trim(value), "chunked"sv) == false)
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, 400, "unsupported transfer-encoding"sv);

				m_msg_http_request.is_chunked = true;
			}

			// - 추가한다.
			m_msg_http_request.headers.push_back(temp_param);
		}

		// - "Transfer-Encoding" overrides "Content-Length" (RFC 9112 6.3. otherwise the body would be read both ways)
		if (m_msg_http_request.is_chunked)
			m_msg_http_request.content_length_now = 0;
	}

	// 5) buf_head를 설정한다.
	m_msg_http_request.buf_head = _buffer;
}

result_code net::io::Nhttp_server::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code result = eRESULT::BYPASS;

	// 1) message function
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


