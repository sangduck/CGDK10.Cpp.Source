#include "pch.h"
#include <random>
#include <vector>
#include <cstring>

//-----------------------------------------------------------------------------
// unit test - context::from_json()
//
//   1) members are applied in order (later key overwrites, objects are merged)
//   2) "@merge_keys_from_env" : merge the json object of an environment variable
//   3) "@add_keys_from_env"   : add the keys of the json object of an environment
//                               variable (existing key throws an exception)
//   4) "@del_key"             : delete keys
//   5) names of special keys and environment variables are case-insensitive
//-----------------------------------------------------------------------------
namespace CGDK
{
namespace
{
	// set an environment variable during the scope (removed at the end of scope)
	class scoped_env
	{
	public:
		scoped_env(std::string _name, const char* _value) : m_name(std::move(_name)) { set(m_name.c_str(), _value); }
		~scoped_env() { set(m_name.c_str(), nullptr); }

		static void set(const char* _name, const char* _value)
		{
		#if defined(_WIN32)
			_putenv_s(_name, (_value != nullptr) ? _value : "");
		#else
			if (_value != nullptr)
				setenv(_name, _value, 1);
			else
				unsetenv(_name);
		#endif
		}

	private:
		std::string m_name;
	};

	std::string str(const context& _context)
	{
		std::string value = _context;
		return value;
	}

	context load(std::string_view _json)
	{
		const std::string json(_json);
		context result;
		result.from_json(json);
		return result;
	}

	// return) description of exception (empty if no exception)
	std::string load_error(std::string_view _json)
	{
		try
		{
			load(_json);
		}
		catch (const throwable::failure& _e)
		{
			return std::string(_e.description());
		}
		catch (...)
		{
			return "<unknown exception>";
		}

		return {};
	}

	bool contains(const std::string& _text, std::string_view _sub)
	{
		return _text.find(_sub) != std::string::npos;
	}

	const char* JSON_SOCKET_TO_ADMIN = R"({
		"service": {
			"socket_to_admin": {
				"remote_endpoint": { "address": "192.168.0.83", "port": 10001 },
				"reconnection": true,
				"reconnection_interval": 5,
				"@merge_keys_from_env": "CGDK_UT_JSON_SOCKET_TO_ADMIN"
			}
		}
	})";
}

//-----------------------------------------------------------------------------
// 1) basic loading / order of members
//-----------------------------------------------------------------------------
TEST(CGDK10_object_context_json, load_basic)
{
	const auto c = load(R"({"a":1,"b":"text","c":true,"d":{"e":2},"f":[10,{"g":3}]})");

	EXPECT_EQ(str(c["a"]), "1");
	EXPECT_EQ(str(c["b"]), "text");
	EXPECT_EQ(str(c["c"]), "true");
	EXPECT_EQ(str(c["d"]["e"]), "2");
	EXPECT_EQ(str(c["f"]["0"]), "10");
	EXPECT_EQ(str(c["f"]["1"]["g"]), "3");
}

TEST(CGDK10_object_context_json, load_key_case_insensitive)
{
	const auto c = load(R"({"Remote_Endpoint":{"PORT":10001}})");

	EXPECT_EQ(str(c["remote_endpoint"]["port"]), "10001");
}

TEST(CGDK10_object_context_json, order_later_key_overwrites)
{
	const auto c = load(R"({"a":1,"a":2,"B":"x","b":"y"})");

	EXPECT_EQ(str(c["a"]), "2");
	EXPECT_EQ(str(c["b"]), "y");
}

TEST(CGDK10_object_context_json, order_object_merged)
{
	const auto c = load(R"({"o":{"x":1,"y":2},"o":{"y":3,"z":4}})");

	EXPECT_EQ(str(c["o"]["x"]), "1");
	EXPECT_EQ(str(c["o"]["y"]), "3");
	EXPECT_EQ(str(c["o"]["z"]), "4");
}

TEST(CGDK10_object_context_json, order_array_replaced)
{
	const auto c = load(R"({"x":[1,2,3],"x":[9]})");

	EXPECT_EQ(str(c["x"]["0"]), "9");
	EXPECT_FALSE(c["x"].has_key("1"));
	EXPECT_FALSE(c["x"].has_key("2"));
}

TEST(CGDK10_object_context_json, order_value_type_replaced)
{
	const auto c = load(R"({"o":{"k":1},"o":"s","p":"s","p":{"k":2}})");

	EXPECT_EQ(str(c["o"]), "s");
	EXPECT_FALSE(c["o"].has_key("k"));
	EXPECT_EQ(str(c["p"]["k"]), "2");
}

TEST(CGDK10_object_context_json, root_must_be_object)
{
	EXPECT_FALSE(load_error(R"([1,2,3])").empty());
	EXPECT_FALSE(load_error(R"("text")").empty());
}

//-----------------------------------------------------------------------------
// 2) "@merge_keys_from_env"
//-----------------------------------------------------------------------------
TEST(CGDK10_object_context_json, merge_env_not_set)
{
	scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", nullptr);

	const auto c = load(JSON_SOCKET_TO_ADMIN);
	const auto& s = c["service"]["socket_to_admin"];

	EXPECT_EQ(str(s["remote_endpoint"]["address"]), "192.168.0.83");
	EXPECT_EQ(str(s["remote_endpoint"]["port"]), "10001");
	EXPECT_EQ(str(s["reconnection_interval"]), "5");
	EXPECT_FALSE(s.has_key("@merge_keys_from_env"));
}

TEST(CGDK10_object_context_json, merge_env_empty)
{
	scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", "   ");

	const auto c = load(JSON_SOCKET_TO_ADMIN);

	EXPECT_EQ(str(c["service"]["socket_to_admin"]["remote_endpoint"]["address"]), "192.168.0.83");
}

TEST(CGDK10_object_context_json, merge_env_overwrites_only_same_keys)
{
	scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", R"({"remote_endpoint":{"address":"10.0.0.5"},"reconnection_interval":3,"new_key":"n"})");

	const auto c = load(JSON_SOCKET_TO_ADMIN);
	const auto& s = c["service"]["socket_to_admin"];

	EXPECT_EQ(str(s["remote_endpoint"]["address"]), "10.0.0.5");	// overwritten
	EXPECT_EQ(str(s["remote_endpoint"]["port"]), "10001");			// kept (nested merge)
	EXPECT_EQ(str(s["reconnection"]), "true");						// kept
	EXPECT_EQ(str(s["reconnection_interval"]), "3");				// overwritten
	EXPECT_EQ(str(s["new_key"]), "n");								// added
}

TEST(CGDK10_object_context_json, merge_env_position_order)
{
	scoped_env env("CGDK_UT_JSON_MERGE", R"({"a":"env","b":"env","o":{"x":"env","y":"env"}})");

	// - keys before '@merge_keys_from_env' are overwritten, keys after it overwrite
	const auto c = load(R"({"a":"file","o":{"x":"file"},"@merge_keys_from_env":"CGDK_UT_JSON_MERGE","b":"file","o":{"y":"file"}})");

	EXPECT_EQ(str(c["a"]), "env");
	EXPECT_EQ(str(c["b"]), "file");
	EXPECT_EQ(str(c["o"]["x"]), "env");
	EXPECT_EQ(str(c["o"]["y"]), "file");
}

TEST(CGDK10_object_context_json, merge_env_root)
{
	scoped_env env("CGDK_UT_JSON_ROOT", R"({"x":{"y":7},"w":3})");

	const auto c = load(R"({"w":1,"z":2,"@merge_keys_from_env":"CGDK_UT_JSON_ROOT"})");

	EXPECT_EQ(str(c["x"]["y"]), "7");
	EXPECT_EQ(str(c["w"]), "3");
	EXPECT_EQ(str(c["z"]), "2");
}

TEST(CGDK10_object_context_json, merge_env_in_array_element)
{
	scoped_env env("CGDK_UT_JSON_ITEM", R"({"b":"bb"})");

	const auto c = load(R"({"list":[{"a":1,"@merge_keys_from_env":"CGDK_UT_JSON_ITEM"},2]})");

	EXPECT_EQ(str(c["list"]["0"]["a"]), "1");
	EXPECT_EQ(str(c["list"]["0"]["b"]), "bb");
	EXPECT_EQ(str(c["list"]["1"]), "2");
}

TEST(CGDK10_object_context_json, merge_env_nested_reference)
{
	scoped_env env_a("CGDK_UT_JSON_NEST_A", R"({"a":1,"@merge_keys_from_env":"CGDK_UT_JSON_NEST_B"})");
	scoped_env env_b("CGDK_UT_JSON_NEST_B", R"({"b":2})");

	const auto c = load(R"({"@merge_keys_from_env":"CGDK_UT_JSON_NEST_A"})");

	EXPECT_EQ(str(c["a"]), "1");
	EXPECT_EQ(str(c["b"]), "2");
}

TEST(CGDK10_object_context_json, merge_env_errors)
{
	{
		scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", R"({"remote_endpoint": )");
		const auto error = load_error(JSON_SOCKET_TO_ADMIN);
		EXPECT_TRUE(contains(error, "invalid json")) << error;
		EXPECT_TRUE(contains(error, "CGDK_UT_JSON_SOCKET_TO_ADMIN")) << error;
	}
	{
		scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", R"("default")");
		const auto error = load_error(JSON_SOCKET_TO_ADMIN);
		EXPECT_TRUE(contains(error, "must be an object")) << error;
	}
	{
		scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", R"({"@merge_keys_from_env":"CGDK_UT_JSON_SOCKET_TO_ADMIN"})");
		const auto error = load_error(JSON_SOCKET_TO_ADMIN);
		EXPECT_TRUE(contains(error, "nested too deep")) << error;
	}
	{
		const auto error = load_error(R"({"a":1,"@merge_keys_from_env":3})");
		EXPECT_TRUE(contains(error, "must be a string")) << error;
	}
}

//-----------------------------------------------------------------------------
// 3) "@add_keys_from_env"
//-----------------------------------------------------------------------------
TEST(CGDK10_object_context_json, add_env_not_set)
{
	scoped_env env("CGDK_UT_JSON_ADD", nullptr);

	const auto c = load(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_EQ(str(c["a"]), "1");
	EXPECT_FALSE(c.has_key("@add_keys_from_env"));
}

TEST(CGDK10_object_context_json, add_env_adds_new_keys)
{
	scoped_env env("CGDK_UT_JSON_ADD", R"({"b":2,"o":{"x":1}})");

	const auto c = load(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_EQ(str(c["a"]), "1");
	EXPECT_EQ(str(c["b"]), "2");
	EXPECT_EQ(str(c["o"]["x"]), "1");
}

TEST(CGDK10_object_context_json, add_env_existing_key_throws)
{
	scoped_env env("CGDK_UT_JSON_ADD", R"({"b":2,"a":9})");

	const auto error = load_error(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_TRUE(contains(error, "already exists")) << error;
	EXPECT_TRUE(contains(error, "'a'")) << error;
	EXPECT_TRUE(contains(error, "CGDK_UT_JSON_ADD")) << error;
}

TEST(CGDK10_object_context_json, add_env_existing_object_key_throws)
{
	// - objects are not merged by '@add_keys_from_env'
	scoped_env env("CGDK_UT_JSON_ADD", R"({"o":{"y":2}})");

	const auto error = load_error(R"({"o":{"x":1},"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_TRUE(contains(error, "already exists")) << error;
}

TEST(CGDK10_object_context_json, add_env_existing_key_case_insensitive_throws)
{
	scoped_env env("CGDK_UT_JSON_ADD", R"({"PORT":2})");

	const auto error = load_error(R"({"Port":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_TRUE(contains(error, "already exists")) << error;
}

TEST(CGDK10_object_context_json, add_env_duplicated_key_in_env_throws)
{
	scoped_env env("CGDK_UT_JSON_ADD", R"({"b":1,"b":2})");

	const auto error = load_error(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_TRUE(contains(error, "already exists")) << error;
}

TEST(CGDK10_object_context_json, add_env_keys_after_follow_normal_rule)
{
	// - keys after '@add_keys_from_env' overwrite (normal rule)
	scoped_env env("CGDK_UT_JSON_ADD", R"({"b":2})");

	const auto c = load(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD","b":3})");

	EXPECT_EQ(str(c["b"]), "3");
}

TEST(CGDK10_object_context_json, add_env_with_del_key)
{
	// - '@del_key' in the json of environment variable deletes the key first
	scoped_env env("CGDK_UT_JSON_ADD", R"({"@del_key":"a","a":2})");

	const auto c = load(R"({"a":1,"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");

	EXPECT_EQ(str(c["a"]), "2");
}

TEST(CGDK10_object_context_json, add_env_errors)
{
	{
		scoped_env env("CGDK_UT_JSON_ADD", R"({"b": )");
		const auto error = load_error(R"({"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");
		EXPECT_TRUE(contains(error, "'@add_keys_from_env': invalid json")) << error;
	}
	{
		scoped_env env("CGDK_UT_JSON_ADD", R"([1,2])");
		const auto error = load_error(R"({"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");
		EXPECT_TRUE(contains(error, "must be an object")) << error;
	}
	{
		scoped_env env("CGDK_UT_JSON_ADD", R"({"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");
		const auto error = load_error(R"({"@add_keys_from_env":"CGDK_UT_JSON_ADD"})");
		EXPECT_TRUE(contains(error, "nested too deep")) << error;
	}
	{
		const auto error = load_error(R"({"@add_keys_from_env":true})");
		EXPECT_TRUE(contains(error, "must be a string")) << error;
	}
}

//-----------------------------------------------------------------------------
// 4) "@del_key"
//-----------------------------------------------------------------------------
TEST(CGDK10_object_context_json, del_key_string)
{
	const auto c = load(R"({"a":1,"b":2,"@del_key":"a"})");

	EXPECT_FALSE(c.has_key("a"));
	EXPECT_EQ(str(c["b"]), "2");
	EXPECT_FALSE(c.has_key("@del_key"));
}

TEST(CGDK10_object_context_json, del_key_array_case_insensitive)
{
	const auto c = load(R"({"a":1,"b":2,"c":3,"@del_key":["A","b"]})");

	EXPECT_FALSE(c.has_key("a"));
	EXPECT_FALSE(c.has_key("b"));
	EXPECT_EQ(str(c["c"]), "3");
}

TEST(CGDK10_object_context_json, del_key_order)
{
	// - a key after '@del_key' is added again
	const auto c = load(R"({"a":1,"@del_key":"a","a":2})");

	EXPECT_EQ(str(c["a"]), "2");
}

TEST(CGDK10_object_context_json, del_key_not_exist_ignored)
{
	const auto c = load(R"({"a":1,"@del_key":"zzz"})");

	EXPECT_EQ(str(c["a"]), "1");
}

TEST(CGDK10_object_context_json, del_key_in_env)
{
	scoped_env env("CGDK_UT_JSON_SOCKET_TO_ADMIN", R"({"@del_key":"reconnection_interval","remote_endpoint":{"@del_key":["port"]}})");

	const auto c = load(JSON_SOCKET_TO_ADMIN);
	const auto& s = c["service"]["socket_to_admin"];

	EXPECT_FALSE(s.has_key("reconnection_interval"));
	EXPECT_FALSE(s["remote_endpoint"].has_key("port"));
	EXPECT_EQ(str(s["remote_endpoint"]["address"]), "192.168.0.83");
	EXPECT_EQ(str(s["reconnection"]), "true");
}

TEST(CGDK10_object_context_json, del_key_errors)
{
	EXPECT_TRUE(contains(load_error(R"({"a":1,"@del_key":3})"), "must be a string or an array of strings"));
	EXPECT_TRUE(contains(load_error(R"({"a":1,"@del_key":["a",3]})"), "must be a string or an array of strings"));
}

//-----------------------------------------------------------------------------
// 5) case-insensitive names
//-----------------------------------------------------------------------------
TEST(CGDK10_object_context_json, special_key_case_insensitive)
{
	scoped_env env("CGDK_UT_JSON_CASE", R"({"b":2})");

	const auto c = load(R"({"a":1,"x":0,"@MERGE_KEYS_FROM_ENV":"CGDK_UT_JSON_CASE","@Del_Key":"x"})");

	EXPECT_EQ(str(c["b"]), "2");
	EXPECT_FALSE(c.has_key("x"));
}

TEST(CGDK10_object_context_json, env_name_case_insensitive)
{
	scoped_env env("CGDK_UT_JSON_CASE", R"({"b":2})");

	const auto c = load(R"({"@merge_keys_from_env":"cgdk_ut_json_Case"})");

	EXPECT_EQ(str(c["b"]), "2");
}

#if !defined(_WIN32)
TEST(CGDK10_object_context_json, env_name_exact_match_first)
{
	scoped_env env_1("CGDK_UT_JSON_Dup", R"({"v":1})");
	scoped_env env_2("CGDK_UT_JSON_DUP", R"({"v":2})");

	const auto c = load(R"({"@merge_keys_from_env":"CGDK_UT_JSON_DUP"})");

	EXPECT_EQ(str(c["v"]), "2");
}

TEST(CGDK10_object_context_json, env_name_ambiguous_throws)
{
	scoped_env env_1("CGDK_UT_JSON_Dup", R"({"v":1})");
	scoped_env env_2("CGDK_UT_JSON_DUP", R"({"v":2})");

	const auto error = load_error(R"({"@merge_keys_from_env":"cgdk_ut_json_dup"})");

	EXPECT_TRUE(contains(error, "ambiguous")) << error;
}
#endif

//-----------------------------------------------------------------------------
// unit test - charset conversion AVX2 ASCII fast-path (encode_string)
//   - the SIMD ASCII prefix must produce the same result as the scalar path,
//     across the 16/32 unit block boundary, and fall back on non-ASCII.
//   - the multibyte bytes are written explicitly so the source stays ASCII.
//-----------------------------------------------------------------------------
namespace
{
	inline std::string _u8_to_s(std::u8string_view _v) { return std::string(reinterpret_cast<const char*>(_v.data()), _v.size()); }
}

TEST(CGDK10_string_convert, utf8_wchar_roundtrip_ascii_boundary)
{
	// - ASCII of every length 0..80 (crosses the 16 and 32 unit SIMD blocks)
	for (size_t n = 0; n <= 80; ++n)
	{
		std::u8string s8;
		for (size_t i = 0; i < n; ++i)
			s8.push_back(static_cast<char8_t>('A' + (i % 26)));

		// utf-8 -> wchar
		wchar_t wbuf[256];
		auto w = encode_string(wbuf, std::u8string_view(s8), 256);
		ASSERT_EQ(w.size(), n) << "n=" << n;
		for (size_t i = 0; i < n; ++i)
			EXPECT_EQ(static_cast<uint32_t>(w[i]), static_cast<uint32_t>('A' + (i % 26))) << "n=" << n << " i=" << i;

		// wchar -> utf-8
		char8_t bbuf[256];
		auto b = encode_string(bbuf, std::wstring_view(w), 256);
		EXPECT_EQ(_u8_to_s(b), _u8_to_s(s8)) << "n=" << n;
	}
}

TEST(CGDK10_string_convert, ascii_prefix_then_multibyte)
{
	// - 0xEAB080=U+AC00, 0xEB8098=U+B098, 0xEB8BA4=U+B2E4 (hangul), 0xF09F9880=U+1F600 (emoji), then "xyz"
	static const unsigned char tail_bytes[] = { 0xEA,0xB0,0x80, 0xEB,0x82,0x98, 0xEB,0x8B,0xA4, 0xF0,0x9F,0x98,0x80, 'x','y','z' };
	const std::u8string tail(reinterpret_cast<const char8_t*>(tail_bytes), sizeof(tail_bytes));

	// - an ASCII run of every length 0..40 (crosses the blocks), then the multibyte tail (scalar path)
	for (size_t n = 0; n <= 40; ++n)
	{
		std::u8string s8(n, static_cast<char8_t>('Q'));
		s8 += tail;

		wchar_t wbuf[256];
		auto w = encode_string(wbuf, std::u8string_view(s8), 256);

		char8_t bbuf[512];
		auto b = encode_string(bbuf, std::wstring_view(w), 512);

		EXPECT_EQ(_u8_to_s(b), _u8_to_s(s8)) << "n=" << n;
	}
}

//-----------------------------------------------------------------------------
// unit test - additive checksum AVX2 (must equal the scalar sum, mod 2^32)
//-----------------------------------------------------------------------------
TEST(CGDK10_checksum, avx2_matches_scalar)
{
	std::mt19937 rng(12345);
	for (uint32_t n : { 0u, 1u, 7u, 8u, 15u, 16u, 31u, 32u, 33u, 64u, 100u, 1000u, 4096u, 65537u })
	{
		std::vector<uint8_t> buf(n ? n : 1);
		for (uint32_t i = 0; i < n; ++i) buf[i] = static_cast<uint8_t>(rng());

		// 8-bit
		uint32_t ref8 = 0; for (uint32_t i = 0; i < n; ++i) ref8 += buf[i];
		EXPECT_EQ(edc::get_checksum8(buf.data(), n), ref8) << "n=" << n;

		// 16-bit (n rounded down to a multiple of 2)
		const uint32_t len16 = n & ~1u;
		uint32_t ref16 = 0; for (uint32_t i = 0; i < (len16 >> 1); ++i) { uint16_t v; memcpy(&v, buf.data() + i * 2, 2); ref16 += v; }
		EXPECT_EQ(edc::get_checksum16(reinterpret_cast<uint16_t*>(buf.data()), len16), ref16) << "n=" << n;

		// 32-bit (n rounded down to a multiple of 4)
		const uint32_t len32 = n & ~3u;
		uint32_t ref32 = 0; for (uint32_t i = 0; i < (len32 >> 2); ++i) { uint32_t v; memcpy(&v, buf.data() + i * 4, 4); ref32 += v; }
		EXPECT_EQ(edc::get_checksum32(reinterpret_cast<uint32_t*>(buf.data()), len32), ref32) << "n=" << n;
	}
}

//-----------------------------------------------------------------------------
// unit test - MBCS <-> wide ASCII fast-path (AVX2) round trip
//-----------------------------------------------------------------------------
TEST(CGDK10_string_convert, mbcs_wide_ascii_roundtrip)
{
	for (size_t n = 0; n <= 80; ++n)
	{
		std::string s;
		for (size_t i = 0; i < n; ++i) s.push_back(static_cast<char>('a' + (i % 26)));

		wchar_t wbuf[256];
		auto w = encode_string(wbuf, std::string_view(s), 256);		// mbcs -> wide
		ASSERT_EQ(w.size(), n) << "n=" << n;
		for (size_t i = 0; i < n; ++i) EXPECT_EQ(static_cast<uint32_t>(w[i]), static_cast<uint32_t>('a' + (i % 26))) << "n=" << n << " i=" << i;

		char cbuf[256];
		auto c = encode_string(cbuf, std::wstring_view(w), 256);		// wide -> mbcs
		EXPECT_EQ(std::string(c), s) << "n=" << n;
	}
}

}
