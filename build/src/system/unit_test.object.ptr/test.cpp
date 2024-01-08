#include "pch.h"


namespace CGDK
{
	class foo : virtual public Ireferenceable
	{
	};

	class koo : virtual public Ireferenceable
	{
	public:
		koo(int _x, int _y) : m_x(_x), m_y(_y) {}
	public:
		int m_x;
		int m_y;
	};

	class fal : public foo
	{
	};

	TEST(CGDK10_ptr, ptr_object_ptr_ref)
	{
		// 1) 
		object_ptr<foo> p1 = make_object<foo>();

		// check) 
		EXPECT_TRUE(p1->reference_count() == 1);

		// 2) ref count-1 (copy constructor)
		{
			object_ptr<foo> p2 = p1;

			// check) 
			EXPECT_TRUE(p1->reference_count() == 2);
			EXPECT_TRUE(p1.get() == p2.get());
			
		}

		// check) 
		EXPECT_TRUE(p1->reference_count() == 1);

		// 3) operator=()/ reset()
		{
			object_ptr<foo> p2;

			// - assign
			p2 = p1;

			// check) 
			EXPECT_TRUE(p2->reference_count() == 2);
			EXPECT_TRUE(p1.get() == p2.get());

			// - reset
			p2.reset();

			// check) 
			EXPECT_TRUE(p1->reference_count() == 1);
			EXPECT_TRUE(p2.get() == nullptr);
		}

		// 3) operator=()
		{
			// - 
			object_ptr<fal> p2 = make_object<fal>();

			// - assign
			object_ptr<fal> p3 = p2;

			// check) 
			EXPECT_TRUE(p3->reference_count() == 2);
			EXPECT_TRUE(p3.get() == p2.get());
		}
	}


	static bool is_final_released = false;
	TEST(CGDK10_ptr, ptr_own_ptr_ref)
	{
		// reset) 
		is_final_released = false;

		// definition
		class cal : virtual public Ireferenceable, public object::Idestroyable
		{
		public:
			virtual	result_code destroy() noexcept override
			{
				is_destroyed = true;
				return result_code(eRESULT::SUCCESS);
			}

			bool is_destroyed = false;

			virtual	void on_final_release() noexcept override
			{
				is_final_released = true;
			}

		};

		// 1) make_own
		own_ptr<cal> p1 = make_own<cal>();

		// check) 
		EXPECT_TRUE(p1->reference_count() == 1);
		EXPECT_TRUE(p1->is_destroyed == false);
		EXPECT_TRUE(is_final_released == false);


		// 2) assign
		object_ptr<cal> p2 = p1;

		// check) 
		EXPECT_TRUE(p2->reference_count() == 2);
		EXPECT_TRUE(p2->is_destroyed == false);
		EXPECT_TRUE(is_final_released == false);

		// 3) move
		own_ptr<cal> p3 = std::move(p1);

		// check) 
		EXPECT_TRUE(p3->reference_count() == 2);
		EXPECT_TRUE(p3->is_destroyed == false);
		EXPECT_TRUE(is_final_released == false);
		EXPECT_TRUE(p1.empty());

		// 4) move and call destroy
		{
			// - move
			own_ptr<cal> p4 = std::move(p3);

			EXPECT_TRUE(p4->reference_count() == 2);
			EXPECT_TRUE(p4->is_destroyed == false);
			EXPECT_TRUE(is_final_released == false);

		}

		// check)
		EXPECT_TRUE(p2->reference_count() == 1);
		EXPECT_TRUE(p2->is_destroyed == true);
		EXPECT_TRUE(is_final_released == false);

		// 5) on_final_release
		{
			object_ptr<cal> p5 = std::move(p2);

			EXPECT_TRUE(p5->reference_count() == 1);
			EXPECT_TRUE(p5->is_destroyed == true);
			EXPECT_TRUE(is_final_released == false);
		}

		// check) 
		EXPECT_TRUE(is_final_released == true);
	}

	//class foo : virtual public Ireferenceable
	//{
	//};

	//class koo : virtual public Ireferenceable
	//{
	//public:
	//	koo(int _x, int _y) : m_x(_x), m_y(_y) {}
	//public:
	//	int m_x;
	//	int m_y;
	//};

	//class fal : public foo
	//{
	//};

	TEST(CGDK10_ptr, ptr_any_ptr_construct)
	{
		// 1) test
		object_ptr<foo> p1 = make_object<foo>();
		object_ptr<foo> p2 = make_object<foo>();
		object_ptr<foo> p3 = p1;

		// 2) l-value construct
		any_ptr temp_1(p1);

		// check) 
		EXPECT_TRUE(temp_1.get<foo>() == p3);

		any_ptr temp_2(p1.get());

		// check) 
		EXPECT_TRUE(temp_2.get<foo>() == p3);

		// 3) r-value construct
		any_ptr temp_3(std::move(p1));

		// check) 
		EXPECT_TRUE(temp_3.get<foo>() == p3);
		EXPECT_TRUE(p1.empty());

		// 3) 
		any_ptr temp_4(temp_1);

		// check) 
		EXPECT_TRUE(temp_4.get<foo>() == p3);

		// 4) r-value construct
		any_ptr temp_5(std::move(temp_1));

		// check) 
		EXPECT_TRUE(temp_5.get<foo>() == p3);
		EXPECT_TRUE(temp_1.has_value() == false);
	}

	TEST(CGDK10_ptr, ptr_any_ptr_assign_1)
	{
		object_ptr<foo> p1 = make_object<foo>();
		object_ptr<foo> p2 = make_object<foo>();
		object_ptr<foo> p3 = p1;

		any_ptr temp_1;
		any_ptr temp_2;
		any_ptr temp_3;
		any_ptr temp_4;
		any_ptr temp_5;

		// 2) l-value assign
		temp_1 = p1;

		// check) 
		EXPECT_TRUE(temp_1.get<foo>() == p3);

		// 3) pointer assign
		temp_2 = p2.get();

		// check) 
		EXPECT_TRUE(temp_2.get<foo>() == p2);

		// 3) r-value move
		temp_3 = std::move(p1);

		// check) 
		EXPECT_TRUE(temp_3.get<foo>() == p3);
		EXPECT_TRUE(p1.empty());

		// 3) 
		temp_4 = temp_1;

		// check) 
		EXPECT_TRUE(temp_4.get<foo>() == p3);

		// 4) move
		temp_5 = std::move(temp_1);

		// check) 
		EXPECT_TRUE(temp_5.get<foo>() == p3);
		EXPECT_TRUE(temp_1.has_value() == false);

		// 5) move
		temp_5 = temp_2;

		// check) 
		EXPECT_TRUE(temp_5.get<foo>() == p2);
		EXPECT_TRUE(temp_1.has_value() == false);

		// 5) move
		temp_4 = std::move(temp_5);

		// check) 
		EXPECT_TRUE(temp_4.get<foo>() == p2);
		EXPECT_TRUE(temp_5.has_value() == false);
	}

	TEST(CGDK10_ptr, ptr_any_ptr_assign_2)
	{
		object_ptr<foo> p1 = make_object<foo>();
		object_ptr<foo> p2 = make_object<foo>();

		any_ptr temp_1;
		any_ptr temp_2;

		// assign test.A
		temp_1 = p1;

		// swap test
		temp_2.swap(temp_1);
	}

	TEST(CGDK10_ptr, ptr_any_ptr_swap)
	{
		object_ptr<foo> p1 = make_object<foo>();
		object_ptr<foo> p2 = make_object<foo>();

		any_ptr temp_1;
		any_ptr temp_2;
	}
}