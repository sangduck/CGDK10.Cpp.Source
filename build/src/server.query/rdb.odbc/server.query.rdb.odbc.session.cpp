#include "pch.h"

using namespace CGDK;


query::rdb::odbc::session::session() :
	m_pdata_source(),
	m_handle_dbc(0),
	m_pquerier(),
	m_count_requested(0)
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

void query::rdb::odbc::session::open(data_source* _pdata_source)
{
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

	// 4) assing to member variable
	scoped_lock(this->m_lockable_odc)
	{
		this->m_handle_dbc = handle_dbc;
	}
}

void query::rdb::odbc::session::reopen()
{
	// check) 
	THROW_IF(this->m_pdata_source.empty(), throwable::failure(eRESULT::NOT_READY));

	// return)
	this->open(m_pdata_source);
}


void query::rdb::odbc::session::close() noexcept
{
	// Declare)
	SQLHDBC handle_dbc;

	// 1) copy handle values and clear
	scoped_lock(this->m_lockable_odc)
	{
		// - copy handle
		handle_dbc = this->m_handle_dbc;

		// - reset handle variable
		this->m_handle_dbc = 0;
		this->m_count_requested = 0;
	}

	// 2) close dbc(db connection) handle
	if(handle_dbc != 0)
	{
		// - close db connection
		::SQLDisconnect(handle_dbc);

		// - elease dbc handle
		::SQLFreeHandle(SQL_HANDLE_DBC, handle_dbc);
	}
}

result_code query::rdb::odbc::session::start_transaction() noexcept
{
	// declare) 
	SQLHDBC handle_dbc;

	// 1) get handle stmt
	scoped_lock(this->m_lockable_odc)
	{
		// check)
		RETURN_IF(this->m_handle_dbc == nullptr, eRESULT::NOT_READY);

		// check)
		RETURN_IF(this->m_flag_transaction == true, eRESULT::BUSY);

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
		// - reset flag
		scoped_lock(this->m_lockable_odc)
		{
			this->m_flag_transaction = false;
		}

		// return) 
		return eRESULT::FAIL;
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code query::rdb::odbc::session::commit_transaction() noexcept
{
	// declare) 
	SQLHDBC handle_dbc;

	// 1) get handle stmt
	scoped_lock(this->m_lockable_odc)
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
	scoped_lock(this->m_lockable_odc)
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
	scoped_lock(this->m_lockable_odc)
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
	scoped_lock(this->m_lockable_odc)
	{
		this->m_flag_transaction = false;
	}

	// check) 
	RETURN_IF(SQL_SUCCEEDED(result_sql) == false, eRESULT::FAIL);

	// return)
	return eRESULT::SUCCESS;
}

void query::rdb::odbc::session::process_dispose() noexcept
{
	if(this->m_pquerier.exist())
	{
		// 1) get handle stmt
		scoped_lock(this->m_lockable_odc)
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
