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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::emergency::Nacceptor
//
//  an acceptor for emergencies : ex. an admin channel that must keep working
//  while the system_executor is jammed (all of its threads blocked).
//
//  1) the acceptor runs on its own executor (executor::make_io_context_threads() :
//     its own iocp/epoll and threads), never on the system_executor.
//     (the listen socket, the accepts)
//
//     the threads of the executors run at the top priority (TIME_CRITICAL, set by
//     each thread before its first execution. executor_thread_priority())
//
//  2) every accepted socket gets its own executor (1 thread) : the socket's
//     receives and sends are served there. the executor is stopped and removed
//     when the socket is disconnected. (a session stuck in a handler blocks
//     neither the acceptor nor the other sessions)
//
//  3) nothing here dies on an exception : an exception closes a socket at most.
//     the acceptor and its executors are stopped only by stop()/destroy().
//     (net::emergency::manager stops them on the normal destroy of the
//      system_executor)
//
//  4) supervise() must be called periodically (net::emergency::manager does) :
//     - removes the executors of the disconnected sockets
//     - windows : requests the accepts lost by an exception again
//       (the retry of acceptor::Ntcp runs on the system scheduler : not usable)
//
//  * the start parameters are those of net::acceptor<T> (connective::acceptor::Ntcp).
//    'prepare_on_start' / 'must_prepare' default to 2 / 1 here : every prepared
//    socket costs an executor (a thread).
//    (windows only : linux prepares no socket. an accepted socket gets its executor,
//     so socket_executor_count() is the count of the sockets connected or retired)
//
//-----------------------------------------------------------------------------
class net::emergency::Nacceptor : public net::io::connective::acceptor::Ntcp
{
// definitions)
public:
			using executor_t = executor::Iio_context_threads;

// constructor/destructor)
public:
			Nacceptor();
	virtual	~Nacceptor() noexcept;

// publics)
public:
	// 1) start
			result_code			start() { context _context; return this->start(_context); }
			result_code			start(const START_PARAMETER& _parameter);
			result_code			start(const context& _context) { context context_copy(_context); return this->start(context_copy); }
	virtual	result_code			start(context& _context) override;

	// 2) executors
	[[nodiscard]] auto			own_executor() const noexcept { std::lock_guard lock(this->m_lockable_executor); return this->m_pexecutor_own; }
			void				executor_thread_count(int _count) noexcept { this->m_count_thread_executor = (_count > 0) ? _count : 1; }
	[[nodiscard]] int			executor_thread_count() const noexcept { return this->m_count_thread_executor; }
			// - the priority of the threads of the executors (the acceptor's and the sockets'). default : TIME_CRITICAL (the top)
			//   set before start() : applied to the executors created afterwards
			void				executor_thread_priority(eTHREAD_PRIORITY _priority) noexcept { this->m_thread_priority = _priority; }
	[[nodiscard]] eTHREAD_PRIORITY executor_thread_priority() const noexcept { return this->m_thread_priority; }
	[[nodiscard]] std::size_t	socket_executor_count() const noexcept;

	// 3) supervise (periodic) : the executors of the disconnected sockets, the lost accepts
			void				supervise() noexcept;

// frameworks)
protected:
	// 4) connective::acceptor::Ntcp
	virtual	result_code			_process_starting(context& _context) override;
	virtual	result_code			_process_stop() noexcept override;
	virtual	result_code			_process_destroy() noexcept override;
	[[nodiscard]] virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable* _pconnectable) override;
	virtual	void				process_on_fail_accept(net::io::Iconnectable* _pconnectable) noexcept override;
	virtual	void				process_connective_disconnect(net::io::Iconnectable* _pconnectable) override;

// implementation)
private:
			struct SOCKET_EXECUTOR
			{
				object_ptr<net::io::Iconnectable> psocket;
				object_ptr<executor_t> pexecutor;
				chrono::tick::time_point tick{};			// - created (alive) / retired, stop requested (retired)
				bool			is_stop_requested = false;
				bool			is_warned = false;
			};

			object_ptr<executor_t> _alloc_socket_executor();
			object_ptr<executor_t> _make_executor(std::string_view _name, int _thread_count);
			void				_retire_socket_executor(net::io::Iconnectable* _pconnectable) noexcept;
			void				_reap_socket_executors(bool _wait_all) noexcept;
			void				_supervise_accept() noexcept;
			void				_stop_own_executor() noexcept;
	static	bool				_stop_executor(object_ptr<executor_t>& _pexecutor, chrono::tick::duration _wait) noexcept;

	mutable	lock_object			m_lockable_executor;
			object_ptr<executor_t> m_pexecutor_own;
			int					m_count_thread_executor = 1;
			eTHREAD_PRIORITY	m_thread_priority = eTHREAD_PRIORITY::TIME_CRITICAL;
			std::map<net::io::Iconnectable*, SOCKET_EXECUTOR> m_map_socket_executor;	// - the executors of the sockets alive
			std::vector<SOCKET_EXECUTOR> m_vector_retired;								// - to be stopped (by supervise())
			std::atomic<int>	m_serial_socket_executor{ 0 };
};


//-----------------------------------------------------------------------------
//
// net::emergency::acceptor<TSOCKET>
//
//  net::acceptor<TSOCKET> for emergencies. (see net::emergency::Nacceptor)
//
//  ex)
//      auto pacceptor = make_own<net::emergency::acceptor<admin_socket>>();
//      pacceptor->start({ { net::ip::tcp::v6(), 30000 } });
//
//-----------------------------------------------------------------------------
template <class TSOCKET>
class net::emergency::acceptor :
	public						net::emergency::Nacceptor,
	public						Nmessage_transmitter,
	public						net::io::connectable_manager::list<TSOCKET>
{
public:
			acceptor()
			{
				std::string temp_name("emergency acceptor<");
				temp_name += typeid(TSOCKET).name();
				temp_name += ">"sv;

				this->name(temp_name);
			}
	virtual	~acceptor() noexcept {}

protected:
	virtual	object_ptr<net::io::Iconnectable> process_alloc_connectable() override { return alloc_object<TSOCKET>(); }
	virtual result_code			_process_destroy() noexcept override { auto result = this->net::emergency::Nacceptor::_process_destroy(); this->reset_message_transmitter(); return result; }
};


}
