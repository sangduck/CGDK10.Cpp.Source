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

query::rdb::odbc::querier::querier(const std::string_view _name) : Nnameable(_name)
{
	// 1) Set Object type
	this->set_object_type(eOBJECT_TYPE::QUERIER_DB_SQL);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::rdb::odbc::querier::~querier() noexcept
{
	// 1) eOBJECT_STATE::RUNNING 상태일 때만 eOBJECT_STATE::STOP_PENDING 상태로 변경한다.
	if (set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::STOP_PENDING) == true)
	{
		// - process_stop함수를 호출한다.
		this->process_stop();

		// - 상태를 eOBJECT_STATE::STOPPED로 변경한다.
		this->set_object_state(eOBJECT_STATE::STOPPED);
	}

	// 2) eOBJECT_STATE::STOPPED 상태일 때만 eOBJECT_STATE::DESTROYING_PENDING로 상태를 바꾼다.
	if (this->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::DESTROYING_PENDING) == true)
	{
		// - process_destroy 함수를 호출한다.
		this->process_destroy();

		// - 상태를 eOBJECT_STATE::NONE로 변경한다.
		this->set_object_state(eOBJECT_STATE::NONE);
	}

	// 3) unregister
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
		// 1) allocate data_source ojbect
		auto pdata_source = make_object<query::rdb::odbc::data_source>();

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
		// trace) outpur oledb error recoreds
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

	// declare)
	auto str_name = this->name();
	context context_old	 = _context;

	// 2) change context now
	if (str_name.empty() == false)
	{
		auto& context_find = _context[str_name];

		if (context_find.is_valid())
		{
			_context = context_find;
		}
	}

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
	object_ptr<Iexecutor> pexecutor = dynamic_pointer_cast<Iexecutor>(m_pexecutor);
	own_ptr<Iexecutor> pexecutor_own;

	// declare) 
	START_PARAMETER parameter;
	std::string str_provider;

	// declare) 
	std::string str_connection_string;

	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		// - get parameter
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - get suitable provider;
		str_provider = get_suitable_odbc_provider(parameter.provider);
	}
	else
	{
		// - "connection_string"
		str_connection_string = (std::string)_context.find("connection_string"sv, "connection string"sv, "connectionstring"sv);

		// - "connection parameters"
		if (str_connection_string.empty())
		{
			str_provider = (std::string)_context["provider"sv];
			parameter.data_source = (std::string)_context.find("data_source"sv, "data source"sv, "datasource"sv, "server"sv, "filename"sv);
			parameter.user_id = (std::string)_context.find("user_id"sv, "user id"sv, "userid"sv, "uid"sv);
			parameter.password = (std::string)_context.find("password"sv, "pwd"sv);
			parameter.initial_catalog = (std::string)_context.find("initial_catalog"sv, "initial catalog"sv, "initialcatalog"sv, "database"sv);
			parameter.character_set = (std::string)_context.find("charset"sv, "char_set"sv, "char set"sv, "character_set"sv, "character set"sv);
			parameter.options = (std::string)_context.find("option"sv, "options"sv);
		}

		// - session Per Source
		auto value_find = _context["session"sv];

		if (value_find.is_valid())
		{
			parameter.count_session = value_find;

			// trace)
			LOG_INFO << "  + parameter['session']: "sv << parameter.count_session;
		}

		// - data Source Count
		value_find = _context.find("data_source_count"sv, "data source count"sv, "datasourcecount"sv);

		// - set data_source_ount if key exist
		if (value_find.is_valid())
		{
			parameter.count_data_source = value_find;

			// trace)
			LOG_INFO << "  + parameter['data_source_count']: "sv << parameter.count_data_source;
		}
	}

	// 7) get _provider 'connection_string' 
	{
		// declare) 
		auto str_initial = alloc_shared_buffer(8192);
		auto str_output = alloc_shared_buffer(8192);
		bool is_access = false;

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

		// trace) 
		LOG_INFO << "  > trying to creating session("sv << name() << ")..."sv;

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
				throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, __format(
					"<TERMINATE> start QUERIER_RDB_ODBC[{}]: connection string is invalid [P2] (connection_string: {})",
					(str_name.empty() == false) ? str_name.data() : "",
					(str_output.data() != nullptr) ? str_output.data() : "-")
				);
			}
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

	// 8) interal options
	{
		// - session Per Source
		auto value_find = _context.find("logging_warning_info"sv
									  , "logging warning info"sv
									  , "logging_warning"sv
									  , "logging warning"sv);

		if (value_find.is_valid())
		{
			std::string str_find = value_find;

			// - true면 재접속을 활성화한다.
			if (__stricmp(str_find, "true"sv) == 0
			 || __stricmp(str_find, "enable"sv) == 0)
			{
				// - ...
				this->enable_logging_warning_info();

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'enable'"sv;
			}
			// - false면 재접속을 비활성화한다.
			else if (__stricmp(str_find, "false"sv) == 0
				  || __stricmp(str_find, "disable"sv) == 0)
			{
				// - ...
				this->disable_logging_warning_info();

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'disable'"sv;
			}
			// - default면 기본 값을 사용한다. (기본값은 true이다.)
			else if (__stricmp(str_find, "default"sv) == 0)
			{
				// - set as default
				this->enable_logging_warning_info(true);

				// trace)
				LOG_INFO << "  + parameter['logging_warning_info']: 'default'[enable]"sv;
			}
		}
	}

	try
	{
		// 7) executor가 nullptr이면 새로 생성한다.
		if(pexecutor == nullptr)
		{
			// - executor Thread를 생성한다.
			auto temp_executor = make_own<executor::io_context_threads>("own"sv);

			// - thread를 시작한다.
			temp_executor->start({ list_session.size() + 1 });

			// - executor를 설정.
			pexecutor = temp_executor;
			pexecutor_own = std::move(temp_executor);
		}

		// check) Thread Pool 이 있다면 Thread의 갯수를 확인한다.

		// 8) Thread Pool이 있는지 확인한다.
		auto pthread_pool = dynamic_pointer_cast<api::threads>(pexecutor);

		// check) Thread Pool 이 있다면 thread의 갯수를 확인한다.
		if (pthread_pool.exist())
		{
			// - trace) thread의 갯수가 현재 session 수보다 적으면 경고한다.
			EXECUTE_IF(pthread_pool->reserved_size() < list_session.size() + 1, LOG_INFO << "  ! (warning) thread count is too small. at least "sv << list_session.size() + 1 << " is needed.  ("sv << name() << ")!"sv);
		}

		// 9) executor를 설정한다.
		this->m_stack_session.swap(list_session);
		this->m_pexecutor = std::move(pexecutor);
		this->m_pexecutor_own = std::move(pexecutor_own);
		this->m_count_session_created	+= (long)this->m_stack_session.size();
		this->statistics_on_create_session(this->m_stack_session.size());
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
	// 4. ChildnAcceptPreparedOnStart만큼 Accept걸기.
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
		while (this->m_queue_query.empty() == false || this->m_count_session_created != this->m_stack_session.size())
		{
			std::this_thread::sleep_for(1ms);
		}

		// - executor를 Reset한다.
		pexecutor.reset();
		pexecutor_own.destroy();

		scoped_lock(this->m_lockable_query)
		{
			// - 먼저 Querier를 Reset한다.(이걸 하지 않으면 무한 free된다.)
			for (auto& iter_session : this->m_stack_session)
			{
				iter_session->m_pquerier.reset();
			}

			// Statistics)
			this->statistics_on_destroy_session(this->m_stack_session.size());

			// - session을 clear한다.
			this->m_stack_session.clear();

			// - 생성된 수를 0개로 설정한다.
			this->m_count_session_created = 0;

			// - Query를 clear한다.
			this->m_queue_query.clear();
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

	// trace) 
	LOG_INFO << "  - created session: "sv << m_count_session_created;


	//-----------------------------------------------------------------
	// 5. 마무리
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
	//while(m_queue_query.empty()==false || m_count_session_created!=this->m_stack_session.size())
	//{
	//	Sleep(1);
	//}

	// 4) executor를 Reset한다.
	this->m_pexecutor.reset();
	this->m_pexecutor_own.destroy();

	//scoped_lock(this->m_lockable_query)
	//{
	//	// 5) 먼저 Querier를 Reset한다.(이걸 하지 않으면 무한 free된다.)
	//	for(auto& iter:this->m_stack_session)
	//	{
	//		iter->m_pquerier.reset();
	//	}

	//	// Statistics)
	//	statistics_on_destroy_session(this->m_stack_session.size());

	//	// 6) session을 clear한다.
	//	this->m_stack_session.clear();

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
		// check) _connection_string가 empty면 안됀다.
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
		// check) _connection_string가 empty면 안됀다.
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

	// 2) set paprameters
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
	// 1) session을 clear한다.
	scoped_lock(this->m_lockable_query)
	{
		this->m_queue_query.clear();
	}
}

object_ptr<query::rdb::session_t> query::rdb::odbc::querier::alloc_session()
{
	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// declare)
	object_ptr<session_t> psession;

	// scoped_lock) 
	scoped_lock_named(this->m_lockable_query, cs)
	{
		// 1) session이 있을 경우 하나의 session을 얻는다.
		if (this->m_stack_session.empty() == false)
		{
			// - 일단 DBConnection을 얻는다.
			psession = std::move(this->m_stack_session.front());

			// - DBConnection을 하나 뽑아낸다.
			this->m_stack_session.pop_front();

			// - session에 querier를 설정한다.
			psession->m_pquerier = this;

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - 사용수를 증가시킨다.
			++psession->m_count_requested;
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// declare) 
			std::condition_variable_any cv_complete;

			// - push 
			this->m_queue_query.emplace_back({ [&](object_ptr<session>&& _psession)
				{
					psession = std::move(_psession);
					cv_complete.notify_one();
				}});

			// - session을 얻을 때까지 wait한다.
			cv_complete.wait(cs);
		}

		// statistics) 
		this->statistics_on_alloc_session();
	}

	// return)
	return psession;
}

void query::rdb::odbc::querier::post_alloc_session(std::function<void(session_t*)>&& _lambda)
{
	// check) is ready?
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

	class executable_query : public Iexecutable_query
	{
	public:
		virtual	intptr_t process_execute(intptr_t, std::size_t) override
		{
			// 1) get session
			auto psession = std::move(this->m_psession);

			// check)
			CGASSERT_ERROR(psession.exist());

			// 3) reset session
			this->m_lambda(psession);

			// return) 
			return 1;
		}

		std::function<void(session_t*)> m_lambda;
	};

	// 1) create query object
	auto pexecutable_query = make_object<executable_query>();
	pexecutable_query->m_lambda = std::move(_lambda);

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

	// 2) post
	scoped_lock_named(this->m_lockable_query, cs)
	{
		// 1) session이 있을 경우 하나의 session을 얻는다.
		if (this->m_stack_session.empty() == false)
		{
			// - 일단 DBConnection을 얻는다.
			auto psession = std::move(this->m_stack_session.front());

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - DBConnection을 하나 뽑아낸다.
			this->m_stack_session.pop_front();

			// - session에 querier를 설정한다.
			psession->m_pquerier = this;

			// - 사용수를 증가시킨다.
			++psession->m_count_requested;

			// - set session
			_pexecutable->m_psession = std::move(psession);

			// - post!
			try 
			{
				pexecutor->post(std::move(_pexecutable));
			}
			catch (...)
			{
				// - rollcack
				psession = std::move(_pexecutable->m_psession);

				// - reset
				psession->m_pquerier.reset();

				// - push back session
				this->m_stack_session.push_back(std::move(psession));

				// reraise)
				throw;
			}
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// - push
			this->m_queue_query.emplace_back({[=](object_ptr<session>&& _psession)
				{
					_pexecutable->m_psession = std::move(_psession);
					pexecutor->post(std::move(_pexecutable));
				}});
		}

		// statistics) 
		this->statistics_on_alloc_session();
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

	// declare) 
	QUEUED_QUERY pqueued_query;

	scoped_lock(this->m_lockable_query)
	{
		// 1) 더이상 queuing된 query가 없으면 session을 stack시킨다.
		if(this->m_queue_query.empty())
		{
			// - session을 Reset한다.
			_psession->m_pquerier.reset();

			// - session을 스택시킨다.
			this->m_stack_session.push_back(_psession);

			// return)
			return;
		}

		// 2) 제일 앞에 query를 얻는다.
		pqueued_query = std::move(this->m_queue_query.front());

		// 3) query 하나를 뽑아낸다.
		this->m_queue_query.pop_front();

		// 4) 사용 수를 증가시킨다.
		++_psession->m_count_requested;

		// statostics) 
		this->statistics_on_alloc_session();
	}

	// check) _psession이 nullptr이면 안됀다.
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