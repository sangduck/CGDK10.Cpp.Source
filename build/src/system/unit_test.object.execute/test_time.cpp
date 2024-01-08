#include "pch.h"

namespace CGDK
{
	TEST(CGDK10_time, time_system_time_point_duration)
	{
		// test contexts)
		// - time_point의  마이너스(-) 연산확인
		// - day/hour/minute/sec/faction을 검사.
		// - 연산 단위 넘김 검사

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 1) differ - day
		{
			auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });
			auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY + 1,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::days(1));
		}

		// 2) differ - hour
		{
			const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });
			const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR + 1,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::hours(1));
		}

		// 3) differ - minute
		{
			const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });
			const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE + 1,TEST_SECONDS,TEST_FRACTION });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::minutes(1));
		}

		// 4) differ - second
		{
			const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });
			const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS + 1,TEST_FRACTION });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::seconds(1));
		}

		// 4) differ - millisecond
		if constexpr (chrono::time::clock::ticks_per_time_t >= 1000LL)
		{
			const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,1000000 / chrono::time::clock::nanoseconds_per_tick });
			const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,2000000 / chrono::time::clock::nanoseconds_per_tick });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::milliseconds(1));
		}

		// 5) differ - microsecond
		if constexpr (chrono::time::clock::ticks_per_time_t >= 1000000LL)
		{
			const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,1000 / chrono::time::clock::nanoseconds_per_tick });
			const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,2000 / chrono::time::clock::nanoseconds_per_tick });

			const chrono::time::duration diff = time_2 - time_1;

			// check) 
			EXPECT_TRUE(diff == chrono::microseconds(1));
		}
	}

	TEST(CGDK10_time, time_system_time_point_std)
	{
		// test contexts)
		// - std::chrono::system_clock과 변환 


	}

	TEST(CGDK10_time, time_system_time_point_time_t)
	{
		// test contexts)
		// - time_point -> date_tiem 변환
		// - date_tiem -> time_point 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 1) differ - day
		const auto time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// 2) to datetime
		const chrono::date_time dt_1 = chrono::to_date_time(time_1);

		// check) 
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// 3) to time_point
		const auto time_2 = chrono::from_date_time<chrono::time::local::time_point>(dt_1);

		// check) 
		EXPECT_TRUE(time_2 == time_1);
	}

	TEST(CGDK10_time, time_system_time_point_date_time_fractioin)
	{
		// test contexts)
		// - chrono::time::local -> chrono::time::utc 변환
		// - chrono::time::utc -> chrono::time::local 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 1) 
		const chrono::time::local::time_point time_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// 2) local -> utc
		const chrono::time::utc::time_point time_2 = time_1;

		// 3) utc -> local
		const chrono::time::local::time_point time_3 = time_2;

		// check) 
		EXPECT_TRUE(time_1 == time_3);

		chrono::date_time dt_1 = chrono::to_date_time(time_3);
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// 3) local -> utc
		const chrono::time::utc::time_point time_4 = time_3;

		// check) 
		EXPECT_TRUE(time_2 == time_4);
	}

	TEST(CGDK10_time, time_system_time_point_tm)
	{
		// test contexts)
		// - time_point와 tm의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;

		// 1) differ - day
		const chrono::time::utc::time_point time_1 = chrono::from_date_time<chrono::time::utc::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,0 });

		// 2) time...
		const auto tm_1 = chrono::to_tm(time_1);

		// check) 
		EXPECT_TRUE(tm_1.tm_year == TEST_YEAR);
		EXPECT_TRUE(tm_1.tm_mon == TEST_MONTH);
		EXPECT_TRUE(tm_1.tm_mday == TEST_DAY);
		EXPECT_TRUE(tm_1.tm_hour == TEST_HOUR);
		EXPECT_TRUE(tm_1.tm_min == TEST_MINUTE);
		EXPECT_TRUE(tm_1.tm_sec == TEST_SECONDS);

		// 3) to time_point
		const chrono::time::utc::time_point time_2 = chrono::from_tm<chrono::time::utc::time_point>(tm_1);

		// check) 
		EXPECT_TRUE(time_1 == time_2);
	}

	TEST(CGDK10_time, time_system_time_point_string)
 	{
		// test contexts)
		// - time_point와 string과의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 1) make utc::time_ptr
		const chrono::time::utc::time_point time_1 = chrono::from_date_time<chrono::time::utc::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// 2) to string - char
		const auto s1 = to_string<char>(time_1);

		// check) 
		EXPECT_TRUE(s1 == "2017-12-11 05:10:20.000000010"sv);

		// 3) to string - wchar_t
		const auto s2 = to_string<wchar_t>(time_1);

		// check) 
		EXPECT_TRUE(s2 == L"2017-12-11 05:10:20.000000010"sv);

		// 4) from string - char
		const auto t1 = from_string<chrono::time::utc::time_point>(s1);
		auto dt_1 = chrono::to_date_time(t1);
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// 5) from string - wchar_t
		const auto t2 = from_string<chrono::time::utc::time_point>(s2);
		const auto dt_2 = chrono::to_date_time(t2);
		EXPECT_TRUE(dt_2.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_2.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_2.t_day == TEST_DAY);
		EXPECT_TRUE(dt_2.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_2.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_2.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_2.t_fraction == TEST_FRACTION);
	}

	TEST(CGDK10_time, time_system_time_tm_string)
	{
		// test contexts)
		// - time_point와 string과의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;

		// 1) differ - day
		chrono::time::utc::time_point time_1 = chrono::from_date_time<chrono::time::utc::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,0 });

		// 2) tm으로 변환
		const auto tm_1 = chrono::to_tm(time_1);

		// check) 
		EXPECT_TRUE(tm_1.tm_year == TEST_YEAR);
		EXPECT_TRUE(tm_1.tm_mon == TEST_MONTH);
		EXPECT_TRUE(tm_1.tm_mday == TEST_DAY);
		EXPECT_TRUE(tm_1.tm_hour == TEST_HOUR);
		EXPECT_TRUE(tm_1.tm_min == TEST_MINUTE);
		EXPECT_TRUE(tm_1.tm_sec == TEST_SECONDS);

		// 3)
		const auto str_1 = to_string<char>(tm_1);

		// check) 
		EXPECT_TRUE(str_1 == "2017-12-11 05:10:20");

		// 3)
		const auto str_2 = to_string<wchar_t>(tm_1);

		// check) 
		EXPECT_TRUE(str_2 == L"2017-12-11 05:10:20");
	}

	TEST(CGDK10_time, time_system_time_duration_string)
	{
		const auto dur_1 = 1h + 2min + 30s;
		const auto str_1 = to_string<char>(dur_1);

		// check) 
		EXPECT_TRUE(str_1 == "1h 2m 30s");


		const auto dur_2 = 300h + 2min + 30s;
		const auto str_2 = to_string<char>(dur_2);

		// check) 
		EXPECT_TRUE(str_2 == "12d 12h 2m 30s");
	}

	TEST(CGDK10_time, time_system_date_time_string)
	{
		// test contexts)
		// - date_time과 string과의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 11) date_time 객체 생성
		const chrono::date_time time_1{ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION };

		// 2) 문자열로 변환
		const auto str_1 = to_string<char>(time_1);

		// check)
		EXPECT_TRUE(str_1 == "2017-12-11 05:10:20.000000010");
	}

	TEST(CGDK10_time, time_system_day_time_string)
	{
		// test contexts)
		// - day_time과  string과의 상호 변환

		// definition) 
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 10;

		// 11) day_time 객체 생성
		const chrono::day_time time_1{ TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION };

		// 2) 문자열로 변환
		const auto str_1 = to_string<char>(time_1);

		// check)
		EXPECT_TRUE(str_1 == "11d 5h 10m 20s");
	}

	TEST(CGDK10_time, time_system_client_time_local)
	{
		// test contexts)
		// - day_time과  string과의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 1000;

		// 1) local time - constructor
		const auto t_1 = chrono::from_date_time<chrono::time::local::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// - constructor
		const chrono::time::client::time_point x_1 = t_1;

		// - to_date_time
		const auto dt_1 = chrono::to_date_time(x_1);

		// check) 
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// declare) 
		chrono::time::client::time_point x_2;

		// - operator assignment
		x_2 = t_1;

		// - to_date_time
		const auto dt_2 = chrono::to_date_time(x_2);

		// check) 
		EXPECT_TRUE(dt_2.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_2.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_2.t_day == TEST_DAY);
		EXPECT_TRUE(dt_2.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_2.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_2.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_2.t_fraction == TEST_FRACTION);

		// - to string
		const auto str_2 = to_string<char>(x_2);

		// check) 
		EXPECT_TRUE(str_2 == "2017-12-11 05:10:20.000001000"sv);
	}

	TEST(CGDK10_time, time_system_client_time_utc)
	{
		// test contexts)
		// - day_time과  string과의 상호 변환

		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 100;

		// 1) utc time - constructor
		const auto t_1 = chrono::from_date_time<chrono::time::utc::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// - constructor
		const chrono::time::client::time_point x_1 = t_1;

		// - to_date_time
		const auto dt_1 = chrono::to_date_time(x_1);

		// check) 
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// declare) 
		chrono::time::client::time_point x_2;

		// - operator assignment
		x_2 = t_1;

		// - to_date_time
		const auto dt_2 = chrono::to_date_time(x_2);

		// check) 
		EXPECT_TRUE(dt_2.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_2.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_2.t_day == TEST_DAY);
		EXPECT_TRUE(dt_2.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_2.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_2.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_2.t_fraction == TEST_FRACTION);

		// - to string
		const auto str_2 = to_string<char>(x_2);

		// check) 
		EXPECT_TRUE(str_2 == "2017-12-11 05:10:20.000000100"sv);
	}

	TEST(CGDK10_time, time_system_client_time_time_t)
	{
		// definition) 
		const int16_t	TEST_YEAR = 2017;
		const uint16_t	TEST_MONTH = 12;
		const uint16_t	TEST_DAY = 11;
		const uint16_t	TEST_HOUR = 5;
		const uint16_t	TEST_MINUTE = 10;
		const uint16_t	TEST_SECONDS = 20;
		const uint32_t	TEST_FRACTION = 0;

		// 1) chrono::time::client::time_point - constructor
		const auto ct_1 = chrono::from_date_time<chrono::time::client::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// - to_date_time
		const auto dt_1 = chrono::to_date_time(ct_1);

		// check) 
		EXPECT_TRUE(dt_1.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_1.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_1.t_day == TEST_DAY);
		EXPECT_TRUE(dt_1.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_1.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_1.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_1.t_fraction == TEST_FRACTION);

		// 2) chrono::time::utc::time_point - constructor
		const auto t_1 = chrono::from_date_time<chrono::time::utc::time_point>({ TEST_YEAR,TEST_MONTH,TEST_DAY,TEST_HOUR,TEST_MINUTE,TEST_SECONDS,TEST_FRACTION });

		// check)
		EXPECT_TRUE(chrono::client_clock::to_time_t(ct_1) == chrono::time::utc::to_time_t(t_1));

		const auto t_2 = chrono::time::utc::from_time_t(chrono::client_clock::to_time_t(ct_1));

		// - to_date_time
		const auto dt_2 = chrono::to_date_time(t_2);

		// check) 
		EXPECT_TRUE(dt_2.t_year == TEST_YEAR);
		EXPECT_TRUE(dt_2.t_month == TEST_MONTH);
		EXPECT_TRUE(dt_2.t_day == TEST_DAY);
		EXPECT_TRUE(dt_2.t_hour == TEST_HOUR);
		EXPECT_TRUE(dt_2.t_minute == TEST_MINUTE);
		EXPECT_TRUE(dt_2.t_second == TEST_SECONDS);
		EXPECT_TRUE(dt_2.t_fraction == TEST_FRACTION);
	}

}