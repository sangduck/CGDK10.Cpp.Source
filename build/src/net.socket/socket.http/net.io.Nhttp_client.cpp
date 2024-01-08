#include "pch.h"

using namespace CGDK;


bool net::io::Nhttp_client::send(buffer_view _buffer, eHTTP_METHOD _method)
{
	// check) websocket only
	CGASSERT_ERROR(is_websocket() == true);

	// check) websocket only
	RETURN_IF(is_websocket() == false, false);

	// 1) message 
	sWEBSOCKET_MESSAGE message_websocket(_buffer, _method);

	// 2) alloc shared_buffer
	auto buf_send = alloc_shared_buffer(get_size_of(message_websocket));

	// 3) append message
	buf_send.append(message_websocket);

	// 4) send
	return process_send(std::move(buf_send), 1, 0);
}

bool net::io::Nhttp_client::send(const std::string_view _string)
{
	// check) websocket only
	CGASSERT_ERROR(is_websocket() == true);

	// check) websocket only
	RETURN_IF(is_websocket() == false, false);

	// 1) message 
	sWEBSOCKET_MESSAGE message_websocket(_string);

	// 2) alloc shared_buffer
	auto buf_send = alloc_shared_buffer(get_size_of(message_websocket));

	// 3) append message
	buf_send.append(message_websocket);

	// 4) send
	return process_send(std::move(buf_send), 1, 0);
}

result_code net::io::Nhttp_client::on_message(sMESSAGE_HTTP_RESPONSE& /*_msg*/)
{
	// 1) ...
	auto prequest = m_prequest;

	// check) throw exception if request objects is not exist
	RETURN_IF(prequest.empty(), eRESULT::BYPASS)

	// 2) set psource as this
	m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

	// 4) send response message if Exist
	auto result = prequest->process_response(m_msg_http_response);

	// 5) close socket if completed
	if (m_msg_http_response.is_completed)
	{
		// - close socket
		closesocket();

		// - reset 
		m_prequest.reset();
	}

	// return)
	return result;
}

result_code net::io::Nhttp_client::on_message(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/)
{
	// 1) ...
	auto prequest = m_prequest;

	// check) throw exception if request objects is not exist
	RETURN_IF(prequest.empty(), 0)

	// 2) set psource as this
	m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

	// 3) send response message if Exist
	auto result = prequest->process_response(m_msg_http_response);

	// 4) close socket if completed
	if (m_msg_http_response.is_completed)
	{
		// - close socket
		closesocket();

		// - reset 
		m_prequest.reset();
	}

	// return)
	return result;
}

size_t net::io::Nhttp_client::process_packet(shared_buffer& _buffer, const net::sockaddr& _socket_address)
{
	// check) _buffer.data_가 0이어서는 안된다.
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_DEBUG << "(err ) _buffer.data_ is nullptr. _buffer.size_: "sv << _buffer.size() << " " __CGFUNCTION__ ""sv)

	// check) 최하 4Byte보다 작을 경우 그냥 끝낸다.
	RETURN_IF(_buffer.size() < 4, 0);

	// 1) message 처리
	switch (m_msg_http_response.protocol)
	{
	case	eHTTP_PROTOCOL::WEBSOCKET_:
			process_packet_websocket(_buffer, _socket_address);
			break;

	default:
			process_packet_http(_buffer, _socket_address);
			break;
	}

	// return) 
	return	8192;
}


bool check_web_socket_key(const std::string_view _compare, const std::string_view _source);

size_t net::io::Nhttp_client::process_packet_http( shared_buffer& _buffer, const net::sockaddr& /*_psocket_address*/)
{
	// 1) header를 parsing한다.
	try
	{
		// - http메시지 Parsin을 진행한다.
		auto size_process = process_parsing_http(_buffer);

		// - 진행된 만큼 buffer에서 빼낸다.
		_buffer += offset(size_process);
	}
	catch (throwable::failure & _e)
	{
		// - set  result
		m_msg_http_response.is_completed = true;
		m_msg_http_response.status_code = static_cast<eHTTP_STATUS_CODE>(_e.m_param);
		m_msg_http_response.status_message = to_string<char>(m_msg_http_response.status_code);
	}
	catch (...)
	{
		// - set  result
		m_msg_http_response.is_completed = true;
		m_msg_http_response.status_code = eHTTP_STATUS_CODE::UNKNOWN_ERROR_;
		m_msg_http_response.status_message = to_string<char>(m_msg_http_response.status_code);
	}

	// 2) completed되면 여기서 끝낸다.
	if (m_msg_http_response.is_completed)
	{
		// - web socket이면...
		if (is_certified() == false)
		{
			if (m_msg_http_response.get_header("Upgrade"sv) == "websocket"sv)
			{
				// check) 
				THROW_IF(m_msg_http_response.status_code != eHTTP_STATUS_CODE::SWITCHING_PROTOCOLS_, throwable::failure(eRESULT::FAIL, static_cast<intptr_t>(m_msg_http_response.status_code), std::string(m_msg_http_response.status_message)));

				// - WebSocket-Key를 얻는다.
				auto str_web_socket_key = m_msg_http_response.get_header("Sec-WebSocket-Accept"sv);

				// check) 
				CGASSERT_ERROR(str_web_socket_key.size() == 28);

				// check) throw exception if 'Sec-WebSocket-Accept'
				THROW_IF(str_web_socket_key.size() != 28, throwable::failure(eRESULT::FAIL_NO_WEBSOCKET_KEY))

				// - get 'Sec-WebSocket-Accept' value and check valication
				auto is_success = check_web_socket_key(str_web_socket_key, m_prequest->m_keys);

				// check) 
				THROW_IF(is_success == false, throwable::failure(eRESULT::FAIL, 0, "invalid websocket key"sv));

				// - set as 'websocket'
				m_msg_http_response.protocol = eHTTP_PROTOCOL::WEBSOCKET_;

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
		process_pre_message(m_msg_http_response);

		// - message를 Imessageable로 전달하여 처리한다.
		process_message(m_msg_http_response);

		// statistics) 
		statistics_on_receive_message();

		// - reset message
		m_msg_http_response.reset();

		// return)
		return 0;
	}

	// 3) chunked면 추가 처리해 준다.
	if (m_msg_http_response.is_chunked)
	{
		while (m_msg_http_response.content_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			process_pre_message(m_msg_http_response);

			// - message를 Imessageable로 전달하여 처리한다.
			process_message(m_msg_http_response);

			// - m_msg_http_response를 reset
			m_msg_http_response.content_completed = false;

			// - 다시 parsing 처리 함.
			auto size_process = process_parsing_http(_buffer);

			// - 처리된 만큼 뺀다.
			_buffer += offset(size_process);
		}

		// - complete면...
		if (m_msg_http_response.is_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			process_pre_message(m_msg_http_response);

			// - message를 Imessageable로 전달하여 처리한다.
			process_message(m_msg_http_response);

			// statistics) 
			statistics_on_receive_message();

			// - reset message
			m_msg_http_response.reset();
		}
	}

	// return) 
	return m_msg_http_response.buf_message.size();
}

char get_next_code_utf8(char*& _pnow, const char* _pend);

size_t net::io::Nhttp_client::process_packet_websocket( shared_buffer& _buffer, const net::sockaddr& /*_address*/)
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
			bool is_fin = (header & 0x0080)!=0;
			uint16_t Opcode = (header & 0x000f);

			// 4) message Length
			bool is_masked = (header & 0x8000) != 0;
			uint16_t length = (header & 0x7f00)>>8;

			// 5) length가 126일 경우 길이용 바이트로 2Byte 추가.
			if (length == 126)
			{
				// - 길이를 0으로 리셋
				length	 = 0;

				// - 2Byte 읽어 뒤집어 사용한다.
				for(int i=0;i<2; ++i)
				{
					length += static_cast<size_t>(buf_apply.extract<uint8_t>()) << (8 * (2 - 1 - i));
				}
			}
			// 6) length가 127일 경우 길이용 바이트로 8Byte 추가.
			else if(length==127)
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
			if(is_masked)
			{
				// check) length보다 길이가 작으면 여기서 끝낸다.
				//        주의!!) 이 BREAK문은 반드시 여기서 BREAK를 해야한다. 이유는 중복 Masking 처리를 방지하기 위해서이다.
				BREAK_IF(buf_apply.size() - sizeof(uint32_t) < length);

				// declare) 
				uint8_t	mask[4];
		
				// - Mask를 읽어낸다
				*(uint32_t*)mask = buf_apply.extract<uint32_t>();

				// - Read/ReadEnd...
				auto read_pos_now = (char*)buf_apply.data();
				auto read_pos_end = (char*)buf_apply.data() + length;

				// - Mask값을 XOR처리한다.
				for (int c=0; read_pos_now != read_pos_end; ++read_pos_now, ++c)
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
			auto	buf_payload = buffer_view(buf_apply.data(), length);

			// 9) 길이를 진행
			buf_apply	+= offset(length);
			buf_recv	 = buf_apply;

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
						msg.buf_message = _buffer ^ buf_payload;

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
						msg.buf_message = _buffer ^ buf_payload;

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
						header	 = 1;
					}
					break;

			// Default) ...
			default:
					{
						header	 = 2;
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
	return		0;
}


std::string_view find_paragraph(std::string_view& _string)
{
	// declare)
	std::string_view str_result;

	// 1) ...
	const char*	pos_begin = _string.data();
	const char*	pos_end = pos_begin + _string.size() - 1;
	const char* pos_now = pos_begin;

	// 2) _tok를 찾는다.
	for(;;)
	{
		// check) 문장의 끝을 찾지 못했으면 빈 string을 리턴한다.
		if (pos_now >= pos_end)
		{
			// break)
			break;
		}
			
		// check) 문장이 끝났다면...
		if (*pos_now == '\r' && *(pos_now + 1) == '\n')
		{
			// - 남은 부분을  _string에 적용한다.
			_string = std::string_view(pos_now + 2, pos_end - pos_now - 1);

			// - 리턴할 찾은 문장을 저장한다.
			str_result = std::string_view(pos_begin, pos_now - pos_begin);

			// break)
			break;
		}

		++pos_now;
	}

	// return) 
	return str_result;
}

std::string_view find_tok(std::string_view& _string, char _tok)
{
	// 1) ...
	const char*	pos_begin = _string.data();
	const char*	pos_end = pos_begin + _string.size();
	const char* pos_now = pos_begin;

	// 2) _tok를 찾는다.
	while(pos_now < pos_end && *pos_now != _tok)
	{
		++pos_now;
	}

	// 3) ...
	std::string_view str_result(pos_begin, pos_now - pos_begin);

	// 4) trim
	if (pos_now < pos_end)
	{
		for (++pos_now; pos_now < pos_end && *pos_now == ' '; ++pos_now)
		{
		}
	}

	// 4) _string에 적용한다.
	_string = std::string_view(pos_now, pos_end - pos_now);

	// return) 
	return	str_result;
}

uint32_t string_to_uint_hex(const std::string_view _string)
{
	// declare) 
	uint32_t value = 0;

	for(auto& iter: _string)
	{
		// - shift
		value *= 16;

		// - 0 ~ 9
		if (iter >= '0' && iter <= '9')
		{
			value += (iter - '0');
		}
		// - a ~ f
		else if (iter >= 'a' && iter <= 'f')
		{
			value += (iter - 'a' + 10);
		}
		// - A ~ F
		else if (iter >= 'A' && iter <= 'F')
		{
			value += (iter - 'A' + 10);
		}
	}

	// return)
	return value;
}

char* str_find_terminal(char* _str_begin, char* _str_end) noexcept
{
	// declare) 
	const uint32_t TERMINAL_STRING = (((uint32_t)'\n') << 24) | (((uint32_t)'\r') << 16) | (((uint32_t)'\n') << 8) | ((uint32_t)'\r');

	while (_str_begin < _str_end)
	{
		BREAK_IF(*(uint32_t*)_str_begin == TERMINAL_STRING);

		++_str_begin;
	}

	return	_str_begin;
}

size_t net::io::Nhttp_client::process_parsing_http(shared_buffer _buffer)
{
	// declare) 
	size_t processed = 0;

	// check)
	RETURN_IF(_buffer.size() <= 4, processed);

	// declare) 
	char* pos_now = _buffer.data();
	char* pos_end = _buffer.data() + _buffer.size();

	// 1) http (non-chunk)
	if(m_msg_http_response.is_chunked == false)
	{
		// - http_request head
		if(m_msg_http_response.content_length_now == 0)
		{
			// - 처리된 만큼을 건너띈다.
			pos_now += m_msg_http_response.buf_head.size();

			// - TERINAL_STRING(\n\r\n\r)을 찾는다.
			pos_now = str_find_terminal(pos_now, (pos_end - 3));

			// check) 해당 문자열을 찾지 못했으므로 끝낸다.
			if (pos_now == (pos_end - 3))
			{
				// - process를 저장한다.
				m_msg_http_response.buf_head.set_size( pos_now - _buffer.data() );

				// return) 0
				return	processed;
			}

			// - TERMINAML_STRING만큼을 이동...
			pos_now += 4;

			// - process를 저장한다.
			processed = pos_now - _buffer.data();

			// - http_resquest_head를 저장한다. (길이 +2는 마지막 \n\r\n\r을 넣기 위해서다.)
			m_msg_http_response.buf_head = _buffer ^ (pos_now - _buffer.data());

			// - Parsing [http_head]
			process_parsing_http_head(m_msg_http_response.buf_head);

			// - context가 없으면 여기서 종료한다.
			if (m_msg_http_response.is_chunked == false && m_msg_http_response.content_length_now == 0)
			{
				// - buf_message를 써넣는다. 
				m_msg_http_response.buf_message = _buffer;

				// - complete~
				m_msg_http_response.is_completed = true;
			}
		}

		// - http_request context
		if (m_msg_http_response.content_length_now != 0)
		{
			// - 남은 길이를 얻는다.
			size_t length = pos_end - pos_now;

			// - 남은 길이가 buf_content.size_보다 크면 끝낸다.
			if ((m_msg_http_response.content_length_now) <= length)
			{
				// check)
				CGASSERT_ERROR(m_msg_http_response.contents.empty())

				// - shared_buffer를 설정한다.
				m_msg_http_response.contents.emplace_back(_buffer ^ buffer_view{ pos_now, m_msg_http_response.content_length_now });
				m_msg_http_response.buf_message = m_msg_http_response.contents[0];

				// - completed를 증가시킨다.
				processed += m_msg_http_response.content_length_now;
				m_msg_http_response.content_completed = true;

				// - complete~
				m_msg_http_response.is_completed = true;
			}
		}
	}

	// 2) chunk
	if(m_msg_http_response.is_chunked == true)
	{
		// - 얻기...
		std::string_view temp_str_pos = std::string_view(pos_now, static_cast<uint32_t>(pos_end - pos_now));

		// - 첫줄을 읽는다.
		if (m_msg_http_response.content_length_now == 0)
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
					m_msg_http_response.content_length_now = chunk_length;

					// - 진행 처리...
					processed += (temp_str_pos.data() - temp_str_first.data());
				}
				// - 첫줄이 '0'이면 => 종료.
				else
				{
					// - clear
					m_msg_http_response.content_length_now = 0;

					// - complete~
					m_msg_http_response.is_completed = true;
				}
			}
		}

		// - chunk를 읽는다.
		if(m_msg_http_response.content_length_now != 0)
		{
			// - shared_buffer를 설정한다.
			if((m_msg_http_response.content_length_now) <= temp_str_pos.size())
			{
				// - append exist data
				m_msg_http_response.contents[0].reserve(m_msg_http_response.contents[0].size() + m_msg_http_response.content_length_now);

				// - ...
				auto ptr_old = m_msg_http_response.contents[0].get_back_ptr();

				// - 설정한다.
				m_msg_http_response.contents[0] += buffer_view { const_cast<char*>(temp_str_pos.data()), m_msg_http_response.content_length_now };
				m_msg_http_response.buf_message  = m_msg_http_response.contents[0] ^ buffer_view{ ptr_old , m_msg_http_response.content_length_now };
				m_msg_http_response.content_completed = true;

				// - 진행 처리...
				processed += m_msg_http_response.content_length_now;
			}
		}
	}

	// return)
	return	processed;
}

void net::io::Nhttp_client::process_parsing_http_head(shared_buffer _buffer)
{
	// check)
	CGASSERT_ERROR(m_msg_http_response.headers.empty());
	CGASSERT_ERROR(m_msg_http_response.contents.empty());
	CGASSERT_ERROR(m_msg_http_response.status_code == eHTTP_STATUS_CODE::NOT_DEFINED_);
	CGASSERT_ERROR(m_msg_http_response.status_message.empty());

	// 1) decoding처리를 한다.

	// 2) parsing string
	std::string_view temp_str_pos = _buffer;

	// 3) status line (첫번째 줄, HTTP1.1 400 Bad request)
	{
		// - 첫번째 줄을 얻는다.
		auto temp_str_first = find_paragraph(temp_str_pos);

		// check) 없으면 Exception을 던진다.
		THROW_IF(temp_str_first.empty(), throwable::failure(eRESULT::FAIL));

		// - result code를 얻는다.
		{
			// - HTTP/1.1 
			auto temp_str_http = find_tok(temp_str_first, ' ');

			// check) HTTP/1.1 을 얻는다.
			if((temp_str_http[0] != 'H' && temp_str_http[0] != 'h')
			|| (temp_str_http[1] != 'T' && temp_str_http[0] != 't')
			|| (temp_str_http[2] != 'T' && temp_str_http[0] != 't')
			|| (temp_str_http[3] != 'P' && temp_str_http[0] != 'p'))
			{
				throw throwable::failure(eRESULT::FAIL);
			}
		}

		// - result code를 얻는다.
		{
			auto str_version = find_tok(temp_str_first, ' ');

			// check) version이 없다면 Exception을 던진다.
			THROW_IF(str_version.empty(), throwable::failure(eRESULT::FAIL));

			// - version 정보를 parsing한다.
			m_msg_http_response.status_code = static_cast<eHTTP_STATUS_CODE>(__from_string<uint32_t>(str_version));
		}

		// - result message를 얻는다.
		m_msg_http_response.status_message = temp_str_first;
	}

	// 4) response heasers (둘째줄부터~끝)
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

			// -trim
			temp_param.value.remove_prefix(std::min(temp_param.value.find_first_not_of(" "),temp_param.value.size()));

			// - "Content-Length"인지 확인한다.
			if (temp_param.key == "Content-Length")
			{
				m_msg_http_response.content_length_now = __from_string<uint32_t>(temp_param.value);
			}
			else if (temp_param.key == "Content-Type")
			{
				m_msg_http_response.content_type = temp_param.value;
			}
			else if (temp_param.key == "Transfer-Encoding")
			{
				if(temp_param.value == "chunked")
				{
					m_msg_http_response.is_chunked = true;
				}
			}

			// - 추가한다.
			m_msg_http_response.headers.push_back(temp_param);
		}
	}

	// 5) buf_head를 설정한다.
	m_msg_http_response.buf_head = _buffer;
}

result_code net::io::Nhttp_client::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code	result = eRESULT::BYPASS;

	// 1) message fuction
	if ((_msg.message & eMESSAGE::SYSTEM::NETWORK_MASK) == eMESSAGE::SYSTEM::NETWORK)
	{
		switch (_msg.message)
		{
		case	eMESSAGE::SYSTEM::HTTP_REQUEST:
				CGASSERT_ERROR(false);
				break;

		case	eMESSAGE::SYSTEM::HTTP_RESPONSE:
				result = on_message(static_cast<sMESSAGE_HTTP_RESPONSE&>(_msg));
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

void net::io::Nhttp_client::process_on_connect()
{
	// 1) ...
	auto prequest = m_prequest;

	try
	{
		// check) 
		CGASSERT_ERROR(prequest.exist());

		// check)
		THROW_IF(prequest.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

		// 2) send Request
		auto result = process_send(shared_buffer(prequest->m_buffer_request), 1, 0);

		// check) ...
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
	}
	catch (...)
	{
		// - socket의 psocket을 this로 설정한다.
		m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

		// - set as completed
		m_msg_http_response.is_completed = true;

		// - send response message if Exist
		prequest->process_response(m_msg_http_response);

		// - request를 reset한다.
		reset_request();

		// - close socket
		process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
	}
}

void net::io::Nhttp_client::process_on_fail_connect(uint64_t /*_disconnect_reason*/)
{
	// 1) ...
	auto prequest = move_request();

	// check) throw exception if request objects is not exist
	RETURN_IF(prequest.empty())

	// 1) socket의 psocket을 this로 설정한다.
	m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

	// 2) set as completed
	m_msg_http_response.is_completed = true;
	m_msg_http_response.status_code = eHTTP_STATUS_CODE::NOT_FOUND_;
	m_msg_http_response.status_message = to_string<char>(m_msg_http_response.status_code);

	// 3) send response message if Exist
	prequest->process_response(m_msg_http_response);
}

void net::io::Nhttp_client::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	// 1) ...
	auto prequest = move_request();

	// check) throw exception if request objects is not exist
	RETURN_IF(prequest.empty())

	// 2) set source
	m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

	// 3) send Response message if Exist
	if (m_msg_http_response.is_completed == false)
	{
		// - 완료로 처리
		m_msg_http_response.is_completed = true;
		m_msg_http_response.status_code = eHTTP_STATUS_CODE::NOT_DEFINED_;
		m_msg_http_response.status_message = to_string<char>(eHTTP_STATUS_CODE::NOT_DEFINED_);

		// - 완료 처리
		prequest->process_response(m_msg_http_response);
	}

	// 4) call on disconnect
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
	{
		on_disconnect(_disconnect_reason);
	}
}
