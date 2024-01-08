#include "pch.h"
#if defined(_WIN32)
	#include "Userenv.h"
	#include "Wtsapi32.h"
#elif defined(__linux__)
	#include <sys/wait.h>
	#include <pthread.h>
#else
#endif

//#ifdef _USE_SVN_CONTROL
//, virtual public server_admin::Isvn_notify
//#endif

server_admin::instance_service_local::instance_service_local()
{
}

server_admin::instance_service_local::~instance_service_local() noexcept
{
}

#if defined(_WIN32)
bool create_process_to_session(const std::string_view _str_execute, const shared_buffer& _buf_environment, const std::string_view _str_path_working, char* _args, STARTUPINFOA* p_psi, PROCESS_INFORMATION* _ppi, UINT _session_id)
 {
	 // declare) 
	HANDLE handle_token_new = NULL;
	HANDLE handle_token_duplicate = NULL;

	// 1) UserToken을 Queyr한다.
	auto result = WTSQueryUserToken(_session_id, &handle_token_new);

	// check) 
	ERROR_RETURN_IF(result == FALSE, false, LOG_PROGRESS << "(err ) fail to 'WTSQueryUserToken' ("sv << api::get_last_error() << ")"sv);

	// 2) Token을 Duplicate한다.
	result = ::DuplicateTokenEx(handle_token_new,MAXIMUM_ALLOWED,NULL,SecurityIdentification,TokenPrimary,&handle_token_duplicate);

	// check) 
	if(result == FALSE)
	{
		// - close handle
		::CloseHandle(handle_token_new);

		// return) 
		return false;
	}

	// 3) Token 핸들을 닫는다.
	::CloseHandle(handle_token_new);

	// declare) 
	uint32_t dwFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
	void*  penv_block = nullptr;

	// 4) EnvironmentBlock을 생성한다.
	if (_buf_environment.data() == nullptr)
	{
		result = ::CreateEnvironmentBlock(&penv_block, handle_token_duplicate, FALSE);

		// check)
		ERROR_RETURN_IF(result == FALSE, false, LOG_PROGRESS << "(err ) fail to 'CreateEnvironmentBlock' ("sv << api::get_last_error() << ")"sv);
	}
	else
	{
		penv_block = _buf_environment.data();
	}

	// 5) Procsss를 연다.
    result = ::CreateProcessAsUserA(handle_token_duplicate, const_cast<char*>(_str_execute.data()), _args, nullptr, nullptr, FALSE, dwFlag, penv_block, const_cast<char*>(_str_path_working.data()), p_psi, _ppi);

	// 6) Environemnt Block을 닫는다.
    if(_buf_environment.data() == nullptr && penv_block != nullptr)
	{
         ::DestroyEnvironmentBlock(penv_block);
	}

	// 7) Token Handle을 닫는다.
	::CloseHandle(handle_token_duplicate);

	// return) 
	return result;
 }
#elif defined(__linux__)

struct FORK_DATA
{
	object_ptr<server_admin::instance_Iservice>	pservice_entity;
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
		pfork_data->pservice_entity->m_instance.info.pid = pid;
		pfork_data->pservice_entity->m_instance.status.pid = pid;

		// declare) 
		int ret = 0;

		// - wait util sub process is end
		::wait(&ret);
		
		// check)
		if(WIFEXITED(ret) == false || WIFSIGNALED(ret))
		{
			pfork_data->pservice_entity->m_instance.info.pid = pid;
			pfork_data->pservice_entity->m_instance.status.pid = pid;
		}
	}

	// 3)
	delete pfork_data;

	// return)
	return nullptr;
}

bool create_process_to_session(server_admin::instance_Iservice* _pservice_entity, const std::string_view _str_execute, const shared_buffer& _buf_environment, const std::string_view _str_path_working, char* _args)
{
	// 1) set fork data
	FORK_DATA* pfork_data = new FORK_DATA;
	pfork_data->pservice_entity = _pservice_entity;
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

bool server_admin::instance_service_local::request_service_initialize()
{
	scoped_lock(this->m_cs_instance)
	{
		// check) Socket이 Empty()면... 
		RETURN_IF(this->m_psocket.exist(), false);

		// check)
		if (this->m_instance.status.state != eOBJECT_STATE::NONE)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::NONE'eOBJECT_STATE::NONE 'INITIALIZE service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return)
			return false;
		}

		// declare)
		char str_execute[512]{ 0 };

		// 1) 실행할 Full Path File을 설정한다.
		if (this->m_instance.setting.str_path.empty() == false)
		{
			if(this->m_instance.setting.str_path.back()=='/' || this->m_instance.setting.str_path.back() == '\\')
			{
				__sprintf(str_execute, "%s%s", this->m_instance.setting.str_path.c_str(), this->m_instance.setting.file_execute.c_str());
			}
			else
			{
				__sprintf(str_execute, "%s/%s", this->m_instance.setting.str_path.c_str(), this->m_instance.setting.file_execute.c_str());
			}
		}
		else
		{
			__sprintf(str_execute, "%s", this->m_instance.setting.file_execute.c_str());
		}

		// 2) Environment Block을 설정한다.
		if(this->m_instance.setting.buf_environment.size() == 0)
		{
			this->m_instance.setting.buf_environment.clear();
		}

		// log) 
		LOG_PROGRESS << "@ SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'INITIALIZE serivce'"sv;

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
			::ProcessIdToSessionId(::GetCurrentProcessId(), &id_session);

			// - Process를 생성한다.
			result = create_process_to_session(str_execute, this->m_instance.setting.buf_environment, this->m_instance.setting.str_path_working, nullptr, &sStartupInfo, &infoProcess, id_session);
		}
		// 3-B) Console mode일 경우 CreateProcess로 간단히 시작
		else
		{
			// - CreateProcess로 Process를 생성한다.
			result = ::CreateProcessA(
				str_execute,																// Application Name		__in_opt    LPCWSTR					lpApplicationName
				NULL,																		// Command line			__inout_opt LPWSTR					lpCommandLine
				NULL,																		// ProcessAttributes	__in_opt    LPSECURITY_ATTRIBUTES	lpProcessAttributes
				NULL,																		// ThreadAttributes		__in_opt    LPSECURITY_ATTRIBUTES	lpThreadAttributes
				FALSE,																		// Inherit Handle		__in        BOOL					bInheritHandles
				NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,	// Creation Flag		__in        DWORD					dwCreationFlags
				this->m_instance.setting.buf_environment.data(),							// Environment			__in_opt    LPVOID					lpEnvironment
				this->m_instance.setting.str_path_working.c_str(),							// CurrentDirectory 	__in_opt    LPCWSTR					lpCurrentDirectory
				&sStartupInfo,																// StartupInfo			__in        LPSTARTUPINFOW			lpStartupInfo
				&infoProcess																// Process Information	__out       LPPROCESS_INFORMATION	lpProcessInformation
			);
		}

		// check) 
		if (result == FALSE)
		{
			// log) 
			LOG_ERROR << "@ SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: failed 'INITIALIZE service'"sv;

			// return) 
			return false;
		}

		// 6) close exist handle
		if (this->process_handle != INVALID_PROCESS_HANDLE)
		{
			// log) 
			LOG_WARNING << "  (!) fail to get process handle of 'service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			::CloseHandle(this->process_handle);
		}

		// log) 
		LOG_PROGRESS << "@ SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: sucess 'INITIALIZEservice'"sv;	

		// 7) Process 정보를 설정한다.
		this->process_handle = infoProcess.hProcess;
		this->m_instance.info.pid = infoProcess.dwProcessId;
		this->m_instance.status.pid = infoProcess.dwProcessId;

		// 8) Process가 시작되길 기다린다.
		auto dwResult = WaitForInputIdle(infoProcess.hProcess, INFINITE);

		// 9) close handle
		::CloseHandle(infoProcess.hThread);

		// check) 
		if (dwResult != 0)
		{
			// log) 
			LOG_ERROR << "  (err) waiting error! fail to 'INITIALIZE service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return0
			return false;
		}


	#elif defined(__linux__)
			// - Process
		auto result = create_process_to_session(this, str_execute, this->m_instance.info.buf_environment, this->m_instance.info.str_path_working, nullptr);

		if(result == false)
		{
			// - ...
			this->process_handle = INVALID_PROCESS_HANDLE;

			// log) 
			LOG_ERROR << "  (err) fail to 'INITIALIZE service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return) 
			return false;
		}

	#else
		#error "CGDK Not supported platform"
	#endif

		// log) 
		LOG_INFO << "  - success on 'INITIALIZE service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;
	}

	// return) 
	return true;
}

bool server_admin::instance_service_local::request_service_destroy()
{
	// declare)
	bool return_v = true;

	scoped_lock(this->m_cs_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'DESTROY service'"sv;

		// check)
		if (this->m_instance.status.state == eOBJECT_STATE::NONE)
		{
			// log) 
			LOG_ERROR << "  fail: service state is already 'eOBJECT_STATE::NONE'eOBJECT_STATE::NONE 'DESTROY service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(this->m_instance.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'DESTROY service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'DESTROY service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "] "sv << log_endl
					  << "    reason"sv << _e.description() << "("sv << to_string<char>(_e.reason()) << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'DESTROY service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_admin::instance_service_local::request_service_start(const std::string& _setting)
{
	// declare)
	bool return_v = true;

	scoped_lock(this->m_cs_instance)
	{
		// log)
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'START service'"sv;

		// 1) 먼저 Reset;
		this->m_instance.setting.buf_configure.clear();

		try
		{
			// Case) 이미 떠 있는 service의 경우 그냥 start 신호만 전달한다.
			if (this->m_psocket.exist())
			{
				// check)
				THROW_IF(this->m_instance.status.state == eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::ALEADY_DONE, 0, "service is aleady opened"sv));

				// check)
				THROW_IF(this->m_instance.status.state != eOBJECT_STATE::STOPPED, throwable::failure(eRESULT::ALEADY_DONE, 0, "aleady requested or not ready"sv));

				// 1) start 신호를 전송한다.
				auto result = this->m_psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_START_SERVICE(this->m_instance.setting.iid, _setting));

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));
			}
			// Case) 떠 있지도 않은 서비스의 경우 일단 initialize부터 시작해서service가 접속되면 start 신호를 전송한다.
			else
			{
				// - start 신호 전송을 예약한다.(이건 반드시 RequestInitServer를 호출하기 전에 설정되어야 한다. 다중쓰레드문제로 인해...)
				this->m_is_preserve_start = true;

				// - Setting string을 저장해 놓는ㄷ다.
				this->m_str_confugure = _setting;

				//  - 먼저 InitService를 먼저 요청한다.
				this->request_service_initialize();
			}

			// log)
			LOG_INFO << "  - try to 'START service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'START service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "] "sv << log_endl
					  << "    reason"sv << _e.description() << "("sv << to_string<char>(_e.reason()) << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'START service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_admin::instance_service_local::request_service_stop()
{
	// declare)
	bool return_v = true;

	scoped_lock(this->m_cs_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'STOP service'"sv;

		// check)
		if (this->m_instance.status.state != eOBJECT_STATE::RUNNING)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::RUNNING 'STOP service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_STOP_SERVICE(this->m_instance.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'STOP service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << to_string<char>(_e.reason()) << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_admin::instance_service_local::request_service_pause()
{
	// declare)
	bool return_v = true;

	scoped_lock(this->m_cs_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'PAUSE service'"sv;

		// check)
		if (this->m_instance.status.state != eOBJECT_STATE::RUNNING)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::RUNNING 'PAUSE service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(this->m_instance.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'PAUSE service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << to_string<char>(_e.reason()) << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_admin::instance_service_local::request_service_continue()
{
	// declare)
	bool return_v = true;

	scoped_lock(this->m_cs_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]: request 'CONTINUE service'"sv;

		// check)
		if (this->m_instance.status.state != eOBJECT_STATE::PAUSED)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::PAUSED 'RESUME service['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// check) Socket이 Empty()면... 
			RETURN_IF(this->m_psocket.empty(), false);

			// 1) start신호를 전송한다.
			this->m_psocket->send(server_admin::message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(this->m_instance.setting.iid));

			// log)
			LOG_INFO << "  - try to 'CONTINUE service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'CONTINUE service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << to_string<char>(_e.reason()) << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'CONTINUE service'['"sv << this->m_instance.setting.nickname << "':"sv << this->m_instance.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

#if defined(_WIN32)
bool safe_terminate_process(PROCESS_HANDLE _handle_process, uintptr_t uExitCode)
{
	// declare) 
	DWORD dwTID = 0;
	DWORD dwCode = 0;
	DWORD dwErr = 0;
	PROCESS_HANDLE handle_process_dup = INVALID_PROCESS_HANDLE;
	PROCESS_HANDLE hRT = NULL;

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
		FARPROC	pfnExitProc	 = ::GetProcAddress(hKernel, "ExitProcess");

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
		::WaitForSingleObject((bDup) ? handle_process_dup : _handle_process, INFINITE);
		CloseHandle(hRT);
		is_success = true;
	}

	// 5) Duplicate한 Handle을 닫는다.
	SAFE_CLOSEHANDLE(handle_process_dup);

	// 6) 
	if(!is_success)
		::SetLastError(dwErr);
   
	// return) 
	return is_success;
}
#elif defined(__linux__)
bool safe_terminate_process(PROCESS_HANDLE /*_handle_process*/, uintptr_t /*uExitCode*/)
{
	return false;
}
#else
	#error "CGDK Not supported platform"
#endif

bool server_admin::instance_service_local::request_service_kill()
{
	// check) Socket이 Empty()면... 
	RETURN_IF(this->m_psocket.empty(), false);

	// check) Process Handle을 얻는다.
	RETURN_IF(this->process_handle == INVALID_PROCESS_HANDLE, false);

	// 1) handle_process값을 읽고 초기화한다.
	auto handle_process_target = this->process_handle;

	// 2) reset variable
	this->process_handle = INVALID_PROCESS_HANDLE;

	// 3) 해당 Process를 강제로 죽인다.
	auto result = safe_terminate_process(handle_process_target, (UINTPTR_MAX)-1);

	// check) 실패했는가!!
	RETURN_IF(result == false, false);

	// 3) 성공후 처리! 없음.
#if defined(_WIN32)
	::CloseHandle(handle_process_target);
#elif defined(__linux__)
	kill(handle_process_target, SIGINT);
#else
	#error "CGDK Not supported platform"
#endif

	// return) 
	return true;
}

bool server_admin::instance_service_local::request_service_update()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.str_path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	this->status.compare_exchange_weak(status_temp, 1);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status_temp != 0, false);

#ifdef _USE_SVN_CONTROL
	// trace) trace를 건다.
	LOG_PROGRESS << "@UPDATE requested ("sv << info_server_setting.nickname << ")"sv;
	LOG_PROGRESS << "  path : "sv << info_server_setting.str_path;
	LOG_PROGRESS << "  URL  : "sv << info_server_setting.strURL;
	if (info_server_setting.iRevision != -1)
	{
		LOG(log_type_t::progress | log_type_t::CONTINUE_) << "  rev  : "sv << info_server_setting.iRevision;
	}

	// 2) 디렉토리를 생성한다.
	//CreateDirectory(_pregister_info->info_server_setting.dirLocation, NULL);

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
	LOG_PROGRESS << "@UPDATE completed ("sv << this->m_instance.setting.nickname << ")"sv;

	// 5) 상태를 다시 0으로 바꾼다.
	this->status.exchange(0);

	// return) 
	return true;
}

bool server_admin::instance_service_local::request_service_check_out()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.str_path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	this->status.compare_exchange_weak(status_temp, 1);

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
	this->status.exchange(0);

	// return) 
	return true;
}

void server_admin::instance_service_local::reset_instance_status_info()
{
	// 1) Process를 닫는다.
	if(this->process_handle != INVALID_PROCESS_HANDLE)
	{
	#if defined(_WIN32)
		CloseHandle(this->process_handle);
	#elif defined(__linux__)
		kill(this->process_handle, SIGINT);
	#else
		#error "CGDK Not supported platform"
	#endif
	}
	this->process_handle = INVALID_PROCESS_HANDLE;

	// 2) Process의 사용률
	this->m_instance.status.clear();

	// 3) ...
	this->perfornamce_processor.reset();
}

bool server_admin::instance_service_local::request_service_switch()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.str_path.empty(), false);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(this->status != 0, false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	this->status.compare_exchange_weak(status_temp, 1);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(status_temp != 0, false);

#ifdef _USE_SVN_CONTROL
	// 2) Update를 요청한다.
	auto result = server_admin::SVN::Switch(info_server_setting.strURL, info_server_setting.str_path.c_str(), this);

	// 3) 완료 후 처리를 한다.
	if (result.IsFail())
	{
	}
#endif

	// 4) 상태를 다시 0으로 바꾼다.
	this->status.exchange(0);

	// return) 
	return true;
}
 
void server_admin::instance_service_local::set_instance_info(server_admin::sINSTANCE_SERVICE&& _instance_service)
{
	// declare) 
	std::string	str_file_path;
	std::string	str_file_name;
	PROCESS_HANDLE temp_process_handle = INVALID_PROCESS_HANDLE;

	// 5) get file path and filename
	if (this->process_handle == INVALID_PROCESS_HANDLE)
	{
	#if defined(_WIN32)
		// declare) 
		char str_exe[260];

		// 6) convert process id to process handle
		temp_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, static_cast<DWORD>(_instance_service.info.pid));

		// declare) 
		DWORD dwSize = sizeof(str_exe);

		// 7) get image name
		::QueryFullProcessImageNameA(temp_process_handle, 0, str_exe, &dwSize);
	#elif defined(__linux__)
		temp_process_handle = INVALID_PROCESS_HANDLE;
	#else
		#error "CGDK Not supported platform"
	#endif

		// declare) 
		char name[260];
		char str_ext[260];
		char str_drive[32];
		char str_path[260];

	#if defined(_WIN32)
		// - extract filename
		_splitpath_s(str_exe, str_drive, 32, str_path, 260, name, 260, str_ext, 260);
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// - splite file path and filename and store
		str_file_path = NFMT::format("{}{}", str_drive, str_path);
		str_file_name = NFMT::format("{}{}", name, str_ext);
	}

	// declare) 
	shared_buffer buf_update_requested_info;

	scoped_lock(this->m_cs_instance)
	{
		// 9) registered만 빼고 모두 옮긴다.(registered는 그대로 쓴다.)
		this->m_instance.info = std::move(_instance_service.info);
		this->m_instance.status = std::move(_instance_service.status);
		this->m_instance.vector_executable = std::move(_instance_service.vector_executable);
		
		// 10) instance id
		this->m_instance.info.host_iid = service::pmanager_instance->m_instance_host_self->get_iid();
		this->m_instance.info.time_update = chrono::time::utc::now();

		// case) process handle가 설정되지 않는 경우.
		if (this->process_handle == INVALID_PROCESS_HANDLE)
		{
			// - Process ID로 Pross Handle을 얻어낸다.
			this->process_handle = temp_process_handle;

			// - Path/execute/Working을 설정한다.
			this->m_instance.info.str_path = str_file_path;
			this->m_instance.info.file_execute = str_file_name;
			this->m_instance.info.str_path_working = str_file_path;
		}

	#if defined(_WIN32)
		// declare) 
		TCHAR	str_query[256];
		swprintf_s(str_query, 256, TEXT("\\Processor(%hs)\\%%Processor Time"), str_file_name.c_str());

		// 5) Performance Query를 수행한다.
		perfornamce_processor = make_object<api::performance_counter>(str_query);
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// 11)
		buf_update_requested_info = message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(this->get_iid(), service::pmanager_instance->m_instance_host_self->get_iid(), this->m_instance.setting);
	}

	// 2) 해당 server에게 registered 정보를 전송한다.
	if (buf_update_requested_info.exist())
	{
		this->send(buf_update_requested_info);
	}
}

void server_admin::instance_service_local::update_application_status()
{
	// check) 
	RETURN_IF(this->process_handle == INVALID_PROCESS_HANDLE);

	// declare) 
	PROCESS::sSTATUS process_status;

	// 1) Process
	process_status.usage_process = (perfornamce_processor.exist()) ? perfornamce_processor->get_value_long() : 0;

#if defined(_WIN32)
	// 2) Memory 정보를 얻는다.
	PROCESS_MEMORY_COUNTERS_EX	sInfoMemoryCounters;
	ZeroMemory(&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));
	sInfoMemoryCounters.cb = sizeof(sInfoMemoryCounters);
	GetProcessMemoryInfo((HANDLE)this->process_handle, (PPROCESS_MEMORY_COUNTERS)&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));
	process_status.bytes_memory_usage = sInfoMemoryCounters.PrivateUsage;
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
	process_status.size_traffic = 0;

	// 4) push_back
	this->m_instance.status.vector_process_status.push_back(process_status);

}


void server_admin::instance_service_local::load(std::string_view _str_path)
{
	this->m_instance.setting.load(_str_path);
}

void server_admin::instance_service_local::save()
{
	this->m_instance.setting.save();
}

bool server_admin::instance_service_local::process_sservice_disconnected(socket_server* _psocket)
{
	scoped_lock(this->m_cs_instance)
	{
		// check)
		RETURN_IF(this->m_psocket != _psocket, true);

		// 1) reset socket
		this->m_psocket.reset();

		// 2) state를 none으로..
		this->set_instance_state(eOBJECT_STATE::NONE);

		// 3) is registered;
		return this->m_instance.setting.rank.exist();
	}

	// return)
	return false;
}
