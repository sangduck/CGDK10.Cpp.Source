#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::_ODBC_get_binary_data(SQLHSTMT _handle_stmt, SQLUSMALLINT _ordinal, SQLSMALLINT _target_type)
{
	// definition)
	const SQLLEN INITIAL_BUFFER_SIZE = 256;

	// 1) alloc buffer
	shared_buffer buf_result = alloc_shared_buffer(INITIAL_BUFFER_SIZE);

	// declare) 
	SQLLEN indicator = 0;

	// 2) read SQL data
	auto result = SQLGetData(_handle_stmt, static_cast<SQLUSMALLINT>(_ordinal), _target_type, buf_result.data<SQLPOINTER>(), INITIAL_BUFFER_SIZE, &indicator);

	// check)  failed?
	if (!SQL_SUCCEEDED(result))
	{
		// declare) 
		throwable::failure_ex e;

		// - add error log
		e.add_description("(failed) _ODBC_get_binary_data");

		// - get error records
		CGDK::query::rdb::odbc::_dump_error_records(e, SQL_HANDLE_STMT, _handle_stmt);

		// throw
		throw e;
	}
	
	// check) 
	if (indicator <= 0)
	{
		buf_result.clear();
		return buf_result;
	}

	// 3) buffer overflow? -> re-alloc buffer and retry
	if (indicator > INITIAL_BUFFER_SIZE)
	{
		shared_buffer buf_new = alloc_shared_buffer(indicator);
		buf_new.append(INITIAL_BUFFER_SIZE - 1, buf_result.data());
		buf_result = std::move(buf_new);

		// - SQLGetData
		result = SQLGetData(_handle_stmt, static_cast<SQLUSMALLINT>(_ordinal), _target_type, buf_result.get_back_ptr<SQLPOINTER>(), indicator - INITIAL_BUFFER_SIZE + 2, &indicator);

		// check)  failed?
		if (!SQL_SUCCEEDED(result) || indicator <= 0)
		{
			buf_result.clear();
			return buf_result;
		}
	}

	// 4) set size
	buf_result.add_size(indicator);

	// check)
	if (result == SQL_SUCCESS_WITH_INFO && CGDK::query::rdb::odbc::querier::is_enable_logging_warning_info())
	{
		// declare) 
		throwable::failure_ex e;

		// - add error log
		e.add_description("(warning) _ODBC_get_binary_data");

		// - get error records
		CGDK::query::rdb::odbc::_dump_error_records(e, SQL_HANDLE_STMT, _handle_stmt);

		// throw
		throw e;
	}

	// return)
	return buf_result;
}
