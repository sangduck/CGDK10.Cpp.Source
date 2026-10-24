#include "pch.h"
#include <bit>
#include <cstring>

//-----------------------------------------------------------------------------
// crc
//
//  1) slicing-by-16 : 16 bytes per iteration with 16 tables (all platforms)
//  2) PCLMULQDQ     : carry-less multiply folding (x86/x64 with PCLMULQDQ, runtime detected)
//                     - 4 x 128-bit lanes (64 bytes per iteration)
//                     - fold constants are generated from the key, so it works with any key
//                     - the folded 128-bit remainder and the tail are finished by the tables
//-----------------------------------------------------------------------------
#if defined(_M_X64) || defined(__x86_64__)
	#define CGDK_CRC_PCLMUL
	#include <emmintrin.h>
	#include <wmmintrin.h>
	#if defined(_MSC_VER)
		#include <intrin.h>
	#endif
	#if defined(__GNUC__) || defined(__clang__)
		#define CGDK_CRC_TARGET_PCLMUL	__attribute__((target("pclmul,sse2")))
	#else
		#define CGDK_CRC_TARGET_PCLMUL
	#endif
#endif

namespace CGDK
{

namespace
{
	using crc_tables_t = std::array<std::array<uint32_t, 256>, 16>;

	[[nodiscard]] uint32_t _reflect32(uint32_t _value) noexcept
	{
		_value = ((_value >> 1) & 0x55555555u) | ((_value & 0x55555555u) << 1);
		_value = ((_value >> 2) & 0x33333333u) | ((_value & 0x33333333u) << 2);
		_value = ((_value >> 4) & 0x0f0f0f0fu) | ((_value & 0x0f0f0f0fu) << 4);
		_value = ((_value >> 8) & 0x00ff00ffu) | ((_value & 0x00ff00ffu) << 8);
		return (_value >> 16) | (_value << 16);
	}

	// - x^_n mod P (P : normal representation without x^32)
	[[nodiscard]] uint32_t _xpow_mod(uint32_t _poly, size_t _n) noexcept
	{
		uint32_t value = 1;
		for (; _n != 0; --_n)
			value = (value & 0x80000000u) ? ((value << 1) ^ _poly) : (value << 1);
		return value;
	}

	// - PCLMULQDQ constant multiplying a reflected 64-bit lane by x^_n
	//   (clmul of reflected values is one bit short, so x^(_n-1) is used)
	[[nodiscard]] uint64_t _fold_constant(uint32_t _poly, size_t _n) noexcept
	{
		return static_cast<uint64_t>(_reflect32(_xpow_mod(_poly, _n - 1))) << 32;
	}

	[[nodiscard]] inline uint32_t _load32(const uint8_t* _p) noexcept
	{
		uint32_t value;
		memcpy(&value, _p, sizeof(value));
		return value;
	}

	[[nodiscard]] uint32_t _crc_slice16(const crc_tables_t& _t, uint32_t _crc, const uint8_t* _p, size_t _length) noexcept
	{
		if constexpr (std::endian::native == std::endian::little)
		{
			// 1) 16 bytes
			while (_length >= 16)
			{
				const uint32_t w0 = _load32(_p) ^ _crc;
				const uint32_t w1 = _load32(_p + 4);
				const uint32_t w2 = _load32(_p + 8);
				const uint32_t w3 = _load32(_p + 12);

				_crc = _t[15][w0 & 0xff] ^ _t[14][(w0 >> 8) & 0xff] ^ _t[13][(w0 >> 16) & 0xff] ^ _t[12][w0 >> 24]
					 ^ _t[11][w1 & 0xff] ^ _t[10][(w1 >> 8) & 0xff] ^ _t[ 9][(w1 >> 16) & 0xff] ^ _t[ 8][w1 >> 24]
					 ^ _t[ 7][w2 & 0xff] ^ _t[ 6][(w2 >> 8) & 0xff] ^ _t[ 5][(w2 >> 16) & 0xff] ^ _t[ 4][w2 >> 24]
					 ^ _t[ 3][w3 & 0xff] ^ _t[ 2][(w3 >> 8) & 0xff] ^ _t[ 1][(w3 >> 16) & 0xff] ^ _t[ 0][w3 >> 24];

				_p += 16;
				_length -= 16;
			}

			// 2) 8 bytes
			if (_length >= 8)
			{
				const uint32_t w0 = _load32(_p) ^ _crc;
				const uint32_t w1 = _load32(_p + 4);

				_crc = _t[7][w0 & 0xff] ^ _t[6][(w0 >> 8) & 0xff] ^ _t[5][(w0 >> 16) & 0xff] ^ _t[4][w0 >> 24]
					 ^ _t[3][w1 & 0xff] ^ _t[2][(w1 >> 8) & 0xff] ^ _t[1][(w1 >> 16) & 0xff] ^ _t[0][w1 >> 24];

				_p += 8;
				_length -= 8;
			}
		}

		// 3) remained bytes
		for (; _length != 0; --_length, ++_p)
			_crc = (_crc >> 8) ^ _t[0][(_crc ^ *_p) & 0xff];

		return _crc;
	}

#if defined(CGDK_CRC_PCLMUL)
	[[nodiscard]] bool _is_pclmul_supported() noexcept
	{
		static const bool s_supported = []() noexcept
		{
		#if defined(_MSC_VER)
			int info[4]{};
			__cpuid(info, 1);
			return (info[2] & (1 << 1)) != 0;
		#else
			__builtin_cpu_init();
			return __builtin_cpu_supports("pclmul") != 0;
		#endif
		}();
		return s_supported;
	}

	CGDK_CRC_TARGET_PCLMUL inline __m128i _fold(__m128i _x, __m128i _k) noexcept
	{
		return _mm_xor_si128(_mm_clmulepi64_si128(_x, _k, 0x00), _mm_clmulepi64_si128(_x, _k, 0x11));
	}

	// - _length >= 64
	CGDK_CRC_TARGET_PCLMUL uint32_t _crc_pclmul(const crc_tables_t& _t, const std::array<uint64_t, 4>& _k, uint32_t _crc, const uint8_t* _p, size_t _length) noexcept
	{
		const __m128i k512 = _mm_set_epi64x(static_cast<int64_t>(_k[1]), static_cast<int64_t>(_k[0]));
		const __m128i k128 = _mm_set_epi64x(static_cast<int64_t>(_k[3]), static_cast<int64_t>(_k[2]));

		// 1) first 64 bytes (the crc is xored into the first 4 bytes)
		__m128i x0 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(_p)), _mm_cvtsi32_si128(static_cast<int>(_crc)));
		__m128i x1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 16));
		__m128i x2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 32));
		__m128i x3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 48));
		_p += 64;
		_length -= 64;

		// 2) fold 4 x 128 bits (64 bytes per iteration)
		while (_length >= 64)
		{
			x0 = _mm_xor_si128(_fold(x0, k512), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p)));
			x1 = _mm_xor_si128(_fold(x1, k512), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 16)));
			x2 = _mm_xor_si128(_fold(x2, k512), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 32)));
			x3 = _mm_xor_si128(_fold(x3, k512), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p + 48)));
			_p += 64;
			_length -= 64;
		}

		// 3) 4 lanes -> 1 lane
		x1 = _mm_xor_si128(_fold(x0, k128), x1);
		x2 = _mm_xor_si128(_fold(x1, k128), x2);
		x3 = _mm_xor_si128(_fold(x2, k128), x3);

		// 4) fold 128 bits (16 bytes per iteration)
		while (_length >= 16)
		{
			x3 = _mm_xor_si128(_fold(x3, k128), _mm_loadu_si128(reinterpret_cast<const __m128i*>(_p)));
			_p += 16;
			_length -= 16;
		}

		// 5) the folded 128 bits has the same remainder as the processed data
		alignas(16) uint8_t folded[16];
		_mm_store_si128(reinterpret_cast<__m128i*>(folded), x3);
		const auto crc = _crc_slice16(_t, 0, folded, sizeof(folded));

		// return) remained bytes
		return _crc_slice16(_t, crc, _p, _length);
	}
#endif
}

void edc::crc::generate_crc_table(uint32_t _key)
{
	// Assert) _key must not be 0
	CGASSERT_ERROR(_key != 0);

	// 1) base table
	uint32_t value = 0;
	for (auto& entry : m_table_CRC[0])
	{
		auto temp_crc = value++;
		for (int j = 8; j > 0; --j)
			temp_crc = (temp_crc & 1) ? ((temp_crc >> 1) ^ _key) : (temp_crc >> 1);

		entry = temp_crc;
	}

	// 2) slicing tables (table[n] : the byte followed by n zero bytes)
	for (size_t n = 1; n < m_table_CRC.size(); ++n)
	{
		for (size_t i = 0; i < m_table_CRC[n].size(); ++i)
		{
			const auto prev = m_table_CRC[n - 1][i];
			m_table_CRC[n][i] = (prev >> 8) ^ m_table_CRC[0][prev & 0xff];
		}
	}

	// 3) PCLMULQDQ fold constants (the key is a reflected polynomial)
	const auto poly = _reflect32(_key);
	m_fold_constant[0] = _fold_constant(poly, 512 + 64);	// fold 512 bits - high lane
	m_fold_constant[1] = _fold_constant(poly, 512);			// fold 512 bits - low lane
	m_fold_constant[2] = _fold_constant(poly, 128 + 64);	// fold 128 bits - high lane
	m_fold_constant[3] = _fold_constant(poly, 128);			// fold 128 bits - low lane

	// 4) key
	m_key = _key;
}

uint32_t edc::crc::get_crc(const uint8_t* _pdata, size_t _data_length) const
{
	// check)
	CGASSERT_ERROR(_pdata != nullptr);
	CGASSERT_ERROR(_data_length != 0);

	// check)
	if(_pdata == nullptr)
		return 0;

#if defined(CGDK_CRC_PCLMUL)
	// 1) PCLMULQDQ
	if (_data_length >= 64 && _is_pclmul_supported())
		return _crc_pclmul(m_table_CRC, m_fold_constant, 0, _pdata, _data_length);
#endif

	// 2) slicing-by-16
	return _crc_slice16(m_table_CRC, 0, _pdata, _data_length);
}

}
