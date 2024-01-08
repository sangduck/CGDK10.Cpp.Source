#include "pch.h"

using namespace CGDK;


query::http::querier::querier() : 
	m_is_create(false)
{
	// 1) set Object type as QUERIER_HTTP
	this->set_object_type(eOBJECT_TYPE::QUERIER_HTTP);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::http::querier::querier(const std::string_view _name) :
	m_is_create(false)
{
	// 1) set Object type as QUERIER_HTTP
	this->set_object_type(eOBJECT_TYPE::QUERIER_HTTP);

	// 2) Name을 설정한다.
	this->set_name(_name);

	// 3) Register Querier to Manager
	this->register_querier();
}

query::http::querier::~querier() noexcept
{
	// 1) unregister
	this->unregister_querier();
}

result_code query::http::querier::start(Iexecutor* /*_pexecutor*/)
{
	context temp_msg;	// _pexecutor

	return this->start(temp_msg);
}

result_code query::http::querier::_process_initializing(context& _context)
{
	return this->Ninitializable::_process_initializing(_context);
}
result_code query::http::querier::_process_initialize(context& _context)
{
	return this->Ninitializable::_process_initialize(_context);
}
result_code query::http::querier::_process_destroying() noexcept
{
	return this->Ninitializable::_process_destroying();
}
result_code query::http::querier::_process_destroy() noexcept
{
	return this->Ninitializable::_process_destroy();
}

result_code query::http::querier::_process_starting(context& _context)
{
	scoped_lock(this->m_lockable_query)
	{
		// 1) executor가 nullptr이면 기본 executor를 사용한다.
		if(this->m_pexecutor.empty())
		{
			this->m_pexecutor = system_executor::executor();
			this->m_is_create = false;
		}
	}

	// 2)
	return this->Nstartable::_process_starting(_context);
}

result_code query::http::querier::_process_start(context& _context)
{
	return this->Nstartable::_process_start(_context);
}

result_code query::http::querier::_process_stopping() noexcept
{
	// 1)
	auto result = this->Nstartable::_process_stopping();

	// declare) 
	object_ptr<Iexecutor> pexecutor;
	bool is_created = false;

	// 2) 교체한다.
	scoped_lock(this->m_lockable_query)
	{
		// - executor를 Swap
		pexecutor = this->m_pexecutor;
		pexecutor.reset();

		// - 일단 create flag는 무조건 false로...
		is_created = this->m_is_create;
		this->m_is_create = false;
	}

	// 3) executor가 존재하고 여기서 생성된 것이라면 close한다.
	if(pexecutor.exist() && is_created == true)
	{
		pexecutor->stop();
	}

	// return)
	return result;
}

result_code query::http::querier::_process_stop() noexcept
{
	return this->Nstartable::_process_stop();
}

void query::http::querier::process_execute_query(Iserver_query* /*_pquery*/)
{
	//// 1) Casting한다.
	//auto pquery = dynamic_cast<query*>(_pquery);

	//scoped_lock_named(m_lockable_query, cs)
	//{
	//	// 1) setting
	//	pquery->m_pquerier = this;

	//	// 2) execute를 건다.
	//	pquery->request();

	//	// statistics)
	//	statistics_on_query_try();

	//	// 3) 완료를 wait한다.
	//	pquery->m_cv_complete.wait(cs);
	//}

	//// 3) reset 
	//pquery->m_pquerier.reset();
}

void query::http::querier::process_post_execute_query(Iserver_query* /*_pquery*/)
{
	//// 1) Casting한다.
	//auto	pquery = dynamic_cast<query*>(_pquery);

	//// check) ...
	//THROW_IF(pquery == nullptr, throwable::failure(), pquery);

	//// 2) ...
	//pquery->post_execute_query();
}

bool query::http::querier::cancel_query(Iserver_query* /*_pquery*/) noexcept
{
	return true;
}
