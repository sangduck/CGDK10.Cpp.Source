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

@class		Nstream
@brief 스트림(TCP)용 저수준 I/O receive 처리 클래스

TCP와 같이 Stream으로 전달되어 오는 데이터의 receive를 처리하는 class이다.
구현은 Pro-Actor패턴인 IOCP로 구현되어 있기 때문에 접속과 동시에 prepare_receive()함수를 호출하여
receive을 걸어놓는다.
Stream처리이기 때문에 동시에 한번의 receive I/O만 걸어놓는다.
receive 완료된 후 받은 message를 완전히 다 처리한 후 다시 receive를 걸도록 설계되었다.
이로 인해 대량의 데이터를 receive할 경우 처리가 지연될 수 있다.
receive의 완료가 0Byte 혹은 Error로 되면 접속 종료처리를 한다.

@todo
@ref		net::io::Receiveable::Nstream_queue, net::io::Receiveable::Nstream_rio 
@details	\n

*/
//-----------------------------------------------------------------------------
class net::io::receivable::Nstream :
	virtual public				net::io::Isocket,
	virtual public				net::io::Ireceivable_stream,
	virtual public				net::io::Iconnectable,
	virtual public				net::io::Ipacketable
{
protected:
			bool m_send_cork_enabled = true;
			Nstream();
	virtual	~Nstream() noexcept;

public:
			void send_cork_enabled(bool enabled) noexcept { this->m_send_cork_enabled = enabled; }
	[[nodiscard]] bool send_cork_enabled() const noexcept { return this->m_send_cork_enabled; }
			std::size_t			minimum_message_buffer_size(std::size_t _size) noexcept;
	[[nodiscard]] std::size_t	minimum_message_buffer_size() noexcept;
			std::size_t			maximum_message_buffer_size(std::size_t _size) noexcept;
	[[nodiscard]] std::size_t	maximum_message_buffer_size() noexcept;
	[[nodiscard]] std::size_t	queued_receive_bytes() const noexcept { return m_buffer_received.size(); }
	#if defined(_SYS_EPOLL_H)
	//! @brief io_uring completion : the receive mode (true : the multishot RECV into the provided buffers, false : a RECV into the receive buffer that grows like the IOCP one)
	[[nodiscard]] bool			is_receive_multishot_mode() const noexcept { return this->m_receive_mode == RECEIVE_MODE_MULTISHOT; }
	#endif

protected:
	virtual	void				on_receive( const buffer_view& /*_buffer*/, const net::sockaddr& /*_address*/)	{}

	#if defined(_WINSOCK2API_)
			bool				receive();
	#elif defined(_SYS_EPOLL_H)
	// - io_uring completion (net.io.receivable.stream.uring.cpp) : one receive op in flight, posted by receive() under the receiving lock
	//   (the handle is read and the op is submitted under it : a close swaps the handle under the same lock and cancels the ops after).
	//   a multishot RECV into the provided buffers of the io_context (the data copied into m_buffer_received, the buffer given back),
	//   else (_is_multishot false : after -ENOBUFS, or not available) a RECV into m_buffer_received. its completions
	//   (process_complete_receivable_cq) process the bytes, the end of the op posts the next one. 0 bytes / an error : the disconnect
	virtual	bool				is_io_completion_supported() const noexcept override { return true; }
	virtual	bool				process_start_receive() noexcept override;
			bool				receive(bool _is_multishot = true);
			void				process_complete_receivable_cq(intptr_t _result, std::size_t _flags);
	#endif

	virtual	void				process_reset_receivable() override;
	virtual	bool				prepare_receive() override;
	virtual void				process_on_receive( const buffer_view& _buffer, const net::sockaddr& _address) { on_receive (_buffer, _address);}
	virtual	intptr_t			process_complete_receivable( intptr_t _result, std::size_t _transferred) override;
			void				process_clear_buffer() noexcept;

			std::size_t			m_minimum_message_buffer_size;
			std::size_t			m_maximum_message_buffer_size;

		#if defined(_WINSOCK2API_)
			class executable_receiving : virtual public Iexecutable, public Npoolable<executable_receiving>, public factory::_traits_system
			{
			public:
				Nstream*		m_preceivable_stream = nullptr;
				virtual	intptr_t process_execute( intptr_t _result, std::size_t _param);
			};

			class executable_receiving_dispatch : public executable_receiving
			{
			public:
				virtual	intptr_t process_execute(intptr_t _result, std::size_t _param);
			};

			shared_buffer		m_buffer_received; // 현재 메시지 처리하기 위해 남은 잔량 buffer...
			WSABUF				m_wsabuf_receiving; // receive를 걸어 놓은 shared_buffer...
			DWORD				m_wsa_bytes_received{ 0 };
			DWORD				m_wsa_flag{ 0 };

			object_ptr<executable_receiving> m_pexecutable_receiving; //! @brief receive를 처리할 Iexecutable 객체

			// - the receive buffer for the next receive (the rules of the epoll one) : the buffer only this socket refers to is reused (the data
			//   not processed moved to the front) instead of a new one at every wrap, grown 8x up to MAX_BYTES_RECEIVE_WINDOW_GROWN when a receive
			//   filled the window, shrunk after COUNT_SMALL_RECEIVES_TO_SHRINK small receives in a row
			void				_process_prepare_receive_buffer(std::size_t _bytes_received, bool _is_window_filled, std::size_t _required_bytes);
	static	constexpr std::size_t MAX_BYTES_RECEIVE_WINDOW_GROWN = 64 * 1024;
	static	constexpr uint32_t	COUNT_SMALL_RECEIVES_TO_SHRINK = 64;
			uint32_t			m_count_receive_small = 0;
		#elif defined(_SYS_EPOLL_H)
			shared_buffer		m_buffer_received; // 수신 후 처리되지 않은 남은 데이터 (그 뒤의 빈 공간에 이어서 받는다)
			int					m_bytes_buffer_receive; // 현재 수신 버퍼의 크기

			void				_process_prepare_receive_buffer(std::size_t _bytes_received, bool _is_window_filled, std::size_t _required_bytes);
	static	constexpr int		MAX_RECEIVE_PER_EVENT = 8; // 한 event에서 recv하는 최대 횟수
	static	constexpr std::size_t MAX_BYTES_RECEIVE_WINDOW_GROWN = 64 * 1024; // 창을 다 채워 받았을 때 늘리는 수신 버퍼의 최대 크기 (더 큰 버퍼는 message가 그만큼 클 때만)
	static	constexpr uint32_t	COUNT_SMALL_RECEIVES_TO_SHRINK = 64; // the buffer shrinks only after this many consecutive small receives (hysteresis : with variable arrivals it shrank to the minimum at the first small receive and grew again at the next burst : a reallocation per event, ~700B per recv)
			uint32_t			m_count_receive_small = 0; // consecutive receives smaller than the shrink size (reset by a larger one or a filled window)

			// - io_uring completion : the executable of the receive op (pooled. holds the socket (add_ref) while an op is in flight).
			//   m_pio_context : the io_context the op was submitted to (its provided buffers go back there : the socket releases its
			//   io_context at the disconnect. alive while it delivers the completions)
			class executable_receiving : virtual public Iexecutable, public Npoolable<executable_receiving>, public factory::_traits_system
			{
			public:
				virtual ~executable_receiving() noexcept {}
				virtual	intptr_t process_execute(intptr_t _result, std::size_t _param) override;
				virtual	void on_final_release() noexcept override { this->m_preceivable_stream = nullptr; this->m_pio_context = nullptr; }
				Nstream*		m_preceivable_stream = nullptr;
				executor::Iio_context* m_pio_context = nullptr;
			};
			// - the zero copy receive : the data of a provided buffer processed in place (net.io.receivable.stream.uring.cpp)
			void				_process_receive_in_place(executor::Iio_context* _pio_context, const void* _pdata, std::size_t _bytes, std::size_t _flags, bool _is_more);

			// - io_uring completion : the receive mode (_process_select_receive_mode). the multishot into the provided buffers while the
			//   arrivals fit in one, a RECV into m_buffer_received (the IOCP sizing of _process_prepare_receive_buffer) when they outgrow it
	static	constexpr uint8_t	RECEIVE_MODE_MULTISHOT = 0;
	static	constexpr uint8_t	RECEIVE_MODE_RECV = 1;
	static	constexpr uint32_t	COUNT_SMALL_RECEIVES_TO_MULTISHOT = 16; // RECVs in a row smaller than half a provided buffer => the multishot again
			uint8_t				m_receive_mode = RECEIVE_MODE_MULTISHOT;
			bool				m_is_receive_mode_switching = false; // the multishot was cancelled for the RECV mode (once : its final completion posts the RECV)
			uint32_t			m_count_receive_small_recv = 0;
			void				_process_select_receive_mode(std::size_t _bytes, std::size_t _required_bytes, bool _is_provided, bool _is_more);
			object_ptr<executable_receiving> m_pexecutable_receiving;
		#endif
};

namespace net::io::receivable
{
	//! @brief '기본 최소 메시지 버퍼 크기'를 설정한다. @param _bytes_size 설정할 '기본 최소 메시지 버퍼 크기'
	void	default_message_buffer_min_size(std::size_t _bytes_size);
	//! @brief '기본 최대 메시지 버퍼 크기'를 설정한다. @param _bytes_size 설정할 '기본 최대 메시지 버퍼 크기'
	void	default_message_buffer_max_size(std::size_t _bytes_size);
	//! @brief '기본 메시지 버퍼 크기'를 설정한다. @param _bytes_size_min 설정할 '기본 최소 메시지 버퍼 크기' @param _bytes_size_max 설정할 '기본 최대 메시지 버퍼 크기'
	void	set_default_packet_buffer_size(std::size_t _bytes_size_min, std::size_t _bytes_size_max);
	//! @brief '기본 최소 메시지 버퍼 크기'를 얻는다. @return 설정된 '기본 최소 메시지 버퍼 크기'
	std::size_t	default_message_buffer_min_size();
	//! @brief '기본 최대 메시지 버퍼 크기'를 얻는다. @return 설정된 '기본 최대 메시지 버퍼 크기'
	std::size_t	default_message_buffer_max_size();
}


}