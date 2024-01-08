#include "pch.h"

using namespace CGDK;


#if defined(_WIN32)

// 정확한 OS Version을 구하기 위한 처리 함수.
////#include <VersionHelpers.h>
//#include <lmwksta.h>
//#include <lmerr.h>
//#include <LMAPIbuf.h>
//
//#include <Lm.h>
//#pragma comment(lib, "netapi32.lib")
//
//bool GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor)
//{
//    static DWORD dwMajorCache = 0, dwMinorCache = 0;
//    if (0 != dwMajorCache)
//    {
//        dwMajor = dwMajorCache;
//        dwMinor = dwMinorCache;
//        return true;
//    }
//
//    LPWKSTA_INFO_100 pBuf = nullptr;
//    if (NERR_Success != NetWkstaGetInfo(nullptr, 100, (LPBYTE*)&pBuf))
//        return false;
//
//    dwMajor = dwMajorCache = pBuf->wki100_ver_major;
//    dwMinor = dwMinorCache = pBuf->wki100_ver_minor;
//    NetApiBufferFree(pBuf);
//
//    return true;
//}
//
//bool GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor, DWORD& dwServicePack)
//{
//    if (!GetWindowsVersion(dwMajor, dwMinor))
//        return false;
//
//    static DWORD dwServicePackCache = ULONG_MAX;
//    if (ULONG_MAX != dwServicePackCache)
//    {
//        dwServicePack = dwServicePackCache;
//        return true;
//    }
//
//    const int nServicePackMax = 10;
//    OSVERSIONINFOEX osvi;
//    DWORDLONG dwlConditionMask = 0;
//
//    __zero_memory(&osvi, sizeof(OSVERSIONINFOEX));
//    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
//    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_EQUAL);
//
//    for (int i = 0; i < nServicePackMax; ++i)
//    {
//        osvi.wServicePackMajor = static_cast<WORD>(i);
//        if (VerifyVersionInfo(&osvi, VER_SERVICEPACKMAJOR, dwlConditionMask))
//        {
//            dwServicePack = dwServicePackCache = i;
//            return true;
//        }
//    }
//
//	// return) 
//    return false;
//}


uint32_t api::get_service_path_name(char* _str_service_name, char* _str_binary_path_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name == nullptr, 0);

	// declare) 
	char strModuleFileName[MAX_PATH] { 0, };

	// 1) Module File Name을 얻언내다.
	auto result = GetModuleFileNameA(NULL, strModuleFileName, MAX_PATH);
	
	// check) 함수 호출이 실패했다면 0을 return한다.
	RETURN_IF(result == 0, 0);

	// 3) PathName을 복사한다.
	if(_str_binary_path_name != nullptr)
	{
		__strcpy(_str_binary_path_name, MAX_PATH, strModuleFileName);
	}

	// 4) 초기화
	_str_service_name[0] = NULL;

	// 5) String의 길이를 얻는다.
	size_t iStrLength = __strlen(strModuleFileName);

	// 6) File 이름부분만을 얻어낸다.
	int i = static_cast<int>(iStrLength) - 1;
	for(; i>= 0; --i) 
	{
		if(strModuleFileName[i] == '\\') 
		{
			__strcpy(_str_service_name, MAX_PATH, strModuleFileName + (i+1));
			break;
		}
	}

	// 7) '\\'를 찾지 못했을 경우 그냥 통체로 복사한다.
	if (i<0)
	{
		__strcpy(_str_service_name, MAX_PATH, strModuleFileName);
	}

	// 8) ServiceName을 얻는다.
	for (i = (int)__strlen(_str_service_name) - 1; i >= 0; --i)
	{
		if(_str_service_name[i] == '.') 
		{
			_str_service_name[i] = 0;
			break;
		}
	}

	// return)
	return	result;
}

uint32_t api::get_service_path_name(wchar_t* _str_service_name, wchar_t* _str_binary_path_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name == nullptr, 0);

	// declare) 
	wchar_t	strModuleFileName[MAX_PATH] { 0, };

	// 1) Module File Name을 얻언내다.
	auto result = GetModuleFileNameW(NULL, strModuleFileName, MAX_PATH);

	// check) 함수 호출이 실패했다면 0을 return한다.
	RETURN_IF(result == 0, 0);

	// 3) PathName을 복사한다.
	if (_str_binary_path_name != nullptr)
	{
		__strcpy(_str_binary_path_name, MAX_PATH, strModuleFileName);
	}

	// 4) 초기화
	_str_service_name[0] = NULL;

	// 5) String의 길이를 얻는다.
	size_t	iStrLength = __strlen(strModuleFileName);

	// 6) File 이름부분만을 얻어낸다.
	int i = (int)iStrLength - 1;
	for (; i >= 0; --i)
	{
		if (strModuleFileName[i] == '\\')
		{
			__strcpy(_str_service_name, MAX_PATH, strModuleFileName + (i + 1));
			break;
		}
	}

	// 7) '\\'를 찾지 못했을 경우 그냥 통체로 복사한다.
	if (i<0)
	{
		__strcpy(_str_service_name, MAX_PATH, strModuleFileName);
	}

	// 8) ServiceName을 얻는다.
	for (i = (int)__strlen(_str_service_name) - 1; i >= 0; --i)
	{
		if (_str_service_name[i] == '.')
		{
			_str_service_name[i] = 0;
			break;
		}
	}

	// return)
	return	result;
}

bool api::service_install(
	std::string_view	_str_service_display_name,
	std::string_view	_str_service_description,
	uint32_t			_desired_access,
	uint32_t			_service_type,
	uint32_t			_start_type,
	uint32_t			_error_control) noexcept
{
	try
	{
		// declare) 
		char str_service_name[MAX_PATH]{ 0, };
		char str_binary_path_name[MAX_PATH]{ 0, };

		// 1) Service Name과 Binary Path Name을 얻어온다.
		auto result = get_service_path_name(str_service_name, str_binary_path_name);

		// check) 
		RETURN_IF(result == 0, FALSE);

		// 2) 만약 DisplayName이 없으면 Service 이름과 같도록한다.
		if(_str_service_display_name.empty())
		{
			_str_service_display_name	 = str_service_name;
		}

		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 3) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);
    
		// declare) 
		OBJ<api::service_handle> temp_service;

		// 4) Service를 생성한다.
		temp_service.create(sSCManager, str_service_name, _str_service_display_name,
			_desired_access, 
			_service_type, 
			_start_type, 
			_error_control, 
			str_binary_path_name);
    
		// 5) Service Description을 설정한다.
		if(_str_service_description.empty() == false)
		{
			temp_service.set_description(_str_service_description);
		}
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_install(
	std::wstring_view	_str_service_display_name,
	std::wstring_view	_str_service_description,
	uint32_t			_desired_access,
	uint32_t			_service_type,
	uint32_t			_start_type,
	uint32_t			_error_control) noexcept
{
	try
	{
		// declare) 
		wchar_t	str_service_name[MAX_PATH] = { 0, };
		wchar_t	str_binary_path_name[MAX_PATH] = { 0, };

		// 1) Service Name과 Binary Path Name을 얻어온다.
		auto result = get_service_path_name(str_service_name, str_binary_path_name);

		// check) 
		RETURN_IF(result == 0, FALSE);

		// 2) 만약 DisplayName이 없으면 Service 이름과 같도록한다.
		if(_str_service_display_name.empty())
		{
			_str_service_display_name = str_service_name;
		}

		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 3) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);
    
		// declare) 
		OBJ<api::service_handle> temp_service;

		// 4) Service를 생성한다.
		temp_service.create(sSCManager, str_service_name, _str_service_display_name,
			_desired_access, 
			_service_type, 
			_start_type, 
			_error_control, 
			str_binary_path_name);
    
		// 5) Service Description을 설정한다.
		if(_str_service_description.empty() == false)
		{
			temp_service.set_description(_str_service_description);
		}
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return) 
    return true;
}

bool api::service_uninstall() noexcept
{
	try
	{
		// declare) 
		wchar_t str_service_name[MAX_PATH]{ 0 };

		// 1) Service Name과 Binary Path Name을 얻어온다.
		auto result = get_service_path_name(str_service_name);

		// check) 
		RETURN_IF(result == 0, FALSE);

		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 2) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_CREATE_SERVICE);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 3) Service를 open한다.
		temp_service.open(sSCManager, str_service_name, SERVICE_ALL_ACCESS);

		// 4) Service를 지운다.
		temp_service.destroy();
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_start(std::string_view _str_service_name, uint32_t _dwNumServiceArgs, const char** _lpServiceArgVectors) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.start(_dwNumServiceArgs, _lpServiceArgVectors);
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_start(std::wstring_view _str_service_name, uint32_t _dwNumServiceArgs, const wchar_t** _lpServiceArgVectors) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.start(_dwNumServiceArgs, _lpServiceArgVectors);
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_stop(std::string_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.stop();
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_stop(std::wstring_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.stop();
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_pause(std::string_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.pause();
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_pause(std::wstring_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.pause();
	}
	catch(...)
	{
		// return)
		return	false;
	}

	// return) 
    return true;
}

bool api::service_continue(std::string_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.resume();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return) 
    return true;
}

bool api::service_continue(std::wstring_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), false);

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 3) Service를 시작한다.
		temp_service.resume();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return) 
    return true;
}

// 3) Status
uint32_t api::service_status(std::string_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), ERROR_SUCCESS);

	// declare) 
	uint32_t current_status = ERROR_SUCCESS;

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 2) Service를 지운다.
		current_status = temp_service.get_status();

		// check) 
		RETURN_IF(current_status == ERROR_SUCCESS, ERROR_SUCCESS);

		// return) 
		return current_status;
	}
	catch(...)
	{
	}

	// return) 
	return current_status;
}

uint32_t api::service_status(std::wstring_view _str_service_name) noexcept
{
	// check) 
	RETURN_IF(_str_service_name.empty(), ERROR_SUCCESS);

	// declare) 
	uint32_t current_status = ERROR_SUCCESS;

	try
	{
		// declare) 
		OBJ<api::service_handle> sSCManager;

		// 1) SCManager를 open한다.
		sSCManager.open_SC_manager(SC_MANAGER_ALL_ACCESS);

   		// declare) 
		OBJ<api::service_handle> temp_service;

		// 2) Service를 지운다.
		temp_service.open(sSCManager, _str_service_name, SERVICE_ALL_ACCESS);

		// 2) Service를 지운다.
		current_status = temp_service.get_status();

		// check) 
		RETURN_IF(current_status == ERROR_SUCCESS, ERROR_SUCCESS);

		// return) 
		return current_status;
	}
	catch(...)
	{
	}

	// return) 
	return current_status;
}
#elif defined(__linux__)
extern char* __progname;
uint32_t api::get_service_path_name(char* _str_service_name, char* /*_str_binary_path_name*/) noexcept
{
	// check)
	RETURN_IF(__progname == nullptr, 0);

	strcpy(_str_service_name, __progname);

	// return) 
	return 1;
}

uint32_t api::get_service_path_name(wchar_t* /*_str_service_name*/, wchar_t* /*_str_binary_path_name*/) noexcept
{
	return 1;
}

bool api::service_install(
	std::string_view	/*_str_service_display_name*/,
	std::string_view	/*_str_service_description*/,
	uint32_t			/*_desired_access*/,
	uint32_t			/*_service_type*/,
	uint32_t			/*_start_type*/,
	uint32_t			/*_error_control*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_install(
	std::wstring_view	/*_str_service_display_name*/,
	std::wstring_view	/*_str_service_description*/,
	uint32_t			/*_desired_access*/,
	uint32_t			/*_service_type*/,
	uint32_t			/*_start_type*/,
	uint32_t			/*_error_control*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_uninstall() noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_start(std::string_view /*_str_service_name*/, uint32_t /*_dwNumServiceArgs*/, const char** /*_lpServiceArgVectors*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_start(std::wstring_view /*_str_service_name*/, uint32_t /*_dwNumServiceArgs*/, const wchar_t** /*_lpServiceArgVectors*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_stop(std::string_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_stop(std::wstring_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_pause(std::string_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_pause(std::wstring_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_continue(std::string_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

bool api::service_continue(std::wstring_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

uint32_t api::service_status(std::string_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

uint32_t api::service_status(std::wstring_view /*_str_service_name*/) noexcept
{
	CGASSERT_ERROR(false);
	return	false;
}

#else
#endif
