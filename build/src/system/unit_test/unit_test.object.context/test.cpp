#include "pch.h"

namespace CGDK
{
	TEST(CGDK10_object_context, context_simple_data) 
	{
		// Test 1. 기본적인 값 테스트
		//
		// bool, integer, float, string 등 제공하는 기본적인 자료형이 제대로 설정되는지를 확인한다.
		//

		context	temp_context;

		temp_context["a"] = 1;
		temp_context["b"] = 2.0f;
		temp_context["c"] = "test text"sv;
		temp_context["d"] = "test text"sv;
		temp_context["e"] = true;


		auto& a1_r = temp_context["a"];
		EXPECT_TRUE(a1_r.v.type == context::value_type_t::number_);
		int v1_r = a1_r;
		EXPECT_TRUE(v1_r == 1);

		auto a1_c = temp_context["a"];
		EXPECT_TRUE(a1_c.v.type == context::value_type_t::number_);
		int v1_c = a1_c;
		EXPECT_TRUE(v1_c == 1);


		auto& a2_r = temp_context["b"];
		EXPECT_TRUE(a2_r.v.type == context::value_type_t::double_);
		float v2_r = a2_r;
		EXPECT_TRUE(v2_r == 2.0f);

		auto a2_c = temp_context["b"];
		EXPECT_TRUE(a2_c.v.type == context::value_type_t::double_);
		float v2_c = a2_c;
		EXPECT_TRUE(v2_c == 2.0f);



		auto& a3_r = temp_context["c"];
		EXPECT_TRUE(a3_r.v.type == context::value_type_t::string_);
		std::string v3_r = a3_r;
		EXPECT_TRUE(v3_r == "test text");

		auto a3_c = temp_context["c"];
		EXPECT_TRUE(a3_c.v.type == context::value_type_t::string_);
		std::string v3_c = a3_c;
		EXPECT_TRUE(v3_c == "test text");


		auto& a4_r = temp_context["e"];
		EXPECT_TRUE(a4_r.v.type == context::value_type_t::bool_);
		bool v4_r = a4_r;
		EXPECT_TRUE(v4_r == true);

		auto a4_c = temp_context["e"];
		EXPECT_TRUE(a4_c.v.type == context::value_type_t::bool_);
		bool v4_c = a4_c;
		EXPECT_TRUE(v4_c == true);
	}

	TEST(CGDK10_object_context, context_value_update)
	{
		// Test 2. 값의 Update 테스트
		//
		// 하나의 key에 다른 데이터 형의 value로 Update를 테스트한다.
		//

		context	temp_context;

		temp_context["a"] = 1;
		int v1 = temp_context["a"];
		EXPECT_TRUE(v1 == 1);

		temp_context["a"] = 2.0f;
		float v2 = temp_context["a"];
		EXPECT_TRUE(v2 == 2.0f);

		temp_context["a"] = "test text";
		std::string v3 = temp_context["a"];
		EXPECT_TRUE(v3 == "test text");

		temp_context["a"] = true;
		bool v5 = temp_context["a"];
		EXPECT_TRUE(v5 == true);

		temp_context["b"] = "true";
		bool v6 = temp_context["b"];
		EXPECT_TRUE(v6 == true);

		temp_context["c"] = false;
		bool v7 = temp_context["c"];
		EXPECT_TRUE(v7 == false);

		temp_context["d"] = "false";
		bool v8 = temp_context["d"];
		EXPECT_TRUE(v8 == false);
	}

	TEST(CGDK10_object_context, context_key_case_insensitive)
	{
		// Test 3. key값 인식 테스트
		//
		// key값이 case sensitive하지 않게 동작하는지 확인한다.
		//

		context	temp_context;

		temp_context["a"] = 1;

		int v11 = temp_context["a"];
		EXPECT_TRUE(v11 == 1);

		int v12 = temp_context["A"];
		EXPECT_TRUE(v12 == 1);
	}

	TEST(CGDK10_object_context, context_invalid_key)
	{
		// Test 3. key값 인식 테스트
		//
		// key값이 case sensitive하지 않게 동작하는지 확인한다.
		//
		context	temp_context;

		{
			auto& x1 = temp_context["x"];

			EXPECT_TRUE(x1.is_valid() == false);
			EXPECT_TRUE(x1.is_invalid() == true);

			temp_context["X"sv] = 100;

			EXPECT_TRUE(x1.is_valid() == true);
			EXPECT_TRUE(x1.is_invalid() == false);

			int v = temp_context["x"];

			EXPECT_TRUE(v == 100);
		}

		{
			temp_context["a"]["b"]["c"]["d"] = 100;

			auto b1 = temp_context["a"];

			EXPECT_TRUE(b1.is_valid() == true);
			EXPECT_TRUE(b1.is_invalid() == false);

			auto& b2 = temp_context["a"]["b"];

			EXPECT_TRUE(b2.is_valid() == true);
			EXPECT_TRUE(b2.is_invalid() == false);

			auto& b3 = temp_context["a"]["b"]["c"];

			EXPECT_TRUE(b3.is_valid() == true);
			EXPECT_TRUE(b3.is_invalid() == false);

			auto& b4 = temp_context["a"]["b"]["c"]["d"];

			EXPECT_TRUE(b4.is_valid() == true);
			EXPECT_TRUE(b4.is_invalid() == false);

			EXPECT_TRUE((int64_t)b4 == 100);
			//EXPECT_TRUE(b4.type == context::value_type_t::INTEGER_);
		}

	}

	TEST(CGDK10_object_context, context_get_set_value_int)
	{
		context	temp_context;

		auto& y1 = temp_context["a"]["b"]["c"]["d"];

		EXPECT_TRUE(y1.is_valid() == false);
		EXPECT_TRUE(y1.is_invalid() == true);

		auto& a1 = temp_context["a"sv];

		EXPECT_TRUE(a1.is_valid() == false);
		EXPECT_TRUE(a1.is_invalid() == true);

		auto& a2 = temp_context["a"]["b"];

		EXPECT_TRUE(a2.is_valid() == false);
		EXPECT_TRUE(a2.is_invalid() == true);

		auto& a3 = temp_context["a"]["b"]["c"];

		EXPECT_TRUE(a3.is_valid() == false);
		EXPECT_TRUE(a3.is_invalid() == true);

		try
		{
			y1 = 10000;
		}
		catch (...)
		{
			EXPECT_TRUE(false);
		}

		try
		{
			int y2 = temp_context["a"]["b"]["c"]["d"];

			EXPECT_TRUE(y2 == 10000);
		}
		catch (...)
		{
			EXPECT_TRUE(false);
		}

		try
		{
			a3 = 10000;

			EXPECT_TRUE(false);
		}
		catch (...)
		{

		}

		try
		{
			[[maybe_unused]] int value = a3;

			EXPECT_TRUE(false);
		}
		catch (...)
		{
		}

		try
		{
			[[maybe_unused]] int value = a2;

			EXPECT_TRUE(false);
		}
		catch (...)
		{

		}

		try
		{
			a2 = 1000;

			EXPECT_TRUE(false);
		}
		catch (...)
		{
		}

		try
		{
			[[maybe_unused]] int value = a2;

			EXPECT_TRUE(false);
		}
		catch (...)
		{
		}

		try
		{
			a1 = 100;

			EXPECT_TRUE(false);
		}
		catch (...)
		{
		}

		try
		{
			[[maybe_unused]] int value = a1;

			EXPECT_TRUE(false);
		}
		catch (...)
		{
		}
	}
	TEST(CGDK10_object_context, context_value_array)
	{
		// Test 4. 배열 테스트
		//
		// 배열 값이 제대로 설정되고 읽을 수 있는지 테스트 한다.
		//
		context	temp_context;

		temp_context["a"][0] = "string array x"sv;
		temp_context["a"][1] = "best foo"sv;
		temp_context["a"][2] = "variable"sv;

		std::string v11 = temp_context["a"][0];
		EXPECT_TRUE(v11.compare("string array x") == 0);

		std::string v12 = temp_context["a"][1];
		EXPECT_TRUE(v12.compare("best foo") == 0);

		std::string v13 = temp_context["a"][2];
		EXPECT_TRUE(v13.compare("variable") == 0);
	}

	TEST(CGDK10_object_context, context_hierarchy_1)
	{
		// Test 5. 다계층 값 테스트
		//
		// 다계층 값이 제대로 설정되고 읽을 수 있는지 테스트 한다.
		//
		context	temp_context;

		temp_context["a"]["X"] = "string array x"sv;
		temp_context["a"]["Y"] = "best foo"sv;
		temp_context["a"]["Z"] = "variable"sv;

		std::string v11 = temp_context["a"]["x"];
		EXPECT_TRUE(v11 == "string array x");

		std::string v12 = temp_context["a"]["y"];
		EXPECT_TRUE(v12 == "best foo");

		std::string v13 = temp_context["a"]["z"];
		EXPECT_TRUE(v13 == "variable"sv);

		temp_context["b"]["fllowe_x"] = 1324;
		temp_context["b"]["ttexfd"] = 123.0f;
		temp_context["b"]["fdaf_2"] = "testfafdas";

		uint64_t v21 = temp_context["b"]["fllowe_x"];
		EXPECT_TRUE(v21 == 1324);

		float v22 = temp_context["b"]["ttexfd"];
		EXPECT_TRUE(v22 == 123.0f);

		std::string v23 = temp_context["b"]["fdaf_2"];
		EXPECT_TRUE(v23 == "testfafdas"sv);
	}

	TEST(CGDK10_object_context, context_string)
	{
		// Test 5. 다계층 값 테스트
		//
		// 다계층 값이 제대로 설정되고 읽을 수 있는지 테스트 한다.
		//
		context	temp_context;

		temp_context["a"] = "string array x"s;
		temp_context["b"] = "best foo"s;
		temp_context["c"] = "variable"s;

		std::string v11 = temp_context["a"];
		EXPECT_TRUE(v11 == "string array x"s);

		std::string v12;
		v12 = (std::string&&)temp_context["a"];
		EXPECT_TRUE(v12 == "string array x");

		std::string v13;
		v13 = (const std::string&)temp_context["a"];
		EXPECT_TRUE(v13 == "string array x");

		std::string v14;
		v14 = (std::string&&)temp_context["b"];
		EXPECT_TRUE(v14 == "best foo"s);

		//std::string v15;
		//v15 = (std::string)temp_context["c"];
		//EXPECT_TRUE(v15 == "variable");
	}

	TEST(CGDK10_object_context, context_struct)
	{
		context	temp_context;

	}

	struct TEST_STRUCT1
	{
		int value1;
		float value2;
		char value3;
	};

	struct TEST_STRUCT2
	{
		ENABLE_STRUCT_SERIALIZABLE

		int value1;
		std::string value2;
		std::map<int, int> value3;
		std::map<int, std::string> value4;
	};

	TEST(CGDK10_object_context, context_struct_serializable)
	{
		context	temp_context;

		TEST_STRUCT1 test_struct1;
		test_struct1.value1 = 100;
		test_struct1.value2 = 10.0f;
		test_struct1.value3 = 1;

		TEST_STRUCT2 test_struct2;
		test_struct2.value1 = 100;
		test_struct2.value2 = "test string";
		test_struct2.value3[0] = 10;
		test_struct2.value3[1] = 20;
		test_struct2.value3[2] = 30;
		test_struct2.value4[0] = "test1";
		test_struct2.value4[1] = "test2";
		test_struct2.value4[2] = "test3";

		temp_context["a"] = test_struct1;
		temp_context["b"] = test_struct2;

		TEST_STRUCT1 v1;
		v1 = temp_context["a"];

		EXPECT_TRUE(v1.value1 == 100);
		EXPECT_TRUE(v1.value2 == 10.0f);
		EXPECT_TRUE(v1.value3 == 1);


		TEST_STRUCT2 v2;
		v2 = temp_context["b"];

		EXPECT_TRUE(v2.value1 == 100);
		EXPECT_TRUE(v2.value2 == "test string");
		EXPECT_TRUE(v2.value3[0] == 10);
		EXPECT_TRUE(v2.value3[1] == 20);
		EXPECT_TRUE(v2.value3[2] == 30);
		EXPECT_TRUE(v2.value4[0] == "test1");
		EXPECT_TRUE(v2.value4[1] == "test2");
		EXPECT_TRUE(v2.value4[2] == "test3");


		auto size_context = get_size_of(temp_context);
		auto buffer_temp = alloc_shared_buffer(size_context);

		buffer_temp.append(temp_context);

		auto t3 = buffer_temp.extract<context>();

		TEST_STRUCT2 v3;
		v3 = t3["b"];

		EXPECT_TRUE(v3.value1 == 100);
		EXPECT_TRUE(v3.value2 == "test string");
		EXPECT_TRUE(v3.value3[0] == 10);
		EXPECT_TRUE(v3.value3[1] == 20);
		EXPECT_TRUE(v3.value3[2] == 30);
		EXPECT_TRUE(v3.value4[0] == "test1");
		EXPECT_TRUE(v3.value4[1] == "test2");
		EXPECT_TRUE(v3.value4[2] == "test3");
	}

	TEST(CGDK10_object_context, context_struct_copy_assignment)
	{
		// declare)
		context	temp_context;

		// 1) prepare context
		TEST_STRUCT1 test_struct1;
		test_struct1.value1 = 100;
		test_struct1.value2 = 10.0f;
		test_struct1.value3 = 1;

		TEST_STRUCT2 test_struct2;
		test_struct2.value1 = 100;
		test_struct2.value2 = "test string";
		test_struct2.value3[0] = 10;
		test_struct2.value3[1] = 20;
		test_struct2.value3[2] = 30;
		test_struct2.value4[0] = "test1";
		test_struct2.value4[1] = "test2";
		test_struct2.value4[2] = "test3";

		temp_context["a"] = test_struct1;
		temp_context["b"] = test_struct2;

		auto size_context = get_size_of(temp_context);

		// 2) copy 
		{
			// - copy
			context temp_context_copy = temp_context;

			// - get size of copy
			auto size_context_copy = get_size_of(temp_context_copy);

			EXPECT_TRUE(size_context == size_context_copy);

			TEST_STRUCT1 v1;
			v1 = temp_context_copy["a"];

			EXPECT_TRUE(v1.value1 == 100);
			EXPECT_TRUE(v1.value2 == 10.0f);
			EXPECT_TRUE(v1.value3 == 1);

			TEST_STRUCT2 v2;
			v2 = temp_context_copy["b"];

			EXPECT_TRUE(v2.value1 == 100);
			EXPECT_TRUE(v2.value2 == "test string");
			EXPECT_TRUE(v2.value3[0] == 10);
			EXPECT_TRUE(v2.value3[1] == 20);
			EXPECT_TRUE(v2.value3[2] == 30);
			EXPECT_TRUE(v2.value4[0] == "test1");
			EXPECT_TRUE(v2.value4[1] == "test2");
			EXPECT_TRUE(v2.value4[2] == "test3");
		}

		// 3) assignment
		{
			// 1) copy
			context temp_context_assign;
			
			temp_context_assign = temp_context;

			// - get size of copy
			auto size_context_assign = get_size_of(temp_context_assign);

			EXPECT_TRUE(size_context == size_context_assign);

			TEST_STRUCT1 v1;
			v1 = temp_context_assign["a"];

			EXPECT_TRUE(v1.value1 == 100);
			EXPECT_TRUE(v1.value2 == 10.0f);
			EXPECT_TRUE(v1.value3 == 1);

			TEST_STRUCT2 v2;
			v2 = temp_context_assign["b"];

			EXPECT_TRUE(v2.value1 == 100);
			EXPECT_TRUE(v2.value2 == "test string");
			EXPECT_TRUE(v2.value3[0] == 10);
			EXPECT_TRUE(v2.value3[1] == 20);
			EXPECT_TRUE(v2.value3[2] == 30);
			EXPECT_TRUE(v2.value4[0] == "test1");
			EXPECT_TRUE(v2.value4[1] == "test2");
			EXPECT_TRUE(v2.value4[2] == "test3");
		}
	}

	TEST(CGDK10_object_context, context_node_clear_1)
	{
		// declare)
		context	temp_context;

		// 1) set value
		temp_context["a"] = 100;

		// check) 
		EXPECT_TRUE(temp_context["a"].v.type == context::value_type_t::number_);

		// 2) clear
		temp_context.clear();

		// check) 
		EXPECT_TRUE(temp_context.v.type == context::value_type_t::null_);
	}
	TEST(CGDK10_object_context, context_node_clear_2)
	{
		// declare)
		context	temp_context;

		temp_context["a"]["b"]["c"]["d"] = 100;

		// check) 
		EXPECT_TRUE(temp_context["a"].v.type == context::value_type_t::node_);

		// check) 
		EXPECT_TRUE(temp_context["a"]["b"].v.type == context::value_type_t::node_);

		// check) 
		EXPECT_TRUE(temp_context["a"]["b"]["c"].v.type == context::value_type_t::node_);

		// check) 
		EXPECT_TRUE(temp_context["a"]["b"]["c"]["d"].v.type == context::value_type_t::number_);

		temp_context["a"]["b"]["c"]["d"].clear();

		// check) 
		EXPECT_TRUE(temp_context["a"]["b"]["c"].is_valid() == false);

		// check) 
		EXPECT_TRUE(temp_context["a"]["b"].is_valid() == false);

		// check) 
		EXPECT_TRUE(temp_context["a"].is_valid() == false);

		// check) 
		EXPECT_TRUE(temp_context.is_valid() == false);
	}

	struct TEST_STRUCT3 : public Ibuffer_serializable
	{
		int value1;
		std::string value2;
		std::map<int, int> value3;
		std::map<int, std::string> value4;

		virtual	void process_serialize_in(buffer_view& _buffer) override
		{
			this->value1 = _buffer.extract<int>();
			this->value2 = _buffer.extract<std::string>();
			this->value3 = _buffer.extract<std::map<int, int>>();
			this->value4 = _buffer.extract<std::map<int, std::string>>();
		}
		virtual	void process_serialize_out(buffer& _buffer) override
		{
			_buffer.append<int>(this->value1);
			_buffer.append<std::string>(this->value2);
			_buffer.append<std::map<int, int>>(this->value3);
			_buffer.append<std::map<int, std::string>>(this->value4);
		}
		virtual std::size_t	process_size_of() const override
		{
			return get_size_of(this->value1) + get_size_of(this->value2) + get_size_of(this->value3) + get_size_of(this->value4);
		}
	};

	TEST(CGDK10_object_context, context_struct_Ibuffer_serializable)
	{
		context	temp_context;

		TEST_STRUCT3 test_struct;
		test_struct.value1 = 100;
		test_struct.value2 = "test string";
		test_struct.value3[0] = 10;
		test_struct.value3[1] = 20;
		test_struct.value3[2] = 30;
		test_struct.value4[0] = "test1";
		test_struct.value4[1] = "test2";
		test_struct.value4[2] = "test3";

		temp_context["a"] = test_struct;

		TEST_STRUCT3 v2;
		v2 = temp_context["a"];


		EXPECT_TRUE(v2.value1 == 100);
		EXPECT_TRUE(v2.value2 == "test string");
		EXPECT_TRUE(v2.value3[0] == 10);
		EXPECT_TRUE(v2.value3[1] == 20);
		EXPECT_TRUE(v2.value3[2] == 30);
		EXPECT_TRUE(v2.value4[0] == "test1");
		EXPECT_TRUE(v2.value4[1] == "test2");
		EXPECT_TRUE(v2.value4[2] == "test3");


		auto size_context = get_size_of(temp_context);
		auto buffer_temp = alloc_shared_buffer(size_context);

		buffer_temp.append(temp_context);

		auto t3 = buffer_temp.extract<context>();

		TEST_STRUCT2 v3;
		v3 = t3["a"];

		EXPECT_TRUE(v3.value1 == 100);
		EXPECT_TRUE(v3.value2 == "test string");
		EXPECT_TRUE(v3.value3[0] == 10);
		EXPECT_TRUE(v3.value3[1] == 20);
		EXPECT_TRUE(v3.value3[2] == 30);
		EXPECT_TRUE(v3.value4[0] == "test1");
		EXPECT_TRUE(v3.value4[1] == "test2");
		EXPECT_TRUE(v3.value4[2] == "test3");
	}

	TEST(CGDK10_object_context, context_serialize)
	{
		context	temp_context;

		constexpr auto  tv1 = "string array x"sv;
		constexpr auto  tv2 = "best foo"sv;
		constexpr auto  tv3 = "variable"sv;

		temp_context["a"]["X"] = tv1;
		temp_context["a"]["Y"] = tv2;
		temp_context["a"]["Z"] = tv3;

		std::string v11 = temp_context["a"]["x"];
		EXPECT_TRUE(v11 == tv1);

		std::string v12 = temp_context["a"]["y"];
		EXPECT_TRUE(v12 == tv2);

		std::string v13 = temp_context["a"]["z"];
		EXPECT_TRUE(v13 == tv3);

		constexpr auto  tv4 = 1324;
		constexpr auto  tv5 = 123.0f;
		constexpr auto  tv6 = "testfafdas";

		temp_context["b"]["fllowe_x"] = tv4;
		temp_context["b"]["ttexfd"] = tv5;
		temp_context["b"]["fdaf_2"] = tv6;

		auto size_context = get_size_of(temp_context);

		auto buffer_temp = alloc_shared_buffer(size_context);

		buffer_temp.append(temp_context);

		EXPECT_TRUE(buffer_temp.size() == size_context);

		auto v = buffer_temp.extract<context>();

		EXPECT_TRUE(buffer_temp.size() == 0);

		std::string	vax = v["a"]["X"];
		std::string	vay = v["a"]["Y"];
		std::string vaz = v["a"]["Z"];

		EXPECT_TRUE(vax == tv1);
		EXPECT_TRUE(vay == tv2);
		EXPECT_TRUE(vaz == tv3);

		int64_t		vb1 = v["b"]["fllowe_x"];
		double		vb2 = v["b"]["ttexfd"];
		std::string vb3 = v["b"]["fdaf_2"];

		EXPECT_TRUE(vb1 == tv4);
		EXPECT_TRUE(vb2 == tv5);
		EXPECT_TRUE(vb3 == tv6);

	}

	TEST(CGDK10_object_context, context_chrono)
	{
		context	temp_context;

		auto time_point_1 = chrono::tick::now();
		auto time_point_2 = chrono::time::local::now();
		auto time_point_3 = chrono::time::utc::now();

		temp_context["a"] = time_point_1;
		EXPECT_TRUE(temp_context["a"].is_valid());
		EXPECT_TRUE(temp_context["a"].type() == context::value_type_t::chrono_time_point_);

		temp_context["b"] = time_point_2;
		EXPECT_TRUE(temp_context["b"].is_valid());
		EXPECT_TRUE(temp_context["b"].type() == context::value_type_t::chrono_time_point_);

		temp_context["c"] = time_point_3;
		EXPECT_TRUE(temp_context["c"].is_valid());
		EXPECT_TRUE(temp_context["c"].type() == context::value_type_t::chrono_time_point_);

		chrono::tick::time_point v1 = temp_context["a"];
		chrono::time::local::time_point v2 = temp_context["b"];
		chrono::time::utc::time_point v3 = temp_context["c"];

		EXPECT_TRUE(time_point_1 == v1);
		EXPECT_TRUE(time_point_2 == v2);
		EXPECT_TRUE(time_point_3 == v3);
	}

	TEST(CGDK10_object_context, context_compare)
	{
	}
	//-------------------------------------------------------------------------
	// 수정된 버그 확인
	//-------------------------------------------------------------------------
	// - 멤버 함수 안에서 operator std::string()을 불러도 문자열로 변환된다. (operator T()가 선택되지 않는다.)
	struct _context_string_reader
	{
		std::string read(const context& _context) const { return _context.operator std::string(); }
		std::string read_assign(const context& _context) const { std::string value = _context; return value; }
	};

	TEST(CGDK10_object_context, fix_string_conversion_in_member_function)
	{
		context temp;
		temp["a"] = "running"sv;
		temp["n"] = 12;

		_context_string_reader reader;
		EXPECT_EQ(reader.read(temp["a"]), "running");
		EXPECT_EQ(reader.read_assign(temp["a"]), "running");
		EXPECT_EQ(reader.read(temp["n"]), "12");

		// - string_view: only for string value
		std::string_view view = temp["a"];
		EXPECT_EQ(view, "running"sv);
		EXPECT_ANY_THROW({ std::string_view view_number = temp["n"]; (void)view_number; });
	}

	TEST(CGDK10_object_context, fix_compare_operators)
	{
		context temp;
		temp["n"] = 5;
		temp["s"] = "abc"sv;
		temp["d"] = 2.5;

		const auto& n = temp["n"];
		EXPECT_TRUE(n < int64_t(6));
		EXPECT_FALSE(n < int64_t(5));
		EXPECT_TRUE(n >= int64_t(5));
		EXPECT_FALSE(n >= int64_t(6));
		EXPECT_FALSE(int64_t(5) != n);
		EXPECT_TRUE(int64_t(4) != n);

		const auto& s = temp["s"];
		EXPECT_FALSE("abc"sv != s);
		EXPECT_TRUE("abd"sv != s);

		const auto& d = temp["d"];
		EXPECT_TRUE(d == 2.5);
		EXPECT_TRUE(d < 3.0);
		EXPECT_FALSE(d < 2.0);
		EXPECT_TRUE(n == 5.0);

		// - double compare with string value is false (was compared with the pointer value)
		EXPECT_FALSE(s == 0.0);
		EXPECT_FALSE(s > -1.0);
	}

	TEST(CGDK10_object_context, fix_bool_from_number)
	{
		context temp;
		temp["one"] = 1;
		temp["zero"] = 0;
		temp["t"] = "TRUE"sv;

		EXPECT_TRUE(static_cast<bool>(temp["one"]));
		EXPECT_FALSE(static_cast<bool>(temp["zero"]));
		EXPECT_TRUE(static_cast<bool>(temp["t"]));
	}

	TEST(CGDK10_object_context, fix_const_index)
	{
		context temp;
		temp.from_json(R"({ "list": [ "a", "b" ] })"sv);

		const context& list = temp["list"];
		EXPECT_EQ(list[0].operator std::string(), "a");
		EXPECT_EQ(list[1].operator std::string(), "b");
		EXPECT_TRUE(list[2].is_invalid());
	}

	TEST(CGDK10_object_context, fix_erase_case_insensitive)
	{
		context temp;
		temp["Key"] = 1;
		temp["other"] = 2;

		std::string buffer = "KEYxxxx";
		EXPECT_TRUE(temp.erase(std::string_view(buffer.data(), 3)));	// - not null-terminated view
		EXPECT_FALSE(temp.has_key("key"));
		EXPECT_TRUE(temp.has_key("OTHER"));
	}

	TEST(CGDK10_object_context, fix_self_assignment_and_swap)
	{
		context temp;
		temp["a"]["b"] = "value"sv;

		auto& self = temp;
		temp = self;
		EXPECT_EQ(temp["a"]["b"].operator std::string(), "value");

		context other;
		other["x"] = 1;

		temp.swap(other);
		EXPECT_EQ(static_cast<int>(temp["x"]), 1);
		EXPECT_EQ(other["a"]["b"].operator std::string(), "value");
		EXPECT_EQ(other["a"].pnode_parent, &other);
		EXPECT_EQ(temp["x"].pnode_parent, &temp);
	}

	TEST(CGDK10_object_context, fix_append_and_plus)
	{
		context base;
		base.from_json(R"({ "a": 1, "obj": { "x": 1, "y": 2 } })"sv);

		context over;
		over.from_json(R"({ "a": 10, "obj": { "y": 20 }, "b": "new" })"sv);

		auto merged = base + over;
		EXPECT_EQ(static_cast<int>(merged["a"]), 10);
		EXPECT_EQ(static_cast<int>(merged["obj"]["x"]), 1);
		EXPECT_EQ(static_cast<int>(merged["obj"]["y"]), 20);
		EXPECT_EQ(merged["b"].operator std::string(), "new");
		EXPECT_EQ(merged["obj"].pnode_parent, &merged);

		context l, r;
		l["a"] = 1;
		r["b"] = 2;
		auto moved = std::move(l) + std::move(r);
		EXPECT_EQ(static_cast<int>(moved["a"]), 1);
		EXPECT_EQ(static_cast<int>(moved["b"]), 2);
	}

	TEST(CGDK10_object_context, fix_serialize_does_not_change_source)
	{
		context temp;
		temp["a"]["b"] = 1;
		temp["a"]["c"] = "x"sv;

		auto buf = alloc_shared_buffer(temp.get_size_of());
		temp.serialize_out(buf);

		// - extract twice from the same buffer (count was changed by '--count' before)
		for (int i = 0; i < 2; ++i)
		{
			buffer_view view = buf;
			context read;
			read.serialize_in(view);
			EXPECT_EQ(static_cast<int>(read["a"]["b"]), 1);
			EXPECT_EQ(read["a"]["c"].operator std::string(), "x");
			EXPECT_EQ(read["a"].pnode_parent, &read);
		}
	}

	TEST(CGDK10_object_context, fix_json_number_double_escape)
	{
		context temp;
		temp.from_json(R"({ "i": 3, "d": 1.5, "s": "a\"b\\c\nd", "b": true, "o": { "k": -7 } })"sv);

		EXPECT_EQ(temp["i"].type(), context::value_type_t::number_);
		EXPECT_EQ(temp["d"].type(), context::value_type_t::double_);
		EXPECT_DOUBLE_EQ(static_cast<double>(temp["d"]), 1.5);
		EXPECT_EQ(temp["s"].operator std::string(), "a\"b\\c\nd");

		// - round trip
		auto json = temp.to_json();
		std::string_view json_text(json.data<char>(), json.size());
		EXPECT_EQ(json_text.find('\0'), std::string_view::npos);

		context read;
		read.from_json(json_text);
		EXPECT_EQ(static_cast<int>(read["i"]), 3);
		EXPECT_DOUBLE_EQ(static_cast<double>(read["d"]), 1.5);
		EXPECT_EQ(read["s"].operator std::string(), "a\"b\\c\nd");
		EXPECT_TRUE(static_cast<bool>(read["b"]));
		EXPECT_EQ(static_cast<int>(read["o"]["k"]), -7);
	}

	TEST(CGDK10_object_context, fix_json_not_null_terminated)
	{
		std::string source = R"({ "a": 1 })" "GARBAGE";
		context temp;
		temp.from_json(std::string_view(source.data(), 10));
		EXPECT_EQ(static_cast<int>(temp["a"]), 1);
	}

	TEST(CGDK10_object_context, fix_json_skips_null_first_node)
	{
		context temp;
		(void)temp["aaa"];		// - null node (created by find)
		temp["b"] = 1;

		auto json = temp.to_json();
		EXPECT_EQ(std::string_view(json.data<char>(), json.size()), R"({"b":1})"sv);
	}

	TEST(CGDK10_object_context, fix_copy_buffer_value)
	{
		// - buffer value which is a part of the original buffer
		auto buf_source = alloc_shared_buffer(16);
		buf_source.append<uint32_t>(0x11111111);
		buf_source.append<uint32_t>(0x22222222);

		context temp;
		temp["buf"] = shared_buffer(buf_source + offset(4));

		context copied = temp;
		buffer_view view = copied["buf"];
		ASSERT_EQ(view.size(), 4u);
		EXPECT_EQ(view.front<uint32_t>(), 0x22222222u);
	}
	TEST(CGDK10_object_context, try_find_does_not_create_node)
	{
		context temp;
		temp["a"]["b"] = 1;

		// - exist
		const context* pa = temp.try_find("A");
		ASSERT_NE(pa, nullptr);
		ASSERT_NE(pa->try_find("b"), nullptr);
		EXPECT_EQ(static_cast<int>(*pa->try_find("b")), 1);

		// - not exist: no node is created
		EXPECT_EQ(temp.try_find("none"), nullptr);
		EXPECT_EQ(temp.map_nodes.count("none"), 0u);

		// - null node (created by find) is not returned
		(void)temp["empty"];
		EXPECT_EQ(temp.try_find("empty"), nullptr);
	}
}