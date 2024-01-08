#include "pch.h"


server_entity::manager_instance::manager_instance(std::string_view _name) :
	Nnameable(_name)
{
}
server_entity::manager_instance::~manager_instance()
{
}

object_ptr<server_entity::instance> server_entity::manager_instance::register_instance(const server_admin::sSERVER_REGISTER& _server_register, server_entity::Isocket* _psocket)
{
	// declare)
	object_ptr<server_entity::instance> pserver_instance;
	object_ptr<server_entity::Isocket> psocket_old;

	scoped_lock(m_lockable_register)
	{
		auto id_key = _server_register.server_setting.id.key;

		// 1) find server_instance
		auto iter_find = m_map_server_register.find(id_key);

		// 2) get server_instance if founded
		if (iter_find != m_map_server_register.end())
		{
			// - server Entity를 복사...
			pserver_instance = iter_find->second;
		}
		else
		{
			// - 신규로 생성한다.
			pserver_instance = make_object<server_entity::instance>();

			// - 추가한다.
			m_map_server_register.insert({ id_key, pserver_instance });
		}

		// 3) set server settings
		pserver_instance->registered = _server_register;

		// 4) set socket
		psocket_old = std::move(pserver_instance->psocket);
		pserver_instance->psocket = _psocket;

		// notify) Server가 Update 되었음을 Notify한다.
		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_REGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
		g_pservice->m_pgroup_client->send(pserver_instance->message_SERVER_ADMIN_SERVER_SETTING_ADD());
	}

	// 5) close socket
	if (psocket_old.exist())
	{
	}

	LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << _server_register.server_setting.name << "'] registered";

	// return)
	return pserver_instance;
}

void server_entity::manager_instance::enter_instance(server_admin::server_info&& _server_info_update, server_entity::Isocket* _psocket)
{
	// declare) 
	object_ptr<server_entity::instance>	pserver_instance;
	object_ptr<server_entity::Isocket>	psocket_old;

	scoped_lock(m_lockable_register)
	{
		// 1) find server_instance
		auto iter_find = m_map_server_register.find(_server_info_update.registered.server_setting.id.key);

		// 2) get server_instance if founded
		RETURN_IF(iter_find == m_map_server_register.end())

		// - Server Entity를 복사...
		pserver_instance = iter_find->second;
		
		// 3) 값을 복사한다.
		{
			pserver_instance->registered = _server_info_update.registered;
			//pserver_instance->status_server = ;

			// - factory/executable/executor/schedulable
			pserver_instance->vector_factory = std::move(_server_info_update.vector_factory);
			pserver_instance->vector_executor = std::move(_server_info_update.vector_executor);
			pserver_instance->vector_schedulable = std::move(_server_info_update.vector_schedulable);

			// - set vector_executable
			for (auto& iter : pserver_instance->vector_factory)
			{
				if (iter.object_type == eOBJECT_TYPE::EXECUTABLE)
				{
					pserver_instance->vector_executable.push_back(iter);
				}
			}

			// - connective info
			vector<server_admin::connective_info_store>	temp_vector_connective_info;
			for (auto& iter : _server_info_update.vector_connective)
			{
				server_admin::connective_info_store temp_connective_info_store;

				temp_connective_info_store.name = iter.name;
				temp_connective_info_store.time_create = iter.time_create;
				temp_connective_info_store.status = iter.status;
				temp_connective_info_store.index = 0;
				temp_connective_info_store.period.push_back(iter.now);

				temp_vector_connective_info.push_back(temp_connective_info_store);
			}
			pserver_instance->vector_connective = std::move(temp_vector_connective_info);

			// - binder_info / time_event_info
			pserver_instance->vector_binder = std::move(_server_info_update.vector_binder);
			pserver_instance->vector_time_event = std::move(_server_info_update.vector_time_event);

			// - connective info 
			server_admin::connective_snap temp_connective_info;
			temp_connective_info.index = 0;
			temp_connective_info.vector_unit_connective_snap.push_back(_server_info_update.connective_info);
			pserver_instance->connective_info = std::move(temp_connective_info);

			// - traffic info
			server_admin::traffic_snap temp_traffic_info;
			temp_traffic_info.index = 0;
			temp_traffic_info.vector_unit_traffic_snap.push_back(_server_info_update.traffic_info);
			pserver_instance->traffic_info = std::move(temp_traffic_info);

			// - log_record
			pserver_instance->vector_log_records = std::move(_server_info_update.vector_log_record);
		}

		// 4) 이전의 socketServer를 복사해 놓는다.
		psocket_old = std::move(pserver_instance->psocket);
		pserver_instance->psocket = _psocket;

		// notify) Server가 Update 되었음을 Notify한다.
		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_instance.get())));
		g_pservice->m_pgroup_client->send(pserver_instance->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	}

	// 6) close socket
	if (psocket_old.exist())
	{
	}

	// log) 
	LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << _server_info_update.registered.server_setting.name << "'] connected";
}

void server_entity::manager_instance::unregister_instance(instance* _pserver_instance) noexcept
{
	// declare) 
	bool is_unregistered = false;

	scoped_lock(m_lockable_register)
	{
		// 1) find server_instance
		auto iter_find = m_map_server_register.find(_pserver_instance->registered.server_setting.id.key);

		// 2) get server_instance if founded
		RETURN_IF(iter_find == m_map_server_register.end());

		// 3) registered or not
		is_unregistered = iter_find->second->registered.is_registered != 0;

		// 4) remove
		if(is_unregistered == false)
			m_map_server_register.erase(iter_find);

		// notify) Server가 Update 되었음을 Notify한다.
		//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
		//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));
	}

	// notify) Server가 Update 되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));

	// log) 
	if (is_unregistered)
		LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << _pserver_instance->registered.server_setting.name << "'] unregistered";
	else
		LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << _pserver_instance->registered.server_setting.name << "'] disconnected";
}

void server_entity::manager_instance::unregister_instance(const server_admin::sID& _id) noexcept
{
	// declare) 
	bool is_unregistered = false;

	// declare) 
	object_ptr<instance> pserver_instance;

	scoped_lock(m_lockable_register)
	{
		// 1) find server_instance
		auto iter_find = m_map_server_register.find(_id.key);

		// 2) get server_instance if founded
		RETURN_IF(iter_find == m_map_server_register.end());

		// 3) set...
		pserver_instance = iter_find->second;

		// 3) registered or not
		is_unregistered = iter_find->second->registered.is_registered != 0;

		// 4) remove
		if (is_unregistered == false)
			m_map_server_register.erase(iter_find);

		// notify) Server가 Update 되었음을 Notify한다.
		//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
		//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));
	}

	// notify) Server가 Update 되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));

	// log) 
	if (is_unregistered)
		LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << pserver_instance->registered.server_setting.name << "'] unregistered";
	else
		LOG_INFO_IMPORTANT << "  - SERVER_INSTANCE['" << pserver_instance->registered.server_setting.name << "'] disconnected";
}


void server_entity::manager_instance::update_instance(server_admin::server_info_update&& _server_info_update) noexcept
{
	scoped_lock(m_lockable_register)
	{
		// 1) find server_instance
		auto iter_find = m_map_server_register.find(_server_info_update.status_server.id.key);

		// 2) get server_instance if founded
		RETURN_IF(iter_find == m_map_server_register.end());

		// 3) updaate
		auto pserver_instance = iter_find->second;

		// 4) 값을 복사한다.
		pserver_instance->data_factory_info_update(std::move(_server_info_update.vector_factory_update));
		pserver_instance->data_executor_info_update(std::move(_server_info_update.vector_executor_update));
		pserver_instance->data_schedulable_info_update(std::move(_server_info_update.vector_schedulable_update));
		pserver_instance->data_connective_info_update(std::move(_server_info_update.vector_connective_update));
		pserver_instance->data_binder_info_update(std::move(_server_info_update.vector_binder_update));
		pserver_instance->data_connective_statistics_update(_server_info_update.connective_info);
		pserver_instance->data_traffic_statistics_update(_server_info_update.traffic_info);

		// notify) Server가 Update 되었음을 Notify한다.
		//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
		//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));
	}

	// notify) Server가 Update 되었음을 Notify한다.
	//g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, reinterpret_cast<uintptr_t>(pserver_instance.get())));
	//g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_SERVER_SETTING_REMOVE::MAKE_BUFFER(pserver_instance));
}

object_ptr<server_entity::instance> server_entity::manager_instance::find_instance(const server_admin::sHOST_ID& _hid, const server_admin::sID& _id) const noexcept
{
	scoped_lock(m_lockable_register)
	{
		// declare)
		object_ptr<server_entity::instance> pserver_instance;

		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
		auto iter_find = m_map_server_register.find(_id.key);

		// check) 찾지 못했으면 ...
		RETURN_IF(iter_find == m_map_server_register.end(), object_ptr<server_entity::instance>());

		// 2) hid가 동일한지 확인한다.
		RETURN_IF(iter_find->second->registered.server_setting.hid != _hid, object_ptr<instance>());

		// return)
		return iter_find->second;
	}
}

object_ptr<server_entity::instance> server_entity::manager_instance::find_instance(uint64_t _id) const noexcept
{
	scoped_lock(m_lockable_register)
	{
		// declare)
		object_ptr<server_entity::instance> pserver_instance;

		// 1) 먼저 이미 등록된 것이 있는지 찾는다.
		auto iter_find = m_map_server_register.find(_id);

		// check) 찾지 못했으면 ...
		if (iter_find != m_map_server_register.end())
		{
			pserver_instance = iter_find->second;
		}

		// return)
		return pserver_instance;
	}
}

vector<object_ptr<server_entity::instance>> server_entity::manager_instance::search_instance(const server_admin::sHOST_ID& _hid, const server_admin::sID& _id) const noexcept
{
	// declare)
	vector<object_ptr<server_entity::instance>> vector_result;

	scoped_lock(m_lockable_register)
	{
		if (_hid.exist())
		{
			if (_id.exist())
			{
				auto temp = find_instance(_hid, _id);

				if(temp.exist())
				{
					vector_result.push_back(temp);
				}
			}
			else
			{
				for (auto& iter : m_map_server_register)
				{
					if(iter.second->registered.server_setting.hid==_hid)
					{
						vector_result.push_back(iter.second);
					}
				}
			}
		}
		else if (_id.key != 0)
		{
			// - 해당 Server Entity를 찾는다.
			auto pserver_instance = find_instance(_id.key);

			// - 해당 Server Entity를 찾았다면...
			if (pserver_instance.exist())
			{
				vector_result.push_back(pserver_instance);
			}
		}
		else
		{
			for(auto& iter: m_map_server_register)
			{
				vector_result.push_back(iter.second);
			}
		}
	}

	// return)
	return vector_result;
}

void server_entity::manager_instance::on_initializing(context& /*_context*/)
{
}

void server_entity::manager_instance::on_initialize(context& /*_context*/)
{
}

void server_entity::manager_instance::on_destroying()
{
}

void server_entity::manager_instance::on_destroy()
{
}

void server_entity::manager_instance::on_starting(context& /*_context*/)
{
}

void server_entity::manager_instance::on_start(context& /*_context*/)
{
}

void server_entity::manager_instance::on_stopping()
{
}

void server_entity::manager_instance::on_stop()
{
}

shared_buffer server_entity::manager_instance::message_SERVER_ADMIN_HOST_SOCKET_LIST()
{
	// declare) 
	vector<sADMIN_HOST_SOCKET> vector_host_info;

	//// 1) Reserve한다.
	//scoped_lock(m_lockable_register)
	//{
	//	// - Reserve
	//	vector_host_info.reserve(get_member_count());

	//	// - 데이터를 써넣는다.
	//	for (auto& iter : m_container_member)
	//	{
	//		const auto& member = iter.get_member();
	//		vector_host_info.emplace_back(member->m_info_host, member->m_status_host);
	//	}
	//}

	// 2) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(vector_host_info));

	//// 3) 메시지 헤더를 써넣는다.
	//buf_temp.append<uint32_t>();
	//buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_HOST_SOCKET_LIST);

	//// 4) 메시지 내용을 써넣는다.
	//buf_temp.append(vector_host_info);

	//// 5) 메시지 길이를 써넣는다.
	//buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}

shared_buffer server_entity::manager_instance::message_SERVER_ADMIN_SERVER_SETTING_LIST()
{
	// declare) 
	vector<sADMIN_SERVER_SETTING> vector_admin_server_setting;

	//// 1) 메시지 내용을 작성한다.
	//scoped_lock(m_lockable_register)
	//{
	//	vector_admin_server_setting.reserve(m_map_server_register.size() + m_vector_server_unregister.size());

	//	for (auto& iter : m_map_server_register)
	//	{
	//		vector_admin_server_setting.push_back(iter.second->Get_ADMIN_SERVER_SETTING());
	//	}

	//	for (auto& iter : m_vector_server_unregister)
	//	{
	//		vector_admin_server_setting.push_back(iter->Get_ADMIN_SERVER_SETTING());
	//	}
	//}

	// 1) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(sizeof(MSG_HEADER) + get_size_of(vector_admin_server_setting));

	//// 2) 메시지 헤더를 써넣는다.
	//buf_temp.append<uint32_t>();
	//buf_temp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::ADMIN_SERVER_SETTING_LIST);

	//// 3) 메시지 내용을 써넣는다.
	//buf_temp.append(vector_admin_server_setting);

	//// 4) 메시지 길이를 써넣는다.
	//buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
}
