#include "pch.h"

using namespace CGDK;


bool net::io::sender::Nhttp::process_send( shared_buffer&& _buffer, std::size_t _count_message, uint64_t /*_Option*/)
{
	// check) is _buffer->size_ is 0
	RETURN_IF(_buffer.size() == 0, false)

	// check) is _buffer->data_ is nullptr
	RETURN_IF(_buffer.data() == nullptr, false)

	// check) is overflow
	CGASSERT(_buffer._is_buffer_overflow() == false, false);

	// check) return if socket is not ready
	RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

	// declare)
	net::sockaddr address_peer;

	// 1) do send
	return this->process_sendable(_buffer, _buffer.move_out_source(), _count_message, address_peer);
}
