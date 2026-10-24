#include "pch.h"

namespace CGDK
{

#if defined(_WIN32)
	#include <pdh.h>
	#include <pdhmsg.h>

api::manager_performance_query::manager_performance_query() : m_hquery(NULL)
{
	// declare) 
	HQUERY hquery;
	PDH_STATUS pdh_status;

	// 1) Query를 open한다.
	pdh_status = PdhOpenQuery(NULL, NULL, &hquery);

	// check) 생성에 실패했으면 Throw한다!!
	if(pdh_status != ERROR_SUCCESS)
		throw throwable::failure(eRESULT::BAD_FUNCTION_CALL);

	// 2) 값을 설정한다.
	this->m_hquery = hquery;
}


api::manager_performance_query::~manager_performance_query() noexcept
{
	// check) 
	CGASSERT_FATAL(m_hquery != NULL);

   // 1) Query를 닫는다.
	PdhCloseQuery (m_hquery);
}

bool api::manager_performance_query::update_query()
{
	// declare) 
	PDH_STATUS pdh_status;

     // 1) 
	pdh_status = PdhCollectQueryData (m_hquery);

	// check) 
	if(pdh_status != ERROR_SUCCESS)
		return false;

	// return) 
	return true;
}


bool api::manager_performance_query::add_counter(performance_counter* _pcounter, std::string_view _query)
{
	SCOPED_LOCK(m_lockable_counter)
	{
		// check) 
		if(_pcounter == nullptr)
			return false;

		// check) 이미 생성되어 있는가?
		if(_pcounter->m_hCounter != NULL)
			return false;

		// check) 
		if(_query.empty())
			return false;

		// check) 
		CGASSERT_FATAL(this->m_hquery != NULL);

		// declare) 
		PDH_HCOUNTER hCounter;
		PDH_STATUS pdh_status;

		// 1) Counter를 추가한다.
		pdh_status = PdhAddCounterA(this->m_hquery, _query.data(), 0, &hCounter);

		// check) Counter의 추가가 실패했을 경우 false를 리턴한다.
		if(pdh_status != ERROR_SUCCESS)
			return false;

		// 2) 값을 설정한다.
		_pcounter->m_hCounter = hCounter;
		_pcounter->m_pmanager = this;
	}

   // return) 
   return true;
}

bool api::manager_performance_query::add_counter(performance_counter* _pcounter, std::wstring_view _query)
{
	SCOPED_LOCK(m_lockable_counter)
	{
		// check) 
		if(_pcounter == nullptr)
			return false;

		// check) 이미 생성되어 있는가?
		if(_pcounter->m_hCounter != NULL)
			return false;

		// check) 
		if(_query.empty())
			return false;

		// check) 
		CGASSERT_FATAL(this->m_hquery != NULL);

		// declare) 
		PDH_HCOUNTER hCounter;
		PDH_STATUS pdh_status;
		
		// 1) Counter를 추가한다.
		pdh_status = PdhAddCounterW(this->m_hquery, _query.data(), 0, &hCounter);
		
		// check) Counter의 추가가 실패했을 경우 false를 리턴한다.
		if(pdh_status != ERROR_SUCCESS)
		   return false;
		
		// 2) 값을 설정한다.
		_pcounter->m_hCounter = hCounter;
		_pcounter->m_pmanager = this;
	}

   // return) 
   return true;
}

bool api::manager_performance_query::remove_counter(performance_counter* _pcounter) noexcept
{
	SCOPED_LOCK(m_lockable_counter)
	{
		// check) 
		if(_pcounter == nullptr)
			return false;

		// check) 
		if(_pcounter->m_hCounter==NULL)
			return false;

		// check) 
		CGASSERT_FATAL(this->m_hquery!=NULL);

		// declare) 
		PDH_STATUS pdh_status;

		// 1) Counter를 제거한다.
		pdh_status = PdhRemoveCounter(_pcounter->m_hCounter);

	   // check) Counter의 제거를 실패했을 경우 false를 리턴한다.
		if(pdh_status!=ERROR_SUCCESS)
			return false;
	}

	// 2) 값을 설정한다.
	_pcounter->m_hCounter = NULL;
	_pcounter->m_pmanager.reset();

   // return) 
   return true;
}

#elif(__linux__)

api::manager_performance_query::manager_performance_query()
{
}

api::manager_performance_query::~manager_performance_query() noexcept
{
}

bool api::manager_performance_query::update_query()
{
	// return) 
	return true;
}

bool api::manager_performance_query::add_counter(performance_counter* /*_pcounter*/, std::string_view /*_query*/)
{
	// return) 
	return true;
}

bool api::manager_performance_query::add_counter(performance_counter* /*_pcounter*/, std::wstring_view /*_query*/)
{
	// return) 
	return true;
}

bool api::manager_performance_query::remove_counter(performance_counter* /*_pcounter*/) noexcept
{
	// return) 
	return true;
}


#else
	#error	"[CGDK10] Not supported Platform"
#endif

void api::manager_performance_query::initialize_instance()
{
	// locked)
	{
		// SCOPED_LOCK) 
		std::lock_guard lock(m_lockable_create);

		// check) 이미 생성되어 있으면 그냥 끝낸다.
		RETURN_IF(m_pinstance.exist());

		// 1) 신규로 생성한다.
		m_pinstance.exchange_release(_make_own_raw<manager_performance_query>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}


}