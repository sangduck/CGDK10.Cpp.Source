#include "pch.h"
#include <cstring>

using namespace CGDK;


#if defined(_WIN32)

api::process_info::process_info() :
	m_process_time(0),
	m_hquery(0),
	m_hCounter_ProcessTime(NULL)
{
}

api::process_info::~process_info()
{
	this->close_process_info();
}

BOOL api::process_info::init_process_info(std::string_view _process)
{
	// declare) 
	PDH_STATUS result;

	// 1) Query Handle을 얻어온다.
	result = PdhOpenQuery( nullptr, 0, &this->m_hquery);

	// trace) 
	EXECUTE_IF(result != ERROR_SUCCESS, LOG_ERROR << "(err ) failure to add counter"sv);

	// declare) 
	char tempCounterPath[256];

	// 1) Processor Time Handle얻기
	wsprintfA(tempCounterPath, "\\Process(%s)\\% Processor Time", _process.data());

	// 2) Add Querier를 한다~~
    result = ::PdhAddCounterA(this->m_hquery, tempCounterPath, 0, &this->m_hCounter_ProcessTime);

	// check) 
	CGASSERT(result == ERROR_SUCCESS, FALSE);

	// return) 
	return TRUE;
}

BOOL api::process_info::init_process_info(std::wstring_view _process)
{
	// declare) 
	PDH_STATUS result;

	// 1) Query Handle을 얻어온다.
	result = PdhOpenQuery( nullptr, 0, &this->m_hquery);

	// trace) 
	EXECUTE_IF(result != ERROR_SUCCESS, LOG_ERROR << "(err ) failure to add counter"sv);

	// declare) 
	wchar_t tempCounterPath[256];

	// 1) Processor Time Handle얻기
	wsprintfW(tempCounterPath, L"\\Process(%s)\\% Processor Time", _process.data());

	// 2) Add Querier를 한다~~
    result = ::PdhAddCounterW(this->m_hquery, tempCounterPath, 0, &this->m_hCounter_ProcessTime);

	// check) 
	CGASSERT(result == ERROR_SUCCESS, FALSE);

	// return) 
	return TRUE;
}


VOID api::process_info::close_process_info() noexcept
{
	// 1) Query를 close한다.
	PdhCloseQuery(this->m_hquery);
}



PDH_STATUS api::process_info::get_counter(std::string_view _path, size_t _length_max)
{
	// declare) 
	PDH_BROWSE_DLG_CONFIG_A	BrowseDlgData;

	// 1) 구조체 memset
	std::memset(&BrowseDlgData, 0, sizeof(PDH_BROWSE_DLG_CONFIG_A));

	// 2) initialize the browser dialog window settings.
	BrowseDlgData.bIncludeInstanceIndex		 = FALSE;   
	BrowseDlgData.bSingleCounterPerAdd		 = TRUE;
	BrowseDlgData.bSingleCounterPerDialog	 = TRUE;  
	BrowseDlgData.bLocalCountersOnly		 = FALSE;      
	BrowseDlgData.bWildCardInstances		 = TRUE;
	BrowseDlgData.bHideDetailBox			 = TRUE;
	BrowseDlgData.bInitializePath			 = FALSE;     
	BrowseDlgData.bDisableMachineSelection	 = FALSE;
	BrowseDlgData.bIncludeCostlyObjects		 = FALSE;
	BrowseDlgData.hWndOwner					 = NULL;   
	BrowseDlgData.bReserved					 = FALSE;           
	BrowseDlgData.szReturnPathBuffer		 = (char*)_path.data();
	BrowseDlgData.cchReturnPathLength		 = static_cast<uint32_t>(_length_max);
	BrowseDlgData.pCallBack					 = nullptr;   
	BrowseDlgData.dwCallBackArg				 = 0;
	BrowseDlgData.CallBackStatus			 = ERROR_SUCCESS;
	BrowseDlgData.dwDefaultDetailLevel		 = PERF_DETAIL_WIZARD;
	BrowseDlgData.szDialogBoxCaption		 = (LPSTR)"Select a counter to monitor.";

	// return) 
	return	::PdhBrowseCountersA(&BrowseDlgData);
}

PDH_STATUS api::process_info::get_counter(std::wstring_view _path, size_t _length_max)
{
	// declare) 
	PDH_BROWSE_DLG_CONFIG_W	BrowseDlgData;

	// 1) 구조체 zero_memory
	std::memset(&BrowseDlgData, 0, sizeof(PDH_BROWSE_DLG_CONFIG_W));

	// 2) initialize the browser dialog window settings.
	BrowseDlgData.bIncludeInstanceIndex		 = FALSE;   
	BrowseDlgData.bSingleCounterPerAdd		 = TRUE;
	BrowseDlgData.bSingleCounterPerDialog	 = TRUE;  
	BrowseDlgData.bLocalCountersOnly		 = FALSE;      
	BrowseDlgData.bWildCardInstances		 = TRUE;
	BrowseDlgData.bHideDetailBox			 = TRUE;
	BrowseDlgData.bInitializePath			 = FALSE;     
	BrowseDlgData.bDisableMachineSelection	 = FALSE;
	BrowseDlgData.bIncludeCostlyObjects		 = FALSE;
	BrowseDlgData.hWndOwner					 = NULL;   
	BrowseDlgData.bReserved					 = FALSE;           
	BrowseDlgData.szReturnPathBuffer		 = (wchar_t*)_path.data();
	BrowseDlgData.cchReturnPathLength		 = static_cast<uint32_t>(_length_max);
	BrowseDlgData.pCallBack					 = nullptr;   
	BrowseDlgData.dwCallBackArg				 = 0;
	BrowseDlgData.CallBackStatus			 = ERROR_SUCCESS;
	BrowseDlgData.dwDefaultDetailLevel		 = PERF_DETAIL_WIZARD;
	BrowseDlgData.szDialogBoxCaption		 = (LPWSTR)L"Select a counter to monitor.";

	// return) 
	return	::PdhBrowseCountersW(&BrowseDlgData);
}


void api::process_info::update_counter()
{
	// 1) Data를 Query해온다.
	auto result = PdhCollectQueryData(this->m_hquery);

	// trace) 
	ERROR_RETURN_IF(result != ERROR_SUCCESS, , LOG_ERROR << "(err ) failure to add counter"sv);

	// declare) 
	PDH_FMT_COUNTERVALUE fmt_value;

    // 2) Get the formatted version of the counter value
    ::PdhGetFormattedCounterValue(this->m_hCounter_ProcessTime, PDH_FMT_LONG | PDH_FMT_1000, 0, &fmt_value);

	// 3) 값을 대입한다.
	this->m_process_time = fmt_value.longValue;
}

#elif defined(__linux__)

#else
	#error	"[CGDK10] Not supported Android Platform"
#endif