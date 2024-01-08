#include "pch.h"


template<> std::string_view to_string<char>(eCHARACTER_SET _character_set) noexcept
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
template<> std::wstring_view to_string<wchar_t>(eCHARACTER_SET _character_set) noexcept
{
	std::wstring_view result;

	switch (_character_set)
	{
	case	eCHARACTER_SET::NONE:	result = L"none"sv; break;
	case	eCHARACTER_SET::MBCS:	result = L"mbcs"sv; break;
	case	eCHARACTER_SET::UTF8:	result = L"utf8"sv; break;
	case	eCHARACTER_SET::UCS2:	result = L"ucs2"sv; break;
	case	eCHARACTER_SET::UCS4:	result = L"ucs32"sv; break;
	default:						result = L""sv; break;
	}

	return result;
}

template<> std::string_view to_string<char>(eEOL_TYPE _eol_code) noexcept
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
template<> std::wstring_view to_string<wchar_t>(eEOL_TYPE _eol_code) noexcept
{
	std::wstring_view result;

	switch (_eol_code)
	{
	case	eEOL_TYPE::NONE:	result = L"none"sv; break;
	case	eEOL_TYPE::CRLF:	result = L"crlf"sv; break;
	case	eEOL_TYPE::LF:		result = L"lf"sv; break;
	case	eEOL_TYPE::CR:		result = L"cr"sv; break;
	case	eEOL_TYPE::NULL_:	result = L"null"sv; break;
	case	eEOL_TYPE::CR_NULL:	result = L"cr_null"sv; break;
	default:					result = L""sv; break;
	}

	return	result;
}


namespace CGDK
{

template <>
std::string_view to_string<char>(eOS_TYPE _os_type) noexcept
{
	// declare) 
	std::string_view	str;

	switch (_os_type)
	{
	case	eOS_TYPE::NOT_DEFINED:			str = "NOT_DEFINED"; break;
	case	eOS_TYPE::IOS:					str = "IOS"; break;
	case	eOS_TYPE::ANDROID:				str = "ANDROID"; break;
	case	eOS_TYPE::WINDOWS_NAVTIVE:		str = "WINDOWS_NAVTIVE"; break;
	case	eOS_TYPE::WINDOWS_UNIVERSAL:	str = "WINDOWS_UNIVERSAL"; break;
	case	eOS_TYPE::LINUX:				str = "LINUX"; break;
	case	eOS_TYPE::MACOS:				str = "MACOS"; break;
	default:								str = " - "; break;
	}

	// return)
	return	str;
};
template <>
std::wstring_view to_string<wchar_t>(eOS_TYPE _os_type) noexcept
{
	// declare) 
	std::wstring_view	str;

	switch (_os_type)
	{
	case	eOS_TYPE::NOT_DEFINED:			str = L"NOT_DEFINED"; break;
	case	eOS_TYPE::IOS:					str = L"IOS"; break;
	case	eOS_TYPE::ANDROID:				str = L"ANDROID"; break;
	case	eOS_TYPE::WINDOWS_NAVTIVE:		str = L"WINDOWS_NAVTIVE"; break;
	case	eOS_TYPE::WINDOWS_UNIVERSAL:	str = L"WINDOWS_UNIVERSAL"; break;
	case	eOS_TYPE::LINUX:				str = L"LINUX"; break;
	case	eOS_TYPE::MACOS:				str = L"MACOS"; break;
	default:								str = L" - "; break;
	}

	// return)
	return	str;
};

template<>
std::string_view to_string<char>(eLANGUAGE _language) noexcept
{
	// declare) 
	std::string_view	str;

	switch (_language)
	{
	case	eLANGUAGE::NOT_DEFINED:			str = "NOT_DEFINED"; break;
	case	eLANGUAGE::KOREAN:				str = "KOREAN"; break;
	case	eLANGUAGE::MANDARIN:			str = "MANDARIN"; break;
	case	eLANGUAGE::CANTONESE:			str = "CANTONESE"; break;
	case	eLANGUAGE::JANANESE:			str = "JANANESE"; break;
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
template<>
std::wstring_view to_string<wchar_t>(eLANGUAGE _language) noexcept
{
	// declare) 
	std::wstring_view	str;

	switch (_language)
	{
	case	eLANGUAGE::NOT_DEFINED:			str = L"NOT_DEFINED"; break;
	case	eLANGUAGE::KOREAN:				str = L"KOREAN"; break;
	case	eLANGUAGE::MANDARIN:			str = L"MANDARIN"; break;
	case	eLANGUAGE::CANTONESE:			str = L"CANTONESE"; break;
	case	eLANGUAGE::JANANESE:			str = L"JANANESE"; break;
	case	eLANGUAGE::TAI:					str = L"TAI"; break;
	case	eLANGUAGE::ENGLISH:				str = L"ENGLISH"; break;
	case	eLANGUAGE::SPAINESE:			str = L"SPAINESE"; break;
	case	eLANGUAGE::PORTUGUESE:			str = L"PORTUGUESE"; break;
	case	eLANGUAGE::DEUTSCHESE:			str = L"DEUTSCHESE"; break;
	case	eLANGUAGE::DUTCH:				str = L"DUTCH"; break;
	case	eLANGUAGE::RUSSIAN:				str = L"RUSSIAN"; break;
	default:								str = L" - "; break;
	}

	// return)
	return	str;
};

std::u8string_view encode_string(char8_t* _dest, std::string_view _source, size_t _size)
{
	//// wide to UTF-8
	//std::wstring_convert<std::codecvt_utf8<wchar_t>>	convert;
	//return	convert.to_bytes(_source);

	auto size = _source.size() * sizeof(wchar_t);

	// 1) ucs2로 변환하기 위한 버퍼를 할당한다.
	auto temp_buf = alloc_shared_buffer(size);

	// 2) ucs2로 변환을 먼저한다.
	auto result = encode_string(temp_buf.data<wchar_t>(), _source, size);

	// 3) utf-8보 변환한다.
	return encode_string(_dest, result, _size);
}
std::u8string_view encode_string(char8_t* _dest, std::wstring_view _source, size_t /*_size*/)
{
	// 1) 값을 복사한다.
	const wchar_t* pch_read_now = _source.data();
	const wchar_t* pch_read_end = pch_read_now + _source.size();
	char8_t* pch_write_now = _dest;

	for (; pch_read_now != pch_read_end; )
	{
		wchar_t	ch_read_now = *pch_read_now++;

		// Case-1) 
		if (ch_read_now < 0x80)
		{
			*pch_write_now++ = (char)ch_read_now;
		}
		// Case-2) 110xxxxx 10xxxxxx
		else if (ch_read_now < 0x800)
		{
			*pch_write_now++ = (((ch_read_now << 2) & 0xcf00) | 0xc000) >> 8;
			*pch_write_now++ = ((ch_read_now & 0x3f) & 0x80);
		}
		// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
		else
		{
			*pch_write_now++ = (char8_t)((((ch_read_now << 4) & 0x3f0000) | 0xe00000) >> 16);
			*pch_write_now++ = (char8_t)((((ch_read_now << 2) & 0x3f00) | 0x8000) >> 8);
			*pch_write_now++ = (char8_t)(((ch_read_now & 0x3f) | 0x80));
		}
	}
	*pch_write_now = 0;

	// 3) 길이를 리턴한다
	return std::u8string_view{ _dest, size_t(pch_write_now - _dest) };
}
std::string_view encode_string_to_utf8(char* _dest, std::wstring_view _source, size_t _size)
{
	auto result = encode_string(reinterpret_cast<char8_t*>(_dest), _source, _size);

	return std::string_view(reinterpret_cast<const char*>(result.data()), result.size());
}

std::u8string_view encode_string(char8_t* /*_dest*/, std::u16string_view /*_source*/, size_t /*_size*/)
{
	return std::u8string_view();
}
std::u8string_view encode_string(char8_t* /*_dest*/, std::u32string_view /*_source*/, size_t /*_size*/)
{
	return std::u8string_view();
}

std::u16string_view encode_string(char16_t* /*_dest*/, std::u8string_view /*_source*/, size_t /*_size*/)
{
	return std::u16string_view();
}
std::u32string_view encode_string(char32_t* /*_dest*/, std::u8string_view /*_source*/, size_t /*_size*/)
{
	return std::u32string_view();
}
std::string_view encode_string(char* _dest, std::u8string_view _source, size_t _size)
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(_source.size() * sizeof(wchar_t));

	// 2) ...
	auto result = encode_string(buf_temp.data<wchar_t>(), _source, buf_temp.size());

	// 3) convert to unicode
	return encode_string(_dest, result, _size);
}
std::wstring_view encode_string(wchar_t* _dest, std::u8string_view _source, size_t /*_size*/)
{
	// declare) 
	std::wstring_view result;

	// 1) 값을 복사한다.
	const char8_t* pch_read_now	 = _source.data();
	const char8_t* pch_read_end	 = pch_read_now + _source.size();
	wchar_t*	   pch_write_now = _dest;

	for (; pch_read_now != pch_read_end; )
	{
		// - 첫번째 문자를 읽는다.
		wchar_t	ch_read_1 = *pch_read_now++;

		// Case-1) 0x80보다 작으면 그냥 일반 코드
		if ((ch_read_1 & 0x80) == 0)
		{
			*pch_write_now++ = ch_read_1;
		}
		// Case-2) 110xxxxx 10xxxxxx
		else if ((ch_read_1 & 0xe0) == 0xc0)
		{
			// - 두번째 문자 읽기
			wchar_t	ch_read_2 = (wchar_t)*pch_read_now++;

			// check) NULL이 돌아오면 Exception을 던진다.
			if(ch_read_2 == 0)
				return	result;

			// - 문자를 작성한다.
			*pch_write_now++ = ((ch_read_1 & 0x1f) << 6) | (ch_read_2 & 0x3f);
		}
		// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
		else if ((ch_read_1 & 0xf0) == 0xe0)
		{
			// - 두번째 문자 읽기
			wchar_t	ch_read_2 = (wchar_t)*pch_read_now++;

			// check) NULL이 돌아오면 Exception을 던진다.
			if (ch_read_2 == 0)
				return result;

			// - 세번째 문자 읽기
			wchar_t	ch_read_3 = (wchar_t)*pch_read_now++;

			// check) ch_read_3이 0이면 여기서 끝낸다.
			if (ch_read_3 == 0)
				return	result;

			// - 문자를 작성한다.
			*pch_write_now++ = ((ch_read_1 & 0x0f) << 12) | ((ch_read_2 & 0x3f) << 6) | (ch_read_3 & 0x3f);
		}
		// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		else if ((ch_read_1 & 0xf8) == 0xf0)
		{
			// - 두번째 문자 읽기
			wchar_t	ch_read_2 = (wchar_t)*pch_read_now++;

			// check) NULL이 돌아오면 Exception을 던진다.
			if (ch_read_2 == 0)
				return result;

			// - 세번째 문자 읽기
			wchar_t	ch_read_3 = (wchar_t)*pch_read_now++;

			// check) ch_read_3이 0이면 여기서 끝낸다.
			if (ch_read_3 == 0)
				return result;

			// - 네번째 문자 읽기
			wchar_t	ch_read_4 = (wchar_t)*pch_read_now++;

			// check) ch_read_4이 0이면 여기서 끝낸다.
			if (ch_read_4 == 0)
				return result;

			// - 문자를 작성한다.
			*pch_write_now++ = ((ch_read_1 & 0x07) << 18) | ((ch_read_2 & 0x3f) << 12) | ((ch_read_3 & 0x3f) << 6) | (ch_read_4 & 0x3f);
		}
	}
	*pch_write_now = 0;

	result = std::wstring_view{ _dest, static_cast<size_t>(pch_write_now - _dest) };

	// 3) 길이를 리턴한다
	return result;
}

std::wstring_view encode_string_from_utf8(wchar_t* _dest, std::string_view _source, size_t _size)
{
	return encode_string(_dest, std::u8string_view(reinterpret_cast<const char8_t*>(_source.data()), _source.size()), _size);
}

void __convert_unicode_to_utf8(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 2) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 3) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// 4) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract source string
		auto str_src = _buf_src.extract<std::wstring_view>();

		// - make room for writing string_length
		auto& str_len = _buf_dst.append<COUNT_T>(0);

		// - convert string
		auto size_added = encode_string(_buf_dst.data<char8_t>() + _buf_dst.size(), str_src).size();

		// - write length
		str_len = static_cast<COUNT_T>(size_added + 1);
		_buf_dst.add_size(size_added * sizeof(char8_t));
		_buf_dst.append<char8_t>(0);
	}
}
void __convert_unicode_to_mbcs(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 1) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 2) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// 3) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract한다.
		auto str_src = _buf_src.extract<std::wstring_view>();

		// - make room for writing string_length
		auto& str_len = _buf_dst.append<COUNT_T>();

		// - convert string
		auto size_added = encode_string(_buf_dst.data<char>() + _buf_dst.size(), str_src).size();

		// - write length
		str_len = static_cast<COUNT_T>(size_added + 1);
		_buf_dst.add_size(size_added * sizeof(char));
		_buf_dst.append<char>(0);
	}
}
void __convert_mbcs_to_unicode(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 1) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 2) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// 3) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract source string
		auto str_src = _buf_src.extract<std::string_view>();

		// - make room for writing string_length
		auto& str_len = _buf_dst.append<COUNT_T>(0);

		// - convert string
		auto size_added = encode_string(_buf_dst.data<wchar_t>(_buf_dst.size()), str_src, MAX_TRACE_STRING_LENGTH).size();

		// - write length
		str_len = static_cast<uint32_t>(size_added + 1);
		_buf_dst.add_size(size_added * sizeof(wchar_t));
		_buf_dst.append<wchar_t>(0);
	}
}
void __convert_utf8_to_unicode(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 1) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 2) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// 3) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract source string
		auto str_src = _buf_src.extract<std::u8string_view>();

		// - make room for writing string_length
		auto& str_len = _buf_dst.append<COUNT_T>(0);

		// - 변환한다.
		auto size_added = encode_string(_buf_dst.data<wchar_t>(_buf_dst.size()), str_src).size();

		// - write length
		str_len = static_cast<uint32_t>(size_added + 1);
		_buf_dst.add_size(size_added * sizeof(wchar_t));
		_buf_dst.append<wchar_t>(0);
	}
}
void __convert_mbcs_to_utf8(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 1) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 2) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// declare)
	auto source_length = _buf_src.size() + 1;

	// declare) temporary shared_buffer
	auto temp_buffer = alloc_shared_buffer(source_length * sizeof(wchar_t) + 64);

	// 3) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract source string
		auto str_src = _buf_src.extract<std::string_view>();

		// - make room for writing string_length
		auto& str_len = _buf_dst.append<COUNT_T&>();

		// declare) temporary shared_buffer for converting
		auto* temp_dst = temp_buffer.data<wchar_t>();

		// - convert mbcs string to unicode
		auto str_temp = encode_string(temp_dst, str_src, source_length);

		// - convert unicode string to utf8
		auto size_added = encode_string(_buf_dst.data<char8_t>() + _buf_dst.size(), str_temp).size();

		// - write length
		str_len = static_cast<COUNT_T>(size_added + 1);
		_buf_dst.add_size(size_added + sizeof(char8_t));
		_buf_dst.append<char8_t>(0);
	}
}
void __convert_utf8_to_mbcs(shared_buffer& _buf_dst, const_buffer_view _buf_src)
{
	// 1) line수를 얻는다.
	auto count_line = _buf_src.extract<COUNT_T>();

	// 2) line수를 써넣는다.
	_buf_dst.append<COUNT_T>(count_line);

	// declare)
	auto source_length = _buf_src.size();

	// declare) temporary shared_buffer
	auto temp_buffer = alloc_shared_buffer(source_length * sizeof(wchar_t) + 64);

	// 3) 모두 convert해서 추가한다.
	for (auto i = 0; i < count_line; ++i)
	{
		// - extract source string
		auto str_src = _buf_src.extract<std::u8string_view>();

		// - 문자열 길이를 써넣는다.
		auto& str_len = _buf_dst.append<COUNT_T&>();

		// - target
		auto* temp_dst = temp_buffer.data<wchar_t>();

		// declare) temporary shared_buffer for converting
		auto str_temp = encode_string(temp_dst, str_src, source_length);

		// - unicode를 mbcs로 변환한다.
		auto size_added = encode_string(_buf_dst.data<char>() + _buf_dst.size(), str_temp).size();

		// - write length
		str_len = static_cast<COUNT_T>(size_added + 1);
		_buf_dst.add_size(size_added * sizeof(char));
		_buf_dst.append<char>(0);
	}
}


}