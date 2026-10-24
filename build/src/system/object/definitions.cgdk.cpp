#include "pch.h"
#include <charconv>

namespace CGDK
{

extern	std::vector<sCOMPONENT_INFO>	g_vector_component;


std::string_view to_string(CGDK::eCOMPILER _eCompiler) noexcept
{
	// declare)
	std::string_view result;

	switch(_eCompiler)
	{
	case	CGDK::eCOMPILER::NOT_DEFINED:		result = "not defined"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_8:	result = "Visual Studio 2005"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_9:	result = "Visual Studio 2008"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_10:	result = "Visual Studio 2010"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_11:	result = "Visual Studio 2012"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_12:	result = "Visual Studio 2013"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_14:	result = "Visual Studio 2015"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_15:	result = "Visual Studio 2017"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_16:	result = "Visual Studio 2019"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_17:	result = "Visual Studio 2022"; break;
	case	CGDK::eCOMPILER::ECLIPS:			result = "ECLIPS"; break;
	case	CGDK::eCOMPILER::GCC:				result = "gcc"; break;
	case	CGDK::eCOMPILER::ALL:				result = "all"; break;
	default:									result = "Unknowned"; break;
	}

	// return)
	return result;
}

std::string_view to_string(CGDK::eCONFIGURATION _configure) noexcept
{
	// declare)
	std::string_view result;

	switch (_configure)
	{
	case	CGDK::eCONFIGURATION::NOT_DEFINED:	result = "not defined"; break;
	case	CGDK::eCONFIGURATION::DEBUG_:		result = "DEBUG";	break;
	case	CGDK::eCONFIGURATION::DEBUG_DLL_:	result = "DEBUG DLL";	break;
	case	CGDK::eCONFIGURATION::RELEASE_:		result = "RELEASE"; break;
	case	CGDK::eCONFIGURATION::RELEASE_DLL_:	result = "RELEASE DLL"; break;
	case	CGDK::eCONFIGURATION::ALL_:			result = "all"; break;
	default:									result = "Unknowned";	break;
	}

	// return)
	return result;
}

std::string_view to_string(CGDK::ePLATFORM _platform) noexcept
{
	// declare)
	std::string_view result;

	switch (_platform)
	{
	case	CGDK::ePLATFORM::NOT_DEFINED:		result = "not defined"; break;
	case	CGDK::ePLATFORM::WINDOWS_X86:		result = "Windows x86"; break;
	case	CGDK::ePLATFORM::WINDOWS_X64:		result = "Windows x64"; break;
	case	CGDK::ePLATFORM::LINUX:				result = "Linux"; break;
	case	CGDK::ePLATFORM::ANDROID:			result = "Android"; break;
	case	CGDK::ePLATFORM::MACOS:				result = "Macos"; break;
	case	CGDK::ePLATFORM::IOS:				result = "ios"; break;
	case	CGDK::ePLATFORM::ALL:				result = "all"; break;
	default:									result = "Unknowned"; break;
	}

	// return)
	return result;
}

int get_version() noexcept
{
	return SVN_COMMIT_REVISION;
}

// 13) CGDK Component Info
std::vector<sCOMPONENT_INFO>& _get_component_info() noexcept
{
	static std::vector<sCOMPONENT_INFO> vector_component;
	return vector_component;
}
const std::vector<sCOMPONENT_INFO>& get_component_info() noexcept
{
	return _get_component_info();
}

void add_component_info(sCOMPONENT_INFO&& _component) noexcept
{
	_get_component_info().emplace_back(std::move(_component));
}

sCOMPONENT_INFO get_component_info(std::string_view _name) noexcept
{
	const auto& vector_component = _get_component_info();

	for (auto& iter : vector_component)
	{
		if (iter.component_info.name == _name)
		{
			return iter;
		}
	}

	return sCOMPONENT_INFO();
}

std::string_view get_string_license() noexcept
{
	return	"copyright 2015 CHO SANGHYUN. all rights reserved";
}

namespace
{
	// - [_pos, _pos + _count) (empty if out of range)
	std::string_view _sub(std::string_view _value, size_t _pos, size_t _count = std::string_view::npos) noexcept
	{
		return (_pos < _value.size()) ? _value.substr(_pos, _count) : std::string_view();
	}

	// - leading spaces are skipped ('__DATE__' pads the day with a space)
	uint16_t _parse_uint16(std::string_view _value) noexcept
	{
		while (!_value.empty() && _value.front() == ' ')
			_value.remove_prefix(1);

		uint16_t result = 0;
		std::from_chars(_value.data(), _value.data() + _value.size(), result);
		return result;
	}

	uint16_t _parse_month(std::string_view _value) noexcept
	{
		constexpr std::string_view months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

		for (uint16_t i = 0; i < 12; ++i)
		{
			if (_sub(_value, 0, 3) == months[i])
				return static_cast<uint16_t>(i + 1);
		}

		return 0;
	}
}

chrono::time::utc::time_point get_build_date(const std::string_view _str_date, const std::string_view _str_time)
{
	// declare)
	chrono::date_time time_temp;

	// 1) __DATE__ : "Mmm dd yyyy"
	time_temp.t_month = _parse_month(_str_date);
	time_temp.t_day   = _parse_uint16(_sub(_str_date, 4, 2));
	time_temp.t_year  = static_cast<decltype(time_temp.t_year)>(_parse_uint16(_sub(_str_date, 7)));

	// 2) __TIME__ : "hh:mm:ss"
	time_temp.t_hour     = _parse_uint16(_sub(_str_time, 0, 2));
	time_temp.t_minute   = _parse_uint16(_sub(_str_time, 3, 2));
	time_temp.t_second   = _parse_uint16(_sub(_str_time, 6, 2));
	time_temp.t_fraction = 0;

	// return)
	return chrono::from_date_time<chrono::time::utc::time_point>(time_temp);
}

}