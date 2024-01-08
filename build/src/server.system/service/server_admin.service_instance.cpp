#include "pch.h"

using namespace CGDK;


void server_admin::sINSTANCE_SERVICE::set_intance(server_admin::sINSTANCE_SERVICE&& _instance_service)
{
	this->info = std::move(_instance_service.info);
	this->status = std::move(_instance_service.status);
}

void server_admin::sINSTANCE_SERVICE::set_intance_setting(server_admin::sSERVICE_SETTING&& _servic_setting)
{
	this->setting = std::move(_servic_setting);
}

void server_admin::sINSTANCE_SERVICE::update_intance_status(server_admin::sSERVICE_INSTANCE_STATUS_UPDATE&& _service_instance_info_update)
{
	this->status.vector_process_status.push_back(std::move(_service_instance_info_update.process_status));
	this->update_factory_info(std::move(_service_instance_info_update.vector_factory_update));
	this->update_executor_info(std::move(_service_instance_info_update.vector_executor_update));
	this->update_schedulable_info(std::move(_service_instance_info_update.vector_schedulable_update));
	this->update_connective_info(std::move(_service_instance_info_update.vector_connective_update));
	this->update_binder_info(std::move(_service_instance_info_update.vector_binder_update));
	this->update_connective_statistics(_service_instance_info_update.connective_info);
	this->update_traffic_statistics(_service_instance_info_update.traffic_info);
}

void server_admin::sINSTANCE_SERVICE::register_factory_info(server_admin::factory_snap&& _data)
{
	// 1) add
	if (_data.object_type == eOBJECT_TYPE::EXECUTABLE)
	{
		this->vector_executable.push_back(_data);
	}

	// 2) add
	this->status.vector_factory.push_back(std::move(_data));
}

void server_admin::sINSTANCE_SERVICE::unregister_factory_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->status.vector_factory.begin(), this->status.vector_factory.end(), [=](const server_admin::factory_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->status.vector_factory.end());

	// check) 
	RETURN_IF(iter_find == this->status.vector_factory.end());

	// 2) add
	if (iter_find->object_type == eOBJECT_TYPE::EXECUTABLE)
	{
		// - find
		auto iter_find_nested = std::find_if(this->vector_executable.begin(), this->vector_executable.end(), [=](const server_admin::factory_snap& _iter) { return _iter.updatable.id == _index; });

		// check)
		CGASSERT_ERROR(iter_find_nested != vector_executable.end());

		// - erase
		this->vector_executable.erase(iter_find_nested);
	}

	// 3) erase
	this->status.vector_factory.erase(iter_find);
}

void server_admin::sINSTANCE_SERVICE::update_factory_info(std::vector<server_admin::factory_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->status.vector_factory.size() != _data.size(), );

	// declare)
	auto iter_factory = this->status.vector_factory.begin();
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

void server_admin::sINSTANCE_SERVICE::register_executor_info(server_admin::executor_snap&& _data)
{
	// 1) insert 
	this->status.vector_executor.push_back(std::move(_data));
}

void server_admin::sINSTANCE_SERVICE::unregister_executor_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->status.vector_executor.begin(), this->status.vector_executor.end(), [=](const server_admin::executor_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->status.vector_executor.end());

	// check) 
	RETURN_IF(iter_find == this->status.vector_executor.end());

	// 2) erase
	this->status.vector_executor.erase(iter_find);
}

void server_admin::sINSTANCE_SERVICE::update_executor_info(std::vector<server_admin::executor_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->status.vector_executor.size() != _data.size(),);

	// declare)
	auto iter_dest = this->status.vector_executor.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->updatable = *iter_src;
	}
}

void server_admin::sINSTANCE_SERVICE::update_schedulable_info(std::vector<server_admin::schedulable_update_snap>&& _data)
{
	// declare)
	std::vector<server_admin::schedulable_snap> vector_deleted;
	std::vector<server_admin::schedulable_update_snap> vector_new;

	// 1) get added/deleted schedulable
	auto iter_dest = this->status.vector_schedulable.begin();
	auto iter_dest_end = this->status.vector_schedulable.end();
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
	CGASSERT_ERROR(_data.size() + vector_deleted.size() == this->status.vector_schedulable.size() + vector_new.size())
}

void server_admin::sINSTANCE_SERVICE::register_connective_info(server_admin::connective_info_snap&& _data)
{
	// 1) insert 
	this->status.vector_connective.push_back(std::move(_data));
}

void server_admin::sINSTANCE_SERVICE::unregister_connective_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->status.vector_connective.begin(), this->status.vector_connective.end(), [=](const server_admin::connective_info_snap& _iter) { return _iter.index == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->status.vector_connective.end());

	// check) 
	RETURN_IF(iter_find == this->status.vector_connective.end());

	// 2) erase
	this->status.vector_connective.erase(iter_find);
}

void server_admin::sINSTANCE_SERVICE::update_connective_info(std::vector<server_admin::connective_info_update_snap>&& _vector_data)
{
	// check)
	RETURN_IF(this->status.vector_connective.size() != _vector_data.size(),);

	// declare)
	auto iter_dest = this->status.vector_connective.begin();
	auto iter_src = _vector_data.begin();
	auto iter_src_end = _vector_data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->status = iter_src->status;
		iter_dest->now = iter_src->now;
	}
}

void server_admin::sINSTANCE_SERVICE::register_binder_info(server_admin::socket_info_snap&& _data)
{
	this->status.vector_binder.push_back(std::move(_data));
}

void server_admin::sINSTANCE_SERVICE::unregister_binder_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->status.vector_binder.begin(), this->status.vector_binder.end(), [=](const server_admin::socket_info_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->status.vector_binder.end());

	// check) 
	RETURN_IF(iter_find == this->status.vector_binder.end());

	// 2) erase
	this->status.vector_binder.erase(iter_find);
}

void server_admin::sINSTANCE_SERVICE::update_binder_info(std::vector<server_admin::socket_info_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->status.vector_binder.size() != _data.size(),);

	// declare)
	auto iter_dest = this->status.vector_binder.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->updatable = std::move(*iter_src);
	}
}

void server_admin::sINSTANCE_SERVICE::register_time_event_info(server_admin::time_event_snap&& _data)
{
	// 1) insert 
	this->status.vector_time_event.push_back(std::move(_data));
}

void server_admin::sINSTANCE_SERVICE::unregister_time_event_info(uint64_t _index)
{
	// 1) find
	auto iter_find = std::find_if(this->status.vector_time_event.begin(), this->status.vector_time_event.end(), [=](const server_admin::time_event_snap& _iter) { return _iter.updatable.id == _index; });

	// check)
	CGASSERT_ERROR(iter_find != this->status.vector_time_event.end());

	// check) 
	RETURN_IF(iter_find == this->status.vector_time_event.end());

	// 2) erase
	this->status.vector_time_event.erase(iter_find);
}

void server_admin::sINSTANCE_SERVICE::update_time_event_info(std::vector<server_admin::time_event_update_snap>&& _data)
{
	// check)
	RETURN_IF(this->status.vector_time_event.size() != _data.size(),);

	// declare)
	auto iter_dest = this->status.vector_time_event.begin();
	auto iter_src = _data.begin();
	auto iter_src_end = _data.end();

	// 1) set...
	for (; iter_src != iter_src_end; ++iter_src, ++iter_dest)
	{
		iter_dest->updatable = *iter_src;
	}
}

void server_admin::sINSTANCE_SERVICE::update_connective_statistics(const statistics::unit_connective_snap& _data)
{
	this->status.connective_info = _data;
}

void server_admin::sINSTANCE_SERVICE::update_traffic_statistics(const statistics::unit_traffic_snap& _data)
{
	this->status.traffic_info = _data;
}

void server_admin::sINSTANCE_SERVICE::add_log_records(circular_list<object_ptr<LOG_RECORD>>&& _vector_log_records) noexcept
{
	this->status.vector_log_record.splice(this->status.vector_log_record.end(), _vector_log_records);
}

void server_admin::sINSTANCE_SERVICE::set_crash_info(shared_buffer /*_buf_data*/)
{
}

void server_admin::sINSTANCE_SERVICE::update_crash_info(shared_buffer /*_buf_data*/)
{
}

void server_admin::sINSTANCE_SERVICE::process_serialize_in(buffer_view& _buffer)
{
	this->service_type = _buffer.extract<int>();
	this->setting = _buffer.extract<sSERVICE_SETTING>();
	this->info = _buffer.extract<sSERVICE_INSTANCE_INFO>();
	this->status = _buffer.extract<sSERVICE_INSTANCE_STATUS>();

	// check)
	CGASSERT_ERROR(this->service_type == eINSTANCE_TYPE::SERVICE);

	for (auto& iter : this->status.vector_factory)
	{
		if (iter.object_type == eOBJECT_TYPE::EXECUTABLE)
		{
			this->vector_executable.push_back(iter);
		}
	}
}

void server_admin::sINSTANCE_SERVICE::process_serialize_out(buffer& _buffer)
{
	_buffer.append(this->service_type);
	_buffer.append(this->setting);
	_buffer.append(this->info);
	_buffer.append(this->status);
}

std::size_t server_admin::sINSTANCE_SERVICE::process_size_of() const
{
	return get_size_of(this->service_type) + get_size_of(this->setting) + get_size_of(this->info) + get_size_of(this->status);
}
