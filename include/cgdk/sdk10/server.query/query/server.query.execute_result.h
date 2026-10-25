//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                               server.query                                *
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

//-----------------------------------------------------------------------------
//
// query::execute_result<TRESULT>
//
//  - the return type of execute_query() of the queriers and the queries.
//    (query::rdb::odbc, query::http, query::redis)
//    TRESULT: object_ptr<TQUERY> for the queriers, void for the queries
//
//  1) without 'co_await': executed as before.
//
//        auto pquery = pquerier->execute_query("SELECT * FROM test_table"sv);
//
//     - out of a coroutine, it is executed at once in execute_query(). (same as before)
//     - in a coroutine, it is executed synchronously when the result is used
//       (->, *, get(), exist(), empty(), conversion to object_ptr<TQUERY>)
//       or at the end of the statement when the result is not used.
//
//  2) with 'co_await' (in a CGDK coroutine): executed as a coroutine.
//
//        auto pquery = co_await pquerier->execute_query("SELECT * FROM test_table"sv);
//        co_await pquery->execute_query("get users:1"sv);
//
//     - the thread of the coroutine is not blocked while it runs.
//     - rdb.odbc : the query is executed on a thread of the system_executor.
//     - http/redis : the request is sent and the coroutine is resumed when the
//       response is received. no thread waits for the response.
//
//  3) an exception is thrown where it is executed.
//     (execute_query(), the first use, or 'co_await')
//  4) the command and the parameters are copied when the execution is deferred.
//     (string views and C strings are copied as strings)
//  5) without coroutine support (C++17), it is always executed at once.
//
//-----------------------------------------------------------------------------
namespace CGDK::query
{
	//-----------------------------------------------------------------------------
	// is_deferred_execution()
	//  - true: execute_query() defers the execution (called in a CGDK coroutine)
	//-----------------------------------------------------------------------------
	[[nodiscard]] inline bool is_deferred_execution() noexcept
	{
	#if defined(CGDK_SYSTEM_COROUTINE)
		return coroutine::is_in_coroutine();
	#else
		return false;
	#endif
	}

	namespace detail
	{
		//-----------------------------------------------------------------------------
		// deferred parameters
		//  - the parameters of a deferred execution are copied.
		//    string views and C strings are copied as strings, so they are safe even
		//    if the source is changed or destroyed before the execution.
		//  - the parameters are passed with their original types when executed.
		//-----------------------------------------------------------------------------
		template <class T>
		struct deferred_param { using storage_type = T; static const T& get(const T& _value) noexcept { return _value; } };
		template <>
		struct deferred_param<std::string_view> { using storage_type = std::string; static std::string_view get(const std::string& _value) noexcept { return _value; } };
		template <>
		struct deferred_param<std::wstring_view> { using storage_type = std::wstring; static std::wstring_view get(const std::wstring& _value) noexcept { return _value; } };
		template <>
		struct deferred_param<const char*> { using storage_type = std::string; static const char* get(const std::string& _value) noexcept { return _value.c_str(); } };
		template <>
		struct deferred_param<char*> { using storage_type = std::string; static char* get(const std::string& _value) noexcept { return const_cast<char*>(_value.c_str()); } };
		template <>
		struct deferred_param<const wchar_t*> { using storage_type = std::wstring; static const wchar_t* get(const std::wstring& _value) noexcept { return _value.c_str(); } };
		template <>
		struct deferred_param<wchar_t*> { using storage_type = std::wstring; static wchar_t* get(const std::wstring& _value) noexcept { return const_cast<wchar_t*>(_value.c_str()); } };

		template <class... TARGS>
		using deferred_params = std::tuple<typename deferred_param<std::decay_t<TARGS>>::storage_type...>;

		template <class... TARGS, class TFUNCTION, class TTUPLE, std::size_t... INDEX>
		decltype(auto) apply_deferred_params_impl(TFUNCTION& _function, const TTUPLE& _params, std::index_sequence<INDEX...>)
		{
			return _function(deferred_param<std::decay_t<TARGS>>::get(std::get<INDEX>(_params))...);
		}

		// - calls '_function' with the stored parameters (with their original types)
		template <class... TARGS, class TFUNCTION, class TTUPLE>
		decltype(auto) apply_deferred_params(TFUNCTION&& _function, const TTUPLE& _params)
		{
			return apply_deferred_params_impl<TARGS...>(_function, _params, std::index_sequence_for<TARGS...>{});
		}
	}

	namespace detail
	{
		// - the function which starts the request asynchronously
		//   (sets the result before sending the request and calls '_on_complete' when it is completed)
		template <class TRESULT>
		struct execute_start { using type = std::function<void(TRESULT& _result, std::function<void()>&& _on_complete)>; };
		template <>
		struct execute_start<void> { using type = std::function<void(std::function<void()>&& _on_complete)>; };

		//-----------------------------------------------------------------------------
		// execute_base<TRESULT>
		//-----------------------------------------------------------------------------
		template <class TRESULT>
		class execute_base
		{
		public:
			using result_type = TRESULT;
			using function_type = std::function<TRESULT()>;
			using start_type = typename execute_start<TRESULT>::type;
			using storage_type = std::conditional_t<std::is_void_v<TRESULT>, char, TRESULT>;

		protected:
				execute_base() noexcept = default;
				execute_base(execute_base&& _move) noexcept : m_function(std::exchange(_move.m_function, nullptr)), m_start(std::exchange(_move.m_start, nullptr)), m_result(std::move(_move.m_result)), m_exception(std::exchange(_move.m_exception, nullptr)), m_resume_executor(std::move(_move.m_resume_executor)), m_preserve_custom_executor(_move.m_preserve_custom_executor) {}
				execute_base(const execute_base& _copy) : m_result((_copy._ensure(), _copy.m_result)) {}
				~execute_base() noexcept(false) { this->_ensure_at_destruction(); }

				execute_base&		operator = (execute_base&& _move) { if (this != &_move) { this->_ensure(); this->m_function = std::exchange(_move.m_function, nullptr); this->m_start = std::exchange(_move.m_start, nullptr); this->m_result = std::move(_move.m_result); this->m_exception = std::exchange(_move.m_exception, nullptr); this->m_resume_executor = std::move(_move.m_resume_executor); this->m_preserve_custom_executor = _move.m_preserve_custom_executor; } return *this; }
				execute_base&		operator = (const execute_base& _copy) { if (this != &_copy) { this->_ensure(); _copy._ensure(); this->m_result = _copy.m_result; } return *this; }

		public:
			// - executes now if it is deferred
				void				wait() { this->_ensure(); }

		#if defined(CGDK_SYSTEM_COROUTINE)
		public:
			// awaiter) 'co_await execute_query(...)'
			[[nodiscard]] bool		await_ready() const noexcept { return !this->m_function; }
				template <class TPROMISE>
			[[nodiscard]] bool		await_suspend(std::coroutine_handle<TPROMISE> _handle) noexcept
			{
				static_assert(coroutine::is_cgdk_promise_v<TPROMISE>, "co_await execute_query() can be used only in CGDK coroutines");

				// 1) the coroutine to resume
				auto ppromise = &_handle.promise();
				this->m_ppromise = ppromise;

				// 2) asynchronous request (http, redis): resumed when it is completed
				if (this->m_start)
				{
					auto start = std::exchange(this->m_start, nullptr);
					this->m_function = nullptr;

					try
					{
						// - the coroutine is resumed on its executor (posted) when the request is completed
						auto selected = std::move(this->m_resume_executor);
						if (this->m_preserve_custom_executor && ppromise->executor().exist() &&
							ppromise->executor().get() != static_cast<Iexecutor*>(system_executor::executor_ptr_peek())) selected.reset();
						auto on_complete = [ppromise, selected = std::move(selected)]() {
							if (selected.exist()) ppromise->executor(selected);
							(void)ppromise->_resume_on(ppromise->executor().get());
						};

						// - start (the coroutine may be resumed already after it. nothing is touched any more)
						if constexpr (std::is_void_v<TRESULT>)
							start(std::move(on_complete));
						else
							start(this->m_result, std::move(on_complete));

						// return) suspended
						return true;
					}
					catch (...)
					{
						// - failed to send the request: continue without suspending
						this->m_exception = std::current_exception();
						return false;
					}
				}

				// 3) synchronous function (rdb.odbc): executed on the system_executor (posted: this object must not be touched any more)
				if (ppromise->_post_to_system(&execute_base::_execute, this))
					return true;

				// 4) refused (the system_executor is stopped): execute here and continue without suspending
				this->m_ppromise = nullptr;
				this->_invoke();

				// return)
				return false;
			}
				TRESULT				await_resume()
			{
				// check) rethrow the exception
				if (this->m_exception)
					std::rethrow_exception(std::exchange(this->m_exception, nullptr));

				// return)
				if constexpr (!std::is_void_v<TRESULT>)
					return std::move(this->m_result);
			}

		private:
			// - runs on a thread of the system_executor
			static	void			_execute(void* _pcontext) noexcept
			{
				auto pthis = static_cast<execute_base*>(_pcontext);

				// 1) execute
				pthis->_invoke();

				// 2) resume the coroutine (this object may be destroyed by it. nothing is touched after it)
				pthis->m_ppromise->_resume_here_or_on_executor();
			}
				void				_invoke() noexcept
			{
				try
				{
					this->_ensure();
				}
				catch (...)
				{
					this->m_exception = std::current_exception();
				}
			}
		#endif

		protected:
			// - executes synchronously if it is deferred
				void				_ensure() const
			{
				// check)
				if (!this->m_function)
					return;

				// - execute (once)
				auto function = std::exchange(this->m_function, nullptr);
				this->m_start = nullptr;

				if constexpr (std::is_void_v<TRESULT>)
					function();
				else
					this->m_result = function();
			}
				void				_ensure_at_destruction()
			{
				// check) executed or awaited
				if (!this->m_function)
					return;

				// - an exception is being propagated: don't throw
				if (std::uncaught_exceptions() > this->m_count_uncaught_exceptions)
				{
					try { this->_ensure(); } catch (...) {}
					return;
				}

				// - not used: execute at the end of the statement
				this->_ensure();
			}

		protected:
			mutable	function_type		m_function;
			mutable	start_type			m_start;
			mutable	storage_type		m_result {};
					std::exception_ptr	m_exception;
					object_ptr<Iexecutor> m_resume_executor;
					bool m_preserve_custom_executor = false;
		#if defined(CGDK_SYSTEM_COROUTINE)
					coroutine::promise_base* m_ppromise = nullptr;
		#endif
					int					m_count_uncaught_exceptions = std::uncaught_exceptions();
		};
	}

	//-----------------------------------------------------------------------------
	// execute_result<object_ptr<TQUERY>> (execute_query() of the queriers)
	//-----------------------------------------------------------------------------
	template <class TRESULT>
	class execute_result : public detail::execute_base<TRESULT>
	{
	public:
		using base_type = detail::execute_base<TRESULT>;
		using typename base_type::function_type;
		using typename base_type::start_type;

	public:
		// 1) already executed
		static	execute_result		executed(TRESULT&& _result) noexcept { execute_result temp; temp.m_result = std::move(_result); return temp; }
		// 2) deferred
		//    - _function : executes it synchronously (without 'co_await')
		//    - _start    : starts it asynchronously ('co_await'). if empty, _function is executed on the system_executor
		static	execute_result		deferred(function_type&& _function, start_type&& _start = nullptr, object_ptr<Iexecutor> _executor = nullptr, bool _preserve_custom_executor = false) noexcept { execute_result temp; temp.m_function = std::move(_function); temp.m_start = std::move(_start); temp.m_resume_executor = std::move(_executor); temp.m_preserve_custom_executor = _preserve_custom_executor; return temp; }

	public:
			execute_result& resume_on(const object_ptr<Iexecutor>& executor) & noexcept { this->m_resume_executor = executor; this->m_preserve_custom_executor = false; return *this; }
			execute_result&& resume_on(const object_ptr<Iexecutor>& executor) && noexcept { this->m_resume_executor = executor; this->m_preserve_custom_executor = false; return std::move(*this); }

	public:
			execute_result() noexcept = default;
			execute_result(execute_result&&) noexcept = default;
			execute_result(const execute_result&) = default;
			execute_result&		operator = (execute_result&&) = default;
			execute_result&		operator = (const execute_result&) = default;

	public:
		// - same as object_ptr<TQUERY> (executed first if it is deferred)
		[[nodiscard]] auto			operator -> () const { this->_ensure(); return this->m_result.operator->(); }
		[[nodiscard]] decltype(auto) operator * () const { this->_ensure(); return *this->m_result; }
		[[nodiscard]] auto			get() const { this->_ensure(); return this->m_result.get(); }
		[[nodiscard]] bool			exist() const { this->_ensure(); return this->m_result.exist(); }
		[[nodiscard]] bool			empty() const { this->_ensure(); return this->m_result.empty(); }
		[[nodiscard]] explicit		operator bool() const { this->_ensure(); return this->m_result.exist(); }
		[[nodiscard]]				operator typename TRESULT::value_type* () const { this->_ensure(); return this->m_result.get(); }
									operator const TRESULT& () const & { this->_ensure(); return this->m_result; }
									operator TRESULT () && { this->_ensure(); return std::move(this->m_result); }
	};

	//-----------------------------------------------------------------------------
	// execute_result<void> (execute_query() of the queries)
	//-----------------------------------------------------------------------------
	template <>
	class execute_result<void> : public detail::execute_base<void>
	{
	public:
		using base_type = detail::execute_base<void>;

	public:
		// 1) already executed
		static	execute_result		executed() noexcept { return execute_result(); }
		// 2) deferred
		static	execute_result		deferred(function_type&& _function, start_type&& _start = nullptr, object_ptr<Iexecutor> _executor = nullptr, bool _preserve_custom_executor = false) noexcept { execute_result temp; temp.m_function = std::move(_function); temp.m_start = std::move(_start); temp.m_resume_executor = std::move(_executor); temp.m_preserve_custom_executor = _preserve_custom_executor; return temp; }

	public:
			execute_result& resume_on(const object_ptr<Iexecutor>& executor) & noexcept { this->m_resume_executor = executor; this->m_preserve_custom_executor = false; return *this; }
			execute_result&& resume_on(const object_ptr<Iexecutor>& executor) && noexcept { this->m_resume_executor = executor; this->m_preserve_custom_executor = false; return std::move(*this); }

	public:
			execute_result() noexcept = default;
			execute_result(execute_result&&) noexcept = default;
			execute_result(const execute_result&) = delete;
			execute_result&		operator = (execute_result&&) = delete;
			execute_result&		operator = (const execute_result&) = delete;
	};
}
