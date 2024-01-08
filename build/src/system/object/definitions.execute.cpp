#include "pch.h"

namespace CGDK
{

template <>
std::string_view to_string<char>(eEXECUTOR_TYPE _executor_type) noexcept
{
	std::string_view result;

	switch (_executor_type)
	{
	case	eEXECUTOR_TYPE::DEFAULT:	result = "default";			break;
	case	eEXECUTOR_TYPE::SYSTEM:		result = "system";			break;
	case	eEXECUTOR_TYPE::USER:		result = "user";			break;
	default:							result = "";				break;
	}

	return result;
}

template <>
std::wstring_view to_string<wchar_t>(eEXECUTOR_TYPE _executor_type) noexcept
{
	std::wstring_view result;

	switch (_executor_type)
	{
	case	eEXECUTOR_TYPE::DEFAULT:	result = L"default";		break;
	case	eEXECUTOR_TYPE::SYSTEM:		result = L"system";			break;
	case	eEXECUTOR_TYPE::USER:		result = L"user";			break;
	default:							result = L"";				break;
	}

	return result;
}

statistics::unit_executor::unit_executor() noexcept
{
}

statistics::unit_executor::unit_executor(const unit_executor& _rhs) noexcept :
	tick_snap(_rhs.tick_snap),
	thread_count(_rhs.thread_count),
	count_execute(_rhs.count_execute.load())
{
}

statistics::unit_executor& statistics::unit_executor::operator=(const statistics::unit_executor& _rhs) noexcept
{
	tick_snap = _rhs.tick_snap;
	thread_count = _rhs.thread_count;
	count_execute = _rhs.count_execute.load();

	return *this;
}

void statistics::unit_executor::reset() noexcept
{
	tick_snap = chrono::tick::time_point::min();
	thread_count = 0;
	count_execute = 0;
}

statistics::unit_executor_snap& statistics::unit_executor_snap::operator = (const unit_executor& _copy) noexcept
{
	tick_snap = _copy.tick_snap;
	thread_count = _copy.thread_count;
	count_execute = _copy.count_execute;

	//return) 
	return *this;
}

void statistics::unit_executor_snap::reset() noexcept
{
	tick_snap = chrono::tick::time_point::min();
	thread_count = 0;
	count_execute = 0;
}



}