#include "pch.h"
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

//-----------------------------------------------------------------------------
// additive checksums (sum of the elements, modulo 2^32)
//  - the AVX2 path produces the same result as the scalar sum (wrap-around mod 2^32)
//-----------------------------------------------------------------------------
namespace CGDK
{
#if defined(CGDK_SIMD_AVX2)
	// - sum of 8-bit elements
	CGDK_TARGET_AVX2 static uint32_t _checksum8_avx2(const uint8_t* _p, size_t _count) noexcept
	{
		const __m256i zero = _mm256_setzero_si256();
		__m256i acc = _mm256_setzero_si256();
		size_t i = 0;
		for (; i + 32 <= _count; i += 32)
		{
			// - sad_epu8 : four 64-bit lanes, each the sum of its 8 bytes
			acc = _mm256_add_epi64(acc, _mm256_sad_epu8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(_p + i)), zero));
		}

		// - horizontal sum of the four lanes (low 32 bits are enough : mod 2^32)
		uint32_t sum = static_cast<uint32_t>(_mm256_extract_epi64(acc, 0) + _mm256_extract_epi64(acc, 1) + _mm256_extract_epi64(acc, 2) + _mm256_extract_epi64(acc, 3));
		_mm256_zeroupper();

		// - tail
		for (; i < _count; ++i)
			sum += _p[i];

		return sum;
	}

	// - sum of 16-bit elements
	CGDK_TARGET_AVX2 static uint32_t _checksum16_avx2(const uint16_t* _p, size_t _count) noexcept
	{
		__m256i acc = _mm256_setzero_si256();
		size_t i = 0;
		for (; i + 16 <= _count; i += 16)
		{
			const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_p + i));	// 16 x u16
			acc = _mm256_add_epi32(acc, _mm256_cvtepu16_epi32(_mm256_castsi256_si128(v)));
			acc = _mm256_add_epi32(acc, _mm256_cvtepu16_epi32(_mm256_extracti128_si256(v, 1)));
		}

		// - horizontal sum of the eight 32-bit lanes
		alignas(32) uint32_t lane[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(lane), acc);
		_mm256_zeroupper();
		uint32_t sum = lane[0] + lane[1] + lane[2] + lane[3] + lane[4] + lane[5] + lane[6] + lane[7];

		// - tail
		for (; i < _count; ++i)
			sum += _p[i];

		return sum;
	}

	// - sum of 32-bit elements
	CGDK_TARGET_AVX2 static uint32_t _checksum32_avx2(const uint32_t* _p, size_t _count) noexcept
	{
		__m256i acc = _mm256_setzero_si256();
		size_t i = 0;
		for (; i + 8 <= _count; i += 8)
			acc = _mm256_add_epi32(acc, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_p + i)));

		// - horizontal sum of the eight lanes
		alignas(32) uint32_t lane[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(lane), acc);
		_mm256_zeroupper();
		uint32_t sum = lane[0] + lane[1] + lane[2] + lane[3] + lane[4] + lane[5] + lane[6] + lane[7];

		// - tail
		for (; i < _count; ++i)
			sum += _p[i];

		return sum;
	}
#endif
}

uint32_t CGDK::edc::get_checksum8(uint8_t* _pdata, uint32_t _data_length)
{
#if defined(CGDK_SIMD_AVX2)
	if (cpu::is_avx2_supported())
		return _checksum8_avx2(_pdata, _data_length);
#endif

	// 1) sum of the bytes
	uint32_t tempResult = 0;
	uint8_t* tempEnd = _pdata + _data_length;

	for (; _pdata < tempEnd; _pdata++)
		tempResult += *_pdata;

	// return)
	return tempResult;
}

uint32_t CGDK::edc::get_checksum16(uint16_t* _pdata, uint32_t _data_length)
{
	// check) the byte length must be a multiple of 2
	CGASSERT((_data_length & 0x01) == 0, 0);

#if defined(CGDK_SIMD_AVX2)
	if (cpu::is_avx2_supported())
		return _checksum16_avx2(_pdata, _data_length >> 1);
#endif

	// 1) sum of the 16-bit elements
	uint32_t tempResult = 0;
	uint16_t* tempEnd = _pdata + (_data_length >> 1);

	for (; _pdata < tempEnd; _pdata++)
		tempResult += *_pdata;

	// return)
	return tempResult;
}

uint32_t CGDK::edc::get_checksum32(uint32_t* _pdata, uint32_t _data_length)
{
	// check) the byte length must be a multiple of 4
	CGASSERT((_data_length & 0x03) == 0, 0);

#if defined(CGDK_SIMD_AVX2)
	if (cpu::is_avx2_supported())
		return _checksum32_avx2(_pdata, _data_length >> 2);
#endif

	// 1) sum of the 32-bit elements
	uint32_t tempResult = 0;
	uint32_t* tempEnd = _pdata + (_data_length >> 2);

	for (; _pdata < tempEnd; _pdata++)
		tempResult += *_pdata;

	// return)
	return tempResult;
}
