//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin - Admin Server                     *
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
//-----------------------------------------------------------------------------
//
// admin server
//
//  - server.system의 host/service manager 위에 올린 얇은 admin server이다.
//
//      service ──> host_agent(22044) ──> admin_server(21887)
//
//  - host_agent가 접속하면 (host::socket)
//     1) CERTIFY_REQUEST/INSTANCE_CERTIFY_REQUEST로 host와 service instance들을 등록한다.
//     2) INSTANCE_UPDATE_STATE/INFO/STATUS로 instance의 상태를 갱신한다.
//     3) instance에 send()하면 host_agent를 거쳐 service로 요청(REQUEST_xxx)이 전달된다.
//        요청에는 request id를 붙이고, 처리 결과(REQUEST_RESULT)의 request id로 맞춘다.
//  - setting (CONFIGURE_FILE)
//     "acceptor_host_agent"      : host_agent 접속을 받는 acceptor
//                                  (local_endpoint, "certify_key", "allowed_addresses")
//     "service_instance_manager" : "auto_start"        - true면 초기화가 끝난(STOPPED) service를 start한다.
//                                  "desired_file"      - service별 원하는 상태(running/stopped)를 저장하는 파일
//                                  "restart_interval"  - 자동 재시작(INIT) 최소 간격(초, 기본 30)
//                                  "restart_max_count" - restart_window 안의 최대 재시작 횟수(기본 5, 0: 제한 없음)
//                                  "restart_window"    - 재시작 횟수를 세는 시간(초, 기본 300)
//                                  "request_timeout"   - 요청 결과를 기다리는 시간(초, 기본 60)
//     "acceptor_control"         : http control api (local_endpoint, "certify_key", "allowed_addresses")
//                                  (없으면 127.0.0.1:21888. admin_server.control.h 참고)
//  - console 명령 (help 참고)
//     list / status <iid> / start|stop|init|destroy|kill|pause|continue <iid> / want <iid> running|stopped|none / reload / quit
//
//-----------------------------------------------------------------------------
// definition)
#define	CONFIGURE_FILE			"config/admin_server.setting.json"sv
#define	DESIRED_STATE_FILE		"config/admin_server.desired_state.json"sv

namespace admin
{
	class event_command;
	class socket_host_agent;
	class host_instance_manager;
	class service_instance_manager;
}

//-----------------------------------------------------------------------------
// admin::event_command
//
//  - 사건이 생기면 외부 명령을 실행한다. ("event_command": "/opt/cgdk/notify.sh")
//      <command> <event> <iid> <name> <message>
//    event : host_connected / host_disconnected / service_down / crash_loop / request_failed
//  - 인자는 shell을 거치지 않고 그대로 넘긴다. (windows: .ps1은 powershell, .bat/.cmd는 cmd로 실행한다.)
//  - 기다리지 않는다. 동시에 4개를 넘게 실행하지 않는다. (넘으면 버린다.)
//  - 예) deploy/tools/notify_slack.sh (curl로 slack webhook에 보낸다.)
//-----------------------------------------------------------------------------
class admin::event_command
{
public:
			void				set_command(std::string _command);
	[[nodiscard]] std::string	get_command() const;
			void				fire(std::string_view _event, server_system::iid_t _iid, std::string_view _name, std::string_view _message);

private:
			void				process_execute(std::vector<std::string> _arguments);

	mutable	std::mutex			m_lockable;
			std::string			m_command;
			std::atomic<int>	m_count_running{ 0 };
};

//-----------------------------------------------------------------------------
// admin::socket_host_agent (host_agent -> admin server)
//-----------------------------------------------------------------------------
class admin::socket_host_agent :
	public						server_system::host::socket
{
protected:
	virtual void				on_connect() override;
	virtual	bool				process_certify(const server_system::sCERTIFY_INFO& _certify_info, std::string_view _certify_key) override;

			std::atomic<bool>	m_is_rejected{ false };
};

//-----------------------------------------------------------------------------
// admin::host_instance_manager
//-----------------------------------------------------------------------------
class admin::host_instance_manager :
	public						server_system::host::manager
{
public:
	[[nodiscard]] std::size_t	get_count() const;
	[[nodiscard]] std::vector<object_ptr<server_system::host::Ninstance>> get_instances() const;

	// - control api / metrics (host system status)
	[[nodiscard]] std::string	to_json_hosts() const;
	[[nodiscard]] std::string	to_metrics() const;

private:
	mutable	std::mutex			m_lockable_hosts;
			std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>> m_map_hosts;

	virtual	void				on_register_instance(const object_ptr<server_system::host::Ninstance>& _phost_instance) override;
	virtual	void				on_unregister_instance(const object_ptr<server_system::host::Ninstance>& _phost_instance) noexcept override;
	virtual	void				on_update_instance_state(const object_ptr<server_system::host::Ninstance>& _phost_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old) override;
};

//-----------------------------------------------------------------------------
// admin::service_instance_manager
//-----------------------------------------------------------------------------
class admin::service_instance_manager :
	public						server_system::service::manager
{
public:
	// - service에 요청한다. (host_agent를 거쳐 전달된다. return: request id, 보내지 못하면 0)
			uint64_t			request(server_system::iid_t _iid, uint32_t _request);
			uint64_t			request_start(server_system::iid_t _iid) { return this->request(_iid, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE); }
			uint64_t			request_stop(server_system::iid_t _iid) { return this->request(_iid, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE); }
	[[nodiscard]] bool			has_instance(server_system::iid_t _iid) const { return this->find_instance(_iid).exist(); }

	// - setting ("service_instance_manager") : 시작할 때와 reload할 때
			void				apply_setting(context& _context);

	// - 원하는 상태
			void				set_desired_state(server_system::iid_t _iid, eDESIRED_STATE _state);
	[[nodiscard]] eDESIRED_STATE get_desired_state(server_system::iid_t _iid) const { return this->m_desired.get(_iid); }

	// - 자동 재시작 중지(crash loop)를 푼다. (운영자 명령)
			void				reset_restart_limit(server_system::iid_t _iid) { this->m_restart.reset(_iid); }

	// - 출력
			void				print_instances() const;
			void				print_instance(server_system::iid_t _iid) const;

	// - control api / metrics
	[[nodiscard]] std::string	to_json_instances() const;
	[[nodiscard]] std::optional<std::string> to_json_instance(server_system::iid_t _iid) const;
	[[nodiscard]] std::string	to_metrics() const;

private:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_register_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance) override;
	virtual	void				on_unregister_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance) noexcept override;
	virtual	void				on_update_instance_state(const object_ptr<server_system::service::Ninstance>& _pservice_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old) override;
	virtual	void				on_request_result(const object_ptr<server_system::service::Ninstance>& _pservice_instance, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message) override;

			uint64_t			process_send_request(const object_ptr<server_system::service::Ninstance>& _pservice_instance, uint32_t _request);
	[[nodiscard]] std::vector<object_ptr<server_system::service::Ninstance>> process_get_instances() const;
	[[nodiscard]] std::string	process_to_json_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance, bool _is_detail) const;
			void				process_apply_desired_state(const object_ptr<server_system::service::Ninstance>& _pservice_instance, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old);
			void				process_purge_requests();

			std::atomic<bool>	m_is_auto_start{ false };
			std::string			m_desired_file{ DESIRED_STATE_FILE };
			std::chrono::seconds m_request_timeout{ 60 };

			desired_state_store	m_desired;
			restart_limiter		m_restart;
			request_tracker		m_requests;

	mutable	lock_object			m_lockable_instances;
			std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> m_map_instances;
};

//-----------------------------------------------------------------------------
// service
//-----------------------------------------------------------------------------
class service : public server_system::Xservice
{
public:
			service();

	// - console 명령을 처리한다. (false면 종료 요청)
			bool				execute_command(std::string_view _command_line);

	// - control api (admin::socket_control)
			struct sCONTROL_RESULT
			{
				eHTTP_STATUS_CODE	status_code{ eHTTP_STATUS_CODE::OK_ };
				std::string			body;
			};
	[[nodiscard]] sCONTROL_RESULT process_control(const admin::sCONTROL_ROUTE& _route);

	// - 운영자 요청 (console/control api 공통. desired state와 restart limit을 맞춘다. return: request id)
			uint64_t			process_request_command(server_system::iid_t _iid, uint32_t _request);

	// - setting 파일을 다시 읽는다. (접속 정책, service_instance_manager 설정, desired state)
			bool				reload_setting();

	// - setting의 접속 정책을 적용한다.
	static	void				apply_access_policy(context& _context_setting);

	static	object_ptr<admin::host_instance_manager>	phost_instance_manager;
	static	object_ptr<admin::service_instance_manager>	pservice_instance_manager;
	static	own_ptr<net::acceptor<admin::socket_host_agent>> pacceptor_host_agent;
	static	own_ptr<net::acceptor<admin::socket_control>> pacceptor_control;
	static	server_system::access_policy_holder		access_policy_host_agent;
	static	server_system::access_policy_holder		access_policy_control;
	static	admin::auth_fail_limiter				control_auth_limiter;
	static	admin::signature_verifier				control_signature;
	static	admin::event_command					event_hook;
	static	server_system::audit_log				audit;
	static	server_system::sHTTP_ACCEPTOR_LIMITS	limits_control;
	static	std::atomic<bool>						control_require_signature;
	static	std::atomic<bool>						control_metrics_require_signature;
};

//-----------------------------------------------------------------------------
// global
//-----------------------------------------------------------------------------
extern own_ptr<service> g_pservice;

// - 종료 시작 (main이 g_pservice를 닫기 전에 true로 한다.)
//   (다른 thread(socket 콜백)는 g_pservice를 직접 읽지 않고 이것을 본다 : main의 reset과 경합하지 않도록)
extern std::atomic<bool> g_is_service_closing;
