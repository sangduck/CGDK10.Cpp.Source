#include "pch.h"

#include <openssl/ssl3.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

namespace CGDK
{

crypt::ssl_manager::ssl_manager() noexcept
{
	this->name("ssl manager"sv);
}

crypt::ssl_manager::~ssl_manager() noexcept
{
	this->destroy();
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
	m_s_pinstance.exchange_release(std::move(pssl));
}

void crypt::ssl_manager::set_instance(own_ptr<ssl_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<ssl_manager> pinstance_pre;

	{
		// lock)
		std::lock_guard<std::mutex> lock(m_s_lockable_create);

		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void crypt::ssl_manager::reset_instance() noexcept
{
	// declare)
	own_ptr<ssl_manager> pinstance;

	{
		// lock)
		std::lock_guard<std::mutex> lock(m_s_lockable_create);

		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

result_code crypt::ssl_manager::initialize()
{
	// lock) 
	std::lock_guard cs(this->m_lockable_vector_ssl_method);

	// check)
	RETURN_IF(this->m_vector_ssl_method.empty()==false, eRESULT::NOT_READY);

	// 1) Library를 초기화한다.
	SSL_library_init();

	// 2) SSL 알고리즘들을 추가한다.
	OpenSSL_add_ssl_algorithms();

	// 3) SSL 에러 스트링을 읽어들인다.
	SSL_load_error_strings();

	// 4) SSL Server Method를 얻는다.
	auto pssl_method_server = SSLv23_server_method();

	// check)
	THROW_IF(pssl_method_server == nullptr, throwable::failure(eRESULT::FAIL));

	// 5) SSL Client Method를 얻는다.
	auto pssl_method_client = SSLv23_client_method();

	// check)
	THROW_IF(pssl_method_client == nullptr, throwable::failure(eRESULT::FAIL));

	// 7) 추가
	this->m_vector_ssl_method.push_back(pssl_method_server);
	this->m_vector_ssl_method.push_back(pssl_method_client);

	// return) 
	return true;
}

result_code crypt::ssl_manager::destroy() noexcept
{
	// lock) 
	std::lock_guard cs(this->m_lockable_vector_ssl_method);

	// check)
	RETURN_IF(this->m_vector_ssl_method.empty(), eRESULT::NOT_READY);

	// - OpenSSL 1.1.0 or later initializes and cleans itself up (OPENSSL_cleanup() at exit).
	//   calling the cleanup functions here frees its internal data twice and crashes at exit.
	//   (ex. sk_SSL_COMP_free(SSL_COMP_get_compression_methods())) so they are called only for older versions.
#if OPENSSL_VERSION_NUMBER < 0x10100000L
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
#endif

	// 8) reset
	this->m_vector_ssl_method.clear();

	// return) 
	return result_code(eRESULT::SUCCESS);
}

// 2) SSL Methods
const SSL_METHOD* crypt::ssl_manager::get_ssl_method(int _index) const noexcept
{
	// lock) 
	std::lock_guard lock(this->m_lockable_vector_ssl_method);

	// check)
	RETURN_IF(_index < 0 || static_cast<size_t>(_index) >= this->m_vector_ssl_method.size(), nullptr);

	// return) 
	return this->m_vector_ssl_method.at(_index);
}


}