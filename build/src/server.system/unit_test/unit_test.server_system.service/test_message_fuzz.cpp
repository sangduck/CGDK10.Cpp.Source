#include "pch.h"
#include <cstdlib>
#include <random>

//-----------------------------------------------------------------------------
// random input (fuzz) : the messages of the admin system (admin_server <-> host_agent <-> service)
//
//  - a valid message (make_message_SERVER_ADMIN_xxx) is changed at random and read in the same order as the handler
//    (server_admin.group_socket.cpp, host_agent group_peer_immigration.cpp / socket_admin_server.cpp, server.service.cpp)
//  - context::from_json (setting files, desired state, control api)
//  - no crash, no hang (an exception is allowed : the handler catches it and the message is ignored / the socket is closed)
//  - CGDK_FUZZ_ITERATIONS : the iterations (default 3000) / CGDK_FUZZ_SEED : the seed (default 20261011)
//-----------------------------------------------------------------------------
namespace CGDK
{

namespace
{
	std::size_t message_fuzz_env(const char* _name, std::size_t _default)
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

	std::string message_fuzz_mutate(std::mt19937& _rng, std::string _data)
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
			case 5:	// - a length field (4 bytes) set to a large value
					if (_data.size() >= 4)
					{
						const uint32_t value = (_rng() % 2) ? 0xffffffffu : (0x7fffffffu - static_cast<uint32_t>(_rng() % 16));
						memcpy(_data.data() + (_rng() % (_data.size() - 3)), &value, 4);
					}
					break;
			}
		}
		return _data;
	}

	std::string to_string(const shared_buffer& _buffer)
	{
		return std::string(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
	}

	// - reads a message like the handler (exceptions are caught by the handler)
	template <class TREAD>
	void read_message(const std::string& _data, TREAD&& _fn_read)
	{
		auto buf = alloc_shared_buffer(_data.size() + 8);
		buf.append(_data.size(), _data.data());
		try
		{
			buffer_view buf_recv = buf;
			if (buf_recv.size() < 8)
				return;
			buf_recv += offset(8);
			_fn_read(buf_recv);
		}
		catch (...)
		{
		}
	}

	// - Debug : the CGDK libraries assert on a buffer overflow (by design. Release : exception). the message tests run in Release
#if defined(NDEBUG)
	#define MESSAGE_FUZZ_RELEASE_ONLY()
#else
	#define MESSAGE_FUZZ_RELEASE_ONLY()	GTEST_SKIP() << "Debug : the CGDK libraries assert on a buffer overflow (by design). runs in Release"
#endif

	struct sMESSAGE_FUZZ
	{
		std::size_t		iterations;
		std::mt19937	rng;

		explicit sMESSAGE_FUZZ(uint32_t _seed_offset) :
			iterations(message_fuzz_env("CGDK_FUZZ_ITERATIONS", 3000)),
			rng(static_cast<uint32_t>(message_fuzz_env("CGDK_FUZZ_SEED", 20261011)) + _seed_offset)
		{
		}

		template <class TREAD>
		void			run(const std::vector<std::string>& _seeds, TREAD&& _fn_read)
		{
			for (std::size_t i = 0; i < this->iterations; ++i)
				read_message(message_fuzz_mutate(this->rng, _seeds[this->rng() % _seeds.size()]), _fn_read);
		}
	};
}

TEST(message_fuzz, certify_request)
{
	MESSAGE_FUZZ_RELEASE_ONLY();

	sMESSAGE_FUZZ fuzz(10);
	server_system::sCERTIFY_INFO certify_info;
	const std::vector<std::string> seeds =
	{
		to_string(server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(certify_info)),
		to_string(server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(certify_info, "certify-key")),
	};

	// - host_agent group_peer_immigration::on_message_server_system_CERTIFY_REQUEST
	fuzz.run(seeds, [](buffer_view& _buf)
	{
		auto info = _buf.extract<server_system::sCERTIFY_INFO>();
		std::string_view key = (_buf.size() != 0) ? _buf.extract<std::string_view>() : std::string_view();
		(void)info;
		(void)key;
	});
}

TEST(message_fuzz, instance_update)
{
	MESSAGE_FUZZ_RELEASE_ONLY();

	sMESSAGE_FUZZ fuzz(11);
	server_system::sSERVICE service;
	const std::vector<std::string> seeds =
	{
		to_string(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(21101, static_cast<eOBJECT_STATE>(4), static_cast<eOBJECT_STATE>(2), &service)),
		to_string(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(21101, static_cast<eOBJECT_STATE>(4), static_cast<eOBJECT_STATE>(2), nullptr)),
		to_string(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(21101, service)),
	};

	// - admin group_socket::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE / UPDATE_INFO
	fuzz.run(seeds, [](buffer_view& _buf)
	{
		buffer_view buf_info = _buf;
		(void)buf_info.extract<server_system::iid_t>();
		(void)buf_info.extract<server_system::sSERVICE>();

		(void)_buf.extract<server_system::iid_t>();
		(void)_buf.extract<eOBJECT_STATE>();
		(void)_buf.extract<eOBJECT_STATE>();
		if (_buf.extract<bool>())
			(void)_buf.extract<server_system::sSERVICE>();
	});
}

TEST(message_fuzz, instance_status)
{
	MESSAGE_FUZZ_RELEASE_ONLY();

	sMESSAGE_FUZZ fuzz(12);
	server_system::sSERVICE::sSTATUS status;
	server_system::sHOST_WEB_STATUS web_status;
	const std::vector<std::string> seeds =
	{
		to_string(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(21101, status)),
		to_string(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, SYSTEM::sSTATUS{})),
		to_string(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, SYSTEM::sSTATUS{}, &web_status)),
		to_string(server_system::make_message_SERVER_ADMIN_REQUEST_RESULT(21101, 3, 12345, eRESULT::SUCCESS, "done")),
	};

	// - admin group_socket::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS / HOST_UPDATE_STATUS / REQUEST_RESULT
	fuzz.run(seeds, [](buffer_view& _buf)
	{
		buffer_view buf_status = _buf;
		(void)buf_status.extract<server_system::iid_t>();
		(void)buf_status.extract<server_system::sSERVICE_STATUS_UPDATE>();

		buffer_view buf_host = _buf;
		(void)buf_host.extract<server_system::iid_t>();
		(void)server_system::extract_system_status(buf_host);
		(void)server_system::extract_host_web_status(buf_host);

		(void)_buf.extract<server_system::iid_t>();
		(void)_buf.extract<uint32_t>();
		(void)_buf.extract<uint64_t>();
		(void)_buf.extract<eRESULT>();
		(void)_buf.extract<std::string_view>();
	});
}

TEST(message_fuzz, request)
{
	MESSAGE_FUZZ_RELEASE_ONLY();

	sMESSAGE_FUZZ fuzz(13);
	context setting;
	setting.from_json("{ \"service\": { \"name\": \"fuzz\" }, \"port\": 21101, \"list\": [1, 2, 3] }");
	server_system::sSERVICE::sSETTING service_setting;
	const std::vector<std::string> seeds =
	{
		to_string(server_system::make_message_SERVER_ADMIN_REQUEST_START_SERVICE(21101, setting)),
		to_string(server_system::make_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(21101, 7, service_setting)),
	};

	// - host_agent socket_admin_server (REQUEST_START_SERVICE : context, REQUEST_UPDATE_SETTING : sSETTING)
	fuzz.run(seeds, [](buffer_view& _buf)
	{
		buffer_view buf_setting = _buf;
		(void)buf_setting.extract<server_system::iid_t>();
		(void)buf_setting.extract<server_system::iid_t>();
		(void)buf_setting.extract<server_system::sSERVICE::sSETTING>();

		(void)_buf.extract<server_system::iid_t>();
		(void)_buf.extract<context>();
	});
}

TEST(message_fuzz, context_json)
{
	sMESSAGE_FUZZ fuzz(14);
	const std::vector<std::string> seeds =
	{
		"{ \"service\": { \"name\": \"fuzz\", \"iid\": 21101 }, \"acceptor_control\": { \"local_endpoint\": { \"address\": \"127.0.0.1\", \"port\": 21888 } } }",
		"{ \"a\": [1, 2.5, -3e10, true, false, null, \"\\u0041\\n\"], \"b\": { \"c\": { \"d\": [] } } }",
		"[ { \"x\": 1 }, { \"y\": [ [ [ ] ] ] } ]",
		"{}",
	};

	for (std::size_t i = 0; i < fuzz.iterations; ++i)
	{
		const auto text = message_fuzz_mutate(fuzz.rng, seeds[fuzz.rng() % seeds.size()]);
		try
		{
			context value;
			value.from_json(text);
		}
		catch (...)
		{
		}
	}
}


//-----------------------------------------------------------------------------
// HOST_UPDATE_STATUS + web status (sHOST_WEB_STATUS) : round trip / old and new host agents
//-----------------------------------------------------------------------------
TEST(admin_message, host_web_status)
{
	SYSTEM::sSTATUS system_status;
	system_status.processor_usage = 1234;
	system_status.bytes_memory_usage = 5678;

	server_system::sHOST_WEB_STATUS web_status;
	web_status.web_connections = 1;
	web_status.web_timeout_request = 2;
	web_status.web_timeout_idle = 3;
	web_status.websocket_connections = 4;
	web_status.websocket_timeout_request = 5;
	web_status.websocket_timeout_idle = 6;

	auto f_read = [](const shared_buffer& _message)
	{
		buffer_view buf = _message;
		buf += offset(8);
		EXPECT_EQ(buf.extract<server_system::iid_t>(), 7u);
		auto status = server_system::extract_system_status(buf);
		EXPECT_EQ(status.processor_usage, 1234u);
		EXPECT_EQ(status.bytes_memory_usage, 5678u);
		return server_system::extract_host_web_status(buf);
	};

	// 1) with the web status
	EXPECT_EQ(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, system_status, &web_status).front<uint32_t>(), 8 + 8 + 8 * 6 + 4 + 8 * 6u);
	EXPECT_EQ(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, system_status).front<uint32_t>(), 8 + 8 + 8 * 6u);
	auto web = f_read(server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, system_status, &web_status));
	ASSERT_TRUE(web.has_value());
	EXPECT_EQ(web->web_connections, 1u);
	EXPECT_EQ(web->web_timeout_request, 2u);
	EXPECT_EQ(web->web_timeout_idle, 3u);
	EXPECT_EQ(web->websocket_connections, 4u);
	EXPECT_EQ(web->websocket_timeout_request, 5u);
	EXPECT_EQ(web->websocket_timeout_idle, 6u);

	// 2) old host agent (no web status)
	auto message_old = server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(7, system_status);
	EXPECT_FALSE(f_read(message_old).has_value());

	// 3) newer host agent (more fields : the unknown fields are skipped)
	{
		auto message_new = alloc_shared_buffer(message_old.size() + 4 + 8 * 8);
		message_new.append(message_old.size(), message_old.data());
		message_new.append<uint32_t>(8);
		for (uint64_t i = 1; i <= 8; ++i)
			message_new.append<uint64_t>(i * 10);
		auto web_new = f_read(message_new);
		ASSERT_TRUE(web_new.has_value());
		EXPECT_EQ(web_new->web_connections, 10u);
		EXPECT_EQ(web_new->websocket_timeout_idle, 60u);
	}

	// 4) broken (the count is larger than the data)
	{
		auto message_broken = alloc_shared_buffer(message_old.size() + 4 + 8 * 2);
		message_broken.append(message_old.size(), message_old.data());
		message_broken.append<uint32_t>(6);
		message_broken.append<uint64_t>(1);
		message_broken.append<uint64_t>(2);
		EXPECT_FALSE(f_read(message_broken).has_value());
	}
}

}
