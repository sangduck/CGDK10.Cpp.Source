#include "pch.h"


namespace CGDK
{
	static const int BENCH_COUNT   = 10'000;
	static const int ALLOC_COUNT_1 =    250;
	static const int ALLOC_COUNT_2 =  1'000;
	static const int ALLOC_COUNT_3 =  4'000;

	template <int IALLOC_COUNT, int IALLOC_SIZE, int ITHREAD>
	void test_tcmalloc()
	{
		std::vector<std::shared_ptr<std::thread>> vector_thread;

		// definition) 
		auto f_test = []() {
			void* array_temp[IALLOC_COUNT];

			for (auto j = 0; j < BENCH_COUNT; ++j)
			{
				for (auto i = 0; i < IALLOC_COUNT; ++i)
				{
					array_temp[i] = tc_malloc(IALLOC_COUNT);
				}

				for (auto i = 0; i < IALLOC_COUNT; ++i)
				{
					tc_free(array_temp[i]);
				}
			}
		};

		// 1) start thread
		vector_thread.reserve(ITHREAD);
		for (int i = 0; i < ITHREAD; ++i)
		{
			vector_thread.emplace_back(std::make_shared<std::thread>(f_test));
		}

		// 2) wait complete
		for (auto& iter : vector_thread)
		{
			iter->join();
		}
	}


	//-----------------------------------------------------------------------------
	//
	// test memory alloc - 250times 
	//
	//-----------------------------------------------------------------------------
	TEST(CGDK10_ptr, pool_bench_alloc__250t__064b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__064b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__064b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__064b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64, 64>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc__250t__256b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__256b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__256b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__256b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256, 64>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc__250t___1Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___1Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___1Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___1Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 1024, 64>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc__250t___4Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 4 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___4Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 4 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___4Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 4 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t___4Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 4 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc__250t__16Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 16 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__16Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 16 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__16Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 16 * 1024, 16>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc__250t__16Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 16 * 1024, 64>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__64Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64 * 1024, 1>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc__250t__64Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__64Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t__64Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 64 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc__250t_256Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t_256Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t_256Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc__250t_256Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_1, 256 * 1024, 64>();
	}


	//-----------------------------------------------------------------------------
	//
	// test memory alloc - 1000times 
	//
	//-----------------------------------------------------------------------------
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__064b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__064b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__064b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64, 16>();
	}

	TEST(CGDK10_ptr, pool_bench_alloc_1000t__064b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64, 64>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__256b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__256b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__256b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__256b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_1000t___1Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___1Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___1Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___1Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_1000t___4Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 4 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___4Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 4 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___4Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 4 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t___4Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 4 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_1000t__16Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 16 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__16Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 16 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__16Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 16 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__16Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 16 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_1000t__64Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__64Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__64Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t__64Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 64 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_1000t_256Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t_256Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t_256Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_1000t_256Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_2, 256 * 1024, 64>();
	}


	//-----------------------------------------------------------------------------
	//
	// test memory alloc - 4000times 
	//
	//-----------------------------------------------------------------------------
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__064b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__064b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__064b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__064b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_4000t__256b_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 256, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__256b_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 256, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__256b_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 256, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__256b_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 256, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_4000t___1Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___1Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___1Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___1Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_4000t___4Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 4 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___4Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 4 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___4Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 4 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t___4Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 4 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_4000t__16Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 16 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__16Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 16 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__16Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 16 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__16Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 16 * 1024, 64>();
	}


	TEST(CGDK10_ptr, pool_bench_alloc_4000t__64Kb_01t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64 * 1024, 1>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__64Kb_04t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64 * 1024, 4>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__64Kb_16t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64 * 1024, 16>();
	}
	TEST(CGDK10_ptr, pool_bench_alloc_4000t__64Kb_64t_tcmalloc)
	{
		test_tcmalloc<ALLOC_COUNT_3, 64 * 1024, 64>();
	}
}