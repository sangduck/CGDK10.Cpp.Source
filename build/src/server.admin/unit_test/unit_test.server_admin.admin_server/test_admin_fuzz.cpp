#include "pch.h"
#include <cstdlib>
#include <random>

//-----------------------------------------------------------------------------
// random input (fuzz) : the parsers of the remote input (control api / web ui / setting)
//
//  - control api : parse_control_route (method, path), signature_verifier::verify (headers)
//  - web ui      : parse_web_host, is_web_host_allowed, is_web_origin_allowed, get_cookie_value, get_form_value
//  - setting     : desired_state_store::from_json (context::from_json), parse_command, parse_iid
//  - valid inputs are changed at random or random bytes. no crash, no hang (an exception is allowed : the request fails)
//  - CGDK_FUZZ_ITERATIONS : the iterations (default 3000) / CGDK_FUZZ_SEED : the seed (default 20261011)
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	std::size_t admin_fuzz_env(const char* _name, std::size_t _default)
	{
#if defined(_MSC_VER)
		char* value = nullptr;
		std::size_t length = 0;
		if (_dupenv_s(&value, &length, _name) != 0 || value == nullptr)
			return _default;
		std::unique_ptr<char, decltype(&free)> holder(value, &free);
#else
		const char* value = std::getenv(_name);
#endif
		return (value != nullptr && *value != '\0') ? static_cast<std::size_t>(std::strtoull(value, nullptr, 10)) : _default;
	}

	std::string admin_fuzz_mutate(std::mt19937& _rng, std::string _data)
	{
		const auto count = 1 + _rng() % 4;
		for (std::size_t i = 0; i < count; ++i)
		{
			switch (_rng() % 6)
			{
			case 0:	if (_data.empty() == false) _data[_rng() % _data.size()] ^= static_cast<char>(1 << (_rng() % 8)); break;
			case 1:	if (_data.empty() == false) _data[_rng() % _data.size()] = static_cast<char>(_rng() % 256); break;
			case 2:	_data.insert(_data.begin() + static_cast<std::ptrdiff_t>(_data.empty() ? 0 : _rng() % _data.size()), static_cast<char>(_rng() % 256)); break;
			case 3:	if (_data.empty() == false) _data.erase(_rng() % _data.size(), 1 + _rng() % 8); break;
			case 4:	if (_data.empty() == false) _data.resize(_rng() % _data.size()); break;
			case 5:	_data += _data.substr(0, _rng() % (_data.size() + 1)); break;
			}
		}
		return _data;
	}

	std::string admin_fuzz_input(std::mt19937& _rng, const std::vector<std::string>& _seeds, std::size_t _index)
	{
		if (_index % 8 == 7)
		{
			std::string data(_rng() % 129, '\0');
			for (auto& iter : data)
				iter = static_cast<char>(_rng() % 256);
			return data;
		}
		return admin_fuzz_mutate(_rng, _seeds[_rng() % _seeds.size()]);
	}

	struct sADMIN_FUZZ
	{
		std::size_t		iterations;
		std::mt19937	rng;

		explicit sADMIN_FUZZ(uint32_t _seed_offset) :
			iterations(admin_fuzz_env("CGDK_FUZZ_ITERATIONS", 3000)),
			rng(static_cast<uint32_t>(admin_fuzz_env("CGDK_FUZZ_SEED", 20261011)) + _seed_offset)
		{
		}
	};
}

TEST(admin_fuzz, control_route)
{
	sADMIN_FUZZ fuzz(0);
	const std::vector<std::string> methods = { "GET", "POST", "PUT", "DELETE", "" };
	const std::vector<std::string> paths =
	{
		"/api/instances", "/api/instances/21101", "/api/instances/21101/start", "/api/instances/7/desired/running",
		"/api/hosts", "/api/reload", "/metrics", "/api/instances?x=1&y=2", "/api/instances/18446744073709551615/stop",
	};

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto method = admin_fuzz_input(fuzz.rng, methods, i);
		const auto path = admin_fuzz_input(fuzz.rng, paths, i);
		try
		{
			auto route = admin::parse_control_route(method, path);

			// - a request has an instance id
			if (route.action == admin::eCONTROL_ACTION::REQUEST || route.action == admin::eCONTROL_ACTION::DESIRED)
			{
				EXPECT_NE(route.iid, 0u) << "path " << path;
			}
		}
		catch (...)
		{
		}
	}
}

TEST(admin_fuzz, command)
{
	sADMIN_FUZZ fuzz(1);
	const std::vector<std::string> lines = { "start 21101", "  want\t7 Running", "list", "status 4206232923320050582", "reload  ", "" };

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto line = admin_fuzz_input(fuzz.rng, lines, i);
		try
		{
			auto command = admin::parse_command(line);
			for (const auto& iter : command.args)
				(void)admin::parse_iid(iter);
		}
		catch (...)
		{
		}
		(void)admin::parse_iid(line);
	}
}

TEST(admin_fuzz, desired_state_json)
{
	sADMIN_FUZZ fuzz(2);
	const std::vector<std::string> texts =
	{
		"{ \"1\": \"running\", \"2\": \"stopped\" }",
		"{ \"21101\": \"running\", \"abc\": [1, 2, {\"x\": null}], \"3\": 5 }",
		"{}", "[]", "{ \"1\": { \"nested\": [ true, false, 1.5e10, \"\\u0041\" ] } }",
	};

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto text = admin_fuzz_input(fuzz.rng, texts, i);
		try
		{
			admin::desired_state_store store;
			store.from_json(text);

			// - only the valid entries are kept : the saved json is read again as the same
			admin::desired_state_store store_read;
			store_read.from_json(store.to_json());
			EXPECT_EQ(store_read.size(), store.size()) << "json " << text;
			EXPECT_EQ(store_read.to_json(), store.to_json()) << "json " << text;
		}
		catch (...)
		{
		}
	}
}

TEST(admin_fuzz, web)
{
	using namespace server_system;

	sADMIN_FUZZ fuzz(3);
	const std::vector<std::string> hosts = { "127.0.0.1:22052", "[::1]:22053", "localhost", "admin.example.com:22052", "192.168.0.10" };
	const std::vector<std::string> origins = { "http://127.0.0.1:22052", "http://[::1]:22052", "https://admin.example.com:22052", "null", "file://" };
	const std::vector<std::string> cookies = { "cgdk_session=abc", "a=1; cgdk_session=abc ; b=2", "cgdk_session=", "=;;=" };
	const std::vector<std::string> forms = { "key=abc", "a=1&key=a+b%21%2f", "key=50%", "key=%zz&key=%4" };
	const std::vector<std::string> none;
	const std::vector<std::string> allowed = { "http://admin.example.com:22052" };

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto host = admin_fuzz_input(fuzz.rng, hosts, i);
		const auto origin = admin_fuzz_input(fuzz.rng, origins, i);
		const auto cookie = admin_fuzz_input(fuzz.rng, cookies, i);
		const auto form = admin_fuzz_input(fuzz.rng, forms, i);
		try
		{
			auto web_host = parse_web_host(host, 80);
			if (web_host.is_valid)
			{
				EXPECT_NE(web_host.port, 0);
			}
			(void)is_ip_literal_or_localhost(host);
			(void)is_web_host_allowed(host, none);
			(void)is_web_host_allowed(host, allowed);
			(void)is_web_origin_allowed(origin, host, 22052, none);
			(void)is_web_origin_allowed(origin, host, 22052, allowed);
			(void)get_cookie_value(cookie, "cgdk_session");
			(void)get_form_value(form, "key");
		}
		catch (...)
		{
		}
	}
}

TEST(admin_fuzz, signature)
{
	sADMIN_FUZZ fuzz(4);
	const int64_t now = 1'800'000'000;
	const std::string timestamp = std::to_string(now);
	const std::vector<std::string> signatures = { admin::make_signature("key", "POST", "/api/instances/1/start", timestamp, ""), std::string(64, '0'), "" };
	const std::vector<std::string> timestamps = { timestamp, "-1", "99999999999999999999999", "" };

	admin::signature_verifier verifier;
	verifier.set_allowed_skew(60s);

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto signature = admin_fuzz_input(fuzz.rng, signatures, i);
		const auto time = admin_fuzz_input(fuzz.rng, timestamps, i);
		try
		{
			auto result = verifier.verify("key", "POST", "/api/instances/1/start", time, "", signature, now);

			// - a changed signature or timestamp is never accepted (except the same values)
			if (result == admin::signature_verifier::eRESULT_VERIFY::OK)
			{
				EXPECT_TRUE(signature == signatures[0] || time == timestamp) << "signature " << signature << " timestamp " << time;
			}
		}
		catch (...)
		{
		}
	}
}

}
