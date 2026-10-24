//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin - Admin Server                     *
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
//-----------------------------------------------------------------------------
//
// admin server policies (independent of network. unit tested)
//
//  1) desired_state_store : service별 원하는 상태(running/stopped) 저장소 (json 파일)
//  2) restart_limiter     : 자동 재시작(INIT) 간격/횟수 제한 (crash loop 감지)
//  3) request_tracker     : REQUEST_xxx의 request id와 결과(REQUEST_RESULT)를 맞춘다.
//  4) parse_command       : console 명령 한 줄을 나눈다.
//
//-----------------------------------------------------------------------------
namespace admin
{
	//-------------------------------------------------------------------------
	// desired state
	//-------------------------------------------------------------------------
	enum class eDESIRED_STATE : int
	{
		NONE		= 0,	// 관리하지 않는다.
		RUNNING		= 1,	// 항상 실행 (STOPPED면 start, NONE(process 종료)이면 init)
		STOPPED		= 2,	// 실행하지 않는다. (auto_start도 하지 않는다.)
	};
	[[nodiscard]] std::string_view to_desired_string(eDESIRED_STATE _state) noexcept;
	[[nodiscard]] eDESIRED_STATE to_desired_state(std::string_view _string) noexcept;

	class desired_state_store
	{
	public:
		[[nodiscard]] eDESIRED_STATE get(server_system::iid_t _iid) const;
				void			set(server_system::iid_t _iid, eDESIRED_STATE _state);
		[[nodiscard]] std::size_t size() const;

		// - json: { "<iid>": "running"|"stopped", ... }
				void			from_json(std::string_view _json);
		[[nodiscard]] std::string to_json() const;

		// - file (load: 파일이 없으면 비어 있다. save: 실패하면 false)
				void			load_file(std::string_view _filename);
				bool			save_file(std::string_view _filename) const;

	private:
		mutable	std::mutex		m_lockable;
				std::map<server_system::iid_t, eDESIRED_STATE> m_map;
	};

	//-------------------------------------------------------------------------
	// restart limiter (auto INIT)
	//  - interval   : 같은 service에 INIT을 다시 보내기까지의 최소 간격
	//  - max_count  : window 안에서 허용하는 INIT 횟수. 넘으면 crash loop로 보고 중지한다.
	//                 (0이면 횟수 제한 없음)
	//  - 중지된 service는 운영자가 start/init/want running 하면(reset) 다시 시작한다.
	//-------------------------------------------------------------------------
	class restart_limiter
	{
	public:
		using clock_t = std::chrono::steady_clock;

		enum class eDECISION
		{
			ALLOWED,			// - INIT을 보낸다.
			TOO_SOON,			// - interval이 지나지 않았다.
			SUSPENDED,			// - crash loop로 중지되어 있다.
			CRASH_LOOP,			// - 이번에 crash loop로 판정되어 중지되었다.
		};

	public:
				void			set_policy(std::chrono::seconds _interval, int _max_count, std::chrono::seconds _window) noexcept;
		[[nodiscard]] auto		get_interval() const noexcept { return this->m_interval; }
		[[nodiscard]] int		get_max_count() const noexcept { return this->m_max_count; }
		[[nodiscard]] auto		get_window() const noexcept { return this->m_window; }

		[[nodiscard]] eDECISION	request(server_system::iid_t _iid, clock_t::time_point _now = clock_t::now());
				void			reset(server_system::iid_t _iid);
		[[nodiscard]] bool		is_suspended(server_system::iid_t _iid) const;

	private:
		struct sHISTORY
		{
			std::deque<clock_t::time_point> times;
			bool			is_suspended{ false };
		};

		mutable	std::mutex		m_lockable;
				std::map<server_system::iid_t, sHISTORY> m_map;
				std::chrono::seconds m_interval{ 30 };
				int				m_max_count{ 5 };
				std::chrono::seconds m_window{ 300 };
	};

	//-------------------------------------------------------------------------
	// request tracker
	//-------------------------------------------------------------------------
	class request_tracker
	{
	public:
		using clock_t = std::chrono::steady_clock;

		struct sPENDING
		{
			uint64_t		request_id{ 0 };
			server_system::iid_t iid{ 0 };
			uint32_t		request{ 0 };
			clock_t::time_point time_request;
		};

	public:
		[[nodiscard]] uint64_t	add(server_system::iid_t _iid, uint32_t _request, clock_t::time_point _now = clock_t::now());
		[[nodiscard]] std::optional<sPENDING> complete(uint64_t _request_id);
		[[nodiscard]] std::vector<sPENDING> purge(std::chrono::seconds _timeout, clock_t::time_point _now = clock_t::now());
		[[nodiscard]] std::size_t size() const;

	private:
		mutable	std::mutex		m_lockable;
				std::map<uint64_t, sPENDING> m_map;
				uint64_t		m_id_last{ 0 };
	};

	//-------------------------------------------------------------------------
	// auth fail limiter (control api)
	//  - 같은 주소에서 window 안에 key가 max_fail번 틀리면 block_time 동안 막는다. (key 대입 공격)
	//  - 성공하면 그 주소의 실패 횟수를 지운다.
	//-------------------------------------------------------------------------
	class auth_fail_limiter
	{
	public:
		using clock_t = std::chrono::steady_clock;

	public:
				void			set_policy(int _max_fail, std::chrono::seconds _window, std::chrono::seconds _block_time) noexcept;

		// - 막혀 있으면 남은 시간 (막혀 있지 않으면 0)
		[[nodiscard]] std::chrono::seconds get_blocked(const std::string& _address, clock_t::time_point _now = clock_t::now()) const;
		// - 실패 (return: 이번에 막혔으면 true)
				bool			on_fail(const std::string& _address, clock_t::time_point _now = clock_t::now());
				void			on_success(const std::string& _address);
		[[nodiscard]] std::size_t size() const;

	private:
		struct sHISTORY
		{
			std::deque<clock_t::time_point> times;
			clock_t::time_point	time_block_end{};
		};

		mutable	std::mutex		m_lockable;
				std::map<std::string, sHISTORY> m_map;
				int				m_max_fail{ 5 };
				std::chrono::seconds m_window{ 60 };
				std::chrono::seconds m_block_time{ 300 };
	};

	//-------------------------------------------------------------------------
	// console command
	//  - 공백으로 나눈다. 첫 단어(명령)는 소문자로 바꾼다.
	//-------------------------------------------------------------------------
	struct sCOMMAND
	{
		std::string					command;
		std::vector<std::string>	args;
	};
	[[nodiscard]] sCOMMAND parse_command(std::string_view _command_line);

	// - iid 인자 (숫자가 아니면 0)
	[[nodiscard]] server_system::iid_t parse_iid(std::string_view _arg) noexcept;

	// - request 이름 <-> message (start/stop/init/destroy/kill/pause/continue. 모르면 0 / "UNKNOWN")
	[[nodiscard]] uint32_t to_request_message(std::string_view _name) noexcept;
	[[nodiscard]] std::string_view to_request_name(uint32_t _request) noexcept;

	//-------------------------------------------------------------------------
	// control api (http)
	//
	//   GET  /api/hosts                              : host 목록 (system status)
	//   GET  /api/instances                          : service instance 목록
	//   GET  /api/instances/<iid>                    : service instance 상세 (status)
	//   POST /api/instances/<iid>/<request>          : start|stop|init|destroy|kill|pause|continue
	//   PUT  /api/instances/<iid>/desired/<state>    : running|stopped|none
	//   POST /api/reload                             : setting 파일을 다시 읽는다.
	//   GET  /metrics                                : prometheus text format
	//-------------------------------------------------------------------------
	enum class eCONTROL_ACTION
	{
		NOT_FOUND,			// - 404
		METHOD_NOT_ALLOWED,	// - 405
		LIST,
		HOSTS,
		GET,
		REQUEST,
		DESIRED,
		RELOAD,
		METRICS,
	};

	struct sCONTROL_ROUTE
	{
		eCONTROL_ACTION		action{ eCONTROL_ACTION::NOT_FOUND };
		server_system::iid_t iid{ 0 };
		std::string			argument;		// - request name or desired state
	};
	[[nodiscard]] sCONTROL_ROUTE parse_control_route(std::string_view _method, std::string_view _path);

	// - json string (quoted and escaped)
	[[nodiscard]] std::string to_json_string(std::string_view _value);

	// - prometheus label value (escaped, not quoted)
	[[nodiscard]] std::string to_prometheus_label(std::string_view _value);

	//-------------------------------------------------------------------------
	// control api request signature (HMAC-SHA256)
	//  - TLS를 쓰지 않으므로 key를 그대로 보내지 않고 요청마다 서명한다.
	//    'X-CGDK-Timestamp: <unix time(초)>'
	//    'X-CGDK-Signature: hex(HMAC-SHA256(key, METHOD + "\n" + path + "\n" + timestamp + "\n" + hex(SHA256(body))))'
	//  - 시각이 allowed_skew(기본 60초)를 넘게 차이 나거나 같은 서명을 다시 쓰면(replay) 거부한다.
	//  - 예) deploy/tools/cgdk_control.sh, cgdk_control.ps1
	//-------------------------------------------------------------------------
	[[nodiscard]] std::string sha256_hex(std::string_view _data);
	[[nodiscard]] std::string hmac_sha256_hex(std::string_view _key, std::string_view _data);
	[[nodiscard]] std::string make_signature_message(std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body);
	[[nodiscard]] std::string make_signature(std::string_view _key, std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body);

	class signature_verifier
	{
	public:
		enum class eRESULT_VERIFY
		{
			OK,
			INVALID_TIMESTAMP,		// - 숫자가 아니거나 시각 차이가 크다.
			INVALID_SIGNATURE,		// - 서명이 다르다.
			REPLAYED,				// - 이미 쓴 서명이다.
		};

	public:
				void			set_allowed_skew(std::chrono::seconds _skew) noexcept { std::lock_guard lock(this->m_lockable); this->m_allowed_skew = _skew; }
		[[nodiscard]] eRESULT_VERIFY verify(std::string_view _key, std::string_view _method, std::string_view _path, std::string_view _timestamp, std::string_view _body, std::string_view _signature, int64_t _now_unix);
		[[nodiscard]] std::size_t size() const;

	private:
		mutable	std::mutex		m_lockable;
				std::map<std::string, int64_t> m_map_used;		// - signature -> timestamp
				std::chrono::seconds m_allowed_skew{ 60 };
	};
	[[nodiscard]] std::string_view to_verify_string(signature_verifier::eRESULT_VERIFY _result) noexcept;
}
