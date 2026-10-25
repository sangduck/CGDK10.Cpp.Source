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
			auto				execute_query(std::string_view _command) -> execute_result<void>;
			template <class TA, class... TARGS>
			auto				execute_query(const std::string_view _format, TA _first, TARGS... _args) -> execute_result<void>;
			void				post_execute_query(std::string_view _command) { this->_process_post_execute_query(_command, nullptr); }
			void				post_execute_query(std::string_view _command, std::function<void()>&& _response) { this->_process_post_execute_query(_command, std::move(_response)); }
			void				post_execute_query(std::string&& _command) { this->post_execute_query(std::move(_command), nullptr); }
			void				post_execute_query(std::string&& _command, std::function<void()>&& _response);
	virtual	void				close_query() noexcept;

			void				_set_function_on_complete(std::function<void()>&& _function);
			void				_process_on_complete();
			auto				_execute_query_deferred(std::string&& _command) -> execute_result<void>;

	[[nodiscard]] auto&			result() const noexcept { return this->m_result; }

protected:
	virtual	void				process_response();
	virtual	void				process_execute_query() override;
			void				process_set_session(session* _psession);
	virtual	void				process_reset_session() noexcept override;
			void				_process_notify_to_requestor() noexcept;
			void				_process_execute_query(std::string_view _command);
			void				_process_post_execute_query(std::string_view _command, std::function<void()>&& _response, std::function<void()>&& complete = nullptr);
			void				_process_post_fresh_query(std::string_view command, std::function<void()>&& response);
			template <class TLOCK>
			void				process_wait(TLOCK& _cs) { this->m_cv_complete.wait(_cs); }

	virtual	void				on_final_release() noexcept override { this->reset_session(); this->m_result.reset(); this->m_state_query = eOBJECT_STATE::NONE; this->m_request_completion.store(0, std::memory_order_relaxed); }

			resp_data			m_result;

			// Protected by the querier lock; used only while allocating a session.
			std::condition_variable_any m_cv_complete;
			object_ptr<session>	m_psession;
			// Low three bits describe ownership; the upper bits identify each reuse.
            std::atomic<uint64_t> m_request_completion{0};
            uint64_t _begin_request(bool require_ready = true);
            void _publish_request(uint64_t generation) noexcept;
            void _abort_request(uint64_t generation) noexcept;
            void _wait_request(uint64_t generation, bool notify_executor = true);
            uint64_t _reserve_idle(bool may_wait = true);
            void _release_idle(uint64_t previous) noexcept;
			std::function<void()> m_function_on_complete;

			friend class querier;
			friend class session;
};

inline query::execute_result<void> query::redis::Iquery::execute_query(std::string_view _command)
{
	// 1) out of a coroutine: executed at once (same as before)
	if (!is_deferred_execution())
	{
		this->_process_execute_query(_command);
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
		this->_process_execute_query(_format, _first, _args...);
		return execute_result<void>::executed();
	}

	// 2) in a coroutine: deferred
	return this->_execute_query_deferred(std::vformat(_format, std::make_format_args(_first, _args...)));
}


}
