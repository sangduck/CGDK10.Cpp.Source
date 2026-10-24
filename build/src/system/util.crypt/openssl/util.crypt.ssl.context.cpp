#include "pch.h"

#include <openssl/ssl3.h>
#include <openssl/err.h>
#include <filesystem>

namespace CGDK
{

// internal helpers
namespace
{
	// - 공유 client context
	std::mutex						g_lockable_default_client;
	object_ptr<crypt::ssl_context>	g_pdefault_client;
	std::string						g_default_client_ca_file;

	// - OpenSSL error queue에서 마지막 오류 문자열을 얻는다. (thread safe buffer 사용)
	std::string _get_openssl_error_string()
	{
		char buffer[256]{};
		ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
		ERR_clear_error();
		return std::string(buffer);
	}

	// - wide 문자열 경로를 UTF-8로 바꾼다. (OpenSSL은 Windows에서 UTF-8 경로를 받는다.)
	std::string _to_utf8_path(std::wstring_view _path)
	{
		auto u8 = std::filesystem::path(_path).u8string();
		return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
	}
}

crypt::ssl_context::ssl_context() noexcept : m_pssl_context(nullptr)
{
}

crypt::ssl_context::ssl_context(const std::string_view _pem_cert, const std::string_view _pem_private_key) : m_pssl_context(nullptr)
{
	this->initialize_server(_pem_cert, _pem_private_key);
}

crypt::ssl_context::ssl_context(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key) : m_pssl_context(nullptr)
{
	this->initialize_server(_pem_cert, _pem_private_key);
}

crypt::ssl_context::~ssl_context() noexcept
{
	// - SSL_CTX를 해제한다.
	this->destroy();
}

void crypt::ssl_context::initialize_server(const std::string_view _pem_cert, const std::string_view _pem_private_key)
{
	// declare)
	SSL_CTX* pssl_context{ nullptr };

	// - string_view는 NUL로 끝나지 않을 수 있으므로 복사한다.
	const std::string file_cert(_pem_cert);
	const std::string file_private_key(_pem_private_key);

	try
	{
		// 1) ...
		object_ptr<crypt::ssl_manager>	pmanager_ssl = crypt::ssl_manager::instance();

		// check)
		if(pmanager_ssl.empty())
			throw throwable::failure(eRESULT::FAIL, 0, "ssl manager is nullptr"sv);

		// 2) 여기서는 Server용 SSL Method들을 얻는다. (0:Server, 1:Client)
		auto pssl_method = pmanager_ssl->get_ssl_method(0);

		// check)
		CGASSERT_ERROR(pssl_method != nullptr);

		// 3) 새로운 Context를 생성한다.
		pssl_context = SSL_CTX_new(pssl_method);

		// check) 생성에 실패하면 Exception을 던진다.
		if(pssl_context == nullptr)
			throw throwable::failure(eRESULT::FAIL, 0, _get_openssl_error_string());

		// 4) SSL Context를 설정한다. (TLS 1.2 이상만 허용한다.)
		SSL_CTX_set_ecdh_auto(pssl_context, 1);
		SSL_CTX_set_min_proto_version(pssl_context, TLS1_2_VERSION);
		SSL_CTX_set_options(pssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE | SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_RENEGOTIATION);

		// 5) Certi 파일을 읽어들인다. (leaf + 중간 CA 인증서 chain)
		int result = SSL_CTX_use_certificate_chain_file(pssl_context, file_cert.c_str());

		// check)
		if(result != 1)
			throw throwable::failure(eRESULT::FAIL_TO_USE_SSL_CERTIFICATION_FILE, 0, _get_openssl_error_string());

		// 6) Private Key 파일을 읽어들인다.
		result = SSL_CTX_use_PrivateKey_file(pssl_context, file_private_key.c_str(), SSL_FILETYPE_PEM);

		// check)
		if(result != 1)
			throw throwable::failure(eRESULT::FAIL_TO_USE_SSL_PRIVATE_KEY, 0, _get_openssl_error_string());

		// 7) 확인한다.
		result = SSL_CTX_check_private_key(pssl_context);

		// check)
		if(result != 1)
			throw throwable::failure(eRESULT::FAIL_TO_CHECK_SSL_PUBLIC_KEY, 0, _get_openssl_error_string());

		// 8) 기존 context를 해제한다.
		this->destroy();

		// 9) 최종 설정한다.
		this->m_pssl_context = pssl_context;
		this->m_file_cert = file_cert;
		this->m_file_private_key = file_private_key;
		this->m_pmanager = std::move(pmanager_ssl);
	}
	catch(...)
	{
		if(pssl_context != nullptr)
			SSL_CTX_free(pssl_context);

		// rerthrow)
		throw;
	}
}

void crypt::ssl_context::initialize_server(const std::wstring_view _pem_cert, const std::wstring_view _pem_private_key)
{
	initialize_server(_to_utf8_path(_pem_cert), _to_utf8_path(_pem_private_key));
}

void crypt::ssl_context::initialize_client(const std::string_view _ca_file)
{
	// declare)
	SSL_CTX* pssl_context = nullptr;

	try
	{
		// 1) ...
		auto pmanager_ssl = crypt::ssl_manager::instance();

		// check)
		if(pmanager_ssl.empty())
			throw throwable::failure(eRESULT::FAIL, 0, "ssl manager is nullptr"sv);

		// 2) 여기서는 Client용 SSL Method들을 얻는다. (0:Server, 1:Client)
		auto pssl_method = pmanager_ssl->get_ssl_method(1);

		// check)
		if(pssl_method == nullptr)
			throw throwable::failure(eRESULT::FAIL, 0, "ssl method is nullptr"sv);

		// 3) 새로운 Context를 생성한다.
		pssl_context = SSL_CTX_new(pssl_method);

		// check) 생성에 실패하면 Exception을 던진다.
		if(pssl_context == nullptr)
			throw throwable::failure(eRESULT::FAIL, 0, _get_openssl_error_string());

		// 4) TLS 1.2 이상만 허용한다.
		SSL_CTX_set_min_proto_version(pssl_context, TLS1_2_VERSION);
		SSL_CTX_set_options(pssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_RENEGOTIATION);

		// 5) 서버 인증서를 검증한다. (검증에 실패하면 handshake가 실패한다.)
		SSL_CTX_set_verify(pssl_context, SSL_VERIFY_PEER, nullptr);

		// 6) 신뢰할 CA를 읽어 들인다.
		int count_ca_loaded = 0;

		//    - OpenSSL 기본 경로 (SSL_CERT_FILE/SSL_CERT_DIR 환경변수, linux의 /etc/ssl/certs 등)
		if (SSL_CTX_set_default_verify_paths(pssl_context) == 1)
			++count_ca_loaded;

	#if defined(_WIN32)
		//    - Windows 인증서 저장소 (OpenSSL 3.2 이상)
		if (SSL_CTX_load_verify_store(pssl_context, "org.openssl.winstore://") == 1)
			++count_ca_loaded;
	#endif

		//    - 지정한 CA 파일 (PEM)
		if (!_ca_file.empty())
		{
			const std::string ca_file(_ca_file);

			// check) 지정한 CA 파일은 반드시 읽혀야 한다.
			if(SSL_CTX_load_verify_locations(pssl_context, ca_file.c_str(), nullptr) != 1)
				throw throwable::failure(eRESULT::FAIL_TO_USE_SSL_CERTIFICATION_FILE, 0, _get_openssl_error_string());

			++count_ca_loaded;
		}

		// - 위에서 실패한 항목의 error는 지운다.
		ERR_clear_error();

		// check)
		if(count_ca_loaded == 0)
			throw throwable::failure(eRESULT::FAIL, 0, "no trusted CA (set SSL_CERT_FILE or ca_file)"sv);

		// 7) 기존 context를 해제한다.
		this->destroy();

		// 8) 최종 설정한다.
		this->m_pssl_context = pssl_context;
		this->m_pmanager = std::move(pmanager_ssl);
	}
	catch(...)
	{
		if(pssl_context != nullptr)
			SSL_CTX_free(pssl_context);

		// rerthrow)
		throw;
	}
}

object_ptr<crypt::ssl_context> crypt::ssl_context::get_default_client()
{
	// 설명) client용 SSL_CTX는 연결마다 만들지 않고 하나를 공유한다.
	//       (SSL_CTX는 thread safe하며, CA 저장소를 읽는 비용이 크다.)
	std::lock_guard cs(g_lockable_default_client);

	// - 처음 사용할 때 만든다. (실패하면 다음 요청에서 다시 시도한다.)
	if (g_pdefault_client.empty())
	{
		auto pcontext = make_object<crypt::ssl_context>();
		pcontext->initialize_client(g_default_client_ca_file);
		g_pdefault_client = std::move(pcontext);
	}

	// return)
	return g_pdefault_client;
}

void crypt::ssl_context::set_default_client_ca_file(std::string_view _ca_file)
{
	std::lock_guard cs(g_lockable_default_client);

	// - 다음 get_default_client()에서 새로 만든다. (이미 연결 중인 SSL은 기존 context를 계속 사용한다.)
	g_default_client_ca_file = _ca_file;
	g_pdefault_client.reset();
}

result_code crypt::ssl_context::destroy() noexcept
{
	// 1) destroy...
	if (this->m_pssl_context != nullptr)
	{
		SSL_CTX_free(this->m_pssl_context);
		this->m_pssl_context = nullptr;
	}

	// 2) reset
	this->m_pmanager.reset();

	// return)
	return result_code(eRESULT::SUCCESS);
}

object_ptr<crypt::ssl> crypt::ssl_context::alloc_ssl(int _handle)
{
	// check) 초기화되지 않은 context
	if(this->m_pssl_context == nullptr)
		throw throwable::failure(eRESULT::NOT_READY, 0, "ssl context is not initialized"sv);

	// 1)
	auto pcrypt = make_object<crypt::ssl>();

	// 2) 신규 SSL을 생성한다.
	SSL* pssl = SSL_new(this->m_pssl_context);

	// check) 실패했을 경우 Exception을 던진다.
	if(pssl == nullptr)
		throw throwable::failure(eRESULT::FAIL, 0, _get_openssl_error_string());

	// 3) FD를 설정...
	auto result = SSL_set_fd(pssl, (int)_handle);

	// check) 실패했을 경우 SSL을 해제하고 Exception을 던진다.
	if (result != 1)
	{
		SSL_free(pssl);
		throw throwable::failure(eRESULT::FAIL, 0, _get_openssl_error_string());
	}

	// 4) 값을 설정한다.
	pcrypt->m_ssl = pssl;
	pcrypt->m_pbio_receive = nullptr;
	pcrypt->m_pbio_send = nullptr;
	pcrypt->m_pssl_context = this;

	// return)
	return pcrypt;
}


}
