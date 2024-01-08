#include "pch.h"

namespace CGDK
{


class query::rdb::oledb::querier::executable_query : public Iexecutable, public Npoolable<executable_query>, public factory::_traits_memory
{
public:
	virtual	void				on_final_release() noexcept override { m_pquery.reset(); m_pquerier.reset();}
	virtual	intptr_t			process_execute(intptr_t _return, std::size_t _param) override;

			object_ptr<Iquery>	m_pquery;
			object_ptr<Iquerier> m_pquerier;
};

intptr_t query::rdb::oledb::querier::executable_query::process_execute(intptr_t /*_return*/, std::size_t /*_param*/)
{
	// check) is m_pquery is nullptr?
	CGASSERT_ERROR(this->m_pquery.exist());

	// 1) execute query
	this->m_pquery->process_execute_query();

	// 2) reset session
	this->m_pquery->reset_session();

	// return) 
	return 1;
}

query::rdb::oledb::querier::querier() :
	Nnameable()
{
	// 1) set Object type
	this->set_object_type(eOBJECT_TYPE::QUERIER_DB_SQL);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::rdb::oledb::querier::querier(const std::string_view _name) :
	Nnameable(_name)
{
	// 1) set Object type
	this->set_object_type(eOBJECT_TYPE::QUERIER_DB_SQL);

	// 2) Register Querier to Manager
	this->register_querier();
}

query::rdb::oledb::querier::~querier() noexcept
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
	if (set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::DESTROYING_PENDING) == true)
	{
		// - process_destroy 함수를 호출한다.
		this->process_destroy();

		// - 상태를 eOBJECT_STATE::NONE로 변경한다.
		this->set_object_state(eOBJECT_STATE::NONE);
	}

	// 3) unregister
	unregister_querier();
}

circular_list<object_ptr<query::rdb::oledb::session>> query::rdb::oledb::querier::process_create_session(std::wstring_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	// check) parameter '_count_data_count' must be bigger than 0
	CGASSERT_ERROR(_count_data_source > 0);

	// check) throw exception if parameter '_count_data_count' is under 0
	THROW_IF(_count_data_source <= 0, throwable::failure(eRESULT::DOMAIN_ERROR, 0, std::string_view("(err ) query::rdb::oledb: _count_data_source is <0")));

	// free-Release) free-Release일 경우 session수의 최대를 1개로 제한한다.
#ifdef _FREE_RELEASE
	SATURATE_MAX(_count_data_source, 1);
#endif

	// declare) 
	circular_list<object_ptr<session>> list_session;

	// declare)
	throwable::failure_ex e(eRESULT::SUCCESS);

	for(std::size_t i = 0; i<_count_data_source; ++i)
	{
		// 1) allocate data_source ojbect
		auto pdata_source = make_object<data_source>();

		// 2) open data_source
		pdata_source->open(_connection_string);

		// 3) session을 Source당 session수만큼 연다.
		for(std::size_t j = 0; j < _count_session; ++j)
		{
			// - allocate new session object
			auto psession = new session();

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

				// break)
				break;
			}
			catch(...)
			{
				// - delete session
				delete psession;

				// - copy exception
				e = throwable::failure_ex(eRESULT::DOMAIN_ERROR, 0, "(err ) query::rdb::odbc: fail to creating sessions"sv);

				// break)
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

namespace query::rdb::oledb
{
	std::string_view get_suitable_oledb_provider(eDB_PROVIDER _provider);
	std::string_view get_suitable_odbc_driver(eDB_PROVIDER _provider);
}


result_code query::rdb::oledb::querier::process_start(context& _context)
{
	// notify) notify openning message
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::OPENNING, 0, reinterpret_cast<uintptr_t>(this)));

	// trace) 
	LOG_PROGRESS << "@ <BEGIN> start QUERIER_RDB_OLEDB["sv << name() << "]"sv;

	// 1) CoInitialize
#if defined(_WIN32)
	auto result = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);

	// check) 
	THROW_IF(
		result != S_OK &&
		result != S_FALSE,		// S_FALSE indicates that the COM library is already initialized.
		throwable::failure(eRESULT::BAD_FUNCTION_CALL));
#elif defined(__linux__)
#else
	#error "CGDK Not supported platform"
#endif

	// declare)
	auto str_name = name();
	context	context_old	 = _context;

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
		LOG_FATAL << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << str_name << "]: exception complete [P1-1]"sv;

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
		LOG_FATAL << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << str_name << "]: exception complete [P1-2]"sv;

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
		str_provider = get_suitable_oledb_provider(parameter.provider);
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
			parameter.options = (std::string)_context.find("option"sv, "options"sv);
		}

		// - session Per Source
		auto value_find = _context["session"sv];

		if (value_find.is_valid())
		{
			// - ...
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

	// 7) get value 'connection_string' 
	{
		// declare) 
		auto wstr_initial = alloc_shared_buffer(8192);
		auto str_output = alloc_shared_buffer(8192);
		bool use_odbc_driver = false;
		bool is_access = false;

		// - if key "connection_string" exist
		if (str_connection_string.empty() == false)
		{
			wstr_initial.append_text(encode_string(wstr_initial.data<wchar_t>(), static_cast<std::string_view>(str_connection_string)));
			wstr_initial.append<wchar_t>(0);
			str_output.append_text(static_cast<std::string_view>(str_connection_string));
			str_output.append<char>(0);
		}
		else
		{
			// declare) 
			auto temp_buffer = alloc_shared_buffer(8192);
			std::wstring_view wstr_initial_option;
			std::string_view str_initial_option;
			std::string_view str_provider_suitable;

			// - try to get key "provider"
			if (str_provider.empty() == false)
			{
				// - to lower case
				auto value = __to_lower(std::string_view(str_provider));

				// is value "dsn"
				if (__stricmp(value, "dsn"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::DSN);	// "MSDASQL1"
					wstr_initial_option	 = L"";
					str_initial_option	 = "";
				}
				// is value "sql_server"
				else if (__stricmp(value, "sqlserver"sv) == 0 || __stricmp(value, "sql_server"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::SQL_SERVER); // "SQLOLEDB.1";
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
				}
				// is value "sqlserver_native"
				else if (__stricmp(value, "sqlserver_native") == 0 || __stricmp(value, "sql_server_native"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::SQL_SERVER_NATIVE);	// "{SQL Server}";
					str_initial_option	 = ""sv;
				}
				// is value "sql_server_18"
				else if (__stricmp(value, "sqlserver_18"sv) == 0 || __stricmp(value, "sql_server_18"sv) == 0)
				{
					str_provider_suitable = "MSOLEDBSQL"sv;
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
				}
				// is value "sql_server_odbc"
				else if (__stricmp(value, "sql_server_odbc"sv) == 0 || __stricmp(value, "sqlserver_odbc"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::SQL_SERVER_ODBC);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
				}
				// is value "mysql"
				else if (__stricmp(value, "mysql"sv) == 0 || __stricmp(value, "my_sql"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_ansi"
				else if (__stricmp(value, "mysql_ansi"sv) == 0 || __stricmp(value, "my_sql_ansi"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_ANSI);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_unicode"
				else if (__stricmp(value, "mysql_unicode"sv) == 0 || __stricmp(value, "my_sql_unicode"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_3_5"
				else if (__stricmp(value, "mysql_3_5"sv) == 0 || __stricmp(value, "my_sql_3_5"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_1"
				else if (__stricmp(value, "mysql_5_1"sv) == 0 || __stricmp(value, "my_sql_5_1"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_1_ansi"
				else if (__stricmp(value, "mysql_5_1_ansi"sv) == 0 || __stricmp(value, "my_sql_5_1_ansi"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_1_unicode"
				else if (__stricmp(value, "mysql_5_1_unicode"sv) == 0 || __stricmp(value, "my_sql_5_1_unicode"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_2"
				else if (__stricmp(value, "mysql_5_2"sv) == 0 || __stricmp(value, "my_sql_5_2"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_2_ansi"
				else if (__stricmp(value, "mysql_5_2_ansi"sv) == 0 || __stricmp(value, "my_sql_5_2_ansi"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_2_unicode"
				else if (__stricmp(value, "mysql_5_2_unicode"sv) == 0 || __stricmp(value, "my_sql_5_2_unicode"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_3"
				else if (__stricmp(value, "mysql_5_3"sv) == 0 || __stricmp(value, "my_sql_5_3"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_3_ansi"
				else if (__stricmp(value, "mysql_5_3_ansi"sv) == 0 || __stricmp(value, "my_sql_5_3_ansi"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_5_3_unicode"
				else if (__stricmp(value, "mysql_5_3_unicode"sv) == 0 || __stricmp(value, "my_sql_5_3_unicode"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_8"
				else if (__stricmp(value, "mysql_8"sv) == 0 || __stricmp(value, "my_sql_8"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_8_ansi"
				else if (__stricmp(value, "mysql_8_ansi"sv) == 0 || __stricmp(value, "my_sql_8_ansi"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mysql_8_unicode"
				else if (__stricmp(value, "mysql_8_unicode"sv) == 0 || __stricmp(value, "my_sql_8_unicode"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MYSQL_UNICODE);
					wstr_initial_option	 = L";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					str_initial_option	 = ";option=67108872"sv; // {MULTI_STATEMENTS} 67108864 + {BIG_PACKETS} 8
					use_odbc_driver		 = true;
				}
				// is value "mariadb"
				else if (__stricmp(value, "mariadb"sv) == 0 || __stricmp(value, "maria_db"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MARIADB);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
					use_odbc_driver		 = true;
				}
				// is value "mariadb_3_1"
				else if (__stricmp(value, "mariadb_3_1"sv) == 0 || __stricmp(value, "maria_db_3_1"sv) == 0)
				{
					str_provider_suitable = get_suitable_odbc_driver(eDB_PROVIDER::MARIADB_3_1);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
					use_odbc_driver		 = true;
				}
				// is value "SQLOLEDB.1"
				else if (__stricmp(value, "SQLOLEDB.1"sv) == 0 || __stricmp(value, "SQLOLEDB"sv) == 0)
				{
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
				}
				// is value "MSDASQL.1"
				else if (__stricmp(value, "MSDASQL.1"sv) == 0 || __stricmp(value, "MSDASQL"sv) == 0)
				{
				}
				// is value "excel"
				else if (__stricmp(value, "excel"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::EXCEL);
					wstr_initial_option	 = L"Extended Properties=\"Excel 12.0 Xml;HDR=YES;READONLY=FALSE;\""sv;
					str_initial_option	 = "Extended Properties=\"Excel 12.0 Xml;HDR=YES;READONLY=FALSE;\""sv;
					is_access = true;
				}
				// is value "access"
				else if (__stricmp(value, "access"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::ACCESS);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
					is_access = true;
					//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
				}
				// is value "access_dbase"
				else if (__stricmp(value, "access_dbase"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::ACCESS_DBASE);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
					is_access = true;
					//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
				}
				// is value "access_text"
				else if (__stricmp(value, "access_text"sv) == 0)
				{
					str_provider_suitable = get_suitable_oledb_provider(eDB_PROVIDER::ACCESS_TEXT);
					wstr_initial_option	 = L""sv;
					str_initial_option	 = ""sv;
					is_access = true;
					//str_initial_option	 = "FIRSTROWHASNAMES=1;READONLY=FALSE"sv;
				}
				else
				{
				}

				// check) str_value is nullptr?
				if (str_provider_suitable.empty())
				{
					// trace) 
					LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << name() << "]: cann't find oledb driver for provider '"sv << str_provider << "'"sv;

					// throw)
					throw throwable::failure_ex(eRESULT::INVALID_ARGUMENT);
				}

				if (use_odbc_driver == false)
				{
					wstr_initial.append_text<wchar_t>(L"Provider="sv);
					wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), str_provider_suitable));
					wstr_initial.append_text<wchar_t>(L";"sv);

					str_output.append_text<char>("Provider="sv );
					str_output.append_text<char>(str_provider_suitable);
					str_output.append_text<char>(";"sv);
				}
				else
				{
					wstr_initial.append_text<wchar_t>(L"Provider=MSDASQL;Driver="sv);
					wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), str_provider_suitable));
					wstr_initial.append_text<wchar_t>(L";"sv);

					str_output.append_text<char>("Provider=MSDASQL;Driver="sv);
					str_output.append_text<char>(str_provider_suitable);
					str_output.append_text<char>(";"sv);
				}
			}
			else
			{
				wstr_initial.append_text<wchar_t>(L"Provider=SQLOLEDB.1;"sv);
				str_output.append_text<char>("Provider=SQLOLEDB.1;"sv);
			}


			// - try to get key "data_source"
			if(parameter.data_source.empty() == false)
			{
				if (use_odbc_driver == false)
				{
					wstr_initial.append_text<wchar_t>(L"Data Source="sv);
					wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.data_source));
					wstr_initial.append_text<wchar_t>(L";"sv);

					str_output.append_text<char>("Data Source="sv);
					str_output.append_text<char>(parameter.data_source);
					str_output.append_text<char>(";"sv);
				}
				else
				{
					wstr_initial.append_text<wchar_t>(L"Server="sv);
					wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.data_source));
					wstr_initial.append_text<wchar_t>(L";"sv);

					str_output.append_text<char>("Server="sv);
					str_output.append_text<char>(parameter.data_source);
					str_output.append_text<char>(";"sv);
				}
			}

			// - try to get key "user_id"
			if (parameter.user_id.empty() == false)
			{
				wstr_initial.append_text<wchar_t>((use_odbc_driver == false) ? L"User ID="sv : L"Uid="sv);
				wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.user_id));
				wstr_initial.append_text<wchar_t>(L";"sv);

				str_output.append_text<char>((use_odbc_driver == false) ? std::string_view("User ID="sv) : std::string_view("Uid="sv));
				str_output.append_text<char>(parameter.user_id);
				str_output.append_text<char>(";"sv);
			}

			// - try to get key "password"
			if (parameter.password.empty() == false)
			{
				wstr_initial.append_text<wchar_t>((use_odbc_driver == false) ? L"Password="sv : L"Pwd="sv);
				wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.password));
				wstr_initial.append_text<wchar_t>(L";"sv);

				str_output.append_text<char>((use_odbc_driver == false) ? std::string_view("Password="sv) : std::string_view("Pwd="sv));
				str_output.append_text<char>("*"sv);
				str_output.append_text<char>(";"sv);
			}

			// - try to get key "initial_catalog"
			if (parameter.initial_catalog.empty() == false)
			{
				wstr_initial.append_text<wchar_t>((use_odbc_driver == false) ? L"Initial Catalog="sv : L"Database="sv);
				wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.initial_catalog));

				str_output.append_text<char>((use_odbc_driver == false) ? "Initial Catalog="sv : "Database="sv);
				str_output.append_text<char>(parameter.initial_catalog);
			}

			// - append initial options
			if (wstr_initial_option.empty() == false)
			{
				wstr_initial.append_text<wchar_t>(wstr_initial_option);
				str_output.append_text<char>(str_initial_option);
			}

			// - append options
			if (parameter.options.empty() == false)
			{
				wstr_initial.append_text<wchar_t>(encode_string(temp_buffer.data<wchar_t>(), parameter.options));
				str_output.append_text<char>(parameter.options);

				// trace)
				LOG_INFO << "  + parameter['option']: "sv << parameter.options;
			}

			//if(provider == eDB_PROVIDER::SQL_SERVER)
			//{
			//	// mssql
			//	wstr_initial.append_text(L";Persist Security Info=false;Use Procedure for prepare=1;Auto Translate=True;Packet Size=8192;Use Encryption for Data=False;Tag with column collation when possible=False"sv);
			//	str_output.append_text (";Persist Security Info=false;Use Procedure for prepare=1;Auto Translate=True;Packet Size=8192;Use Encryption for Data=False;Tag with column collation when possible=False"sv);
			//}
			//else if (provider == eDB_PROVIDER::MYSQL)
			//{
			//	// mysql
			//	wstr_initial.append_text(L";Persist Security Info=false;Max Pool Size=256"sv);
			//	str_output.append_text(";Persist Security Info=false;Max Pool Size=256"sv);
			//}
			//else
			//{
			//	CGASSERT_ERROR(false);
			//	throw	throwable::failure(eRESULT::INVALID_ARGUMENT);
			//}

			// - 마지막에 NULL을 붙여준다.
			wstr_initial.append<wchar_t>(0);
			str_output.append<char>(0);

			// trace)
			LOG_INFO << "  + parameter['connection_string']: '"sv << str_output.data<char>() << "'"sv;
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
		LOG_INFO << "  > trying to creating session(" << name() << ")..."sv;

		try
		{
			// - create session
			list_session = process_create_session(wstr_initial.data<wchar_t>(), parameter.count_session, parameter.count_data_source);

			// check) throw exception if failed
			if (list_session.empty())
			{
				// trace) 
				LOG_PROGRESS << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << str_name << "]: connection string is invalid [P2] (connection_string: "sv << str_output.data<char>() << ")"sv;

				// throw) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, __format(
					"<TERMINATE> start QUERIER_RDB_OLEDB[{}]: connection string is invalid [P2] (connection_string: {}) ({})",
					(str_name.empty() == false) ? str_name.data() : "",
					(str_output.data() != nullptr) ? str_output.data() : "-")
				);
			}
		}
		catch (...)
		{
			// trace) 
			LOG_ERROR << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << str_name << "]: connection string is invalid [P2] (connection_string: "sv << str_output.data<char>() << ")"sv;

			// throw)
			throw;
		}

		// - disable retry if access
		if (is_access)
		{
			set_max_retry_on_fail(0);
		}
	}

	try
	{
		// 8) executor가 nullptr이면 새로 생성한다.
		if(pexecutor == nullptr)
		{
			// - executor Thread를 생성한다.
			auto temp_executor = make_own<executor::io_context_threads>();

			// - Thread를 시작한다.
			temp_executor->start({ list_session.size() + 1 });

			// - executor를 설정.
			pexecutor = temp_executor;
			pexecutor_own = std::move(temp_executor);
		}

		// 8) thread pool이 있는지 확인한다.
		auto pthread_pool = dynamic_pointer_cast<api::threads>(pexecutor);

		// check) thread pool 이 있다면 thread의 갯수를 확인한다.
		if (pthread_pool.exist())
		{
			// - trace) thread의 갯수가 현재 session 수보다 적으면 경고한다.
			EXECUTE_IF(pthread_pool->reserved_size() < list_session.size() + 1, LOG_INFO << "  ! (warning) thread count is too small. at least "sv << list_session.size() + 1 << " is needed.  ("sv << name() << ")!");
		}

		// 9) executor를 설정한다.
		this->m_stack_session.swap(list_session);
		this->m_pexecutor = std::move(pexecutor);
		this->m_pexecutor_own = std::move(pexecutor_own);
		this->m_count_session_created	+= (long)m_stack_session.size();
		this->statistics_on_create_session(m_stack_session.size());
	}
	catch (std::exception& e)
	{
		// trace) 
		LOG_ERROR << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << name() << "]:  fail to starting oledb!!! [P3-1]"sv;

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
		LOG_ERROR << "@ <TERMINATE> start QUERIER_RDB_OLEDB["sv << name() << "]:  fail to starting oledb!!! [P3-2]"sv;

		// - setting을 원래대로 돌린다.
		_context = context_old;

		// reraise)
		throw;
	}


	//-----------------------------------------------------------------
	// 4. ChildnAcceptPreparedOnStart만큼 Accept걸기.
	//-----------------------------------------------------------------
	// declare)
	auto iter = m_container.begin();
	auto iterEnd = m_container.end();

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
		while (m_queue_query.empty() == false || m_count_session_created != m_stack_session.size())
		{
			Sleep(1);
		}

		// - executor를 reset한다.
		pexecutor.reset();
		pexecutor_own.destroy();

		scoped_lock(m_lockable_query)
		{
			// - 먼저 Querier를 reset한다.(이걸 하지 않으면 무한 free된다.)
			for (auto& iter_session : m_stack_session)
			{
				iter_session->m_pquerier.reset();
			}

			// statistics)
			statistics_on_destroy_session(m_stack_session.size());

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
	LOG_PROGRESS << "@ <END> start QUERIER_RDB_OLEDB["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code query::rdb::oledb::querier::process_stop() noexcept
{
	// Message) Message 날리기..
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::CLOSING, 0, reinterpret_cast<uintptr_t>(this)));

	// 1) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
	}

	// 2) Childe를 stop한다.
	for (auto riter = m_container.rbegin(); riter != m_container.rend(); ++riter)
	{
		object::stop(*riter);
	}

	// 3) 모든 Query가 완료되고 모든 session이 완료되도록 기다린다.
	while(m_queue_query.empty() == false || m_count_session_created != m_stack_session.size())
	{
		Sleep(1);
	}

	// 4) executor를 reset한다.
	this->m_pexecutor.reset();
	this->m_pexecutor_own.destroy();

	scoped_lock(this->m_lockable_query)
	{
		// 5) 먼저 Querier를 reset한다.(이걸 하지 않으면 무한 free된다.)
		for(auto& iter:m_stack_session)
		{
			iter->m_pquerier.reset();
		}

		// statistics)
		this->statistics_on_destroy_session(m_stack_session.size());

		// 6) session을 clear한다.
		this->m_stack_session.clear();

		// 7) 생성된 수를 0개로 설정한다.
		this->m_count_session_created = 0;

		// 8) Query를 clear한다.
		this->m_queue_query.clear();
	}

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

result_code query::rdb::oledb::querier::_process_destroy() noexcept
{
	// 1) Call Parent's _process_destroy function
	auto result = this->Ninitializable::_process_destroy();

	// 2) unregister all messageable objects
	this->reset_message_transmitter();

	// return)
	return result;
}


result_code query::rdb::oledb::querier::process_message(sMESSAGE& _msg)
{
	// 1) Log Message Dispatch
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}

result_code query::rdb::oledb::querier::start(const std::string_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _connection_string가 empty면 안됀다.
		THROW_IF(_connection_string.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter['_connection_string'] is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: invalid parameters"sv;

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

result_code query::rdb::oledb::querier::start(Iexecutor* _pexecutor, const std::string_view _connection_string, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _connection_string가 empty면 안됀다.
		THROW_IF(_connection_string.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter['_connection_string'] is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// 1) 잠시 저장해 놓음
	auto pexecutor = this->m_pexecutor;

	// 2) 
	context context_setting;

	context_setting["connection_string"sv] = static_cast<std::string_view>(_connection_string);
	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// declare)
	result_code result;

	try
	{
		// 3) executor를 설정한다.
		this->m_pexecutor = _pexecutor;

		// 4) start한다.
		result = ((Istartable*)this)->start(context_setting);
	}
	catch(...)
	{
		// - executor를 되돌린다.
		this->m_pexecutor = pexecutor;

		// reraise) 
		throw;
	}

	// return)
	return result;
}

result_code query::rdb::oledb::querier::start(const std::string_view _provider, const std::string_view _data_source, const std::string_view _user_id, const std::string_view _password, const std::string_view _init_catalog, const std::string_view _option, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _provider가 nullptr이면 안됀다.
		THROW_IF(_count_data_source <= 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_count_data_source is 0 or smaller. must be >1"sv));

		// check) _provider가 nullptr이면 안됀다.
		THROW_IF(_provider.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_provider is empty"sv));

		// check) _data_source가 nullptr이면 안됀다.
		THROW_IF(_data_source.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_data_source is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// declare)
	context context_setting;

	// 1) set parameters
	context_setting["provider"sv] = static_cast<std::string_view>(_provider);
	context_setting["data_source"sv] = static_cast<std::string_view>(_data_source);

	if (_user_id.empty() == false)
		context_setting["user_id"sv] = static_cast<std::string_view>(_user_id);

	if (_password.empty() == false)
		context_setting["password"sv] = static_cast<std::string_view>(_password);

	if (_init_catalog.empty() == false)
		context_setting["initial_catalog"sv] = static_cast<std::string_view>(_init_catalog);

	if (_option.empty() == false)
		context_setting["option"sv] = static_cast<std::string_view>(_option);

	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// 2) start
	return this->start(context_setting);
}

template<class T_CHAR>
std::basic_string_view<T_CHAR> _get_provider_string(eDB_PROVIDER _provider);

result_code query::rdb::oledb::querier::start(eDB_PROVIDER _provider, const std::string_view _data_source, const std::string_view _user_id, const std::string_view _password, const std::string_view _init_catalog, const std::string_view _option, std::size_t _count_session, std::size_t _count_data_source)
{
	try
	{
		// check) _provider가 nullptr이면 안됀다.
		THROW_IF(_count_data_source <= 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_count_data_source is 0 or smaller. must be >1"sv));

		// check) _data_source가 nullptr이면 안됀다.
		THROW_IF(_data_source.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "_data_source is empty"sv));
	}
	catch (throwable::failure& _e)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: "sv << _e.description();

		// reraise) 
		throw;
	}
	catch (...)
	{
		// declare)
		auto str_name = this->name();

		// trace)
		LOG_FATAL << "@ failed QUERIER_RDB_OLEDB["sv << str_name << "]: invalid parameters"sv;

		// reraise) 
		throw;
	}

	// declare)
	context context_setting;

	// 1) set parameters
	context_setting["provider"sv] = CGDK::_get_provider_string<char>(_provider);;
	context_setting["data_source"sv] = static_cast<std::string_view>(_data_source);

	if (_user_id.empty() == false)
		context_setting["user_id"sv] = static_cast<std::string_view>(_user_id);

	if (_password.empty() == false)
		context_setting["password"sv] = static_cast<std::string_view>(_password);

	if (_init_catalog.empty() == false)
		context_setting["initial_catalog"sv] = static_cast<std::string_view>(_init_catalog);

	if (_option.empty() == false)
		context_setting["option"sv] = static_cast<std::string_view>(_option);

	context_setting["session"sv] = _count_session;
	context_setting["data_source_count"sv] = _count_data_source;

	// 2) start
	return this->start(context_setting);
}

void query::rdb::oledb::querier::process_post_execute_query(Iserver_query* _pquery)
{
	//-----------------------------------------------------------------
	// 동작설명)
	//
	//  1. post_execute_query는 Query를 비동기적 Query걸어주는 함수다.
	//  2. post_execute_query를 수행하게 되면 일단 Stack된 DBconnection이 
	//     있는 지를 먼저 검사한다. 
	//  3. DBconnection이 있을 경우 DBconnection을 받아서 바로 Query를 
	//     걸게 된다.
	//  4. 만약에 DBconnection이 없을 경우 Qerier는 m_queue_query에 
	//     큐잉한다.
	//  
	//-----------------------------------------------------------------
	// 1) casting
	auto pquery = dynamic_cast<Iquery*>(_pquery);

	// check) pquery must not be nullptr
	CGASSERT_ERROR(pquery != nullptr);

	// check) throw exception if pquery is nullptr
	THROW_IF(pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter 'pquery' is nullptr"sv));

	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// declare) 
	object_ptr<session>	psession;

	scoped_lock(this->m_lockable_query)
	{
		// 1) DBconnection이 하나도 없을 경우 그냥 Querier를 Queuing한다.
		if(this->m_stack_session.empty())
		{
			// declare) 
			QUEUED_QUERY temp_query(this);

			// - Queuing한 Query값을 설정한다.
			temp_query.pquery = pquery;
			temp_query.f_execute = &querier::query_post;

			// - Querier를 queuing한다.
			this->m_queue_query.push_back(std::move(temp_query));

			// return) 종료~
			return;
		}

		// 2) 일단 DBconnection을 얻는다.
		psession = this->m_stack_session.front();

		// 3) session에 Querier를 설정한다.
		psession->m_pquerier = this;

		// 4) DBconnection을 하나 뽑아낸다.
		this->m_stack_session.pop_front();
	}

	// check) psession이 nullptr이면 안됀다.
	CGASSERT_ERROR(psession.exist());

	// 5) 사용 수를 증가시킨다.
	++psession->m_count_requested;

	// 6) session을 설정한다.
	pquery->set_session(psession);

	// 7) Query 수행함...
	this->query_post(pquery);

	// 통계) 
	this->statistics_on_alloc_session();
}

void query::rdb::oledb::querier::process_execute_query(Iserver_query* _pquery)
{
	// 1) casting
	auto pquery = dynamic_cast<Iquery*>(_pquery);

	// check) _pquery must not be nullptr
	CGASSERT_ERROR(pquery != nullptr);

	// check) throw exception if _pquery is nullptr
	THROW_IF(pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_pquery' is nullptr"sv));

	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	// 1) session을 할당받는다.
	this->alloc_session(pquery);

	// 2) QUery를 건다.
	pquery->process_execute_query();
}

bool query::rdb::oledb::querier::cancel_query(Iserver_query* _pquery) noexcept
{
	// check) _pquery must not be nullptr
	RETURN_IF(_pquery == nullptr, false);

	scoped_lock(this->m_lockable_query)
	{
		// 1) Begin/End
		auto iter_find = std::find(this->m_queue_query.begin(), this->m_queue_query.end(), _pquery);

		// check) 찾지 못했으면 끝낸다.
		RETURN_IF(iter_find == this->m_queue_query.end(), false)

		// 2) 삭제
		this->m_queue_query.erase(iter_find);
	}

	// return) 
	return true;
}

void query::rdb::oledb::querier::cancel_all_queries() noexcept
{
	scoped_lock(this->m_lockable_query)
	{
		// 1) session을 clear한다.
		this->m_queue_query.clear();
	}
}

void query::rdb::oledb::querier::alloc_session(Iquery* _pquery)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_pquery != nullptr);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_pquery == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "parameter '_pquery' is nullptr"sv));

	// check) object_state() must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING'
	THROW_IF(this->get_object_state() != eOBJECT_STATE::RUNNING && this->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, "querier is not ready (state is must be 'eOBJECT_STATE::RUNNING' or 'eOBJECT_STATE::START_PENDING')"sv));

	scoped_lock_named(this->m_lockable_query, cs)
	{
		// 1) session이 있을 경우 하나의 session을 얻는다.
		if(this->m_stack_session.empty() == false)
		{
			// - 일단 DBconnection을 얻는다.
			auto psession = this->m_stack_session.front();

			// - DBconnection을 하나 뽑아낸다.
			this->m_stack_session.pop_front();

			// - session에 Querier를 설정한다.
			psession->m_pquerier = this;

			// check) psession은 반드시 nullptr이 아니어야 한다.
			CGASSERT_ERROR(psession.exist());

			// - 사용수를 증가시킨다.
			++psession->m_count_requested;

			// - session을 설정한다.
			_pquery->set_session(psession);

			// check) m_psession이 nullptr이면 안됀다.
			CGASSERT_ERROR(_pquery->m_psession.exist());
		}
		// 2) session이 없을 경우 대기한다.
		else
		{
			// declare) 
			QUEUED_QUERY temp_query(this);

			temp_query.pquery = _pquery;
			temp_query.f_execute = &querier::query_request;

			// - 먼저 Push한다.
			this->m_queue_query.push_back(temp_query);

			// - session을 얻을 때까지 wait한다.
			_pquery->m_cv_complete.wait(cs);

			// check) m_psession이 nullptr이면 안됀다.
			CGASSERT_ERROR(_pquery->get_session().exist());
		}

		// 통계) 
		this->statistics_on_alloc_session();
	}

	// check) m_psession이 nullptr이면 안됀다.
	CGASSERT_ERROR(_pquery->m_psession.exist());
}

void query::rdb::oledb::querier::free_session(session* _psession) noexcept
{
	// check) _psession must not be nullptr
	CGASSERT_ERROR(_psession != nullptr);

	// check) return if _psession is nullptr
	RETURN_IF(_psession == nullptr);

	// statistics) 
	this->statistics_on_free_session();

	// declare) 
	QUEUED_QUERY pqueued_query(this);

	scoped_lock(this->m_lockable_query)
	{
		// 1) 먼저 Regeneration 과정을 처리한다.
		if (this->m_interval_regeneration > 0)
		{
			// - 만약 설정된 Regeneration 수를 Query했을 경우 session을 재생성한다.
			if (_psession->m_count_requested > this->m_interval_regeneration)
			{
				// - 먼저 session을 닫는다.
				_psession->Close();

				// - 새로운 session을 생성한다.
				auto psession = new session();

				try
				{
					// - 새로운 session을 open한다.
					psession->open(_psession->m_pdata_source);

					// - 기존 session에서 정보를 옮기고 기존 session을 제거한다.
					psession->m_pdata_source = std::move(_psession->m_pdata_source);
					psession->m_pquerier = std::move(_psession->m_pquerier);
					delete _psession;

					// - session을 교체
					_psession = psession;
				}
				catch(...)
				{
					// - 지운다.
					delete psession;
				}
			}
		}

		// 2) 더이상 Queuing된 Query가 없으면 session을 스택시켜놓는다.
		if(this->m_queue_query.empty())
		{
			// - session을 reset한다.
			_psession->m_pquerier.reset();

			// - session을 스택시킨다.
			this->m_stack_session.push_back(_psession);

			// return) 리턴한다.
			return;
		}

		// 3) 제일 앞에 Query를 얻는다.
		pqueued_query = std::move(this->m_queue_query.front());

		// 4) Query 하나를 뽑아낸다.
		this->m_queue_query.pop_front();

		// check) _psession이 nullptr이면 안됀다.
		CGASSERT_ERROR(_psession != nullptr);

		// 5) 사용 수를 증가시킨다.
		++_psession->m_count_requested;

		// 6) session을 설정한다.
		pqueued_query.pquery->set_session(_psession);

		// statistics) 
		this->statistics_on_alloc_session();
	}

	// 8) execute queued query
	(this->*pqueued_query.f_execute)(pqueued_query.pquery);
}

void query::rdb::oledb::querier::query_request(Iquery* _pquery)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_pquery != nullptr);

	// 1) notify complete
	_pquery->m_cv_complete.notify_one();
}

void query::rdb::oledb::querier::query_post(Iquery* _pquery)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_pquery != nullptr);

	// 1) Query Executable을 얻는다.
	auto pexecutable_query = make_object<executable_query>();

	// 2) 설정하기
	pexecutable_query->m_pquery = _pquery;
	pexecutable_query->m_pquerier = this;

	// check)
	CGASSERT_ERROR(pexecutable_query->m_pquery!=nullptr);

	// 3) execute 걸기.
	this->m_pexecutor->post(std::move(pexecutable_query));
}

result_code query::rdb::oledb::querier::process_transmit_message(sMESSAGE& _msg)
{
	// 1) 기존의 Dispatch를 수행한다.
	auto result = this->Nmessage_transmitter::process_transmit_message(_msg);

	// 2) Querier Manager에 Dispatch한다.
	CGDK::query::manager_querier::instance()->transmit_message(_msg);

	// return) 
	return	result;
}


}