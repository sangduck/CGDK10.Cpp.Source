#include "pch.h"
#include "cgdk/sdk10/server.query.http.h"

//-----------------------------------------------------------------------------
// query::http::query::process_response
//
//  - Nhttp_client delivers each chunk of a chunked response with is_completed == false and the whole body at the end
//  - a chunk without a process function is ignored (the empty function was called : bad_function_call)
//-----------------------------------------------------------------------------
namespace CGDK
{
	class test_http_query : public query::http::query
	{
	public:
				void	set_requesting() noexcept { this->m_state_query = eOBJECT_STATE::REQUESTING; }
	};

	TEST(CGDK10_query_http, chunk_without_process_function)
	{
		auto pquery = make_own<test_http_query>();
		pquery->set_requesting();

		int count_response = 0;
		pquery->set_function([&](sMESSAGE_HTTP_RESPONSE&) { ++count_response; });

		// - a chunk (no process function)
		sMESSAGE_HTTP_RESPONSE msg_chunk;
		msg_chunk.is_completed = false;
		EXPECT_NO_THROW(pquery->process_response(msg_chunk));
		EXPECT_EQ(count_response, 0);

		// - completed
		sMESSAGE_HTTP_RESPONSE msg_complete;
		msg_complete.is_completed = true;
		EXPECT_NO_THROW(pquery->process_response(msg_complete));
		EXPECT_EQ(count_response, 1);
		EXPECT_EQ(pquery->get_object_state(), eOBJECT_STATE::PROCESSED);
	}

	TEST(CGDK10_query_http, chunk_with_process_function)
	{
		auto pquery = make_own<test_http_query>();
		pquery->set_requesting();

		int count_chunk = 0;
		pquery->set_process_function([&](sMESSAGE_HTTP_RESPONSE&) { ++count_chunk; });

		sMESSAGE_HTTP_RESPONSE msg_chunk;
		msg_chunk.is_completed = false;
		EXPECT_NO_THROW(pquery->process_response(msg_chunk));
		EXPECT_NO_THROW(pquery->process_response(msg_chunk));
		EXPECT_EQ(count_chunk, 2);
	}
}
