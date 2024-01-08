#include "pch.h"


server_entity::instance::instance() noexcept :
	registered(),
	status_server(),
	psocket(),
	index_view(-1)
{

}

server_entity::instance::~instance() noexcept
{
}

result_code server_entity::instance::destroy() noexcept
{
	// 1) reset socket
	psocket.reset();

	// return)
	return eRESULT::SUCCESS;
}

void server_entity::instance::request_service_initialize() noexcept
{
	// check)
	RETURN_IF(psocket.empty());

	//// 1) 전송한다.
	//psocket->request_init_process(*this);
}

void server_entity::instance::request_service_start(const std::string& /*_str_setting*/) noexcept
{
	// check) 
	RETURN_IF(psocket.empty());

	// 1) 전송한다.
	//psocket->request_start_process(*this, _str_setting);
}

void server_entity::instance::request_service_stop() noexcept
{
	// check)
	RETURN_IF(psocket.empty(), );

	//// 1) 전송한다.
	//psocket->request_service_stop();
}

void server_entity::instance::request_service_destroy() noexcept
{
	// check)
	RETURN_IF(psocket.empty(), );

	//// 1) 전송한다.
	//psocket->request_service_destroy();
}

void server_entity::instance::request_service_kill() noexcept
{
	// check)
	RETURN_IF(psocket.empty(), );

	//// 1) 전송한다.
	//psocket->request_kill_process(*this);
}

shared_buffer server_entity::instance::message_SERVER_ADMIN_SERVER_SETTING_ADD()
{
	// declare) 
	server_admin::sSERVER_STATUS	xstatusServer;
	std::string						host_name;
	server_admin::sSERVER_RANK		info_server_setting_rank;
	sVERSION_EX						info_server_setting_version;
	net::sockaddr					addr_peer;

	if(psocket.exist())
	{
		//host_name					 = psocket->m_info_host.host_name;
		//xstatusServer				 = psocket->m_pserver_entity->status_server;
		//info_server_setting_rank	 = psocket->m_info_server_setting.rank;
		//info_server_setting_version	 = psocket->m_info_server_setting.version;
		//addr_peer					 = psocket->get_peer_address();
	}

	// 1) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(registered) + sizeof(server_admin::sSERVER_STATUS) + get_size_of(host_name) + sizeof(server_admin::sSERVER_RANK) + sizeof(sVERSION_EX) + sizeof(net::sockaddr));

	// 2) 메시지 헤더를 써넣는다.
	buf_temp.append<uint32_t>();
	buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_SERVER_SETTING_ADD);

	// 3) 메시지 내용을 써넣는다.
	buf_temp.append<server_admin::sSERVER_REGISTER>(registered);
	buf_temp.append<server_admin::sSERVER_STATUS>(status_server);
	buf_temp.append<std::string>(host_name);
	buf_temp.append<server_admin::sSERVER_RANK>(info_server_setting_rank);
	buf_temp.append<sVERSION_EX>(info_server_setting_version);
	buf_temp.append<net::sockaddr>(addr_peer);

	// 4) 메시지 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

sADMIN_SERVER_SETTING server_entity::instance::Get_ADMIN_SERVER_SETTING() const noexcept
{
	// declare)
	sADMIN_SERVER_SETTING admin_server_setting;
	admin_server_setting.registered = registered;

	//if (psocket.exist())
	//{
	//	admin_server_setting.host_name = psocket->m_info_host.host_name;
	//}

	// return)
	return admin_server_setting;
}

shared_buffer server_entity::instance::message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE()
{
	// declare)
	sADMIN_SERVER_SETTING admin_server_setting = Get_ADMIN_SERVER_SETTING();

	// 1) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(admin_server_setting));

	// 2) 메시지 헤더를 써넣는다.
	buf_temp.append<uint32_t>();
	buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_SERVER_SETTING_UPDATE);

	// 3) 메시지 내용을 써넣는다.
	buf_temp.append<sADMIN_SERVER_SETTING>(admin_server_setting);

	// 4) 메시지 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

void server_entity::instance::set_server_register_info(const server_admin::sSERVER_REGISTER& _info_server_register)
{
	// 1) 값을 복사한다.
	registered = _info_server_register;
}

void server_entity::instance::update_revision_info()
{
	// check)
	RETURN_IF(registered.str_path.empty() == false);

	// 2) URL정보를 읽어서 설정한다.
	if (registered.svn_url.empty() == true)
	{
	}
}

void server_entity::instance::data_factory_info_set(vector<server_admin::factory_snap>&& _data)
{
	scoped_lock(cs_server_data)
	{
		// 1) set executable_info
		vector<server_admin::factory_snap> temp_vector_factory_info;
		vector<server_admin::factory_snap> temp_vector_executable_info;

		for (auto& iter : _data)
		{
			if (iter.object_type == eOBJECT_TYPE::EXECUTABLE)
			{
				temp_vector_executable_info.push_back(iter);
			}

			temp_vector_factory_info.push_back(std::move(iter));
		}

		// 2) set factory info
		vector_factory = std::move(temp_vector_factory_info);
		vector_executable = std::move(temp_vector_executable_info);
	}
}

void server_entity::instance::data_factory_info_update(vector<server_admin::factory_update_snap>&& _data)
{
	// check)
	CGASSERT_ERROR(vector_factory.size() == _data.size());

	scoped_lock(cs_server_data)
	{
		// declare)
		auto iter_factory = vector_factory.begin();
		auto iter_executable = vector_executable.begin();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		// 1) set...
		for (; iter_src != iter_src_end; ++iter_src, ++iter_factory)
		{
			if (iter_factory->object_type == eOBJECT_TYPE::EXECUTABLE)
			{
				iter_executable->updatable = *iter_src;
				++iter_executable;
			}

			iter_factory->updatable = std::move(*iter_src);
		}

		//// 2) ...
		//pobservable_websocket_server_executor_info->request_update_webpage_executable();
		//pobservable_websocket_server_factory_info->request_update_webpage_factory();
	}
}

void server_entity::instance::data_factory_info_register(server_admin::factory_snap&& _data)
{
	scoped_lock(cs_server_data)
	{
		if (_data.object_type == eOBJECT_TYPE::EXECUTABLE)
		{
			vector_executable.push_back(_data);
		}

		vector_factory.push_back(std::move(_data));
	}
}

void server_entity::instance::data_factory_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::instance::data_executor_info_set(vector<server_admin::executor_snap>&& _vector_data)
{
	scoped_lock(cs_server_data)
	{
		// declare) 
		vector<server_admin::executor_snap> temp_vector_executor_snap;

		// 1) set 
		for (auto& iter : _vector_data)
		{
			server_admin::executor_snap temp_executor_snap;

			temp_executor_snap.name = iter.name;
			temp_executor_snap.type = iter.type;
			temp_executor_snap.updatable = iter.updatable;

			temp_vector_executor_snap.push_back(temp_executor_snap);
		}
		vector_executor = std::move(temp_vector_executor_snap);
	}

	//// 2) notify
	//pobservable_websocket_server_executor_info->request_update_webpage_executor();
}

void server_entity::instance::data_executor_info_update(vector<server_admin::executor_update_snap>&& _data)
{
	// check)
	CGASSERT_ERROR(vector_executor.size() == _data.size());

	scoped_lock(cs_server_data)
	{
		// declare)
		auto iter_dest = vector_executor.begin();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		// 1) set...
		for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		{
			iter_dest->updatable = *iter_src;
		}
	}

	//// 2) notify
	//pobservable_websocket_server_executor_info->request_update_webpage_executor();
}

void server_entity::instance::data_executor_info_register(server_admin::executor_snap&& /*_data*/)
{
}

void server_entity::instance::data_executor_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::instance::data_schedulable_info_set(vector<server_admin::schedulable_snap>&& _data)
{
	scoped_lock(cs_server_data)
	{
		// declare)
		vector<server_admin::schedulable_snap> vector_deleted;
		vector<server_admin::schedulable_snap> vector_new;

		// 1) get added/deleted schedulable
		auto iter_dest = vector_schedulable.begin();
		auto iter_dest_end = vector_schedulable.end();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		while (iter_src != iter_src_end && iter_dest != iter_dest_end)
		{
			// - updated
			if (iter_dest->updatable.id == iter_src->updatable.id)
			{
				++iter_dest;
				++iter_src;
			}
			// - new
			else if (iter_dest->updatable.id > iter_src->updatable.id)
			{
				vector_new.push_back(*iter_src);
				++iter_src;
			}
			// - deleted
			else if (iter_dest->updatable.id < iter_src->updatable.id)
			{
				vector_deleted.push_back(*iter_dest);
				++iter_dest;
			}
		}

		for (; iter_src != iter_src_end; ++iter_src)
		{
			vector_new.push_back(*iter_src);
		}

		for (; iter_dest != iter_dest_end; ++iter_dest)
		{
			vector_deleted.push_back(*iter_dest);
		}

		// check)
		CGASSERT_ERROR(_data.size() + vector_deleted.size() == vector_schedulable.size() + vector_new.size())

			// 2) set 
			vector_schedulable = std::move(_data);
	}

	//// 3) request update
	//pobservable_websocket_server_executor_info->request_update_webpage_schedulable();
}

void server_entity::instance::data_schedulable_info_update(vector<server_admin::schedulable_update_snap>&& _data)
{
	scoped_lock(cs_server_data)
	{
		// declare)
		vector<server_admin::schedulable_snap> vector_deleted;
		vector<server_admin::schedulable_update_snap> vector_new;

		// 1) get added/deleted schedulable
		auto iter_dest = vector_schedulable.begin();
		auto iter_dest_end = vector_schedulable.end();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		while (iter_src != iter_src_end && iter_dest != iter_dest_end)
		{
			// - updated
			if (iter_dest->updatable.id == iter_src->id)
			{
				iter_dest->updatable = *iter_src;
				++iter_dest;
				++iter_src;
			}
			// - new
			else if (iter_dest->updatable.id > iter_src->id)
			{
				vector_new.push_back(*iter_src);
				++iter_src;
			}
			// - deleted
			else if (iter_dest->updatable.id < iter_src->id)
			{
				vector_deleted.push_back(*iter_dest);
				++iter_dest;
			}
		}

		for (; iter_src != iter_src_end; ++iter_src)
		{
			vector_new.push_back(*iter_src);
		}

		for (; iter_dest != iter_dest_end; ++iter_dest)
		{
			vector_deleted.push_back(*iter_dest);
		}

		// check)
		CGASSERT_ERROR(_data.size() + vector_deleted.size() == vector_schedulable.size() + vector_new.size())
	}

	//// 3) request update
	//pobservable_websocket_server_executor_info->request_update_webpage_schedulable();
}


void server_entity::instance::data_connective_info_set(vector<server_admin::connective_info_snap>&& _vector_data)
{
	scoped_lock(cs_server_data)
	{
		// check)
		CGASSERT_ERROR(_vector_data.size() == vector_connective.size());

		auto iter_src = _vector_data.begin();
		auto iter_src_end = _vector_data.end();
		auto iter_dest = vector_connective.begin();

		for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		{
			const auto index = iter_src->index;

			if (iter_dest->period.size() <= index)
				iter_dest->period.resize(index + 1);

			iter_dest->status = iter_src->status;
			iter_dest->period.at(index) = iter_src->now;
		}
	}
}

void server_entity::instance::data_connective_info_update(vector<server_admin::connective_info_update_snap>&& _vector_data)
{
	// check)
	CGASSERT_ERROR(vector_connective.size() == _vector_data.size());

	scoped_lock(cs_server_data)
	{
		// declare)
		auto iter_dest = vector_connective.begin();
		auto iter_src = _vector_data.begin();
		auto iter_src_end = _vector_data.end();

		// 1) set...
		for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		{
			const auto index = 0;

			if (iter_dest->period.size() <= index)
				iter_dest->period.resize(index + 1);

			iter_dest->status = iter_src->status;
			iter_dest->period.at(index) = iter_src->now;
		}
	}

	//// 2) notify
	//pobservable_websocket_server_network_info->request_update_webpage_connective();
}

void server_entity::instance::data_connective_info_register(server_admin::connective_info_snap&& /*_data*/)
{
}

void server_entity::instance::data_connective_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::instance::data_binder_info_set(vector<server_admin::socket_info_snap>&& _data)
{
	scoped_lock(cs_server_data)
	{
		vector_binder = std::move(_data);
	}
}

void server_entity::instance::data_binder_info_update(vector<server_admin::socket_info_update_snap>&& _data)
{
	// check)
	CGASSERT_ERROR(vector_binder.size() == _data.size());

	scoped_lock(cs_server_data)
	{
		// declare)
		auto iter_dest = vector_binder.begin();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		// 1) set...
		for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		{
			iter_dest->updatable = std::move(*iter_src);
		}
	}

	//// 2) notify
	//pobservable_websocket_server_network_info->request_update_webpage_binder();
}

void server_entity::instance::data_binder_info_register(server_admin::socket_info_snap&& /*_data*/)
{
}

void server_entity::instance::data_binder_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::instance::data_time_event_info_set(vector<server_admin::time_event_snap>&& _data)
{
	scoped_lock(cs_server_data)
	{
		// 1) ....
		vector_time_event = std::move(_data);
	}

	//// 2) notify
	//pobservable_websocket_server_overview_info->request_update_webpage_time_event();
}

void server_entity::instance::data_time_event_info_update(vector<server_admin::time_event_update_snap>&& _data)
{
	// check)
	CGASSERT_ERROR(vector_time_event.size() == _data.size());

	scoped_lock(cs_server_data)
	{
		// declare)
		auto iter_dest = vector_time_event.begin();
		auto iter_src = _data.begin();
		auto iter_src_end = _data.end();

		// 1) set...
		for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		{
			iter_dest->updatable = *iter_src;
		}
	}

	//// 2) notify
	//pobservable_websocket_server_overview_info->request_update_webpage_time_event();
}

void server_entity::instance::data_time_event_info_register(server_admin::time_event_snap&& /*_data*/)
{
}

void server_entity::instance::data_time_event_info_unregister(uint64_t /*_index*/)
{
}

void server_entity::instance::data_connective_statistics_set(const statistics::unit_connective_snap& _data)
{
	// 1) set
	scoped_lock(cs_server_data)
	{
		connective_info.index = 0;
		connective_info.vector_unit_connective_snap.clear();
		//connective_info.vector_unit_connective_snap.reserve();
		connective_info.vector_unit_connective_snap.push_back(_data);
	}
}

void server_entity::instance::data_connective_statistics_update(const statistics::unit_connective_snap& _data)
{
	scoped_lock(cs_server_data)
	{
		auto index = 0;
		if (connective_info.vector_unit_connective_snap.size() <= index)
		{
			connective_info.vector_unit_connective_snap.resize(index + 1);
		}
		connective_info.vector_unit_connective_snap.at(index) = _data;
	}
}

void server_entity::instance::data_traffic_statistics_set(const statistics::unit_traffic_snap& _data)
{
	scoped_lock(cs_server_data)
	{
		traffic_info.index = 0;
		traffic_info.vector_unit_traffic_snap.clear();
		//traffic_info.vector_unit_traffic_snap.reserve();
		traffic_info.vector_unit_traffic_snap.push_back(_data);
	}
}

void server_entity::instance::data_traffic_statistics_update(const statistics::unit_traffic_snap& _data)
{
	scoped_lock(cs_server_data)
	{
		auto index = 0;
		if (traffic_info.vector_unit_traffic_snap.size() <= index)
		{
			traffic_info.vector_unit_traffic_snap.resize(index + 1);
		}
		traffic_info.vector_unit_traffic_snap.at(index) = _data;
	}
}

void server_entity::instance::data_traffic_log_records_set(circular_list<object_ptr<LOG_RECORD>>&& _vector_log_records) noexcept
{
	scoped_lock(cs_server_data)
	{
		vector_log_records = std::move(_vector_log_records);
	}
}

void server_entity::instance::data_traffic_log_records_add(circular_list<object_ptr<LOG_RECORD>>&& _vector_log_records) noexcept
{
	scoped_lock(cs_server_data)
	{
		vector_log_records.splice(vector_log_records.end(), _vector_log_records);
	}
}

void server_entity::instance::data_crash_info_set(shared_buffer _buf_data)
{
	scoped_lock(cs_server_data)
	{
	}
}

void server_entity::instance::data_crash_info_update(shared_buffer _buf_data)
{
	scoped_lock(cs_server_data)
	{
	}
}
