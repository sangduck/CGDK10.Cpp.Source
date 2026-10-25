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
#include <exception>
#include <optional>
#include <variant>
#include <deque>

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::redis::session
//
//
// ----------------------------------------------------------------------------
class query::redis::session :
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::sendable::Ntcp_async_gather_buffered,
	public						net::io::sender::Nresp,
	public						net::io::receivable::tcp::Ndefault,
	public						net::io::packetable::Nresp,
	virtual public				Imessageable
{
public:
			session();
	virtual	~session() noexcept;

public:
			struct REQUEST_QUERY
			{
				object_ptr<Iquery>		pquery;
				shared_buffer			buf_request;
				std::function<void()>	response;
				std::size_t count_batch = 0;
				std::size_t received_batch = 0;
				bool callback_affinity = false;
                std::variant<std::monostate, std::string, std::vector<std::string>> input;
			};

			bool				execute_query(REQUEST_QUERY&& _query, bool allow_send_queue = false, bool count_statistics = true);
			bool				post_command(REQUEST_QUERY&& request, std::string command, bool allow_send_queue);
			bool				post_batch(REQUEST_QUERY&& request, std::vector<std::string> commands);
			void				cancel_query_all() noexcept;
			[[nodiscard]] auto	request_send_window() const noexcept->std::size_t { return this->m_request_send_window; }
			[[nodiscard]] auto	coroutine_resume_policy() const { return std::pair{this->m_pcoroutine_executor, this->m_preserve_coroutine_executor}; }
			[[nodiscard]] bool	is_send_defer_enabled() const noexcept { return this->m_enable_send_defer; }
			void				add_query_reference();
			void				release_query_reference() noexcept;

protected:
	virtual std::size_t			process_packet(shared_buffer& _buffer, const net::sockaddr& _address) override;
	virtual std::size_t			process_resp_message(sMESSAGE_RESP& _msg, shared_buffer& _remaining) override;
			std::size_t			process_batch_responses(sMESSAGE_RESP& _msg, shared_buffer& _remaining);
	virtual void				on_request_connect(const net::sockaddr& _paddress_peer) override;
	virtual void				on_connect() override;
	virtual void				on_fail_connect(uint64_t _disconnect_reason) override;
	virtual	void				on_disconnect(uint64_t _disconnect_reason) override;

	virtual	result_code			process_message(sMESSAGE& _msg) override;

			std::recursive_mutex m_cs_queue_query;
			circular_list<REQUEST_QUERY> m_queue_send;

			circular_list<REQUEST_QUERY> m_queue_deferred;

			std::atomic<std::size_t> m_pending_send_count{0};
			std::size_t			m_pending_send_bytes = 0;
			using ENCODING_REQUEST = REQUEST_QUERY;

			bool				submit_encoding(ENCODING_REQUEST&& request);
			void				schedule_encoding(std::size_t epoch, object_ptr<querier> owner) noexcept;
			void				drain_encoding(std::size_t epoch, object_ptr<querier> owner) noexcept;
			void				cancel_encoding_epoch(std::size_t epoch, querier* owner) noexcept;
	static	void				finish_canceled(REQUEST_QUERY& request) noexcept;
	static	void				cancel_staged(std::span<ENCODING_REQUEST> requests, querier* owner) noexcept;
    static void cancel_staged(circular_list<REQUEST_QUERY>& requests, querier* owner) noexcept;
			void				process_flush_deferred_send() noexcept override;
			std::mutex m_cs_queue_encoding;
			circular_list<REQUEST_QUERY> m_queue_query;
			std::atomic<std::size_t> m_encoding_epoch{0};
			bool m_encoding_scheduled = false;
			const void* m_encoding_defer_owner = nullptr;
			bool m_encode_on_session = false;
			bool m_enable_auto_send = false;
			bool m_enable_send_defer = true;
			std::size_t m_request_send_window = 8;
			object_ptr<Iexecutor> m_pcoroutine_executor;
			bool m_preserve_coroutine_executor = true;

			void				send_query_locked(REQUEST_QUERY& request);
			std::exception_ptr	drain_send_locked(std::optional<circular_list<REQUEST_QUERY>>& canceled) noexcept;
			static void			cancel_unsent(circular_list<REQUEST_QUERY>& canceled, querier* owner) noexcept;
			object_ptr<querier>	m_pquerier;

			void				push_query(REQUEST_QUERY _request_info);

			object_ptr<querier>	process_attach_querier(querier* _pquerier) noexcept;
			object_ptr<querier>	process_detach_querier(bool& _is_in_use) noexcept;
			void				process_cancel_query_all(querier* _pquerier) noexcept;

			bool m_enable_batch_receive = false;
			std::atomic<std::size_t> m_pending_batch_queries{0};
			static constexpr uint32_t dedicated_reference = uint32_t(1) << 31;
            static constexpr uint32_t reference_mask = dedicated_reference - 1;
            std::atomic<uint32_t> m_query_reference;
            static constexpr uint64_t shared_routes_closed = uint64_t(1) << 63;
            std::atomic<uint64_t> m_shared_route_access{0};
            void close_shared_routes() noexcept;
            uint32_t query_reference_count() const noexcept { return m_query_reference.load(std::memory_order_acquire) & reference_mask; }
			std::atomic<bool> m_is_dedicated{false};
			friend class querier;
			friend class Iquery;
            struct COMPLETED_RESPONSE {
                REQUEST_QUERY request;
                object_ptr<querier> owner;
                bool may_reuse = false;
            };
            class response_executable;
            void complete_response(REQUEST_QUERY&& request, querier* owner, bool may_reuse) noexcept;
            void schedule_response_callbacks() noexcept;
            void drain_response_callbacks(bool all = false) noexcept;
            void invoke_response(COMPLETED_RESPONSE& response) noexcept;
            std::mutex m_cs_response_queue;
            std::recursive_mutex m_cs_response_processing;
            std::deque<COMPLETED_RESPONSE> m_queue_response;
            bool m_response_scheduled = false;
            bool m_post_response = true;
            object_ptr<Iexecutor> m_response_executor;
            class encoding_executable;
};


}