#include "pch.h"

using namespace CGDK;


void query::rdb::oledb::Iquery::process_set_session(session* _psession)
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_psession.empty());

		// 1) set
		this->m_psession = _psession;

		// 2) set query state
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::rdb::oledb::Iquery::process_reset_session() noexcept
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::READY);

		// check) 
		RETURN_IF(this->m_psession.empty());

		// 1) reset session
		this->m_psession.reset();

		// 2) set query state
		this->m_state_query = eOBJECT_STATE::NONE;
	}

	// 2) notify to requestor
	this->_process_notify_to_requestor();
}

void query::rdb::oledb::Iquery::_process_notify_to_requestor() noexcept
{
	// check) requestor가 없으면 끝낸다.
	RETURN_IF(this->m_prequestor.empty())

	// 1) ProcessCompleteQuery를 호출한다.
	try
	{
		this->m_prequestor->process_notify_complete_query(this);
	}
	catch (...)
	{
	}

	// 2) reset requestor
	this->m_prequestor.reset();
}

void query::rdb::oledb::Iquery::_process_regenerate_session()
{
	// 1) 기존 session을 얻는다.
	auto psession_old = get_session();

	// check) 기존 session을 닫는다.
	THROW_IF(psession_old.empty(), throwable::failure(eRESULT::NOT_READY, 0, std::string_view("exception) session is nullptr. ")));

	// 2) 기존 session을 닫는다.
	psession_old->Close();

	// 3) 새로운 session을 생성한다.
	auto psession = new session();

	try
	{
		// 4) 새로운 session을 open한다.
		psession->open(psession_old->m_pdata_source);

		// 5) Querier를 설정한다.
		psession->m_pdata_source = std::move(psession_old->m_pdata_source);
		psession->m_pquerier = std::move(psession_old->m_pquerier);

		// 6) 새로운 session을 세팅한다.
		this->set_session(psession);
	}
	catch (...)
	{
		// - ...
		delete psession;

		// reraise) 
		throw;
	}
}
