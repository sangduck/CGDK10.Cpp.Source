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
		// 1) the implementation of system_executor::post() / execute() with a coroutine (coroutine.post.h)
		template <class TRESULT>
		struct post_impl;
		template <class TCALLABLE>
		struct post_impl_auto;
		template <class TCALLABLE>
		struct execute_impl_auto;

		// 2) the owner of system_executor::post(): a pointer or an object_ptr of Ireferenceable (not an executor)
		//    - an executor is not an owner: 'system_executor::post(pstrand, foo())' is a compile error.
		//      (use 'pstrand->post(foo())'. object_ptr converts to a pointer implicitly)
		template <class T> struct post_owner_type { using type = void; };
		template <class T> struct post_owner_type<T*> { using type = T; };
		template <class T> struct post_owner_type<object_ptr<T>> { using type = T; };

		template <class TOWNER, class T = typename post_owner_type<std::remove_cvref_t<TOWNER>>::type>
		constexpr bool is_post_owner_v = !std::is_void_v<T>
			&& std::is_base_of_v<Ireferenceable, std::remove_cv_t<T>>
			&& !std::is_base_of_v<Iexecutor, std::remove_cv_t<T>>;

		template <class T> Ireferenceable* to_post_owner(T* _powner) noexcept { return _powner; }
		template <class T> Ireferenceable* to_post_owner(const object_ptr<T>& _powner) noexcept { return _powner.get(); }
	}
}
#endif

namespace CGDK
{//-----------------------------------------------------------------------------
/**

 @class		Default
 @brief		내부적으로 자동 생성되는 기본 executor이다.
 @todo		
 @ref		
 @details	\n
 내부적으로 사용되는 모든 실행처리에 사용되는 싱글턴 executor이다.
 내부적으로 다중 쓰레드 executor와 SingleExecutor와 schedulable_manager를 가지고 있다.
 기본적으로는 자동생성되나 초기화기 필요할 경우 initialize_instance()를 호출해 초기화할 수도 있다.
 실행요청은 post(...)함수로 가능하며 register_schedulable로 Scheduler에 등록할 수도 있다.
 소멸처리도 자동으로 되기 때문에 다른 처리를 해줄 필요도 없다.


  configure
			"thread_count" => "default", "auto", "no thread", number
			"enable_thread_observer" => "default", "enable", "disable"

*///-----------------------------------------------------------------------------
class system_executor : 
	public						object::Nnameable,
	public						object::Nstate,
	public						object::Ninitializable,
	public						Idisposable::Nno_act,
	public						factory::_traits_system,
	private						non_copyable
{
public:
	using schedulable_manager_t = schedulable::manager;
	using executable_scheduler_t = executor::scheduler;
	using factory_sustain_t = schedulable::system::factory_sustain;
	using thread_observer_t = schedulable::system::exception_observer;

protected:
			system_executor() noexcept;
	virtual ~system_executor() noexcept;

public:
	static	void				initialize_instance(int _count_thread = static_cast<int>(CGEXECUTOR_AUTO));
	static	void				initialize_instance(context& _context);
	static	void				destroy_instance() noexcept;
	static	void				detach_instance() noexcept;
	static	system_executor*	instance() { if(m_s_instance.m_pexecutable_scheduler.get_acquire() == nullptr) initialize_instance(); return &m_s_instance;}
	static	system_executor*	peek_instance() noexcept { if (m_s_instance.m_pexecutable_scheduler.get_acquire() == nullptr) return nullptr; return &m_s_instance; }

	static	auto				executor() { auto p = m_s_instance.m_psystem_executor.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_instance.m_psystem_executor.get_acquire(); } return object_ptr<executor::Iio_context>(p); }
	static	auto				schedulable_manager() { auto p = m_s_instance.m_pschedulable_manager.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_instance.m_pschedulable_manager.get_acquire(); } return object_ptr<schedulable_manager_t>(p); }
	static	auto				scheduler() { auto p = m_s_instance.m_pexecutable_scheduler.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_instance.m_pexecutable_scheduler.get_acquire(); } return object_ptr<executable_scheduler_t>(p); }
	[[nodiscard]] static size_t	thread_count() noexcept;					  
		
	static	void				run_executor();

	static	bool				start_factory_garbage_collection();
	static	bool				stop_factory_garbage_collection() noexcept;
	static	bool				start_thread_observer();
	static	bool				stop_thread_observer() noexcept;

	// - system executor의 raw pointer (참조 count 없이 쓴다 : post할 때마다 모든 thread가 같은 참조 count를 올리고 내리지 않도록)
	//   destroy되어도 객체는 해제하지 않고 system_executor가 소멸될 때까지 보관하므로(m_vector_executor_retired) 쓰는 중에 해제되지 않는다.
	//   (destroy된 뒤의 post는 handle이 닫혀 있어 false를 돌려준다)
	[[nodiscard]] static executor::Iio_context* executor_ptr() { auto p = m_s_instance.m_psystem_executor.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_instance.m_psystem_executor.get_acquire(); } return p; }
	[[nodiscard]] static executor::Iio_context* executor_ptr_peek() noexcept { return m_s_instance.m_psystem_executor.get_acquire(); }	// - 만들지 않는다. (비교할 때 : 종료 중에 다시 만들지 않도록)

	static	bool				post(object_ptr<Iexecutable>&& _pexecutable) { auto pexecutor = executor_ptr(); return (pexecutor != nullptr) ? pexecutor->post(std::move(_pexecutable)) : false; }
	static	bool				post(const object_ptr<Iexecutable>& _pexecutable) { return post(object_ptr<Iexecutable>(_pexecutable)); }

	static	bool				post(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute) { return scheduler()->post_at(std::move(_pexecutable), _tick_execute); }
	static	bool				post(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::duration _durationtick_after) { return scheduler()->post_at(std::move(_pexecutable), chrono::tick::now() + _durationtick_after); }
	static	bool				post(const object_ptr<Iexecutable>& _pexecutable, chrono::tick::time_point _tick_execute) { return post(object_ptr<Iexecutable>(_pexecutable), _tick_execute); }
	static	bool				post(const object_ptr<Iexecutable>& _pexecutable, chrono::tick::duration _durationtick_after) { return post(object_ptr<Iexecutable>(_pexecutable), _durationtick_after); }

#if defined(__cpp_impl_coroutine)
	// - post: starts a coroutine::task (or a function, wrapped in a coroutine) on the system_executor without waiting (coroutine.post.h)
	template <class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(TCALLABLE&& _callable) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(), nullptr, std::forward<TCALLABLE>(_callable)); }
	template <class TOWNER, class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_owner_v<TOWNER> && coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(const TOWNER& _powner, TCALLABLE&& _callable) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(), coroutine::detail::to_post_owner(_powner), std::forward<TCALLABLE>(_callable)); }
	template <class TCALLABLE, class FCOMPLETE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								post(TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(), nullptr, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }
	template <class TOWNER, class TCALLABLE, class FCOMPLETE>
	static	std::enable_if_t<coroutine::detail::is_post_owner_v<TOWNER> && coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								post(const TOWNER& _powner, TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(), coroutine::detail::to_post_owner(_powner), std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }

	// - execute: runs the task right here until it suspends for the first time, then continues on the system_executor (coroutine.post.h)
	template <class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								execute(TCALLABLE&& _callable) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(), nullptr, std::forward<TCALLABLE>(_callable)); }
	template <class TOWNER, class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_owner_v<TOWNER> && coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								execute(const TOWNER& _powner, TCALLABLE&& _callable) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(), coroutine::detail::to_post_owner(_powner), std::forward<TCALLABLE>(_callable)); }
	template <class TCALLABLE, class FCOMPLETE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								execute(TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(), nullptr, std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }
	template <class TOWNER, class TCALLABLE, class FCOMPLETE>
	static	std::enable_if_t<coroutine::detail::is_post_owner_v<TOWNER> && coroutine::detail::is_post_callable_v<TCALLABLE> && std::is_invocable_v<std::decay_t<FCOMPLETE>&, std::exception_ptr>, void>
								execute(const TOWNER& _powner, TCALLABLE&& _callable, FCOMPLETE&& _on_complete) { coroutine::detail::execute_impl_auto<TCALLABLE>::execute(object_ptr<Iexecutor>(), coroutine::detail::to_post_owner(_powner), std::forward<TCALLABLE>(_callable), std::forward<FCOMPLETE>(_on_complete)); }

	// - post with time scheduling: schedule coroutine task or function for later execution
	template <class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(TCALLABLE&& _callable, chrono::tick::time_point _tick_execute)
	{
		post(make_executable([_callable = std::forward<TCALLABLE>(_callable)]() mutable
		{
			coroutine::detail::post_impl_auto<TCALLABLE>::post(object_ptr<Iexecutor>(), nullptr, std::forward<TCALLABLE>(_callable));
		}), _tick_execute);
	}
	template <class TCALLABLE>
	static	std::enable_if_t<coroutine::detail::is_post_callable_v<TCALLABLE>, void>
								post(TCALLABLE&& _callable, chrono::tick::duration _duration)
	{
		post(std::forward<TCALLABLE>(_callable), chrono::tick::now() + _duration);
	}
#endif

	static	bool				register_schedulable(object_ptr<Ischedulable>&& _pschedulable);
	static	bool				register_schedulable(const object_ptr<Ischedulable>& _pschedulable) { return register_schedulable(object_ptr<Ischedulable>(_pschedulable)); }
	static	bool				unregister_schedulable(Ischedulable* _pschedulable) noexcept;

	// - destroy handlers : called at the beginning of the (normal) destroy of the system_executor, before the system
	//   executor and the scheduler are stopped. (ex. net::emergency::manager stops the emergency acceptor and its executors there)
	//   * one-shot : the handlers are taken out when they are called (register again after a re-initialize if needed)
	//   * never called on an abnormal termination (crash, std::terminate, _Exit ...)
	//   * the same name replaces the handler registered before. the handlers are called in the reverse order of the registration.
	using destroy_handler_t = std::function<void()>;
	static	bool				register_destroy_handler(std::string_view _name, destroy_handler_t&& _handler);
	static	bool				unregister_destroy_handler(std::string_view _name) noexcept;

private:
	static std::mutex			m_s_lockable_create;
	static system_executor		m_s_instance;

public:
	[[nodiscard]] auto			_executor() noexcept { return object_ptr<executor::Iio_context>(this->m_psystem_executor); }
	[[nodiscard]] auto			_schedulable_manager() noexcept { return object_ptr<schedulable_manager_t>(this->m_pschedulable_manager); }
	[[nodiscard]] auto			_scheduler_singly() noexcept { return object_ptr<executable_scheduler_t>(this->m_pexecutable_scheduler); }

private:
	virtual	result_code			process_initialize(context& _context) override;
	virtual	result_code			process_destroy() noexcept override;
			void				process_detach_instance() noexcept;
	virtual	result_code			destroy() noexcept override { return object::Ninitializable::destroy();}

			own_ptr<executor::Iio_context> m_psystem_executor;
			own_ptr<executable_scheduler_t> m_pexecutable_scheduler;
			own_ptr<schedulable_manager_t> m_pschedulable_manager;
			object_ptr<factory_sustain_t> m_pschedulable_factory_garbage_collection;
			object_ptr<thread_observer_t> m_pschedulable_thread_observer;
			std::vector<own_ptr<executor::Iio_context>> m_vector_executor_retired;	// - destroy된 system executor (executor_ptr()로 쓰는 중일 수 있어 소멸될 때 해제한다)
			std::recursive_mutex m_lockable_lock;
			int					m_init_param = static_cast<int>(CGEXECUTOR_AUTO);

			struct DESTROY_HANDLER { std::string name; destroy_handler_t handler; };
			std::vector<DESTROY_HANDLER> m_vector_destroy_handler;	// - guarded by m_lockable_lock
			void				process_call_destroy_handlers() noexcept;
};


}
