#include "pch.h"

using namespace CGDK;


bool net::io::sender::Nresp::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_Option*/)
{
	// check) _buffer->size_이 0이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer->data_가 nullptr이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// check)
	CGASSERT_ERROR(_buffer._is_buffer_overflow() == false);

	// check) 버퍼가 Overflow되었는가?
	if(_buffer._is_buffer_overflow() == true)
		return false;

	// check) Socket이 Established상태가 아니라면 전송은 안 된다.
	if(socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// declare)
	net::sockaddr address_peer;

	// 1) send
	return process_sendable(_buffer, _buffer.move_out_source(), _count_message, address_peer);
}
