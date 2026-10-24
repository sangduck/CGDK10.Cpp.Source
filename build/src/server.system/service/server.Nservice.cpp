#include "pch.h"

using namespace CGDK;

namespace CGDK
{
	extern lock_object g_lockable_pservice_global;
	extern object_ptr<server_system::Nservice> g_pservice_global;
}

void get_parameter_service_setting(server_system::sSERVICE::sSETTING& _service_setting, context& _context_service);

object_ptr<server_system::Nservice> CGDK::server_system::get_service() noexcept
{
	std::lock_guard lock(g_lockable_pservice_global);

	return g_pservice_global;
}

server_system::Nservice::Nservice() noexcept
{
}

server_system::Nservice::Nservice(const std::string_view _name) noexcept
{
	// 1) set name
	this->process_set_name(_name);
}

server_system::Nservice::~Nservice() noexcept
{
}

void server_system::Nservice::host_instance(object_ptr<server_system::host::instance_local>&& _phost_instance)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// check) already exist
		CGASSERT_ERROR(this->m_phost_instance.empty());

		// - set instance
		this->m_phost_instance = std::move(_phost_instance);
	}
}


void server_system::Nservice::set_service_instance(own_ptr<server_system::service::instance_local>&& _pservice_instance)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// check) already exist
		CGASSERT_ERROR(this->m_pservice_instance.empty());

		// - set instance
		this->m_pservice_instance = std::move(_pservice_instance);
	}
}

bool server_system::Nservice::wait_destroy(const chrono::tick::duration& _wait)
{
	CONDITIONAL_LOCK(this->m_lockable_destroy, cs)
	{
		// check)
		RETURN_IF(this->get_object_state() == eOBJECT_STATE::NONE || this->get_object_state() == eOBJECT_STATE::DESTROYING_PENDING, true);

		if (_wait != chrono::tick::duration::max())
		{
			// 1) wait
			auto result = this->m_cv_destroy.wait_for(cs, _wait);

			// check)
			RETURN_IF(result == std::cv_status::timeout, false);

			// check) 
			RETURN_IF(this->get_object_state() != eOBJECT_STATE::NONE && this->get_object_state() != eOBJECT_STATE::DESTROYING_PENDING, false);
		}
		else
		{
			// 1) wait for infinity
			this->m_cv_destroy.wait(cs);
		}
	}

	// return)
	return true;
}

void server_system::Nservice::on_initialize_setting(context& /*_context*/)
{
}

void server_system::Nservice::on_start_setting(context& _context)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		if (this->m_pservice_instance.exist())
		{
			if (this->m_pservice_instance->info.setting.context_start.is_valid())
			{
				_context = this->m_pservice_instance->info.setting.context_start;
			}
		}
	}
}

result_code server_system::Nservice::process_initialize(context& _context)
{
	// declare) 
	result_code result;

	try
	{
		// declare) 
		object_ptr<server_system::Nservice> pservice_old;

		// 1) get old global service
		SCOPED_LOCK(g_lockable_pservice_global)
		{
			// - get old global service
			pservice_old = g_pservice_global;

			// - set service as this
			g_pservice_global = this;
		}

		// 2) destroy service if old service exists
		if (pservice_old.exist())
		{
			pservice_old->destroy();
		}

		// trace)
		LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> initialize SERVICE['"sv << name() << "']"sv;

		try
		{
			// 3) init 'system_executor' (system_executor reads the 'system_executor' section by itself)
			auto& context_system_executor = _context.find("system_executor"sv);
			if (context_system_executor.is_valid())
			{
				system_executor::initialize_instance(_context);
			}

			// 4) init 'dump manager' ('dump' or 'system_executor/dump' section)
			//    - applied even if system_executor was already initialized
			if (auto context_dump = _context.find("dump"sv); context_dump.is_valid())
			{
				dump_manager::initialize_instance(context_dump);
			}
			else if (context_system_executor.is_valid())
			{
				if (auto context_dump_se = context_system_executor.find("dump"sv); context_dump_se.is_valid())
					dump_manager::initialize_instance(context_dump_se);
			}

			// 5) process initialize
			result = this->Ninitializable::process_initialize(_context);
		}
		catch (...)
		{
			// - reset service
			SCOPED_LOCK(g_lockable_pservice_global)
			{
				g_pservice_global.reset();
			}

			// - result
			result = eRESULT::EXCEPTION;

			// reraise) 
			throw;
		}

		// trace)
		LOG_PROGRESS << log_level_t::important() << "@ <END> initialize SERVICE['"sv << name() << "']"sv;
	}
	catch (throwable::failure_ex& _exception)
	{
		// log out
		_log(nullptr, _exception.get_log_record());

		// reraise)
		throw;
	}

	// return)
	return result;
}

result_code server_system::Nservice::process_destroy() noexcept
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> destroy SERVICE['"sv << name() << "']"sv;

	// 1) process initialize
	auto result = this->Ninitializable::process_destroy();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> destroy SERVICE['"sv << name() << "']"sv;

	// 2) notify destroy
	{
		// - scoped lock
		std::unique_lock cs_lock(this->m_lockable_destroy);

		// - notify all
		this->m_cv_destroy.notify_one();
	}

	// 3) reset logger
	this->reset_logger();

	// 4) destroy loggers (the logs passed from the default logger are written)
	this->m_vector_logger.clear();

	// 5) reset global service
	SCOPED_LOCK(g_lockable_pservice_global)
	{
		if (g_pservice_global == this)
		{
			g_pservice_global.reset();
		}
	}


	// return)
	return result;
}

result_code server_system::Nservice::process_start(context& _context)
{
	// declare) 
	result_code result;

	try
	{
		// trace)
		LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> start SERVICE['"sv << name() << "']"sv;

		// 1) process start
		result = this->Nstartable::process_start(_context);

		// 2) time_event
		if (auto& context_time_event = _context.find("scheduled_event"sv, "time_event"sv); context_time_event.is_valid())
		{
			time_event::entity_manager::instance()->update(context_time_event);
		}
		// trace)
		LOG_PROGRESS << log_level_t::important() << "@ <END> start SERVICE['"sv << name() << "']"sv;
	}
	catch (throwable::failure_ex& _exception)
	{
		// log out
		_log(nullptr, _exception.get_log_record());

		// reraise)
		throw;
	}

	// return)
	return result;
}

result_code server_system::Nservice::process_stop() noexcept
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> stop SERVICE['"sv << name() << "']"sv;

	// 1) process start
	auto result = this->Nstartable::process_stop();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> stop SERVICE['"sv << name() << "']"sv;

	// return)
	return result;
}

void server_system::Nservice::process_pause()
{
	// 1) process start
	this->Npausable::process_pause();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ pause SERVICE['"sv << name() << "']"sv;
}

void server_system::Nservice::process_resume()
{
	// 1) process start
	this->Npausable::process_resume();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ resume SERVICE['"sv << name() << "']"sv;
}

result_code server_system::Nservice::_process_initializing(context& _context)
{
	// 1) 'service' context
	auto& context_service = _context.find("service");

	// 2) create service instance
	this->_process_initializing_service_instance(context_service);

	// 3) on_initialize_setting
	this->on_initialize_setting(_context);

	// 4) store context
	SCOPED_LOCK(this->m_lockable_setting)
	{
		if (this->m_pservice_instance.exist())
		{
			this->m_pservice_instance->info.setting.context_initialize = _context;
		}
	}

	// check) 
	if (context_service.is_valid())
	{
		// - initializing setting
		this->_process_initializing_service_setting(context_service);

		// - initializing logger
		this->_process_initializing_loggers(context_service);
	}

	// 5) on_initializing
	this->on_initializing(_context);

	// 6) default executor 
	system_executor::initialize_instance(_context);

	// return)
	return eRESULT::SUCCESS;
}

void _do_process_initialize_complete(object::Istate* _pobject, context& _context)
{
	// declare)
	std::vector<object_ptr<object::Istate>> vector_child_object;

	// 1) 모든 자식 객체를 복사한다.
	for (auto& iter : _pobject->m_container)
	{
		vector_child_object.push_back(iter);
	}

	// 2) Nameable 객체 포인터를 얻고 현재의 Context를 저장해 놓는다.
	auto* pnameable = dynamic_cast<object::Inameable*>(_pobject);
	auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : ""s;

	// 3) get now context
	auto* pcontext_now = &_context;

	// 4) context를 변경한다.
	if (pname.empty() == false)
	{
		auto& context_find = _context[pname];

		if (context_find.is_valid())
		{
			pcontext_now = &context_find;
		}
	}

	// 5) call all 'on_service_complete_initialize'
	for (auto& iter : vector_child_object)
	{
		// - casting to 'server_system::Iservice_component'
		auto pservice_component = dynamic_pointer_cast<server_system::Iservice_component>(iter);

		// - call 'on_service_complete_initialize'
		if (pservice_component.exist())
		{
			pservice_component->on_service_complete_initialize(*pcontext_now);
		}

		// - call child
		_do_process_initialize_complete(iter, _context);
	}
}

void server_system::Nservice::_process_initialize_complete(context& _context)
{
	_do_process_initialize_complete(this, _context);
}

void server_system::Nservice::_process_initializing_service_instance(context& _context)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// check)
		RETURN_IF(this->m_pservice_instance.exist(), )

		// 1) create service instance
		this->m_pservice_instance = this->process_alloc_service_instance_local(_context);

		// check)
		CGASSERT_ERROR(this->m_pservice_instance.exist());

		// check) 
		THROW_IF(this->m_pservice_instance.empty(), throwable::failure(eRESULT::INVALID_STATE, 0, "'service instance' is nullptr"sv));

		// 2) set process id
		this->m_pservice_instance->info.info.host_iid = (this->m_phost_instance.exist()) ? this->m_phost_instance->get_iid() : 0;
		this->m_pservice_instance->info.info.pid = api::get_process_id();

		// 3) apply the name set before the instance was created (the name in the service setting overrides it later)
		if (this->m_name_pending.empty() == false)
		{
			this->m_pservice_instance->info.setting.name = std::move(this->m_name_pending);
			this->m_name_pending.clear();
		}
	}
}

void server_system::Nservice::_process_initializing_service_setting(context& _context)
{
	// check) 
	RETURN_IF(_context.is_invalid(), )

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// check)
		RETURN_IF(this->m_pservice_instance.empty(), );

		// 1) get service_setting
		server_system::sSERVICE::sSETTING temp_setting = this->m_pservice_instance->info.setting;

		// 2) read service_setting_parameter
		get_parameter_service_setting(temp_setting, _context);

		// 3) set name
		if (temp_setting.name.empty() == false)
		{
			this->name(temp_setting.name);
		}

		// 4) set service setting
		this->m_pservice_instance->info.setting = std::move(temp_setting);
	}
}

void server_system::Nservice::_process_initializing_loggers(context& _context)
{
	// 1) init 'system_executor'
	auto& context_system_executor = _context.find("logger"sv);

	//check)
	RETURN_IF(context_system_executor.is_invalid(), );

	// 2) get logger
	auto plogger_global = this->get_logger();

	// check) 
	THROW_IF(plogger_global.empty(), throwable::failure(eRESULT::INVALID_STATE, 0, "default 'global logger' does not exist"sv));

	// declare) cast to logs;
	std::vector<object_ptr<LOG_RECORD>> vector_existing_logs;

	// 3) get archive dlogger
	auto plogger_hub_archive = dynamic_pointer_cast<logger_hub_archive>(plogger_global);

	// 4) get archived logs
	if (plogger_hub_archive.exist())
	{
		vector_existing_logs = plogger_hub_archive->get_stored_log();
	}

	// declare) 
	std::vector<own_ptr<Ilogger>> vector_logger;

	// 5) create loggers
	for (auto& iter_logger : context_system_executor)
	{
		// - logger type ("type": "file"(기본) / "system"(windows: event log, linux: syslog))
		own_ptr<Nlogger> plogger;
		auto& context_type = iter_logger.second.find("type"sv);
		//   "file" : logger_file, "console" : logger_console, "slack" : logger_slack (server.log.slack.h), ... (create_logger)
		const std::string str_type = context_type.is_valid() ? context_type.operator std::string() : std::string();
		plogger = create_logger(str_type);

		// check) unknown type ("slack" : registered by including "cgdk/sdk10/server.log.slack.h")
		if (plogger.empty())
		{
			LOG_ERROR << "@ (err) unknown logger type '"sv << str_type << "' (logger '"sv << iter_logger.first << "')"sv;
			continue;
		}
		
		// - set name
		plogger->name(iter_logger.first);

		// - initialize
		plogger->initialize(iter_logger.second);

		// - restore logs
		for (auto& iter_log : vector_existing_logs)
		{
			plogger->trace(object_ptr<LOG_RECORD>(iter_log));
		}

		// check)
		CONTINUE_IF(plogger.empty());

		// - register logger to default logger
		plogger_global->register_log_targetable(plogger);

		vector_logger.push_back(std::move(plogger));
	}

	// 6) set created logger;
	this->m_vector_logger = std::move(vector_logger);
}

result_code server_system::Nservice::_process_starting(context& _context)
{
	// 1) default executor 
	system_executor::initialize_instance(_context);

	// 2) on_start_setting
	this->on_start_setting(_context);

	// 3) on_starting
	this->on_starting(_context);

	// return)
	return eRESULT::SUCCESS;
}

void _do_process_start_complete(object::Istate* _pobject, context& _context)
{
	// declare)
	std::vector<object_ptr<object::Istate>> vector_child_object;

	// 1) 모든 자식 객체를 복사한다.
	for (auto& iter : _pobject->m_container)
	{
		vector_child_object.push_back(iter);
	}

	// 2) Nameable 객체 포인터를 얻고 현재의 Context를 저장해 놓는다.
	auto* pnameable = dynamic_cast<object::Inameable*>(_pobject);
	auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : ""s;

	// 3) get now context
	auto* pcontext_now = &_context;

	// 4) context를 변경한다.
	if (pname.empty() == false)
	{
		auto& context_find = _context[pname];

		if (context_find.is_valid())
		{
			pcontext_now = &context_find;
		}
	}

	// 5) call all 'on_service_complete_initialize'
	for (auto& iter : vector_child_object)
	{
		// - casting to 'server_system::Iservice_component'
		auto pservice_component = dynamic_pointer_cast<server_system::Iservice_component>(iter);

		// - call 'on_service_complete_initialize'
		if (pservice_component.exist())
		{
			pservice_component->on_service_complete_start(*pcontext_now);
		}

		// - call child
		_do_process_start_complete(iter, _context);
	}
}

void server_system::Nservice::_process_start_complete(context& _context)
{
	_do_process_start_complete(this, _context);
}

void _do_process_stop_complete(object::Istate* _pobject)
{
	// declare)
	std::vector<object_ptr<object::Istate>> vector_child_object;

	// 1) 모든 자식 객체를 복사한다.
	for (auto& iter : _pobject->m_container)
	{
		vector_child_object.push_back(iter);
	}

	// 2) call all 'on_service_complete_initialize'
	for (auto& iter : vector_child_object)
	{
		// - call child
		_do_process_stop_complete(iter);

		// - casting to 'server_system::Iservice_component'
		auto pservice_component = dynamic_pointer_cast<server_system::Iservice_component>(iter);

		// - call 'on_service_complete_initialize'
		if (pservice_component.exist())
		{
			try
			{
				pservice_component->on_service_complete_stop();
			}
			catch (...)
			{
			}
		}
	}
}

void server_system::Nservice::_process_stop_complete() noexcept
{
	// 1) child stopped
	_do_process_stop_complete(this);

	// 2) stop completed
	object::Nstartable::_process_stop_complete();

}

void _do_process_destroy_complete(object::Istate* _pobject)
{
	// declare)
	std::vector<object_ptr<object::Istate>> vector_child_object;

	// 1) 모든 자식 객체를 복사한다.
	for (auto& iter : _pobject->m_container)
	{
		vector_child_object.push_back(iter);
	}

	// 2) call 'on_service_complete_destroy' on each child (depth-first)
	for (auto& iter : vector_child_object)
	{
		// - call child
		_do_process_destroy_complete(iter);

		// - casting to 'server_system::Iservice_component'
		auto pservice_component = dynamic_pointer_cast<server_system::Iservice_component>(iter);

		// - call 'on_service_complete_destroy'
		if (pservice_component.exist())
		{
			try
			{
				pservice_component->on_service_complete_destroy();
			}
			catch (...)
			{
			}
		}
	}
}

void server_system::Nservice::_process_destroy_complete() noexcept
{
	// 1) destroy service instance
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - reset service_instance
		this->m_pservice_instance.reset();
	}

	// 2) call destroy child objects
	_do_process_destroy_complete(this);

	// 3) set state to eOBJECT_STATE::NONE
	this->CGDK::object::Ninitializable::_process_destroy_complete();

	// 4) the loggers are destroyed in process_destroy after the default logger is reset
	//    (the default logger passes its queued logs to them first : no log is lost)
}

void server_system::Nservice::process_state_changed(eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) set state
		if (this->m_pservice_instance.exist())
		{
			this->m_pservice_instance->info.status.state = _state_new;
		}
	}

	// message) openning상태라고 message를 날린다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::UPDATE_SERVICE_STATUS, static_cast<uintptr_t>(_state_new), static_cast<uintptr_t>(_state_old)));
}

void server_system::Nservice::process_set_name(std::string_view _name) 
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) get service instance
		auto& pservice_instance = this->m_pservice_instance;

		// check) not created yet (e.g. called in the constructor) => keep it until the instance is created
		if (pservice_instance.empty())
		{
			this->m_name_pending = std::string(_name);
			return;
		}

		// 2) set
		pservice_instance->info.setting.name = std::string(_name);
	}
}

bool server_system::Nservice::process_is_this(std::string_view _str_compare) const noexcept
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) get service instance
		auto& pservice_instance = this->m_pservice_instance;

		// check) not created yet
		RETURN_IF(pservice_instance.empty(), this->m_name_pending == _str_compare);

		// return)
		return pservice_instance->info.setting.name == _str_compare;
	}
}

std::string server_system::Nservice::process_get_name() const noexcept
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) get service instance
		auto& pservice_instance = this->m_pservice_instance;

		// check) not created yet
		RETURN_IF(pservice_instance.empty(), this->m_name_pending);

		// return)
		return pservice_instance->info.setting.name;
	}
}

own_ptr<server_system::service::instance_local> server_system::Nservice::process_alloc_service_instance_local(context& /*_context*/)
{
	// 1) make service instance for local
	auto pservice_instance = make_own<server_system::service::instance_local>();

	// 2) initialize1 service instance`
	pservice_instance->instance_key.initialize();

	// return) 
	return pservice_instance;
}

own_ptr<server_system::service::manager> server_system::Nservice::process_alloc_service_instance_manager(context& /*_context*/)
{
	// 1) make service instance manager
	auto pservice_instance_manager = make_own<server_system::service::manager>();

	// 2) initialize service instance manager

	// return)
	return pservice_instance_manager;
}

own_ptr<server_system::host::manager> server_system::Nservice::process_alloc_host_instance_manager(context& /*_context*/)
{
	// 1) make host instance manager
	auto phost_instance_manager = make_own<server_system::host::manager>();

	// return) 
	return phost_instance_manager;
}
