#include "pch.h"
#include "net.io.receivable.executable_receive.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;


intptr_t net::io::receivable::Ndatagram::executable_receive::process_execute( intptr_t /*_result*/, std::size_t _param)
{
	// check) m_preceivable_datagram이 nullptr이어서는 안된다.
	CGASSERT(this->m_preceivable_datagram != nullptr, intptr_t(1));

	// check) m_buffer->data_이 nullptr이어서는 안된다.
	CGASSERT(this->m_buffer.data() != nullptr, intptr_t(1));
	
	// 1) Receiver에게 보내기...
	this->m_preceivable_datagram->process_complete_receivable(reinterpret_cast<uintptr_t>(this), _param, m_buffer, m_socket_address[0]);

	// return) Success!
	return	intptr_t(0);
}


#endif