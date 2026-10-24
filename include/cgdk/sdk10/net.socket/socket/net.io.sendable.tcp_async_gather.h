//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
/**

 @class		Ntcp_async_gather
 @brief		
 @todo		
 @ref		
 @details	\n

 */
//-----------------------------------------------------------------------------
class net::io::sendable::Ntcp_async_gather :
	public						net::io::sendable::Itcp_gather,
	public						Isend_deferrable,
	virtual public				net::io::Isender_closser
{
public:
		#if defined(_WINSOCK2API_)
			using QUEUE_SEND = net::io::sendable::QUEUE_SEND;

		#elif defined(_SYS_EPOLL_H)
			struct QUEUE_SEND
			{
				struct NODE
				{
					object_ptr<Ireferenceable> powner;

					std::size_t		bytes_data{ 0 };		// - requested bytes (statistics)
					std::size_t		count_message{ 0 };
				};

				// - array_buffer[i] is the (remained) buffer of array_node[i]. it is passed to sendmsg() as it is.
				//   (std::vector : m_queue_pending and m_queue_sending are swapped. the capacity is reused)
				std::vector<::iovec> array_buffer;
				std::vector<NODE> array_node;
				std::size_t		bytes_data{ 0 };

				void clear() noexcept { this->array_buffer.clear(); this->array_node.clear(); this->bytes_data = 0; }
				void swap(QUEUE_SEND& _rhs) noexcept { this->array_buffer.swap(_rhs.array_buffer); this->array_node.swap(_rhs.array_node); std::swap(this->bytes_data, _rhs.bytes_data); }
				void push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _bytes_data, std::size_t _count_message);
				// - expands the last buffer if _buffer follows it in memory with the same owner. otherwise push_back. (bytes_data is added)
				void push_back_or_expand(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message);
			};
		#endif

protected:
	virtual	bool				process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;
	virtual	void				process_complete_sendable( uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message) override;
#if defined(_WINSOCK2API_)
			bool				process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message);
	virtual	std::size_t			process_get_sending_queued_bytes() const noexcept override;
#elif defined(_SYS_EPOLL_H)
	virtual	uint32_t			process_recover_sendable(intptr_t _event, std::size_t _param);
	virtual	std::size_t			process_get_sending_queued_bytes() const noexcept override;
			void				_process_clear_send_buffers();
			uint32_t			_process_send_queued(CGSOCKET _handle_socket, std::size_t _bytes_completed, std::size_t _count_completed);
	static	constexpr uint32_t	SEND_QUEUED_FAILED = 0xffffffff;

	// - io_uring completion (net.io.sendable.tcp_async_gather.uring.cpp) : the sends are still direct (send/sendmsg MSG_DONTWAIT by the
	//   caller thread). the socket buffer full (EWOULDBLOCK / a partial send / EINPROGRESS of TFO) posts a SENDMSG op of the rest
	//   (m_queue_sending from m_index_sending) instead of waiting for EPOLLOUT : its completion consumes the bytes and continues with
	//   _process_send_queued (the op is the sending thread meanwhile : m_flag_sending). a single large buffer (>= zerocopy_threshold())
	//   goes as a SEND_ZC op : the first CQE gives the bytes, the notification CQE (F_NOTIF) releases the owner held in the executable
	virtual	bool				is_io_completion_supported() const noexcept override { return true; }
	//! @brief io_uring completion : the sends of the thread that owns the ring of this socket go as SEND ops on that ring (true) instead of
	//!        direct sends. off by default (CGDK_IO_URING_SEND_OP=1 or enable_send_on_owner_ring(true) : on)
	[[nodiscard]] bool			_is_send_on_owner_ring(CGSOCKET _handle_socket) const noexcept;
	static	void				enable_send_on_owner_ring(bool _enable = true) noexcept;
	[[nodiscard]] static bool	is_send_on_owner_ring_enabled() noexcept;
			void				_process_post_send_wait() noexcept { if (this->is_io_completion()) this->_process_post_send_op(); else this->post_epoll_event_EPOLLOUT(); }
			bool				_process_post_send_op() noexcept;
			bool				_process_complete_send_op(intptr_t _result, std::size_t _flags) noexcept;	// - true : another op was posted
	[[nodiscard]] bool			_is_zerocopy_op(std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) const noexcept;
#endif
	// - send defer (CGDK::send_defer : in a group lock / net::io::send_cork)
	virtual	void				process_flush_deferred_send() noexcept override;
	virtual	void				process_flush_deferred_send_on_close() noexcept override;
	[[nodiscard]] bool			_is_send_defer_full() const noexcept;
			void				_process_send_deferred() noexcept;
	virtual	void				process_close_sender() override;
	virtual	void				process_reset_sendable() noexcept override;
	virtual	lock_object*		process_get_lockable_sending() noexcept override { return &this->m_lockable_sending; }

protected:
	mutable	lock_object			m_lockable_sending;
			QUEUE_SEND			m_queue_pending;
			QUEUE_SEND			m_queue_sending;
			bool				m_flag_sending{ false };

			// - send defer (CGDK::send_defer : in a group lock / net::io::send_cork)
			//   the first deferred send of a thread makes the thread the sending thread (m_flag_sending) and is queued in m_queue_sending
			//   without sending. the next sends of the thread are queued after it (after m_queue_pending if other threads queued).
			//   process_flush_deferred_send() sends them. they are sent earlier when they are many (half of the queue limit or the counts below).
			//   m_pdefer_owner : the token of the deferring thread (send_defer::token()). nullptr : not deferred
			std::atomic<const void*> m_pdefer_owner{ nullptr };
	static	constexpr std::size_t	SEND_DEFER_MAX_COUNT = 1024;		// - messages
	static	constexpr std::size_t	SEND_DEFER_MAX_BYTES = 64 * 1024;	// - bytes (a larger message is sent at once)

		#if defined(_WINSOCK2API_)
			object_ptr<executable_send_buffer_list> m_pexecutable_send_buffer_list;
		#elif defined(_SYS_EPOLL_H)
			// - m_queue_sending is used only by the sending thread (m_flag_sending). it is sent from array_buffer[m_index_sending]
			std::size_t			m_index_sending{ 0 };
			// - remained in m_queue_sending while waiting for EPOLLOUT (under m_lockable_sending : duplicated EPOLLOUT check, sending_queued_bytes_now())
			//   not counted for the send queue limit. (the batch taken by the sending thread is not counted : like the WSASend in progress on windows)
			std::size_t			m_count_sending_parked{ 0 };
			std::size_t			m_bytes_sending_parked{ 0 };

			// - send cork (net::io::send_cork) : the sends of the thread processing the received messages are deferred (send defer)
			//   and sent at the end of the processing. out of a group lock they are sent earlier when they are old.
	static	constexpr auto		SEND_CORK_DURATION = std::chrono::microseconds(200); // - the oldest corked send waits at most this long (checked when sending)
	static	constexpr std::size_t	SEND_CORK_CHECK_MAX = 32;	// - the age is checked every 1~32 sends (as many sends as take SEND_CORK_DURATION/8 : the clock is not read at every send)

			// - io_uring completion : the executable of the SEND op (pooled. one per socket, one op in flight at a time : the sending thread)
			//   holds the socket (add_ref) while the op is in flight, the owner of a zerocopy buffer till its notification, and the queue
			//   of a socket reset while the op was in flight (m_queue_hold : the buffers stay till the kernel is done with the op)
			class executable_sending : virtual public Iexecutable, public Npoolable<executable_sending>, public factory::_traits_system
			{
			public:
				virtual ~executable_sending() noexcept {}
				virtual	intptr_t process_execute(intptr_t _result, std::size_t _param) override;
				virtual	void on_final_release() noexcept override { this->m_psendable = nullptr; this->m_is_reposted = false; this->m_owners_zerocopy.clear(); this->m_queue_hold.clear(); }
				Ntcp_async_gather* m_psendable = nullptr;
				::msghdr		m_msg{};
				bool			m_is_reposted = false;			// - the completion of this op posted another one (the wait for the socket buffer continues)
				std::vector<object_ptr<Ireferenceable>> m_owners_zerocopy;	// - the owners of the zerocopy buffers waiting for their notification (in order)
				QUEUE_SEND		m_queue_hold;
			};
			object_ptr<executable_sending> m_pexecutable_sending;
			int					m_count_sending_op{ 0 };		// - the SEND ops in flight (under m_lockable_sending. 2 : a repost before the notification of a zerocopy send)
		#endif
};


}