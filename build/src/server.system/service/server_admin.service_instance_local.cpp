#include "pch.h"

using namespace CGDK;

result_code server_system::service::instance_local::process_message(CGDK::sMESSAGE& /*_msg*/)
{
	return eRESULT::BYPASS;
}

void server_system::service::instance_local::process_serialize_in(buffer_view& _buffer)
{
	std::lock_guard lock(this->m_lockable_instance);

	// 1) extract and assign service _info
	this->info = _buffer.extract<server_system::sSERVICE>();

	// 2) extract instance key
	this->instance_key = _buffer.extract<server_system::sINSTANCE_KEY>();

	// 3) extract and assign host instance id
	this->host_instance_id = _buffer.extract<server_system::iid_t>();
}	
