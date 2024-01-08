//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ¨Ï2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once

class service : 
// inherited classes)
	public						server::Nservice,
	public						schedulable::Nexecutable
{
// constructor/destructor)
public:
			service() noexcept;
	virtual	~service() noexcept;

// public)
public:
			void				add_event_log(server_event_log* _pserver_event_log);
			//shared_buffer		message_SERVER_ADMIN_SERVER_TIME_EVENT_LOG_LIST();

// frameworks) 
private:
	// 1) setting
	virtual	void				on_service_setting(context& _context) override;

	// 2) initialize/destroy
	virtual	void				on_initializing	(context& _context) override;
	virtual	void				on_initialize	(context& _context) override;
	virtual	void				on_destroying	() override;
	virtual	void				on_destroy		() override;

	// 3) start/Stop
	virtual	void				on_starting		(context& _context) override;
	virtual	void				on_start		(context& _context) override;
	virtual	void				on_stopping		() override;
	virtual	void				on_stop			() override;

	// 4) executable
	virtual	void				on_execute		() override;

// implementation) 
public:
	// 1) querier
			own_ptr<server_entity::manager_instance>		m_pmanager_server_instance;
			own_ptr<querier_server>							m_pquerier_server;

	// 2) host agent
			own_ptr<server_entity::host::group_immigration>	m_pgroup_server_entity_agent_immigration;
			own_ptr<server_entity::host::group>				m_pgroup_server_entity_agent;
			own_ptr<net::acceptor<server_entity::host::socket>> m_pacceptor_server_entity_agent;

	// 3) server
			own_ptr<server_entity::group>					m_pgroup_server_entity;
			own_ptr<server_entity::group_immigration>		m_pgroup_server_entity_immigration;
			own_ptr<net::acceptor<server_entity::socket>>	m_pacceptor_server_entity;

	// 4) admin client - application
			own_ptr<client::app::group_immigration>			m_pgroup_client_immigration;
			own_ptr<client::app::group>						m_pgroup_client;
			own_ptr<net::acceptor<client::app::socket>>		m_pacceptor_client;

	// 5) admin client - web
			own_ptr<net::acceptor<client::web::socket>>		m_pacceptor_client_web;
			own_ptr<client::web::group_immigration>			m_pgroup_client_web_immigration;
			own_ptr<client::web::group>						m_pgroup_client_web;

	// 6) time event
			own_ptr<time_event_server::manager>				m_pmanager_time_event;

	// 7) Server Event Log
			lockable<circular_list<object_ptr<server_event_log>>> m_list_server_event_log;

	// 8) server info
			std::string										m_filename_configure;
};
