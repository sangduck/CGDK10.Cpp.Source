#include "pch.h"


namespace CGDK
{

namespace chrono
{

template<>
date_time from_string_to_date_time<char>(std::basic_string_view<char> _string, const std::string_view _format)
{
    // declare) 
    uint32_t t_year = 0, t_month = 0, t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    // 1) get ...
    __sscanf(_string.data(), (_format.empty() == false) ? _format : "%04u-%02u-%02u %02u:%02u:%02u.%09u"sv, &t_year, &t_month, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);

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

template<>
date_time from_string_to_date_time<wchar_t>(std::basic_string_view<wchar_t> _string, const std::wstring_view _format)
{
    // declare) 
    uint32_t t_year = 0, t_month = 0, t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    __sscanf(_string.data(), (_format.empty() == false) ? _format : L"%04u-%02u-%02u %02u:%02u:%02u.%09u"sv, &t_year, &t_month, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);

    // declare) 
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

template<>
day_time from_string_to_duration<char>(std::basic_string_view<char> _string, const std::string_view _format)
{
    // declare) 
    uint32_t t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    // 1) get ...
    __sscanf(_string.data(), (_format.empty() == false) ? _format : "%ud %uh %um %us.:%u"sv, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);

    chrono::day_time temp_time;
    temp_time.t_day = static_cast<uint16_t>(t_day);
    temp_time.t_hour = static_cast<uint16_t>(t_hour);
    temp_time.t_minute = static_cast<uint16_t>(t_minute);
    temp_time.t_second = static_cast<uint16_t>(t_second);
    temp_time.t_fraction = static_cast<uint32_t>(t_fraction);

    return temp_time;
}

template<>
day_time from_string_to_duration<wchar_t>(std::basic_string_view<wchar_t> _string, const std::wstring_view _format)
{
    // declare) 
    uint32_t t_day = 0, t_hour = 0, t_minute = 0, t_second = 0, t_fraction = 0;

    __sscanf(_string.data(), (_format.empty() == false) ? _format : L"%ud %uh %um %us.:%u"sv, &t_day, &t_hour, &t_minute, &t_second, &t_fraction);

    // declare) 
    chrono::day_time temp_time;
    temp_time.t_day = static_cast<uint16_t>(t_day);
    temp_time.t_hour = static_cast<uint16_t>(t_hour);
    temp_time.t_minute = static_cast<uint16_t>(t_minute);
    temp_time.t_second = static_cast<uint16_t>(t_second);
    temp_time.t_fraction = static_cast<uint32_t>(t_fraction);

    return temp_time;
}

}


std::string_view to_string(char(&_str_output)[64], const chrono::day_time& _time) noexcept
{
    int len = 0;
    if (_time.t_day != 0) len = __sprintf(_str_output, "%dd %uh %um %us"sv, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = __sprintf(_str_output, "%uh %um %us"sv, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = __sprintf(_str_output, "%um %us"sv, _time.t_minute, _time.t_second);
    else len = __sprintf(_str_output, "%us", _time.t_second);
    return std::string_view(_str_output, len);
}

std::wstring_view to_string(wchar_t(&_str_output)[64], const chrono::day_time& _time) noexcept
{
    int len = 0;
    if (_time.t_day != 0) len = __sprintf(_str_output, L"%dd %uh %um %us"sv, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = __sprintf(_str_output, L"%uh %um %us"sv, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = __sprintf(_str_output, L"%um %us"sv, _time.t_minute, _time.t_second);
    else len = __sprintf(_str_output, L"%us", _time.t_second);
    return std::wstring_view(_str_output, len);
}

std::u16string_view to_string(char16_t(&/*_str_output*/)[64], const chrono::day_time /*_time*/)
{
    CGASSERT_ERROR(false);
    return std::u16string_view();
}

std::u32string_view to_string(char32_t(&/*_str_output*/)[64], const chrono::day_time /*_time*/)
{
    CGASSERT_ERROR(false);
    return std::u32string_view();
}

std::string_view to_string(char(&_str_output)[64], const chrono::day_time _time, const chrono::fraction_part _faction) noexcept
{
    int len = 0;

    if (_time.t_day != 0) len = __sprintf(_str_output, "%dd %uh %um %us"sv, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = __sprintf(_str_output, "%uh %um %us"sv, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = __sprintf(_str_output, "%um %us"sv, _time.t_minute, _time.t_second);
    else if (_time.t_second != 0 || _time.t_fraction == 0) len = __sprintf(_str_output, "%us", _time.t_second);

    if (_time.t_fraction != 0)
    {
        if (_faction.t_millisecond != 0) len += __sprintf(_str_output + len, 64, " %ums"sv, _faction.t_millisecond);
        if (_faction.t_microsecond != 0) len += __sprintf(_str_output + len, 64, " %uus"sv, _faction.t_microsecond);
        if (_faction.t_nanosecond != 0) len += __sprintf(_str_output + len, 64, " %uns"sv, _faction.t_nanosecond);
    }

    return std::string_view(_str_output, len);
}

std::wstring_view to_string(wchar_t(&_str_output)[64], const chrono::day_time _time, const chrono::fraction_part _faction) noexcept
{
    int len = 0;

    if (_time.t_day != 0) len = __sprintf(_str_output, L"%dd %uh %um %us"sv, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_hour != 0) len = __sprintf(_str_output, L"%uh %um %us"sv, _time.t_hour, _time.t_minute, _time.t_second);
    else if (_time.t_minute != 0) len = __sprintf(_str_output, L"%um %us"sv, _time.t_minute, _time.t_second);
    else if (_time.t_second != 0 || _time.t_fraction == 0) len = __sprintf(_str_output, L"%us"sv, _time.t_second);

    if (_time.t_fraction != 0)
    {
        if (_faction.t_millisecond != 0) len += __sprintf(_str_output + len, 64, L" %ums"sv, _faction.t_millisecond);
        if (_faction.t_microsecond != 0) len += __sprintf(_str_output + len, 64, L" %uus"sv, _faction.t_microsecond);
        if (_faction.t_nanosecond != 0) len += __sprintf(_str_output + len, 64, L" %uns"sv, _faction.t_nanosecond);
    }

    return std::wstring_view(_str_output, len);
}

std::u16string_view to_string(char16_t(&/*_str_output*/)[64], const chrono::day_time /*_time*/, const chrono::fraction_part /*_faction*/)
{
    CGASSERT_ERROR(false);
    return std::u16string_view();
}

std::u32string_view to_string(char32_t(&/*_str_output*/)[64], const chrono::day_time /*_time*/, const chrono::fraction_part /*_faction*/)
{
    CGASSERT_ERROR(false);
    return std::u32string_view();
}

template <>
std::basic_string<char> to_string<char>(const chrono::date_time& _time) 
{
    return NFMT::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:09}"sv, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
}
template <>
std::basic_string<wchar_t> to_string<wchar_t>(const chrono::date_time& _time)
{
    return NFMT::format(L"{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:09}"sv, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
}

template <>
std::basic_string<char> to_string<char>(const tm& _time)
{
    return NFMT::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
}

template <>
std::basic_string<wchar_t> to_string<wchar_t>(const tm& _time)
{
    return NFMT::format(L"{:04}-{:02}-{:02} {:02}:{:02}:{:02}"sv, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
}

#if !defined(FMT_FORMAT_H_)
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
#endif

template <>
std::basic_string<char> to_string<char>(const chrono::date_time& _time, const std::string_view _format)
{
#if !defined(FMT_FORMAT_H_)
    if (_format.empty())
        return to_string<char>(_time);
    else
        return temp_format(_format, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
#else
    if (_format.empty())
        return to_string<char>(_time);
    else
        return fmt::format(_format, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
#endif
}

template <>
std::basic_string<wchar_t> to_string<wchar_t>(const chrono::date_time& _time, const std::wstring_view _format)
{
#if !defined(FMT_FORMAT_H_)
    if (_format.empty())
        return to_string<wchar_t>(_time);
    else
        return temp_format(_format, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
#else
    if (_format.empty())
        return to_string<wchar_t>(_time);
    else
        return fmt::format(_format, _time.t_year, _time.t_month, _time.t_day, _time.t_hour, _time.t_minute, _time.t_second, _time.t_fraction);
#endif
}

template <>
std::basic_string<char> to_string<char>(const tm& _time, const std::string_view _format)
{
#if !defined(FMT_FORMAT_H_)
    if (_format.empty())
        return to_string<char>(_time);
    else
        return temp_format(_format, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
#else
    if (_format.empty())
        return to_string<char>(_time);
    else
        return fmt::format(_format, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
#endif
}

template <>
std::basic_string<wchar_t> to_string<wchar_t>(const tm& _time, const std::wstring_view _format)
{
#if !defined(FMT_FORMAT_H_)
    if (_format.empty())
        return to_string<wchar_t>(_time);
    else
        return temp_format(_format, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
#else
        if (_format.empty())
            return to_string<wchar_t>(_time);
        else
            return fmt::format(_format, _time.tm_year, _time.tm_mon, _time.tm_mday, _time.tm_hour, _time.tm_min, _time.tm_sec);
#endif
}

template<>
std::basic_string<char> to_string2<char>(const chrono::date_time& _date_time)
{
    // "Mon, 6 June 2022 05:19:05";
    auto day_of_week = get_day_of_week(_date_time.t_year, _date_time.t_month, _date_time.t_day);
    return NFMT::format("{}, {} {} {} {:02}:{:02}:{:02}"sv, chrono::to_string_day_of_week<char>(day_of_week), _date_time.t_day, chrono::to_string_month<char>(_date_time.t_month - 1), _date_time.t_year, _date_time.t_hour, _date_time.t_minute, _date_time.t_second);
}

template<>
std::basic_string<wchar_t> to_string2<wchar_t>(const chrono::date_time& _date_time)
{
    // "Mon, 6 June 2022 05:19:05";
    auto day_of_week = get_day_of_week(_date_time.t_year, _date_time.t_month, _date_time.t_day);
    return NFMT::format(L"{}, {} {} {} {:02}:{:02}:{:02}"sv, chrono::to_string_day_of_week<wchar_t>(day_of_week), _date_time.t_day, chrono::to_string_month<wchar_t>(_date_time.t_month - 1), _date_time.t_year, _date_time.t_hour, _date_time.t_minute, _date_time.t_second);
}

chrono::time::duration chrono::time::_get_utc_diff() noexcept
{
	std::time_t t_utc = std::time(nullptr);
	std::time_t t_local = std::time(nullptr);

#if defined(_MSC_VER)
	tm time_utc;
	gmtime_s(&time_utc, &t_utc);
	auto x_utc = from_tm<chrono::time::utc::time_point>(time_utc);

	tm time_local;
	localtime_s(&time_local, &t_local);
	auto x_local = from_tm<chrono::time::local::time_point>(time_local);

#else
	tm* time_utc = gmtime(&t_utc);
	auto x_utc = from_tm<chrono::time::utc::time_point>(*time_utc);

	tm* time_local = localtime(&t_local);
	auto x_local = from_tm<chrono::time::local::time_point>(*time_local);
#endif

	return chrono::time::duration(x_local - x_utc);
}


}