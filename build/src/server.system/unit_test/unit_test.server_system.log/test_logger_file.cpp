#include "pch.h"
#include "cgdk/sdk10/server.log.slack.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

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
			fs::path path = fs::temp_directory_path() / ("cgdk_unit_test_log_" + std::to_string(api::get_process_id()));
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

	// - lines of all files in the directory (recursive)
	static std::vector<std::string> read_lines(const fs::path& _file)
	{
		std::vector<std::string> lines;
		std::ifstream in(_file);
		for (std::string line; std::getline(in, line);)
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (!line.empty())
				lines.push_back(line);
		}
		return lines;
	}

	static std::vector<fs::path> files_in(const fs::path& _path)
	{
		std::vector<fs::path> files;
		std::error_code ec;
		if (fs::exists(_path, ec))
			for (auto& iter : fs::directory_iterator(_path))
				if (iter.is_regular_file())
					files.push_back(iter.path());
		return files;
	}

	static bool contains(const std::vector<std::string>& _lines, const std::string& _text)
	{
		for (auto& iter : _lines)
			if (iter.find(_text) != std::string::npos)
				return true;
		return false;
	}

	// - write lines one by one (each line is written in its own batch)
	static void write_lines(own_ptr<logger_file>& _plogger, int _count)
	{
		for (int i = 0; i < _count; ++i)
		{
			LOG(_plogger, log_type_t::info) << "line " << i;
			api::sleep_for(30ms);
		}
		api::sleep_for(100ms);
	}

	//-----------------------------------------------------------------------------
	// tests
	//-----------------------------------------------------------------------------
	// 1) logs are written to the file (the directories are created)
	TEST(CGDK10_logger_file, write_and_create_directories)
	{
		auto base = test_directory("write");
		auto path = base / "a" / "b";

		{
			auto plogger = make_own<logger_file>();

			logger_file::INITIALIZE_PARAMETER param;
			auto str_path = path.string();
			param.filename = "test.log";
			param.path = str_path;
			auto result = plogger->initialize(param);
			ASSERT_EQ(result, eRESULT::SUCCESS);

			LOG(plogger, log_type_t::info) << "first line";
			LOG(plogger, log_type_t::info) << "second line " << 2;
			LOG(plogger, log_type_t::info) << "third line";
			api::sleep_for(100ms);

			plogger->destroy();
		}

		ASSERT_TRUE(fs::is_directory(path));

		auto lines = read_lines(path / "test.log");
		EXPECT_EQ(lines.size(), 3u);
		EXPECT_TRUE(contains(lines, "first line"));
		EXPECT_TRUE(contains(lines, "second line 2"));
		EXPECT_TRUE(contains(lines, "third line"));

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 2) recreation by lines (INITIALIZE_PARAMETER::limit_lines) : the old file is moved to the backup path
	TEST(CGDK10_logger_file, recreation_by_lines_parameter)
	{
		auto base = test_directory("lines_param");
		auto path = base / "now";
		auto path_backup = base / "backup" / "c" / "d";

		{
			auto plogger = make_own<logger_file>();

			auto str_path = path.string();
			auto str_path_backup = path_backup.string();

			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			param.backup_path = str_path_backup;
			param.limit_lines = 5;
			ASSERT_EQ(plogger->initialize(param), eRESULT::SUCCESS);

			write_lines(plogger, 12);
			plogger->destroy();
		}

		auto lines_now = read_lines(path / "test.log");
		auto lines_backup = read_lines(path_backup / "test.log");

		// - the old file (with the first lines) is in the backup path, the new file has the last lines
		ASSERT_FALSE(lines_backup.empty());
		EXPECT_TRUE(contains(lines_backup, "line 0"));
		EXPECT_FALSE(contains(lines_backup, "line 11"));
		EXPECT_TRUE(contains(lines_now, "line 11"));
		EXPECT_FALSE(contains(lines_now, "line 0"));

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 3) recreation by lines (context "limit_lines")
	TEST(CGDK10_logger_file, recreation_by_lines_context)
	{
		auto base = test_directory("lines_context");
		auto path = base / "now";
		auto path_backup = base / "backup";

		{
			auto plogger = make_own<logger_file>();

			context setting;
			setting["filename"sv] = "test.log"sv;
			setting["path"sv] = path.string();
			setting["backup_path"sv] = path_backup.string();
			setting["limit_lines"sv] = "5"sv;
			ASSERT_EQ(plogger->initialize(setting), eRESULT::SUCCESS);

			write_lines(plogger, 12);
			plogger->destroy();
		}

		auto lines_backup = read_lines(path_backup / "test.log");
		auto lines_now = read_lines(path / "test.log");

		ASSERT_FALSE(lines_backup.empty());
		EXPECT_TRUE(contains(lines_backup, "line 0"));
		EXPECT_TRUE(contains(lines_now, "line 11"));

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 4) recreation by bytes (INITIALIZE_PARAMETER::limit_bytes)
	TEST(CGDK10_logger_file, recreation_by_bytes_parameter)
	{
		auto base = test_directory("bytes_param");
		auto path = base / "now";
		auto path_backup = base / "backup";

		{
			auto plogger = make_own<logger_file>();

			auto str_path = path.string();
			auto str_path_backup = path_backup.string();

			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			param.backup_path = str_path_backup;
			param.limit_bytes = 150;
			ASSERT_EQ(plogger->initialize(param), eRESULT::SUCCESS);

			write_lines(plogger, 12);
			plogger->destroy();
		}

		auto lines_backup = read_lines(path_backup / "test.log");
		auto lines_now = read_lines(path / "test.log");

		ASSERT_FALSE(lines_backup.empty());
		EXPECT_TRUE(contains(lines_backup, "line 0"));
		EXPECT_TRUE(contains(lines_now, "line 11"));

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 5) no recreation without limits (all lines in one file, no backup)
	TEST(CGDK10_logger_file, no_recreation_without_limit)
	{
		auto base = test_directory("no_limit");
		auto path = base / "now";
		auto path_backup = base / "backup";

		{
			auto plogger = make_own<logger_file>();

			auto str_path = path.string();
			auto str_path_backup = path_backup.string();

			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			param.backup_path = str_path_backup;
			ASSERT_EQ(plogger->initialize(param), eRESULT::SUCCESS);

			write_lines(plogger, 8);
			plogger->destroy();
		}

		EXPECT_EQ(read_lines(path / "test.log").size(), 8u);
		EXPECT_TRUE(files_in(path_backup).empty());

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 6) recreation more than once : every backup is kept (a backup with the same name gets a new name)
	TEST(CGDK10_logger_file, recreation_keeps_every_backup)
	{
		auto base = test_directory("lines_repeat");
		auto path = base / "now";
		auto path_backup = base / "backup";

		{
			auto plogger = make_own<logger_file>();

			auto str_path = path.string();
			auto str_path_backup = path_backup.string();

			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			param.backup_path = str_path_backup;
			param.limit_lines = 5;
			ASSERT_EQ(plogger->initialize(param), eRESULT::SUCCESS);

			write_lines(plogger, 20);
			plogger->destroy();
		}

		// - recreated more than once
		auto backups = files_in(path_backup);
		EXPECT_GE(backups.size(), 2u);

		// - no line is lost
		size_t count = read_lines(path / "test.log").size();
		for (auto& iter : backups)
			count += read_lines(iter).size();
		EXPECT_EQ(count, 20u);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 7) logger_hub_archive -> logger_file : every log is written when they are destroyed
	TEST(CGDK10_logger_file, hub_archive_destroy_writes_every_log)
	{
		auto base = test_directory("hub_destroy");

		{
			auto pfile = make_own<logger_file>();

			auto str_path = base.string();
			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			ASSERT_EQ(pfile->initialize(param), eRESULT::SUCCESS);

			auto phub = make_object<logger_hub_archive>();
			phub->initialize();
			phub->register_log_targetable(object_ptr<Ilogger>(pfile.get()));

			for (int i = 0; i < 10000; ++i)
				LOG(phub, log_type_t::info) << "line " << i;

			// - destroy : the hub passes the queued logs, the file logger writes them
			phub->destroy();
			pfile->destroy();

			// - the counters : every log counted
			EXPECT_EQ(phub->get_log_count(), 10000u);
			EXPECT_EQ(pfile->get_log_count(), 10000u);
		}

		EXPECT_EQ(read_lines(base / "test.log").size(), 10000u);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 8) logs from 16 threads to a logger_file (lock-free queue) : no log is lost, the order of each thread is kept
	TEST(CGDK10_logger_file, multi_thread_queue_keeps_order)
	{
		auto base = test_directory("multi_thread");
		constexpr int THREADS = 16;
		constexpr int COUNT = 2000;

		{
			auto pfile = make_own<logger_file>();

			auto str_path = base.string();
			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			ASSERT_EQ(pfile->initialize(param), eRESULT::SUCCESS);

			std::vector<std::thread> threads;
			for (int t = 0; t < THREADS; ++t)
			{
				threads.emplace_back([&pfile, t]
				{
					for (int i = 0; i < COUNT; ++i)
						LOG(pfile, log_type_t::info) << "thread " << t << " seq " << i;
				});
			}
			for (auto& iter : threads)
				iter.join();

			// - destroy : every queued log is written
			pfile->destroy();

			// - the counter (sharded by thread) : every log counted
			EXPECT_EQ(pfile->get_log_count(), size_t(THREADS) * COUNT);
			EXPECT_EQ(pfile->get_log_count(log_type_t::info), size_t(THREADS) * COUNT);
		}

		auto lines = read_lines(base / "test.log");
		ASSERT_EQ(lines.size(), size_t(THREADS) * COUNT);

		// - the order of each thread
		std::vector<int> next(THREADS, 0);
		for (auto& iter : lines)
		{
			const auto pos_thread = iter.find("thread ");
			const auto pos_seq = iter.find(" seq ");
			ASSERT_NE(pos_thread, std::string::npos);
			ASSERT_NE(pos_seq, std::string::npos);
			const int t = std::stoi(iter.substr(pos_thread + 7, pos_seq - pos_thread - 7));
			const int i = std::stoi(iter.substr(pos_seq + 5));
			ASSERT_GE(t, 0);
			ASSERT_LT(t, THREADS);
			ASSERT_EQ(i, next[t]);
			++next[t];
		}

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 9) logs from 16 threads to a logger_hub (the counter of the hub is increased on 16 threads, the children snapshot is cached)
	TEST(CGDK10_logger_file, hub_multi_thread_counts_every_log)
	{
		auto base = test_directory("hub_multi_thread");
		constexpr int THREADS = 16;
		constexpr int COUNT = 2000;

		{
			auto pfile = make_own<logger_file>();

			auto str_path = base.string();
			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			ASSERT_EQ(pfile->initialize(param), eRESULT::SUCCESS);

			auto phub = make_object<logger_hub>();
			phub->initialize();
			phub->register_log_targetable(object_ptr<Ilogger>(pfile.get()));

			std::vector<std::thread> threads;
			for (int t = 0; t < THREADS; ++t)
			{
				threads.emplace_back([&phub, t]
				{
					for (int i = 0; i < COUNT; ++i)
						LOG(phub, log_type_t::info) << "thread " << t << " seq " << i;
				});
			}
			for (auto& iter : threads)
				iter.join();

			// - the counter of the hub (sharded by thread)
			EXPECT_EQ(phub->get_log_count(), size_t(THREADS) * COUNT);
			EXPECT_EQ(phub->get_log_count(log_type_t::info), size_t(THREADS) * COUNT);
			EXPECT_EQ(phub->_log_count()[static_cast<size_t>(log_type_t::info)], size_t(THREADS) * COUNT);

			// - reset
			phub->reset_log_count();
			EXPECT_EQ(phub->get_log_count(), 0u);

			phub->destroy();
			pfile->destroy();
		}

		EXPECT_EQ(read_lines(base / "test.log").size(), size_t(THREADS) * COUNT);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 10) logger_hub : detach all by nullptr (it locked the mutex again : a dead lock)
	TEST(CGDK10_logger_file, hub_detach_all_by_null)
	{
		auto phub = make_object<logger_hub>();
		phub->initialize();

		auto plogger_1 = make_object<logger_hub>();
		auto plogger_2 = make_object<logger_hub>();
		plogger_1->initialize();
		plogger_2->initialize();
		phub->register_log_targetable(object_ptr<Ilogger>(plogger_1));
		phub->register_log_targetable(object_ptr<Ilogger>(plogger_2));

		// - detach all (returns : no dead lock)
		EXPECT_TRUE(phub->unregister_log_targetable(nullptr));

		// - no child : nothing to detach
		EXPECT_FALSE(phub->unregister_log_targetable(plogger_1.get()));

		plogger_1->destroy();
		plogger_2->destroy();
		phub->destroy();
	}

	// 11) filter : set / reset while logging (the filter is read without lock)
	class test_filter_reject_all : public Ilog_filter
	{
	public:
		virtual bool process_filtering(const LOG_RECORD* /*_plog_record*/) override { return false; }
	};

	TEST(CGDK10_logger_file, filter_set_and_reset)
	{
		auto base = test_directory("filter");

		{
			auto pfile = make_own<logger_file>();

			auto str_path = base.string();
			logger_file::INITIALIZE_PARAMETER param;
			param.filename = "test.log";
			param.path = str_path;
			ASSERT_EQ(pfile->initialize(param), eRESULT::SUCCESS);

			// - the filter of a logger_hub is checked on the thread of the log
			auto phub = make_object<logger_hub>();
			phub->initialize();
			phub->register_log_targetable(object_ptr<Ilogger>(pfile.get()));

			// - rejected by the filter
			auto pfilter = make_object<test_filter_reject_all>();
			phub->set_filter(pfilter);
			EXPECT_EQ(phub->get_filter().get(), pfilter.get());
			for (int i = 0; i < 100; ++i)
				LOG(phub, log_type_t::info) << "rejected " << i;

			// - another thread sets / resets the filter while logging (no crash)
			std::atomic<bool> stop{ false };
			std::thread changer([&]
			{
				while (!stop.load())
				{
					phub->set_filter(make_object<test_filter_reject_all>());
					phub->set_filter(nullptr);
				}
			});
			for (int i = 0; i < 2000; ++i)
				LOG(phub, log_type_t::info) << "racing " << i;
			stop = true;
			changer.join();

			// - no filter : written
			phub->set_filter(nullptr);
			EXPECT_TRUE(phub->get_filter().empty());
			for (int i = 0; i < 100; ++i)
				LOG(phub, log_type_t::info) << "accepted " << i;

			phub->destroy();
			pfile->destroy();
		}

		auto lines = read_lines(base / "test.log");
		size_t rejected = 0, accepted = 0;
		for (auto& iter : lines)
		{
			if (iter.find("rejected ") != std::string::npos) ++rejected;
			if (iter.find("accepted ") != std::string::npos) ++accepted;
		}
		EXPECT_EQ(rejected, 0u);
		EXPECT_EQ(accepted, 100u);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	//-----------------------------------------------------------------------------
	// logger_slack (offline : no request is sent)
	//-----------------------------------------------------------------------------
	// 1) invalid settings : the initialization fails
	TEST(CGDK10_logger_slack, invalid_settings_fail)
	{
		{
			auto plogger = make_own<logger_slack>();
			context setting;
			EXPECT_ANY_THROW(plogger->initialize(setting));
		}
		{
			auto plogger = make_own<logger_slack>();
			context setting;
			setting["token"sv] = "xoxb-test"sv;
			EXPECT_ANY_THROW(plogger->initialize(setting));
		}
		{
			auto plogger = make_own<logger_slack>();
			context setting;
			setting["webhook_url"sv] = "http://hooks.example.invalid/services/x"sv;
			EXPECT_ANY_THROW(plogger->initialize(setting));
		}
	}

	// 2) the logs not in "types" are not sent
	TEST(CGDK10_logger_slack, types_filter_sends_nothing)
	{
		auto plogger = make_own<logger_slack>();

		context setting;
		setting["webhook_url"sv] = "https://hooks.example.invalid/services/x"sv;
		setting["types"sv] = "fatal"sv;
		ASSERT_EQ(plogger->initialize(setting), eRESULT::SUCCESS);

		for (int i = 0; i < 100; ++i)
			LOG(plogger, log_type_t::info) << "not sent " << i;
		plogger->destroy();

		EXPECT_EQ(plogger->get_sent_count(), 0u);
		EXPECT_EQ(plogger->get_failed_count(), 0u);
	}
	// 3) ansi : "format" -> slack marks, "strip" -> removed, "raw" -> as it is ('&', '<', '>' escaped always)
	TEST(CGDK10_logger_slack, ansi_to_slack_text)
	{
		using eANSI = logger_slack::eANSI;

		EXPECT_EQ(logger_slack::to_slack_text("plain <a> & b"), "plain &lt;a&gt; &amp; b");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1mbold\x1b[0m text"), "*bold* text");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1;31m error \x1b[0m:"), " *error* :");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[3mi\x1b[23m \x1b[9ms\x1b[29m"), "_i_ ~s~");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1m\x1b[3mbi\x1b[0m"), "*_bi_*");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[38;2;1;3;9mcolor\x1b[0m"), "color");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1mline1\nline2\x1b[0m"), "*line1*\n*line2*");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1mbold\x1b[0m", eANSI::strip), "bold");
		EXPECT_EQ(logger_slack::to_slack_text("\x1b[1mb<\x1b[0m", eANSI::raw), "\x1b[1mb&lt;\x1b[0m");
	}

	// 4) logger factory : "slack" is registered by server.log.slack.h
	TEST(CGDK10_logger_slack, factory_creates_slack)
	{
		auto plogger = create_logger("slack"sv);
		ASSERT_FALSE(plogger.empty());
		EXPECT_NE(dynamic_cast<logger_slack*>(plogger.get()), nullptr);

		EXPECT_NE(dynamic_cast<logger_file*>(create_logger("file"sv).get()), nullptr);
		EXPECT_TRUE(create_logger("unknown_type"sv).empty());
	}

	//-----------------------------------------------------------------------------
	// logger factory / logger_hub "child"
	//-----------------------------------------------------------------------------
	// - a logger type of the test (registered by register_logger_creator)
	class test_logger_custom : public logger_file
	{
	};

	// 5) register_logger_creator : a new type, replaced by the same name
	TEST(CGDK10_logger_factory, register_and_replace)
	{
		EXPECT_TRUE(create_logger("test_custom"sv).empty());

		register_logger_creator("test_custom"sv, []() -> own_ptr<Nlogger> { return make_own<test_logger_custom>(); });
		{
			auto plogger = create_logger("test_custom"sv);
			ASSERT_FALSE(plogger.empty());
			EXPECT_NE(dynamic_cast<test_logger_custom*>(plogger.get()), nullptr);
		}

		// - replaced
		register_logger_creator("test_custom"sv, []() -> own_ptr<Nlogger> { return make_own<logger_console>(); });
		{
			auto plogger = create_logger("test_custom"sv);
			ASSERT_FALSE(plogger.empty());
			EXPECT_NE(dynamic_cast<logger_console*>(plogger.get()), nullptr);
		}

		// - the built-in types ("" : file)
		EXPECT_NE(dynamic_cast<logger_file*>(create_logger(""sv).get()), nullptr);
		EXPECT_NE(dynamic_cast<logger_console*>(create_logger("console"sv).get()), nullptr);

		// - invalid registrations are ignored
		register_logger_creator(""sv, []() -> own_ptr<Nlogger> { return make_own<logger_console>(); });
		register_logger_creator("test_null"sv, nullptr);
		EXPECT_TRUE(create_logger("test_null"sv).empty());
	}

	// 6) logger_hub "child" : created by create_logger, owned by the hub (unknown types are skipped)
	TEST(CGDK10_logger_factory, hub_child_from_setting)
	{
		auto base = test_directory("hub_child");
		const auto str_path = base.string();

		{
			context setting;
			setting["child"sv]["file_a"sv]["type"sv] = "file"sv;
			setting["child"sv]["file_a"sv]["filename"sv] = "a.log"sv;
			setting["child"sv]["file_a"sv]["path"sv] = str_path;
			setting["child"sv]["file_b"sv]["type"sv] = "file"sv;
			setting["child"sv]["file_b"sv]["filename"sv] = "b.log"sv;
			setting["child"sv]["file_b"sv]["path"sv] = str_path;
			setting["child"sv]["unknown"sv]["type"sv] = "no_such_type"sv;

			auto phub = make_object<logger_hub>();
			ASSERT_EQ(phub->initialize(setting), eRESULT::SUCCESS);

			for (int i = 0; i < 10; ++i)
				LOG(phub, log_type_t::info) << "child line " << i;

			// - the children are destroyed with the hub (the queued logs are written)
			phub->destroy();
		}

		const auto lines_a = read_lines(base / "a.log");
		const auto lines_b = read_lines(base / "b.log");
		EXPECT_EQ(lines_a.size(), 10u);
		EXPECT_EQ(lines_b.size(), 10u);
		EXPECT_TRUE(contains(lines_a, "child line 9"));
		EXPECT_TRUE(contains(lines_b, "child line 0"));

		std::error_code ec;
		fs::remove_all(base, ec);
	}


	//-----------------------------------------------------------------------------
	// logger_slack : sending (asynchronous)
	//
	//  - https://127.0.0.1/... : no server (the connection is refused) -> failed
	//  - the callers and destroy() never hang
	//-----------------------------------------------------------------------------
	// 7) no server : failed (not sent), destroy() returns in the limit
	TEST(CGDK10_logger_slack, send_fails_without_server)
	{
		auto plogger = make_own<logger_slack>();

		context setting;
		setting["webhook_url"sv] = "https://127.0.0.1/services/test"sv;
		setting["types"sv] = "error"sv;
		setting["show_time"sv] = "false"sv;
		setting["timeout"sv] = "3000"sv;
		ASSERT_EQ(plogger->initialize(setting), eRESULT::SUCCESS);

		// - the callers only queue (returns at once)
		const auto time_log = std::chrono::steady_clock::now();
		for (int i = 0; i < 3; ++i)
			LOG(plogger, log_type_t::error) << "error " << i;
		EXPECT_LT(std::chrono::steady_clock::now() - time_log, 1s);

		// - destroy : waits for the messages up to 3 times of the timeout
		const auto time_destroy = std::chrono::steady_clock::now();
		plogger->destroy();
		EXPECT_LT(std::chrono::steady_clock::now() - time_destroy, 12s);

		EXPECT_EQ(plogger->get_sent_count(), 0u);
		EXPECT_GE(plogger->get_failed_count() + plogger->get_dropped_count(), 1u);
	}

	// 8) "max_queue" : the messages more than it are dropped
	TEST(CGDK10_logger_slack, max_queue_drops)
	{
		auto plogger = make_own<logger_slack>();

		context setting;
		setting["webhook_url"sv] = "https://127.0.0.1/services/test"sv;
		setting["types"sv] = "error"sv;
		setting["max_message_length"sv] = "500"sv;
		setting["max_queue"sv] = "1"sv;
		setting["timeout"sv] = "3000"sv;
		ASSERT_EQ(plogger->initialize(setting), eRESULT::SUCCESS);

		// - a log of 20 lines (400 characters each) : split into about 20 messages at once
		{
			std::string text;
			for (int i = 0; i < 20; ++i)
				text += std::string(400, 'x') + "\n";

			auto plog = make_object<LOG_RECORD>();
			plog->type = log_type_t::error;
			plog->level = log_level_t::normal();
			plog->time_occurred = chrono::time::utc::now();
			plog->set_message(std::string_view(text));
			plogger->trace(std::move(plog));
		}
		plogger->destroy();

		// - 1 in flight + 1 queued : the others are dropped
		EXPECT_EQ(plogger->get_sent_count(), 0u);
		EXPECT_GE(plogger->get_dropped_count(), 15u);
		EXPECT_LE(plogger->get_failed_count(), 2u);
	}

	// 9) "ansi" / "show_time" settings : initialized (the conversion is tested by ansi_to_slack_text)
	TEST(CGDK10_logger_slack, settings)
	{
		for (std::string_view ansi : { "format"sv, "raw"sv, "strip"sv, "unknown"sv })
		{
			auto plogger = make_own<logger_slack>();

			context setting;
			setting["webhook_url"sv] = "https://127.0.0.1/services/test"sv;
			setting["ansi"sv] = ansi;
			setting["show_time"sv] = "false"sv;
			setting["max_message_length"sv] = "100"sv;		// - clamped to 500
			EXPECT_EQ(plogger->initialize(setting), eRESULT::SUCCESS) << ansi;
			plogger->destroy();
		}

		// - by INITIALIZE_PARAMETER
		auto plogger = make_own<logger_slack>();
		logger_slack::INITIALIZE_PARAMETER param;
		param.webhook_url = "https://127.0.0.1/services/test";
		param.is_show_time = false;
		param.ansi = logger_slack::eANSI::strip;
		EXPECT_EQ(plogger->initialize(param), eRESULT::SUCCESS);
		plogger->destroy();
	}

	//-----------------------------------------------------------------------------
	// built-in logger types / asynchronous loggers (system, socket, syslog)
	//-----------------------------------------------------------------------------
	// 10) create_logger : "system", "socket" ("syslog" : linux) are asynchronous
	TEST(CGDK10_logger_factory, builtin_types)
	{
		{
			auto plogger = create_logger("system"sv);
			ASSERT_FALSE(plogger.empty());
			EXPECT_NE(dynamic_cast<logger_system*>(plogger.get()), nullptr);
			EXPECT_NE(dynamic_cast<Nlogger_async*>(plogger.get()), nullptr);
		}
		{
			auto plogger = create_logger("socket"sv);
			ASSERT_FALSE(plogger.empty());
			EXPECT_NE(dynamic_cast<logger_socket*>(plogger.get()), nullptr);
			EXPECT_NE(dynamic_cast<Nlogger_async*>(plogger.get()), nullptr);
		}
	#if defined(__linux__)
		{
			auto plogger = create_logger("syslog"sv);
			ASSERT_FALSE(plogger.empty());
			EXPECT_NE(dynamic_cast<logger_syslog*>(plogger.get()), nullptr);
			EXPECT_NE(dynamic_cast<Nlogger_async*>(plogger.get()), nullptr);
		}
	#endif
	}

	// 11) logger_hub initialized again : the children of the previous setting are replaced (not written twice)
	TEST(CGDK10_logger_factory, hub_child_initialized_again)
	{
		auto base = test_directory("hub_child_again");
		const auto str_path = base.string();

		{
			context setting;
			setting["child"sv]["file_a"sv]["type"sv] = "file"sv;
			setting["child"sv]["file_a"sv]["filename"sv] = "a.log"sv;
			setting["child"sv]["file_a"sv]["path"sv] = str_path;

			auto phub = make_object<logger_hub>();
			ASSERT_EQ(phub->initialize(setting), eRESULT::SUCCESS);

			// - initialized already : refused (the children aren't added again)
			EXPECT_NE(phub->initialize(setting), eRESULT::SUCCESS);

			// - destroyed and initialized again : the children of the first setting were destroyed
			phub->destroy();
			ASSERT_EQ(phub->initialize(setting), eRESULT::SUCCESS);

			for (int i = 0; i < 5; ++i)
				LOG(phub, log_type_t::info) << "again line " << i;

			phub->destroy();
		}

		EXPECT_EQ(read_lines(base / "a.log").size(), 5u);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

	// 12) logger_system : asynchronous, destroy() waits for the queued logs
	//     (info is not written by default : the system log of the test machine isn't written)
	TEST(CGDK10_logger_system, async_not_written_types)
	{
		auto plogger = make_own<logger_system>();

		logger_system::INITIALIZE_PARAMETER param;
		param.source = "cgdk_unit_test";
		param.all_types = false;
		ASSERT_EQ(plogger->initialize(param), eRESULT::SUCCESS);

		for (int i = 0; i < 100; ++i)
			LOG(plogger, log_type_t::info) << "not written " << i;

		plogger->destroy();
		EXPECT_EQ(plogger->get_log_count(), 0u);
	}


	//-----------------------------------------------------------------------------
	// logger_socket (e2e) : the logs of a batch are sent in a message
	//-----------------------------------------------------------------------------
	namespace
	{
		// - the first port tried by the receiver (31400 + pid % 500. the port bound is used : it is moved to the next one if used)
		uint16_t logger_socket_test_port()
		{
			return static_cast<uint16_t>(31400 + api::get_process_id() % 500);
		}

		// - the probe log : sent until one is received (the logs before the connection are not sent). not counted
		constexpr std::string_view LOG_SOCKET_PROBE = "socket probe";

		struct sLOG_RECEIVED
		{
			std::atomic<int>	messages{ 0 };
			std::atomic<int>	records{ 0 };
			std::atomic<int>	probes{ 0 };
			std::mutex			lockable;
			std::vector<std::string> texts;

			void reset()
			{
				std::lock_guard lock(this->lockable);
				this->messages = 0;
				this->records = 0;
				this->probes = 0;
				this->texts.clear();
			}
		};

		sLOG_RECEIVED& log_received()
		{
			static sLOG_RECEIVED value;
			return value;
		}
	}

	// - receives eMESSAGE::SYSTEM::LOG_ : [size][LOG_][count][records]
	class test_log_receive_socket : public net::socket::tcp<>
	{
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			buffer_view buf = _msg.buf_message;
			buf.extract<uint32_t>();
			if (buf.extract<uint32_t>() != eMESSAGE::SYSTEM::LOG_)
				return eRESULT::BYPASS;

			const auto count = buf.extract<COUNT_T>();
			int count_records = 0;
			for (COUNT_T i = 0; i < count; ++i)
			{
				auto plog = buf.extract<object_ptr<LOG_RECORD>>();
				if (plog.empty())
					continue;

				auto buf_line = alloc_shared_buffer(LOG_RECORD::get_buffer_size(plog->buf_message, plog->char_encoding, eCHARACTER_SET::UTF8) + 16);
				plog->get_log_message(buf_line, eCHARACTER_SET::UTF8, sizeof(COUNT_T));

				std::string text(buf_line.data(), buf_line.size());

				// - the probe (not counted)
				if (text.starts_with(LOG_SOCKET_PROBE))
				{
					++log_received().probes;
					continue;
				}

				std::lock_guard lock(log_received().lockable);
				log_received().texts.push_back(std::move(text));
				++count_records;
			}

			// - the messages which have the logs of the test
			if (count_records != 0)
			{
				++log_received().messages;
				log_received().records += count_records;
			}
			return eRESULT::DONE;
		}
	};

	TEST(CGDK10_logger_socket, sends_batches)
	{
		// - the counts of the previous run (--gtest_repeat) are cleared
		log_received().reset();

		// 1) receiver
		//  - the port bound is used : if the port is used by another test process running at the same time, the acceptor binds
		//    the next one (reallocate_port_at_duplicate_port_error) and the logger was connected to the receiver of that process
		//  - port 0 (a free port) is refused by the acceptor
		auto pacceptor = make_own<net::acceptor<test_log_receive_socket>>();
		net::io::connective::acceptor::Ntcp_ssl::START_PARAMETER parameter;		// - no ssl
		parameter.local_endpoint = net::ip::tcp::endpoint(net::ip::address_v4::loopback(), logger_socket_test_port());
		parameter.reallocate_port_at_duplicate_port_error = true;
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);

		const auto port = pacceptor->local_endpoint().port();
		ASSERT_NE(port, 0);

		// 2) logger_socket ("host:port")
		auto plogger = make_own<logger_socket>();
		ASSERT_EQ(plogger->initialize("127.0.0.1:" + std::to_string(port)), eRESULT::SUCCESS);

		// - connected : the probes are sent until one is received (the logs before the connection are not sent.
		//   a fixed wait was not enough under load : all the logs were lost)
		{
			const auto time_end = std::chrono::steady_clock::now() + 10s;
			while (log_received().probes == 0 && std::chrono::steady_clock::now() < time_end)
			{
				LOG(plogger, log_type_t::info) << LOG_SOCKET_PROBE;
				std::this_thread::sleep_for(20ms);
			}
			ASSERT_NE(log_received().probes, 0);
		}

		// 3) logs
		constexpr int COUNT = 200;
		for (int i = 0; i < COUNT; ++i)
			LOG(plogger, log_type_t::info) << "socket log " << i;

		// 4) received
		const auto time_end = std::chrono::steady_clock::now() + 10s;
		while (log_received().records < COUNT && std::chrono::steady_clock::now() < time_end)
			std::this_thread::sleep_for(10ms);

		EXPECT_EQ(log_received().records, COUNT);
		EXPECT_LE(log_received().messages, COUNT);		// - batched (a message has several records)
		{
			std::lock_guard lock(log_received().lockable);
			ASSERT_EQ(log_received().texts.size(), static_cast<size_t>(COUNT));
			EXPECT_EQ(log_received().texts.front(), "socket log 0");
			EXPECT_EQ(log_received().texts.back(), "socket log " + std::to_string(COUNT - 1));
		}

		// 5) close
		plogger->destroy();
		pacceptor->stop();
	}
	// benchmark) 16 threads logging to a hub -> file. run under a profiler with:
	//   --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_bench_hub_16threads*
	TEST(CGDK10_logger_file, DISABLED_bench_hub_16threads)
	{
		auto base = test_directory("bench_hub16");
		constexpr int THREADS = 16;
		constexpr int COUNT = 50000;		// 800k logs total

		auto pfile = make_own<logger_file>();
		auto str_path = base.string();
		logger_file::INITIALIZE_PARAMETER param;
		param.filename = "bench.log";
		param.path = str_path;
		ASSERT_EQ(pfile->initialize(param), eRESULT::SUCCESS);

		auto phub = make_object<logger_hub>();
		phub->initialize();
		phub->register_log_targetable(object_ptr<Ilogger>(pfile.get()));

		std::atomic<int> ready{ 0 };
		std::atomic<bool> go{ false };
		std::vector<std::thread> threads;
		std::vector<double> ns_per_log(THREADS, 0.0);

		for (int t = 0; t < THREADS; ++t)
		{
			threads.emplace_back([&, t]
			{
				ready.fetch_add(1, std::memory_order_acq_rel);
				while (!go.load(std::memory_order_acquire)) {}

				const auto t0 = std::chrono::steady_clock::now();
				for (int i = 0; i < COUNT; ++i)
					LOG(phub, log_type_t::info) << "thread " << t << " seq " << i;
				const auto t1 = std::chrono::steady_clock::now();
				ns_per_log[t] = std::chrono::duration<double, std::nano>(t1 - t0).count() / COUNT;
			});
		}

		while (ready.load(std::memory_order_acquire) != THREADS) {}
		const auto wall0 = std::chrono::steady_clock::now();
		go.store(true, std::memory_order_release);
		for (auto& iter : threads)
			iter.join();
		const auto wall_produce = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wall0).count();

		phub->destroy();
		pfile->destroy();
		const auto wall_total = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wall0).count();

		double avg = 0.0; for (auto v : ns_per_log) avg += v; avg /= THREADS;
		printf("[bench] %d threads x %d logs : produce %.0f ms, +drain total %.0f ms, producer-side %.0f ns/log (avg per thread)\n",
			THREADS, COUNT, wall_produce, wall_total, avg);

		std::error_code ec;
		fs::remove_all(base, ec);
	}

}
