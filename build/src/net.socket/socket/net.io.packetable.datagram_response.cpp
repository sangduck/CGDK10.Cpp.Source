#include "pch.h"

using namespace CGDK;


std::size_t net::io::packetable::Ndatagram_response::process_packet( shared_buffer& _buffer, const net::sockaddr& _address)
{
	// check) Debug를 위한 잘못된 값을 검사.
	if(_buffer.get_source().exist())
	{
		// check) message 크기가 shared_buffer 크기보다 크다면 이건 Error다!!!
		THROW_IF(_buffer.front<uint32_t>() > _buffer.size(), throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "message length is longer then shared_buffer size"sv), LOG_DEBUG << "(err ) CGDK.net.socket: message length is longer then shared_buffer. size:"sv << _buffer.front<uint32_t>() << ", received:"sv << _buffer.size() << " " __CGFUNCTION__ ""sv);

		// 1) extra byte Size크기 얻기. (tail message를 처리)
		auto extra_bytes = _buffer.size() - _buffer.front<uint32_t>();

		// 2) extra byte가 존재할 경우...
		if(extra_bytes >= 6)
		{
			// - tail message를 얻는다.
			uint32_t tail_message = _buffer.back<uint32_t>(-4);

			// In-Case) eMESSAGE_SYSTEM_REQUEST_ACK
			if(tail_message == eMESSAGE::NETWORK::REQUEST_ACK)
			{
				// - ACK Serial을 얻어서 전송한다.
				send_ack(_buffer.back<uint32_t>(-8), _address);

				// - Extra Byte 크기 8을 줄인다.
				_buffer.sub_size(8);
			}
		}
	}
	else
	{
	}

	// 3) message를 만든다.
	sMESSAGE_NETWORK msg(_buffer, _address);

	// 4) message를 Imessageable로 전달하여 처리한다.
	this->process_message(msg);

	// statistics) receive message수를 세린다.
	this->statistics_on_receive_message();

	// return) 다음 message의 크기를 return한다!(Datagram에서는 별 의미없다. 그래서 0)
	return 0;
}

bool net::io::packetable::Ndatagram_response::send_ack( uint32_t _Serial, const net::sockaddr& _address)
{
	//-----------------------------------------------------------------
	// 주의) ACK의 경우 뒤에서부터 읽는 tail message이라 따라서 message를
	//       뒷쪽에다 붙히고 Serial을 그 이전에 붙이게 된다.
	//-----------------------------------------------------------------
	// 1) shared_buffer를 생성한다.
	auto temp_buffer = alloc_shared_buffer(64);

	// 2) message를 작성한다.
	temp_buffer.append<uint32_t>(8);
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::ACK);
	temp_buffer.append<uint32_t>(_Serial);
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::ACK);

	// 3) 전송한다.
	return this->request_send(temp_buffer, temp_buffer.get_source(), 1, _address);
}
