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

 @class		Nudp
 @brief		비동기 전송을 하는 UDP sendable 클래스
 @todo		
 @ref		
 @details	\n

 비동기 전송을 수행하는 UDP sendable 클래스이다.
 Overlapped를 사용해 전송을 수행한다.
*/
//-----------------------------------------------------------------------------
class net::io::sendable::Nudp : 
	virtual public				net::io::Isocket,
	virtual public				net::io::Isendable,
	virtual public				net::io::statistics::Nsocket
#if defined(_SYS_EPOLL_H)
	, public					Isend_deferrable
#endif
{
protected:
	virtual	bool				process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;
	virtual	void				process_complete_sendable( uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message) override;

#if defined(_SYS_EPOLL_H)
	virtual	uint32_t			process_recover_sendable( intptr_t _event, std::size_t _param) override;

	// - sendmmsg : the datagrams queued while a thread sends (other threads), deferred (send_defer : a group lock / send cork)
	//   or waiting for the socket buffer (EPOLLOUT) are sent in one syscall (SEND_BATCH_MAX at a time).
	//   a datagram that can not be sent (ICMP unreachable, EMSGSIZE ...) is dropped : UDP, the socket is not closed.
	virtual	void				process_flush_deferred_send() noexcept override;
	virtual	void				process_flush_deferred_send_on_close() noexcept override;
	virtual	lock_object*		process_get_lockable_sending() noexcept override { return &this->m_lockable_sending; }

			struct NODE
			{
				buffer_view		buf_send;
				object_ptr<Ireferenceable> powner;
				net::sockaddr	address;
				std::size_t		count_message = 0;
			};

			uint32_t			_process_send_pending(CGSOCKET _handle_socket);
			void				_process_send_deferred() noexcept;
			void				_process_set_epollout(bool _enable) noexcept;
			void				_process_clear_send_buffers();

	mutable	lock_object			m_lockable_sending;
			std::list<NODE>		m_queue_pending;
			std::size_t			m_bytes_pending{ 0 };
			bool				m_flag_sending{ false };		// - a thread is sending or EPOLLOUT is awaited : the sends are queued (in order)
			bool				m_flag_epollout{ false };		// - EPOLLOUT is registered (the socket buffer was full)
			std::atomic<const void*> m_pdefer_owner{ nullptr };	// - the token of the thread deferring its sends (send_defer::token()). nullptr : none

	static	constexpr std::size_t	SEND_BATCH_MAX = 64;			// - datagrams per sendmmsg
	static	constexpr std::size_t	SEND_DEFER_MAX_COUNT = 256;		// - deferred datagrams sent early when they are this many
	static	constexpr std::size_t	SEND_DEFER_MAX_BYTES = 256 * 1024;
#endif

	//! @brief 초기화 될 때 호출되는 함수.
	virtual	void				process_reset_sendable() noexcept override;
};


}