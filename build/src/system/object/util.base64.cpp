#include "pch.h"
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

//-----------------------------------------------------------------------------
// base64
//
//  - encode : 3 bytes -> 4 chars ('=' padding)
//  - decode : stops at the first '=' or non-base64 char
//  - AVX2 (x64, runtime detected)
//    encode : 24 bytes -> 32 chars per iteration
//    decode : 32 chars -> 24 bytes per iteration
//             (a block with '=' or a non-base64 char is left to the scalar loop)
//-----------------------------------------------------------------------------
namespace CGDK
{

namespace
{
	constexpr char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789+/";

	// - char -> 6 bits value (0xff : not base64)
	constexpr std::array<uint8_t, 256> BASE64_DECODE = []() constexpr
	{
		std::array<uint8_t, 256> table{};
		table.fill(0xff);
		for (uint8_t i = 0; i < 64; ++i)
			table[static_cast<uint8_t>(BASE64_CHARS[i])] = i;
		return table;
	}();

	char* _encode_scalar(char* _out, const uint8_t* _p, size_t _size) noexcept
	{
		// 1) 3 bytes -> 4 chars
		for (; _size >= 3; _p += 3, _size -= 3)
		{
			const uint32_t value = (static_cast<uint32_t>(_p[0]) << 16) | (static_cast<uint32_t>(_p[1]) << 8) | _p[2];
			*_out++ = BASE64_CHARS[value >> 18];
			*_out++ = BASE64_CHARS[(value >> 12) & 0x3f];
			*_out++ = BASE64_CHARS[(value >> 6) & 0x3f];
			*_out++ = BASE64_CHARS[value & 0x3f];
		}

		// 2) remained 1~2 bytes (padding)
		if (_size != 0)
		{
			const uint32_t value = (static_cast<uint32_t>(_p[0]) << 16) | ((_size == 2) ? (static_cast<uint32_t>(_p[1]) << 8) : 0);
			*_out++ = BASE64_CHARS[value >> 18];
			*_out++ = BASE64_CHARS[(value >> 12) & 0x3f];
			*_out++ = (_size == 2) ? BASE64_CHARS[(value >> 6) & 0x3f] : '=';
			*_out++ = '=';
		}

		return _out;
	}

	uint8_t* _decode_scalar(uint8_t* _out, const char* _p, size_t _size) noexcept
	{
		// 1) 4 chars -> 3 bytes
		uint32_t value = 0;
		int count = 0;
		for (; _size != 0; --_size, ++_p)
		{
			const uint8_t bits = BASE64_DECODE[static_cast<uint8_t>(*_p)];
			if (bits == 0xff)
				break;

			value = (value << 6) | bits;
			if (++count == 4)
			{
				*_out++ = static_cast<uint8_t>(value >> 16);
				*_out++ = static_cast<uint8_t>(value >> 8);
				*_out++ = static_cast<uint8_t>(value);
				value = 0;
				count = 0;
			}
		}

		// 2) remained chars (2 chars -> 1 byte, 3 chars -> 2 bytes)
		if (count >= 2)
		{
			value <<= (4 - count) * 6;
			*_out++ = static_cast<uint8_t>(value >> 16);
			if (count == 3)
				*_out++ = static_cast<uint8_t>(value >> 8);
		}

		return _out;
	}

#if defined(CGDK_SIMD_AVX2)
	// return) processed bytes (multiple of 24)
	CGDK_TARGET_AVX2 size_t _encode_avx2(char* _out, const uint8_t* _p, size_t _size) noexcept
	{
		// - [b0 b1 b2] -> [b1 b0 b2 b1] in each 32-bit lane
		const __m256i shuffle = _mm256_setr_epi8(
			1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10,
			1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10);
		// - 6 bits value -> char offset ('A', 'a', '0', '+', '/')
		const __m256i lut = _mm256_setr_epi8(
			65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0,
			65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);

		size_t processed = 0;

		// - reads 28 bytes (16 bytes at +0 and +12)
		for (; _size - processed >= 28; processed += 24, _out += 32)
		{
			const uint8_t* p = _p + processed;
			__m256i in = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p))), _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 12)), 1);
			in = _mm256_shuffle_epi8(in, shuffle);

			// - split into 4 x 6 bits
			const __m256i t0 = _mm256_mulhi_epu16(_mm256_and_si256(in, _mm256_set1_epi32(0x0fc0fc00)), _mm256_set1_epi32(0x04000040));
			const __m256i t1 = _mm256_mullo_epi16(_mm256_and_si256(in, _mm256_set1_epi32(0x003f03f0)), _mm256_set1_epi32(0x01000010));
			const __m256i bits = _mm256_or_si256(t0, t1);

			// - 6 bits value -> char
			__m256i index = _mm256_subs_epu8(bits, _mm256_set1_epi8(51));
			index = _mm256_sub_epi8(index, _mm256_cmpgt_epi8(bits, _mm256_set1_epi8(25)));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_out), _mm256_add_epi8(bits, _mm256_shuffle_epi8(lut, index)));
		}

		return processed;
	}

	// return) processed chars (multiple of 32)
	//  * writes 32 bytes per 24 bytes decoded (_out needs 8 bytes more)
	CGDK_TARGET_AVX2 size_t _decode_avx2(uint8_t* _out, const char* _p, size_t _size) noexcept
	{
		// - validation (lut_lo[lo nibble] & lut_hi[hi nibble] != 0 : not base64)
		const __m256i lut_lo = _mm256_setr_epi8(
			0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x13, 0x1a, 0x1b, 0x1b, 0x1b, 0x1a,
			0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x13, 0x1a, 0x1b, 0x1b, 0x1b, 0x1a);
		const __m256i lut_hi = _mm256_setr_epi8(
			0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
			0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10);
		// - char -> 6 bits value offset (by hi nibble, '/' : index 1)
		const __m256i lut_roll = _mm256_setr_epi8(
			0, 16, 19, 4, -65, -65, -71, -71, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 16, 19, 4, -65, -65, -71, -71, 0, 0, 0, 0, 0, 0, 0, 0);
		const __m256i mask_2f = _mm256_set1_epi8(0x2f);
		// - 3 bytes of each 32-bit lane -> packed 12 bytes per 128-bit lane
		const __m256i pack = _mm256_setr_epi8(
			2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1,
			2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1);

		size_t processed = 0;

		for (; _size - processed >= 32; processed += 32, _out += 24)
		{
			__m256i str = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_p + processed));

			// - check) '=' or not base64 -> scalar
			const __m256i hi_nibbles = _mm256_and_si256(_mm256_srli_epi32(str, 4), mask_2f);
			const __m256i lo_nibbles = _mm256_and_si256(str, mask_2f);
			if (!_mm256_testz_si256(_mm256_shuffle_epi8(lut_lo, lo_nibbles), _mm256_shuffle_epi8(lut_hi, hi_nibbles)))
				break;

			// - char -> 6 bits value
			const __m256i roll = _mm256_shuffle_epi8(lut_roll, _mm256_add_epi8(_mm256_cmpeq_epi8(str, mask_2f), hi_nibbles));
			str = _mm256_add_epi8(str, roll);

			// - 4 x 6 bits -> 24 bits
			const __m256i merged = _mm256_madd_epi16(_mm256_maddubs_epi16(str, _mm256_set1_epi32(0x01400140)), _mm256_set1_epi32(0x00011000));
			const __m256i out = _mm256_permutevar8x32_epi32(_mm256_shuffle_epi8(merged, pack), _mm256_setr_epi32(0, 1, 2, 4, 5, 6, 7, 7));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_out), out);
		}

		return processed;
	}
#endif
}

std::string crypt::encode_base64(const_buffer_view _source)
{
	// declare)
	std::string ret;

	// check)
	if (_source.size() == 0)
		return ret;

	// 1) prepare
	auto		src  = _source.data<const uint8_t>();
	size_t		size = _source.size();
	ret.resize((size + 2) / 3 * 4);
	char*		out  = ret.data();

#if defined(CGDK_SIMD_AVX2)
	// 2) AVX2
	if (size >= 32 && cpu::is_avx2_supported())
	{
		const auto processed = _encode_avx2(out, src, size);
		src  += processed;
		size -= processed;
		out  += processed / 3 * 4;
	}
#endif

	// 3) remained
	[[maybe_unused]] const auto out_end = _encode_scalar(out, src, size);

	// check)
	CGASSERT_ERROR(out_end == ret.data() + ret.size());

	// return)
	return ret;
}

shared_buffer crypt::decode_base64(const std::string_view _source)
{
	// 1) alloc (max decoded size + 32 bytes for the AVX2 store)
	shared_buffer ret = alloc_shared_buffer(_source.size() / 4 * 3 + 3 + 32);

	// 2) prepare
	auto		src  = _source.data();
	size_t		size = _source.size();
	uint8_t*	out  = ret.data<uint8_t>();

#if defined(CGDK_SIMD_AVX2)
	// 3) AVX2
	if (size >= 32 && cpu::is_avx2_supported())
	{
		const auto processed = _decode_avx2(out, src, size);
		src  += processed;
		size -= processed;
		out  += processed / 4 * 3;
	}
#endif

	// 4) remained
	out = _decode_scalar(out, src, size);

	// 5) set size
	ret ^= static_cast<size_t>(out - ret.data<uint8_t>());

	// return)
	return ret;
}


}
