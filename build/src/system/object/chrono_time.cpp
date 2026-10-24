#include "pch.h"


namespace CGDK
{

chrono::time::duration _get_utc_diff() noexcept
{
	std::time_t t_utc = std::time(nullptr);
	std::time_t t_local = std::time(nullptr);

#if defined(_MSC_VER)
	tm time_utc;
	gmtime_s(&time_utc, &t_utc);
	auto x_utc = chrono::from_tm<chrono::time::utc::time_point>(time_utc);

	tm time_local;
	localtime_s(&time_local, &t_local);
	auto x_local = chrono::from_tm<chrono::time::local::time_point>(time_local);

#else
	tm* time_utc = gmtime(&t_utc);
	auto x_utc = chrono::from_tm<chrono::time::utc::time_point>(*time_utc);

	tm* time_local = localtime(&t_local);
	auto x_local = chrono::from_tm<chrono::time::local::time_point>(*time_local);
#endif

	return chrono::time::duration(x_local - x_utc);
}

chrono::clock::duration get_utc_diff() noexcept
{
    static auto time_diff = _get_utc_diff();
    return time_diff;
}

chrono::time::clock::time_point __convert_time_to_local(const chrono::time::time_point& _utc_time)
{
    return _utc_time + get_utc_diff();
}

chrono::time::clock::time_point __convert_time_to_utc(const chrono::time::time_point& _local_time)
{
    return _local_time - get_utc_diff();
}

chrono::time::local::time_point::time_point(const utc::time_point& _utc_time) : chrono::time::clock::time_point(__convert_time_to_local(_utc_time))
{
}

chrono::time::local::time_point& chrono::time::local::time_point::operator = (const chrono::time::utc::time_point& _rhs)
{
    *reinterpret_cast<chrono::time::clock::time_point*>(this) = __convert_time_to_local(_rhs);
    return *this;
}

chrono::time::local::time_point chrono::time::local::now() noexcept
{
    return chrono::time::clock::now() + get_utc_diff();
}

chrono::time::utc::time_point::time_point(const chrono::time::local::time_point& _local_time) : clock::time_point(__convert_time_to_utc(_local_time))
{
}

chrono::time::utc::time_point& chrono::time::utc::time_point::operator = (const chrono::time::local::time_point& _rhs)
{
    *reinterpret_cast<chrono::time::clock::time_point*>(this) = __convert_time_to_utc(_rhs);
    return *this;
}

chrono::time::utc::time_point chrono::time::utc::now() noexcept
{
    return chrono::time::clock::now();
}


}