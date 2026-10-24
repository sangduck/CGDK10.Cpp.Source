//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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

namespace CGDK
{
// ----------------------------------------------------------------------------
//
// Xservice
//
// 
// ----------------------------------------------------------------------------
class server_system::Xservice : public Nservice
{
public:
			Xservice();
			Xservice(const std::string_view _name);
	virtual ~Xservice();

			auto				get_host_instance_manager() const noexcept -> object_ptr<server_system::host::manager> { std::lock_guard lock(this->m_lockable_setting); return this->m_phost_instance_manager; }
			void				set_host_instance_manager(own_ptr<server_system::host::manager>&& _phost_instance_manager);
			void				reset_host_instance_manager() noexcept;
			auto				get_service_instance_manager() const noexcept -> object_ptr<server_system::service::manager> { std::lock_guard lock(this->m_lockable_setting); return this->m_pservice_instance_manager; }
			void				set_service_instance_manager(own_ptr<server_system::service::manager>&& _pservice_instance_manager);
			void				reset_service_instance_manager() noexcept;

			auto				get_socket_to_admin() -> object_ptr<server_system::socket::client>;
			void				send_to_admin_server(const shared_buffer& _buffer);

	// run) runs the service on the console or as a service. (blocks until the service is destroyed)
	//  - [options...]               : console mode. initialize -> start(is_start_on_run) -> wait. exits with ctrl-X.
	//  - service ([name] default: '<exe name>.<current folder name>')
	//    . windows: windows service (SCM. administrator)
	//    . linux  : systemd service ('/etc/systemd/system/<name>.service'. root)
	//    --install [name] [options...]  : installs a service running in the current folder. (auto start on boot)
	//                                     [options...] are registered and passed to the service when it starts.
	//                                     windows: registers the event source '<name>' for logger_system.
	//                                     restarts on failure (windows: after 5s, 10s, 30s. the failure count is reset after 1 day / linux: after 5s)
	//                                     linux: 'TimeoutStopSec' is 'service.setting.stop_grace_period' of the setting file. (if exists)
	//                                     (windows: the wait hint of STOP_PENDING is 'stop_grace_period'. default 60s)
	//    --uninstall [name]         : uninstalls the service. (stops it first)
	//    --start [name]             : starts the service.
	//    --stop [name]              : stops the service.
	//    --service <name> <folder> [options...] : (used by SCM/systemd) runs as the service in <folder>.
	//                                 logs (fatal/error/warning) are also written to the system log. (logger_system: event log/syslog)
	//                                 linux: stops on SIGTERM ('systemctl stop')
	//  - options for '--install' (not registered as the service arguments)
	//    --account <name>           : account to run the service. (default: windows LocalSystem / linux root)
	//                                 windows ex) "NT AUTHORITY\LocalService", "NT AUTHORITY\NetworkService", ".\user", "domain\user"
	//                                 windows: 'Log on as a service' right is granted to the account. (except built-in/virtual accounts)
	//                                 linux: systemd 'User='
	//    --password <password>      : (windows) password of the account. (if omitted, asks on the console)
	//    --no-restart               : does not restart on failure.
	//  - options
	//    --setting <file>           : setting file. (default: _setting_file. relative path is from the running folder)
	//    (others)                   : get_run_arguments() (unknown '--' options are warned on the log)
	//  - _setting_file              : default setting file (json) to initialize the service
	//  - _fn_console_key            : called with the key pressed on console mode (except ctrl-X)
			int					run(int _argc, char** _argv, std::string_view _setting_file, const std::function<void(int)>& _fn_console_key = {});
	[[nodiscard]] static bool	is_windows_service_mode() noexcept { return m_s_is_windows_service_mode; }
	[[nodiscard]] const auto&	get_run_setting_file() const noexcept { return m_run_setting_file; }
	[[nodiscard]] const auto&	get_run_arguments() const noexcept { return m_run_arguments; }
	[[nodiscard]] const auto&	get_run_warnings() const noexcept { return m_run_warnings; }

protected:
	virtual result_code			process_message(sMESSAGE& _msg) override;
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_START_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE( sMESSAGE_NETWORK& _msg);
	virtual	result_code			on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg);

	virtual	void				on_server_certified();

protected:
	[[nodiscard]] virtual auto	process_alloc_socket_client(context& _context) -> object_ptr<server_system::socket::client>;
			auto				process_prepare_socket_client(context& _context) -> object_ptr<server_system::socket::client>;
			void				process_close_socket_client() noexcept;
			void				process_socket_client_certified(server_system::socket::client* _psocket_to_admin, std::map <server_system::iid_t, server_system::sHOST >&& _map_host_info_confirmed, std::map<server_system::iid_t, server_system::sSERVICE>&& _map_service_info_confirmed);

	virtual	result_code			_process_initializing(context& _context) override;
	virtual	void				_process_initializing_pre(context& _context);
	virtual	void				_process_initializing_post(context& _context);
	virtual void				_process_initializing_instance_manager(context& _context);
	virtual	void				_process_initializing_socket_to_admin(context& _context);

	virtual	result_code			_process_initialize(context& _context) override;
	virtual	void				_process_initialize_pre(context& _context);
	virtual	void				_process_initialize_post(context& _context);

	virtual result_code			_process_starting(context& _context) override;
	virtual result_code			_process_stopping() noexcept;

	virtual	result_code			_process_destroying() noexcept override;
	virtual	void				_process_destroying_pre() noexcept;
	virtual	void				_process_destroying_post() noexcept;

	virtual	result_code			_process_destroy() noexcept override;
	virtual	void				_process_destroy_pre() noexcept;
	virtual	void				_process_destroy_post() noexcept;

	virtual	void				process_state_changed(eOBJECT_STATE _state_new, eOBJECT_STATE _state_old) override;

			object_ptr<server_system::socket::client> m_psocket_to_admin;
			friend class server_system::socket::client;

			own_ptr<server_system::host::manager> m_phost_instance_manager;
			own_ptr<server_system::service::manager> m_pservice_instance_manager;

	// run)
	virtual	void				on_console_key(int _key);
	// - returns true to start right after initialize on run. (default: no 'socket_to_admin' setting. if it exists, the admin server requests start.)
	virtual	bool				is_start_on_run(context& _context);
			bool				process_run_initialize(std::string_view _setting_file);
	// - time to wait on stop (ms. setting 'stop_grace_period'(s). default 60s)
	[[nodiscard]] uint32_t		process_get_stop_wait_milliseconds() noexcept;
			int					process_run_console(std::string_view _setting_file, const std::function<void(int)>& _fn_console_key);
#if defined(_WIN32)
			int					process_run_windows_service(std::string_view _service_name, std::string_view _folder, std::string_view _setting_file);
			void				process_windows_service_main();
			void				process_set_windows_service_status(uint32_t _state, uint32_t _exit_code = 0, uint32_t _wait_hint = 0) noexcept;
	static	void WINAPI			fwindows_service_main(DWORD _argc, LPWSTR* _argv);
	static	DWORD WINAPI		fwindows_service_handler(DWORD _control, DWORD _event_type, LPVOID _pevent_data, LPVOID _pcontext);

			std::string			m_windows_service_name;
			std::string			m_windows_service_setting_file;
			SERVICE_STATUS_HANDLE m_hwindows_service_status = nullptr;
			std::atomic<bool>	m_is_windows_service_stop_requested{ false };
	static	inline Xservice*	m_s_pwindows_service = nullptr;
#elif defined(__linux__)
			int					process_run_linux_service(std::string_view _service_name, std::string_view _folder, std::string_view _setting_file);
#endif
	static	inline bool			m_s_is_windows_service_mode = false;
			std::string			m_run_setting_file;
			std::vector<std::string> m_run_arguments;
			std::vector<std::string> m_run_warnings;		// - warnings found on parsing the arguments (written to the log on initialize)
			own_ptr<logger_system> m_plogger_system;		// - service mode: system log (windows: event log, linux: syslog)
};


}