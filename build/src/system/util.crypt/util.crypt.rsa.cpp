#include "pch.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
# include <openssl/bio.h>

namespace CGDK::crypt::rsa
{
	// - OpenSSL error queue에서 오류 문자열을 얻는다. (thread safe buffer)
	static std::string _get_openssl_error_string()
	{
		char buffer[256]{};
		ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
		return std::string(buffer);
	}

	// - RSA 암호화 padding: OAEP (SHA-256, MGF1-SHA-256)
	//   . PKCS#1 v1.5 padding은 padding oracle 공격(Bleichenbacher)에 취약하므로 사용하지 않는다.
	//   . 한번에 암호화할 수 있는 최대 크기 = key bytes - 66 (2048bit: 190 bytes)
	static void _set_oaep_padding(EVP_PKEY_CTX* _ctx)
	{
		THROW_IF(EVP_PKEY_CTX_set_rsa_padding(_ctx, RSA_PKCS1_OAEP_PADDING) <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to setting rsa oaep padding: " + _get_openssl_error_string()));
		THROW_IF(EVP_PKEY_CTX_set_rsa_oaep_md(_ctx, EVP_sha256()) <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to setting rsa oaep md: " + _get_openssl_error_string()));
		THROW_IF(EVP_PKEY_CTX_set_rsa_mgf1_md(_ctx, EVP_sha256()) <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to setting rsa mgf1 md: " + _get_openssl_error_string()));
	}

    // RAII 클래스
	class safe_BIO
	{
	public:
		BIO* v;

		safe_BIO(BIO* _v) noexcept : v(_v) {}
		safe_BIO(const safe_BIO&) = delete;
		safe_BIO& operator=(const safe_BIO&) = delete;
		~safe_BIO() noexcept
		{
			if (v)
				BIO_free(v);
		}

		bool empty() const noexcept
		{
			return v == nullptr;
		}
		void reset() noexcept
		{
			if (v == nullptr)
				return;

			BIO_free(v);
			v = nullptr;
		}
	};

	class safe_EVP_PKEY
	{
	public:
		EVP_PKEY* v;

		safe_EVP_PKEY(EVP_PKEY* _v) noexcept : v(_v) {}
		// - 복사하면 같은 EVP_PKEY를 두 번 해제하므로 복사를 막고 이동만 허용한다.
		safe_EVP_PKEY(const safe_EVP_PKEY&) = delete;
		safe_EVP_PKEY& operator=(const safe_EVP_PKEY&) = delete;
		safe_EVP_PKEY(safe_EVP_PKEY&& _rhs) noexcept : v(_rhs.v) { _rhs.v = nullptr; }
		EVP_PKEY* release() noexcept { auto p = v; v = nullptr; return p; }
		~safe_EVP_PKEY() noexcept
		{
			if (v)
				EVP_PKEY_free(v);
		}

		bool empty() const noexcept
		{
			return v == nullptr;
		}
	};

	class safe_EVP_PKEY_CTX
	{
	public:
		safe_EVP_PKEY_CTX() noexcept {}
		safe_EVP_PKEY_CTX(EVP_PKEY_CTX* _v) noexcept : m_value(_v) {}
		safe_EVP_PKEY_CTX(const safe_EVP_PKEY_CTX&) = delete;
		safe_EVP_PKEY_CTX& operator=(const safe_EVP_PKEY_CTX&) = delete;
		~safe_EVP_PKEY_CTX() noexcept { this->reset(); }

		bool empty() const noexcept { return this->m_value == nullptr; }
		auto get() const noexcept { return this->m_value; }
		void reset() noexcept { if (this->m_value == nullptr) return; EVP_PKEY_CTX_free(this->m_value); this->m_value = nullptr; }

	private:
		EVP_PKEY_CTX* m_value{ nullptr };
	};

	class safe_BIGNUM
	{
	public:
		BIGNUM* v;

		safe_BIGNUM(BIGNUM* _v = nullptr) noexcept : v(_v) {}
		safe_BIGNUM(const safe_BIGNUM&) = delete;
		safe_BIGNUM& operator=(const safe_BIGNUM&) = delete;
		~safe_BIGNUM() noexcept
		{
			if (v)
				BN_free(v);
		}

		bool empty() const noexcept
		{
			return v == nullptr;
		}
		void reset() noexcept
		{
			if (v == nullptr)
				return;

			BN_free(v);
			v = nullptr;
		}
	};

	safe_EVP_PKEY create_evp_pkey_by_pem(const public_key_t& _rsa_key)
	{
		// 1) make BIO
		safe_BIO bio = BIO_new_mem_buf(_rsa_key.key.c_str(), -1);

		// check) 
		THROW_IF(bio.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating BIO"sv));

		// 2) read public key
		safe_EVP_PKEY pkey = PEM_read_bio_PUBKEY(bio.v, NULL, NULL, NULL);

		// check)
		THROW_IF(pkey.empty(), throwable::failure(eRESULT::FAIL, 0, CGDK::stream_string("fail to loading rsa public key: ", _get_openssl_error_string())));

		// return)
		return pkey;
	}

	safe_EVP_PKEY create_evp_pkey_by_pem(const private_key_t& _rsa_key)
	{
		// 1) make BIO
		safe_BIO bio = BIO_new_mem_buf(_rsa_key.key.c_str(), -1);

		// check) 
		THROW_IF(bio.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating BIO"sv));

		// 2) read public key
		safe_EVP_PKEY pkey = PEM_read_bio_PrivateKey(bio.v, NULL, NULL, NULL);

		// check)
		THROW_IF(pkey.empty(), throwable::failure(eRESULT::FAIL, 0, CGDK::stream_string("fail to loading rsa private key: ", _get_openssl_error_string())));

		// return)
		return pkey;
	}

	[[nodiscard]] private_key_t generate_private_key(int _bits_key_length)
	{
		// check) 최소 2048 bit (512/1024 bit는 안전하지 않고, OAEP(SHA-256)로 암호화할 수 있는 크기가 너무 작다.)
		THROW_IF(_bits_key_length < 2048, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "rsa key length must be at least 2048 bits"sv));

		// check) maximum key lengths in bit is 384
		CGASSERT_ERROR(_bits_key_length <= 15360);

		// declare) 
		safe_EVP_PKEY pkey{ nullptr };

		// 1) make PKEY
		{
			// - Create EVP_PKEY_CTX for key generation
			safe_EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);

			// check) 
			THROW_IF(ctx.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating EVP_PKEY_CTX: " + _get_openssl_error_string()));

			// - Initialize key generation context
			auto result = EVP_PKEY_keygen_init(ctx.get());

			// check) 
			THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to initializing keygen context: " + _get_openssl_error_string()));

			// - Set key length
			result = EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), _bits_key_length);

			// check) 
			THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to setting keygen parameters: " + _get_openssl_error_string()));

			// - Generate the key
			result = EVP_PKEY_keygen(ctx.get(), &pkey.v);

			// check) 
			THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to generating RSA key: " + _get_openssl_error_string()));
		}

		// 2) Create BIO for private key
		safe_BIO bio = BIO_new(BIO_s_mem());

		// check)
		THROW_IF(bio.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating BIO"sv));

		// 3) Write private key to BIO
		auto result = PEM_write_bio_PrivateKey(bio.v, pkey.v, nullptr, nullptr, 0, nullptr, nullptr);

		// check) 
		THROW_IF(!result, throwable::failure(eRESULT::FAIL, 0, "fail to writing private key: " + _get_openssl_error_string()))

		// 4) Get private key as string
		char* pem_data = nullptr;
		long pem_length = BIO_get_mem_data(bio.v, &pem_data);

		// return) Return private key
		return private_key_t{ std::string(pem_data, pem_length) };
	}

	[[nodiscard]] public_key_t generate_public_key(const private_key_t& _private_key)
	{
		// 1) Create EVP_PKEY from private key
		safe_EVP_PKEY pkey = create_evp_pkey_by_pem(_private_key);

		// 2) Create BIO for public key
		safe_BIO bio = BIO_new(BIO_s_mem());

		// check)
		THROW_IF(bio.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating BIO"sv));

		// 3) Write public key to BIO
		auto result = PEM_write_bio_PUBKEY(bio.v, pkey.v);

		// check) 
		THROW_IF(!result, throwable::failure(eRESULT::FAIL, 0, "fail to writing public key: " + _get_openssl_error_string()));

		// 4) Get public key as string
		char* pem_data{ nullptr };
		long pem_length = BIO_get_mem_data(bio.v, &pem_data);

		// return)
		return public_key_t{ std::string(pem_data, pem_length) };
	}

	key_pair_t generate_key_pair(int _key_length)
	{
		auto private_key = generate_private_key(_key_length);
		auto public_key = generate_public_key(private_key);
		return key_pair_t{ std::move(public_key), std::move(private_key) };
	}

	template<class TKEY>
	shared_buffer _encrypt_rsa(const TKEY& _rsa_key, buffer_view _buf_source)
	{
		// 1) create EVP_PKEY
		auto evp_pkey = create_evp_pkey_by_pem(_rsa_key);

		// check) 
		CGASSERT_ERROR(!evp_pkey.empty());

		// 2) EVP_PKEY_CTX 생성
		safe_EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new(evp_pkey.v, nullptr);

		// check) 
		THROW_IF(ctx.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating EVP_PKEY_CTX"sv));

		// 3) 복호화 초기화
		auto result = EVP_PKEY_encrypt_init(ctx.get());

		// check)	
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to initializing encryption"sv));

		// - padding (OAEP)
		_set_oaep_padding(ctx.get());

		// declare) [필요 최대 버퍼 크기]
		size_t out_len{ 0 };

		// 4) [필요 최대 버퍼 크기]를 먼저 구한다.
		result = EVP_PKEY_encrypt(ctx.get(), nullptr, &out_len, _buf_source.data<unsigned char>(), _buf_source.size());

		// check)
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to determining buffer length for encryption"sv));

		// 5) [필요한 최대 버퍼 크기] 만큼 buf_encrypted를 할당한다.
		auto buf_encrypted = alloc_shared_buffer(out_len);

		// 6) [암호화된 데이터]에 buf_encrypted를 쓴다.
		result = EVP_PKEY_encrypt(ctx.get(), reinterpret_cast<unsigned char*>(buf_encrypted.data()), &out_len, _buf_source.data<unsigned char>(), _buf_source.size());

		// check) 
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to during encryption: " + _get_openssl_error_string()));

		// 7) [복호화된 데이터 크기]를 buf_encrypted의 크기를 설정한다.
		buf_encrypted.set_size(out_len);

		// return)
		return buf_encrypted;
	}

	template<class TKEY>
	shared_buffer _decrypt_rsa(const TKEY& _rsa_key, buffer_view _buf_source)
	{
		// 1) create EVP_PKEY
		auto evp_pkey = create_evp_pkey_by_pem(_rsa_key);

		// check) 
		CGASSERT_ERROR(!evp_pkey.empty());

		// 2) EVP_PKEY_CTX 생성
		safe_EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new(evp_pkey.v, nullptr);

		// check) 
		THROW_IF(ctx.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating EVP_PKEY_CTX"sv));

		// 3) 복호화 초기화
		auto result = EVP_PKEY_decrypt_init(ctx.get());

		// check)	
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to initializing decryption"sv));

		// - padding (OAEP)
		_set_oaep_padding(ctx.get());

		// declare) [필요 최대 버퍼 크기]
		size_t out_len{ 0 };

		// 4) [필요 최대 버퍼 크기]를 먼저 구한다.
		result = EVP_PKEY_decrypt(ctx.get(), nullptr, &out_len, _buf_source.data<unsigned char>(), _buf_source.size());

		// check)
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to determining buffer length for decryption"sv));

		// 5) [필요한 최대 버퍼 크기] 만큼 buf_decrypted를 할당한다.
		auto buf_decrypted = alloc_shared_buffer(out_len);

		// 6) [복호화된 데이터]에 buf_decrypt를 쓴다.
		result = EVP_PKEY_decrypt(ctx.get(), reinterpret_cast<unsigned char*>(buf_decrypted.data()), &out_len, _buf_source.data<unsigned char>(), _buf_source.size());

		// check) 
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to during decryption: " + _get_openssl_error_string()));

		// 7) [복호화된 데이터 크기]를 buf_decrypted의 크기를 설정한다.
		buf_decrypted.set_size(out_len);

		// return)
		return buf_decrypted;
	}

	shared_buffer encrypt(const public_key_t& _rsa_key, buffer_view _buf_source)
	{
		return _encrypt_rsa(_rsa_key, _buf_source);
	}

	bool encrypt(buffer& _buf_dest, const public_key_t& _rsa_key, buffer_view _buf_source)
	{
		try
		{
			// 1) decrypt
			auto buf_result = _encrypt_rsa(_rsa_key, _buf_source);
			_buf_dest.append(buf_result.size(), buf_result.data());
		}
		catch (...)
		{
			return false;
		}

		// return)
		return true;
	}

	shared_buffer encrypt(const private_key_t& _rsa_key, buffer_view _buf_source)
	{
		return _encrypt_rsa(_rsa_key, _buf_source);
	}

	bool encrypt(buffer& _buf_dest, const private_key_t& _rsa_key, buffer_view _buf_source)
	{
		try
		{
			// 1) decrypt
			auto buf_result = _encrypt_rsa(_rsa_key, _buf_source);
			_buf_dest.append(buf_result.size(), buf_result.data());
		}
		catch (...)
		{
			return false;
		}

		// return)
		return true;
	}

	shared_buffer decrypt(const public_key_t& _rsa_key, buffer_view _buf_source)
	{
		return _decrypt_rsa(_rsa_key, _buf_source);
	}

	bool decrypt(buffer& _buf_dest, const public_key_t& _rsa_key, buffer_view _buf_source)
	{
		try
		{
			// 1) decrypt
			auto buf_result = _decrypt_rsa(_rsa_key, _buf_source);
			_buf_dest.append(buf_result.size(), buf_result.data());
		}
		catch (...)
		{
			return false;
		}

		// return)
		return true;
	}

	shared_buffer decrypt(const private_key_t& _rsa_key, buffer_view _buf_source)
	{
		return _decrypt_rsa(_rsa_key, _buf_source);
	}

	bool decrypt(buffer& _buf_dest, const private_key_t& _rsa_key, buffer_view _buf_source)
	{
		try
		{
			// 1) decrypt
			auto buf_result = _decrypt_rsa(_rsa_key, _buf_source);
			_buf_dest.append(buf_result.size(), buf_result.data());
		}
		catch (...)
		{
			return false;
		}

		// return)
		return true;
	}


	//-----------------------------------------------------------------------------
	// encryptor / decryptor
	//
	//  - EVP_PKEY_CTX는 thread safe하지 않으므로 공유하지 않는다.
	//    (EVP_PKEY만 공유하고 호출마다 EVP_PKEY_CTX를 새로 만든다. EVP_PKEY는 읽기 전용으로 thread safe하다.)
	//-----------------------------------------------------------------------------
	using shared_evp_pkey = std::shared_ptr<EVP_PKEY>;

	static shared_evp_pkey _make_shared_evp_pkey(safe_EVP_PKEY&& _pkey)
	{
		return shared_evp_pkey(_pkey.release(), [](EVP_PKEY* _ptr) { if (_ptr != nullptr) EVP_PKEY_free(_ptr); });
	}

	class encryptor : virtual public CGDK::Iencryptor
	{
	public:
		virtual ~encryptor() noexcept;
		void initialize(const public_key_t& _pem_rsa);
		void destroy() noexcept;
		virtual shared_buffer	encrypt(const_buffer_view _source) override;

	private:
		std::mutex m_lock_pkey;
		shared_evp_pkey m_pkey;
	};

	encryptor::~encryptor() noexcept
	{
		this->destroy();
	}

	shared_buffer encryptor::encrypt(const_buffer_view _source)
	{
		// check) 
		CGASSERT_ERROR(_source.exist());

		// check)
		THROW_IF(_source.empty(), throwable::failure(eRESULT::FAIL, 0, "error: rsa encrypting source is empty"sv));

		// declare) 
		shared_evp_pkey pkey;

		// 1) get key
		{
			// lock)
			std::lock_guard<std::mutex> lock(this->m_lock_pkey);

			// check)
			THROW_IF(!this->m_pkey, throwable::failure(eRESULT::FAIL, 0, "error: rsa key is not initialized or destroyed"sv));

			// - get key
			pkey = this->m_pkey;
		}

		// 2) EVP_PKEY_CTX 생성 (호출마다 만든다.)
		safe_EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new(pkey.get(), nullptr);

		// check) 
		THROW_IF(ctx.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating EVP_PKEY_CTX"sv));

		// 3) 암호화 초기화
		THROW_IF(EVP_PKEY_encrypt_init(ctx.get()) <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to initializing encryption"sv));

		// - padding (OAEP)
		_set_oaep_padding(ctx.get());

		// declare) [필요 최대 버퍼 크기]
		size_t out_len{ 0 };

		// 4) [필요 최대 버퍼 크기]를 먼저 구한다.
		auto result = EVP_PKEY_encrypt(ctx.get(), nullptr, &out_len, _source.data<unsigned char>(), _source.size());

		// check)
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "Error determining buffer length for encryption"sv));

		// 5) [필요한 최대 버퍼 크기] 만큼 buf_encrypted를 할당한다.
		auto buf_encrypted = alloc_shared_buffer(out_len);

		// 6) [암호화된 데이터]에 buf_encrypted를 쓴다.
		result = EVP_PKEY_encrypt(ctx.get(), reinterpret_cast<unsigned char*>(buf_encrypted.data()), &out_len, _source.data<unsigned char>(), _source.size());

		// check) 
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "Error during encryption: " + _get_openssl_error_string()));

		// 7) [암호화된 데이터 크기]를 buf_encrypted의 크기를 설정한다.
		buf_encrypted.set_size(out_len);

		// return)
		return buf_encrypted;
	}

	void encryptor::initialize(const public_key_t& _pem_rsa)
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lock_pkey);

		// check) already initialized?
		RETURN_IF(this->m_pkey, );

		// 1) create EVP_PKEY
		auto evp_pkey = create_evp_pkey_by_pem(_pem_rsa);

		// check) 
		CGASSERT_ERROR(!evp_pkey.empty());

		// 2) set
		this->m_pkey = _make_shared_evp_pkey(std::move(evp_pkey));
	}

	void encryptor::destroy() noexcept
	{
		// declare) 
		shared_evp_pkey pkey;

		// 1) reset m_pkey
		{
			// lock)
			std::lock_guard<std::mutex> lock(this->m_lock_pkey);

			pkey = std::move(this->m_pkey);
		}

		// 2) reset 
		pkey.reset();
	}

	class decryptor : virtual public CGDK::Idecryptor
	{
	public:
		virtual ~decryptor() noexcept;
		void initialize(const private_key_t& _rsa_key);
		void destroy() noexcept;
		virtual shared_buffer decrypt(const_buffer_view _source) override;
	private:
		std::mutex m_lock_pkey;
		shared_evp_pkey m_pkey;
	};

	decryptor::~decryptor() noexcept
	{
		this->destroy();
	}

	shared_buffer decryptor::decrypt(const_buffer_view _source)
	{
		// check) 
		CGASSERT_ERROR(_source.exist());

		// check)
		THROW_IF(_source.empty(), throwable::failure(eRESULT::FAIL, 0, "error: rsa decrypting source is empty"sv));

		// declare) 
		shared_evp_pkey pkey;

		// 1) get key
		{
			// lock)
			std::lock_guard<std::mutex> lock(this->m_lock_pkey);

			// check)
			THROW_IF(!this->m_pkey, throwable::failure(eRESULT::FAIL, 0, "error: rsa key is not initialized or destroyed"sv));

			// - get key
			pkey = this->m_pkey;
		}

		// 2) EVP_PKEY_CTX 생성 (호출마다 만든다.)
		safe_EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new(pkey.get(), nullptr);

		// check) 
		THROW_IF(ctx.empty(), throwable::failure(eRESULT::FAIL, 0, "fail to creating EVP_PKEY_CTX"sv));

		// 3) 복호화 초기화
		THROW_IF(EVP_PKEY_decrypt_init(ctx.get()) <= 0, throwable::failure(eRESULT::FAIL, 0, "fail to initializing decryption"sv));

		// - padding (OAEP)
		_set_oaep_padding(ctx.get());

		// declare) [필요 최대 버퍼 크기]
		size_t out_len{ 0 };

		// 4) [필요 최대 버퍼 크기]를 먼저 구한다.
		auto result = EVP_PKEY_decrypt(ctx.get(), nullptr, &out_len, _source.data<unsigned char>(), _source.size());

		// check)
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "Error determining buffer length for decryption"sv));

		// 5) [필요한 최대 버퍼 크기] 만큼 buf_decrypted를 할당한다.
		auto buf_decrypted = alloc_shared_buffer(out_len);

		// 6) [복호화된 데이터]에 buf_decrypt를 쓴다.
		result = EVP_PKEY_decrypt(ctx.get(), reinterpret_cast<unsigned char*>(buf_decrypted.data()), &out_len, _source.data<unsigned char>(), _source.size());

		// check) 
		THROW_IF(result <= 0, throwable::failure(eRESULT::FAIL, 0, "Error during decryption: " + _get_openssl_error_string()));

		// 7) [복호화된 데이터 크기]를 buf_decrypted의 크기를 설정한다.
		buf_decrypted.set_size(out_len);

		// return)
		return buf_decrypted;
	}

	void decryptor::initialize(const private_key_t& _rsa_key)
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lock_pkey);

		// check)
		RETURN_IF(this->m_pkey, );

		// 1) create EVP_PKEY
		auto evp_pkey = create_evp_pkey_by_pem(_rsa_key);

		// check) 
		CGASSERT_ERROR(!evp_pkey.empty());

		// 2) set
		this->m_pkey = _make_shared_evp_pkey(std::move(evp_pkey));
	}

	void decryptor::destroy() noexcept
	{
		// declare) 
		shared_evp_pkey pkey;

		// 1) reset m_pkey
		{
			// lock)
			std::lock_guard<std::mutex> lock(this->m_lock_pkey);

			pkey = std::move(this->m_pkey);
		}

		// 2) reset 
		pkey.reset();
	}
		
	object_ptr<Iencryptor> make_encryptor(const public_key_t& _pem_ras_key)
	{
		// 1) make rsa encryptor object
		auto pencryptor = make_object<encryptor>();

		// 2) initialize encryptor
		pencryptor->initialize(_pem_ras_key);

		// return) 
		return pencryptor;
	}

	object_ptr<Idecryptor> make_decryptor(const private_key_t& _pem_ras_key)
	{
		// 1) make rsa decryptor object
		auto pdecryptor = make_object<decryptor>();

		// 2) initialize decryptor
		pdecryptor->initialize(_pem_ras_key);

		// return) 
		return pdecryptor;
	}

	class cryptor : virtual public Icryptor
	{
	public:
		void					initialize(const key_pair_t& _pair_rsa_key);

		virtual shared_buffer	encrypt(const_buffer_view _source) override;
		virtual shared_buffer	decrypt(const_buffer_view _source) override;

	private:
		key_pair_t				m_pair_rsa_key;
		object_ptr<Iencryptor>	m_pencryptor;
		object_ptr<Idecryptor>	m_pdecryptor;
	};

	void cryptor::initialize(const key_pair_t& _pair_rsa_key)
	{
		// declare)
		object_ptr<Iencryptor> pencryptor;
		object_ptr<Idecryptor> pdecryptor;

		// 1) make encryptor/decryptor
		if (!_pair_rsa_key.public_key.key.empty())
		{
			pencryptor= make_encryptor(_pair_rsa_key.public_key);
		}

		// 2) make decryptor
		if (!_pair_rsa_key.private_key.key.empty())
		{
			pdecryptor= make_decryptor(_pair_rsa_key.private_key);
		}

		// 2) move
		this->m_pencryptor = std::move(pencryptor);
		this->m_pdecryptor = std::move(pdecryptor);
	}

	shared_buffer cryptor::encrypt(const_buffer_view _source)
	{
		// check) 
		if (this->m_pencryptor.empty())
			return {};

		// return) encrypt and return
		return this->m_pencryptor->encrypt(_source);
	}

	shared_buffer cryptor::decrypt(const_buffer_view _source)
	{
		// check) 
		if (this->m_pdecryptor.empty())
			return {};

		// return) encrypt and return
		return this->m_pdecryptor->decrypt(_source);
	}

	object_ptr<Icryptor> make_cryptor(const key_pair_t& _pair_rsa_key)
	{
		// 1) make rsa cryptor
		auto pcryptor = make_object<cryptor>();

		// 2) initialize
		pcryptor->initialize(_pair_rsa_key);

		// return) 
		return pcryptor;
	}
}
