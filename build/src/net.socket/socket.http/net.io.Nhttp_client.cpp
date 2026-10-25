#include "pch.h"
#include <charconv>
#include <random>
#include <bit>
#include <cstring>
#include <limits>
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

using namespace CGDK;

void apply_websocket_mask(char* _data, std::size_t _size, const uint8_t* _mask) noexcept;


net::io::Nhttp_client::Nhttp_client()
{
	// - 응답 대기 시간 (기본 60초)
	this->time_out_receive(std::chrono::seconds(60));

	// - 접속 시간 제한 (기본 10초 : Nsocket_sustain이 DISCONNECT_REASON_SUSTAIN_TIMEOUT으로 끊는다 -> '504 Gateway Timeout')
	this->time_out_connecting(std::chrono::seconds(10));
}

void net::io::Nhttp_client::on_timeout_receive(chrono::tick::duration /*_tick_differ*/)
{
	// check) websocket은 받을 것이 없을 수 있다.
	if (this->is_websocket())
		return;

	// check) 기다리는 요청이 없다.
	{
		std::lock_guard cs(this->m_lock_prequest);
		if (this->m_prequest.empty())
			return;
	}

	// - 끊는다. (process_on_disconnect에서 DISCONNECT_REASON_SUSTAIN_TIMEOUT을 보고 '504 Gateway Timeout'으로 끝낸다.)
	this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_SUSTAIN_TIMEOUT);
}

bool net::io::Nhttp_client::send(buffer_view _buffer, eHTTP_METHOD _method)
{
	// check) websocket only
	CGASSERT_ERROR(is_websocket() == true);

	// check) websocket only
	if(is_websocket() == false)
		return false;

	// 1) message 
	sWEBSOCKET_MESSAGE message_websocket(_buffer, _method);

	// 2) alloc shared_buffer
	auto buf_send = alloc_shared_buffer(get_size_of(message_websocket));

	// 3) append message
	buf_send.append(message_websocket);

	// 4) send
	return this->process_send(std::move(buf_send), 1, 0);
}

bool net::io::Nhttp_client::send(const std::string_view _string)
{
	// check) websocket only
	CGASSERT_ERROR(this->is_websocket() == true);

	// check) websocket only
	if(this->is_websocket() == false)
		return false;

	// 1) message 
	sWEBSOCKET_MESSAGE message_websocket(_string);

	// 2) alloc shared_buffer
	auto buf_send = alloc_shared_buffer(get_size_of(message_websocket));

	// 3) append message
	buf_send.append(message_websocket);

	// 4) send
	return this->process_send(std::move(buf_send), 1, 0);
}

result_code net::io::Nhttp_client::on_message(sMESSAGE_HTTP_RESPONSE& _msg)
{
	// 1) the request (in the lock)
	//    - completed : taken out (the final response is delivered once : process_on_disconnect finds no request)
	//    - a chunk   : copied
	const bool is_completed = _msg.is_completed;
	auto prequest = is_completed ? this->move_request() : this->_get_request();

	// check) no request
	if(prequest.empty())
		return eRESULT::BYPASS;

	// 2) set psource as this
	_msg.set_source(static_cast<net::io::Iconnectable*>(this));

	// 3) send response message
	auto result = prequest->process_response(_msg);

	// 4) close socket if completed
	if (is_completed)
	{
		this->closesocket();
	}

	// return)
	return result;
}

result_code net::io::Nhttp_client::on_message(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/)
{
	// - a websocket frame isn't the response of the request (the derived class receives the frames)
	return 0;
}

size_t net::io::Nhttp_client::process_packet(shared_buffer& _buffer, const net::sockaddr& _socket_address)
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

	// 1) message 처리 (한 번 받은 데이터에 message가 여럿 있으면 모두 처리한다.)
	for (;;)
	{
		auto size_before = _buffer.size();
		auto protocol_before = this->m_msg_http_response.protocol;

		if (protocol_before == eHTTP_PROTOCOL::WEBSOCKET_)
			this->process_packet_websocket(_buffer, _socket_address);
		else
			this->process_packet_http(_buffer, _socket_address);

		// check) 더 처리한 것이 없다.
		BREAK_IF(_buffer.size() == 0 || (_buffer.size() == size_before && this->m_msg_http_response.protocol == protocol_before));

		// check) 접속을 끊는 중이다.
		auto state = this->socket_state();
		BREAK_IF(state >= eSOCKET_STATE::SEND_DISCONNECTED && state <= eSOCKET_STATE::LAST_ACK);
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
		// - http메시지 Parsing을 진행한다.
		auto size_process = this->process_parsing_http(_buffer);

		// - 진행된 만큼 buffer에서 빼낸다.
		_buffer += offset(size_process);
	}
	catch (throwable::failure & _e)
	{
		// - set  result
		this->m_msg_http_response.is_completed = true;
		this->m_msg_http_response.status_code = static_cast<eHTTP_STATUS_CODE>(_e.m_param);
		this->m_msg_http_response.status_message = to_string(m_msg_http_response.status_code);
		_buffer.clear();		// - 남은 데이터는 쓸 수 없다. (오류 응답 후 접속을 끊는다.)
	}
	catch (...)
	{
		// - set  result
		this->m_msg_http_response.is_completed = true;
		this->m_msg_http_response.status_code = eHTTP_STATUS_CODE::UNKNOWN_ERROR_;
		this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);
		_buffer.clear();		// - 남은 데이터는 쓸 수 없다. (오류 응답 후 접속을 끊는다.)
	}

	// 2) completed되면 여기서 끝낸다.
	if (this->m_msg_http_response.is_completed)
	{
		// - web socket이면...
		if (this->is_certified() == false)
		{
			// - "101 Switching Protocols" + "Upgrade: websocket" (case insensitive)
			//   (an error response to the upgrade request is a normal response : delivered and closed)
			if (this->m_msg_http_response.status_code == eHTTP_STATUS_CODE::SWITCHING_PROTOCOLS_
			 && __icompare(this->m_msg_http_response.get_header("Upgrade"sv), "websocket"sv))
			{
				// - a failed handshake : '502 Bad Gateway' (the request is completed by process_on_disconnect)
				auto f_fail_handshake = [this](eRESULT _result, std::string_view _message)
				{
					this->m_msg_http_response.status_code = eHTTP_STATUS_CODE::BAD_GATEWAY_;
					this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);
					throw throwable::failure(_result, 0, _message);
				};

				// - WebSocket-Key를 얻는다.
				auto str_web_socket_key = this->m_msg_http_response.get_header("Sec-WebSocket-Accept"sv);

				// check) throw exception if 'Sec-WebSocket-Accept'
				//        (받은 응답이므로 assert하지 않는다. 잘못된 응답은 예외 -> 접속 종료)
				if(str_web_socket_key.size() != 28)
					f_fail_handshake(eRESULT::FAIL_NO_WEBSOCKET_KEY, "no websocket key"sv);

				// - 요청이 없다. (요청하지 않은 응답)
				auto prequest = this->_get_request();
				if (prequest.empty())
					f_fail_handshake(eRESULT::FAIL, "websocket response without the request"sv);

				// - get 'Sec-WebSocket-Accept' value and check validation
				auto is_success = check_web_socket_key(str_web_socket_key, prequest->m_keys);

				// check)
				if(is_success == false)
					f_fail_handshake(eRESULT::FAIL, "invalid websocket key"sv);

				// - set as 'websocket'
				this->m_msg_http_response.protocol = eHTTP_PROTOCOL::WEBSOCKET_;

				try
				{
					// - set object_state (CAS : ESTABLISHED일 때만 CERTIFIED로 바꾸고 on_connect를 호출한다.
					//   그 사이 닫히는 중이면(오류 응답 후 disconnect 등) 상태를 덮어쓰지 않고 on_connect 없이 메시지만 전달한다)
					if (this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED))
						this->on_connect();
				}
				catch (...)
				{
					// - restore socket status (CAS : CERTIFIED일 때만)
					this->set_socket_state_if(eSOCKET_STATE::CERTIFIED, eSOCKET_STATE::ESTABLISHED);

					// reraise)
					throw;
				}

				// - the request is completed by the '101' response (the connection is kept for the frames)
				if (auto prequest_done = this->move_request(); prequest_done.exist())
				{
					this->m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));
					prequest_done->process_response(this->m_msg_http_response);
				}

				// - reset the message (the protocol is kept : websocket)
				this->m_msg_http_response.reset();
				this->m_msg_http_response.protocol = eHTTP_PROTOCOL::WEBSOCKET_;

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
						this->on_connect();
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
		this->process_pre_message(this->m_msg_http_response);

		// - message를 Imessageable로 전달하여 처리한다.
		this->process_message(this->m_msg_http_response);

		// statistics) 
		this->statistics_on_receive_message();

		// - reset message
		this->m_msg_http_response.reset();
		this->_reset_chunk();

		// return)
		return 0;
	}

	// 3) chunked : chunk마다 전달한다. (is_completed == false. buf_message: 그 chunk)
	//    마지막 chunk('0')까지 오면 전체 body를 전달한다. (is_completed == true)
	if (this->m_msg_http_response.is_chunked)
	{
		// - 첫 응답이면 접속 완료 처리를 한다. (on_connect)
		if (this->m_msg_http_response.content_completed && this->is_certified() == false)
		{
			try
			{
				// - set object_state (CAS : ESTABLISHED일 때만 CERTIFIED로 바꾸고 on_connect를 호출한다.
				//   그 사이 닫히는 중이면(오류 응답 후 disconnect 등) 상태를 덮어쓰지 않고 on_connect 없이 메시지만 전달한다)
				if (this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::CERTIFIED))
					this->on_connect();
			}
			catch (...)
			{
				// - restore socket status (CAS : CERTIFIED일 때만)
				this->set_socket_state_if(eSOCKET_STATE::CERTIFIED, eSOCKET_STATE::ESTABLISHED);
				throw;
			}
		}

		while (this->m_msg_http_response.content_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			this->process_pre_message(this->m_msg_http_response);

			// - message를 Imessageable로 전달하여 처리한다.
			this->process_message(this->m_msg_http_response);

			// - 다음 chunk (오류는 첫 parsing과 같이 오류 응답으로 바꾼다.)
			this->m_msg_http_response.content_completed = false;

			try
			{
				auto size_process = this->process_parsing_http(_buffer);
				_buffer += offset(size_process);
			}
			catch (throwable::failure& _e)
			{
				this->m_msg_http_response.is_completed = true;
				this->m_msg_http_response.status_code = static_cast<eHTTP_STATUS_CODE>(_e.m_param);
				this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);
				_buffer.clear();		// - 남은 데이터는 쓸 수 없다. (오류 응답 후 접속을 끊는다.)
			}
			catch (...)
			{
				this->m_msg_http_response.is_completed = true;
				this->m_msg_http_response.status_code = eHTTP_STATUS_CODE::UNKNOWN_ERROR_;
				this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);
				_buffer.clear();		// - 남은 데이터는 쓸 수 없다. (오류 응답 후 접속을 끊는다.)
			}
		}

		// - complete면...
		if (this->m_msg_http_response.is_completed)
		{
			// - 훅함수 process_pre_message()를 호출한다.
			this->process_pre_message(this->m_msg_http_response);

			// - message를 Imessageable로 전달하여 처리한다.
			this->process_message(this->m_msg_http_response);

			// statistics)
			this->statistics_on_receive_message();

			// - reset message
			this->m_msg_http_response.reset();
			this->_reset_chunk();
		}
	}

	// return)
	return this->m_msg_http_response.buf_message.size();
}

char get_next_code_utf8(char*& _pnow, const char* _pend);

void net::io::Nhttp_client::_process_websocket_data(uint16_t _opcode, bool _is_fin, const shared_buffer& _buffer, const buffer_view& _payload)
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

			// 3) 첫 번째 메시지인가 아닌가.
			bool is_fin = (header & 0x0080)!=0;
			uint16_t opcode = (header & 0x000f);

			// 4) message Length (126: 2 byte 길이, 127: 8 byte 길이. 길이 byte가 아직 다 오지 않았으면 기다린다.)
			bool is_masked = (header & 0x8000) != 0;
			uint64_t length = (header & 0x7f00)>>8;

			BREAK_IF(length == 126 && buf_apply.size() < 2);
			BREAK_IF(length == 127 && buf_apply.size() < 8);

			// 5) length가 126일 경우 길이용 바이트로 2Byte 추가.
			if (length == 126)
			{
				// - 길이를 0으로 리셋
				length	 = 0;

				// - 2Byte 읽어 뒤집어 사용한다.
				for(int i=0;i<2; ++i)
				{
					length += static_cast<uint64_t>(buf_apply.extract<uint8_t>()) << (8 * (2 - 1 - i));
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
					length += static_cast<uint64_t>(buf_apply.extract<uint8_t>()) << (8 * (8 - 1 - i));
				}
			}

			// check) frame 크기 제한
			if (this->m_max_body_size != 0 && length > this->m_max_body_size)
				this->_throw_too_large("websocket frame"sv, static_cast<std::size_t>(length), this->m_max_body_size);

			// 7) Masked일 경우 Masked 처리를 한다.
			if(is_masked)
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
				apply_websocket_mask((char*)buf_apply.data(), static_cast<std::size_t>(length), mask);
			}
			else
			{
				// check) length보다 길이가 작으면 여기서 끝낸다.
				BREAK_IF(buf_apply.size() < length);
			}
		
			// 8) shared_buffer를 설정한다.
			auto buf_payload = buffer_view(buf_apply.data(), length);

			// 9) 길이를 진행
			buf_apply += offset(length);
			buf_recv = buf_apply;

			// 10) opcode 처리하기.
			//	*  0x0			Denotes a continuation frame
			//	*  0x1			Denotes a text frame
			//	*  0x2			Denotes a binary frame
			//	*  0x3 ~ 0x7	Reserved for further non - control frames
			//	*  0x8			Denotes a connection close
			//	*  0x9			Denotes a ping
			//	*  0xA			Denotes a pong
			//	*  0xB ~ 0xF	Reserved for further control frames
			switch(opcode)
			{
			// CASE) continue / Text / Binary (조각은 모아서 하나로 전달한다.)
			case	0:
			case	1:
			case	2:
					this->_process_websocket_data(opcode, is_fin, _buffer, buf_payload);
					break;

			// CASE) close
			case	8:
					{
						this->disconnect();
					}
					break;

			// CASE) PING 요청 시
			case	9:
					{
						// - Pong을 전송한다. (opcode 0x0a, Ping과 같은 payload)
						//   client가 보내는 frame은 반드시 mask해야 한다. (RFC 6455 5.3. 이 frame의 payload만 보낸다. control frame은 125 byte 이하)
						if (length <= 125)
						{
							std::random_device random_device;
							auto mask_value = static_cast<uint32_t>(random_device());
							uint8_t mask[4];
							memcpy(mask, &mask_value, 4);

							auto buf_pong = alloc_shared_buffer(6 + static_cast<std::size_t>(length));
							buf_pong.append<uint8_t>(0x8a);
							buf_pong.append<uint8_t>(static_cast<uint8_t>(0x80 | length));
							buf_pong.append(4, mask);
							for (std::size_t i = 0; i < static_cast<std::size_t>(length); ++i)
								buf_pong.append<uint8_t>(static_cast<uint8_t>(buf_payload.data<uint8_t>()[i] ^ mask[i % 4]));

							this->process_send(std::move(buf_pong), 1, 0);
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
		_buffer = buf_recv;
	}
	catch(...)
	{
		// - 처리된 메시지 버퍼를 업데이트한다.
		_buffer = buf_recv;

		// reraise)
		throw;
	}

	// return)
	return 0;
}


// - returns the line before the first "\r\n" and moves _string after it
//   (not found : returns empty and _string is not changed)
std::string_view find_paragraph(std::string_view& _string)
{
	const char* pos_now = _string.data();
	const char* pos_end = pos_now + _string.size();

	while (pos_now < pos_end)
	{
		// - next '\r' (memchr is vectorized by the CRT)
		pos_now = static_cast<const char*>(memchr(pos_now, '\r', static_cast<size_t>(pos_end - pos_now)));
		if (pos_now == nullptr || pos_now + 1 >= pos_end)
			break;

		if (pos_now[1] == '\n')
		{
			const std::string_view str_result(_string.data(), static_cast<size_t>(pos_now - _string.data()));
			_string = std::string_view(pos_now + 2, static_cast<size_t>(pos_end - pos_now - 2));
			return str_result;
		}

		++pos_now;
	}

	return std::string_view();
}
// - returns the part before _tok and moves _string after _tok and the following spaces
//   (no _tok : returns the whole and _string becomes empty)
std::string_view find_tok(std::string_view& _string, char _tok)
{
	const char* pos_begin = _string.data();
	const char* pos_end = pos_begin + _string.size();

	// 1) _tok
	const char* pos_now = _string.empty() ? nullptr : static_cast<const char*>(memchr(pos_begin, _tok, _string.size()));
	if (pos_now == nullptr)
		pos_now = pos_end;

	const std::string_view str_result(pos_begin, static_cast<size_t>(pos_now - pos_begin));

	// 2) skip _tok and spaces
	if (pos_now < pos_end)
	{
		for (++pos_now; pos_now < pos_end && *pos_now == ' '; ++pos_now)
		{
		}
	}

	_string = std::string_view(pos_now, static_cast<size_t>(pos_end - pos_now));

	return str_result;
}
//-----------------------------------------------------------------------------
// str_find_terminal
//
//  - returns the first position p (p < _str_end) where p[0..3] == "\r\n\r\n",
//    or _str_end if not found
//  - reads up to _str_end + 3 (the caller passes 'end - 3')
//  - AVX2 (x64, runtime detected) : 32 positions per iteration
//-----------------------------------------------------------------------------
#if defined(CGDK_SIMD_AVX2)
// return) found position or nullptr (_str_begin : advanced to the remained position)
CGDK_TARGET_AVX2 static char* _str_find_terminal_avx2(char*& _str_begin, char* _str_end) noexcept
{
	const __m256i cr = _mm256_set1_epi8('\r');
	const __m256i lf = _mm256_set1_epi8('\n');

	char* found = nullptr;

	// - positions p..p+31 : reads p..p+34 (< _str_end + 3)
	for (; _str_end - _str_begin >= 32; _str_begin += 32)
	{
		const __m256i m0 = _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_str_begin    )), cr);
		const __m256i m1 = _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_str_begin + 1)), lf);
		const __m256i m2 = _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_str_begin + 2)), cr);
		const __m256i m3 = _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_str_begin + 3)), lf);
		const auto mask = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_and_si256(_mm256_and_si256(m0, m1), _mm256_and_si256(m2, m3))));

		if (mask != 0)
		{
			found = _str_begin + std::countr_zero(mask);
			break;
		}
	}

	// - avoid AVX-SSE transition penalty
	_mm256_zeroupper();

	return found;
}
#endif

char* str_find_terminal(char* _str_begin, char* _str_end) noexcept
{
	// declare) "\r\n\r\n" in memory order
	uint32_t terminal;
	memcpy(&terminal, "\r\n\r\n", sizeof(terminal));

#if defined(CGDK_SIMD_AVX2)
	// 1) AVX2
	if (_str_end - _str_begin >= 32 && cpu::is_avx2_supported())
	{
		if (auto found = _str_find_terminal_avx2(_str_begin, _str_end); found != nullptr)
			return found;
	}
#endif

	// 2) remained
	for (; _str_begin < _str_end; ++_str_begin)
	{
		uint32_t value;
		memcpy(&value, _str_begin, sizeof(value));
		BREAK_IF(value == terminal);
	}

	return	_str_begin;
}

void net::io::Nhttp_client::_throw_too_large(std::string_view _what, std::size_t _size, std::size_t _max)
{
	// log)
	LOG_WARNING << "(warn) CGDK.net.socket.http: response "sv << _what << " is too large ("sv << _size << " > "sv << _max << " bytes). disconnect"sv;

	// - 접속을 끊는다. (남은 데이터는 쓸 수 없다.)
	this->disconnect();

	// - 응답은 413(Payload Too Large)으로 전달된다. (process_packet_http에서 오류 응답으로 바꾼다.)
	throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, static_cast<intptr_t>(eHTTP_STATUS_CODE::PAYLOAD_TOO_LARGE_), "too large"sv);
}

size_t net::io::Nhttp_client::process_parsing_http(shared_buffer _buffer)
{
	// declare) 
	size_t processed = 0;

	// check)
	if(_buffer.size() == 0)
		return processed;

	// 0) the body until the connection is closed (no Content-Length, not chunked : completed by process_on_disconnect)
	if (this->m_is_body_until_close)
	{
		if (this->m_max_body_size != 0 && this->m_body_until_close.size() + _buffer.size() > this->m_max_body_size)
			this->_throw_too_large("http body"sv, this->m_body_until_close.size() + _buffer.size(), this->m_max_body_size);

		this->m_body_until_close.append(_buffer.data(), _buffer.size());
		return _buffer.size();
	}

	// declare)
	char* pos_now = _buffer.data();
	char* pos_end = _buffer.data() + _buffer.size();

	// 1) http (non-chunk)
	if(this->m_msg_http_response.is_chunked == false)
	{
		// - http_request head
		if(this->m_msg_http_response.content_length_now == 0)
		{
			// - 처리된 만큼을 건너띈다.
			//   (head가 아직 다 오지 않았을 때 찾아본 위치. buf_head는 비어 있으므로 크기를 바꾸지 않는다.)
			pos_now += this->m_size_head_scanned;

			// check) head 끝(CRLF CRLF)을 찾을 수 있을 만큼 오지 않았다. (4 byte)
			if (pos_end - pos_now < 4)
				return processed;

			// - TERINAL_STRING(\n\r\n\r)을 찾는다.
			pos_now = str_find_terminal(pos_now, (pos_end - 3));

			// check) 해당 문자열을 찾지 못했으므로 끝낸다.
			if (pos_now == (pos_end - 3))
			{
				// check) header가 너무 길면 끊는다.
				if (this->m_max_header_size != 0 && static_cast<std::size_t>(pos_end - _buffer.data()) > this->m_max_header_size)
					this->_throw_too_large("http header"sv, static_cast<std::size_t>(pos_end - _buffer.data()), this->m_max_header_size);

				// - process를 저장한다.
				this->m_size_head_scanned = static_cast<std::size_t>(pos_now - _buffer.data());

				// return) 0
				return processed;
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
			this->m_msg_http_response.buf_head = _buffer ^ (pos_now - _buffer.data());

			// - Parsing [http_head]
			net::io::Nhttp_client::process_parsing_http_head(this->m_msg_http_response);

			// check) body 크기 제한 (Content-Length)
			if (this->m_max_body_size != 0 && this->m_msg_http_response.content_length_now > this->m_max_body_size)
				this->_throw_too_large("http body"sv, this->m_msg_http_response.content_length_now, this->m_max_body_size);

			// declare)
			const auto status = static_cast<uint32_t>(this->m_msg_http_response.status_code);

			// - 1xx (except '101 Switching Protocols') : an interim response. ignored (the final response follows)
			if (status >= 100 && status < 200 && status != 101)
			{
				this->m_msg_http_response.reset();
				return processed;
			}

			// - no body : the response of HEAD, 1xx, 204, 304 (RFC 9112 6.3)
			const bool is_no_body = (this->m_request_method == eHTTP_METHOD::HEAD_) || (status >= 100 && status < 200) || status == 204 || status == 304;

			// - "Content-Length" (ignored with "Transfer-Encoding")
			const bool is_content_length = this->m_msg_http_response.get_header("Content-Length"sv).empty() == false
										&& this->m_msg_http_response.get_header("Transfer-Encoding"sv).empty();

			if (is_no_body || (this->m_msg_http_response.is_chunked == false && is_content_length && this->m_msg_http_response.content_length_now == 0))
			{
				// - completed without the body
				this->m_msg_http_response.is_chunked = false;
				this->m_msg_http_response.content_length_now = 0;
				this->m_msg_http_response.buf_message = shared_buffer();
				this->m_msg_http_response.is_completed = true;
				return processed;
			}

			// - no "Content-Length" and not chunked : the body until the connection is closed
			if (this->m_msg_http_response.is_chunked == false && is_content_length == false)
			{
				this->m_is_body_until_close = true;

				const auto size_rest = static_cast<std::size_t>(pos_end - pos_now);
				if (this->m_max_body_size != 0 && size_rest > this->m_max_body_size)
					this->_throw_too_large("http body"sv, size_rest, this->m_max_body_size);

				this->m_body_until_close.assign(pos_now, size_rest);
				return processed + size_rest;
			}
		}

		// - http_request context
		if (this->m_msg_http_response.content_length_now != 0)
		{
			// - 남은 길이를 얻는다.
			size_t length = pos_end - pos_now;

			// - 남은 길이가 buf_content.size_보다 크면 끝낸다.
			if ((this->m_msg_http_response.content_length_now) <= length)
			{
				// check)
				CGASSERT_ERROR(this->m_msg_http_response.contents.empty())

				// - shared_buffer를 설정한다.
				this->m_msg_http_response.contents.emplace_back(_buffer ^ buffer_view{ pos_now, this->m_msg_http_response.content_length_now });
				this->m_msg_http_response.buf_message = this->m_msg_http_response.contents[0];

				// - completed를 증가시킨다.
				processed += this->m_msg_http_response.content_length_now;
				this->m_msg_http_response.content_completed = true;

				// - complete~
				this->m_msg_http_response.is_completed = true;
			}
		}
	}

	// 2) chunked body
	//    - size line(hex[;ext]) -> data + CRLF -> ... -> '0' -> trailer header들 -> 빈 줄
	//    - chunk 하나를 읽으면 content_completed(buf_message: 그 chunk)로 돌아간다. (process_packet_http가 전달하고 다시 부른다.)
	//    - 끝나면 is_completed(buf_message, contents[0]: 전체 body)
	if (this->m_msg_http_response.is_chunked == true && this->m_msg_http_response.is_completed == false && this->m_msg_http_response.content_completed == false)
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
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, static_cast<intptr_t>(eHTTP_STATUS_CODE::BAD_GATEWAY_), "invalid chunk size"sv);

				pos_now += pos_line + 2;
				processed = static_cast<size_t>(pos_now - _buffer.data());

				if (chunk_size == 0)
				{
					this->m_chunk_state = 2;
					continue;
				}

				// check) the size + CRLF must not overflow
				if (chunk_size > (std::numeric_limits<std::size_t>::max)() - 2 - this->m_chunked_body.size())
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, static_cast<intptr_t>(eHTTP_STATUS_CODE::BAD_GATEWAY_), "invalid chunk size"sv);

				if (this->m_max_body_size != 0 && this->m_chunked_body.size() + chunk_size > this->m_max_body_size)
					this->_throw_too_large("http chunked body"sv, this->m_chunked_body.size() + chunk_size, this->m_max_body_size);

				this->m_chunk_size = chunk_size;
				this->m_chunk_state = 1;
			}
			// - data + CRLF (chunk 하나를 전달한다.)
			else if (this->m_chunk_state == 1)
			{
				BREAK_IF(rest.size() < this->m_chunk_size + 2);

				// check) the data must be followed by CRLF
				if (rest[this->m_chunk_size] != '\r' || rest[this->m_chunk_size + 1] != '\n')
					throw throwable::failure(eRESULT::INVALID_ARGUMENT, static_cast<intptr_t>(eHTTP_STATUS_CODE::BAD_GATEWAY_), "invalid chunk data"sv);

				auto data = rest.substr(0, this->m_chunk_size);
				this->m_chunked_body.append(data);

				auto buf_chunk = alloc_shared_buffer(data.size() + 1);
				buf_chunk.append(data.size(), data.data());
				this->m_msg_http_response.buf_message = std::move(buf_chunk);
				this->m_msg_http_response.content_completed = true;

				pos_now += this->m_chunk_size + 2;
				processed = static_cast<size_t>(pos_now - _buffer.data());
				this->m_chunk_state = 0;
				break;
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

				// - complete (전체 body)
				auto buf_body = alloc_shared_buffer(this->m_chunked_body.size() + 1);
				if (this->m_chunked_body.empty() == false)
					buf_body.append(this->m_chunked_body.size(), this->m_chunked_body.data());

				this->m_msg_http_response.contents.clear();
				this->m_msg_http_response.contents.emplace_back(std::move(buf_body));
				this->m_msg_http_response.buf_message = this->m_msg_http_response.contents[0];
				this->m_msg_http_response.content_completed = false;
				this->m_msg_http_response.is_completed = true;

				this->_reset_chunk();
				break;
			}
		}
	}

	// return)
	return processed;
}

void net::io::Nhttp_client::process_parsing_http_head(sMESSAGE_HTTP_RESPONSE& _msg_http_response) // 
{
	// check)
	CGASSERT_ERROR(_msg_http_response.headers.empty());
	CGASSERT_ERROR(_msg_http_response.contents.empty());
	CGASSERT_ERROR(_msg_http_response.status_code == eHTTP_STATUS_CODE::NOT_DEFINED_);
	CGASSERT_ERROR(_msg_http_response.status_message.empty());

	// 1) decoding처리를 한다.
	shared_buffer buffer_http_response = _msg_http_response.buf_head;

	// 2) parsing string
	std::string_view temp_str_pos = buffer_http_response;

	// 3) status line (첫 번째 줄, HTTP1.1 400 Bad request)
	{
		// - 첫 번째 줄을 얻는다.
		auto temp_str_first = find_paragraph(temp_str_pos);

		// check) 없으면 Exception을 던진다.
		if(temp_str_first.empty())
			throw throwable::failure(eRESULT::FAIL);

		// - result code를 얻는다.
		{
			// - HTTP/1.1 
			auto temp_str_http = find_tok(temp_str_first, ' ');

			// check) HTTP/1.1 을 얻는다. (4자보다 짧을 수 있다. 대소문자 구분 없음)
			if (temp_str_http.size() < 4 || __icompare(temp_str_http.substr(0, 4), "HTTP"sv) == false)
			{
				throw throwable::failure(eRESULT::FAIL);
			}
		}

		// - result code를 얻는다.
		{
			auto str_version = find_tok(temp_str_first, ' ');

			// check) version이 없다면 Exception을 던진다.
			if(str_version.empty())
				throw throwable::failure(eRESULT::FAIL);

			// - version 정보를 parsing한다.
			uint32_t status_code = 0;
			std::from_chars(str_version.data(), str_version.data() + str_version.size(), status_code);
			_msg_http_response.status_code = static_cast<eHTTP_STATUS_CODE>(status_code);
		}

		// - result message를 얻는다.
		_msg_http_response.status_message = temp_str_first;
	}

	// 4) response headers (둘째줄부터~끝)
	{
		// declare)
		std::string_view temp_str_now;
		bool is_content_length = false;
		bool is_transfer_encoding = false;

		// - an invalid response : '502 Bad Gateway'
		auto f_invalid = [](std::string_view _message)
		{
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, static_cast<intptr_t>(eHTTP_STATUS_CODE::BAD_GATEWAY_), _message);
		};

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
			temp_param.value = f_trim(temp_str_now);

			// - "Content-Length" : digits only (the same value if repeated)
			if (__icompare(temp_param.key, "Content-Length"sv))
			{
				std::size_t length = 0;
				const auto [ptr, ec] = std::from_chars(temp_param.value.data(), temp_param.value.data() + temp_param.value.size(), length);
				if (temp_param.value.empty() || ec != std::errc() || ptr != temp_param.value.data() + temp_param.value.size())
					f_invalid("invalid content-length"sv);
				if (is_content_length && length != _msg_http_response.content_length_now)
					f_invalid("different content-lengths"sv);

				is_content_length = true;
				_msg_http_response.content_length_now = length;
			}
			else if (__icompare(temp_param.key, "Content-Type"sv))
			{
				_msg_http_response.content_type = temp_param.value;
			}
			// - "Transfer-Encoding" : chunked if the last coding is "chunked" ("gzip, chunked")
			else if (__icompare(temp_param.key, "Transfer-Encoding"sv))
			{
				is_transfer_encoding = true;

				auto value = temp_param.value;
				if (const auto pos = value.rfind(','); pos != std::string_view::npos)
					value = value.substr(pos + 1);

				_msg_http_response.is_chunked = __icompare(f_trim(value), "chunked"sv);
			}

			// - 추가한다.
			_msg_http_response.headers.push_back(temp_param);
		}

		// - "Transfer-Encoding" overrides "Content-Length" (RFC 9112 6.3)
		//   (not chunked : the body until the connection is closed)
		if (is_transfer_encoding)
			_msg_http_response.content_length_now = 0;
	}

	// 5) buf_head를 설정한다.
	_msg_http_response.buf_head = buffer_http_response;
}

result_code net::io::Nhttp_client::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code	result = eRESULT::BYPASS;

	// 1) message function
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
	// - reset
	this->m_is_body_until_close = false;
	this->m_body_until_close.clear();

	// 1) the request (in the lock)
	auto prequest = this->_get_request();

	// - the method of the request (the response of HEAD has no body)
	this->m_request_method = prequest.exist() ? prequest->m_http_method : eHTTP_METHOD::NONE_;

	try
	{
		// check)
		CGASSERT_ERROR(prequest.exist());

		// check)
		if(prequest.empty())
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);

		// 2) send Request
		auto result = process_send(shared_buffer(prequest->m_buffer_request), 1, 0);

		// check) ...
		if(result == false)
			throw throwable::failure(eRESULT::FAIL);
	}
	catch (...)
	{
		// - the request (taken out : completed once)
		if (auto prequest_fail = this->move_request(); prequest_fail.exist())
		{
			// - socket의 psocket을 this로 설정한다.
			this->m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

			// - set as completed ('503 Service Unavailable' : fail to send the request)
			this->m_msg_http_response.is_completed = true;
			this->m_msg_http_response.status_code = eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_;
			this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);

			// - send response message
			prequest_fail->process_response(this->m_msg_http_response);
		}

		// - close socket
		this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
	}
}

void net::io::Nhttp_client::process_on_fail_connect(uint64_t _disconnect_reason)
{
	// 1) ...
	auto prequest = move_request();

	// check) throw exception if request objects is not exist
	if(prequest.empty())
		return;

	// 1) socket의 psocket을 this로 설정한다.
	this->m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

	// 2) set as completed
	this->m_msg_http_response.is_completed = true;
	//    ('504 Gateway Timeout' : the connect timeout, '503 Service Unavailable' : fail to connect)
	this->m_msg_http_response.status_code = ((_disconnect_reason & DISCONNECT_REASON_MASK_USER) == DISCONNECT_REASON_SUSTAIN_TIMEOUT) ? eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_ : eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_;
	this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);

	// 3) send response message if Exist
	prequest->process_response(this->m_msg_http_response);
}

void net::io::Nhttp_client::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE _socket_state)
{
	// 1) the request not completed yet (a completed request was taken out by on_message)
	auto prequest = move_request();

	// 2) completes the request
	if (prequest.exist())
	{
		// - set source
		this->m_msg_http_response.set_source(static_cast<net::io::Iconnectable*>(this));

		// - not completed
		//   1) the body until the connection is closed : completed (the status of the response)
		//   2) the others : '504 Gateway Timeout' (timeout) or '502 Bad Gateway' (closed before the response)
		//   (completed already : an error response (ex. a failed websocket handshake) is delivered as it is)
		if (this->m_msg_http_response.is_completed == false)
		{
			const bool is_timeout = ((_disconnect_reason & DISCONNECT_REASON_MASK_USER) == DISCONNECT_REASON_SUSTAIN_TIMEOUT);	// - the response timeout / the connect timeout (Nsocket_sustain)

			if (this->m_is_body_until_close && is_timeout == false)
			{
				auto buf_body = alloc_shared_buffer(this->m_body_until_close.size() + 1);
				if (this->m_body_until_close.empty() == false)
					buf_body.append(this->m_body_until_close.size(), this->m_body_until_close.data());

				this->m_msg_http_response.contents.clear();
				this->m_msg_http_response.contents.emplace_back(std::move(buf_body));
				this->m_msg_http_response.buf_message = this->m_msg_http_response.contents[0];
			}
			else
			{
				this->m_msg_http_response.status_code = is_timeout ? eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_ : eHTTP_STATUS_CODE::BAD_GATEWAY_;
				this->m_msg_http_response.status_message = to_string(this->m_msg_http_response.status_code);
			}

			this->m_msg_http_response.is_completed = true;
		}

		// - 완료 처리
		prequest->process_response(this->m_msg_http_response);
	}

	// 3) reset the body until close
	this->m_is_body_until_close = false;
	this->m_body_until_close.clear();

	// 4) call on disconnect
	if (_socket_state == eSOCKET_STATE::CERTIFIED)
	{
		this->on_disconnect(_disconnect_reason);
	}
}

