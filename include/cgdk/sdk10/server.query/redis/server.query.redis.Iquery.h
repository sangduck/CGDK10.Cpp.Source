//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            server.query.redis                             *
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
#include <format>

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::redis::Iquery
//
//
//-----------------------------------------------------------------------------
class query::redis::Iquery : virtual public Iserver_query
{
public:
	typedef	Iquery					IQUERY;
	typedef	querier					QUERIER;
	typedef	sMESSAGE_REDIS_RESPONSE	RESPONSE;

	virtual ~Iquery();

	[[nodiscard]] auto			get_session() const noexcept { return this->m_psession;}
			void				set_session(session* _psession) { this->process_set_session(_psession);}
			void				reset_session() noexcept { this->process_reset_session();}

			void				wait();
			//    - execute_query()      : without 'co_await' executed as before. with 'co_await' the coroutine is resumed when the response is received. (execute_result)
			//    - execute_query_sync() : always executed at once (waits for the response)
			execute_result<void> execute_query(std::string_view _command);
			template <class TA, class... TARGS>
			execute_result<void> execute_query(const std::string_view _format, TA _first, TARGS... _args);
			void				execute_query_sync(std::string_view _command);
			template <class TA, class... TARGS>
			void				execute_query_sync(const std::string_view _format, TA _first, TARGS... _args);
			void				post_execute_query(std::string_view _command) { this->post_execute_query(std::string(_command), nullptr); }
			void				post_execute_query(std::string_view _command, std::function<void()>&& _response) { this->post_execute_query(std::string(_command), std::move(_response)); }
			void				post_execute_query(std::string&& _command) { this->post_execute_query(std::move(_command), nullptr); }
			void				post_execute_query(std::string&& _command, std::function<void()>&& _response);
	virtual	void				close_query() noexcept;

		// coroutine) called once after the request is completed (used by 'co_await execute_query()')
			void				_set_function_on_complete(std::function<void()>&& _function);
			void				_process_on_complete();
			execute_result<void> _execute_query_deferred(std::string&& _command);

	[[nodiscard]] auto&			result() const noexcept { return this->m_result; }

protected:
	virtual	void				process_response();
	virtual	void				process_execute_query() override;
			void				process_set_session(session* _psession);
	virtual	void				process_reset_session() noexcept override;
			void				_process_notify_to_requestor() noexcept;
			void				_process_execute_query(std::string_view _command);
			void				_process_post_execute_query(std::string&& _command, std::function<void()>&& _response);
			template <class TLOCK>
			void				process_wait(TLOCK& _cs) { this->m_cv_complete.wait(_cs); }

	virtual	void				on_final_release() noexcept override { this->reset_session(); this->m_result.reset(); }

			shared_buffer		m_buffer_query;
			resp_data			m_result;

			std::condition_variable_any m_cv_complete;
			object_ptr<session>	m_psession;
			std::recursive_mutex m_lockable_state_query;
			std::function<void()> m_function_on_complete;

			friend class querier;
			friend class session;
};

template <class TA, class... TARGS>
void query::redis::Iquery::execute_query_sync(const std::string_view _format, TA _first, TARGS... _args)
{
	// check) _format shouldn't be nullptr
	CGASSERT_ERROR(_format.empty() == false);

	// 1) generate string
	auto query_string = std::vformat(_format, std::make_format_args(_first, _args...));

	// 2) execute query
	return this->_process_execute_query(query_string);
}

inline query::execute_result<void> query::redis::Iquery::execute_query(std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
	{
		this->execute_query_sync(_command);
		return execute_result<void>::executed();
	}

	// 2) in a coroutine: deferred
	return this->_execute_query_deferred(std::string(_command));
}

template <class TA, class... TARGS>
query::execute_result<void> query::redis::Iquery::execute_query(const std::string_view _format, TA _first, TARGS... _args)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
	{
		this->execute_query_sync(_format, _first, _args...);
		return execute_result<void>::executed();
	}

	// 2) in a coroutine: deferred
	return this->_execute_query_deferred(std::vformat(_format, std::make_format_args(_first, _args...)));
}

inline query::execute_result<void> query::redis::Iquery::_execute_query_deferred(std::string&& _command)
{
	// - 'co_await' sends the request and resumes the coroutine when the response is received
	auto pquery = object_ptr<Iquery>(this);
	std::string command_sync(_command);

	return execute_result<void>::deferred(
		[pquery, command = std::move(command_sync)]() { pquery->execute_query_sync(command); },
		[pquery, command = std::move(_command)](std::function<void()>&& _on_complete) { pquery->_set_function_on_complete(std::move(_on_complete)); pquery->post_execute_query(std::string(command)); });
}

inline void query::redis::Iquery::_set_function_on_complete(std::function<void()>&& _function)
{
	SCOPED_LOCK(this->m_lockable_state_query)
	{
		this->m_function_on_complete = std::move(_function);
	}
}

inline void query::redis::Iquery::_process_on_complete()
{
	std::function<void()> function;

	SCOPED_LOCK(this->m_lockable_state_query)
	{
		function = std::exchange(this->m_function_on_complete, nullptr);
	}

	if (function)
		function();
}


}