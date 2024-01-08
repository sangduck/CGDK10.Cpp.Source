#include "pch.h"

namespace CGDK
{

throwable::failure_ex query::rdb::oledb::trace_error_records(Iquery* _pquery, HRESULT _result, const wstring_param _query_string) noexcept
{
	// check)
	CGASSERT_ERROR(FAILED(_result));

	// check) S_OK라면 그냥 끝낸다.
	RETURN_IF(FAILED(_result) == false, throwable::failure_ex(eRESULT::SUCCESS));

	// declare) 
	throwable::failure_ex e(eRESULT::FAIL, 0x00000000ffffffff);

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
	object_ptr<Imessageable> pmessageable = dynamic_pointer_cast<Imessageable>(pquerier);

	// 3) notify message
	if(pmessageable.exist())
	{
		// declare) 
		SERVER_QUERY_ERROR_INFO	temp_query_error_info;

		// - set error info
		temp_query_error_info.pquerier = pquerier;
		temp_query_error_info.pquery = _pquery;
		temp_query_error_info.result = _result;
		//temp_query_error_info.str_query = _query_string;
		temp_query_error_info.exception = e;

		// - notify
		pmessageable->request_process_message(sMESSAGE_DATA<SERVER_QUERY_ERROR_INFO>(eMESSAGE::SERVER::QUERY::FAILURE, temp_query_error_info));
	}

	// 4) get error records
	e = _dump_error_records(_result, _query_string);

	// return) 
	return e;
}

void query::rdb::oledb::regen_session(session* _psession, HRESULT _result)
{
	// check) 
	RETURN_IF(_psession == nullptr);

	// check) return if it occure general error
	RETURN_IF((_result & 0x00000000ffffffff) != 0x0000000080004005)

	// - session을 reopen한다.
	_psession->reopen();
}


}