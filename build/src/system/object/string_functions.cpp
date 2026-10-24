#include "pch.h"
#include <charconv>

//-----------------------------------------------------------------------------
// stream_xxx helpers (declared in cgdk/sdk10/common/string_functions.h)
//
//  - number formatting with std::to_chars (same output as std::ostringstream)
//  - kept out of the header so <charconv> isn't included by every source file
//-----------------------------------------------------------------------------
namespace CGDK
{
namespace detail_stream
{

void append_integer(std::string& _out, long long _value)
{
	char temp[24];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value);
	_out.append(temp, result.ptr);
}

void append_integer(std::string& _out, unsigned long long _value)
{
	char temp[24];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value);
	_out.append(temp, result.ptr);
}

void append_floating(std::string& _out, double _value)
{
	// - ostream default : %g, precision 6
	char temp[48];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value, std::chars_format::general, 6);
	_out.append(temp, result.ptr);
}

void append_floating(std::string& _out, long double _value)
{
	char temp[48];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value, std::chars_format::general, 6);
	_out.append(temp, result.ptr);
}

bool append_fixed(std::string& _out, double _value, int _precision)
{
	// - "%.*f" (negative precision : 6 like printf)
	char temp[384];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value, std::chars_format::fixed, (_precision < 0) ? 6 : _precision);
	if (result.ec != std::errc{})
		return false;

	_out.append(temp, result.ptr);
	return true;
}

bool append_fixed(std::string& _out, long double _value, int _precision)
{
	char temp[384];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value, std::chars_format::fixed, (_precision < 0) ? 6 : _precision);
	if (result.ec != std::errc{})
		return false;

	_out.append(temp, result.ptr);
	return true;
}

void pad(std::string& _out, int _width, char _fill, bool _right)
{
	if (_width > 0 && static_cast<std::size_t>(_width) > _out.size())
	{
		const auto count = static_cast<std::size_t>(_width) - _out.size();
		if (_right)
			_out.insert(0, count, _fill);
		else
			_out.append(count, _fill);
	}
}

std::string format_hex(std::uint64_t _value, int _width, bool _upper, bool _prefix)
{
	// 1) hex digits
	char temp[24];
	const auto result = std::to_chars(temp, temp + sizeof(temp), _value, 16);
	if (_upper)
	{
		for (auto p = temp; p != result.ptr; ++p)
		{
			if (*p >= 'a')
				*p = static_cast<char>(*p - ('a' - 'A'));
		}
	}

	// 2) "0x" + '0' padded digits (setw applies to the digits only)
	std::string str_result;
	if (_prefix)
		str_result.append("0x", 2);

	const auto length = static_cast<std::size_t>(result.ptr - temp);
	if (_width > 0 && static_cast<std::size_t>(_width) > length)
		str_result.append(static_cast<std::size_t>(_width) - length, '0');

	str_result.append(temp, length);
	return str_result;
}

}
}
