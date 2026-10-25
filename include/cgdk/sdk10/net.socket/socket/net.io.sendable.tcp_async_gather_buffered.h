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

 @class		Ntcp_async_gather_buffered
 @brief
 @todo		
 @ref		
 @details	\n

 */
//-----------------------------------------------------------------------------
class net::io::sendable::Ntcp_async_gather_buffered :
	public						net::io::sendable::Itcp_gather,
	public						Isend_deferrable,
	virtual public				net::io::Isender_closser
{
public:
		#if defined(_WINSOCK2API_)
			using QUEUE_SEND = net::io::sendable::QUEUE_SEND_bufferd;
		#elif defined(_SYS_EPOLL_H)
			template <class T> using container_t = std::list<T>;
			struct QUEUE_SEND
			{
				struct NODE
				{
					buffer_view		buf_send;
					object_ptr<Ireferenceable> powner;

					std::size_t		bytes_data = 0;
					std::size_t		count_message = 0;
					std::size_t		bytes_remained = 0;
				};

				container_t<NODE> array_node;
				std::size_t		bytes_data = 0;

				void clear() noexcept;
				void push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages);
			};
		#endif

protected:
	virtual	bool				process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;
	virtual	void				process_complete_sendable(uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message) override;
	#if defined(_SYS_EPOLL_H)
			bool				process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message);
	virtual	uint32_t			process_recover_sendable(intptr_t _event, std::size_t _param);
			void				_process_clear_send_buffers();
			uint32_t			_process_send_pending(CGSOCKET _handle_socket, std::size_t _bytes_completed, std::size_t _count_completed);
			// - shutdown(SD_SEND) of _handle_socket under m_lockable_send_call (not done when closed meanwhile)
			void				_process_shutdown_send(CGSOCKET _handle_socket) noexcept;
	static	constexpr uint32_t	SEND_PENDING_FAILED = 0xffffffff;

			// - send cork (net::io::send_cork) : the sends of the thread processing the received messages are deferred (send defer)
			//   and sent at the end of the processing. out of a group lock they are sent earlier when they are old.
	static	constexpr auto		SEND_CORK_DURATION = std::chrono::microseconds(200); // - the oldest corked send waits at most this long (checked when sending)
	static	constexpr std::size_t	SEND_CORK_CHECK_MAX = 32;	// - the age is checked every 1~32 sends (as many sends as take SEND_CORK_DURATION/8 : the clock is not read at every send)

	// - io_uring completion (net.io.sendable.tcp_async_gather_buffered.uring.cpp) : the sends are direct like epoll (send/sendmsg MSG_DONTWAIT
	//   by the sending thread). the socket buffer full (EWOULDBLOCK / a partial send / EINPROGRESS of TFO) posts a SENDMSG op of the front of
	//   m_queue_pending (the rest put back there, and what was queued behind it : up to IOV_MAX buffers, moved into m_queue_sending) instead of
	//   waiting for EPOLLOUT : its completion consumes the bytes and continues with _process_send_pending (the op is the sending thread meanwhile :
	//   m_flag_sending). a single large buffer of the user (>= zerocopy_threshold(), AF_INET/AF_INET6) goes as a SEND_ZC op : the notification
	//   CQE (F_NOTIF) releases the owner held in the executable
	virtual	bool				is_io_completion_supported() const noexcept override { return true; }
			void				_process_post_send_wait() noexcept { if (this->is_io_completion()) this->_process_post_send_op(); else this->post_epoll_event_EPOLLOUT(); }
			bool				_process_post_send_op() noexcept;
			bool				_process_complete_send_op(intptr_t _result) noexcept;	// - true : another op was posted
	[[nodiscard]] bool			_is_zerocopy_op(std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) const noexcept;
	#endif
	virtual	std::size_t			process_get_sending_queued_bytes() const noexcept override;
	#if defined(_WINSOCK2API_)
			// - sends m_queue_sending (the sending thread, out of m_lockable_sending : see m_lockable_send_call)
			bool				_process_send_queue_sending() noexcept;
	#endif
	// - send defer (CGDK::send_defer : in a group lock / net::io::send_cork)
	virtual	void				process_flush_deferred_send() noexcept override;
	virtual	void				process_flush_deferred_send_on_close() noexcept override;
	[[nodiscard]] bool			_is_send_defer_full() const noexcept;
			void				_process_send_deferred() noexcept;
	virtual	void				process_close_sender() override;
	virtual	void				process_reset_sendable() noexcept override;
#if defined(_WINSOCK2API_)
	virtual	lock_object*		process_get_lockable_sending() noexcept override { return &this->m_lockable_send_call; }
#else
	virtual	lock_object*		process_get_lockable_sending() noexcept override { return &this->m_lockable_sending; }
	virtual	lock_object*		process_get_lockable_send_call() noexcept override { return &this->m_lockable_send_call; }
#endif

protected:
	mutable	lock_object			m_lockable_sending;
			QUEUE_SEND			m_queue_pending;
			QUEUE_SEND			m_queue_sending;
			bool				m_flag_sending{ false };

			// - send defer (CGDK::send_defer : in a group lock / net::io::send_cork)
			//   the first deferred send of a thread makes the thread the sending thread (m_flag_sending) and is queued without sending
			//   (windows : m_queue_sending, epoll : m_queue_pending). the next sends of the thread are queued after it (in order with other threads).
			//   process_flush_deferred_send() sends them. they are sent earlier when they are many (half of the queue limit or the counts below).
			//   m_pdefer_owner : the token of the deferring thread (send_defer::token()). nullptr : not deferred
			std::atomic<const void*> m_pdefer_owner{ nullptr };
	static	constexpr std::size_t	SEND_DEFER_MAX_COUNT = 1024;		// - messages
	static	constexpr std::size_t	SEND_DEFER_MAX_BYTES = 64 * 1024;	// - bytes (a larger message is sent at once)

		#if defined(_WINSOCK2API_)
			object_ptr<executable_send_buffer_list> m_pexecutable_send_buffer_list;
			// - the messages of the WSASend in flight (its completion counts them : m_queue_sending may be cleared by a reset before it)
			std::atomic<std::size_t> m_count_message_sending{ 0 };

			// - the sending thread (m_flag_sending) calls WSASend out of m_lockable_sending : the other threads queue to m_queue_pending meanwhile
			//   (they waited for the WSASend in the lock). m_lockable_send_call : the handle is read and WSASend / shutdown are called under it.
			//   the close swaps the handle under it (process_get_lockable_sending) : a handle closed and made again for another socket is not
			//   sent to. lock order : m_lockable_sending -> m_lockable_send_call (never the other way)
			mutable	lock_object			m_lockable_send_call;
		#elif defined(_SYS_EPOLL_H)
			// - the sending thread (m_flag_sending) sends directly out of m_lockable_sending. m_lockable_send_call : the handle is checked and
			//   the send system call is made under it. the close swaps the handle under it (process_get_lockable_send_call) : a handle closed
			//   and made again for another socket is not sent to (io_uring completion closes the handle in process_close_socket).
			//   lock order : m_lockable_sending -> m_lockable_send_call
			mutable	lock_object			m_lockable_send_call;

			// - the front of m_queue_pending put back by the sending thread while waiting for EPOLLOUT (under m_lockable_sending)
			//   not counted for the send queue limit. (the batch taken by the sending thread is not counted : like the WSASend in progress on windows)
			std::size_t			m_count_sending_parked{ 0 };
			std::size_t			m_bytes_sending_parked{ 0 };

			// - io_uring completion : the executable of the SEND op (pooled. one per socket, one op in flight at a time : the sending thread)
			//   holds the socket (add_ref) while the op is in flight, the iovecs of the op, the owner of a zerocopy buffer till its notification,
			//   and the nodes of a socket reset while the op was in flight (m_queue_hold : the buffers stay till the kernel is done with the op).
			//   the nodes of the op are in m_queue_sending (not counted for the send queue limit, m_bytes_sending_op : sending_queued_bytes_now())
			class executable_sending : virtual public Iexecutable, public Npoolable<executable_sending>, public factory::_traits_system
			{
			public:
				virtual ~executable_sending() noexcept {}
				virtual	intptr_t process_execute(intptr_t _result, std::size_t _param) override;
				virtual	void on_final_release() noexcept override { this->m_psendable = nullptr; this->m_is_reposted = false; this->m_iovec.clear(); this->m_owners_zerocopy.clear(); this->m_queue_hold.clear(); }
				Ntcp_async_gather_buffered* m_psendable = nullptr;
				::msghdr		m_msg{};
				std::vector<::iovec> m_iovec;
				bool			m_is_reposted = false;			// - the completion of this op posted another one (the wait for the socket buffer continues)
				struct ZEROCOPY_OP { object_ptr<Ireferenceable> powner; bool is_failed = false; };	// - is_failed : its result was an error (-ENOBUFS / -ENOMEM : sent again as a copy) : its notification is not counted (copied)
				std::vector<ZEROCOPY_OP> m_owners_zerocopy;	// - an entry per op : the owner of a SEND_ZC (empty : a SENDMSG) till its notification (in order)
				container_t<QUEUE_SEND::NODE> m_queue_hold;
			};
			object_ptr<executable_sending> m_pexecutable_sending;
			int					m_count_sending_op{ 0 };		// - the SEND ops in flight (under m_lockable_sending. 2 : a repost before the notification of a zerocopy send)
			std::size_t			m_bytes_sending_op{ 0 };		// - the bytes left in m_queue_sending for the op (under m_lockable_sending)
		#endif
};


}