#include "pch.h"

//-----------------------------------------------------------------------------
// pn_code
//
//  - key[pos] ^= key[pos + offset], out = in ^ key[pos] (pos : uint8_t)
//  - key[pos + offset] was updated 'lag = 256 - offset' bytes before, so up to
//    'lag' bytes can be processed at once. (32 bytes with AVX2, 16 bytes with
//    SSE2/NEON, or 8 bytes)
//  - m_array_key has a mirror of key[0..31] at key[256..287] so a block reading
//    key[pos + offset] never has to wrap around.
//-----------------------------------------------------------------------------
#if defined(_M_X64) || defined(__x86_64__) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
	#define CGDK_PN_CODE_SSE2
	#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(_M_ARM64)
	#define CGDK_PN_CODE_NEON
	#include <arm_neon.h>
#endif
#include <cstring>
#include <openssl/rand.h>
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

namespace CGDK
{
namespace
{
	constexpr size_t PN_KEY_SIZE = crypt::pn_code::MAX_BYTES_KEY_LENGTH;
	constexpr size_t PN_KEY_MIRROR = 32;
	static_assert(PN_KEY_SIZE == 256, "pn_code position is uint8_t");

	// - 16 bytes : key[0..15] ^= key_get[0..15], dest = src ^ key
	inline void _pn_block16(uint8_t* _dest, const uint8_t* _src, uint8_t* _key, const uint8_t* _key_get, uint8_t* _mirror) noexcept
	{
	#if defined(CGDK_PN_CODE_SSE2)
		const __m128i key = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(_key)), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_key_get)));
		const __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_key), key);
		if (_mirror != nullptr)
			_mm_storeu_si128(reinterpret_cast<__m128i*>(_mirror), key);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dest), _mm_xor_si128(src, key));
	#elif defined(CGDK_PN_CODE_NEON)
		const uint8x16_t key = veorq_u8(vld1q_u8(_key), vld1q_u8(_key_get));
		const uint8x16_t src = vld1q_u8(_src);
		vst1q_u8(_key, key);
		if (_mirror != nullptr)
			vst1q_u8(_mirror, key);
		vst1q_u8(_dest, veorq_u8(src, key));
	#else
		uint64_t key[2], key_get[2], src[2];
		memcpy(key, _key, 16);
		memcpy(key_get, _key_get, 16);
		memcpy(src, _src, 16);
		key[0] ^= key_get[0];
		key[1] ^= key_get[1];
		memcpy(_key, key, 16);
		if (_mirror != nullptr)
			memcpy(_mirror, key, 16);
		src[0] ^= key[0];
		src[1] ^= key[1];
		memcpy(_dest, src, 16);
	#endif
	}

	// - 8 bytes
	inline void _pn_block8(uint8_t* _dest, const uint8_t* _src, uint8_t* _key, const uint8_t* _key_get, uint8_t* _mirror) noexcept
	{
		uint64_t key, key_get, src;
		memcpy(&key, _key, 8);
		memcpy(&key_get, _key_get, 8);
		memcpy(&src, _src, 8);
		key ^= key_get;
		memcpy(_key, &key, 8);
		if (_mirror != nullptr)
			memcpy(_mirror, &key, 8);
		src ^= key;
		memcpy(_dest, &src, 8);
	}

#if defined(CGDK_SIMD_AVX2)
	// - 32 bytes blocks (AVX2)
	//   (_pos_out : aligned to 16, lag >= 32. stops when key[pos..pos+31] wraps around)
	//   * _pos_out is passed by value : taking the address of the caller's pos_out makes
	//     every uint8_t store to the key reload it (char types alias everything)
	// return) processed bytes (multiple of 32, pos_out advances by the same)
	CGDK_TARGET_AVX2 size_t _pn_encrypt_avx2(uint8_t* _dest, const uint8_t* _src, size_t _size, uint8_t* _key, uint8_t _pos_out, uint8_t _offset) noexcept
	{
		size_t processed = 0;

		for (; _size - processed >= 32 && _pos_out <= PN_KEY_SIZE - 32; processed += 32)
		{
			const __m256i key = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_key + _pos_out)), _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_key + static_cast<uint8_t>(_pos_out + _offset))));
			const __m256i src = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_src + processed));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_key + _pos_out), key);

			// - mirror (key[0..31] -> key[256..287])
			if (_pos_out == 0)
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(_key + PN_KEY_SIZE), key);
			else if (_pos_out == 16)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(_key + PN_KEY_SIZE + 16), _mm256_castsi256_si128(key));

			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dest + processed), _mm256_xor_si256(src, key));
			_pos_out = static_cast<uint8_t>(_pos_out + 32);
		}

		// - avoid AVX-SSE transition penalty
		_mm256_zeroupper();

		return processed;
	}
#endif
}

class _pn_code
{
public:
	void				initialize(const_buffer_view _seed_info);

	[[nodiscard]] buffer_view	encrypt(buffer_view _dest, const_buffer_view _source);
	[[nodiscard]] shared_buffer	encrypt(const_buffer_view _source);

protected:
	std::array<uint8_t, PN_KEY_SIZE + PN_KEY_MIRROR> m_array_key{};
	uint8_t				m_pos_out{ 0 };
	uint8_t				m_pos_get{ 0 };
};

void _pn_code::initialize(const_buffer_view _seed_info)
{
	// check) _seed_info가 0이어서는 안 된다.
	CGASSERT_ERROR(_seed_info.size() != 0);

	// 1) Key의 개수를 얻는다.
	size_t key_count = _seed_info.extract<uint32_t>();

	// check) Key의 개수가 MIN_BYTES_KEY_LENGTH개보다 크고 MAX_BYTES_KEY_LENGTH개보다 작아야 한다.
	if(key_count < crypt::pn_code::MIN_BYTES_KEY_LENGTH || key_count > crypt::pn_code::MAX_BYTES_KEY_LENGTH)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// check) _length가 16+key_count*sizeof(uint32_t) 여야한다.
	if(_seed_info.size() != (sizeof(uint32_t) + sizeof(uint32_t) + key_count * sizeof(uint8_t)))
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// 3) 기타 정보를 얻는다.
	auto temp_offset = _seed_info.extract<uint32_t>();
	auto temp_round = _seed_info.extract<uint32_t>();

	// check) offset이 0이면 key[pos] ^= key[pos]로 key가 모두 0이 되어 암호화되지 않는다.
	if ((temp_offset % crypt::pn_code::MAX_BYTES_KEY_LENGTH) == 0)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "pn_code: invalid key (offset is 0)"sv);

	// declare)
	std::array<uint8_t, crypt::pn_code::MAX_BYTES_KEY_LENGTH> array_key{};

	// 4) Key를 Generate한다.(일반적으로 4개의 Seed로 key를 Generate한다.)
	{
		// 1) 일단 받은 키값은 복사를 한다.
		size_t i = 0;
		size_t i_end = key_count;
		for (; i < i_end; ++i)
		{
			array_key[i] = _seed_info[i];
		}

		// 2) 받은 키값 이후에서 MAX_BYTES_KEY_LENGTH까지의 값은 만들어 낸다.
		size_t pos_a = i;
		size_t pos_b = i >> 1;
		for (; i < crypt::pn_code::MAX_BYTES_KEY_LENGTH; ++i)
		{
			array_key[i] = array_key[i - pos_a] * array_key[i - pos_b];

			if (array_key[i] == 0)
			{
				array_key[i] = array_key[i - 1];
			}
		}
	}

	// 5) 최초 회전을 수행한다.
	uint8_t	pos_output_a = 0;
	uint8_t	pos_output_b = (uint8_t)temp_offset;

	for (uint32_t i = 0; i < temp_round; ++i)
	{
		array_key[pos_output_a] = array_key[pos_output_a] ^ array_key[pos_output_b];

		++pos_output_a;
		++pos_output_b;
	}

	// 6) 최종 값을 설정한다.
	std::copy(array_key.begin(), array_key.end(), this->m_array_key.begin());
	std::copy_n(array_key.begin(), PN_KEY_MIRROR, this->m_array_key.begin() + PN_KEY_SIZE);
	this->m_pos_out = pos_output_a;
	this->m_pos_get = (uint8_t)temp_offset;
}

buffer_view _pn_code::encrypt(buffer_view _dest, const_buffer_view _source)
{
	// check) 
	CGASSERT_ERROR(_source.size() != 0);

	// 1) prepare
	auto		src		 = _source.data<const uint8_t>();
	auto		dest	 = _dest.data<uint8_t>();
	size_t		remained = _source.size();
	uint8_t		pos_out	 = this->m_pos_out;
	const uint8_t offset = this->m_pos_get;
	uint8_t*	key		 = this->m_array_key.data();

	// 2) less than 16 bytes : byte by byte (same as before)
	if (remained < 16)
	{
		const uint8_t pos_start = pos_out;
		uint8_t pos_get = static_cast<uint8_t>(pos_out + offset);
		for (; remained != 0; --remained)
		{
			key[pos_out] = key[pos_out] ^ key[pos_get];
			*dest++ = static_cast<uint8_t>(*src++ ^ key[pos_out]);
			++pos_out;
			++pos_get;
		}

		// - refresh the mirror if key[0..31] was updated
		if (pos_start < PN_KEY_MIRROR || pos_start + _source.size() > PN_KEY_SIZE)
			memcpy(key + PN_KEY_SIZE, key, PN_KEY_MIRROR);

		this->m_pos_out = pos_out;
		return buffer_view{ _dest.data(), _source.size() };
	}

	// 3) byte by byte (keeps the mirror)
	auto process_byte = [&]() noexcept
	{
		const uint8_t value = static_cast<uint8_t>(key[pos_out] ^ key[static_cast<uint8_t>(pos_out + offset)]);
		key[pos_out] = value;
		if (pos_out < PN_KEY_MIRROR)
			key[PN_KEY_SIZE + pos_out] = value;
		*dest++ = static_cast<uint8_t>(*src++ ^ value);
		++pos_out;
		--remained;
	};

	// 4) block size (can't be longer than lag)
	const size_t lag = (offset == 0) ? PN_KEY_SIZE : (PN_KEY_SIZE - offset);
	const size_t block = (lag >= 16) ? 16 : (lag >= 8) ? 8 : 1;
#if defined(CGDK_SIMD_AVX2)
	// - AVX2 only when lag >= 128
	//   (a smaller lag makes a 32-byte load read key bytes stored 1~3 blocks before at a
	//    misaligned position, so store-to-load forwarding fails and it gets slower than 16 bytes)
	const bool use_avx2 = (lag >= 128) && cpu::is_avx2_supported();
#endif

	if (block > 1)
	{
		// - align pos_out to the block size (then key[pos_out..] never wraps around)
		while (remained != 0 && (pos_out & (block - 1)) != 0)
			process_byte();

		if (block == 16)
		{
		#if defined(CGDK_SIMD_AVX2)
			// - 32 bytes (AVX2)
			//   (kept out of the 16 bytes loop so that loop stays as tight as before)
			if (use_avx2)
			{
				while (remained >= 32)
				{
					const auto processed = _pn_encrypt_avx2(dest, src, remained, key, pos_out, offset);
					dest += processed;
					src += processed;
					pos_out = static_cast<uint8_t>(pos_out + processed);
					remained -= processed;

					// - stopped at pos 240 (key[pos..pos+31] wraps around) : one 16 bytes block
					if (remained < 32)
						break;
					_pn_block16(dest, src, key + pos_out, key + static_cast<uint8_t>(pos_out + offset), nullptr);
					dest += 16;
					src += 16;
					pos_out = static_cast<uint8_t>(pos_out + 16);
					remained -= 16;
				}
			}
		#endif

			// - 16 bytes (SSE2/NEON)
			while (remained >= 16)
			{
				_pn_block16(dest, src, key + pos_out, key + static_cast<uint8_t>(pos_out + offset), (pos_out < PN_KEY_MIRROR) ? key + PN_KEY_SIZE + pos_out : nullptr);
				dest += 16;
				src += 16;
				pos_out = static_cast<uint8_t>(pos_out + 16);
				remained -= 16;
			}
		}

		// - 8 bytes
		while (remained >= 8)
		{
			_pn_block8(dest, src, key + pos_out, key + static_cast<uint8_t>(pos_out + offset), (pos_out < PN_KEY_MIRROR) ? key + PN_KEY_SIZE + pos_out : nullptr);
			dest += 8;
			src += 8;
			pos_out = static_cast<uint8_t>(pos_out + 8);
			remained -= 8;
		}
	}

	// 5) remained bytes
	while (remained != 0)
		process_byte();

	// 6) ...
	this->m_pos_out = pos_out;

	// return) ...
	return buffer_view{ _dest.data(), _source.size() };
}

shared_buffer _pn_code::encrypt(const_buffer_view _source)
{
	// declare)
	shared_buffer buf_result;

	// check) 
	if(_source.size() == 0)
		return buf_result;

	// 1) get size
	const auto size_alloc = _source.size() + 1;

	// 1) alloc buffer
	buf_result = alloc_shared_buffer(size_alloc) ^ size_alloc;

	// 2) encrypt
	buf_result ^= this->encrypt(buf_result,  _source);

	// return) 
	return buf_result;
}

// 설명) encryptor/decryptor/cryptor 본체는 풀과 무관하게 둔다.
//       socket 처럼 접속마다 만들어지는 곳에는 이들을 상속한 *_pooled 클래스(Npoolable)를 쓴다. (new/delete 를 피한다)
//       풀의 객체는 생성자를 다시 타지 않지만 initialize() 가 상태를 전부 다시 채우므로 재사용해도 된다.
class encryptor : virtual public Iencryptor
{
public:
	void initialize(const_buffer_view _seed_info)
	{
		this->m_cryptor.initialize(_seed_info);
	}

	virtual shared_buffer encrypt(const_buffer_view _source)
	{
		return this->m_cryptor.encrypt(_source);
	}

private:
	_pn_code	m_cryptor;
};

class decryptor : virtual public Idecryptor
{
public:
	void initialize(const_buffer_view _seed_info)
	{
		this->m_cryptor.initialize(_seed_info);
	}

	virtual shared_buffer decrypt(const_buffer_view _source)
	{
		return this->m_cryptor.encrypt(_source);
	}

private:
	_pn_code	m_cryptor;
};

// - 풀에서 할당하는 encryptor/decryptor (make_encryptor/make_decryptor 가 만든다)
class encryptor_pooled : public encryptor, public Npoolable<encryptor_pooled> {};
class decryptor_pooled : public decryptor, public Npoolable<decryptor_pooled> {};

crypt::pn_code::key_t crypt::pn_code::generate_key(int _min_count, int _max_count)
{
	// check)
	CGASSERT_ERROR(_min_count >= MIN_BYTES_KEY_LENGTH);

	// check)
	CGASSERT_ERROR(_max_count <= MAX_BYTES_KEY_LENGTH);

	// check)
	CGASSERT_ERROR(_min_count <= _max_count);

	// check)
	if(_min_count > _max_count)
		throw throwable::failure(eRESULT::OUT_OF_RANGE, 0, "value invalid (_min_count > _max_count)"sv);

	// check) initialize에서 받아들이는 범위
	if(_min_count < MIN_BYTES_KEY_LENGTH || _max_count > MAX_BYTES_KEY_LENGTH)
		throw throwable::failure(eRESULT::OUT_OF_RANGE, 0, "value invalid (key length out of range)"sv);

	// 설명) key는 암호학적 난수(RAND_bytes)로 만든다.
	//       - rand()는 예측 가능하고(MSVC는 15bit) 같은 tick에 만든 key가 같아진다.
	auto random_bytes = [](void* _dest, std::size_t _size)
	{
		THROW_IF(RAND_bytes(static_cast<unsigned char*>(_dest), static_cast<int>(_size)) != 1, throwable::failure(eRESULT::FAIL, 0, "pn_code: RAND_bytes failed"sv));
	};
	auto random_uint32 = [&random_bytes]() { uint32_t value = 0; random_bytes(&value, sizeof(value)); return value; };

	// declare)
	int key_count = 0;

	// 1) key 개수를 random하게 얻는다. (_min_count와 _max_count 사이의 4의 배수값)
	{
		// - radom key size
		key_count = _min_count;
		auto differ_key_count = _max_count - _min_count;
		if (differ_key_count != 0)
			key_count += static_cast<int>(random_uint32() % static_cast<uint32_t>(differ_key_count));

		// - 4의 배수로 만듬.
		key_count >>= 2;
		key_count <<= 2;
	}

	// 2) key를 저장할 buffer를 할당한다.
	auto buf_keys = alloc_shared_buffer(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + key_count * sizeof(char));

	// 3) buf_keys에 Key의 개수를 써넣는다.
	buf_keys.append<uint32_t>(key_count);

	// 4) buf_keys에 offset값을 random하게 얻는다.
	//    - offset이 0이면 key[pos] ^= key[pos]로 key가 모두 0이 되어 암호화가 되지 않는다. (1 ~ 255)
	buf_keys.append<uint32_t>(1 + random_uint32() % (MAX_BYTES_KEY_LENGTH - 1));

	// 5) buf_keys에 round값을 random하게 얻는다.
	buf_keys.append<uint32_t>(random_uint32() % MAX_BYTES_KEY_LENGTH);

	// 6) random한 key를 생성한다.
	for (auto i = key_count >> 2; i>0; --i)
		buf_keys.append<uint32_t>(random_uint32());

	// return)
	return buf_keys;
}

object_ptr<Iencryptor> crypt::pn_code::make_encryptor(const_buffer_view _encrypt_key)
{
	auto pencryptor = make_object<encryptor_pooled>();

	pencryptor->initialize(_encrypt_key);

	return pencryptor;
}

object_ptr<Idecryptor> crypt::pn_code::make_decryptor(const_buffer_view _decrypt_key)
{
	auto pdecryptor = make_object<decryptor_pooled>();

	pdecryptor->initialize(_decrypt_key);

	return pdecryptor;
}

class cryptor : virtual public Icryptor
{
public:
	void initialize(const crypt::pn_code::key_t& _keys_encrypt, const crypt::pn_code::key_t& _keys_decrypt);
	[[nodiscard]] virtual shared_buffer		encrypt(const_buffer_view _source) override;
	[[nodiscard]] virtual shared_buffer		decrypt(const_buffer_view _source) override;

	// - encryptor/decryptor 를 놓아 준다. (풀에서 쉬는 동안 붙들고 있지 않게. initialize() 가 다시 채운다)
	void reset() noexcept { this->m_pencryptor.reset(); this->m_pdecryptor.reset(); }

private:
	object_ptr<Iencryptor>	m_pencryptor;
	object_ptr<Idecryptor>	m_pdecryptor;
};

void cryptor::initialize(const crypt::pn_code::key_t& _keys_encrypt, const crypt::pn_code::key_t& _keys_decrypt)
{
	// 1) make encryptor & decryptor
	auto pencryptor = crypt::pn_code::make_encryptor(_keys_encrypt);
	auto pdecryptor = crypt::pn_code::make_decryptor(_keys_decrypt);

	// 2) set
	this->m_pencryptor = std::move(pencryptor);
	this->m_pdecryptor = std::move(pdecryptor);
}

shared_buffer cryptor::encrypt(const_buffer_view _source)
{
	return this->m_pencryptor->encrypt(_source);
}

shared_buffer cryptor::decrypt(const_buffer_view _source)
{
	return this->m_pdecryptor->decrypt(_source);
}

// - 풀에서 할당하는 cryptor (socket(net::io::Ntcp_crypt)이 접속마다 쓰는 make_cryptor 가 만든다. 풀에 돌아갈 때 encryptor/decryptor 를 놓아 준다)
class cryptor_pooled : public cryptor, public Npoolable<cryptor_pooled>
{
public:
	virtual void on_final_release() noexcept override { this->reset(); }
};

object_ptr<Icryptor> crypt::pn_code::make_cryptor(const key_t& _keys_encrypt, const key_t& _keys_decrypt)
{
	auto pcryptor = make_object<cryptor_pooled>();

	pcryptor->initialize(_keys_encrypt, _keys_decrypt);

	return pcryptor;
}

}
