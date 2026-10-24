#include "pch.h"

using namespace CGDK;


time_event::schedule::service_init::service_init(server_system::Iservice* _pservice) :
	m_pservice(_pservice)
{
}

time_event::schedule::service_init::~service_init()
{
}

void time_event::schedule::service_init::set_service(server_system::Iservice* _pservice)
{
	this->m_pservice = _pservice;
}

result_code time_event::schedule::service_init::process_execute_function(chrono::time::utc::time_point /*_time*/)
{
	// check) 
	RETURN_IF(this->m_pservice.empty(), eRESULT::FAIL);

	// declare)
	context context_setting;

	// 1) Service를 시작한다.
	this->m_pservice->initialize(context_setting);

	// return) 
	return eRESULT::SUCCESS;
}
