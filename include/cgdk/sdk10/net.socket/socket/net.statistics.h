//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::io::statistics
//
// 1. net::io::statistics은~
//    1) socket관련된 각종 통계처리를 위한 구조체와 클래스들이다.
//    2) 통계를 위한 구조체
//       - CONNECTIVE_DWORD/CONNECTIVE_QWORD 
//       - TRAFFIC/TRAFFIC_QWORD
//       - connective_info
//       - traffic_info
//    3) 클래스 객체를 위한 클래스
//       - Nconnective
//       - Nsocket
//
//-----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// i/o statistics
// ----------------------------------------------------------------------------
extern	int						g_statistics_time;

// 1) acceptor Info
namespace statistics
{

struct unit_connective;
struct unit_connective_snap;
struct unit_traffic;
struct unit_traffic_snap;

struct unit_connective : public Ibuffer_serializable
{
public:
	chrono::tick::time_point	tick_snap = (chrono::tick::time_point::min)();

	std::atomic<uint64_t>		count_try = 0;				// 접속시도 회수.
	std::atomic<uint64_t>		count_success_connect = 0;	// 접속성공 회수.
	std::atomic<uint64_t>		count_fail_connect = 0;		// 접속성공 회수.
	std::atomic<uint64_t>		count_disconnect = 0;		// 접속종료 회수.
	std::atomic<uint64_t>		count_error_disconnect = 0;	// 오류로 인한 접속 종료 회수.
	std::atomic<uint64_t>		count_keep = 0;				// 현 접속수.
	std::atomic<uint64_t>		count_async = 0;			// Overlapped I/O횟수

public:
	unit_connective() noexcept {}
	unit_connective(const unit_connective& _copy) noexcept;
	unit_connective& operator =(const unit_connective& _rhs) noexcept;
	unit_connective& operator =(const unit_connective_snap& _rhs) noexcept;
	void reset() noexcept;

	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;
};

struct unit_connective_snap
{
	chrono::tick::time_point	tick_snap = (chrono::tick::time_point::min)();

	uint64_t					count_try = 0;
	uint64_t					count_success_connect = 0;
	uint64_t					count_fail_connect = 0;
	uint64_t					count_disconnect = 0;
	uint64_t					count_error_disconnect = 0;
	uint64_t					count_keep = 0;
	uint64_t					count_async = 0;

	unit_connective_snap& operator =(const unit_connective& _rhs) noexcept;
};

// 2) unit traffic
struct unit_traffic : public Ibuffer_serializable
{
public:
	chrono::tick::time_point	tick_snap = (chrono::tick::time_point::min)();

	std::atomic<uint64_t>		count_received_message = 0;	// 받은 message 수.
	std::atomic<uint64_t>		count_received_bytes = 0;	// 받은 Bytes

	std::atomic<uint64_t>		count_sent_message = 0;	// 보낸 message 수.
	std::atomic<uint64_t>		count_sent_bytes = 0;		// 보낸 Bytes

	std::atomic<uint64_t>		count_send_io = 0;			// 송신 I/O횟수
	std::atomic<uint64_t>		count_receive_io = 0;		// 수신 I/O횟수
	std::atomic<uint64_t>		count_general_io = 0;		// 일반 송수신 I/O수

	std::atomic<uint64_t>		count_error_on_send = 0;	// send시 Error
	std::atomic<uint64_t>		count_error_on_receive = 0;	// receive시 Error

public:
	unit_traffic() noexcept {}
	unit_traffic(const unit_traffic& _rhs) noexcept;
	unit_traffic& operator=(const unit_traffic& _rhs) noexcept;
	unit_traffic& operator=(const unit_traffic_snap& _rhs) noexcept;
	void reset() noexcept;

	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;
};

// 2) unit traffic snap
struct unit_traffic_snap
{
	chrono::tick::time_point	tick_snap = (chrono::tick::time_point::min)();

	uint64_t					count_received_message = 0;	// 받은 message 수.
	uint64_t					count_received_bytes = 0;	// 받은 Bytes

	uint64_t					count_sent_message = 0;	// 보낸 message 수.
	uint64_t					count_sent_bytes = 0;		// 보낸 Bytes

	uint64_t					count_send_io = 0;			// 송신 I/O횟수
	uint64_t					count_receive_io = 0;		// 수신 I/O횟수
	uint64_t					count_general_io = 0;		// 일반 송수신 I/O수

	uint64_t					count_error_on_send = 0;	// send시 Error
	uint64_t					count_error_on_receive = 0;	// receive시 Error

	unit_traffic_snap& operator = (const unit_traffic& _rhs) noexcept;
	void						reset() noexcept;
};


}


class net::io::statistics::Nconnective
{
// constructor/destructor)
public:
			Nconnective() noexcept;
			~Nconnective() noexcept;

// publics)
public:
			void				reset_statistics() noexcept;

	[[nodiscard]] auto			get_connective_id() const noexcept { return this->m_connective_id;}
																							  
	[[nodiscard]] const auto&	statistics_get_connection_info() const noexcept { return this->m_statistics_connection;}
																							  
	[[nodiscard]] auto			statistics_get_try() const noexcept { return this->m_statistics_connection.count_try.load();}
	[[nodiscard]] auto			statistics_get_success_connect() const noexcept { return this->m_statistics_connection.count_success_connect.load();}
	[[nodiscard]] auto			statistics_get_fail_connect() const noexcept { return this->m_statistics_connection.count_fail_connect.load();}
	[[nodiscard]] auto			statistics_get_disconnect() const noexcept { return this->m_statistics_connection.count_disconnect.load();}
	[[nodiscard]] auto			statistics_get_error_disconnect() const noexcept { return this->m_statistics_connection.count_error_disconnect.load();}
	[[nodiscard]] auto			statistics_get_keep() const noexcept { return this->m_statistics_connection.count_keep.load(); }
	[[nodiscard]] auto			statistics_get_now_async_io() const noexcept { return this->m_count_async.load(std::memory_order_acquire);}
																							  
	[[nodiscard]] auto			statistics_get_create_time() const noexcept { return this->m_time_create;}
	[[nodiscard]] auto			statistics_get_start_time() const noexcept { return this->m_time_start;}
	[[nodiscard]] auto			statistics_get_stop_time() const noexcept { return this->m_time_stop;}
																							  
	// - 통계 값 : relaxed (순서를 맞출 다른 data가 없다. x86 비용은 같고 arm64에서 barrier가 없다.)
			void				statistics_on_try() noexcept { this->m_statistics_connection.count_try.fetch_add(1, std::memory_order_relaxed); m_statistics_connection_total.count_try.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_success_connect() noexcept { this->m_statistics_connection.count_success_connect.fetch_add(1, std::memory_order_relaxed); this->m_statistics_connection.count_keep.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_fail_connect() noexcept { this->m_statistics_connection.count_fail_connect.fetch_add(1, std::memory_order_relaxed); m_statistics_connection_total.count_fail_connect.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_disconnect() noexcept { this->m_statistics_connection.count_disconnect.fetch_add(1, std::memory_order_relaxed); this->m_statistics_connection.count_keep.fetch_sub(1, std::memory_order_relaxed);}
			void				statistics_on_error_disconnect( uint64_t value) noexcept { this->m_statistics_connection.count_error_disconnect.fetch_add(value, std::memory_order_relaxed);}

			auto				statistics_on_increase_async() noexcept { return this->m_count_async.fetch_add(1, std::memory_order_relaxed) + 1;}
			auto				statistics_on_decrease_async() noexcept { return this->m_count_async.fetch_sub(1, std::memory_order_release) - 1;}
																							  
			void				statistics_set_create_time() noexcept { this->m_time_create = chrono::time::utc::now();}
			void				statistics_set_start_time() noexcept { this->m_time_start = chrono::time::utc::now();}
			void				statistics_set_stop_time() noexcept { this->m_time_stop = chrono::time::utc::now();}

	static	void				statistics_total_on_try() noexcept { m_statistics_connection_total.count_try.fetch_add(1, std::memory_order_relaxed);}
	static	void				statistics_total_on_success_connect() noexcept { m_statistics_connection_total.count_success_connect.fetch_add(1, std::memory_order_relaxed); m_statistics_connection_total.count_keep.fetch_add(1, std::memory_order_relaxed);}
	static	void				statistics_total_on_disconnect() noexcept { m_statistics_connection_total.count_disconnect.fetch_add(1, std::memory_order_relaxed); m_statistics_connection_total.count_keep.fetch_sub(1, std::memory_order_relaxed);}
	static	void				statistics_total_on_error_disconnect( uint64_t _value) noexcept { m_statistics_connection_total.count_error_disconnect.fetch_add(_value, std::memory_order_relaxed); }

	[[nodiscard]] static const auto& statistics_get_connective_info_total() noexcept { return m_statistics_connection_total;}

// implementation) 
private:
	// 1) 전송량 통계
			CGDK::statistics::unit_connective	m_statistics_connection;
	static	CGDK::statistics::unit_connective	m_statistics_connection_total;

	// 2) connective ID
			uint64_t			m_connective_id;

	// 3) 시간
			chrono::time::utc::time_point m_time_create;
			chrono::time::utc::time_point m_time_start;
			chrono::time::utc::time_point m_time_stop;

			std::atomic<int>	m_count_async;

			friend class net::io::connective::manager;
			friend class net::io::statistics::Nsocket;
};

class net::io::statistics::Nsocket
{
// constructor/destructor) 
public:
			Nsocket();
			~Nsocket() noexcept;

// publics) 
public:
	// 1) get function
	[[nodiscard]] auto			statistics_get_protocol() const noexcept { return this->m_statistics_protocol; }

	[[nodiscard]] auto			statistics_get_count_connect_try() const noexcept { return this->m_count_connect_try.load();}
	[[nodiscard]] auto			statistics_get_connect_time() const noexcept { return this->m_time_connect.load();}
	[[nodiscard]] auto			statistics_get_tick_connect() const noexcept { return this->m_tick_connect.load();}
	[[nodiscard]] auto			statistics_get_tick_disconnect() const noexcept { return this->m_tick_disconnect.load();}
	[[nodiscard]] auto			statistics_get_tick_connect_try() const noexcept { return this->m_tick_connect_try.load();}	// - 접속 요청 시각 (접속 중일 때만. 접속/종료하면 0)
																							  
	[[nodiscard]] auto			statistics_get_received_message() const noexcept { return this->m_statistics_traffic.count_received_message.load();}
	[[nodiscard]] auto			statistics_get_received_bytes() const noexcept { return this->m_statistics_traffic.count_received_bytes.load();}
	[[nodiscard]] auto			statistics_get_tick_last_send() const noexcept { return this->m_tick_last_send.load();}
	[[nodiscard]] auto			statistics_get_sent_message() const noexcept { return this->m_statistics_traffic.count_sent_message.load();}
	[[nodiscard]] auto			statistics_get_sent_bytes() const noexcept { return this->m_statistics_traffic.count_sent_bytes.load();}
	[[deprecated("use statistics_get_sent_message")]] [[nodiscard]] auto statistics_get_sended_message() const noexcept { return this->statistics_get_sent_message(); }
	[[deprecated("use statistics_get_sent_bytes")]] [[nodiscard]] auto statistics_get_sended_bytes() const noexcept { return this->statistics_get_sent_bytes(); }
	[[nodiscard]] auto			statistics_get_tick_last_receive() const noexcept { return this->m_tick_last_receive.load();}
																							  
	[[nodiscard]] auto			statistics_get_now_async_io() const noexcept { return this->m_count_async_con_dis.load(std::memory_order_acquire) + this->m_count_async_sending.load(std::memory_order_acquire) + this->m_count_async_receiving.load(std::memory_order_acquire);}
	[[nodiscard]] auto			statistics_get_async_send() const noexcept { return this->m_statistics_traffic.count_send_io.load();}
	[[nodiscard]] auto			statistics_get_async_sending() const noexcept { return this->m_count_async_sending.load(std::memory_order_acquire);}
	[[nodiscard]] auto			statistics_get_async_receive() const noexcept { return this->m_statistics_traffic.count_receive_io.load();}
	[[nodiscard]] auto			statistics_get_async_receiving() const noexcept { return this->m_count_async_receiving.load(std::memory_order_acquire);}
	[[nodiscard]] auto			statistics_get_overlapped_general() const noexcept { return this->m_statistics_traffic.count_general_io.load();}

	[[nodiscard]] auto			statistics_get_error_disconnected() const noexcept { return this->m_is_error_disconnect.load();}

	[[nodiscard]] auto			statistics_get_total_count_connect_try() const noexcept { return this->m_count_total_connect_try.load();}
	[[nodiscard]] auto			statistics_get_total_count_connect_success() const noexcept { return this->m_count_total_connect.load();}
	[[nodiscard]] auto			statistics_get_total_count_connect_fail() const noexcept { return this->m_count_total_connect_try - this->m_count_total_connect;}

	[[nodiscard]] const auto&	statistics_get_traffic_info() const noexcept { return this->m_statistics_traffic;}
	[[nodiscard]] static const auto& statistics_get_traffic_info_total() noexcept { return m_statistics_traffic_total;}

	// 2) ...
			void				reset_statistics() noexcept;

			void				statistics_on_connect_try() noexcept { ++this->m_count_connect_try; this->m_count_total_connect_try.fetch_add(1, std::memory_order_relaxed); Nconnective::statistics_total_on_try(); this->m_tick_disconnect = chrono::tick::time_point(chrono::tick::duration::zero()); this->m_tick_connect_try = chrono::tick::now();}
			void				statistics_on_connect() noexcept { this->m_time_connect = chrono::time::utc::now(); this->m_tick_last_receive = this->m_tick_last_send = this->m_tick_connect = chrono::tick::now(); this->m_tick_connect_try = chrono::tick::time_point(); Nconnective::statistics_total_on_success_connect(); this->m_count_total_connect.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_disconnect() noexcept { this->m_tick_disconnect = chrono::tick::now(); this->m_count_connect_try = 0; this->m_tick_connect_try = chrono::tick::time_point();}
			void				statistics_set_error_disconnect() noexcept { this->m_is_error_disconnect = static_cast<int>(true);}
			void				statistics_check_error_disconnect() noexcept { int count_error_disconnect = this->m_is_error_disconnect.exchange(false); net::io::statistics::Nconnective::m_statistics_connection_total.count_error_disconnect.fetch_add(count_error_disconnect, std::memory_order_relaxed);}

	// - 송수신 통계
	//   * socket별 : relaxed fetch_add (send는 여러 thread가 동시에 부를 수 있다.)
	//   * 전체 합계(m_statistics_traffic_total) : thread별로 쌓았다가 TRAFFIC_TOTAL_FLUSH_COUNT건 또는 TRAFFIC_TOTAL_FLUSH_INTERVAL마다 한 번에 더한다.
	//     (모든 thread가 같은 cache line을 매번 쓰면 thread가 많을수록 느려진다. 읽는 쪽은 thread마다 그만큼 늦게 볼 수 있다.)
			void				statistics_on_receive_message() noexcept { this->m_statistics_traffic.count_received_message.fetch_add(1, std::memory_order_relaxed); auto& total = _traffic_total_tls(); ++total.count_received_message; total.on_event();}
			void				statistics_on_receive_message( uint64_t _count) noexcept { this->m_statistics_traffic.count_received_message.fetch_add(_count, std::memory_order_relaxed); auto& total = _traffic_total_tls(); total.count_received_message += _count; total.on_event();}
			void				statistics_on_receive_bytes( uint64_t _bytes) noexcept { const auto tick_now = chrono::tick::now_coarse(); this->m_statistics_traffic.count_received_bytes.fetch_add(_bytes, std::memory_order_relaxed); auto& total = _traffic_total_tls(); total.count_received_bytes += _bytes; total.on_event(tick_now); this->m_tick_last_receive = tick_now;}
			void				statistics_on_receive_async() noexcept { this->m_statistics_traffic.count_receive_io.fetch_add(1, std::memory_order_relaxed); auto& total = _traffic_total_tls(); ++total.count_receive_io; total.on_event();}
			void				statistics_on_general_async() noexcept { this->m_statistics_traffic.count_general_io.fetch_add(1, std::memory_order_relaxed); auto& total = _traffic_total_tls(); ++total.count_general_io; total.on_event();}
			void				statistics_on_send_message() noexcept { this->m_statistics_traffic.count_sent_message.fetch_add(1, std::memory_order_relaxed); auto& total = _traffic_total_tls(); ++total.count_sent_message; total.on_event();}
			void				statistics_on_send_message( uint64_t _count) noexcept { this->m_statistics_traffic.count_sent_message.fetch_add(_count, std::memory_order_relaxed); auto& total = _traffic_total_tls(); total.count_sent_message += _count; total.on_event();}
			void				statistics_on_send_bytes( uint64_t _bytes) noexcept { const auto tick_now = chrono::tick::now_coarse(); this->m_statistics_traffic.count_sent_bytes.fetch_add(_bytes, std::memory_order_relaxed); auto& total = _traffic_total_tls(); total.count_sent_bytes += _bytes; total.on_event(tick_now); this->m_tick_last_send = tick_now;}
			void				statistics_on_send_async() noexcept { this->m_statistics_traffic.count_send_io.fetch_add(1, std::memory_order_relaxed); auto& total = _traffic_total_tls(); ++total.count_send_io; total.on_event();}
			void				statistics_on_error_send() noexcept { m_statistics_traffic_total.count_error_on_send.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_error_receive() noexcept { m_statistics_traffic_total.count_error_on_receive.fetch_add(1, std::memory_order_relaxed);}

	// - 이 thread가 쌓아 둔 전체 합계를 지금 더한다. (thread가 끝날 때는 자동으로 더한다.)
	static	void				statistics_flush_traffic_total() noexcept { _traffic_total_tls().flush(); }
			void				statistics_update_tick_last_send(chrono::tick::time_point _tick) noexcept{ this->m_tick_last_send = _tick; }
			void				statistics_update_tick_last_receive(chrono::tick::time_point _tick) noexcept{ this->m_tick_last_receive = _tick; }

			auto				statistics_on_increase_async_con_dis() noexcept { return m_count_async_con_dis.fetch_add(1, std::memory_order_relaxed) + 1;}
			auto				statistics_on_decrease_async_con_dis() noexcept { return m_count_async_con_dis.fetch_sub(1, std::memory_order_release) - 1;}
			auto				statistics_on_increase_async_sending() noexcept { return m_count_async_sending.fetch_add(1, std::memory_order_relaxed) + 1;}
			auto				statistics_on_decrease_async_sending() noexcept { return m_count_async_sending.fetch_sub(1, std::memory_order_release) - 1;}
			auto				statistics_on_increase_async_receiveing() noexcept { return m_count_async_receiving.fetch_add(1, std::memory_order_relaxed) + 1;}
			auto				statistics_on_decrease_async_receiveing() noexcept { return m_count_async_receiving.fetch_sub(1, std::memory_order_release) - 1;}

			void				enable_socket_debug_logging(bool _enable = true) noexcept { this->m_enable_socket_debug_logging = _enable; }
			void				disable_socket_debug_logging() noexcept { this->enable_socket_debug_logging(false); }
	[[nodiscard]] bool			socket_debug_logging() const noexcept { return this->m_enable_socket_debug_logging; }
	[[nodiscard]] bool			is_socket_debug_logging_enabled() const noexcept { return this->socket_debug_logging(); }

protected:
			void				statistics_set_protocol(int _protocol) noexcept { this->m_statistics_protocol = _protocol; }

// implementation) 
private:
	// 1) 전송량 통계
			int							m_statistics_protocol;

			CGDK::statistics::unit_traffic m_statistics_traffic;
	static	CGDK::statistics::unit_traffic m_statistics_traffic_total;

	// 2) Total관련(이것은 socket이 disconnect되어도 reset되지 않는다.)
			std::atomic<uint64_t>		m_count_total_connect_try;	// 총 접속 시도 횟수
			std::atomic<uint64_t>		m_count_total_connect;		// 총 접속 성공 횟수

	// 3) 전송시간 관련
	//    (socket thread가 쓰고 sustain/monitor thread가 읽는다 : relaxed atomic. 일반 변수와 같은 명령이다.)
			CGDK::statistics::stat_counter<uint64_t> m_count_connect_try;			// 접속 시도 횟수
			CGDK::statistics::stat_counter<chrono::time::utc::time_point> m_time_connect;	// 접속한 시간.
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_connect;		// 접속한 시간.(Tick)
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_last_send;		// 최후에 보낸 시간.(Tick)
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_last_receive;	// 최후에 받은 시간.(Tick)
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_wait_time;		// 기다리는 시간
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_connect_try;	// 접속 요청한 시간.(Tick : statistics_on_connect_try. 접속/종료하면 0)
			CGDK::statistics::stat_counter<chrono::tick::time_point> m_tick_disconnect;		// 접속종료한 시간.(Tick)
			std::atomic<int>			m_is_error_disconnect;		// Error로 인한 강제접속종료 여부

			// - 지금 걸려 있는 I/O 수 : 증가 relaxed, 감소 release, 읽기 acquire (여러 I/O가 동시에 끝날 수 있어 RMW는 그대로 쓴다)
			std::atomic<int>			m_count_async_con_dis;		// 현재 진행 중인 connect/disconnect Overlapped I/O의 수.
			std::atomic<int>			m_count_async_sending;		// 현재 진행 중인 send Overlapped I/O의 수.
			std::atomic<int>			m_count_async_receiving;	// 현재 진행 중인 receive Overlapped I/O의 수.

			bool						m_enable_socket_debug_logging = false;

	// 4) 전체 합계를 thread별로 쌓아 둔다. (statistics_on_xxx)
public:
	static constexpr uint32_t	TRAFFIC_TOTAL_FLUSH_COUNT = 64;
	static constexpr std::chrono::milliseconds TRAFFIC_TOTAL_FLUSH_INTERVAL{ 100 };

	struct sTRAFFIC_TOTAL_TLS
	{
		uint64_t				count_received_message = 0;
		uint64_t				count_received_bytes = 0;
		uint64_t				count_sent_message = 0;
		uint64_t				count_sent_bytes = 0;
		uint64_t				count_send_io = 0;
		uint64_t				count_receive_io = 0;
		uint64_t				count_general_io = 0;
		uint32_t				count_pending = 0;
		chrono::tick::time_point tick_flush{};

		~sTRAFFIC_TOTAL_TLS() noexcept { this->flush(); }

		void					on_event() noexcept { if (++this->count_pending >= TRAFFIC_TOTAL_FLUSH_COUNT) this->flush(); }
		void					on_event(chrono::tick::time_point _tick_now) noexcept
		{
			if (++this->count_pending >= TRAFFIC_TOTAL_FLUSH_COUNT || _tick_now - this->tick_flush >= TRAFFIC_TOTAL_FLUSH_INTERVAL)
			{
				this->tick_flush = _tick_now;
				this->flush();
			}
		}
		void					flush() noexcept;
	};

private:
	static	sTRAFFIC_TOTAL_TLS&	_traffic_total_tls() noexcept { thread_local sTRAFFIC_TOTAL_TLS s_traffic_total; return s_traffic_total; }
};


//-----------------------------------------------------------------------------
//
// CGWeb::statistics
//
//
// ----------------------------------------------------------------------------
extern	int						g_statistics_http_time;

namespace statistics
{

struct unit_http
{
public:
	chrono::tick::time_point	tick_snap = (chrono::tick::time_point::min)();

	std::atomic<uint64_t>		count_request;
	std::atomic<uint64_t>		count_response_success;
	std::atomic<uint64_t>		count_response_fail;

public:
	unit_http&	operator =(const unit_http& rhs) noexcept
	{
		this->count_request.store(rhs.count_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
		this->count_response_success.store(rhs.count_response_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
		this->count_response_fail.store(rhs.count_response_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
		return	*this;
	}
	void		reset() noexcept;
};

class Nhttp_connector
{
public:
			Nhttp_connector() noexcept {}
			~Nhttp_connector() noexcept {}
																							  
public:																						  
			void				reset_statistics() noexcept { this->m_statistics_http.reset();}
																							  
	[[nodiscard]] const auto&	statistics_get_query_info() const noexcept { return this->m_statistics_http;}
																							  
	[[nodiscard]] auto			statistics_get_request() const noexcept { return this->m_statistics_http.count_request.load(std::memory_order_relaxed);}
	[[nodiscard]] auto			statistics_get_response_success() const noexcept { return this->m_statistics_http.count_response_success.load(std::memory_order_relaxed);}
	[[nodiscard]] auto			statistics_get_response_fail() const noexcept { return this->m_statistics_http.count_response_fail.load(std::memory_order_relaxed);}
	[[nodiscard]] auto			statistics_getResponseProcessing() const noexcept { return (statistics_get_request()-statistics_get_response_success()-statistics_get_response_fail());}
																							  
			void				statistics_on_request() noexcept { this->m_statistics_http.count_request.fetch_add(1, std::memory_order_relaxed); statistics_total_on_request();}
			void				statistics_on_response_success() noexcept { this->m_statistics_http.count_response_success.fetch_add(1, std::memory_order_relaxed); statistics_total_on_response_success();}
			void				statistics_on_response_fail() noexcept { this->m_statistics_http.count_response_fail.fetch_add(1, std::memory_order_relaxed); statistics_total_on_response_fail();}
																							  
			static	void		statistics_total_on_request() noexcept { m_s_statistics_http_total.count_request.fetch_add(1, std::memory_order_relaxed);}
			static	void		statistics_total_on_response_success() noexcept { m_s_statistics_http_total.count_response_success.fetch_add(1, std::memory_order_relaxed);}
			static	void		statistics_total_on_response_fail() noexcept { m_s_statistics_http_total.count_response_fail.fetch_add(1, std::memory_order_relaxed);}
	[[nodiscard]] static	const auto&	statistics_get_total_query_info() noexcept { return m_s_statistics_http_total;}

private:
			unit_http			m_statistics_http;
	static	unit_http			m_s_statistics_http_total;
};

}
}