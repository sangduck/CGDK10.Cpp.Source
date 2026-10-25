#include "pch.h"

using namespace CGDK;

#define	BUFFER_MIN	4096

bool do_extract_text(resp_data& _resp_data, shared_buffer& _buffer);
bool do_process_resp_parsing(resp_data& _resp_data, shared_buffer& _buffer);
bool do_process_resp_parsing_bulk_string(resp_data& _resp_data, shared_buffer& _buffer);
bool do_process_resp_parsing_array(resp_data& _resp_data, shared_buffer& _buffer);
shared_buffer make_message_RESP_PONG();

void net::io::packetable::Nresp::process_reset_packetable() noexcept
{
	this->m_msg_resp.data.reset();
}

std::size_t net::io::packetable::Nresp::process_packet( shared_buffer& _buffer, const net::sockaddr&)
{
	// static) 
	static shared_buffer buf_PONG = make_message_RESP_PONG();

	// definitions) 
	const uint32_t CONST_PING = ('G' << 24) | ('N' << 16) | ('I' << 8) | 'P';
	const uint16_t TERMINAL_CHARACTER = ('\n' << 8) | '\r';

	// check) _buffer.data_가 0이어서는 안된다.
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT), LOG_DEBUG << "(err ) CGDK.net.socket: _buffer.data_ is nullptr. _buffer.size_: " << _buffer.size() << " " __CGFUNCTION__);

	// check) _buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size() != 0)


	//-----------------------------------------------------------------
	// 1. Packet 처리
	//    * 주의) Message가 완전히 완료 될 때까지 Extract를 적용해서는
	//      안된다. 완전히 하나의 Message인 것을 확인 후에 Extract를
	//      적용해야 한다. 
	//      그렇지 않으면 Message가 끊겼을 경우 제대로 동작하지 않을 수
	//      있다.
	//-----------------------------------------------------------------
	// declare) 
	shared_buffer temp_buffer = _buffer;

	try
	{
		int count_message = 0;

		// 1) process remained resp message
		if (this->m_msg_resp.data.type != eRESP_DATA_TYPE::NONE_)
		{
			// check)
			CGASSERT_ERROR(this->m_msg_resp.data.type != eRESP_DATA_TYPE::PING_)

			// check) 당연히 temp_buffer.size_은 0이면 안 된다.
			CGASSERT_ERROR(temp_buffer.size() != 0)

			// - RedisResul를 진행한다.
			auto result = do_process_resp_parsing(this->m_msg_resp.data, temp_buffer);

			// check) 완전히 처리하지 못했다면 처리하던 Redis객체를 저장해 놓고 끝낸다.
			//        - 나머지 데이터가 더 도착하면 이어서 처리하기 위해...
			//        - 준비해야 할 버퍼의 크기(현재 받아놓은 버퍼의 크기 + BUFFER_MIN)를 리턴한다.
			//   (what was consumed is kept : the resumed parsing may have read a line. returned unchanged, the line was read again)
			if (result == false)
			{
				_buffer = std::move(temp_buffer);
				return _buffer.size() + BUFFER_MIN;
			}

			// - Message 사전 처리 프로세스를 호출
			process_pre_message(this->m_msg_resp);

			// - Message 처리 프로세스 호출
			process_message(this->m_msg_resp);

			// - reset msg_resp
			this->m_msg_resp.data.reset();

			// - Packet 수증가.
			++count_message;
		}

		// 2) pross message
		while(temp_buffer.size() > 0)
		{
			// - get RESP_DATA_TYPE
			auto resp_type = static_cast<eRESP_DATA_TYPE>(*temp_buffer.data<char>());

			if(resp_type != eRESP_DATA_TYPE::PING_)
			{
				// declare) 
				this->m_msg_resp.data.type = resp_type;

				// declare)
				auto result = do_process_resp_parsing(this->m_msg_resp.data, temp_buffer);

				// check) 완전히 처리하지 못했다면(result==false일 경우) 처리하던 Redis객체를 저장해 놓고 끝낸다.
				//        (데이터가 더 도착하면 이어서 처리하기 위해...)
				BREAK_IF(result == false);

				// - process pre-message
				this->process_pre_message(this->m_msg_resp);

				// - process message
				this->process_message(this->m_msg_resp);

				// - reset msg_resp
				this->m_msg_resp.data.reset();
			}
			else
			{
				// check) 
				BREAK_IF(temp_buffer.size() < 6);

				// check) 
				THROW_IF((*temp_buffer.data<uint32_t>() != CONST_PING) || (*temp_buffer.data<uint16_t>(4) != TERMINAL_CHARACTER), throwable::failure(eRESULT::INVALID_ARGUMENT));

				// - replay ping
				this->send(buf_PONG);

				temp_buffer += offset(6);
			}

			// - increase message count
			++count_message;
		}

		// statistics) Receie받은 Message수를 증가.
		statistics_on_receive_message(count_message);
	}
	catch(...)
	{
		// - the partial message is dropped (the socket is closed)
		this->m_msg_resp.data.reset();

		// check) 최대 메시지 크기보다 버퍼가 커진 경우 Exception을 던진다.
		THROW_IF(temp_buffer.size() + BUFFER_MIN > MAX_TCP_MESSAGE_SIZE, throwable::failure(eRESULT::OVERFLOW_ERROR));

		// reraise) an incomplete message returns false (it does not come here) : a parse error or an exception of
		//          process_message(). swallowed, the partial state stayed (the replies were matched to the wrong queries)
		//          : the socket is closed
		throw;
	}

	// - set
	_buffer = std::move(temp_buffer);

	// return) 
	return _buffer.size() + BUFFER_MIN;
}

shared_buffer make_message_RESP_PONG()
{
	// 1) alloc shared_buffer
	shared_buffer buf_temp = alloc_shared_buffer(16);

	// 2) write pong message
	buf_temp.append_text("+PONG\r\n");

	// return)
	return buf_temp;
}

bool do_extract_integer(buffer_view& _buffer, int64_t& _result)
{
	// check) _buffer.data_ must be not nullptr
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// 1) (the line may not be complete yet : every read is checked against the end first)
	char* pos_begin = _buffer.data() + 1;
	char* pos_now = pos_begin;
	char* pos_end = _buffer.data() + _buffer.size();
	int64_t	value = 0;
	bool is_negative = false;

	// check)
	RETURN_IF(pos_now >= pos_end, false);

	// 2) get sign
	if (*pos_now == '-')
	{
		// - negative
		is_negative = true;

		// - next
		++pos_now;
	}

	// 2) parsing integer value
	for (;;)
	{
		// check) not complete yet
		RETURN_IF(pos_now >= pos_end, false);

		// - 값을 읽는다.
		char temp_num = *pos_now;

		BREAK_IF(temp_num == '\r');

		// check) 
		THROW_IF(temp_num<'0' || temp_num>'9', throwable::failure(eRESULT::INVALID_ARGUMENT));

		// check) overflow
		THROW_IF(value > ((std::numeric_limits<int64_t>::max)() - 9) / 10, throwable::failure(eRESULT::INVALID_ARGUMENT));

		// - value...
		value = value * 10 - '0' + temp_num;

		// - next
		++pos_now;
	}

	// check) no digit
	THROW_IF(pos_now == pos_begin + (is_negative ? 1 : 0), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check) '\r' is the last byte received : '\n' is not here yet
	RETURN_IF(pos_now + 1 >= pos_end, false);

	// check) ...
	THROW_IF(*(pos_now + 1) != '\n', throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 3) negative
	if (is_negative)
	{
		value = (~value) + 1;
	}

	// 4) set...
	_buffer = buffer_base<char>{ static_cast<std::size_t>(pos_end - pos_now - 2), pos_now + 2 };

	// check) 
	CGASSERT_ERROR(_buffer.size() < 0x1fffffff);

	// 5) result를 써넣는다
	_result = value;

	// return) 
	return true;
}

bool do_extract_text(resp_data& _resp_data, shared_buffer& _buffer)
{
	// 1) (pos_end : the end of the data received. every read is checked against it first)
	char* pos_begin = _buffer.data();
	char* pos_now = pos_begin + 1;
	char* pos_end = pos_begin + _buffer.size();

	// 2) parsing integer value
	for (;;)
	{
		// check) 끝까지 왔다면 0을 리턴한다.
		RETURN_IF(pos_now >= pos_end, false);

		BREAK_IF(*pos_now == '\r');

		// - next
		++pos_now;
	}

	// check) '\r' is the last byte received : '\n' is not here yet
	RETURN_IF(pos_now + 1 >= pos_end, false);

	// check) 문자열의 끝이 '\r\n'이 아니라면 exception을 던진다.
	THROW_IF(*(pos_now + 1) != '\n', throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 3) set...
	_buffer = buffer_base<char>{ static_cast<std::size_t>(pos_end - pos_now - 2), pos_now + 2 };

	// check) 
	CGASSERT_ERROR(_buffer.size() < 0x1fffffff);

	// 4) buf_value를 설정한다.
	_resp_data.str_value = std::string_view(pos_begin + 1, pos_now - pos_begin - 1);
	_resp_data.buf_store = _buffer ^ buffer_view(pos_begin, pos_now - pos_begin + 2);

	// return) 
	return true;
}

bool do_process_resp_parsing_bulk_string(resp_data& _resp_data, shared_buffer& _buffer)
{
	// declare) 
	char* pos_begin = _buffer.data();
	char* pos_str = nullptr;
	int64_t len_bulk_string = 0;

	// 1-1) pos_processing이 1이면 len_bulk_string을 읽어낸다.
	if (_resp_data.pos_processing == 0)
	{
		// - 첫줄을 읽어낸다.
		auto result = do_extract_integer(_buffer, len_bulk_string);

		// check) 
		RETURN_IF(result == false, false);

		// - null bulk string ("$-1\r\n") : no data and no '\r\n' after it
		//   (2 bytes were skipped as its '\r\n' : they were the head of the next reply and the replies got out of order)
		if (len_bulk_string < 0)
		{
			_resp_data.pos_processing = 0;
			_resp_data.buf_store = _buffer ^ buffer_view{ pos_begin, static_cast<std::size_t>(_buffer.data() - pos_begin) };
			return true;
		}

		// check) the length + '\r\n' must not overflow
		THROW_IF(len_bulk_string > static_cast<int64_t>(0x7fffffff), throwable::failure(eRESULT::INVALID_ARGUMENT));
	}
	// 1-2) m_posProcess=2이면 [문자열]의 길이는 읽어낸 것이므로...
	else
	{
		len_bulk_string = _resp_data.pos_processing - 2;
	}

	// 2) len_bulk_string이 0보다 클 경우 문자열을 읽어낸다.
	//    (Bulk의 경우 아무런 데이터가 전송되지 않을 시 length가 -1이 전송되는 경우도 있다.)
	//    (also with the length 0 : "$0\r\n\r\n" has the '\r\n' after the empty data)
	if(len_bulk_string >= 0)
	{
		// check) 남은 데이터의 길이가 length + tail('\r\n')보다 짧으면 처리를 중단하고 false를 리턴한다.
		if (_buffer.size() < static_cast<std::size_t>(len_bulk_string + 2))
		{
			// - store pos_processing
			_resp_data.pos_processing = len_bulk_string + 2;

			// return) 
			return false;
		}
	
		// definitions) 
		const uint16_t terminal_resp = ('\n' << 8) | '\r';
	
		// check) 문자열의 마지막에 '\r\n'이 아니라면 잘못된 것이므로 Exception을 던진다.
		THROW_IF(*_buffer.data<const uint16_t>(len_bulk_string) != terminal_resp, throwable::failure(eRESULT::INVALID_ARGUMENT));
	
		// - 둘쨋줄을 읽어낸다.
		pos_str = _buffer.data();
	}

	// 3) process add length
	_buffer += offset(len_bulk_string + 2);

	// check) 
	CGASSERT_ERROR(_buffer.size() < 0x1fffffff);

	// 4) Processing을 0으로 reset해준다.
	_resp_data.pos_processing = 0;

	// 5) 결과를 저장한다. Bulk String을 추가한다. (len_bulk_string가 0보다는 커야한다.)
	if (len_bulk_string > 0)
	{
		_resp_data.str_value = std::string_view(pos_str, static_cast<uint32_t>(len_bulk_string));
	}
	_resp_data.buf_store = _buffer ^ buffer_view{ pos_begin, static_cast<std::size_t>(_buffer.data() - pos_begin) };

	// return) 
	return true;
}

bool do_process_resp_parsing_array(resp_data& _resp_data, shared_buffer& _buffer)
{
	// declare)
	int64_t	count = 0;

	// 1) posProcessing이 0일 경우 - count를 읽어낸다.
	if (_resp_data.pos_processing == 0)
	{
		// 1) 첫줄을 읽어낸다.
		auto result = do_extract_integer(_buffer, count);

		// check) 만약 Line을 다 완성하지 못했으면 끝낸다
		RETURN_IF(result == false, false);

		// check) 여기서는 반드시 비어 있어야 한다.
		CGASSERT_ERROR(_resp_data.vector_child.empty());

		// 3) std::verter를 Reserve한다.
		if (count > 0)
		{
			_resp_data.vector_child.reserve(static_cast<std::size_t>((std::min)(count, int64_t(1024))));
		}
	}
	else
	{
		// check) 크기가 0보다는 커야한다.
		CGASSERT_ERROR(_resp_data.vector_child.size() > 0);

		// - 개수를 복원 한다.
		count = _resp_data.pos_processing - 2;

		// - Processing 중이었다면 이후 계속 읽음.
		if (count & 0x01)
		{
			// - pasing resp data
			auto result = do_process_resp_parsing(_resp_data.vector_child.back(), _buffer);

			// check) 완전히 처리하지 못했다면 처리하던 Redis객체를 저장해 놓고 끝낸다.
			//        (나머지 데이터가 더 도착하면 이어서 처리하기 위해...)
			RETURN_IF(result == false, false);
		}

		// - count를 나누기 2한다.
		count >>= 1;
	}

	// 4) get array  account Array 개수만큼 돌리며 Array 값을 읽어낸다.
	for(; count >0; --count)
	{
		// check) return if length of _buffer is below 1(length < 1)
		if(_buffer.size() < 1)
		{
			// - store 'pos_processing'
			_resp_data.pos_processing = (count *2) + 2;

			// return) 
			return false;
		}

		// - push_back
		//   (the type is the first byte : it read 4 bytes as the enum and an element of an array never matched a type)
		auto& temp = _resp_data.vector_child.emplace_back(static_cast<eRESP_DATA_TYPE>(*_buffer.data<char>()));

		// - process parsing
		auto result = do_process_resp_parsing(temp, _buffer);

		// check) 완전히 처리하지 못했다면 처리하던 Redis객체를 저장해 놓고 끝낸다.
		//        (나머지 데이터가 더 도착하면 이어서 처리하기 위해...)
		if (result == false)
		{
			// - store 'pos_processing'
			_resp_data.pos_processing = (count *2 - 1) + 2;

			// return) 
			return false;
		}
	}

	// 5) reset m_pos_precessing
	_resp_data.pos_processing = 0;

	// return) 
	return true;
}

bool do_process_resp_parsing(resp_data& _resp_data, shared_buffer& _buffer)
{
	// check)
	CGASSERT_ERROR(_resp_data.type != eRESP_DATA_TYPE::PING_);

	// declare)
	bool result;

	switch (_resp_data.type)
	{
	case	eRESP_DATA_TYPE::SIMPLE_STRING_:
	case	eRESP_DATA_TYPE::ERROR_:
	case	eRESP_DATA_TYPE::INTEGER_:		result = do_extract_text (_resp_data, _buffer); break;
	case	eRESP_DATA_TYPE::BULK_STRING_:	result = do_process_resp_parsing_bulk_string (_resp_data, _buffer); break;
	case	eRESP_DATA_TYPE::ARRAY_:		result = do_process_resp_parsing_array (_resp_data, _buffer); break;
	default:								CGASSERT_ERROR(false);	LOG_DEBUG << "(err ) RESP: syntax error. ["sv << static_cast<int>(_resp_data.type) << "]"sv; throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	};

	// return)
	return result;
}
