//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                Web Classes                                *
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
//
// net::io::Nsocket_ssl
//
//
//-----------------------------------------------------------------------------
class net::io::Nsocket_ssl:
	public						net::io::receivable::tcp::Ndefault,
	public						net::io::sendable::tcp::Ndefault,
	virtual public				net::io::Isender,
	virtual public				Issl_cryptable
{
protected:
	virtual	void				process_reset_receivable() override;
	virtual	void				process_reset_sendable() noexcept override;



	virtual void				process_on_receive( const buffer_view& _buffer, const net::sockaddr& _address) override;
	virtual	intptr_t			process_complete_receivable( intptr_t _result, std::size_t _transferred) override;

	virtual	bool				process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _paddress) override;
	virtual	void				process_complete_sendable(uintptr_t _result, std::size_t _transferred_bytes, std::size_t _transferred_message) override;

public:
	// - (client) 접속 후 SSL handshake를 시작한다. (ClientHello를 전송한다. 이미 시작했으면 할 것이 없다.)
			bool				ssl_start_handshake();
	// - SSL handshake가 _timeout 안에 끝나지 않으면 접속을 끊는다. (handshake를 하지 않고 접속만 붙잡아 두는 것을 막는다.)
			void				start_ssl_handshake_timer(chrono::tick::duration _timeout);
	static constexpr std::chrono::seconds DEFAULT_SSL_HANDSHAKE_TIMEOUT{ 10 };

protected:
	// - 이미 암호화된(또는 handshake) 데이터를 그대로 전송한다. (m_lockable_ssl을 잡고 호출해야 한다.)
			bool				_ssl_send_raw(shared_buffer&& _buffer, std::size_t _count_message);

public:
			buffer_view			m_buffer_decrypt_receiving;
			shared_buffer		m_buffer_decrypt_received;
			
};


}
