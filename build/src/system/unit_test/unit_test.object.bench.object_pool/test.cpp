#include "pch.h"

namespace CGDK
{
	static const int BENCH_COUNT = 10'000;
	static const int ALLOC_COUNT_1 = 250;
	static const int ALLOC_COUNT_2 = 1'000;
	static const int ALLOC_COUNT_3 = 4'000;

	template <int IALLOC_COUNT, int IALLOC_SIZE, int ITHREAD>
	void test_CGDK_object_pool()
	{
		class TEMP_OBJECT : public Npoolable<TEMP_OBJECT>
		{
			char temp[IALLOC_SIZE];
		};

		// declare) 
		std::vector<std::shared_ptr<std::thread>> vector_thread;

		// definition) 
		auto f_test = []() {
			TEMP_OBJECT* array_temp[IALLOC_COUNT];

			for (auto j = 0; j < BENCH_COUNT; ++j)
			{
				for (auto i = 0; i < IALLOC_COUNT; ++i)
				{
					array_temp[i] = TEMP_OBJECT::alloc();
				}

				for (auto i = 0; i < IALLOC_COUNT; ++i)
				{
					TEMP_OBJECT::dealloc(array_temp[i]);
				}
			}

			};

		// 1) start thread
		vector_thread.reserve(ITHREAD);

		for (int i = 0; i < ITHREAD; ++i)
		{
			vector_thread.push_back(std::make_shared<std::thread>(f_test));
		}

		// 2) wait complete
		for (auto& iter : vector_thread)
		{
			iter->join();
		}
	}


	//-----------------------------------------------------------------------------
	//
	// test object alloc - 250times 
	//
	//-----------------------------------------------------------------------------
	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__064b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__064b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__064b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__064b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__256b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__256b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__256b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t__256b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_001Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_001Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_001Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_001Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_004Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 4 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_004Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 4 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_004Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 4 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_004Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 4 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_016Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 16 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_016Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 16 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_016Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 16 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_016Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 16 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_064Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_064Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_064Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_064Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 64 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_256Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_256Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_256Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc__250t_256Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_1, 256 * 1024, 64>();
	}



	//-----------------------------------------------------------------------------
	//
	// test object alloc - 1000times 
	//
	//-----------------------------------------------------------------------------
	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__064b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__064b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__064b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__064b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__256b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__256b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__256b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t__256b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_001Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_001Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_001Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_001Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_004Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 4 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_004Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 4 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_004Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 4 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_004Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 4 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_016Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 16 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_016Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 16 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_016Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 16 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_016Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 16 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_064Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_064Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_064Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_064Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 64 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_256Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_256Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_256Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_1000t_256Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_2, 256 * 1024, 64>();
	}



	//-----------------------------------------------------------------------------
	//
	// test object alloc - 4000times 
	//
	//-----------------------------------------------------------------------------
	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__064b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__064b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__064b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__064b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__256b_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 256, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__256b_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 256, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__256b_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 256, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t__256b_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 256, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_001Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_001Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_001Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_001Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_004Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 4 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_004Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 4 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_004Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 4 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_004Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 4 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_016Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 16 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_016Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 16 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_016Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 16 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_016Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 16 * 1024, 64>();
	}


	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_064Kb_01t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64 * 1024, 1>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_064Kb_04t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64 * 1024, 4>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_064Kb_16t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64 * 1024, 16>();
	}

	TEST(bench_alloc_cgpool, pool_bench_alloc_4000t_064Kb_64t_CGDK_object_pool)
	{
		test_CGDK_object_pool<ALLOC_COUNT_3, 64 * 1024, 64>();
	}
}


//-----------------------------------------------------------------------------
//
// object_pool_tls - close / destroy with objects cached in tls stack
//
//-----------------------------------------------------------------------------
#include <barrier>

namespace CGDK
{
namespace test_pool_tls_close
{
	static const int THREAD_COUNT = 4;
	static const int CACHE_COUNT = 200;

	// - counts living objects (constructed - destructed)
	template <class TTAG>
	class counted_object : public Npoolable<counted_object<TTAG>>
	{
	public:
		counted_object() noexcept { s_living.fetch_add(1); }
		virtual ~counted_object() noexcept { s_living.fetch_sub(1); }

		static inline std::atomic<int64_t> s_living{ 0 };
		char data[64]{};
	};

	// - alloc and free objects in this thread (freed objects are cached in tls stack and central stack)
	template <class TOBJECT>
	void cache_objects()
	{
		std::vector<object_ptr<TOBJECT>> vector_object;
		vector_object.reserve(CACHE_COUNT);

		for (int j = 0; j < 4; ++j)
		{
			for (int i = 0; i < CACHE_COUNT; ++i)
				vector_object.push_back(make_object<TOBJECT>());
			vector_object.clear();
		}
	}

	// - create new pool (the pool of previous test run is closed)
	template <class TOBJECT>
	void create_pool()
	{
		TOBJECT::get_factory() = _make_own_raw<typename TOBJECT::factory_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA std::string_view(), eFACTORY_TYPE::DEFAULT);
	}

	// - pool which is not held by factory::manager after unregister_factory() (it is deleted when the last object is deleted)
	template <class TOBJECT>
	class test_pool : public factory::object_pool_tls<TOBJECT>
	{
	public:
		test_pool(std::string_view _name, eFACTORY_TYPE _factory_type) : factory::object_pool_tls<TOBJECT>(_name, _factory_type) {}
		virtual ~test_pool() noexcept { s_destroyed.fetch_add(1); }

		void unregister_factory() noexcept { this->process_unregister_factory(); }

		static inline std::atomic<int> s_destroyed{ 0 };
	};

	struct tag_next_free {};
	struct tag_thread_exit {};
	struct tag_new_thread {};
	struct tag_recreate {};
}

// 1) close() while objects are cached in tls stack of living threads -> emptied at the next free of each thread
TEST(object_pool_tls, close_empties_tls_stack_at_next_free)
{
	using object_t = test_pool_tls_close::counted_object<test_pool_tls_close::tag_next_free>;

	test_pool_tls_close::create_pool<object_t>();

	std::barrier sync(test_pool_tls_close::THREAD_COUNT + 1);
	std::vector<std::thread> vector_thread;

	for (int i = 0; i < test_pool_tls_close::THREAD_COUNT; ++i)
	{
		vector_thread.emplace_back([&]()
		{
			test_pool_tls_close::cache_objects<object_t>();
			auto pkeep = make_object<object_t>();

			sync.arrive_and_wait();	// (1) objects cached
			sync.arrive_and_wait();	// (2) pool closed

			// - next free of this thread (to the closed pool)
			pkeep.reset();

			sync.arrive_and_wait();	// (3) freed
			sync.arrive_and_wait();	// (4) checked
		});
	}

	sync.arrive_and_wait();	// (1)
	EXPECT_GT(object_t::s_living.load(), test_pool_tls_close::THREAD_COUNT);

	// - close
	object_t::get_factory().reset();
	sync.arrive_and_wait();	// (2)
	sync.arrive_and_wait();	// (3)

	// check) all objects are deleted while the threads are alive
	EXPECT_EQ(object_t::s_living.load(), 0);
	sync.arrive_and_wait();	// (4)

	for (auto& iter : vector_thread)
		iter.join();

	EXPECT_EQ(object_t::s_living.load(), 0);
}

// 2) close() while objects are cached in tls stack of living threads -> emptied at thread exit
TEST(object_pool_tls, close_empties_tls_stack_at_thread_exit)
{
	using object_t = test_pool_tls_close::counted_object<test_pool_tls_close::tag_thread_exit>;

	test_pool_tls_close::create_pool<object_t>();

	std::barrier sync(test_pool_tls_close::THREAD_COUNT + 1);
	std::vector<std::thread> vector_thread;

	for (int i = 0; i < test_pool_tls_close::THREAD_COUNT; ++i)
	{
		vector_thread.emplace_back([&]()
		{
			test_pool_tls_close::cache_objects<object_t>();

			sync.arrive_and_wait();	// (1) objects cached
			sync.arrive_and_wait();	// (2) pool closed
		});
	}

	sync.arrive_and_wait();	// (1)
	EXPECT_GT(object_t::s_living.load(), 0);

	// - close
	object_t::get_factory().reset();
	sync.arrive_and_wait();	// (2)

	for (auto& iter : vector_thread)
		iter.join();

	// check) objects cached in tls stack of the exited threads are deleted
	EXPECT_EQ(object_t::s_living.load(), 0);
}

// 3) the first free after close() in a new thread -> not cached in tls stack of the new thread
TEST(object_pool_tls, close_then_free_in_new_thread)
{
	using object_t = test_pool_tls_close::counted_object<test_pool_tls_close::tag_new_thread>;

	test_pool_tls_close::create_pool<object_t>();

	// - objects allocated before close()
	std::vector<object_ptr<object_t>> vector_object;
	for (int i = 0; i < test_pool_tls_close::CACHE_COUNT; ++i)
		vector_object.push_back(make_object<object_t>());

	// - close
	object_t::get_factory().reset();

	std::barrier sync(2);

	std::thread thread_free([&]()
	{
		// - free in a thread which didn't use the pool before close()
		vector_object.clear();

		sync.arrive_and_wait();	// (1) freed
		sync.arrive_and_wait();	// (2) checked
	});

	sync.arrive_and_wait();	// (1)

	// check) no object is left in tls stack of the living thread
	EXPECT_EQ(object_t::s_living.load(), 0);
	sync.arrive_and_wait();	// (2)

	thread_free.join();

	EXPECT_EQ(object_t::s_living.load(), 0);
}

// 4) destroy and recreate the pool of same T from threads that used the old pool
TEST(object_pool_tls, destroy_and_recreate_pool_of_same_type)
{
	using object_t = test_pool_tls_close::counted_object<test_pool_tls_close::tag_recreate>;
	using pool_t = test_pool_tls_close::test_pool<object_t>;

	static const int ROUND_COUNT = 8;
	const int destroyed_base = pool_t::s_destroyed.load();

	std::barrier sync(test_pool_tls_close::THREAD_COUNT + 1);
	std::vector<std::thread> vector_thread;

	for (int i = 0; i < test_pool_tls_close::THREAD_COUNT; ++i)
	{
		vector_thread.emplace_back([&]()
		{
			for (int round = 0; round < ROUND_COUNT; ++round)
			{
				sync.arrive_and_wait();	// (1) pool created

				test_pool_tls_close::cache_objects<object_t>();
				auto pkeep = make_object<object_t>();

				sync.arrive_and_wait();	// (2) objects cached
				sync.arrive_and_wait();	// (3) pool closed and released

				// - the last object deletes the pool
				pkeep.reset();

				sync.arrive_and_wait();	// (4) freed
			}
		});
	}

	for (int round = 0; round < ROUND_COUNT; ++round)
	{
		// - create pool
		object_t::get_factory() = _make_own_raw<pool_t>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA std::string_view("test_pool_tls_close"), eFACTORY_TYPE::POOL_USER);
		static_cast<pool_t*>(object_t::get_factory().get())->unregister_factory();
		sync.arrive_and_wait();	// (1)
		sync.arrive_and_wait();	// (2)

		EXPECT_GT(object_t::s_living.load(), 0);

		// - close and release
		object_t::get_factory().reset();
		sync.arrive_and_wait();	// (3)
		sync.arrive_and_wait();	// (4)

		// check) all objects and the pool are deleted
		EXPECT_EQ(object_t::s_living.load(), 0);
		EXPECT_EQ(pool_t::s_destroyed.load(), destroyed_base + round + 1);
	}

	for (auto& iter : vector_thread)
		iter.join();

	EXPECT_EQ(object_t::s_living.load(), 0);
	EXPECT_EQ(pool_t::s_destroyed.load(), destroyed_base + ROUND_COUNT);
}

}



//-----------------------------------------------------------------------------
//
// factory::object (statistics factory of make_object<T>) - tls data release / recreate
//
//-----------------------------------------------------------------------------
namespace CGDK
{
namespace test_factory_object_tls
{
	static const int THREAD_COUNT = 4;
	static const int OBJECT_COUNT = 100;

	class referenceable_object : virtual public Ireferenceable
	{
	public:
		char data[16]{};
	};

	// - Ifactory::process_unregister_factory() is protected
	struct factory_unregister : public Ifactory
	{
		static void unregister(Ifactory* _pfactory) { (_pfactory->*(&factory_unregister::process_unregister_factory))(); }
	};

	void make_objects()
	{
		std::vector<object_ptr<referenceable_object>> vector_object;
		vector_object.reserve(OBJECT_COUNT);

		for (int i = 0; i < OBJECT_COUNT; ++i)
			vector_object.push_back(make_object<referenceable_object>());
	}
}

// 1) destroy and recreate factory::object<T> from threads that used the old one
//    - statistics of the threads go to the new factory (not to tls data of the destroyed factory)
//    - tls data is released at thread exit and factory destruction (LeakSanitizer)
TEST(factory_object_tls, destroy_and_recreate_factory_of_same_type)
{
	using factory_t = factory::object<test_factory_object_tls::referenceable_object>;

	static const int ROUND_COUNT = 8;
	const auto count_expected = static_cast<uint64_t>(test_factory_object_tls::THREAD_COUNT * test_factory_object_tls::OBJECT_COUNT);

	std::barrier sync(test_factory_object_tls::THREAD_COUNT + 1);
	std::vector<std::thread> vector_thread;

	for (int i = 0; i < test_factory_object_tls::THREAD_COUNT; ++i)
	{
		vector_thread.emplace_back([&]()
		{
			for (int round = 0; round < ROUND_COUNT; ++round)
			{
				sync.arrive_and_wait();	// (1) factory created

				test_factory_object_tls::make_objects();

				sync.arrive_and_wait();	// (2) objects created and deleted
			}
		});
	}

	for (int round = 0; round < ROUND_COUNT; ++round)
	{
		// - create factory (not held by factory::manager : deleted when the last reference is released)
		auto pfactory = factory_t::instance(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
		test_factory_object_tls::factory_unregister::unregister(pfactory.get());

		sync.arrive_and_wait();	// (1)
		sync.arrive_and_wait();	// (2)

		// check) statistics of all threads are gathered by this factory
		pfactory->sustain();
		EXPECT_EQ(pfactory->statistics().now.alloc_create, count_expected);
		EXPECT_EQ(pfactory->statistics().now.free_delete, count_expected);

		// - destroy factory
		pfactory->destroy();
		pfactory.reset();
	}

	for (auto& iter : vector_thread)
		iter.join();
}

}
