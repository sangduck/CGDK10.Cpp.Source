#include "pch.h"

#include <vector>
#include <algorithm>
#include <unordered_set>

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


	//-----------------------------------------------------------------------------
	// regression tests of the object_ptr / own_ptr / any_ptr rewrite (2026-10-19)
	//  - template move constructor/assignment (object_ptr<base> = make_object<derived>())
	//  - self move, reset(object_ptr&&), move(), detach()/adopt_ref
	//  - static/const/dynamic_pointer_cast (lvalue : add_ref, rvalue : moved)
	//  - comparison (==, !=, <, <=, >, >=, <=>) between object_ptr/own_ptr/raw pointer/nullptr
	//  - swap, std::vector<own_ptr>
	//  - own_ptr exchange_release()/force_casting_move()/force_assign(nullptr)/destroy()
	//  - any_ptr reset(object_ptr&&), from own_ptr
	//-----------------------------------------------------------------------------
	static int final_release_count = 0;

	TEST(CGDK10_ptr, ptr_object_ptr_template_move)
	{
		// reset)
		final_release_count = 0;

		// definition) counts on_final_release()
		class gal : public fal
		{
		public:
			virtual	void on_final_release() noexcept override
			{
				++final_release_count;
			}
		};

		{
			// 1) template move constructor : the source must be emptied
			object_ptr<gal> p1 = make_object<gal>();
			gal* raw_1 = p1.get();
			object_ptr<foo> p2 = std::move(p1);

			// check)
			EXPECT_TRUE(p1.empty());
			EXPECT_TRUE(p2.get() == raw_1);
			EXPECT_TRUE(p2->reference_count() == 1);

			// 2) object_ptr<base> = make_object<derived>() (the former double release)
			object_ptr<foo> p3 = make_object<gal>();

			// check)
			EXPECT_TRUE(p3->reference_count() == 1);
			EXPECT_TRUE(dynamic_cast<gal*>(p3.get()) != nullptr);

			// 3) template move assign : the old object is released
			p3 = make_object<gal>();

			// check)
			EXPECT_TRUE(final_release_count == 1);
			EXPECT_TRUE(p3->reference_count() == 1);

			// 4) template copy constructor
			object_ptr<foo> p4 = p3;

			// check)
			EXPECT_TRUE(p3->reference_count() == 2);

			// 5) copy assign (a different object)
			p4 = p2;

			// check)
			EXPECT_TRUE(p3->reference_count() == 1);
			EXPECT_TRUE(p2->reference_count() == 2);
		}

		// check) the three objects are released exactly once
		EXPECT_TRUE(final_release_count == 3);
	}

	TEST(CGDK10_ptr, ptr_object_ptr_self_move_and_reset)
	{
		// 1)
		object_ptr<fal> p1 = make_object<fal>();
		object_ptr<foo> p2 = p1;

		// check)
		EXPECT_TRUE(p1->reference_count() == 2);

		// 2) self move assign
		object_ptr<foo>& p2_ref = p2;
		p2 = std::move(p2_ref);

		// check)
		EXPECT_TRUE(p2.get() == p1.get());
		EXPECT_TRUE(p1->reference_count() == 2);

		// 3) template copy assign of the same object
		p2 = p1;

		// check)
		EXPECT_TRUE(p1->reference_count() == 2);

		// 4) reset(const object_ptr<T>&) of the same object
		p2.reset(p1);

		// check)
		EXPECT_TRUE(p1->reference_count() == 2);

		// 5) reset(object_ptr<T>&&) : adopted, the old one is released
		p2.reset(std::move(p1));

		// check)
		EXPECT_TRUE(p1.empty());
		EXPECT_TRUE(p2->reference_count() == 1);

		// 6) move() from an empty one
		p2.move(p1);

		// check)
		EXPECT_TRUE(p2.empty());

		// 7) move(object_ptr<T>&&)
		object_ptr<fal> p3 = make_object<fal>();
		p2.move(std::move(p3));

		// check)
		EXPECT_TRUE(p3.empty());
		EXPECT_TRUE(p2->reference_count() == 1);

		// 8) detach() : the reference is handed over
		fal* raw_detached = static_cast<fal*>(p2.detach());

		// check)
		EXPECT_TRUE(p2.empty());
		EXPECT_TRUE(raw_detached->reference_count() == 1);

		// 9) adopt_ref : no add_ref()
		object_ptr<fal> p4(raw_detached, adopt_ref);

		// check)
		EXPECT_TRUE(p4->reference_count() == 1);
	}

	TEST(CGDK10_ptr, ptr_object_ptr_cast)
	{
		// reset)
		final_release_count = 0;

		// definition) counts on_final_release()
		class gal : public fal
		{
		public:
			virtual	void on_final_release() noexcept override
			{
				++final_release_count;
			}
		};

		{
			// 1) static_pointer_cast (lvalue : add_ref)
			object_ptr<foo> p1 = make_object<gal>();
			object_ptr<fal> p2 = static_pointer_cast<fal>(p1);

			// check)
			EXPECT_TRUE(p1->reference_count() == 2);

			// 2) static_pointer_cast (rvalue : moved)
			object_ptr<fal> p3 = static_pointer_cast<fal>(std::move(p1));

			// check)
			EXPECT_TRUE(p1.empty());
			EXPECT_TRUE(p3->reference_count() == 2);

			// 3) dynamic_pointer_cast (lvalue, fails : empty)
			object_ptr<foo> p4 = p3;
			object_ptr<koo> p5 = dynamic_pointer_cast<koo>(p4);

			// check)
			EXPECT_TRUE(p5.empty());
			EXPECT_TRUE(p4->reference_count() == 3);

			// 4) dynamic_pointer_cast (rvalue, fails : the source keeps its reference)
			object_ptr<koo> p6 = dynamic_pointer_cast<koo>(std::move(p4));

			// check)
			EXPECT_TRUE(p6.empty());
			EXPECT_TRUE(p4.exist());
			EXPECT_TRUE(p4->reference_count() == 3);

			// 5) dynamic_pointer_cast (rvalue, succeeds : moved)
			object_ptr<fal> p7 = dynamic_pointer_cast<fal>(std::move(p4));

			// check)
			EXPECT_TRUE(p4.empty());
			EXPECT_TRUE(p7->reference_count() == 3);

			// 6) const_pointer_cast (lvalue : add_ref)
			object_ptr<const foo> p8 = p7;

			// check)
			EXPECT_TRUE(p8->reference_count() == 4);

			object_ptr<foo> p9 = const_pointer_cast<foo>(p8);

			// check)
			EXPECT_TRUE(p9->reference_count() == 5);

			// 7) const_pointer_cast (rvalue : moved)
			object_ptr<foo> p10 = const_pointer_cast<foo>(std::move(p8));

			// check)
			EXPECT_TRUE(p8.empty());
			EXPECT_TRUE(p10->reference_count() == 5);
		}

		// check)
		EXPECT_TRUE(final_release_count == 1);

		// 8) casts from own_ptr (lvalue only : add_ref)
		own_ptr<fal> p11 = make_own<fal>();
		object_ptr<Ireferenceable> p12 = static_pointer_cast<Ireferenceable>(p11);

		// check)
		EXPECT_TRUE(p11->reference_count() == 2);

		object_ptr<fal> p13 = dynamic_pointer_cast<fal>(p12);

		// check)
		EXPECT_TRUE(p13.get() == p11.get());
		EXPECT_TRUE(p11->reference_count() == 3);
	}

	TEST(CGDK10_ptr, ptr_compare)
	{
		// 1) object_ptr <-> object_ptr (different types) / raw pointer / nullptr
		object_ptr<fal> p1 = make_object<fal>();
		object_ptr<foo> p2 = p1;
		object_ptr<foo> p3;
		foo* raw_foo = p1.get();
		fal* raw_fal = p1.get();

		// check) ==, !=
		EXPECT_TRUE(p1 == p2);
		EXPECT_TRUE(p2 == p1);
		EXPECT_TRUE(!(p1 != p2));
		EXPECT_TRUE(!(p2 != p1));
		EXPECT_TRUE(p1 == raw_foo);
		EXPECT_TRUE(raw_foo == p1);
		EXPECT_TRUE(p2 == raw_fal);
		EXPECT_TRUE(raw_fal == p2);
		EXPECT_TRUE(!(p1 != raw_foo));
		EXPECT_TRUE(!(raw_foo != p1));
		EXPECT_TRUE(p3 == nullptr);
		EXPECT_TRUE(nullptr == p3);
		EXPECT_TRUE(p1 != nullptr);
		EXPECT_TRUE(nullptr != p1);

		// check) <, <=, >, >=, <=>
		EXPECT_TRUE(!(p1 < p2));
		EXPECT_TRUE(!(p2 < p1));
		EXPECT_TRUE(p1 <= p2);
		EXPECT_TRUE(p2 >= p1);
		EXPECT_TRUE(!(p1 > p2));
		EXPECT_TRUE((p1 <=> p2) == 0);
		EXPECT_TRUE(p1 <= raw_foo);
		EXPECT_TRUE(raw_foo >= p1);
		EXPECT_TRUE(!(p1 > raw_foo));
		EXPECT_TRUE(!(raw_fal < p2));
		EXPECT_TRUE(p3 != p1);
		EXPECT_TRUE(p3 < p1);
		EXPECT_TRUE(p1 > p3);

		// 2) own_ptr <-> own_ptr / object_ptr / raw pointer / nullptr
		own_ptr<fal> p4 = make_own<fal>();
		own_ptr<fal> p5;
		object_ptr<fal> p6 = p4;
		fal* raw_own = p4.get();

		// check)
		EXPECT_TRUE(p4 == p6);
		EXPECT_TRUE(p6 == p4);
		EXPECT_TRUE(!(p4 != p6));
		EXPECT_TRUE(!(p6 != p4));
		EXPECT_TRUE(p4 == raw_own);
		EXPECT_TRUE(raw_own == p4);
		EXPECT_TRUE(p4 != nullptr);
		EXPECT_TRUE(nullptr != p4);
		EXPECT_TRUE(p5 == nullptr);
		EXPECT_TRUE(p4 != p5);
		EXPECT_TRUE(!(p4 < p4));
		EXPECT_TRUE(p4 <= p6);
		EXPECT_TRUE(p6 >= p4);
		EXPECT_TRUE(!(p4 > p6));
		EXPECT_TRUE((p4 <=> p6) == 0);
		EXPECT_TRUE((p6 <=> p4) == 0);
		EXPECT_TRUE((p4 <=> p5) != 0);
		EXPECT_TRUE(p5 < p4);
		EXPECT_TRUE(p4 > p5);
		EXPECT_TRUE(p5 <= p4);
		EXPECT_TRUE(p4 >= raw_own);
		EXPECT_TRUE(raw_own <= p4);

		// 3) std::sort / std::unordered_set / hash
		std::vector<object_ptr<foo>> list;
		list.push_back(p2);
		list.push_back(p3);
		list.push_back(p1);
		std::sort(list.begin(), list.end());

		// check)
		EXPECT_TRUE(list[0] == nullptr);
		EXPECT_TRUE(list[1] == list[2]);

		std::unordered_set<object_ptr<foo>> set;
		set.insert(p2);
		set.insert(p1);

		// check)
		EXPECT_TRUE(set.size() == 1);
		EXPECT_TRUE(hash_object_ptr<foo>()(p2) == std::hash<object_ptr<foo>>()(p2));
		EXPECT_TRUE(hash_own_ptr<fal>()(p4) == std::hash<own_ptr<fal>>()(p4));
	}

	TEST(CGDK10_ptr, ptr_swap)
	{
		// definition
		class cal : virtual public Ireferenceable, public object::Idestroyable
		{
		public:
			virtual	result_code destroy() noexcept override
			{
				++destroy_count;
				return result_code(eRESULT::SUCCESS);
			}

			int destroy_count = 0;
		};

		// 1) object_ptr : swap (ADL) / std::swap / swap() / swap_object()
		object_ptr<foo> p1 = make_object<foo>();
		object_ptr<foo> p2 = make_object<fal>();
		foo* raw_1 = p1.get();
		foo* raw_2 = p2.get();

		using std::swap;
		swap(p1, p2);

		// check)
		EXPECT_TRUE(p1.get() == raw_2 && p2.get() == raw_1);

		std::swap(p1, p2);

		// check)
		EXPECT_TRUE(p1.get() == raw_1 && p2.get() == raw_2);

		p1.swap(p2);

		// check)
		EXPECT_TRUE(p1.get() == raw_2);

		swap_object(p1, p2);

		// check)
		EXPECT_TRUE(p1.get() == raw_1);
		EXPECT_TRUE(raw_1->reference_count() == 1 && raw_2->reference_count() == 1);

		// 2) own_ptr : swap does not destroy
		own_ptr<cal> p3 = make_own<cal>();
		own_ptr<cal> p4 = make_own<cal>();
		cal* raw_3 = p3.get();
		cal* raw_4 = p4.get();

		swap(p3, p4);

		// check)
		EXPECT_TRUE(p3.get() == raw_4 && p4.get() == raw_3);

		std::swap(p3, p4);

		// check)
		EXPECT_TRUE(p3.get() == raw_3 && p4.get() == raw_4);
		EXPECT_TRUE(raw_3->destroy_count == 0 && raw_4->destroy_count == 0);
		EXPECT_TRUE(raw_3->reference_count() == 1 && raw_4->reference_count() == 1);

		// 3) std::vector<own_ptr> : erase() / clear()
		std::vector<own_ptr<cal>> list;
		list.push_back(make_own<cal>());
		list.push_back(make_own<cal>());
		list.push_back(make_own<cal>());
		cal* raw_middle = list[1].get();
		object_ptr<cal> keep_middle = list[1];

		list.erase(list.begin());

		// check)
		EXPECT_TRUE(list.size() == 2 && list[0].get() == raw_middle);
		EXPECT_TRUE(raw_middle->destroy_count == 0);

		list.clear();

		// check)
		EXPECT_TRUE(raw_middle->destroy_count == 1);
		EXPECT_TRUE(keep_middle->reference_count() == 1);
	}

	TEST(CGDK10_ptr, ptr_own_ptr_move_and_assign)
	{
		// definition
		class cal : virtual public Ireferenceable, public object::Idestroyable
		{
		public:
			virtual	result_code destroy() noexcept override
			{
				++destroy_count;
				return result_code(eRESULT::SUCCESS);
			}

			int destroy_count = 0;
		};

		class dal : public cal
		{
		};

		// 1) exchange_release() : the previous owner is handed over (not re-counted)
		own_ptr<cal> p1 = make_own<cal>();
		cal* raw_1 = p1.get();
		object_ptr<cal> keep_1 = p1;

		// check)
		EXPECT_TRUE(p1.get_acquire() == raw_1);

		own_ptr<cal> p2 = p1.exchange_release(make_own<cal>());

		// check)
		EXPECT_TRUE(p2.get() == raw_1);
		EXPECT_TRUE(p1.get() != raw_1);
		EXPECT_TRUE(raw_1->reference_count() == 2);
		EXPECT_TRUE(p1->reference_count() == 1);

		p2.reset();

		// check)
		EXPECT_TRUE(raw_1->destroy_count == 1);
		EXPECT_TRUE(raw_1->reference_count() == 1);

		// 2) force_casting_move() : the reference is moved
		own_ptr<dal> p3 = make_own<dal>();
		dal* raw_3 = p3.get();
		own_ptr<cal> p4 = p3.force_casting_move<cal>();

		// check)
		EXPECT_TRUE(p3.empty());
		EXPECT_TRUE(p4.get() == raw_3);
		EXPECT_TRUE(p4->reference_count() == 1);

		// 3) template move constructor
		own_ptr<dal> p5 = make_own<dal>();
		dal* raw_5 = p5.get();
		own_ptr<cal> p6 = std::move(p5);

		// check)
		EXPECT_TRUE(p5.empty());
		EXPECT_TRUE(p6.get() == raw_5);
		EXPECT_TRUE(p6->reference_count() == 1);

		// 4) template move assign : the old object is destroyed and released
		object_ptr<cal> keep_5 = p6;
		own_ptr<dal> p7 = make_own<dal>();
		dal* raw_7 = p7.get();
		p6 = std::move(p7);

		// check)
		EXPECT_TRUE(p7.empty());
		EXPECT_TRUE(p6.get() == raw_7);
		EXPECT_TRUE(raw_5->destroy_count == 1);
		EXPECT_TRUE(raw_5->reference_count() == 1);

		// 5) self move : nothing happens
		own_ptr<cal>& p6_ref = p6;
		p6 = std::move(p6_ref);

		// check)
		EXPECT_TRUE(p6.get() == raw_7);
		EXPECT_TRUE(raw_7->destroy_count == 0);

		// 6) object_ptr = own_ptr<T>&& : the reference is copied, then the owned object is destroyed
		object_ptr<cal> p8;
		own_ptr<dal> p9 = make_own<dal>();
		dal* raw_9 = p9.get();
		p8 = std::move(p9);

		// check)
		EXPECT_TRUE(p9.empty());
		EXPECT_TRUE(p8.get() == raw_9);
		EXPECT_TRUE(raw_9->destroy_count == 1);
		EXPECT_TRUE(raw_9->reference_count() == 1);

		// 7) force_assign() / force_assign(nullptr) (the former nullptr dereference)
		own_ptr<cal> p10;
		p10.force_assign(raw_9);

		// check)
		EXPECT_TRUE(p10.get() == raw_9);
		EXPECT_TRUE(raw_9->reference_count() == 2);

		p10.force_assign(static_cast<cal*>(nullptr));

		// check)
		EXPECT_TRUE(p10.empty());
		EXPECT_TRUE(raw_9->destroy_count == 2);
		EXPECT_TRUE(raw_9->reference_count() == 1);

		// 8) destroy() then = nullptr
		own_ptr<cal> p11 = make_own<cal>();
		cal* raw_11 = p11.get();
		object_ptr<cal> keep_11 = p11;

		p11.destroy();

		// check)
		EXPECT_TRUE(raw_11->destroy_count == 1);
		EXPECT_TRUE(p11.get() == raw_11);

		p11 = nullptr;

		// check)
		EXPECT_TRUE(p11.empty());
		EXPECT_TRUE(raw_11->destroy_count == 2);
		EXPECT_TRUE(raw_11->reference_count() == 1);

		// 9) get_object_ptr()
		object_ptr<cal> p12 = p6.get_object_ptr();

		// check)
		EXPECT_TRUE(p12.get() == raw_7);
		EXPECT_TRUE(raw_7->reference_count() == 2);

		object_ptr<Ireferenceable> p13 = p6.get_object_ptr<Ireferenceable>();

		// check)
		EXPECT_TRUE(p13.get() == static_cast<Ireferenceable*>(raw_7));
		EXPECT_TRUE(raw_7->reference_count() == 3);
	}

	TEST(CGDK10_ptr, ptr_any_ptr_move_and_own_ptr)
	{
		// 1) reset(object_ptr<T>&&) : moved
		object_ptr<foo> p1 = make_object<foo>();
		foo* raw_1 = p1.get();
		any_ptr temp_1;

		temp_1.reset(std::move(p1));

		// check)
		EXPECT_TRUE(p1.empty());
		EXPECT_TRUE(temp_1.get<foo>().get() == raw_1);
		EXPECT_TRUE(raw_1->reference_count() == 1);

		// 2) reset(object_ptr<T>&&) of a template move constructed one (object_ptr<base> from make_object<derived>)
		temp_1.reset(object_ptr<foo>(make_object<fal>()));

		// check)
		EXPECT_TRUE(temp_1.get<foo>()->reference_count() == 1);
		EXPECT_TRUE(dynamic_cast<fal*>(temp_1.get<foo>().get()) != nullptr);

		// 3) copy construct / move construct
		any_ptr temp_2(temp_1);

		// check)
		EXPECT_TRUE(temp_1.get<foo>()->reference_count() == 2);

		any_ptr temp_3(std::move(temp_1));

		// check)
		EXPECT_TRUE(temp_1.has_value() == false);
		EXPECT_TRUE(temp_3.get<foo>()->reference_count() == 2);

		// 4) from own_ptr : construct / assign / conversion (add_ref only)
		own_ptr<fal> p2 = make_own<fal>();
		any_ptr temp_4(p2);

		// check)
		EXPECT_TRUE(p2->reference_count() == 2);

		temp_4 = p2;

		// check)
		EXPECT_TRUE(p2->reference_count() == 2);

		object_ptr<fal> p3 = temp_4;

		// check)
		EXPECT_TRUE(p3.get() == p2.get());
		EXPECT_TRUE(p2->reference_count() == 3);
	}
}
