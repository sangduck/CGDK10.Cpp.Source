#include "pch.h"

using namespace CGDK;


#if defined(_WIN32)

api::server_status_handle::server_status_handle() : 
	m_hservice_status(NULL)
{
}
	
api::server_status_handle::~server_status_handle() noexcept
{
}


uint32_t api::server_status_handle::register_service_ctrl_handle(std::string_view _lpServiceName, __callback LPHANDLER_FUNCTION_EX _lp_handler_proc, void* _pcontext)
{
	// 1) ServiceHandler를 등록한다.
	auto hServerStatus = ::RegisterServiceCtrlHandlerExA(_lpServiceName.data(), _lp_handler_proc, _pcontext);

	// check) 
	RETURN_IF(hServerStatus==NULL, GetLastError());

	// 2) 설정한다.
	m_hservice_status = hServerStatus;

	// Return 
	return ERROR_SUCCESS;
}

uint32_t api::server_status_handle::register_service_ctrl_handle(std::wstring_view _lpServiceName, __callback LPHANDLER_FUNCTION_EX _lp_handler_proc, void* _pcontext)
{
	// 1) ServiceHandler를 등록한다.
	auto hServerStatus = ::RegisterServiceCtrlHandlerExW(_lpServiceName.data(), _lp_handler_proc, _pcontext);

	// check) 
	RETURN_IF(hServerStatus == NULL, GetLastError());

	// 2) 설정한다.
	m_hservice_status = hServerStatus;

	// Return 
	return ERROR_SUCCESS;
}

bool api::server_status_handle::set_service_status(LPSERVICE_STATUS _lpServiceStatus)
{
	// check) 
	RETURN_IF(m_hservice_status==NULL, false);

	// 1) Service를 start한다.
	return	::SetServiceStatus(m_hservice_status, _lpServiceStatus)!=FALSE;
}

eOBJECT_STATE api::server_status_handle::set_service_status(eOBJECT_STATE _service_status, eOBJECT_STATE _control_accept)
{
	// check) 
	RETURN_IF(m_hservice_status == NULL, eOBJECT_STATE::NONE);

	// 1) ServiceStatus를 설정한다.
	SERVICE_STATUS sServiceStatus;

    sServiceStatus.dwServiceType			 = SERVICE_WIN32_OWN_PROCESS;
    sServiceStatus.dwCurrentState			 = (uint32_t)_service_status;
    sServiceStatus.dwControlsAccepted		 = (uint32_t)_control_accept;
    sServiceStatus.dwWin32ExitCode			 = 0;
    sServiceStatus.dwServiceSpecificExitCode = 0;
    sServiceStatus.dwCheckPoint				 = 0;
    sServiceStatus.dwWaitHint				 = 0;

	// 2) Service의 상태를 설정한다.
	auto result = ::SetServiceStatus(m_hservice_status, &sServiceStatus);

	// check) 실패했을 경우 Exception을 던진다.
	THROW_IF(result == FALSE, throwable::failure(eRESULT::BAD_FUNCTION_CALL), LOG_ERROR << "(err ) api: Fail to 'set_service_status'"sv);	// GetLastError()

	// return) 
	return (eOBJECT_STATE)sServiceStatus.dwCurrentState;
}


#elif defined(__linux__)
api::server_status_handle::server_status_handle() :
	m_hservice_status(0)
{
}

api::server_status_handle::~server_status_handle()
{
}


uint32_t api::server_status_handle::register_service_ctrl_handle(std::string_view /*_lpServiceName*/, __callback LPHANDLER_FUNCTION_EX /*_lp_handler_proc*/, void* /*_pcontext*/)
{
	return	0;
}

uint32_t api::server_status_handle::register_service_ctrl_handle(std::wstring_view /*_lpServiceName*/, __callback LPHANDLER_FUNCTION_EX /*_lp_handler_proc*/, void* /*_pcontext*/)
{
	return	0;
}


bool api::server_status_handle::set_service_status(LPSERVICE_STATUS /*_lpServiceStatus*/)
{
	return true;
}

eOBJECT_STATE api::server_status_handle::set_service_status(eOBJECT_STATE _service_status, eOBJECT_STATE /*_control_accept*/)
{
	return _service_status;
}

#else
#endif