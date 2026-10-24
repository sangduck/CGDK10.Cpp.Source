#include "pch.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <cstdlib>
#if !defined(_MSC_VER)
#include <strings.h>
#endif
#include "cgdk/sdk10/system/object/util.cpu_feature.h"

namespace CGDK
{

#if defined(CGDK_SIMD_AVX2)
//-----------------------------------------------------------------------------
// ASCII fast-path (AVX2)
//
//  - most logs / names / json are ASCII. convert the leading ASCII run with
//    AVX2 and let the scalar loop handle the rest (from the first non-ASCII byte).
//  - works for both a 16-bit wchar_t (windows) and a 32-bit wchar_t (linux).
//-----------------------------------------------------------------------------
// - UTF-8 -> wchar : zero-extend the leading ASCII bytes. returns the count converted.
CGDK_TARGET_AVX2 static std::size_t _widen_ascii_avx2(const char8_t* _src, wchar_t* _dst, std::size_t _count) noexcept
{
	std::size_t i = 0;
	for (; i + 32 <= _count; i += 32)
	{
		const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_src + i));

		// - any byte >= 0x80 : stop (let the scalar loop decode the multibyte sequence)
		if (_mm256_movemask_epi8(v) != 0)
			break;

		const __m128i lo = _mm256_castsi256_si128(v);
		const __m128i hi = _mm256_extracti128_si256(v, 1);

		if constexpr (sizeof(wchar_t) == 2)
		{
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i),      _mm256_cvtepu8_epi16(lo));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i + 16), _mm256_cvtepu8_epi16(hi));
		}
		else
		{
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i),      _mm256_cvtepu8_epi32(lo));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i + 8),  _mm256_cvtepu8_epi32(_mm_srli_si128(lo, 8)));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i + 16), _mm256_cvtepu8_epi32(hi));
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(_dst + i + 24), _mm256_cvtepu8_epi32(_mm_srli_si128(hi, 8)));
		}
	}

	// - avoid the AVX-SSE transition penalty
	_mm256_zeroupper();

	return i;
}

// - wchar -> UTF-8 : narrow the leading ASCII units. returns the count converted.
CGDK_TARGET_AVX2 static std::size_t _narrow_ascii_avx2(const wchar_t* _src, char8_t* _dst, std::size_t _count) noexcept
{
	std::size_t i = 0;

	if constexpr (sizeof(wchar_t) == 2)
	{
		const __m256i mask = _mm256_set1_epi16(static_cast<short>(0xFF80));
		for (; i + 16 <= _count; i += 16)
		{
			const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_src + i));	// 16 x u16

			// - any unit >= 0x80 : stop
			if (_mm256_testz_si256(v, mask) == 0)
				break;

			const __m128i lo = _mm256_castsi256_si128(v);
			const __m128i hi = _mm256_extracti128_si256(v, 1);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + i), _mm_packus_epi16(lo, hi));	// src[0..15]
		}
	}
	else
	{
		const __m256i mask = _mm256_set1_epi32(static_cast<int>(0xFFFFFF80));
		for (; i + 8 <= _count; i += 8)
		{
			const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_src + i));	// 8 x u32

			// - any unit >= 0x80 : stop
			if (_mm256_testz_si256(v, mask) == 0)
				break;

			const __m128i lo = _mm256_castsi256_si128(v);
			const __m128i hi = _mm256_extracti128_si256(v, 1);
			const __m128i p16 = _mm_packus_epi32(lo, hi);				// 8 x u16
			_mm_storel_epi64(reinterpret_cast<__m128i*>(_dst + i), _mm_packus_epi16(p16, p16));	// 8 x u8
		}
	}

	// - avoid the AVX-SSE transition penalty
	_mm256_zeroupper();

	return i;
}
#endif

std::string_view to_string(eCHARACTER_SET _character_set) noexcept
{
	std::string_view result;

	switch (_character_set)
	{
	case	eCHARACTER_SET::NONE:	result = "none"sv; break;
	case	eCHARACTER_SET::MBCS:	result = "mbcs"sv; break;
	case	eCHARACTER_SET::UTF8:	result = "utf8"sv; break;
	case	eCHARACTER_SET::UCS2:	result = "ucs2"sv; break;
	case	eCHARACTER_SET::UCS4:	result = "ucs4"sv; break;
	default:						result = ""sv; break;
	}

	return result;
}

std::string_view to_string(eEOL_TYPE _eol_code) noexcept
{
	std::string_view result;

	switch (_eol_code)
	{
	case	eEOL_TYPE::NONE:	result = "none"sv; break;
	case	eEOL_TYPE::CRLF:	result = "crlf"sv; break;
	case	eEOL_TYPE::LF:		result = "lf"sv; break;
	case	eEOL_TYPE::CR:		result = "cr"sv; break;
	case	eEOL_TYPE::NULL_:	result = "null"sv; break;
	case	eEOL_TYPE::CR_NULL:	result = "cr_null"sv; break;
	default:					result = ""sv; break;
	}

	return	result;
}

std::string_view to_string(eOS_TYPE _os_type) noexcept
{
	// declare) 
	std::string_view	str;

	switch (_os_type)
	{
	case	eOS_TYPE::NOT_DEFINED:			str = "NOT_DEFINED"; break;
	case	eOS_TYPE::IOS:					str = "IOS"; break;
	case	eOS_TYPE::ANDROID:				str = "ANDROID"; break;
	case	eOS_TYPE::WINDOWS_NATIVE:		str = "WINDOWS_NATIVE"; break;
	case	eOS_TYPE::WINDOWS_UNIVERSAL:	str = "WINDOWS_UNIVERSAL"; break;
	case	eOS_TYPE::LINUX:				str = "LINUX"; break;
	case	eOS_TYPE::MACOS:				str = "MACOS"; break;
	default:								str = " - "; break;
	}

	// return)
	return	str;
};

std::string_view to_string(eLANGUAGE _language) noexcept
{
	// declare) 
	std::string_view	str;

	switch (_language)
	{
	case	eLANGUAGE::NOT_DEFINED:			str = "NOT_DEFINED"; break;
	case	eLANGUAGE::KOREAN:				str = "KOREAN"; break;
	case	eLANGUAGE::MANDARIN:			str = "MANDARIN"; break;
	case	eLANGUAGE::CANTONESE:			str = "CANTONESE"; break;
	case	eLANGUAGE::JAPANESE:			str = "JAPANESE"; break;
	case	eLANGUAGE::TAI:					str = "TAI"; break;
	case	eLANGUAGE::ENGLISH:				str = "ENGLISH"; break;
	case	eLANGUAGE::SPAINESE:			str = "SPAINESE"; break;
	case	eLANGUAGE::PORTUGUESE:			str = "PORTUGUESE"; break;
	case	eLANGUAGE::DEUTSCHESE:			str = "DEUTSCHESE"; break;
	case	eLANGUAGE::DUTCH:				str = "DUTCH"; break;
	case	eLANGUAGE::RUSSIAN:				str = "RUSSIAN"; break;
	default:								str = " - "; break;
	}

	// return)
	return	str;
};

namespace
{
	// - true if all chars are ASCII (16 bytes per iteration)
	[[nodiscard]] inline bool _is_ascii(const char* _p, size_t _size) noexcept
	{
		size_t i = 0;
		for (; i + 16 <= _size; i += 16)
		{
			uint64_t a, b;
			memcpy(&a, _p + i, sizeof(a));
			memcpy(&b, _p + i + 8, sizeof(b));
			if (((a | b) & 0x8080808080808080ull) != 0)
				return false;
		}
		for (; i < _size; ++i)
		{
			if (static_cast<unsigned char>(_p[i]) >= 0x80)
				return false;
		}
		return true;
	}

	[[nodiscard]] inline bool _is_ascii(const wchar_t* _p, size_t _size) noexcept
	{
		uint32_t bits = 0;
		for (size_t i = 0; i < _size; ++i)
			bits |= static_cast<uint32_t>(_p[i]);
		return bits < 0x80;
	}

	// - temporary wchar_t buffer (stack for small strings)
	class _temp_wchar_buffer
	{
	public:
		explicit _temp_wchar_buffer(size_t _count)
		{
			if (_count > std::size(m_stack))
			{
				m_heap = alloc_shared_buffer(_count * sizeof(wchar_t));
				m_ptr = m_heap.data<wchar_t>();
			}
		}
		[[nodiscard]] wchar_t* data() noexcept { return this->m_ptr; }

	private:
		wchar_t			m_stack[512]{};
		shared_buffer	m_heap;
		wchar_t*		m_ptr = m_stack;
	};
}

std::wstring_view encode_string(wchar_t* _dest, std::string_view _source, size_t _size)
{
	// check)
	RETURN_IF(_dest == nullptr || _size == 0, std::wstring_view());

	// 1) ASCII : same in every MBCS code page -> just widen
	if (_source.size() < _size && _is_ascii(_source.data(), _source.size()))
	{
		size_t i = 0;
	#if defined(CGDK_SIMD_AVX2)
		if (cpu::is_avx2_supported())
			i = _widen_ascii_avx2(reinterpret_cast<const char8_t*>(_source.data()), _dest, _source.size());
	#endif
		for (; i < _source.size(); ++i)
			_dest[i] = static_cast<wchar_t>(static_cast<unsigned char>(_source[i]));
		_dest[_source.size()] = 0;
		return std::wstring_view(_dest, _source.size());
	}

#if defined(_WIN32)
	// 2) MultiByteToWideChar (_size - 1 : room for the terminator, returns 0 on failure)
	const auto result_length = MultiByteToWideChar(CP_ACP, 0, _source.data(), static_cast<int>(_source.size()), _dest, static_cast<int>(_size - 1));
	if (result_length <= 0)
	{
		_dest[0] = 0;
		return std::wstring_view();
	}

	_dest[result_length] = 0;
	return std::wstring_view(_dest, static_cast<size_t>(result_length));
#else
	// 2) mbstowcs (_source(string_view) may not be null-terminated, so convert a copy)
	std::string temp_source(_source);
	auto result_length = mbstowcs(_dest, temp_source.c_str(), _size - 1);
	RETURN_IF(result_length == static_cast<size_t>(-1), (_dest[0] = 0, std::wstring_view()));
	_dest[result_length] = 0;
	return std::wstring_view(_dest, result_length);
#endif
}

std::string_view encode_string(char* _dest, std::wstring_view _source, size_t _size)
{
	// check)
	RETURN_IF(_dest == nullptr || _size == 0, std::string_view());

	// 1) ASCII : same in every MBCS code page -> just narrow
	if (_source.size() < _size && _is_ascii(_source.data(), _source.size()))
	{
		size_t i = 0;
	#if defined(CGDK_SIMD_AVX2)
		if (cpu::is_avx2_supported())
			i = _narrow_ascii_avx2(_source.data(), reinterpret_cast<char8_t*>(_dest), _source.size());
	#endif
		for (; i < _source.size(); ++i)
			_dest[i] = static_cast<char>(_source[i]);
		_dest[_source.size()] = 0;
		return std::string_view(_dest, _source.size());
	}

#if defined(_WIN32)
	// 2) WideCharToMultiByte (_size - 1 : room for the terminator, returns 0 on failure)
	const auto result_length = WideCharToMultiByte(CP_ACP, 0, _source.data(), static_cast<int>(_source.size()), _dest, static_cast<int>(_size - 1), nullptr, nullptr);
	if (result_length <= 0)
	{
		_dest[0] = 0;
		return std::string_view();
	}

	_dest[result_length] = 0;
	return std::string_view(_dest, static_cast<size_t>(result_length));
#else
	// 2) wcstombs (_source(wstring_view) may not be null-terminated, so convert a copy)
	std::wstring temp_source(_source);
	auto result_length = wcstombs(_dest, temp_source.c_str(), _size - 1);
	RETURN_IF(result_length == static_cast<size_t>(-1), (_dest[0] = 0, std::string_view()));
	_dest[result_length] = 0;
	return std::string_view(_dest, result_length);
#endif
}
std::u8string_view encode_string(char8_t* _dest, std::string_view _source, size_t _size)
{
	// check)
	RETURN_IF(_dest == nullptr || _size == 0, std::u8string_view());

#if defined(_WIN32)
	// 1) not ASCII : MBCS -> UCS2 -> UTF-8 (UCS2 count <= MBCS bytes)
	if (!_is_ascii(_source.data(), _source.size()))
	{
		const size_t count = _source.size() + 1;
		_temp_wchar_buffer temp(count);
		const auto wide = encode_string(temp.data(), _source, count);
		return encode_string(_dest, wide, _size);
	}
#endif

	// 2) ASCII, or linux (std::string is UTF-8 already) : copy
	const auto copy_length = (_source.size() < _size) ? _source.size() : (_size - 1);
	memcpy(_dest, _source.data(), copy_length);
	_dest[copy_length] = 0;
	return std::u8string_view(_dest, copy_length);
}
std::u8string_view encode_string(char8_t* _dest, std::wstring_view _source, size_t _size)
{
	// 1) 값을 복사한다.
	const wchar_t* pch_read_now = _source.data();
	const wchar_t* pch_read_end = pch_read_now + _source.size();
	char8_t* pch_write_now = _dest;
	[[maybe_unused]] char8_t* pch_write_end = _dest + _size;

	// - ASCII fast-path (AVX2) : an ASCII unit becomes 1 byte, so the destination always has room
#if defined(CGDK_SIMD_AVX2)
	if (cpu::is_avx2_supported())
	{
		const auto done = _narrow_ascii_avx2(pch_read_now, pch_write_now, static_cast<std::size_t>(pch_read_end - pch_read_now));
		pch_read_now += done;
		pch_write_now += done;
	}
#endif

	for (; pch_read_now != pch_read_end; )
	{
		wchar_t	ch_read_now = *pch_read_now++;

		// Case-1) 
		if (ch_read_now < 0x80)
		{
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = (char)ch_read_now;
		}
		// Case-2) 110xxxxx 10xxxxxx
		else if (ch_read_now < 0x800)
		{
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = (((ch_read_now << 2) & 0x1f00) | 0xc000) >> 8;
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = ((ch_read_now & 0x3f) | 0x80);
		}
		// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
		// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000 ~ U+10FFFF)
		else
		{
			auto code = static_cast<uint32_t>(ch_read_now);

			// - UTF-16 surrogate pair (wchar_t is 16 bits on Windows)
			if (code >= 0xd800 && code <= 0xdbff && pch_read_now != pch_read_end
			 && static_cast<uint32_t>(*pch_read_now) >= 0xdc00 && static_cast<uint32_t>(*pch_read_now) <= 0xdfff)
			{
				code = 0x10000 + ((code - 0xd800) << 10) + (static_cast<uint32_t>(*pch_read_now++) - 0xdc00);
			}
			// - lone surrogate or out of range -> U+FFFD (replacement character)
			else if ((code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff)
			{
				code = 0xfffd;
			}

			if (code < 0x10000)
			{
				CGASSERT_ERROR(pch_write_now + 3 <= pch_write_end);
				*pch_write_now++ = (char8_t)(0xe0 | (code >> 12));
				*pch_write_now++ = (char8_t)(0x80 | ((code >> 6) & 0x3f));
				*pch_write_now++ = (char8_t)(0x80 | (code & 0x3f));
			}
			else
			{
				CGASSERT_ERROR(pch_write_now + 4 <= pch_write_end);
				*pch_write_now++ = (char8_t)(0xf0 | (code >> 18));
				*pch_write_now++ = (char8_t)(0x80 | ((code >> 12) & 0x3f));
				*pch_write_now++ = (char8_t)(0x80 | ((code >> 6) & 0x3f));
				*pch_write_now++ = (char8_t)(0x80 | (code & 0x3f));
			}
		}
	}

	// - terminate (callers allocate 4 bytes per char, so 4-byte sequences can fill it up)
	if (pch_write_now < pch_write_end)
		*pch_write_now = 0;

	// 3) 길이를 리턴한다
	return std::u8string_view{ _dest, size_t(pch_write_now - _dest) };
}
std::string_view encode_string_to_utf8(char* _dest, std::wstring_view _source, size_t _size)
{
	auto result = encode_string(reinterpret_cast<char8_t*>(_dest), _source, _size);

	return std::string_view(reinterpret_cast<const char*>(result.data()), result.size());
}

std::string_view encode_string(char* _dest, std::u8string_view _source, size_t _size)
{
	// check)
	RETURN_IF(_dest == nullptr || _size == 0, std::string_view());

	const auto source = reinterpret_cast<const char*>(_source.data());

#if defined(_WIN32)
	// 1) not ASCII : UTF-8 -> UCS2 -> MBCS (UCS2 count <= UTF-8 bytes)
	if (!_is_ascii(source, _source.size()))
	{
		const size_t count = _source.size() + 1;
		_temp_wchar_buffer temp(count);
		const auto wide = encode_string(temp.data(), _source, count);
		return encode_string(_dest, wide, _size);
	}
#endif

	// 2) ASCII, or linux (std::string is UTF-8 already) : copy
	const auto copy_length = (_source.size() < _size) ? _source.size() : (_size - 1);
	memcpy(_dest, source, copy_length);
	_dest[copy_length] = 0;
	return std::string_view(_dest, copy_length);
}std::wstring_view encode_string(wchar_t* _dest, std::u8string_view _source, size_t _size)
{
	// declare) 
	std::wstring_view result;

	// 1) 값을 복사한다.
	const char8_t* pch_read_now	 = _source.data();
	const char8_t* pch_read_end	 = pch_read_now + _source.size();
	wchar_t* pch_write_now = _dest;
	[[maybe_unused]] wchar_t* pch_write_end = _dest + _size;

	// - ASCII fast-path (AVX2) : an ASCII byte becomes 1 wchar, so the destination always has room
#if defined(CGDK_SIMD_AVX2)
	if (cpu::is_avx2_supported())
	{
		const auto done = _widen_ascii_avx2(pch_read_now, pch_write_now, static_cast<std::size_t>(pch_read_end - pch_read_now));
		pch_read_now += done;
		pch_write_now += done;
	}
#endif

	for (; pch_read_now != pch_read_end; )
	{
		// - 첫 번째 문자를 읽는다.
		const uint32_t ch_read_1 = *pch_read_now++;

		// Case-1) 0x80보다 작으면 그냥 일반 코드
		if (ch_read_1 < 0x80)
		{
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = static_cast<wchar_t>(ch_read_1);
			continue;
		}

		// Case-2) 110xxxxx 10xxxxxx                   (U+0080 ~ U+07FF)
		// Case-3) 1110xxxx 10xxxxxx 10xxxxxx          (U+0800 ~ U+FFFF)
		// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000 ~ U+10FFFF)
		//  - the 2nd byte range excludes overlong, surrogate and > U+10FFFF (Unicode Table 3-7)
		//  - invalid lead byte, invalid continuation byte or truncated input
		//    -> U+FFFD for the maximal subpart, then continue from the failed byte
		size_t		count = 0;
		uint32_t	code = 0;
		uint8_t		lower = 0x80;
		uint8_t		upper = 0xbf;

		if (ch_read_1 >= 0xc2 && ch_read_1 <= 0xdf)
		{
			count = 1;
			code = ch_read_1 & 0x1f;
		}
		else if (ch_read_1 >= 0xe0 && ch_read_1 <= 0xef)
		{
			count = 2;
			code = ch_read_1 & 0x0f;
			if (ch_read_1 == 0xe0) lower = 0xa0;		// overlong
			else if (ch_read_1 == 0xed) upper = 0x9f;	// surrogate
		}
		else if (ch_read_1 >= 0xf0 && ch_read_1 <= 0xf4)
		{
			count = 3;
			code = ch_read_1 & 0x07;
			if (ch_read_1 == 0xf0) lower = 0x90;		// overlong
			else if (ch_read_1 == 0xf4) upper = 0x8f;	// > U+10FFFF
		}

		// - continuation bytes (never reads past pch_read_end)
		size_t read = 0;
		for (; read < count && pch_read_now != pch_read_end; ++read)
		{
			const uint8_t ch = *pch_read_now;
			if (ch < lower || ch > upper)
				break;

			code = (code << 6) | (ch & 0x3f);
			lower = 0x80;
			upper = 0xbf;
			++pch_read_now;
		}

		// - invalid -> U+FFFD (replacement character)
		if (count == 0 || read != count)
		{
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = static_cast<wchar_t>(0xfffd);
		}
		else if (code >= 0x10000)
		{
			// - UTF-16 surrogate pair (wchar_t is 16 bits on Windows)
			if constexpr (sizeof(wchar_t) == 2)
			{
				CGASSERT_ERROR(pch_write_now + 2 <= pch_write_end);
				*pch_write_now++ = static_cast<wchar_t>(0xd800 + ((code - 0x10000) >> 10));
				*pch_write_now++ = static_cast<wchar_t>(0xdc00 + ((code - 0x10000) & 0x3ff));
			}
			// - UTF-32 (wchar_t is 32 bits on Linux)
			else
			{
				CGASSERT_ERROR(pch_write_now < pch_write_end);
				*pch_write_now++ = static_cast<wchar_t>(code);
			}
		}
		else
		{
			CGASSERT_ERROR(pch_write_now < pch_write_end);
			*pch_write_now++ = static_cast<wchar_t>(code);
		}
	}

	// - terminate
	if (pch_write_now < pch_write_end)
		*pch_write_now = 0;

	result = std::wstring_view{ _dest, static_cast<size_t>(pch_write_now - _dest) };

	// 3) 길이를 리턴한다
	return result;
}

std::wstring_view encode_string_from_utf8(wchar_t* _dest, std::string_view _source, size_t _size)
{
	return encode_string(_dest, std::u8string_view(reinterpret_cast<const char8_t*>(_source.data()), _source.size()), _size);
}

std::size_t __convert_text(void* _dst, std::size_t _dst_count, eCHARACTER_SET _dst_character_set, const void* _src, std::size_t _src_count, eCHARACTER_SET _src_character_set)
{
	// check)
	if (_dst == nullptr || _dst_count == 0)
		return 0;

	// 1) same character set : copy (cut to fit, _dst_count - 1 : room for the terminator)
	if (_dst_character_set == _src_character_set)
	{
		if (_src_count > _dst_count - 1)
			_src_count = _dst_count - 1;

		if (_dst_character_set == eCHARACTER_SET::UCS2)
		{
			memcpy(_dst, _src, _src_count * sizeof(wchar_t));
			static_cast<wchar_t*>(_dst)[_src_count] = 0;
		}
		else
		{
			memcpy(_dst, _src, _src_count);
			static_cast<char*>(_dst)[_src_count] = 0;
		}
		return _src_count;
	}

	// 2) _dst may be too small for the worst case : convert into a temporary buffer and copy what fits
	const auto ratio = __get_convert_ratio(_dst_character_set, _src_character_set);
	if (_src_count > (_dst_count - 1) / ratio)
	{
		const auto unit = __get_unit_size(_dst_character_set);
		const auto temp_count = _src_count * ratio + 1;
		std::vector<char> temp(temp_count * unit);
		const auto converted = __convert_text(temp.data(), temp_count, _dst_character_set, _src, _src_count, _src_character_set);

		auto count = (converted < _dst_count - 1) ? converted : _dst_count - 1;

		// - UTF8 : don't cut a character
		if (_dst_character_set == eCHARACTER_SET::UTF8 && count < converted)
		{
			while (count > 0 && (static_cast<unsigned char>(temp[count]) & 0xc0) == 0x80)
				--count;
		}

		memcpy(_dst, temp.data(), count * unit);
		if (unit == sizeof(wchar_t))
			static_cast<wchar_t*>(_dst)[count] = 0;
		else
			static_cast<char*>(_dst)[count] = 0;

		return count;
	}

	// 3) convert
	const auto src_mbcs = std::string_view(static_cast<const char*>(_src), _src_count);
	const auto src_ucs2 = std::wstring_view(static_cast<const wchar_t*>(_src), _src_count);
	const auto src_utf8 = std::u8string_view(static_cast<const char8_t*>(_src), _src_count);

	switch (_dst_character_set)
	{
	case	eCHARACTER_SET::MBCS:
			if (_src_character_set == eCHARACTER_SET::UCS2)	return encode_string(static_cast<char*>(_dst), src_ucs2, _dst_count).size();
			if (_src_character_set == eCHARACTER_SET::UTF8)	return encode_string(static_cast<char*>(_dst), src_utf8, _dst_count).size();
			break;

	case	eCHARACTER_SET::UCS2:
			if (_src_character_set == eCHARACTER_SET::MBCS)	return encode_string(static_cast<wchar_t*>(_dst), src_mbcs, _dst_count).size();
			if (_src_character_set == eCHARACTER_SET::UTF8)	return encode_string(static_cast<wchar_t*>(_dst), src_utf8, _dst_count).size();
			break;

	case	eCHARACTER_SET::UTF8:
			if (_src_character_set == eCHARACTER_SET::MBCS)	return encode_string(static_cast<char8_t*>(_dst), src_mbcs, _dst_count).size();
			if (_src_character_set == eCHARACTER_SET::UCS2)	return encode_string(static_cast<char8_t*>(_dst), src_ucs2, _dst_count).size();
			break;

	default:
			break;
	}

	// error)
	CGASSERT_ERROR(false);

	// return)
	return 0;
}

void __append_line(shared_buffer& _dst, eCHARACTER_SET _dst_character_set, const void* _src, std::size_t _src_count, eCHARACTER_SET _src_character_set)
{
	// declare)
	const auto unit = __get_unit_size(_dst_character_set);

	// 1) length (set after converting)
	auto& length = _dst.append<COUNT_T>(0);

	// 2) text + terminator
	const auto written = __convert_text(_dst.data() + _dst.size(), _dst.get_remained_size() / unit, _dst_character_set, _src, _src_count, _src_character_set);
	_dst.add_size((written + 1) * unit);

	// 3) length (terminator included)
	length = static_cast<COUNT_T>(written + 1);
}

void __convert_lines(shared_buffer& _dst, eCHARACTER_SET _dst_character_set, const_buffer_view _src, eCHARACTER_SET _src_character_set)
{
	// 1) line count
	const auto count_line = _src.extract<COUNT_T>();
	_dst.append<COUNT_T>(count_line);

	// 2) lines
	for (COUNT_T i = 0; i < count_line; ++i)
	{
		if (_src_character_set == eCHARACTER_SET::UCS2)
		{
			const auto line = _src.extract<std::wstring_view>();
			__append_line(_dst, _dst_character_set, line.data(), line.size(), _src_character_set);
		}
		else
		{
			const auto line = _src.extract<std::string_view>();
			__append_line(_dst, _dst_character_set, line.data(), line.size(), _src_character_set);
		}
	}
}
namespace detail
{
	// - ASCII only (same as the "C" locale, no locale call)
	[[nodiscard]] constexpr unsigned char ascii_lower(unsigned char _ch) noexcept
	{
		return (static_cast<unsigned>(_ch - 'A') < 26u) ? static_cast<unsigned char>(_ch | 0x20) : _ch;
	}
}

int __stricmp(const char* _lhs, const char* _rhs) noexcept
{
	for (;; ++_lhs, ++_rhs)
	{
		const int lhs = detail::ascii_lower(static_cast<unsigned char>(*_lhs));
		const int rhs = detail::ascii_lower(static_cast<unsigned char>(*_rhs));
		if (lhs != rhs || lhs == 0)
			return lhs - rhs;
	}
}
int __stricmp(std::string_view _lhs, std::string_view _rhs)
{
	// - no allocation (compares the whole views, embedded '\0' included)
	const size_t count = (_lhs.size() < _rhs.size()) ? _lhs.size() : _rhs.size();
	for (size_t i = 0; i < count; ++i)
	{
		const int lhs = detail::ascii_lower(static_cast<unsigned char>(_lhs[i]));
		const int rhs = detail::ascii_lower(static_cast<unsigned char>(_rhs[i]));
		if (lhs != rhs)
			return lhs - rhs;
	}

	return (_lhs.size() < _rhs.size()) ? -1 : (_lhs.size() > _rhs.size()) ? 1 : 0;
}
std::string __to_lower(std::string_view _source)
{
	std::string value(_source.size(), '\0');
	for (size_t i = 0; i < _source.size(); ++i)
		value[i] = static_cast<char>(detail::ascii_lower(static_cast<unsigned char>(_source[i])));
	return value;
}


}