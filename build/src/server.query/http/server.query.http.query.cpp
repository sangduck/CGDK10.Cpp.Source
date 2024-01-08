#include "pch.h"

namespace CGDK
{


void query::http::query::execute_query(const std::string_view _command)
{
	scoped_lock_named(m_lockable_state_query, cs)
	{
		// 1) get querier
		auto pquerier = this->m_pquerier;

		// check)
		CGASSERT_ERROR(pquerier.exist());

		// check) start된 상태가 아니라면 안됀다.
		THROW_IF(pquerier->get_object_state() != eOBJECT_STATE::RUNNING && pquerier->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

		// 1) set query string
		this->set_request(_command);

		// statistics)
		pquerier->statistics_on_query_try();

		// check) check query status
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) check query status
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_ALLOWED));

		try
		{
			// 1) set state
			this->m_state_query = eOBJECT_STATE::REQUESTING;

			// 2) execute query
			this->request();

			// 3) wait complete
			this->m_cv_complete.wait(cs);
		}
		catch (...)
		{
			// statistics)
			pquerier->statistics_on_query_fail();

			// 1) set state
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
	}
}

void query::http::query::post_execute_query(const std::string_view _command)
{
	scoped_lock_named(this->m_lockable_state_query, cs)
	{
		// 1) get querier
		auto pquerier = m_pquerier;

		// check)
		CGASSERT_ERROR(pquerier.exist());

		// check) start된 상태가 아니라면 안됀다.
		THROW_IF(pquerier->get_object_state() != eOBJECT_STATE::RUNNING && pquerier->get_object_state() != eOBJECT_STATE::START_PENDING, throwable::failure(eRESULT::NOT_READY, 0, std::string_view("querier is not started")));

		// 1) set query string
		this->set_request(_command);

		// statistics)
		pquerier->statistics_on_query_try();

		// check) check query status
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) check query status
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_ALLOWED));

		try
		{
			// 1) set state
			this->m_state_query = eOBJECT_STATE::REQUESTING;

			// 2) execute query
			this->request();
		}
		catch (...)
		{
			// statistics)
			pquerier->statistics_on_query_fail();

			// - set state
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
	}
}

void query::http::query::close_query() noexcept
{
	scoped_lock(this->m_lockable_state_query)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::PROCESSED || this->m_state_query == eOBJECT_STATE::REQUESTING);

		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::PROCESSED && this->m_state_query != eOBJECT_STATE::REQUESTING);

		// 2) reset result
		this->m_result.reset();

		// 3) set state
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::http::query::porcess_set_querier(querier* _pquerier)
{
	scoped_lock(this->m_lockable_state_query)
	{
		// 1) set querier
		this->m_pquerier = _pquerier;
			
		// 2) set querier
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::http::query::process_reset_session() noexcept
{
	scoped_lock(this->m_lockable_state_query)
	{
		// 1) reset result
		this->m_result.reset();
	
		// 2) reset querier
		this->m_pquerier.reset();

		// 3) set querier
		this->m_state_query = eOBJECT_STATE::NONE;
	}
}

int query::http::query::process_response(sMESSAGE& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.message == eMESSAGE::SYSTEM::HTTP_RESPONSE);

	// check) 
	CGASSERT_ERROR(m_state_query == eOBJECT_STATE::REQUESTING);

	// -MESSAGE를 casting한다.
	auto& msg_http = static_cast<sMESSAGE_HTTP_RESPONSE&>(_msg);

	// 1) 설정
	this->m_buffer_response = msg_http.buf_message;
	this->m_result = msg_http;

	if (msg_http.is_completed)
	{
		// 2) Function을 호출한다.
		if (this->m_function_on_response)
		{
			this->m_function_on_response(msg_http);
		}

		scoped_lock(this->m_lockable_state_query)
		{
			// - set state
			this->m_state_query = eOBJECT_STATE::PROCESSED;

			// - set notify one
			this->m_cv_complete.notify_one();
		}
	}
	else
	{
		// 2) ...
		this->m_function_on_process(msg_http);
	}

	// return)  
	return 1;
}


}