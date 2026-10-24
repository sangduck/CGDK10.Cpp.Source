#include "pch.h"

namespace CGDK
{
	static const int TEST_COUNT = 1'000'000;

	class test_no_lock_counter
	{
	public:
		auto read() const
		{
			return m_value;
		}
		void write()
		{
			++m_value;
		}

	private:
		uint64_t m_value = 1;
	};

	class test_atomic_counter
	{
	public:
		auto read() const
		{
			return m_value.load();
		}
		void write()
		{
			++m_value;
		}

	private:
		std::atomic<uint64_t> m_value = 1;
	};

	class test_std_mutex_counter
	{
	public:
		auto read() const
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_value;
		}
		void write()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			++m_value;
		}

	private:
		mutable std::mutex m_mutex;
		uint64_t m_value = 1;
	};

	class test_std_recursive_mutex_counter
	{
	public:
		auto read() const
		{
			std::unique_lock<std::recursive_mutex> lock(m_mutex);
			return m_value;
		}
		void write()
		{
			std::unique_lock<std::recursive_mutex> lock(m_mutex);
			++m_value;
		}

	private:
		mutable std::recursive_mutex m_mutex;
		uint64_t m_value = 1;
	};

	class test_std_shared_mutex_counter
	{
	public:
		auto read() const
		{
			std::shared_lock<std::shared_mutex> lock(m_mutex);
			return m_value;
		}
		void write()
		{
			std::unique_lock<std::shared_mutex> lock(m_mutex);
			++m_value;
		}

	private:
		mutable std::shared_mutex m_mutex;
		uint64_t m_value = 1;
	};

	template <class T>
	void test_lock_1write(int _write_thread_count, int _read_thread_count)
	{
		T test_object;

		std::vector<std::shared_ptr<std::thread>> vector_thread_write;
		std::vector<std::shared_ptr<std::thread>> vector_thread_read;

		// 1) write test
		if (_write_thread_count != 0)
		{
			auto f_write = [&test_object]() {
				for (int i = 0; i < TEST_COUNT; ++i)
				{
					test_object.write();
				}
				};

			vector_thread_write.reserve(_write_thread_count);

			for (int i = 0; i < _write_thread_count; ++i)
			{
				vector_thread_write.emplace_back(std::make_shared<std::thread>(f_write));
			}
		}

		// declare)
		uint64_t temp_result = 0;

		// 2) read test
		if (_read_thread_count != 0)
		{
			auto f_read = [&test_object, &temp_result]() {
				for (int i = 0; i < TEST_COUNT; ++i)
				{
					temp_result = test_object.read();
				}
				};

			vector_thread_read.reserve(_read_thread_count);

			for (int i = 0; i < _read_thread_count; ++i)
			{
				vector_thread_read.emplace_back(std::make_shared<std::thread>(f_read));
			}
		}

		// 3) wait complete
		for (auto& iter : vector_thread_write)
		{
			iter->join();
		}

		for (auto& iter : vector_thread_read)
		{
			iter->join();
		}

		// check) temp_result가 최적화로 인해 제거되는 것을 피하기 위해...
		EXPECT_TRUE((_read_thread_count == 0) || (temp_result != 0));
	}


	//-----------------------------------------------------------------------------
	//
	// 1. lock test - write
	//
	//-----------------------------------------------------------------------------
	// 1) write-4threads
	TEST(CGDK10_lock, lock_bench_w04_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04_atomic)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 2) write-8threads
	TEST(CGDK10_lock, lock_bench_w08_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08_atomic)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 3) write-16threads
	TEST(CGDK10_lock, lock_bench_w16_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16_atomic)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 4) write-32threads
	TEST(CGDK10_lock, lock_bench_w32_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32_atomic)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 5) write-64threads
	TEST(CGDK10_lock, lock_bench_w64_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64_atomic)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 0;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}


	//-----------------------------------------------------------------------------
	//
	// 2. lock test - read
	//
	//-----------------------------------------------------------------------------
	// 1) read-4threads
	TEST(CGDK10_lock, lock_bench_r04_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r04_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r04_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r04_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r04_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 2) read-8threads
	TEST(CGDK10_lock, lock_bench_r08_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r08_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r08_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r08_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r08_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 3) read-16threads
	TEST(CGDK10_lock, lock_bench_r16_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r16_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r16_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r16_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r16_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 4) read-32threads
	TEST(CGDK10_lock, lock_bench_r32_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r32_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r32_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r32_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r32_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 5) read-64threads
	TEST(CGDK10_lock, lock_bench_no_lock_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r64_atomic)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r64_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r64_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_r64_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 0;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}


	//-----------------------------------------------------------------------------
	//
	// 3. lock test - 1write n_read
	//
	//-----------------------------------------------------------------------------
	// 1) write-1thread, read-4threads
	TEST(CGDK10_lock, lock_bench_w01r04_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r04_atomic)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r04_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r04_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r04_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 2) write-1thread, read-8threads
	TEST(CGDK10_lock, lock_bench_w01r08_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r08_atomic)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r08_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r08_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r08_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 3) write-1thread, read-16threads
	TEST(CGDK10_lock, lock_bench_w01r16_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r16_atomic)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r16_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r16_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r16_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 4) write-1thread, read-32threads
	TEST(CGDK10_lock, lock_bench_w01r32_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r32_atomic)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r32_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r32_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r32_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 5) write-1thread, read-64threads
	TEST(CGDK10_lock, lock_bench_w01r64_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r64_atomic)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r64_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r64_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w01r64_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 1;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}


	//-----------------------------------------------------------------------------
	//
	// 3. lock test - n_write 1read
	//
	//-----------------------------------------------------------------------------
	// 1) write-4thread, read-1threads
	TEST(CGDK10_lock, lock_bench_w04r01_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r01_atomic)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r01_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r01_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r01_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 2) write-8thread, read-1threads
	TEST(CGDK10_lock, lock_bench_w08r01_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r01_atomic)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r01_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r01_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r01_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 3) write-16thread, read-1threads
	TEST(CGDK10_lock, lock_bench_w16r01_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r01_atomic)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r01_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r01_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r01_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 4) write-32thread, read-1threads
	TEST(CGDK10_lock, lock_bench_w32r01_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r01_atomic)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r01_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r01_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r01_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 5) write-64thread, read-1threads
	TEST(CGDK10_lock, lock_bench_w64r01_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r01_atomic)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r01_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r01_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r01_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 1;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}



	//-----------------------------------------------------------------------------
	//
	// 3. lock test - n_write n_read
	//
	//-----------------------------------------------------------------------------
	// 1) write-4thread, read-4threads
	TEST(CGDK10_lock, lock_bench_w04r04_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r04_atomic)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r04_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r04_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w04r04_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 4;
		static const int READ_THREAD_COUNT = 4;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 2) write-8thread, read-8threads
	TEST(CGDK10_lock, lock_bench_w08r08_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r08_atomic)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r08_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r08_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w08r08_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 8;
		static const int READ_THREAD_COUNT = 8;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 3) write-16thread, read-16threads
	TEST(CGDK10_lock, lock_bench_w16r16_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r16_atomic)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r16_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r16_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w16r16_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 16;
		static const int READ_THREAD_COUNT = 16;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 4) write-32thread, read-32threads
	TEST(CGDK10_lock, lock_bench_w32r32_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r32_atomic)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r32_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r32_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w32r32_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 32;
		static const int READ_THREAD_COUNT = 32;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}

	// 5) write-64thread, read-64threads
	TEST(CGDK10_lock, lock_bench_w64r64_no_lock)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_no_lock_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r64_atomic)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r64_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r64_recursive_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
	TEST(CGDK10_lock, lock_bench_w64r64_shared_mutex)
	{
		static const int WRITE_THREAD_COUNT = 64;
		static const int READ_THREAD_COUNT = 64;

		test_lock_1write<test_atomic_counter>(WRITE_THREAD_COUNT, READ_THREAD_COUNT);
	}
}