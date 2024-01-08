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
//*                 ⓒ 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// host_agent::socket
//
//-----------------------------------------------------------------------------
class host_agent::socket :
	public						Nsocket_server,
	public						Igroupable<socket>,
	public						Nmessage_transmitter
{
public:
			socket() noexcept;
	virtual ~socket() noexcept;

protected:
	// 1) Connect/disconnect시
	virtual void				on_request_connect	(const net::sockaddr& _paddr_peer) override;
	virtual void				on_connect			(net::io::Iconnective* _pconnective) override;
	virtual void				on_fail_connect		(net::io::Iconnective* _pconnective, uint64_t _reason) override;
	virtual	void				on_disconnect		(uint64_t _reason) override;

	// 2) Message받았을 시
	virtual result_code			on_message			(sMESSAGE_NETWORK& _msg) override;

public:
	// 1) Server setting Info
			bool				request_admin_send_host_setting				(const server_admin::sHOST_SETTING& _host_setting);
			bool				request_admin_send_admin_server_address		(std::string_view _strAdminServerAddress);

			object_ptr<server_entity> register_server_setting				(const server_admin::sSERVER_REGISTER& _server_register, const server_admin::sSERVER_STATUS& _statusServer);
			object_ptr<server_entity> register_server_setting				(host_agent::socket* _psocketHost, const server_admin::sSERVER_REGISTER& _server_register);
			object_ptr<server_entity> unregister_server_setting				(const server_admin::sID& _id) noexcept;
			bool				unregister_all_server_setting				() noexcept;
			bool				update_server_setting						(const server_admin::sSERVER_REGISTER& _server_register);

	// 2) Host Info
			auto&				get_host_setting							() const noexcept { return m_setting_host; }
			auto&				get_host_info								() const noexcept { return m_info_host;}
			void				set_host_info								(const server_admin::sHOST_INFO& _host_info);
			void				set_host_setting							(const server_admin::sHOST_SETTING& _host_setting);
			void				set_host_status								(const server_admin::sHOST_STATUS& _host_info);

	// 3) Server setting Info
			bool				request_register_server_entity				(const server_admin::sSERVER_REGISTER& _server_register, bool _bUpdate = false);
			bool				request_unregister_server_entity			(const server_admin::sSERVER_REGISTER& _server_register);
			bool				request_update_server_entity				(const server_admin::sSERVER_REGISTER& _server_register, bool _bUpdate = false);

			bool				request_version_update						(const server_admin::sSERVER_REGISTER& _server_register);

			bool				request_init_process						(const server_entity& _serer_register_info);
			bool				request_start_process						(const server_entity& _serer_register_info, const std::string& _strSetting);
			bool				request_kill_process						(const server_entity& _serer_register_info);

	// 4) Event
			void				set_event_list								(int _count, const time_event::sEVENT_SETTING* _list_event_info);
			void				add_event									(const time_event::sEVENT_SETTING& _event_info);
			void				remove_event								(const time_event::sEVENT_SETTING& _event_info);
			void				update_event								(const time_event::sEVENT_SETTING& _event_info);
			void				execute_event								(const time_event::sEVENT_SETTING& _event_info);

	// 5) Authority
			void				set_authority								(uint64_t _authority);
			void				set_log_index								(uint64_t _index_logIn);

public:
	// 2) 등록된 Server정보
			lockable<std::vector<server_admin::sSERVER_REGISTER>> m_vector_server_register;
			object_ptr<time_event_server::manager>	m_pmanager_event;

	// 3) Host Info
			server_admin::sHOST_SETTING				m_setting_host;
			server_admin::sHOST_INFO				m_info_host;

			server_admin::sHOST_STATUS				m_statusHost;
			lockable<>								m_csData;

	// 4) Authority
			uint64_t								m_authority;

	// Statistics)
			int										m_idx_list;
			uint64_t								m_indexLog;

	// 4) ...
			object_ptr<server_entity>	_FindServerEntity			(const server_admin::sID& _idServer);
};
