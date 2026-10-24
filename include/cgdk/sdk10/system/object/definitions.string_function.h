//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              Factory Classes                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// Definitions for buffer
//
//-----------------------------------------------------------------------------
namespace CGDK
{
	
[[nodiscard]] std::u8string_view	encode_string (char8_t* _dest, std::string_view _source, size_t _size = MAX_STRING_LENGTH);
[[nodiscard]] std::u8string_view	encode_string (char8_t* _dest, std::wstring_view _source, size_t _size = MAX_STRING_LENGTH);
[[nodiscard]] std::string_view		encode_string_to_utf8 (char* _dest, std::wstring_view _source, size_t _size = MAX_STRING_LENGTH);

[[nodiscard]] std::string_view		encode_string (char* _dest, std::u8string_view _source, size_t _size = MAX_STRING_LENGTH);
[[nodiscard]] std::wstring_view		encode_string (wchar_t* _dest, std::u8string_view _source, size_t _size = MAX_STRING_LENGTH);
[[nodiscard]] std::wstring_view		encode_string_from_utf8	(wchar_t* _dest, std::string_view _source, size_t _size = MAX_STRING_LENGTH);

//-----------------------------------------------------------------------------
// text conversion between character sets (MBCS / UCS2(wchar_t) / UTF8)
//
//  - a unit is wchar_t for UCS2, char for MBCS / UTF8
//  - __get_convert_ratio : max destination units per source unit
//  - __convert_text      : _src (_src_count units) -> _dst (_dst_count units, terminator included)
//                          returns written units (terminator excluded)
//                          _src is cut to fit when _dst is too small (never overflows)
//  - __append_line       : appends "[COUNT_T length (terminator included)][text][terminator]"
//  - __convert_lines     : converts "[COUNT_T line count][line]..." (log message format)
//-----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t	__get_unit_size(eCHARACTER_SET _character_set) noexcept { return (_character_set == eCHARACTER_SET::UCS2) ? sizeof(wchar_t) : sizeof(char); }
[[nodiscard]] constexpr std::size_t	__get_convert_ratio(eCHARACTER_SET _dst, eCHARACTER_SET _src) noexcept { return (_dst == _src || _dst == eCHARACTER_SET::UCS2) ? 1 : 4; }
std::size_t __convert_text (void* _dst, std::size_t _dst_count, eCHARACTER_SET _dst_character_set, const void* _src, std::size_t _src_count, eCHARACTER_SET _src_character_set);
void __append_line (shared_buffer& _dst, eCHARACTER_SET _dst_character_set, const void* _src, std::size_t _src_count, eCHARACTER_SET _src_character_set);
void __convert_lines (shared_buffer& _dst, eCHARACTER_SET _dst_character_set, const_buffer_view _src, eCHARACTER_SET _src_character_set);

}