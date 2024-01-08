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
// group_server
//
// 1. group_server는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class service :
// inherited classes)
	public						server_system::service_application<service>,
	public						schedulable::Nexecutable
{
// constructor/destructor)
public:
			service();
	virtual ~service();

// public) 
public:
	// 1) server register info
			bool				register_server_setting		(server_entity* _pregister_info);
			bool				unregister_server_setting	(server_entity* _pregister_info);
			void				unregister_server_setting_all();
			object_ptr<server_entity> update_server_setting	(const server_admin::sSERVER_REGISTER& _server_register_info);
			object_ptr<server_entity> find_server_entity(uint64_t _id_select) const noexcept;

	// 2) request
			bool				request_init_service		(const server_admin::sID& _id);
			bool				request_destroy_service		(const server_admin::sID& _id);
			bool				request_start_service		(const server_admin::sID& _id, const std::string& _setting);
			bool				request_stop_service		(const server_admin::sID& _id);
			bool				request_pause_service		(const server_admin::sID& _id);
			bool				request_continue_service	(const server_admin::sID& _id);
			bool				request_kill_service		(const server_admin::sID& _id);
			bool				request_version_update		(const server_admin::sID& _i64SystemID);

	// 3) set/get
			const auto&			get_host_info				() const noexcept { return m_info_host; }
			const auto&			get_host_setting			() const noexcept { return m_setting_host; }
			void				set_host_info				(const server_admin::sHOST_INFO& _host_info);
			void				set_host_setting			(const server_admin::sHOST_SETTING& _host_setting);
			void				set_admin_server_address	(const std::string_view _address);

	// 4) notify
	static	void				notify_to_controls			(const shared_buffer& _buffer);

// framework)
protected:
	virtual	void				on_service_setting			(context& _context) override;

	virtual	void				on_initializing				(context& _context) override;
	virtual	void				on_initialize				(context& _context) override;
	virtual	void				on_destroying				() override;
	virtual	void				on_destroy					() override;
	virtual	void				on_starting					(context& _context) override;
	virtual	void				on_start					(context& _context) override;
	virtual	void				on_stopping					() override;
	virtual	void				on_stop						() override;

	virtual	void				on_pause					() override;
	virtual	void				on_resume					() override;

	virtual	void				on_execute					() override;

// message) 
protected:
	DECLARE_CGMESSAGE_MAP

	virtual result_code			process_message(sMESSAGE& _msg) override;

			result_code			on_message_WINDOWS_NOTIFY_UPDATE(sMESSAGE_NOTIFY& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& _msg);
			result_code			on_message_LOG(sMESSAGE_LOG& _msg);
public:
			void				send_message_SYSTEM_STATUS_INFO();

			shared_buffer		message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS();
			shared_buffer		message_SERVER_ADMIN_SEND_HOSTINFO();
			shared_buffer		message_SERVER_ADMIN_SEND_HOST_SETTING();
			shared_buffer		message_SERVER_ADMIN_HOST_STATUS_UPDATE();
			shared_buffer		message_SERVER_ADMIN_SERVER_SETTING_LIST();

protected:
			void				_notify_to_controls(const shared_buffer& _buffer);
			void				_notify_to_controls(shared_buffer&& _buffer);

// implementation) 
public:
	// 1) socket/acceptor/group
			own_ptr<net::acceptor<socket_server>>		m_pacceptor_server;
			own_ptr<net::acceptor<socket_host_agent>>	m_pacceptor_control;
			own_ptr<group_server_immigration>			m_pgroup_server_immigration;
			own_ptr<group_server>						m_pgroup_server;
			own_ptr<group_host_agent_immigration>		m_pgroup_host_agent_immigration;
			own_ptr<group_host_agent>					m_pgroup_host_agent;

	// 2) socket/web
			own_ptr<net::acceptor<socket_web>>			m_pacceptor_web;
			own_ptr<net::acceptor<socket_websocket>>	m_pacceptor_websocket;
			own_ptr<group_websocket_immigration>		m_pgroup_websocket_immigration;
			own_ptr<group_websocket>					m_pgroup_websocket;

	// 3) Host Info
			server_admin::sHOST_SETTING					m_setting_host;
			server_admin::sHOST_INFO					m_info_host;
			server_admin::sHOST_STATUS					m_status_host;

	// 4) status
			bool										m_is_disconnected_unregistered;
			object_ptr<api::performance_counter>		m_perfornamce_processor;
			std::string									m_address_admin_server;
			
	// 5) registered server info
			lockable<list<object_ptr<server_entity>>>	m_list_server_entity;
			
	// 6) ...
			void										load_host_server_info();
			void										save_host_server_info();
			void										load_server_list();
			void										save_server_list();

			void										update_system_status_info();

			object_ptr<server_entity>					find_registered_server_setting	(const server_admin::sID& _id) const noexcept;
			object_ptr<server_entity>					find_registered_server_setting	(uint64_t _pid) const noexcept;
			object_ptr<server_entity>					init_server_register_info		(uint64_t _pid, const sVERSION& _Version);

	static	uint64_t									_alloc_object_id();
	static	std::atomic<uint64_t>						m_id_alloc;
	
	// 7) Log
			own_ptr<logger_file>						m_plogger_file;
};
