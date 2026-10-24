#include "pch.h"
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace CGDK
{
	namespace fs = std::filesystem;

	//-----------------------------------------------------------------------------
	// helpers
	//-----------------------------------------------------------------------------
	// - base directory of this run (under the temp directory. unique per process,
	//   so a directory left by another user does not break the test. removed when the test ends)
	static const fs::path& base_directory()
	{
		struct sBASE
		{
			fs::path path = fs::temp_directory_path() / ("cgdk_unit_test_dump_" + std::to_string(api::get_process_id()));
			~sBASE() { std::error_code ec; fs::remove_all(path, ec); }
		};
		static const sBASE base;
		return base.path;
	}

	// - empty test directory (under the base directory)
	static fs::path test_directory(const char* _name)
	{
		auto path = base_directory() / _name;
		std::error_code ec;
		fs::remove_all(path, ec);
		return path;
	}

	// - count of entries (files) whose name starts with _prefix
	static size_t count_entry(const fs::path& _path, const std::string& _prefix)
	{
		size_t count = 0;
		std::error_code ec;
		if (!fs::exists(_path, ec))
			return 0;
		for (auto& iter : fs::directory_iterator(_path))
			if (iter.path().filename().string().rfind(_prefix, 0) == 0)
				++count;
		return count;
	}

	// - restore the settings of dump_manager (global) when the test ends
	class dump_setting_restore
	{
	public:
		dump_setting_restore() :
			pdump_manager(dump_manager::instance()),
			is_enable(pdump_manager->is_exception_handling()),
			path(pdump_manager->get_dump_path()),
			count_limit(pdump_manager->get_dump_count_limit()),
			type(pdump_manager->get_dump_type())
		{
		}
		~dump_setting_restore()
		{
			pdump_manager->enable_exception_handling(is_enable);
			pdump_manager->set_dump_path(path);
			pdump_manager->set_dump_count_limit(count_limit);
			pdump_manager->set_dump_type(type);
		}

		object_ptr<dump_manager> pdump_manager;
		bool			is_enable;
		std::string		path;
		size_t			count_limit;
		eDUMP_CONTEXT	type;
	};

	static bool is_snap_dump_available()
	{
	#if defined(__linux__)
		// - snap dump of linux uses gdb's gcore
		return std::system("which gcore > /dev/null 2>&1") == 0;
	#else
		return true;
	#endif
	}

	//-----------------------------------------------------------------------------
	// tests
	//-----------------------------------------------------------------------------
	// 1) api::file::create_parent_directories
	TEST(CGDK10_file, create_parent_directories)
	{
		auto base = test_directory("mkdir");
		auto file = base / "a" / "b" / "c" / "file.txt";

		// - all missing directories are created (not the file)
		EXPECT_TRUE(api::file::create_parent_directories(file.string()));
		EXPECT_TRUE(fs::is_directory(base / "a" / "b" / "c"));
		EXPECT_FALSE(fs::exists(file));

		// - already exists
		EXPECT_TRUE(api::file::create_parent_directories(file.string()));

		// - no directory part (current directory)
		EXPECT_TRUE(api::file::create_parent_directories("file_only.txt"));

		// - a file exists where a directory is needed
		{ std::ofstream(base / "a" / "plain_file"); }
		EXPECT_FALSE(api::file::create_parent_directories((base / "a" / "plain_file" / "x.txt").string()));

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 2) dump_manager::initialize_instance(context)
	TEST(CGDK10_dump, dump_manager_initialize_with_context)
	{
		dump_setting_restore restore;
		auto pdump_manager = restore.pdump_manager;

		// - all settings
		{
			context setting;
			setting.from_json(R"({ "enable": false, "path": "dumps/unit_test", "count_limit": 7, "dump_type": "full" })");
			dump_manager::initialize_instance(setting);

			EXPECT_FALSE(pdump_manager->is_exception_handling());
			EXPECT_EQ(pdump_manager->get_dump_path(), "dumps/unit_test");
			EXPECT_EQ(pdump_manager->get_dump_count_limit(), 7u);
			EXPECT_EQ(pdump_manager->get_dump_type(), eDUMP_CONTEXT::FULL);
		}

		// - other values (case insensitive, string number) / 'path' is kept
		{
			context setting;
			setting.from_json(R"({ "enable": "enable", "count_limit": "0", "dump_type": "Detail" })");
			dump_manager::initialize_instance(setting);

			EXPECT_TRUE(pdump_manager->is_exception_handling());
			EXPECT_EQ(pdump_manager->get_dump_path(), "dumps/unit_test");
			EXPECT_EQ(pdump_manager->get_dump_count_limit(), 0u);
			EXPECT_EQ(pdump_manager->get_dump_type(), eDUMP_CONTEXT::DETAIL);
		}

		// - invalid values are ignored
		{
			context setting;
			setting.from_json(R"({ "enable": "maybe", "count_limit": "-3", "dump_type": "huge" })");
			dump_manager::initialize_instance(setting);

			EXPECT_TRUE(pdump_manager->is_exception_handling());
			EXPECT_EQ(pdump_manager->get_dump_count_limit(), 0u);
			EXPECT_EQ(pdump_manager->get_dump_type(), eDUMP_CONTEXT::DETAIL);
		}
	}

	// 3) snap_dump : the directories of the dump file are created
	TEST(CGDK10_dump, snap_dump_creates_directories)
	{
		if (!is_snap_dump_available())
			GTEST_SKIP() << "gcore is not installed";

		dump_setting_restore restore;
		auto base = test_directory("snap");

		// - file name (windows: the file / linux: '<file>.<pid>')
		auto path_named = base / "x" / "y";
		EXPECT_TRUE(snap_dump((path_named / "named_snap.dmp").string()));
		EXPECT_EQ(count_entry(path_named, "named_snap.dmp"), 1u);

		// - default file name in the dump path ('snap_...')
		auto path_default = base / "p" / "q";
		restore.pdump_manager->set_dump_path(path_default.string());
		EXPECT_TRUE(snap_dump());
		EXPECT_EQ(count_entry(path_default, "snap_"), 1u);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 4) crash on CGDK thread : dump is created and system_exception is thrown
	//    - release build only ('/EHa' on windows, '-fnon-call-exceptions' on linux)
	static volatile int* volatile g_pnull = nullptr;

	TEST(CGDK10_dump, crash_dump_and_exception)
	{
	#if !defined(NDEBUG)
		GTEST_SKIP() << "crash is converted to exception only in the release build";
	#else
		if (!is_snap_dump_available())
			GTEST_SKIP() << "gcore is not installed";

		// - the system_executor is not used : the crash handler is installed by the api::thread itself
		//   (windows : SE translator per thread, linux : crash signal handler)
		dump_setting_restore restore;
		auto base = test_directory("crash");

		restore.pdump_manager->set_dump_path(base.string());
		restore.pdump_manager->set_dump_count_limit(0);
		enable_exception_handling(true);

		// - 1: caught system_exception, 2: not crashed, 3: other exception
		std::atomic<int> result{ 0 };
		std::atomic<int> destructed{ 0 };
		struct guard { std::atomic<int>& d; ~guard() { ++d; } };

		auto pthread = make_object<api::thread>();
		pthread->begin_thread([&]
		{
			try
			{
				guard g{ destructed };
				*g_pnull = 1;
				result = 2;
			}
			catch (const throwable::system_exception&)
			{
				result = 1;
			}
			catch (...)
			{
				result = 3;
			}
		});
		pthread->wait();

		EXPECT_EQ(result.load(), 1);
		EXPECT_EQ(destructed.load(), 1);						// stack is unwound
		EXPECT_EQ(count_entry(base, "crash_"), 1u);			// crash dump

		std::error_code ec;
		fs::remove_all(base, ec);
	#endif
	}
}
