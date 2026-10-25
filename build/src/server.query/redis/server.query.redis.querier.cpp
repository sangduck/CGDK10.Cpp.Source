#include "pch.h"
#include "../../system/object/platform.h"

using namespace CGDK;

// - a blocked shared allocation waits behind the dedicated ones at most this long, then shares a session in the stack
static constexpr std::chrono::milliseconds TICK_SHARED_WAIT_MAX{ 100 };

query::redis::querier::querier()
{
	// 1) set object type to QUERIER_REDIS
	this->set_object_type(eOBJECT_TYPE::QUERIER_REDIS);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::redis::querier::~querier() noexcept
{
	// 1) unregister
	this->unregister_querier();
}

result_code query::redis::querier::_process_starting(context& _context)
{
    this->m_fixed_routes_enabled.store(true, std::memory_order_release);
    this->close_fixed_routes();
	// trace)
	LOG_PROGRESS << "@ <BEGIN> start redis QUERIER['"sv << name() << "']"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_starting(_context);

	// declare) 
	START_PARAMETER parameter;
	own_ptr<executor::Iio_context> pio_context_own;

	// 2) parameter 읽기
	if (_context.type() == context::value_type_t::buffer_)
	{
		parameter = _context.v.value_buffer.front<START_PARAMETER>();
	}
	else
	{
		// - socket setting을 읽어낸다.
		auto socket_connection_parameter = get_parameter_connection(_context, true);

		// - get remote endpoint
		parameter.remote_endpoint = socket_connection_parameter.remote_endpoint;

		// - session
		{
			// - session Per Source
			auto value_find = _context["session"sv];

			if(value_find.is_valid())
			{
				parameter.count_session = value_find;

				// trace)
				LOG_INFO << "  + parameter['session']: "sv << parameter.count_session;
			}
		}
		// - wait Time
		{
			// - session Per Source
			auto value_find = _context["wait"sv];

			if (value_find.is_valid())
			{
				parameter.wait_seconds = value_find;

				// trace)
				LOG_INFO << "  + parameter['wait']: "sv << parameter.wait_seconds;
			}
		}
		if (const auto value = _context["post_response"sv]; value.is_valid()) parameter.post_response = static_cast<bool>(value);
		if (const auto value = _context["encode_on_session"sv]; value.is_valid()) parameter.encode_on_session = static_cast<bool>(value);
		const auto send_defer_option = _context["send_defer"sv];
		if (send_defer_option.is_valid())
			parameter.enable_send_defer = static_cast<bool>(send_defer_option);
		if (const auto value = _context["request_send_window"sv]; value.is_valid())
		{
			const int64_t window = value;
			THROW_IF(window < 0, throwable::failure(eRESULT::INVALID_ARGUMENT));
			parameter.request_send_window = static_cast<std::size_t>(window);
		}
	}

	// - address가 empty면 localhost로 설정한다.
	if (parameter.remote_endpoint.address().is_unspecified())
	{
		parameter.remote_endpoint.address(net::ip::address_v4::loopback());
	}

	// - Port가 0이면 Redis Default Port로 설정한다.
	if (parameter.remote_endpoint.port() == 0)
	{
		parameter.remote_endpoint.port(process_get_default_port());
	}

	try
	{
		// 2) count_session을 확인한다
		if(parameter.count_session <=0)
		{
			// - session를 Core 객수에 따라 결정한다.
			parameter.count_session = static_cast<int>(api::get_core_count() * 2);
		}

		// 3) executor가 nullptr이면 새로 생성한다.
		if(parameter.pio_context.empty())
		{
			// - executor를 생성한다.
			auto ptemp_io_context = make_own<executor::io_context_threads>();

			// - Thread를 시작한다.
			ptemp_io_context->start({ parameter.count_session });

			// - Executor를 저장...
			parameter.pio_context = ptemp_io_context;
			pio_context_own = std::move(ptemp_io_context);
		}
		else
		{
			// - Thread Pool이 있는지 확인한다.
			auto pthread_pool = dynamic_pointer_cast<api::threads>(parameter.pio_context);

			// check) Thread Pool 이 있다면 Thread의 개수를 확인한다.
			if(pthread_pool.exist())
			{
				// check) Thread의 개수가 현재 session 수보다 적으면 ASSERT건다.
				CGASSERT_ERROR(pthread_pool->get_existing_thread_count()>=static_cast<std::size_t>(parameter.count_session));

				// - trace) Thread의 개수가 현재 session 수보다 적으면 경고한다.
				EXECUTE_IF(static_cast<int>(pthread_pool->get_existing_thread_count()) < parameter.count_session, LOG_INFO << log_level_t::important() << "(prgr) thread count is too small. at least "sv << parameter.count_session << " is needed.  ("sv << name() << ")!"sv);
			}
		}
	}
	catch (std::exception& /*_e*/)
	{
		// Log) 
		LOG_FATAL << "(excp) exception complete[start: "sv << name() << "]"sv;

		// reraise)
		throw;
	}
	catch(...)
	{
		// trace) prepare receive에 실패했다!
		LOG_FATAL << "(excp) exception complete[start: "sv << name() << "]"sv;

		// reraise)
		throw;
	}

	// declare) 
	std::vector<object_ptr<session>> vector_session;

	// - Reserve
	vector_session.reserve(parameter.count_session);

	try
	{
		// - Socket을 생성하고 connect 시도한다.
		for(int i = 0; i < parameter.count_session; ++i)
		{
			// - Redis용 Socket을 생성한다.
			auto psession = make_object<session>();
			psession->m_enable_batch_receive = true;
			psession->m_enable_auto_send = true;
			psession->m_encode_on_session = parameter.encode_on_session;
            psession->m_post_response = parameter.post_response;
            psession->m_response_executor = parameter.presponse_executor.exist() ? parameter.presponse_executor : object_ptr<Iexecutor>(parameter.pio_context);
			psession->m_enable_send_defer = parameter.enable_send_defer;
			psession->m_request_send_window = parameter.request_send_window;
			psession->m_pcoroutine_executor = parameter.pcoroutine_executor.exist() ? parameter.pcoroutine_executor : object_ptr<Iexecutor>(parameter.pio_context);
			psession->m_preserve_coroutine_executor = parameter.pcoroutine_executor.empty();

			// - executor를 설정한다.
			psession->io_context(parameter.pio_context);

			// - socket Redis를 추가한다.
			vector_session.push_back(std::move(psession));
		}
	}
	catch (std::exception& /*_e*/)
	{
		// Log) 
		LOG_FATAL << "(excp) exception complete[start: " << name() << "]";

		// reraise)
		throw;
	}
	catch(...)
	{
		// trace) prepare receive에 실패했다!
		LOG_FATAL << "(excp) exception complete[start: " << name() << "]";

		// reraise)
		throw;
	}

	// 5) 생성한 Socket을 설정한다.
	SCOPED_LOCK(this->m_lockable_query)
	{
		this->m_pcoroutine_executor = parameter.pcoroutine_executor.exist() ? parameter.pcoroutine_executor : object_ptr<Iexecutor>(parameter.pio_context);
		this->m_preserve_coroutine_executor = parameter.pcoroutine_executor.empty();
		this->m_pio_context = std::move(parameter.pio_context);
		this->m_pio_context_own = std::move(pio_context_own);

		for(auto& iter:vector_session)
		{
			this->register_session(iter);
		}
	}

	// 6) connect session
	for (auto iter = vector_session.begin(); iter != vector_session.end();)
	{
		// - 재접속이 되도록 설정한다.
		(*iter)->enable_reconnect();

		// - try connecting
		auto result = (*iter)->start({ parameter.remote_endpoint, {}, chrono::tick::seconds{5}, 0, 0, 0 });

		// check)
		if (result != eRESULT::SUCCESS)
		{
			this->unregister_session(*iter);
			iter = vector_session.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// check) wait for Completion of Establishment
	//if (parameter.wait_seconds != 0)
	if (parameter.wait_seconds >= 0)
	{
		// - 현재 시간 계산하기
		auto time_end = chrono::tick::now() + chrono::seconds((parameter.wait_seconds != 0) ? parameter.wait_seconds : 5);

		// - wait until every session is established or closed (failed to connect). the ones still connecting at the end are kept
		while (chrono::tick::now() < time_end)
		{
			auto iter = vector_session.begin();

			while(iter != vector_session.end())
			{
				auto socket_state = (*iter)->socket_state();

				if (socket_state >= eSOCKET_STATE::ESTABLISHED)
				{
					++iter;
				}
				else if (socket_state == eSOCKET_STATE::CLOSED)
				{
					this->unregister_session(*iter);
					iter = vector_session.erase(iter);
				}
				else
				{
					break;
				}
			}

			if (iter == vector_session.end())
				break;

			// - sleep
			std::this_thread::sleep_for(1ms);
		}
	}

	// - established
	const auto count_established = std::count_if(vector_session.begin(), vector_session.end(), [](const object_ptr<session>& _psession) { return _psession->socket_state() >= eSOCKET_STATE::ESTABLISHED; });

	// - failed : no session left, or none established after waiting
	if (vector_session.empty() || (parameter.wait_seconds >= 0 && count_established == 0))
	{
		// check) 접속에 실패했을 경우
		// - unregister the rest (closed. not reconnected)
		for (auto& iter : vector_session)
			this->unregister_session(iter);

		// - release executor
		decltype(this->m_pio_context_own) pio_context_own_fail;
		object_ptr<Iexecutor> pcoroutine_executor_fail;

		SCOPED_LOCK(this->m_lockable_query)
		{
			pcoroutine_executor_fail = std::move(this->m_pcoroutine_executor);
			this->m_pio_context.reset();
			pio_context_own_fail = std::move(this->m_pio_context_own);
		}

		if (pio_context_own_fail.exist())
			pio_context_own_fail.destroy();

		// raise)
		throw throwable::failure(eRESULT::FAIL_TO_CONNECT, 0, "connected redis session is 0"sv);
	}

	this->open_fixed_routes(std::move(vector_session));

	// trace) 
	LOG_PROGRESS << "  > query::redis: success connecting To redis server ("sv << name() << ")!"sv;
	LOG_PROGRESS << log_continue_t() << "    - remote endpoint: "sv << ((parameter.remote_endpoint.address().is_loopback() == false) ? parameter.remote_endpoint.address().to_string().c_str() : "BIND_ANY") << ":"sv << parameter.remote_endpoint.port();
	LOG_PROGRESS << log_continue_t() << "    - requested connection: "sv << parameter.count_session;
	LOG_PROGRESS << log_continue_t() << "    - Established connection  : "sv << count_established << " (ReqLimit:"sv << parameter.count_session << ", sessions:"sv << this->m_fixed_sessions.size() << ")"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code query::redis::querier::_process_start(context& _context)
{
	// trace)
	LOG_PROGRESS << "@ <END> start redis QUERIER['"sv << name() << "']"sv;

	// 1) ...
	return Nstartable::_process_start(_context);
}

result_code query::redis::querier::_process_stopping() noexcept
{
    this->close_fixed_routes();
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop redis QUERIER['"sv << name() << "']"sv;

	// 1) call 'on_stopping'
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
	}

	// declare) 
	decltype(this->m_sessions_registered) sessions;
	std::vector<object_ptr<querier>> queriers_detached;
	circular_list<QUEUED_QUERY> list_queued;
	object_ptr<Iexecutor> pcoroutine_executor;
	decltype(this->m_pio_context) pio_context;
	decltype(this->m_pio_context_own) pio_context_own;

	// 3) apply 
	SCOPED_LOCK(m_lockable_query)
	{
		// - remove sessions (all the registered ones. a dedicated one in use is not in the stack) and detach them
		sessions = std::move(this->m_sessions_registered);
		queriers_detached.reserve(sessions.size());
		for (auto& iter : sessions)
			queriers_detached.push_back(this->process_detach_session(iter));
		this->m_sessions_stack.clear();
        this->m_fixed_sessions.clear();

		// - the queries waiting for a session (no session will come)
		list_queued = std::move(this->m_queue_query);
		this->m_has_queued_query.store(false, std::memory_order_release);

		// - remove executor
		pcoroutine_executor = std::move(this->m_pcoroutine_executor);
		pio_context = std::move(this->m_pio_context);
		pio_context_own = std::move(this->m_pio_context_own);
	}

	// 4) close all sessions and cancel all requests
	for(auto& iter: sessions)
	{
		// - SessionSocket을 unregister한다.
		this->process_close_session(iter);
	}

	// 5) cancel the queries waiting for a session
	this->process_cancel_queued(list_queued);

	// 6) close 'executor'
	pio_context.reset();
	if(pio_context_own.exist())
	{
		pio_context_own.destroy();
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code query::redis::querier::_process_stop() noexcept
{
	// 1) call 'on_stop'
	try
	{
		this->on_stop();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stop' function call redis QUERIER['" << name() << "']";
	}

	// trace)
	LOG_PROGRESS << "@ <END> stop redis QUERIER['" << name() << "']";

	// return)
	return eRESULT::SUCCESS;
}

result_code query::redis::querier::_process_destroy() noexcept
{
	// 1) call Parent's _process_destroy function
	auto result = this->object::Ninitializable::_process_destroy();

	// 2) unregister all messageable objects
	this->reset_message_transmitter();

	// return0
	return result;
}

bool query::redis::querier::cancel_query(Iserver_query* /*_pquery*/) noexcept
{
	return true;
}

void query::redis::querier::process_execute_query(Iserver_query* /*_pquery*/)
{
}

void query::redis::querier::process_post_execute_query(Iserver_query* /*_pquery*/)
{
}

void query::redis::querier::process_execute_query(Iquery* /*_pquery*/)
{
}
void query::redis::querier::process_post_execute_query(Iquery* /*_pquery*/)
{
}

void query::redis::querier::register_session(session* _psession)
{
	// declare)
	object_ptr<querier>	pquerier_old;

	SCOPED_LOCK(this->m_lockable_query)
	{
		// check) already exist?
		CGASSERT_ERROR(std::find(this->m_sessions_registered.begin(), this->m_sessions_registered.end(), _psession) == this->m_sessions_registered.end());

		// check)
		RETURN_IF(std::find(this->m_sessions_registered.begin(), this->m_sessions_registered.end(), _psession) != this->m_sessions_registered.end());

		// 1) register session
		this->m_sessions_registered.push_back(_psession);
		this->m_sessions_stack.push_back(_psession);
		_psession->m_is_dedicated = false;

		// 2) set querier
		pquerier_old = _psession->process_attach_querier(this);
	}

	// statistics)
	this->statistics_on_create_session();
}

void query::redis::querier::unregister_session(session* _psession) noexcept
{
	// check)
	RETURN_IF(_psession == nullptr);

	// declare)
	object_ptr<session>	psession_hold;
	object_ptr<querier>	pquerier_hold;

	// 1) remove from querier (a dedicated one in use too) and detach it
	SCOPED_LOCK(this->m_lockable_query)
	{
		// - find session
		auto iter_find = std::find(this->m_sessions_registered.begin(), this->m_sessions_registered.end(), _psession);

		// check)
		RETURN_IF(iter_find == this->m_sessions_registered.end());

		// - remove session
		psession_hold = std::move(*iter_find);
		this->m_sessions_registered.erase(iter_find);

		// - detach (removed from the stack)
		pquerier_hold = this->process_detach_session(_psession);
	}

	// 2) close it and cancel its requests
	this->process_close_session(_psession);
}

object_ptr<query::redis::querier> query::redis::querier::process_detach_session(session* _psession) noexcept
{
	// - (holding 'm_lockable_query') removed from the stack
	auto iter_stack = std::find(this->m_sessions_stack.begin(), this->m_sessions_stack.end(), _psession);

	if (iter_stack != this->m_sessions_stack.end())
		this->m_sessions_stack.erase(iter_stack);

	_psession->close_shared_routes();
    _psession->m_is_dedicated = false;
    _psession->m_query_reference.fetch_or(session::dedicated_reference, std::memory_order_acq_rel);

	// - detached : its release does not go back to free_session() any more
	bool is_in_use = false;
	auto pquerier = _psession->process_detach_querier(is_in_use);

	// statistics) in use : the free is counted here (its last release is not counted)
	if (is_in_use)
		this->statistics_on_free_session();

	// return) released out of the lock
	return pquerier;
}

void query::redis::querier::process_close_session(session* _psession) noexcept
{
	// 1) disable 'reconnection' and taken out of the reconnector now (holding the session : its last release must not happen in the reconnector)
	_psession->disable_reconnect();
	_psession->process_cancel_reconnector();

	// 2) close session
	_psession->closesocket();

	// 3) cancel the requests waiting for responses (closed first : no more request is sent)
	_psession->process_cancel_query_all(this);

	// statistics) 
	this->statistics_on_destroy_session();
}

result_code query::redis::querier::process_message(sMESSAGE& _msg)
{
	// 1) Log Message Dispatch
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

result_code query::redis::querier::process_transmit_message(sMESSAGE& _msg)
{
	// 1) dispatch to 
	result_code	result = this->Nmessage_transmitter::process_transmit_message(_msg);

	//// 2) Querier Manager에 Dispatch한다.
	//query::manager_querier::instance()->transmit_message(_msg);

	// return) 
	return result;
}

void query::redis::querier::alloc_session(Iquery* _pquery)
{
    THROW_IF(this->m_fixed_routes_enabled.load(std::memory_order_acquire) &&
        (this->m_route_access.load(std::memory_order_acquire) & routes_closed), throwable::failure(eRESULT::NOT_READY));
	// check) 
	CGASSERT_FATAL(_pquery != nullptr);

	// check) 
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
	{
        this->ensure_fixed_routes_open();
		// 1) 이 있을 경우 하나의 session을 얻는다.
		if (auto psession = this->process_take_unused_session(); psession.exist())
		{
			// - set session to query
			_pquery->set_session(psession);
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// check) stopped (no session will come)
			THROW_IF(this->process_is_running() == false, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			// declare) 
			QUEUED_QUERY temp_query(this);

			// 1) set event info
			temp_query.pquery = _pquery;
			temp_query.f_execute = [=]()
				{
					_pquery->m_cv_complete.notify_one();
				};
			temp_query.f_cancel = temp_query.f_execute;

			// 2) 먼저 Push한다.
			this->m_queue_query.push_back(std::move(temp_query));
			this->m_has_queued_query.store(true, std::memory_order_release);

			// 3) wait for session (until free_session() hands one over. throws eRESULT::CANCEL when the querier stops)
			this->process_wait_queued(_pquery, cs);
		}
	}

	// check) m_psession이 nullptr이면 안 된다.
	CGASSERT_ERROR(_pquery->get_session().exist());
}

object_ptr<query::redis::session> query::redis::querier::process_take_unused_session() noexcept
{
	// - (holding 'm_lockable_query') a dedicated query takes only a session no query uses. (a shared one may be in use)
	for (auto iter = this->m_sessions_stack.end(); iter != this->m_sessions_stack.begin();)
	{
		--iter;

		// check) used by shared queries
		uint32_t expected = 0;
        if (!(*iter)->m_query_reference.compare_exchange_strong(expected, session::dedicated_reference, std::memory_order_acq_rel))
			continue;

		// 1) take it out of the stack
		auto psession = std::move(*iter);
		this->m_sessions_stack.erase(iter);

		// 2) dedicated (goes back to the stack by free_session())
		psession->m_is_dedicated = true;

		// return)
		return psession;
	}

	// return) none
	return {};
}

bool query::redis::querier::_try_alloc_session(Iquery* _pquery)
{
    THROW_IF(this->m_fixed_routes_enabled.load(std::memory_order_acquire) &&
        (this->m_route_access.load(std::memory_order_acquire) & routes_closed), throwable::failure(eRESULT::NOT_READY));
	// check)
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	SCOPED_LOCK(this->m_lockable_query)
	{
        this->ensure_fixed_routes_open();
		// - a session no query uses
		auto psession = this->process_take_unused_session();

		// check) 지금 쓸 수 있는 session이 없다. (기다리지 않는다)
		if (psession.empty())
			return false;

		// 1) session을 하나 꺼내 query에 붙인다.
		_pquery->set_session(psession);
	}

	// return)
	return true;
}

bool query::redis::querier::_alloc_session_or_queue(Iquery* _pquery, std::function<void()>&& _f_allocated)
{
    THROW_IF(this->m_fixed_routes_enabled.load(std::memory_order_acquire) &&
        (this->m_route_access.load(std::memory_order_acquire) & routes_closed), throwable::failure(eRESULT::NOT_READY));
	// check)
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	SCOPED_LOCK(this->m_lockable_query)
	{
        this->ensure_fixed_routes_open();
		// 1) session이 있으면 바로 붙인다.
		if (auto psession = this->process_take_unused_session(); psession.exist())
		{
			_pquery->set_session(psession);
		}
		// 2) 없으면 queue에 넣는다. (free_session()이 session을 붙인 뒤 '_f_allocated'를 부른다. 같은 lock 안에서 넣으므로 그 사이에 생긴 session을 놓치지 않는다)
		else
		{
			// check) stopped (no session will come)
			THROW_IF(this->process_is_running() == false, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			QUEUED_QUERY temp_query(this);

			temp_query.pquery = _pquery;
			temp_query.f_execute = std::move(_f_allocated);

			this->m_queue_query.push_back(std::move(temp_query));
			this->m_has_queued_query.store(true, std::memory_order_release);

			// return)
			return false;
		}
	}

	// return)
	return true;
}

bool query::redis::querier::_has_session_now()
{
	SCOPED_LOCK(this->m_lockable_query)
	{
		// - a shared query does not go ahead of the waiting ones
		return this->m_sessions_stack.empty() == false && this->m_queue_query.empty();
	}
}

bool query::redis::querier::try_alloc_callback_session(Iquery* query)
{
	const auto scope = t_callback_scope;
	if (!scope || scope->owner != this || this->m_has_queued_query.load(std::memory_order_acquire) || !this->process_is_running())
		return false;
	auto session = scope->selected_session.exist() ? scope->selected_session.get() : (scope->may_reuse ? scope->psession : nullptr);
	if (!session) return false;
	std::lock_guard<std::mutex> lock(session->m_cs_queue_encoding);
	if (session->m_pquerier.get() != this || session->m_is_dedicated.load(std::memory_order_relaxed) ||
		session->query_reference_count() == 0 ||
		this->m_has_queued_query.load(std::memory_order_acquire) || session->socket_state() != eSOCKET_STATE::ESTABLISHED)
		return false;
	// A fresh query is still private to the caller. The callback already holds
	// a session reference, so attaching cannot be the 0->1 allocation transition.
	if (query->m_psession.exist() || query->m_state_query != eOBJECT_STATE::NONE) return false;
	query->m_psession = session;
	query->m_state_query = eOBJECT_STATE::READY;
	++session->m_query_reference;
	return true;
}

void query::redis::querier::alloc_shared_session(Iquery* _pquery)
{
	// check) 
	CGASSERT_FATAL(_pquery != nullptr);

	// check) 
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

    if (this->try_alloc_fixed_session(_pquery)) return;

	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
	{
        this->ensure_fixed_routes_open();
		// 1) 이 있을 경우 하나의 session을 얻는다.
		//    (not ahead of the waiting ones : the session a dedicated one waits for is not used by new shared ones any more)
		if (this->m_sessions_stack.empty() == false && this->m_queue_query.empty())
		{
			// - get session (stays in the stack. used with other shared queries)
			auto psession = this->process_get_shared_session_locked();

			// - set session to query
			_pquery->set_session(psession);
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// check) stopped (no session will come)
			THROW_IF(this->process_is_running() == false, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			// declare) 
			QUEUED_QUERY temp_query(this);

			// 1) set event info
			temp_query.pquery = _pquery;
			temp_query.is_shared = true;
			temp_query.f_execute = [=]()
				{
					_pquery->m_cv_complete.notify_one();
				};
			temp_query.f_cancel = temp_query.f_execute;

			// 2) 먼저 Push한다.
			this->m_queue_query.push_back(std::move(temp_query));
			this->m_has_queued_query.store(true, std::memory_order_release);

			// 3) wait for session (until free_session() hands one over. throws eRESULT::CANCEL when the querier stops)
			//    (shared : shares a session in the stack after TICK_SHARED_WAIT_MAX)
			this->process_wait_queued(_pquery, cs, true);
		}
	}

	// check) m_psession이 nullptr이면 안 된다.
	CGASSERT_ERROR(_pquery->get_session().exist());
}

void query::redis::querier::free_session(session* _psession) noexcept
{
	// check) 
	CGASSERT_ERROR(_psession != nullptr);

	// check) 
	RETURN_IF(_psession == nullptr);

	// - called when the last query released the session (statistics: counted by session::release_query_reference())

	// declare) 
	QUEUED_QUERY pqueued_query(this);
	bool is_shared_waiting = false;

	SCOPED_LOCK(this->m_lockable_query)
	{
		// check) unregistered before the lock (detached. not back to the stack)
		RETURN_IF(_psession->m_pquerier.get() != this);

		// check) used again before the lock (a shared query got it, or a dedicated one took it out of the stack)
		RETURN_IF(_psession->query_reference_count() != 0);

		// 1) 더 이상 Queuing된 Query가 없으면 session을 스택시켜놓는다.
		//    (a dedicated one goes back to the stack. a shared one is in the stack already : not pushed twice.
		//     a shared query waiting at the front uses it in the stack)
		if (this->m_queue_query.empty() || this->m_queue_query.front().is_shared)
		{
			if (_psession->m_is_dedicated)
			{
				_psession->m_is_dedicated = false;
				this->m_sessions_stack.push_back(_psession);
                _psession->m_query_reference.fetch_and(session::reference_mask, std::memory_order_release);
			}

			// return) no query waiting
			RETURN_IF(this->m_queue_query.empty());
		}
		// - a dedicated query waiting at the front takes it (nobody uses it now. a shared one is taken out of the stack)
		else if (_psession->m_is_dedicated == false)
		{
            uint32_t expected = 0;
            if (!_psession->m_query_reference.compare_exchange_strong(expected, session::dedicated_reference, std::memory_order_acq_rel)) return;
			auto iter_find = std::find(this->m_sessions_stack.begin(), this->m_sessions_stack.end(), _psession);

			if (iter_find != this->m_sessions_stack.end())
				this->m_sessions_stack.erase(iter_find);

			_psession->m_is_dedicated = true;
		}

		// 2) get queued query
		pqueued_query = std::move(this->m_queue_query.front());

		// 3) pop query
		this->m_queue_query.pop_front();
		this->m_has_queued_query.store(!this->m_queue_query.empty(), std::memory_order_release);

		// 4) set session
		pqueued_query.pquery->set_session(_psession);

		// - more shared queries waiting at the front (the sessions in the stack are handed to them)
		is_shared_waiting = this->m_queue_query.empty() == false && this->m_queue_query.front().is_shared && this->m_sessions_stack.empty() == false;
	}

	// 5) execute query
	this->process_execute_queued(pqueued_query);

	// 6) the shared queries waiting next
	if (is_shared_waiting)
		this->process_execute_queued_shared();
}

void query::redis::querier::process_execute_queued(QUEUED_QUERY& _queued_query) noexcept
{
	// check)
	RETURN_IF(!_queued_query.f_execute);

	try
	{
		_queued_query.f_execute();
	}
	catch (...)
	{
		// log) not executed (the response is not called)
		LOG_ERROR << "(err ) query::redis: fail to execute the queued query ("sv << this->name() << ")"sv;
	}
}

void query::redis::querier::process_execute_queued_shared() noexcept
{
	for (;;)
	{
		// declare)
		QUEUED_QUERY pqueued_query(this);

		SCOPED_LOCK(this->m_lockable_query)
		{
			// check) a shared query waiting at the front and a session in the stack
			RETURN_IF(this->m_queue_query.empty() || this->m_queue_query.front().is_shared == false || this->m_sessions_stack.empty());

			// 1) get queued query
			pqueued_query = std::move(this->m_queue_query.front());
			this->m_queue_query.pop_front();
			this->m_has_queued_query.store(!this->m_queue_query.empty(), std::memory_order_release);

			// 2) set a shared session (stays in the stack)
			auto psession = this->process_get_shared_session_locked();
			pqueued_query.pquery->set_session(psession);
		}

		// 3) execute query
		this->process_execute_queued(pqueued_query);
	}
}

void query::redis::querier::process_wait_queued(Iquery* _pquery, std::unique_lock<std::recursive_mutex>& _lock, bool _is_shared)
{
	// - (holding 'm_lockable_query' once. the wait releases it) free_session() sets the session holding the lock, then notifies
	const auto tick_shared_wait_end = std::chrono::steady_clock::now() + TICK_SHARED_WAIT_MAX;

	while (_pquery->get_session().empty())
	{
		// - find the queued one
		auto iter_find = std::find_if(this->m_queue_query.begin(), this->m_queue_query.end(), [=](const QUEUED_QUERY& _queued) { return _queued.pquery.get() == _pquery; });

		// check) canceled : taken out of the queue without a session (the querier stops)
		THROW_IF(iter_find == this->m_queue_query.end(), throwable::failure(eRESULT::CANCEL, 0, std::string_view("session allocation is canceled")));

		// check) shared : waited behind the dedicated ones long enough -> shares a session in the stack
		//    (the thread may hold a shared query of the session the dedicated one waits for. it must not wait for it forever)
		const bool is_shared_wait_over = _is_shared && std::chrono::steady_clock::now() >= tick_shared_wait_end;

		if (is_shared_wait_over && this->m_sessions_stack.empty() == false)
		{
			this->m_queue_query.erase(iter_find);
			this->m_has_queued_query.store(!this->m_queue_query.empty(), std::memory_order_release);

			auto psession = this->process_get_shared_session_locked();
			_pquery->set_session(psession);
			break;
		}

		// - wait (woken up spuriously : waits again)
		if (_is_shared && is_shared_wait_over == false)
			_pquery->m_cv_complete.wait_until(_lock, tick_shared_wait_end);
		else
			_pquery->process_wait(_lock);
	}
}

void query::redis::querier::process_cancel_queued(circular_list<QUEUED_QUERY>& _list_queued) noexcept
{
	// 1) the blocked ones wake up (alloc_session()/alloc_shared_session() throw eRESULT::CANCEL)
	for (auto& iter : _list_queued)
	{
		// check)
		if (!iter.f_cancel)
			continue;

		try
		{
			iter.f_cancel();
		}
		catch (...)
		{
		}
	}

	// 2) released : co_alloc_query() resumes with eRESULT::CANCEL. post_alloc_session()/post_execute_query() are not called
	_list_queued.clear();
}

void query::redis::querier::post_alloc_session(Iquery* _pquery, std::function<void(Iquery*)>&& _response)
{
	// check) 
	CGASSERT_FATAL(_pquery != nullptr);

	// check) 
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
	{
        this->ensure_fixed_routes_open();
		// 1) allocate session and go ahead
		if (auto psession = this->process_take_unused_session(); psession.exist())
		{
			// - set session to query
			_pquery->set_session(psession);

			// check) 
			CGASSERT_ERROR(_pquery->get_session().exist());
		}
		// 2) queuing request
		else
		{
			// check) stopped (no session will come)
			THROW_IF(this->process_is_running() == false, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

			// declare) 
			QUEUED_QUERY temp_query(this);

			temp_query.pquery = _pquery;
			temp_query.f_execute = [_pquery, response=std::move(_response)]()
				{
					if (response)
						response(_pquery);
				};

			// - push
			this->m_queue_query.push_back(std::move(temp_query));
			this->m_has_queued_query.store(true, std::memory_order_release);

			// return) called by free_session() when a session is freed (out of the lock)
			return;
		}
	}

	// 3) response (out of the lock)
	if (_response)
		_response(_pquery);
}

//void query::redis::querier::post_alloc_shared_session(Iquery* _pquery, std::function<void()>&& _f_response)
//{
//	// check) 
//	CGASSERT_ERROR(_pquery != nullptr);
//
//	// check) 
//	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));
//
//	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
//	{
//		// 1) allocate session and go ahead
//		if (this->m_sessions_stack.empty() == false)
//		{
//			// - get session
//			auto psession = this->m_sessions_stack.front();
//
//			// check) 
//			CGASSERT_ERROR(psession.exist());
//
//			// - pop session
//			this->m_sessions_stack.pop_front();
//
//			// - set session to query
//			_pquery->set_session(psession);
//
//			// check) 
//			CGASSERT_ERROR(_pquery->get_session().exist());
//		}
//		// 2) queuing request
//		else
//		{
//			// declare) 
//			QUEUED_QUERY temp_query(this);
//
//			temp_query.pquery = _pquery;
//			temp_query.f_execute = std::move(_f_response);
//
//			// - push
//			this->m_queue_query.push_back(temp_query);
//		}
//	}
//}

namespace {
std::vector<std::string> own_redis_commands(std::span<const std::string_view> commands)
{
    THROW_IF(commands.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));
    std::vector<std::string> result; result.reserve(commands.size());
    for (auto command : commands) {
        THROW_IF(command.find_first_not_of(" \t") == std::string_view::npos, throwable::failure(eRESULT::INVALID_ARGUMENT));
        result.emplace_back(command);
    }
    return result;
}
}

void query::redis::querier::post_execute_query(std::span<const std::string_view> commands, batch_callback response)
{
	auto owned_commands = own_redis_commands(commands);
	auto pquery = make_object<redis::query>();

	struct completion
	{
		std::atomic<bool> done{false};
		object_ptr<redis::query> pquery;
		batch_callback response;
	};
	auto state = std::make_shared<completion>();
	state->pquery = pquery;
	state->response = std::move(response);
	auto finish = [state, count = commands.size()]() {
        if (state->done.exchange(true)) return;
        try { state->pquery->m_result.vector_child.resize(count); } catch (...) {}
        if (state->response) state->response(std::move(state->pquery->m_result.vector_child));
    };
	auto execute = [pquery, commands = std::move(owned_commands), count = commands.size(), finish]() mutable {
        uint64_t generation = 0;
		try
		{
			generation = pquery->_begin_request();
            pquery->_publish_request(generation);
			session::REQUEST_QUERY request;
			request.pquery = pquery;
			request.response = finish;
			request.count_batch = count;
			pquery->get_session()->post_batch(std::move(request), std::move(commands));
		}
		catch (...)
		{
			if (generation) pquery->_abort_request(generation);
            // A rejected submission must release the query before completing the batch.
			try { finish(); } catch (...) {}
		}
	};
    if (this->try_alloc_fixed_session(pquery.get())) { execute(); return; }

	{
		std::unique_lock<std::recursive_mutex> lock(this->m_lockable_query);
        this->ensure_fixed_routes_open();
		THROW_IF(!this->process_is_running(), throwable::failure(eRESULT::NOT_READY));
		if (!this->m_sessions_stack.empty() && this->m_queue_query.empty())
		{
			pquery->set_session(this->process_get_shared_session_locked());
		}
		else
		{
			QUEUED_QUERY queued(this);
			queued.pquery = pquery;
			queued.is_shared = true;
			queued.f_execute = std::move(execute);
			queued.f_cancel = std::move(finish);
			this->m_queue_query.push_back(std::move(queued));
			this->m_has_queued_query.store(true, std::memory_order_release);
			return;
		}
	}
	execute();
}
namespace
{
	std::vector<std::string_view> redis_command_views(std::span<const std::string> commands)
	{
		std::vector<std::string_view> views;
		views.reserve(commands.size());
		for (const auto& command : commands) views.emplace_back(command);
		return views;
	}
}

void query::redis::querier::post_execute_query(std::span<const std::string> commands, batch_callback response)
{
	auto views = redis_command_views(commands);
	this->post_execute_query(std::span<const std::string_view>(views), std::move(response));
}

object_ptr<query::redis::query> query::redis::querier::_execute_query_sync(std::span<const std::string_view> commands)
{
	THROW_IF(!this->process_is_running(), throwable::failure(eRESULT::NOT_READY));
	auto owned_commands = own_redis_commands(commands);
	auto pquery = make_object<redis::query>();

	// Flush/suspend deferred sends and lend the executor worker while waiting.
	// Use the existing synchronous shared-session allocator, including its FIFO
	// and 100ms shared fallback, rather than waiting on an asynchronous callback.
	scoped_thread_block thread_block;
	this->alloc_shared_session(pquery);
	const auto generation = pquery->_begin_request();
	pquery->_publish_request(generation);
	try
	{
		session::REQUEST_QUERY request;
		request.pquery = pquery;
		request.count_batch = commands.size();
		pquery->get_session()->post_batch(std::move(request), std::move(owned_commands));
		pquery->_wait_request(generation, false);
	}
	catch (...)
	{
		pquery->_abort_request(generation);
		throw;
	}
	return pquery;
}

object_ptr<query::redis::query> query::redis::querier::_execute_query_sync(std::span<const std::string> commands)
{
	auto views = redis_command_views(commands);
	return this->_execute_query_sync(std::span<const std::string_view>(views));
}

query::execute_result<object_ptr<query::redis::query>> query::redis::querier::execute_query(std::span<const std::string_view> commands)
{
	if (!is_deferred_execution())
		return execute_result<object_ptr<redis::query>>::executed(this->_execute_query_sync(commands));

	// A deferred coroutine must own the inputs until it is awaited/executed.
	auto owned = std::make_shared<std::vector<std::string>>();
	owned->reserve(commands.size());
	for (auto command : commands) owned->emplace_back(command);
	auto pquerier = object_ptr<querier>(this);
	auto resume = this->coroutine_resume_policy();
	return execute_result<object_ptr<redis::query>>::deferred(
		[pquerier, owned]() { return pquerier->_execute_query_sync(std::span<const std::string>(*owned)); },
		[pquerier, owned](object_ptr<redis::query>& result, std::function<void()>&& complete) {
			// Allocate before starting: a failure here propagates to the awaiter,
			// rather than throwing inside a response callback and losing its wakeup.
			auto pquery = make_object<redis::query>();
			result = pquery;
			pquerier->post_execute_query(std::span<const std::string>(*owned),
				[pquery, complete = std::move(complete)](batch_results&& results) {
					pquery->m_result.type = eRESP_DATA_TYPE::ARRAY_;
					pquery->m_result.vector_child = std::move(results);
					pquery->m_state_query.store(eOBJECT_STATE::PROCESSED, std::memory_order_release);
					complete();
				});
		}, std::move(resume.first), resume.second);
}

query::execute_result<object_ptr<query::redis::query>> query::redis::querier::execute_query(std::span<const std::string> commands)
{
	auto views = redis_command_views(commands);
	return this->execute_query(std::span<const std::string_view>(views));
}
void query::redis::querier::close_fixed_routes() noexcept
{
    auto access = this->m_route_access.fetch_or(routes_closed, std::memory_order_acq_rel) | routes_closed;
    while (access != routes_closed) {
        this->m_route_access.wait(access, std::memory_order_acquire);
        access = this->m_route_access.load(std::memory_order_acquire);
    }
    for (auto& target : this->m_fixed_sessions) target->close_shared_routes();
}
void query::redis::querier::open_fixed_routes(std::vector<object_ptr<session>> sessions)
{
    this->close_fixed_routes();
    this->m_fixed_sessions = std::move(sessions);
    for (auto& target : this->m_fixed_sessions) target->m_shared_route_access.store(0, std::memory_order_release);
    static std::atomic<uint64_t> epochs{0};
    this->m_route_epoch.store(epochs.fetch_add(1, std::memory_order_relaxed) + 1, std::memory_order_release);
    this->m_fixed_routes_enabled.store(true, std::memory_order_release);
    this->m_route_access.store(0, std::memory_order_release);
}
bool query::redis::querier::try_alloc_fixed_session(Iquery* query)
{
    if (!this->m_fixed_routes_enabled.load(std::memory_order_acquire)) return false;
    this->ensure_fixed_routes_open();
    if (this->m_has_queued_query.load(std::memory_order_acquire)) return false;
    if (query->m_psession.exist() || query->m_state_query.load(std::memory_order_relaxed) != eOBJECT_STATE::NONE) return false;
    const auto scope = t_callback_scope;
    struct route_cache { querier* owner = nullptr; uint64_t epoch = 0; object_ptr<session> target; };
    static thread_local route_cache cache;
    const auto epoch = this->m_route_epoch.load(std::memory_order_acquire);
    auto attach = [&](session* target) -> bool {
        auto access = target->m_shared_route_access.load(std::memory_order_acquire);
        do {
            if (access & session::shared_routes_closed) return false;
        } while (!target->m_shared_route_access.compare_exchange_weak(access, access + 1, std::memory_order_acquire));
        struct guard {
            std::atomic<uint64_t>& access;
            ~guard() { if (access.fetch_sub(1, std::memory_order_release) == (routes_closed | 1)) access.notify_all(); }
        } hold{target->m_shared_route_access};
        auto count = target->m_query_reference.load(std::memory_order_acquire);
        while (!(count & session::dedicated_reference)) {
            if ((count & session::reference_mask) == session::reference_mask) return false;
            if (!target->m_query_reference.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel)) continue;
            query->m_psession = target; query->m_state_query.store(eOBJECT_STATE::READY, std::memory_order_relaxed);
            if (count == 0) this->statistics_on_alloc_session();
            if (scope && scope->owner == this && !scope->may_reuse && scope->selected_session.empty()) {
                scope->selected_session = target; scope->selected_query = query;
            }
            return true;
        }
        return false;
    };
    if (scope && scope->owner == this) {
        auto preferred = scope->selected_session.exist() ? scope->selected_session.get() : (scope->may_reuse ? scope->psession : nullptr);
        if (preferred && attach(preferred)) return true;
    } else if (cache.owner == this && cache.epoch == epoch && cache.target.exist() && attach(cache.target.get())) return true;
    // Only a cache miss reads the fixed vector under the lifecycle reader gate.
    auto access = this->m_route_access.load(std::memory_order_acquire);
    for (;;) {
        THROW_IF(access & routes_closed, throwable::failure(eRESULT::NOT_READY));
        if (this->m_route_access.compare_exchange_weak(access, access + 1, std::memory_order_acquire)) break;
    }
    struct reader_guard {
        std::atomic<uint64_t>& access;
        ~reader_guard() { if (access.fetch_sub(1, std::memory_order_release) == (routes_closed | 1)) access.notify_all(); }
    } guard{this->m_route_access};
    THROW_IF(epoch != this->m_route_epoch.load(std::memory_order_acquire), throwable::failure(eRESULT::CANCEL));
    const auto size = this->m_fixed_sessions.size();
    if (!size) throw throwable::failure(eRESULT::NOT_READY);
    const auto first = scope && scope->owner == this && !scope->may_reuse ?
        this->m_next_callback_session.fetch_add(1, std::memory_order_relaxed) % size : _get_thread_index() % size;
    for (std::size_t offset = 0; offset < size; ++offset) {
        auto target = this->m_fixed_sessions[(first + offset) % size].get();
        if (!attach(target)) continue;
        if (!scope || scope->owner != this) { cache.owner = this; cache.epoch = epoch; cache.target = target; }
        return true;
    }
    return false;
}

std::pair<object_ptr<Iexecutor>, bool> query::redis::querier::coroutine_resume_policy()
{
    if (!this->m_fixed_routes_enabled.load(std::memory_order_acquire)) {
        std::lock_guard<std::recursive_mutex> lock(this->m_lockable_query);
        return {this->m_pcoroutine_executor, this->m_preserve_coroutine_executor};
    }
    auto access = this->m_route_access.load(std::memory_order_acquire);
    for (;;) {
        THROW_IF(access & routes_closed, throwable::failure(eRESULT::NOT_READY));
        if (this->m_route_access.compare_exchange_weak(access, access + 1, std::memory_order_acquire)) break;
    }
    struct guard {
        std::atomic<uint64_t>& access;
        ~guard() { if (access.fetch_sub(1, std::memory_order_release) == (routes_closed | 1)) access.notify_all(); }
    } hold{this->m_route_access};
    return {this->m_pcoroutine_executor, this->m_preserve_coroutine_executor};
}
