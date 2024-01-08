#include "pch.h"

using namespace CGDK;


std::size_t net::io::Nudp_resend::process_packet( shared_buffer& _buffer, const net::sockaddr& _address)
{
	// 1) 만약에 nullptr이 아니라면 이쪽으로 들어간다.
	if(_buffer.get_source().exist())
	{
		// 2) Packet Size가 shared_buffer Size보다 큰거라면 이건 Error다!!!
		ERROR_RETURN_IF(_buffer.front<uint32_t>() > _buffer.size(), false, LOG_DEBUG << "(err ) CGDK.net.socket: udp message is short (size:%u, received:"sv << _buffer.front<uint32_t>() << "). (" << _buffer.size() << ") " __CGFUNCTION__ ""sv);

		// explanation) tail message를 처리한다.

		// 1) Extra Byte를 얻는다.
		auto extra_bytes = (static_cast<int>(_buffer.size()) - static_cast<int>(_buffer.front<uint32_t>()));

		// 2) Extra Byte가 6Byte가 넘을 경우에 처리...
		if(extra_bytes >= 6)
		{
			// - tail message를 얻는다.
			uint32_t tail_message = _buffer.back<uint32_t>(-4);

			// - message종류에 따라 처리한다.
			switch(tail_message)
			{
			// In-Case) eMESSAGE::SOCKET::ACK
			case	eMESSAGE::NETWORK::ACK:
					{
						// - ACK한다.(마지막 4Byte가 ACK이다.)
						this->ack(_buffer.back<uint32_t>(-6));

						// - Extra Byte 크기 6을 줄인다.
						_buffer.sub_size(6);
					}
					return true;

			// In-Case) eMESSAGE::SOCKET::REQUEST_ACK
			case	eMESSAGE::NETWORK::REQUEST_ACK:
					{
						// - ACK Serial을 얻어서 전송한다.
						this->send_ack(_buffer.back<uint32_t>(-6), _address);

						// - Extra Byte 크기 6을 줄인다.
						_buffer.sub_size(6);
					}
					break;
			};
		}
	}
	else
	{
	}

	// statistics) receive message수를 세린다.
	this->statistics_on_receive_message();

	// explanation) 메시지 처리를 한다.

	// 1) message를 만든다.
	sMESSAGE_NETWORK msg(_buffer, _address);

	// 2) message을 처리한다.
	this->process_message(msg);

	// statistics) receive Packet수를 세린다.
	this->statistics_on_receive_message();

	// return) 다음 message의 크기를 return한다!(Datagram에서는 별 의미없다. 그래서 0)
	return 0;
}
