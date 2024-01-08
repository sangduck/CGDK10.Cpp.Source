#include "pch.h"

using namespace CGDK;


#if defined (_WIN32)
api::service_handle::service_handle() noexcept :
	m_hService(NULL)
{
}


api::service_handle::~service_handle() noexcept
{
	close();
}

void api::service_handle::open_SC_manager(uint32_t _desired_access)
{
	open_SC_manager((const wchar_t*)nullptr, (const wchar_t*)nullptr, _desired_access);
}

void api::service_handle::open_SC_manager(std::string_view _lpMachineName, std::string_view _lpDatabaseName, uint32_t _dwDesiredAccess)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service handle is aleady created"sv);

	// declare) 
    SC_HANDLE hSCManager; 

	// 1) SCManager를 open한다.
	hSCManager = ::OpenSCManagerA(_lpMachineName.data(), _lpDatabaseName.data(), _dwDesiredAccess);
    
	// check) Windows Vista에서는 "관리자 권한"이 없는 경우
	THROW_IF(hSCManager == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to open SCManager"sv);

	// 2) Handle을 설정한다.
	m_hService = hSCManager;
}

void api::service_handle::open_SC_manager(std::wstring_view _lpMachineName, std::wstring_view _lpDatabaseName, uint32_t _dwDesiredAccess)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service handle is aleady created"sv);

	// declare) 
    SC_HANDLE hSCManager; 

	// 1) SCManager를 open한다.
	hSCManager = ::OpenSCManagerW(_lpMachineName.data(), _lpDatabaseName.data(), _dwDesiredAccess);
    
	// check) Windows Vista에서는 "관리자 권한"이 없는 경우
	THROW_IF(hSCManager == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to open SCManager"sv);

	// 2) Handle을 설정한다.
	m_hService	 = hSCManager;
}

void api::service_handle::open(SC_HANDLE p_hSCManager, std::string_view _lpServiceName, uint32_t _dwDesiredAccess)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service handle is aleady created"sv);

	// declare) 
    SC_HANDLE	hService; 

	// 1) SCManager를 open한다.
	hService = ::OpenServiceA(p_hSCManager, _lpServiceName.data(), _dwDesiredAccess);
    
	// check) Windows Vista에서는 "관리자 권한"이 없는 경우
	THROW_IF(hService == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to open SCManager"sv);

	// 2) Handle을 설정한다.
	m_hService	 = hService;
}

void api::service_handle::open(SC_HANDLE p_hSCManager, std::wstring_view _lpServiceName, uint32_t _dwDesiredAccess)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service handle is aleady created"sv);

	// declare) 
    SC_HANDLE	hService; 

	// 1) SCManager를 open한다.
	hService = ::OpenServiceW(p_hSCManager, _lpServiceName.data(), _dwDesiredAccess);
    
	// check) Windows Vista에서는 "관리자 권한"이 없는 경우
	THROW_IF(hService == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to open SCManager"sv);

	// 2) Handle을 설정한다.
	m_hService	 = hService;
}

void api::service_handle::create(
	SC_HANDLE				_hSCManager,
	std::string_view		_lpServiceName,
	std::string_view		_lpDisplayName,
	uint32_t				_desired_access,
	uint32_t				_service_type,
	uint32_t				_start_type,
	uint32_t				_error_control,
	std::string_view		_lpBinaryPathName,
	std::string_view		_lpLoadOrderGroup,
	uint32_t*				_lpdwTagId,
	std::string_view		_lpDependencies,
	std::string_view		_lpServiceStartName,
	std::string_view		_lpPassword)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service Handle is aleady created"sv);

	// 1) Service를 생성한다.
	auto hService = ::CreateServiceA(
		_hSCManager,
		_lpServiceName.data(),
		_lpDisplayName.data(), 
		_desired_access, 
		_service_type, 
		_start_type, 
		_error_control, 
		_lpBinaryPathName.data(),
		_lpLoadOrderGroup.data(),
		(LPDWORD)_lpdwTagId,
		_lpDependencies.data(),
		_lpServiceStartName.data(),
		_lpPassword.data());
    
	// check) Service의 생성에 실패했으면 Exception을 던진다.
	THROW_IF(hService == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to create Service"sv);

	// 2) Handle을 설정한다.
	m_hService = hService;
}

void api::service_handle::create(
	SC_HANDLE				_hSCManager,
	std::wstring_view		_lpServiceName,
	std::wstring_view		_lpDisplayName,
	uint32_t				_desired_access,
	uint32_t				_service_type,
	uint32_t				_start_type,
	uint32_t				_error_control,
	std::wstring_view		_lpBinaryPathName,
	std::wstring_view		_lpLoadOrderGroup,
	uint32_t*				_lpdwTagId,
	std::wstring_view		_lpDependencies,
	std::wstring_view		_lpServiceStartName,
	std::wstring_view		_lpPassword)
{
	// check) 
	THROW_IF(m_hService != NULL, throwable::failure(eRESULT::NO_INITIALIZED), LOG_ERROR << "(err ) service Handle is aleady created"sv);

	// declare) 
    SC_HANDLE	hService; 

	// 1) Service를 생성한다.
	hService = ::CreateServiceW(
		_hSCManager,
		_lpServiceName.data(),
		_lpDisplayName.data(), 
		_desired_access, 
		_service_type, 
		_start_type, 
		_error_control, 
		_lpBinaryPathName.data(),
		_lpLoadOrderGroup.data(),
		(LPDWORD)_lpdwTagId,
		_lpDependencies.data(),
		_lpServiceStartName.data(),
		_lpPassword.data());
    
	// check) Service의 생성에 실패했으면 Exception을 던진다.
	THROW_IF(hService == NULL, throwable::failure(eRESULT::ACCESS_DENIED), LOG_ERROR << "(err ) fail to create service"sv);

	// 2) Handle을 설정한다.
	m_hService = hService;
}

bool api::service_handle::destroy() noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) Service를 지운다.
    return ::DeleteService(m_hService) != FALSE;
}

bool api::service_handle::close() noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) SC Manager를 닫는다.
    return ::CloseServiceHandle(m_hService)!=FALSE;
}

bool api::service_handle::control(uint32_t _dwControl, LPSERVICE_STATUS _lpServiceStatus) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) Service를 start한다.
	return ::ControlService(m_hService, _dwControl, _lpServiceStatus)!=FALSE;
}

bool api::service_handle::start(uint32_t _num_service_args, _In_reads_opt_(_num_service_args) const char** _lpServiceArgVectors) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) Service를 start한다.
	return ::StartServiceA(m_hService, _num_service_args, _lpServiceArgVectors)!=FALSE;
}

bool api::service_handle::start(uint32_t _num_service_args, _In_reads_opt_(_num_service_args) const wchar_t** _lpServiceArgVectors) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) Service를 start한다.
	return ::StartServiceW(m_hService, _num_service_args, _lpServiceArgVectors)!=FALSE;
}

bool api::service_handle::stop() noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// declare) 
	SERVICE_STATUS ssStatus;

	// 1) Service 제어
	auto result = control(SERVICE_CONTROL_STOP, &ssStatus);

	// check) 
	RETURN_IF(result == FALSE, false);

	while((eOBJECT_STATE)ssStatus.dwCurrentState==eOBJECT_STATE::STOP_PENDING)
	{
		// - 잠시 대기
		api::sleep_for(100ms);

		// - 상태를 다시 Query
		result = query_status(&ssStatus);

		// check) 
		RETURN_IF(result == FALSE, false);
	}

	// return)
	return true;
}

bool api::service_handle::pause() noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// declare) 
	SERVICE_STATUS ssStatus;

	// 1) Service 제어
	auto result = control(SERVICE_CONTROL_PAUSE, &ssStatus);

	// check) 
	RETURN_IF(result == FALSE, false);

	// return)
	return true;
}

bool api::service_handle::resume() noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// declare) 
	SERVICE_STATUS ssStatus;

	// 1) Service 제어
	auto result = control(SERVICE_CONTROL_CONTINUE, &ssStatus);

	// check) 
	RETURN_IF(result == FALSE, false);

	// return)
	return true;
}

bool api::service_handle::get_status(LPSERVICE_STATUS _lpServiceStatus) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// check) _lpServiceStatus이 nullptr이면 false를 리턴한다.
	RETURN_IF(_lpServiceStatus == nullptr, false);

	// 1) Service 제어
	auto result = control(SERVICE_CONTROL_INTERROGATE, _lpServiceStatus);

	// check) 
	RETURN_IF(result == FALSE, false);

	// return)
	return true;
}

uint32_t api::service_handle::get_status() noexcept
{
	// declare) 
	SERVICE_STATUS sServiceStatus;

	// 1) 상태를 얻는다.
	auto result = get_status(&sServiceStatus);

	// check)
	RETURN_IF(result == FALSE, ERROR_SUCCESS);

	// return) 현재의 상태를 리턴한다.
	return sServiceStatus.dwCurrentState;
}

bool api::service_handle::query_status(LPSERVICE_STATUS _lpServiceStatus) noexcept
{
	// check) 
	RETURN_IF(m_hService == NULL, false);

	// 1) Service를 start한다.
	return ::QueryServiceStatus(m_hService, _lpServiceStatus)!=FALSE;
}

bool api::service_handle::set_config(uint32_t p_dwInfoLevel, void* _lpInfo) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// 1) Service Config를 수정한다.
	return ::ChangeServiceConfig2(m_hService, p_dwInfoLevel, _lpInfo)!=FALSE;
}

bool api::service_handle::set_description(std::string_view _str_service_description) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// declare) 
	SERVICE_DESCRIPTIONA sd;

	// 1) Description을 설정한다.
	sd.lpDescription = (char*)_str_service_description.data();
	
	// 2) Description을 수정한다.
	auto result = ::ChangeServiceConfig2A(m_hService, SERVICE_CONFIG_DESCRIPTION, &sd);

	// check) 
	RETURN_IF(result == FALSE, false);

	// return) 
	return true;
}

bool api::service_handle::set_description(std::wstring_view _str_service_description) noexcept
{
	// check) 이미 생성되어 있으면 그냥 끝낸다.
	RETURN_IF(m_hService == NULL, false);

	// declare) 
	SERVICE_DESCRIPTIONW sd;

	// 1) Description을 설정한다.
	sd.lpDescription = (wchar_t*)_str_service_description.data();
	
	// 2) Description을 수정한다.
	auto result = ::ChangeServiceConfig2W(m_hService, SERVICE_CONFIG_DESCRIPTION, &sd);

	// check) 
	RETURN_IF(result == FALSE, false);

	// return) 
	return true;
}

#elif defined(__linux)
api::service_handle::service_handle()
{
}

api::service_handle::~service_handle()
{
}

#else

#endif