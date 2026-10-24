#include "pch.h"

using namespace CGDK;

void server_system::service::instance_local::process_serialize_out(buffer& _buffer)
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	// 1) append service _info
	_buffer.append<server_system::sSERVICE>(this->info);

	// 2) append instance key
	_buffer.append<server_system::sINSTANCE_KEY>(this->instance_key);

	// 3) append host instance id
	_buffer.append<server_system::iid_t>(this->host_instance_id);
}

std::size_t server_system::service::instance_local::process_size_of() const
{
	// lock)
	std::lock_guard lock(this->m_lockable_instance);

	return get_size_of(this->info) + get_size_of(this->instance_key) + get_size_of(this->host_instance_id);
}
