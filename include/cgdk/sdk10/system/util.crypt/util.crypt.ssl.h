//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
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
// CGDK::crypt::ssl
//
//
//-----------------------------------------------------------------------------
namespace crypt
{

class ssl :
	virtual public				object::Idestroyable
{
public:
			ssl() noexcept;
	virtual ~ssl() noexcept;

public:
	virtual	void				initialize_handshake_server(Issl_cryptable* _pcryptable_ssl);
	virtual	void				initialize_handshake_client(Issl_cryptable* _pcryptable_ssl);
			// - client: 접속할 서버 이름 (SNI + 인증서 hostname/IP 검증). handshake 전에 설정한다.
			void				set_host_name(std::string_view _host_name);
	virtual	result_code			destroy() noexcept override;

	virtual	bool				encode(buffer_view& _dest, const const_buffer_view* _source, int _count);
	virtual	bool				decode(buffer_view& _dest, const_buffer_view* _source, int _count);

	// 설명) 비동기 handshake (memory BIO)
	//       - handshake는 socket을 직접 읽지 않고 수신한 데이터로 진행한다. (io thread를 막지 않는다.)
	//       - 아래 함수들이 돌려주는 buffer(_send_out)는 이미 암호화된(또는 handshake) 데이터이므로
	//         그대로 socket으로 전송해야 한다. (비어 있으면 보낼 것이 없다.)
	//       - 호출하는 쪽에서 Issl_cryptable::m_lockable_ssl을 잡고 호출하고 전송까지 lock 안에서 해야 한다.
			// - 평문을 암호화한다. handshake 중이면 평문을 보관했다가 handshake가 끝나면 암호화한다. (실패하면 exception)
	[[nodiscard]] shared_buffer	encode_buffer(const buffer_view& _source);
			// - 받은 데이터를 넣고 handshake/복호화를 진행한다. 복호화한 평문은 _dest에, 보낼 데이터는 _send_out에 돌려준다.
			//   (_source는 모두 사용된다. 실패하면 exception)
			bool				process_receive(buffer_view& _dest, const_buffer_view& _source, shared_buffer& _send_out);
			// - (client) handshake를 시작한다. (ClientHello를 돌려준다.)
	[[nodiscard]] shared_buffer	start_handshake();
	[[nodiscard]] bool			is_handshake_completed() const noexcept;

public:
			SSL*				m_ssl;
			BIO*				m_pbio_receive;
			BIO*				m_pbio_send;

			object_ptr<crypt::ssl_context>	m_pssl_context;

private:
			void				_do_handshake();
			void				_flush_pending_send();
			shared_buffer		_read_send_bio();

			// - handshake가 끝나기 전에 보내려고 한 평문
			std::vector<shared_buffer> m_vector_pending_send;
};
}

class Issl_cryptable
{
public:
	// - m_pssl_cryptor(SSL 객체)는 송신/수신 thread에서 동시에 사용되므로
	//   encode/decode와 m_pssl_cryptor의 설정/해제는 m_lockable_ssl을 잡고 수행한다.
	object_ptr<crypt::ssl> m_pssl_cryptor;
	mutable lock_object	m_lockable_ssl;

	[[nodiscard]] object_ptr<crypt::ssl> get_ssl_cryptor() const { std::lock_guard cs(this->m_lockable_ssl); return this->m_pssl_cryptor; }
	void				set_ssl_cryptor(object_ptr<crypt::ssl> _pcryptor) { std::lock_guard cs(this->m_lockable_ssl); this->m_pssl_cryptor = std::move(_pcryptor); }
	void				reset_ssl_cryptor() noexcept { object_ptr<crypt::ssl> ptemp; { std::lock_guard cs(this->m_lockable_ssl); ptemp = std::move(this->m_pssl_cryptor); } }
};

}
