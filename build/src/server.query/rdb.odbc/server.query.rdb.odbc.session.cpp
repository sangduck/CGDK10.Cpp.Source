#include "pch.h"

using namespace CGDK;


query::rdb::odbc::session::session() :
	m_pdata_source(),
	m_handle_dbc(0),
	m_pquerier(),
	m_count_requested(0),
	m_tick_last_used(chrono::tick::now())
{
	// 1) Factory
	auto pfactory_object = factory::object<session>::instance(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	// statistics) 
	pfactory_object->statistics_on_alloc_create();

	// 2) assign
	this->m_factory_object = std::move(pfactory_object);
}
query::rdb::odbc::session::~session() noexcept
{
	// 1) close session
	this->close();

	// Declare)
	object_ptr<factory::object<session>> pfactory_object;

	// 1) assign factory object temporary
	pfactory_object = std::move(this->m_factory_object);

	// statistics) 
	if(pfactory_object.exist())
	{
		pfactory_object->statistics_on_free_delete();
	}
}

// - executes a statement directly and discards the result (true : succeeded)
static bool _execute_direct(SQLHDBC _handle_dbc, std::string_view _query) noexcept
{
	// check)
	if (_handle_dbc == 0 || _query.empty())
		return false;

	// 1) statement handle
	SQLHSTMT handle_stmt = SQL_NULL_HSTMT;
	if (SQL_SUCCEEDED(::SQLAllocHandle(SQL_HANDLE_STMT, _handle_dbc, &handle_stmt)) == false)
		return false;

	// 2) execute
	std::string str_query(_query);
	auto result = ::SQLExecDirectA(handle_stmt, reinterpret_cast<SQLCHAR*>(str_query.data()), SQL_NTS);

	// 3) free (the result set is discarded)
	::SQLFreeHandle(SQL_HANDLE_STMT, handle_stmt);

	// return)
	return SQL_SUCCEEDED(result) || result == SQL_NO_DATA;
}

// - 'SET SESSION wait_timeout' on connecting (MySQL/MariaDB only)
//   * the server closes the connection idle for _seconds. (a connection whose client is gone is not kept for the global 'wait_timeout')
static void _apply_session_wait_timeout(SQLHDBC _handle_dbc, uint32_t _seconds) noexcept
{
	// check)
	if (_seconds == 0)
		return;

	// 1) DBMS name
	char dbms_name[64] = {};
	SQLSMALLINT length = 0;
	if (SQL_SUCCEEDED(::SQLGetInfoA(_handle_dbc, SQL_DBMS_NAME, dbms_name, static_cast<SQLSMALLINT>(sizeof(dbms_name)), &length)) == false)
		return;

	std::string str_dbms(dbms_name);
	std::transform(str_dbms.begin(), str_dbms.end(), str_dbms.begin(), [](unsigned char _c) { return static_cast<char>(std::tolower(_c)); });

	// check) MySQL/MariaDB only
	if (str_dbms.find("mysql") == std::string::npos && str_dbms.find("mariadb") == std::string::npos)
		return;

	// 2) set
	if (_execute_direct(_handle_dbc, std::format("SET SESSION wait_timeout = {}", _seconds)) == false)
	{
		LOG_WARNING << "(warn) query::rdb::odbc: fail to set 'session_wait_timeout' ("sv << _seconds << ")"sv;
	}
}

bool query::rdb::odbc::session::ping(std::string_view _query) noexcept
{
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// check)
		if (this->m_handle_dbc == 0)
			return false;

		// 1) execute
		auto result = _execute_direct(this->m_handle_dbc, _query.empty() ? std::string_view("SELECT 1") : _query);

		// 2) used now
		if (result)
			this->m_tick_last_used = chrono::tick::now();

		// return)
		return result;
	}
	return false;
}
void query::rdb::odbc::session::open(data_source* _pdata_source)
{
	// check)
	CGASSERT_FATAL(_pdata_source != nullptr);

	// check)
	THROW_IF(_pdata_source == nullptr, throwable::failure(eRESULT::ALREADY_EXIST, 0, "(error) '_pdata_source' is nullptr"sv));

	// check)
	CGASSERT_ERROR(this->m_handle_dbc == 0);

	// check)
	THROW_IF(this->m_handle_dbc != 0, throwable::failure(eRESULT::ALREADY_EXIST, 0, "(error) odbc handle already initialized"sv));

	// check)
	CGASSERT_ERROR(_pdata_source->m_handle_env != 0);

	// check)
	THROW_IF(_pdata_source->m_handle_env == 0, throwable::failure(eRESULT::ALREADY_EXIST, 0, "(error) data source handle is nullptr"sv));

	// check)
	CGASSERT_ERROR(_pdata_source->m_str_initialization.empty() == false);

	// check)
	THROW_IF(_pdata_source->m_str_initialization.empty(), throwable::failure(eRESULT::NOT_READY, 0, "(error) data source initialization string is empty"sv));

	// declare) 
	SQLHDBC handle_dbc = nullptr;
	std::array<SQLCHAR, 1024> out_str;
	SQLSMALLINT out_strlen = 0;

	// 1) Allocate a connection handle
	auto result_sql = ::SQLAllocHandle(SQL_HANDLE_DBC, _pdata_source->m_handle_env, &handle_dbc);

	// check) 
	THROW_IF(SQL_SUCCEEDED(result_sql) == false, _dump_error_records_env(_pdata_source->m_handle_env))

	try
	{
		// 2) connect to DSN
		result_sql = ::SQLDriverConnect(handle_dbc, NULL, (SQLCHAR*)_pdata_source->m_str_initialization.c_str(), SQL_NTS, out_str.data(), static_cast<SQLSMALLINT>(out_str.size()), &out_strlen, _pdata_source->m_driver_completion);

		// check) throw exception if it failed
		THROW_IF(SQL_SUCCEEDED(result_sql) == false, _dump_error_records_dbc(handle_dbc));
	}
	catch (...)
	{
		// - free odbc handle
		::SQLFreeHandle(SQL_HANDLE_DBC, handle_dbc);

		// reraise)
		throw;
	}

	// 3) session wait timeout (MySQL/MariaDB)
	_apply_session_wait_timeout(handle_dbc, _pdata_source->m_session_wait_timeout);

	// 4) assing to member variable
	SCOPED_LOCK(this->m_lockable_odc)
	{
		this->m_handle_dbc = handle_dbc;
		this->m_tick_last_used = chrono::tick::now();
	}
}

void query::rdb::odbc::session::reopen()
{
	// ---------------------------------------------------------------------------------
	// 설명) session를 재생성한다.
	//       
	//   1) 기존 database connection handle(dbc) 접속을 끊는다.
	//   2) 기존 database connection handle(dbc)을 할당 해제한다.
	//   3) database connection handle(dbc) 재할당한다.
	//   4) database connection handle(dbc) 재연결한다.
	//   5) 재연결된 database connection handle(dbc)을 저장한다.
	// 
	// ---------------------------------------------------------------------------------
	// declare)
	object_ptr<data_source>	pdata_source;

	// 1) close existing dbc
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// - get datasource
		pdata_source = this->m_pdata_source;

		// check) 
		THROW_IF(pdata_source.empty(), throwable::failure(eRESULT::NOT_READY));

		// - free all cached prepared stmt handles (they are children of the old dbc handle)
		this->_clear_prepared_stmt();

		// - get dbc handle
		auto handle_dbc = this->m_handle_dbc;

		// - reset dbc handle
		if (handle_dbc != 0)
		{
			try
			{
				// - close db connection
				::SQLDisconnect(handle_dbc);

				// - release dbc handle
				::SQLFreeHandle(SQL_HANDLE_DBC, handle_dbc);
			}
			catch (...)
			{
				// ignore any exception
			}

			// - reset handle_dbc
			this->m_handle_dbc = 0;
		}

		// return)
		this->open(pdata_source);
	}
}

void query::rdb::odbc::session::close() noexcept
{
	// Declare)
	SQLHDBC handle_dbc;

	// 1) copy handle values and clear
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// - free all cached prepared stmt handles (while the dbc handle is still valid)
		this->_clear_prepared_stmt();

		// - copy handle
		handle_dbc = this->m_handle_dbc;

		// - reset handle variable
		this->m_handle_dbc = 0;
		this->m_count_requested = 0;
	}

	// 2) close dbc(db connection) handle
	if(handle_dbc != 0)
	{
		try
		{
			// - close db connection
			::SQLDisconnect(handle_dbc);

			// - release dbc handle
			::SQLFreeHandle(SQL_HANDLE_DBC, handle_dbc);
		}
		catch (...)
		{
			// ignore any exception
		}
	}
}

void query::rdb::odbc::session::start_transaction() 
{
	// declare) 
	SQLHDBC handle_dbc;

	// 1) get handle stmt
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// check)
		THROW_IF(this->m_handle_dbc == nullptr, throwable::failure(eRESULT::NOT_READY));

		// check)
		THROW_IF(this->m_flag_transaction == true, throwable::failure(eRESULT::BUSY));

		// - copy handle
		handle_dbc = this->m_handle_dbc;

		// - set flag
		this->m_flag_transaction = true;
	}

	// 2) begin transaction
	auto result = ::SQLSetConnectAttr(handle_dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);

	// check) ...
	if (SQL_SUCCEEDED(result) == false)
	{
		// declare)
		auto pquerier = this->m_pquerier;
		auto max_retry = pquerier->get_max_retry_on_fail();
		size_t retry = 0;
		throwable::failure_ex e;

		// - add error log
		e.add_description("(error) fail to 'start_transaction'");

		// - get error records
		CGDK::query::rdb::odbc::_dump_error_records(e, SQL_HANDLE_DBC, this->m_handle_dbc);

		// - set reason
		e.m_reason = result;

		while(SQL_SUCCEEDED(result) == false)
		{
			// check) throw if general query error or retry count out
			if (retry >= max_retry)
			{
				// - trace retry count
				if (retry > 0)
					e.add_description(CGDK::stream_string("- ", retry, " failed and re-tries"));

				// - reset flag
				SCOPED_LOCK(this->m_lockable_odc)
				{
					this->m_flag_transaction = false;
				}

				// statistics) 
				pquerier->statistics_on_query_fail();

				// raise)
				throw e;
			}

			// statistics) 
			pquerier->statistics_on_query_retry();

			// - decrease retry count
			++retry;

			try
			{
				// - regen session
				this->reopen();

				// - begin transaction
				result = ::SQLSetConnectAttr(this->m_handle_dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
			}
			catch (...)
			{

			}
		}
	}
}

result_code query::rdb::odbc::session::commit_transaction() noexcept
{
	// declare) 
	SQLHDBC handle_dbc;

	// 1) get handle stmt
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// check)
		RETURN_IF(this->m_handle_dbc == nullptr, eRESULT::NOT_READY);

		// check)
		RETURN_IF(this->m_flag_transaction == false, eRESULT::NOT_READY);

		// - copy handle
		handle_dbc = this->m_handle_dbc;
	}

	// 2) commit
	auto result_sql = ::SQLEndTran(SQL_HANDLE_DBC, handle_dbc, SQL_COMMIT);

	// 3) restore commit mode (to auto commit mode)
	::SQLSetConnectAttr(handle_dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);

	// 4) reset flag
	SCOPED_LOCK(this->m_lockable_odc)
	{
		this->m_flag_transaction = false;
	}

	// check) 
	RETURN_IF(SQL_SUCCEEDED(result_sql) == false, eRESULT::FAIL);

	// return)
	return eRESULT::SUCCESS;
}

result_code query::rdb::odbc::session::abort_transaction() noexcept
{
	// declare) 
	SQLHDBC handle_dbc;

	// 1) get handle stmt
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// check)
		RETURN_IF(this->m_flag_transaction == false, eRESULT::NOT_READY);

		// check)
		RETURN_IF(this->m_handle_dbc == nullptr, eRESULT::NOT_READY);

		// - copy handle
		handle_dbc = this->m_handle_dbc;
	}

	// 2) commit
	auto result_sql = ::SQLEndTran(SQL_HANDLE_DBC, handle_dbc, SQL_ROLLBACK);

	// 3) srestore commit mode (to auto commit mode)
	::SQLSetConnectAttr(handle_dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);

	// 4) reset flag
	SCOPED_LOCK(this->m_lockable_odc)
	{
		this->m_flag_transaction = false;
	}

	// check) 
	RETURN_IF(SQL_SUCCEEDED(result_sql) == false, eRESULT::FAIL);

	// return)
	return eRESULT::SUCCESS;
}

bool query::rdb::odbc::session::checkout_prepared_stmt(const std::string_view _command, SQLHSTMT& _handle_stmt, std::vector<sPARAM>& _vector_params) noexcept
{
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// 1) find by SQL text
		auto iter_find = this->m_map_prepared_stmt.find(_command);

		// check) not cached
		RETURN_IF(iter_find == this->m_map_prepared_stmt.end(), false);

		// 2) move out (the handle ownership moves to the caller)
		_handle_stmt = iter_find->second.handle_stmt;
		_vector_params = std::move(iter_find->second.vector_params);

		// 3) erase
		this->m_map_prepared_stmt.erase(iter_find);

		// return) got it
		return true;
	}
	return false;
}

bool query::rdb::odbc::session::checkin_prepared_stmt(std::string&& _command, SQLHSTMT _handle_stmt, std::vector<sPARAM>&& _vector_params) noexcept
{
	// check)
	RETURN_IF(_handle_stmt == nullptr || _command.empty(), false);

	SCOPED_LOCK(this->m_lockable_odc)
	{
		// check) the session must be connected (on a closed session the stmt handle is already invalid)
		RETURN_IF(this->m_handle_dbc == 0, false);

		// check) cache full (the caller must free the handle)
		RETURN_IF(this->m_map_prepared_stmt.size() >= MAX_PREPARED_STMT, false);

		// check) the same SQL text is already cached (keep the existing one)
		RETURN_IF(this->m_map_prepared_stmt.find(_command) != this->m_map_prepared_stmt.end(), false);

		// 1) store (the handle ownership moves to this session)
		this->m_map_prepared_stmt.emplace(std::move(_command), sPREPARED_STMT{ _handle_stmt, std::move(_vector_params) });

		// return) stored
		return true;
	}
	return false;
}

void query::rdb::odbc::session::_clear_prepared_stmt() noexcept
{
	SCOPED_LOCK(this->m_lockable_odc)
	{
		// 1) free all cached stmt handles (must be done before the dbc handle is freed)
		for (auto& iter : this->m_map_prepared_stmt)
		{
			try { ::SQLFreeHandle(SQL_HANDLE_STMT, iter.second.handle_stmt); } catch (...) {}
		}

		// 2) clear
		this->m_map_prepared_stmt.clear();
	}
}

void query::rdb::odbc::session::process_dispose() noexcept
{
	if(this->m_pquerier.exist())
	{
		// 1) get handle stmt
		SCOPED_LOCK(this->m_lockable_odc)
		{
			// check)
			CGASSERT_ERROR(this->m_flag_transaction == false);
		}

		this->m_pquerier->free_session(this);
	}
	else
	{
		delete this; 
	}
}
