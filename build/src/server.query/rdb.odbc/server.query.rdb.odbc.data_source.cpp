#include "pch.h"

using namespace CGDK;


query::rdb::odbc::data_source::data_source() noexcept :
	m_handle_env(nullptr),
	m_driver_completion(SQL_DRIVER_NOPROMPT),	// SQL_DRIVER_COMPLETE
	m_str_initialization()
{
}

query::rdb::odbc::data_source::~data_source() noexcept
{
	if (this->m_handle_env != 0)
	{
		try
		{
			::SQLFreeHandle(SQL_HANDLE_ENV, this->m_handle_env);
		}
		catch (...)
		{
			// ignore any exception
		}

		this->m_handle_env = 0;
	}
}

void query::rdb::odbc::data_source::open(const std::string_view _initialization)
{
	try
	{
		// declare) 
		SQLHENV	handle_env;

		// 1) allocate an environment handle
		auto result = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handle_env);

		//// check) ...
		THROW_IF(SQL_SUCCEEDED(result) == false, throwable::failure(eRESULT::FAIL, 0, std::string_view("(err ) query::rdb::odbc: fail to alloc ENV handle ")));

		// 2) first try to odeb 3.8
		result = ::SQLSetEnvAttr(handle_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3_80, 0);

		// 3) we want odbc 3 support
		if (SQL_SUCCEEDED(result) == false)
		{
			// - odbc 3
			::SQLSetEnvAttr(handle_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

			// check) ...
			THROW_IF(SQL_SUCCEEDED(result) == false, throwable::failure(eRESULT::FAIL, 0, std::string_view("(err ) query::rdb::odbc: fail to set attribute 'SQL_ATTR_ODBC_VERSION' ")));
		}

		// 4) set result
		this->m_str_initialization = _initialization;
		this->m_handle_env = handle_env;
	}
	catch (...)
	{
		// - ATL trace
		LOG_ERROR << "(err ) fail to initialize ENV";

		// reraise)
		throw;
	}
}
