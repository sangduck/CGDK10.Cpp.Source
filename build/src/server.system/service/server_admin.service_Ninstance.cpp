#include "pch.h"

using namespace CGDK;


void server_system::service::Ninstance::set_instance(server_system::service::Ninstance&& _instance_service)
{
	this->info = std::move(_instance_service.info);
}

void server_system::service::Ninstance::set_instance_setting(server_system::sSERVICE::sSETTING&& _service_setting)
{
	this->info.setting = std::move(_service_setting);
}

void server_system::service::Ninstance::update_instance_status(sSERVICE_STATUS_UPDATE&& _service_info_update)
{
	//this->info.status.vector_process_status.push_back(std::move(_service_info_update.process_status));
	this->update_factory_info(std::move(_service_info_update.vector_factory_update));
	this->update_executor_info(std::move(_service_info_update.vector_executor_update));
	this->update_schedulable_info(std::move(_service_info_update.vector_schedulable_update));
	this->update_connective_info(std::move(_service_info_update.vector_connective_update));
	this->update_binder_info(std::move(_service_info_update.vector_binder_update));
	this->update_connective_statistics(_service_info_update.connective_info);
	this->update_traffic_statistics(_service_info_update.traffic_info);
}

void server_system::service::Ninstance::register_factory_info(server_system::factory_snap&& _data)
{
	// 1) add
	if (_data.object_type == eOBJECT_TYPE::EXECUTABLE)
	{
		this->vector_executable.push_back(_data);
	}

	// 2) add
	this->info.status.vector_factory.push_back(std::move(_data));
}

void server_system::service::Ninstance::unregister_factory_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->info.status.vector_factory.begin(), this->info.status.vector_factory.end(), [=](const server_system::factory_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->info.status.vector_factory.end());

	// check) 
	RETURN_IF(iter_find == this->info.status.vector_factory.end());

	// 2) add
	if (iter_find->object_type == eOBJECT_TYPE::EXECUTABLE)
	{
		// - find
		auto iter_find_nested = std::find_if(this->vector_executable.begin(), this->vector_executable.end(), [=](const server_system::factory_snap& _iter) { return _iter.updatable.id == _index; });

		// check)
		CGASSERT_ERROR(iter_find_nested != vector_executable.end());

		// - erase
		this->vector_executable.erase(iter_find_nested);
	}

	// 3) erase
	this->info.status.vector_factory.erase(iter_find);
}

void server_system::service::Ninstance::update_factory_info(std::vector<server_system::factory_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->info.status.vector_factory.size() != _data.size(), );

	// declare)
	auto iter_factory = this->info.status.vector_factory.begin();
	auto iter_executable = this->vector_executable.begin();
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
}

void server_system::service::Ninstance::register_executor_info(server_system::executor_snap&& _data)
{
	// 1) insert 
	this->info.status.vector_executor.push_back(std::move(_data));
}

void server_system::service::Ninstance::unregister_executor_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->info.status.vector_executor.begin(), this->info.status.vector_executor.end(), [=](const server_system::executor_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->info.status.vector_executor.end());

	// check) 
	RETURN_IF(iter_find == this->info.status.vector_executor.end());

	// 2) erase
	this->info.status.vector_executor.erase(iter_find);
}

void server_system::service::Ninstance::update_executor_info(std::vector<server_system::executor_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->info.status.vector_executor.size() != _data.size(), );

	// declare)
	auto iter_dest = this->info.status.vector_executor.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
		iter_dest->updatable = *iter_src;
}

void server_system::service::Ninstance::update_schedulable_info(std::vector<server_system::schedulable_update_snap>&& _data)
{
	// declare)
	std::vector<server_system::schedulable_snap> vector_deleted;
	std::vector<server_system::schedulable_update_snap> vector_new;

	// 1) get added/deleted schedulable
	auto iter_dest = this->info.status.vector_schedulable.begin();
	auto iter_dest_end = this->info.status.vector_schedulable.end();
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
	CGASSERT_ERROR(_data.size() + vector_deleted.size() == this->info.status.vector_schedulable.size() + vector_new.size());

	// 2) remove deleted items
	for (auto& del : vector_deleted)
	{
		auto it = std::find_if(
			this->info.status.vector_schedulable.begin(),
			this->info.status.vector_schedulable.end(),
			[&](const server_system::schedulable_snap& s) { return s.updatable.id == del.updatable.id; });
		if (it != this->info.status.vector_schedulable.end())
			this->info.status.vector_schedulable.erase(it);
	}

	// 3) add new items (sorted by id)
	for (auto& item : vector_new)
	{
		server_system::schedulable_snap snap;
		snap.updatable = item;
		auto it = std::lower_bound(
			this->info.status.vector_schedulable.begin(),
			this->info.status.vector_schedulable.end(),
			item.id,
			[](const server_system::schedulable_snap& s, uint64_t id) { return s.updatable.id < id; });
		this->info.status.vector_schedulable.insert(it, std::move(snap));
	}
}

void server_system::service::Ninstance::register_connective_info(server_system::connective_info_snap&& _data)
{
	// 1) insert 
	this->info.status.vector_connective.push_back(std::move(_data));
}

void server_system::service::Ninstance::unregister_connective_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->info.status.vector_connective.begin(), this->info.status.vector_connective.end(), [=](const server_system::connective_info_snap& _iter) { return _iter.index == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->info.status.vector_connective.end());

	// check) 
	RETURN_IF(iter_find == this->info.status.vector_connective.end());

	// 2) erase
	this->info.status.vector_connective.erase(iter_find);
}

void server_system::service::Ninstance::update_connective_info(std::vector<server_system::connective_info_update_snap>&& _vector_data)
{
	// check)
	RETURN_IF(this->info.status.vector_connective.size() != _vector_data.size(), );

	// declare)
	auto iter_dest = this->info.status.vector_connective.begin();
	auto iter_src = _vector_data.begin();
	auto iter_src_end = _vector_data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->status = iter_src->status;
		iter_dest->now = iter_src->now;
	}
}

void server_system::service::Ninstance::register_binder_info(server_system::socket_info_snap&& _data)
{
	this->info.status.vector_binder.push_back(std::move(_data));
}

void server_system::service::Ninstance::unregister_binder_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->info.status.vector_binder.begin(), this->info.status.vector_binder.end(), [=](const server_system::socket_info_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->info.status.vector_binder.end());

	// check) 
	RETURN_IF(iter_find == this->info.status.vector_binder.end());

	// 2) erase
	this->info.status.vector_binder.erase(iter_find);
}

void server_system::service::Ninstance::update_binder_info(std::vector<server_system::socket_info_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->info.status.vector_binder.size() != _data.size(), );

	// declare)
	auto iter_dest = this->info.status.vector_binder.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->updatable = std::move(*iter_src);
	}
}

void server_system::service::Ninstance::register_time_event_info(server_system::time_event_snap&& _data)
{
	// 1) insert 
	this->info.status.vector_time_event.push_back(std::move(_data));
}

void server_system::service::Ninstance::unregister_time_event_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->info.status.vector_time_event.begin(), this->info.status.vector_time_event.end(), [=](const server_system::time_event_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->info.status.vector_time_event.end());

	// check) 
	RETURN_IF(iter_find == this->info.status.vector_time_event.end());

	// 2) erase
	this->info.status.vector_time_event.erase(iter_find);
}

void server_system::service::Ninstance::update_time_event_info(std::vector<server_system::time_event_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->info.status.vector_time_event.size() != _data.size(), );

	// declare)
	auto iter_dest = this->info.status.vector_time_event.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->updatable = *iter_src;
	}
}

void server_system::service::Ninstance::update_connective_statistics(const statistics::unit_connective_snap& _data)
{
	this->info.status.connective_info = _data;
}

void server_system::service::Ninstance::update_traffic_statistics(const statistics::unit_traffic_snap& _data)
{
	this->info.status.traffic_info = _data;
}

void server_system::service::Ninstance::add_log_records(circular_list<object_ptr<LOG_RECORD>>&& _vector_log_records) noexcept
{
	this->info.status.vector_log_record.splice(this->info.status.vector_log_record.end(), _vector_log_records);
}

void server_system::service::Ninstance::set_crash_info(shared_buffer /*_buf_data*/)
{
}

void server_system::service::Ninstance::update_crash_info(shared_buffer /*_buf_data*/)
{
}

void get_parameter_service_setting(server_system::sSERVICE::sSETTING& _service_setting, context& _context_service);
void server_system::service::Ninstance::load_from(context _context)
{
	get_parameter_service_setting(this->info.setting, _context);
}

bool server_system::service::Ninstance::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option)
{
	// 1) get sender
	auto psender = this->m_psender;

	// check) 
	RETURN_IF(psender.empty(), false);

	// 2) send 
	return psender->process_send(std::move(_buffer), _count_message, _option);
}

//result_code server_system::service::Ninstance::process_message(CGDK::sMESSAGE& /*_msg*/)
//{
//	return eRESULT::BYPASS;
//}

object_ptr<server_system::host::Ninstance> _get_host_instance(server_system::iid_t _host_iid)
{
	// 1) get _service
	auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

	// check)
	RETURN_IF(pservice.empty(), {});

	// 2) get instance_manager
	auto phost_instance_manager = pservice->get_host_instance_manager();

	// check) register host instance
	RETURN_IF(phost_instance_manager.empty(), {});

	// 3) find & return
	return phost_instance_manager->find_instance(_host_iid);
}

void server_system::service::Ninstance::process_serialize_in(buffer_view& _buffer)
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		// 1) extract instance info
		auto temp_info = _buffer.extract<sSERVICE>();

		// 2) extract instance key
		auto temp_instance_key = _buffer.extract<server_system::sINSTANCE_KEY>();

		// declare)
		std::vector<server_system::factory_snap> temp_vector_executable;

		// 3) generate executable info
		for (auto& iter : temp_info.status.vector_factory)
		{
			if (iter.object_type == eOBJECT_TYPE::EXECUTABLE)
				temp_vector_executable.push_back(iter);
		}

		// 4) get host instance id	
		auto temp_host_instance_id = _buffer.extract<server_system::iid_t>();

		// 5) set
		this->info = std::move(temp_info);
		this->instance_key = temp_instance_key;
		this->host_instance_id = temp_host_instance_id;
		this->vector_executable = std::move(temp_vector_executable);
	}
}

void server_system::service::Ninstance::process_serialize_out(buffer& _buffer)
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		// 1) append service _info
		_buffer.append<server_system::sSERVICE>(this->info);

		// 2) append instance key
		_buffer.append<server_system::sINSTANCE_KEY>(this->instance_key);

		// 3) append host instance id
		_buffer.append<server_system::iid_t>(host_instance_id);
	}
}

std::size_t server_system::service::Ninstance::process_size_of() const
{
	SCOPED_LOCK(this->m_lockable_instance)
	{
		return get_size_of(this->info) + get_size_of(this->instance_key) + get_size_of(this->host_instance_id);
	}
}
