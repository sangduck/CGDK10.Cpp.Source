#include "pch.h"

using namespace CGDK;


inline int64_t	scale_pow(int _scale)
{
	switch (_scale)
	{
	case	0:	return	1LL;
	case	1:	return	10LL;
	case	2:	return	100LL;
	case	3:	return	1000LL;
	case	4:	return	10000LL;
	case	5:	return	100000LL;
	case	6:	return	1000000LL;
	case	7:	return	10000000LL;
	case	8:	return	100000000LL;
	case	9:	return	1000000000LL;
	case	10:	return	10000000000LL;
	case	11:	return	100000000000LL;
	case	12:	return	1000000000000LL;
	case	13:	return	10000000000000LL;
	case	14:	return	100000000000000LL;
	case	15:	return	1000000000000000LL;
	case	16:	return	10000000000000000LL;
	case	17:	return	100000000000000000LL;
	case	18:	return	1000000000000000000LL;
	default:	CGASSERT_ERROR(false);	throw throwable::failure(eRESULT::OUT_OF_RANGE);
	}
}

NUMERIC CGDK::__to_numeric(const SQL_NUMERIC_STRUCT& _decimal) noexcept
{
	NUMERIC	dest;

	int64_t	pow	 = scale_pow(_decimal.scale);

	int64_t	value = *(uint64_t*)(_decimal.val + 8);
	int64_t	a	  = ((value / pow) << 32);
	int64_t	f	  = ((value % pow) << 32) + *(uint32_t*)(_decimal.val + 4);

	int64_t	a1	  = (a + (f / pow)) << 32;
	int64_t f1	  = ((f % pow) << 32) + *(uint32_t*)_decimal.val;

	dest.value	  = a1 + (f1 / pow);
	dest.fraction = f1 % pow;

	if(_decimal.sign==0)
	{
		dest.value = -dest.value;
		dest.fraction = -dest.fraction;
	}

	// return)
	return dest;
}

SQL_NUMERIC_STRUCT CGDK::__to_sql_numeric_struct(const NUMERIC& /*_numeric*/) noexcept
{
	// return)
	return SQL_NUMERIC_STRUCT();
}

throwable::failure_ex CGDK::query::rdb::odbc::_dump_error_records_env(SQLHENV _handle_env) noexcept
{
	//declare) 
	throwable::failure_ex e;

	// 1) get dump record
	_dump_error_records(e, SQL_HANDLE_ENV, _handle_env);

	//return)
	return e;
}

throwable::failure_ex CGDK::query::rdb::odbc::_dump_error_records_dbc(SQLHDBC _handle_dbc) noexcept
{
	//declare) 
	throwable::failure_ex e;

	// 1) get dump record
	_dump_error_records(e, SQL_HANDLE_DBC, _handle_dbc);

	//return)
	return e;
}

void CGDK::query::rdb::odbc::_dump_error_records_stmt(throwable::failure_ex& _e, SQLHSTMT _handle_stmt) noexcept
{
	// 1) get dump record
	_dump_error_records(_e, SQL_HANDLE_STMT, _handle_stmt);
}

std::string_view to_string_handle_type(SQLSMALLINT _handle_type)
{
	// declare) 
	std::string_view str_temp;

	switch (_handle_type)
	{
	case	SQL_HANDLE_ENV:		str_temp = "SQL_HANDLE_ENV"; break;
	case	SQL_HANDLE_DBC:		str_temp = "SQL_HANDLE_DBC"; break;
	case	SQL_HANDLE_STMT:	str_temp = "SQL_HANDLE_STMT"; break;
	case	SQL_HANDLE_DESC:	str_temp = "SQL_HANDLE_DESC"; break;
	default:					str_temp = ""; break;
	}

	// return)
	return	str_temp;
}

void CGDK::query::rdb::odbc::_dump_error_records(throwable::failure_ex& _e, SQLSMALLINT _handle_type, SQLHANDLE _handle) noexcept
{
	// check)
	RETURN_IF(_handle == 0);

	_e.m_reason = eRESULT::FAIL;
	_e.m_description = std::format("query::rdb::odbc: fail to execute [{} {:#x}]", to_string_handle_type(_handle_type), reinterpret_cast<uint64_t>(_handle));

	for(SQLSMALLINT i = 1; ; ++i)
    {
		// declare) 
		SQLINTEGER native;
		SQLCHAR state[7];
		auto buf_text = alloc_shared_buffer(8192);
		SQLSMALLINT	size_;

		// - Error log를 얻는다.
		auto ret = SQLGetDiagRec(_handle_type, _handle, i, state, &native, buf_text.data<SQLCHAR>(), 8192, &size_);

		// check) 실패일 경우 루프를 탈출한다.
		BREAK_IF (SQL_SUCCEEDED(ret) == false);

		// - detect connection error (08XXX is connection failure)
		if (state[0] == '0' && state[1] == '8')
		{
			_e.m_reason = eRESULT::CONNECTION_FAILURE;
		}

		if (state[0] == '0' && state[1] == '0')
		{
			_e.m_reason = eRESULT::CONNECTION_FAILURE;
		}

		if (ret == 2003)
		{
			_e.m_reason = eRESULT::CONNECTION_FAILURE;
		}

		// - log를 출력한다.
		_e.add_description(std::format("- row(state:{} native:{:#06x}) {}", reinterpret_cast<char*>(state), native, buf_text.data<const char>()));
    }
}

throwable::failure_ex CGDK::query::rdb::odbc::_dump_error_records(SQLRETURN _result, const std::string_view _command, SQLHANDLE _handle_stmt) noexcept
{
	//declare) 
	throwable::failure_ex e(eRESULT::FAIL, _result);

	// 1) 
	e.m_description = std::format("query::rdb::odbc: query failure. [result: {:#x}][{} {:#x}]", _result, to_string_handle_type(SQL_HANDLE_STMT), reinterpret_cast<uint64_t>(_handle_stmt));

	// 2) trace 'command' string
	if(_command.empty() == false)
	{
		e.add_description(CGDK::stream_string("- command string: '", _command, "'"));
	}

	// 3) get odbc error records 
	_dump_error_records_stmt(e, _handle_stmt);

	// return)
	return e;
}

SQL_TIMESTAMP_STRUCT CGDK::__to_sql_TIMESTAMP_struct(const chrono::time::time_point& _time) noexcept
{
	// declare) 
	SQL_TIMESTAMP_STRUCT temp;

	auto temp_dt = chrono::to_date_time(_time);

	temp.year		 = temp_dt.t_year;
	temp.month		 = temp_dt.t_month;
	temp.day		 = temp_dt.t_day;
	temp.hour		 = temp_dt.t_hour;
	temp.minute		 = temp_dt.t_minute;
	temp.second		 = temp_dt.t_second;
	temp.fraction	 = temp_dt.t_fraction;

	// return)
	return temp;
}

SQL_TIMESTAMP_STRUCT CGDK::__to_sql_TIMESTAMP_struct(const chrono::time::utc::time_point& _time) noexcept
{
	return __to_sql_TIMESTAMP_struct((const chrono::time::time_point&)_time);
}

SQL_TIMESTAMP_STRUCT CGDK::__to_sql_TIMESTAMP_struct(const chrono::time::local::time_point& _time) noexcept
{
	return __to_sql_TIMESTAMP_struct((const chrono::time::time_point&)_time);
}

throwable::failure_ex CGDK::query::rdb::odbc::trace_error_records(Iquery* _pquery, SQLRETURN _result, const std::string_view _str_query_string)
{
	// declare) 
	throwable::failure_ex e(eRESULT::FAIL, _result);

	// check) S_OK라면 그냥 끝낸다.
	RETURN_IF(_result == 0, e);

	// check)
	CGASSERT_ERROR(_pquery != nullptr);

	// check) return if _pquery is nullptr
	RETURN_IF(_pquery == nullptr, e);

	// check) _pquery->m_psession must not be nullptr
	CGASSERT_ERROR(_pquery->m_psession.exist());

	// check) return if _pquery->m_psession is nullptr
	RETURN_IF(_pquery->m_psession.empty(), e);

	// 1) get querier
	auto pquerier = _pquery->m_psession->m_pquerier;

	// check) _pquery->m_psession->m_pquerier must not be nullptr
	CGASSERT_ERROR(pquerier.exist());

	// check) return if _pquery->m_psession->m_pquerier is nullptr
	RETURN_IF(pquerier.empty(), e);

	// 2) get dispatchable interface 
	auto pmessageable = dynamic_pointer_cast<Imessageable>(pquerier);

	// 3) notify message
	if(pmessageable.exist())
	{
		// declare) 
		SERVER_QUERY_ERROR_INFO	temp_query_error_info;

		// - set error info
		temp_query_error_info.pquerier = pquerier;
		temp_query_error_info.pquery = _pquery;
		temp_query_error_info.result = _result;
		//temp_query_error_info.str_query = _str_query_string;
		temp_query_error_info.exception = e;

		// - notify
		pmessageable->request_process_message(sMESSAGE_DATA<SERVER_QUERY_ERROR_INFO>(eMESSAGE::SERVER::QUERY::FAILURE, temp_query_error_info));
	}

	// 4) get error records
	e = _dump_error_records(_result, _str_query_string, _pquery->m_handle_stmt);

	// return)
	return e;
}
