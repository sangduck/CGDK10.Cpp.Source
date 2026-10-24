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
// net::io::add_on::Nsocket_sustain
//
// 1. net::io::add_on::Nsocket_sustain란!
//    1) socket을 유지하기 위한 처리를 하는 클래스이다.
//    2) 이 클래스를 상속받아 부착하게 되면 일정시만 마다 ProcessSustain()함수가
//       호출이 되며 이 때 일반적인 socket의 유지를 위한 처리를 수행하게 된다.
//    3) 일반적으로 Ghost가 된 socket이면 제거하거나 일정시간 동안 송수신이 없을 시
//       접속을 종료시키는 등의 역할을 한다.
//    4) 기본적으로 접속된 socket이 일정 시간 동안 아무런 송신이나 수신이 없을 때
//       각각 OnReceiveTimeOut(), OnSendTimeOut()을 호출해준다.
//    5) Add-On형식이라 상속받아 결합하는 Class에 부가적으로 붙어서 동작한다.
//    6) 일반적으로 Server쪽의 socket에 사용된다.
//
//
// 2. net::io::add_on::Nsocket_sustain의 주요 Function의 기능 설명
//    1) set_connect_time_out()함수와 ConnectTimeOut 설정.
//
//		 virtual void on_timeout_connect();
//
//	     접속된 후 on_timeout_connect()이 지나면 호출될지 시간을 나타내는 변수
//       이다.
//       이는 설정하거나 얻는 함수는 아래와 같다.
//
//		 void set_connect_time_out(tick::time_point _tick)
//		 tick:duration get_connect_time_out()
//
//    2) on_timeout_disconnect()함수와 DisconnectTimeOut 설정.
//
//		 virtual void on_timeout_disconnect();
//
//	     접속종료된 이후 얼마 후 on_timeout_disconnect()이 호출될지 시간을 
//       나타내는 변수이다.
//       이는 설정하거나 얻는 함수는 아래와 같다.
//
//		 void set_disconnect_time_out(tick::time_point _tick)
//		 tick:duration get_disconnect_time_out()
//
//    3) on_timeout_send()함수와 SendTimeOut 설정.
//
//		 virtual void on_timeout_send();
//
//	     얼마 동안 send가 없어야 on_timeout_send()이 호출될지 시간을 나타내는 변수
//       이다.
//       이는 설정하거나 얻는 함수는 아래와 같다.
//
//		 void set_send_time_out(tick::time_point _tick)
//		 tick:duration get_send_time_out()
//
//    4) on_timeout_receive()함수와 ReceiveTimeOut
//
//		 virtual void on_timeout_receive();
//
//	     얼마 동안 receive가 없어야 on_timeout_receive()이 호출될지 시간을 나타내는
//       변수이다.
//       이는 설정하거나 얻는 함수는 아래와 같다.
//
//		 void set_receive_time_out(tick::time_point _tick)
//		 tick:duration get_receive_time_out()
//
//    5) 이 클래스는 Add-On으로 socket이 생성되는 즉시 동작하게 된다.
//    6) 자동으로 접속을 종료처리를 하는 것은 socket이 eSOCKET_STATE::ESTABLISED
//       일 때만 동작된다. 즉 접속 중일 때만 동작된다.
//
//    7) 각각의 Time-Out값이 0으로 설정되면 해당 Time-Out은 동작하지 않도록
//       Disable된다.
//    8) OnTimeOut은 한번  실행된 이후에 내부에서 다시 TimeOut을 설정하지 않는한
//       TimeOut이 0으로 reset되어서 OnTimeOut...()함수들은 한번만 실행된다.
//       만약 OnTimeOut...()함수내에서 다시 Set...TimeOut()함수를 사용하여
//       TimeOut()을 다시 설정한다면 다시 Time Out이 발생한다.
//       (이때 connect나 disconnect의 Time-Out의 설정은 접속시점 혹은 접속 종료 
//       시점을 기준으로 하는 함수므로 주의해야 한다.
//
//       주의) OnTimeOut함수가 호출될 때 해당 m_tick_timeout...값들은 이미 0으로
//             reset된 이후므로 이 값을 참조하게 되면 0을 얻을 뿐이다.
//
//  3. 주의
//     1) 모든 Time-Out의 설정값은 0x7fffffff보다 작아야 한다.
//
//
//-----------------------------------------------------------------------------
class net::io::add_on::Nsocket_sustain :
	virtual public				net::io::Isocket,
	virtual	public				net::io::statistics::Nsocket
{
protected:
			Nsocket_sustain();
	virtual ~Nsocket_sustain();

public:
			bool				start_socket_sustain();
			bool				stop_socket_sustain();

			void				time_out_connect(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_connect = _tick; }
	[[nodiscard]] auto			time_out_connect() const noexcept { return m_tick_timeout_connect;}
			void				reset_time_out_connect() noexcept { this->time_out_connect(chrono::tick::duration::zero());}

			void				time_out_disconnect(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_disconnect = _tick; }
	[[nodiscard]] auto			time_out_disconnect() const noexcept { return m_tick_timeout_disconnect;}
			void				reset_time_out_disconnect() noexcept { this->time_out_disconnect(chrono::tick::duration::zero());}
																				  
			void				time_out_receive(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_receive = _tick; }
	[[nodiscard]] auto			time_out_receive() const noexcept { return m_tick_timeout_receive;}
			void				reset_time_out_receive() noexcept { this->time_out_receive(chrono::tick::duration::zero());}
																				  
			void				time_out_send(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_send = _tick; }
	[[nodiscard]] auto			time_out_send() const noexcept { return m_tick_timeout_send;}
			void				reset_time_out_send() noexcept { this->time_out_send(chrono::tick::duration::zero());}

	// - 공통 시간 제한 (0 : 쓰지 않는다. 시간이 지나 끊을 때는 DISCONNECT_REASON_SUSTAIN_TIMEOUT으로 끊는다)
	//   (ex. http server : connecting 0, certify = request_timeout, idle = idle_timeout, ping_interval (websocket))
			struct sTIMEOUT
			{
				chrono::tick::duration	connecting{ chrono::tick::duration::zero() };	// - 접속 중(SYN) (접속 중인 것을 처음 본 시각부터 센다. sustain 주기만큼 늦을 수 있다)
				chrono::tick::duration	certify{ chrono::tick::duration::zero() };		// - 접속 후 CERTIFIED가 되지 않음 (ex. http : 첫 요청이 끝나지 않음(천천히 보내며 접속을 잡아 두는 것))
				chrono::tick::duration	idle{ chrono::tick::duration::zero() };			// - 받은 것이 없음 (= time_out_receive : on_timeout_receive)
				chrono::tick::duration	ping_interval{ chrono::tick::duration::zero() };	// - keepalive : 받은 것이 없으면 이 시간마다 process_keepalive_ping(), 두 배의 시간 동안 받은 것이 없으면 끊는다
			};
			// - 시간 제한으로 끊은 수 (socket보다 오래 있어야 한다)
			struct sTIMEOUT_COUNT
			{
				std::atomic<uint64_t>	connecting{ 0 };
				std::atomic<uint64_t>	certify{ 0 };
				std::atomic<uint64_t>	idle{ 0 };
			};
			void				set_sustain_timeout(const sTIMEOUT& _timeout, sTIMEOUT_COUNT* _pcount = nullptr) noexcept;

			void				time_out_connecting(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_connecting = _tick; }
	[[nodiscard]] auto			time_out_connecting() const noexcept { return m_tick_timeout_connecting;}
			void				time_out_certify(chrono::tick::duration _tick) noexcept { this->m_tick_timeout_certify = _tick; }
	[[nodiscard]] auto			time_out_certify() const noexcept { return m_tick_timeout_certify;}
			void				keepalive_ping_interval(chrono::tick::duration _tick) noexcept { this->m_tick_ping_interval = _tick; }
	[[nodiscard]] auto			keepalive_ping_interval() const noexcept { return m_tick_ping_interval;}

private:
	virtual	void				on_timeout_connect(chrono::tick::duration /*p_tickdifferExecute*/) {}
	virtual	void				on_timeout_disconnect(chrono::tick::duration /*p_tickdifferExecute*/) {}
	virtual	void				on_timeout_receive(chrono::tick::duration /*p_tickdifferExecute*/) { this->_close_by_sustain_timeout(&sTIMEOUT_COUNT::idle); }
	virtual	void				on_timeout_send(chrono::tick::duration /*p_tickdifferExecute*/) {}

			friend class manager_socket_sustain;

	// - sustain 대상인가? (manager_socket_sustain이 lock 안에서 모든 socket에 대해 참조를 잡기 전에 부른다)
	//   - 가상 함수를 부르지 않고 이 class와 기반의 값만 읽는다. (다른 thread에서 소멸 중인 socket에도 불릴 수 있다 : 그 동안 이 값들은 남아 있다)
	//   - 지금 상태에서 쓰는 시간 제한이 하나도 없으면 false : 참조를 잡지 않고 건너뛴다. (socket이 많아도 대상만 참조 count를 건드린다)
			bool				_is_sustain_target() noexcept;

			chrono::tick::duration	m_tick_timeout_connect;
			chrono::tick::duration	m_tick_timeout_disconnect;
			chrono::tick::duration	m_tick_timeout_receive;
			chrono::tick::duration	m_tick_timeout_send;
			chrono::tick::duration	m_tick_timeout_connecting{ chrono::tick::duration::zero() };
			chrono::tick::duration	m_tick_timeout_certify{ chrono::tick::duration::zero() };
			chrono::tick::duration	m_tick_ping_interval{ chrono::tick::duration::zero() };
			chrono::tick::time_point m_tick_connecting_start{};	// - 접속 중(SYN)인 것을 처음 본 시각 (sustain만 쓴다)
			chrono::tick::time_point m_tick_last_ping{};
			sTIMEOUT_COUNT*		m_pcount_timeout{ nullptr };

			circular_list<Nsocket_sustain*>::iterator m_iter_list_socket_sustain;
			object_ptr<manager_socket_sustain> m_pmanager;

protected:
	// - 파생 class가 덮어쓰고 이 함수(기본 동작)를 부를 수 있다.
	virtual void				process_socket_sustain();

	// - keepalive PING을 보낸다. (ex. websocket. true : 보냄 / false : 보낼 수 없음)
	virtual	bool				process_keepalive_ping() { return false; }

	// - 시간 제한으로 끊는다. (DISCONNECT_REASON_SUSTAIN_TIMEOUT. sTIMEOUT_COUNT가 있으면 센다)
			void				_close_by_sustain_timeout(std::atomic<uint64_t> sTIMEOUT_COUNT::* _pcount);

public:
	static	void				detach_instance() noexcept;
};


}