#include "pch.h"

using namespace CGDK;


time_event::schedule::service_destroy::service_destroy(server_system::Iservice* _pservice) :
	m_pservice(_pservice)
{
}

time_event::schedule::service_destroy::~service_destroy()
{
}

void time_event::schedule::service_destroy::set_service(server_system::Iservice* _pservice)
{
	m_pservice = _pservice;
}

result_code time_event::schedule::service_destroy::process_execute_function(const chrono::time::utc::time_point& /*_time*/)
{
	// check) 
	RETURN_IF(m_pservice.empty(), eRESULT::FAIL);

	// 1) Service를 시작한다.
	m_pservice->destroy();

	// return) 
	return eRESULT::SUCCESS;
}
