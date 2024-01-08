#include "pch.h"

namespace CGDK
{

extern	std::vector<sCOMPONENT_LICENSE>	g_vector_component;


template<>
std::string_view to_string<char>(CGDK::eCOMPILER _eCompiler) noexcept
{
	// declare)
	std::string_view result;

	switch(_eCompiler)
	{
	case	CGDK::eCOMPILER::NOT_DEFINED:		result	 = "not defined"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_8:	result	 = "Visual Studio 2005"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_9:	result	 = "Visual Studio 2008"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_10:	result	 = "Visual Studio 2010"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_11:	result	 = "Visual Studio 2012"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_12:	result	 = "Visual Studio 2013"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_14:	result	 = "Visual Studio 2015"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_15:	result	 = "Visual Studio 2017"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_16:	result	 = "Visual Studio 2019"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_17:	result	 = "Visual Studio 2022"; break;
	case	CGDK::eCOMPILER::ECLIPS:			result	 = "ECLIPS"; break;
	case	CGDK::eCOMPILER::GCC:				result	 = "gcc"; break;
	case	CGDK::eCOMPILER::ALL:				result	 = "all"; break;
	default:									result	 = "Unknowned"; break;
	}

	// return)
	return result;
}

template<>
std::wstring_view to_string< wchar_t>(CGDK::eCOMPILER _eCompiler) noexcept
{
	// declare)
	std::wstring_view result;

	switch (_eCompiler)
	{
	case	CGDK::eCOMPILER::NOT_DEFINED:		result	 = L"not defined"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_8:	result	 = L"Visual Studio 2005"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_9:	result	 = L"Visual Studio 2008"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_10:	result	 = L"Visual Studio 2010"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_11:	result	 = L"Visual Studio 2012"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_12:	result	 = L"Visual Studio 2013"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_14:	result	 = L"Visual Studio 2015"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_15:	result	 = L"Visual Studio 2017"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_16:	result	 = L"Visual Studio 2019"; break;
	case	CGDK::eCOMPILER::VISUALSTUDIO_17:	result	 = L"Visual Studio 2022"; break;
	case	CGDK::eCOMPILER::ECLIPS:			result	 = L"ECLIPS"; break;
	case	CGDK::eCOMPILER::GCC:				result	 = L"gcc"; break;
	case	CGDK::eCOMPILER::ALL:				result	 = L"all"; break;
	default:									result	 = L"Unknowned"; break;
	}

	// return)
	return result;
}

template<>
std::string_view to_string<char>(CGDK::eCONFIGURATION _configuration) noexcept
{
	// declare)
	std::string_view result;

	switch (_configuration)
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

template<>
std::wstring_view to_string<wchar_t>(CGDK::eCONFIGURATION _configuration) noexcept
{
	// declare)
	std::wstring_view result;

	switch (_configuration)
	{
	case	CGDK::eCONFIGURATION::NOT_DEFINED:		result = L"not defined"; break;
	case	CGDK::eCONFIGURATION::DEBUG_:			result = L"DEBUG"; break;
	case	CGDK::eCONFIGURATION::DEBUG_DLL_:		result = L"DEBUG DLL"; break;
	case	CGDK::eCONFIGURATION::RELEASE_:			result = L"RELEASE"; break;
	case	CGDK::eCONFIGURATION::RELEASE_DLL_:		result = L"RELEASE DLL"; break;
	case	CGDK::eCONFIGURATION::ALL_:				result = L"all"; break;
	default:										result = L"Unknowned"; break;
	}

	// return)
	return result;
}


template<>
std::string_view to_string<char>(CGDK::ePLATFORM _platform) noexcept
{
	// declare)
	std::string_view result;

	switch (_platform)
	{
	case	CGDK::ePLATFORM::NOT_DEFINED:			result = "not defined"; break;
	case	CGDK::ePLATFORM::WINDOWS_X86:			result	= "Windows x86"; break;
	case	CGDK::ePLATFORM::WINDOWS_X64:			result	= "Windows x64"; break;
	case	CGDK::ePLATFORM::LINUX:					result	= "Linux"; break;
	case	CGDK::ePLATFORM::ANDROID:				result	= "Android"; break;
	case	CGDK::ePLATFORM::MACOS:					result	= "Macos"; break;
	case	CGDK::ePLATFORM::IOS:					result	= "ios"; break;
	case	CGDK::ePLATFORM::ALL:					result = "all"; break;
	default:										result	= "Unknowned"; break;
	}

	// return)
	return result;
}

template<>
std::wstring_view to_string<wchar_t>(CGDK::ePLATFORM _platform) noexcept
{
	// declare)
	std::wstring_view result;

	switch (_platform)
	{
	case	CGDK::ePLATFORM::NOT_DEFINED:			result = L"not defined"; break;
	case	CGDK::ePLATFORM::WINDOWS_X86:			result = L"Windows x86"; break;
	case	CGDK::ePLATFORM::WINDOWS_X64:			result = L"Windows x64"; break;
	case	CGDK::ePLATFORM::LINUX:					result = L"Linux"; break;
	case	CGDK::ePLATFORM::ANDROID:				result = L"Android"; break;
	case	CGDK::ePLATFORM::MACOS:					result = L"Macos"; break;
	case	CGDK::ePLATFORM::IOS:					result = L"ios"; break;
	case	CGDK::ePLATFORM::ALL:					result = L"all"; break;
	default:										result = L"Unknowned"; break;
	}

	// return)
	return result;
}

template<>
std::string_view to_string<char>(CGDK::eLICENSE_TYPE _license_type) noexcept
{
	// declare)
	std::string_view result;

	switch (_license_type)
	{
	case	CGDK::eLICENSE_TYPE::NOT_DEFINED:		result = "not defined"; break;
	case	CGDK::eLICENSE_TYPE::FREE:				result = "free"; break;
	case	CGDK::eLICENSE_TYPE::INDIE:				result = "indie"; break;
	case	CGDK::eLICENSE_TYPE::LIMITED_PROJECT:	result = "project limited"; break;
	case	CGDK::eLICENSE_TYPE::LIMITED_COMPPANY:	result = "company limited"; break;
	case	CGDK::eLICENSE_TYPE::UNLIMITED:			result = "unlimited"; break;
	case	CGDK::eLICENSE_TYPE::DEVELOPER:			result = "developer"; break;
	default:										result = "unknowned"; break;
	}

	// return) 
	return result;
}

template<>
std::wstring_view to_string<wchar_t>(CGDK::eLICENSE_TYPE _license_type) noexcept
{
	// declare)
	std::wstring_view result;

	switch (_license_type)
	{
	case	CGDK::eLICENSE_TYPE::NOT_DEFINED:		result = L"not defined"; break;
	case	CGDK::eLICENSE_TYPE::FREE:				result = L"Free"; break;
	case	CGDK::eLICENSE_TYPE::INDIE:				result = L"Indie"; break;
	case	CGDK::eLICENSE_TYPE::LIMITED_PROJECT:	result = L"Project limited"; break;
	case	CGDK::eLICENSE_TYPE::LIMITED_COMPPANY:	result = L"Company limited"; break;
	case	CGDK::eLICENSE_TYPE::UNLIMITED:			result = L"Unlimited"; break;
	case	CGDK::eLICENSE_TYPE::DEVELOPER:			result = L"Developer"; break;
	default:										result = L"Unknowned"; break;
	}

	// return) 
	return result;
}

int get_version() noexcept
{
	return SVN_COMMIT_REVISION;
}

// 13) CGDK Component Info
sLICENSE_INFO& _get_license_info() noexcept
{
	static sLICENSE_INFO license_info;
	return license_info;
}

const sLICENSE_INFO& get_license_info() noexcept
{
	return _get_license_info();
}

std::vector<sCOMPONENT_LICENSE>& _get_component_info() noexcept
{
	static std::vector<sCOMPONENT_LICENSE> vector_component;
	return vector_component;
}
const std::vector<sCOMPONENT_LICENSE>& get_component_info() noexcept
{
	return _get_component_info();
}

void add_component_info(sCOMPONENT_LICENSE&& _component) noexcept
{
	_get_component_info().emplace_back(std::move(_component));
}

sCOMPONENT_LICENSE get_component_info(std::string_view _name) noexcept
{
	auto vector_component = _get_component_info();

	for (auto& iter : vector_component)
	{
		if (iter.component_info.name == _name)
		{
			return iter;
		}
	}

	return sCOMPONENT_LICENSE();
}

bool _has_expired_component(chrono::time::utc::time_point _time_now) noexcept
{
	auto vector_component = _get_component_info();

	for (auto& iter : vector_component)
	{
		if (iter.license_type == eLICENSE_TYPE::FREE)
			return true;

		if (iter.license_type != eLICENSE_TYPE::UNLIMITED && iter.license_type != eLICENSE_TYPE::DEVELOPER)
		{
			if (iter.component_info.time_expire > _time_now)
			{
				return true;
			}
		}
	}

	return false;
}

bool get_license_alert() noexcept
{
	static std::atomic<int> enable_license_aleart = 0;
	return 1 == (++enable_license_aleart);
}

std::string_view get_string_license() noexcept
{
	return	"copyright 2015 CHO SANGHYUN. all rights reserved";
}

uint16_t GET_NUMBER(char*& _strNow)
{
	while(*_strNow==' ')
	{
		++_strNow;
	}

	char*	str_value = _strNow;

	while (*_strNow != ' ')
	{
		if (*_strNow == 0)
			break;

		++_strNow;
	}
	*_strNow  = 0;
	++_strNow;

	// return)
	return __from_string<uint16_t>(str_value);
}

uint16_t GET_MONTH(char*& _strNow)
{
	char* str_value = _strNow;
	_strNow[3] = 0;
	_strNow	+= 4;

	std::array<const char*, 12> str_compare {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	int	month = 0;

	for(; month<12; ++month)
	{
		if (strcmp(str_value, str_compare.at(month)) == 0)
		{
			++month;
			break;
		}
	}

	// return)
	return static_cast<uint16_t>(month);
}

uint16_t GET_VALUE(char*& _strNow)
{
	char* str_value = _strNow;

	while (*_strNow != ':')
	{
		if (*_strNow == 0)
			break;

		++_strNow;
	}
	*_strNow  = 0;
	++_strNow;

	// return)
	return __from_string<uint16_t>(str_value);
}

chrono::time::utc::time_point get_build_date(const std::string_view _str_date, const std::string_view _str_time)
{
	// declare) 
	chrono::date_time time_temp;

	{
		char strDate[256];
		__strcpy(strDate, _str_date.data());

		char* str_now = strDate;
		time_temp.t_month = GET_MONTH(str_now);
		time_temp.t_day   = GET_NUMBER(str_now);
		time_temp.t_year  = GET_NUMBER(str_now);
	}

	// 2) Hour/Minute/Second/Millisecond는 0로 리셋한다.
	{
		char strTime[256];
	#ifdef _WIN32
		strcpy_s(strTime, _str_time.data());
	#else
		strcpy(strTime, _str_time.data());
	#endif

		char* str_now = strTime;

		time_temp.t_hour     = GET_VALUE(str_now);
		time_temp.t_minute   = GET_VALUE(str_now);
		time_temp.t_second   = GET_VALUE(str_now);
		time_temp.t_fraction = 0;
	}

	// return) 
	return chrono::from_date_time<chrono::time::utc::time_point>(time_temp);
}


}