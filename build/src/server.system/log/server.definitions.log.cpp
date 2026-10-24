#include "pch.h"


namespace CGDK
{

std::string_view to_string(eORIGIN_TIME_TYPE _origin_time_type, bool _upper_case) noexcept
{
	static std::string_view	str_type_upper[] =
	{
		"NOW",				// 0
		"MINUTE_BEGIN",		// 1
		"HOUR_BEGIN",		// 2
		"DAY_BEGIN",		// 3
		"WEEK_BEGIN",		// 4
		"MONTH_BEGIN",		// 5
		"YEAR_BEGIN",		// 6
		"SPECIFIED",		// 7
		"DEFAULT"			// 8
	};

	static std::string_view	strType_lower[] =
	{
		"now",				// 0
		"minute_begin",		// 1
		"hour_begin",		// 2
		"day_begin",		// 3
		"week_begin",		// 4
		"month_begin",		// 5
		"year_begin",		// 6
		"specified",		// 7
		"default"			// 8
	};

	// auto
	auto index = static_cast<int>(_origin_time_type);

	// check) 
	RETURN_IF(index < static_cast<int>(eORIGIN_TIME_TYPE::NOW) || index >= static_cast<int>(eORIGIN_TIME_TYPE::MAX), {});

	// return) 
	return (_upper_case) ? str_type_upper[index] : strType_lower[index];
}

}