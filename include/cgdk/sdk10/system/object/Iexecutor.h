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

#if defined(__cpp_impl_coroutine)
namespace CGDK::coroutine
{
	template <class TRESULT>
	class task;

	namespace detail
	{
		// 1) the implementation of Iexecutor::post() / execute() and system_executor::post() / execute() with a coroutine (coroutine.post.h)
		template <class TRESULT>
		struct post_impl;
		template <class TCALLABLE>
		struct post_impl_auto;
		template <class TCALLABLE>
		struct execute_impl_auto;

		// 2) the argument of post() / execute(): a coroutine::task<T> (an rvalue: moved) or a callable without parameters
		//    (an executable, a duration or a time point is not one: the other overloads of post() / execute() take them)
		template <class>
		inline constexpr bool is_task_argument_v = false;
		template <class TRESULT>
		inline constexpr bool is_task_argument_v<task<TRESULT>> = true;

		template <class TCALLABLE>
		inline constexpr bool is_post_callable_v = is_task_argument_v<std::remove_cvref_t<TCALLABLE>> || std::is_invocable_v<TCALLABLE>;
	}
}
#endif

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		Iexecutor
 

*///-----------------------------------------------------------------------------
class thread_list
{
public:
	~thread_list() { CGASSERT_ERROR(this->m_vector_thread.empty()); }

public:
	using ITERATOR = std::vector<object_ptr<api::thread>>::iterator;
	using CONST_ITERATOR = std::vector<object_ptr<api::thread>>::const_iterator;

			auto				begin() { return this->m_vector_thread.begin();}
			auto				begin() const { return this->m_vector_thread.begin();}
			auto				end() { return this->m_vector_thread.end();}
			auto				end() const { return this->m_vector_thread.end();}
			auto&				get_threads() noexcept { return this->m_vector_thread; }
			const auto&			get_lockable() const noexcept { return this->m_lockable_vector_thread; }
			auto				reserved_size() const noexcept { return this->m_vector_thread.size(); }
								operator const lock_object&() const noexcept { return this->get_lockable(); }
protected:
	mutable	lock_object			m_lockable_vector_thread;
			std::vector<object_ptr<api::thread>> m_vector_thread;
};

class Iexecutor : 
	virtual public				thread_list,
	virtual public				object::Iidentifiable,
	virtual public				object::Inameable,
	virtual public				object::Istartable
{
public:
	virtual ~Iexecutor() noexcept {}

public:
			bool				post(object_ptr<Iexecutable>&& _pexecutable) { return this->process_post(std::move(_pexecutable)); }
			bool				post(const object_ptr<Iexecutable>& _pexecutable) { return this->process_post(object_ptr<Iexecutable>(_pexecutable)); }
			bool				execute(chrono::tick::duration _tick_diff, intptr_t _option = 0) { return this->process_execute(_tick_diff, _option); }
			void				thread_blocked() noexcept { this->process_control_thread_blocked(); }
			void				thread_unblocked() noexcept { this->process_control_thread_unblocked(); }
	[[nodiscard]] const auto&	statistics() const noexcept { return this->m_statistics;}

#if defined(__cpp_impl_coroutine)
	// - post: starts a coroutine::task (or a function, wrapped in a coroutine) on this executor without waiting (coroutine.post.h)
	template <class TCALLABLE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(TCALLABLE&& _callable) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(this), nullptr, std::forward<TCALLABLE>(_callable)); }

	template <class TCALLABLE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(Ireferenceable* _powner, TCALLABLE&& _callable) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(this), _powner, std::forward<TCALLABLE>(_callable)); }

	template <class TCALLABLE, class FCOMPLETE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								post(TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(this), nullptr, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }

	template <class TCALLABLE, class FCOMPLETE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								post(Ireferenceable* _powner, TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(this), _powner, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }

	// - execute: runs the task right here until it suspends for the first time, then continues on this executor (coroutine.post.h)
	template <class TCALLABLE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								execute(TCALLABLE&& _callable) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(this), nullptr, std::forward<TCALLABLE>(_callable)); }

	template <class TCALLABLE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								execute(Ireferenceable* _powner, TCALLABLE&& _callable) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(this), _powner, std::forward<TCALLABLE>(_callable)); }

	template <class TCALLABLE, class FCOMPLETE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								execute(TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(this), nullptr, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }

	template <class TCALLABLE, class FCOMPLETE>
			std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								execute(Ireferenceable* _powner, TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(this), _powner, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }
#endif

protected:
	virtual	bool				process_post(object_ptr<Iexecutable>&& _pexecutable) PURE;
	virtual	bool				process_execute(chrono::tick::duration _ticks_wait, intptr_t _option) PURE;
	virtual	bool				process_control_thread_blocked() noexcept { return true; }
	virtual	bool				process_control_thread_unblocked() noexcept { return true; }
	virtual	void				process_idle() noexcept;
 			void				process_reset_statistics() noexcept;

			void				process_register_executor();
			void				process_unregister_executor() noexcept;

			lock_object			m_lockable_manager;
			object_ptr<executor::manager> m_pmanager;

			statistics::executor m_statistics;
			statistics::unit_executor_snap m_statistics_snap;

			friend class executor::manager;
			friend class system_executor;
};

class scoped_thread_block
{
public:
	scoped_thread_block();
	~scoped_thread_block() noexcept;
private:
	Iexecutor* m_pexecutor{ nullptr };
};

}
