#include "pch.h"
#include <cstring>
#include <charconv>


// ----------------------------------------------------------------------------
// to_string scaled (1000) / to_string scaled byte (1024)
//
//  - "%.2f" + unit (K, M, G, T)
//  - the unit is chosen by the magnitude, so negative values are scaled too
// ----------------------------------------------------------------------------
namespace
{
	std::string _to_string_scaled(double _value, double _base)
	{
		constexpr char units[] = { 'K', 'M', 'G', 'T' };

		// 1) unit
		const double magnitude = (_value < 0) ? -_value : _value;
		double divisor = 1.0;
		int unit = -1;
		while (unit < 3 && magnitude >= divisor * _base)
		{
			divisor *= _base;
			++unit;
		}

		// 2) "%.2f" + unit
		char temp[64];
		auto result = std::to_chars(temp, temp + sizeof(temp) - 1, _value / divisor, std::chars_format::fixed, 2);
		if (result.ec != std::errc{})
			return std::string();
		if (unit >= 0)
			*result.ptr++ = units[unit];

		return std::string(temp, result.ptr);
	}
}

std::string CGDK::to_string_scaled(int64_t _value)			{ return _to_string_scaled(static_cast<double>(_value), 1000.0); }
std::string CGDK::to_string_scaled(uint64_t _value)			{ return _to_string_scaled(static_cast<double>(_value), 1000.0); }
std::string CGDK::to_string_scaled(float _value)			{ return _to_string_scaled(static_cast<double>(_value), 1000.0); }
std::string CGDK::to_string_scaled_byte(int64_t _value)		{ return _to_string_scaled(static_cast<double>(_value), 1024.0); }
std::string CGDK::to_string_scaled_byte(uint64_t _value)	{ return _to_string_scaled(static_cast<double>(_value), 1024.0); }
std::string CGDK::to_string_scaled_byte(float _value)		{ return _to_string_scaled(static_cast<double>(_value), 1024.0); }