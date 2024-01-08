#include "pch.h"

using namespace CGDK;


//-----------------------------------------------------------------------------
// factory
//-----------------------------------------------------------------------------
server_admin::factory_snap server_admin::get_snap(const Ifactory* _pfactory) noexcept
{
	// 1) get factory
	const auto& factory_statistics = _pfactory->statistics();

	// declare) 
	server_admin::factory_snap temp;

	temp.name = factory_statistics.name;
	temp.type = factory_statistics.type;
	temp.object_type = factory_statistics.object_type;
	temp.object_size = _pfactory->object_size();

	temp.updatable = get_update_snap(_pfactory);

	// return)
	return temp;
}

server_admin::factory_update_snap server_admin::get_update_snap(const Ifactory* _pfactory) noexcept
{
	// 1) get factory
	const auto& factory_statistics = _pfactory->statistics();

	// declare) 
	server_admin::factory_update_snap temp;

	// 2) set value
	temp.id = factory_statistics.id;
	temp.stacked = _pfactory->stacked_count();
	temp.now = factory_statistics.now;

	// return)
	return temp;
}

std::vector<server_admin::factory_snap> server_admin::get_snap(factory::manager* _pfactory_manager)
{
	// declare) 
	std::vector<server_admin::factory_snap> vector_result;

	scoped_lock(_pfactory_manager->get_lockable())
	{
		// - reserve
		vector_result.reserve(_pfactory_manager->size());

		// - gether snap
		auto iter = _pfactory_manager->begin();
		auto iter_end = _pfactory_manager->end();

		for (; iter != iter_end; ++iter)
		{
			vector_result.push_back(get_snap(*iter));
		}
	}

	// return)
	return vector_result;
}

std::vector<server_admin::factory_update_snap> server_admin::get_update_snap(factory::manager* _pfactory_manager)
{
	// declare) 
	std::vector<server_admin::factory_update_snap> vector_result;

	scoped_lock(_pfactory_manager->get_lockable())
	{
		// - reserve
		vector_result.reserve(_pfactory_manager->size());

		// - gether updatable snap
		auto iter = _pfactory_manager->begin();
		auto iter_end = _pfactory_manager->end();

		for (; iter != iter_end; ++iter)
		{
			vector_result.push_back(get_update_snap(*iter));
		}
	}

	// return)
	return vector_result;
}

std::vector<server_admin::factory_snap> server_admin::get_snap(factory::manager* _pfactory_manager, eOBJECT_TYPE _object_type)
{
	// declare) 
	std::vector<server_admin::factory_snap> vector_result;

	scoped_lock(_pfactory_manager->get_lockable())
	{
		// - reserve
		vector_result.reserve(_pfactory_manager->size());

		// - gether updatable snap
		auto iter = _pfactory_manager->begin();
		auto iter_end = _pfactory_manager->end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter)->statistics().object_type == _object_type)
			{
				vector_result.push_back(get_snap(*iter));
			}
		}
	}

	// return)
	return vector_result;
}

std::vector<server_admin::factory_update_snap> server_admin::get_update_snap(factory::manager* _pfactory_manager, eOBJECT_TYPE _object_type)
{
	// declare) 
	std::vector<server_admin::factory_update_snap> vector_result;

	scoped_lock(_pfactory_manager->get_lockable())
	{
		// - reserve
		vector_result.reserve(_pfactory_manager->size());

		// - gether updatable snap
		auto iter = _pfactory_manager->begin();
		auto iter_end = _pfactory_manager->end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter)->statistics().object_type == _object_type)
			{
				vector_result.push_back(get_update_snap(*iter));
			}
		}
	}

	// return)
	return vector_result;
}

//-----------------------------------------------------------------------------
// executor
//-----------------------------------------------------------------------------
server_admin::executor_snap server_admin::get_snap(Iexecutor* _pexector) noexcept
{
	// 1) get statistics
	const auto& statistics_executor = _pexector->statistics();

	// declare) 
	server_admin::executor_snap temp;

	// 2) copy 
	temp.name = statistics_executor.name;
	temp.type = statistics_executor.type;

	// 3) updatable
	temp.updatable = get_update_snap(_pexector);

	// return) 
	return temp;
}

server_admin::executor_update_snap server_admin::get_update_snap(Iexecutor* _pexector) noexcept
{
	// 1) get statistics
	const auto& statistics_executor = _pexector->statistics();

	// declare) 
	server_admin::executor_update_snap temp;

	// 2) copy 
	temp.id = statistics_executor.id;
	temp.count_execute = statistics_executor.now.count_execute.load();

	// 3) get snap
	scoped_lock(_pexector->get_lockable())
	{
		auto iter = _pexector->begin();
		auto iter_end = _pexector->end();

		for (; iter != iter_end; ++iter)
		{
			temp.vector_threads.push_back(get_snap(*iter));
		}
	}

	// return) 
	return temp;
}

std::vector<server_admin::executor_snap> server_admin::get_snap(executor::manager* _pexecutor_manager)
{
	// declare) 
	std::vector<server_admin::executor_snap> vector_result;

	// locked) 
	{
		// scoped_lock)
		std::lock_guard<std::shared_mutex> cs(_pexecutor_manager->get_lockable());

		// 1) reserve
		vector_result.reserve(_pexecutor_manager->size());

		// 2) 
		auto iter = _pexecutor_manager->begin();
		auto iter_end = _pexecutor_manager->end();

		for (; iter != iter_end; ++iter)
		{
			vector_result.push_back(get_snap(*iter));
		}
	}

	// return)
	return vector_result;
}

std::vector<server_admin::executor_update_snap> server_admin::get_update_snap(executor::manager* _pexecutor_manager)
{
	// declare) 
	std::vector<server_admin::executor_update_snap> vector_result;

	// locked) 
	{
		// scoped_lock)
		std::lock_guard<std::shared_mutex> cs(_pexecutor_manager->get_lockable());

		// 1) reserve
		vector_result.reserve(_pexecutor_manager->size());

		// 2) ...
		auto iter = _pexecutor_manager->begin();
		auto iter_end = _pexecutor_manager->end();

		for (; iter != iter_end; ++iter)
		{
			vector_result.push_back(get_update_snap(*iter));
		}
	}

	// return)
	return vector_result;
}

//-----------------------------------------------------------------------------
// thread
//-----------------------------------------------------------------------------
std::vector<server_admin::unit_thread> server_admin::get_snap(api::manager_thread* _pthread_manager)
{
	// declare)
	std::vector<server_admin::unit_thread> vector_statistics;

	// locked) 
	{
		// scoped_lock)
		std::lock_guard<std::shared_mutex> cs(_pthread_manager->get_lockable());

		// 1) reserve
		vector_statistics.reserve(_pthread_manager->size());

		// 2) ...
		auto iter = _pthread_manager->begin();
		auto iter_end = _pthread_manager->end();

		for (; iter != iter_end; ++iter)
		{
			vector_statistics.emplace_back(get_snap(*iter));
		}
	}

	// return)
	return vector_statistics;
}

server_admin::unit_thread server_admin::get_snap(api::Nthread* _pthread) noexcept
{
	// 1) get statistics
	const auto& statistics_thread = _pthread->statistics();

	// declare) 
	server_admin::unit_thread temp;

	// 2) copy
#if defined(_WIN32)
	temp.id_thread = reinterpret_cast<uint64_t>(statistics_thread.id_thread);
#else
	temp.id_thread = static_cast<uint64_t>(statistics_thread.id_thread);
#endif
	temp.count_executed = statistics_thread.count_executed;
	temp.count_error = statistics_thread.count_error;
	temp.tick_last_execute = statistics_thread.tick_last_execute;
	temp.is_executing = statistics_thread.is_executing;

	// return) 
	return temp;
}

//-----------------------------------------------------------------------------
// schedulable
//-----------------------------------------------------------------------------
server_admin::schedulable_snap server_admin::get_snap(const Ischedulable* _pschedulable) noexcept
{
	// check)
	CGASSERT_ERROR(_pschedulable != nullptr);

	// declare) 
	server_admin::schedulable_snap temp;

	// check)
	RETURN_IF(_pschedulable == nullptr, temp);

	// 1) name
	auto pnameable = dynamic_cast<const object::Inameable*>(_pschedulable);
	if (pnameable != nullptr)
	{
		temp.name = pnameable->get_name();
	}
	if (temp.name.empty() && _pschedulable->get_factory() != nullptr)
	{
		temp.name = _pschedulable->get_factory()->get_name();
	}

	// 2) id/type
	temp.type = _pschedulable->statistics().type;

	// 3) updatable
	temp.updatable = get_update_snap(_pschedulable);

	// return)
	return temp;
}

server_admin::schedulable_update_snap server_admin::get_update_snap(const Ischedulable* _pschedulable) noexcept
{
	// check)
	CGASSERT_ERROR(_pschedulable != nullptr);

	// 1) get statistics
	const auto& statistics_schedulable = _pschedulable->statistics();

	// declare) 
	server_admin::schedulable_update_snap temp;

	// check)
	RETURN_IF(_pschedulable == nullptr, temp);

	// 1) id/type
	temp.id = statistics_schedulable.id;

	// 2) schedulable
	auto pNschedulable = dynamic_cast<const Nschedulable*>(_pschedulable);
	if (pNschedulable != nullptr)
		temp.duration_interval = pNschedulable->execute_interval();

	// 3) next tick
	temp.tick_next = _pschedulable->next_tick();

	// 4) executed
	temp.count_execute = statistics_schedulable.count_execute;

	// return)
	return temp;
}

std::vector<server_admin::schedulable_snap> server_admin::get_snap(schedulable::manager* _pschedulable_manager)
{
	// declare)
	std::vector<server_admin::schedulable_snap> vector_temp;

	scoped_lock(_pschedulable_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pschedulable_manager->size());

		// 2) add all 
		auto iter = _pschedulable_manager->begin();
		auto iter_end = _pschedulable_manager->end();
		for (; iter != iter_end; ++iter)
		{
			vector_temp.push_back(get_snap(*iter));
		}
	}

	// return)
	return vector_temp;
}

std::vector<server_admin::schedulable_update_snap> server_admin::get_update_snap(schedulable::manager* _pschedulable_manager)
{
	// declare)
	std::vector<server_admin::schedulable_update_snap> vector_temp;

	scoped_lock(_pschedulable_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pschedulable_manager->size());

		// 2) get all 
		auto iter = _pschedulable_manager->begin();
		auto iter_end = _pschedulable_manager->end();
		for (; iter != iter_end; ++iter)
		{
			vector_temp.push_back(get_update_snap(*iter));
		}
	}

	// return)
	return vector_temp;
}

//-----------------------------------------------------------------------------
// connective_info
//-----------------------------------------------------------------------------
server_admin::connective_status_snap _get_status(const net::io::Iconnective* _pconnective) noexcept
{
	// check)
	CGASSERT_ERROR(_pconnective != nullptr);

	// declare)
	server_admin::connective_status_snap temp;

	// check)
	RETURN_IF(_pconnective == nullptr, temp);

	temp.id = _pconnective->get_connective_id();
	temp.time_start = _pconnective->statistics_get_start_time();
	temp.time_stop = _pconnective->statistics_get_stop_time();

	auto pacceptor = dynamic_cast<const net::io::connective::Iacceptor*>(_pconnective);

	if (pacceptor != nullptr)
	{
		temp.addr_local = pacceptor->local_endpoint();
		temp.state = pacceptor->socket_state();
	}
	else
	{
		__zero_memory(&temp.addr_local, sizeof(net::sockaddr));
		temp.state = eSOCKET_STATE::CLOSED;
	}

	temp.is_enable = _pconnective->is_enable();

	// return)
	return temp;
}

statistics::unit_connective_snap server_admin::get_snap(const statistics::unit_connective& _unit_connective) noexcept
{
	// declare)
	statistics::unit_connective_snap temp;

	temp.tick_snap = _unit_connective.tick_snap;

	temp.count_try = _unit_connective.count_try;
	temp.count_success_connect = _unit_connective.count_success_connect;
	temp.count_fail_connect = _unit_connective.count_fail_connect;
	temp.count_disconnect = _unit_connective.count_disconnect;
	temp.count_error_disconnect = _unit_connective.count_error_disconnect;
	temp.count_keep = _unit_connective.count_keep;

	temp.count_async = _unit_connective.count_async;

	// return) 
	return temp;
}

statistics::unit_connective_snap server_admin::get_update_snap(const statistics::unit_connective& _connective) noexcept
{
	// declare)
	statistics::unit_connective_snap temp;

	// 1) 
	temp = _connective;

	// return)
	return temp;
}

server_admin::connective_info_snap server_admin::get_snap(const net::io::Iconnective* _pconnective) noexcept
{
	// declare)
	server_admin::connective_info_snap temp;

	// 1) set name
	temp.name = _pconnective->get_name();
	temp.time_create = _pconnective->statistics_get_create_time();

	// 2) get status
	temp.status = _get_status(_pconnective);

	// 3) get connective_info_snap
	temp.now = get_snap(_pconnective->statistics_get_connection_info());

	// return)
	return temp;
}

server_admin::connective_info_update_snap server_admin::get_update_snap(const net::io::Iconnective* _pconnective) noexcept
{
	// declare)
	server_admin::connective_info_update_snap temp;

	// 1) get status
	temp.status = _get_status(_pconnective);

	// 2) get connective_info_snap
	temp.now = get_update_snap(_pconnective->statistics_get_connection_info());

	// return)
	return temp;
}

std::vector<server_admin::connective_info_snap> server_admin::get_snap(net::io::connective::manager* _pconnective_manager)
{
	// declare)
	std::vector<server_admin::connective_info_snap> vector_temp;

	scoped_lock(_pconnective_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pconnective_manager->size());

		// 2) get all 
		for (auto& iter : *_pconnective_manager)
		{
			vector_temp.push_back(get_snap(iter));
		}
	}

	// return)
	return vector_temp;
}

std::vector<server_admin::connective_info_update_snap> server_admin::get_update_snap(net::io::connective::manager* _pconnective_manager)
{
	// declare)
	std::vector<server_admin::connective_info_update_snap> vector_temp;

	scoped_lock(_pconnective_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pconnective_manager->size());

		// 2) get all 
		for (auto& iter : *_pconnective_manager)
		{
			vector_temp.push_back(get_update_snap(iter));
		}
	}

	// return)
	return vector_temp;
}

//-----------------------------------------------------------------------------
// traffic
//-----------------------------------------------------------------------------
statistics::unit_traffic_snap server_admin::get_snap(const statistics::unit_traffic& _traffic_info) noexcept
{
	// declare)
	statistics::unit_traffic_snap snap_now;

	// 1) set total
	snap_now = _traffic_info;

	// return) 
	return snap_now;
}

statistics::unit_traffic_snap server_admin::get_update_snap(const statistics::unit_traffic& _traffic_info) noexcept
{
	return get_snap(_traffic_info);
}

//-----------------------------------------------------------------------------
// socket_info
//-----------------------------------------------------------------------------
server_admin::socket_info_snap server_admin::get_snap(const net::io::Ibinder* _pbinder) noexcept
{
	// declare) 
	server_admin::socket_info_snap temp;

	auto* pnameable = dynamic_cast<const object::Inameable*>(_pbinder);
	if (pnameable != nullptr)
	{
		temp.name = pnameable->get_name();
	}

	auto tick_now = chrono::tick::now();

	auto* psocket = dynamic_cast<const net::io::socket::Nbase*>(_pbinder);
	if (psocket != nullptr)
	{
		temp.updatable.id = reinterpret_cast<uint64_t>(_pbinder);

		temp.updatable.time_bind = psocket->statistics_get_connect_time();
		temp.updatable.time_connect = psocket->statistics_get_connect_time();
		temp.updatable.tick_send = tick_now - psocket->statistics_get_tick_last_send();
		temp.updatable.tick_receive = tick_now - psocket->statistics_get_tick_last_receive();

		temp.updatable.addr_local = psocket->local_endpoint();
		temp.updatable.addr_remote = psocket->remote_endpoint();

		temp.updatable.state = psocket->socket_state();
		temp.updatable.flag_socket = ((psocket->address_family() == AF_INET6) ? 0x01 : 0x00)
			  					   | ((psocket->statistics_get_protocol() == IPPROTO_TCP) ? 0x02 : 0x00);
		temp.updatable.traffic = psocket->statistics_get_traffic_info();
	}

	// return) 
	return temp;
}

server_admin::socket_info_snap server_admin::get_snap(const net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket) noexcept
{
	// declare) 
	server_admin::socket_info_snap temp;

	auto tick_now = chrono::tick::now();

	temp.updatable.id = reinterpret_cast<uint64_t>(_pbinder);
	temp.updatable.time_bind = _pstatistics_socket->statistics_get_connect_time();
	temp.updatable.time_connect = _pstatistics_socket->statistics_get_connect_time();
	temp.updatable.tick_send = tick_now - _pstatistics_socket->statistics_get_tick_last_send();
	temp.updatable.tick_receive = tick_now - _pstatistics_socket->statistics_get_tick_last_receive();
	//temp.updatable.addr_local = _pstatistics_socket->local_endpoint();
	//temp.updatable.addr_remote = _pstatistics_socket->remote_endpoint();
	temp.updatable.state = eSOCKET_STATE::CLOSED;
	temp.updatable.flag_socket = 0;
	temp.updatable.traffic = _pstatistics_socket->statistics_get_traffic_info();

	// return) 
	return temp;
}

server_admin::socket_info_update_snap server_admin::get_update_snap(const net::io::Ibinder* _pbinder) noexcept
{
	// declare) 
	server_admin::socket_info_update_snap temp;

	auto tick_now = chrono::tick::now();

	auto* psocket = dynamic_cast<const net::io::socket::Nbase*>(_pbinder);
	if (psocket != nullptr)
	{
		temp.id = reinterpret_cast<uint64_t>(_pbinder);

		temp.time_bind = psocket->statistics_get_connect_time();
		temp.time_connect = psocket->statistics_get_connect_time();
		temp.tick_send = tick_now - psocket->statistics_get_tick_last_send();
		temp.tick_receive = tick_now - psocket->statistics_get_tick_last_receive();

		temp.addr_local = psocket->local_endpoint();
		temp.addr_remote = psocket->remote_endpoint();

		temp.state = psocket->socket_state();
		temp.flag_socket = ((psocket->address_family() == AF_INET6) ? 0x01 : 0x00)
			  		     | ((psocket->statistics_get_protocol() == IPPROTO_TCP) ? 0x02 : 0x00);
		temp.traffic = psocket->statistics_get_traffic_info();
	}

	// return) 
	return temp;
}

std::vector<server_admin::socket_info_snap> server_admin::get_snap(net::io::binder::manager* _pbinder_manager)
{
	// declare)
	std::vector<server_admin::socket_info_snap> vector_temp;

	scoped_lock(_pbinder_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pbinder_manager->size());

		// 2) get all 
		for (auto& iter : *_pbinder_manager)
		{
			vector_temp.push_back(get_snap(iter));
		}
	}

	// return)
	return vector_temp;
}

std::vector<server_admin::socket_info_update_snap> server_admin::get_update_snap(net::io::binder::manager* _pbinder_manager)
{
	// declare)
	std::vector<server_admin::socket_info_update_snap> vector_temp;

	scoped_lock(_pbinder_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pbinder_manager->size());

		// 2) get all 
		for (auto& iter : *_pbinder_manager)
		{
			vector_temp.push_back(get_update_snap(iter));
		}
	}

	// return)
	return vector_temp;
}

//-----------------------------------------------------------------------------
// time_event
//-----------------------------------------------------------------------------
server_admin::time_event_snap server_admin::get_snap(const time_event::Ientity* _ptime_event) noexcept
{
	// declare) 
	server_admin::time_event_snap data_snap;

	data_snap.updatable.id = _ptime_event->entity_status().id;
	data_snap.updatable.entity_setting = _ptime_event->entity_setting();
	data_snap.updatable.entity_status = _ptime_event->entity_status();

	// eturn) 
	return data_snap;
}

server_admin::time_event_update_snap server_admin::get_update_snap(const time_event::Ientity* _ptime_event) noexcept
{
	// declare) 
	server_admin::time_event_update_snap data_update_snap;

	data_update_snap.id = _ptime_event->entity_status().id;
	data_update_snap.entity_setting = _ptime_event->entity_setting();
	data_update_snap.entity_status = _ptime_event->entity_status();

	// eturn) 
	return data_update_snap;
}

std::vector<server_admin::time_event_snap> server_admin::get_snap(time_event::entity_manager* _pentity_manager)
{
	// declare)
	std::vector<server_admin::time_event_snap> vector_temp;

	scoped_lock(_pentity_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pentity_manager->size());

		// 2) get all 
		for (auto& iter: *_pentity_manager)
		{
			vector_temp.push_back(get_snap(iter));
		}
	}

	// return)
	return vector_temp;
}

std::vector<server_admin::time_event_update_snap> server_admin::get_update_snap(time_event::entity_manager* _pentity_manager)
{
	// declare)
	std::vector<time_event_update_snap> vector_temp;

	scoped_lock(_pentity_manager->get_lockable())
	{
		// 1) reserve
		vector_temp.reserve(_pentity_manager->size());

		// 2) get all 
		for (auto& iter: *_pentity_manager)
		{
			vector_temp.push_back(get_update_snap(iter));
		}
	}

	// return)
	return vector_temp;
}
