//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                  Common                                   *
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
// 1. String Functions
//                                                                            
//                                                                            
//-----------------------------------------------------------------------------
#include <string>	// C++ STL string
#include <string_view>
#include <cstdint>
#include <climits>	// LLONG_MAX / LONG_MAX
#include <iomanip>
#include <sstream>
#include <utility>
#include <type_traits>

namespace CGDK
{
	//-----------------------------------------------------------------------------
	// stream_xxx
	//
	//  - same output as std::ostringstream
	//  - strings, chars, bool, integers and floating points are appended directly
	//    (no ostringstream construction : locale, heap)
	//  - any other type (user operator<<, enum, pointer...) falls back to ostringstream
	//  - number formatting is done by the non-template functions in string_functions.cpp
	//    (keeps <charconv> out of this header : it's included almost everywhere)
	//-----------------------------------------------------------------------------
	namespace detail_stream
	{
		// - string_functions.cpp
		void			append_integer(std::string& _out, long long _value);
		void			append_integer(std::string& _out, unsigned long long _value);
		void			append_floating(std::string& _out, double _value);			// %g, precision 6
		void			append_floating(std::string& _out, long double _value);
		bool			append_fixed(std::string& _out, double _value, int _precision);	// false : too long
		bool			append_fixed(std::string& _out, long double _value, int _precision);
		void			pad(std::string& _out, int _width, char _fill, bool _right);
		std::string		format_hex(std::uint64_t _value, int _width, bool _upper, bool _prefix);

		template<class T> using bare_t = std::remove_cv_t<std::remove_reference_t<T>>;

		template<class T> constexpr bool is_char_v = std::is_same_v<T, char> || std::is_same_v<T, signed char> || std::is_same_v<T, unsigned char>;
		template<class T> constexpr bool is_integer_v = std::is_integral_v<T> && !is_char_v<T> && !std::is_same_v<T, bool>
			&& !std::is_same_v<T, wchar_t> && !std::is_same_v<T, char16_t> && !std::is_same_v<T, char32_t>
		#if defined(__cpp_char8_t)
			&& !std::is_same_v<T, char8_t>
		#endif
			;
		template<class T> constexpr bool is_string_v = !std::is_arithmetic_v<T> && !std::is_null_pointer_v<T> && std::is_convertible_v<const T&, std::string_view>;
		template<class T> constexpr bool is_fast_v = is_string_v<T> || is_char_v<T> || std::is_same_v<T, bool> || is_integer_v<T> || std::is_floating_point_v<T>;

		template<class T>
		inline void append(std::string& _out, const T& _value)
		{
			if constexpr (std::is_pointer_v<T>)
			{
				// - null char* : ostream sets badbit, here it is just empty
				if (_value != nullptr)
					_out.append(std::string_view(_value));
			}
			else if constexpr (is_string_v<T>)
			{
				_out.append(std::string_view(_value));
			}
			else if constexpr (is_char_v<T>)
			{
				_out.push_back(static_cast<char>(_value));
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				_out.push_back(_value ? '1' : '0');
			}
			else if constexpr (is_integer_v<T>)
			{
				if constexpr (std::is_signed_v<T>)
					append_integer(_out, static_cast<long long>(_value));
				else
					append_integer(_out, static_cast<unsigned long long>(_value));
			}
			else if constexpr (std::is_same_v<T, long double>)
			{
				append_floating(_out, _value);
			}
			else
			{
				// - float -> double is exact, so "%g" output is the same
				append_floating(_out, static_cast<double>(_value));
			}
		}
	}

	template<class... TARGS>
	inline std::string stream_string(TARGS&&... _args)
	{
		if constexpr ((detail_stream::is_fast_v<detail_stream::bare_t<TARGS>> && ...))
		{
			std::string result;
			(detail_stream::append<detail_stream::bare_t<TARGS>>(result, _args), ...);
			return result;
		}
		else
		{
			std::ostringstream oss;
			(oss << ... << std::forward<TARGS>(_args));
			return oss.str();
		}
	}

	template<class T>
	inline std::string stream_width(T&& _value, int _width, char _fill = ' ', bool _right = true)
	{
		if constexpr (detail_stream::is_fast_v<detail_stream::bare_t<T>>)
		{
			std::string result;
			detail_stream::append<detail_stream::bare_t<T>>(result, _value);
			detail_stream::pad(result, _width, _fill, _right);
			return result;
		}
		else
		{
			std::ostringstream oss;
			if (_right == false)
				oss << std::left;
			oss << std::setw(_width) << std::setfill(_fill) << std::forward<T>(_value);
			return oss.str();
		}
	}

	template<class T>
	inline std::string stream_hex(T _value, int _width = 0, bool _upper = false, bool _prefix = false)
	{
		return detail_stream::format_hex(static_cast<std::uint64_t>(_value), _width, _upper, _prefix);
	}

	template<class T>
	inline std::string stream_fixed(T _value, int _precision, int _width = 0)
	{
		using type = detail_stream::bare_t<T>;
		if constexpr (detail_stream::is_integer_v<type>)
		{
			// - std::fixed doesn't affect integers
			std::string str_result;
			detail_stream::append<type>(str_result, _value);
			detail_stream::pad(str_result, _width, ' ', true);
			return str_result;
		}
		else
		{
			if constexpr (std::is_floating_point_v<type>)
			{
				// - float -> double is exact, so "%.*f" output is the same
				std::string str_result;
				bool done;
				if constexpr (std::is_same_v<type, long double>)
					done = detail_stream::append_fixed(str_result, _value, _precision);
				else
					done = detail_stream::append_fixed(str_result, static_cast<double>(_value), _precision);

				if (done)
				{
					detail_stream::pad(str_result, _width, ' ', true);
					return str_result;
				}
			}

			// - others (or too long)
			std::ostringstream oss;
			if (_width > 0)
				oss << std::setw(_width);
			oss << std::fixed << std::setprecision(_precision) << _value;
			return oss.str();
		}
	}
}



//-----------------------------------------------------------------------------
//                                                                            
// 1. Definition
//
//-----------------------------------------------------------------------------
#define	MAX_STRING_LENGTH	2048


//-----------------------------------------------------------------------------
//                                                                            
// 2. Character type
//
//-----------------------------------------------------------------------------
// 1) _CGTEXT
#define	__CGTEXT_CHAR_(quote)		quote
#define	__CGTEXT_WCHAR_(quote)		L##quote
#define	__CGTEXT_UTF8_(quote)		u8##quote

#define	_CGTEXT_CHAR(quote)			__CGTEXT_CHAR_(quote)
#define	_CGTEXT_WCHAR(quote)		__CGTEXT_WCHAR_(quote)
#define	_CGTEXT_UTF8(quote)			__CGTEXT_UTF8_(quote)


//-----------------------------------------------------------------------------
// converting functions 
//-----------------------------------------------------------------------------
namespace CGDK
{
std::wstring_view					encode_string(wchar_t* _dest, std::string_view _source, size_t _size = MAX_STRING_LENGTH);
std::string_view					encode_string(char* _dest, std::wstring_view _source, size_t _size = MAX_STRING_LENGTH);

std::string							__to_lower(std::string_view _source);

int									__stricmp(const char* _lhs, const char* _rhs) noexcept;
int									__stricmp(std::string_view _lhs, std::string_view _rhs);

}
