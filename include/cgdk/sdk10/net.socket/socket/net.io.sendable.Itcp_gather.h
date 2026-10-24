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

 @class		Itcp_gather
 @brief		비동기 모움 전송을 수행하는 sendable 클래스
 @todo		
 @ref		
 @details	\n

 비동기 송신 중일 경우나 소켓의 송신버퍼가 꽉차 블로킹된 상태(WOULD_BLOCK)인 상태일 때 
 send를 요청하면 큐잉해 놓았다가 비동기가 완료되거나 블로킹이 해제되면 모아 놓았던 데이터를 
 전송하는 sendable클래스이다.
 송신을 비동기적으로 수행함으로써 송신 성능을 극대화하고 CPU의 사용을 최소화 할 수 있다.
 
*/
//-----------------------------------------------------------------------------
class net::io::sendable::Itcp_gather :
	virtual public				net::io::Isendable,
	virtual public				net::io::Iconnectable
{
public:
			Itcp_gather() noexcept;

public:
	// - send queue limit (buffers / bytes) : counts the sends queued behind the batch being sent. a send over it fails (process_on_send_overflow)
	//   the batch being sent is not counted (windows : the WSASend in progress, epoll : the batch taken by the sending thread incl. the rest waiting for EPOLLOUT)
	//   so a socket can hold about the batch being sent + the limit. sending_queued_bytes_now() : epoll includes the rest waiting for EPOLLOUT
	[[nodiscard]] size_t		maximum_depth_of_send_buffer() const noexcept { return this->m_max_depth_of_send_buffer; }
			void				maximum_depth_of_send_buffer(std::size_t _depth) noexcept;
	[[nodiscard]] size_t		maximum_depth_of_send_buffer_bytes() const noexcept { return this->m_bytes_max_queued; }
			void				maximum_depth_of_send_buffer_bytes(std::size_t _size) noexcept;
	[[nodiscard]] size_t		sending_queued_bytes_now() const noexcept { return this->process_get_sending_queued_bytes(); }
	//! @brief the send queue limits back to the ones of the socket object. called by the acceptor / connector at each allocation of the
	//!        socket (a socket from the pool kept the limits its last connection set). the first call keeps the limits it finds : the
	//!        ones of the constructor. a socket's own limits : set in the constructor, on_request_accept() or on_alloc_connectable()
			void				reset_send_queue_limit() noexcept;

	static	void				default_maximum_depth_of_send_buffer(std::size_t _max) noexcept;
	[[nodiscard]] static size_t default_maximum_depth_of_send_buffer() noexcept;
	static	void				default_maximum_queued_bytes(std::size_t _max) noexcept;
	[[nodiscard]] static size_t default_maximum_queued_bytes() noexcept;
			// - acceptor로 받은 접속(client)의 기본값 (기본 4MB)
	static	void				default_maximum_queued_bytes_accepted(std::size_t _max) noexcept;
	[[nodiscard]] static size_t default_maximum_queued_bytes_accepted() noexcept;

protected:
	virtual	std::size_t			process_get_sending_queued_bytes() const noexcept PURE;

#if defined(_SYS_EPOLL_H)
public:
	// - MSG_ZEROCOPY (linux, the io_uring backend only) : a send of one large buffer (>= zerocopy_threshold()) pins its pages instead of copying them
	//   into the socket buffer. the buffer (its owner) is held until the kernel notifies the completion on the error queue (EPOLLERR : process_complete_error_queue).
	//   AF_INET/AF_INET6 only (not a UDS). a socket on an epoll io_context, or a kernel without SO_ZEROCOPY => copied as before. loopback => the kernel copies (notified as copied).
	static	void				enable_zerocopy(bool _enable = true) noexcept { s_enable_zerocopy.store(_enable, std::memory_order_relaxed); }
	[[nodiscard]] static bool	is_zerocopy_enabled() noexcept { return s_enable_zerocopy.load(std::memory_order_relaxed); }
	static	void				zerocopy_threshold(std::size_t _bytes) noexcept { s_zerocopy_threshold.store(_bytes, std::memory_order_relaxed); }
	[[nodiscard]] static std::size_t zerocopy_threshold() noexcept { return s_zerocopy_threshold.load(std::memory_order_relaxed); }
	[[nodiscard]] std::size_t	zerocopy_pending_count() const noexcept { std::lock_guard lock(this->m_lockable_zerocopy); return this->m_queue_zerocopy.size(); }	// - buffers waiting for the notification
	[[nodiscard]] std::size_t	zerocopy_sent_count() const noexcept { return this->m_zerocopy_count_sent.load(std::memory_order_relaxed); }				// - sends done with MSG_ZEROCOPY
	[[nodiscard]] std::size_t	zerocopy_fallback_count() const noexcept { return this->m_zerocopy_count_fallback.load(std::memory_order_relaxed); }		// - zerocopy sends refused (ENOBUFS) and sent again as a copy
	virtual	bool				process_complete_error_queue() noexcept override;

protected:
			struct ZEROCOPY_NODE
			{
				uint32_t		id;
				buffer_view		buf_send;
				object_ptr<Ireferenceable> powner;
			};
			bool				_zerocopy_prepare(CGSOCKET _handle_socket) noexcept;
			int					_zerocopy_send_flags(CGSOCKET _handle_socket, std::size_t _bytes, const object_ptr<Ireferenceable>& _powner) noexcept;
			void				_zerocopy_hold(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner) noexcept;
			void				_zerocopy_reset() noexcept;
			ssize_t				_zerocopy_send(CGSOCKET _handle_socket, const void* _data, std::size_t _bytes, int& _flags) noexcept;
			void				_zerocopy_pause() noexcept;
			[[nodiscard]] bool	_is_zerocopy_paused() const noexcept { const auto until = this->m_zerocopy_paused_until.load(std::memory_order_relaxed); return until != 0 && std::chrono::steady_clock::now().time_since_epoch().count() < until; }

	mutable	std::mutex			m_lockable_zerocopy;
			std::atomic<int>	m_zerocopy_state{ 0 };		// - 0 : not checked yet, 1 : SO_ZEROCOPY set (io_uring io_context), -1 : not used on this socket
			uint32_t			m_zerocopy_id_next{ 0 };	// - the sequence of the next zerocopy send (the kernel counts them : a notification is a range)
			uint32_t			m_zerocopy_id_notified_end{ 0 };	// - a notification read before its send was held (the sender holds it after send() returns) : the ids before this end are done
			bool				m_zerocopy_is_notified_early{ false };
			std::list<ZEROCOPY_NODE> m_queue_zerocopy;		// - the buffers waiting for the notification (under m_lockable_zerocopy)
			std::atomic<std::size_t> m_zerocopy_count_sent{ 0 };
			std::atomic<std::size_t> m_zerocopy_count_fallback{ 0 };
			std::atomic<int64_t> m_zerocopy_paused_until{ 0 };	// - steady_clock : no zerocopy till then (a zerocopy send was refused with ENOBUFS. 0 : not paused)
	static inline std::atomic<bool> s_enable_zerocopy{ true };
	static inline std::atomic<std::size_t> s_zerocopy_threshold{ 16 * 1024 };
	static constexpr std::chrono::milliseconds ZEROCOPY_PAUSE_AFTER_ENOBUFS{ 100 };
#endif

protected:
			std::size_t			m_bytes_max_queued;
			std::size_t			m_max_depth_of_send_buffer;
			std::size_t			m_bytes_max_queued_initial{ 0 };			// - the limits of the socket object (reset_send_queue_limit() : 0 = not kept yet)
			std::size_t			m_max_depth_of_send_buffer_initial{ 0 };

	static	std::size_t			m_s_default_max_queued_bytes;
	static	std::size_t			m_s_default_max_queued_bytes_accepted;
	static	std::size_t			m_s_default_max_depth_of_send_buffer;
};


}
