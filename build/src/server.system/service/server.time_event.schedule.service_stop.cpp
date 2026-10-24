#include "pch.h"

using namespace CGDK;


time_event::schedule::service_stop::service_stop(server_system::Iservice* _pservice) :
	m_pservice(_pservice)
{
}

time_event::schedule::service_stop::~service_stop()
{
}

void time_event::schedule::service_stop::set_service(server_system::Iservice* _pservice)
{
	this->m_pservice = _pservice;
}

result_code time_event::schedule::service_stop::process_execute_function(chrono::time::utc::time_point /*_time*/)
{
	// check) 
	RETURN_IF(this->m_pservice.empty(), eRESULT::FAIL);

	// 1) Service를 시작한다.
	this->m_pservice->stop();

	// return) 
	return eRESULT::SUCCESS;
}
