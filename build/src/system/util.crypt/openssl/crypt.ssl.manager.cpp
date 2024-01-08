#include "pch.h"

#include <openssl/rsa.h>
#include <openssl/crypto.h> 
#include <openssl/x509.h> 
#include <openssl/pem.h> 
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

namespace CGDK
{

crypt::ssl_manager::ssl_manager() noexcept:
	object::Nnameable("ssl manager")
{
}

crypt::ssl_manager::~ssl_manager() noexcept
{
	destroy();
}

void crypt::ssl_manager::initialize_instance()
{
	// lock) 
	std::lock_guard<std::mutex> cs(m_s_lockable_create);

	// check)
	RETURN_IF(m_s_pinstance.exist());
		
	// 1) create ssl_manager
	auto pssl = _make_own_raw<crypt::ssl_manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
		
	// 2) initialize
	pssl->initialize();
		
	// 3) set
	m_s_pinstance = std::move(pssl);
}

void crypt::ssl_manager::set_instance(own_ptr<ssl_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<ssl_manager> pinstance_pre;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void crypt::ssl_manager::reset_instance() noexcept
{
	// declare)
	own_ptr<ssl_manager> pinstance;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

result_code crypt::ssl_manager::initialize()
{
	// lock) 
	std::lock_guard<lockable<>> cs(m_vector_ssl_method);

	// check)
	RETURN_IF(m_vector_ssl_method.empty()==false, eRESULT::NOT_READY);

	// 1) Library를 초기화한다.
	SSL_library_init();

	// 2) SSL 알고리즘들을 추가한다.
	OpenSSL_add_ssl_algorithms();

	// 3) SSL 에러 스트링을 읽어들인다.
	SSL_load_error_strings();

	// 4) BIO
	ERR_load_BIO_strings();

	// 5) SSL Server Method를 얻는다.
	auto pssl_method_server = SSLv23_server_method();

	// check)
	THROW_IF(pssl_method_server == nullptr, throwable::failure(eRESULT::FAIL));

	// 6) SSL Client Method를 얻는다.
	auto pssl_method_client = SSLv23_client_method();

	// check)
	THROW_IF(pssl_method_client == nullptr, throwable::failure(eRESULT::FAIL));

	// 7) 추가
	m_vector_ssl_method.push_back(pssl_method_server);
	m_vector_ssl_method.push_back(pssl_method_client);

	// return) 
	return true;
}

result_code crypt::ssl_manager::destroy() noexcept
{
	// lock) 
	std::lock_guard<lockable<>> cs(m_vector_ssl_method);

	// check)
	RETURN_IF(m_vector_ssl_method.empty(), eRESULT::NOT_READY);

	// 1) State (Deprecated)
	#if 0
	ERR_remove_state(0);
	#endif

	// 2) Cleanup
	ENGINE_cleanup();

	// 3) Unload
	CONF_modules_unload(1);

	// 4) free String
	ERR_free_strings();

	// 5) EVP Cleanup
	EVP_cleanup();

	// 6) ...
	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

	// 7) ...
	CRYPTO_cleanup_all_ex_data();

	// 8) reset
	m_vector_ssl_method.clear();

	// return) 
	return result_code(eRESULT::SUCCESS);
}

// 2) SSL Methods
const SSL_METHOD* crypt::ssl_manager::get_ssl_method(int _index) const noexcept
{
	// lock) 
	std::lock_guard<lockable<>> cs(const_cast<lockable<std::vector<const SSL_METHOD*>>&>(this->m_vector_ssl_method));

	// check)
	RETURN_IF(_index < 0 || static_cast<size_t>(_index) >= m_vector_ssl_method.size(), nullptr);

	// return) 
	return m_vector_ssl_method.at(_index);
}


}