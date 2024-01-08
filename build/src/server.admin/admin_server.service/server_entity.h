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
// server_entity
//
// ----------------------------------------------------------------------------
#if defined(_WIN32)
	using PROCESS_ID = HANDLE;
	#define INVALID_PROCESS_ID	INVALID_HANDLE_VALUE
#else
	using PROCESS_ID = pid_t;
	#define INVALID_PROCESS_ID 0
#endif

class server_entity : 
#ifdef _USE_SVN_CONTROL
	virtual public server_admin::Isvn_notify,
#else
	virtual public Ireferenceable,
#endif
	public observable::list_sender<socket_websocket>,
	public net::io::messageable::Nbase
{
public:
			// 1) server setting
			server_admin::sSERVER_REGISTER info_server_register;

			// 2) server info
			server_admin::sSERVER_STATUS status_server;

			// 3) SERVER STATUS 
			PROCESS_ID					handle_process;
			sSYSTEM_INFO				usage_system;
			object_ptr<api::performance_counter> perfornamce_processor;
			std::atomic<int>			status;
			bool						is_preserve_start;
			std::string					m_setting;

			// 4) socket
			object_ptr<socket_server>	psocket;

			// 5) data
			lockable<>									cs_server_data;
			vector<server_admin::factory_snap>			vector_factory_info;
			vector<server_admin::factory_snap>			vector_executable_info;
			vector<server_admin::executor_snap>			vector_executor_info;
			vector<server_admin::schedulable_snap>		vector_schedulable_info;
			vector<server_admin::connective_info_store>	vector_connective_info;
			vector<server_admin::socket_info_snap>		vector_binder_info;
			vector<server_admin::time_event_snap>		vector_time_event_info;
			server_admin::connective_snap				connective_info;
			server_admin::traffic_snap					traffic_info;
			circular_list<object_ptr<LOG_RECORD>>		vector_log_records;
			int											statistics_time = 0;

	// implementation) 
private:
			//own_ptr<observable_websocket_server_overview_info>		pobservable_websocket_server_overview_info;
			//own_ptr<observable_websocket_server_factory_info>		pobservable_websocket_server_factory_info;
			//own_ptr<observable_websocket_server_executor_info>		pobservable_websocket_server_executor_info;
			//own_ptr<observable_websocket_server_network_info>		pobservable_websocket_server_network_info;
			//own_ptr<observable_websocket_server_querier_info>		pobservable_websocket_server_querier_info;
			//own_ptr<observable_websocket_server_crash_info>			pobservable_websocket_server_crash_info;
public:
			server_entity();
	virtual~ server_entity() noexcept;

public:
			server_entity&		operator = (const server_entity& _rhs)
			{
				set_server_register_info(_rhs.info_server_register);
				status_server = _rhs.status_server;

				handle_process = _rhs.handle_process;
				usage_system = _rhs.usage_system;
				perfornamce_processor = _rhs.perfornamce_processor;
				status.store(_rhs.status.load());
				psocket = _rhs.psocket;
				is_preserve_start = _rhs.is_preserve_start;

				// return) 
				return *this;
			}

public:
			void				reset_system_status_info();

			void				load(const char* p_str_path = ".");
			void				save();

			bool				request_init_service();
			bool				request_destroy_service();
			bool				request_start_service(const std::string& _setting);
			bool				request_stop_service();
			bool				request_continue_service();
			bool				request_pause_service();
			bool				request_kill_service();

			bool				request_update();
			bool				request_check_out();
			bool				request_switch();

			void				set_server_register_info(const server_admin::sSERVER_REGISTER& _info_server_register);
			void				update_application_status();
			void				update_revision_info();

			void				data_factory_info_set				(vector<server_admin::factory_snap>&& _data);
			void				data_factory_info_update			(vector<server_admin::factory_update_snap>&& _data);
			void				data_factory_info_register			(server_admin::factory_snap&& _data);
			void				data_factory_info_unregister		(uint64_t _index);

			void				data_executor_info_set				(vector<server_admin::executor_snap>&& _data);
			void				data_executor_info_update			(vector<server_admin::executor_update_snap>&& _data);
			void				data_executor_info_register			(server_admin::executor_snap&& _data);
			void				data_executor_info_unregister		(uint64_t _index);

			void				data_schedulable_info_set			(vector<server_admin::schedulable_snap>&& _data);

			void				data_connective_info_set			(vector<server_admin::connective_info_snap>&& _data);
			void				data_connective_info_update			(vector<server_admin::connective_info_update_snap>&& _data);
			void				data_connective_info_register		(server_admin::connective_info_snap&& _data);
			void				data_connective_info_unregister		(uint64_t _index);

			void				data_binder_info_set				(vector<server_admin::socket_info_snap>&& _data);
			void				data_binder_info_update				(vector<server_admin::socket_info_update_snap>&& _data);
			void				data_binder_info_register			(server_admin::socket_info_snap&& _data);
			void				data_binder_info_unregister			(uint64_t _index);

			void				data_time_event_info_set			(vector<server_admin::time_event_snap>&& _data);
			void				data_time_event_info_update			(vector<server_admin::time_event_update_snap>&& _data);
			void				data_time_event_info_register		(server_admin::time_event_snap&& _data);
			void				data_time_event_info_unregister		(uint64_t _index);

			void				data_connective_statistics_set		(const statistics::unit_connective_snap& _data);
			void				data_connective_statistics_update	(const statistics::unit_connective_snap& _data);

			void				data_traffic_statistics_set			(const statistics::unit_traffic_snap& _buf_data);
			void				data_traffic_statistics_update		(const statistics::unit_traffic_snap& _buf_data);

			void				data_traffic_log_records_set		(circular_list<object_ptr<LOG_RECORD>>&& _log_records) noexcept;
			void				data_traffic_log_records_add		(circular_list<object_ptr<LOG_RECORD>>&& _log_records) noexcept;

			void				data_crash_info_set					(shared_buffer _buf_data);
			void				data_crash_info_update				(shared_buffer _buf_data);

			eOBJECT_STATE		set_server_state(eOBJECT_STATE _state);

public:
			bool				operator!=(const server_entity& _rhs) const	{ return info_server_register.server_setting.id != _rhs.info_server_register.server_setting.id;}
			bool				operator==(const server_entity& _rhs) const	{ return info_server_register.server_setting.id == _rhs.info_server_register.server_setting.id;}
			bool				operator< (const server_entity& _rhs) const	{ return info_server_register.server_setting.id <  _rhs.info_server_register.server_setting.id;}
			bool				operator<=(const server_entity& _rhs) const	{ return info_server_register.server_setting.id <= _rhs.info_server_register.server_setting.id;}
			bool				operator> (const server_entity& _rhs) const	{ return info_server_register.server_setting.id >  _rhs.info_server_register.server_setting.id;}
			bool				operator>=(const server_entity& _rhs) const	{ return info_server_register.server_setting.id >= _rhs.info_server_register.server_setting.id;}
																													 
			bool				operator!=(const server_admin::sID& _rhs) const { return info_server_register.server_setting.id != _rhs;}
			bool				operator==(const server_admin::sID& _rhs) const { return info_server_register.server_setting.id == _rhs;}
			bool				operator< (const server_admin::sID& _rhs) const { return info_server_register.server_setting.id <  _rhs;}
			bool				operator<=(const server_admin::sID& _rhs) const { return info_server_register.server_setting.id <= _rhs;}
			bool				operator> (const server_admin::sID& _rhs) const { return info_server_register.server_setting.id >  _rhs;}
			bool				operator>=(const server_admin::sID& _rhs) const { return info_server_register.server_setting.id >= _rhs;}

private:
	virtual	result_code			on_observer_registering		(socket_websocket* _pobserver, observer_channel_t _ocid, observer_data_t& _observer_data) override;
	virtual	void				on_observer_registered		(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering	(socket_websocket* _pobserver, observer_channel_t _ocid) override;
	virtual	void				on_observer_unregistered	(socket_websocket* _pobserver, observer_channel_t _ocid, uintptr_t _result) override;
	virtual	uintptr_t			on_observer_data_updating	(socket_websocket* _pobserver, observer_channel_t _ocid, observer_data_t& _observer_data) override;
	virtual	void				on_observer_data_update		(socket_websocket* _pobserver, observer_channel_t _ocid, uintptr_t _result) override;

	virtual result_code			on_message					(sMESSAGE_NETWORK& _msg);

private:
			object_ptr<Iobservable_websocket> _get_pobservable_webpage(char _select) const noexcept;


public:
#ifdef _USE_SVN_CONTROL
	// Isvn_notify을(를) 통해 상속됨
	virtual	void				on_svn_notify	(server_admin::Isvn_command * _baton, const svn_wc_notify_t * _notify, apr_pool_t * _pool) override;
	virtual void				on_svn_procgress(apr_off_t _progress, apr_off_t _total, server_admin::Isvn_command * _baton, apr_pool_t * _pool) override;
	virtual svn_error_t*		on_svn_log		(const char ** _log_msg, const char ** _tmp_file, const apr_array_header_t * _commit_items, server_admin::Isvn_command * _baton, apr_pool_t * _pool) override;
	virtual svn_error_t*		on_svn_conflict_resolver(svn_wc_conflict_result_t ** _result, const svn_wc_conflict_description2_t * _description, server_admin::Isvn_command * _baton, apr_pool_t * _result_pool, apr_pool_t * _scratch_pool) override;
	virtual svn_error_t*		on_svn_cancel	(server_admin::Isvn_command * _cancel_baton) override;
	virtual void				on_svn_complete	(server_admin::Isvn_command * _cancel_baton, const SVN_RESULT& _result) override;
#endif
};
