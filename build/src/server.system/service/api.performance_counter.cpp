#include "pch.h"

using namespace CGDK;


api::performance_counter::performance_counter()
#if defined (_WIN32)
	: m_hCounter(NULL)
#endif
{
}

api::performance_counter::performance_counter(std::string_view _query)
#if defined (_WIN32)
	: m_hCounter(NULL)
#endif
{
	// 1) 생성한다.
	auto result = create(_query);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::BAD_ALLOC), LOG_ERROR << "(err ) fail to create performance counter"sv);
}

api::performance_counter::performance_counter(std::wstring_view _query)
#if defined (_WIN32)
	: m_hCounter(NULL)
#endif
{
	// 1) 생성한다.
	auto result = create(_query);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::BAD_ALLOC), LOG_ERROR << "(err ) fail to create performance counter"sv);
}

api::performance_counter::~performance_counter() noexcept
{
	destroy();
}

#if defined (_WIN32)
bool api::performance_counter::create(std::string_view _query)
{
	// check) 
	RETURN_IF(m_hCounter != NULL, false);

	// 1) Counter를 추가한다.
	object_ptr<manager_performance_query>	pmanager = manager_performance_query::instance();

	// check) 
	CGASSERT_ERROR(pmanager.exist());

	// 2) 추가한다.
	pmanager->add_counter(this, _query);

	// return) 
	return true;
}
#elif(__linux__)
bool api::performance_counter::create(std::string_view /*_query*/)
{
	// return) 
	return true;
}
#else
#endif

#if defined (_WIN32)
bool api::performance_counter::create(std::wstring_view _query)
{
	// check) 
	RETURN_IF(m_hCounter != NULL, false);

	// 1) Counter를 추가한다.
	object_ptr<manager_performance_query>	pmanager	 = manager_performance_query::instance();

	// check) 
	CGASSERT_ERROR(pmanager.exist());

	// 2) 추가한다.
	pmanager->add_counter(this, _query);

	// return) 
	return true;
}
#elif(__linux__)
bool api::performance_counter::create(std::wstring_view /*_query*/)
{
	// return) 
	return true;
}
#else
#endif

void api::performance_counter::destroy() noexcept
{
#if defined (_WIN32)
	// check) 
	RETURN_IF(m_hCounter == NULL);

	// 1) Remove한다.
	m_pmanager->remove_counter(this);
#elif(__linux__)
#else
#endif
}

#if defined (_WIN32)
long api::performance_counter::get_value_long(uint32_t _flag)
{
	// declare) 
	PDH_STATUS				pdhStatus;
	PDH_FMT_COUNTERVALUE	fmtValue;
	uint32_t				ctrType{ 0 };

	// 1) 값을 읽어들인다.
	pdhStatus = PdhGetFormattedCounterValue (m_hCounter, PDH_FMT_LONG | _flag, (LPDWORD)&ctrType, &fmtValue);
	
	// check) 
	RETURN_IF(pdhStatus != ERROR_SUCCESS, 0);

	// 2) 값을 Return한다.
	return fmtValue.longValue;
}
#elif(__linux__)
long api::performance_counter::get_value_long(uint32_t /*_flag*/)
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return 0;
}
#else
#endif

#if defined (_WIN32)
double api::performance_counter::get_value_double()
{
	// declare) 
	PDH_STATUS				pdhStatus;
	PDH_FMT_COUNTERVALUE	fmtValue;
	uint32_t				ctrType{0};

	// 1) 값을 읽어들인다.
 	pdhStatus = PdhGetFormattedCounterValue (m_hCounter, PDH_FMT_DOUBLE, (LPDWORD)&ctrType, &fmtValue);
	 
	// check) 
	RETURN_IF(pdhStatus != ERROR_SUCCESS, 0.0f);

	// 2) 값을 Return한다.
	return	fmtValue.doubleValue;
}
#elif(__linux__)
double api::performance_counter::get_value_double()
{
	// error) 
	CGASSERT_ERROR(false);

	// return) 
	return 0.0f;
}
#else
#endif
