#include "pch.h"

using namespace CGDK;


query::redis::Iquery::~Iquery()			
{
}

void query::redis::Iquery::wait()
{
	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check)
		RETURN_IF(this->m_state_query != eOBJECT_STATE::REQUESTING,);

		// 1) wait
		this->m_cv_complete.wait(cs);
	}
}

void query::redis::Iquery::process_set_session(session* _psession)
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) 같은 session이면 여기서 끝낸다.
		RETURN_IF(this->m_psession == _psession);

		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::NONE);

		// 1) 기존 session이 있으면 그걸 먼저 해제해준다.
		if (this->m_psession.exist())
			this->m_psession->release_query_reference();

		// 2) 새로운 session을 설정한다.
		this->m_psession = _psession;

		// 3) set state
		this->m_state_query = eOBJECT_STATE::READY;

		// 4) 새로 설정된 SEssion의 QueryReference 카운트를 증가시킨다.
		if (_psession != nullptr)
		{
			_psession->add_query_reference();
		}
	}
}

void query::redis::Iquery::close_query() noexcept
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) 
		RETURN_IF(this->m_state_query != eOBJECT_STATE::PROCESSED && this->m_state_query != eOBJECT_STATE::REQUESTING);

		// 1) reset
		this->m_buffer_query.clear();
		this->m_result.reset();

		// 2) set 
		this->m_state_query = eOBJECT_STATE::READY;
	}
}

void query::redis::Iquery::process_response()
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check)
		RETURN_IF(this->m_state_query!= eOBJECT_STATE::REQUESTING);

		// 2) set state
		this->m_state_query = eOBJECT_STATE::PROCESSED;

		// 3) requesting
		this->m_cv_complete.notify_one();
	}

	// - coroutine) resume the coroutine awaiting this request
	this->_process_on_complete();
}

void query::redis::Iquery::process_execute_query()
{
	// 1) set query state
	this->m_state_query = eOBJECT_STATE::PROCESSED;
}

void query::redis::Iquery::process_reset_session() noexcept
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		// check) session이 없으면 그냥 여기서 끝낸다.
		RETURN_IF(this->m_psession.empty());

		// 1) close query
		this->close_query();

		// 2) session의 Query Reference를 줄인다.
		this->m_psession->release_query_reference();

		// 3) session 리셋
		this->m_psession.reset();

		// 4) set state
		this->m_state_query = eOBJECT_STATE::NONE;
	}
	
	// 5) Notify하기
	this->_process_notify_to_requestor();
}

void query::redis::Iquery::_process_notify_to_requestor() noexcept
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

shared_buffer generate_redis_command(std::string_view _command)
{
	// definitions)
	const int MAX_STRING_SIZE = 4096;
	const int OFFSET = 8;

	// 1) alloc memory shared_buffer
	auto bufTemp = alloc_shared_buffer(MAX_STRING_SIZE + OFFSET);

	// declare) 
	int count = 0;
	char* str_now = bufTemp.data<char>() + OFFSET;
	const char* str_end = _command.data() + MAX_STRING_SIZE;

	for(const char* iter =_command.data(); *iter!=0 && iter<str_end; )
	{
		// - trimming head
		while(iter != str_end && *iter!=0 && (*iter == ' ' || *iter == '\t'))
		{
			++iter;
		}

		// declare)
		const char*	str_para = iter;

		// - find string end
		while(iter != str_end && *iter!=0 && (*iter != ' ' && *iter != '\t'))
		{
			++iter;
		}

		// - get string length
		int	param_length = static_cast<int>(iter - str_para);

		// - append string length
		*(str_now++) = '$';
		str_now	+= snprintf(str_now, 256, "%d", param_length);
		*(str_now++) = '\r';
		*(str_now++) = '\n';

		// - append string
		memcpy(str_now, str_para, param_length);
		str_now		+= param_length;
		*(str_now++) = '\r';
		*(str_now++) = '\n';

		// - increase count
		++count;
	}

	// 4) write head
	char* str_now_head = bufTemp.data<char>() + OFFSET-1;
	*(str_now_head--) = '\n';
	*(str_now_head--) = '\r';

	for(int	i = count; i != 0; i /= 10)
	{
		*(str_now_head--) = (i % 10)+'0';
	}
	*(str_now_head)	 = '*';

	// 5) set shared_buffer for output
	return bufTemp ^ buffer_view{ str_now_head, static_cast<std::size_t>(str_now - str_now_head) };
}

void query::redis::Iquery::execute_query_sync(std::string_view _command)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter '_command' is nullptr")));

	// 1) process execute
	_process_execute_query(_command);
}

void query::redis::Iquery::_process_execute_query(std::string_view _command)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// 1) generate redis command
	auto buf_query = generate_redis_command(_command);

	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query == eOBJECT_STATE::READY);

		// check) must be prepare
		THROW_IF(this->m_state_query != eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// 2) set query
		this->m_buffer_query = buf_query;

		try
		{
			// 3) ...
			this->m_state_query = eOBJECT_STATE::REQUESTING;

			// declare) 
			CGDK::query::redis::session::REQUEST_QUERY request_info;
			request_info.pquery = this;
			request_info.buf_request = std::move(buf_query);

			// notify) thread block
			scoped_thread_block thread_block;

			// 4) query를 요구한다.
			this->m_psession->execute_query(std::move(request_info));

			// - wait for the response (the state is changed by process_response())
			while (this->m_state_query == eOBJECT_STATE::REQUESTING)
				this->m_cv_complete.wait(cs);
		}
		catch (throwable::failure& /*_e*/)
		{
			// - set state (roll back) 
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
		catch (...)
		{
			// - set state (roll back) 
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise) 
			throw;
		}
	}
}

void query::redis::Iquery::post_execute_query(std::string&& _command, std::function<void()>&& _response)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// check) throw exception if _pquery is nullptr
	THROW_IF(_command.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("parameter '_command' is nullptr")));

	// 1) post execute query
	this->_process_post_execute_query(std::move(_command), std::move(_response));
}

void query::redis::Iquery::_process_post_execute_query(std::string&& _command, std::function<void()>&& _response)
{
	// check) _pquery must not be nullptr
	CGASSERT_ERROR(_command.empty() == false);

	// 1) generate redis command
	auto buf_query = generate_redis_command(_command);

	SCOPED_LOCK_NAMED(this->m_lockable_state_query, cs)
	{
		// check) 
		CGASSERT_ERROR(this->m_state_query >= eOBJECT_STATE::READY);

		// check) must be prepare
		THROW_IF(this->m_state_query < eOBJECT_STATE::READY, throwable::failure(eRESULT::NOT_READY));

		// 1) query command를 작성한다.
		if (this->m_state_query == eOBJECT_STATE::READY)
		{
			// - query string
			this->m_buffer_query = buf_query;

			// - set state
			this->m_state_query = eOBJECT_STATE::REQUESTING;
		}

		try
		{
			// 2) set request query
			CGDK::query::redis::session::REQUEST_QUERY request_info;
			request_info.pquery = this;
			request_info.buf_request = std::move(buf_query);
			request_info.response = std::move(_response);

			// notify) thread block
			scoped_thread_block thread_block;

			// 3) query를 요구한다.
			this->m_psession->execute_query(std::move(request_info));
		}
		catch (...)
		{
			// - rollback state
			this->m_state_query = eOBJECT_STATE::READY;

			// reraise)
			throw;
		}
	}
}
