//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            server.query.redis                             *
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
#include <format>

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::redis::querier
//
//
//-----------------------------------------------------------------------------
class query::redis::querier : 
	public						Iquerier,
	public						object::Nnameable,
	public						object::Ninitializable_startable,
	public						Nmessage_transmitter,
	virtual public				Imessageable
{
public:
	using	end_point_type = net::ip::tcp::endpoint;
			struct START_PARAMETER
			{
				end_point_type		remote_endpoint;
				int					count_session = 32;
				int					wait_seconds = 0;	// waits until the sessions are connected (or failed). 0: 5 seconds, < 0: not waited
				object_ptr<executor::Iio_context> pio_context;
			};

			querier();
	virtual	~querier() noexcept;

			result_code			start() { context context_start; return object::Nstartable::start(context_start);}
			result_code			start(const context& _context) { return this->object::Nstartable::start(_context);}
	virtual	result_code			start(context& _context) override { return this->object::Nstartable::start(_context);}
			result_code			start(const START_PARAMETER& _parameter) { context temp; temp=buffer_view((char*)&_parameter, sizeof(START_PARAMETER)); return this->start(temp); }

			//    - execute_query()      : without 'co_await' executed as before. with 'co_await' the coroutine is resumed when the response is received. (execute_result)
			//    - execute_query_sync() : always executed at once (waits for the response)
			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								execute_query(std::string_view _command);
			template <class TQUERY, class TA, class... TARGS>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								execute_query(const std::string_view _format, TA _first, TARGS... _args);
			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, object_ptr<TQUERY>>
								execute_query_sync(std::string_view _command);
			template <class TQUERY, class TA, class... TARGS>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, object_ptr<TQUERY>>
								execute_query_sync(const std::string_view _format, TA _first, TARGS... _args);
			template <class TQUERY>
			execute_result<object_ptr<TQUERY>> _execute_query_deferred(std::string&& _command);
			template<class TQUERY>
			std::enable_if_t<std::is_base_of_v<Iquery, TQUERY>, void>
								post_execute_query(std::string_view _command, std::function<void(TQUERY*)>&& _response);
			void				post_execute_query(std::string_view _command, std::function<void(redis::query*)>&& _response) { this->post_execute_query<redis::query>(_command, std::move(_response)); }
			template<class TQUERY>
			std::enable_if_t<std::is_base_of_v<Iquery, TQUERY>, void>
								post_execute_query(std::string&& _command, std::function<void(TQUERY*)>&& _response) { this->post_execute_query<TQUERY>(std::move(_command), std::move(_response)); }
			void				post_execute_query(std::string&& _command, std::function<void(redis::query*)>&& _response) { this->post_execute_query<redis::query>(std::move(_command), std::move(_response)); }
#if defined(__cpp_impl_coroutine)
			//    - co_execute_query()      : 지금 쓸 수 있는 session이 있으면 이 thread에서 바로 실행한다. (응답까지 블로킹. 'co_await'해도 suspend하지 않는다) 없으면 co_post_execute_query()와 같다.
			//    - co_post_execute_query() : 요청을 보내고 응답이 오면 coroutine이 이어진다. ('co_await execute_query()'와 같다. thread는 기다리지 않는다)
			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								co_execute_query(std::string_view _command);
			template <class TQUERY, class TA, class... TARGS>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								co_execute_query(const std::string_view _format, TA _first, TARGS... _args);
			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								co_post_execute_query(std::string_view _command) { return this->_execute_query_deferred<TQUERY>(std::string(_command)); }
			template <class TQUERY, class TA, class... TARGS>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, execute_result<object_ptr<TQUERY>>>
								co_post_execute_query(const std::string_view _format, TA _first, TARGS... _args) { return this->_execute_query_deferred<TQUERY>(std::vformat(_format, std::make_format_args(_first, _args...))); }

			// - 전용 session이 붙은 query 객체를 co_await로 받는다. (alloc_query()의 coroutine 판. object_ptr<TQUERY>를 가지고 있는 동안 그 session은 이 query의 것이다)
			//   co_post_alloc_query() : 항상 suspend된다. session이 할당되면 executor에서 이어진다.
			//   co_alloc_query()      : 지금 쓸 수 있는 session이 있으면 suspend하지 않고 바로 돌려준다. 없으면 co_post_alloc_query()와 같다.
			//
			//      auto pquery = co_await pquerier->co_alloc_query();
			//      co_await pquery->execute_query("get users:1"sv);
			//
			template <class TQUERY>
			class [[nodiscard]] alloc_query_awaiter
			{
			public:
								alloc_query_awaiter(querier* _pquerier, bool _is_execute) : m_pquerier(_pquerier), m_is_execute(_is_execute), m_pquery(make_object<TQUERY>()) {}

				// - execute: 지금 쓸 수 있는 session이 있으면 suspend하지 않는다.
				[[nodiscard]] bool	await_ready() { return this->m_is_execute && this->m_pquerier->_try_alloc_session(this->m_pquery); }
					template <class TPROMISE>
					void			await_suspend(std::coroutine_handle<TPROMISE> _handle)
				{
					static_assert(coroutine::is_cgdk_promise_v<TPROMISE>, "co_alloc_query can be awaited only in CGDK coroutines");

					// 1) promise를 기억해 둔다.
					auto ppromise = &_handle.promise();

					// 2) 지금 받을 수 있으면 받고 executor에서 이어간다(post). 없으면 session이 생길 때 이어간다.
					//    (주의: 이 호출 뒤에 awaiter 멤버를 건드리면 안 된다. 이미 resume되었을 수 있다.)
					//    - the queued one is resumed once : by free_session() with a session, or without a session when it is released
					//      without being called (the querier is destroyed, ...) -> await_resume() throws eRESULT::CANCEL
					auto presume = std::make_shared<resume_once>(ppromise);
					bool is_allocated = false;
					try
					{
						is_allocated = this->m_pquerier->_alloc_session_or_queue(this->m_pquery, [presume]() { presume->resume(); });
					}
					catch (...)
					{
						// - not queued : the exception goes to the coroutine (not resumed here)
						presume->dismiss();
						throw;
					}
					if (is_allocated)
					{
						presume->dismiss();
						(void)ppromise->_resume_on(ppromise->executor().get());
					}
				}
				[[nodiscard]] auto	await_resume() -> object_ptr<TQUERY> { THROW_IF(this->m_pquery->get_session().empty(), throwable::failure(eRESULT::CANCEL, 0, std::string_view("session allocation is canceled"))); return std::move(this->m_pquery); }

			private:
				class resume_once
				{
				public:
					explicit		resume_once(coroutine::promise_base* _ppromise) noexcept : m_ppromise(_ppromise) {}
									~resume_once() noexcept { this->resume(); }
					void			resume() noexcept { if (auto ppromise = std::exchange(this->m_ppromise, nullptr); ppromise != nullptr) ppromise->_resume_here_or_on_executor(); }
					void			dismiss() noexcept { this->m_ppromise = nullptr; }
				private:
					coroutine::promise_base* m_ppromise;
				};

					querier*	m_pquerier;
					bool		m_is_execute;
					object_ptr<TQUERY> m_pquery;
			};

			template <class TQUERY = query>
			[[nodiscard]] auto	co_post_alloc_query() { return alloc_query_awaiter<TQUERY>(this, false); }
			template <class TQUERY = query>
			[[nodiscard]] auto	co_alloc_query() { return alloc_query_awaiter<TQUERY>(this, true); }
#endif

	virtual	bool				cancel_query(Iserver_query* _pquery) noexcept;

			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, object_ptr<TQUERY>>
								alloc_query() { auto pquery = make_object<TQUERY>(); alloc_session(pquery); return pquery;}
			template <class TQUERY = query>
			std::enable_if_t<std::is_base_of_v<redis::Iquery, TQUERY>, void>
								post_alloc_query(std::function<void(TQUERY*)>&& _response);
			template <class TQUERY = query>
			[[nodiscard]] std::enable_if_t<std::is_base_of_v<Iquery, TQUERY>, object_ptr<TQUERY>>
								alloc_shared_query() { auto pquery = make_object<TQUERY>(); this->alloc_shared_session(pquery); return pquery;}
protected:
	virtual	void				process_execute_query(Iserver_query* _pquery);
	virtual	void				process_post_execute_query(Iserver_query* _pquery);
	virtual	void				process_execute_query(Iquery* _pquery);
	virtual	void				process_post_execute_query(Iquery* _pquery);
	[[nodiscard]] virtual uint16_t process_get_default_port() noexcept { return 6379; }

	virtual result_code			_process_starting(context& _context) override;
	virtual result_code			_process_start(context& _context) override;
	virtual result_code			_process_stopping() noexcept override;
	virtual result_code			_process_stop() noexcept override;
	virtual result_code			_process_destroy() noexcept override;
	virtual	result_code			process_message(sMESSAGE& _msg) override;
	
			void				register_session(session* _psocket);
			void				unregister_session(session* _psocket) noexcept;

	virtual	result_code			process_transmit_message(sMESSAGE& _msg) override;
	virtual	void				free_session(session* _psession) noexcept;

			void				alloc_session(Iquery* _pquery);
			void				post_alloc_session(Iquery* _pquery, std::function<void(Iquery*)>&& _response);
			// - coroutine용 (alloc_query_awaiter / co_execute_query)
			bool				_try_alloc_session(Iquery* _pquery);										// 지금 쓸 수 있는 session이 있으면 query에 붙이고 true. 없으면 기다리지 않고 false
			bool				_alloc_session_or_queue(Iquery* _pquery, std::function<void()>&& _f_allocated);	// 지금 받을 수 있으면 붙이고 true. 없으면 queue에 넣고 false (session이 생기면 붙인 뒤 '_f_allocated'가 불린다)
			bool				_has_session_now();														// 지금 쓸 수 있는 session이 있는가

			void				alloc_shared_session(Iquery* _pquery);
			//void				post_alloc_shared_session(Iquery* _pquery);

			// - session ownership (holding 'm_lockable_query')
			//   'm_sessions_stack' : the sessions not dedicated. each one once
			//   shared    : alloc_shared_session(), post_execute_query(). uses a session in the stack with other shared queries (not taken out)
			//   dedicated : alloc_session(), post_alloc_session(), co_alloc_query(). takes a session no query uses out of the stack.
			//               it goes back to the stack (or to a waiting query) when the last query releases it. (free_session())
			//   fairness  : a new shared one does not go ahead of the waiting ones (FIFO) : the session a dedicated one waits for is drained.
			//               a blocked shared one (alloc_shared_session()) shares a session in the stack after 100ms. (it may hold a shared
			//               query of that session itself : not waited forever)
			object_ptr<session>	process_get_shared_session() noexcept { SCOPED_LOCK(this->m_lockable_query) { return this->m_sessions_stack.front(); } }
			object_ptr<session>	process_take_unused_session() noexcept;	// a session no query uses taken out of the stack. empty if there is none
			struct QUEUED_QUERY
			{
			public:
				QUEUED_QUERY(Ireferenceable* ) noexcept : f_execute(nullptr) {}
				QUEUED_QUERY(const QUEUED_QUERY& _copy) noexcept : f_execute(_copy.f_execute), f_cancel(_copy.f_cancel), pquery(_copy.pquery), is_shared(_copy.is_shared) {}
				QUEUED_QUERY(QUEUED_QUERY&& _move) noexcept : f_execute(std::move(_move.f_execute)), f_cancel(std::move(_move.f_cancel)), pquery(std::move(_move.pquery)), is_shared(_move.is_shared) { _move.reset();}

			public:
				std::function<void()> f_execute;
				std::function<void()> f_cancel;		// called when the querier stops instead of 'f_execute' (wakes up a blocked waiter. nullptr : just released)
				object_ptr<Iquery>	pquery;
				bool				is_shared = false;	// waits for a session in the stack (shared) or for a session no query uses (dedicated)

				QUEUED_QUERY&	operator=(QUEUED_QUERY&& _move) noexcept { f_execute = std::move(_move.f_execute); f_cancel = std::move(_move.f_cancel); this->pquery = std::move(_move.pquery); this->is_shared = _move.is_shared; return *this; }
				bool			operator!=(Iserver_query* _pquery) const noexcept { return this->pquery!=_pquery;}
				bool			operator==(Iserver_query* _pquery) const noexcept { return this->pquery==_pquery;}

				void			reset() noexcept { this->pquery.reset();}
			};
			void				process_execute_queued(QUEUED_QUERY& _queued_query) noexcept;	// calls 'f_execute' (out of the lock)
			void				process_execute_queued_shared() noexcept;						// hands the sessions in the stack to the shared queries waiting at the front
			void				process_wait_queued(Iquery* _pquery, std::unique_lock<std::recursive_mutex>& _lock, bool _is_shared = false);	// blocks until free_session() hands a session over (throws eRESULT::CANCEL when the querier stops)
			void				process_cancel_queued(circular_list<QUEUED_QUERY>& _list_queued) noexcept;	// the waiting ones when the querier stops (out of the lock)
			// - unregistering : detached holding 'm_lockable_query' (its release does not go back to free_session() any more),
			//   then closed and its requests canceled out of the lock
			[[nodiscard]] object_ptr<querier> process_detach_session(session* _psession) noexcept;
			void				process_close_session(session* _psession) noexcept;
			[[nodiscard]] bool	process_is_running() const noexcept { return this->get_object_state() == eOBJECT_STATE::RUNNING || this->get_object_state() == eOBJECT_STATE::START_PENDING; }

			std::recursive_mutex m_lockable_query;
			std::vector<object_ptr<session>>	m_sessions_stack;
			std::vector<object_ptr<session>>	m_sessions_registered;	// all the sessions registered (in the stack or dedicated)
			circular_list<QUEUED_QUERY> m_queue_query;

			object_ptr<executor::Iio_context>	m_pio_context;
			own_ptr<executor::Iio_context>		m_pio_context_own;
			
			class executable_query;
			friend class	Iquery;
			friend class	session;
			friend class	executable_query;
};


template <class TQUERY>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, object_ptr<TQUERY>>
query::redis::querier::execute_query_sync(std::string_view _command)
{
	// check) 
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

	// 1) alloc query
	auto pquery = this->alloc_shared_query<TQUERY>();

	// 2) execute query
	pquery->_process_execute_query(_command);

	// return) 
	return pquery;
}

template <class TQUERY, class TA, class... TARGS>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, object_ptr<TQUERY>>
query::redis::querier::execute_query_sync(const std::string_view _format, TA _first, TARGS... _args)
{
	// check) _format shouldn't be nullptr
	CGASSERT_ERROR(_format.empty() == false);

	// check) 
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

	// 1) generate string
	auto command = std::vformat(_format, std::make_format_args(_first, _args...));

	// 2) alloc query
	auto pquery = this->alloc_shared_query<TQUERY>();

	// 3) execute query
	pquery->_process_execute_query(std::move(command));

	// return) 
	return pquery;
}

template <class TQUERY>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, query::execute_result<object_ptr<TQUERY>>>
query::redis::querier::execute_query(std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
		return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_command));

	// 2) in a coroutine: deferred
	return this->_execute_query_deferred<TQUERY>(std::string(_command));
}

template <class TQUERY, class TA, class... TARGS>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, query::execute_result<object_ptr<TQUERY>>>
query::redis::querier::execute_query(const std::string_view _format, TA _first, TARGS... _args)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
		return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_format, _first, _args...));

	// 2) in a coroutine: deferred
	return this->_execute_query_deferred<TQUERY>(std::vformat(_format, std::make_format_args(_first, _args...)));
}

template <class TQUERY>
query::execute_result<object_ptr<TQUERY>> query::redis::querier::_execute_query_deferred(std::string&& _command)
{
	// - 'co_await' sends the request and resumes the coroutine when the response is received
	auto pquerier = object_ptr<querier>(this);
	std::string command_sync(_command);

	return execute_result<object_ptr<TQUERY>>::deferred(
		[pquerier, command = std::move(command_sync)]() { return pquerier->template execute_query_sync<TQUERY>(command); },
		[pquerier, command = std::move(_command)](object_ptr<TQUERY>& _result, std::function<void()>&& _on_complete)
		{
			// check)
			THROW_IF(pquerier->get_object_state() != eOBJECT_STATE::RUNNING && pquerier->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			auto pquery = pquerier->template alloc_shared_query<TQUERY>();
			pquery->_set_function_on_complete(std::move(_on_complete));
			_result = pquery;
			pquery->post_execute_query(std::string(command));
		});
}

#if defined(__cpp_impl_coroutine)
template <class TQUERY>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, query::execute_result<object_ptr<TQUERY>>>
query::redis::querier::co_execute_query(std::string_view _command)
{
	// 1) 지금 쓸 수 있는 session이 없으면 post한다.
	if (this->_has_session_now() == false)
		return this->co_post_execute_query<TQUERY>(_command);

	// 2) 이 thread에서 바로 실행한다. (응답까지 블로킹. 'co_await'해도 suspend하지 않는다)
	return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_command));
}

template <class TQUERY, class TA, class... TARGS>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, query::execute_result<object_ptr<TQUERY>>>
query::redis::querier::co_execute_query(const std::string_view _format, TA _first, TARGS... _args)
{
	// 1) 지금 쓸 수 있는 session이 없으면 post한다.
	if (this->_has_session_now() == false)
		return this->co_post_execute_query<TQUERY>(_format, _first, _args...);

	// 2) 이 thread에서 바로 실행한다. (응답까지 블로킹. 'co_await'해도 suspend하지 않는다)
	return execute_result<object_ptr<TQUERY>>::executed(this->execute_query_sync<TQUERY>(_format, _first, _args...));
}
#endif

template <class TQUERY>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, void>
query::redis::querier::post_execute_query(std::string_view _command, std::function<void(TQUERY*)>&& _response)
{
	// check) 
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_command'] is empty")));

	// 1) 새로운 Query를 생성한다.
	auto pquery = make_object<TQUERY>();

	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
	{
		// - 이 있을 경우 하나의 session을 얻는다.
		//   (not ahead of the waiting ones : the session a dedicated one waits for is not used by new shared ones any more)
		if (this->m_sessions_stack.empty() == false && this->m_queue_query.empty())
		{
			// - get session
			auto psession = this->process_get_shared_session();

			// - set session to query (in the lock. a dedicated query must not take it while it is not set yet)
			pquery->set_session(psession);

			// - lock leave
			SCOPED_LOCK_LEAVE(cs);

			// - post session
			pquery->_process_post_execute_query(std::string(_command), [xpquery=std::move(pquery), xresponse=std::move(_response)]() {
					xresponse(xpquery);
				});
		}
		// - session이 없을 경우 대기한다.
		else
		{
			// check) stopped (no session will come)
			THROW_IF(this->process_is_running() == false, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			// declare) 
			QUEUED_QUERY temp_query(this);

			// - set event info (free_session() sets a shared session and calls it out of the lock : sends the command like the one above.
			//   the exception is logged by free_session(). the response is not called then, as the one above throws to the caller)
			temp_query.pquery = pquery;
			temp_query.is_shared = true;
			temp_query.f_execute = [pquery, command = std::string(_command), response = std::move(_response)]()
				{
					pquery->_process_post_execute_query(std::string(command), [pquery, response]()
						{
							response(pquery);
						});
				};

			// 2) 먼저 Push한다.
			this->m_queue_query.push_back(std::move( temp_query));
		}
	}
}

template <class TQUERY>
std::enable_if_t<std::is_base_of_v<query::redis::Iquery, TQUERY>, void>
query::redis::querier::post_alloc_query(std::function<void(TQUERY*)>&& _response)
{
	// check) 
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

	// 1) create query object
	auto pquery = make_object<TQUERY>();

	// 2) post execute
	this->post_alloc_session(pquery, [this, response = std::move(_response)](Iquery* _pquery)
	{
		// check)
		CGASSERT_FATAL(_pquery != nullptr);

		// check)
		CGASSERT_ERROR(_pquery->get_session().exist());

		try
		{
			if (response != nullptr)
			{
				response(dynamic_cast<TQUERY*>(_pquery));
			}
		}
		catch (...)
		{
			// statistics) 
			this->statistics_on_query_fail();

			// - reset session
			_pquery->reset_session();
		}
	});
}


}