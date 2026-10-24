#include "pch.h"

//-----------------------------------------------------------------------------
// service
//-----------------------------------------------------------------------------
own_ptr<service> g_pservice;
std::atomic<bool> g_is_service_closing{ false };

object_ptr<admin::host_instance_manager>			service::phost_instance_manager;
object_ptr<admin::service_instance_manager>			service::pservice_instance_manager;
own_ptr<net::acceptor<admin::socket_host_agent>>	service::pacceptor_host_agent;
own_ptr<net::acceptor<admin::socket_control>>		service::pacceptor_control;
server_system::access_policy_holder					service::access_policy_host_agent;
server_system::access_policy_holder					service::access_policy_control;
admin::auth_fail_limiter							service::control_auth_limiter;
admin::signature_verifier							service::control_signature;
admin::event_command								service::event_hook;
server_system::audit_log							service::audit;
server_system::sHTTP_ACCEPTOR_LIMITS				service::limits_control{ 10, 60, 64 };
std::atomic<bool>									service::control_require_signature{ false };
std::atomic<bool>									service::control_metrics_require_signature{ false };

//-----------------------------------------------------------------------------
// admin::event_command
//-----------------------------------------------------------------------------
void admin::event_command::set_command(std::string _command)
{
	std::lock_guard lock(this->m_lockable);
	this->m_command = std::move(_command);
}

std::string admin::event_command::get_command() const
{
	std::lock_guard lock(this->m_lockable);
	return this->m_command;
}

void admin::event_command::fire(std::string_view _event, server_system::iid_t _iid, std::string_view _name, std::string_view _message)
{
	// 1) command
	auto command = this->get_command();
	if (command.empty())
		return;

	// check) 동시에 실행하는 수
	if (++this->m_count_running > 4)
	{
		--this->m_count_running;
		LOG_WARNING << "(warn) event command is dropped (too many running) [event: "sv << _event << "]"sv;
		return;
	}

	// 2) arguments
	std::vector<std::string> arguments{ std::move(command), std::string(_event), std::to_string(_iid), std::string(_name), std::string(_message) };

	// 3) 실행 (기다리지 않는다.)
	try
	{
		std::thread([this, arguments = std::move(arguments)]() mutable
		{
			try { this->process_execute(std::move(arguments)); } catch (...) {}
			--this->m_count_running;
		}).detach();
	}
	catch (...)
	{
		--this->m_count_running;
	}
}

void admin::event_command::process_execute(std::vector<std::string> _arguments)
{
#if defined(_WIN32)
	// 1) command line (.ps1 -> powershell, .bat/.cmd -> cmd)
	auto& command = _arguments.front();
	auto extension = std::filesystem::path(command).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char _ch) { return static_cast<char>(std::tolower(_ch)); });

	std::string command_line;
	if (extension == ".ps1")
		command_line = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File ";
	else if (extension == ".bat" || extension == ".cmd")
		command_line = "cmd.exe /c ";

	for (std::size_t i = 0; i < _arguments.size(); ++i)
	{
		if (i != 0)
			command_line += ' ';
		command_line += server_system::sSERVICE::sSETTING::quote_argument(_arguments[i]);
	}

	// 2) process
	std::wstring command_line_w = server_system::utf8_to_utf16(command_line);
	STARTUPINFOW startup_info{};
	startup_info.cb = sizeof(startup_info);
	PROCESS_INFORMATION process_info{};

	if (::CreateProcessW(nullptr, command_line_w.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info) == FALSE)
	{
		LOG_WARNING << "(warn) fail to run event command '"sv << command << "' (error: "sv << ::GetLastError() << ")"sv;
		return;
	}

	// 3) wait (최대 60초)
	::WaitForSingleObject(process_info.hProcess, 60'000);
	::CloseHandle(process_info.hThread);
	::CloseHandle(process_info.hProcess);
#else
	// 1) argv
	std::vector<char*> argv;
	for (auto& iter : _arguments)
		argv.push_back(iter.data());
	argv.push_back(nullptr);

	// 2) spawn (admin server의 socket 등은 넘기지 않는다.)
	posix_spawn_file_actions_t file_actions;
	posix_spawn_file_actions_init(&file_actions);
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 34))
	posix_spawn_file_actions_addclosefrom_np(&file_actions, 3);
#endif

	pid_t pid = 0;
	auto result = ::posix_spawnp(&pid, argv[0], &file_actions, nullptr, argv.data(), environ);
	posix_spawn_file_actions_destroy(&file_actions);

	if (result != 0)
	{
		LOG_WARNING << "(warn) fail to run event command '"sv << _arguments.front() << "' (error: "sv << result << ")"sv;
		return;
	}

	// 3) wait
	int status = 0;
	while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
#endif
}

//-----------------------------------------------------------------------------
// service
//-----------------------------------------------------------------------------
service::service() :
	server_system::Xservice("admin_server"sv)
{
	// 1) host instance manager
	auto phost_manager = make_own<admin::host_instance_manager>();
	phost_manager->name("host_instance_manager"sv);
	phost_instance_manager = phost_manager.get_object_ptr();
	this->set_host_instance_manager(std::move(phost_manager));

	// 2) service instance manager
	auto pservice_manager = make_own<admin::service_instance_manager>();
	pservice_manager->name("service_instance_manager"sv);
	pservice_instance_manager = pservice_manager.get_object_ptr();
	this->set_service_instance_manager(std::move(pservice_manager));

	// 3) acceptor (host_agent)
	pacceptor_host_agent = make_own<net::acceptor<admin::socket_host_agent>>();
	pacceptor_host_agent->name("acceptor_host_agent"sv);
	this->attach_child_object(pacceptor_host_agent);

	// 4) acceptor (control api)
	pacceptor_control = make_own<net::acceptor<admin::socket_control>>();
	pacceptor_control->name("acceptor_control"sv);
	this->attach_child_object(pacceptor_control);
}

void service::apply_access_policy(context& _context_setting)
{
	// 1) host agent ("acceptor_host_agent": certify_key, allowed_addresses)
	access_policy_host_agent.load(_context_setting["acceptor_host_agent"sv]);

	// 2) control api ("acceptor_control": certify_key, allowed_addresses. 없으면 loopback만 허용)
	access_policy_control.load(_context_setting["acceptor_control"sv]);

	// log)
	auto ppolicy_host_agent = access_policy_host_agent.get();
	auto ppolicy_control = access_policy_control.get();
	LOG_INFO << " - host agent access policy (certify_key: "sv << (ppolicy_host_agent->certify_key.empty() ? "none"sv : "set"sv) << ", allowed_addresses: "sv << ppolicy_host_agent->allowed_addresses.size() << ")"sv;
	LOG_INFO << " - control access policy (certify_key: "sv << (ppolicy_control->certify_key.empty() ? "none"sv : "set"sv) << ", allowed_addresses: "sv << ppolicy_control->allowed_addresses.size() << (ppolicy_control->is_secured() ? ")"sv : ", loopback only)"sv);

	if (ppolicy_host_agent->is_secured() == false)
		LOG_WARNING << "(warn) 'acceptor_host_agent' has no 'certify_key' and no 'allowed_addresses' (any host agent can connect)"sv;

	// 3) control api key 실패 제한 ("acceptor_control": "auth_fail_max"(기본 5, 0: 제한 없음), "auth_fail_window"(초, 기본 60), "auth_block_time"(초, 기본 300))
	{
		auto& context_control = _context_setting["acceptor_control"sv];
		auto f_get = [&](std::string_view _name, int64_t _default) -> int64_t
		{
			auto& context_value = context_control.find(_name);
			return context_value.is_valid() ? static_cast<int64_t>(context_value) : _default;
		};
		auto max_fail = static_cast<int>(f_get("auth_fail_max"sv, 5));
		auto window = std::chrono::seconds(f_get("auth_fail_window"sv, 60));
		auto block_time = std::chrono::seconds(f_get("auth_block_time"sv, 300));
		control_auth_limiter.set_policy(max_fail, window, block_time);

		LOG_INFO << " - control auth fail limit (max "sv << max_fail << " in "sv << window.count() << "s, block "sv << block_time.count() << "s)"sv;

		// - 서명 ("require_signature": true면 key 헤더는 받지 않는다. "signature_skew": 허용 시각 차이(초, 기본 60))
		auto& context_require = context_control.find("require_signature"sv);
		control_require_signature = context_require.is_valid() && static_cast<bool>(context_require);
		auto& context_metrics_require = context_control.find("metrics_require_signature"sv);
		control_metrics_require_signature = context_metrics_require.is_valid() && static_cast<bool>(context_metrics_require);
		control_signature.set_allowed_skew(std::chrono::seconds(f_get("signature_skew"sv, 60)));

		LOG_INFO << " - control signature (require: "sv << (control_require_signature ? "true"sv : "false"sv) << ")"sv;
	}

	// 6) control api 시간/연결 제한 ("acceptor_control": "request_timeout", "idle_timeout", "max_connections")
	{
		limits_control.load(_context_setting["acceptor_control"sv]);
		if (pacceptor_control.exist())
			pacceptor_control->set_max_socket_count(limits_control.max_connections == 0 ? SIZE_MAX : static_cast<std::size_t>(limits_control.max_connections.load()));

		LOG_INFO << " - control limits (request_timeout "sv << limits_control.request_timeout.load() << "s, idle_timeout "sv << limits_control.idle_timeout.load() << "s, max_connections "sv << limits_control.max_connections.load() << ")"sv;
	}

	// 5) audit log ("audit_log": 파일. 기본 "log/admin_audit.jsonl". ""이면 남기지 않는다.)
	{
		auto& context_audit = _context_setting.find("audit_log"sv);
		audit.set_file(context_audit.is_valid() ? context_audit.operator std::string() : std::string("log/admin_audit.jsonl"));
		auto file_audit = audit.get_file();
		LOG_INFO << " - audit log: "sv << (file_audit.empty() ? std::string("none") : file_audit);
	}

	// 4) event command ("event_command": 사건이 생기면 실행할 명령. admin::event_command 참고)
	{
		auto& context_command = _context_setting.find("event_command"sv);
		event_hook.set_command(context_command.is_valid() ? context_command.operator std::string() : std::string());

		auto command = event_hook.get_command();
		if (command.empty() == false)
			LOG_INFO << " - event command: '"sv << command << "'"sv;
	}
}

bool service::reload_setting()
{
	// 1) read
	context context_setting;
	try
	{
		context_setting.from_json(api::file::read_file(CONFIGURE_FILE));
	}
	catch (...)
	{
		LOG_WARNING << "(warn) reload: fail to read setting file '"sv << CONFIGURE_FILE << "'"sv;
		return false;
	}

	// 2) access policy
	service::apply_access_policy(context_setting);

	// 3) service instance manager (restart policy, auto_start, desired state ...)
	if (pservice_instance_manager.exist())
		pservice_instance_manager->apply_setting(context_setting["service_instance_manager"sv]);

	// log)
	LOG_INFO << "(!) setting reloaded from '"sv << CONFIGURE_FILE << "'"sv;

	// return)
	return true;
}

service::sCONTROL_RESULT service::process_control(const admin::sCONTROL_ROUTE& _route)
{
	// declare)
	sCONTROL_RESULT result;
	auto pmanager = service::pservice_instance_manager;

	auto f_error = [&](eHTTP_STATUS_CODE _status_code, std::string_view _message)
	{
		result.status_code = _status_code;
		result.body = "{\"error\":" + admin::to_json_string(_message) + "}";
		return result;
	};

	// check)
	if (pmanager.empty())
		return f_error(eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_, "service instance manager is not ready"sv);

	switch (_route.action)
	{
	case	admin::eCONTROL_ACTION::LIST:
			result.body = pmanager->to_json_instances();
			break;

	case	admin::eCONTROL_ACTION::HOSTS:
			result.body = phost_instance_manager.exist() ? phost_instance_manager->to_json_hosts() : std::string("[]");
			break;

	case	admin::eCONTROL_ACTION::GET:
			{
				auto json = pmanager->to_json_instance(_route.iid);
				if (json.has_value() == false)
					return f_error(eHTTP_STATUS_CODE::NOT_FOUND_, "instance not found"sv);
				result.body = std::move(*json);
			}
			break;

	case	admin::eCONTROL_ACTION::REQUEST:
			{
				// check)
				if (pmanager->has_instance(_route.iid) == false)
					return f_error(eHTTP_STATUS_CODE::NOT_FOUND_, "instance not found"sv);

				// - same as console command (desired state / restart limit)
				auto request = admin::to_request_message(_route.argument);
				auto request_id = this->process_request_command(_route.iid, request);

				if (request_id == 0)
					return f_error(eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_, "fail to send request (not connected)"sv);

				result.status_code = eHTTP_STATUS_CODE::ACCEPTED_;
				result.body = "{\"iid\":" + std::to_string(_route.iid) + ",\"request\":" + admin::to_json_string(admin::to_request_name(request)) + ",\"request_id\":" + std::to_string(request_id) + "}";
			}
			break;

	case	admin::eCONTROL_ACTION::DESIRED:
			{
				pmanager->reset_restart_limit(_route.iid);
				pmanager->set_desired_state(_route.iid, admin::to_desired_state(_route.argument));
				result.body = "{\"iid\":" + std::to_string(_route.iid) + ",\"desired\":" + admin::to_json_string(admin::to_desired_string(pmanager->get_desired_state(_route.iid))) + "}";
			}
			break;

	case	admin::eCONTROL_ACTION::RELOAD:
			if (this->reload_setting() == false)
				return f_error(eHTTP_STATUS_CODE::INTERNAL_SERVER_ERROR_, "fail to read setting file"sv);
			result.body = "{\"reloaded\":true}";
			break;

	case	admin::eCONTROL_ACTION::METRICS:
			result.body = pmanager->to_metrics();
			if (phost_instance_manager.exist())
				result.body += phost_instance_manager->to_metrics();
			break;

	case	admin::eCONTROL_ACTION::METHOD_NOT_ALLOWED:
			return f_error(eHTTP_STATUS_CODE::METHOD_NOT_ALLOWED_, "method not allowed"sv);

	default:
			return f_error(eHTTP_STATUS_CODE::NOT_FOUND_, "not found"sv);
	}

	// return)
	return result;
}

uint64_t service::process_request_command(server_system::iid_t _iid, uint32_t _request)
{
	// declare)
	auto pmanager = service::pservice_instance_manager;

	// check)
	if (pmanager.empty() || _request == 0)
		return 0;

	// 1) 원하는 상태와 반대되는 요청이면 원하는 상태를 바꾼다. (자동 복구와 충돌하지 않도록)
	auto desired = pmanager->get_desired_state(_iid);
	if (desired == admin::eDESIRED_STATE::RUNNING && (_request == eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE || _request == eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE || _request == eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE))
		pmanager->set_desired_state(_iid, admin::eDESIRED_STATE::STOPPED);
	else if (desired == admin::eDESIRED_STATE::STOPPED && (_request == eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE || _request == eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE))
		pmanager->set_desired_state(_iid, admin::eDESIRED_STATE::RUNNING);

	// 2) 운영자의 start/init은 crash loop 중지를 푼다.
	if (_request == eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE || _request == eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE)
		pmanager->reset_restart_limit(_iid);

	// 3) send
	return pmanager->request(_iid, _request);
}

bool service::execute_command(std::string_view _command_line)
{
	// 1) parse
	auto command = admin::parse_command(_command_line);

	// check)
	if (command.command.empty())
		return true;

	auto pmanager = service::pservice_instance_manager;

	// audit) 조회(help/list/status)가 아닌 명령
	if (command.command != "help" && command.command != "list" && command.command != "ls" && command.command != "status")
		service::audit.write("console"sv, "local"sv, _command_line, 0);

	// 2) commands
	if (command.command == "quit" || command.command == "exit")
	{
		return false;
	}
	else if (command.command == "help" || command.command == "?")
	{
		LOG_INFO << "commands:"sv << log_endl
			<< "  list                                       - service instances"sv << log_endl
			<< "  start|stop|init|destroy|kill|pause|continue <iid>"sv << log_endl
			<< "  want <iid> running|stopped|none            - desired state (saved)"sv << log_endl
			<< "  status <iid>                               - status of service instance"sv << log_endl
			<< "  reload                                     - reload setting file"sv << log_endl
			<< "  quit"sv;
	}
	else if (command.command == "list" || command.command == "ls")
	{
		if (pmanager.exist())
			pmanager->print_instances();
	}
	else if (command.command == "status")
	{
		// check)
		if (command.args.empty() || pmanager.empty() || admin::parse_iid(command.args[0]) == 0)
		{
			LOG_WARNING << "usage: status <iid>"sv;
			return true;
		}

		pmanager->print_instance(admin::parse_iid(command.args[0]));
	}
	else if (command.command == "reload")
	{
		this->reload_setting();
	}
	else if (command.command == "want")
	{
		// check)
		if (command.args.size() < 2 || pmanager.empty() || admin::parse_iid(command.args[0]) == 0)
		{
			LOG_WARNING << "usage: want <iid> running|stopped|none"sv;
			return true;
		}

		auto iid = admin::parse_iid(command.args[0]);

		// - 운영자가 원하는 상태를 정하면 crash loop 중지도 푼다.
		pmanager->reset_restart_limit(iid);
		pmanager->set_desired_state(iid, admin::to_desired_state(command.args[1]));
	}
	else
	{
		// - request commands (start|stop|init|destroy|kill|pause|continue)
		auto request = admin::to_request_message(command.command);

		// check)
		if (request == 0)
		{
			LOG_WARNING << "unknown command '"sv << command.command << "' (type 'help')"sv;
			return true;
		}

		// check)
		if (command.args.empty() || pmanager.empty() || admin::parse_iid(command.args[0]) == 0)
		{
			LOG_WARNING << "usage: "sv << command.command << " <iid>"sv;
			return true;
		}

		auto iid = admin::parse_iid(command.args[0]);

		// - send (desired state / restart limit은 process_request_command에서 맞춘다.)
		if (this->process_request_command(iid, request) == 0)
			LOG_WARNING << "fail to request "sv << admin::to_request_name(request) << " [iid:"sv << iid << "] (not registered or not connected)"sv;
	}

	// return)
	return true;
}


//-----------------------------------------------------------------------------
// admin::socket_host_agent
//-----------------------------------------------------------------------------
void admin::socket_host_agent::on_connect()
{
	// check) 허용된 주소가 아니면 끊는다.
	if (service::access_policy_host_agent.get()->is_address_allowed(this->remote_endpoint().address()) == false)
	{
		LOG_WARNING << "(warn) host agent connection rejected (not allowed address: "sv << this->remote_endpoint().address().to_string() << ")"sv;
		this->m_is_rejected = true;
		this->disconnect();
		return;
	}
	this->m_is_rejected = false;

	// 1) base
	server_system::host::socket::on_connect();
}

bool admin::socket_host_agent::process_certify(const server_system::sCERTIFY_INFO& /*_certify_info*/, std::string_view _certify_key)
{
	// check) 이미 거부된 접속 (주소)
	if (this->m_is_rejected)
		return false;

	// check) certify key
	if (service::access_policy_host_agent.get()->is_key_matched(_certify_key) == false)
	{
		LOG_WARNING << "(warn) host agent certify rejected (certify key mismatch) [ip "sv << this->remote_endpoint().address().to_string() << "]"sv;
		return false;
	}

	// return)
	return true;
}


//-----------------------------------------------------------------------------
// admin::host_instance_manager
//-----------------------------------------------------------------------------
std::size_t admin::host_instance_manager::get_count() const
{
	std::lock_guard lock(this->m_lockable_hosts);
	return this->m_map_hosts.size();
}

std::vector<object_ptr<server_system::host::Ninstance>> admin::host_instance_manager::get_instances() const
{
	std::vector<object_ptr<server_system::host::Ninstance>> vector_hosts;

	std::lock_guard lock(this->m_lockable_hosts);
	for (auto& iter : this->m_map_hosts)
		vector_hosts.push_back(iter.second);
	return vector_hosts;
}

std::string admin::host_instance_manager::to_json_hosts() const
{
	std::string json = "[";
	bool is_first = true;
	for (const auto& iter : this->get_instances())
	{
		if (!is_first)
			json += ",";
		is_first = false;

		json += "{\"iid\":" + std::to_string(iter->get_iid());
		json += ",\"name\":" + to_json_string(iter->get_name());

		if (auto status = iter->get_system_status(); status.has_value())
		{
			json += ",\"status\":{";
			json += "\"cpu_usage\":" + std::to_string(static_cast<double>(status->processor_usage) / 1000.0);
			json += ",\"memory_used_bytes\":" + std::to_string(status->bytes_memory_usage);
			json += ",\"threads\":" + std::to_string(status->thread_count);
			json += ",\"processes\":" + std::to_string(status->process_count);
			json += ",\"handles\":" + std::to_string(status->handle_count);
			json += ",\"network_bytes\":" + std::to_string(status->bytes_traffic);
			json += "}";
		}
		if (auto web = iter->get_web_status(); web.has_value())
		{
			json += ",\"web\":{";
			json += "\"web_connections\":" + std::to_string(web->web_connections);
			json += ",\"web_timeout_request\":" + std::to_string(web->web_timeout_request);
			json += ",\"web_timeout_idle\":" + std::to_string(web->web_timeout_idle);
			json += ",\"websocket_connections\":" + std::to_string(web->websocket_connections);
			json += ",\"websocket_timeout_request\":" + std::to_string(web->websocket_timeout_request);
			json += ",\"websocket_timeout_idle\":" + std::to_string(web->websocket_timeout_idle);
			json += "}";
		}
		json += "}";
	}
	json += "]";
	return json;
}

std::string admin::host_instance_manager::to_metrics() const
{
	//-----------------------------------------------------------------
	// host system status (prometheus text format. HOST_UPDATE_STATUS)
	//-----------------------------------------------------------------
	struct sMETRIC
	{
		std::string_view	name;
		std::string_view	type;
		std::string_view	help;
		std::string			lines;
	};
	sMETRIC metrics[] =
	{
		{ "cgdk_host_cpu_usage_percent"sv,			"gauge"sv,		"cpu usage of the host (%)"sv, {} },
		{ "cgdk_host_memory_used_bytes"sv,			"gauge"sv,		"used physical memory of the host"sv, {} },
		{ "cgdk_host_threads"sv,					"gauge"sv,		"threads of the host"sv, {} },
		{ "cgdk_host_processes"sv,					"gauge"sv,		"processes of the host"sv, {} },
		{ "cgdk_host_handles"sv,					"gauge"sv,		"open handles(files) of the host"sv, {} },
		{ "cgdk_host_network_bytes_total"sv,		"counter"sv,	"received + sent bytes of the host (physical interfaces. except loopback)"sv, {} },
		{ "cgdk_host_web_connections"sv,			"gauge"sv,		"connections of the web ui of the host agent (acceptor: web / websocket)"sv, {} },
		{ "cgdk_host_web_timeouts_total"sv,			"counter"sv,	"connections of the web ui closed by the time limits (reason: request / idle)"sv, {} },
	};

	for (const auto& iter : this->get_instances())
	{
		auto status = iter->get_system_status();
		if (status.has_value() == false)
			continue;

		auto labels = "{host=\"" + std::to_string(iter->get_iid()) + "\",name=\"" + to_prometheus_label(iter->get_name()) + "\"} ";
		char str_cpu[32];
		std::snprintf(str_cpu, sizeof(str_cpu), "%.3f", static_cast<double>(status->processor_usage) / 1000.0);

		metrics[0].lines += std::string(metrics[0].name) + labels + str_cpu + "\n";
		metrics[1].lines += std::string(metrics[1].name) + labels + std::to_string(status->bytes_memory_usage) + "\n";
		metrics[2].lines += std::string(metrics[2].name) + labels + std::to_string(status->thread_count) + "\n";
		metrics[3].lines += std::string(metrics[3].name) + labels + std::to_string(status->process_count) + "\n";
		metrics[4].lines += std::string(metrics[4].name) + labels + std::to_string(status->handle_count) + "\n";
		metrics[5].lines += std::string(metrics[5].name) + labels + std::to_string(status->bytes_traffic) + "\n";

		// - web ui (host_agent가 보낸 경우만)
		if (auto web = iter->get_web_status(); web.has_value())
		{
			auto label_base = "{host=\"" + std::to_string(iter->get_iid()) + "\",name=\"" + to_prometheus_label(iter->get_name()) + "\",acceptor=\"";
			metrics[6].lines += std::string(metrics[6].name) + label_base + "web\"} " + std::to_string(web->web_connections) + "\n";
			metrics[6].lines += std::string(metrics[6].name) + label_base + "websocket\"} " + std::to_string(web->websocket_connections) + "\n";
			metrics[7].lines += std::string(metrics[7].name) + label_base + "web\",reason=\"request\"} " + std::to_string(web->web_timeout_request) + "\n";
			metrics[7].lines += std::string(metrics[7].name) + label_base + "web\",reason=\"idle\"} " + std::to_string(web->web_timeout_idle) + "\n";
			metrics[7].lines += std::string(metrics[7].name) + label_base + "websocket\",reason=\"request\"} " + std::to_string(web->websocket_timeout_request) + "\n";
			metrics[7].lines += std::string(metrics[7].name) + label_base + "websocket\",reason=\"idle\"} " + std::to_string(web->websocket_timeout_idle) + "\n";
		}
	}

	std::string text;
	for (const auto& iter : metrics)
	{
		text += "# HELP " + std::string(iter.name) + " " + std::string(iter.help) + "\n";
		text += "# TYPE " + std::string(iter.name) + " " + std::string(iter.type) + "\n";
		text += iter.lines;
	}
	return text;
}

void admin::host_instance_manager::on_register_instance(const object_ptr<server_system::host::Ninstance>& _phost_instance)
{
	{
		std::lock_guard lock(this->m_lockable_hosts);
		this->m_map_hosts[_phost_instance->get_iid()] = _phost_instance;
	}

	// log)
	LOG_PROGRESS << "> HOST['"sv << _phost_instance->info.setting.name << "'] registered (iid:"sv << _phost_instance->get_iid() << ")"sv;

	// event)
	service::event_hook.fire("host_connected"sv, _phost_instance->get_iid(), _phost_instance->get_name(), "registered"sv);
}

void admin::host_instance_manager::on_unregister_instance(const object_ptr<server_system::host::Ninstance>& _phost_instance) noexcept
{
	{
		std::lock_guard lock(this->m_lockable_hosts);
		this->m_map_hosts.erase(_phost_instance->get_iid());
	}

	// log)
	LOG_PROGRESS << "< HOST['"sv << _phost_instance->info.setting.name << "'] unregistered (iid:"sv << _phost_instance->get_iid() << ")"sv;

	// event) (admin server가 끝날 때는 보내지 않는다.)
	if (g_is_service_closing == false)
	{
		try { service::event_hook.fire("host_disconnected"sv, _phost_instance->get_iid(), _phost_instance->get_name(), "unregistered"sv); } catch (...) {}
	}
}

void admin::host_instance_manager::on_update_instance_state(const object_ptr<server_system::host::Ninstance>& _phost_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	// log)
	LOG_INFO << "  HOST['"sv << _phost_instance->info.setting.name << "'] state ["sv << to_string(_state_old) << "] -> ["sv << to_string(_state_new) << "]"sv;
}


//-----------------------------------------------------------------------------
// admin::service_instance_manager
//-----------------------------------------------------------------------------
void admin::service_instance_manager::on_starting(context& _context)
{
	this->apply_setting(_context);
}

void admin::service_instance_manager::apply_setting(context& _context)
{
	// 1) auto_start (초기화가 끝난 service를 start한다.)
	if (auto& context_value = _context.find("auto_start"sv); context_value.is_valid())
		this->m_is_auto_start = static_cast<bool>(context_value);

	// 2) desired state file
	if (auto& context_value = _context.find("desired_file"sv); context_value.is_valid())
		this->m_desired_file = context_value.operator std::string();

	// 3) restart policy
	auto restart_interval = this->m_restart.get_interval();
	auto restart_max_count = this->m_restart.get_max_count();
	auto restart_window = this->m_restart.get_window();

	if (auto& context_value = _context.find("restart_interval"sv); context_value.is_valid())
		restart_interval = std::chrono::seconds(static_cast<int64_t>(context_value));
	if (auto& context_value = _context.find("restart_max_count"sv); context_value.is_valid())
		restart_max_count = static_cast<int>(context_value);
	if (auto& context_value = _context.find("restart_window"sv); context_value.is_valid())
		restart_window = std::chrono::seconds(static_cast<int64_t>(context_value));

	this->m_restart.set_policy(restart_interval, restart_max_count, restart_window);

	// 4) request timeout
	if (auto& context_value = _context.find("request_timeout"sv); context_value.is_valid())
		this->m_request_timeout = std::chrono::seconds(std::max<int64_t>(1, static_cast<int64_t>(context_value)));

	// 5) load desired state
	this->m_desired.load_file(this->m_desired_file);

	// log)
	LOG_INFO << " - service instance manager (auto_start: "sv << (this->m_is_auto_start ? "true"sv : "false"sv)
		<< ", desired: "sv << this->m_desired.size() << " in '"sv << this->m_desired_file << "'"sv
		<< ", restart: interval "sv << this->m_restart.get_interval().count() << "s, max "sv << this->m_restart.get_max_count() << " in "sv << this->m_restart.get_window().count() << "s)"sv;
}

uint64_t admin::service_instance_manager::request(server_system::iid_t _iid, uint32_t _request)
{
	// 1) find instance
	auto pservice_instance = this->find_instance(_iid);

	// check)
	RETURN_IF(pservice_instance.empty(), 0);

	// 2) send request
	return this->process_send_request(pservice_instance, _request);
}

uint64_t admin::service_instance_manager::process_send_request(const object_ptr<server_system::service::Ninstance>& _pservice_instance, uint32_t _request)
{
	// declare)
	auto iid = _pservice_instance->get_iid();
	shared_buffer buf_request;

	// 1) make message
	switch (_request)
	{
	case	eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE:		buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(iid); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE:	buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(iid); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE:		buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_KILL_SERVICE(iid); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE:		buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_START_SERVICE(iid, _pservice_instance->info.setting.context_start); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE:		buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(iid); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE:		buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(iid); break;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE:	buf_request = server_system::make_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(iid); break;
	default:													return 0;
	}

	// 2) 오래된 요청을 정리한다. (결과가 오지 않은 요청)
	this->process_purge_requests();

	// 3) request id (REQUEST_RESULT의 request id로 결과를 맞춘다.)
	auto request_id = this->m_requests.add(iid, _request);
	buf_request = server_system::append_request_id(buf_request, request_id);

	// 4) send (host_agent를 거쳐 service에 전달된다.)
	auto result = _pservice_instance->send(std::move(buf_request));

	// check) 보내지 못했으면 결과도 오지 않는다.
	if (result == false)
		(void)this->m_requests.complete(request_id);

	// log)
	LOG_INFO << "  SERVICE['"sv << _pservice_instance->info.setting.name << "'] request #"sv << request_id << " "sv << admin::to_request_name(_request) << (result ? " sent"sv : " failed (not connected)"sv);

	// return)
	return result ? request_id : 0;
}

void admin::service_instance_manager::process_purge_requests()
{
	for (const auto& iter : this->m_requests.purge(this->m_request_timeout))
	{
		LOG_WARNING << "  (warn) SERVICE[iid:"sv << iter.iid << "] request #"sv << iter.request_id << " "sv << admin::to_request_name(iter.request) << " - no result in "sv << this->m_request_timeout.count() << "s"sv;
	}
}

void admin::service_instance_manager::set_desired_state(server_system::iid_t _iid, eDESIRED_STATE _state)
{
	// 1) set & save
	this->m_desired.set(_iid, _state);

	if (this->m_desired.save_file(this->m_desired_file) == false)
		LOG_WARNING << "(warn) fail to save desired state file '"sv << this->m_desired_file << "'"sv;

	// log)
	LOG_INFO << "  SERVICE[iid:"sv << _iid << "] desired state -> "sv << to_desired_string(_state);

	// 2) apply now
	auto pservice_instance = this->find_instance(_iid);

	if (pservice_instance.exist())
		this->process_apply_desired_state(pservice_instance, pservice_instance->info.status.state, eOBJECT_STATE::DEFAULT);
}

void admin::service_instance_manager::print_instances() const
{
	// declare)
	std::vector<object_ptr<server_system::service::Ninstance>> vector_instances;

	SCOPED_LOCK(this->m_lockable_instances)
	{
		for (auto& iter : this->m_map_instances)
			vector_instances.push_back(iter.second);
	}

	// log)
	LOG_INFO << "service instances ("sv << vector_instances.size() << ")"sv;

	for (auto& iter : vector_instances)
	{
		LOG_INFO << "  [iid:"sv << iter->get_iid() << "] '"sv << iter->info.setting.name << "' state:"sv << to_string(iter->info.status.state) << " host:"sv << iter->get_host_instance_iid()
			<< " desired:"sv << to_desired_string(this->m_desired.get(iter->get_iid())) << (this->m_restart.is_suspended(iter->get_iid()) ? " (auto restart suspended)"sv : ""sv);
	}
}

void admin::service_instance_manager::process_apply_desired_state(const object_ptr<server_system::service::Ninstance>& _pservice_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	// declare)
	auto iid = _pservice_instance->get_iid();
	auto desired = this->m_desired.get(iid);

	// check) 실행하지 않기를 원하면 아무것도 하지 않는다.
	if (desired == eDESIRED_STATE::STOPPED)
		return;

	// 1) STOPPED (초기화 완료 또는 정지)
	if (_state_new == eOBJECT_STATE::STOPPED)
	{
		// - 막 초기화가 끝났거나(INITIALIZING/NONE -> STOPPED), 등록/운영자 지정(DEFAULT)일 때만 start한다.
		//   (종료 중인 service(RUNNING -> CLOSING -> STOPPED)에는 start를 보내지 않는다.)
		const bool is_initialized = (_state_old == eOBJECT_STATE::INITIALIZING_PENDING || _state_old == eOBJECT_STATE::NONE || _state_old == eOBJECT_STATE::DEFAULT);

		if (is_initialized && (desired == eDESIRED_STATE::RUNNING || this->m_is_auto_start))
			this->process_send_request(_pservice_instance, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);
	}
	// 2) NONE (process가 없다.) - 원하는 상태가 RUNNING이면 init(process 실행)을 요청한다. (restart_limiter)
	else if (_state_new == eOBJECT_STATE::NONE && desired == eDESIRED_STATE::RUNNING)
	{
		switch (this->m_restart.request(iid))
		{
		case	restart_limiter::eDECISION::ALLOWED:
				this->process_send_request(_pservice_instance, eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE);
				break;

		case	restart_limiter::eDECISION::CRASH_LOOP:
				LOG_ERROR << "(err ) SERVICE['"sv << _pservice_instance->info.setting.name << "'] restarted "sv << this->m_restart.get_max_count() << " times in "sv << this->m_restart.get_window().count()
					<< "s. auto restart is suspended (crash loop). use 'start/init "sv << iid << "' or 'want "sv << iid << " running' to resume"sv;
				service::event_hook.fire("crash_loop"sv, iid, _pservice_instance->info.setting.name, "auto restart is suspended"sv);
				break;

		default:
				break;
		}
	}
}

void admin::service_instance_manager::on_register_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance)
{
	// 1) register
	SCOPED_LOCK(this->m_lockable_instances)
	{
		this->m_map_instances[_pservice_instance->get_iid()] = _pservice_instance;
	}

	// log)
	LOG_PROGRESS << "> SERVICE['"sv << _pservice_instance->info.setting.name << "'] registered (iid:"sv << _pservice_instance->get_iid() << ", host:"sv << _pservice_instance->get_host_instance_iid() << ", state:"sv << to_string(_pservice_instance->info.status.state) << ")"sv;

	// 2) 원하는 상태로 맞춘다. (admin server 재시작 후에도 유지된다.)
	//    - 등록 중(INSTANCE_CERTIFY_RESPONSE 전)에는 요청을 보내지 않고 post로 미룬다.
	if (this->m_desired.get(_pservice_instance->get_iid()) == eDESIRED_STATE::RUNNING)
	{
		object_ptr<service_instance_manager> pself(this);
		auto pservice_instance = _pservice_instance;

		system_executor::post(make_executable([=]
		{
			pself->process_apply_desired_state(pservice_instance, pservice_instance->info.status.state, eOBJECT_STATE::DEFAULT);
		}));
	}
}

void admin::service_instance_manager::on_unregister_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance) noexcept
{
	// 1) unregister
	SCOPED_LOCK(this->m_lockable_instances)
	{
		this->m_map_instances.erase(_pservice_instance->get_iid());
	}

	// log)
	LOG_PROGRESS << "< SERVICE['"sv << _pservice_instance->info.setting.name << "'] unregistered (iid:"sv << _pservice_instance->get_iid() << ")"sv;
}

void admin::service_instance_manager::on_update_instance_state(const object_ptr<server_system::service::Ninstance>& _pservice_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	// log)
	LOG_INFO << "  SERVICE['"sv << _pservice_instance->info.setting.name << "'] state ["sv << to_string(_state_old) << "] -> ["sv << to_string(_state_new) << "]"sv;

	// event) 실행 중이던 service의 process가 없어졌다. (RUNNING/PAUSED -> NONE)
	if (_state_new == eOBJECT_STATE::NONE && (_state_old == eOBJECT_STATE::RUNNING || _state_old == eOBJECT_STATE::PAUSED))
		service::event_hook.fire("service_down"sv, _pservice_instance->get_iid(), _pservice_instance->info.setting.name, std::string("state ") + std::string(to_string(_state_old)) + " -> " + std::string(to_string(_state_new)));

	// 1) auto start / 원하는 상태
	this->process_apply_desired_state(_pservice_instance, _state_new, _state_old);
}

void admin::service_instance_manager::on_request_result(const object_ptr<server_system::service::Ninstance>& _pservice_instance, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message)
{
	// 1) request id로 요청을 찾는다. (0: request id 없는 host_agent)
	std::string str_elapsed;
	if (_request_id != 0)
	{
		auto pending = this->m_requests.complete(_request_id);

		if (pending.has_value())
			str_elapsed = ", " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(request_tracker::clock_t::now() - pending->time_request).count()) + "ms";
		else
			str_elapsed = ", unknown request";
	}

	// log)
	if (_result == eRESULT::SUCCESS)
		LOG_INFO << "  SERVICE['"sv << _pservice_instance->info.setting.name << "'] request #"sv << _request_id << " "sv << admin::to_request_name(_request) << " result: SUCCESS ("sv << _message << str_elapsed << ")"sv;
	else
	{
		LOG_WARNING << "  SERVICE['"sv << _pservice_instance->info.setting.name << "'] request #"sv << _request_id << " "sv << admin::to_request_name(_request) << " result: "sv << to_string(_result) << " ("sv << _message << str_elapsed << ")"sv;

		// event)
		service::event_hook.fire("request_failed"sv, _pservice_instance->get_iid(), _pservice_instance->info.setting.name, std::string(admin::to_request_name(_request)) + " " + std::string(to_string(_result)) + " (" + std::string(_message) + ")");
	}
}

//-----------------------------------------------------------------------------
// admin::service_instance_manager - status / control api / metrics
//-----------------------------------------------------------------------------
std::vector<object_ptr<server_system::service::Ninstance>> admin::service_instance_manager::process_get_instances() const
{
	std::vector<object_ptr<server_system::service::Ninstance>> vector_instances;

	SCOPED_LOCK(this->m_lockable_instances)
	{
		for (auto& iter : this->m_map_instances)
			vector_instances.push_back(iter.second);
	}

	return vector_instances;
}

std::string admin::service_instance_manager::process_to_json_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance, bool _is_detail) const
{
	// 1) copy (lock)
	server_system::sSERVICE info;
	server_system::iid_t host_iid;
	SCOPED_LOCK(_pservice_instance->get_lockable())
	{
		info.setting.iid = _pservice_instance->info.setting.iid;
		info.setting.name = _pservice_instance->info.setting.name;
		info.setting.rank = _pservice_instance->info.setting.rank;
		info.info.pid = _pservice_instance->info.info.pid;
		info.status.state = _pservice_instance->info.status.state;
		if (_is_detail)
		{
			info.status.connective_info = _pservice_instance->info.status.connective_info;
			info.status.traffic_info = _pservice_instance->info.status.traffic_info;
			info.status.vector_factory = _pservice_instance->info.status.vector_factory;
			info.status.vector_executor = _pservice_instance->info.status.vector_executor;
			info.status.vector_binder = _pservice_instance->info.status.vector_binder;
			info.status.vector_connective = _pservice_instance->info.status.vector_connective;
		}
		host_iid = _pservice_instance->get_host_instance_iid();
	}

	auto iid = info.setting.iid;

	// 2) json
	std::string json = "{";
	json += "\"iid\":" + std::to_string(iid);
	json += ",\"name\":" + to_json_string(info.setting.name);
	json += ",\"rank\":" + to_json_string(info.setting.rank.to_string());
	json += ",\"host\":" + std::to_string(host_iid);
	json += ",\"pid\":" + std::to_string(info.info.pid);
	json += ",\"state\":" + to_json_string(to_string(info.status.state));
	json += ",\"desired\":" + to_json_string(to_desired_string(this->m_desired.get(iid)));
	json += ",\"restart_suspended\":" + std::string(this->m_restart.is_suspended(iid) ? "true" : "false");

	if (_is_detail)
	{
		const auto& connective = info.status.connective_info;
		const auto& traffic = info.status.traffic_info;

		json += ",\"status\":{";
		json += "\"connections\":" + std::to_string(connective.count_keep);
		json += ",\"connect_try\":" + std::to_string(connective.count_try);
		json += ",\"connect_success\":" + std::to_string(connective.count_success_connect);
		json += ",\"connect_fail\":" + std::to_string(connective.count_fail_connect);
		json += ",\"disconnect\":" + std::to_string(connective.count_disconnect);
		json += ",\"received_messages\":" + std::to_string(traffic.count_received_message);
		json += ",\"received_bytes\":" + std::to_string(traffic.count_received_bytes);
		json += ",\"sent_messages\":" + std::to_string(traffic.count_sent_message);
		json += ",\"sent_bytes\":" + std::to_string(traffic.count_sent_bytes);
		json += ",\"factories\":" + std::to_string(info.status.vector_factory.size());
		json += ",\"executors\":" + std::to_string(info.status.vector_executor.size());
		json += ",\"binders\":" + std::to_string(info.status.vector_binder.size());
		json += ",\"connectives\":" + std::to_string(info.status.vector_connective.size());
		json += "}";
	}

	json += "}";
	return json;
}

std::string admin::service_instance_manager::to_json_instances() const
{
	std::string json = "[";
	bool is_first = true;
	for (const auto& iter : this->process_get_instances())
	{
		if (!is_first)
			json += ",";
		json += this->process_to_json_instance(iter, false);
		is_first = false;
	}
	json += "]";
	return json;
}

std::optional<std::string> admin::service_instance_manager::to_json_instance(server_system::iid_t _iid) const
{
	auto pservice_instance = this->find_instance(_iid);

	// check)
	if (pservice_instance.empty())
		return std::nullopt;

	return this->process_to_json_instance(pservice_instance, true);
}

void admin::service_instance_manager::print_instance(server_system::iid_t _iid) const
{
	auto json = this->to_json_instance(_iid);

	if (json.has_value())
		LOG_INFO << "service instance: "sv << *json;
	else
		LOG_WARNING << "service instance not found [iid:"sv << _iid << "]"sv;
}

std::string admin::service_instance_manager::to_metrics() const
{
	//-----------------------------------------------------------------
	// prometheus text format (version 0.0.4)
	//-----------------------------------------------------------------
	std::string text;

	auto vector_instances = this->process_get_instances();

	// 1) admin
	auto phost_manager = service::phost_instance_manager;
	text += "# HELP cgdk_admin_hosts number of connected host agents\n# TYPE cgdk_admin_hosts gauge\n";
	text += "cgdk_admin_hosts " + std::to_string(phost_manager.exist() ? phost_manager->get_count() : 0) + "\n";
	text += "# HELP cgdk_admin_services number of registered service instances\n# TYPE cgdk_admin_services gauge\n";
	text += "cgdk_admin_services " + std::to_string(vector_instances.size()) + "\n";
	text += "# HELP cgdk_admin_pending_requests requests waiting for the result\n# TYPE cgdk_admin_pending_requests gauge\n";
	text += "cgdk_admin_pending_requests " + std::to_string(this->m_requests.size()) + "\n";

	// - control api 접속 (max_connections에 닿으면 끝날 때까지 받지 않는다.)
	{
		std::size_t count_connections = 0;
		std::size_t count_max = 0;
		if (service::pacceptor_control.exist())
		{
			auto& pacceptor = service::pacceptor_control;
			auto count_allocated = pacceptor->allocated_socket_count();
			auto count_prepared = pacceptor->prepared_socket_count();
			count_connections = (count_allocated > count_prepared) ? count_allocated - count_prepared : 0;
			count_max = pacceptor->max_socket_count();
		}
		text += "# HELP cgdk_admin_control_connections connections of the control api\n# TYPE cgdk_admin_control_connections gauge\n";
		text += "cgdk_admin_control_connections " + std::to_string(count_connections) + "\n";
		text += "# HELP cgdk_admin_control_max_connections max_connections of the control api (0: no limit)\n# TYPE cgdk_admin_control_max_connections gauge\n";
		text += "cgdk_admin_control_max_connections " + std::to_string(count_max == SIZE_MAX ? 0 : count_max) + "\n";
		text += "# HELP cgdk_admin_control_timeouts_total connections of the control api closed by the time limits\n# TYPE cgdk_admin_control_timeouts_total counter\n";
		text += "cgdk_admin_control_timeouts_total{reason=\"request\"} " + std::to_string(service::limits_control.count_timeout.certify.load()) + "\n";
		text += "cgdk_admin_control_timeouts_total{reason=\"idle\"} " + std::to_string(service::limits_control.count_timeout.idle.load()) + "\n";
	}

	// 2) services
	struct sMETRIC
	{
		std::string_view	name;
		std::string_view	type;
		std::string_view	help;
		std::string			lines;
	};
	sMETRIC metrics[] =
	{
		{ "cgdk_service_state"sv,						"gauge"sv,		"eOBJECT_STATE of the service (4: running)"sv, {} },
		{ "cgdk_service_running"sv,						"gauge"sv,		"1 if the service is running"sv, {} },
		{ "cgdk_service_desired_running"sv,				"gauge"sv,		"1 if the desired state is running"sv, {} },
		{ "cgdk_service_restart_suspended"sv,			"gauge"sv,		"1 if the auto restart is suspended (crash loop)"sv, {} },
		{ "cgdk_service_connections"sv,					"gauge"sv,		"connections of the service"sv, {} },
		{ "cgdk_service_received_messages_total"sv,		"counter"sv,	"received messages"sv, {} },
		{ "cgdk_service_received_bytes_total"sv,		"counter"sv,	"received bytes"sv, {} },
		{ "cgdk_service_sent_messages_total"sv,			"counter"sv,	"sent messages"sv, {} },
		{ "cgdk_service_sent_bytes_total"sv,			"counter"sv,	"sent bytes"sv, {} },
	};

	for (const auto& iter : vector_instances)
	{
		// - copy (lock)
		std::string name;
		eOBJECT_STATE state;
		statistics::unit_connective_snap connective;
		statistics::unit_traffic_snap traffic;
		server_system::iid_t host_iid;
		SCOPED_LOCK(iter->get_lockable())
		{
			name = iter->info.setting.name;
			state = iter->info.status.state;
			connective = iter->info.status.connective_info;
			traffic = iter->info.status.traffic_info;
			host_iid = iter->get_host_instance_iid();
		}

		auto iid = iter->get_iid();
		auto labels = "{iid=\"" + std::to_string(iid) + "\",name=\"" + to_prometheus_label(name) + "\",host=\"" + std::to_string(host_iid) + "\"} ";

		metrics[0].lines += std::string(metrics[0].name) + labels + std::to_string(static_cast<int>(state)) + "\n";
		metrics[1].lines += std::string(metrics[1].name) + labels + (state == eOBJECT_STATE::RUNNING ? "1" : "0") + "\n";
		metrics[2].lines += std::string(metrics[2].name) + labels + (this->m_desired.get(iid) == eDESIRED_STATE::RUNNING ? "1" : "0") + "\n";
		metrics[3].lines += std::string(metrics[3].name) + labels + (this->m_restart.is_suspended(iid) ? "1" : "0") + "\n";
		metrics[4].lines += std::string(metrics[4].name) + labels + std::to_string(connective.count_keep) + "\n";
		metrics[5].lines += std::string(metrics[5].name) + labels + std::to_string(traffic.count_received_message) + "\n";
		metrics[6].lines += std::string(metrics[6].name) + labels + std::to_string(traffic.count_received_bytes) + "\n";
		metrics[7].lines += std::string(metrics[7].name) + labels + std::to_string(traffic.count_sent_message) + "\n";
		metrics[8].lines += std::string(metrics[8].name) + labels + std::to_string(traffic.count_sent_bytes) + "\n";
	}

	for (const auto& iter : metrics)
	{
		text += "# HELP " + std::string(iter.name) + " " + std::string(iter.help) + "\n";
		text += "# TYPE " + std::string(iter.name) + " " + std::string(iter.type) + "\n";
		text += iter.lines;
	}

	// return)
	return text;
}
