#include "pch.h"

namespace CGDK
{

namespace query::rdb::odbc
{
	std::string_view get_suitable_odbc_provider(eDB_PROVIDER _provider);
	void get_connection_string(querier* _pquerier, shared_buffer& _str_initial, shared_buffer& _str_output, const querier::START_PARAMETER& _parameter, std::string_view _str_provider, bool _is_access);
}

query::rdb::odbc::querier::querier()
{
	// 1) Set Object type
	this->set_object_type(eOBJECT_TYPE::QUERIER_DB_SQL);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::rdb::odbc::querier::~querier() noexcept
{
	// 1) unregister
	this->unregister_querier();
}

circular_list<object_ptr<query::rdb::odbc::session>> query::rdb::odbc::querier::process_create_session(std::string_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	// check) parameter '_count_data_count' must be bigger than 0
	CGASSERT_ERROR(_count_data_source > 0);

	// check) throw exception if parameter '_count_data_count' is under 0
	THROW_IF(_count_data_source <= 0, throwable::failure(eRESULT::DOMAIN_ERROR, 0, "(err ) query::oledb: _count_data_source is <0"sv));

	// free-Release) free-Release일 경우 session수의 최대를 1개로 제한한다.
#ifdef _FREE_RELEASE
	SATURATE_MAX(_count_data_source, 1);
#endif

	// declare) 
	circular_list<object_ptr<query::rdb::odbc::session>> list_session;

	// declare)
	throwable::failure_ex e(eRESULT::SUCCESS);

	for(std::size_t i = 0; i<_count_data_source; ++i)
	{
		// 1) allocate data_source object
		auto pdata_source = make_object<query::rdb::odbc::data_source>();

		// - 'SET SESSION wait_timeout' on connecting (option)
		pdata_source->m_session_wait_timeout = this->m_session_wait_timeout;

		// 2) open data_source
		pdata_source->open(_connection_string);

		// 3) session을 Source당 session수만큼 연다.
		for(std::size_t j = 0; j < _count_session; ++j)
		{
			// - allocate new session object
			auto psession = new query::rdb::odbc::session();

			try
			{
				// - open session
				psession->open(pdata_source);

				// - queuing session
				list_session.push_back(psession);

				// - set data_source and querier (must set after push_back to list_session)
				psession->m_pdata_source = pdata_source;
				psession->m_pquerier.reset();
			}
			catch (throwable::failure_ex& _e)
			{
				// - delete session
				delete psession;

				// - copy exception
				e = _e;

				// reraise)
				break;
			}
			catch(...)
			{
				// - delete session
				delete psession;

				// - copy exception
				e = throwable::failure_ex(eRESULT::DOMAIN_ERROR, 0, "(err ) query::rdb::odbc: fail to creating sessions"sv);

				// reraise)
				break;
			}
		}
	}

	// 3) error trace
	if (e.reason<eRESULT>() != eRESULT::SUCCESS)
	{
		// trace) output oledb error records
		_log(DEFAULT_LOGGER, static_cast<object_ptr<LOG_RECORD>>(e));
	}

	// check) throw exception if create nothing
	THROW_IF(list_session.size() == 0, e);

	// 4) Swap
	return list_session;
}

result_code query::rdb::odbc::querier::process_start(context& _context)
{
	// notify) notify openning message
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::OPENNING, 0, reinterpret_cast<uintptr_t>(this)));

	// trace) 
	LOG_PROGRESS << "@ <BEGIN> start QUERIER_RDB_ODBC["sv << name() << "]"sv;

	//-----------------------------------------------------------------
	// 1. prepare
	//-----------------------------------------------------------------
	// declare)
	auto str_name = this->name();
	context context_old	 = _context;

	// 1) change context now
	if (str_name.empty() == false)
	{
		auto& context_find = _context[str_name];

		if (context_find.is_valid())
		{
			_context = context_find;
		}
	}

	//-----------------------------------------------------------------
	// 2. get parameters
	//-----------------------------------------------------------------
	try
	{
		// 3) call on_starting
		this->on_starting(_context);
	}
	catch (std::exception& e)
	{
		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: exception complete [P1-1]"sv;

		// - rollback context now
		_context = context_old;

		// - call exception handler
		if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
		{
			// reraise)
			throw;
		}
	}
	catch(...)
	{
		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: exception complete [P1-2]"sv;

		// - rollback context now
		_context = context_old;

		// reraise)
		throw;
	}
		
	// declare) 
	circular_list<object_ptr<session>> list_session;
	object_ptr<Iexecutor> pexecutor = dynamic_pointer_cast<Iexecutor>(this->m_pexecutor);
	own_ptr<Iexecutor> pexecutor_own;

	// declare) 
	START_PARAMETER parameter;
	std::string str_provider;

	// declare) 
	std::string str_connection_string;

	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - get suitable provider;
		str_provider = get_suitable_odbc_provider(parameter.provider);
	}
	else
	{
		// - "connection parameters"
		if (auto& context_connection_string = _context.find("connection_string", "connection string", "connectionstring"); context_connection_string.is_valid())
		{
			str_connection_string = (std::string&&)context_connection_string;
		}
		else
		{
			str_provider = (std::string&&)_context["provider"sv];
			parameter.data_source = (std::string&&)_context.find("data_source", "data source", "datasource", "server", "filename");
			parameter.user_id = (std::string&&)_context.find("user_id", "user id", "userid", "uid");
			parameter.password = (std::string&&)_context.find("password", "pwd");
			parameter.initial_catalog = (std::string&&)_context.find("initial_catalog", "initial catalog", "initialcatalog", "database");
			parameter.character_set = (std::string&&)_context.find("charset", "char_set", "char set", "character_set", "character set");
			parameter.options = (std::string&&)_context.find("option", "options");
		}

		// - session Per Source
		if (auto& context_find = _context["session"]; context_find.is_valid())
		{
			parameter.count_session = context_find;

			// trace)
			LOG_INFO << "  + parameter['session']: "sv << parameter.count_session;
		}

		// - data Source Count
		if (auto& context_find = _context.find("data_source_count", "data source count", "datasourcecount"); context_find.is_valid())
		{
			parameter.count_data_source = context_find;

			// trace)
			LOG_INFO << "  + parameter['data_source_count']: "sv << parameter.count_data_source;
		}
	}

	// 7-1) keepalive / session wait timeout (options. both disabled by default)
	//      "keepalive":            { "enable": true, "interval": 300, "query": "SELECT 1" }
	//      "session_wait_timeout": { "enable": true, "timeout": 1800 }
	{
		// - default
		START_PARAMETER option;

		if (_context.type() == context::value_type_t::buffer_)
		{
			option.keepalive_enable = parameter.keepalive_enable;
			option.keepalive_interval = parameter.keepalive_interval;
			option.keepalive_query = parameter.keepalive_query;
			option.session_wait_timeout_enable = parameter.session_wait_timeout_enable;
			option.session_wait_timeout = parameter.session_wait_timeout;
		}
		else
		{
			// - "keepalive"
			if (auto& context_keepalive = _context["keepalive"sv]; context_keepalive.is_valid())
			{
				if (auto& context_find = context_keepalive["enable"sv]; context_find.is_valid())
					option.keepalive_enable = static_cast<bool>(context_find);
				if (auto& context_find = context_keepalive["interval"sv]; context_find.is_valid())
					option.keepalive_interval = static_cast<uint32_t>(context_find);
				if (auto& context_find = context_keepalive["query"sv]; context_find.is_valid())
					option.keepalive_query = (std::string&&)context_find;
			}

			// - "session_wait_timeout"
			if (auto& context_wait_timeout = _context["session_wait_timeout"sv]; context_wait_timeout.is_valid())
			{
				if (auto& context_find = context_wait_timeout["enable"sv]; context_find.is_valid())
					option.session_wait_timeout_enable = static_cast<bool>(context_find);
				if (auto& context_find = context_wait_timeout["timeout"sv]; context_find.is_valid())
					option.session_wait_timeout = static_cast<uint32_t>(context_find);
			}
		}

		// check) the value must be bigger than 0 (0 : default)
		if (option.keepalive_interval == 0)
			option.keepalive_interval = START_PARAMETER().keepalive_interval;
		if (option.session_wait_timeout == 0)
			option.session_wait_timeout = START_PARAMETER().session_wait_timeout;
		if (option.keepalive_query.empty())
			option.keepalive_query = "SELECT 1";

		// 1) set (0 : disabled)
		this->m_keepalive_interval = option.keepalive_enable ? chrono::tick::duration(std::chrono::seconds(option.keepalive_interval)) : chrono::tick::duration::zero();
		this->m_keepalive_query = option.keepalive_query;
		this->m_session_wait_timeout = option.session_wait_timeout_enable ? option.session_wait_timeout : 0;

		// trace)
		if (option.keepalive_enable)
			LOG_INFO << "  + parameter['keepalive']: enable (interval: "sv << option.keepalive_interval << "s, query: '"sv << option.keepalive_query << "')"sv;
		if (option.session_wait_timeout_enable)
			LOG_INFO << "  + parameter['session_wait_timeout']: enable (timeout: "sv << option.session_wait_timeout << "s, MySQL/MariaDB)"sv;

		// check) the keepalive must be done before the server closes the idle connection
		if (option.keepalive_enable && option.session_wait_timeout_enable && option.keepalive_interval >= option.session_wait_timeout)
			LOG_WARNING << "  ! (warning) keepalive 'interval'("sv << option.keepalive_interval << "s) must be shorter than session_wait_timeout 'timeout'("sv << option.session_wait_timeout << "s) ("sv << name() << ")"sv;
	}
	// 8) internal options
	{
		// - session Per Source
		auto& context_find = _context.find("logging_warning_info"
			, "logging warning info"
			, "logging_warning"
			, "logging warning");

		if (context_find.is_valid())
		{
			std::string str_find = context_find;

			// - true면 재접속을 활성화한다.
			if (CGDK::__stricmp(str_find, "true") == 0
				|| CGDK::__stricmp(str_find, "enable") == 0)
			{
				// - ...
				this->enable_logging_warning_info();

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (CGDK::__stricmp(str_find, "false") == 0
				|| CGDK::__stricmp(str_find, "disable") == 0)
			{
				// - ...
				this->disable_logging_warning_info();

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 true이다.)
			else if (CGDK::__stricmp(str_find, "default") == 0)
			{
				// - set as default
				this->enable_logging_warning_info(true);

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'default'[enable]"sv;
			}
		}
	}

	//-----------------------------------------------------------------
	// 3. prepare sessions
	//-----------------------------------------------------------------
	{
		// declare) 
		auto str_initial = alloc_shared_buffer(8192);
		auto str_output = alloc_shared_buffer(8192);
		bool is_access = false;

		// 9) get _provider 'connection_string' 
		{
			// - if key "connection_string" exist
			if (str_connection_string.empty() == false)
			{
				str_initial.append_text(static_cast<std::string_view>(str_connection_string));
				str_initial.append<char>(0);
				str_output.append_text(static_cast<std::string_view>(str_connection_string));
				str_output.append<char>(0);
			}
			else
			{
				get_connection_string(this, str_initial, str_output, parameter, str_provider, is_access);
			}

			// - session count
			if (parameter.count_session == 0)
			{
				if (is_access == false)
					parameter.count_session = 32;
				else
					parameter.count_session = 1;
			}

			// - saturate 'parameter.count_session' and 'data_source count'
			SATURATE(parameter.count_session, 1, 1024);
			SATURATE(parameter.count_data_source, 1, 1024);
		}

		// trace) 
		LOG_INFO << "  > trying to creating session("sv << name() << ")..."sv;

		// 10) prepare sessions and connect
		try
		{
			// - create session
			list_session = process_create_session(str_initial, parameter.count_session, parameter.count_data_source);

			// check) throw exception if failed
			if (list_session.empty())
			{
				// trace) 
				LOG_PROGRESS << "@ <TERMINATE> start QUERIER_RDB_ODBC["sv << str_name << "]: connection string is invalid [P2] (connection_string: "sv << str_output.data<char>() << ")"sv;

				// throw) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string("<TERMINATE> start QUERIER_RDB_ODBC[") + ((str_name.empty() == false) ? str_name.data() : "") + "]: connection string is invalid [P2] (connection_string: " + ((str_output.data() != nullptr) ? str_output.data() : "-") + ")");
			}

			// trace) 
			LOG_INFO << "  - created session: "sv << list_session.size();
		}
		catch (...)
		{
			// trace) 
			LOG_ERROR << "@ <TERMINATE> start QUERIER_RDB_ODBC["sv << str_name << "]: connection string is invalid [P2] (connection_string: "sv << str_output.data<char>() << ")"sv;

			// throw)
			throw;
		}

		// - disable retry if access
		if (is_access)
		{
			this->set_max_retry_on_fail(0);
		}
	}

	// 11) create executor
	try
	{
		// - executor가 nullptr이면 새로 생성한다.
		if(pexecutor == nullptr)
		{
			// - executor Thread를 생성한다.
			auto temp_executor = make_own<executor::queue_list_threads>();

			// - set name
			temp_executor->name("own"sv);

			// - thread를 시작한다.
			temp_executor->start({ static_cast<int>(list_session.size() + 1) });

			// - executor를 설정.
			pexecutor = temp_executor;
			pexecutor_own = std::move(temp_executor);
		}

		// check) Thread Pool 이 있다면 Thread의 개수를 확인한다.

		// - Thread Pool이 있는지 확인한다.
		auto pthread_pool = dynamic_pointer_cast<api::threads>(pexecutor);

		// check) Thread Pool 이 있다면 thread의 개수를 확인한다.
		if (pthread_pool.exist())
		{
			// - trace) thread의 개수가 현재 session 수보다 적으면 경고한다.
			EXECUTE_IF(pthread_pool->reserved_size() < list_session.size() + 1, LOG_INFO << "  ! (warning) thread count is too small. at least "sv << list_session.size() + 1 << " is needed.  ("sv << name() << ")!"sv);
		}

		// - executor를 설정한다.
		this->m_stack_sessions.swap(list_session);
		this->m_pexecutor = std::move(pexecutor);
		this->m_pexecutor_own = std::move(pexecutor_own);
		this->m_count_session_created += (long)this->m_stack_sessions.size();
		this->statistics_on_create_session(this->m_stack_sessions.size());
	}
	catch (std::exception& e)
	{
		// trace) 
		LOG_FATAL << "@ <TERMINATE> start QUERIER_RDB_ODBC["sv << name() << "]:  fail to starting odbc!!! [P3-1]"sv;

		// - setting을 원래대로 돌린다.
		_context = context_old;

		// - 할당받아 놓았던 것을 지운다.
		if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
		{
			// reraise)
			throw;
		}
	}
	catch (...)
	{
		// trace) 
		LOG_ERROR << "@ <TERMINATE> start QUERIER_RDB_ODBC["sv << name() << "]:  fail to starting odbc!!! [P3-2]"sv;

		// - setting을 원래대로 돌린다.
		_context = context_old;

		// reraise)
		throw;
	}


	//-----------------------------------------------------------------
	// 4. child objects
	//-----------------------------------------------------------------
	// declare)
	auto iter = this->m_container.begin();
	auto iterEnd = this->m_container.end();

	try
	{
		// 1) 모든 자식 객체를 start한다.
		for (; iter != iterEnd; ++iter)
		{
			object::start(*iter, _context);
		}

		// 2) stet state to 'eOBJECT_STATE::RUNNING'
		this->set_object_state(eOBJECT_STATE::RUNNING);

		// - keepalive (option) : checks every 1/4 of the interval (1s ~ 60s)
		if (this->m_keepalive_interval > chrono::tick::duration::zero())
		{
			auto tick_check = std::clamp<chrono::tick::duration>(this->m_keepalive_interval / 4, std::chrono::seconds(1), std::chrono::seconds(60));
			auto pschedulable = make_own<schedulable::executable_function<>>();
			pschedulable->start({ [this] { this->process_keepalive(); }, tick_check });
			this->m_pschedulable_keepalive = std::move(pschedulable);
		}

		// 3) on_start 함수를 호출한다.
		this->on_start(_context);
	}
	catch (...)
	{
		// - open했던 자식 객체를 Roll-back한다.
		for (auto iter_rollback = m_container.begin(); iter_rollback != iter; ++iter_rollback)
		{
			object::stop(*iter_rollback);
		}

		// - 모든 Query가 완료되고 모든 session이 완료되도록 기다린다.
		//   (m_queue_query / m_stack_sessions는 executor thread가 lock 안에서 바꾼다 -> lock 안에서 읽는다)
		for (;;)
		{
			bool is_done = false;
			SCOPED_LOCK(this->m_lockable_query)
			{
				is_done = this->m_queue_query.empty() && this->m_count_session_created == this->m_stack_sessions.size();
			}
			BREAK_IF(is_done);

			std::this_thread::sleep_for(1ms);
		}

		// - executor를 Reset한다.
		pexecutor.reset();
		pexecutor_own.destroy();

		// declare) the queued queries are released out of the lock (their destructors can run any code)
		circular_list<QUEUED_QUERY> queue_released;

		SCOPED_LOCK(this->m_lockable_query)
		{
			// - 먼저 Querier를 Reset한다.(이걸 하지 않으면 무한 free된다.)
			for (auto& iter_session : this->m_stack_sessions)
			{
				iter_session->m_pquerier.reset();
			}

			// Statistics)
			this->statistics_on_destroy_session(this->m_stack_sessions.size());

			// - session을 clear한다.
			this->m_stack_sessions.clear();

			// - 생성된 수를 0개로 설정한다.
			this->m_count_session_created = 0;

			// - Query를 clear한다.
			this->m_queue_query.swap(queue_released);
		}

		// - on_stop 함수를 호출한다.(on_starting()함수와 짝함수이므로 호출된다.)
		try
		{
			this->on_stop();
		}
		catch (...)
		{
		}

		// - Context 원상 복구한다.
		_context = context_old;

		// reraise) 
		throw;
	}

	// Notify) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::OPEN, 0, reinterpret_cast<uintptr_t>(this)));



	//-----------------------------------------------------------------
	// 5. end
	//-----------------------------------------------------------------
	// 1) Context 원상 복구한다.
	_context = context_old;

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> start QUERIER_RDB_ODBC["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code query::rdb::odbc::querier::process_stop() noexcept
{
	// Message) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::CLOSING, 0, reinterpret_cast<uintptr_t>(this)));

	// 1-1) stop the keepalive (waits until the running keepalive is finished)
	if (this->m_pschedulable_keepalive.exist())
	{
		this->m_pschedulable_keepalive->stop();
		this->m_pschedulable_keepalive.reset();
	}
	{
		std::lock_guard lock(this->m_lockable_keepalive);
	}
	// 1) on_stopping 함수를 호출한다.
	try
	{
		on_stopping();
	}
	catch (...)
	{
	}

	// 2) Childe를 stop한다.
	for (auto riter = m_container.rbegin(); riter != m_container.rend(); ++riter)
	{
		object::stop(*riter);
	}

	//// 3) 모든 Query가 완료되고 모든 session이 완료되도록 기다린다.
	//while(m_queue_query.empty()==false || m_count_session_created!=this->m_stack_sessions.size())
	//{
	//	Sleep(1);
	//}

	// 4) executor를 Reset한다.
	this->m_pexecutor.reset();
	this->m_pexecutor_own.destroy();

	//SCOPED_LOCK(this->m_lockable_query)
	//{
	//	// 5) 먼저 Querier를 Reset한다.(이걸 하지 않으면 무한 free된다.)
	//	for(auto& iter:this->m_stack_sessions)
	//	{
	//		iter->m_pquerier.reset();
	//	}

	//	// Statistics)
	//	statistics_on_destroy_session(this->m_stack_sessions.size());

	//	// 6) session을 clear한다.
	//	this->m_stack_sessions.clear();

	//	// 7) 생성된 수를 0개로 설정한다.
	//	this->m_count_session_created	 = 0;

	//	// 8) Query를 clear한다.
	//	this->m_queue_query.clear();
	//}

	// 9) 상태를 eOBJECT_STATE::STOPPED로 변경한다.
	this->set_object_state(eOBJECT_STATE::STOPPED);

	// 10) on_stop 함수를 호출한다.
	try
	{
		this->on_stop();
	}
	catch (...)
	{
	}

	// Message) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::CLOSE, 0, reinterpret_cast<uintptr_t>(this)));

	// return)
	return eRESULT::SUCCESS;
}

result_code query::rdb::odbc::querier::_process_destroy() noexcept
{
	// 1) Call Parent's _process_destroy function
	auto result = Ninitializable::_process_destroy();

	// 2) unregister all messageable objects
	reset_message_transmitter();

	// return)
	return result;
}


result_code query::rdb::odbc::querier::process_message(sMESSAGE& _msg)
{
	// 1) Log Message Dispatch
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

result_code query::rdb::odbc::querier::start(const std::string_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _connection_string가 empty면 안 된다.
		THROW_IF(_connection_string.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_connection_string' is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// declare) 
	context context_setting;

	// 1) set parameters
	context_setting["connection_string"sv] = static_cast<std::string_view>(_connection_string);
	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// 2) start
	return ((Istartable*)this)->start(context_setting);
}

result_code query::rdb::odbc::querier::start(Iexecutor* _pexecutor, const std::string_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _connection_string가 empty면 안 된다.
		THROW_IF(_connection_string.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_connection_string' is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// 1) get executor
	auto pexecutor = m_pexecutor;

	// declare) 
	context context_setting;

	// 2) set parameters
	context_setting["connection_string"sv] = static_cast<std::string_view>(_connection_string);
	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// declare)
	result_code result;

	try
	{
		// 3) executor를 설정한다.
		m_pexecutor = _pexecutor;

		// 4) start한다.
		result = ((Istartable*)this)->start(context_setting);
	}
	catch(...)
	{
		// - executor를 되돌린다.
		m_pexecutor = pexecutor;

		// reraise) 
		throw;
	}

	// return)
	return result;
}

template<class T_CHAR>
std::basic_string_view<T_CHAR> _get_provider_string(eDB_PROVIDER _provider);

result_code query::rdb::odbc::querier::start(eDB_PROVIDER _provider, const std::string_view _data_source, const std::string_view _user_id, const std::string_view _password, const std::string_view _init_catalog, const std::string_view _character_set, const std::string_view _option, std::size_t _count_session, std::size_t _count_data_source)
{
	return this->start(CGDK::_get_provider_string<char>(_provider), _data_source, _user_id, _password, _init_catalog, _character_set, _option, _count_session, _count_data_source);
}

result_code query::rdb::odbc::querier::start(const std::string_view _provider, const std::string_view _data_source, const std::string_view _user_id, const std::string_view _password, const std::string_view _init_catalog, const std::string_view _character_set, const std::string_view _option, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _provider must not be nullptr
		THROW_IF(_count_data_source <= 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_count_data_source' is 0 or smaller. must be >1"sv));

		// check) _data_source must not be nullptr
		THROW_IF(_data_source.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_data_source' is empty"sv));

		// check) throw exception if _provider is nullptr
		THROW_IF(_provider.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_provider' is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_ODBC["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// declare)
	context context_setting;

	// 1) set parameters
	context_setting["provider"sv] = static_cast<std::string_view>(_provider);
	context_setting["server"sv] = static_cast<std::string_view>(_data_source);

	if(_user_id.empty() == false)
		context_setting["uid"sv] = static_cast<std::string_view>(_user_id);

	if(_password.empty() == false)
		context_setting["pwd"sv] = static_cast<std::string_view>(_password);

	if(_init_catalog.empty() == false)
		context_setting["initial_catalog"sv] = static_cast<std::string_view>(_init_catalog);

	if(_character_set.empty() == false)
		context_setting["character_set"sv] = static_cast<std::string_view>(_character_set);

	if(_option.empty() == false)
		context_setting["option"sv] = static_cast<std::string_view>(_option);

	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// 2) start
	return this->start(context_setting);
}

void query::rdb::odbc::querier::cancel_queries() noexcept
{
	// declare) the canceled queries are released out of the lock (their destructors can run any code)
	circular_list<QUEUED_QUERY> queue_canceled;

	// 1) session을 clear한다.
	SCOPED_LOCK(this->m_lockable_query)
	{
		this->m_queue_query.swap(queue_canceled);
	}
}

object_ptr<query::rdb::session_t> query::rdb::odbc::querier::alloc_session()
{
	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// declare)
	object_ptr<session_t> psession;

	// SCOPED_LOCK) 
	SCOPED_LOCK_NAMED(this->m_lockable_query, cs)
	{
		// 1) session이 있을 경우 하나의 session을 얻는다.
		if (this->m_stack_sessions.empty() == false)
		{
			// - 일단 DBConnection을 얻는다.
			psession = std::move(this->m_stack_sessions.front());

			// - DBConnection을 하나 뽑아낸다.
			this->m_stack_sessions.pop_front();

			// - session에 querier를 설정한다.
			psession->m_pquerier = this;

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - 사용수를 증가시킨다.
			++psession->m_count_requested;

			// statistics)
			this->statistics_on_alloc_session();
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// declare) shared with the queued function (it can be called or released after this frame is gone)
			struct WAIT_SESSION
			{
				object_ptr<session> psession;
				std::condition_variable_any cv_complete;
				bool is_done = false;
			};
			auto pwait = std::make_shared<WAIT_SESSION>();

			// - ticket : its deleter wakes up this thread when the queued function is released without being called
			//   (cancel_queries(), rollback of start)
			std::shared_ptr<void> pticket(nullptr, [this, pwait](void*)
				{
					SCOPED_LOCK(this->m_lockable_query)
					{
						pwait->is_done = true;
						pwait->cv_complete.notify_one();
					}
				});

			// - push 
			//   free_session() calls this out of the lock. so hands over the session under the lock
			this->m_queue_query.emplace_back({ [this, pwait, pticket = std::move(pticket)](object_ptr<session>&& _psession)
				{
					SCOPED_LOCK(this->m_lockable_query)
					{
						pwait->psession = std::move(_psession);
						pwait->is_done = true;
						pwait->cv_complete.notify_one();
					}
				}});

			// notify) thread block
			scoped_thread_block thread_block;

			// - session을 얻을 때까지 wait한다.
			pwait->cv_complete.wait(cs, [&] { return pwait->is_done; });

			// check) the queued function is released without being called
			THROW_IF(pwait->psession.empty(), throwable::failure(eRESULT::CANCEL, 0, "session allocation is canceled"sv));

			// - get session (statistics: counted by free_session() which handed it over)
			psession = std::move(pwait->psession);
		}
	}

	// return)
	return psession;
}

object_ptr<query::rdb::session_t> query::rdb::odbc::querier::try_alloc_session()
{
	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// declare)
	object_ptr<session_t> psession;

	SCOPED_LOCK(this->m_lockable_query)
	{
		// check) 지금 쓸 수 있는 session이 없다. (기다리지 않는다)
		if (this->m_stack_sessions.empty())
			return psession;

		// 1) session을 하나 꺼낸다.
		psession = std::move(this->m_stack_sessions.front());
		this->m_stack_sessions.pop_front();

		// check)
		CGASSERT_ERROR(psession.exist());

		// - session의 querier를 설정하고 요청 수를 증가시킨다.
		psession->m_pquerier = this;
		++psession->m_count_requested;

		// statistics)
		this->statistics_on_alloc_session();
	}

	// return)
	return psession;
}

void query::rdb::odbc::querier::post_alloc_session(std::function<void(session_t*)>&& _lambda, std::function<void()>&& _on_cancel)
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

			// check)
			CGASSERT_ERROR(psession.exist());

			// 3) reset session
			this->m_lambda(psession);

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

		std::function<void(session_t*)> m_lambda;
		std::function<void()> m_on_cancel;
	};

	// 1) create query object
	auto pexecutable_query = make_object<executable_query>();
	pexecutable_query->m_lambda = std::move(_lambda);
	pexecutable_query->m_on_cancel = std::move(_on_cancel);

	// 2) post  
	this->process_post_alloc_session(std::move(pexecutable_query));
}

void query::rdb::odbc::querier::process_post_alloc_session(object_ptr<Iexecutable_query>&& _pexecutable)
{
	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// 1) get executor
	auto pexecutor = this->m_pexecutor;
	
	// check) m_pexecutor must not be nullptr
	THROW_IF(pexecutor.empty(), throwable::failure(eRESULT::NOT_READY, 0, "executor is nullptr"sv));

	// declare)
	object_ptr<session> psession;

	// 2) get the session or queue the query
	SCOPED_LOCK(this->m_lockable_query)
	{
		// 1) session이 있을 경우 하나의 session을 얻는다.
		if (this->m_stack_sessions.empty() == false)
		{
			// - 일단 DBConnection을 얻는다.
			psession = std::move(this->m_stack_sessions.front());

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - DBConnection을 하나 뽑아낸다.
			this->m_stack_sessions.pop_front();

			// - session에 querier를 설정한다.
			psession->m_pquerier = this;

			// - 사용수를 증가시킨다.
			++psession->m_count_requested;

			// statistics) before post (the session can be freed on the executor before post() returns)
			this->statistics_on_alloc_session();
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// - push (statistics: counted by free_session() which hands the session over)
			//   free_session() calls it out of the lock. not posted : canceled when the executable is released (the session goes back to the pool)
			this->m_queue_query.emplace_back({[this, pexecutor, pexecutable = _pexecutable](object_ptr<session>&& _psession)
				{
					pexecutable->m_psession = std::move(_psession);

					bool is_posted = false;
					try
					{
						is_posted = pexecutor->post(object_ptr<Iexecutable>(pexecutable));
					}
					catch (...)
					{
					}

					if (is_posted == false)
						LOG_ERROR << "(err ) query::rdb::odbc: fail to post the queued query. it is canceled ("sv << this->name() << ")"sv;
				}});

			// - pending : canceled if it is released without being executed (cancel_queries(), refused by the executor)
			_pexecutable->m_is_pending = true;

			// return) posted by free_session() when a session is freed
			return;
		}
	}

	// 3) post out of the lock
	//    - nothing out of the querier must run holding 'm_lockable_query'. (an executor running the query at once,
	//      or the old session of '_pexecutable' released by the assignment can get into alloc_session() and wait
	//      holding the lock. the recursive lock is released only one level by the wait -> dead-lock)
	_pexecutable->m_psession = std::move(psession);

	// - pending : canceled if it is released without being executed
	//   (exactly one of them happens : executed, canceled, or the exception to the caller)
	_pexecutable->m_is_pending = true;

	// - post a copy ('_pexecutable' keeps it alive here. 'pexecutable' is moved if the executor took it)
	object_ptr<Iexecutable> pexecutable = _pexecutable;
	bool is_posted = false;
	std::exception_ptr pexception;

	try
	{
		is_posted = pexecutor->post(std::move(pexecutable));
	}
	catch (...)
	{
		pexception = std::current_exception();
	}

	// check) posted
	RETURN_IF(is_posted);

	// check) the executor took it but failed : executed or canceled by the executor (not reported to the caller)
	if (pexecutable.empty())
	{
		LOG_ERROR << "(err ) query::rdb::odbc: fail to post the query ("sv << this->name() << ")"sv;
		return;
	}

	// - rollback : not posted. not canceled (the caller gets the exception)
	//   releasing the session returns it by free_session() (to a waiting query or to the pool)
	_pexecutable->m_is_pending = false;
	_pexecutable->m_psession.reset();

	// reraise)
	if (pexception)
		std::rethrow_exception(pexception);

	throw throwable::failure(eRESULT::NOT_READY, 0, "the executor refused the query (stopped)"sv);
}

void query::rdb::odbc::querier::process_keepalive() noexcept
{
	// check)
	RETURN_IF(this->m_keepalive_interval <= chrono::tick::duration::zero());

	// check)
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	// check) the previous keepalive is still running
	std::unique_lock lock_keepalive(this->m_lockable_keepalive, std::try_to_lock);
	RETURN_IF(lock_keepalive.owns_lock() == false);

	try
	{
		// declare)
		auto tick_now = chrono::tick::now();
		std::vector<object_ptr<session>> vector_session;

		// 1) takes the sessions idle for 'keepalive_interval' out of the pool (same as alloc_session)
		SCOPED_LOCK(this->m_lockable_query)
		{
			for (auto iter = this->m_stack_sessions.begin(); iter != this->m_stack_sessions.end(); )
			{
				if (tick_now - (*iter)->m_tick_last_used < this->m_keepalive_interval)
				{
					++iter;
					continue;
				}

				auto psession = std::move(*iter);
				iter = this->m_stack_sessions.erase(iter);

				// - returned to the pool by session::process_dispose() when it is released
				psession->m_pquerier = this;
				this->statistics_on_alloc_session();

				vector_session.push_back(std::move(psession));
			}
		}

		// 2) ping (reconnects the broken one)
		for (auto& iter : vector_session)
		{
			// - alive
			if (iter->ping(this->m_keepalive_query))
				continue;

			// - broken : reconnect now (before it is used)
			LOG_WARNING << "(warn) query::rdb::odbc: keepalive failed. reconnecting ("sv << this->name() << ")"sv;

			try
			{
				iter->reopen();
				iter->m_tick_last_used = chrono::tick::now();

				LOG_INFO << "  - reconnected by keepalive ("sv << this->name() << ")"sv;
			}
			catch (...)
			{
				LOG_ERROR << "(err ) query::rdb::odbc: fail to reconnect by keepalive ("sv << this->name() << ")"sv;
			}
		}

		// 3) returns the sessions to the pool (session::process_dispose() -> free_session())
		vector_session.clear();
	}
	catch (...)
	{
	}
}
void query::rdb::odbc::querier::free_session(session* _psession) noexcept
{
	// check) _psession must not be nullptr
	CGASSERT_ERROR(_psession != nullptr);

	// check) return if _psession is nullptr
	RETURN_IF(_psession == nullptr);

	// statistics) 
	this->statistics_on_free_session();

	// - last used time (keepalive)
	_psession->m_tick_last_used = chrono::tick::now();

	// declare) 
	QUEUED_QUERY pqueued_query;

	SCOPED_LOCK(this->m_lockable_query)
	{
		// 1) 더 이상 queuing된 query가 없으면 session을 stack시킨다.
		if(this->m_queue_query.empty())
		{
			// - session을 Reset한다.
			_psession->m_pquerier.reset();

			// - session을 스택시킨다.
			this->m_stack_sessions.push_back(_psession);

			// return)
			return;
		}

		// 2) 제일 앞에 query를 얻는다.
		pqueued_query = std::move(this->m_queue_query.front());

		// 3) query 하나를 뽑아낸다.
		this->m_queue_query.pop_front();

		// 4) 사용 수를 증가시킨다.
		++_psession->m_count_requested;

		// statistics) the waiting path of alloc_session()/process_post_alloc_session() is counted only here
		this->statistics_on_alloc_session();
	}

	// check) _psession이 nullptr이면 안 된다.
	CGASSERT_ERROR(_psession != nullptr);

	// 8) execute queued query
	(pqueued_query.f_execute)(_psession);
}

result_code query::rdb::odbc::querier::process_transmit_message(sMESSAGE& _msg)
{
	// 1) dispatch to attached objects _msg
	auto result = Nmessage_transmitter::process_transmit_message(_msg);

	// 2) notify message to querier_manager
	CGDK::query::manager_querier::instance()->transmit_message(_msg);

	// return) 
	return result;
}








//public Npoolable<Iexecutable_query>
//
//// check) 
//CGASSERT_ERROR(_lambda);
//
//// check) m_pexecutor must not be nullptr
//THROW_IF(!_lambda, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "lambda function is empty"sv));

//virtual	intptr_t			process_execute(intptr_t _return, std::size_t _param) override
//{
//	// check) 
//	CGASSERT_ERROR(this->m_lambda);
//
//	// 1) execute query
//	this->m_lambda(this->m_psession);
//
//	// 2) reset session
//	this->m_psession.reset();
//
//	// return) 
//	return 1;
//}
//
//std::function<void(session*)> m_lambda;

//void query::rdb::odbc::querier::post_alloc_session(std::function<void(session_t*)>&& _lambda)
//{
//	// 2) make executable
//	auto pexecutable_query = make_object<executable_query>();
//
//	// 3) set lambda function
//	pexecutable_query->m_lambda = std::move(_lambda);
//
//	// check) 
//	CGASSERT_ERROR(pexecutable_query->m_lambda);
//
//
//}



}