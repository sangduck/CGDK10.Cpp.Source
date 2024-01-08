#include "pch.h"
#include "cgdk/sdk10/net.socket/socket/net.io.sendable.tcp_crypt.executable_buffer.h"

#if defined(_WINSOCK2API_)

using namespace CGDK;


net::io::sendable::Ntcp_crypt::executable_send_buffer::~executable_send_buffer() noexcept
{
	CGASSERT_ERROR(this->m_reference.empty());
	CGASSERT_ERROR(this->m_powner.empty());
	CGASSERT_ERROR(this->m_psendable.empty());
}

void net::io::sendable::Ntcp_crypt::executable_send_buffer::reset() noexcept
{
	this->m_reference.reset();
	this->m_powner.reset();
	this->m_psendable.reset();
}

intptr_t net::io::sendable::Ntcp_crypt::executable_send_buffer::process_execute(intptr_t _result, std::size_t _transferred)
{
	// 1) move psendable
	auto psendable = std::move(this->m_psendable);
	auto powner = std::move(this->m_powner);
	auto reference = std::move(this->m_reference);

	// check) m_buffer->buf이 nullptr이어서는 안된다.
	CGASSERT_ERROR(psendable.exist());

	// 2) call 'process_complete_sendable'
	psendable->process_complete_sendable(_result, _transferred, this->m_count_message);

	// return) 
	return intptr_t(0);
}


#endif