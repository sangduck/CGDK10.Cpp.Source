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
/*!
* @class Ndatagram
*
* @ingroup net::io::receivable
*
* @brief Datagram(UDP)의 저수준 수신을 처리하는 클래스
*
*   UDP용 receive 처리를 정의한 클래스이다. 
*   UDP용 receive는 순서를 고려하지 않고 하나의 socket이 여러 개의 peer로부터 
*   전송을 받기 때문에 최대한 빠르게 다시 receive를 받을 수 있도록 설계되었으며
*   동시에 여러 개의 receive를 요청해 놓는다.
*   저수준 I/O 장치나 API 혹은 처리 방법에 따라 달라질 수 있다.
*
*   receive를 받게될 buffer의 크기를 적절하게 설정해 주어야 한다.
*   만약 receive한 buffer보다 더 큰 Packet이 전달되어 올 경우 Lost되기 때문이다.
*   receive할 버퍼의 크기를 설정할 경우 반드시 bind하기 전에 해주어야 한다.
*   set_receive_buffer_size()함수를 사용하여 receive할 버퍼의 크기를 설정할 수 있다.
*   (Default는 SIZE_OF_RECEIVE_BUFFER)
*/
//-----------------------------------------------------------------------------
class net::io::receivable::Ndatagram : 
	virtual public				net::io::Isocket,
	virtual	public				net::io::Ireceivable_datagram,
	virtual public				net::io::Ipacketable,
	virtual public				net::io::statistics::Nsocket
{
protected:
			Ndatagram() noexcept;
	virtual	~Ndatagram() noexcept {}

public:
	[[nodiscard]] std::size_t	get_message_size() const { return this->m_message_buffer_size;}
			void				set_message_size(std::size_t _message_size);
	[[nodiscard]] std::size_t	get_prepare_receive_count() const;
			void				set_prepare_receive_count(std::size_t _count);
	virtual	void				suspend_receive();
	virtual	void				resume_receive();

protected:
	virtual	void				on_receive( const buffer_view& /*_buffer*/, const net::sockaddr& /*_address*/) {}
	virtual void				on_fail_receive( int /*_error_code*/, const buffer_view& /*_buffer*/, const net::sockaddr& /*_address*/) {}
	virtual	void				on_receive_exhausted() {}

public:
	virtual void				process_on_receive( const buffer_view& _buffer, const net::sockaddr& _address) { on_receive(_buffer, _address); }
	virtual	intptr_t			process_complete_receivable( uintptr_t _result, std::size_t _transferred, shared_buffer& _buffer, const net::sockaddr& _address) override;
	virtual	void				process_reset_receivable() override {}
	virtual	bool				prepare_receive() override;

		#if defined(_WINSOCK2API_)
			class				executable_receive;
		#endif
private:
			std::size_t			m_message_buffer_size;

		#if defined(_WINSOCK2API_)
			std::size_t			m_prepare_receive_count;
		#elif defined(_SYS_EPOLL_H)
			// - recvmmsg : the datagrams received by one syscall (adaptive : a full batch doubles the next one, a partial one sets it)
	static	constexpr int		COUNT_RECEIVE_BATCH_MAX = static_cast<int>(DEFAULT_UDP_COUNT_RECEIVE_BUFFER);
			int					m_count_receive_batch = 1;
		#endif
};


}