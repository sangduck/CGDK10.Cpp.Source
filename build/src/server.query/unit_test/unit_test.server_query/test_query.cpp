#include "pch.h"
#include <string>

namespace CGDK
{
	//-----------------------------------------------------------------------------
	// NUMERIC (fixed point : value + fraction / 10^8)
	//-----------------------------------------------------------------------------
	static NUMERIC N(const char* _value) { return NUMERIC(_value); }
	static std::string S(const NUMERIC& _value) { return _value.to_string<char>(); }

	// 1) from_string / to_string
	TEST(CGDK10_query_numeric, string)
	{
		EXPECT_EQ(N("123.45").value, 123);
		EXPECT_EQ(N("123.45").fraction, 45000000);
		EXPECT_EQ(N("+7").value, 7);
		EXPECT_EQ(N("-1.5").value, -1);
		EXPECT_EQ(N("-1.5").fraction, -50000000);
		EXPECT_EQ(N("0.00000001").fraction, 1);
		EXPECT_EQ(N("1.123456789").fraction, 12345678);		// truncated to 8 digits

		EXPECT_EQ(S(N("0")), "0");
		EXPECT_EQ(S(N("42")), "42");
		EXPECT_EQ(S(N("123.45")), "123.45");
		EXPECT_EQ(S(N("-1.5")), "-1.5");
		EXPECT_EQ(S(N("0.00000001")), "0.00000001");

		// - wide string
		NUMERIC w;
		w.from_string(L"3.25");
		EXPECT_EQ(w, N("3.25"));
		EXPECT_EQ(w.to_string<wchar_t>(), L"3.25");
	}

	// 2) negative value between -1 and 0 keeps the sign (value is 0 : the sign is in the fraction part only)
	TEST(CGDK10_query_numeric, string_negative_fraction)
	{
		EXPECT_EQ(S(N("-0.5")), "-0.5");
		EXPECT_EQ(S(N("-0.00000001")), "-0.00000001");
		EXPECT_EQ(S(N("-1.5")), "-1.5");
		EXPECT_EQ(S(N("0.5")), "0.5");
		EXPECT_EQ(N("-0.25").to_string<wchar_t>(), L"-0.25");

		// - round trip
		EXPECT_EQ(N(S(N("-0.125")).c_str()), N("-0.125"));
	}

	// 3) compare
	TEST(CGDK10_query_numeric, compare)
	{
		EXPECT_TRUE(N("1.5") == N("1.50"));
		EXPECT_TRUE(N("1.5") != N("1.25"));
		EXPECT_TRUE(N("1.5") > N("1.25"));
		EXPECT_TRUE(N("2") > N("1.99999999"));
		EXPECT_TRUE(N("-1") < N("0"));
		EXPECT_TRUE(N("1.5") >= N("1.5"));
		EXPECT_TRUE(N("1.5") <= N("1.5"));

		EXPECT_TRUE(NUMERIC().empty());
		EXPECT_TRUE(N("0.1").exist());
		EXPECT_FALSE(static_cast<bool>(NUMERIC()));
	}

	// 4) additive / substract (carry & borrow)
	TEST(CGDK10_query_numeric, add_sub)
	{
		EXPECT_EQ(N("1.75") + N("2.5"), N("4.25"));
		EXPECT_EQ(N("0.99999999") + N("0.00000001"), N("1"));
		EXPECT_EQ(N("4.25") - N("2.5"), N("1.75"));
		EXPECT_EQ(N("1.25") - N("2.5"), N("-1.25"));
		EXPECT_EQ(N("-1.5") + N("-1.5"), N("-3"));
		EXPECT_EQ(-N("1.5"), N("-1.5"));

		NUMERIC v = N("10");
		v += N("0.5");
		v -= N("3");
		EXPECT_EQ(v, N("7.5"));
	}

	// 5) multiply
	TEST(CGDK10_query_numeric, multiply)
	{
		EXPECT_EQ(N("1.5") * N("2"), N("3"));
		EXPECT_EQ(N("2.5") * N("2.5"), N("6.25"));
		EXPECT_EQ(N("1.5") * N("-2"), N("-3"));
		EXPECT_EQ(N("0.5") * N("0.5"), N("0.25"));
	}

	// 6) divide - integer divisor
	TEST(CGDK10_query_numeric, divide_integer)
	{
		EXPECT_EQ(N("10") / N("4"), N("2.5"));
		EXPECT_EQ(N("7.5") / N("3"), N("2.5"));
		EXPECT_EQ(N("1") / N("8"), N("0.125"));
		EXPECT_EQ(N("-10") / N("4"), N("-2.5"));
		EXPECT_EQ(N("10") / N("-4"), N("-2.5"));
		EXPECT_EQ(N("-0.5") / N("2"), N("-0.25"));
	}

	// 7) divide - divisor with fraction
	TEST(CGDK10_query_numeric, divide_fraction)
	{
		EXPECT_EQ(N("1") / N("0.5"), N("2"));
		EXPECT_EQ(N("3") / N("1.5"), N("2"));
		EXPECT_EQ(N("7.5") / N("2.5"), N("3"));
		EXPECT_EQ(N("0.5") / N("0.25"), N("2"));
		EXPECT_EQ(N("0.00000001") / N("0.00000001"), N("1"));
		EXPECT_EQ(N("-3") / N("1.5"), N("-2"));
		EXPECT_EQ(N("3") / N("-1.5"), N("-2"));
		EXPECT_EQ(N("-3") / N("-1.5"), N("2"));

		// - truncated at 8 digits (toward zero)
		EXPECT_EQ(N("1") / N("3"), N("0.33333333"));
		EXPECT_EQ(N("2") / N("3"), N("0.66666666"));
		EXPECT_EQ(N("-1") / N("3"), N("-0.33333333"));
		EXPECT_EQ(N("1") / N("0.3"), N("3.33333333"));

		// - large values (no overflow of the intermediate result)
		EXPECT_EQ(N("123456789012.5") / N("0.5"), N("246913578025"));
		EXPECT_EQ(N("100000000000") / N("0.001"), N("100000000000000"));
		EXPECT_EQ(N("92233720368.54775807") / N("92233720368.54775807"), N("1"));
	}

	// 7-0) divide by zero : exception
	TEST(CGDK10_query_numeric, divide_by_zero)
	{
		EXPECT_ANY_THROW({ auto r = N("1") / N("0"); (void)r; });
		EXPECT_ANY_THROW({ auto r = N("1.5") / NUMERIC(); (void)r; });
	}

	// 7-1) divide by an integer (NUMERIC::_operate_divide(const NUMERIC&, int64_t))
	TEST(CGDK10_query_numeric, divide_int64)
	{
		EXPECT_EQ(NUMERIC::_operate_divide(N("10"), 4), N("2.5"));
		EXPECT_EQ(NUMERIC::_operate_divide(N("7.5"), 3), N("2.5"));
		EXPECT_EQ(NUMERIC::_operate_divide(N("1"), 8), N("0.125"));
		EXPECT_EQ(NUMERIC::_operate_divide(N("-7.5"), 2), N("-3.75"));
		EXPECT_EQ(NUMERIC::_operate_divide(N("7"), -2), N("-3.5"));
		EXPECT_EQ(NUMERIC::_operate_divide(N("0.5"), -2), N("-0.25"));
	}

	// 8) modulo
	TEST(CGDK10_query_numeric, modulo)
	{
		EXPECT_EQ(N("7") % N("3"), N("1"));
		EXPECT_EQ(N("7.5") % N("3"), N("1.5"));

		// - divisor with fraction : lhs - (integer part of lhs / rhs) x rhs
		EXPECT_EQ(N("7") % N("2.5"), N("2"));
		EXPECT_EQ(N("7.5") % N("2.5"), N("0"));
		EXPECT_EQ(N("-7") % N("2.5"), N("-2"));
		EXPECT_EQ(N("1") % N("0.3"), N("0.1"));
	}

	// 9) float / double
	TEST(CGDK10_query_numeric, float_double)
	{
		EXPECT_EQ(NUMERIC(1.25), N("1.25"));
		EXPECT_EQ(NUMERIC(-2.5), N("-2.5"));
		EXPECT_DOUBLE_EQ(static_cast<double>(N("2.25")), 2.25);
		EXPECT_FLOAT_EQ(static_cast<float>(N("-0.75")), -0.75f);

		EXPECT_FLOAT_EQ(NUMERIC::get_change_ratio(N("110"), N("100")), 10.0f);
		EXPECT_FLOAT_EQ(NUMERIC::get_change_ratio(N("90"), N("100")), -10.0f);
		EXPECT_FLOAT_EQ(NUMERIC::get_change_ratio(N("90"), N("0")), 0.0f);
	}


	//-----------------------------------------------------------------------------
	// querier manager / statistics
	//-----------------------------------------------------------------------------
	class test_querier :
		virtual public Iquerier,
		public object::Nnameable,
		public object::Ninitializable_startable
	{
	public:
		test_querier(std::string_view _name) { this->name(_name); }

		void do_register() { this->register_querier(); }
		void do_unregister() noexcept { this->unregister_querier(); }
	};

	static bool is_registered(Iquerier* _pquerier)
	{
		auto pmanager = query::manager_querier::instance();
		std::lock_guard lock(pmanager->get_lockable());
		return std::find(pmanager->begin(), pmanager->end(), _pquerier) != pmanager->end();
	}

	// 10) register / unregister
	TEST(CGDK10_query_manager, register_unregister)
	{
		auto pmanager = query::manager_querier::instance();
		ASSERT_TRUE(pmanager.exist());

		auto count_before = pmanager->count();

		auto pquerier_a = make_own<test_querier>("test.querier.a");
		auto pquerier_b = make_own<test_querier>("test.querier.b");

		pquerier_a->do_register();
		pquerier_b->do_register();
		EXPECT_EQ(pmanager->count(), count_before + 2);
		EXPECT_TRUE(is_registered(pquerier_a.get()));
		EXPECT_TRUE(is_registered(pquerier_b.get()));

		// - registering twice is ignored
		pquerier_a->do_register();
		EXPECT_EQ(pmanager->count(), count_before + 2);

		pquerier_a->do_unregister();
		EXPECT_FALSE(is_registered(pquerier_a.get()));
		EXPECT_EQ(pmanager->count(), count_before + 1);

		// - unregistering twice is ignored
		pquerier_a->do_unregister();
		EXPECT_EQ(pmanager->count(), count_before + 1);

		pquerier_b->do_unregister();
		EXPECT_EQ(pmanager->count(), count_before);
	}

	// 11) statistics
	TEST(CGDK10_query_manager, statistics)
	{
		auto total_try_before = query::statistics::Nquerier::statistics_get_total_query_info().query_try.load();

		auto pquerier = make_own<test_querier>("test.querier.statistics");

		for (int i = 0; i < 10; ++i) pquerier->statistics_on_query_try();
		for (int i = 0; i < 6; ++i) pquerier->statistics_on_query_success();
		for (int i = 0; i < 2; ++i) pquerier->statistics_on_query_fail();
		pquerier->statistics_on_query_retry();
		pquerier->statistics_on_create_session(4);
		pquerier->statistics_on_destroy_session();
		pquerier->statistics_on_alloc_session();
		pquerier->statistics_on_alloc_session();
		pquerier->statistics_on_free_session();

		EXPECT_EQ(pquerier->statistics_get_query_try(), 10u);
		EXPECT_EQ(pquerier->statistics_get_query_success(), 6u);
		EXPECT_EQ(pquerier->statistics_get_query_fail(), 2u);
		EXPECT_EQ(pquerier->statistics_get_query_retry(), 1u);
		EXPECT_EQ(pquerier->statistics_get_query_processing(), 2u);
		EXPECT_EQ(pquerier->statistics_get_exist_session(), 3u);
		EXPECT_EQ(pquerier->statistics_get_using_session(), 1u);

		// - total
		EXPECT_EQ(query::statistics::Nquerier::statistics_get_total_query_info().query_try.load(), total_try_before + 10);

		// - reset : the counters are cleared. the gauges (processing/exist/using) are the current numbers and kept
		pquerier->reset_statistics();
		EXPECT_EQ(pquerier->statistics_get_query_try(), 0u);
		EXPECT_EQ(pquerier->statistics_get_alloc_session(), 0u);
		EXPECT_EQ(pquerier->statistics_get_query_processing(), 2u);
		EXPECT_EQ(pquerier->statistics_get_exist_session(), 3u);
		EXPECT_EQ(pquerier->statistics_get_using_session(), 1u);

		// - the pairs after the reset bring the gauges to 0 (not below)
		for (int i = 0; i < 2; ++i) pquerier->statistics_on_query_success();
		pquerier->statistics_on_destroy_session(3);
		pquerier->statistics_on_free_session();
		EXPECT_EQ(pquerier->statistics_get_query_processing(), 0u);
		EXPECT_EQ(pquerier->statistics_get_exist_session(), 0u);
		EXPECT_EQ(pquerier->statistics_get_using_session(), 0u);
	}

	// 12) print_statistics - with registered queriers
	TEST(CGDK10_query_manager, print_statistics)
	{
		auto pquerier_a = make_own<test_querier>("test.querier.print_a");
		auto pquerier_b = make_own<test_querier>("");

		pquerier_a->do_register();
		pquerier_b->do_register();
		pquerier_a->statistics_on_query_try();
		pquerier_a->statistics_on_query_success();

		query::manager_querier::print_statistics();

		pquerier_a->do_unregister();
		pquerier_b->do_unregister();
		SUCCEED();
	}
}
