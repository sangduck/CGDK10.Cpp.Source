#include "pch.h"
#include <charconv>

//-----------------------------------------------------------------------------
// desired state
//-----------------------------------------------------------------------------
std::string_view admin::to_desired_string(eDESIRED_STATE _state) noexcept
{
	switch (_state)
	{
	case	eDESIRED_STATE::RUNNING:	return "running"sv;
	case	eDESIRED_STATE::STOPPED:	return "stopped"sv;
	default:							return "none"sv;
	}
}

admin::eDESIRED_STATE admin::to_desired_state(std::string_view _string) noexcept
{
	if (_string == "running"sv)	return eDESIRED_STATE::RUNNING;
	if (_string == "stopped"sv)	return eDESIRED_STATE::STOPPED;
	return eDESIRED_STATE::NONE;
}

admin::eDESIRED_STATE admin::desired_state_store::get(server_system::iid_t _iid) const
{
	std::lock_guard lock(this->m_lockable);

	auto iter_find = this->m_map.find(_iid);
	return (iter_find != this->m_map.end()) ? iter_find->second : eDESIRED_STATE::NONE;
}

void admin::desired_state_store::set(server_system::iid_t _iid, eDESIRED_STATE _state)
{
	std::lock_guard lock(this->m_lockable);

	if (_state == eDESIRED_STATE::NONE)
		this->m_map.erase(_iid);
	else
		this->m_map[_iid] = _state;
}

std::size_t admin::desired_state_store::size() const
{
	std::lock_guard lock(this->m_lockable);
	return this->m_map.size();
}

void admin::desired_state_store::from_json(std::string_view _json)
{
	// 1) parse
	context context_desired;
	context_desired.from_json(_json);

	// 2) { "<iid>": "running"|"stopped" }
	std::map<server_system::iid_t, eDESIRED_STATE> map_desired;

	for (const auto& iter : context_desired.map_nodes)
	{
		// check) string value only
		if (iter.second.type() != context::value_type_t::string_)
			continue;

		auto iid = parse_iid(iter.first);
		auto state = to_desired_state(iter.second.operator std::string_view());

		if (iid != 0 && state != eDESIRED_STATE::NONE)
			map_desired[iid] = state;
	}

	// 3) set
	std::lock_guard lock(this->m_lockable);
	this->m_map = std::move(map_desired);
}

std::string admin::desired_state_store::to_json() const
{
	std::lock_guard lock(this->m_lockable);

	std::string str_json = "{\n";
	bool is_first = true;
	for (const auto& iter : this->m_map)
	{
		if (!is_first)
			str_json += ",\n";
		str_json += "  \"" + std::to_string(iter.first) + "\": \"" + std::string(to_desired_string(iter.second)) + "\"";
		is_first = false;
	}
	str_json += "\n}\n";

	return str_json;
}

void admin::desired_state_store::load_file(std::string_view _filename)
{
	// 1) read (파일이 없으면 비어 있다.)
	shared_buffer buf_file;
	try
	{
		buf_file = api::file::read_file(_filename);
	}
	catch (...)
	{
		std::lock_guard lock(this->m_lockable);
		this->m_map.clear();
		return;
	}

	// 2) parse
	this->from_json(std::string_view(buf_file.data<char>(), buf_file.size()));
}

bool admin::desired_state_store::save_file(std::string_view _filename) const
{
	// 1) json
	auto str_json = this->to_json();

	// 2) write (임시 파일에 쓰고 바꾼다. 쓰는 중에 죽어도 기존 파일은 남는다.)
	const std::string filename(_filename);
	const std::string filename_temp = filename + ".tmp";

	{
		std::ofstream file(filename_temp, std::ios::binary | std::ios::trunc);
		if (file.is_open() == false)
			return false;
		file << str_json;
		if (file.good() == false)
			return false;
	}

	std::error_code error_code;
	std::filesystem::rename(filename_temp, filename, error_code);
	if (error_code)
	{
		// - rename이 실패하면 (windows: 기존 파일) 지우고 다시 시도한다.
		std::filesystem::remove(filename, error_code);
		std::filesystem::rename(filename_temp, filename, error_code);
	}

	return !error_code;
}


//-----------------------------------------------------------------------------
// restart limiter
//-----------------------------------------------------------------------------
void admin::restart_limiter::set_policy(std::chrono::seconds _interval, int _max_count, std::chrono::seconds _window) noexcept
{
	std::lock_guard lock(this->m_lockable);

	this->m_interval = (_interval.count() < 0) ? std::chrono::seconds(0) : _interval;
	this->m_max_count = (_max_count < 0) ? 0 : _max_count;
	this->m_window = (_window.count() < 1) ? std::chrono::seconds(1) : _window;
}

admin::restart_limiter::eDECISION admin::restart_limiter::request(server_system::iid_t _iid, clock_t::time_point _now)
{
	std::lock_guard lock(this->m_lockable);

	auto& history = this->m_map[_iid];

	// check) suspended
	if (history.is_suspended)
		return eDECISION::SUSPENDED;

	// check) interval
	if (history.times.empty() == false && _now - history.times.back() < this->m_interval)
		return eDECISION::TOO_SOON;

	// 1) remove old history (out of window)
	while (history.times.empty() == false && _now - history.times.front() >= this->m_window)
		history.times.pop_front();

	// check) crash loop (max_count INITs in the window already)
	if (this->m_max_count > 0 && static_cast<int>(history.times.size()) >= this->m_max_count)
	{
		history.is_suspended = true;
		return eDECISION::CRASH_LOOP;
	}

	// 2) record
	history.times.push_back(_now);

	// return)
	return eDECISION::ALLOWED;
}

void admin::restart_limiter::reset(server_system::iid_t _iid)
{
	std::lock_guard lock(this->m_lockable);
	this->m_map.erase(_iid);
}

bool admin::restart_limiter::is_suspended(server_system::iid_t _iid) const
{
	std::lock_guard lock(this->m_lockable);

	auto iter_find = this->m_map.find(_iid);
	return iter_find != this->m_map.end() && iter_find->second.is_suspended;
}


//-----------------------------------------------------------------------------
// request tracker
//-----------------------------------------------------------------------------
uint64_t admin::request_tracker::add(server_system::iid_t _iid, uint32_t _request, clock_t::time_point _now)
{
	std::lock_guard lock(this->m_lockable);

	// - 0 is 'no request id'
	auto request_id = ++this->m_id_last;
	this->m_map[request_id] = sPENDING{ request_id, _iid, _request, _now };

	return request_id;
}

std::optional<admin::request_tracker::sPENDING> admin::request_tracker::complete(uint64_t _request_id)
{
	std::lock_guard lock(this->m_lockable);

	auto iter_find = this->m_map.find(_request_id);

	// check)
	if (iter_find == this->m_map.end())
		return std::nullopt;

	auto pending = iter_find->second;
	this->m_map.erase(iter_find);

	return pending;
}

std::vector<admin::request_tracker::sPENDING> admin::request_tracker::purge(std::chrono::seconds _timeout, clock_t::time_point _now)
{
	std::lock_guard lock(this->m_lockable);

	std::vector<sPENDING> vector_expired;
	for (auto iter = this->m_map.begin(); iter != this->m_map.end();)
	{
		if (_now - iter->second.time_request >= _timeout)
		{
			vector_expired.push_back(iter->second);
			iter = this->m_map.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return vector_expired;
}

std::size_t admin::request_tracker::size() const
{
	std::lock_guard lock(this->m_lockable);
	return this->m_map.size();
}


//-----------------------------------------------------------------------------
// auth fail limiter
//-----------------------------------------------------------------------------
void admin::auth_fail_limiter::set_policy(int _max_fail, std::chrono::seconds _window, std::chrono::seconds _block_time) noexcept
{
	std::lock_guard lock(this->m_lockable);
	this->m_max_fail = _max_fail;
	this->m_window = _window;
	this->m_block_time = _block_time;
}

std::chrono::seconds admin::auth_fail_limiter::get_blocked(const std::string& _address, clock_t::time_point _now) const
{
	std::lock_guard lock(this->m_lockable);

	auto iter = this->m_map.find(_address);
	if (iter == this->m_map.end() || iter->second.time_block_end <= _now)
		return std::chrono::seconds(0);

	// - 남은 시간 (올림)
	return std::chrono::ceil<std::chrono::seconds>(iter->second.time_block_end - _now);
}

bool admin::auth_fail_limiter::on_fail(const std::string& _address, clock_t::time_point _now)
{
	std::lock_guard lock(this->m_lockable);

	// check) 제한 없음
	if (this->m_max_fail <= 0)
		return false;

	// 1) 오래된 기록을 지운다. (주소가 많아지지 않도록 다른 주소도 정리한다.)
	for (auto iter = this->m_map.begin(); iter != this->m_map.end();)
	{
		auto& times = iter->second.times;
		while (times.empty() == false && times.front() + this->m_window <= _now)
			times.pop_front();

		if (times.empty() && iter->second.time_block_end <= _now && iter->first != _address)
			iter = this->m_map.erase(iter);
		else
			++iter;
	}

	// 2) 기록
	auto& history = this->m_map[_address];
	history.times.push_back(_now);

	// 3) 넘으면 막는다.
	if (static_cast<int>(history.times.size()) < this->m_max_fail)
		return false;

	history.times.clear();
	history.time_block_end = _now + this->m_block_time;
	return true;
}

void admin::auth_fail_limiter::on_success(const std::string& _address)
{
	std::lock_guard lock(this->m_lockable);
	this->m_map.erase(_address);
}

std::size_t admin::auth_fail_limiter::size() const
{
	std::lock_guard lock(this->m_lockable);
	return this->m_map.size();
}


//-----------------------------------------------------------------------------
// console command
//-----------------------------------------------------------------------------
admin::sCOMMAND admin::parse_command(std::string_view _command_line)
{
	sCOMMAND result;

	std::string word;
	auto f_push = [&]()
	{
		if (word.empty())
			return;

		if (result.command.empty())
		{
			std::transform(word.begin(), word.end(), word.begin(), [](unsigned char _ch) { return static_cast<char>(std::tolower(_ch)); });
			result.command = std::move(word);
		}
		else
		{
			result.args.push_back(std::move(word));
		}
		word.clear();
	};

	for (auto ch : _command_line)
	{
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
			f_push();
		else
			word.push_back(ch);
	}
	f_push();

	return result;
}

server_system::iid_t admin::parse_iid(std::string_view _arg) noexcept
{
	uint64_t value = 0;
	auto result = std::from_chars(_arg.data(), _arg.data() + _arg.size(), value);

	// check) all characters must be digits
	if (result.ec != std::errc() || result.ptr != _arg.data() + _arg.size())
		return 0;

	return static_cast<server_system::iid_t>(value);
}


uint32_t admin::to_request_message(std::string_view _name) noexcept
{
	if (_name == "init"sv)		return eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE;
	if (_name == "destroy"sv)	return eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE;
	if (_name == "kill"sv)		return eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE;
	if (_name == "start"sv)		return eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE;
	if (_name == "stop"sv)		return eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE;
	if (_name == "pause"sv)		return eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE;
	if (_name == "continue"sv)	return eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE;
	return 0;
}

std::string_view admin::to_request_name(uint32_t _request) noexcept
{
	switch (_request)
	{
	case	eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE:		return "INIT"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE:	return "DESTROY"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE:		return "KILL"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE:		return "START"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE:		return "STOP"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE:		return "PAUSE"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE:	return "CONTINUE"sv;
	case	eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING:	return "UPDATE_SETTING"sv;
	default:													return "UNKNOWN"sv;
	}
}


//-----------------------------------------------------------------------------
// control api route
//-----------------------------------------------------------------------------
admin::sCONTROL_ROUTE admin::parse_control_route(std::string_view _method, std::string_view _path)
{
	sCONTROL_ROUTE route;

	// 1) remove query string / trailing '/'
	if (auto pos = _path.find('?'); pos != std::string_view::npos)
		_path = _path.substr(0, pos);
	while (_path.size() > 1 && _path.back() == '/')
		_path.remove_suffix(1);

	// 2) split
	std::vector<std::string_view> segments;
	while (_path.empty() == false)
	{
		if (_path.front() == '/')
		{
			_path.remove_prefix(1);
			continue;
		}
		auto pos = _path.find('/');
		segments.push_back(_path.substr(0, pos));
		_path = (pos == std::string_view::npos) ? std::string_view() : _path.substr(pos);
	}

	const bool is_get = (_method == "GET"sv);
	const bool is_post = (_method == "POST"sv);
	const bool is_put = (_method == "PUT"sv);

	auto f_method = [&](bool _is_allowed, eCONTROL_ACTION _action)
	{
		route.action = _is_allowed ? _action : eCONTROL_ACTION::METHOD_NOT_ALLOWED;
		return route;
	};

	// 3) /metrics
	if (segments.size() == 1 && segments[0] == "metrics"sv)
		return f_method(is_get, eCONTROL_ACTION::METRICS);

	// check) /api/...
	if (segments.size() < 2 || segments[0] != "api"sv)
		return route;

	// 4) /api/reload
	if (segments.size() == 2 && segments[1] == "reload"sv)
		return f_method(is_post, eCONTROL_ACTION::RELOAD);

	// 5) /api/hosts
	if (segments.size() == 2 && segments[1] == "hosts"sv)
		return f_method(is_get, eCONTROL_ACTION::HOSTS);

	// check) /api/instances...
	if (segments[1] != "instances"sv)
		return route;

	// 5) /api/instances
	if (segments.size() == 2)
		return f_method(is_get, eCONTROL_ACTION::LIST);

	// 6) /api/instances/<iid>...
	route.iid = parse_iid(segments[2]);
	if (route.iid == 0)
	{
		route.action = eCONTROL_ACTION::NOT_FOUND;
		return route;
	}

	if (segments.size() == 3)
		return f_method(is_get, eCONTROL_ACTION::GET);

	if (segments.size() == 4)
	{
		static const std::string_view requests[] = { "start"sv, "stop"sv, "init"sv, "destroy"sv, "kill"sv, "pause"sv, "continue"sv };
		if (std::find(std::begin(requests), std::end(requests), segments[3]) == std::end(requests))
			return route;

		route.argument = std::string(segments[3]);
		return f_method(is_post, eCONTROL_ACTION::REQUEST);
	}

	if (segments.size() == 5 && segments[3] == "desired"sv)
	{
		if (segments[4] != "running"sv && segments[4] != "stopped"sv && segments[4] != "none"sv)
			return route;

		route.argument = std::string(segments[4]);
		return f_method(is_put, eCONTROL_ACTION::DESIRED);
	}

	// return) not found
	return route;
}

std::string admin::to_json_string(std::string_view _value)
{
	std::string result;
	result.reserve(_value.size() + 2);
	result.push_back('"');
	for (const char ch : _value)
	{
		switch (ch)
		{
		case	'"':	result += "\\\""; break;
		case	'\\':	result += "\\\\"; break;
		case	'\n':	result += "\\n"; break;
		case	'\r':	result += "\\r"; break;
		case	'\t':	result += "\\t"; break;
		default:
				if (static_cast<unsigned char>(ch) < 0x20)
				{
					constexpr char hex[] = "0123456789abcdef";
					result += "\\u00";
					result.push_back(hex[(ch >> 4) & 0x0f]);
					result.push_back(hex[ch & 0x0f]);
				}
				else
				{
					result.push_back(ch);
				}
				break;
		}
	}
	result.push_back('"');
	return result;
}

std::string admin::to_prometheus_label(std::string_view _value)
{
	std::string result;
	result.reserve(_value.size());
	for (const char ch : _value)
	{
		switch (ch)
		{
		case	'"':	result += "\\\""; break;
		case	'\\':	result += "\\\\"; break;
		case	'\n':	result += "\\n"; break;
		default:		result.push_back(ch); break;
		}
	}
	return result;
}


//-----------------------------------------------------------------------------
// sha256 / hmac-sha256 (FIPS 180-4, RFC 2104)
//-----------------------------------------------------------------------------
namespace
{
	struct sSHA256
	{
		uint32_t	state[8]{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
		uint8_t		block[64]{};
		std::size_t	size_block{ 0 };
		uint64_t	size_total{ 0 };

		static uint32_t rotr(uint32_t _x, int _n) noexcept { return (_x >> _n) | (_x << (32 - _n)); }

		void transform() noexcept
		{
			static constexpr uint32_t K[64] =
			{
				0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
				0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
				0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
				0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
				0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
				0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
				0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
				0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
			};

			uint32_t w[64];
			for (int i = 0; i < 16; ++i)
				w[i] = (uint32_t(this->block[i * 4]) << 24) | (uint32_t(this->block[i * 4 + 1]) << 16) | (uint32_t(this->block[i * 4 + 2]) << 8) | uint32_t(this->block[i * 4 + 3]);
			for (int i = 16; i < 64; ++i)
			{
				uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
				uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
				w[i] = w[i - 16] + s0 + w[i - 7] + s1;
			}

			uint32_t a = this->state[0], b = this->state[1], c = this->state[2], d = this->state[3];
			uint32_t e = this->state[4], f = this->state[5], g = this->state[6], h = this->state[7];
			for (int i = 0; i < 64; ++i)
			{
				uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
				uint32_t ch = (e & f) ^ (~e & g);
				uint32_t temp1 = h + S1 + ch + K[i] + w[i];
				uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
				uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
				uint32_t temp2 = S0 + maj;
				h = g; g = f; f = e; e = d + temp1;
				d = c; c = b; b = a; a = temp1 + temp2;
			}
			this->state[0] += a; this->state[1] += b; this->state[2] += c; this->state[3] += d;
			this->state[4] += e; this->state[5] += f; this->state[6] += g; this->state[7] += h;
		}

		void update_byte(uint8_t _byte) noexcept
		{
			this->block[this->size_block++] = _byte;
			if (this->size_block == 64)
			{
				this->transform();
				this->size_block = 0;
			}
		}

		void update(std::string_view _data) noexcept
		{
			for (char ch : _data)
				this->update_byte(static_cast<uint8_t>(ch));
			this->size_total += _data.size();
		}

		std::array<uint8_t, 32> finish() noexcept
		{
			const uint64_t size_bits = this->size_total * 8;
			this->update_byte(0x80);
			while (this->size_block != 56)
				this->update_byte(0);
			for (int i = 7; i >= 0; --i)
				this->update_byte(static_cast<uint8_t>(size_bits >> (i * 8)));

			std::array<uint8_t, 32> digest{};
			for (int i = 0; i < 8; ++i)
			{
				digest[i * 4] = static_cast<uint8_t>(this->state[i] >> 24);
				digest[i * 4 + 1] = static_cast<uint8_t>(this->state[i] >> 16);
				digest[i * 4 + 2] = static_cast<uint8_t>(this->state[i] >> 8);
				digest[i * 4 + 3] = static_cast<uint8_t>(this->state[i]);
			}
			return digest;
		}
	};

	std::array<uint8_t, 32> _sha256(std::string_view _data) noexcept
	{
		sSHA256 sha;
		sha.update(_data);
		return sha.finish();
	}

	std::string _to_hex(const std::array<uint8_t, 32>& _digest)
	{
		constexpr char hex[] = "0123456789abcdef";
		std::string result;
		result.reserve(64);
		for (auto iter : _digest)
		{
			result.push_back(hex[iter >> 4]);
			result.push_back(hex[iter & 0x0f]);
		}
		return result;
	}

	// - 길이가 같을 때 모든 byte를 비교한다. (timing attack. 대소문자 무시)
	bool _is_equal_constant_time(std::string_view _lhs, std::string_view _rhs) noexcept
	{
		if (_lhs.size() != _rhs.size())
			return false;
		unsigned int diff = 0;
		for (std::size_t i = 0; i < _lhs.size(); ++i)
			diff |= static_cast<unsigned int>(std::tolower(static_cast<unsigned char>(_lhs[i])) ^ std::tolower(static_cast<unsigned char>(_rhs[i])));
		return diff == 0;
	}
}

std::string admin::sha256_hex(std::string_view _data)
{
	return _to_hex(_sha256(_data));
}

std::string admin::hmac_sha256_hex(std::string_view _key, std::string_view _data)
{
	// 1) key (64 byte block. 길면 hash한다.)
	uint8_t key_block[64]{};
	if (_key.size() > 64)
	{
		auto digest = _sha256(_key);
		std::copy(digest.begin(), digest.end(), key_block);
	}
	else
	{
		std::transform(_key.begin(), _key.end(), key_block, [](char _ch) { return static_cast<uint8_t>(_ch); });
	}

	// 2) inner / outer
	char ipad[64];
	char opad[64];
	for (int i = 0; i < 64; ++i)
	{
		ipad[i] = static_cast<char>(key_block[i] ^ 0x36);
		opad[i] = static_cast<char>(key_block[i] ^ 0x5c);
	}

	sSHA256 inner;
	inner.update(std::string_view(ipad, 64));
	inner.update(_data);
	auto digest_inner = inner.finish();

	sSHA256 outer;
	outer.update(std::string_view(opad, 64));
	outer.update(std::string_view(reinterpret_cast<const char*>(digest_inner.data()), digest_inner.size()));
	return _to_hex(outer.finish());
}

std::string admin::make_signature_message(std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body)
{
	std::string message;
	message.reserve(_method.size() + _path.size() + _timestamp.size() + 64 + 3);
	message.append(_method).append("\n").append(_path).append("\n").append(_timestamp).append("\n").append(sha256_hex(_body));
	return message;
}

std::string admin::make_signature(std::string_view _key, std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body)
{
	return hmac_sha256_hex(_key, make_signature_message(_method, _path, _timestamp, _body));
}

admin::signature_verifier::eRESULT_VERIFY admin::signature_verifier::verify(std::string_view _key, std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body, std::string_view _signature, int64_t _now_unix)
{
	// 1) timestamp
	int64_t timestamp = 0;
	auto [ptr, ec] = std::from_chars(_timestamp.data(), _timestamp.data() + _timestamp.size(), timestamp);
	if (_timestamp.empty() || ec != std::errc() || ptr != _timestamp.data() + _timestamp.size())
		return eRESULT_VERIFY::INVALID_TIMESTAMP;

	std::lock_guard lock(this->m_lockable);

	const int64_t skew = this->m_allowed_skew.count();
	if (timestamp < _now_unix - skew || timestamp > _now_unix + skew)
		return eRESULT_VERIFY::INVALID_TIMESTAMP;

	// 2) signature
	if (_is_equal_constant_time(make_signature(_key, _method, _path, _timestamp, _body), _signature) == false)
		return eRESULT_VERIFY::INVALID_SIGNATURE;

	// 3) replay (allowed_skew 안에서 같은 서명은 한 번만 받는다. 지난 것은 지운다.)
	for (auto iter = this->m_map_used.begin(); iter != this->m_map_used.end();)
		iter = (iter->second < _now_unix - skew) ? this->m_map_used.erase(iter) : std::next(iter);

	std::string signature(_signature);
	std::transform(signature.begin(), signature.end(), signature.begin(), [](unsigned char _ch) { return static_cast<char>(std::tolower(_ch)); });
	if (this->m_map_used.emplace(std::move(signature), timestamp).second == false)
		return eRESULT_VERIFY::REPLAYED;

	return eRESULT_VERIFY::OK;
}

std::size_t admin::signature_verifier::size() const
{
	std::lock_guard lock(this->m_lockable);
	return this->m_map_used.size();
}

std::string_view admin::to_verify_string(signature_verifier::eRESULT_VERIFY _result) noexcept
{
	switch (_result)
	{
	case	signature_verifier::eRESULT_VERIFY::OK:					return "ok"sv;
	case	signature_verifier::eRESULT_VERIFY::INVALID_TIMESTAMP:	return "invalid timestamp"sv;
	case	signature_verifier::eRESULT_VERIFY::INVALID_SIGNATURE:	return "invalid signature"sv;
	case	signature_verifier::eRESULT_VERIFY::REPLAYED:			return "replayed"sv;
	default:														return "unknown"sv;
	}
}
