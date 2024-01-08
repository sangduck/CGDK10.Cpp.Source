#include "pch.h"

using namespace CGDK;


std::size_t net::io::packetable::Nstream_json::process_packet( shared_buffer& _buffer, const net::sockaddr& _address)
{
	// check) _buffer.data_가 0이어서는 안된다.
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL), LOG_DEBUG << "(err ) CGDK.net.io: _buffer.data_ is nullptr. _buffer.size_: "sv << _buffer.size() << " " __CGFUNCTION__ ""sv)

	// check) _buffer->size_가 0이어서는 안된다.
	CGASSERT_ERROR(_buffer.size()!= 0)

	// definition) 제일 끝 바이트가 /r/n /r/n인지 확인한다.
	const uint16_t HTTP_TERMINAL = ('\n' << 8) | '}';

	// 1) 종료를 검사한다.
	auto iter_buf = _buffer.data();
	auto iter_buf_end = _buffer.data() + _buffer.size();

	for(; iter_buf != iter_buf_end; ++iter_buf)
	{
		if(*reinterpret_cast<uint16_t*>(iter_buf) == HTTP_TERMINAL)
		{
			// declare) 임시 sMESSAGE_NETWORK_JSON (전달용)
			sMESSAGE_NETWORK_JSON msg;

			// 2) shared_buffer끝을 일단 NULL로 설정한다.
			*(iter_buf + 1) = 0;

			// 3) 처리된 길이를 구한다.
			auto temp_size = iter_buf - _buffer.data() + 2;

			// 4) messageable에 전달할 message를 작성한다.
			msg.address = _address;
			msg.buf_message = _buffer;
			msg.buf_message.set_size(temp_size);

			// 5) 훅함수 process_pre_message()를 호출한다.
			this->process_pre_message(msg);

			// 6) message를 Imessageable로 전달하여 처리한다.
			this->process_message(msg);

			// 7) History에 Queueing한다.
			this->push_message_to_history(_buffer);

			// 8) 처리후 shared_buffer 포인터를 이동시킨다.
			_buffer += offset(temp_size);

			// statistics) receive message수를 세린다.
			this->statistics_on_receive_message(1);
		}
	}

	// return) 처리중인 message의 크기를 return한다.
	return static_cast<std::size_t>(8192);
}
