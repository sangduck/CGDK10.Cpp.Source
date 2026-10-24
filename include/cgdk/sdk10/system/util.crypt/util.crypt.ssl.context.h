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
// CGDK::crypt::ssl_context
//
//-----------------------------------------------------------------------------
namespace crypt
{

class ssl_context : virtual public object::Idestroyable
{
public:
			ssl_context() noexcept;
			ssl_context(const std::string_view _pem_cert, const std::string_view _pem_private_key);
			ssl_context(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key);
	virtual ~ssl_context() noexcept;

public:
			// - server: _pem_cert can contain the full chain (leaf + intermediate CA certificates)
			void				initialize_server(const std::string_view _pem_cert, const std::string_view _pem_private_key);
			void				initialize_server(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key);
			// - client: verify server certificate (system CA store + _ca_file(PEM, optional))
			void				initialize_client(const std::string_view _ca_file = {});
			// - shared client context (created once, verify server certificate with system CA store)
	[[nodiscard]] static object_ptr<ssl_context> get_default_client();
			// - 공유 client context가 추가로 신뢰할 CA 파일(PEM). (사설 CA, 테스트용 자체 서명 인증서 등. 다음 연결부터 적용된다.)
	static	void				set_default_client_ca_file(std::string_view _ca_file);
	virtual	result_code			destroy() noexcept override;

			object_ptr<ssl>		alloc_ssl(int _handle);
	[[nodiscard]] SSL_CTX*		get_ssl_context() const noexcept { return m_pssl_context; }
	[[nodiscard]] std::string_view get_filename_cert() const noexcept { return m_file_cert; }
	[[nodiscard]] std::string_view get_filename_private_key() const noexcept { return m_file_private_key; }

private:
			SSL_CTX*			m_pssl_context{nullptr};
			std::string			m_file_cert;
			std::string			m_file_private_key;

			object_ptr<ssl_manager>	m_pmanager;
};


}
}
