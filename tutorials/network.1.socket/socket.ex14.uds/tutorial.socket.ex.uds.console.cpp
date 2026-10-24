//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                    tutorial socket ex14 - UDS (same host)                 *
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
//-----------------------------------------------------------------------------
//
// tutorial socket ex14 - UDS (Unix domain socket)로 같은 host 접속 빠르게 하기
//
// 1. 무엇인가?
//    - 같은 host의 서버끼리 TCP로 접속하면(127.0.0.1) 데이터가 TCP/IP stack을 모두 거친다.
//    - linux에서는 acceptor에 'enable_uds' 옵션만 켜면 같은 host의 접속을 UDS로 받는다.
//      (windows의 SIO_LOOPBACK_FAST_PATH에 해당. windows는 acceptor가 이미 LOOPBACK_FASTPATH를 켠다)
//    - 접속 지연은 TCP의 약 1/5, 처리량은 1.2~2배 정도 좋아진다. (POKPUNG 측정 기준)
//
// 2. 사용법
//    1) 서버 : START_PARAMETER::enable_uds = true
//       - TCP port와 함께 추상 이름 UDS "\0cgdk.tcp.<port>"로도 listen한다. (파일이 생기지 않는다)
//       - UDS로 받은 socket도 TCP로 받은 socket과 같은 클래스로 생성되고 on_connect/on_message/on_disconnect도 같다.
//       - listen 주소가 any(0.0.0.0, ::)나 loopback일 때만 UDS로도 listen한다.
//    2) 클라이언트 : net::io::connect_requestable::Ntcp::enable_uds_loopback(true) 를 한 번 부른다. (기본 false = TCP 만)
//       - loopback 주소(127.x.x.x, ::1)와 port로 접속하면 먼저 그 port의 UDS로 접속해 보고, 없으면 TCP로 접속한다.
//       - bind_endpoint를 지정하면 UDS를 시도하지 않는다. (TCP)
//       - 전역으로 끄기 : net::io::connect_requestable::Ntcp::enable_uds_loopback(false)
//    3) UDS로 접속된 socket인지 확인 : remote_endpoint().is_unix() (to_string()은 "unix")
//    4) 서버 : START_PARAMETER::uds_only = true
//       - UDS로만 받는다. TCP port는 bind만 하고(다른 프로그램이 쓰지 못하게 잡아 둔다) listen하지 않는다.
//         => 외부에서 TCP로 접속할 수 없다. (같은 host의 내부 서버용)
//       - UDS listen에 실패하면 시작이 실패한다.
//    5) context로 설정 : "enable_uds": "true", "uds_only": "true"
//
// 3. 주의
//    - linux 전용이다. windows에서는 enable_uds/uds_only가 무시되고 TCP로 동작한다.
//    - 같은 host 안에서만 쓰인다. 다른 host로의 접속은 그대로 TCP다.
//
//-----------------------------------------------------------------------------
#include "cgdk/sdk10/net.socket.h"
#include "../define_message.h"

#include <iostream>

using namespace CGDK;

// 1) 서버 socket 클래스를 정의한다. (받은 메시지를 그대로 돌려준다)
//    - TCP로 접속한 socket이든 UDS로 접속한 socket이든 이 클래스로 생성된다.
class socket_server : public net::socket::tcp<>
{
	// (v) 접속 시 호출되는 함수
	virtual void on_connect() override
	{
		// - UDS로 접속되었는지는 remote_endpoint().is_unix()로 알 수 있다.
		std::cout << "   [server] connected ("sv << (this->remote_endpoint().is_unix() ? "UDS"sv : "TCP"sv) << ", remote:"sv << to_string(this->remote_endpoint()) << ")"sv << '\n';
	}

	// (v) 접속 종료 시 호출되는 함수
	virtual void on_disconnect(uint64_t) override
	{
		std::cout << "   [server] disconnected"sv << '\n';
	}

	// (v) 메시지를 수신했을 때 호출 되는 함수. (그대로 돌려준다)
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->send(_msg.buf_message);

		return eRESULT::DONE;
	}
};

// 2) client socket 클래스를 정의한다.
class socket_client : public net::socket::tcp_client<>
{
public:
	std::atomic<bool>	is_connected{ false };
	std::atomic<bool>	is_failed{ false };
	std::atomic<int>	count_received{ 0 };

	// (v) 접속 시 호출되는 함수
	virtual void on_connect() override
	{
		std::cout << "   [client] connected ("sv << (this->remote_endpoint().is_unix() ? "UDS"sv : "TCP"sv) << ")"sv << '\n';
		this->is_connected = true;
	}

	// (v) 접속 실패 시 호출되는 함수
	virtual void on_fail_connect(uint64_t /*_disconnect_reason*/) override
	{
		std::cout << "   [client] fail to connecting"sv << '\n';
		this->is_failed = true;
	}

	// (v) 접속 종료 시 호출되는 함수
	virtual void on_disconnect(uint64_t) override
	{
		std::cout << "   [client] disconnected"sv << '\n';
	}

	// (v) 메시지를 수신했을 때 호출 되는 함수.
	virtual result_code on_message(sMESSAGE_NETWORK& /*_msg*/) override
	{
		++this->count_received;

		return eRESULT::DONE;
	}
};

// - 조건이 될 때까지 기다린다. (최대 3초)
template <class TCONDITION>
bool wait_for(TCONDITION&& _condition)
{
	for (int i = 0; i < 300 && !_condition(); ++i)
		std::this_thread::sleep_for(10ms);

	return _condition();
}

// - 접속해서 메시지 하나를 주고 받는다.
void connect_and_echo(uint16_t _port)
{
	// 1) 접속한다. (loopback 주소 => UDS를 먼저 시도한다)
	auto psocket = make_own<socket_client>();
	psocket->start({{ net::ip::address_v4::loopback(), _port }});

	// check) 접속 실패
	if (wait_for([&]() { return psocket->is_connected.load() || psocket->is_failed.load(); }) == false || psocket->is_failed)
		return;

	// 2) 메시지를 보낸다.
	sMESSAGE_A message;
	message.type = eMESSAGE_TYPE::SEND_A;
	message.x = 1;
	message.size = get_size_of<uint32_t>(message);

	psocket->send(make_shared_buffer(message));

	// 3) 돌아온 메시지를 확인한다.
	wait_for([&]() { return psocket->count_received.load() == 1; });
	std::cout << "   [client] echo received: "sv << psocket->count_received.load() << '\n';

	// 4) 접속을 종료한다.
	psocket->closesocket();
	std::this_thread::sleep_for(100ms);
}

int main()
{
	// trace)
	std::cout << "start [tutorial.socket.ex14.uds]..."sv << '\n';

	// - client switch : try the UDS of the port first when connecting to a loopback address (default false = TCP only)
	net::io::connect_requestable::Ntcp::enable_uds_loopback(true);

#if defined(_WIN32)
	// - windows : enable_uds/uds_only는 무시되고 TCP로 동작한다. (acceptor는 LOOPBACK_FASTPATH를 사용한다)
	std::cout << " (windows : 'enable_uds' and 'uds_only' are ignored. all connections are TCP)"sv << '\n';
#endif

	//-----------------------------------------------------------------
	// 1. enable_uds : TCP와 UDS로 같이 받기
	//-----------------------------------------------------------------
	std::cout << '\n' << "1. enable_uds (TCP + UDS)"sv << '\n';
	{
		// 1) acceptor를 생성한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();

		// 2) START_PARAMETER에 enable_uds를 켜고 시작한다.
		//    - 20000번 port의 TCP와 UDS "\0cgdk.tcp.20000"로 listen한다.
		net::acceptor<socket_server>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20000 };
		parameter.enable_uds = true;
		parameter.enable_progress_log = false;
		pacceptor->start(parameter);

		// 3) 127.0.0.1:20000으로 접속 => UDS로 접속된다.
		std::cout << " - connect to 127.0.0.1:20000"sv << '\n';
		connect_and_echo(20000);

		// 4) UDS를 쓰지 않으려면 => TCP로 접속된다.
		//    (connect_requestable 전역 설정. 다시 켜기 전까지 모든 접속에 적용된다)
		std::cout << " - connect to 127.0.0.1:20000 with enable_uds_loopback(false)"sv << '\n';
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);
		connect_and_echo(20000);
		net::io::connect_requestable::Ntcp::enable_uds_loopback(true);

		// 5) acceptor를 닫는다. (UDS도 함께 닫힌다)
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 2. uds_only : UDS로만 받기 (외부에서 TCP로 접속할 수 없는 내부 서버)
	//-----------------------------------------------------------------
	std::cout << '\n' << "2. uds_only"sv << '\n';
	{
		// 1) acceptor를 생성한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();

		// 2) START_PARAMETER에 uds_only를 켜고 시작한다.
		//    - 20001번 TCP port는 bind만 해서 잡아 두고 listen하지 않는다. UDS "\0cgdk.tcp.20001"로만 받는다.
		net::acceptor<socket_server>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20001 };
		parameter.uds_only = true;
		parameter.enable_progress_log = false;
		pacceptor->start(parameter);

		// 3) 127.0.0.1:20001로 접속 => UDS로 접속된다.
		std::cout << " - connect to 127.0.0.1:20001"sv << '\n';
		connect_and_echo(20001);

		// 4) TCP로 접속하면 => 접속이 거절된다. (linux)
		std::cout << " - connect to 127.0.0.1:20001 with enable_uds_loopback(false)"sv << '\n';
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);
		connect_and_echo(20001);
		net::io::connect_requestable::Ntcp::enable_uds_loopback(true);

		// 5) acceptor를 닫는다.
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 3. context로 설정하기
	//-----------------------------------------------------------------
	std::cout << '\n' << "3. context setting"sv << '\n';
	{
		// 1) context를 작성한다. (설정 파일(json 등)에서 읽어 들인 것과 같다)
		//    - "enable_uds": "true" (또는 "uds_only": "true")
		context context_setting;
		context_setting["local_endpoint"]["port"] = 20002;
		context_setting["enable_uds"] = "true";
		context_setting["progress_log"] = "false";

		// 2) context로 acceptor를 시작한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();
		pacceptor->start(context_setting);

		// 3) 127.0.0.1:20002로 접속 => UDS로 접속된다.
		std::cout << " - connect to 127.0.0.1:20002"sv << '\n';
		connect_and_echo(20002);

		// 4) acceptor를 닫는다.
		pacceptor->stop();
	}

	// trace)
	std::cout << '\n' << "stop [tutorial.socket.ex14.uds]..."sv << '\n';

	// end) 종료
	return 0;
}
