#include "pch.h"

using namespace CGDK;

namespace CGDK
{
	extern lockable<> g_lockable_pservice_global;
	extern object_ptr<server_system::Nservice> g_pservice_global;
}

object_ptr<server_system::Nservice> CGDK::server_system::get_service() noexcept
{
	std::lock_guard<lockable<>> cs(g_lockable_pservice_global);

	return g_pservice_global;
}

server_system::Nservice::Nservice() noexcept
{
}

server_system::Nservice::Nservice(const std::string_view _name) noexcept
{
	// 1) set name
	this->m_service_setting.nickname = _name;
}

server_system::Nservice::~Nservice() noexcept
{
}

void server_system::Nservice::setting(server_admin::sSERVICE_SETTING&& _service_setting)
{
	// 1) on_service_setting 함수를 호출한다.
	this->on_service_setting(_service_setting);

	// 2) copy configure strings (이렇게 안하면 초기화 시 초기화 스트링이 모두 사라져 오류를 발생할 수 있음)
	if (_service_setting.buf_configure.empty())
		_service_setting.buf_configure = std::move(this->m_service_setting.buf_configure);

	if (_service_setting.buf_setting.empty())
		_service_setting.buf_setting = std::move(this->m_service_setting.buf_setting);

	// 3) service setting값을 설정한다.
	this->m_service_setting = std::move(_service_setting);
}

bool server_system::Nservice::wait_destroy(const chrono::tick::duration& _wait)
{
	conditional_lock(this->m_lock_destroy, cs)
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


void server_system::Nservice::on_service_setting(const server_admin::sSERVICE_SETTING& /*_service_setting*/)
{
}

void server_system::Nservice::on_service_info(const shared_buffer& /*_service_info*/)
{
}

void server_system::Nservice::on_service_setting(context& _context)
{
	// 1) configure를 읽어서 설정한다.
	if (this->m_service_setting.buf_configure.size() != 0)
	{
		// - set context!
		_context.read_from_json_string(this->m_service_setting.buf_configure.data<const char>());
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
		scoped_lock(g_lockable_pservice_global)
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
			// 3) init 'system_executor'
			auto context_system_executor = _context.find("system_executor"sv);

			if (context_system_executor.is_valid())
			{
				system_executor::initialize_instance(context_system_executor);
			}

			// 2) process initialize
			result = this->Ninitializable::process_initialize(_context);
		}
		catch (...)
		{
			// - reset service
			scoped_lock(g_lockable_pservice_global)
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

	// 2) signal
	{
		// - scoped lock
		std::unique_lock cs_lock(this->m_lock_destroy);

		// - notify all
		this->m_cv_destroy.notify_one();
	}

	// 3) reset logger
	this->reset_logger();

	// 4) reset global service
	scoped_lock(g_lockable_pservice_global)
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
		auto context_time_event = _context.find("scheduled_event"sv, "time_event"sv);

		if (context_time_event.is_valid())
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
	// 1) default executor 
	system_executor::initialize_instance(_context);

	// 2) on_service_setting
	this->on_service_setting(_context);

	// 3) on_initializing
	this->on_initializing(_context);

	// return)
	return eRESULT::SUCCESS;
}

result_code server_system::Nservice::_process_starting(context& _context)
{
	// 1) default executor 
	system_executor::initialize_instance(_context);

	// 2) on_service_setting
	this->on_service_setting(_context);

	// 3) on_starting
	this->on_starting(_context); 

	// return)
	return eRESULT::SUCCESS;
}

void server_system::Nservice::process_state_changed(eOBJECT_STATE _status_new, eOBJECT_STATE /*_state_old*/)
{
	// message) openning상태라고 message를 날린다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::UPDATE_SERVICE_STATUS, static_cast<uintptr_t>(_status_new)));
}
