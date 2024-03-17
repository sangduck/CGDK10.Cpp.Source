#include "pch.h"

using namespace CGDK;


CGDK::query::rdb::odbc::Nquery::Nquery() noexcept
{
}

CGDK::query::rdb::odbc::Nquery::~Nquery() noexcept
{
	this->close_query();
}

void CGDK::query::rdb::odbc::Nquery::prepare_query(const std::string_view _command)
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) must be preapre
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// declare) 
		size_t retry = 0;
		auto pquerier = this->m_psession->m_pquerier;

		// check) pquerier must not be nullptr
		CGASSERT_ERROR(pquerier != nullptr);

		// check) throw exception if pquerier is nullptr
		THROW_IF(pquerier == nullptr, throwable::failure(eRESULT::NOT_READY));

		// 1) get stmt handle
		auto handle_stmt = this->m_handle_stmt;

		// check) throw exception if handle_stmt is 0
		THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

		// statistics) 
		pquerier->statistics_on_query_try();

		// 1) prepare query
		auto result = this->_prepare_query(_command, handle_stmt);

		if (SQL_SUCCEEDED(result) == false)
		{
			// - get error
			auto e = trace_error_records(this, result, _command);

			for(;;)
			{
				// - get max retry count
				auto max_retry = this->m_psession->m_pquerier->get_max_retry_on_fail();

				// check) throw if general query error or retry count out
				if (retry >= max_retry || e.reason() != eRESULT::CONNECTION_FAILURE)
				{
					// - trace retry count
					if (retry > 0)
					{
						e.add_description(NFMT::format("- {} failed and re-tries", retry));
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

				// - regen session
				regen_session(this->m_psession);

				// - get new stmt handle
				handle_stmt = this->m_handle_stmt;

				// check) throw exception if handle_stmt is 0
				THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

				// - retry query
				result = this->_prepare_query(_command, handle_stmt);

				// check) break if success!!
				BREAK_IF(SQL_SUCCEEDED(result));

				// - get new result
				{
					// declare) 
					throwable::failure_ex e_nest;

					// - get stmt error
					_dump_error_records(e_nest, SQL_HANDLE_STMT, handle_stmt);

					// - get reason
					auto reason_new = e_nest.reason();

					// - update reason
					e.m_reason = reason_new;

					// - add odbc error records
					if (reason_new != eRESULT::CONNECTION_FAILURE)
					{
						// - copy error result
						for (auto& iter_v: e_nest.m_description_ex)
						{
							e.add_description(iter_v);
						}
					}
				}
			}
		}

		// declare) 
		SQLSMALLINT num_params = 0;

		// 3) get params count
		::SQLNumParams(handle_stmt, &num_params);

		// 4) get parameter infos
		if (num_params != 0)
		{
			// check)
			CGASSERT_ERROR(this->m_vector_params.empty());

			// - reserve
			this->m_vector_params.reserve(num_params + 1);

			// - index 0
			this->m_vector_params.emplace_back();

			// - index 1 ~ num_params
			for (SQLSMALLINT i = 1; i <= num_params; ++i)
			{
				// - add new..
				auto& iter = this->m_vector_params.emplace_back();

				// - get params info
				auto result_nest = ::SQLDescribeParam(handle_stmt, i, &iter.data_type, &iter.param_size, &iter.decimal_digits, &iter.nullable);

				// check)
				if (SQL_SUCCEEDED(result_nest) == false)
				{
					throw trace_error_records(this, result_nest, _command);
				}

				// - SQL_VCHAR이면 strLen_or_indptr을 SQL_NTS으로 설정한다.
				if (iter.data_type == SQL_VARCHAR)
				{
					iter.strLen_or_indptr = SQL_NTS;
				}
			}
		}

		// 5) set state query
		this->m_state_query = eOBJECT_STATE::REQUESTING;
	}
}

SQLRETURN CGDK::query::rdb::odbc::Nquery::_prepare_query(std::string_view _command, SQLHSTMT _handle_stmt)
{
	return ::SQLPrepareA(_handle_stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(_command.data())), static_cast<SQLINTEGER>(_command.size() + 1));
}

SQLRETURN CGDK::query::rdb::odbc::Nquery::_prepare_query(std::wstring_view _command, SQLHSTMT _handle_stmt)
{
	return ::SQLPrepareW(_handle_stmt, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(_command.data())), static_cast<SQLINTEGER>(_command.size() + 1));
}


void CGDK::query::rdb::odbc::Nquery::execute_query(const std::string_view _command)
{
	// 1) query string을 설정한다.
	this->set_query_string(_command);

	// 2) query를 실행한다.
	this->_execute_query(this->m_query_string);
}

void CGDK::query::rdb::odbc::Nquery::execute_query()
{
	this->_execute_query();
}

result_code CGDK::query::rdb::odbc::Nquery::move_first()
{
	// 1) get hstmt
	SQLHSTMT handle_stmt = this->m_handle_stmt;

	// check)
	RETURN_IF(handle_stmt == 0, eRESULT::NOT_READY);

	// 2) Move Next
	auto result = ::SQLFetch(handle_stmt);

	// check) throw if occure error
	if (result == SQL_ERROR)
	{
		// raise)
		throw trace_error_records(this, result, std::string_view());
	}

	// check)
	RETURN_IF(result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO, eRESULT::FAIL);

	// return) 성공
	return eRESULT::SUCCESS;
}

result_code CGDK::query::rdb::odbc::Nquery::move_next() noexcept
{
	// 1) get hstmt
	SQLHSTMT handle_stmt = this->m_handle_stmt;

	// check)
	RETURN_IF(handle_stmt == 0, eRESULT::NOT_READY);

	// 2 Move Next
	auto result = ::SQLFetch(handle_stmt);

	// check)
	RETURN_IF(result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO, eRESULT::FAIL);

	// return) 성공
	return eRESULT::SUCCESS;
}

result_code CGDK::query::rdb::odbc::Nquery::get_next_result(bool /*_bind*/) noexcept
{
	// 1) get hstmt
	SQLHSTMT handle_stmt = this->m_handle_stmt;

	// check) 
	RETURN_IF(handle_stmt == 0, eRESULT::NOT_READY);

	// 2) get next result
	auto result = ::SQLMoreResults(handle_stmt);

	// check) return if failed
	RETURN_IF(SQL_SUCCEEDED(result) == false, eRESULT::FAIL);

	// 3) get affected row count
	SQLSMALLINT	length = sizeof(this->m_row_affected);
	SQLGetInfo(handle_stmt, SQL_BATCH_ROW_COUNT, &this->m_row_affected, sizeof(int), &length);

	// return)
	return eRESULT::SUCCESS;
}

void CGDK::query::rdb::odbc::Nquery::set_param(std::size_t _ordinal, SQLSMALLINT _inputoutput_type, SQLSMALLINT _value_type, SQLSMALLINT _parameter_type, SQLULEN _column_size, SQLSMALLINT _decimal_digits, SQLPOINTER _parameter_value_ptr, SQLLEN _buffer_length, SQLLEN* _strlen_or_int_ptr)
{
	// check)
	THROW_IF(_ordinal >= this->m_vector_params.size(), throwable::failure_ex(eRESULT::OUT_OF_RANGE, 0, std::string_view("(error) parameter index is out of range")));

	// 1) call bind
	auto result = ::SQLBindParameter(this->m_handle_stmt, static_cast<SQLUSMALLINT>(_ordinal), _inputoutput_type, _value_type, _parameter_type, _column_size, _decimal_digits, _parameter_value_ptr, _buffer_length, _strlen_or_int_ptr);

	// check) ...
	if (SQL_SUCCEEDED(result) == false)
	{
		// raise)
		throw trace_error_records(this, result, std::string_view());
	}
}

std::size_t CGDK::query::rdb::odbc::Nquery::get_ordinal(const std::string_view _column)
{
	// Assert) 
	CGASSERT_ERROR(false);

	// 1) get column name
	if (this->m_map_column.empty())
	{
		// declare) 
		SQLSMALLINT count = 0;

		// - get column count
		::SQLNumResultCols(this->m_handle_stmt, &count);

		for (SQLSMALLINT i = 0; i < count; ++i)
		{
			// declare) 
			SQLCHAR column_name[64];
			SQLSMALLINT column_name_length = 0;

			// - ...
			::SQLDescribeColA(this->m_handle_stmt, i, column_name, 64, &column_name_length, nullptr, nullptr, nullptr, nullptr);

			// check)
			static_assert(sizeof(SQLCHAR) == 1, "support only SQLCHAR is 1 or 2");

			// - insert
			this->m_map_column.insert({ std::string{ reinterpret_cast<char*>(column_name), static_cast<std::size_t>(column_name_length)}, i + 1 });
		}
	}

	// 2)  find column
	auto iter_find = this->m_map_column.find(_column.data());

	// check)
	RETURN_IF(iter_find == this->m_map_column.end(), 0);
	
	// return) 
	return iter_find->second;
}


void CGDK::query::rdb::odbc::Nquery::open_query()
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check)
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) must be preapre
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// declare) 
		auto pquerier = this->m_psession->m_pquerier;

		// check) pquerier must not be nullptr
		CGASSERT_ERROR(pquerier != nullptr);

		// check) throw exception if pquerier is nullptr
		THROW_IF(pquerier == nullptr, throwable::failure(eRESULT::NOT_READY));

		// 1) get stmt handle
		auto handle_stmt = this->m_handle_stmt;

		// check) m_handle_stmt must not be zero. must execute prepare() first
		CGASSERT_ERROR(handle_stmt != 0)

		// check) throw exception if handle_stmt is 0
		THROW_IF(handle_stmt == 0, throwable::failure(eRESULT::NOT_READY));

		// 3) open_query
		auto result = ::SQLExecute(handle_stmt);

		// check) 
		if (SQL_SUCCEEDED(result) == false)
		{
			// statistics_  
			pquerier->statistics_on_query_fail();

			// raise)
			throw trace_error_records(this, result, std::string_view());
		}

		// 4) set state
		this->m_state_query = eOBJECT_STATE::PROCESSED;

		// statistics)
		pquerier->statistics_on_query_success();
	}
}

void CGDK::query::rdb::odbc::Nquery::close_query() noexcept
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::PROCESSED && this->m_state_query != eOBJECT_STATE::REQUESTING);

		// check) 
		RETURN_IF(this->m_handle_stmt == 0);

		// 1) release allocated handles
		::SQLFreeStmt(this->m_handle_stmt, SQL_CLOSE);
		::SQLFreeStmt(this->m_handle_stmt, SQL_UNBIND);
		::SQLFreeStmt(this->m_handle_stmt, SQL_RESET_PARAMS);

		// 3) set state as READY
		this->m_state_query = eOBJECT_STATE::READY;

		// 4) params
		this->m_vector_params.clear();
		this->m_map_column.clear();
	}
}

void CGDK::query::rdb::odbc::Nquery::process_execute_query()
{
	this->execute_query(this->get_query_string());
}

void CGDK::query::rdb::odbc::Nquery::process_reset_session() noexcept
{
	// 1) close query 
	this->close_query();

	// 2) call 'Iquery::process_reset_session'
	this->Iquery::process_reset_session();

}

void CGDK::query::rdb::odbc::Nquery::_execute_query(std::string_view _command)
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) must be preapre
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// check) 
		CGASSERT_ERROR(this->m_psession.exist());

		// check) 
		THROW_IF(this->m_psession.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['m_psession'] is nullptr")));

		// declare) 
		size_t retry = 0;
		auto pquerier = this->m_psession->m_pquerier;

		// check) 
		CGASSERT_ERROR(pquerier != nullptr);

		// check) 
		THROW_IF(pquerier == nullptr, throwable::failure(eRESULT::NOT_READY));

		// 1) get stmt handle
		auto handle_stmt = this->m_handle_stmt;

		// check) 
		THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

		// statistics) 
		pquerier->statistics_on_query_try();

		// 2) execute query
		auto result = ::SQLExecDirectA(handle_stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(_command.data())), SQL_NTS);

		if (SQL_SUCCEEDED(result) == false)
		{
			// - get error
			auto e = trace_error_records(this, result, _command);

			for(;;)
			{
				// - get max retry count
				auto max_retry = this->m_psession->m_pquerier->get_max_retry_on_fail();

				// check) throw if general query error or retry count out
				if (retry >= max_retry || e.reason() != eRESULT::CONNECTION_FAILURE)
				{
					// - trace retry count
					if (retry > 0)
					{
						e.add_description(NFMT::format("- {} failed and re-tries", retry));
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

				// - regen session
				regen_session(this->m_psession);

				// - get new stmt handle
				handle_stmt = this->m_handle_stmt;

				// check) throw exception if handle_stmt is 0
				THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

				// - retry query
				result = ::SQLExecDirectA(handle_stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(_command.data())), SQL_NTS);

				// check) break if success!!
				BREAK_IF(SQL_SUCCEEDED(result));

				// - get new result
				{
					// declare) 
					throwable::failure_ex e_nest;

					// - get stmt error
					_dump_error_records(e_nest, SQL_HANDLE_STMT, handle_stmt);

					// - get reason
					auto reason_new = e_nest.reason();

					// - update reason
					e.m_reason = reason_new;

					// - add odbc error records
					if (reason_new != eRESULT::CONNECTION_FAILURE)
					{
						// - copy error result
						for (auto& iter_v: e_nest.m_description_ex)
						{
							e.add_description(iter_v);
						}
					}
				}
			}
		}

		// 3) set state query
		this->m_state_query = eOBJECT_STATE::PROCESSED;

		// statistics) 
		pquerier->statistics_on_query_success();
	}
}

void CGDK::query::rdb::odbc::Nquery::_execute_query()
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::REQUESTING);

		// check) must be preapre
		THROW_IF(this->m_state_query != eOBJECT_STATE::REQUESTING, throwable::failure(eRESULT::NOT_READY));

		// check) m_psession must not be nullptr
		CGASSERT_ERROR(this->m_psession.exist());

		// check) throw exception if m_psession is nullptr
		THROW_IF(this->m_psession.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter['m_psession'] is nullptr")));

		// declare) 
		size_t retry = 0;
		auto pquerier = this->m_psession->m_pquerier;

		// check) pquerier must not be nullptr
		CGASSERT_ERROR(pquerier != nullptr);

		// check) throw exception if pquerier is nullptr
		THROW_IF(pquerier == nullptr, throwable::failure(eRESULT::NOT_READY));

		// 1) get stmt handle
		SQLHSTMT handle_stmt = this->m_handle_stmt;

		// check) handle_stmt가 0이면 안됀다.(이미 prepare되어 있을 것이므로)
		CGASSERT_ERROR(handle_stmt != nullptr)

		// check) throw exception if handle_stmt is 0
		THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

		// statistics) 
		pquerier->statistics_on_query_try();

		// 2) execute query
		auto result = ::SQLExecute(handle_stmt);

		if (SQL_SUCCEEDED(result) == false)
		{
			// - get error
			auto e = trace_error_records(this, result, this->m_query_string);

			for(;;)
			{
				// - get max retry count
				auto max_retry = this->m_psession->m_pquerier->get_max_retry_on_fail();

				// check) throw if general query error or retry count out
				if (retry >= max_retry || e.reason() != eRESULT::CONNECTION_FAILURE)
				{
					// - trace retry count
					if (retry > 0)
					{
						e.add_description(NFMT::format("- {} failed and re-tries", retry));
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

				// - regen session
				regen_session(this->m_psession);

				// - get new stmt handle
				handle_stmt = this->m_handle_stmt;

				// check) throw exception if handle_stmt is 0
				THROW_IF(handle_stmt == nullptr, throwable::failure(eRESULT::NOT_READY));

				// - retry query
				result = ::SQLExecute(handle_stmt);

				// check) break if success!!
				BREAK_IF(SQL_SUCCEEDED(result));

				// - get new result
				{
					// declare) 
					throwable::failure_ex e_nest;

					// - get stmt error
					_dump_error_records(e_nest, SQL_HANDLE_STMT, handle_stmt);

					// - get reason
					auto reason_new = e_nest.reason();

					// - update reason
					e.m_reason = reason_new;

					// - add odbc error records
					if (reason_new != eRESULT::CONNECTION_FAILURE)
					{
						// - copy error result
						for (auto& iter_v: e_nest.m_description_ex)
						{
							e.add_description(iter_v);
						}
					}
				}
			}
		}

		// 3) state rollback
		this->m_state_query = eOBJECT_STATE::PROCESSED;

		// statistics) 
		pquerier->statistics_on_query_success();
	}
}
