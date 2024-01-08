#include "pch.h"


namespace CGDK
{

crypt::ssl_context::ssl_context() noexcept : m_pssl_context(nullptr)
{
}

crypt::ssl_context::ssl_context(const std::string_view _pem_cert, const std::string_view _pem_private_key) : m_pssl_context(nullptr)
{
	initialize_server(_pem_cert, _pem_private_key);
}

crypt::ssl_context::ssl_context(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key) : m_pssl_context(nullptr)
{
	initialize_server(_pem_cert, _pem_private_key);
}

crypt::ssl_context::~ssl_context() noexcept
{
}

void crypt::ssl_context::initialize_server(const std::string_view _pem_cert, const std::string_view _pem_private_key)
{
	// declare)  
	SSL_CTX* pssl_context = nullptr;

	try
	{
		// 1) ...
		object_ptr<crypt::ssl_manager>	pmanager_ssl = crypt::ssl_manager::instance();

		// check) 
		THROW_IF(pmanager_ssl.empty(), throwable::failure(eRESULT::FAIL));

		// 2) 여기서는 Server용 SSL Method들을 얻는다. (0:Server, 1:Client)
		auto pssl_method = pmanager_ssl->get_ssl_method(0);

		// check)
		CGASSERT_ERROR(pssl_method!=nullptr);

		// 3) 새로운 Context를 생성한다.
		pssl_context = SSL_CTX_new(pssl_method);

		// check) 생성에 실패하면 Exception을 던진다.
		THROW_IF(pssl_context==nullptr, throwable::failure(eRESULT::FAIL));

		// declare)
		int	result;

		// 4) SSL Context를 설정한다.
		SSL_CTX_set_ecdh_auto(pssl_context, 1);
		SSL_CTX_set_options(pssl_context, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

		// 5) Certi 파일을 읽어들인다.
		result = SSL_CTX_use_certificate_file(pssl_context, _pem_cert.data(), SSL_FILETYPE_PEM);

		// check) 
		THROW_IF(result != 1, throwable::failure(eRESULT::FAIL));

		// 6) Private Key 파일을 읽어들인다.
		result	 = SSL_CTX_use_PrivateKey_file(pssl_context, _pem_private_key.data(), SSL_FILETYPE_PEM);

		// check) 
		THROW_IF(result != 1, throwable::failure(eRESULT::FAIL));

		// 7) 확인한다.
		result	 = SSL_CTX_check_private_key(pssl_context);

		// check) 
		THROW_IF(result != 1 , throwable::failure(eRESULT::FAIL));

		// 6) 최종 설정한다.
		m_pssl_context	 = std::move(pssl_context);
		m_file_cert		 = _pem_cert;
		m_file_private_key = _pem_private_key;
		m_pmanager		 = std::move(pmanager_ssl);
	}
	catch(...)
	{
		if(pssl_context != nullptr)
		{
			SSL_CTX_free(pssl_context);
		}

		m_pmanager.reset();
	}
}

void crypt::ssl_context::initialize_server(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key)
{
	// declare) 
	char str_cert[512];
	char str_private_key[512];

	// 1) ....
	initialize_server(encode_string(str_cert, _pem_cert.data(), _pem_cert.size()), encode_string(str_private_key, _pem_private_key.data(), _pem_private_key.size()));
}

void crypt::ssl_context::initialize_client()
{
	// declare)  
	SSL_CTX* pssl_context = nullptr;

	try
	{
		// 1) ...
		auto pmanager_ssl = crypt::ssl_manager::instance();

		// check) 
		THROW_IF(pmanager_ssl.empty(), throwable::failure(eRESULT::FAIL));

		// 2) 여기서는 Server용 SSL Method들을 얻는다. (0:Server, 1:Client)
		auto pssl_method = pmanager_ssl->get_ssl_method(1);

		// check)
		CGASSERT_ERROR(pssl_method!=nullptr);

		// 3) 새로운 Context를 생성한다.
		pssl_context = SSL_CTX_new(pssl_method);

		// check) 생성에 실패하면 Exception을 던진다.
		THROW_IF(pssl_context == nullptr, throwable::failure(eRESULT::FAIL));

		// 4) Disabling SSLv3 will leave v2 and TSLv1 for negotiation
		SSL_CTX_set_options(pssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

		// 6) 최종 설정한다.
		m_pssl_context = std::move(pssl_context);
		m_pmanager = std::move(pmanager_ssl);
	}
	catch(...)
	{
		if(pssl_context != nullptr)
		{
			SSL_CTX_free(pssl_context);
		}

		m_pmanager.reset();
	}
}


result_code crypt::ssl_context::destroy() noexcept
{
	// 1) destroy...
	if (m_pssl_context != nullptr)
	{
		SSL_CTX_free(m_pssl_context);
	}

	// 2) reset
	m_pmanager.reset();

	// return)
	return result_code(eRESULT::SUCCESS);
}

object_ptr<crypt::ssl> crypt::ssl_context::alloc_ssl(int _handle)
{
	// 1) 
	auto pcrypt = make_object<crypt::ssl>();

	// 2) 
	// 1) 신규 SSL을 생성한다.
	SSL* pSSL = SSL_new(m_pssl_context);

	// check) 실패했을 경우 Exception을 던진다.
	THROW_IF(pSSL == nullptr, throwable::failure(eRESULT::FAIL));

	// 2) FD를 설정...
	auto result = SSL_set_fd(pSSL, (int)_handle);

	// check) 실패했을 경우 Exception을 던진다.
	THROW_IF(result != 1, throwable::failure(eRESULT::FAIL));

	// 3) 값을 설정한다.
	pcrypt->m_pSSL		   = pSSL;
	pcrypt->m_pbio_receive = nullptr;
	pcrypt->m_pbio_send	   = nullptr;
	pcrypt->m_pssl_context = this;

	// return)
	return	pcrypt;
}


}