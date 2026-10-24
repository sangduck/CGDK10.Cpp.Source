#include "pch.h"

#if defined(_WIN32)
	#include <shellapi.h>
	#pragma comment(lib, "shell32.lib")
#endif

//-----------------------------------------------------------------------------
// sSERVICE::sSETTING / Xservice::run 단위 테스트
//
//  - quote_argument / get_arguments_string (windows: CommandLineToArgvW로 다시 나누면 원래 인자가 된다.)
//  - get_environment_block (windows: 기본 환경에 추가/덮어쓰기, 이름순 정렬, 대소문자 무시)
//  - eSERVICE_TYPE <-> 문자열
//  - Xservice::run의 옵션 경고
//-----------------------------------------------------------------------------
namespace CGDK
{
using SETTING = server_system::sSERVICE::sSETTING;

//-----------------------------------------------------------------------------
// quote_argument
//-----------------------------------------------------------------------------
TEST(service_setting, quote_argument_plain)
{
	EXPECT_EQ(SETTING::quote_argument("abc"), "abc");
	EXPECT_EQ(SETTING::quote_argument("--setting"), "--setting");
	EXPECT_EQ(SETTING::quote_argument("config/a.json"), "config/a.json");
}

TEST(service_setting, quote_argument_quoted)
{
	EXPECT_EQ(SETTING::quote_argument(""), "\"\"");
	EXPECT_EQ(SETTING::quote_argument("a b"), "\"a b\"");
	EXPECT_EQ(SETTING::quote_argument("a\"b"), "\"a\\\"b\"");
	EXPECT_EQ(SETTING::quote_argument("C:\\path with space\\"), "\"C:\\path with space\\\\\"");
}

TEST(service_setting, arguments_string)
{
	SETTING setting;
	setting.arguments = { "--setting", "config/a b.json", "x" };

	EXPECT_EQ(setting.get_arguments_string(), "--setting \"config/a b.json\" x");
	EXPECT_EQ(setting.get_arguments_string("C:/bin/server.exe"), "C:/bin/server.exe --setting \"config/a b.json\" x");
}

#if defined(_WIN32)
TEST(service_setting, arguments_string_round_trip)
{
	// - CommandLineToArgvW로 다시 나누면 원래 인자가 되어야 한다.
	std::vector<std::string> arguments = { "plain", "with space", "quote\"inside", "trail\\", "space trail\\", "", "\xED\x95\x9C\xEA\xB8\x80 arg", "back\\\\slash\\\"q" };

	SETTING setting;
	setting.arguments = arguments;

	auto command_line = server_system::utf8_to_utf16(setting.get_arguments_string("C:\\bin\\server.exe"));

	int count = 0;
	auto pargv = ::CommandLineToArgvW(command_line.c_str(), &count);
	ASSERT_NE(pargv, nullptr);
	ASSERT_EQ(count, static_cast<int>(arguments.size() + 1));

	EXPECT_EQ(server_system::utf16_to_utf8(pargv[0]), "C:\\bin\\server.exe");
	for (std::size_t i = 0; i < arguments.size(); ++i)
		EXPECT_EQ(server_system::utf16_to_utf8(pargv[i + 1]), arguments[i]) << "argument " << i;

	::LocalFree(pargv);
}

//-----------------------------------------------------------------------------
// get_environment_block
//-----------------------------------------------------------------------------
namespace
{
	std::vector<std::wstring> _split_block(const std::wstring& _block)
	{
		std::vector<std::wstring> result;
		for (const wchar_t* pos = _block.c_str(); *pos != L'\0'; pos += std::wcslen(pos) + 1)
			result.emplace_back(pos);
		return result;
	}
}

TEST(service_setting, environment_block_empty)
{
	SETTING setting;

	// - environment가 없으면 빈 문자열 (기본 환경을 그대로 쓴다.)
	EXPECT_TRUE(setting.get_environment_block().empty());
	EXPECT_TRUE(setting.get_environment_block(L"A=1\0B=2\0").empty());
}

TEST(service_setting, environment_block_merge)
{
	SETTING setting;
	setting.environment = { "path=C:\\new", "NEW_VALUE=\xED\x95\x9C\xEA\xB8\x80" };

	// - 기본 환경 ('=C:'처럼 '='로 시작하는 숨은 변수 포함)
	const wchar_t base[] = L"=C:=C:\\work\0Path=C:\\old\0TEMP=C:\\temp\0Alpha=1\0";

	auto block = setting.get_environment_block(base);

	// - 끝은 NULL 두 개
	ASSERT_GE(block.size(), 2u);
	EXPECT_EQ(block[block.size() - 1], L'\0');
	EXPECT_EQ(block[block.size() - 2], L'\0');

	auto items = _split_block(block);

	// - Path는 대소문자와 관계없이 덮어쓴다. (기존 'Path=C:\old'는 없다.)
	EXPECT_EQ(std::count(items.begin(), items.end(), std::wstring(L"Path=C:\\old")), 0);
	EXPECT_EQ(std::count(items.begin(), items.end(), std::wstring(L"path=C:\\new")), 1);

	// - 기본 환경의 나머지는 남는다.
	EXPECT_EQ(std::count(items.begin(), items.end(), std::wstring(L"=C:=C:\\work")), 1);
	EXPECT_EQ(std::count(items.begin(), items.end(), std::wstring(L"TEMP=C:\\temp")), 1);
	EXPECT_EQ(std::count(items.begin(), items.end(), std::wstring(L"Alpha=1")), 1);

	// - 추가한 값 (UTF-8 -> UTF-16)
	EXPECT_EQ(std::count(items.begin(), items.end(), server_system::utf8_to_utf16("NEW_VALUE=\xED\x95\x9C\xEA\xB8\x80")), 1);

	// - 이름순 (대소문자 무시)
	auto upper_name = [](const std::wstring& _item) { auto name = _item.substr(0, _item.find(L'=', 1)); for (auto& ch : name) ch = static_cast<wchar_t>(::towupper(ch)); return name; };
	for (std::size_t i = 1; i < items.size(); ++i)
		EXPECT_LE(upper_name(items[i - 1]), upper_name(items[i]));
}
#endif

//-----------------------------------------------------------------------------
// eSERVICE_TYPE
//-----------------------------------------------------------------------------
TEST(service_setting, service_type_string)
{
	EXPECT_EQ(server_system::to_service_type("execute"), server_system::eSERVICE_TYPE::EXECUTE);
	EXPECT_EQ(server_system::to_service_type("service"), server_system::eSERVICE_TYPE::SERVICE);
	EXPECT_EQ(server_system::to_service_type("windows_service"), server_system::eSERVICE_TYPE::SERVICE);
	EXPECT_EQ(server_system::to_service_type("docker"), server_system::eSERVICE_TYPE::DOCKER);
	EXPECT_EQ(server_system::to_service_type("unknown"), server_system::eSERVICE_TYPE::EXECUTE);

	EXPECT_EQ(server_system::to_service_type_string(server_system::eSERVICE_TYPE::EXECUTE), "execute");
	EXPECT_EQ(server_system::to_service_type_string(server_system::eSERVICE_TYPE::SERVICE), "service");
	EXPECT_EQ(server_system::to_service_type_string(server_system::eSERVICE_TYPE::DOCKER), "docker");
}

//-----------------------------------------------------------------------------
// Xservice::run option warnings
//  - '--stop <없는 service>'는 바로 실패하고 끝난다. (옵션만 확인한다.)
//-----------------------------------------------------------------------------
TEST(service_run, option_warnings)
{
	auto pservice = make_own<server_system::Xservice>("unit_test_service"sv);

	std::vector<std::string> arguments = { "unit_test", "--stop", "cgdk_unit_test_no_such_service", "--foo", "--setting", "a.json", "--setting", "b.json", "--no-restart", "--install" };
	std::vector<char*> argv;
	for (auto& iter : arguments)
		argv.push_back(iter.data());

#if defined(_WIN32)
	// - windows는 GetCommandLineW를 쓰므로 이 테스트의 인자는 linux에서만 확인한다.
	GTEST_SKIP() << "windows: run() reads the arguments from GetCommandLineW()";
#else
	auto result = pservice->run(static_cast<int>(argv.size()), argv.data(), "none.json");
	EXPECT_NE(result, 0);

	const auto& warnings = pservice->get_run_warnings();
	const auto has = [&](std::string_view _text) { return std::any_of(warnings.begin(), warnings.end(), [&](const std::string& _w) { return _w.find(_text) != std::string::npos; }); };

	EXPECT_TRUE(has("unknown option '--foo'"));
	EXPECT_TRUE(has("'--setting' is given more than once"));
	EXPECT_TRUE(has("'--no-restart' is used only with '--install'"));
	EXPECT_TRUE(has("'--install' must be the first argument"));

	EXPECT_EQ(pservice->get_run_setting_file(), "b.json");
#endif
}

//-----------------------------------------------------------------------------
// service -> host_agent -> admin_server (e2e)
//
//  - 환경변수 CGDK_TEST_HOST_AGENT가 있을 때만 실행한다.
//  - 준비) admin_server ("service_instance_manager": { "auto_start": true })와
//          host_agent(127.0.0.1:22044)를 띄워 둔다. (port는 환경변수 CGDK_TEST_HOST_AGENT_PORT로 바꿀 수 있다.)
//          host_agent의 server list에 rank {0,0,10,1} (iid 21101) 항목이 있어야 한다.
//  - service는 initialize만 한다. host_agent를 거쳐 admin_server에 STOPPED가 전달되면
//    admin_server가 REQUEST_START를 보내고 service가 RUNNING이 되면 성공이다.
//-----------------------------------------------------------------------------
TEST(service_admin, e2e_through_host_agent)
{
#if defined(_WIN32)
	if (GetEnvironmentVariableA("CGDK_TEST_HOST_AGENT", nullptr, 0) == 0)
#else
	if (std::getenv("CGDK_TEST_HOST_AGENT") == nullptr)
#endif
		GTEST_SKIP() << "set CGDK_TEST_HOST_AGENT=1 (needs admin_server + host_agent running)";

	auto pservice = make_own<server_system::Xservice>("unit_test_e2e"sv);

	// - host_agent port (CGDK_TEST_HOST_AGENT_PORT. 없으면 "default"(127.0.0.1:22044))
	std::string str_socket_to_admin = R"("default")";
	{
		std::string str_port;
	#if defined(_WIN32)
		char buf_port[16]{ 0 };
		if (GetEnvironmentVariableA("CGDK_TEST_HOST_AGENT_PORT", buf_port, static_cast<DWORD>(std::size(buf_port))) > 0)
			str_port = buf_port;
	#else
		if (auto pport = std::getenv("CGDK_TEST_HOST_AGENT_PORT"); pport != nullptr)
			str_port = pport;
	#endif
		if (str_port.empty() == false)
			str_socket_to_admin = std::string(R"({ "remote_endpoint": { "address": "127.0.0.1", "port": )") + std::to_string(std::atoi(str_port.c_str())) + " } }";
	}

	context context_setting;
	context_setting.from_json(std::string(R"({
		"service": {
			"iid": 21101,
			"setting": {
				"name": "unit_test_e2e",
				"rank": { "group": 0, "field": 0, "role": 10, "index": 1 }
			},
			"socket_to_admin": )") + str_socket_to_admin + R"(
		}
	})");

	pservice->initialize(context_setting);

	// - admin_server의 REQUEST_START로 RUNNING이 될 때까지 기다린다. (최대 20초)
	bool is_running = false;
	for (int i = 0; i < 200 && is_running == false; ++i)
	{
		std::this_thread::sleep_for(100ms);
		is_running = (pservice->get_object_state() == eOBJECT_STATE::RUNNING);
	}

	EXPECT_TRUE(is_running);

	pservice->destroy();
}

//-----------------------------------------------------------------------------
// net::ip::address_v6::get_v4_mapped / sACCESS_POLICY
//-----------------------------------------------------------------------------
TEST(service_admin, address_v4_mapped)
{
	auto v6 = net::ip::make_address_v6(net::ip::v4_mapped_t::v4_mapped, net::ip::make_address_v4("192.168.0.10"sv));
	EXPECT_TRUE(v6.is_v4_mapped());
	EXPECT_EQ(v6.get_v4_mapped().to_string(), "192.168.0.10");

	auto v6_plain = net::ip::make_address_v6("::1"sv);
	EXPECT_FALSE(v6_plain.is_v4_mapped());
	EXPECT_EQ(v6_plain.get_v4_mapped().to_uint(), 0u);
}

TEST(service_admin, access_policy)
{
	context setting;
	setting.from_json(R"({ "certify_key": "k1", "allowed_addresses": [ "127.0.0.1", "10.0.0.5", "::1", "bad address" ] })"sv);

	server_system::sACCESS_POLICY policy;
	policy.load(setting);

	EXPECT_TRUE(policy.is_secured());
	EXPECT_EQ(policy.allowed_addresses.size(), 3u);		// - 'bad address' is ignored

	// - address (ipv4 / ipv4-mapped ipv6 / ipv6)
	EXPECT_TRUE(policy.is_address_allowed(net::ip::make_address_v4("127.0.0.1"sv)));
	EXPECT_TRUE(policy.is_address_allowed(net::ip::address(net::ip::make_address_v6(net::ip::v4_mapped_t::v4_mapped, net::ip::make_address_v4("10.0.0.5"sv)))));
	EXPECT_TRUE(policy.is_address_allowed(net::ip::make_address_v6("::1"sv)));
	EXPECT_FALSE(policy.is_address_allowed(net::ip::make_address_v4("10.0.0.6"sv)));

	// - key
	EXPECT_TRUE(policy.is_key_matched("k1"sv));
	EXPECT_FALSE(policy.is_key_matched("k2"sv));
	EXPECT_FALSE(policy.is_key_matched(""sv));
	EXPECT_FALSE(policy.is_key_matched("k1x"sv));

	// - no setting: everything is allowed
	context empty_setting;
	server_system::sACCESS_POLICY policy_open;
	policy_open.load(empty_setting);
	EXPECT_TRUE(policy_open.is_address_allowed(net::ip::make_address_v4("1.2.3.4"sv)));
	EXPECT_TRUE(policy_open.allowed_addresses.empty());
}

}
