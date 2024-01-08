#include "pch.h"
#include <iostream>

#if defined(_WIN32)
	#include <conio.h>
#elif defined(__linux__)
	#include <cstdio>
	#include <sys/wait.h>
	#include <sys/stat.h>
#else
#endif

using namespace CGDK;

lockable<> server_system::Nservice_application::m_lockable_create;
own_ptr<server_system::Nservice_application> server_system::Nservice_application::m_pinstance;

server_system::Nservice_application::Nservice_application() :
	m_status_object(eOBJECT_STATE::NONE)
{
	try
	{
		// 1) get logger
		initialize_logger();

		// declare) 
		_CHAR str_service_name[MAX_PATH] = { L'0', };

		// 2) get service path name
		auto result = api::get_service_path_name(str_service_name);

		// check)  
		THROW_IF(result == 0, throwable::failure(eRESULT::BAD_ALLOC), LOG_ERROR << "(err ) fail to get_service_path_name()"sv);

		// 3) set name
		set_name(str_service_name);
	}
	catch (throwable::failure_ex& _exception)
	{
		// log out
		_log(nullptr, _exception.get_log_record());

		// reraise)
		throw;
	}
}

server_system::Nservice_application::~Nservice_application()
{
}

result_code server_system::Nservice_application::start(context& /*_context*/)
{
	// check)
	RETURN_IF(name().empty(), false);

	// 3) Result
	auto result = api::service_start(name(), 0, nullptr)!=0;

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::BAD_FUNCTION_CALL), LOG_ERROR << "(err ) fail to start()"sv);

	// return) 
	return eRESULT::SUCCESS;
}

result_code server_system::Nservice_application::stop() noexcept
{
	// check)
	RETURN_IF(name().empty(), eRESULT::NOT_READY);

	// Delcare)
	result_code result;

#if defined(_WIN32)
	// 1) chekc service mode or not
	bool is_service_mode = GetConsoleWindow() ? false : true;

	// 2-A) stop if console mode
	if (is_service_mode == false)
	{
		// - stop
		process_service_stop();

		// - set result as true
		result = eRESULT::SUCCESS;
	}
	// 2-B) call API 'api::service_stop' if service mode
	else
	{
		result = (api::service_stop(name()) != 0) ? eRESULT::SUCCESS : eRESULT::FAIL;
	}

	// check) 
	ERROR_RETURN_IF(result == eRESULT::SUCCESS, result, LOG_ERROR << "(err ) application<TCLASS>: fail to stop"sv);
#elif defined(__linux__)
	// 2)
	process_service_stop();
#else
	#error	"[CGDK10] Not supported Platform"
#endif

	// return)
	return result;
}

result_code server_system::Nservice_application::pause()
{
	// check)
	RETURN_IF(name().empty(), eRESULT::NOT_READY);

	// 1) Result
	auto result = api::service_pause(name())!=0;

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::BAD_FUNCTION_CALL), LOG_ERROR << "(err ) fail to pause()"sv);

	// return) 
	return eRESULT::SUCCESS;
}

result_code server_system::Nservice_application::resume()
{
	// check)
	RETURN_IF(name().empty(), eRESULT::NOT_READY);

	// 1) Result
	auto result = api::service_continue(name())!=0;

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::BAD_FUNCTION_CALL), LOG_ERROR << "(err ) fail to continue()"sv);

	// return) 
	return eRESULT::SUCCESS;
}

eOBJECT_STATE server_system::Nservice_application::get_object_state() const noexcept
{
	return this->m_status_object;
}

eOBJECT_STATE server_system::Nservice_application::set_object_state(eOBJECT_STATE _status) noexcept
{
	// 1) change object state to _status
	auto status_old = this->m_status_object.exchange(_status);
	
	// check) return object_state is same.
	RETURN_IF(status_old == _status, status_old);

	// 2) call 'process_state_changed;
	this->process_state_changed(_status, status_old);

	// return) 
	return status_old;
}

bool server_system::Nservice_application::set_object_state_if(eOBJECT_STATE _status_compare, eOBJECT_STATE _status) noexcept
{
	// 1) change object_state to _status if only old object_state is _status_compare
	auto is_changed = this->m_status_object.compare_exchange_weak(_status_compare, _status);

	// check) return if no changed
	RETURN_IF(is_changed == false, false);

	// 2) call 'process_state_changed;
	process_state_changed(_status, _status_compare);

	// return) 
	return true;
}

void server_system::Nservice_application::process_service_start(context& _context)
{
	// 1) iinitialize
	initialize(_context);

	// 2) if pre-object_state is eOBJECT_STATE::STOPPED, change object_state to eOBJECT_STATE::START_PENDING.
	auto is_changed = this->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::START_PENDING);

	// check) return if no changed (pre-object_state is not eOBJECT_STATE::STOPPED)
	RETURN_IF(is_changed == false);

	try
	{
		// 3) process start self
		this->process_start(_context);
	}
	catch (...)
	{
		// - roll back object_state to eOBJECT_STATE::STOPPED
		this->set_object_state(eOBJECT_STATE::STOPPED);

		// - call 'destroy'
		this->destroy();

		// return) 
		return;
	}

	// 6) set object_state to eOBJECT_STATE::RUNNING
	this->set_object_state(eOBJECT_STATE::RUNNING);

	// 7) set state of m_handle_server_status too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::RUNNING);
}
 
void server_system::Nservice_application::process_service_stop()
{
	// 1) if pre-object_state is eOBJECT_STATE::RUNNING, change object_state to eOBJECT_STATE::STOP_PENDING.
	auto is_changed = this->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::STOP_PENDING);

	// check) return if no changed (pre-object_state is not eOBJECT_STATE::RUNNING)
	RETURN_IF(is_changed == false);

	// 2) change 'm_handle_server_status''s state too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::STOP_PENDING);

	try
	{
		// 3) call main process (process_pause)
		this->process_stop();
	}
	catch (...)
	{
	}

	// 5) set object_state to eOBJECT_STATE::STOPPED
	this->set_object_state(eOBJECT_STATE::STOPPED);

	// 6) set m_handle_server_status status too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::STOPPED);
}

void server_system::Nservice_application::process_service_pause(uint32_t /*_type*/, void* /*_pevent_data*/, void* /*_pcontext*/)
{
	// 1) if pre-object_state is eOBJECT_STATE::RUNNING, change object_state to eOBJECT_STATE::PAUSE_PENDING.
	auto is_changed = this->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::PAUSE_PENDING);

	// check) return if no changed (pre-object_state is not eOBJECT_STATE::RUNNING)
	RETURN_IF(is_changed == false);

	// 2) change 'm_handle_server_status''s state too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::PAUSE_PENDING);

	// 3) call main process (process_pause)
	this->process_pause();

	// 4) set object_state to eOBJECT_STATE::PAUSED
	this->set_object_state(eOBJECT_STATE::PAUSED);

	// 5) set m_handle_server_status status too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::PAUSED);
}

void server_system::Nservice_application::process_service_continue(uint32_t /*_type*/, void* /*_pevent_data*/, void* /*_pcontext*/)
{
	// 1) if pre-object_state is eOBJECT_STATE::PAUSED, change object_state to eOBJECT_STATE::CONTINUE_PENDING.
	auto is_changed = this->set_object_state_if(eOBJECT_STATE::PAUSED, eOBJECT_STATE::CONTINUE_PENDING);

	// check) return if no changed (pre-object_state is not eOBJECT_STATE::PAUSED)
	RETURN_IF(is_changed == false);

	// 2) change 'm_handle_server_status''s state too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::CONTINUE_PENDING);

	// 3) call main process (process_continue)
	this->process_resume();

	// 4) set object_state to eOBJECT_STATE::RUNNING
	this->set_object_state(eOBJECT_STATE::RUNNING);

	// 5) set m_handle_server_status status too
	this->m_handle_server_status.set_service_status(eOBJECT_STATE::RUNNING);
}

#if defined(__linux__)
void server_system::Nservice_application::process_signal_stop(int /*_signal*/)
{
	// 1) get service instance
	object_ptr<Nservice_application> pinstance = m_pinstance;

	// check) 
	RETURN_IF(pinstance.empty());

	// 2) ...
	pinstance->stop();
}

void server_system::Nservice_application::process_signal_pause(int /*_signal*/)
{
	// 1) get service instance
	object_ptr<Nservice_application> pinstance = m_pinstance;

	// check) 
	RETURN_IF(pinstance.empty());

	// 2) ...
	pinstance->pause();
}

void server_system::Nservice_application::process_signal_continue(int /*_signal*/)
{
	// 1) get service instance
	object_ptr<Nservice_application> pinstance = m_pinstance;

	// check) 
	RETURN_IF(pinstance.empty());

	// 2) ...
	pinstance->resume();
}

void server_system::Nservice_application::process_signal_hup(int /*_signal*/)
{
}

void server_system::Nservice_application::process_signal_interrupt(int /*_signal*/)
{
	// 1) get service instance
	object_ptr<Nservice_application> pinstance = m_pinstance;

	// check) 
	RETURN_IF(pinstance.empty());

	// 2) ...
	pinstance->destroy();
}

void server_system::Nservice_application::process_signal_quit(int /*_signal*/)
{
	// 1) get service instance
	object_ptr<Nservice_application> pinstance = m_pinstance;

	// check) 
	RETURN_IF(pinstance.empty());

	// 2) ...
	pinstance->destroy();
}

void server_system::Nservice_application::process_signal_segment_fault(int /*_signal*/)
{
}
#endif

void server_system::Nservice_application::process_state_changed(eOBJECT_STATE _state_new, eOBJECT_STATE /*_state_old*/)
{
	if (_state_new == eOBJECT_STATE::NONE)
	{
		this->m_cv_complete.notify_one();
	}
}

#if defined(_WIN32)
int server_system::Nservice_application::process_service_main(int /*_argc*/, char** /*_pargv*/, __callback LPHANDLER_FUNCTION_EX _lp_handler_proc)
{
	// 1) 
	if(_lp_handler_proc != nullptr)
	{
		// - 
		auto result = this->m_handle_server_status.register_service_ctrl_handle(name(), _lp_handler_proc);

		// check) 
		THROW_IF(result != ERROR_SUCCESS, throwable::failure(eRESULT::BAD_FUNCTION_CALL), LOG_ERROR << "(err ) fail to register_service_ctrl_handle()"sv);
	}

	// declare) 
	context context_setting;

	// 2) start service
	this->process_service_start(context_setting);

	if(_lp_handler_proc != nullptr)
	{
		// 3) 
		while(this->get_object_state() != eOBJECT_STATE::NONE)
		{
			// - call 'process_service_run'
			this->process_service_run();

			// - wait untile complete
			scoped_lock_named(this->m_lock_complete, lock_wait)
			{
				this->m_cv_complete.wait(lock_wait);
			}
		}
	}
	else
	{
		int ch;

		// - wail until 'ESC'
		while((ch = _getch()) != 27)
		{
			switch(ch)
			{
			case	'=':
					factory::manager::print_statistics(true);
					break;

			case	'+':
					factory::manager::print_statistics();
					break;

			case	'-':
					executor::manager::print_statistics(true);
					break;

			case	'_':
					executor::manager::print_statistics();
					break;
			}

			// - call 'process_service_run'
			process_service_run();

			// - sleep
			std::this_thread::sleep_for(100ms);
		}

		// - destroy
		destroy();
	}

	// return)
	return 1;
}
#elif defined(__linux__)
int server_system::Nservice_application::process_service_main(int /*_argc*/, char** /*_pargv*/, __callback LPHANDLER_FUNCTION_EX /*_lp_handler_proc*/)
{
	// 1) set 
	signal(SIGPIPE, SIG_IGN);
	signal(SIGSYS,  SIG_IGN);
	signal(SIGTSTP, process_signal_stop);
	signal(SIGSTOP, process_signal_pause);
	signal(SIGCONT, process_signal_continue);
	signal(SIGHUP,  process_signal_hup);
	signal(SIGINT,  process_signal_interrupt);
	signal(SIGQUIT, process_signal_quit);
	//signal(SIGSEGV, signal_segment_fault);

	// declare) 
	context context_setting;

	// 3) start
	this->process_service_start(context_setting);

	// 4) work look 
	while(this->get_object_state() != eOBJECT_STATE::NONE)
	{
		// - call 'process_service_run'
		this->process_service_run();

		// - wait untile complete
		scoped_lock_named(this->m_lock_complete, lock_wait)
		{
			this->m_cv_complete.wait(lock_wait);
		}
	}

	// return)
	return 1;
}
#else
#endif

uint32_t server_system::Nservice_application::process_service_handler(uint32_t _dwControl, uint32_t _type, void* _pevent_data, void* _pcontext)
{
	switch (_dwControl)
	{
	case	SERVICE_CONTROL_STOP:
			this->process_service_stop();
			break;

	case	SERVICE_CONTROL_PAUSE:
			this->process_service_pause(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_CONTINUE:
			this->process_service_continue(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_INTERROGATE:
			this->process_interrogate(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_SHUTDOWN:
			this->process_shutdown(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_PARAMCHANGE:
			this->process_param_change(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_NETBINDADD:
			this->process_net_bind_add(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_NETBINDREMOVE:
			this->process_net_bind_remove(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_NETBINDENABLE:
			this->process_net_bind_enable(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_NETBINDDISABLE:
			this->process_net_bind_disable(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_DEVICEEVENT:
			this->process_device_event(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_HARDWAREPROFILECHANGE:
			this->process_hardware_profile_change(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_POWEREVENT:
			this->process_power_event(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_SESSIONCHANGE:
			this->process_session_change(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_PRESHUTDOWN:
			this->process_pre_shutdown(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_TIMECHANGE:
			this->process_time_change(_type, _pevent_data, _pcontext);
			break;

	case	SERVICE_CONTROL_TRIGGEREVENT:
			this->process_trigger_event(_type, _pevent_data, _pcontext);
			break;

	default:
			break;
	};

	// return) 
	return 0;
}

void server_system::Nservice_application::process_service_run()
{
}

result_code server_system::Nservice_application::process_initialize(context& _context)
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> initialize SERVICE APPLICATION['"sv << name() << "']"sv;

	// declare)
	result_code result;

	try
	{
		// 1) Process initialize
		result = this->Ninitializable::process_initialize(_context);
	}
	catch (throwable::failure_ex& _exception)
	{
		// log out
		_log(nullptr, _exception.get_log_record());

		// reraise)
		throw;
	}

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> initialize SERVICE APPLICATION['"sv << name() << "']"sv;

	// return)
	return result;
}

result_code server_system::Nservice_application::process_destroy() noexcept
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> destroy SERVICE APPLICATION['"sv << name() << "']"sv;

	// 1) Process initialize
	auto result = this->Ninitializable::process_destroy();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> destroy SERVICE APPLICATION['"sv << name() << "']"sv;

	// 2) reset logger
	this->reset_logger();

	// return)
	return result;
}

result_code server_system::Nservice_application::process_start(context& _context)
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> start SERVICE APPLICATION['"sv << name() << "']"sv;

	// declare)
	result_code result;

	try
	{
		// 1) Process start
		result = this->Nstartable::process_start(_context);
	}
	catch (throwable::failure_ex& _exception)
	{
		// log out
		_log(nullptr, _exception.get_log_record());

		// reraise)
		throw;
	}

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> start SERVICE APPLICATION['"sv << name() << "']"sv;

	// return)
	return result;
}

result_code server_system::Nservice_application::process_stop() noexcept
{
	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <BEGIN> stop SERVICE APPLICATION['"sv << name() << "']"sv;

	// 1) Process start
	auto result = this->Nstartable::process_stop();

	// trace)
	LOG_PROGRESS << log_level_t::important() << "@ <END> stop SERVICE APPLICATION['"sv << name() << "']"sv;

	// return)
	return result;
}

result_code server_system::Nservice_application::_process_initializing(context& _context)
{
	// 1) Default executor 
	system_executor::initialize_instance(_context);

	// 2) on_service_setting
	this->on_service_setting(_context);

	// 3) on_initializing
	this->on_initializing(_context);

	// return)
	return eRESULT::SUCCESS;
}

result_code server_system::Nservice_application::_process_starting(context& _context)
{
	// 1) on_service_setting
	this->on_service_setting(_context);

	// 2) on_starting
	this->on_starting(_context); 

	// return)
	return eRESULT::SUCCESS;
}

void server_system::Nservice_application::process_interrogate(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_interrogate(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_shutdown(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_shutdown(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_param_change(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_param_change(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_net_bind_add(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_net_bind_add(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_net_bind_remove(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_net_bind_remove(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_net_bind_enable(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_net_bind_enable(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_net_bind_disable(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_net_bind_disable(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_device_event(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_device_event(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_hardware_profile_change(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_hardware_profile_change(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_power_event(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_power_event(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_session_change(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_session_change(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_pre_shutdown(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_pre_shutdown(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_time_change(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_time_change(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_trigger_event(uint32_t _type, void* _pevent_data, void* _pcontext)
{
	this->on_process_trigger_event(_type, _pevent_data, _pcontext);
}

void server_system::Nservice_application::process_log(const std::vector<object_ptr<LOG_RECORD>>& /*_vector_log_record*/)
{
}

#if defined(_WIN32)
uint32_t CALLBACK server_system::Nservice_application::fservice_main(int _argc, char** _pargv)
{
	return server_system::Nservice_application::m_pinstance->process_service_main(_argc, _pargv, fservice_handler);
}

DWORD CALLBACK server_system::Nservice_application::fservice_handler(DWORD _Control, DWORD _type, LPVOID _pevent_data, LPVOID _pcontext)
{
	return server_system::Nservice_application::m_pinstance->process_service_handler(_Control, _type, _pevent_data, _pcontext);
}
#endif

int server_system::Nservice_application::process_run(int _argc, char** _pargv)
{
	// check) get name and check
	RETURN_IF(name().empty(), -1);

	// declare)
	int result_return = 0;

	try
	{
	#if defined(_WIN32)
		// 1-A) call 'process_service_main' if console mode (for debug)
		if(GetConsoleWindow() != 0)
		{
			// - create logger
			own_ptr<logger_console> plogger_console = make_own<logger_console>();

			// - initialize
			plogger_console->initialize();

			// - get logger;
			auto plogger_hub = this->get_logger();

			// - register console logger
			if (plogger_hub.exist())
			{
				plogger_hub->register_log_targetable(plogger_console);
			}

			// - process_service_main
			result_return = this->process_service_main(_argc, _pargv, nullptr);

			// - unregister console logger
			if (plogger_hub.exist())
			{
				plogger_hub->unregister_log_targetable(plogger_console);
			}
		}
		// 1-B) call WIN32 service API 'StartServiceCtrlDispatcher' if service mode
		else
		{
			// declare) 
			SERVICE_TABLE_ENTRYA sSTE[] = 
			{
				{ (char*)name().data(), reinterpret_cast<LPSERVICE_MAIN_FUNCTIONA>(fservice_main) },
				{ 0, 0}
			};

			// - dispatch command to service
			auto result = ::StartServiceCtrlDispatcherA(sSTE);

			// - result
			result_return = (result != FALSE) ? 1 : 0;
		}

	#elif defined(__linux__)
		// 1) if 'fork' is executed, additional release() is called.
		add_ref();

		// 2) call 'process_service_main'
		result_return = this->process_service_main(_argc, _pargv, nullptr);
	#else
		#error "[CGDK10] Not supported Platform"
	#endif
	}
	catch (Ithrowable& _e)
	{
		std::cout << "(excp) service terminated abnormaly\n";
		std::cout << "       " << _e.description() << '\n';
	}
	catch (...)
	{
		std::cout << "(excp) service terminated abnormaly\n";
		std::cout << "       unknowned reason\n";
	}

	// return) 
	return result_return;
}

#if defined(_WIN32)
bool server_system::Nservice_application::process_service_parpare(int _argc, char** _ppargv, Nservice_application* _pservice)
{
	// 1) parsing arguments
	if(_argc >= 2)
	{
		// - is first character is '-' or '/'
		int	character = _ppargv[1][0];

		if(character =='-' || character =='/')
		{
			// declare) 
			int	command = _ppargv[1][1];

			// - process by command
			switch(command)
			{
			// - Install
			case	'i':
			case	'I':
					if(api::service_install(m_pinstance->get_name().data(), ""))
					{
						printf("success on 'Install' service\n");
					}
					else
					{
						printf("error!! fail to 'Install' service\n");
					}
					return true;

			// - Uninstall
			case	'u':
			case	'U':
					if(api::service_uninstall())
					{
						printf("success on 'uninstall' service");
					}
					else
					{
						printf("error! fail to 'Uninstall' service\n");
					}
					return true;

			// - start
			case	's':
			case	'S':
					if(_pservice->start())
					{
						printf("success on 'start' service\n");
					}
					else
					{
						printf("error!! fail to 'start' service\n");
					}
					return true;

			// - stop
			case	't':
			case	'T':
					if(_pservice->stop())
					{
						printf("success on 'stop' service\n");
					}
					else
					{
						printf("error! fail to 'stop' service\n");
					}
					return true;

			// - pause
			case	'p':
			case	'P':
					if(_pservice->pause())
					{
						printf("success on 'pause' service\n");
					}
					else
					{
						printf("error! fail to 'pause' service\n");
					}
					return true;

			// - continue
			case	'c':
			case	'C':
					if(_pservice->resume())
					{
						printf("success on 'continue' service\n");
					}
					else
					{
						printf("error! fail to 'continue' service\n");
					}
					return true;

			default:
					return false;
			};
		}
	}

	// return)
	return false;
}
#elif defined(__linux__)
bool server_system::Nservice_application::process_service_parpare([[maybe_unused]] int _argc, [[maybe_unused]] char** _ppargv)
{
	// 1) fork
	auto pid = fork();

	// check) throw exception if failed
	THROW_IF(pid < 0, throwable::failure(eRESULT::FAIL));

	// check) return if parent process
	RETURN_IF(pid > 0, true);

	// 2) change directorys and setsid
	signal(SIGHUP, SIG_IGN);

	// 3) close stdin/stdout/stderr ...
	close(0);
	close(1);
	close(2);
	umask(0);

	// declare) unused
	[[maybe_unused]] int result;

	auto null_file = open("/dev/null", O_RDWR);
	result = dup(null_file);
	result = dup(null_file);

	// 4) root~ and set new session
	result = chdir("/");
	setsid();

	// 3) process...
	while (true) 
	{
		// - fork sub
		auto pid_sub = fork();

		// check) 
		THROW_IF(pid_sub < 0, throwable::failure(eRESULT::FAIL));

		// check) break if child  process
		BREAK_IF(pid_sub == 0);

		// - parent process (wait util child process is gracefully complete)
		if(pid_sub > 0)
		{
			// declare) 
			int ret = 0;

			// - wait util sub process is end
			::wait(&ret);

			// check)
			RETURN_IF(WIFEXITED(ret) || WIFSIGNALED(ret), true);
		}
	}

	// return)
	return false;
}
#endif
