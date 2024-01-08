#include "pch.h"

using namespace CGDK;


query::redis::querier::querier()
{
	// 1) set object type to QUERIER_REDIS
	set_object_type(eOBJECT_TYPE::QUERIER_REDIS);

	// 2) Register Querier to Manager
	register_querier();
}

query::redis::querier::querier(const std::string_view _name) :
	Nnameable(_name)
{
	// 1) set object type to QUERIER_REDIS
	set_object_type(eOBJECT_TYPE::QUERIER_REDIS);

	// 2) Register Querier to Manager
	register_querier();
}

query::redis::querier::~querier() noexcept
{
	// 1) unregister
	unregister_querier();
}

result_code query::redis::querier::_process_starting(context& _context)
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> start redis QUERIER['"sv << name() << "']"sv;

	// 1) on_initializing 함수를 호출한다.
	this->on_starting(_context);

	// declare) 
	START_PARAMETER parameter;
	own_ptr<executor::Iio_context> pio_context_own;

	// 2) parameter 읽기
	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
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
			parameter.count_session = api::get_core_count() * 2 + 8;
		}

		// 3) executor가 nullptr이면 새로 생성한다.
		if(parameter.pio_context.empty())
		{
			// - executor를 생성한다.
			auto ptemp_io_context = make_own<executor::io_context_threads>();

			// - Thread를 시작한다.
			ptemp_io_context->start({ parameter.count_session });

			// - Exeuctor를 저장...
			parameter.pio_context = ptemp_io_context;
			pio_context_own = std::move(ptemp_io_context);
		}
		else
		{
			// - Thread Pool이 있는지 확인한다.
			auto pthread_pool = dynamic_pointer_cast<api::threads>(parameter.pio_context);

			// check) Thread Pool 이 있다면 Thread의 갯수를 확인한다.
			if(pthread_pool.exist())
			{
				// check) Thread의 갯수가 현재 session 수보다 적으면 ASSERT건다.
				CGASSERT_ERROR(pthread_pool->get_existing_thread_count()>=static_cast<std::size_t>(parameter.count_session));

				// - trace) Thread의 갯수가 현재 session 수보다 적으면 경고한다.
				EXECUTE_IF(pthread_pool->get_existing_thread_count() < parameter.count_session, LOG_INFO << log_level_t::important() << "(prgr) thread count is too small. at least "sv << parameter.count_session << " is needed.  ("sv << name() << ")!"sv);
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
	std::vector<object_ptr<session>> vector_session_socket;

	// - Reserve
	vector_session_socket.reserve(parameter.count_session);

	try
	{
		// - Socket을 생성하고 connect 시도한다.
		for(std::size_t i = 0; i < parameter.count_session; ++i)
		{
			// - Redis용 Socket을 생성한다.
			auto psocket_redis = make_object<session>();

			// - executor를 설정한다.
			psocket_redis->io_context(parameter.pio_context);

			// - 재접속이 되도록 설정한다.
			psocket_redis->enable_reconnect();

			// - 접속을 시도한다.
			bool result = psocket_redis->start({ parameter.remote_endpoint, {}, chrono::tick::seconds{5}, 0, 0, 0 });

			//  check) 접속에 실패했으면 다음
			CONTINUE_IF(result == false);

			// - socket Redis를 추가한다.
			vector_session_socket.push_back(std::move(psocket_redis));
		}

		// check) wait for Completion of Establishment
		if (parameter.wait_seconds != 0)
		{
			// - 현재 시간 계산하기
			auto time_end = chrono::tick::now() + chrono::seconds(parameter.wait_seconds);
			auto duration_wait = chrono::seconds(parameter.wait_seconds);

			for (auto iter = vector_session_socket.begin(); iter != vector_session_socket.end(); )
			{
				// - 지나간 시간 계산하기
				auto time_now = chrono::tick::now();

				// - Duration 계산하기
				if (time_end < time_now)
				{
					duration_wait = chrono::seconds(0);
				}

				// - 30초간 대기한다.
				auto state_result = (*iter)->wait_complete_connecting(duration_wait);

				// check) 성공하지 못했을 경우 제거한다.
				if (state_result != eRESULT::SUCCESS)
				{
					(*iter)->disable_reconnect();
					(*iter)->closesocket();

					iter = vector_session_socket.erase(iter);
				}
				else
				{
					++iter;
				}
			}

			// check) 접속에 실패했을 경우
			THROW_IF(vector_session_socket.empty(), throwable::failure(eRESULT::FAIL_TO_CONNECT, 0, "connected redis session is 0"sv));
		}
	}
	catch (std::exception& /*_e*/)
	{
		// - ...
		for (auto& iter : vector_session_socket)
		{
			iter->m_pquerier.reset();
			iter->disable_reconnect();
			iter->reset_io_context();
			iter->closesocket();
		}

		// Log) 
		LOG_FATAL << "(excp) exception complete[start: " << name() << "]";

		// reraise)
		throw;
	}
	catch(...)
	{
		// - ...
		for(auto& iter:vector_session_socket)
		{
			iter->m_pquerier.reset();
			iter->disable_reconnect();
			iter->closesocket();
		}

		// trace) prepare receive에 실패했다!
		LOG_FATAL << "(excp) exception complete[start: " << name() << "]";

		// reraise)
		throw;
	}

	// 5) 생성한 Socket을 설정한다.
	scoped_lock(m_lockable_query)
	{
		this->m_pio_context = std::move(parameter.pio_context);
		this->m_pio_context_own = std::move(pio_context_own);

		for(auto& iter:vector_session_socket)
		{
			this->register_session(iter);
		}
	}

	// trace) 
	LOG_PROGRESS					 << "  > query::redis: success connecting To redis server ("sv << name() << ")!"sv;
	LOG_PROGRESS << log_continue_t() << "    - remote endpoint: "sv << ((parameter.remote_endpoint.address().is_loopback() == false) ? parameter.remote_endpoint.address().to_string<char>().c_str() : "BIND_ANY") << ":"sv << parameter.remote_endpoint.port();
	LOG_PROGRESS << log_continue_t() << "    - requested connection: "sv << parameter.count_session;
	LOG_PROGRESS << log_continue_t() << "    - Established connection  : "sv << vector_session_socket.size() << " (ReqLimit:"sv << parameter.count_session << ")"sv;

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
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop redis QUERIER['"sv << name() << "']"sv;

	// 1) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
	}

	// declare) 
	circular_list<object_ptr<session>> listRedis;
	object_ptr<executor::Iqueue> pio_context;
	own_ptr<executor::Iqueue> pio_context_own;

	// 3) 교체한다.
	scoped_lock(m_lockable_query)
	{
		// - List Redis를 move한다.
		listRedis = std::move(m_stack_session);

		// - executor를 move
		pio_context = std::move(this->m_pio_context);
		pio_context_own = std::move(this->m_pio_context_own);
	}

	// 4) 모든 Query를 Cancel한다.
	for(auto& iter:listRedis)
	{
		// - SessionSocket을 unregister한다.
		this->unregister_session(iter);
	}

	// 5) executor를 닫는다.
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
	// 1) on_stop를 호출한다.
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

object_ptr<query::redis::Iquery> query::redis::querier::execute_query(std::string_view _command)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter '_command' is nullptr")));

	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')")));

	// 1) 새로운 query 객체를 생성한다.
	auto pquery = this->alloc_query();
		
	// 2) execute query
	pquery->execute_query(_command);

	// return) 
	return pquery;
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

void query::redis::querier::register_session(session* _psocket)
{
	scoped_lock(this->m_lockable_query)
	{
		// check) 이미 추가되어 있는지 확인한다.
		CGASSERT_ERROR(std::find(this->m_stack_session.begin(), this->m_stack_session.end(), _psocket) == this->m_stack_session.end());

		// 1) 추가한다.
		this->m_stack_session.push_back(_psocket);
		_psocket->m_pquerier = this;
	}

	// statistics)
	this->statistics_on_create_session();
}

void query::redis::querier::unregister_session(session* _psocket) noexcept
{
	{
		// scoped_lock) 
		std::unique_lock<std::recursive_mutex> cs(this->m_lockable_query);

		// 1) 해당 SessionSocket을 찾는다.
		auto iter_find = std::find(this->m_stack_session.begin(), this->m_stack_session.end(), _psocket);

		// check) 찾지 못했으면 끝낸다.
		RETURN_IF(iter_find == this->m_stack_session.end());

		// - Lock Leave
		cs.unlock();

		// 2) 제거한다.
		this->m_stack_session.erase(iter_find);

		// 3) 모든 Query를 다 취소한다.
		_psocket->cancel_query_all();

		// 4) 재접속이 되도록 설정한다.
		_psocket->disable_reconnect();

		// 5) 접속을 끊는다.
		_psocket->closesocket();

		// 6) Querier reset
		_psocket->m_pquerier.reset();

		// statistics) 
		this->statistics_on_destroy_session();
	}
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
	// 1) 기존의 Dispatch를 수행한다.
	result_code	result = this->Nmessage_transmitter::process_transmit_message(_msg);

	//// 2) Querier Manager에 Dispatch한다.
	//query::manager_querier::instance()->transmit_message(_msg);

	// return) 
	return result;
}

void query::redis::querier::alloc_session(Iquery* _pquery)
{
	// check) _pquery이 nullptr이어서는 안된다.
	CGASSERT_ERROR(_pquery != nullptr);

	// check) _psession가 nullptr이면 안됀다.
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	scoped_lock_named(this->m_lockable_query, cs)
	{
		// 1) 이 있을 경우 하나의 session을 얻는다.
		if(this->m_stack_session.empty() == false)
		{
			// - 일단 Redis Socket session을 얻는다.
			auto psession = this->m_stack_session.front();

			// - Redis Socket session을 하나 얻어낸다.
			this->m_stack_session.pop_front();

			// - session에 Querier를 설정한다.
			psession->m_pquerier = this;

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - session을 설정한다.
			_pquery->set_session(psession);
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// declare) 
			QUEUED_QUERY temp_query(this);

			// 1) set event info
			temp_query.pquery = _pquery;
			temp_query.f_execute = [=]()
			{
				_pquery->m_cv_complete.notify_one();
			};

			// 2) 먼저 Push한다.
			this->m_queue_query.push_back(temp_query);

			// 3) session을 얻을 때까지 wait한다.
			_pquery->wait(cs);
		}
	}

	// check) m_psession이 nullptr이면 안됀다.
	CGASSERT_ERROR(_pquery->get_session().exist());

	// statistics) 
	this->statistics_on_alloc_session();
}

void query::redis::querier::free_session(session* _psession) noexcept
{
	// check) Redis session이 nullptr이어서는 안된다.
	CGASSERT_ERROR(_psession != nullptr);

	// check) _psession이 nullptr이면 안됀다.!
	RETURN_IF(_psession == nullptr);

	// statistics) 
	this->statistics_on_free_session();

	// declare) 
	QUEUED_QUERY pqueued_query(this);

	scoped_lock(this->m_lockable_query)
	{
		// 1) 더이상 Queuing된 Query가 없으면 session을 스택시켜놓는다.
		if(this->m_queue_query.empty())
		{
			// - session을 reset한다.
			_psession->m_pquerier.reset();

			// - session을 스택시킨다.
			this->m_stack_session.push_back(_psession);

			// return) 리턴한다.
			return;
		}

		// 2) 제일 앞에 Query를 얻는다.
		pqueued_query = std::move(this->m_queue_query.front());

		// 3) Query 하나를 뽑아낸다.
		this->m_queue_query.pop_front();

		// check) _psession이 nullptr이면 안됀다.
		CGASSERT_ERROR(_psession != nullptr);

		// 4) session을 설정한다.
		pqueued_query.pquery->set_session(_psession);

		// statistics) 
		this->statistics_on_alloc_session();
	}

	// 5) Queuing된 Query를 Query한다.
	pqueued_query.f_execute();
}

void query::redis::querier::_post_alloc_session(Iquery* _pquery, std::function<void()>&& _f_response)
{
	// check) _pquery이 nullptr이어서는 안된다.
	CGASSERT_ERROR(_pquery != nullptr);

	// check) _psession가 nullptr이면 안됀다.
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['_pquery'] is nullptr")));

	scoped_lock_named(this->m_lockable_query, cs)
	{
		// 1) 이 있을 경우 하나의 session을 얻는다.
		if (this->m_stack_session.empty() == false)
		{
			// - 일단 Redis Socket session을 얻는다.
			auto psession = this->m_stack_session.front();

			// - Redis Socket session을 하나 얻어낸다.
			this->m_stack_session.pop_front();

			// - session에 Querier를 설정한다.
			psession->m_pquerier = this;

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - session을 설정한다.
			_pquery->set_session(psession);

			// check) m_psession이 nullptr이면 안됀다.
			CGASSERT_ERROR(_pquery->get_session().exist());
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// declare) 
			QUEUED_QUERY temp_query(this);

			temp_query.pquery = _pquery;
			temp_query.f_execute = _f_response;

			// - 먼저 Push한다.
			this->m_queue_query.push_back(temp_query);
		}
	}
}
