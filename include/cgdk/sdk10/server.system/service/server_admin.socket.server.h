//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
#include <random>
#include <optional>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <ctime>

namespace CGDK
{
// ----------------------------------------------------------------------------
//
// server_system::socket.server (server -> admin)
//
// 
// ----------------------------------------------------------------------------
class server_system::host::socket :
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::socket::tcp<>,
	public						Igroupable<server_system::host::socket>,
	public						Nmessage_transmitter
{
public:
			socket();
	virtual ~socket();

			void				register_host_instance(host::Ninstance* _host_instance);
			bool				unregister_host_instance(server_system::iid_t _iid) noexcept;
			auto				unregister_host_instance_all() noexcept ->std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>>;
			void				register_service_instance(service::Ninstance* _service_instance);
			bool				unregister_service_instance(server_system::iid_t _iid) noexcept;
			auto				unregister_service_instance_all() noexcept  ->std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>>;
protected:
	virtual void				on_connect () override;
	virtual void				on_disconnect (uint64_t _disconnect_reason) override;
	virtual result_code			on_message (sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_SERVER_ADMIN_CERTIFY_REQUEST (sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST (sMESSAGE_NETWORK& _msg);

	// - CERTIFY_REQUEST를 받을지 결정한다. (false면 거부하고 접속을 끊는다.)
	virtual	bool				process_certify(const sCERTIFY_INFO& /*_certify_info*/, std::string_view /*_certify_key*/) { return true; }

protected:
	mutable	lock_object			m_lockable_instances;
			std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>> m_map_host_instance;
			std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> m_map_service_instance;
			friend class server_system::host::group_socket;
};


// ----------------------------------------------------------------------------
//
// server_system::sACCESS_POLICY
//
//  - admin server/host agent의 접속 허용 정책
//    "certify_key"       : CERTIFY_REQUEST에 같은 key를 보낸 접속만 받는다. (없으면 환경변수 CGDK_ADMIN_CERTIFY_KEY)
//    "allowed_addresses" : [ "127.0.0.1", "192.168.0.10", ... ] 이 주소에서 온 접속만 받는다. (없으면 모두)
//
// ----------------------------------------------------------------------------
namespace server_system {
struct sACCESS_POLICY
{
	std::string				certify_key;
	std::vector<net::ip::address> allowed_addresses;

	// 주의) context의 문자열 변환을 header에서 instantiate하면 안 된다. (특수화보다 먼저 instantiate되어 모든 문자열 변환이 깨진다.)
	void					load(context& _context);
	[[nodiscard]] bool		is_secured() const noexcept { return !this->certify_key.empty() || !this->allowed_addresses.empty(); }
	[[nodiscard]] bool		is_address_allowed(const net::ip::address& _address) const
	{
		// check) 목록이 없으면 모두 허용한다.
		if (this->allowed_addresses.empty())
			return true;

		auto address = normalize(_address);
		return std::any_of(this->allowed_addresses.begin(), this->allowed_addresses.end(), [&](const net::ip::address& _allowed) { return is_same(_allowed, address); });
	}
	[[nodiscard]] bool		is_key_matched(std::string_view _certify_key) const noexcept { return is_certify_key_matched(this->certify_key, _certify_key); }

	// - 주소 비교 (v4는 값, v6는 byte만 비교한다. scope id는 비교하지 않는다.)
	[[nodiscard]] static bool is_same(const net::ip::address& _lhs, const net::ip::address& _rhs)
	{
		if (_lhs.is_v4() && _rhs.is_v4())
			return _lhs.to_v4().to_uint() == _rhs.to_v4().to_uint();
		if (_lhs.is_v6() && _rhs.is_v6())
			return _lhs.to_v6().to_bytes() == _rhs.to_v6().to_bytes();
		return false;
	}

	// - ipv4-mapped ipv6(::ffff:a.b.c.d)는 ipv4로 바꿔서 비교한다.
	[[nodiscard]] static net::ip::address normalize(const net::ip::address& _address)
	{
		if (_address.is_v6() && _address.to_v6().is_v4_mapped())
			return net::ip::address(net::ip::make_address_v4(net::ip::v4_mapped_t::v4_mapped, _address.to_v6()));
		return _address;
	}

	// - 허용 여부 (secured가 아니면 loopback만 허용할지 선택한다.)
	[[nodiscard]] bool		is_address_allowed_or_loopback(const net::ip::address& _address) const
	{
		return this->is_secured() ? this->is_address_allowed(_address) : normalize(_address).is_loopback();
	}
};

//-----------------------------------------------------------------------------
// access_policy_holder
//  - 실행 중에 정책을 바꿀 수 있다. (reload)
//    get()은 그 시점의 정책(불변)을 돌려준다. set()은 새 정책으로 바꾼다.
//-----------------------------------------------------------------------------
class access_policy_holder
{
public:
	[[nodiscard]] std::shared_ptr<const sACCESS_POLICY> get() const
	{
		std::lock_guard lock(this->m_lockable);
		return this->m_ppolicy;
	}
			void			set(sACCESS_POLICY&& _policy)
	{
		auto ppolicy = std::make_shared<const sACCESS_POLICY>(std::move(_policy));
		std::lock_guard lock(this->m_lockable);
		this->m_ppolicy = std::move(ppolicy);
	}
			void			load(context& _context)
	{
		sACCESS_POLICY policy;
		policy.load(_context);
		this->set(std::move(policy));
	}

private:
	mutable	std::mutex		m_lockable;
			std::shared_ptr<const sACCESS_POLICY> m_ppolicy{ std::make_shared<const sACCESS_POLICY>() };
};

//-----------------------------------------------------------------------------
// web ui (host_agent web/websocket) : Host / Origin 검사
//
//  - 브라우저는 다른 site의 page에서도 ws://127.0.0.1:22053 로 접속할 수 있다.
//    (websocket은 same-origin 정책을 따르지 않는다.) 그래서 Origin을 검사한다.
//  - DNS rebinding(공격자 domain이 127.0.0.1을 가리키게 하는 것)을 막기 위해
//    Host/Origin의 host는 ip 주소나 localhost만 받는다. (이름을 쓰려면 allowed_origins에 넣는다.)
//
//  is_web_host_allowed   : http Host header ("127.0.0.1:22052")
//  is_web_origin_allowed : websocket Origin header ("http://127.0.0.1:22052")
//    1) Origin이 없으면 허용 (브라우저가 아닌 client. 주소 정책은 따로 적용된다.)
//    2) allowed_origins에 있으면 허용 (대소문자 무시)
//    3) http/https, host가 ip/localhost, port가 web ui port, host가 Host header의 host와 같으면 허용
//-----------------------------------------------------------------------------
struct sWEB_HOST
{
	std::string_view	host;			// - '[', ']'를 뺀 host
	uint16_t			port{ 0 };
	bool				is_valid{ false };
};

[[nodiscard]] inline bool _is_iequal(std::string_view _lhs, std::string_view _rhs) noexcept
{
	return _lhs.size() == _rhs.size() && std::equal(_lhs.begin(), _lhs.end(), _rhs.begin(), [](char _a, char _b) { return std::tolower(static_cast<unsigned char>(_a)) == std::tolower(static_cast<unsigned char>(_b)); });
}

// - "host[:port]" / "[v6][:port]" (port가 없으면 _default_port)
[[nodiscard]] inline sWEB_HOST parse_web_host(std::string_view _value, uint16_t _default_port) noexcept
{
	sWEB_HOST result;

	// check)
	if (_value.empty())
		return result;

	std::string_view str_port;
	if (_value.front() == '[')
	{
		auto pos = _value.find(']');
		if (pos == std::string_view::npos)
			return result;
		result.host = _value.substr(1, pos - 1);
		auto rest = _value.substr(pos + 1);
		if (rest.empty() == false)
		{
			if (rest.front() != ':')
				return result;
			str_port = rest.substr(1);
		}
	}
	else
	{
		auto pos = _value.rfind(':');
		if (pos != std::string_view::npos && _value.find(':') != pos)
			return result;			// - '[' 없는 v6는 받지 않는다.
		result.host = _value.substr(0, pos);
		if (pos != std::string_view::npos)
			str_port = _value.substr(pos + 1);
	}

	// - port
	if (str_port.empty())
	{
		result.port = _default_port;
	}
	else
	{
		uint32_t port = 0;
		if (str_port.size() > 5)
			return result;
		for (char ch : str_port)
		{
			if (ch < '0' || ch > '9')
				return result;
			port = port * 10 + static_cast<uint32_t>(ch - '0');
		}
		if (port == 0 || port > 65535)
			return result;
		result.port = static_cast<uint16_t>(port);
	}

	result.is_valid = (result.host.empty() == false);
	return result;
}

// - ip 주소(v4 dotted / v6)나 localhost인가?
[[nodiscard]] inline bool is_ip_literal_or_localhost(std::string_view _host) noexcept
{
	if (_is_iequal(_host, "localhost"))
		return true;

	// - v6
	if (_host.find(':') != std::string_view::npos)
		return std::all_of(_host.begin(), _host.end(), [](char _ch) { return std::isxdigit(static_cast<unsigned char>(_ch)) || _ch == ':' || _ch == '.'; });

	// - v4 (a.b.c.d)
	int count_dot = 0;
	int count_digit = 0;
	uint32_t value = 0;
	for (char ch : _host)
	{
		if (ch == '.')
		{
			if (count_digit == 0)
				return false;
			++count_dot;
			count_digit = 0;
			value = 0;
		}
		else if (ch >= '0' && ch <= '9')
		{
			value = value * 10 + static_cast<uint32_t>(ch - '0');
			if (++count_digit > 3 || value > 255)
				return false;
		}
		else
		{
			return false;
		}
	}
	return count_dot == 3 && count_digit > 0;
}

[[nodiscard]] inline bool is_web_host_allowed(std::string_view _host_header, const std::vector<std::string>& _allowed_origins) noexcept
{
	// check) Host가 없으면 (HTTP/1.0) 허용
	if (_host_header.empty())
		return true;

	auto host = parse_web_host(_host_header, 80);
	if (host.is_valid == false)
		return false;

	// 1) ip/localhost
	if (is_ip_literal_or_localhost(host.host))
		return true;

	// 2) allowed_origins의 host
	return std::any_of(_allowed_origins.begin(), _allowed_origins.end(), [&](const std::string& _origin)
	{
		std::string_view origin = _origin;
		auto pos = origin.find("://");
		if (pos == std::string_view::npos)
			return false;
		auto allowed = parse_web_host(origin.substr(pos + 3), 80);
		return allowed.is_valid && _is_iequal(allowed.host, host.host);
	});
}

[[nodiscard]] inline bool is_web_origin_allowed(std::string_view _origin, std::string_view _host_header, uint16_t _web_port, const std::vector<std::string>& _allowed_origins) noexcept
{
	// 1) Origin이 없으면 (브라우저가 아닌 client)
	if (_origin.empty())
		return true;

	// 2) allowed_origins
	if (std::any_of(_allowed_origins.begin(), _allowed_origins.end(), [&](const std::string& _allowed) { return _is_iequal(_allowed, _origin); }))
		return true;

	// 3) scheme
	uint16_t port_default = 0;
	std::string_view rest;
	if (_origin.substr(0, 7) == "http://")
	{
		port_default = 80;
		rest = _origin.substr(7);
	}
	else if (_origin.substr(0, 8) == "https://")
	{
		port_default = 443;
		rest = _origin.substr(8);
	}
	else
	{
		return false;		// - "null" 등
	}

	// 4) host (ip/localhost만) / port (web ui port)
	auto origin = parse_web_host(rest, port_default);
	if (origin.is_valid == false || is_ip_literal_or_localhost(origin.host) == false || origin.port != _web_port)
		return false;

	// 5) Host header의 host와 같아야 한다. (같은 host의 web ui page에서 연 websocket)
	auto host = parse_web_host(_host_header, 80);
	return host.is_valid && _is_iequal(origin.host, host.host);
}

//-----------------------------------------------------------------------------
// web ui login (cookie / form)
//-----------------------------------------------------------------------------
// - 'Cookie: a=1; cgdk_session=xxxx' 에서 이름의 값 (없으면 빈 문자열)
[[nodiscard]] inline std::string_view get_cookie_value(std::string_view _cookie_header, std::string_view _name) noexcept
{
	while (_cookie_header.empty() == false)
	{
		auto pos_end = _cookie_header.find(';');
		auto item = _cookie_header.substr(0, pos_end);
		_cookie_header = (pos_end == std::string_view::npos) ? std::string_view() : _cookie_header.substr(pos_end + 1);

		// - trim
		while (item.empty() == false && item.front() == ' ')
			item.remove_prefix(1);
		while (item.empty() == false && item.back() == ' ')
			item.remove_suffix(1);

		auto pos_equal = item.find('=');
		if (pos_equal != std::string_view::npos && item.substr(0, pos_equal) == _name)
			return item.substr(pos_equal + 1);
	}
	return std::string_view();
}

// - application/x-www-form-urlencoded 의 값 ('key=a%20b&x=1'. '+'는 공백. 없으면 std::nullopt)
[[nodiscard]] inline std::optional<std::string> get_form_value(std::string_view _body, std::string_view _name)
{
	auto f_hex = [](char _ch) -> int
	{
		if (_ch >= '0' && _ch <= '9') return _ch - '0';
		if (_ch >= 'a' && _ch <= 'f') return _ch - 'a' + 10;
		if (_ch >= 'A' && _ch <= 'F') return _ch - 'A' + 10;
		return -1;
	};

	while (_body.empty() == false)
	{
		auto pos_end = _body.find('&');
		auto item = _body.substr(0, pos_end);
		_body = (pos_end == std::string_view::npos) ? std::string_view() : _body.substr(pos_end + 1);

		auto pos_equal = item.find('=');
		if (item.substr(0, pos_equal) != _name)
			continue;

		std::string value;
		auto encoded = (pos_equal == std::string_view::npos) ? std::string_view() : item.substr(pos_equal + 1);
		for (std::size_t i = 0; i < encoded.size(); ++i)
		{
			if (encoded[i] == '+')
			{
				value.push_back(' ');
			}
			else if (encoded[i] == '%' && i + 2 < encoded.size() && f_hex(encoded[i + 1]) >= 0 && f_hex(encoded[i + 2]) >= 0)
			{
				value.push_back(static_cast<char>(f_hex(encoded[i + 1]) * 16 + f_hex(encoded[i + 2])));
				i += 2;
			}
			else
			{
				value.push_back(encoded[i]);
			}
		}
		return value;
	}
	return std::nullopt;
}

//-----------------------------------------------------------------------------
// web_session_store (web ui login)
//  - create()   : 새 session token (32 byte random. hex 64자). login한 주소에 묶는다.
//  - is_valid() : token이 있고, 같은 주소이고, 만료(lifetime)되지 않았고, idle_time 안에 쓴 적이 있나? (쓰면 마지막 사용 시각을 바꾼다.)
//    (TLS를 쓰지 않으므로 cookie를 가로채도 다른 주소에서는 쓸 수 없게 한다.)
//  - 같은 주소에서 login을 max_fail번 틀리면 block_time 동안 막는다.
//-----------------------------------------------------------------------------
class web_session_store
{
public:
	using clock_t = std::chrono::steady_clock;

public:
			void			set_policy(std::chrono::seconds _lifetime, int _max_fail = 5, std::chrono::seconds _block_time = std::chrono::seconds(300), std::chrono::seconds _idle_time = std::chrono::seconds(1800)) noexcept
	{
		std::lock_guard lock(this->m_lockable);
		this->m_lifetime = _lifetime;
		this->m_max_fail = _max_fail;
		this->m_block_time = _block_time;
		this->m_idle_time = _idle_time;
	}

	[[nodiscard]] std::string create(std::string_view _address, clock_t::time_point _now = clock_t::now())
	{
		// - token
		std::random_device random_device;
		constexpr char hex[] = "0123456789abcdef";
		std::string token;
		token.reserve(64);
		for (int i = 0; i < 8; ++i)
		{
			auto value = random_device();
			for (int j = 0; j < 8; ++j)
			{
				token.push_back(hex[value & 0x0f]);
				value >>= 4;
			}
		}

		std::lock_guard lock(this->m_lockable);

		// - 만료된 session을 지운다.
		for (auto iter = this->m_map_session.begin(); iter != this->m_map_session.end();)
			iter = this->_is_expired(iter->second, _now) ? this->m_map_session.erase(iter) : std::next(iter);

		this->m_map_session[token] = sSESSION{ _now + this->m_lifetime, _now, std::string(_address) };
		return token;
	}

	[[nodiscard]] bool		is_valid(std::string_view _token, std::string_view _address, clock_t::time_point _now = clock_t::now())
	{
		if (_token.size() != 64)
			return false;

		std::lock_guard lock(this->m_lockable);
		auto iter = this->m_map_session.find(std::string(_token));
		if (iter == this->m_map_session.end())
			return false;

		// check) 만료 (지운다.)
		if (this->_is_expired(iter->second, _now))
		{
			this->m_map_session.erase(iter);
			return false;
		}

		// check) 다른 주소 (cookie를 가로채 다른 곳에서 쓰는 것)
		if (iter->second.address != _address)
			return false;

		// - 마지막 사용 시각
		iter->second.time_last = _now;
		return true;
	}

			void			remove(std::string_view _token)
	{
		std::lock_guard lock(this->m_lockable);
		this->m_map_session.erase(std::string(_token));
	}

	[[nodiscard]] std::size_t size() const
	{
		std::lock_guard lock(this->m_lockable);
		return this->m_map_session.size();
	}

	// - login 실패 제한
	[[nodiscard]] bool		is_blocked(const std::string& _address, clock_t::time_point _now = clock_t::now()) const
	{
		std::lock_guard lock(this->m_lockable);
		auto iter = this->m_map_fail.find(_address);
		return iter != this->m_map_fail.end() && iter->second.time_block_end > _now;
	}
			bool			on_fail(const std::string& _address, clock_t::time_point _now = clock_t::now())
	{
		std::lock_guard lock(this->m_lockable);
		if (this->m_max_fail <= 0)
			return false;

		auto& fail = this->m_map_fail[_address];
		if (fail.time_block_end != clock_t::time_point{} && fail.time_block_end <= _now)
			fail = sFAIL{};
		if (++fail.count < this->m_max_fail)
			return false;

		fail.count = 0;
		fail.time_block_end = _now + this->m_block_time;
		return true;
	}
			void			on_success(const std::string& _address)
	{
		std::lock_guard lock(this->m_lockable);
		this->m_map_fail.erase(_address);
	}

private:
	struct sSESSION
	{
		clock_t::time_point	time_expire{};
		clock_t::time_point	time_last{};
		std::string			address;
	};
	struct sFAIL
	{
		int					count{ 0 };
		clock_t::time_point	time_block_end{};
	};

	[[nodiscard]] bool		_is_expired(const sSESSION& _session, clock_t::time_point _now) const noexcept
	{
		return _session.time_expire <= _now || (this->m_idle_time.count() > 0 && _session.time_last + this->m_idle_time <= _now);
	}

	mutable	std::mutex		m_lockable;
			std::map<std::string, sSESSION> m_map_session;
			std::map<std::string, sFAIL> m_map_fail;
			std::chrono::seconds m_lifetime{ 12 * 3600 };
			int				m_max_fail{ 5 };
			std::chrono::seconds m_block_time{ 300 };
			std::chrono::seconds m_idle_time{ 1800 };
};

//-----------------------------------------------------------------------------
// log_throttle
//  - 같은 경고를 interval마다 한 번만 남긴다. (잘못된 메시지를 계속 보내는 상대 : log가 넘치지 않도록)
//  - try_log() : 남길 차례면 그 사이 남기지 않은 수를 돌려준다. (아니면 std::nullopt)
//  - 예) static log_throttle s_log(std::chrono::seconds(10));
//        if (auto suppressed = s_log.try_log(); suppressed.has_value()) LOG_WARNING << "..." << " (" << *suppressed << " suppressed)";
//-----------------------------------------------------------------------------
class log_throttle
{
public:
	using clock_t = std::chrono::steady_clock;

			explicit log_throttle(std::chrono::nanoseconds _interval) noexcept : m_interval(_interval.count()) {}

	[[nodiscard]] std::optional<uint64_t> try_log(clock_t::time_point _now = clock_t::now()) noexcept
	{
		const int64_t now = _now.time_since_epoch().count();
		int64_t last = this->m_last.load();

		// check) interval 안 : 세기만 한다.
		if (last != 0 && now - last < this->m_interval)
		{
			++this->m_suppressed;
			return std::nullopt;
		}

		// check) 다른 thread가 먼저 남겼다.
		if (this->m_last.compare_exchange_strong(last, now) == false)
		{
			++this->m_suppressed;
			return std::nullopt;
		}

		return this->m_suppressed.exchange(0);
	}

private:
			int64_t			m_interval;
			std::atomic<int64_t> m_last{ 0 };
			std::atomic<uint64_t> m_suppressed{ 0 };
};

//-----------------------------------------------------------------------------
// rate_limiter (token bucket)
//  - 초당 rate개, 최대 burst개까지 몰아서 허용한다. (rate 0: 제한 없음)
//  - 예) websocket 메시지 (host_agent "acceptor_websocket": "max_messages_per_second")
//-----------------------------------------------------------------------------
class rate_limiter
{
public:
	using clock_t = std::chrono::steady_clock;

public:
			void			set_rate(double _rate_per_second, double _burst) noexcept
	{
		this->m_rate = _rate_per_second;
		this->m_burst = (_burst > 0.0) ? _burst : _rate_per_second;
		this->m_tokens = this->m_burst;
		this->m_time_last = clock_t::time_point{};
	}

	[[nodiscard]] bool		try_acquire(clock_t::time_point _now = clock_t::now()) noexcept
	{
		// check) 제한 없음
		if (this->m_rate <= 0.0)
			return true;

		// 1) 지난 시간만큼 채운다.
		if (this->m_time_last != clock_t::time_point{})
		{
			auto elapsed = std::chrono::duration<double>(_now - this->m_time_last).count();
			if (elapsed > 0.0)
				this->m_tokens = (std::min)(this->m_burst, this->m_tokens + elapsed * this->m_rate);
		}
		this->m_time_last = _now;

		// 2) 하나 쓴다.
		if (this->m_tokens < 1.0)
			return false;
		this->m_tokens -= 1.0;
		return true;
	}

private:
			double			m_rate{ 0.0 };
			double			m_burst{ 0.0 };
			double			m_tokens{ 0.0 };
			clock_t::time_point m_time_last{};
};

//-----------------------------------------------------------------------------
// audit_log (JSON lines)
//  - 운영 동작(제어 요청, login 등)을 한 줄에 json 하나씩 별도 파일에 남긴다. (log 수집기/SIEM용)
//    {"time":"2026-10-11T12:00:00.123Z","source":"control","address":"10.0.0.1","action":"POST /api/instances/1/start","result":200,"detail":"..."}
//  - 파일이 없으면(설정하지 않으면) 남기지 않는다. 매번 열고 닫는다. (logrotate copytruncate/move 모두 된다.)
//-----------------------------------------------------------------------------
class audit_log
{
public:
			void			set_file(std::string _filename)
	{
		std::lock_guard lock(this->m_lockable);
		this->m_filename = std::move(_filename);
	}
	[[nodiscard]] std::string get_file() const
	{
		std::lock_guard lock(this->m_lockable);
		return this->m_filename;
	}

	// - json string (quoted and escaped)
	[[nodiscard]] static std::string to_json_string(std::string_view _value)
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

	// - 한 줄 (time은 UTC ISO 8601)
	[[nodiscard]] static std::string make_line(std::chrono::system_clock::time_point _time, std::string_view _source, std::string_view _address, std::string_view _action, int _result, std::string_view _detail)
	{
		auto time_t_now = std::chrono::system_clock::to_time_t(_time);
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(_time.time_since_epoch()).count() % 1000;
		std::tm tm_utc{};
	#if defined(_WIN32)
		gmtime_s(&tm_utc, &time_t_now);
	#else
		gmtime_r(&time_t_now, &tm_utc);
	#endif
		char str_time[96];
		std::snprintf(str_time, sizeof(str_time), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday, tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec, static_cast<int>(milliseconds));

		std::string line = "{\"time\":\"";
		line += str_time;
		line += "\",\"source\":" + to_json_string(_source);
		line += ",\"address\":" + to_json_string(_address);
		line += ",\"action\":" + to_json_string(_action);
		line += ",\"result\":" + std::to_string(_result);
		line += ",\"detail\":" + to_json_string(_detail);
		line += "}";
		return line;
	}

			bool			write(std::string_view _source, std::string_view _address, std::string_view _action, int _result, std::string_view _detail = {})
	{
		auto line = make_line(std::chrono::system_clock::now(), _source, _address, _action, _result, _detail);

		std::lock_guard lock(this->m_lockable);
		if (this->m_filename.empty())
			return false;

		// - directory
		std::error_code ec;
		auto path = std::filesystem::path(this->m_filename);
		if (path.has_parent_path())
			std::filesystem::create_directories(path.parent_path(), ec);

		std::ofstream file(path, std::ios::app | std::ios::binary);
		if (!file)
			return false;
		file << line << '\n';
		return static_cast<bool>(file);
	}

private:
	mutable	std::mutex		m_lockable;
			std::string		m_filename;
};

//-----------------------------------------------------------------------------
// http server socket : 시간/연결 제한 (admin control api, host_agent web ui)
//
//  - request_timeout : 접속 후 이 시간 안에 첫 요청이 끝나지 않으면 끊는다. (천천히 보내면서 접속을 잡아 두는 것(slowloris))
//  - idle_timeout    : 이 시간 동안 받은 것이 없으면 끊는다. (0: 끊지 않음)
//  - max_connections : acceptor가 동시에 받는 접속 수 (넘으면 끝날 때까지 받지 않는다(backlog))
//  - ping_interval   : (websocket) 이 시간 동안 받은 것이 없으면 PING을 보낸다. 두 배의 시간 동안 받은 것이 없으면(PONG도 없음) 끊는다. (0: 보내지 않음)
//  setting) "acceptor_xxx": { "request_timeout": 10, "idle_timeout": 60, "max_connections": 64, "ping_interval": 30 }
//
//  - count_xxx : 시간 제한으로 끊은 수 (/metrics)
//-----------------------------------------------------------------------------
struct sHTTP_ACCEPTOR_LIMITS
{
	std::atomic<int64_t>	request_timeout{ 10 };
	std::atomic<int64_t>	idle_timeout{ 60 };
	std::atomic<int64_t>	max_connections{ 64 };
	std::atomic<int64_t>	ping_interval{ 0 };

	net::io::add_on::Nsocket_sustain::sTIMEOUT_COUNT count_timeout;	// - certify : request_timeout, idle : idle_timeout / ping

			sHTTP_ACCEPTOR_LIMITS() = default;
			sHTTP_ACCEPTOR_LIMITS(int64_t _request_timeout, int64_t _idle_timeout, int64_t _max_connections, int64_t _ping_interval = 0) : request_timeout(_request_timeout), idle_timeout(_idle_timeout), max_connections(_max_connections), ping_interval(_ping_interval) {}

	// - socket sustain의 시간 제한 (Nsocket_sustain::set_sustain_timeout. 0이면 쓰지 않는다)
	[[nodiscard]] net::io::add_on::Nsocket_sustain::sTIMEOUT to_sustain_timeout() const noexcept
	{
		net::io::add_on::Nsocket_sustain::sTIMEOUT timeout;
		timeout.certify = std::chrono::seconds(this->request_timeout.load());
		timeout.idle = std::chrono::seconds(this->idle_timeout.load());
		timeout.ping_interval = std::chrono::seconds(this->ping_interval.load());
		return timeout;
	}

	// - setting을 읽는다. (없는 값은 그대로 둔다.)
			void			load(context& _context)
	{
		auto f_get = [&](std::string_view _name, std::atomic<int64_t>& _value)
		{
			if (auto& context_value = _context.find(_name); context_value.is_valid())
				_value = (std::max)(int64_t(0), static_cast<int64_t>(context_value));
		};
		f_get("request_timeout", this->request_timeout);
		f_get("idle_timeout", this->idle_timeout);
		f_get("max_connections", this->max_connections);
		f_get("ping_interval", this->ping_interval);
	}
};

}

}