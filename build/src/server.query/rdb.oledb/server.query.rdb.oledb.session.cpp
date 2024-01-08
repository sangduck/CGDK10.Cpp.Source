#include "pch.h"

using namespace CGDK;


query::rdb::oledb::session::session() :
	m_count_requested(0)
{
	// declare)
	object_ptr<factory::object<session>> pfactory_object;

	// 1) Factory
	pfactory_object = factory::object<session>::instance(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	// statistics) 
	pfactory_object->statistics_on_alloc_create();

	// 2) assign
	m_factory_object = std::move(pfactory_object);
}
query::rdb::oledb::session::~session() noexcept
{
	// declare)
	object_ptr<factory::object<session>> pfactory_object;

	// 1) assign factory object temporary
	pfactory_object = std::move(this->m_factory_object);

	// statistics) 
	if(pfactory_object.exist())
	{
		pfactory_object->statistics_on_free_delete();
	}
}

void query::rdb::oledb::session::open(data_source* _pdata_source, DBPROPSET* _pprop_set, ULONG _ulPropSets)
{
	// check)
	CGASSERT_ERROR(_pdata_source != nullptr);

	// check)
	THROW_IF(_pdata_source == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) open
	auto result = ATL::CSession::Open(*_pdata_source, _pprop_set, _ulPropSets);

	// check)
	THROW_IF(FAILED(result), throwable::failure(eRESULT::FAIL));
}

void query::rdb::oledb::session::reopen(DBPROPSET* _pprop_set, ULONG _property_sets) noexcept
{
	// check)
	CGASSERT_ERROR(this->m_pdata_source.exist());

	// check) 
	RETURN_IF(this->m_pdata_source.empty());

	// 1) abort
	if (this->m_spOpenRowset != nullptr)
	{
		this->Abort();
	}

	// 2) close existing session
	this->Close();

	// 3) open new session
	try
	{
		this->open(m_pdata_source, _pprop_set, _property_sets);
	}
	catch (...)
	{
	}

	// 4) reset
	this->m_count_requested = 0;
}

result_code query::rdb::oledb::session::start_transaction() noexcept
{
	// check)
	CGASSERT_ERROR(this->m_pdata_source.exist());

	// check) 
	RETURN_IF(this->m_pdata_source.empty(), eRESULT::NOT_READY);

	// 1) start transaction
	this->StartTransaction();

	// return)
	return eRESULT::SUCCESS;
}
//void query::rdb::oledb::Iquery::begin_transaction(ISOLEVEL _isoLevel, ULONG isoFlags, ITransactionOptions* _pOtherOptions, ULONG* _pulTransactionLevel)
//{
//	// check) 
//	THROW_IF(this->m_psession.empty(), throwable::failure(eRESULT::FAIL));
//
//	// 1) Begin Transaction
//	auto result = this->m_psession->StartTransaction(_isoLevel, isoFlags, _pOtherOptions, _pulTransactionLevel);
//
//	// check) 
//	THROW_IF(result != S_OK, trace_error_records(this, result, L"[begin_transaction]"));
//}


result_code query::rdb::oledb::session::commit_transaction() noexcept
{
	// check)
	CGASSERT_ERROR(this->m_pdata_source.exist());

	// check) 
	RETURN_IF(this->m_pdata_source.empty(), eRESULT::NOT_READY);

	//// check) return is m_spOpenRowset is nullptr
	//RETURN_IF(m_spOpenRowset == nullptr)

	// 1) commit
	this->Commit();

	// return)
	return eRESULT::SUCCESS;
}
//result_code query::rdb::oledb::Iquery::abort_transaction(BOID* pboidReason, BOOL _is_retaining, BOOL bAsync) noexcept
//{
//	// check)
//	RETURN_IF(this->m_psession.empty(), eRESULT::NOT_READY);
//
//	// 1) abort
//	auto result = this->m_psession->Abort(pboidReason, _is_retaining, bAsync);
//
//	// return) 
//	return static_cast<eRESULT>(result);
//}

result_code query::rdb::oledb::session::abort_transaction() noexcept
{
	// check)
	CGASSERT_ERROR(this->m_pdata_source.exist());

	// check) 
	RETURN_IF(this->m_pdata_source.empty(), eRESULT::NOT_READY);

	//// check) return is m_spOpenRowset is nullptr
	//RETURN_IF(m_spOpenRowset == nullptr)

	// 1) abort
	this->Abort();

	// return)
	return eRESULT::SUCCESS;
}
//void query::rdb::oledb::Iquery::commit_transaction(BOOL _is_retaining, uint32_t _grfTC, uint32_t _grfRM)
//{
//	// check)
//	RETURN_IF(this->m_psession.empty());
//
//	// 1) commit
//	auto result = this->m_psession->Commit(_is_retaining, _grfTC, _grfRM);
//
//	// check) 
//	THROW_IF(result != S_OK, trace_error_records(this, result, L"[commit_transaction]"));
//}

//result_code query::rdb::oledb::Iquery::get_transaction_info(XACTTRANSINFO* _pInfo) const noexcept
//{
//	// check)
//	RETURN_IF(this->m_psession.empty(), eRESULT::FAIL);
//
//	auto result = this->m_psession->GetTransactionInfo(_pInfo);
//
//	// return) 
//	return static_cast<eRESULT>(result);
//}


void query::rdb::oledb::session::process_dispose() noexcept
{
	if(this->m_pquerier.exist())
	{
		this->m_pquerier->free_session(this);
	}
	else
	{
		delete this; 
	}
}





