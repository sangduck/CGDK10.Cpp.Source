#include "pch.h"

using namespace CGDK;


query::redis::session::session() : 
	m_query_reference(0)
{
	minimum_message_buffer_size	(        64*1024);
	maximum_message_buffer_size	( 64 * 1024*1024);
}

query::redis::session::~session()
{
}

void query::redis::session::add_query_reference()
{
	++this->m_query_reference;
}
void query::redis::session::release_query_reference()
{
	if ((--this->m_query_reference) == 0)
	{
		if (this->m_pquerier.exist())
		{
			this->m_pquerier->free_session(this);
		}
	}
}

bool query::redis::session::execute_query(REQUEST_QUERY&&_query)
{
	// check) ... 
	CGASSERT_ERROR(_query.pquery.exist());

	// check) ...
	THROW_IF(_query.pquery.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) Query를 먼저 Queuing한다
	this->push_query(_query);

	// 2) send를 전송한다.
	this->send(_query.buf_request);

	// return)
	return true;
}

void query::redis::session::cancel_query_all() noexcept
{
	// locked)
	{
		// scoped_lock) 
		std::lock_guard<std::recursive_mutex> cs(this->m_cs_list_query);

		// check)
		RETURN_IF(this->m_pquerier.empty());

		for(auto& iter: this->m_list_query)
		{
			try
			{
				// - set result as ...
				iter.pquery->m_result.type = eRESP_DATA_TYPE::NONE_;

				// - process response
				iter.pquery->process_response();
			}
			catch (...)
			{
			}

			// statistics) 
			this->m_pquerier->statistics_on_free_session();

			// statistics) 
			this->m_pquerier->statistics_on_query_fail();
		}

		// - clear한다.
		this->m_list_query.clear();
	}
}

void query::redis::session::push_query(REQUEST_QUERY _request_infoy)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<std::recursive_mutex> cs(this->m_cs_list_query);

		// 1) Queuing한다.
		this->m_list_query.push_back(_request_infoy);

		// statistics) 
		this->m_pquerier->statistics_on_alloc_session();
	}
}

query::redis::session::REQUEST_QUERY query::redis::session::pop_query() noexcept
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<std::recursive_mutex> cs(this->m_cs_list_query);

		// check) 
		RETURN_IF(this->m_list_query.empty(), REQUEST_QUERY());

		// 1) 제일 앞에 것을 얻는다.
		auto request_info = this->m_list_query.front();

		// 2) 하나를 빼낸다.
		this->m_list_query.pop_front();

		// statistics) 
		this->m_pquerier->statistics_on_free_session();

		// return) 
		return request_info;
	}
}

void query::redis::session::on_request_connect(const net::sockaddr& /*_paddrPeer*/) 
{
	// 1) set as nodelay (turn off nagle)
	api::socket::instance().set_tcp_nodelay(native_handle(), true);
}

void query::redis::session::on_connect()
{
	this->minimum_message_buffer_size(64 * 1024);
	this->maximum_message_buffer_size(64 * 1024 * 1024);

	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<std::recursive_mutex> cs(this->m_cs_list_query);

		// 1) 접속시 Queuing된 데이터들을 모두 전송한다.
		for (auto& iter : this->m_list_query)
		{
			// 2) send를 전송한다.
			this->send(iter.pquery->m_buffer_query);
		}
	}
}

void query::redis::session::on_fail_connect(uint64_t /*_Reason*/)
{
}

void query::redis::session::on_disconnect(uint64_t /*_Reason*/)
{
}

result_code query::redis::session::process_message(sMESSAGE& _msg)
{
	// 1) process message
	if(_msg.message == eMESSAGE::SYSTEM::NETWORK_RESP)
	{
		// declare) 
		REQUEST_QUERY request_info;
		object_ptr<querier>	pquerier;

		// 1) poping queued query
		scoped_lock(this->m_cs_list_query)
		{
			request_info = this->pop_query();
			pquerier = this->m_pquerier;
		}

		// check) return if pquery is empty
		RETURN_IF(pquerier.empty(), 0);

		// check) return if pquery is empty
		RETURN_IF(request_info.pquery.empty(), eRESULT::BYPASS);

		// declare) 
		auto& msg_redis = (sMESSAGE_RESP&)_msg;

		// - result를 설정한다.
		request_info.pquery->m_result = std::move(msg_redis.data);

		// - 완료 시...
		try
		{
			// - call response function
			if (request_info.response != nullptr)
			{
				request_info.response();
			}

			// - response한다.
			request_info.pquery->process_response();

			// statistics) 
			if (request_info.pquery->m_result.type != eRESP_DATA_TYPE::ERROR_)
			{
				pquerier->statistics_on_query_success();
			}
			else
			{
				pquerier->statistics_on_query_fail();
			}

			// statistics) 
			--pquerier->m_count_query_queued;
		}
		catch (...)
		{
			pquerier->statistics_on_query_fail();
		}
	}
	
	// return) 
	return eRESULT::BYPASS;
}
