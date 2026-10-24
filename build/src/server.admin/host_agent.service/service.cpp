#include "pch.h"

own_ptr<server_system::instance_manager>		service::pmanager_instance;
own_ptr<net::acceptor<socket_peer>>		service::pacceptor_peer;
own_ptr<group_peer_immigration>			service::pgroup_peer_immigration;
own_ptr<group_peer_server>						service::pgroup_peer_server;
own_ptr<group_peer_client>						service::pgroup_peer_client;
own_ptr<net::acceptor<socket_web>>			service::pacceptor_web;
own_ptr<net::acceptor<socket_websocket>>	service::pacceptor_websocket;
own_ptr<group_websocket_immigration>		service::pgroup_websocket_immigration;
own_ptr<group_websocket>					service::pgroup_websocket;
own_ptr<socket_admin_server>				service::psocket_to_admin_server;
server_system::access_policy_holder		service::access_policy_server;
server_system::access_policy_holder		service::access_policy_web;
server_system::access_policy_holder		service::access_policy_websocket;
server_system::web_session_store			service::web_sessions;
server_system::audit_log					service::audit;
server_system::sHTTP_ACCEPTOR_LIMITS		service::limits_web{ 10, 60, 64 };
server_system::sHTTP_ACCEPTOR_LIMITS		service::limits_websocket{ 10, 0, 64, 30 };

service::service()
{
	pmanager_instance = make_own<server_system::instance_manager>();
	pmanager_instance->name("server_entity_manager"sv);
	this->attach_child_object(pmanager_instance);

	pgroup_peer_immigration = make_own<group_peer_immigration>();
	pgroup_peer_immigration->name("group_peer_immigration"sv);
	this->attach_child_object(pgroup_peer_immigration);
	
	pgroup_peer_server = make_own<group_peer_server>();
	pgroup_peer_server->name("group_peer_server"sv);
	this->attach_child_object(pgroup_peer_server);
	
	pgroup_peer_client = make_own<group_peer_client>();
	pgroup_peer_client->name("group_peer_client"sv );
	this->attach_child_object(pgroup_peer_client);

	pacceptor_peer = make_own<net::acceptor<socket_peer>>();
	pacceptor_peer->name("acceptor_server"sv);
	this->attach_child_object(pacceptor_peer);

	pgroup_websocket_immigration = make_own<group_websocket_immigration>();
	pgroup_websocket_immigration->name("group_websocket_immigration"sv);
	this->attach_child_object(pgroup_websocket_immigration);

	pgroup_websocket = make_own<group_websocket>();
	pgroup_websocket->name("group_websocket"sv);
	this->attach_child_object(pgroup_websocket);

	pacceptor_web = make_own<net::acceptor<socket_web>>();
	pacceptor_web->name("acceptor_web"sv);
	this->attach_child_object(pacceptor_web);

	pacceptor_websocket = make_own<net::acceptor<socket_websocket>>();
	pacceptor_websocket->name("acceptor_websocket"sv);
	this->attach_child_object(pacceptor_websocket);

	psocket_to_admin_server = make_own<socket_admin_server>();
	psocket_to_admin_server->name("socket_to_admin_server"sv);
	this->attach_child_object(psocket_to_admin_server);
}

void service::on_initialize_setting(context& _context)
{
	// definition) 
#if defined(_WIN32)
	#define	SETTING_FILE_PATH	"./"sv
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#elif defined(__linux__)
	#define	SETTING_FILE_PATH	"./"sv						// - working directory (foreground 실행. systemd는 WorkingDirectory=로 지정한다.)
	#define	SETTING_FILE_NAME	"host_agent_setting.json"sv
#endif

	// 2) read setting json file
	load_json_file(_context, api::file::make_full_path(SETTING_FILE_PATH, SETTING_FILE_NAME));

	// 3) access policy
	apply_access_policy(_context);
}

void service::apply_access_policy(context& _context)
{
	// 1) service 접속 (acceptor_server)
	access_policy_server.load(_context["acceptor_server"sv]);

	auto ppolicy_server = access_policy_server.get();
	LOG_INFO << " - service access policy (certify_key: "sv << (ppolicy_server->certify_key.empty() ? "none"sv : "set"sv) << ", allowed_addresses: "sv << ppolicy_server->allowed_addresses.size() << ")"sv;

	if (ppolicy_server->is_secured() == false)
		LOG_WARNING << "(warn) 'acceptor_server' has no 'certify_key' and no 'allowed_addresses' (any service can connect)"sv;

	// 2) web ui 접속 (acceptor_web / acceptor_websocket. 설정이 없으면 loopback만 허용한다.)
	access_policy_web.load(_context["acceptor_web"sv]);
	access_policy_websocket.load(_context["acceptor_websocket"sv]);

	if (access_policy_web.get()->allowed_addresses.empty())
		LOG_INFO << " - web access policy: loopback only (set 'allowed_addresses' of 'acceptor_web' to allow remote access)"sv;
	if (access_policy_websocket.get()->allowed_addresses.empty())
		LOG_INFO << " - websocket access policy: loopback only (set 'allowed_addresses' of 'acceptor_websocket' to allow remote access)"sv;

	// 3) web ui Host/Origin (web ui port, allowed_origins)
	sWEB_ORIGIN_POLICY policy_origin;
	if (auto& context_web = _context.find("acceptor_web"sv); context_web.is_valid())
	{
		if (auto& context_port = context_web.find("local_endpoint"sv).find("port"sv); context_port.is_valid())
		{
			auto port = static_cast<int64_t>(context_port);
			if (port > 0 && port <= 65535)
				policy_origin.web_port = static_cast<uint16_t>(port);
		}
	}
	if (auto& context_websocket = _context.find("acceptor_websocket"sv); context_websocket.is_valid())
	{
		if (auto& context_origins = context_websocket.find("allowed_origins"sv); context_origins.is_valid())
		{
			for (auto& iter : context_origins.map_nodes)
				policy_origin.allowed_origins.push_back(iter.second.operator std::string());
		}
	}
	// 4) web ui login ("acceptor_web": "login_key", "login_session_time"(초, 기본 43200). 없으면 환경변수 CGDK_HOST_AGENT_WEB_KEY)
	{
		int64_t session_time = 12 * 3600;
		int64_t idle_time = 1800;
		if (auto& context_web = _context.find("acceptor_web"sv); context_web.is_valid())
		{
			// - "login_key_file" > "login_key"
			policy_origin.login_key = server_system::get_key_from_setting(context_web, "login_key"sv);
			if (auto& context_time = context_web.find("login_session_time"sv); context_time.is_valid())
				session_time = static_cast<int64_t>(context_time);
			if (auto& context_idle = context_web.find("login_idle_time"sv); context_idle.is_valid())
				idle_time = static_cast<int64_t>(context_idle);
		}
		if (policy_origin.login_key.empty())
		{
		#if defined(_WIN32)
			char buf_key[256]{ 0 };
			if (GetEnvironmentVariableA("CGDK_HOST_AGENT_WEB_KEY", buf_key, static_cast<DWORD>(std::size(buf_key))) > 0)
				policy_origin.login_key = buf_key;
		#else
			if (auto pkey = std::getenv("CGDK_HOST_AGENT_WEB_KEY"); pkey != nullptr)
				policy_origin.login_key = pkey;
		#endif
		}
		web_sessions.set_policy(std::chrono::seconds(session_time > 0 ? session_time : 12 * 3600), 5, std::chrono::seconds(300), std::chrono::seconds(idle_time >= 0 ? idle_time : 1800));

		if (policy_origin.is_login_required())
			LOG_INFO << " - web ui login: required (session "sv << session_time << "s, idle "sv << idle_time << "s, bound to the login address)"sv;
		else if (access_policy_web.get()->allowed_addresses.empty() == false || access_policy_websocket.get()->allowed_addresses.empty() == false)
			LOG_WARNING << "(warn) web ui is open to 'allowed_addresses' without login. set 'login_key' of 'acceptor_web' (anyone from those addresses can control the services)"sv;
	}

	// 5) websocket 메시지 제한
	if (auto& context_websocket = _context.find("acceptor_websocket"sv); context_websocket.is_valid())
	{
		if (auto& context_rate = context_websocket.find("max_messages_per_second"sv); context_rate.is_valid())
			policy_origin.max_messages_per_second = static_cast<double>(static_cast<int64_t>(context_rate));
		if (auto& context_burst = context_websocket.find("max_messages_burst"sv); context_burst.is_valid())
			policy_origin.max_messages_burst = static_cast<double>(static_cast<int64_t>(context_burst));
	}

	// 7) web ui 시간/연결 제한
	{
		limits_web.load(_context["acceptor_web"sv]);
		limits_websocket.load(_context["acceptor_websocket"sv]);
		if (pacceptor_web.exist())
			pacceptor_web->set_max_socket_count(limits_web.max_connections == 0 ? SIZE_MAX : static_cast<std::size_t>(limits_web.max_connections.load()));
		if (pacceptor_websocket.exist())
			pacceptor_websocket->set_max_socket_count(limits_websocket.max_connections == 0 ? SIZE_MAX : static_cast<std::size_t>(limits_websocket.max_connections.load()));
	}

	// 6) audit log
	{
		auto& context_audit = _context.find("audit_log"sv);
		audit.set_file(context_audit.is_valid() ? context_audit.operator std::string() : std::string("log/host_agent_audit.jsonl"));
		auto file_audit = audit.get_file();
		LOG_INFO << " - audit log: "sv << (file_audit.empty() ? std::string("none") : file_audit);
	}

	LOG_INFO << " - web ui origin policy (web port: "sv << policy_origin.web_port << ", allowed_origins: "sv << policy_origin.allowed_origins.size() << ")"sv;
	set_web_origin_policy(std::move(policy_origin));
}

bool service::is_web_session_valid(const sMESSAGE_HTTP_REQUEST& _request, std::string_view _address)
{
	// check) login이 필요 없다.
	if (get_web_origin_policy()->is_login_required() == false)
		return true;

	// - session은 login한 주소에서만 쓸 수 있다.
	return web_sessions.is_valid(server_system::get_cookie_value(_request.get_header("Cookie"sv), WEB_SESSION_COOKIE), _address);
}

namespace
{
	std::mutex												g_lockable_web_origin_policy;
	std::shared_ptr<const service::sWEB_ORIGIN_POLICY>		g_pweb_origin_policy{ std::make_shared<const service::sWEB_ORIGIN_POLICY>() };
}

std::shared_ptr<const service::sWEB_ORIGIN_POLICY> service::get_web_origin_policy()
{
	std::lock_guard lock(g_lockable_web_origin_policy);
	return g_pweb_origin_policy;
}

void service::set_web_origin_policy(sWEB_ORIGIN_POLICY&& _policy)
{
	auto ppolicy = std::make_shared<const sWEB_ORIGIN_POLICY>(std::move(_policy));
	std::lock_guard lock(g_lockable_web_origin_policy);
	g_pweb_origin_policy = std::move(ppolicy);
}
