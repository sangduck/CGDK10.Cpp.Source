#include "pch.h"


time_event_server::server_operation::server_operation() noexcept
{
}

time_event_server::server_operation::~server_operation() noexcept
{
}

result_code time_event_server::server_operation::process_event(chrono::time::utc::time_point /*_time*/)
{
	//// 1) find server instance
	//auto vector_find = g_pservice->m_pmanager_server_instance->search_instance(m_operation.hid, m_operation.id_server);

	//// 2) reun
	//switch(m_operation.operation)
	//{
	//case	eSERVER_OPERATION::SERVER_INITIALIZE:
	//		for (auto& iter : vector_find)
	//		{
	//			iter->request_service_initialize();
	//		}
	//		break;

	//case	eSERVER_OPERATION::SERVER_DESTROY:
	//		for (auto& iter : vector_find)
	//		{
	//			iter->request_service_destroy();
	//		}
	//		break;

	//case	eSERVER_OPERATION::SERVER_START:
	//		for (auto& iter : vector_find)
	//		{
	//			iter->request_service_start(m_operation.setting);
	//		}
	//		break;

	//case	eSERVER_OPERATION::SERVER_STOP:
	//		for (auto& iter : vector_find)
	//		{
	//			iter->request_service_stop();
	//		}
	//		break;

	//default:
	//		break;
	//}

	//// 3) store result
	//g_pservice->m_pmanager_time_event->store_time_event_result(this);

	// return) 
	return eRESULT::SUCCESS;
}

void time_event_server::server_operation::process_serialize_in(buffer_view& /*_buffer*/)
{
	//*(EVENT_SCHEDULE*)this	 = _buffer.extract<EVENT_SCHEDULE>();
	//m_operation				 = _buffer.extract<sSERVER_OPERATION>();
}

void time_event_server::server_operation::process_serialize_out(basic_buffer& /*_buffer*/)
{
	//_buffer.append<EVENT_SCHEDULE>(*(EVENT_SCHEDULE*)this);
	//_buffer.append<sSERVER_OPERATION>(m_operation);
}

size_t time_event_server::server_operation::process_size_of() const
{
	//return sizeof(EVENT_SCHEDULE) + sizeof(sSERVER_OPERATION);
	return 0;
}
