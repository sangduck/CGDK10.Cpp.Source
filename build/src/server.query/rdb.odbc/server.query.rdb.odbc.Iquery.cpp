#include "pch.h"

using namespace CGDK;


void query::rdb::odbc::Iquery::process_set_session(session* _psession)
{
	// check)
	CGASSERT_FATAL(_psession != nullptr);

	// 1) alloc statement
	SQLHDBC handle_dbc = _psession->m_handle_dbc;

	// check) throw exception if it failed
	THROW_IF(handle_dbc == nullptr, throwable::failure(eRESULT::NOT_READY, 0, "(excp) ODBC dbc handle is not ready"sv));

	// declare) 
	SQLHSTMT handle_stmt = nullptr;

	// 1) alloc statement handle
	auto result_sql = ::SQLAllocHandle(SQL_HANDLE_STMT, handle_dbc, &handle_stmt);

	// check) throw exception if it failed
	THROW_IF(SQL_SUCCEEDED(result_sql) == false, _dump_error_records_dbc(handle_dbc));

	// 2) set session
	SCOPED_LOCK(this->m_lockable_state_query)
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

	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::READY);

		// check) 
		RETURN_IF(this->m_psession.empty());

		// - reset session
		psession = std::move(this->m_psession);

		// - statement
		handle_stmt = this->m_handle_stmt;
		this->m_handle_stmt = 0;

		// - set query state
		this->m_state_query = eOBJECT_STATE::NONE;
	}

	// 2) release statement
	if (handle_stmt != nullptr)
	{
		try
		{
			::SQLFreeHandle(SQL_HANDLE_STMT, handle_stmt);
		}
		catch(...)
		{
			// ignore any exception
		}
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

void query::rdb::odbc::Iquery::_process_regen_session()
{
	// ---------------------------------------------------------------------------------
	// 설명) session을 재생성한다.
	//       
	//   1) 기존 statement handle(stmt)를 stmt를 먼저 할당해제한다.
	//   2) session을 regent한다.
	//      - odc를 다시 할당한다. (재접속)
	//   3) statement handle(stmt)를 새로 할당한다.
	// 
	// ---------------------------------------------------------------------------------
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// 1) get session
		auto psession = this->m_psession;

		// check) 
		THROW_IF(psession.empty(), throwable::failure(eRESULT::NOT_READY, 0, "(excp) ODBC dbc handle is not ready"sv));

		// 2) release old statement
		if (this->m_handle_stmt != nullptr)
		{
			try
			{
 				::SQLFreeHandle(SQL_HANDLE_STMT, this->m_handle_stmt);
			}
			catch (throwable::failure_ex& _e)
			{
				_log(nullptr, _e.get_log_record());
			}
			catch (...)
			{
				// ignore any exception
			}

			this->m_handle_stmt = nullptr;
		}

		// 3) reopen session
		psession->reopen();

		// 4) alloc statement
		SQLHDBC handle_dbc = psession->m_handle_dbc;

		// check) throw exception if it failed
		THROW_IF(handle_dbc == nullptr, throwable::failure(eRESULT::NOT_READY, 0, "(excp) ODBC dbc handle is not ready"sv));

		// declare) 
		SQLHSTMT handle_stmt = nullptr;

		// 5) alloc new statement handle
		auto result_sql = ::SQLAllocHandle(SQL_HANDLE_STMT, handle_dbc, &handle_stmt);

		// check) throw exception if it failed
		THROW_IF(SQL_SUCCEEDED(result_sql) == false, _dump_error_records_dbc(handle_dbc));

		// 6) set new hstmt
		this->m_handle_stmt = handle_stmt;
	}
} 
