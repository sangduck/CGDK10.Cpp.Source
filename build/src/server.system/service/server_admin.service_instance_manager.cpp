#include "pch.h"

using namespace CGDK;

void server_system::service::manager::register_instance(object_ptr<server_system::service::Ninstance>&& _pservice_instance)
{
	// check)
	CGASSERT_ERROR(_pservice_instance.exist());

	// check)
	if(_pservice_instance.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_pservice_instance' is nullptr"sv);

	// check) don;t register if iid is zero
	if(_pservice_instance->get_iid() == 0)
		return;

	// declare) 
	auto pservice_instance = _pservice_instance;

	// declare)
	server_system::iid_t iid{0};

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) get iid
		iid = _pservice_instance->get_iid();

		// check)
		if(iid == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_service_instance' id is zero (2)"sv);

		// 2) is iid duplicated?
		auto iter_find = this->m_map_service_instances.find(iid);

		if (iter_find == this->m_map_service_instances.end())
		{
			// 3) on_register
			this->on_registering_instance(pservice_instance);

			try
			{
				// 4) set register time
				_pservice_instance->info.info.time_register = chrono::time::utc::now();
				_pservice_instance->info.info.time_update;		// - update time

				// 5) add
				this->m_map_service_instances.insert({ iid, std::move(_pservice_instance) });

				// 6) on_register
				this->on_register_instance(pservice_instance);
			}
			catch (...)
			{
				// comment) don;t care iid is zero, because it makes noting happened

				// - rollback
				this->m_map_service_instances.erase(iid);

				// reraise)
				throw;
			}
		}
		else if (iter_find->second->instance_key == _pservice_instance->instance_key)
		{
			// comment) 재접속한 것이므로  복구한다.
			
			// - log
			LOG_INFO << "service instance id(" << iid << ") restored";

			// - get previous instance info
			auto state_old = iter_find->second->info.status.state;
			auto state_new = _pservice_instance->info.status.state;

			// - restore instance info
			iter_find->second->info = pservice_instance->info;
			iter_find->second->info.info.time_update = chrono::time::utc::now();
			iter_find->second->instance_key = pservice_instance->instance_key;
			iter_find->second->set_sender(pservice_instance->get_sender());

			this->on_update_instance_info(pservice_instance);	
			if (state_old != state_new)
				this->update_instance_state(pservice_instance->get_iid(), state_old, state_new);
		}
		else
		{
			// comment) if iid is duplicated, it means that the instance is already registered. so, we can consider it as an error.

			// - log
			LOG_ERROR << "service instance id(" << iid << ") already exist";

			// - throw
			throw throwable::failure(eRESULT::ALREADY_EXIST, 0, CGDK::stream_string("'_service_instance' id(", iid, ") already exist"));
		}
	}
}

bool server_system::service::manager::unregister_instance(iid_t _iid) noexcept
{
	// check)
	if(_iid == 0)
		return false;

	// declare)
	object_ptr<service::Ninstance> pservice_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_service_instances.find(_iid);

		// check)
		if(iter_find == this->m_map_service_instances.end())
			return false;

		// 2) move
		pservice_instance = std::move(iter_find->second);

		// 3) erase
		this->m_map_service_instances.erase(iter_find);

		// 4) on erase
		this->on_unregister_instance(pservice_instance);
	}

	// return) 
	return true;
}

object_ptr<server_system::service::Ninstance> server_system::service::manager::find_instance(iid_t _iid) const
{
	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_service_instances.find(_iid);

		// check)
		if(iter_find == this->m_map_service_instances.end())
			return {};

		// return) 
		return iter_find->second;
	}
}

void server_system::service::manager::update_instance_state(iid_t _iid, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	// check)
	if(_iid == 0)
		return;

	// declare)
	object_ptr<service::Ninstance> pservice_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) finOPEN
		auto iter_find = this->m_map_service_instances.find(_iid);

		// check)
		if(iter_find == this->m_map_service_instances.end())
			return;

		// 2) move
		pservice_instance = iter_find->second;

		// 3) set info
		pservice_instance->info.status.state = _state_new;

		// 4) on erase
		this->on_update_instance_state(pservice_instance, _state_new, _state_old);
	}
}

void server_system::service::manager::update_instance_info(iid_t _iid, server_system::sSERVICE&& _service_info)
{
	// check)
	if(_iid == 0)
		return;

	// declare)
	object_ptr<service::Ninstance> pservice_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_service_instances.find(_iid);

		// check)
		if(iter_find == this->m_map_service_instances.end())
			return;

		// 2) move
		pservice_instance = iter_find->second;

		// check)
		CGASSERT_ERROR(_service_info.setting.rank.role == pservice_instance->info.setting.rank.role);

		// 3) set info
		pservice_instance->info = std::move(_service_info);

		// 4) on erase
		this->on_update_instance_info(pservice_instance);
	}
}

void server_system::service::manager::update_request_result(iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message)
{
	// declare)
	object_ptr<service::Ninstance> pservice_instance;

	SCOPED_LOCK(this->m_lockable_manager)
	{
		// 1) find
		auto iter_find = this->m_map_service_instances.find(_iid);

		// check)
		if (iter_find == this->m_map_service_instances.end())
			return;

		// 2) get
		pservice_instance = iter_find->second;
	}

	// 3) on request result
	this->on_request_result(pservice_instance, _request, _request_id, _result, _message);
}

result_code server_system::service::manager::_process_initializing(context& _context)
{
	// 1) on initialize
	this->on_initializing(_context);

	// return) 
	return result_code(eRESULT::SUCCESS);
}

result_code server_system::service::manager::_process_destroy() noexcept
{
	try 
	{
		// 1) on destroy
		this->on_destroy();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}
	
	return result_code(eRESULT::SUCCESS);
}

result_code server_system::service::manager::process_message(CGDK::sMESSAGE& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE,			process_message_SERVER_REQUEST_INIT_SERVICE,		sMESSAGE_NETWORK);
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE,		process_message_SERVER_REQUEST_START_SERVICE,		sMESSAGE_NETWORK);
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE,			process_message_SERVER_REQUEST_STOP_SERVICE,		sMESSAGE_NETWORK);
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE,		process_message_SERVER_REQUEST_DESTROY_SERVICE,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::BYPASS;
}

result_code server_system::service::manager::process_message_SERVER_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) get message buf
	//auto buf_recv = _msg.buf_message + offset(8);

	//// 2) get iid
	//auto iid = buf_recv.extract<iid_t>();

	//// 3) get 
	//auto instance = this->find_instance(iid);

	//// check)
	//if(instance.empty())
	//	return eRESULT::DONE;

	//// 4) start!
	////instance->

	// return) 
	return eRESULT::DONE;
}

result_code server_system::service::manager::process_message_SERVER_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//context context_start;
	//g_pservice_global->start(context_start);

	// return) 
	return eRESULT::DONE;
}

result_code server_system::service::manager::process_message_SERVER_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//g_pservice_global->stop();

	// return) 
	return eRESULT::DONE;
}

result_code server_system::service::manager::process_message_SERVER_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//g_pservice_global->destroy();

	// return) 
	return eRESULT::DONE;
}


void server_system::service::manager::process_serialize_in(buffer_view& _buffer)
{
	// declare) 
	std::map<iid_t, object_ptr<service::Ninstance>> map_service_instances;

	// 1) service instances
	{
		// - extract
		auto count = _buffer.extract<COUNT_T>();

		// - extract instances	
		for (; count > 0; --count)
		{
			// make 
			auto pservice_instance = this->process_alloc_instance();

			// - extract service_instance
			pservice_instance->serialize_in(_buffer);

			// - get iid
			auto iid = pservice_instance->get_iid();

			// check)
			CGASSERT_ERROR(iid != 0);

			// - insert
			map_service_instances.insert({ iid, std::move(pservice_instance) });
		}
	}

	// 2) apply result
	this->m_map_service_instances = std::move(map_service_instances);
}

void server_system::service::manager::process_serialize_out(buffer& _buffer)
{
	// 1) append count
	_buffer.append<COUNT_T>(static_cast<COUNT_T>(this->m_map_service_instances.size()));

	// 2) append instances
	for (const auto& iter : this->m_map_service_instances)
	{
		_buffer.append(iter.second);
	}
}

std::size_t server_system::service::manager::process_size_of() const
{
	// declare)
	size_t size = 0;

	// 1) add size of count
	size += sizeof(COUNT_T); // size of count

	// 2) add size of instances
	for (const auto& iter : this->m_map_service_instances)
	{
		size += get_size_of(iter.second);
	}

	// return) 
	return size;
}

object_ptr<server_system::service::Ninstance> server_system::service::manager::process_alloc_instance()
{
	return make_object<service::instance>();
}

server_system::iid_t _process_alloc_service_instance_iid()
{
	static std::atomic<uint32_t> s_iid = 0;

	return ++s_iid;
}

int32_t _process_alloc_service_instance_rank_index(int32_t _role)
{
	static std::map<int32_t,int32_t> s_map_index;
	static std::mutex s_cs_map_index;

	int32_t temp_index;

	SCOPED_LOCK(s_cs_map_index)
	{
		auto iter = s_map_index.find(_role);

		if (iter == s_map_index.end())
		{
			iter = s_map_index.insert({ _role , 0}).first;
		}

		temp_index = ++iter->second;
	}

	return temp_index;
}

server_system::sRANK _get_serice_rank()
{
	// 1) get service
	auto pservice = dynamic_pointer_cast<server_system::Xservice>(server_system::get_service());

	// check)
	if(pservice.empty())
		return {};

	// 2) get sercvice_instance
	auto pserivce_instance = pservice->get_service_instance();

	// check)
	if(pserivce_instance.empty())
		return {};

	// return)
	return pserivce_instance->info.setting.rank;
}

bool server_system::service::manager::process_confirm_instance(const object_ptr<server_system::service::Ninstance>& _pservice_instance)
{
	// declare)
	bool result = false;

	// 1) allocate iid
	if (_pservice_instance->info.setting.iid == IID_AUTO)
	{
		// declare)
		server_system::iid_t iid_new;

		while (true)
		{
			// - generate new iid
			iid_new = _process_alloc_service_instance_iid();

			// check) 중복 검사
			if (this->m_map_service_instances.find(iid_new) != this->m_map_service_instances.end())
				continue;

			// break
			break;
		}

		// - set new iid
		_pservice_instance->info.setting.iid = iid_new;

		// - set result true
		result = true;
	}

	// 2) get rank of service
	auto rank = _get_serice_rank();

	// 2) set rank.group
	if (_pservice_instance->info.setting.rank.group == RANK_AUTO)
	{
		_pservice_instance->info.setting.rank.group = rank.group;
	}

	// 3) set rank.field
	if (_pservice_instance->info.setting.rank.field == RANK_AUTO)
	{
		_pservice_instance->info.setting.rank.field = rank.field;
	}

	// 4) set rank.field
	if (_pservice_instance->info.setting.rank.index == RANK_AUTO)
	{
		while (true)
		{
			// - generate new iid
			rank.index = _process_alloc_service_instance_rank_index(_pservice_instance->info.setting.rank.role);

			// check) 중복 검사
			if (std::find_if(this->m_map_service_instances.begin(), this->m_map_service_instances.end(), [=](const auto& _iter) { return _iter.second->info.setting.rank == rank; }) != this->m_map_service_instances.end())
				continue;

			// break
			break;
		}

		// - set result true
		result = true;
	}

	// return)
	return result;
}
