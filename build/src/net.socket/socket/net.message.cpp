#include "pch.h"

using namespace CGDK;


sMESSAGE_RESPONSE::sMESSAGE_RESPONSE( net::io::Isender* _psender, const shared_buffer& _buffer) :
	sMESSAGE_NETWORK(eMESSAGE::NETWORK::SOCKET::REMOTE_RESPONSE, _buffer + offset(16)),
	psender(_psender),								// sender를 설정한다.
	request_id(_buffer.front<int>(8)),				// Request ID를 설정한다.
	request_serial(_buffer.front<uint32_t>(12))		// Request Serial을 설정한다.
{
}

bool sMESSAGE_RESPONSE::response_message( const shared_buffer& _buffer)
{
	// check) 
	RETURN_IF(psender.empty(), false);

	// 1) 버퍼를 할당한다.
	shared_buffer temp_buffer = alloc_shared_buffer(32 + _buffer.size());

	// 2) message를 만든다.
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::REMOTE_RESPONSE);
	temp_buffer.append<int>(request_id);
	temp_buffer.append<uint32_t>(request_serial);
	temp_buffer.append(_buffer.size(), _buffer.data());
	temp_buffer.front<uint32_t>() = static_cast<uint32_t>(temp_buffer.size());

	// 3) 전송한다.
	return	psender->send(temp_buffer);
}