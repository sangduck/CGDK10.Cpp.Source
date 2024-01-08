#include "pch.h"
#if defined(_WIN32)
	#include "Userenv.h"
	#include "Wtsapi32.h"
#elif defined(__linux__)
	#include <sys/wait.h>
	#include <pthread.h>
#else
#endif

server_entity::server_entity() :
	handle_process(INVALID_PROCESS_ID),
	status(0),
	is_preserve_start(false)
{
	//pobservable_websocket_server_overview_info = make_own<observable_websocket_server_overview_info>("observable_websocket_server_overview_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_overview_info);
	//pobservable_websocket_server_overview_info->m_pserver_entity = this;
	//pobservable_websocket_server_factory_info = make_own<observable_websocket_server_factory_info>("observable_websocket_server_factory_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_factory_info);
	//pobservable_websocket_server_factory_info->m_pserver_entity = this;
	//pobservable_websocket_server_executor_info = make_own<observable_websocket_server_executor_info>("observable_websocket_server_executor_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_executor_info);
	//pobservable_websocket_server_executor_info->m_pserver_entity = this;
	//pobservable_websocket_server_network_info = make_own<observable_websocket_server_network_info>("observable_websocket_server_network_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_network_info);
	//pobservable_websocket_server_network_info->m_pserver_entity = this;
	//pobservable_websocket_server_querier_info = make_own<observable_websocket_server_querier_info>("observable_websocket_server_querier_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_querier_info);
	//pobservable_websocket_server_querier_info->m_pserver_entity = this;
	//pobservable_websocket_server_crash_info = make_own<observable_websocket_server_crash_info>("observable_websocket_server_crash_info"); //Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_crash_info);
	//pobservable_websocket_server_crash_info->m_pserver_entity = this;
}

server_entity::~server_entity() noexcept
{
}

void server_entity::reset_system_status_info()
{
	// 1) Process를 닫는다.
	if(handle_process != INVALID_PROCESS_ID)
	{
	#if defined(_WIN32)
		CloseHandle(handle_process);
	#elif defined(__linux__)
		kill(handle_process, SIGINT);
	#else
		#error "CGDK Not supported platform"
	#endif
	}
	handle_process = INVALID_PROCESS_ID;

	// 2) Process의 사용률
	status_server.usage_process	 = 0;
	status_server.usage_memory	 = 0;
	status_server.usage_network	 = 0;

	// 4) ...
	perfornamce_processor.reset();
}

void server_entity::load(const char* p_str_path)
{
	info_server_register.load(p_str_path);
}

void server_entity::save()
{
	info_server_register.save();
}

#if defined(_WIN32)
bool create_process_to_session(const std::string_view _str_execute, const shared_buffer& _buf_environment, const std::string_view _str_path_working, char* _args, STARTUPINFOA* p_psi, PROCESS_INFORMATION* p_ppi, UINT _dwSessionID)
 {
	 // declare) 
	HANDLE handle_token_new = NULL;
	HANDLE handle_token_duplicate = NULL;

	// 1) UserToken을 Queyr한다.
	auto result = WTSQueryUserToken(_dwSessionID, &handle_token_new);

	// check) 
	ERROR_RETURN_IF(result == FALSE, false, LOG_PROGRESS << "(err ) fail to 'WTSQueryUserToken' (" << api::get_last_error() << ")");

	// 2) Token을 Duplicate한다.
	result = DuplicateTokenEx(handle_token_new,MAXIMUM_ALLOWED,NULL,SecurityIdentification,TokenPrimary,&handle_token_duplicate);

	// check) 
	if(result == FALSE)
	{
		CloseHandle(handle_token_new);
		return false;
	}

	// 3) Token 핸들을 닫는다.
	CloseHandle(handle_token_new);

	// declare) 
	uint32_t dwFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
	void*  penv_block = nullptr;

	// 4) EnvironmentBlock을 생성한다.
	if (_buf_environment.data() == nullptr)
	{
		result = CreateEnvironmentBlock(&penv_block, handle_token_duplicate, FALSE);

		// check)
		ERROR_RETURN_IF(result == FALSE, false, LOG_PROGRESS << "(err ) fail to 'CreateEnvironmentBlock' (" << api::get_last_error() << ")");
	}
	else
	{
		penv_block = _buf_environment.data();
	}

	// 5) Procsss를 연다.
    result = ::CreateProcessAsUserA(handle_token_duplicate, const_cast<char*>(_str_execute.data()), _args, nullptr, nullptr, FALSE, dwFlag, penv_block, const_cast<char*>(_str_path_working.data()), p_psi, p_ppi);

	// 6) Environemnt Block을 닫는다.
    if(_buf_environment.data() == nullptr && penv_block != nullptr)
	{
         DestroyEnvironmentBlock(penv_block);
	}

	// 7) Token Handle을 닫는다.
	CloseHandle(handle_token_duplicate);

	// return) 
	return result;
 }
#elif defined(__linux__)

struct FORK_DATA
{
	object_ptr<server_entity>	pserver_entity;
	std::string_view			str_execute;
	char*						environment_ptr;
	std::size_t					environment_bytes;
	std::string_view			str_path_working;
	char*						args;
};

void* fn_fork(void* _param)
{
	// 1) fork data
	FORK_DATA* pfork_data = reinterpret_cast<FORK_DATA*>(_param);

	// 2) fork
	auto pid = fork();

	// check) throw exception if failed
	RETURN_IF(pid < 0, nullptr);

	if(pid == 0)
	{
		// - 다중 쓰레드 상에서의 문제로 인해...
		resource::manager::detach_instance();
		factory::manager::detach_instance();
		factory::memory_block::detach();
		api::manager_thread::detach_instance();
		dump_manager::detach_instance();
		executor::manager::detach_instance();
		system_executor::detach_instance();
		net::io::connective::manager::detach_instance();
		net::io::binder::manager::detach_instance();
		net::io::add_on::Nsocket_sustain::detach_instance();
		net::io::restore_manager::detach_instance();
		net::io::connect_requestable::Ntcp_reconnect::detach_instance();

		// - change directorys and setsid
		signal(SIGHUP, SIG_IGN);

		// - close stdin/stdout/stderr ...
		close(0);
		close(1);
		close(2);

		// - root~ and set new session
		[[maybe_unused]] auto result = chdir( (pfork_data->str_path_working.empty() == false) ? pfork_data->str_path_working.data(): "/" );
		setsid();

		char* const argv[] {const_cast<char*>(pfork_data->str_execute.data()), 0};
		char* const env = nullptr;

		// - case child process => execv
		if(env == nullptr)
		{
			execv(pfork_data->str_execute.data(), argv);
		}
		else
		{
			execvpe(pfork_data->str_execute.data(), argv, &env);
		}

		// - exit
		exit(EXIT_FAILURE);
	}
	else
	{
		// - set info
		pfork_data->pserver_entity->handle_process = pid;
		pfork_data->pserver_entity->info_server_register.pid = pid;
		pfork_data->pserver_entity->status_server.pid = pid;

		// declare) 
		int ret = 0;

		// - wait util sub process is end
		::wait(&ret);
		
		// check)
		if(WIFEXITED(ret) == false || WIFSIGNALED(ret))
		{
			pfork_data->pserver_entity->handle_process = pid;
			pfork_data->pserver_entity->info_server_register.pid = pid;
			pfork_data->pserver_entity->status_server.pid = pid;
		}
	}

	// 3)
	delete pfork_data;

	// return)
	return nullptr;
}

bool create_process_to_session(server_entity* _pserver_entity, const std::string_view _str_execute, const shared_buffer& _buf_environment, const std::string_view _str_path_working, char* _args)
{
	// 1) set fork data
	FORK_DATA* pfork_data = new FORK_DATA;
	pfork_data->pserver_entity = _pserver_entity;
	pfork_data->str_execute = _str_execute;
	pfork_data->environment_ptr = _buf_environment.data();
	pfork_data->environment_bytes = _buf_environment.size();
	pfork_data->str_path_working = _str_path_working;
	pfork_data->args = _args;

	// declare)
	pthread_t temp_thread[2];

	// 2) begin thread
	auto thread_id = pthread_create(temp_thread, nullptr, fn_fork, reinterpret_cast<void*>(pfork_data));

	// check)
	RETURN_IF(thread_id < 0, false);

	// return) 
	return true;
}
#else
#endif

bool server_entity::request_init_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.exist(), false);

	// declare)
	char str_execute[512];

	// 1) 실행할 Full Path File을 설정한다.
	if (info_server_register.str_path.empty() == false)
	{
		if(info_server_register.str_path.back()=='/' || info_server_register.str_path.back() == '\\')
		{
			__sprintf(str_execute, "%s%s", info_server_register.str_path.c_str(), info_server_register.file_execute.c_str());
		}
		else
		{
			__sprintf(str_execute, "%s/%s", info_server_register.str_path.c_str(), info_server_register.file_execute.c_str());
		}
	}
	else
	{
		__sprintf(str_execute, "%s", info_server_register.file_execute.c_str());
	}

	// 2) Environment Block을 설정한다.
	if(info_server_register.buf_environment.size() == 0)
	{
		info_server_register.buf_environment.clear();
	}

#if defined(_WIN32)
	// 3) reset STARTUPINFO
	STARTUPINFOA sStartupInfo;
	ZeroMemory(&sStartupInfo, sizeof(sStartupInfo));
	sStartupInfo.cb = sizeof(sStartupInfo);

	// 4) reset PROCESS_INFORMATION
	PROCESS_INFORMATION	infoProcess;
	ZeroMemory(&infoProcess, sizeof(infoProcess));

	// declare) 
	BOOL result;

	// 3-A) Console mode가 아닐 경우(즉, ServiceMode일 경우)는 복잡하게 시작......
	if (GetConsoleWindow() == NULL)
	{
		// declare) 
		DWORD id_session;

		// - Session ID를 얻는다.
		//id_session = WTSGetActiveConsoleSessionId();
		ProcessIdToSessionId(GetCurrentProcessId(), &id_session);

		// - Process를 생성한다.
		result = create_process_to_session(str_execute, info_server_register.buf_environment, info_server_register.str_path_working, nullptr, &sStartupInfo, &infoProcess, id_session);

	}
	// 3-B) Console mode일 경우 CreateProcess로 간단히 시작
	else
	{
		// - CreateProcess로 Process를 생성한다.
		result = ::CreateProcessA(
			str_execute,																	// Application Name		__in_opt    LPCWSTR					lpApplicationName
			NULL,																		// Command line			__inout_opt LPWSTR					lpCommandLine
			NULL,																		// ProcessAttributes	__in_opt    LPSECURITY_ATTRIBUTES	lpProcessAttributes
			NULL,																		// ThreadAttributes		__in_opt    LPSECURITY_ATTRIBUTES	lpThreadAttributes
			FALSE,																		// Inherit Handle		__in        BOOL					bInheritHandles
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,	// Creation Flag		__in        DWORD					dwCreationFlags
			info_server_register.buf_environment.data(),								// Environment			__in_opt    LPVOID					lpEnvironment
			info_server_register.str_path_working.c_str(),								// CurrentDirectory		__in_opt    LPCWSTR					lpCurrentDirectory
			&sStartupInfo,																// StartupInfo			__in        LPSTARTUPINFOW			lpStartupInfo
			&infoProcess																// Process Information	__out       LPPROCESS_INFORMATION	lpProcessInformation
		);
	}

	// check) 
	if(result == FALSE)
	{
		// - Process ID를 0으로 초기화한다.
		handle_process = INVALID_PROCESS_ID;

		// return) 
		return false;
	}

	// 6) Process 정보를 설정한다.
	handle_process = infoProcess.hProcess;
	info_server_register.pid = infoProcess.dwProcessId;
	status_server.pid = infoProcess.dwProcessId;

	// 7) Process가 시작되길 기다린다.
	auto dwResult = WaitForInputIdle(infoProcess.hProcess, INFINITE);

	// check) 
	RETURN_IF(dwResult != 0, false);
#elif defined(__linux__)
		// - Process
	auto result = create_process_to_session(this, str_execute, info_server_register.buf_environment, info_server_register.str_path_working, nullptr);

	if(result == false)
	{
		// - ...
		handle_process = INVALID_PROCESS_ID;

		// return) 
		return false;
	}

#else
	#error "CGDK Not supported platform"
#endif

	// log) 
	LOG_PROGRESS << "@SERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST INIT server";

	// return) 
	return true;
}

bool server_entity::request_destroy_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.empty(), false);

	// 1) start신호를 전송한다.
	psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(service::get_instance()->m_info_host.hid, info_server_register.server_setting.id));

	// log) 
	LOG_PROGRESS << "@SERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST DESTROY service";

	// return) 
	return true;
}

bool server_entity::request_start_service(const std::string& _setting)
{
	// 1) 먼저 Reset;
	m_setting.clear();

	if (psocket.exist())
	{
		// 1) start 신호를 전송한다.
		psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_START_SERVICE(service::get_instance()->m_info_host.hid, info_server_register.server_setting.id, _setting));

		// log)
		LOG_PROGRESS << "@SERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST START service";
	}
	else
	{
		// - start 신호 전송을 예약한다.(이건 반드시 RequestInitServer를 호출하기 전에 설정되어야 한다. 다중쓰레드문제로 인해...)
		is_preserve_start	 = true;

		// - Setting string을 저장해 놓는ㄷ다.
		m_setting = _setting;

		//  - 먼저 InitService를 먼저 요청한다.
		request_init_service();
	}

	// return) 
	return true;
}

bool server_entity::request_stop_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.empty(), false);

	// 1) start신호를 전송한다.
	psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_STOP_SERVICE(service::get_instance()->m_info_host.hid, info_server_register.server_setting.id));
	
	// log)
	LOG_PROGRESS << "@ADMINSERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST STOP service";

	// return) 
	return true;
}

bool server_entity::request_pause_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.empty(), false);

	// 1) start신호를 전송한다.
	psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(service::get_instance()->m_info_host.hid, info_server_register.server_setting.id));

	// log)
	LOG_PROGRESS << "@ADMINSERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST PAUSE service";

	// return) 
	return true;
}

bool server_entity::request_continue_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.empty(), false);

	// 1) start신호를 전송한다.
	psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(service::get_instance()->m_info_host.hid, info_server_register.server_setting.id));

	// log)
	LOG_PROGRESS << "@ADMINSERVER['" << info_server_register.server_setting.name << "':" << info_server_register.server_setting.id.key << "]: REQUEST CONTINUE service";

	// return) 
	return true;
}

bool safe_terminate_process(PROCESS_ID _handle_process, uintptr_t uExitCode)
{
#if defined(_WIN32)
	// declare) 
	DWORD  dwTID, dwCode, dwErr = 0;
	PROCESS_ID handle_process_dup = INVALID_PROCESS_ID;
	PROCESS_ID hRT = NULL;

	// 1) Kernel32를 얻는다.
	auto hKernel = GetModuleHandle(TEXT("Kernel32"));

	// check)
	RETURN_IF(hKernel == 0, false);
   
	// declare) 
	auto is_success = false;

	// 2) Duplicate를 먼저한다.
	auto bDup = DuplicateHandle(GetCurrentProcess(), 
                  _handle_process, 
                  GetCurrentProcess(), 
                  &handle_process_dup,
                  PROCESS_ALL_ACCESS, 
                  FALSE, 
                  0);

	// 3) ExitCodeProcess를 얻는다.
   if(GetExitCodeProcess((bDup) ? handle_process_dup : _handle_process, &dwCode) && (dwCode == STILL_ACTIVE))
   {
	   // - Process를 얻는다.
		FARPROC	pfnExitProc	 = GetProcAddress(hKernel, "ExitProcess");

		// - RemoteThread를 생성한다.
		hRT = CreateRemoteThread(
								(bDup) ? handle_process_dup : _handle_process,
								(LPSECURITY_ATTRIBUTES)nullptr,
								(SIZE_T)0, 
								(LPTHREAD_START_ROUTINE)pfnExitProc,
								(PVOID)uExitCode,
								(DWORD)0,
								(LPDWORD)&dwTID);

		// check) ...
		if(hRT == NULL)
			dwErr = api::get_last_error();
	}
	else 
	{
		dwErr = ERROR_PROCESS_ABORTED;
	}

   // 4) 핸들을 얻었다면 완료될 때까지 기다린다.
	if(hRT)
	{
		WaitForSingleObject((bDup) ? handle_process_dup : _handle_process, INFINITE);
		CloseHandle(hRT);
		is_success = true;
	}

	// 5) Duplicate한 Handle을 닫는다.
	SAFE_CLOSEHANDLE(handle_process_dup);

	// 6) 
	if(!is_success)
		SetLastError(dwErr);
   
	// return) 
	return is_success;
#elif defined(__linux__)

	return false;
#else
	#error "CGDK Not supported platform"
#endif
}

bool server_entity::request_kill_service()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(psocket.empty(), false);

	// check) Process Handle을 얻는다.
	RETURN_IF(handle_process == INVALID_PROCESS_ID, false);

	// 1) handle_process값을 읽고 초기화한다.
	auto handle_process_target = handle_process;

	handle_process = INVALID_PROCESS_ID;

	// 2) 해당 Process를 강제로 죽인다.
	auto result = safe_terminate_process(handle_process_target, (UINTPTR_MAX)-1);

	// check) 실패했는가!!
	RETURN_IF(result == false, false);

	// 3) 성공후 처리! 없음.
#if defined(_WIN32)
	CloseHandle(handle_process_target);
#elif defined(__linux__)
	kill(handle_process_target, SIGINT);
#else
	#error "CGDK Not supported platform"
#endif

	// return) 
	return true;
}

bool server_entity::request_update()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.svn_url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.str_path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	status.compare_exchange_weak(status_temp, 1);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status_temp != 0, false);

#ifdef _USE_SVN_CONTROL
	// trace) trace를 건다.
	LOG_PROGRESS << "@UPDATE requested (" << info_server_setting.name << ")";
	LOG_PROGRESS << "  path : " << info_server_setting.str_path;
	LOG_PROGRESS << "  URL  : " << info_server_setting.strURL;
	if (info_server_setting.iRevision != -1)
	{
		LOG(eLOG_TYPE::PROGRESS_ | eLOG_TYPE::CONTINUE_) << "  rev  : " << info_server_setting.iRevision;
	}

	// 2) 디렉토리를 생성한다.
	//CreateDirectory((const char*)_pregister_info->info_server_setting.dirLocation, NULL);

	// declare) 
	svn_client_info2_t svn_info;

	// 3) 정보를 얻는다.
	SVN_RESULT result = server_admin::SVN::GetInfo(info_server_setting.str_path.c_str(), svn_info, this);

	// 4) Error가 갔으면 CheckOupt하고 아니면 Update를 한다.
	if (result.IsSuccess())
	{
		// 4-1) Head version이면 Update를 수행한다.
		result = server_admin::SVN::Update(info_server_setting.str_path.c_str(), info_server_setting.iRevision, this);

		// check) 완료 후 처리를 한다.
		if (result.IsSuccess())
		{
		}
	}
	else
	{
		// - Update를 요청한다.
		result = server_admin::SVN::CheckOut(info_server_setting.str_path.c_str(), info_server_setting.strURL, this);

		// check) 완료 후 처리를 한다.
		if (result.IsSuccess())
		{
		}
	}
#endif

	// Log) 
	LOG_PROGRESS << "@UPDATE completed (" << info_server_register.server_setting.name << ")";

	// 5) 상태를 다시 0으로 바꾼다.
	status.exchange(0);

	// return) 
	return true;
}

bool server_entity::request_check_out()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.svn_url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.str_path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	status.compare_exchange_weak(status_temp, 1);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status_temp != 0, false);

#ifdef _USE_SVN_CONTROL
	// declare) 
	SVN_RESULT	result;

	// 2) Update를 요청한다.
	result = server_admin::SVN::CheckOut(info_server_setting.strURL, info_server_setting.str_path.c_str(), this);

	// 3) 완료 후 처리를 한다.
	if (result.IsFail())
	{
	}
#endif

	// 4) 상태를 다시 0으로 바꾼다.
	status.exchange(0);

	// return) 
	return true;
}

bool server_entity::request_switch()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.svn_url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(info_server_register.str_path.empty(), false);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status != 0, false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	status.compare_exchange_weak(status_temp, 1);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status_temp != 0, false);

#ifdef _USE_SVN_CONTROL
	// 1) Update를 요청한다.
	auto result = server_admin::SVN::Switch(info_server_setting.strURL, info_server_setting.str_path.c_str(), this);

	// 3) 완료 후 처리를 한다.
	if (result.IsFail())
	{
	}
#endif

	// 4) 상태를 다시 0으로 바꾼다.
	status.exchange(0);

	// return) 
	return true;
}
 
void server_entity::update_application_status()
{
	// check) 
	RETURN_IF(handle_process == INVALID_PROCESS_ID);

	// 1) Process
	status_server.usage_process = (perfornamce_processor.exist()) ? perfornamce_processor->GetValueLONG() : 0;

#if defined(_WIN32)
	// 2) Memory 정보를 얻는다.
	PROCESS_MEMORY_COUNTERS_EX	sInfoMemoryCounters;
	ZeroMemory(&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));
	sInfoMemoryCounters.cb = sizeof(sInfoMemoryCounters);
	GetProcessMemoryInfo((HANDLE)handle_process, (PPROCESS_MEMORY_COUNTERS)&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));
	status_server.usage_memory = sInfoMemoryCounters.PrivateUsage;
#elif defined(__linux__)

	////#include <iostream>
	////#include <fstream>

	//void process_mem_usage(double& vm_usage, double& resident_set)
	//{
	//using std::ios_base;
	//using std::ifstream;
	//using std::string;

	//vm_usage     = 0.0;
	//resident_set = 0.0;

	//// 'file' stat seems to give the most reliable results
	////
	//ifstream stat_stream("/proc/self/stat",ios_base::in);

	//// dummy vars for leading entries in stat that we don't care about
	////
	//string pid, comm, state, ppid, pgrp, session, tty_nr;
	//string tpgid, flags, minflt, cminflt, majflt, cmajflt;
	//string utime, stime, cutime, cstime, priority, nice;
	//string O, itrealvalue, starttime;

	//// the two fields we want
	////
	//unsigned long vsize;
	//long rss;

	//stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
	//			>> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
	//			>> utime >> stime >> cutime >> cstime >> priority >> nice
	//			>> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

	//stat_stream.close();

	//long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
	//vm_usage     = vsize / 1024.0;
	//resident_set = rss * page_size_kb;
	//}

#else
	#error "CGDK Not supported platform"
#endif

	// 3) Network 정보
	status_server.usage_network = 0;
}

void server_entity::set_server_register_info(const server_admin::sSERVER_REGISTER& _info_server_register)
{
	// 1) 값을 복사한다.
	info_server_register = _info_server_register;
}

void server_entity::update_revision_info()
{
//	// check)
//	RETURN_IF(info_server_register.str_path.empty() == false);
//
//	// 2) URL정보를 읽어서 설정한다.
//	if (info_server_register.svn_url.empty()==true)
//	{
//#ifdef _USE_SVN_CONTROL
//		// declare) 
//		svn_client_info2_t tempClientInfo;
//
//		// - 정보를 읽는다.
//		auto	result	 = server_admin::SVN::GetInfo(info_server_setting.str_path.c_str(), tempClientInfo);
//
//		if (result.IsSuccess())
//		{
//			// - URL을 읽어넣는다.
//			__convert_string(info_server_setting.strURL, tempClientInfo.URL);
//
//			// - Version은 Default로...
//			info_server_setting.iRevision = -1;
//		}
//#endif
//	}
}

void server_entity::data_factory_info_set(vector<server_admin::factory_snap>&& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// 1) set executable_info
	//	vector<server_admin::factory_snap> temp_vector_factory_info;
	//	vector<server_admin::factory_snap> temp_vector_executable_info;

	//	for (auto& iter : _data)
	//	{
	//		if (iter.object_type == eOBJECT_TYPE::EXECUTABLE)
	//		{
	//			temp_vector_executable_info.push_back(iter);
	//		}

	//		temp_vector_factory_info.push_back(std::move(iter));
	//	}

	//	// 2) set factory info
	//	vector_factory_info = std::move(temp_vector_factory_info);
	//	vector_executable_info = std::move(temp_vector_executable_info);
	//}
}

void server_entity::data_factory_info_update(vector<server_admin::factory_update_snap>&& /*_data*/)
{
	//// check)
	//CGASSERT_ERROR(vector_factory_info.size() == _data.size());

	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	auto iter_factory = vector_factory_info.begin();
	//	auto iter_executable = vector_executable_info.begin();
	//	auto iter_src = _data.begin();
	//	auto iter_src_end = _data.end();

	//	// 1) set...
	//	for (; iter_src != iter_src_end; ++iter_src, ++iter_factory)
	//	{
	//		if (iter_factory->object_type == eOBJECT_TYPE::EXECUTABLE)
	//		{
	//			iter_executable->updatable = *iter_src;
	//			++iter_executable;
	//		}

	//		iter_factory->updatable = std::move(*iter_src);
	//	}

	//	// 2) ...
	//	pobservable_websocket_server_executor_info->request_update_webpage_executable();
	//	pobservable_websocket_server_factory_info->request_update_webpage_factory();
	//}
}

void server_entity::data_factory_info_register(server_admin::factory_snap&& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	if (_data.object_type == eOBJECT_TYPE::EXECUTABLE)
	//	{
	//		vector_executable_info.push_back(_data);
	//	}

	//	vector_factory_info.push_back(std::move(_data));
	//}
}

void server_entity::data_factory_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::data_executor_info_set(vector<server_admin::executor_snap>&& /*_vector_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// declare) 
	//	vector<server_admin::executor_snap> temp_vector_executor_snap;

	//	// 1) set 
	//	for (auto& iter : _vector_data)
	//	{
	//		server_admin::executor_snap temp_executor_snap;

	//		temp_executor_snap.name = iter.name;
	//		temp_executor_snap.type = iter.type;
	//		temp_executor_snap.updatable = iter.updatable;

	//		temp_vector_executor_snap.push_back(temp_executor_snap);
	//	}
	//	vector_executor_info = std::move(temp_vector_executor_snap);
	//}

	//// 2) notify
	//pobservable_websocket_server_executor_info->request_update_webpage_executor();
}

void server_entity::data_executor_info_update(vector<server_admin::executor_update_snap>&& /*_data*/)
{
	//// check)
	//CGASSERT_ERROR(vector_executor_info.size() == _data.size());

	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	auto iter_dest = vector_executor_info.begin();
	//	auto iter_src = _data.begin();
	//	auto iter_src_end = _data.end();

	//	// 1) set...
	//	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	//	{
	//		iter_dest->updatable = *iter_src;
	//	}
	//}

	//// 2) notify
	//pobservable_websocket_server_executor_info->request_update_webpage_executor();
}

void server_entity::data_executor_info_register(server_admin::executor_snap&& /*_data*/)
{
}

void server_entity::data_executor_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::data_schedulable_info_set(vector<server_admin::schedulable_snap>&& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	vector<server_admin::schedulable_snap> vector_deleted;
	//	vector<server_admin::schedulable_snap> vector_new;

	//	// 1) get added/deleted schedulable
	//	auto iter_dest = vector_schedulable_info.begin();
	//	auto iter_dest_end = vector_schedulable_info.end();
	//	auto iter_src = _data.begin();
	//	auto iter_src_end = _data.end();

	//	while (iter_src != iter_src_end && iter_dest != iter_dest_end)
	//	{
	//		// - updated
	//		if (iter_dest->updatable.id == iter_src->updatable.id)
	//		{
	//			++iter_dest;
	//			++iter_src;
	//		}
	//		// - new
	//		else if (iter_dest->updatable.id > iter_src->updatable.id)
	//		{
	//			vector_new.push_back(*iter_src);
	//			++iter_src;
	//		}
	//		// - deleted
	//		else if (iter_dest->updatable.id < iter_src->updatable.id)
	//		{
	//			vector_deleted.push_back(*iter_dest);
	//			++iter_dest;
	//		}
	//	}

	//	for (; iter_src != iter_src_end; ++iter_src)
	//	{
	//		vector_new.push_back(*iter_src);
	//	}

	//	for (; iter_dest != iter_dest_end; ++iter_dest)
	//	{
	//		vector_deleted.push_back(*iter_dest);
	//	}

	//	// check)
	//	CGASSERT_ERROR(_data.size() + vector_deleted.size() == vector_schedulable_info.size() + vector_new.size())

	//	// 2) set 
	//	vector_schedulable_info = std::move(_data);
	//}

	//// 3) request update
	//pobservable_websocket_server_executor_info->request_update_webpage_schedulable();
}

void server_entity::data_connective_info_set(vector<server_admin::connective_info_snap>&& /*_vector_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	vector<server_admin::connective_info_store> temp_vector_connective_info;

	//	for (auto& iter : _vector_data)
	//	{
	//		auto& temp = temp_vector_connective_info.emplace_back();

	//		temp.name = iter.name;
	//		temp.time_create = iter.time_create;
	//		temp.status = iter.status;
	//		temp.index = iter.index;
	//		temp.period.resize(TOTAL_STATISTICS_INFO);
	//		temp.period[iter.index] = iter.now;
	//	}

	//	vector_connective_info = std::move(temp_vector_connective_info);
	//}
}

void server_entity::data_connective_info_update(vector<server_admin::connective_info_update_snap>&& /*_vector_data*/)
{
	//// check)
	//CGASSERT_ERROR(vector_connective_info.size() == _vector_data.size());

	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	auto iter_dest = vector_connective_info.begin();
	//	auto iter_src = _vector_data.begin();
	//	auto iter_src_end = _vector_data.end();

	//	// 1) set...
	//	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	//	{
	//		iter_dest->status = iter_src->status;
	//		iter_dest->period[iter_dest->index] = iter_src->now;
	//	}
	//}

	//// 2) notify
	//pobservable_websocket_server_network_info->request_update_webpage_connective();
}

void server_entity::data_connective_info_register(server_admin::connective_info_snap&& /*_data*/)
{
}

void server_entity::data_connective_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::data_binder_info_set(vector<server_admin::socket_info_snap>&& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// 1) reset binder
	//	vector_binder_info.clear();

	//	// 2) set...
	//	for (auto& iter: _data)
	//	{
	//		server_admin::socket_info_snap temp_socket_info;

	//		temp_socket_info.name = std::move(iter.name);
	//		temp_socket_info.updatable = std::move(iter.updatable);

	//		vector_binder_info.push_back(temp_socket_info);
	//	}
	//}
}

void server_entity::data_binder_info_update(vector<server_admin::socket_info_update_snap>&& /*_data*/)
{
	//// check)
	//CGASSERT_ERROR(vector_binder_info.size() == _data.size());

	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	auto iter_dest = vector_binder_info.begin();
	//	auto iter_src = _data.begin();
	//	auto iter_src_end = _data.end();

	//	// 1) set...
	//	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	//	{
	//		iter_dest->updatable = std::move(*iter_src);
	//	}
	//}

	//// 2) notify
	//pobservable_websocket_server_network_info->request_update_webpage_binder();
}

void server_entity::data_binder_info_register(server_admin::socket_info_snap&& /*_data*/)
{
}

void server_entity::data_binder_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::data_time_event_info_set(vector<server_admin::time_event_snap>&& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// 1) ....
	//	vector_time_event_info = std::move(_data);
	//}

	//// 2) notify
	//pobservable_websocket_server_overview_info->request_update_webpage_time_event();
}

void server_entity::data_time_event_info_update(vector<server_admin::time_event_update_snap>&& /*_data*/)
{
	//// check)
	//CGASSERT_ERROR(vector_time_event_info.size() == _data.size());

	//scoped_lock(cs_server_data)
	//{
	//	// declare)
	//	auto iter_dest = vector_time_event_info.begin();
	//	auto iter_src = _data.begin();
	//	auto iter_src_end = _data.end();

	//	// 1) set...
	//	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	//	{
	//		iter_dest->updatable = *iter_src;
	//	}
	//}

	//// 2) notify
	//pobservable_websocket_server_overview_info->request_update_webpage_time_event();
}

void server_entity::data_time_event_info_register(server_admin::time_event_snap&& /*_data*/)
{
}

void server_entity::data_time_event_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::data_connective_statistics_set(const statistics::unit_connective_snap& /*_data*/)
{
	//// 1) set
	//scoped_lock(cs_server_data)
	//{
	//	if(connective_info.vector_unit_connective_snap.size() <= connective_info.index)
	//		connective_info.vector_unit_connective_snap.resize(connective_info.index + 1);

	//	connective_info.vector_unit_connective_snap[connective_info.index] = _data;
	//}
}

void server_entity::data_connective_statistics_update(const statistics::unit_connective_snap& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	connective_info.vector_unit_connective_snap[connective_info.index] = _data;
	//}
}

void server_entity::data_traffic_statistics_set(const statistics::unit_traffic_snap& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	if (traffic_info.vector_unit_traffic_snap.size() <= traffic_info.index)
	//		traffic_info.vector_unit_traffic_snap.resize(traffic_info.index + 1);

	//	traffic_info.vector_unit_traffic_snap.at(traffic_info.index) = _data;
	//}
}

void server_entity::data_traffic_statistics_update(const statistics::unit_traffic_snap& /*_data*/)
{
	//scoped_lock(cs_server_data)
	//{
	//	// 4) set traffic info
	//	traffic_info.vector_unit_traffic_snap.at(traffic_info.index) = _data;
	//}
}

void server_entity::data_traffic_log_records_set(circular_list<object_ptr<LOG_RECORD>>&& /*_vector_log_records*/) noexcept
{
	//scoped_lock(cs_server_data)
	//{
	//	vector_log_records = std::move(_vector_log_records);
	//}
}

void server_entity::data_traffic_log_records_add(circular_list<object_ptr<LOG_RECORD>>&& /*_vector_log_records*/) noexcept
{
	//scoped_lock(cs_server_data)
	//{
	//	vector_log_records.splice(vector_log_records.end(), _vector_log_records);
	//}
}

void server_entity::data_crash_info_set(shared_buffer /*_buf_data*/)
{
	scoped_lock(cs_server_data)
	{
	}
}

void server_entity::data_crash_info_update(shared_buffer /*_buf_data*/)
{
	scoped_lock(cs_server_data)
	{
	}
}

eOBJECT_STATE server_entity::set_server_state(eOBJECT_STATE _state)
{
	// 1) Old State를 얻어 놓느다.
	auto state_old = status_server.state;

	// check) Old와 동일하면 그냥 끝낸다.
	RETURN_IF(state_old == _state, state_old)

	// 2) 변경한다.
	status_server.state = _state;

	// 3) is_preserve_start가 true인지를 확인한다.
	if(_state==eOBJECT_STATE::STOPPED && is_preserve_start==true)
	{
		// - 먼저 false로 변경한 후...
		is_preserve_start	 = false;

		// - start한다.
		request_start_service(m_setting);
	}
	else if(_state == eOBJECT_STATE::RUNNING)
	{
		is_preserve_start = false;
	}

	// 4) Setting String을 clear한다.
	m_setting.clear();

	// return) 
	return state_old;
}

result_code server_entity::on_observer_registering(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void server_entity::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// 2) send factory list
					// 1) get 
	auto pobservable_webpage = _get_pobservable_webpage(_pobserver->m_webpage_selected);

	_pobserver->m_webpage_ptarget = pobservable_webpage;
	_pobserver->send(_pobserver->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE_B, pobservable_webpage->m_webpage_str_target));// "BSCF"
}

uintptr_t server_entity::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) register mediator
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

void server_entity::on_observer_unregistered(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

uintptr_t server_entity::on_observer_data_updating(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, observer_data_t& /*_observer_data*/)
{
	// return) 
	return 0;
}

void server_entity::on_observer_data_update(socket_websocket* /*_pobserver*/, observer_channel_t /*_ocid*/, uintptr_t /*_result*/)
{
}

object_ptr<Iobservable_websocket> server_entity::_get_pobservable_webpage(char /*_select*/) const noexcept
{
	// declare) 
	object_ptr<Iobservable_websocket> pobservable_webpage;

	//switch (_select)
	//{
	//case	'A':
	//		pobservable_webpage = pobservable_websocket_server_overview_info;
	//		break;

	//case	'B':
	//		pobservable_webpage = pobservable_websocket_server_factory_info;
	//		break;

	//case	'C':
	//		pobservable_webpage = pobservable_websocket_server_executor_info;
	//		break;

	//case	'D':
	//		pobservable_webpage = pobservable_websocket_server_network_info;
	//		break;

	//case	'E':
	//		pobservable_webpage = pobservable_websocket_server_querier_info;
	//		break;

	//case	'F':
	//		pobservable_webpage = pobservable_websocket_server_crash_info;
	//		break;

	//default:
	//		pobservable_webpage = pobservable_websocket_server_overview_info;
	//		break;
	//};

	// return)
	return pobservable_webpage;
}


result_code server_entity::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message 
	sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get service instance 
	object_ptr<service> pservice = service::get_instance();

	// 3) get message head
	buffer_view buf_temp = msg.buf_message;

	auto message_type = buf_temp.extract_text<char>(size(4));
	auto pmember = msg.get_source<socket_websocket>();

	if (message_type[0] == 'B')
	{
		switch(message_type[1])
		{
		case 'A':
				{
					// - get observable
					auto pobservable_webpage = _get_pobservable_webpage(message_type[2]);

					// check) 
					RETURN_IF(pobservable_webpage.empty(), eRESULT::DONE);

					// check) same webpage?
					RETURN_IF(pmember->is_observable(pobservable_webpage, OCID_FRAME3), eRESULT::DONE);

					// - unregister old observable
					pmember->unsubscribe(OCID_FRAME3);

					// - set..
					pmember->m_webpage_ptarget = pobservable_webpage;
					pmember->m_webpage_selected = pobservable_webpage->m_webpage_index;

					// - request 
					pmember->send(pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE_B, pobservable_webpage->m_webpage_str_target)); // "BSCF"
				}
				break;

		case	'B':
				{
					// declare) 
					auto pobservable_webpage = std::move(pmember->m_webpage_ptarget);

					// check)
					RETURN_IF(pobservable_webpage.empty(), eRESULT::DONE);

					// - register member
					pobservable_webpage->subscribe(pmember, OCID_FRAME3);
				}
				break;
		};
	}

	// return) 
	return eRESULT::BYPASS;
}

std::string_view GetSVNActionString(int _Notify)
{
	// declare) 
	std::string_view	strAction;

	// 1) Action에 따라...
	switch (_Notify)
	{
#ifdef _USE_SVN_CONTROL
	// Adding a path to revision control.
	case	svn_wc_notify_add:
			strAction = "Add";
			break;

	// Copying a versioned path.
	case	svn_wc_notify_copy:
			strAction = "Copy";
			break;

	// Deleting a versioned path.
	case	svn_wc_notify_delete:
			strAction = "Delete";
			break;

	// Restoring a missing path from the pristine text-base.
	case	svn_wc_notify_restore:
			strAction = "Restore";
			break;

	// Reverting a modified path.
	case	svn_wc_notify_revert:
			strAction = "Revert";
			break;

	// A revert operation has failed.
	case	svn_wc_notify_failed_revert:
			strAction = "Failed revert";
			break;

	// Resolving a conflict.
	case	svn_wc_notify_resolved:
			strAction = "Resolved";
			break;

	// Skipping a path.
	case	svn_wc_notify_skip:
			strAction = "Skip";
			break;

	// Got a delete in an update.
	case	svn_wc_notify_update_delete:
			strAction = "Update delete";
			break;

	// Got an add in an update.
	case	svn_wc_notify_update_add:
			strAction = "Update add";
			break;

	// Got any other action in an update.
	case	svn_wc_notify_update_update:
			strAction = "Update update";
			break;

	// The last notification in an update (including updates of externals).
	case	svn_wc_notify_update_completed:
			strAction = "Update completed";
			break;

	// Updating an external module.
	case	svn_wc_notify_update_external:
			strAction = "Update external";
			break;

	// The last notification in a status (including status on externals).
	case	svn_wc_notify_status_completed:
			strAction = "Status completed";
			break;

	// Running status on an external module.
	case	svn_wc_notify_status_external:
			strAction = "Status external";
			break;

	// Committing a modification.
	case	svn_wc_notify_commit_modified:
			strAction = "Commit modified";
			break;

	// Committing an addition.
	case	svn_wc_notify_commit_added:
			strAction = "Commit added";
			break;

	// Committing a deletion.
	case	svn_wc_notify_commit_deleted:
			strAction = "Commit deleted";
			break;

	// Committing a replacement.
	case	svn_wc_notify_commit_replaced:
			strAction = "Commit replaced";
			break;

	// Transmitting post-fix text-delta data for a file.
	case	svn_wc_notify_commit_postfix_txdelta:
			strAction = "Commit postfix txdelta";
			break;

	// Processed a single revision's blame.
	case	svn_wc_notify_blame_revision:
			strAction = "Blame revision";
			break;

	// Locking a path. @since New in 1.2.
	case	svn_wc_notify_locked:
			strAction = "Locked";
			break;

	// Unlocking a path. @since New in 1.2.
	case	svn_wc_notify_unlocked:
			strAction = "Unlocked";
			break;

	// Failed to lock a path. @since New in 1.2.
	case	svn_wc_notify_failed_lock:
			strAction = "Failed lock";
			break;

	// Failed to unlock a path. @since New in 1.2.
	case	svn_wc_notify_failed_unlock:
			strAction = "Failed unlock";
			break;

	// Tried adding a path that already exists. @since New in 1.5.
	case	svn_wc_notify_exists:
			strAction = "Exists";
			break;

	// Changelist name set. @since New in 1.5.
	case	svn_wc_notify_changelist_set:
			strAction = "Changelist set";
			break;

	// Changelist name cleared. @since New in 1.5.
	case	svn_wc_notify_changelist_clear:
			strAction = "Changelist clear";
			break;

	// Warn user that a path has moved from one changelist to another. @since New in 1.5. @deprecated As of 1.7, separate clear and set notifications are sent.
	case	svn_wc_notify_changelist_moved:
			strAction = "Changelist moved";
			break;

	// A merge operation (to path) has begun.  See #svn_wc_notify_t.merge_range. @since New in 1.5.
	case	svn_wc_notify_merge_begin:
			strAction = "Merge begin";
			break;

	// A merge operation (to path) from a foreign repository has begun. See #svn_wc_notify_t.merge_range.  @since New in 1.5.
	case	svn_wc_notify_foreign_merge_begin:
			strAction = "Foreign merge begin";
			break;

	// Replace notification. @since New in 1.5.
	case	svn_wc_notify_update_replace:
			strAction = "Update replace";
			break;

	// Property added. @since New in 1.6.
	case	svn_wc_notify_property_added:
			strAction = "Property added";
			break;

	// Property updated. @since New in 1.6.
	case	svn_wc_notify_property_modified:
			strAction = "Property modified";
			break;

	// Property deleted. @since New in 1.6.
	case	svn_wc_notify_property_deleted:
			strAction = "Property deleted";
			break;

	// Nonexistent property deleted. @since New in 1.6.
	case	svn_wc_notify_property_deleted_nonexistent:
			strAction = "Property deleted nonexistent";
			break;

	// Revprop set. @since New in 1.6.
	case	svn_wc_notify_revprop_set:
			strAction = "Revprop set";
			break;

	// Revprop deleted. @since New in 1.6.
	case	svn_wc_notify_revprop_deleted:
			strAction = "Revprop deleted";
			break;

	// The last notification in a merge. @since New in 1.6.
	case	svn_wc_notify_merge_completed:
			strAction = "Merge completed";
			break;

	// The path is a tree-conflict victim of the intended action (*not* a persistent tree-conflict from an earlier operation, but *this* operation caused the tree-conflict). @since New in 1.6.
	case	svn_wc_notify_tree_conflict:
			strAction = "Tree conflict";
			break;

	// The path is a subdirectory referenced in an externals definition which is unable to be operated on.  @since New in 1.6.
	case	svn_wc_notify_failed_external:
			strAction = "Failed external";
			break;

	// Starting an update operation.  @since New in 1.7.
	case	svn_wc_notify_update_started:
			strAction = "Update started";
			break;

	// An update tried to add a file or directory at a path where a separate working copy was found.  @since New in 1.7.
	case	svn_wc_notify_update_skip_obstruction:
			strAction = "Update skip obstruction";
			break;

	// An explicit update tried to update a file or directory that doesn't live in the repository and can't be brought in. @since New in 1.7.
	case	svn_wc_notify_update_skip_working_only:
			strAction = "Update skip working only";
			break;

	// An update tried to update a file or directory to which access could not be obtained. @since New in 1.7.
	case	svn_wc_notify_update_skip_access_denied:
			strAction = "Update skip access denied";
			break;

	// An update operation removed an external working copy. @since New in 1.7.
	case	svn_wc_notify_update_external_removed:
			strAction = "Update external removed";
			break;

	// A node below an existing node was added during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_add:
			strAction = "Update shadowed add";
			break;

	// A node below an existing node was updated during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_update:
			strAction = "Update shadowed update";
			break;

	// A node below an existing node was deleted during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_delete:
			strAction = "Update shadowed delete";
			break;

	// The mergeinfo on path was updated.  @since New in 1.7.
	case	svn_wc_notify_merge_record_info:
			strAction = "Merge record info";
			break;

	// A working copy directory was upgraded to the latest format. @since New in 1.7.
	case	svn_wc_notify_upgraded_path:
			strAction = "Upgraded path";
			break;

	// Mergeinfo describing a merge was recorded. @since New in 1.7.
	case	svn_wc_notify_merge_record_info_begin:
			strAction = "Merge record info begin";
			break;

	// Mergeinfo was removed due to elision. @since New in 1.7.
	case	svn_wc_notify_merge_elide_info:
			strAction = "Merge elide info";
			break;

	// A file in the working copy was patched. @since New in 1.7.
	case	svn_wc_notify_patch:
			strAction = "Patch";
			break;

	// A hunk from a patch was applied. @since New in 1.7.
	case	svn_wc_notify_patch_applied_hunk:
			strAction = "Patch applied hunk";
			break;

	// A hunk from a patch was rejected. @since New in 1.7.
	case	svn_wc_notify_patch_rejected_hunk:
			strAction = "Patch rejected hunk";
			break;

	// A hunk from a patch was found to already be applied. @since New in 1.7.
	case	svn_wc_notify_patch_hunk_already_applied:
			strAction = "Patch hunk aleady applied";
			break;

	// Committing a non-overwriting copy (path is the target of the copy, not the source). @since New in 1.7.
	case	svn_wc_notify_commit_copied:
			strAction = "Commit copied";
			break;

	// Committing an overwriting (replace) copy (path is the target of the copy, not the source). @since New in 1.7.
	case	svn_wc_notify_commit_copied_replaced:
			strAction = "Commit copied replaced";
			break;

	// The server has instructed the client to follow a URL redirection. @since New in 1.7.
	case	svn_wc_notify_url_redirect:
			strAction = "URL redirect";
			break;

	// The operation was attempted on a path which doesn't exist. @since New in 1.7.
	case	svn_wc_notify_path_nonexistent:
			strAction = "Pathnonexistent";
			break;

	// Removing a path by excluding it. @since New in 1.7.
	case	svn_wc_notify_exclude:
			strAction = "Exclude";
			break;

	// Operation failed because the node remains in conflict @since New in 1.7.
	case	svn_wc_notify_failed_conflict:
			strAction = "Failed Conflict";
			break;

	// Operation failed because an added node is missing @since New in 1.7.
	case	svn_wc_notify_failed_missing:
			strAction = "Failed missing";
			break;

	// Operation failed because a node is out of date @since New in 1.7.
	case	svn_wc_notify_failed_out_of_date:
			strAction = "Failed out of date";
			break;

	// Operation failed because an added parent is not selected @since New in 1.7.
	case	svn_wc_notify_failed_no_parent:
			strAction = "Failed no parent";
			break;

	// Operation failed because a node is locked by another user and/or working copy.  @since New in 1.7.
	case	svn_wc_notify_failed_locked:
			strAction = "Failed locked";
			break;

	// Operation failed because the operation was forbidden by the server. @since New in 1.7.
	case	svn_wc_notify_failed_forbidden_by_server:
			strAction = "Failed forbidden by server";
			break;

	// The operation skipped the path because it was conflicted. @since New in 1.7.
	case	svn_wc_notify_skip_conflicted:
			strAction = "Skip conflicted";
			break;

	// Just the lock on a file was removed during update. @since New in 1.8.
	case	svn_wc_notify_update_broken_lock:
			strAction = "Update broken lock";
			break;

	// Operation failed because a node is obstructed. @since New in 1.8.
	case	svn_wc_notify_failed_obstruction:
			strAction = "Failed obstruction";
			break;

	// Conflict resolver is starting. This can be used by clients to detect when to display conflict summary information, for example. @since New in 1.8.
	case	svn_wc_notify_conflict_resolver_starting:
			strAction = "Conflict resolver starting";
			break;

	// Conflict resolver is done. This can be used by clients to detect when to display conflict summary information, for example. @since New in 1.8.
	case	svn_wc_notify_conflict_resolver_done:
			strAction = "Conflict resolver done";
			break;

	// The current operation left local changes of something that was deleted The changes are available on (and below) the notified path @since New in 1.8. svn_wc_notify_left_local_modifications, A copy from a foreign repository has started @since New in 1.8.
	case	svn_wc_notify_foreign_copy_begin:
			strAction = "Foreign copy begin";
			break;

	// A move in the working copy has been broken, i.e. degraded into a copy + delete. The notified path is the move source (the deleted path). ### TODO: Provide path to move destination as well? @since New in 1.8.
	case	svn_wc_notify_move_broken:
			strAction = "Move broken";
			break;

	// Running cleanup on an external module. @since New in 1.9. */
	case	svn_wc_notify_cleanup_external:
			strAction = "Cleanup external";
			break;

	// The operation failed because the operation (E.g. commit) is only valid if the operation includes this path. @since New in 1.9. */
	case	svn_wc_notify_failed_requires_target:
			strAction = "Failed requires target";
			break;

	//  info on an external module. @since New in 1.9. */
	case	svn_wc_notify_info_external:
			strAction = "Info external";
			break;

	// Finalizing commit.@since New in 1.9. */
	case	svn_wc_notify_commit_finalizing:
			strAction = "Commit finalizing";
			break;
#endif
	default:
			strAction = "-";
			break;
	};

	// return) 
	return	strAction;
}

#ifdef _USE_SVN_CONTROL
void server_entity::on_svn_notify(server_admin::Isvn_command * /*_baton*/, const svn_wc_notify_t * _notify, apr_pool_t * /*_pool*/)
{
	// check)
	RETURN_IF(_notify == nullptr);
	
	// log)
	LOG_INFO << "  - ["sv << GetSVNActionString(_notify->action) << "] "sv << _notify->path;
}

void server_entity::on_svn_procgress(apr_off_t /*_progress*/, apr_off_t /*_total*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_pool*/)
{
	// 제거
}

svn_error_t * server_entity::on_svn_log(const char ** /*_log_msg*/, const char ** /*_tmp_file*/, const apr_array_header_t * /*_commit_items*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_pool*/)
{
	// 제거
	return nullptr;
}

svn_error_t * server_entity::on_svn_conflict_resolver(svn_wc_conflict_result_t ** /*_result*/, const svn_wc_conflict_description2_t * /*_description*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_result_pool*/, apr_pool_t * /*_scratch_pool*/)
{
	// 제거
	return nullptr;
}

svn_error_t * server_entity::on_svn_cancel(server_admin::Isvn_command * /*_cancel_baton*/)
{
	// 제거
	return nullptr;
}

void server_entity::on_svn_complete(server_admin::Isvn_command * /*_cancel_baton*/, const SVN_RESULT& /*_result*/)
{
	// 제거
}
#endif
