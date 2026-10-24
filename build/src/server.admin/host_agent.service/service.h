//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
// ----------------------------------------------------------------------------
//
// service
//
// ----------------------------------------------------------------------------
class service : public server_system::service_application<service>
{
public:
			service();
	virtual	void on_initialize_setting(context& _context) override;

	static	own_ptr<server_system::instance_manager>		pmanager_instance;

	// - service(acceptor_server) 접속 허용 정책 ("acceptor_server": { "certify_key", "allowed_addresses" })
	static	server_system::access_policy_holder		access_policy_server;

	// - web ui(acceptor_web/acceptor_websocket) 접속 허용 정책 (없으면 loopback만 허용한다.)
	static	server_system::access_policy_holder		access_policy_web;
	static	server_system::access_policy_holder		access_policy_websocket;

	// - setting 파일의 접속 정책을 (다시) 적용한다.
	static	void										apply_access_policy(context& _context);

	// - web ui 접속 허용 여부 ('allowed_addresses'가 있으면 그 주소만, 없으면 loopback만 허용한다. web ui는 key를 확인하지 않는다.)
	static	bool										is_web_address_allowed(const server_system::access_policy_holder& _policy, const net::ip::address& _address)
	{
		auto ppolicy = _policy.get();
		return ppolicy->allowed_addresses.empty() ? server_system::sACCESS_POLICY::normalize(_address).is_loopback() : ppolicy->is_address_allowed(_address);
	}

	// - web ui Host/Origin 정책 (server_system::is_web_host_allowed / is_web_origin_allowed 참고)
	//   "acceptor_websocket": { "allowed_origins": [ "http://admin.example.com:22052" ] }
	struct sWEB_ORIGIN_POLICY
	{
		uint16_t					web_port{ 22052 };		// - "acceptor_web"의 port
		std::vector<std::string>	allowed_origins;

		// - login ("acceptor_web": { "login_key": "...", "login_session_time": 43200 }. 없으면 환경변수 CGDK_HOST_AGENT_WEB_KEY)
		//   login_key가 있으면 web ui/websocket은 login한 session(cookie)만 받는다.
		std::string					login_key;

		// - websocket 메시지 제한 ("acceptor_websocket": "max_messages_per_second"(기본 50), "max_messages_burst"(기본 100). 0: 제한 없음)
		double						max_messages_per_second{ 50.0 };
		double						max_messages_burst{ 100.0 };
		[[nodiscard]] bool			is_login_required() const noexcept { return this->login_key.empty() == false; }
	};
	static	std::shared_ptr<const sWEB_ORIGIN_POLICY>	get_web_origin_policy();
	static	void										set_web_origin_policy(sWEB_ORIGIN_POLICY&& _policy);

	// - web ui login session (cookie 'cgdk_session')
	static	server_system::web_session_store			web_sessions;
	static	constexpr std::string_view					WEB_SESSION_COOKIE = "cgdk_session";
	static	bool										is_web_session_valid(const sMESSAGE_HTTP_REQUEST& _request, std::string_view _address);

	// - audit log ("audit_log": 파일. 기본 "log/host_agent_audit.jsonl". ""이면 남기지 않는다.)
	static	server_system::audit_log					audit;

	// - web ui 시간/연결 제한 ("acceptor_web"/"acceptor_websocket": "request_timeout", "idle_timeout", "max_connections")
	//   (websocket은 page가 열려 있는 동안 받을 것이 없을 수 있으므로 idle_timeout 기본값이 0(끊지 않음)이다.)
	static	server_system::sHTTP_ACCEPTOR_LIMITS		limits_web;
	static	server_system::sHTTP_ACCEPTOR_LIMITS		limits_websocket;

	static	own_ptr<socket_admin_server>				psocket_to_admin_server;
	static	own_ptr<net::acceptor<socket_peer>>			pacceptor_peer;
	static	own_ptr<group_peer_immigration>				pgroup_peer_immigration;
	static	own_ptr<group_peer_server>					pgroup_peer_server;
	static	own_ptr<group_peer_client>					pgroup_peer_client;

	static	own_ptr<net::acceptor<socket_web>>			pacceptor_web;
	static	own_ptr<net::acceptor<socket_websocket>>	pacceptor_websocket;
	static	own_ptr<group_websocket_immigration>		pgroup_websocket_immigration;
	static	own_ptr<group_websocket>					pgroup_websocket;
};

//----------------------------------------------------------------------------
// json file
//  - reads a json file into the context. (if the file does not exist or is invalid, warns and keeps the context)
//----------------------------------------------------------------------------
inline bool load_json_file(CGDK::context& _context, std::string_view _filename) noexcept
{
	try
	{
		_context.from_json(CGDK::api::file::read_file(_filename));
		return true;
	}
	catch (...)
	{
		LOG_WARNING << "(warn) fail to read json file '"sv << _filename << "' (default values are used)"sv;
		return false;
	}
}
