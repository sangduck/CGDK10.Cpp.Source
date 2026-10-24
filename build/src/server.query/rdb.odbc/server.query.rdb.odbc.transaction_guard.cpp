#include "pch.h"

using namespace CGDK;


query::rdb::odbc::transaction_guard::transaction_guard(session* _psession) : m_psession(_psession)
{
	// declare)
	auto psession = this->m_psession;

	if (psession.empty())
		return;

	psession->start_transaction();
}

query::rdb::odbc::transaction_guard::~transaction_guard() noexcept 
{
	this->_abort();
}

void query::rdb::odbc::transaction_guard::_commit()
{
	// declare)
	object_ptr<session> psession = std::move(this->m_psession);

	// check)
	RETURN_IF(psession.empty(), );

	// 1) commit
	auto result = psession->commit_transaction();

	// check) 
	if (result != eRESULT::SUCCESS)
	{
		// declare) 
		throwable::failure_ex e;

		// - add error log
		e.add_description("(error) fail to 'commit_transaction'");

		// - get error records
		CGDK::query::rdb::odbc::_dump_error_records(e, SQL_HANDLE_DBC, psession->m_handle_dbc);

		// throw
		throw e;
	}
}

void query::rdb::odbc::transaction_guard::_abort() noexcept 
{
	// declare)
	object_ptr<session> psession = std::move(this->m_psession);

	// check)
	RETURN_IF(psession.empty(), );

	// 1) rollback
	auto result = psession->abort_transaction();
	
	// check) 
	if (result != eRESULT::SUCCESS)
	{
		// declare) 
		throwable::failure_ex e;

		// - add error log
		e.add_description("(error) _abort transaction");

		// - get error records
		CGDK::query::rdb::odbc::_dump_error_records(e, SQL_HANDLE_DBC, psession->m_handle_dbc);

		//// log) 
		//_log(nullptr, e.get_log_record());
	}
}
