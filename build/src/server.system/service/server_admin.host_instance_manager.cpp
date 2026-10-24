#include "pch.h"

using namespace CGDK;

void server_system::host::manager::register_instance(object_ptr<host::Ninstance>&& _phost_instance)
{
	// check)
	CGASSERT_ERROR(_phost_instance.exist());

	// check)
	THROW_IF(_phost_instance.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_phost_instance' is nullptr"sv));

	// 1) get iid
	auto iid = _phost_instance->get_iid();

	// check)
	RETURN_IF(iid == 0, );

	// declare) 
	auto phost_instance = _phost_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 2) find
		auto iter_find = this->m_map_host_instances.find(iid);

		// check)
		if (iter_find == this->m_map_host_instances.end())
		{
			// 3) ...
			this->on_registering_instance(phost_instance);

			// 4) add
			this->m_map_host_instances.insert({ iid, std::move(_phost_instance) });

			// 5) call 'on_register'
			this->on_register_instance(phost_instance);
		}
		else if (iter_find->second->instance_key == phost_instance->instance_key)
		{
			// comment) 재접속한 것이므로  복구한다.

			// - log
			LOG_INFO << "host instance id(" << iid << ") restored";
			// - restore instance info
			iter_find->second->info = phost_instance->info;
			iter_find->second->instance_key = phost_instance->instance_key;
			iter_find->second->set_sender(phost_instance->get_sender());
		}
		else
		{
			// comment) if iid is duplicated, it means that the instance is already registered. so, we can consider it as an error.

			// - log	
			LOG_ERROR << "service instance id(" << iid << ") already exist";

			// - throw
			throw throwable::failure(eRESULT::ALREADY_EXIST, 0, CGDK::stream_string("'_host_instance' id(", iid, ") already exist"));
		}
	}
}

bool server_system::host::manager::unregister_instance(iid_t _iid) noexcept
{
	// check)
	CGASSERT_ERROR(_iid != 0);

	// check)
	RETURN_IF(_iid == 0, false);

	// declare)
	object_ptr<host::Ninstance> phost_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_host_instances.find(_iid);

		// check)
		RETURN_IF(iter_find == this->m_map_host_instances.end(), false);

		// 2) move
		phost_instance = std::move(iter_find->second);

		// 3) erase
		this->m_map_host_instances.erase(iter_find);

		// 4) 
		this->on_unregister_instance(phost_instance);
	}

	// return) 
	return true;
}

object_ptr<server_system::host::Ninstance> server_system::host::manager::find_instance(iid_t _iid) const noexcept
{
	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_host_instances.find(_iid);

		// check)
		RETURN_IF(iter_find == this->m_map_host_instances.end(), {});

		// return) 
		return iter_find->second;
	}
}

result_code server_system::host::manager::_process_initializing(context& _context)
{
	// 1) create group socket
	auto pgroup_socket = this->process_alloc_group_socket(_context);

	// 2) enable enter
	//pgroup_socket->initialize(_context);
	pgroup_socket->enable_member_enter();

	// 3) set group socket
	this->m_pgroup_socket = std::move(pgroup_socket);

	// 4) on initialize
	this->on_initializing(_context);

	// return) 
	return result_code(eRESULT::SUCCESS);
}

result_code server_system::host::manager::_process_destroy() noexcept
{
	try 
	{
		// 1) on destroy
		this->on_destroy();

		// 2) destroy
		own_ptr<server_system::host::group_socket> pgroup_socket = std::move(this->m_pgroup_socket);

		//pgroup_socket->destroy();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}
	
	return result_code(eRESULT::SUCCESS);
}

result_code server_system::host::manager::process_message(CGDK::sMESSAGE& /*_msg*/)
{
	return eRESULT::BYPASS;
}

void server_system::host::manager::process_serialize_in(buffer_view & _buffer)
{
	// declare) 
	std::map<iid_t, object_ptr<host::Ninstance>> map_host_instances;

	// 1) host instances
	{
		// - extract
		auto count = _buffer.extract<COUNT_T>();

		// - extract instances	
		for (; count > 0; --count)
		{
			// - alloc instance
			auto phost_instance = this->process_alloc_instance();

			// - extract host_instance
			phost_instance->serialize_in(_buffer);

			// - get iid
			auto iid = phost_instance->get_iid();

			// check)
			CGASSERT_ERROR(iid != 0);

			// - insert
			map_host_instances.insert({ iid, std::move(phost_instance) });
		}
	}

	// 2) apply result
	this->m_map_host_instances = std::move(map_host_instances);
}

void server_system::host::manager::process_serialize_out(buffer& _buffer)
{
	// 1) append count
	_buffer.append<COUNT_T>(static_cast<COUNT_T>(this->m_map_host_instances.size()));

	// 2) append instances
	for (const auto& iter : this->m_map_host_instances)
	{
		_buffer.append(iter.second);
	}
}

std::size_t server_system::host::manager::process_size_of() const
{
	// declare)
	size_t size = 0;

	// 1) add size of count
	size += sizeof(COUNT_T); // size of count

	// 2) add size of instances
	for (const auto& iter : this->m_map_host_instances)
	{
		size += get_size_of(iter.second);
	}

	return size;
}

object_ptr<server_system::host::Ninstance> server_system::host::manager::process_alloc_instance()
{
	return make_object<host::instance>();
}

own_ptr<server_system::host::group_socket> server_system::host::manager::process_alloc_group_socket(context& /*_context*/)
{
	return make_own<host::group_socket>();
}

server_system::iid_t _process_alloc_host_instance_iid()
{
	static std::atomic<uint32_t> s_iid = 0;

	return ++s_iid;
}

bool server_system::host::manager::process_confirm_instance(const object_ptr<Ninstance>& _phost_instance)
{
	// declare)
	bool result = false;

	// - reallocation host instance serial
	if (_phost_instance->info.setting.iid == server_system::IID_AUTO)
	{
		// - generate new iid
		server_system::iid_t iid_new = _process_alloc_host_instance_iid();

		// - set new iid
		_phost_instance->info.setting.iid = iid_new;

		// - set result true
		result = true;
	}

	// return)
	return result;
}
