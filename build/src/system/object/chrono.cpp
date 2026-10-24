#include "pch.h"
#include <charconv>


namespace CGDK
{

namespace
{
	// - scanf "%Nu" : leading white spaces skipped, up to _width chars (an optional sign included)
	//   ('-' is accepted and wraps around like scanf : "-8" -> 4294967288)
	bool _scan_uint(std::string_view& _source, uint32_t& _value, size_t _width = std::string_view::npos) noexcept
	{
		size_t pos = 0;
		while (pos < _source.size() && (_source[pos] == ' ' || (_source[pos] >= '\t' && _source[pos] <= '\r')))
			++pos;

		const size_t end = (_width < _source.size() - pos) ? pos + _width : _source.size();

		bool negative = false;
		if (pos < end && (_source[pos] == '+' || _source[pos] == '-'))
		{
			negative = (_source[pos] == '-');
			++pos;
		}

		uint32_t value = 0;
		const auto result = std::from_chars(_source.data() + pos, _source.data() + end, value);
		if (result.ec != std::errc{})
			return false;

		_value = negative ? (0u - value) : value;
		_source.remove_prefix(static_cast<size_t>(result.ptr - _source.data()));
		return true;
	}

	// - scanf literal (a white space matches any white spaces)
	bool _scan_literal(std::string_view& _source, char _ch) noexcept
	{
		if (_ch == ' ')
		{
			while (!_source.empty() && (_source.front() == ' ' || (_source.front() >= '\t' && _source.front() <= '\r')))
				_source.remove_prefix(1);
			return true;
		}

		if (_source.empty() || _source.front() != _ch)
			return false;

		_source.remove_prefix(1);
		return true;
	}

	// - custom format : sscanf with null-terminated copies (string_view may not be null-terminated)
	template <class... TARGS>
	int _scan_format(std::string_view _source, std::string_view _format, TARGS*... _args) noexcept
	{
		const std::string str_source(_source);
		const std::string str_format(_format);
	#if defined(_MSC_VER)
		return sscanf_s(str_source.c_str(), str_format.c_str(), _args...);
	#else
		#if defined(__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wformat-nonliteral"
		#endif
		return std::sscanf(str_source.c_str(), str_format.c_str(), _args...);
		#if defined(__GNUC__)
		#pragma GCC diagnostic pop
		#endif
	#endif
	}
}

chrono::date_time chrono::from_string_to_date_time(const std::string_view _string, const std::string_view _format)
{
    // declare) 
    uint32_t t_year = 0, t_month = 0, t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    // 1) "%04u-%02u-%02u %02u:%02u:%02u.%09u" (default) or custom format
    if (_format.empty())
    {
        std::string_view source = _string;
        (void)(_scan_uint(source, t_year, 4) && _scan_literal(source, '-') && _scan_uint(source, t_month, 2) && _scan_literal(source, '-') && _scan_uint(source, t_day, 2)
            && _scan_literal(source, ' ') && _scan_uint(source, t_hour, 2) && _scan_literal(source, ':') && _scan_uint(source, t_minute, 2) && _scan_literal(source, ':') && _scan_uint(source, t_second, 2)
            && _scan_literal(source, '.') && _scan_uint(source, t_fraction, 9));
    }
    else
    {
        _scan_format(_string, _format, &t_year, &t_month, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);
    }
    chrono::date_time temp_time;
    temp_time.t_year = static_cast<int16_t>(t_year);
    temp_time.t_month = static_cast<uint16_t>(t_month);
    temp_time.t_day = static_cast<uint16_t>(t_day);
    temp_time.t_hour = static_cast<uint16_t>(t_hour);
    temp_time.t_minute = static_cast<uint16_t>(t_minute);
    temp_time.t_second = static_cast<uint16_t>(t_second);
    temp_time.t_fraction = static_cast<uint32_t>(t_fraction);

    return temp_time;
}

chrono::day_time chrono::from_string_to_duration(const std::string_view _string, const std::string_view _format)
{
    // declare) 
    uint32_t t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    // 1) "%ud %uh %um %us.:%u" (default) or custom format
    if (_format.empty())
    {
        std::string_view source = _string;
        (void)(_scan_uint(source, t_day) && _scan_literal(source, 'd') && _scan_literal(source, ' ') && _scan_uint(source, t_hour) && _scan_literal(source, 'h')
            && _scan_literal(source, ' ') && _scan_uint(source, t_minute) && _scan_literal(source, 'm') && _scan_literal(source, ' ') && _scan_uint(source, t_second)
            && _scan_literal(source, 's') && _scan_literal(source, '.') && _scan_literal(source, ':') && _scan_uint(source, t_fraction));
    }
    else
    {
        _scan_format(_string, _format, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);
    }
    chrono::day_time temp_time;
    temp_time.t_day = t_day;
    temp_time.t_hour = static_cast<uint16_t>(t_hour);
    temp_time.t_minute = static_cast<uint16_t>(t_minute);
    temp_time.t_second = static_cast<uint16_t>(t_second);
    temp_time.t_fraction = static_cast<uint32_t>(t_fraction);

    return temp_time;
}
std::string_view chrono::to_string_month(int _month)
{
    // 0:January, 1:Febrary, 2:March,3:April, 4:May, 5:June, 6:July, 7:Auguest, 8:September, 9:October, 10:November, 11:December
    static const std::array<std::string_view, 12> str_month{ "Jun", "Feb", "Mar", "Apr", "May", "Jun", "July", "Aug", "Sep", "Oct", "Nov", "Dec" };
    return str_month.at(_month);
}

std::string_view chrono::to_string_day_of_week(int _day_of_week)
{
    // 0:sun, 1: mon, 2: tue, 3:Wed, 4: The, 5: Fri, 6: Sat
    static const std::array<std::string_view, 7> str_day_of_week{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    return str_day_of_week.at(_day_of_week);
}

std::string_view _to_string(char(&_str_output)[64], const chrono::day_time& _time) noexcept
{
    int len = 0;
    if (_time.t_day != 0) len = std::snprintf(_str_output, std::size(_str_output), "%dd %uh %um %us", _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = std::snprintf(_str_output, std::size(_str_output), "%uh %um %us", _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = std::snprintf(_str_output, std::size(_str_output), "%um %us", _time.t_minute, _time.t_second);
    else len = std::snprintf(_str_output, std::size(_str_output), "%us", _time.t_second);
    return std::string_view(_str_output, len);
}

std::string_view _to_string(char(&_str_output)[64], const chrono::day_time& _time, const chrono::fraction_part _faction) noexcept
{
    int len = 0;

    if (_time.t_day != 0) len = std::snprintf(_str_output, std::size(_str_output), "%dd %uh %um %us", _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = std::snprintf(_str_output, std::size(_str_output), "%uh %um %us", _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = std::snprintf(_str_output, std::size(_str_output), "%um %us", _time.t_minute, _time.t_second);
    else if (_time.t_second != 0 || _time.t_fraction == 0) len = std::snprintf(_str_output, std::size(_str_output), "%us", _time.t_second);

    if (_time.t_fraction != 0)
    {
        if (_faction.t_millisecond != 0) len += std::snprintf(_str_output + len, 64, " %ums", _faction.t_millisecond);
        if (_faction.t_microsecond != 0) len += std::snprintf(_str_output + len, 64, " %uus", _faction.t_microsecond);
        if (_faction.t_nanosecond != 0) len += std::snprintf(_str_output + len, 64, " %uns", _faction.t_nanosecond);
    }

    return std::string_view(_str_output, len);
}

std::string to_string(const chrono::date_time& _time)
{
    return CGDK::stream_string(
        CGDK::stream_width(_time.t_year, 4, '0'), "-",
        CGDK::stream_width(_time.t_month, 2, '0'), "-",
        CGDK::stream_width(_time.t_day, 2, '0'), " ",
        CGDK::stream_width(_time.t_hour, 2, '0'), ":",
        CGDK::stream_width(_time.t_minute, 2, '0'), ":",
        CGDK::stream_width(_time.t_second, 2, '0'), ".",
        CGDK::stream_width(_time.t_fraction, 9, '0'));
}

std::string to_string(const tm& _time)
{
    return CGDK::stream_string(
        CGDK::stream_width(_time.tm_year, 4, '0'), "-",
        CGDK::stream_width(_time.tm_mon, 2, '0'), "-",
        CGDK::stream_width(_time.tm_mday, 2, '0'), " ",
        CGDK::stream_width(_time.tm_hour, 2, '0'), ":",
        CGDK::stream_width(_time.tm_min, 2, '0'), ":",
        CGDK::stream_width(_time.tm_sec, 2, '0'));
}

template< class... ARGS >
constexpr std::string temp_format(std::string_view _format, ARGS&&... _args)
{
    return std::vformat(_format, std::make_format_args(_args...));
}

template< class... ARGS >
constexpr std::wstring temp_format(std::wstring_view _format, ARGS&&... _args)
{
    return std::vformat(_format, std::make_wformat_args(_args...));
}

std::string to_string(const chrono::date_time& _time, const std::string_view _format)
{
    if (_format.empty())
        return to_string(_time);
    else
        return temp_format(_format, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
}

std::string to_string(const tm& _time, const std::string_view _format)
{
    if (_format.empty())
        return to_string(_time);
    else
        return temp_format(_format, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
}

std::string to_string2(const chrono::date_time& _date_time)
{
    // "Mon, 6 June 2022 05:19:05";
    auto day_of_week = get_day_of_week(_date_time.t_year, _date_time.t_month, _date_time.t_day);
    return CGDK::stream_string(
        chrono::to_string_day_of_week(day_of_week), ", ",
        _date_time.t_day, " ",
        chrono::to_string_month(_date_time.t_month - 1), " ",
        _date_time.t_year, " ",
        CGDK::stream_width(_date_time.t_hour, 2, '0'), ":",
        CGDK::stream_width(_date_time.t_minute, 2, '0'), ":",
        CGDK::stream_width(_date_time.t_second, 2, '0'));
}

std::string to_string(const chrono::day_time& _time)
{
    // declare) 
    std::string str_output;

    // 1) resize
    str_output.resize(64);

    // 2) get string
    auto result = _to_string(reinterpret_cast<char(&)[64]>(str_output.front()), _time);

    // 3) fit to shrink
    str_output.resize(result.size());

    // return) 
    return str_output;
}

std::string to_string(const chrono::day_time& _time, const chrono::fraction_part _faction)
{
    // declare) 
    std::string str_output;

    // 1) resize
    str_output.resize(64);

    // 2) get string
    auto result = _to_string(reinterpret_cast<char(&)[64]>(str_output.front()), _time, _faction);

    // 3) fit to shrink
    str_output.resize(result.size());

    // return) 
    return str_output;
}


}
