#include "pch.h"

using namespace CGDK;

bool server_system::host::instance_local::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option)
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	// 1) get sender
	auto psender = this->get_sender();

	// check)
	if (!psender)
		return false;

	// 2) send
	return psender->process_send(std::move(_buffer), _count_message, _option);
}

void server_system::host::instance_local::process_serialize_in(buffer_view& _buffer)
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	// 1) extract host info
	this->info = _buffer.extract<sHOST>();

	// 2) extract instance key
	this->instance_key = _buffer.extract<server_system::sINSTANCE_KEY>();
}

void server_system::host::instance_local::process_serialize_out(buffer& _buffer)
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	// 1) append host info
	_buffer.append<sHOST>(this->info);

	// 2) append instance key
	_buffer.append<server_system::sINSTANCE_KEY>(this->instance_key);
}

std::size_t server_system::host::instance_local::process_size_of() const
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	// 1) size of host info + size of instance key
	return get_size_of(this->info) + get_size_of(this->instance_key);
}
