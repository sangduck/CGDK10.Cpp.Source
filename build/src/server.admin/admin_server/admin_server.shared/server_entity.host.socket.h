//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / Release 2017.01.03                        *
//*                                                                           *
//*                            Server Admin Server                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// host_host::socket
//
//-----------------------------------------------------------------------------
class server_entity::host::socket :
	public						Isocket,
	public						Igroupable<socket>
{
public:
			socket() noexcept;
	virtual ~socket() noexcept;

protected:
	// 1) connect/disconnect
	virtual void				on_request_connect	(const net::sockaddr& _addr_peer) override;
	virtual void				on_connect			(net::io::Iconnective* _pconnective) override;
	virtual void				on_fail_connect		(net::io::Iconnective* _pconnective, uint64_t _reason) override;
	virtual	void				on_disconnect		(uint64_t _reason) override;

	// 2) message
	virtual result_code			on_message			(sMESSAGE_NETWORK& _msg) override;

public:
	// 3) server setting info
			bool				request_admin_send_host_setting				(const server_admin::sHOST_SETTING& _host_setting);
			bool				request_admin_send_admin_server_address		(std::string_view _admin_server_address);

			//object_ptr<instance> register_server_setting					(const server_admin::sSERVER_REGISTER& _server_register, const server_admin::sSERVER_STATUS& _status_server);
			//object_ptr<instance> register_server_setting					(socket* _psocket_host, const server_admin::sSERVER_REGISTER& _server_register);
			//object_ptr<instance> unregister_server_setting					(const server_admin::sID& _id) noexcept;
			bool				unregister_all_server_setting				() noexcept;
			bool				update_server_setting						(const server_admin::sSERVER_REGISTER& _server_register);

	// 4) host info
			auto&				get_host_setting							() const noexcept { return m_setting_host; }
			auto&				get_host_info								() const noexcept { return m_info_host;}
			void				set_host_info								(const server_admin::sHOST_INFO& _host_info);
			void				set_host_setting							(const server_admin::sHOST_SETTING& _host_setting);
			void				set_host_status								(const server_admin::sHOST_STATUS& _host_info);

	// 5) server setting info
			bool				request_register_server_entity				(const server_admin::sSERVER_REGISTER& _server_register, bool _is_update = false);
			bool				request_unregister_server_entity			(const server_admin::sSERVER_REGISTER& _server_register);
			bool				request_update_server_entity				(const server_admin::sSERVER_REGISTER& _server_register, bool _is_update = false);
			bool				request_update_server_entity_setting		(const server_admin::sSERVER_REGISTER& _server_register, bool _is_update = false);
			bool				request_version_update						(const server_admin::sSERVER_REGISTER& _server_register);

			bool				request_init_process						(const instance& _serer_register_info);
			bool				request_start_process						(const instance& _serer_register_info, const std::string& _str_setting);
			bool				request_kill_process						(const instance& _serer_register_info);

	// 6) event
			void				set_event_list								(int _count, const time_event::sEVENT_SETTING* _list_event_info);
			void				add_event									(const time_event::sEVENT_SETTING& _event_info);
			void				remove_event								(const time_event::sEVENT_SETTING& _event_info);
			void				update_event								(const time_event::sEVENT_SETTING& _event_info);
			void				execute_event								(const time_event::sEVENT_SETTING& _event_info);

	// 7) authority
			void				set_authority								(uint64_t _authority);
			void				set_log_index								(uint64_t _index_logIn);

public:
	// 8) registered server
			object_ptr<time_event_server::manager>	m_pmanager_event;

	// 9) host info
			server_admin::sHOST_SETTING				m_setting_host;
			server_admin::sHOST_INFO				m_info_host;

			server_admin::sHOST_STATUS				m_status_host;
			lockable<>								m_lockable_data;

	// 10) authority
			uint64_t								m_authority;

	// statistics)
			int										m_idx_list;
			uint64_t								m_index_log;

	// 11) ...
			object_ptr<instance> _find_server_entity(const server_admin::sID& _idServer);
};
