//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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

namespace api
{

template<class TREP, class TPERIOD>
inline void				sleep_for(const std::chrono::duration<TREP, TPERIOD>& _tick) noexcept {std::this_thread::sleep_for(_tick);}
template <class TCLOCK, class TDURATION>
inline void				sleep_until(const std::chrono::time_point<TCLOCK, TDURATION>& _time) noexcept { std::this_thread::sleep_until(_time); }
inline void				sleep_until(const chrono::time::client::time_point& _time) noexcept { std::this_thread::sleep_until(_time.timepoint_cast<chrono::time::time_point>()); }

object_ptr<Nthread>		get_current_thread() noexcept;
int						get_current_pid() noexcept;

}

//-----------------------------------------------------------------------------
/**

 @class		Nthread

*///-----------------------------------------------------------------------------
class api::Nthread :
	virtual public				object::Inameable,
	virtual public				Ireferenceable,
	private						non_copyable
{
public:
			Nthread() noexcept : m_is_blocked(false), m_pexecutor(nullptr) {}
	virtual	~Nthread() noexcept;

// public)
public:
			// - starts the thread (throws when this object already has a thread : call wait() or stop() first)
			//   if _function takes std::stop_token as the first parameter, the stop_token of the thread is passed
			template<class TFN, class... TARGS>
			void				begin_thread(TFN&& _function, TARGS&&... _arguments);

			bool				request_stop() noexcept;
			bool				wait() noexcept;
			bool				stop() noexcept;
			uint32_t			suspend();
			uint32_t			resume();

			// - force raises an exception in this thread (Windows: an APC that catches its own exception : no visible effect, Linux: not supported, false)
			//   (DANGEROUS: may cause memory leaks or data corruption if called during lock acquisition)
			bool				force_raise_exception(const std::string_view& _message = "Forced thread termination") noexcept;

	[[nodiscard]] auto			get_last_execute() const noexcept { return m_statistics_thread.tick_last_execute.load();}
	[[nodiscard]] const auto&	get_thread_statistics() const noexcept { return m_statistics_thread;}
	[[nodiscard]] bool			is_time_out(chrono::tick::duration _tick_differ_time) const noexcept;
	[[nodiscard]] bool			is_time_out(chrono::tick::duration _tick_differ_time, chrono::tick::time_point _tick_now) const noexcept;
			void				reset() noexcept;

	[[nodiscard]] const auto&	statistics() const noexcept { return m_statistics_thread; }
			void				statistics_executing_begin(chrono::tick::time_point _tick) noexcept;
			void				statistics_executing_begin() noexcept;
			void				statistics_executing_end() noexcept;
			void				statistics_error() noexcept;
private:
	template <class TTYPE>
	static	void				fn_process_thread(api::Nthread* _pthread, TTYPE* _functor, std::stop_token _stop_token);
	template <class TTYPE>
	static	void				fn_process_thread_stoppable(api::Nthread* _pthread, TTYPE* _functor, std::stop_token _stop_token);
	template <class TFN, class TTYPE>
			void				_start_thread(TFN _fn_thread, TTYPE* _functor);
			void				_on_thread_started();
			void				_close_control_handle() noexcept;
	static	void				process_set_exception_handler_to_thread() noexcept;

			void				register_thread();
			void				unregister_thread() noexcept;
			void				process_begin_thread();
			void				process_end_thread();
public:
			void				notify_blocked() noexcept;
			void				notify_unblocked() noexcept;

private:
	mutable	lock_object			m_lockable_pmanager;
			object_ptr<api::manager_thread> m_pmanager;
	friend	class api::manager_thread;
	friend	class api::threads;

	// - m_thread : start / join / detach (held while joining, so only one thread joins at a time)
	mutable	std::mutex			m_lockable_thread;
			std::jthread		m_thread;

	// - copies of the controls of m_thread (usable while another thread is joining m_thread)
	mutable	std::mutex			m_lockable_control;
			std::stop_source	m_stop_source{ std::nostopstate };	// created by begin_thread() (no stop state until then)
#if defined(_WIN32)
			HANDLE				m_handle_control = nullptr;		// duplicated thread handle for suspend() / resume()
#endif

			std::atomic<bool>	m_is_deadlock_reported{ false };	// api::manager_thread : reported once per deadlock

			statistics::thread	m_statistics_thread;

public:
	mutable	lock_object			m_lockable_executor;
			bool				m_is_blocked;
			Iexecutor*			m_pexecutor;
};


template <class TTYPE>
void api::Nthread::fn_process_thread(api::Nthread* _pthread, TTYPE* _pfunctor, std::stop_token /*_stop_token*/)
{
	// 1) Hold
	object_ptr<api::Nthread> pthread = _pthread;

	// 2) pre process
	pthread->process_begin_thread();

	// 3) cal functor
	try
	{
		(*_pfunctor)();
	}
	catch (...)
	{
	}

	// 4) delete functor
	delete _pfunctor;

	// 5) post process
	pthread->process_end_thread();

	// 6) release the reference added by _start_thread()
	_pthread->release();
}

template <class TTYPE>
void api::Nthread::fn_process_thread_stoppable(api::Nthread* _pthread, TTYPE* _pfunctor, std::stop_token _stop_token)
{
	// 1) Hold
	object_ptr<api::Nthread> pthread = _pthread;

	// 2) pre process
	pthread->process_begin_thread();

	// 3) cal functor (with the stop_token of this thread)
	try
	{
		(*_pfunctor)(std::move(_stop_token));
	}
	catch (...)
	{
	}

	// 4) delete functor
	delete _pfunctor;

	// 5) post process
	pthread->process_end_thread();

	// 6) release the reference added by _start_thread()
	_pthread->release();
}

template <class TFN, class TTYPE>
void api::Nthread::_start_thread(TFN _fn_thread, TTYPE* _pfunctor)
{
	// lock)
	std::lock_guard lock(this->m_lockable_thread);

	// check) already has a thread (running, or ended but not joined yet)
	if (this->m_thread.joinable())
	{
		delete _pfunctor;
		throw throwable::failure(eRESULT::ALREADY_EXIST, 0, std::string_view("thread is already started (call wait() or stop() first)"));
	}

	// 1) reset deadlock report state
	this->m_is_deadlock_reported = false;

	// 2) stop_source of this thread
	//    - created before the thread starts : the thread may call request_stop() on itself at once
	//    - its token is passed to the thread function (the stop_source of std::jthread is not used)
	std::stop_token stop_token;
	{
		std::lock_guard lock_control(this->m_lockable_control);
		this->m_stop_source = std::stop_source();
		stop_token = this->m_stop_source.get_token();
	}

	// 3) add reference count (released by the thread function when it ends)
	this->add_ref();

	try
	{
		// 4) start thread
		this->m_thread = std::jthread(_fn_thread, this, _pfunctor, std::move(stop_token));
	}
	catch(...)
	{
		// - release
		release();

		// - delete
		delete	_pfunctor;

		// reraise)
		throw;
	}

	// 5) control handle / statistics
	this->_on_thread_started();
}

template<class TFN, class... TARGS>
void api::Nthread::begin_thread(TFN&& _function, TARGS&&... _arguments)
{
	if constexpr (std::is_invocable_v<std::decay_t<TFN>&, std::stop_token, std::decay_t<TARGS>&...>)
	{
		// declare) the function takes the stop_token of the thread as the first parameter
		using TFUNCTOR = decltype(std::bind(DECAY_COPY(std::forward<TFN>(_function)), std::placeholders::_1, DECAY_COPY(std::forward<TARGS>(_arguments))...));

		// 1) create functor
		auto* pfunctor = new TFUNCTOR(std::bind(DECAY_COPY(std::forward<TFN>(_function)), std::placeholders::_1, DECAY_COPY(std::forward<TARGS>(_arguments))...));

		// 2) start thread
		this->_start_thread(fn_process_thread_stoppable<TFUNCTOR>, pfunctor);
	}
	else
	{
		// declare)
		using TFUNCTOR = decltype(std::bind(DECAY_COPY(std::forward<TFN>(_function)), DECAY_COPY(std::forward<TARGS>(_arguments))...));

		// 1) create functor
		auto* pfunctor = new TFUNCTOR(std::bind(DECAY_COPY(std::forward<TFN>(_function)), DECAY_COPY(std::forward<TARGS>(_arguments))...));

		// 2) start thread
		this->_start_thread(fn_process_thread<TFUNCTOR>, pfunctor);
	}
}

class api::thread :
	public						Nthread,
	public						object::Nnameable,
	public						factory::_traits_system
{
public:
			thread()  noexcept {}
	virtual	~thread() noexcept {}
};


}
