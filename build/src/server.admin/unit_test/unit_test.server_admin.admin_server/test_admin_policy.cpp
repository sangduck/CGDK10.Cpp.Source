#include "pch.h"

//-----------------------------------------------------------------------------
// admin server policies (admin_server.policy.h)
//
//  - desired_state_store : json/file round trip, invalid entries
//  - restart_limiter     : interval, crash loop, window, reset
//  - request_tracker     : request id, complete, purge
//  - parse_command       : command/arguments, iid
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

//-----------------------------------------------------------------------------
// desired_state_store
//-----------------------------------------------------------------------------
TEST(admin_policy, desired_state_set_get)
{
	admin::desired_state_store store;

	EXPECT_EQ(store.get(1), admin::eDESIRED_STATE::NONE);

	store.set(1, admin::eDESIRED_STATE::RUNNING);
	store.set(2, admin::eDESIRED_STATE::STOPPED);
	EXPECT_EQ(store.get(1), admin::eDESIRED_STATE::RUNNING);
	EXPECT_EQ(store.get(2), admin::eDESIRED_STATE::STOPPED);
	EXPECT_EQ(store.size(), 2u);

	// - NONE removes
	store.set(1, admin::eDESIRED_STATE::NONE);
	EXPECT_EQ(store.get(1), admin::eDESIRED_STATE::NONE);
	EXPECT_EQ(store.size(), 1u);
}

TEST(admin_policy, desired_state_json_round_trip)
{
	admin::desired_state_store store;
	store.set(21101, admin::eDESIRED_STATE::RUNNING);
	store.set(12105, admin::eDESIRED_STATE::STOPPED);

	admin::desired_state_store read;
	read.from_json(store.to_json());

	EXPECT_EQ(read.size(), 2u);
	EXPECT_EQ(read.get(21101), admin::eDESIRED_STATE::RUNNING);
	EXPECT_EQ(read.get(12105), admin::eDESIRED_STATE::STOPPED);
}

TEST(admin_policy, desired_state_invalid_entries_ignored)
{
	admin::desired_state_store store;
	store.from_json(R"({ "1": "running", "abc": "running", "2": "unknown", "3": 5, "0": "running", "4": "stopped" })"sv);

	EXPECT_EQ(store.size(), 2u);
	EXPECT_EQ(store.get(1), admin::eDESIRED_STATE::RUNNING);
	EXPECT_EQ(store.get(4), admin::eDESIRED_STATE::STOPPED);
}

TEST(admin_policy, desired_state_file)
{
	const std::string filename = "unit_test_admin_desired_state.json";
	std::error_code error_code;
	std::filesystem::remove(filename, error_code);

	// - not exist: empty
	admin::desired_state_store store;
	store.set(7, admin::eDESIRED_STATE::RUNNING);
	store.load_file(filename);
	EXPECT_EQ(store.size(), 0u);

	// - save / load
	store.set(7, admin::eDESIRED_STATE::RUNNING);
	EXPECT_TRUE(store.save_file(filename));

	// - save again (overwrite existing file)
	store.set(8, admin::eDESIRED_STATE::STOPPED);
	EXPECT_TRUE(store.save_file(filename));

	admin::desired_state_store read;
	read.load_file(filename);
	EXPECT_EQ(read.size(), 2u);
	EXPECT_EQ(read.get(7), admin::eDESIRED_STATE::RUNNING);
	EXPECT_EQ(read.get(8), admin::eDESIRED_STATE::STOPPED);

	std::filesystem::remove(filename, error_code);
}

//-----------------------------------------------------------------------------
// restart_limiter
//-----------------------------------------------------------------------------
TEST(admin_policy, restart_interval)
{
	admin::restart_limiter limiter;
	limiter.set_policy(30s, 0, 300s);

	auto now = admin::restart_limiter::clock_t::now();

	EXPECT_EQ(limiter.request(1, now), admin::restart_limiter::eDECISION::ALLOWED);
	EXPECT_EQ(limiter.request(1, now + 10s), admin::restart_limiter::eDECISION::TOO_SOON);
	EXPECT_EQ(limiter.request(1, now + 30s), admin::restart_limiter::eDECISION::ALLOWED);

	// - other service is independent
	EXPECT_EQ(limiter.request(2, now + 10s), admin::restart_limiter::eDECISION::ALLOWED);
}

TEST(admin_policy, restart_crash_loop)
{
	admin::restart_limiter limiter;
	limiter.set_policy(10s, 3, 300s);

	auto now = admin::restart_limiter::clock_t::now();

	EXPECT_EQ(limiter.request(1, now), admin::restart_limiter::eDECISION::ALLOWED);
	EXPECT_EQ(limiter.request(1, now + 10s), admin::restart_limiter::eDECISION::ALLOWED);
	EXPECT_EQ(limiter.request(1, now + 20s), admin::restart_limiter::eDECISION::ALLOWED);

	// - 4th in the window: crash loop -> suspended
	EXPECT_EQ(limiter.request(1, now + 30s), admin::restart_limiter::eDECISION::CRASH_LOOP);
	EXPECT_TRUE(limiter.is_suspended(1));
	EXPECT_EQ(limiter.request(1, now + 1000s), admin::restart_limiter::eDECISION::SUSPENDED);

	// - reset by the operator
	limiter.reset(1);
	EXPECT_FALSE(limiter.is_suspended(1));
	EXPECT_EQ(limiter.request(1, now + 1001s), admin::restart_limiter::eDECISION::ALLOWED);
}

TEST(admin_policy, restart_window_expired)
{
	admin::restart_limiter limiter;
	limiter.set_policy(10s, 2, 60s);

	auto now = admin::restart_limiter::clock_t::now();

	EXPECT_EQ(limiter.request(1, now), admin::restart_limiter::eDECISION::ALLOWED);
	EXPECT_EQ(limiter.request(1, now + 10s), admin::restart_limiter::eDECISION::ALLOWED);

	// - the first one is out of the window (60s): allowed
	EXPECT_EQ(limiter.request(1, now + 61s), admin::restart_limiter::eDECISION::ALLOWED);
	EXPECT_FALSE(limiter.is_suspended(1));
}

TEST(admin_policy, restart_unlimited)
{
	admin::restart_limiter limiter;
	limiter.set_policy(1s, 0, 300s);

	auto now = admin::restart_limiter::clock_t::now();
	for (int i = 0; i < 100; ++i)
		EXPECT_EQ(limiter.request(1, now + std::chrono::seconds(i)), admin::restart_limiter::eDECISION::ALLOWED);
}

//-----------------------------------------------------------------------------
// request_tracker
//-----------------------------------------------------------------------------
TEST(admin_policy, request_tracker)
{
	admin::request_tracker tracker;
	auto now = admin::request_tracker::clock_t::now();

	auto id1 = tracker.add(10, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE, now);
	auto id2 = tracker.add(11, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE, now + 5s);
	EXPECT_NE(id1, 0u);
	EXPECT_NE(id1, id2);
	EXPECT_EQ(tracker.size(), 2u);

	// - complete once
	auto pending = tracker.complete(id1);
	ASSERT_TRUE(pending.has_value());
	EXPECT_EQ(pending->iid, 10u);
	EXPECT_EQ(pending->request, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);
	EXPECT_FALSE(tracker.complete(id1).has_value());
	EXPECT_FALSE(tracker.complete(0).has_value());

	// - purge
	EXPECT_TRUE(tracker.purge(60s, now + 30s).empty());
	auto expired = tracker.purge(60s, now + 65s);
	ASSERT_EQ(expired.size(), 1u);
	EXPECT_EQ(expired[0].request_id, id2);
	EXPECT_EQ(tracker.size(), 0u);
}

//-----------------------------------------------------------------------------
// parse_command
//-----------------------------------------------------------------------------
TEST(admin_policy, parse_command)
{
	auto command = admin::parse_command("  START   21101  \r\n"sv);
	EXPECT_EQ(command.command, "start");
	ASSERT_EQ(command.args.size(), 1u);
	EXPECT_EQ(command.args[0], "21101");

	auto want = admin::parse_command("want\t7 Running"sv);
	EXPECT_EQ(want.command, "want");
	ASSERT_EQ(want.args.size(), 2u);
	EXPECT_EQ(want.args[1], "Running");			// - arguments are not lowered

	EXPECT_TRUE(admin::parse_command("   "sv).command.empty());
}

TEST(admin_policy, parse_iid)
{
	EXPECT_EQ(admin::parse_iid("21101"sv), 21101u);
	EXPECT_EQ(admin::parse_iid("4206232923320050582"sv), 4206232923320050582ull);
	EXPECT_EQ(admin::parse_iid(""sv), 0u);
	EXPECT_EQ(admin::parse_iid("12a"sv), 0u);
	EXPECT_EQ(admin::parse_iid("-1"sv), 0u);
}


//-----------------------------------------------------------------------------
// request name <-> message
//-----------------------------------------------------------------------------
TEST(admin_policy, request_name)
{
	EXPECT_EQ(admin::to_request_message("start"sv), eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);
	EXPECT_EQ(admin::to_request_message("continue"sv), eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE);
	EXPECT_EQ(admin::to_request_message("START"sv), 0u);			// - lower case only
	EXPECT_EQ(admin::to_request_message("restart"sv), 0u);

	for (auto name : { "start"sv, "stop"sv, "init"sv, "destroy"sv, "kill"sv, "pause"sv, "continue"sv })
	{
		auto request = admin::to_request_message(name);
		ASSERT_NE(request, 0u);
		EXPECT_NE(admin::to_request_name(request), "UNKNOWN"sv);
	}

	EXPECT_EQ(admin::to_request_name(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE), "STOP"sv);
	EXPECT_EQ(admin::to_request_name(0), "UNKNOWN"sv);
}

//-----------------------------------------------------------------------------
// control api route
//-----------------------------------------------------------------------------
TEST(admin_policy, control_route)
{
	using admin::eCONTROL_ACTION;

	// - list / get
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances"sv).action, eCONTROL_ACTION::LIST);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances/"sv).action, eCONTROL_ACTION::LIST);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances?x=1"sv).action, eCONTROL_ACTION::LIST);

	auto get = admin::parse_control_route("GET"sv, "/api/instances/21101"sv);
	EXPECT_EQ(get.action, eCONTROL_ACTION::GET);
	EXPECT_EQ(get.iid, 21101u);

	// - request
	auto start = admin::parse_control_route("POST"sv, "/api/instances/21101/start"sv);
	EXPECT_EQ(start.action, eCONTROL_ACTION::REQUEST);
	EXPECT_EQ(start.iid, 21101u);
	EXPECT_EQ(start.argument, "start");
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances/21101/start"sv).action, eCONTROL_ACTION::METHOD_NOT_ALLOWED);
	EXPECT_EQ(admin::parse_control_route("POST"sv, "/api/instances/21101/restart"sv).action, eCONTROL_ACTION::NOT_FOUND);

	// - desired
	auto desired = admin::parse_control_route("PUT"sv, "/api/instances/7/desired/running"sv);
	EXPECT_EQ(desired.action, eCONTROL_ACTION::DESIRED);
	EXPECT_EQ(desired.iid, 7u);
	EXPECT_EQ(desired.argument, "running");
	EXPECT_EQ(admin::parse_control_route("POST"sv, "/api/instances/7/desired/running"sv).action, eCONTROL_ACTION::METHOD_NOT_ALLOWED);
	EXPECT_EQ(admin::parse_control_route("PUT"sv, "/api/instances/7/desired/paused"sv).action, eCONTROL_ACTION::NOT_FOUND);

	// - hosts
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/hosts"sv).action, eCONTROL_ACTION::HOSTS);
	EXPECT_EQ(admin::parse_control_route("POST"sv, "/api/hosts"sv).action, eCONTROL_ACTION::METHOD_NOT_ALLOWED);

	// - reload / metrics
	EXPECT_EQ(admin::parse_control_route("POST"sv, "/api/reload"sv).action, eCONTROL_ACTION::RELOAD);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/reload"sv).action, eCONTROL_ACTION::METHOD_NOT_ALLOWED);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/metrics"sv).action, eCONTROL_ACTION::METRICS);
	EXPECT_EQ(admin::parse_control_route("POST"sv, "/metrics"sv).action, eCONTROL_ACTION::METHOD_NOT_ALLOWED);

	// - not found
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/"sv).action, eCONTROL_ACTION::NOT_FOUND);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api"sv).action, eCONTROL_ACTION::NOT_FOUND);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/users"sv).action, eCONTROL_ACTION::NOT_FOUND);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances/abc"sv).action, eCONTROL_ACTION::NOT_FOUND);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances/0"sv).action, eCONTROL_ACTION::NOT_FOUND);
	EXPECT_EQ(admin::parse_control_route("GET"sv, "/api/instances/1/2/3/4"sv).action, eCONTROL_ACTION::NOT_FOUND);
}

//-----------------------------------------------------------------------------
// json / prometheus escape
//-----------------------------------------------------------------------------
TEST(admin_policy, json_string)
{
	// - (no raw string literal in macro arguments : v143 compiler bug)
	EXPECT_EQ(admin::to_json_string("abc"sv), "\"abc\"");
	EXPECT_EQ(admin::to_json_string(""sv), "\"\"");
	EXPECT_EQ(admin::to_json_string("a\"b\\c"sv), "\"a\\\"b\\\\c\"");
	EXPECT_EQ(admin::to_json_string("a\nb\tc\r"sv), "\"a\\nb\\tc\\r\"");
	EXPECT_EQ(admin::to_json_string(std::string_view("\x01", 1)), "\"\\u0001\"");
	EXPECT_EQ(admin::to_json_string(std::string_view("\x1f", 1)), "\"\\u001f\"");
	EXPECT_EQ(admin::to_json_string("\xea\xb0\x80"sv), "\"\xea\xb0\x80\"");	// - utf-8 is kept
}

TEST(admin_policy, prometheus_label)
{
	EXPECT_EQ(admin::to_prometheus_label("game_server"sv), "game_server");
	EXPECT_EQ(admin::to_prometheus_label("a\"b\\c"sv), "a\\\"b\\\\c");
	EXPECT_EQ(admin::to_prometheus_label("a\nb"sv), "a\\nb");
}


//-----------------------------------------------------------------------------
// auth fail limiter (control api)
//-----------------------------------------------------------------------------
TEST(admin_policy, auth_fail_limiter)
{
	admin::auth_fail_limiter limiter;
	limiter.set_policy(3, 60s, 300s);

	auto now = admin::auth_fail_limiter::clock_t::now();
	const std::string ip = "10.0.0.1";

	// - blocked on the 3rd failure in the window
	EXPECT_FALSE(limiter.on_fail(ip, now));
	EXPECT_FALSE(limiter.on_fail(ip, now + 1s));
	EXPECT_EQ(limiter.get_blocked(ip, now + 1s).count(), 0);
	EXPECT_TRUE(limiter.on_fail(ip, now + 2s));
	EXPECT_EQ(limiter.get_blocked(ip, now + 2s).count(), 300);
	EXPECT_EQ(limiter.get_blocked(ip, now + 100s).count(), 202);
	EXPECT_EQ(limiter.get_blocked(ip, now + 302s).count(), 0);

	// - the other address is not blocked
	EXPECT_EQ(limiter.get_blocked("10.0.0.2", now + 2s).count(), 0);

	// - failures out of the window are not counted
	const std::string ip2 = "10.0.0.3";
	EXPECT_FALSE(limiter.on_fail(ip2, now));
	EXPECT_FALSE(limiter.on_fail(ip2, now + 1s));
	EXPECT_FALSE(limiter.on_fail(ip2, now + 70s));

	// - success clears the failures
	limiter.on_success(ip2);
	EXPECT_FALSE(limiter.on_fail(ip2, now + 71s));
	EXPECT_FALSE(limiter.on_fail(ip2, now + 72s));

	// - 0 : no limit
	admin::auth_fail_limiter unlimited;
	unlimited.set_policy(0, 60s, 300s);
	for (int i = 0; i < 10; ++i)
		EXPECT_FALSE(unlimited.on_fail(ip, now));
	EXPECT_EQ(unlimited.get_blocked(ip, now).count(), 0);
}

//-----------------------------------------------------------------------------
// web ui Host / Origin (host_agent)
//-----------------------------------------------------------------------------
TEST(admin_policy, web_host)
{
	using namespace server_system;

	EXPECT_TRUE(is_ip_literal_or_localhost("127.0.0.1"));
	EXPECT_TRUE(is_ip_literal_or_localhost("192.168.0.10"));
	EXPECT_TRUE(is_ip_literal_or_localhost("LocalHost"));
	EXPECT_TRUE(is_ip_literal_or_localhost("::1"));
	EXPECT_FALSE(is_ip_literal_or_localhost("evil.com"));
	EXPECT_FALSE(is_ip_literal_or_localhost("127.0.0.1.evil.com"));
	EXPECT_FALSE(is_ip_literal_or_localhost("256.0.0.1"));
	EXPECT_FALSE(is_ip_literal_or_localhost("1.2.3"));
	EXPECT_FALSE(is_ip_literal_or_localhost(""));

	auto host = parse_web_host("[::1]:22053", 80);
	EXPECT_TRUE(host.is_valid);
	EXPECT_EQ(host.host, "::1");
	EXPECT_EQ(host.port, 22053);
	EXPECT_EQ(parse_web_host("127.0.0.1", 80).port, 80);
	EXPECT_FALSE(parse_web_host("127.0.0.1:99999", 80).is_valid);
	EXPECT_FALSE(parse_web_host("::1", 80).is_valid);

	const std::vector<std::string> none;
	const std::vector<std::string> allowed = { "http://admin.example.com:22052" };
	EXPECT_TRUE(is_web_host_allowed("127.0.0.1:22052", none));
	EXPECT_TRUE(is_web_host_allowed("localhost:22052", none));
	EXPECT_TRUE(is_web_host_allowed("", none));
	EXPECT_FALSE(is_web_host_allowed("evil.com:22052", none));				// - dns rebinding
	EXPECT_TRUE(is_web_host_allowed("admin.example.com:22052", allowed));
	EXPECT_FALSE(is_web_host_allowed("other.example.com:22052", allowed));
}

TEST(admin_policy, web_origin)
{
	using namespace server_system;

	const std::vector<std::string> none;

	// - same host web ui
	EXPECT_TRUE(is_web_origin_allowed("http://127.0.0.1:22052", "127.0.0.1:22053", 22052, none));
	EXPECT_TRUE(is_web_origin_allowed("http://localhost:22052", "localhost:22053", 22052, none));
	EXPECT_TRUE(is_web_origin_allowed("http://192.168.0.10:22052", "192.168.0.10:22053", 22052, none));
	EXPECT_TRUE(is_web_origin_allowed("http://[::1]:22052", "[::1]:22053", 22052, none));

	// - no origin (not a browser)
	EXPECT_TRUE(is_web_origin_allowed("", "127.0.0.1:22053", 22052, none));

	// - other site / other port / other host / dns rebinding / null
	EXPECT_FALSE(is_web_origin_allowed("http://evil.com", "127.0.0.1:22053", 22052, none));
	EXPECT_FALSE(is_web_origin_allowed("http://127.0.0.1:8080", "127.0.0.1:22053", 22052, none));
	EXPECT_FALSE(is_web_origin_allowed("http://192.168.0.11:22052", "192.168.0.10:22053", 22052, none));
	EXPECT_FALSE(is_web_origin_allowed("http://evil.com:22052", "evil.com:22053", 22052, none));
	EXPECT_FALSE(is_web_origin_allowed("null", "127.0.0.1:22053", 22052, none));
	EXPECT_FALSE(is_web_origin_allowed("file://", "127.0.0.1:22053", 22052, none));

	// - allowed_origins
	const std::vector<std::string> allowed = { "http://admin.example.com:22052" };
	EXPECT_TRUE(is_web_origin_allowed("http://ADMIN.example.com:22052", "admin.example.com:22053", 22052, allowed));
	EXPECT_FALSE(is_web_origin_allowed("http://admin.example.com:22099", "admin.example.com:22053", 22052, allowed));
}

//-----------------------------------------------------------------------------
// web ui login (host_agent)
//-----------------------------------------------------------------------------
TEST(admin_policy, web_cookie_form)
{
	using namespace server_system;

	EXPECT_EQ(get_cookie_value("cgdk_session=abc", "cgdk_session"), "abc");
	EXPECT_EQ(get_cookie_value("a=1; cgdk_session=abc ; b=2", "cgdk_session"), "abc");
	EXPECT_EQ(get_cookie_value("x_cgdk_session=abc", "cgdk_session"), "");
	EXPECT_EQ(get_cookie_value("", "cgdk_session"), "");

	EXPECT_EQ(get_form_value("key=abc", "key").value_or("-"), "abc");
	EXPECT_EQ(get_form_value("a=1&key=a+b%21%2f", "key").value_or("-"), "a b!/");
	EXPECT_EQ(get_form_value("key=", "key").value_or("-"), "");
	EXPECT_EQ(get_form_value("key=50%", "key").value_or("-"), "50%");
	EXPECT_FALSE(get_form_value("keys=1&akey=2", "key").has_value());
}

TEST(admin_policy, web_session_store)
{
	server_system::web_session_store store;
	store.set_policy(3600s, 3, 300s, 600s);

	const std::string ip = "10.0.0.5";
	auto now = server_system::web_session_store::clock_t::now();
	auto token = store.create(ip, now);
	EXPECT_EQ(token.size(), 64u);
	EXPECT_NE(store.create(ip, now), token);
	EXPECT_TRUE(store.is_valid(token, ip, now + 10s));
	EXPECT_FALSE(store.is_valid(token, "10.0.0.6", now + 10s));	// - other address (stolen cookie)
	EXPECT_FALSE(store.is_valid("abc", ip, now));
	EXPECT_FALSE(store.is_valid(std::string(64, '0'), ip, now));

	// - idle time (600s) : used at +10s -> valid until +610s
	EXPECT_TRUE(store.is_valid(token, ip, now + 600s));
	EXPECT_TRUE(store.is_valid(token, ip, now + 1100s));
	EXPECT_FALSE(store.is_valid(token, ip, now + 1800s));		// - idle (last used at +1100s)

	// - lifetime (3600s)
	auto token2 = store.create(ip, now);
	for (auto t = 500s; t < 3600s; t += 500s)
		EXPECT_TRUE(store.is_valid(token2, ip, now + t));
	EXPECT_FALSE(store.is_valid(token2, ip, now + 3601s));		// - expired

	auto token3 = store.create(ip, now);
	store.remove(token3);
	EXPECT_FALSE(store.is_valid(token3, ip, now));

	// - login failures
	EXPECT_FALSE(store.on_fail("10.0.0.1", now));
	EXPECT_FALSE(store.on_fail("10.0.0.1", now));
	EXPECT_TRUE(store.on_fail("10.0.0.1", now));
	EXPECT_TRUE(store.is_blocked("10.0.0.1", now + 10s));
	EXPECT_FALSE(store.is_blocked("10.0.0.1", now + 301s));
	EXPECT_FALSE(store.is_blocked("10.0.0.2", now));
	store.on_success("10.0.0.1");
	EXPECT_FALSE(store.is_blocked("10.0.0.1", now + 10s));
}

//-----------------------------------------------------------------------------
// control api signature (sha256 / hmac-sha256)
//-----------------------------------------------------------------------------
TEST(admin_policy, sha256)
{
	EXPECT_EQ(admin::sha256_hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	EXPECT_EQ(admin::sha256_hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	EXPECT_EQ(admin::sha256_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
	EXPECT_EQ(admin::sha256_hex(std::string(1000, 'a')), "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
}

TEST(admin_policy, hmac_sha256)
{
	// - RFC 4231 test case 1, 2, 6 (key longer than the block)
	EXPECT_EQ(admin::hmac_sha256_hex(std::string(20, '\x0b'), "Hi There"), "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
	EXPECT_EQ(admin::hmac_sha256_hex("Jefe", "what do ya want for nothing?"), "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
	EXPECT_EQ(admin::hmac_sha256_hex(std::string(131, '\xaa'), "Test Using Larger Than Block-Size Key - Hash Key First"), "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
}

TEST(admin_policy, signature_verifier)
{
	using eRESULT_VERIFY = admin::signature_verifier::eRESULT_VERIFY;

	admin::signature_verifier verifier;
	verifier.set_allowed_skew(60s);

	const int64_t now = 1'800'000'000;
	const std::string timestamp = std::to_string(now);
	auto signature = admin::make_signature("key", "POST", "/api/instances/1/start", timestamp, "");

	// - ok / replay
	EXPECT_EQ(verifier.verify("key", "POST", "/api/instances/1/start", timestamp, "", signature, now + 5), eRESULT_VERIFY::OK);
	EXPECT_EQ(verifier.verify("key", "POST", "/api/instances/1/start", timestamp, "", signature, now + 6), eRESULT_VERIFY::REPLAYED);

	// - wrong key / path / method / body
	auto signature2 = admin::make_signature("key", "GET", "/metrics", timestamp, "");
	EXPECT_EQ(verifier.verify("other", "GET", "/metrics", timestamp, "", signature2, now), eRESULT_VERIFY::INVALID_SIGNATURE);
	EXPECT_EQ(verifier.verify("key", "GET", "/api/instances", timestamp, "", signature2, now), eRESULT_VERIFY::INVALID_SIGNATURE);
	EXPECT_EQ(verifier.verify("key", "POST", "/metrics", timestamp, "", signature2, now), eRESULT_VERIFY::INVALID_SIGNATURE);
	EXPECT_EQ(verifier.verify("key", "GET", "/metrics", timestamp, "x", signature2, now), eRESULT_VERIFY::INVALID_SIGNATURE);

	// - upper case hex is accepted
	std::string signature2_upper = signature2;
	std::transform(signature2_upper.begin(), signature2_upper.end(), signature2_upper.begin(), [](unsigned char _ch) { return static_cast<char>(std::toupper(_ch)); });
	EXPECT_EQ(verifier.verify("key", "GET", "/metrics", timestamp, "", signature2_upper, now), eRESULT_VERIFY::OK);

	// - timestamp
	EXPECT_EQ(verifier.verify("key", "GET", "/metrics", timestamp, "", signature2, now + 61), eRESULT_VERIFY::INVALID_TIMESTAMP);
	EXPECT_EQ(verifier.verify("key", "GET", "/metrics", "abc", "", signature2, now), eRESULT_VERIFY::INVALID_TIMESTAMP);
	EXPECT_EQ(verifier.verify("key", "GET", "/metrics", "", "", signature2, now), eRESULT_VERIFY::INVALID_TIMESTAMP);
}

//-----------------------------------------------------------------------------
// rate limiter / audit log
//-----------------------------------------------------------------------------
TEST(admin_policy, rate_limiter)
{
	server_system::rate_limiter limiter;
	limiter.set_rate(10.0, 3.0);

	auto now = server_system::rate_limiter::clock_t::now();

	// - burst 3
	EXPECT_TRUE(limiter.try_acquire(now));
	EXPECT_TRUE(limiter.try_acquire(now));
	EXPECT_TRUE(limiter.try_acquire(now));
	EXPECT_FALSE(limiter.try_acquire(now));

	// - 10/s : 1 token every 100ms
	EXPECT_FALSE(limiter.try_acquire(now + 50ms));
	EXPECT_TRUE(limiter.try_acquire(now + 150ms));
	EXPECT_FALSE(limiter.try_acquire(now + 160ms));

	// - refilled up to the burst
	EXPECT_TRUE(limiter.try_acquire(now + 10s));
	EXPECT_TRUE(limiter.try_acquire(now + 10s));
	EXPECT_TRUE(limiter.try_acquire(now + 10s));
	EXPECT_FALSE(limiter.try_acquire(now + 10s));

	// - 0 : no limit
	server_system::rate_limiter unlimited;
	unlimited.set_rate(0.0, 0.0);
	for (int i = 0; i < 1000; ++i)
		EXPECT_TRUE(unlimited.try_acquire(now));
}

TEST(admin_policy, audit_log)
{
	// - line format
	auto time = std::chrono::system_clock::time_point(std::chrono::milliseconds(1'800'000'000'123));
	auto line = server_system::audit_log::make_line(time, "control", "10.0.0.1", "POST /api/instances/1/start", 200, "a\"b\nc");
	const std::string expected = R"({"time":"2027-01-15T08:00:00.123Z","source":"control","address":"10.0.0.1","action":"POST /api/instances/1/start","result":200,"detail":"a\"b\nc"})";
	EXPECT_EQ(line, expected);

	// - file (not set : not written)
	server_system::audit_log audit;
	EXPECT_FALSE(audit.write("control", "x", "y", 0));

	auto path = std::filesystem::temp_directory_path() / ("cgdk_unit_test_audit_" + std::to_string(api::get_process_id())) / "audit.jsonl";
	std::error_code ec;
	std::filesystem::remove_all(path.parent_path(), ec);

	audit.set_file(path.string());
	EXPECT_TRUE(audit.write("web", "10.0.0.2", "login", 401, "wrong key"));
	EXPECT_TRUE(audit.write("web", "10.0.0.2", "login", 200));

	std::ifstream file(path);
	std::string line1, line2, line3;
	std::getline(file, line1);
	std::getline(file, line2);
	EXPECT_FALSE(static_cast<bool>(std::getline(file, line3)));
	const std::string expected1 = R"("action":"login","result":401,"detail":"wrong key")";
	const std::string expected2 = R"("result":200)";
	EXPECT_NE(line1.find(expected1), std::string::npos);
	EXPECT_NE(line2.find(expected2), std::string::npos);
	file.close();

	std::filesystem::remove_all(path.parent_path(), ec);
}

//-----------------------------------------------------------------------------
// key file / key setting
//-----------------------------------------------------------------------------
TEST(admin_policy, key_file)
{
	auto dir = std::filesystem::temp_directory_path() / ("cgdk_unit_test_key_" + std::to_string(api::get_process_id()));
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);

	auto file_key = (dir / "control.key").string();
	{
		std::ofstream file(file_key, std::ios::binary);
		file << "  file-key-123\r\n";
	}

	// - read (trimmed)
	auto key = server_system::read_key_file(file_key);
	ASSERT_TRUE(key.has_value());
	EXPECT_EQ(*key, "file-key-123");
	EXPECT_FALSE(server_system::read_key_file((dir / "not_exist.key").string()).has_value());

	// - "<name>_file" > "<name>"
	context setting;
	setting["certify_key"] = "plain-key";
	EXPECT_EQ(server_system::get_key_from_setting(setting, "certify_key"), "plain-key");

	setting["certify_key_file"] = file_key;
	EXPECT_EQ(server_system::get_key_from_setting(setting, "certify_key"), "file-key-123");

	// - unreadable file : nobody can be certified (not empty = not 'no check')
	context setting_bad;
	setting_bad["login_key_file"] = (dir / "not_exist.key").string();
	auto key_bad = server_system::get_key_from_setting(setting_bad, "login_key");
	EXPECT_FALSE(key_bad.empty());
	EXPECT_FALSE(server_system::is_certify_key_matched(key_bad, ""));

	// - nothing
	context setting_none;
	EXPECT_EQ(server_system::get_key_from_setting(setting_none, "login_key"), "");

	std::filesystem::remove_all(dir, ec);
}

//-----------------------------------------------------------------------------
// log_throttle : once in the interval, the count of the suppressed
//-----------------------------------------------------------------------------
TEST(admin_policy, log_throttle)
{
	server_system::log_throttle throttle(10s);
	const auto now = server_system::log_throttle::clock_t::now();

	// - the first : logged (0 suppressed)
	auto first = throttle.try_log(now);
	ASSERT_TRUE(first.has_value());
	EXPECT_EQ(*first, 0u);

	// - in the interval : suppressed
	EXPECT_FALSE(throttle.try_log(now + 1s).has_value());
	EXPECT_FALSE(throttle.try_log(now + 5s).has_value());
	EXPECT_FALSE(throttle.try_log(now + 9s).has_value());

	// - after the interval : logged with the count of the suppressed
	auto second = throttle.try_log(now + 10s);
	ASSERT_TRUE(second.has_value());
	EXPECT_EQ(*second, 3u);

	EXPECT_FALSE(throttle.try_log(now + 11s).has_value());
	auto third = throttle.try_log(now + 25s);
	ASSERT_TRUE(third.has_value());
	EXPECT_EQ(*third, 1u);
}

}
