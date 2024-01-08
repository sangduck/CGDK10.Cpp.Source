#pragma once

namespace server_entity
{

struct instance : virtual public Idestroyable
{
public:
			instance() noexcept;
	virtual	~instance() noexcept;

public:
	virtual	result_code			destroy() noexcept override;

			void				request_service_initialize() noexcept;
			void				request_service_start(const std::string& _settings) noexcept;
			void				request_service_stop() noexcept;
			void				request_service_destroy() noexcept;
			void				request_service_kill() noexcept;

public:
			// 1) server setting/status
			server_admin::sSERVER_REGISTER				registered;
			server_admin::sSERVER_STATUS				status_server;

			// 2) statistics - server component
			lockable<>									cs_server_data;
			vector<server_admin::factory_snap>			vector_factory;
			vector<server_admin::factory_snap>			vector_executable;
			vector<server_admin::executor_snap>			vector_executor;
			vector<server_admin::schedulable_snap>		vector_schedulable;
			vector<server_admin::connective_info_store>	vector_connective;
			vector<server_admin::socket_info_snap>		vector_binder;
			vector<server_admin::time_event_snap>		vector_time_event;

			// 3) statistics - traffic
			server_admin::connective_snap				connective_info;
			server_admin::traffic_snap					traffic_info;

			// 4) statistics - log
			circular_list<object_ptr<LOG_RECORD>>		vector_log_records;

			// 5) socket
			object_ptr<Isocket>							psocket;

public:
			// view) 
			int32_t										index_view;

public:
			sADMIN_SERVER_SETTING Get_ADMIN_SERVER_SETTING() const noexcept;
			shared_buffer		message_SERVER_ADMIN_SERVER_SETTING_ADD();
			shared_buffer		message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE();

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
			void				data_schedulable_info_update		(vector<server_admin::schedulable_update_snap>&& _data);

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

};


}
