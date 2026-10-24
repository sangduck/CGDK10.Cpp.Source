//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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
//----------------------------------------------------------------------------
//
//
//  Definitions for Event Classes
//
//
//----------------------------------------------------------------------------
namespace CGDK
{
namespace chrono
{
namespace time
{

struct clock
{
    using rep = long long;

#if defined(_USE_TIME_CLOCK_LINUX_STANDARD)
    // std::chrono::system_clock setting (linux)
    static const rep nanoseconds_per_tick  = 1;
    static const rep ticks_per_time_t      = 1000000000LL;
#elif defined(_USE_TIME_CLOCK_WWINDOWS_STANDARD)
    // std::chrono::system_clock setting (windows)
    static const rep nanoseconds_per_tick  = 100;
    static const rep ticks_per_time_t      = 10000000LL;
#elif defined(_USE_TIME_CLOCK_WWINDOWS_SYSTEMTIME)
    // std::chrono::system_clock setting (windows)
    static const rep nanoseconds_per_tick = 1000000;
    static const rep ticks_per_time_t     = 1000LL;
#else
    // - Not Defined!
    #error "CGDK::tick not defined!"
#endif

    using period = std::ratio_multiply<std::ratio<nanoseconds_per_tick, 1>, std::nano>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<clock>;
    static constexpr bool is_steady = false;

    [[nodiscard]] static time_point now() noexcept { return time_point(std::chrono::duration_cast<duration>(std::chrono::system_clock::now().time_since_epoch())); }
    [[nodiscard]] static time_t     to_time_t(const time_point& _time) noexcept { return static_cast<time_t>(_time.time_since_epoch().count() / ticks_per_time_t); }
    [[nodiscard]] static time_point from_time_t(time_t _tm) noexcept { return time_point(duration(_tm * ticks_per_time_t)); }
};

using time_point = clock::time_point;
using duration = clock::duration;

namespace local
{
    class time_point;
}

namespace client
{
    class time_point;
}

namespace utc
{
	class time_point : public clock::time_point
	{
	public:
        constexpr time_point() = default;
        constexpr explicit time_point(const duration& _other) : clock::time_point(_other) {}
        template <class TDURATION, std::enable_if_t<std::is_convertible_v<TDURATION, duration>, int> = 0>
        constexpr time_point(const std::chrono::time_point<clock, TDURATION>& _time_point) noexcept : clock::time_point(_time_point) {}
                  time_point(const local::time_point& _local_time);
        constexpr time_point(const client::time_point& _client_time);

        [[nodiscard]] constexpr duration time_since_epoch() const noexcept(std::is_arithmetic_v<rep>) { return clock::time_point::time_since_epoch(); }
        constexpr time_point& operator+=(const duration& _duration) noexcept(std::is_arithmetic_v<rep>) { clock::time_point::operator += (_duration); return *this; }
        constexpr time_point& operator-=(const duration& _duration) noexcept(std::is_arithmetic_v<rep>) { clock::time_point::operator -= (_duration); return *this; }
        [[nodiscard]] static constexpr time_point(min)() noexcept { return (clock::time_point::min)(); }
        [[nodiscard]] static constexpr time_point(max)() noexcept { return (clock::time_point::max)(); }
                
                  time_point& operator = (const local::time_point& _rhs);
        constexpr time_point& operator = (const client::time_point& _rhs);
        constexpr time_point& operator = (const clock::time_point& _rhs) { *static_cast<clock::time_point*>(this) = _rhs; return *this; }
    };

    [[nodiscard]] time_point now() noexcept;
	[[nodiscard]] inline std::time_t to_time_t(const time_point& _time) noexcept { return clock::to_time_t(_time); }
	[[nodiscard]] inline time_point from_time_t(std::time_t _tm) noexcept { return clock::from_time_t(_tm); }
}

namespace local
{
	class time_point : public clock::time_point
	{
	public:
        constexpr time_point() = default;
        constexpr explicit time_point(const duration& _other) : clock::time_point(_other) {}
        template <class TDURATION, std::enable_if_t<std::is_convertible_v<TDURATION, duration>, int> = 0>
        constexpr time_point(const std::chrono::time_point<clock, TDURATION>& _Tp) noexcept : clock::time_point(_Tp) {}
                  time_point(const utc::time_point& _utc_time);
        constexpr time_point(const client::time_point& _utc_time);

        [[nodiscard]] constexpr duration time_since_epoch() const noexcept(std::is_arithmetic_v<rep>) { return clock::time_point::time_since_epoch(); }
        constexpr time_point& operator+=(const duration& _duration) noexcept(std::is_arithmetic_v<rep>) { clock::time_point::operator += (_duration); return *this; }
        constexpr time_point& operator-=(const duration& _duration) noexcept(std::is_arithmetic_v<rep>) { clock::time_point::operator -= (_duration); return *this; }
        [[nodiscard]] static constexpr time_point(min)() noexcept { return (clock::time_point::min)(); }
        [[nodiscard]] static constexpr time_point(max)() noexcept { return (clock::time_point::max)(); }

                  time_point& operator = (const utc::time_point& _rhs);
        constexpr time_point& operator = (const client::time_point& _rhs);
        constexpr time_point& operator = (const clock::time_point& _rhs) { *static_cast<clock::time_point*>(this) = _rhs; return *this; }
    };

    [[nodiscard]] time_point now() noexcept;
	[[nodiscard]] inline std::time_t to_time_t(const time_point& _time) noexcept { return clock::to_time_t(_time); }
	[[nodiscard]] inline time_point from_time_t(std::time_t _tm) noexcept { return clock::from_time_t(_tm); }
}


}
}

template<> class is_std_chrono_time_point <chrono::time::time_point> : public std::true_type {};
template<> class is_std_chrono_time_point <chrono::time::utc::time_point> : public std::true_type {};
template<> class is_std_chrono_time_point <chrono::time::local::time_point> : public std::true_type {};

}