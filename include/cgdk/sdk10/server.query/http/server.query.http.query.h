//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                             server.query.http                             *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::http::Cquery
//
//
// ----------------------------------------------------------------------------
class query::http::query :
// inherited classes) 
	public						net::http_request,
	virtual public				Iserver_query
{
// definitions)
public:
	typedef	query	IQUERY;
	typedef	querier	QUERIER;

public:
		// 1) execute_query/post_execute_query/Cancel
			//    - execute_query()      : without 'co_await' executed as before. with 'co_await' the coroutine is resumed when the response is received. (execute_result)
			//    - execute_query_sync() : always executed at once (waits for the response)
			execute_result<void> execute_query(const std::string_view _command);
			void				execute_query_sync(const std::string_view _command);

		// coroutine) called once after the request is completed (used by 'co_await execute_query()')
			void				_set_function_on_complete(std::function<void()>&& _function);
			void				_process_on_complete();

		// 2) execute
			void				post_execute_query(const std::string_view _command);
			template <class TQUERY = query>
			void				post_execute_query(const std::string_view _command, const std::function<void(result_code,TQUERY*)>& _response);
			template <class TQUERY = query>
			void				post_execute_query(const std::string_view _command, std::function<void(result_code,TQUERY*)>&& _response = nullptr);
	virtual	void				close_query() noexcept;

	[[nodiscard]] const sMESSAGE_HTTP& result() const noexcept { return m_result; }
public:
	virtual	void				process_execute_query() override {}
			void				porcess_set_querier(querier* _pquerier);
	virtual	void				process_reset_session() noexcept override;
	virtual	int					process_response(sMESSAGE& _msg) override;

public:
	// 1) querier
			object_ptr<querier>	m_pquerier;
			sMESSAGE_HTTP		m_result;

	// 2) ....
			std::recursive_mutex m_lockable_state_query;
			std::function<void()> m_function_on_complete;
};

inline query::execute_result<void> query::http::query::execute_query(const std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
	{
		this->execute_query_sync(_command);
		return execute_result<void>::executed();
	}

	// 2) in a coroutine: deferred ('co_await' sends the request and resumes the coroutine when the response is received)
	auto pquery = object_ptr<query>(this);
	std::string command_sync(_command);
	std::string command(_command);

	return execute_result<void>::deferred(
		[pquery, command = std::move(command_sync)]() { pquery->execute_query_sync(command); },
		[pquery, command = std::move(command)](std::function<void()>&& _on_complete) { pquery->_set_function_on_complete(std::move(_on_complete)); pquery->post_execute_query(command); });
}

inline void query::http::query::_set_function_on_complete(std::function<void()>&& _function)
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		this->m_function_on_complete = std::move(_function);
	}
}

inline void query::http::query::_process_on_complete()
{
	std::function<void()> function;

	SCOPED_LOCK(this->m_lockable_state_query)
	{
		function = std::exchange(this->m_function_on_complete, nullptr);
	}

	if (function)
		function();
}

template <class TQUERY>
void query::http::query::post_execute_query(const std::string_view _command, const std::function<void(result_code, TQUERY*)>& _response)
{
	auto response = _response;

	post_execute_query(_command, std::move(response));
}

template <class TQUERY>
void query::http::query::post_execute_query(const std::string_view _command, std::function<void(result_code, TQUERY*)>&& _response)
{
	// 1) set function
	set_function([this, response=std::move(_response)](sMESSAGE_HTTP& /*_msg*/)
	{
		response(eRESULT::SUCCESS, this);
	});

	// 2) execute
	this->post_execute_query(_command);
}


}