#include "pch.h"
#if defined(_WIN32)
	#include "Userenv.h"
	#include "Wtsapi32.h"
#elif defined(__linux__)
	#include <sys/wait.h>
	#include <cerrno>
	#include <pthread.h>
	#include <unistd.h>
	#include <spawn.h>
	#include <fcntl.h>
	#include <future>
	#include <cstring>

	extern char** environ;
#else
#endif

//#ifdef _USE_SVN_CONTROL
//, virtual public server_system::Isvn_notify
//#endif

server_system::instance_service_local::instance_service_local()
{
}

server_system::instance_service_local::~instance_service_local() noexcept
{
}

#if defined(_WIN32)
bool create_process_to_session(const std::wstring& _execute, const server_system::sSERVICE::sSETTING& _setting, wchar_t* _args, STARTUPINFOW* p_psi, PROCESS_INFORMATION* _ppi, UINT _session_id)
 {
	 // declare)
	HANDLE handle_token_new = NULL;
	HANDLE handle_token_duplicate = NULL;

	// 1) UserToken을 Query한다.
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
	void*  penv_base = nullptr;

	// 4) 사용자 기본 환경을 만든다.
	result = ::CreateEnvironmentBlock(&penv_base, handle_token_duplicate, FALSE);

	// check)
	if (result == FALSE)
	{
		// log)
		LOG_PROGRESS << "(err ) fail to 'CreateEnvironmentBlock' ("sv << api::get_last_error() << ")"sv;

		// - close handle
		::CloseHandle(handle_token_duplicate);

		// return)
		return false;
	}

	// 5) 기본 환경에 setting.environment를 추가/덮어쓴다. (없으면 기본 환경 그대로)
	std::wstring environment_block = _setting.get_environment_block(reinterpret_cast<const wchar_t*>(penv_base));
	void* penv_block = environment_block.empty() ? penv_base : environment_block.data();

	// 6) Process를 연다. (W API. 경로/인자는 UTF-8 -> UTF-16)
	auto working_path = server_system::utf8_to_utf16(_setting.working_path);
	result = ::CreateProcessAsUserW(handle_token_duplicate, _execute.c_str(), _args, nullptr, nullptr, FALSE, dwFlag, penv_block, working_path.empty() ? nullptr : working_path.c_str(), p_psi, _ppi);

	// 7) Environment Block을 닫는다.
	::DestroyEnvironmentBlock(penv_base);

	// 8) Token Handle을 닫는다.
	::CloseHandle(handle_token_duplicate);

	// return)
	return result;
 }
#elif defined(__linux__)

struct FORK_DATA
{
	object_ptr<server_system::instance_service>	pservice_entity;
	std::string					str_execute;				// - (fork thread에서 쓰므로 복사한다.)
	std::vector<std::string>	environment;				// - 환경 변수 ('name=value'. 부모 환경에 추가/덮어쓴다.)
	std::string					str_path_working;
	std::vector<std::string>	arguments;					// - 실행 인자 (실행 파일 이름 제외)

	// - exec 결과 (0: 성공, 그 외: exec의 errno. fork 실패는 -1)
	//   pipe(O_CLOEXEC)의 쓰기 쪽은 exec에 성공하면 닫히고(EOF), 실패하면 자식이 errno를 써서 알려준다.
	int							fd_exec_result[2]{ -1, -1 };
	std::shared_ptr<std::promise<int>> ppromise_exec;

	void					set_exec_result(int _result) noexcept
	{
		if (this->ppromise_exec)
		{
			try { this->ppromise_exec->set_value(_result); } catch (...) {}
			this->ppromise_exec.reset();
		}
	}
						~FORK_DATA() noexcept { this->close_exec_pipe(); }

	void					close_exec_pipe() noexcept
	{
		for (auto& iter : this->fd_exec_result)
		{
			if (iter >= 0) { close(iter); iter = -1; }
		}
	}
};

void* fn_fork(void* _param)
{
	// 1) fork data
	FORK_DATA* pfork_data = reinterpret_cast<FORK_DATA*>(_param);

	// - argv (실행 파일 이름 + 실행 인자 + nullptr. fork 전에 만든다.)
	auto& str_execute = pfork_data->str_execute;
	std::vector<char*> argv;
	argv.reserve(pfork_data->arguments.size() + 2);
	argv.push_back(str_execute.data());
	for (auto& iter : pfork_data->arguments)
		argv.push_back(iter.data());
	argv.push_back(nullptr);

	// - envp (부모 프로세스의 환경 + environment. 같은 이름은 environment가 덮어쓴다. fork 전에 만든다.)
	std::vector<std::string> environment;
	std::vector<char*> envp;
	if (pfork_data->environment.empty() == false)
	{
		const auto get_name = [](std::string_view _item) { return _item.substr(0, _item.find('=')); };

		for (char** iter = environ; iter != nullptr && *iter != nullptr; ++iter)
		{
			std::string_view item(*iter);
			auto is_overwritten = std::any_of(pfork_data->environment.begin(), pfork_data->environment.end(), [&](const std::string& _env) { return get_name(_env) == get_name(item); });
			if (is_overwritten == false)
				environment.emplace_back(item);
		}

		for (const auto& iter : pfork_data->environment)
			environment.push_back(iter);

		envp.reserve(environment.size() + 1);
		for (auto& iter : environment)
			envp.push_back(iter.data());
		envp.push_back(nullptr);
	}

	// 2) fork
	auto pid = fork();

	// check) throw exception if failed
	if (pid < 0)
	{
		// log)
		LOG_ERROR << "(err ) fail to fork '"sv << pfork_data->str_execute << "' (errno: "sv << errno << ")"sv;

		// - exec 결과 (실패)
		pfork_data->close_exec_pipe();
		pfork_data->set_exec_result(-1);

		// - delete fork data
		delete pfork_data;

		// return)
		return nullptr;
	}

	if(pid == 0)
	{
		//-----------------------------------------------------------------
		// child process
		//  - 여러 thread가 있는 process를 fork했으므로 exec 전까지는 async-signal-safe 함수만 쓴다.
		//    (다른 thread가 잡고 있던 lock은 풀리지 않는다. 메모리 할당/log/소멸자를 쓰면 멈출 수 있다.)
		//  - exec에 실패하면 _exit()로 바로 끝낸다.
		//    (exit()는 atexit/static 소멸자를 실행하므로 host_agent의 정리 작업을 하거나 멈춘 채로 남는다.)
		//-----------------------------------------------------------------
		// 1) signal (handler는 exec에서 기본값으로 바뀌지만 무시(SIG_IGN)와 mask는 그대로 넘어간다.)
		sigset_t sigset_empty;
		sigemptyset(&sigset_empty);
		sigprocmask(SIG_SETMASK, &sigset_empty, nullptr);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGHUP, SIG_IGN);

		// 2) stdin/stdout/stderr -> /dev/null
		if (int fd_null = open("/dev/null", O_RDWR); fd_null >= 0)
		{
			dup2(fd_null, 0);
			dup2(fd_null, 1);
			dup2(fd_null, 2);
			if (fd_null > 2)
				close(fd_null);
		}

		// 3) host_agent의 다른 fd(listen socket, 접속 등)는 넘기지 않는다.
		//    (넘기면 service가 host_agent의 port를 잡고 있게 된다. exec 결과 pipe(O_CLOEXEC)만 남긴다.)
		{
			const int fd_keep = pfork_data->fd_exec_result[1];
		#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 34))
			const bool is_closed = (fd_keep < 3)
				? (close_range(3, ~0U, 0) == 0)
				: ((fd_keep == 3 || close_range(3, static_cast<unsigned int>(fd_keep - 1), 0) == 0) && close_range(static_cast<unsigned int>(fd_keep + 1), ~0U, 0) == 0);
		#else
			const bool is_closed = false;
		#endif
			if (is_closed == false)
			{
				for (int fd = 3; fd < 65536; ++fd)
				{
					if (fd != fd_keep)
						close(fd);
				}
			}
		}

		// 4) working directory and new session
		[[maybe_unused]] auto result = chdir( (pfork_data->str_path_working.empty() == false) ? pfork_data->str_path_working.data(): "/" );
		setsid();

		// 5) exec (environment가 있으면 execve)
		if(envp.empty())
		{
			execv(str_execute.c_str(), argv.data());
		}
		else
		{
			execve(str_execute.c_str(), argv.data(), envp.data());
		}

		// 6) exec 실패 (errno를 부모에게 알려준다.)
		{
			int error_exec = errno;
			[[maybe_unused]] auto result_write = write(pfork_data->fd_exec_result[1], &error_exec, sizeof(error_exec));
		}
		_exit(127);
	}
	else
	{
		// - exec 결과를 기다린다. (exec에 성공하면 쓰기 쪽이 닫혀서 EOF가 온다.)
		{
			close(pfork_data->fd_exec_result[1]);
			pfork_data->fd_exec_result[1] = -1;

			int error_exec = 0;
			ssize_t size_read;
			do
			{
				size_read = read(pfork_data->fd_exec_result[0], &error_exec, sizeof(error_exec));
			} while (size_read < 0 && errno == EINTR);

			pfork_data->close_exec_pipe();
			pfork_data->set_exec_result((size_read == static_cast<ssize_t>(sizeof(error_exec))) ? (error_exec != 0 ? error_exec : -1) : 0);
		}

		// - set info
		pfork_data->pservice_entity->m_instance.info.info.pid = pid;
		pfork_data->pservice_entity->m_instance.info.info.pid = pid;

		// declare)
		int ret = 0;

		// - wait until this sub process is end (다른 자식 프로세스가 아니라 이 pid만 기다린다. 시그널로 중단되면 다시 기다린다.)
		pid_t result_wait;
		do
		{
			result_wait = ::waitpid(pid, &ret, 0);
		} while (result_wait < 0 && errno == EINTR);

		// log) 정상 종료/비정상 종료(시그널)
		if (result_wait == pid && WIFEXITED(ret))
		{
			LOG_INFO << "@ process '"sv << pfork_data->str_execute << "' (pid: "sv << pid << ") exited (exit code: "sv << WEXITSTATUS(ret) << ")"sv;
		}
		else if (result_wait == pid && WIFSIGNALED(ret))
		{
			LOG_WARNING << "(warn) process '"sv << pfork_data->str_execute << "' (pid: "sv << pid << ") terminated by signal "sv << WTERMSIG(ret);
		}
		else
		{
			LOG_WARNING << "(warn) fail to wait process '"sv << pfork_data->str_execute << "' (pid: "sv << pid << ", errno: "sv << errno << ")"sv;
		}

		// - process가 끝났으므로 pid를 지운다.
		pfork_data->pservice_entity->m_instance.info.info.pid = 0;
		pfork_data->pservice_entity->m_instance.info.info.pid = 0;
	}

	// 3)
	delete pfork_data;

	// return)
	return nullptr;
}

bool create_process_to_session(server_system::instance_service* _pservice_entity, const std::string_view _str_execute, const std::vector<std::string>& _environment, const std::string_view _str_path_working, std::vector<std::string>&& _arguments)
{
	// 1) set fork data
	FORK_DATA* pfork_data = new FORK_DATA;
	pfork_data->pservice_entity = _pservice_entity;
	pfork_data->str_execute = std::string(_str_execute);
	pfork_data->environment = _environment;
	pfork_data->str_path_working = std::string(_str_path_working);
	pfork_data->arguments = std::move(_arguments);

	// - exec 결과를 받을 pipe (O_CLOEXEC: exec에 성공하면 자식 쪽이 닫힌다.)
	if (pipe2(pfork_data->fd_exec_result, O_CLOEXEC) != 0)
	{
		LOG_ERROR << "(err ) fail to create pipe for '"sv << _str_execute << "' (errno: "sv << errno << ")"sv;
		delete pfork_data;
		return false;
	}
	pfork_data->ppromise_exec = std::make_shared<std::promise<int>>();
	auto future_exec = pfork_data->ppromise_exec->get_future();

	// declare)
	pthread_t thread_fork;

	// 2) begin thread (pthread_create는 실패하면 0이 아닌 오류 코드를 돌려준다.)
	auto result = pthread_create(&thread_fork, nullptr, fn_fork, reinterpret_cast<void*>(pfork_data));

	// check)
	if (result != 0)
	{
		// log)
		LOG_ERROR << "(err ) fail to create fork thread for '"sv << _str_execute << "' (error: "sv << result << ")"sv;

		// - delete fork data
		pfork_data->close_exec_pipe();
		delete pfork_data;

		// return)
		return false;
	}

	// 3) detach (join하지 않으므로 끝나면 자원이 바로 해제되도록 한다.)
	pthread_detach(thread_fork);

	// 4) exec 결과를 기다린다. (실행 파일이 없거나 권한이 없으면 실패로 돌려준다.)
	if (future_exec.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
	{
		LOG_WARNING << "(warn) no exec result of '"sv << _str_execute << "' (timeout)"sv;
		return true;
	}

	auto result_exec = future_exec.get();
	if (result_exec != 0)
	{
		LOG_ERROR << "(err ) fail to execute '"sv << _str_execute << "' (errno: "sv << result_exec << ((result_exec > 0) ? std::string(", ") + std::strerror(result_exec) : std::string()) << ")"sv;
		return false;
	}

	// return) 
	return true;
}
#else
#endif

namespace
{
	// - start setting(json 문자열) -> context
	context _make_context_from_json(std::string_view _json)
	{
		context result;
		if (_json.empty() == false)
			result.from_json(_json);
		return result;
	}
}

//-----------------------------------------------------------------------------
// service type (service / docker container)
//
//  - eSERVICE_TYPE::SERVICE : setting.name을 서비스 이름으로 시작/중지한다.
//                             (windows: windows service(SCM), linux: 'systemctl start/stop <name>.service')
//  - eSERVICE_TYPE::DOCKER  : setting.name을 container 이름으로 'docker start/stop <name>'을 실행한다.
//-----------------------------------------------------------------------------
namespace
{
	// - 명령을 실행하고 끝날 때까지 기다린다. (종료 코드. 실행하지 못하면 -1. windows는 최대 _wait_milliseconds까지 기다린다.)
	int _run_command(const std::vector<std::string>& _arguments, uint32_t _wait_milliseconds = 60'000, bool _is_quiet = false)
	{
		// declare)
		int exit_code = -1;

	#if defined(_WIN32)
		// 1) command line
		std::string command_line;
		for (const auto& iter : _arguments)
		{
			if (command_line.empty() == false)
				command_line += ' ';

			command_line += server_system::sSERVICE::sSETTING::quote_argument(iter);
		}

		STARTUPINFOW startup_info{};
		startup_info.cb = sizeof(startup_info);
		PROCESS_INFORMATION process_info{};
		auto wcommand_line = server_system::utf8_to_utf16(command_line);

		(void)_is_quiet;

		// 2) create process (창 없이. W API)
		if (::CreateProcessW(nullptr, wcommand_line.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info) == FALSE)
		{
			LOG_ERROR << "(err ) fail to run '"sv << command_line << "' ("sv << ::GetLastError() << ")"sv;
			return -1;
		}

		// 3) wait (최대 _wait_milliseconds)
		if (::WaitForSingleObject(process_info.hProcess, _wait_milliseconds) == WAIT_OBJECT_0)
		{
			DWORD code = 0;
			::GetExitCodeProcess(process_info.hProcess, &code);
			exit_code = static_cast<int>(code);
		}

		::CloseHandle(process_info.hThread);
		::CloseHandle(process_info.hProcess);
	#elif defined(__linux__)
		(void)_wait_milliseconds;

		// 1) argv
		std::vector<std::string> arguments = _arguments;
		std::vector<char*> argv;
		for (auto& iter : arguments)
			argv.push_back(iter.data());
		argv.push_back(nullptr);

		// 2) spawn (_is_quiet: stdout/stderr를 /dev/null로 보낸다.)
		posix_spawn_file_actions_t file_actions;
		posix_spawn_file_actions_init(&file_actions);
		if (_is_quiet)
		{
			posix_spawn_file_actions_addopen(&file_actions, 1, "/dev/null", O_WRONLY, 0);
			posix_spawn_file_actions_addopen(&file_actions, 2, "/dev/null", O_WRONLY, 0);
		}

		//    host_agent의 다른 fd(listen socket 등)는 넘기지 않는다.
	#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 34))
		posix_spawn_file_actions_addclosefrom_np(&file_actions, 3);
	#endif

		pid_t pid = 0;
		auto result_spawn = ::posix_spawnp(&pid, argv[0], &file_actions, nullptr, argv.data(), environ);
		posix_spawn_file_actions_destroy(&file_actions);

		if (result_spawn != 0)
		{
			LOG_ERROR << "(err ) fail to run '"sv << _arguments.front() << "' (errno: "sv << errno << ")"sv;
			return -1;
		}

		// 3) wait
		int status = 0;
		pid_t result_wait;
		do
		{
			result_wait = ::waitpid(pid, &status, 0);
		} while (result_wait < 0 && errno == EINTR);

		if (result_wait == pid && WIFEXITED(status))
			exit_code = WEXITSTATUS(status);
	#endif

		// return)
		return exit_code;
	}

	// - 서비스를 시작/중지한다. (windows: SCM, linux: systemctl)
	bool _control_service(const std::string& _service_name, bool _is_start)
	{
		// check)
		if (_service_name.empty())
		{
			LOG_ERROR << "(err ) service name is empty"sv;
			return false;
		}

	#if defined(_WIN32)
		auto wservice_name = server_system::utf8_to_utf16(_service_name);
		auto result = _is_start ? (api::service_start(std::wstring_view(wservice_name), 0, nullptr) != 0) : (api::service_stop(std::wstring_view(wservice_name)) != 0);
	#elif defined(__linux__)
		auto result = _run_command({ "systemctl", _is_start ? "start" : "stop", _service_name + ".service" }) == 0;
	#else
		auto result = false;
	#endif

		// check)
		if (result == false)
			LOG_ERROR << "(err ) fail to "sv << (_is_start ? "start"sv : "stop"sv) << " service '"sv << _service_name << "'"sv;

		return result;
	}

	// - 'docker <command> [-t <timeout>] <container>'을 실행한다. (종료 코드가 0이면 성공. _timeout_seconds가 0이면 -t를 넣지 않는다.)
	bool _run_docker(std::string_view _command, const std::string& _container, uint32_t _timeout_seconds = 0)
	{
		// check)
		if (_container.empty())
		{
			LOG_ERROR << "(err ) docker container name is empty"sv;
			return false;
		}

		// 1) run
		std::vector<std::string> arguments{ "docker", std::string(_command) };
		if (_timeout_seconds != 0)
		{
			arguments.push_back("-t");
			arguments.push_back(std::to_string(_timeout_seconds));
		}
		arguments.push_back(_container);

		// - stop의 대기 시간(-t)보다 60초 더 기다린다.
		auto exit_code = _run_command(arguments, (_timeout_seconds + 60) * 1'000);

		// check)
		if (exit_code != 0)
		{
			LOG_ERROR << "(err ) fail to 'docker "sv << _command << " "sv << _container << "' (exit code: "sv << exit_code << ")"sv;
			return false;
		}

		// return)
		return true;
	}
}

bool server_system::instance_service_local::request_service_initialize()
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		// check) Socket이 Empty()면... 
		RETURN_IF(this->m_psocket.exist(), false);

		// check)
		if (this->m_instance.info.status.state != eOBJECT_STATE::NONE)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::NONE'eOBJECT_STATE::NONE 'INITIALIZE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return)
			return false;
		}

		// - service / docker container는 이름(setting.name)으로 시작한다. (process handle은 갖지 않는다.)
		if (this->m_instance.info.setting.service_type != server_system::eSERVICE_TYPE::EXECUTE)
		{
			// log)
			LOG_PROGRESS << "@ SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'INITIALIZE service' ("sv << server_system::to_service_type_string(this->m_instance.info.setting.service_type) << ")"sv;

			auto result = (this->m_instance.info.setting.service_type == server_system::eSERVICE_TYPE::SERVICE)
						? _control_service(this->m_instance.info.setting.name, true)
						: _run_docker("start"sv, this->m_instance.info.setting.name);

			// check)
			RETURN_IF(result == false, false);

			// log)
			LOG_INFO << "  - success on 'INITIALIZE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return)
			return true;
		}

		// declare)
		char str_execute[512]{ 0 };

		// 1) 실행할 Full Path File을 설정한다.
		if (this->m_instance.info.setting.path.empty() == false)
		{
			if(this->m_instance.info.setting.path.back()=='/' || this->m_instance.info.setting.path.back() == '\\')
			{
				std::snprintf(str_execute, std::size(str_execute), "%s%s", this->m_instance.info.setting.path.c_str(), this->m_instance.info.setting.filename.c_str());
			}
			else
			{
				std::snprintf(str_execute, std::size(str_execute), "%s/%s", this->m_instance.info.setting.path.c_str(), this->m_instance.info.setting.filename.c_str());
			}
		}
		else
		{
			std::snprintf(str_execute, std::size(str_execute), "%s", this->m_instance.info.setting.filename.c_str());
		}

		// log) 
		LOG_PROGRESS << "@ SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'INITIALIZE service'"sv;

	#if defined(_WIN32)
		// - 실행 파일/실행 인자 (W API. UTF-8 -> UTF-16. arguments가 있으면 "<실행 파일>" <인자...>로 넘긴다. 없으면 nullptr)
		std::wstring str_execute_w = server_system::utf8_to_utf16(str_execute);
		std::wstring str_command_line = (this->m_instance.info.setting.arguments.empty() == false) ? server_system::utf8_to_utf16(this->m_instance.info.setting.get_arguments_string(str_execute)) : std::wstring();
		wchar_t* pcommand_line = str_command_line.empty() ? nullptr : str_command_line.data();

		// 3) reset STARTUPINFO
		STARTUPINFOW sStartupInfo;
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
			result = create_process_to_session(str_execute_w, this->m_instance.info.setting, pcommand_line, &sStartupInfo, &infoProcess, id_session);
		}
		// 3-B) Console mode일 경우 CreateProcess로 간단히 시작
		else
		{
			// - 환경 변수 (부모 프로세스의 환경에 environment를 추가/덮어쓴다. 없으면 nullptr(부모 환경 그대로))
			auto penv_parent = ::GetEnvironmentStringsW();
			std::wstring str_environment = this->m_instance.info.setting.get_environment_block(penv_parent);
			::FreeEnvironmentStringsW(penv_parent);
			void* penvironment = str_environment.empty() ? nullptr : str_environment.data();
			auto str_working_path = server_system::utf8_to_utf16(this->m_instance.info.setting.working_path);

			// - CreateProcess로 Process를 생성한다.
			result = ::CreateProcessW(
				str_execute_w.c_str(),																// Application Name		__in_opt    LPCWSTR					lpApplicationName
				pcommand_line,																// Command line			__inout_opt LPWSTR					lpCommandLine
				NULL,																		// ProcessAttributes	__in_opt    LPSECURITY_ATTRIBUTES	lpProcessAttributes
				NULL,																		// ThreadAttributes		__in_opt    LPSECURITY_ATTRIBUTES	lpThreadAttributes
				FALSE,																		// Inherit Handle		__in        BOOL					bInheritHandles
				NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,	// Creation Flag		__in        DWORD					dwCreationFlags
				penvironment,																// Environment			__in_opt    LPVOID					lpEnvironment
				str_working_path.empty() ? nullptr : str_working_path.c_str(),					// CurrentDirectory 	__in_opt    LPCWSTR					lpCurrentDirectory
				&sStartupInfo,																// StartupInfo			__in        LPSTARTUPINFOW			lpStartupInfo
				&infoProcess																// Process Information	__out       LPPROCESS_INFORMATION	lpProcessInformation
			);
		}

		// check) 
		if (result == FALSE)
		{
			// log) 
			LOG_ERROR << "@ SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: failed 'INITIALIZE service'"sv;

			// return) 
			return false;
		}

		// 6) close exist handle
		if (this->process_handle != INVALID_PROCESS_HANDLE)
		{
			// log) 
			LOG_WARNING << "  (!) fail to get process handle of 'service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			::CloseHandle(this->process_handle);
		}

		// log) 
		LOG_PROGRESS << "@ SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: success 'INITIALIZEservice'"sv;

		// 7) Process 정보를 설정한다.
		this->process_handle = infoProcess.hProcess;
		this->m_instance.info.info.pid = infoProcess.dwProcessId;
		this->m_instance.info.info.pid = infoProcess.dwProcessId;

		// 8) Process가 시작되길 기다린다.
		auto dwResult = WaitForInputIdle(infoProcess.hProcess, INFINITE);

		// 9) close handle
		::CloseHandle(infoProcess.hThread);

		// check) 
		if (dwResult != 0)
		{
			// log) 
			LOG_ERROR << "  (err) waiting error! fail to 'INITIALIZE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return0
			return false;
		}


	#elif defined(__linux__)
			// - Process
		auto result = create_process_to_session(this, str_execute, this->m_instance.info.setting.environment, this->m_instance.info.setting.working_path, std::vector<std::string>(this->m_instance.info.setting.arguments));

		if(result == false)
		{
			// - ...
			this->process_handle = INVALID_PROCESS_HANDLE;

			// log) 
			LOG_ERROR << "  (err) fail to 'INITIALIZE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return) 
			return false;
		}

	#else
		#error "CGDK Not supported platform"
	#endif

		// log) 
		LOG_INFO << "  - success on 'INITIALIZE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;
	}

	// return) 
	return true;
}

bool server_system::instance_service_local::request_service_destroy()
{
	// declare)
	bool return_v = true;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'DESTROY service'"sv;

		// check)
		if (this->m_instance.info.status.state == eOBJECT_STATE::NONE)
		{
			// log) 
			LOG_ERROR << "  fail: service state is already 'eOBJECT_STATE::NONE'eOBJECT_STATE::NONE 'DESTROY service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(this->m_instance.info.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'DESTROY service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'DESTROY service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "] "sv << log_endl
					  << "    reason"sv << _e.description() << "("sv << _e.reason<int>() << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'DESTROY service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_system::instance_service_local::request_service_start(const std::string& _setting)
{
	// declare)
	bool return_v = true;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// log)
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'START service'"sv;

		// 1) 먼저 Reset; (start context)
		this->m_instance.info.setting.context_start.clear();

		try
		{
			// Case) 이미 떠 있는 service의 경우 그냥 start 신호만 전달한다.
			if (this->m_psocket.exist())
			{
				// check)
				THROW_IF(this->m_instance.info.status.state == eOBJECT_STATE::RUNNING, throwable::failure(eRESULT::ALREADY_DONE, 0, "service is already opened"sv));

				// check)
				THROW_IF(this->m_instance.info.status.state != eOBJECT_STATE::STOPPED, throwable::failure(eRESULT::ALREADY_DONE, 0, "already requested or not ready"sv));

				// 1) start 신호를 전송한다.
				auto result = this->m_psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_START_SERVICE(this->m_instance.info.setting.iid, _make_context_from_json(_setting)));

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));
			}
			// Case) 떠 있지도 않은 서비스의 경우 일단 initialize부터 시작해서service가 접속되면 start 신호를 전송한다.
			else
			{
				// - start 신호 전송을 예약한다.(이건 반드시 RequestInitServer를 호출하기 전에 설정되어야 한다. 다중스레드문제로 인해...)
				this->m_is_preserve_start = true;

				// - Setting string을 저장해 놓는ㄷ다.
				this->m_str_configure = _setting;

				//  - 먼저 InitService를 먼저 요청한다.
				this->request_service_initialize();
			}

			// log)
			LOG_INFO << "  - try to 'START service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'START service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "] "sv << log_endl
					  << "    reason"sv << _e.description() << "("sv << _e.reason<int>() << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'START service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_system::instance_service_local::request_service_stop()
{
	// declare)
	bool return_v = true;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'STOP service'"sv;

		// check)
		if (this->m_instance.info.status.state != eOBJECT_STATE::RUNNING)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::RUNNING 'STOP service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(this->m_instance.info.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'STOP service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << _e.reason<int>() << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_system::instance_service_local::request_service_pause()
{
	// declare)
	bool return_v = true;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'PAUSE service'"sv;

		// check)
		if (this->m_instance.info.status.state != eOBJECT_STATE::RUNNING)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::RUNNING 'PAUSE service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

			// return)
			return false;
		}

		try
		{
			// check) Socket이 Empty()면... 
			THROW_IF(this->m_psocket.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready"sv));

			// 1) start신호를 전송한다.
			auto result = this->m_psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(this->m_instance.info.setting.iid));

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "fail to send request to service"sv));

			// log)
			LOG_INFO << "  - try to 'PAUSE service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;
		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << _e.reason<int>() << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'STOP service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

			// - set
			return_v = false;
		}
	}

	// return) 
	return return_v;
}

bool server_system::instance_service_local::request_service_continue()
{
	// declare)
	bool return_v = true;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// log) 
		LOG_PROGRESS << "@SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: request 'CONTINUE service'"sv;

		// check)
		if (this->m_instance.info.status.state != eOBJECT_STATE::PAUSED)
		{
			// log) 
			LOG_ERROR << "  fail: service state is not 'eOBJECT_STATE::PAUSED 'RESUME service['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]'"sv;

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
			this->m_psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(this->m_instance.info.setting.iid));

			// log)
			LOG_INFO << "  - try to 'CONTINUE service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

		}
		catch (throwable::failure& _e)
		{
			// log)
			LOG_ERROR << "  - fail to 'CONTINUE service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "] "sv << log_endl
				<< "    reason"sv << _e.description() << "("sv << _e.reason<int>() << ")"sv << log_endl;

			// - set
			return_v = false;
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "  - fail to 'CONTINUE service'['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]"sv;

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

bool server_system::instance_service_local::request_service_kill()
{
	// - service / docker container는 이름(setting.name)으로 중지한다.
	if (this->m_instance.info.setting.service_type == server_system::eSERVICE_TYPE::SERVICE)
		return _control_service(this->m_instance.info.setting.name, false);

	if (this->m_instance.info.setting.service_type == server_system::eSERVICE_TYPE::DOCKER)
		return _run_docker("stop"sv, this->m_instance.info.setting.name, this->m_instance.info.setting.stop_grace_period);

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

bool server_system::instance_service_local::request_service_update()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	//    (strong : weak는 이유 없이 실패할 수 있고, 그러면 status_temp가 0인 채로 잠금 없이 진행된다.)
	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(this->status.compare_exchange_strong(status_temp, 1) == false, false);

#ifdef _USE_SVN_CONTROL
	// trace) trace를 건다.
	LOG_PROGRESS << "@UPDATE requested ("sv << info_server_setting.nickname << ")"sv;
	LOG_PROGRESS << "  path : "sv << info_server_setting.str_path;
	LOG_PROGRESS << "  URL  : "sv << info_server_setting.strURL;
	if (info_server_setting.iRevision != -1)
	{
		LOG(log_type_t::progress | log_type_t::CONTINUE_) << "  rev  : "sv << info_server_setting.iRevision;
	}

	// 2) 디렉터리를 생성한다.
	//CreateDirectory(_pregister_info->info_server_setting.dirLocation, NULL);

	// declare) 
	svn_client_info2_t svn_info;

	// 3) 정보를 얻는다.
	SVN_RESULT result = server_system::SVN::GetInfo(info_server_setting.str_path.c_str(), svn_info, this);

	// 4) Error가 갔으면 CheckOupt하고 아니면 Update를 한다.
	if (result.IsSuccess())
	{
		// 4-1) Head version이면 Update를 수행한다.
		result = server_system::SVN::Update(info_server_setting.str_path.c_str(), info_server_setting.iRevision, this);

		// check) 완료 후 처리를 한다.
		if (result.IsSuccess())
		{
		}
	}
	else
	{
		// - Update를 요청한다.
		result = server_system::SVN::CheckOut(info_server_setting.str_path.c_str(), info_server_setting.strURL, this);

		// check) 완료 후 처리를 한다.
		if (result.IsSuccess())
		{
		}
	}
#endif

	// Log) 
	LOG_PROGRESS << "@UPDATE completed ("sv << this->m_instance.info.setting.name << ")"sv;

	// 5) 상태를 다시 0으로 바꾼다.
	this->status.exchange(0);

	// return) 
	return true;
}

bool server_system::instance_service_local::request_service_check_out()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.path.empty(), false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	//    (strong : weak는 이유 없이 실패할 수 있고, 그러면 status_temp가 0인 채로 잠금 없이 진행된다.)
	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(this->status.compare_exchange_strong(status_temp, 1) == false, false);

#ifdef _USE_SVN_CONTROL
	// declare) 
	SVN_RESULT	result;

	// 2) Update를 요청한다.
	result = server_system::SVN::CheckOut(info_server_setting.strURL, info_server_setting.str_path.c_str(), this);

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

void server_system::instance_service_local::reset_instance_status_info()
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
	this->m_instance.info.status.clear();

	// 3) ...
	this->performance_processor.reset();
}

bool server_system::instance_service_local::request_service_switch()
{
	// check) URL이 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.revision.url.empty(), false);

	// check) Path가 설정되어 있지 않으면 그냥 끝낸다.
	RETURN_IF(this->m_instance.info.setting.path.empty(), false);

	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(this->status != 0, false);

	// declare)
	int	status_temp = 0;

	// 1) 이전 상태가 0일 때만 1로 바꾼다.
	//    (strong : weak는 이유 없이 실패할 수 있고, 그러면 status_temp가 0인 채로 잠금 없이 진행된다.)
	// check) 상태가 0일 경우만 Update를 실행한다.
	RETURN_IF(this->status.compare_exchange_strong(status_temp, 1) == false, false);

#ifdef _USE_SVN_CONTROL
	// 2) Update를 요청한다.
	auto result = server_system::SVN::Switch(info_server_setting.strURL, info_server_setting.str_path.c_str(), this);

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
 
void server_system::instance_service_local::set_instance_info(server_system::sSERVICE&& _instance_service)
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
		char name[260]{};
		char str_ext[260]{};
		char str_drive[32]{};
		char str_path[260]{};

	#if defined(_WIN32)
		// - extract filename
		_splitpath_s(str_exe, str_drive, 32, str_path, 260, name, 260, str_ext, 260);
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// - splite file path and filename and store
		str_file_path = CGDK::stream_string(str_drive, str_path);
		str_file_name = CGDK::stream_string(name, str_ext);
	}

	// declare) 
	shared_buffer buf_update_requested_info;

	SCOPED_LOCK(this->m_lockable_instance)
	{
		// 9) registered만 빼고 모두 옮긴다.(registered는 그대로 쓴다.)
		this->m_instance.info.info = std::move(_instance_service.info);
		this->m_instance.info.status = std::move(_instance_service.status);
		// (vector_executable is not in sSERVICE. it is updated by the executor messages)
		
		// 10) instance id
		this->m_instance.info.info.host_iid = ::service::pmanager_instance->m_instance_host_self->get_iid();
		this->m_instance.info.info.time_update = chrono::time::utc::now();

		// case) process handle가 설정되지 않는 경우.
		if (this->process_handle == INVALID_PROCESS_HANDLE)
		{
			// - Process ID로 Pross Handle을 얻어낸다.
			this->process_handle = temp_process_handle;

			// - Path/execute/Working을 설정한다.
			this->m_instance.info.setting.path = str_file_path;
			this->m_instance.info.setting.filename = str_file_name;
			this->m_instance.info.setting.working_path = str_file_path;
		}

	#if defined(_WIN32)
		// declare) 
		TCHAR	str_query[256];
		swprintf_s(str_query, 256, TEXT("\\Processor(%hs)\\%%Processor Time"), str_file_name.c_str());

		// 5) Performance Query를 수행한다.
		performance_processor = make_object<api::performance_counter>(str_query);
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// 11)
		buf_update_requested_info = server_system::make_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(this->get_iid(), ::service::pmanager_instance->m_instance_host_self->get_iid(), this->m_instance.info.setting);
	}

	// 2) 해당 server에게 registered 정보를 전송한다.
	if (buf_update_requested_info.exist())
	{
		this->send(buf_update_requested_info);
	}
}

bool server_system::instance_service_local::process_check_service_running()
{
	// - service type이 SERVICE/DOCKER일 때 실행 중인지 확인한다. (process handle이 없다.)
	const auto& setting = this->m_instance.info.setting;

	if (setting.service_type == server_system::eSERVICE_TYPE::SERVICE)
	{
	#if defined(_WIN32)
		// - windows: SCM에 상태를 묻는다.
		bool is_running = false;
		auto hscm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (hscm != nullptr)
		{
			auto hservice = ::OpenServiceW(hscm, server_system::utf8_to_utf16(setting.name).c_str(), SERVICE_QUERY_STATUS);
			if (hservice != nullptr)
			{
				SERVICE_STATUS service_status{};
				is_running = ::QueryServiceStatus(hservice, &service_status) != FALSE && service_status.dwCurrentState == SERVICE_RUNNING;
				::CloseServiceHandle(hservice);
			}
			::CloseServiceHandle(hscm);
		}
		return is_running;
	#else
		// - linux: 'systemctl is-active --quiet <name>.service' (실행 중이면 0)
		return _run_command({ "systemctl", "is-active", "--quiet", setting.name + ".service" }, 10'000, true) == 0;
	#endif
	}

	if (setting.service_type == server_system::eSERVICE_TYPE::DOCKER)
	{
		// - 'docker top <name>' (실행 중이면 0. 멈춰 있거나 없으면 오류)
		return _run_command({ "docker", "top", setting.name }, 10'000, true) == 0;
	}

	// return)
	return false;
}

void server_system::instance_service_local::update_application_status()
{
	// - SERVICE/DOCKER: 5초마다 실행 중인지 확인하고, 멈췄으면 상태를 NONE으로 한다.
	if (this->m_instance.info.setting.service_type != server_system::eSERVICE_TYPE::EXECUTE)
	{
		auto time_now = std::chrono::steady_clock::now();
		RETURN_IF(time_now < this->m_time_check_service_status);
		this->m_time_check_service_status = time_now + std::chrono::seconds(5);

		if (this->process_check_service_running() == false && this->m_instance.info.status.state != eOBJECT_STATE::NONE)
		{
			// log)
			LOG_WARNING << "(warn) SERVICE['"sv << this->m_instance.info.setting.name << "':"sv << this->m_instance.info.setting.iid << "]: is not running ("sv << server_system::to_service_type_string(this->m_instance.info.setting.service_type) << ")"sv;

			this->set_instance_state(eOBJECT_STATE::NONE);
		}
		return;
	}

	// check) 
	RETURN_IF(this->process_handle == INVALID_PROCESS_HANDLE);

	// declare) 
	PROCESS::sSTATUS process_status;

	// 1) Process
	process_status.usage_process = (performance_processor.exist()) ? performance_processor->get_value_long() : 0;

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
	this->m_vector_process_status.push_back(process_status);

}


void server_system::instance_service_local::load(std::string_view _str_path)
{
	// - setting json 파일을 읽어 instance 정보로 설정한다. (service::Ninstance::load_from)
	context context_setting;
	context_setting.from_json(api::file::read_file(_str_path));
	this->m_instance.load_from(std::move(context_setting));
}

void server_system::instance_service_local::save()
{
	// - sSERVICE::sSETTING에는 save가 없다. (설정 파일 저장은 지원하지 않는다.)
	LOG_WARNING << "(warn) SERVICE['"sv << this->m_instance.info.setting.name << "']: save() is not supported"sv;
}

bool server_system::instance_service_local::process_sservice_disconnected(socket_peer* _psocket)
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		// check)
		RETURN_IF(this->m_psocket != _psocket, true);

		// 1) reset socket
		this->m_psocket.reset();

		// 2) state를 none으로..
		this->set_instance_state(eOBJECT_STATE::NONE);

		// 3) is registered;
		return this->m_instance.info.setting.rank.exist();
	}

	// return)
	return false;
}
