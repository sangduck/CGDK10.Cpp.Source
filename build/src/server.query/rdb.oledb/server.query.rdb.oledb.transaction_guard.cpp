#include "pch.h"

using namespace CGDK;


query::rdb::oledb::transaction_guard::transaction_guard(session* _psession) : m_psession(_psession)
{
	if (this->m_psession.empty())
		return;

	this->m_psession->start_transaction();
}

query::rdb::oledb::transaction_guard::~transaction_guard() noexcept
{
	this->_abort();
}

void query::rdb::oledb::transaction_guard::_commit()
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
		e.add_description("(error) _commit transaction");

		// - get error records
		CGDK::query::rdb::oledb::_dump_error_records(e);

		// throw
		throw e;
	}
}

void query::rdb::oledb::transaction_guard::_abort() noexcept
{
	// declare)
	object_ptr<session> psession = std::move(this->m_psession);

	// check)
	RETURN_IF(psession.empty(), );

	// 1) rollback
	auto result = this->m_psession->abort_transaction();

	// check) 
	if (result != eRESULT::SUCCESS)
	{
		// declare) 
		throwable::failure_ex e;

		// - add error log
		e.add_description("(error) _abort transaction");

		// - get error records
		CGDK::query::rdb::oledb::_dump_error_records(e);

		//// log) 
		//_log(nullptr, e.get_log_record());
	}
}
