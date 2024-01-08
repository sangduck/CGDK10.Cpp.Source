#include "pch.h"
#include "cgdk/sdk10/net.socket/socket/net.io.sendable.executable_send_buffer_list.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;


net::io::sendable::executable_send_buffer_list::~executable_send_buffer_list() noexcept
{
	CGASSERT_ERROR(this->m_psendable.empty());
}

void net::io::sendable::executable_send_buffer_list::reset() noexcept
{
	this->m_psendable.reset();
}

intptr_t net::io::sendable::executable_send_buffer_list::process_execute(intptr_t _result, std::size_t _transferred)
{
	// 1) move psendable
	auto psendable = std::move(this->m_psendable);

	// check) m_buffer->buf이 nullptr이어서는 안된다.
	CGASSERT_ERROR(psendable.exist());

	// 2) call 'process_complete_sendable'
	psendable->process_complete_sendable(_result, _transferred, 0);

	// return)
	return intptr_t(0);
}


#endif