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

 @class		Ntcp_crypt_async_gather_buffered
 @brief
 @todo		
 @ref		
 @details	\n

 */
//-----------------------------------------------------------------------------
class net::io::sendable::Ntcp_crypt_async_gather_buffered :
	public						net::io::sendable::Itcp_gather,
	virtual public				net::io::Isender_closser,
	virtual public				Icryptable
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
			void				_process_shutdown_send() noexcept;
			void				_process_wait_epollout() noexcept;
			uint32_t			_process_send_pending();
			void				_process_begin_async_sending() noexcept;
			void				_process_end_async_sending() noexcept;
	static	constexpr uint32_t	SEND_PENDING_MORE = 0x80000000u;	// _process_send_pending() : sent, more queued meanwhile (sent at once)
	#endif
	virtual	std::size_t			process_get_sending_queued_bytes() const noexcept override;
	virtual	void				process_close_sender() override;
	virtual	void				process_reset_sendable() noexcept override;
	virtual	lock_object*		process_get_lockable_sending() noexcept override { return &this->m_lockable_sending; }
	#if defined(_SYS_EPOLL_H)
	// - the direct sends (out of m_lockable_sending of the queue) take m_lockable_sending again around the send system call : the close swaps
	//   the handle under it (a handle closed and made again for another socket is not sent to)
	virtual	lock_object*		process_get_lockable_send_call() noexcept override { return &this->m_lockable_sending; }
	#endif

protected:
	mutable	lock_object			m_lockable_sending;
			QUEUE_SEND			m_queue_pending;
			QUEUE_SEND			m_queue_sending;
			bool				m_flag_sending{ false };

		#if defined(_WINSOCK2API_)
			object_ptr<executable_send_buffer_list> m_pexecutable_send_buffer_list;
		#elif defined(_SYS_EPOLL_H)
			// - the async sending is counted (statistics_on_increase_async_sending) : waiting for EPOLLOUT or the queue being sent. decreased once
			//   (the send of all, a close (process_reset_sendable)). under m_lockable_sending
			bool				m_is_async_sending{ false };
		#endif
};


}