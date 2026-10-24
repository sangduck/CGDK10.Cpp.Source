//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          server.query.rdb.odbc                            *
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
// query::rdb::odbc::querier
//
// * Parameters
//    1) "session"
//    2) "connection_string"
//    3) "provider"
//       "data_source"
//       "user_id"
//       "password"
//       "initial_catalog"
//
//    [CASE MSSQL]
//		"provider"			 = "SQLOLEDB.1"
//		"data_source"		 = "TEST_SERVER"
//		"user_id"			 = "sa"
//		"password"			 = "12341234"
//		"initial_catalog"	 = "Test"
//
//    [CASE MySQL]
//		"provider"			 = "MSDASQL.1"
//		"data_source"		 = "TEST_SERVER"
//		"user_id"			 = "sa"
//		"password"			 = "12341234"
//		"initial_catalog"	 = "Test"
//
//-----------------------------------------------------------------------------
class query::rdb::odbc::querier :
// inherited classes)
	virtual public				Iquerier,
	public						object::Nnameable,
	public                      object::Ninitializable_startable,
	public						Nmessage_transmitter,
	virtual public				Imessageable
{
// definitions)
public:
	using	IQUERY = Iquery;
	using	QUERIER = querier;
			struct START_PARAMETER
			{
				eDB_PROVIDER	provider = eDB_PROVIDER::NONE;
				std::string		data_source;
				std::string		user_id;
				std::string		password;
				std::string		initial_catalog;
				std::string		character_set;
				std::string		options;
				std::size_t		count_session = 32;
				std::size_t		count_data_source = 1;
				uint64_t		additional = 0;
				// - keepalive : pings the sessions idle for 'keepalive_interval' in the pool (default : disabled)
				bool			keepalive_enable = false;
				uint32_t		keepalive_interval = 300;		// (seconds)
				std::string		keepalive_query;				// (empty : "SELECT 1")
				// - session wait timeout : 'SET SESSION wait_timeout' on connecting. MySQL/MariaDB only (default : disabled)
				bool			session_wait_timeout_enable = false;
				uint32_t		session_wait_timeout = 1800;	// (seconds) idle time of the connection
			};

// constructor/destructor) 
public:
			querier();
	virtual ~querier() noexcept;

// public) 
public:
	// 1) start
			result_code			start(const context& _context) { return this->object::Nstartable::start(_context);}
	virtual	result_code			start(context& _context) override { return this->object::Nstartable::start(_context);}
			result_code			start(Iexecutor* _pexecutor, const std::string_view _connection_string, std::size_t _count_session = 32, std::size_t _count_data_source = 1);
			result_code			start(const std::string_view _connection_string, std::size_t _count_session = 32, std::size_t _count_data_source = 1);
			result_code			start(eDB_PROVIDER _provider, const std::string_view _server, const std::string_view _user_id = std::string_view(), const std::string_view _password = std::string_view(), const std::string_view _initial_catalog = std::string_view(), const std::string_view _character_set = std::string_view(), const std::string_view _option = std::string_view(), std::size_t _count_session = 32, std::size_t _count_data_source = 1);
			result_code			start(const std::string_view _provider, const std::string_view _server, const std::string_view _user_id = std::string_view(), const std::string_view _password = std::string_view(), const std::string_view _initial_catalog = std::string_view(), const std::string_view _character_set = std::string_view(), const std::string_view _option = std::string_view(), std::size_t _count_session = 32, std::size_t _count_data_source = 1);
			result_code			start(const START_PARAMETER& _parameter) { context temp; temp=buffer_view((char*)&_parameter, sizeof(START_PARAMETER)); return start(temp); }

	[[nodiscard]] object_ptr<Iexecutor> executor() const { return this->m_pexecutor; }

	// 2) execute_query/post_execute_query/Cancel
			//    - execute_query()      : without 'co_await' executed as before. with 'co_await' executed on the system_executor. (execute_result)
			//    - execute_query_sync() : always executed at once
			template <class TQUERY = query_t>
			execute_result<object_ptr<TQUERY>> execute_query(const std::string_view _command);
			auto				execute_query(const std::string_view _command) { return this->execute_query<query_t>(_command); }
			template <class TQUERY = query::rdb::query_param_t, class T1, class... TARGS>
			execute_result<object_ptr<TQUERY>> execute_query(const std::string_view _command, T1&& _first, TARGS&&... _args);
			template <class TQUERY = query_t>
			object_ptr<TQUERY>	execute_query_sync(const std::string_view _command);
			auto				execute_query_sync(const std::string_view _command) { return this->execute_query_sync<query_t>(_command); }
			template <class TQUERY = query::rdb::query_param_t, class T1, class... TARGS>
			object_ptr<TQUERY>	execute_query_sync(const std::string_view _command, T1&& _first, TARGS&&... _args);
			//    - co_execute_query()      : 지금 쓸 수 있는 session이 있으면 이 thread에서 바로 실행한다. (블로킹. 'co_await'해도 suspend하지 않는다) 없으면 co_post_execute_query()와 같다.
			//    - co_post_execute_query() : system_executor에서 실행하고 끝나면 coroutine이 이어진다. ('co_await execute_query()'와 같다)
			template <class TQUERY = query_t>
			execute_result<object_ptr<TQUERY>> co_execute_query(const std::string_view _command);
			template <class TQUERY = query::rdb::query_param_t, class T1, class... TARGS>
			execute_result<object_ptr<TQUERY>> co_execute_query(const std::string_view _command, T1&& _first, TARGS&&... _args);
			template <class TQUERY = query_t>
			execute_result<object_ptr<TQUERY>> co_post_execute_query(const std::string_view _command);
			template <class TQUERY = query::rdb::query_param_t, class T1, class... TARGS>
			execute_result<object_ptr<TQUERY>> co_post_execute_query(const std::string_view _command, T1&& _first, TARGS&&... _args);

			template <class TQUERY = query_t>
			void				post_execute_query(const std::string_view _command, std::function<void(TQUERY*)>&& _lambda = nullptr, std::function<void()>&& _on_cancel = nullptr);	// '_on_cancel' : called instead of '_lambda' when the query is canceled before it is executed (cancel_queries(), refused by the executor)
			void				post_execute_query(const std::string_view _command, std::function<void(query::rdb::query_t*)>&& _lambda) { this->post_execute_query<query_t>(_command, std::move(_lambda)); }
			void				post_execute_query(const std::string_view _command, std::function<void(query::rdb::query_t*)>&& _lambda, std::function<void()>&& _on_cancel) { this->post_execute_query<query_t>(_command, std::move(_lambda), std::move(_on_cancel)); }
			template <class TQUERY = query_t>
			void				post_execute_query(const std::string_view _command, const std::function<void(TQUERY*)>& _lambda) { this->post_execute_query<query_t>(_command, std::function<void(TQUERY*)>(_lambda)); }
			void				post_execute_query(const std::string_view _command, const std::function<void(query_t*)>& _lambda) { this->post_execute_query<query_t>(_command, std::function<void(query_t*)>(_lambda)); }

			template <class TQUERY = query_param_t>
	[[nodiscard]] object_ptr<TQUERY> prepare_query(const std::string_view _command);

			void				cancel_queries() noexcept;

	// 4) alloc_session/free_session
			object_ptr<session_t> alloc_session();
			object_ptr<session_t> try_alloc_session();	// 지금 쓸 수 있는 session이 없으면 기다리지 않고 empty를 돌려준다.
			void				post_alloc_session(std::function<void(session_t*)>&& _lambda, std::function<void()>&& _on_cancel = nullptr);	// '_on_cancel' : called instead of '_lambda' when the request is canceled (cancel_queries(), refused by the executor)

#if defined(__cpp_impl_coroutine)
		// - session 할당을 co_await로 받는다. (object_ptr<session_t>: 변수가 살아 있는 동안 session을 가진다. 놓으면 pool로 돌아간다)
		//
		//   co_post_alloc_session()
		//   1) 항상 suspend된다. session이 할당되면 그 session 스레드 위에서 resume된다.
		//      이후의 코드는 session 스레드에서 실행되므로 blocking query를 그대로 호출해도 된다. (system_executor 스레드를 점유하지 않는다)
		//
		//   co_alloc_session()
		//   1) 지금 바로 쓸 수 있는 session이 있으면 suspend하지 않고 그 session을 돌려준다. (이후의 코드는 '이 thread'에서 실행된다)
		//   2) 없으면 co_post_alloc_session()과 같다. (session이 생길 때까지 suspend. session 스레드에서 resume)
		//
		//      auto psession = co_await pquerier->co_alloc_session();
		//      auto pquery = psession->alloc_query();
		//      pquery->execute_query(...);
		//
		class [[nodiscard]] session_awaiter
		{
		public:
								session_awaiter(querier* _pquerier, bool _is_execute) noexcept : m_pquerier(_pquerier), m_is_execute(_is_execute) {}

			// - execute: 지금 쓸 수 있는 session이 있으면 suspend하지 않는다.
			[[nodiscard]] bool	await_ready()
			{
				if (this->m_is_execute)
					this->m_psession = this->m_pquerier->try_alloc_session();

				return this->m_psession.exist();
			}
				template <class TPROMISE>
				void			await_suspend(std::coroutine_handle<TPROMISE> _handle)
			{
				static_assert(coroutine::is_cgdk_promise_v<TPROMISE>, "co_alloc_session can be awaited only in CGDK coroutines");

				// 1) promise를 기억해 둔다.
				auto ppromise = &_handle.promise();

				// 2) session이 할당되면 그 session 스레드 위에서 coroutine을 resume한다.
				//    (주의: 이 호출 뒤에 awaiter 멤버를 건드리면 안 된다. 이미 resume되었을 수 있다.)
				this->m_pquerier->post_alloc_session([this, ppromise](session_t* _psession)
					{
						// - session을 가진다. (session 스레드의 참조는 lambda가 끝나면 놓인다. coroutine의 object_ptr가 session을 붙든다)
						this->m_psession = object_ptr<session_t>(_psession);

						// - session 스레드 위에서 resume한다.
						ppromise->_resume_here_or_on_executor();
					},
					[ppromise]()
					{
						// - canceled (cancel_queries(), refused by the executor) : resumed without a session. await_resume() throws eRESULT::CANCEL
						ppromise->_resume_here_or_on_executor();
					});
			}
			[[nodiscard]] auto	await_resume() -> object_ptr<session_t> { THROW_IF(this->m_psession.empty(), throwable::failure(eRESULT::CANCEL, 0, std::string_view("session allocation is canceled"))); return std::move(this->m_psession); }

		private:
				querier*	m_pquerier;
				bool		m_is_execute;
				object_ptr<session_t> m_psession;
		};

		// - session이 생길 때까지 suspend하고 session 스레드에서 resume한다.
		[[nodiscard]] auto	co_post_alloc_session() noexcept { return session_awaiter(this, false); }
		// - 지금 쓸 수 있는 session이 있으면 suspend 없이 바로 돌려주고, 없으면 co_post_alloc_session()과 같다.
		[[nodiscard]] auto	co_alloc_session() noexcept { return session_awaiter(this, true); }
#endif

	virtual	void				free_session(session* _psession) noexcept;

	// 5) alloc query
	template <class TQUERY = query_t>
	[[nodiscard]] object_ptr<TQUERY> alloc_query();

	// 6) 
			auto				get_max_concurrent_session_count() const noexcept { return this->m_count_session_created.load(); }
			auto				get_created_session_count() const noexcept { return this->m_count_session_created.load(); }
			auto				get_stacked_session_count() const noexcept { SCOPED_LOCK(this->m_lockable_query) { return this->m_stack_sessions.size(); } }
			auto				get_queued_stacked_query() const noexcept { SCOPED_LOCK(this->m_lockable_query) { return this->m_queue_query.size(); } }
			auto				get_session_regeneration_interval() const noexcept { return this->m_interval_regeneration; }
	[[nodiscard]] auto			get_keepalive_interval() const noexcept { return this->m_keepalive_interval; }
	[[nodiscard]] auto			get_session_wait_timeout() const noexcept { return this->m_session_wait_timeout; }

	// 7) options
	static	void				enable_logging_warning_info(bool _enable = true) noexcept { m_s_enable_logging_warning_info = _enable; }
	static	void				disable_logging_warning_info() noexcept { enable_logging_warning_info(false); }
	[[nodiscard]] static bool	is_enable_logging_warning_info() noexcept { return m_s_enable_logging_warning_info; }
	[[nodiscard]] static bool	is_disable_logging_warning_info() noexcept { return is_enable_logging_warning_info(); }

protected:
	// declare) 
	class Iexecutable_query : public Iexecutable, public factory::_traits_memory
	{
	public:
		// - released while it is pending (queued or posted, not executed yet) : canceled. (cancel_queries(), refused by the executor, ...)
		//   the session goes back to the pool first (free_session()), then on_cancel() is called. (released out of 'm_lockable_query')
		virtual	void		on_final_release() noexcept override { const auto is_canceled = std::exchange(this->m_is_pending, false); this->m_psession.reset(); if (is_canceled) this->on_cancel(); }
		virtual	void		on_cancel() noexcept {}
		object_ptr<session> m_psession;
		bool				m_is_pending = false;	// set when it is queued or posted. cleared when it is executed
	};
	friend class Iexecutable_query;

	virtual	result_code			process_start(context& _context) override;
	virtual	result_code			process_stop() noexcept override;
	virtual result_code			_process_destroy() noexcept override;

			circular_list<object_ptr<session>> process_create_session(std::string_view _str_initialization, std::size_t _count_session=32, std::size_t _count_data_source=1);
			void				process_post_alloc_session(object_ptr<Iexecutable_query>&& _pexecutable);
			void				process_keepalive() noexcept;

	virtual	result_code			process_message(sMESSAGE& _msg) override;

	// 7) process_transmit_message
	virtual	result_code			process_transmit_message(sMESSAGE& _msg) override;

private:
			// - the session pool. private : only the members of querier lock 'm_lockable_query'.
			//   a thread holding it must not wait in alloc_session() (the wait releases the recursive lock only one level -> dead-lock),
			//   so nothing out of the querier (a derived class, an executor, a query) can hold it,
			//   and the querier runs no outer code holding it (post(), setting the session of a query and releasing the queued queries are out of the lock).
			struct QUEUED_QUERY
			{
				std::function<void(object_ptr<session>&&)> f_execute;
			};

			std::recursive_mutex m_lockable_query;
			circular_list<object_ptr<session>> m_stack_sessions;
			circular_list<QUEUED_QUERY>	m_queue_query;

protected:
			std::atomic<std::size_t> m_count_session_created = 0;
			std::size_t			m_interval_regeneration = 0;

			// - keepalive / session wait timeout (options. both disabled by default)
			//   * "keepalive": { "enable": true, "interval": 300, "query": "SELECT 1" }
			//       pings the sessions idle for 'interval'(seconds) in the pool.
			//       keeps the connection alive through NAT/firewall and reconnects a broken one before it is used.
			//   * "session_wait_timeout": { "enable": true, "timeout": 1800 }
			//       'SET SESSION wait_timeout' on connecting (MySQL/MariaDB).
			//       the server closes the connection idle for 'timeout'(seconds). (a connection whose client is gone is not kept)
			//   * m_keepalive_interval / m_session_wait_timeout : 0 if disabled
			chrono::tick::duration m_keepalive_interval = chrono::tick::duration::zero();
			std::string			m_keepalive_query;
			uint32_t			m_session_wait_timeout = 0;
			std::mutex			m_lockable_keepalive;
			own_ptr<schedulable::executable_function<>> m_pschedulable_keepalive;
	static	bool				m_s_enable_logging_warning_info;

	// 11) executor
			object_ptr<Iexecutor> m_pexecutor;
			own_ptr<Iexecutor>	m_pexecutor_own;

};

template <class TQUERY>
object_ptr<TQUERY> query::rdb::odbc::querier::execute_query_sync(const std::string_view _command)
{
	// 1) alloc session
	auto psession = this->alloc_session();

	// check) 
	CGASSERT_ERROR(psession.exist());

	// 2) alloc query
	auto pquery = psession->alloc_query<TQUERY>();

	// check) 
	CGASSERT_ERROR(pquery.exist());

	// check) 
	CGASSERT_ERROR(pquery->m_psession.exist());

	// s3) execute query
	pquery->execute_query_sync(_command);

	// return) 
	return pquery;
}

template <class TQUERY, class T1, class... TARGS>
object_ptr<TQUERY> query::rdb::odbc::querier::execute_query_sync(const std::string_view _format, T1&& _first, TARGS&&... _args)
{
	// 1) alloc session
	auto psession = this->alloc_session();

	// check) 
	CGASSERT_ERROR(psession.exist());

	// 2) alloc query
	auto pquery = psession->alloc_query<TQUERY>();

	// check) 
	CGASSERT_ERROR(pquery.exist());

	// check) 
	CGASSERT_ERROR(pquery->m_psession.exist());

	// 3) execute query
	pquery->execute_query_sync(_format, _first, std::forward<TARGS>(_args)...);

	// return) 
	return pquery;
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::execute_query(const std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
		return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_command));

	// 2) in a coroutine: deferred ('co_await' executes it on the system_executor)
	return execute_result<object_ptr<TQUERY>>::deferred([pquerier = object_ptr<querier>(this), command = std::string(_command)]() { return pquerier->template execute_query_sync<TQUERY>(command); });
}

template <class TQUERY, class T1, class... TARGS>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::execute_query(const std::string_view _format, T1&& _first, TARGS&&... _args)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
		return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_format, _first, std::forward<TARGS>(_args)...));

	// 2) in a coroutine: deferred ('co_await' executes it on the system_executor. the parameters are copied)
	return execute_result<object_ptr<TQUERY>>::deferred([pquerier = object_ptr<querier>(this), command = std::string(_format), params = query::detail::deferred_params<T1, TARGS...>(_first, _args...)]()
		{
			return query::detail::apply_deferred_params<T1, TARGS...>([&](const auto&... _params) { return pquerier->template execute_query_sync<TQUERY>(command, _params...); }, params);
		});
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::co_post_execute_query(const std::string_view _command)
{
	// - 'co_await'하면 system_executor에서 실행되고, 끝나면 coroutine이 이어진다. (execute_query()의 coroutine 경로와 같다)
	return execute_result<object_ptr<TQUERY>>::deferred([pquerier = object_ptr<querier>(this), command = std::string(_command)]() { return pquerier->template execute_query_sync<TQUERY>(command); });
}

template <class TQUERY, class T1, class... TARGS>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::co_post_execute_query(const std::string_view _format, T1&& _first, TARGS&&... _args)
{
	// - 'co_await'하면 system_executor에서 실행되고, 끝나면 coroutine이 이어진다. (parameter는 복사된다)
	return execute_result<object_ptr<TQUERY>>::deferred([pquerier = object_ptr<querier>(this), command = std::string(_format), params = query::detail::deferred_params<T1, TARGS...>(_first, _args...)]()
		{
			return query::detail::apply_deferred_params<T1, TARGS...>([&](const auto&... _params) { return pquerier->template execute_query_sync<TQUERY>(command, _params...); }, params);
		});
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::co_execute_query(const std::string_view _command)
{
	// 1) 지금 쓸 수 있는 session이 없으면 post한다.
	auto psession = this->try_alloc_session();

	if (psession.empty())
		return this->co_post_execute_query<TQUERY>(_command);

	// 2) 이 thread에서 바로 실행한다. (블로킹. 'co_await'해도 suspend하지 않는다)
	auto pquery = psession->alloc_query<TQUERY>();
	pquery->execute_query_sync(_command);

	// return)
	return execute_result<object_ptr<TQUERY>>::executed(std::move(pquery));
}

template <class TQUERY, class T1, class... TARGS>
query::execute_result<object_ptr<TQUERY>> query::rdb::odbc::querier::co_execute_query(const std::string_view _format, T1&& _first, TARGS&&... _args)
{
	// 1) 지금 쓸 수 있는 session이 없으면 post한다.
	auto psession = this->try_alloc_session();

	if (psession.empty())
		return this->co_post_execute_query<TQUERY>(_format, std::forward<T1>(_first), std::forward<TARGS>(_args)...);

	// 2) 이 thread에서 바로 실행한다. (블로킹. 'co_await'해도 suspend하지 않는다)
	auto pquery = psession->alloc_query<TQUERY>();
	pquery->execute_query_sync(_format, _first, std::forward<TARGS>(_args)...);

	// return)
	return execute_result<object_ptr<TQUERY>>::executed(std::move(pquery));
}

template <class TQUERY>
void query::rdb::odbc::querier::post_execute_query(const std::string_view _command, std::function<void(TQUERY*)>&& _lambda, std::function<void()>&& _on_cancel)
{
	// check) is ready?
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

	class executable_query : public Iexecutable_query
	{
	public:
		virtual	intptr_t process_execute(intptr_t, std::size_t) override
		{
			// - executed (not canceled)
			this->m_is_pending = false;

			// 1) get session
			auto psession = std::move(this->m_psession);
			auto command = std::move(this->m_command);

			// check)
			CGASSERT_ERROR(psession.exist());

			// check)
			CGASSERT_ERROR(command.empty() == false);

			// 2) alloc query
			auto pquery = psession->alloc_query<TQUERY>();

			// 3) query
			pquery->execute_query_sync(command);
		
			// 4) response (none if '_lambda' is empty)
			if (this->m_lambda)
				this->m_lambda(pquery);

			// 5) reset
			pquery->reset_session();
		
			// return) 
			return 1;
		}

		virtual	void on_cancel() noexcept override
		{
			// check)
			RETURN_IF(!this->m_on_cancel);

			try
			{
				this->m_on_cancel();
			}
			catch (...)
			{
			}
		}

		std::function<void(TQUERY*)> m_lambda;
		std::function<void()> m_on_cancel;
		std::string m_command;
	};

	// 1) create query object
	auto pexecutable_query = make_object<executable_query>();
	pexecutable_query->m_lambda = std::move(_lambda);
	pexecutable_query->m_on_cancel = std::move(_on_cancel);
	pexecutable_query->m_command = _command;

	// 2) post  
	this->process_post_alloc_session(std::move(pexecutable_query));
}

template <class TQUERY>
object_ptr<TQUERY> query::rdb::odbc::querier::prepare_query(const std::string_view _command)
{
	// 1) alloc query
	auto pquery = this->alloc_query();

	// check) 
	CGASSERT_ERROR(pquery.exist());

	// check) 
	CGASSERT_ERROR(pquery->m_psession.exist());

	// s3) execute query
	pquery->prepare_query(_command);

	// return) 
	return pquery;
}

template <class TQUERY>
object_ptr<TQUERY> query::rdb::odbc::querier::alloc_query()
{
	// 1) alloc session
	auto psession = this->alloc_session();

	// check) 
	CGASSERT_ERROR(psession.exist());

	// 2) alloc query and return
	return psession->alloc_query<TQUERY>();
}


}