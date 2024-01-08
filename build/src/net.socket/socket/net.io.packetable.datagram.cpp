#include "pch.h"

using namespace CGDK;


std::size_t net::io::packetable::Ndatagram::process_packet( shared_buffer& _buffer, const net::sockaddr& _address)
{
	// 1) message를 만든다.
	sMESSAGE_NETWORK msg(_buffer, _address);

	// 2) message를 Imessageable로 전달하여 처리한다.
	this->process_message(msg);

	// statistics) receive message수를 세린다.
	this->statistics_on_receive_message();
	
	// return) 다음 message의 크기를 return한다!(Datagram에서는 별 의미없다.)
	return sizeof(uint32_t);
}
