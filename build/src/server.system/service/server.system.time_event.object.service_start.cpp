#include "pch.h"

using namespace CGDK;


time_event::object::service_start::service_start(server_system::Iservice* _pservice) :
	m_pservice(_pservice)
{
}

time_event::object::service_start::~service_start()
{
}

void time_event::object::service_start::set_service(server_system::Iservice* _pservice)
{
	m_pservice = _pservice;
}

result_code time_event::object::service_start::process_event(const chrono::time::utc::time_point& /*_time*/)
{
	// check) 
	RETURN_IF(m_pservice.empty(), eRESULT::FAIL);

	// 1) Service를 시작한다.
	m_pservice->start();

	// return) 
	return eRESULT::SUCCESS;
}
