#include "pch.h"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cerrno>

using namespace CGDK;

// statics)
std::atomic<uint32_t>	net::emergency::manager::m_s_signal_pending{ 0 };
std::atomic<int>		net::emergency::manager::m_s_signal_role[net::emergency::manager::MAX_SIGNAL]{};
std::mutex				net::emergency::manager::m_s_lockable_create;
own_ptr<net::emergency::manager> net::emergency::manager::m_s_pinstance;

namespace
{
	constexpr std::string_view DESTROY_HANDLER_NAME = "net::emergency::manager"sv;

	std::string_view _to_string(net::emergency::manager::eSIGNAL _signal) noexcept
	{
		switch (_signal)
		{
		case net::emergency::manager::eSIGNAL::SHUTDOWN:	return "shutdown"sv;
		case net::emergency::manager::eSIGNAL::EMERGENCY:	return "emergency"sv;
		case net::emergency::manager::eSIGNAL::TERMINATE:	return "terminate"sv;
		default:											return "none"sv;
		}
	}
}


//-----------------------------------------------------------------------------
// SETTING
//-----------------------------------------------------------------------------
net::emergency::manager::SETTING::SETTING()
{
#if defined(_WIN32)
	this->signals_shutdown = { SIGINT };
	this->signals_emergency = { SIGBREAK };
	this->signals_terminate = { SIGTERM };
#else
	this->signals_shutdown = { SIGTERM, SIGINT };
	this->signals_emergency = { SIGUSR1 };
	this->signals_terminate = { SIGUSR2 };
#endif
}


//-----------------------------------------------------------------------------
// instance
//-----------------------------------------------------------------------------
net::emergency::manager::manager()
{
	// 1) name
	this->name("emergency manager"sv);
}

net::emergency::manager::~manager() noexcept
{
	this->process_uninstall();
}

result_code net::emergency::manager::destroy() noexcept
{
	this->process_uninstall();

	return eRESULT::SUCCESS;
}

void net::emergency::manager::initialize_instance()
{
	SCOPED_LOCK(m_s_lockable_create)
	{
		// check)
		RETURN_IF(m_s_pinstance.exist());

		// 1) create instance
		m_s_pinstance.exchange_release(_make_own_raw<manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}

void net::emergency::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	SCOPED_LOCK(m_s_lockable_create)
	{
		pinstance = std::move(m_s_pinstance);
	}

	// 1) destroy (uninstall)
	pinstance.reset();
}


//-----------------------------------------------------------------------------
// publics
//-----------------------------------------------------------------------------
void net::emergency::manager::install(SETTING&& _setting)
{
	instance()->process_install(std::move(_setting));
}

void net::emergency::manager::uninstall() noexcept
{
	auto p = m_s_pinstance.get_acquire();

	if (p != nullptr)
		p->process_uninstall();
}

bool net::emergency::manager::is_installed() noexcept
{
	auto p = m_s_pinstance.get_acquire();

	return (p != nullptr) && p->m_is_installed.load();
}

void net::emergency::manager::trigger(eSIGNAL _signal) noexcept
{
	// check)
	RETURN_IF(_signal == eSIGNAL::NONE);

	// 1) raise the flag
	m_s_signal_pending.fetch_or(static_cast<uint32_t>(_signal), std::memory_order_release);

	// 2) wake up the thread (a signal handler can not : the thread polls the flag)
	auto p = m_s_pinstance.get_acquire();

	if (p != nullptr)
	{
		std::lock_guard lock(p->m_lockable_cv);
		p->m_cv.notify_all();
	}
}

object_ptr<net::emergency::Nacceptor> net::emergency::manager::acceptor() noexcept
{
	auto p = m_s_pinstance.get_acquire();

	// check)
	RETURN_IF(p == nullptr, object_ptr<Nacceptor>());

	// lock)
	std::lock_guard lock(p->m_lockable_acceptor);

	// return)
	return p->m_pacceptor.get_object_ptr();
}


//-----------------------------------------------------------------------------
// install / uninstall
//-----------------------------------------------------------------------------
void net::emergency::manager::process_install(SETTING&& _setting)
{
	// check) once
	THROW_IF(this->m_is_installed.load(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "net::emergency::manager is installed already"sv));

	// 1) setting
	this->m_setting = std::move(_setting);

	if (this->m_setting.interval_supervise <= chrono::tick::duration::zero())
		this->m_setting.interval_supervise = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::milliseconds(100));

	if (this->m_setting.interval_restart <= chrono::tick::duration::zero())
		this->m_setting.interval_restart = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::seconds(3));

	this->m_is_shutdown_requested = false;
	this->m_tick_restart = chrono::tick::time_point{};
	m_s_signal_pending.store(0);

	// 2) the system_executor : the manager is stopped on its (normal) destroy
	//    (initialized here if it is not yet. on linux dump_manager installs its handlers of SIGUSR1/SIGUSR2 then : ours replace them below)
	system_executor::instance();
	system_executor::register_destroy_handler(DESTROY_HANDLER_NAME, []() { net::emergency::manager::uninstall(); });

	// 3) the thread (its own thread, not an executor : it must work while every executor is jammed)
	this->m_is_stop = false;

	auto pthread = make_object<api::thread>();

	pthread->name("emergency manager"sv);
	pthread->begin_thread([this]() { this->process_thread(); });

	this->m_pthread = std::move(pthread);

	// 4) the signals
	this->m_is_installed = true;
	this->process_install_signals();

	// trace)
	LOG_INFO << "  - [emergency manager] installed"sv;
}

void net::emergency::manager::process_uninstall() noexcept
{
	// check) once
	RETURN_IF(this->m_is_installed.exchange(false) == false);

	// trace)
	LOG_INFO << "  - [emergency manager] uninstall"sv;

	// 1) the signal handlers back to the previous ones
	this->process_uninstall_signals();

	// 2) the thread (wait() returns at once when this is called on that thread : the default shutdown ends there)
	this->m_is_stop = true;
	{
		std::lock_guard lock(this->m_lockable_cv);
		this->m_cv.notify_all();
	}

	if (auto pthread = std::move(this->m_pthread); pthread.exist())
	{
		pthread->wait();
	}

	// 3) the emergency acceptor (its executors are stopped by its stop())
	own_ptr<Nacceptor> pacceptor;
	{
		std::lock_guard lock(this->m_lockable_acceptor);
		pacceptor = std::move(this->m_pacceptor);
	}

	if (pacceptor.exist())
	{
		// trace)
		LOG_INFO << "  - [emergency manager] stop the emergency acceptor ['"sv << pacceptor->name() << "']"sv;

		// - stop & destroy
		pacceptor->stop();
		pacceptor.destroy();
	}

	// 4) the destroy handler (a no-op when this is called by it)
	system_executor::unregister_destroy_handler(DESTROY_HANDLER_NAME);

	// trace)
	LOG_INFO << "  - [emergency manager] uninstalled"sv;
}

void net::emergency::manager::process_install_signals() noexcept
{
	auto install_role = [this](const std::vector<int>& _signals, eSIGNAL _role) noexcept
	{
		for (auto signal_number : _signals)
		{
			// check)
			if (signal_number <= 0 || signal_number >= MAX_SIGNAL)
			{
				LOG_WARNING << "(warn) [emergency manager] invalid signal number "sv << signal_number << " for '"sv << _to_string(_role) << "'"sv;
				continue;
			}

			// 1) the role (before the handler : the handler reads it)
			m_s_signal_role[signal_number].store(static_cast<int>(_role), std::memory_order_release);

			// 2) the handler (the previous one is kept : restored by uninstall)
			SIGNAL_SAVED saved;
			saved.signal = signal_number;

		#if defined(_WIN32)
			saved.handler_old = std::signal(signal_number, &manager::_fn_signal);

			if (saved.handler_old == SIG_ERR)
			{
				m_s_signal_role[signal_number].store(0, std::memory_order_release);
				LOG_WARNING << "(warn) [emergency manager] fail to install the handler of signal "sv << signal_number << " for '"sv << _to_string(_role) << "'"sv;
				continue;
			}
		#else
			struct sigaction action;
			memset(&action, 0, sizeof(action));
			action.sa_handler = &manager::_fn_signal;
			action.sa_flags = SA_RESTART;
			sigemptyset(&action.sa_mask);

			if (::sigaction(signal_number, &action, &saved.action_old) != 0)
			{
				m_s_signal_role[signal_number].store(0, std::memory_order_release);
				LOG_WARNING << "(warn) [emergency manager] fail to install the handler of signal "sv << signal_number << " for '"sv << _to_string(_role) << "' (errno:"sv << errno << ")"sv;
				continue;
			}
		#endif

			this->m_vector_signal_saved.push_back(saved);

			// trace)
			LOG_INFO << "  - [emergency manager] signal "sv << signal_number << " => '"sv << _to_string(_role) << "'"sv;
		}
	};

	install_role(this->m_setting.signals_shutdown, eSIGNAL::SHUTDOWN);
	install_role(this->m_setting.signals_emergency, eSIGNAL::EMERGENCY);
	install_role(this->m_setting.signals_terminate, eSIGNAL::TERMINATE);
}

void net::emergency::manager::process_uninstall_signals() noexcept
{
	for (auto& saved : this->m_vector_signal_saved)
	{
		// 1) the previous handler back
	#if defined(_WIN32)
		std::signal(saved.signal, (saved.handler_old != SIG_ERR) ? saved.handler_old : SIG_DFL);
	#else
		::sigaction(saved.signal, &saved.action_old, nullptr);
	#endif

		// 2) no role
		if (saved.signal > 0 && saved.signal < MAX_SIGNAL)
			m_s_signal_role[saved.signal].store(0, std::memory_order_release);
	}

	this->m_vector_signal_saved.clear();
}

void net::emergency::manager::_fn_signal(int _signal)
{
	// - async-signal-safe : atomics only. the thread of the manager does the work.
	if (_signal > 0 && _signal < MAX_SIGNAL)
	{
		const auto role = m_s_signal_role[_signal].load(std::memory_order_acquire);

		if (role != 0)
			m_s_signal_pending.fetch_or(static_cast<uint32_t>(role), std::memory_order_release);
	}

#if defined(_WIN32)
	// - windows resets the handler to SIG_DFL before it is called : install it again
	std::signal(_signal, &manager::_fn_signal);
#endif
}


//-----------------------------------------------------------------------------
// the thread
//-----------------------------------------------------------------------------
void net::emergency::manager::process_thread() noexcept
{
	// trace)
	LOG_INFO << "  - [emergency manager] thread started"sv;

	// - the priority of this thread (it creates the emergency acceptor : it must run on a saturated system)
	if (this->m_setting.thread_priority != eTHREAD_PRIORITY::DEFAULT)
	{
		if (api::set_current_thread_priority(this->m_setting.thread_priority))
			LOG_INFO << "  - [emergency manager] thread priority : "sv << to_string(this->m_setting.thread_priority);
		else
			LOG_WARNING << "(warn) [emergency manager] fail to set the thread priority '"sv << to_string(this->m_setting.thread_priority) << "' (no permission?)"sv;
	}

	while (this->m_is_stop.load() == false)
	{
		// 1) wait (a signal handler does not notify : the timeout picks the flag up)
		{
			std::unique_lock lock(this->m_lockable_cv);

			this->m_cv.wait_for(lock, this->m_setting.interval_supervise, [this]() { return this->m_is_stop.load() || m_s_signal_pending.load(std::memory_order_acquire) != 0; });
		}

		// check)
		BREAK_IF(this->m_is_stop.load());

		try
		{
			// 2) the signals
			const auto pending = m_s_signal_pending.exchange(0, std::memory_order_acq_rel);

			if (pending & static_cast<uint32_t>(eSIGNAL::TERMINATE))
			{
				this->process_signal_terminate();	// - the process ends here (unless exit_process_on_terminate is false)
			}

			if (pending & static_cast<uint32_t>(eSIGNAL::SHUTDOWN))
			{
				this->process_signal_shutdown();
			}
			else if (pending & static_cast<uint32_t>(eSIGNAL::EMERGENCY))
			{
				this->process_signal_emergency();
			}

			// 3) the emergency acceptor
			this->process_supervise();
		}
		catch (...)
		{
			// log) never ends on an exception
			LOG_ERROR << "(excp) [emergency manager] exception on the thread ("sv << __FUNCTION__ << ")"sv;
		}
	}

	// trace)
	LOG_INFO << "  - [emergency manager] thread ended"sv;
}

void net::emergency::manager::process_signal_shutdown() noexcept
{
	// check) once
	if (this->m_is_shutdown_requested.exchange(true))
	{
		LOG_WARNING << "  ! [emergency manager] shutdown signal : requested already"sv;
		return;
	}

	// trace)
	LOG_WARNING << log_level_t::important() << "  ! [emergency manager] shutdown signal : stop and destroy the service"sv;

	if (this->m_setting.fn_shutdown)
	{
		// 1) the application stops its service (ex. g_pservice->stop(); g_pservice.destroy();)
		try
		{
			this->m_setting.fn_shutdown();
		}
		catch (...)
		{
			LOG_ERROR << "(excp) [emergency manager] exception on 'fn_shutdown' ("sv << __FUNCTION__ << ")"sv;
		}
	}
	else
	{
		// 2) default : destroy the system executor (this manager is uninstalled there too) and end the process normally
		LOG_WARNING << "  ! [emergency manager] no 'fn_shutdown' : system_executor::destroy_instance() and exit"sv;

		system_executor::destroy_instance();

		std::exit(EXIT_SUCCESS);
	}
}

void net::emergency::manager::process_signal_emergency() noexcept
{
	// check) created already?
	{
		std::lock_guard lock(this->m_lockable_acceptor);

		if (this->m_pacceptor.exist())
		{
			LOG_WARNING << "  ! [emergency manager] emergency signal : the emergency acceptor exists already ['"sv << this->m_pacceptor->name() << "']"sv;
			return;
		}
	}

	// check) how to create?
	if (!this->m_setting.fn_alloc_acceptor)
	{
		LOG_ERROR << log_level_t::important() << "(err ) [emergency manager] emergency signal : no emergency acceptor is set (install<TSOCKET>() or SETTING::fn_alloc_acceptor)"sv;
		return;
	}

	// 1) create
	own_ptr<Nacceptor> pacceptor;

	try
	{
		pacceptor = this->m_setting.fn_alloc_acceptor();
	}
	catch (...)
	{
		LOG_ERROR << log_level_t::important() << "(excp) [emergency manager] exception on creating the emergency acceptor ("sv << __FUNCTION__ << ")"sv;
		return;
	}

	// check)
	if (pacceptor.empty())
	{
		LOG_ERROR << log_level_t::important() << "(err ) [emergency manager] fail to create the emergency acceptor"sv;
		return;
	}

	// 2) set
	pacceptor->executor_thread_count(this->m_setting.acceptor_executor_thread_count);
	pacceptor->executor_thread_priority(this->m_setting.thread_priority);

	{
		std::lock_guard lock(this->m_lockable_acceptor);
		this->m_pacceptor = std::move(pacceptor);
	}

	// trace)
	LOG_WARNING << log_level_t::important() << "  ! [emergency manager] emergency signal : the emergency acceptor is created ['"sv << this->m_pacceptor->name() << "']"sv;

	// 3) start (retried by process_supervise() on a failure)
	this->process_start_acceptor();
}

void net::emergency::manager::process_signal_terminate() noexcept
{
	// trace)
	LOG_FATAL << log_level_t::important() << "  ! [emergency manager] terminate signal : dump, close all connections, exit"sv;

	// 1) dump
	if (this->m_setting.enable_dump_on_terminate)
	{
		try
		{
			snap_dump();
		}
		catch (...)
		{
			LOG_ERROR << "(excp) [emergency manager] exception on snap_dump ("sv << __FUNCTION__ << ")"sv;
		}
	}

	// 2) close every connection (abortive)
	if (this->m_setting.enable_close_connections_on_terminate)
	{
		this->process_close_all_connections();
	}

	// 3) the application
	if (this->m_setting.fn_terminate)
	{
		try
		{
			this->m_setting.fn_terminate();
		}
		catch (...)
		{
			LOG_ERROR << "(excp) [emergency manager] exception on 'fn_terminate' ("sv << __FUNCTION__ << ")"sv;
		}
	}

	// 4) end the process (no destructor runs)
	if (this->m_setting.exit_process_on_terminate)
	{
		// trace)
		LOG_FATAL << log_level_t::important() << "  ! [emergency manager] the process is terminated (exit code:"sv << this->m_setting.exit_code_on_terminate << ")"sv;

		// - a moment for the log
		api::sleep_for(std::chrono::milliseconds(200));

		// - exit
		std::_Exit(this->m_setting.exit_code_on_terminate);
	}
}

void net::emergency::manager::process_close_all_connections() noexcept
{
	// 1) the connectives (acceptors, connectors)
	auto pmanager_connective = net::io::connective::manager::instance();

	// check)
	RETURN_IF(pmanager_connective.empty());

	// 2) the list lock : a jammed thread may hold it => try for a while, then give up
	auto& lockable = pmanager_connective->get_lockable();
	bool is_locked = false;

	for (int i = 0; i < 1000 && is_locked == false; ++i)
	{
		is_locked = lockable.try_lock_shared();

		if (is_locked == false)
			api::sleep_for(std::chrono::milliseconds(1));
	}

	// check)
	if (is_locked == false)
	{
		LOG_ERROR << "(err ) [emergency manager] fail to lock the connective list : the connections are not closed"sv;
		return;
	}

	// 3) hold the connectives
	std::vector<object_ptr<net::io::Iconnective>> vector_connective;

	for (auto& iter : *pmanager_connective)
	{
		// check)
		CONTINUE_IF(iter == nullptr);

		// - hold (skip the one being destroyed)
		if (iter->add_ref_if_alive())
		{
			vector_connective.push_back(object_ptr<net::io::Iconnective>(iter));
			iter->release();
		}
	}

	lockable.unlock_shared();

	// 4) close
	std::size_t count_closed = 0;

	for (auto& pconnective : vector_connective)
	{
		try
		{
			count_closed += pconnective->count();
			pconnective->close_connectable_all();
		}
		catch (...)
		{
		}
	}

	// trace)
	LOG_FATAL << log_level_t::important() << "  ! [emergency manager] "sv << count_closed << " connections of "sv << vector_connective.size() << " connectives are closed"sv;
}

bool net::emergency::manager::process_start_acceptor() noexcept
{
	// 1) the acceptor
	auto pacceptor = acceptor();

	// check)
	RETURN_IF(pacceptor.empty(), false);

	try
	{
		// 2) start
		if (this->m_setting.fn_start_acceptor)
		{
			auto result = this->m_setting.fn_start_acceptor(pacceptor.get());

			THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result, 0, "fail to start the emergency acceptor"sv));
		}
		else
		{
			pacceptor->start();
		}

		// check) listening?
		if (pacceptor->is_listening() == false)
		{
			LOG_ERROR << log_level_t::important() << "(err ) [emergency manager] the emergency acceptor is not listening ['"sv << pacceptor->name() << "']. retry in "sv << std::chrono::duration_cast<std::chrono::seconds>(this->m_setting.interval_restart).count() << "s"sv;
			this->m_tick_restart = chrono::tick::now() + this->m_setting.interval_restart;
			return false;
		}

		// trace)
		LOG_WARNING << log_level_t::important() << "  ! [emergency manager] the emergency acceptor is listening ['"sv << pacceptor->name() << "' "sv << to_string(pacceptor->local_endpoint()) << "]"sv;

		this->m_tick_restart = chrono::tick::time_point{};
	}
	catch (...)
	{
		// log)
		LOG_ERROR << log_level_t::important() << "(excp) [emergency manager] fail to start the emergency acceptor ['"sv << pacceptor->name() << "']. retry in "sv << std::chrono::duration_cast<std::chrono::seconds>(this->m_setting.interval_restart).count() << "s"sv;

		// - retry later
		this->m_tick_restart = chrono::tick::now() + this->m_setting.interval_restart;

		// return)
		return false;
	}

	// return)
	return true;
}

void net::emergency::manager::process_supervise() noexcept
{
	// 1) the acceptor
	auto pacceptor = acceptor();

	// check)
	RETURN_IF(pacceptor.empty());

	// 2) not listening? => restart (with the interval)
	const bool is_listening = (pacceptor->get_object_state() == eOBJECT_STATE::RUNNING) && pacceptor->is_listening();

	if (is_listening == false)
	{
		if (chrono::tick::now() >= this->m_tick_restart)
		{
			// trace)
			LOG_WARNING << "  ! [emergency manager] the emergency acceptor is not listening : restart ['"sv << pacceptor->name() << "']"sv;

			// - stop
			if (pacceptor->get_object_state() != eOBJECT_STATE::STOPPED)
				pacceptor->stop();

			// - start
			this->process_start_acceptor();
		}
	}

	// 3) supervise (the executors of the disconnected sockets, the lost accepts)
	pacceptor->supervise();
}
