#include "pch.h"

using namespace CGDK;


bool net::io::sender::Nresp::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_Option*/)
{
	// check) _buffer->size_이 0이 아닌가?
	RETURN_IF(_buffer.size() == 0, false)

	// check) _buffer->data_가 nullptr이 아닌가?
	RETURN_IF(_buffer.data() == nullptr, false)

	// check) 버퍼가 Overflow되었는가?
	CGASSERT(_buffer._is_buffer_overflow() == false, false);

	// check) Socket이 Established상태가 아니라면 전송은 안됀다.
	RETURN_IF(socket_state() < eSOCKET_STATE::ESTABLISHED, false);

	// declare)
	net::sockaddr address_peer;

	// 1) send
	return process_sendable(_buffer, _buffer.move_out_source(), _count_message, address_peer);
}
