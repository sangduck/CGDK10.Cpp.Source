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

		temp_context["a"sv] = 1;
		temp_context["b"sv] = 2.0f;
		temp_context["c"sv] = "test text"sv;
		temp_context["d"sv] = "test text"sv;
		temp_context["e"sv] = true;

		auto a1 = temp_context["a"sv];
		EXPECT_TRUE(a1.v.type == eCONTEXT_VALUE_TYPE::NUMBER_);
		int v1 = a1;
		EXPECT_TRUE(v1 == 1);

		auto a2 = temp_context["b"sv];
		EXPECT_TRUE(a2.v.type == eCONTEXT_VALUE_TYPE::NUMBER_);
		float v2 = a2;
		EXPECT_TRUE(v2 == 2.0f);

		auto a3 = temp_context["c"sv];
		EXPECT_TRUE(a3.v.type == eCONTEXT_VALUE_TYPE::STRING_);
		std::string v3 = a3;
		EXPECT_TRUE(v3 == "test text"sv);

		auto a4 = temp_context["d"sv];
		EXPECT_TRUE(a3.v.type == eCONTEXT_VALUE_TYPE::STRING_);
		std::wstring v4 = a4;
		EXPECT_TRUE(v4 == L"test text"sv);

		auto a5 = temp_context["e"sv];
		EXPECT_TRUE(a5.v.type == eCONTEXT_VALUE_TYPE::BOOL_);
		bool v5 = a5;
		EXPECT_TRUE(v5 == true);
	}

	TEST(CGDK10_object_context, context_value_update)
	{
		// Test 2. 값의 Update 테스트
		//
		// 하나의 key에 다른 데이터 형의 value로 Update를 테스트한다.
		//

		context	temp_context;

		temp_context["a"sv] = 1;
		int v1 = temp_context["a"sv];
		EXPECT_TRUE(v1 == 1);

		temp_context["a"sv] = 2.0f;
		float v2 = temp_context["a"sv];
		EXPECT_TRUE(v2 == 2.0f);

		temp_context["a"sv] = "test text"sv;
		std::string v3 = temp_context["a"sv];
		EXPECT_TRUE(v3 == "test text"sv);

		temp_context["a"sv] = true;
		bool v5 = temp_context["a"sv];
		EXPECT_TRUE(v5 == true);
	}

	TEST(CGDK10_object_context, context_key_case_insensetive)
	{
		// Test 3. key값 인식 테스트
		//
		// key값이 case sensetive하지 않게 동작하는지 확인한다.
		//

		context	temp_context;

		temp_context["a"sv] = 1;

		int v11 = temp_context["a"sv];
		EXPECT_TRUE(v11 == 1);

		int v12 = temp_context["A"sv];
		EXPECT_TRUE(v12 == 1);
	}

	TEST(CGDK10_object_context, context_invalid_key)
	{
		// Test 3. key값 인식 테스트
		//
		// key값이 case sensetive하지 않게 동작하는지 확인한다.
		//
		context	temp_context;

		{
			auto& x1 = temp_context["x"sv];

			EXPECT_TRUE(x1.is_valid() == false);
			EXPECT_TRUE(x1.is_invalid() == true);

			temp_context["x"sv] = 100;

			EXPECT_TRUE(x1.is_valid() == true);
			EXPECT_TRUE(x1.is_invalid() == false);
		}

		{
			auto& y1 = temp_context["a"sv]["b"sv]["c"sv]["d"sv];

			EXPECT_TRUE(y1.is_valid() == false);
			EXPECT_TRUE(y1.is_invalid() == true);

			auto& a1 = temp_context["a"sv];

			EXPECT_TRUE(a1.is_valid() == false);
			EXPECT_TRUE(a1.is_invalid() == true);

			auto& a2 = temp_context["a"sv]["b"sv];

			EXPECT_TRUE(a2.is_valid() == false);
			EXPECT_TRUE(a2.is_invalid() == true);

			auto& a3 = temp_context["a"sv]["b"sv]["c"sv];

			EXPECT_TRUE(a3.is_valid() == false);
			EXPECT_TRUE(a3.is_invalid() == true);
		}

		{
			temp_context["a"sv]["b"sv]["c"sv]["d"sv] = 100;

			auto b1 = temp_context["a"sv];

			EXPECT_TRUE(b1.is_valid() == true);
			EXPECT_TRUE(b1.is_invalid() == false);

			auto& b2 = temp_context["a"sv]["b"sv];

			EXPECT_TRUE(b2.is_valid() == true);
			EXPECT_TRUE(b2.is_invalid() == false);

			auto& b3 = temp_context["a"sv]["b"sv]["c"sv];

			EXPECT_TRUE(b3.is_valid() == true);
			EXPECT_TRUE(b3.is_invalid() == false);

			auto& b4 = temp_context["a"sv]["b"sv]["c"sv]["d"sv];

			EXPECT_TRUE(b4.is_valid() == true);
			EXPECT_TRUE(b4.is_invalid() == false);

			EXPECT_TRUE((int64_t)b4 == 100);
			//EXPECT_TRUE(b4.type == eCONTEXT_VALUE_TYPE::INTEGER_);
		}
	}
	TEST(CGDK10_object_context, context_value_array)
	{
		// Test 4. 배열 테스트
		//
		// 배열 값이 제대로 설정되고 읽을 수 있는지 테스트 한다.
		//
		context	temp_context;

		temp_context["a"sv][0] = "string array x"sv;
		temp_context["a"sv][1] = "best foo"sv;
		temp_context["a"sv][2] = "variable"sv;

		std::string v11 = temp_context["a"sv][0];
		EXPECT_TRUE(v11.compare("string array x"sv) == 0);

		std::string v12 = temp_context["a"sv][1];
		EXPECT_TRUE(v12.compare("best foo"sv) == 0);

		std::string v13 = temp_context["a"sv][2];
		EXPECT_TRUE(v13.compare("variable"sv) == 0);
	}

	TEST(CGDK10_object_context, context_hierarchy_1)
	{
		// Test 5. 다계층 값 테스트
		//
		// 다계층 값이 제대로 설정되고 읽을 수 있는지 테스트 한다.
		//
		context	temp_context;

		temp_context["a"sv]["X"sv] = "string array x"sv;
		temp_context["a"sv]["Y"sv] = "best foo"sv;
		temp_context["a"sv]["Z"sv] = "variable"sv;

		std::string v11 = temp_context["a"sv]["x"sv];
		EXPECT_TRUE(v11 == "string array x"sv);

		std::string v12 = temp_context["a"sv]["y"sv];
		EXPECT_TRUE(v12 == "best foo"sv);

		std::string v13 = temp_context["a"sv]["z"sv];
		EXPECT_TRUE(v13 == "variable"sv);

		temp_context["b"sv]["fllowe_x"sv] = 1324;
		temp_context["b"sv]["ttexfd"sv] = 123.0f;
		temp_context["b"sv]["fdaf_2"sv] = "testfafdas";

		uint64_t v21 = temp_context["b"sv]["fllowe_x"sv];
		EXPECT_TRUE(v21 == 1324);

		float v22 = temp_context["b"sv]["ttexfd"sv];
		EXPECT_TRUE(v22 == 123.0f);

		std::string v23 = temp_context["b"sv]["fdaf_2"sv];
		EXPECT_TRUE(v23 == "testfafdas"sv);
	}
}