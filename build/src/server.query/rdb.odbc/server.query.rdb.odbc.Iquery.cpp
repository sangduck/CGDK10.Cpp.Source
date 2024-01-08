#include "pch.h"

using namespace CGDK;


void query::rdb::odbc::Iquery::process_set_session(session* _psession)
{
	// check)
	CGASSERT_ERROR(_psession != nullptr);

	// 1) alloc statement
	SQLHDBC handle_dbc = _psession->m_handle_dbc;

	// check) throw exception if it failed
	THROW_IF(handle_dbc == nullptr, throwable::failure(eRESULT::NOT_READY, 0, "(excepp) ODBC dbc handle is not ready"sv));

	// declare) 
	SQLHSTMT handle_stmt = nullptr;

	// 1) alloc statement handle
	auto result_sql = ::SQLAllocHandle(SQL_HANDLE_STMT, handle_dbc, &handle_stmt);

	// check) throw exception if it failed
	THROW_IF(SQL_SUCCEEDED(result_sql) == false, _dump_error_records_dbc(handle_dbc));

	// 2) set session
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_psession.empty());

		// check) 
		CGASSERT_ERROR(this->m_handle_stmt == 0);

		// 1) set
		this->m_psession = _psession;
		this->m_handle_stmt = handle_stmt;
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::rdb::odbc::Iquery::process_reset_session() noexcept
{
	// declare)
	object_ptr<session>	psession;
	SQLHSTMT handle_stmt = nullptr;

	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::READY);

		// check) 
		RETURN_IF(this->m_psession.empty());

		// - reset session
		psession = std::move(this->m_psession);

		// - statment
		handle_stmt = this->m_handle_stmt;
		this->m_handle_stmt = 0;

		// - set query state
		this->m_state_query = eOBJECT_STATE::NONE;
	}

	// 2) release statement
	if (handle_stmt != nullptr)
	{
		::SQLFreeHandle(SQL_HANDLE_STMT, handle_stmt);
	}

	// 3) notify to requestor
	this->_process_notify_to_requestor();
}

void query::rdb::odbc::Iquery::_process_notify_to_requestor() noexcept
{
	// check) requestor가 없으면 끝낸다.
	RETURN_IF(this->m_prequestor.empty())

	try
	{
		this->m_prequestor->process_notify_complete_query(this);
	}
	catch (...)
	{
	}

	// 1) ProcessCompleteQuery를 호출한다.
	this->m_prequestor.reset();
}

void query::rdb::odbc::Iquery::_process_regenerate_session()
{
	// 1) 기존 session을 얻는다.
	auto psession_old = this->get_session();

	// check) 기존 session을 닫는다.
	THROW_IF(psession_old.empty(), throwable::failure(eRESULT::NOT_READY, 0, std::string_view("excpt) session is nullptr. ")));

	// 2) 기존 session을 닫는다.
	psession_old->close();

	// 3) 기존 session의 정보를 얻어 놓는다.
	auto pdata_source = psession_old->m_pdata_source;
	auto pquerier = psession_old->m_pquerier;

	// 4) session에 정보를 제거한다.
	psession_old->m_pdata_source.reset();
	psession_old->m_pquerier.reset();

	// 5) 새로운 session을 생성한다.
	auto psession = new session();

	// 6) 새로운 session을 open한다.
	psession->open(pdata_source);

	// 7) Querier를 설정한다.
	psession->m_pquerier = pquerier;

	// 8) 새로운 session을 세팅한다.
	this->set_session(psession);
}