//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#if !defined(_WIN32)
	#include <signal.h>
#endif

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::emergency::manager
//
//  the signals of a server process (one per process)
//
//  1) shutdown signal  : stops and destroys the service normally.
//     - SETTING::fn_shutdown : the application stops its service there.
//       ex. [](){ g_pservice->stop(); g_pservice.destroy(); } (the main loop
//       waiting on wait_destroy() ends, main() returns, the process ends)
//     - default (no fn_shutdown) : system_executor::destroy_instance() and
//       std::exit(EXIT_SUCCESS). (called on the thread of the manager)
//
//  2) emergency signal : creates and starts the emergency acceptor
//     (net::emergency::acceptor<TSOCKET>, see install<TSOCKET>()) on its own
//     executor. afterwards the acceptor is supervised : restarted when it is
//     not listening, the executors of the disconnected sockets are removed.
//     the acceptor and its executors are stopped only on the normal destroy
//     of the system_executor (system_executor::destroy_instance(), or the
//     end of the process). a second emergency signal is ignored.
//
//  3) terminate signal : writes a dump (snap_dump()), closes every connection
//     of every acceptor/connector abortively, calls SETTING::fn_terminate,
//     then ends the process at once (std::_Exit : no destructor runs).
//
//  * the signal handlers only raise a flag : the work is done by the thread
//    of the manager. (its own thread : it works while every executor is
//    jammed. the flag is polled every SETTING::interval_supervise)
//  * trigger() does the same as a signal from code. (an admin command, a
//    named event on windows ...)
//  * default signals
//       linux   : shutdown = SIGTERM, SIGINT   emergency = SIGUSR1          terminate = SIGUSR2
//       windows : shutdown = SIGINT (ctrl+c)   emergency = SIGBREAK (ctrl+break)   terminate = SIGTERM (raise() only)
//
//  ex)
//      net::emergency::manager::SETTING setting;
//      setting.fn_shutdown = []() { g_pservice->stop(); g_pservice.destroy(); };
//      net::emergency::manager::install<admin_socket>({ { net::ip::tcp::v6(), 30000 } }, std::move(setting));
//      ...
//      system_executor::destroy_instance();   // the emergency acceptor is stopped here (normal termination)
//
//-----------------------------------------------------------------------------
class net::emergency::manager :
	public						object::Nnameable,
	virtual public				object::Idestroyable,
	private						non_copyable
{
// definitions)
public:
			enum class eSIGNAL : uint32_t
			{
				NONE			= 0,
				SHUTDOWN		= 0x01,
				EMERGENCY		= 0x02,
				TERMINATE		= 0x04,
			};

			struct SETTING
			{
				SETTING();

				// 1) the signals of each role (empty : the role has no signal. trigger() only)
				std::vector<int> signals_shutdown;
				std::vector<int> signals_emergency;
				std::vector<int> signals_terminate;

				// 2) shutdown : stops and destroys the service. (default : system_executor::destroy_instance() + std::exit(EXIT_SUCCESS))
				std::function<void()> fn_shutdown;

				// 3) terminate
				bool			enable_dump_on_terminate = true;				// snap_dump()
				bool			enable_close_connections_on_terminate = true;	// closes every connection (abortive)
				std::function<void()> fn_terminate;								// called after the dump and the close, before the process ends
				bool			exit_process_on_terminate = true;				// std::_Exit(exit_code_on_terminate)
				int				exit_code_on_terminate = EXIT_FAILURE;

				// 4) the emergency acceptor (install<TSOCKET>() sets them)
				std::function<own_ptr<Nacceptor>()> fn_alloc_acceptor;
				std::function<result_code(Nacceptor*)> fn_start_acceptor;
				int				acceptor_executor_thread_count = 1;

				// 4-1) the priority of the thread of the manager (it creates the emergency acceptor) and of the threads of the
				//    executors of the emergency acceptor and its sockets. default : the top (they must run on a saturated system)
				eTHREAD_PRIORITY thread_priority = eTHREAD_PRIORITY::TIME_CRITICAL;

				// 5) supervising
				chrono::tick::duration interval_supervise = std::chrono::milliseconds(100);	// - the poll of the signal flags, Nacceptor::supervise()
				chrono::tick::duration interval_restart = std::chrono::seconds(3);			// - the retry of a failed start of the acceptor
			};

	static constexpr int		MAX_SIGNAL = 128;

// constructor/destructor)
protected:
			manager();
	virtual	~manager() noexcept;

// publics)
public:
	// 1) instance
	static	auto				instance() { auto p = m_s_pinstance.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_pinstance.get_acquire(); } return object_ptr<manager>(p); }
	static	void				initialize_instance();
	static	void				reset_instance() noexcept;

	// 2) install (the signal handlers, the thread) / uninstall (the handlers back, the thread, the emergency acceptor)
	static	void				install(SETTING&& _setting);
			template <class TSOCKET>
	static	void				install(const Nacceptor::START_PARAMETER& _parameter, SETTING&& _setting = SETTING());
			template <class TSOCKET>
	static	void				install(const context& _context, SETTING&& _setting = SETTING());
	static	void				uninstall() noexcept;
	[[nodiscard]] static bool	is_installed() noexcept;

	// 3) trigger : the same as the signal (from code)
	static	void				trigger(eSIGNAL _signal) noexcept;

	// 4) the emergency acceptor (empty until the emergency signal)
	[[nodiscard]] static object_ptr<Nacceptor> acceptor() noexcept;

	// 5) destroy
	virtual	result_code			destroy() noexcept override;

// implementation)
private:
			struct SIGNAL_SAVED
			{
				int				signal = 0;
			#if defined(_WIN32)
				void			(*handler_old)(int) = nullptr;
			#else
				struct sigaction action_old {};
			#endif
			};

			void				process_install(SETTING&& _setting);
			void				process_uninstall() noexcept;
			void				process_install_signals() noexcept;
			void				process_uninstall_signals() noexcept;
			void				process_thread() noexcept;
			void				process_signal_shutdown() noexcept;
			void				process_signal_emergency() noexcept;
			void				process_signal_terminate() noexcept;
			void				process_close_all_connections() noexcept;
			bool				process_start_acceptor() noexcept;
			void				process_supervise() noexcept;
	static	void				_fn_signal(int _signal);

			SETTING				m_setting;
			std::atomic<bool>	m_is_installed{ false };
			std::atomic<bool>	m_is_stop{ false };
			std::atomic<bool>	m_is_shutdown_requested{ false };
			std::mutex			m_lockable_cv;
			std::condition_variable	m_cv;
			object_ptr<api::thread>	m_pthread;
	mutable	lock_object			m_lockable_acceptor;
			own_ptr<Nacceptor>	m_pacceptor;
			chrono::tick::time_point m_tick_restart{};
			std::vector<SIGNAL_SAVED> m_vector_signal_saved;

	// - read by the signal handlers (async-signal-safe : atomics only)
	static	std::atomic<uint32_t> m_s_signal_pending;
	static	std::atomic<int>	m_s_signal_role[MAX_SIGNAL];

	static	std::mutex			m_s_lockable_create;
	static	own_ptr<manager>	m_s_pinstance;
};

template <class TSOCKET>
void net::emergency::manager::install(const Nacceptor::START_PARAMETER& _parameter, SETTING&& _setting)
{
	// 1) the emergency acceptor : created on the emergency signal
	_setting.fn_alloc_acceptor = []() -> own_ptr<Nacceptor> { return make_own<net::emergency::acceptor<TSOCKET>>(); };
	_setting.fn_start_acceptor = [parameter = _parameter](Nacceptor* _pacceptor) -> result_code { return _pacceptor->start(parameter); };

	// 2) install
	install(std::move(_setting));
}

template <class TSOCKET>
void net::emergency::manager::install(const context& _context, SETTING&& _setting)
{
	// 1) the emergency acceptor : created on the emergency signal
	_setting.fn_alloc_acceptor = []() -> own_ptr<Nacceptor> { return make_own<net::emergency::acceptor<TSOCKET>>(); };
	_setting.fn_start_acceptor = [context_start = context(_context)](Nacceptor* _pacceptor) -> result_code { return _pacceptor->start(context_start); };

	// 2) install
	install(std::move(_setting));
}


}
