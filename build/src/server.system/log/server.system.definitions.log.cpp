#include "pch.h"


namespace CGDK
{

template <>
std::string_view to_string<char>(eORIGIN_TIME_TYPE _origin_time_type, bool _upper_case) noexcept
{
	// check) 
	RETURN_IF(_origin_time_type < eORIGIN_TIME_TYPE::NOW || _origin_time_type >= eORIGIN_TIME_TYPE::MAX, "");

	static std::string_view	str_type_upper[] =
	{
		"NOW",				// 0
		"MINUTE_BEGIN",		// 1
		"HOUR_BEGIN",		// 2
		"DAY_BEGIN",		// 3
		"WEEK_BEGIN",		// 4
		"MONTH_BEGIN",		// 5
		"YEAR_BEGIN"		// 6
		"SPECIFIED"			// 6
		"DEFAULT"			// 7
	};

	static std::string_view	strType_lower[] =
	{
		"now",				// 0
		"minute_begin",		// 1
		"hour_begin",		// 2
		"day_begin",		// 3
		"week_begin",		// 4
		"month_begin",		// 5
		"year_begin"		// 6
		"specified"			// 6
		"default"			// 7
	};

	// return) 
	return (_upper_case) ? str_type_upper[static_cast<int>(_origin_time_type)] : strType_lower[static_cast<int>(_origin_time_type)];
}

template <>
std::wstring_view to_string<wchar_t>(eORIGIN_TIME_TYPE _origin_time_type, bool _upper_case) noexcept
{
	// check) 
	RETURN_IF(_origin_time_type < eORIGIN_TIME_TYPE::NOW || _origin_time_type >= eORIGIN_TIME_TYPE::MAX, L"");

	static std::wstring_view str_type_upper[] =
	{
		L"NOW",				// 0
		L"MINUTE_BEGIN",	// 1
		L"HOUR_BEGIN",		// 2
		L"DAY_BEGIN",		// 3
		L"WEEK_BEGIN",		// 4
		L"MONTH_BEGIN",		// 5
		L"YEAR_BEGIN"		// 6
		L"SPECIFIED"		// 6
		L"DEFAULT"			// 7
	};

	static std::wstring_view strType_lower[] =
	{
		L"now",				// 0
		L"minute_begin",	// 1
		L"hour_begin",		// 2
		L"day_begin",		// 3
		L"week_begin",		// 4
		L"month_begin",		// 5
		L"year_begin"		// 6
		L"specified"		// 6
		L"default"			// 7
	};

	return (_upper_case) ? str_type_upper[static_cast<int>(_origin_time_type)] : strType_lower[static_cast<int>(_origin_time_type)];
}


}