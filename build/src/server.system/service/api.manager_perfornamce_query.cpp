#include "pch.h"

namespace CGDK
{

#if defined(_WIN32)
	#include <pdh.h>
	#include <pdhmsg.h>

api::manager_perfornamce_query::manager_perfornamce_query() : m_hquery(NULL)
{
	// declare) 
	HQUERY hquery;
	PDH_STATUS pdh_status;

	// 1) Query를 open한다.
	pdh_status = PdhOpenQuery(NULL, NULL, &hquery);

	// check) 생성에 실패했으면 Throw한다!!
	THROW_IF(pdh_status != ERROR_SUCCESS, throwable::failure(eRESULT::BAD_FUNCTION_CALL));

	// 2) 값을 설정한다.
	m_hquery = hquery;
}


api::manager_perfornamce_query::~manager_perfornamce_query() noexcept
{
	// check) 
   CGASSERT_ERROR(m_hquery != NULL);

   // 1) Query를 닫는다.
	PdhCloseQuery (m_hquery);
}

bool api::manager_perfornamce_query::update_query()
{
	// declare) 
	PDH_STATUS pdh_status;

     // 1) 
	pdh_status = PdhCollectQueryData (m_hquery);

	// check) 
	RETURN_IF(pdh_status != ERROR_SUCCESS, false);

	// return) 
	return true;
}


bool api::manager_perfornamce_query::add_counter(performance_counter* _pcounter, std::string_view _query)
{
	scoped_lock(m_lockable_counter)
	{
		// check) 
		RETURN_IF(_pcounter == nullptr, false);

		// check) 이미 생성되어 있는가?
		RETURN_IF(_pcounter->m_hCounter != NULL, false);

		// check) 
		RETURN_IF(_query.empty(), false);

		// check) 
	   CGASSERT(m_hquery != NULL, false);

	   // declare) 
	   PDH_HCOUNTER hCounter;
	   PDH_STATUS pdh_status;

	   // 1) Counter를 추가한다.
	   pdh_status = PdhAddCounterA(m_hquery, _query.data(), 0, &hCounter);

	   // check) Counter의 추가가 실패했을 경우 false를 리턴한다.
	   RETURN_IF(pdh_status != ERROR_SUCCESS, false);

	   // 2) 값을 설정한다.
	   _pcounter->m_hCounter = hCounter;
	   _pcounter->m_pmanager = this;
	}

   // return) 
   return true;
}

bool api::manager_perfornamce_query::add_counter(performance_counter* _pcounter, std::wstring_view _query)
{
	scoped_lock(m_lockable_counter)
	{
		// check) 
		RETURN_IF(_pcounter == nullptr, false);

		// check) 이미 생성되어 있는가?
		RETURN_IF(_pcounter->m_hCounter != NULL, false);

		// check) 
		RETURN_IF(_query.empty(), false);

		// check) 
	   CGASSERT(m_hquery != NULL, false);

	   // declare) 
	   PDH_HCOUNTER hCounter;
	   PDH_STATUS pdh_status;

	   // 1) Counter를 추가한다.
	   pdh_status = PdhAddCounterW(m_hquery, _query.data(), 0, &hCounter);

	   // check) Counter의 추가가 실패했을 경우 false를 리턴한다.
	   RETURN_IF(pdh_status!=ERROR_SUCCESS, false);

	   // 2) 값을 설정한다.
	   _pcounter->m_hCounter = hCounter;
	   _pcounter->m_pmanager = this;
	}

   // return) 
   return true;
}

bool api::manager_perfornamce_query::remove_counter(performance_counter* _pcounter) noexcept
{
	scoped_lock(m_lockable_counter)
	{
		// check) 
		RETURN_IF(_pcounter == nullptr, false);

		// check) 
		RETURN_IF(_pcounter->m_hCounter==NULL, false);

		// check) 
		CGASSERT_ERROR(m_hquery!=NULL);

		// declare) 
		PDH_STATUS pdh_status;

		// 1) Counter를 제거한다.
		pdh_status = PdhRemoveCounter(_pcounter->m_hCounter);

	   // check) Counter의 제거를 실패했을 경우 false를 리턴한다.
		RETURN_IF(pdh_status!=ERROR_SUCCESS, false);
	}

	// 2) 값을 설정한다.
	_pcounter->m_hCounter = NULL;
	_pcounter->m_pmanager.reset();

   // return) 
   return true;
}

#elif(__linux__)

api::manager_perfornamce_query::manager_perfornamce_query()
{
}

api::manager_perfornamce_query::~manager_perfornamce_query() noexcept
{
}

bool api::manager_perfornamce_query::update_query()
{
	// return) 
	return true;
}

bool api::manager_perfornamce_query::add_counter(performance_counter* /*_pcounter*/, std::string_view /*_query*/)
{
	// return) 
	return true;
}

bool api::manager_perfornamce_query::add_counter(performance_counter* /*_pcounter*/, std::wstring_view /*_query*/)
{
	// return) 
	return true;
}

bool api::manager_perfornamce_query::remove_counter(performance_counter* /*_pcounter*/) noexcept
{
	// return) 
	return true;
}


#else
	#error	"[CGDK10] Not supported Platform"
#endif

void api::manager_perfornamce_query::initialize_instance()
{
	// locked)
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_create);

		// check) 이미 생성되어 있으면 그냥 끝낸다.
		RETURN_IF(m_pinstance.exist());

		// 1) 신규로 생성한다.
		m_pinstance = _make_own_raw<manager_perfornamce_query>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}


}