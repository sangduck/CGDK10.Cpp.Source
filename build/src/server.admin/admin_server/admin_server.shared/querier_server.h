//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                                user Server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ¨Ï2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
/**

 @class		querier Server

 @brief
 @todo
 @ref
 @details

*///-----------------------------------------------------------------------------
class querier_server :
	public						query::rdb::querier//,
	//virtual public				wal::Inotifiable<query_event::host_certify>,
	//virtual public				wal::Inotifiable<query_event::host_close>,
	//virtual public				wal::Inotifiable<query_event::server_certify>,
	//virtual public				wal::Inotifiable<query_event::server_close>,
	//virtual public				wal::Inotifiable<query_event::server_state>,
	//virtual public				wal::Inotifiable<query_event::market_state>,
	//virtual public				wal::Inotifiable<query_event::market_status>,
	//virtual public				wal::Inotifiable<query_event::time_event_register>,
	//virtual public				wal::Inotifiable<query_event::time_event_unregister>,
	//virtual public				wal::Inotifiable<query_event::time_event_process>,
	//virtual public				wal::Inotifiable<query_event::time_event_done>,
	//virtual public				wal::Inotifiable<query_event::time_event_update>
{
public:
			querier_server(std::string_view _name);
	virtual ~querier_server();

public:
	// 1) ...
			void				request_host_certify			(server_entity::host::socket* _psocket);
			void				request_host_close				(server_entity::host::socket* _psocket);
			void				request_server_certify			(server_entity::socket* _psocket);
			void				request_server_close			(server_entity::socket* _psocket);
			void				request_server_state			(server_entity::socket* _psocket, eOBJECT_STATE _state);
			void				request_market_state			(server_entity::socket* _psocket, eOBJECT_STATE _state);
			void				request_market_status			(server_entity::socket* _psocket);
			void				request_register_time_event		(time_event_server::operation* _ptime_event);
			void				request_unregister_time_event	(time_event_server::operation* _ptime_event);
			void				request_store_time_event_result	(time_event_server::server_operation* _pserver_operation, uint64_t _id_event);
			void				request_done_time_event_result	(time_event_server::operation* _pserver_operation, uint64_t _id_event);
			void				request_update_time_event		(time_event_server::operation* _pserver_operation, uint64_t _id_event);

private:
	// 2) initialize/destroy
	virtual	void				on_initializing					(context& _context) override;
	virtual	void				on_initialize					(context& _context) override;
	virtual	void				on_destroying					() override;
	virtual	void				on_destroy						() override;

	// 3) start/Stop
	virtual	void				on_starting						(context& _context) override;
	virtual	void				on_start						(context& _context) override;
	virtual	void				on_stopping						() override;
	virtual	void				on_stop							() override;

public:
	//// 5) wal
	//virtual void				process_wal_restore				(query_event::host_certify* _query) override;
	//virtual void				process_wal_appended			(query_event::host_certify* _pquery) override;

	//virtual void				process_wal_restore				(query_event::host_close* _query) override;
	//virtual void				process_wal_appended			(query_event::host_close* _pquery) override;

	//virtual void				process_wal_restore				(query_event::server_certify* _query) override;
	//virtual void				process_wal_appended			(query_event::server_certify* _pquery) override;

	//virtual void				process_wal_restore				(query_event::server_close* _query) override;
	//virtual void				process_wal_appended			(query_event::server_close* _pquery) override;

	//virtual void				process_wal_restore				(query_event::server_state* _query) override;
	//virtual void				process_wal_appended			(query_event::server_state* _pquery) override;

	//virtual void				process_wal_restore				(query_event::market_state* _query) override;
	//virtual void				process_wal_appended			(query_event::market_state* _pquery) override;

	//virtual void				process_wal_restore				(query_event::market_status* _query) override;
	//virtual void				process_wal_appended			(query_event::market_status* _pquery) override;

	//virtual void				process_wal_restore				(query_event::time_event_register* _query) override;
	//virtual void				process_wal_appended			(query_event::time_event_register* _pquery) override;

	//virtual void				process_wal_restore				(query_event::time_event_unregister* _query) override;
	//virtual void				process_wal_appended			(query_event::time_event_unregister* _pquery) override;

	//virtual void				process_wal_restore				(query_event::time_event_process* _query) override;
	//virtual void				process_wal_appended			(query_event::time_event_process* _pquery) override;

	//virtual void				process_wal_restore				(query_event::time_event_done* _query) override;
	//virtual void				process_wal_appended			(query_event::time_event_done* _pquery) override;

	//virtual void				process_wal_restore				(query_event::time_event_update* _query) override;
	//virtual void				process_wal_appended			(query_event::time_event_update* _pquery) override;

public:
			std::atomic<uint64_t>	m_uid_host;
			std::atomic<uint64_t>	m_uid_server;
			std::atomic<uint64_t>	m_uid_market;
			std::atomic<uint64_t>	m_uid_command;

			//own_ptr<wal::querier<query_event::host_certify>>			m_pquerier_wal_host_certify;
			//own_ptr<wal::querier<query_event::host_close>>				m_pquerier_wal_host_close;
			//own_ptr<wal::querier<query_event::server_certify>>			m_pquerier_wal_server_certify;
			//own_ptr<wal::querier<query_event::server_close>>			m_pquerier_wal_server_close;
			//own_ptr<wal::querier<query_event::server_state>>			m_pquerier_wal_server_state;
			//own_ptr<wal::querier<query_event::market_state>>			m_pquerier_wal_market_state;
			//own_ptr<wal::querier<query_event::market_status>>			m_pquerier_wal_market_status;
			//own_ptr<wal::querier<query_event::time_event_register>>		m_pquerier_wal_time_event_register;
			//own_ptr<wal::querier<query_event::time_event_unregister>>	m_pquerier_wal_time_event_unregister;
			//own_ptr<wal::querier<query_event::time_event_process>>		m_pquerier_wal_time_event_process;
			//own_ptr<wal::querier<query_event::time_event_done>>			m_pquerier_wal_time_event_done;
			//own_ptr<wal::querier<query_event::time_event_update>>		m_pquerier_wal_time_event_update;
};
