//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                 tutorial socket ex15 - tcp socket option                  *
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
// tutorial socket ex15 - TCP socket option
//                        (TCP_NODELAY, TCP_QUICKACK, SO_BUSY_POLL, TCP_NOTSENT_LOWAT, TCP Fast Open)
//
// 1. 무엇인가?
//    - 모든 socket은 sSOCKET_OPTION_TCP를 하나 가진다. (socket_option())
//      접속(connect/accept)이 완료될 때 socket handle에 적용된다. (apply_socket_option())
//    - 기본값은 아무것도 켜지 않는다. TCP_NODELAY만 예전과 같이 true이다.
//      => 옵션을 설정하지 않으면 동작은 이전과 완전히 같다.
//    - 옵션
//      . tcp_nodelay       : TCP_NODELAY. Nagle을 끈다. (기본 true)
//      . tcp_quickack      : 지연 ACK를 끈다. (기본 false)
//                            linux   : TCP_QUICKACK (영구가 아니므로 receivable이 recv마다 다시 설정한다)
//                            windows : SIO_TCP_SET_ACK_FREQUENCY = 1 (segment마다 ACK. 영구)
//      . busy_poll         : SO_BUSY_POLL (microseconds. 기본 0 = 설정 안 함. linux 전용)
//                            nonblocking recv(epoll)가 EAGAIN을 바로 돌려주지 않고 NIC 큐를 한 번 폴링한다.
//                            현재 값보다 올리려면 CAP_NET_ADMIN이 필요하다. (sysctl net.core.busy_read가 기본값)
//                            epoll_wait 자체의 busy poll은 executor::set_default_busy_poll() / io_context_threads의 "busy_poll"로 켠다. (아래 5절. 6.9+의 EPIOCSPARAMS)
//                            6.9 미만 커널은 sysctl net.core.busy_poll(전역)로만 켤 수 있다.
//      . tcp_notsent_lowat : TCP_NOTSENT_LOWAT (bytes. 기본 0 = 설정 안 함. linux 전용)
//                            커널 송신 버퍼의 '아직 보내지 않은' 바이트 상한. send()가 EAGAIN을 돌려주고 EPOLLOUT이 오는 기준이
//                            SO_SNDBUF가 아니라 이 값이 된다. => 데이터가 커널이 아니라 socket class의 송신 큐에 남는다. (우선순위, 메모리)
//      . tcp_fastopen      : TCP Fast Open (RFC 7413). 접속하는 socket(클라이언트)용. (기본 false)
//                            windows : ConnectEx 전에 TCP_FASTOPEN. 첫 접속에서 서버의 TFO cookie를 요청해 둔다.
//                            linux   : TCP_FASTOPEN_CONNECT. connect()가 즉시 끝나고 첫 send()가 SYN을 보낸다. (cookie가 있으면 데이터도 SYN에 싣는다)
//    - acceptor의 "tcp_fastopen" : listen socket의 TCP Fast Open. cookie를 가진 클라이언트가 SYN에 실어 보낸 데이터를 받는다.
//                                   (linux는 sysctl net.ipv4.tcp_fastopen에 2 비트가 있어야 한다. ex. 3)
//
// 2. 설정하는 방법
//    1) acceptor (accept되는 socket에 적용)
//       - START_PARAMETER : parameter.socket_option = {...}; parameter.flag_socket_option = 1; parameter.tcp_fastopen = true/queue 길이
//       - context         : "socket_option": { "tcp_nodelay": true, "tcp_quickack": true, ... }, "tcp_fastopen": true
//       - 코드            : pacceptor->socket_option(option) (이후 accept되는 socket), on_request_accept()에서 _pconnectable->socket_option()
//    2) 클라이언트 (connect_requestable, net::socket::tcp_client 등)
//       - START_PARAMETER : parameter.socket_option = {...}; parameter.flag_socket_option = 1;
//       - context         : "socket_option": {...}
//       - 코드            : psocket->socket_option().tcp_quickack = true;  (start() 전에)
//    3) connector
//       - start context의 "socket_option" : connector가 할당하는 socket에 적용 (pconnector->socket_option(option)도 같다)
//       - 접속 context의 "socket_option"  : 그 접속의 socket에 적용
//
// 3. 주의
//    - busy_poll, tcp_notsent_lowat은 windows에서 무시된다. (경고 로그)
//    - linux의 tcp_fastopen(클라이언트)은 on_connect()가 handshake 전에 불린다. 그 안에서 remote_endpoint()는 아직 없고,
//      거절된 접속은 on_fail_connect()가 아니라 on_disconnect()로 온다.
//    - TFO로 SYN에 실린 첫 데이터는 두 번 전달될 수 있다. (SYN 재전송) 첫 요청은 멱등해야 한다.
//    - 옵션이 handle에 적용됐는지는 getsockopt로 확인할 수 있다. (아래 get_option_int)
//
//-----------------------------------------------------------------------------
#include "cgdk/sdk10/net.socket.h"
#include "../define_message.h"

#include <iostream>
#include <sstream>

using namespace CGDK;

// - socket handle의 int 옵션 값을 읽는다. (-1 : 실패)
static int get_option_int(CGSOCKET _handle, int _level, int _name)
{
	int value = 0;		// - windows writes one byte for a bool option (TCP_NODELAY) : start from 0
	int length = sizeof(value);

	if (api::socket::get_sock_opt(_handle, _name, &value, &length, _level) == false)
		return -1;

	return value;
}

// - socket의 옵션 설정값과 handle에 실제 적용된 값을 출력한다.
static void print_socket_option(std::string_view _who, net::io::Isocket* _psocket)
{
	const auto& option = _psocket->socket_option();
	const auto handle = _psocket->native_handle();

	// - 한 줄을 만들어 한 번에 출력한다. (client와 server의 on_connect()는 다른 thread에서 동시에 불리므로 줄이 섞일 수 있다)
	std::ostringstream line;
	line << "   [" << _who << "] option: nodelay=" << option.tcp_nodelay << ", quickack=" << option.tcp_quickack << ", busy_poll=" << option.busy_poll
		 << ", notsent_lowat=" << option.tcp_notsent_lowat << ", fastopen=" << option.tcp_fastopen
		 << " / handle: TCP_NODELAY=" << get_option_int(handle, IPPROTO_TCP, TCP_NODELAY);
#if defined(_SYS_EPOLL_H)
	line << ", TCP_QUICKACK=" << get_option_int(handle, IPPROTO_TCP, TCP_QUICKACK) << ", TCP_NOTSENT_LOWAT=" << get_option_int(handle, IPPROTO_TCP, TCP_NOTSENT_LOWAT) << ", SO_BUSY_POLL=" << get_option_int(handle, SOL_SOCKET, SO_BUSY_POLL);
#endif
	line << '\n';
	std::cout << line.str();
}

// 1) 서버 socket 클래스를 정의한다. (받은 메시지를 그대로 돌려준다)
class socket_server : public net::socket::tcp<>
{
	// (v) 접속 시 호출되는 함수 (옵션은 on_connect 전에 이미 적용되어 있다)
	virtual void on_connect() override
	{
		print_socket_option("server"sv, this);
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

// 2) accept되는 socket의 옵션을 코드로 정하는 acceptor
//    - on_accept()는 accept가 완료된 직후(옵션이 handle에 적용되기 전)에 불린다. 여기서 socket_option()을 바꾸면 그 socket에 적용된다.
//      (acceptor에 socket_option을 주었다면 그 값이 on_accept() 직전에 socket에 복사되므로, 여기서 socket별로 바꿀 수 있다)
class acceptor_server : public net::acceptor<socket_server>
{
	virtual void on_accept(net::io::Iconnectable* _pconnectable) override
	{
		_pconnectable->socket_option().tcp_quickack = true;
	}
};

// 3) client socket 클래스를 정의한다.
class socket_client : public net::socket::tcp_client<>
{
public:
	std::atomic<bool>	is_connected{ false };
	std::atomic<bool>	is_failed{ false };
	std::atomic<int>	count_received{ 0 };

	// (v) 접속 시 호출되는 함수
	virtual void on_connect() override
	{
		print_socket_option("client"sv, this);
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

// - 접속된 client로 메시지 하나를 주고 받고 닫는다.
void echo_and_close(socket_client* _psocket)
{
	// check) 접속 실패
	if (wait_for([&]() { return _psocket->is_connected.load() || _psocket->is_failed.load(); }) == false || _psocket->is_failed)
		return;

	// 1) 메시지를 보낸다.
	sMESSAGE_A message;
	message.type = eMESSAGE_TYPE::SEND_A;
	message.x = 1;
	message.size = get_size_of<uint32_t>(message);

	_psocket->send(make_shared_buffer(message));

	// 2) 돌아온 메시지를 확인한다.
	wait_for([&]() { return _psocket->count_received.load() == 1; });
	std::cout << "   [client] echo received: "sv << _psocket->count_received.load() << '\n';

	// 3) 접속을 종료한다.
	_psocket->closesocket();
	std::this_thread::sleep_for(100ms);
}

int main()
{
	// - unbuffered output (the output of a redirected console is kept in a buffer otherwise)
	std::cout << std::unitbuf;

	// trace)
	std::cout << "start [tutorial.socket.ex15.socket_option]..."sv << '\n';

#if defined(_WIN32)
	std::cout << " (windows : 'busy_poll' and 'tcp_notsent_lowat' are ignored. 'tcp_quickack' is SIO_TCP_SET_ACK_FREQUENCY=1)"sv << '\n';
#endif

	//-----------------------------------------------------------------
	// 1. START_PARAMETER로 설정하기
	//-----------------------------------------------------------------
	std::cout << '\n' << "1. START_PARAMETER"sv << '\n';
	{
		// 1) acceptor를 생성한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();

		// 2) START_PARAMETER에 socket_option과 tcp_fastopen을 넣어 시작한다.
		//    - socket_option      : accept되는 socket마다 적용된다. (flag_socket_option = 1 이어야 적용된다)
		//    - tcp_fastopen       : listen socket의 TCP Fast Open (true = 기본 queue 길이 DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH)
		net::acceptor<socket_server>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20000 };
		parameter.enable_progress_log = false;
		parameter.socket_option.tcp_nodelay = true;
		parameter.socket_option.tcp_quickack = true;
		parameter.socket_option.tcp_notsent_lowat = 64 * 1024;		// - linux
		parameter.flag_socket_option = 1;
		parameter.tcp_fastopen = DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH;
		pacceptor->start(parameter);

		// 3) listen socket에 TCP_FASTOPEN이 켜졌는지 확인한다.
		std::cout << " - listen socket TCP_FASTOPEN="sv << get_option_int(pacceptor->native_handle(), IPPROTO_TCP, TCP_FASTOPEN) << " (acceptor.tcp_fastopen()="sv << pacceptor->tcp_fastopen() << ")"sv << '\n';

		// 4) 클라이언트도 START_PARAMETER의 socket_option으로 설정한다.
		//    - tcp_fastopen : 접속 socket의 TCP Fast Open
		std::cout << " - connect to 127.0.0.1:20000 (START_PARAMETER::socket_option)"sv << '\n';
		auto psocket = make_own<socket_client>();
		sSOCKET_CONNECT_REQUEST_PARAMETER parameter_connect;
		parameter_connect.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), 20000 };
		parameter_connect.enable_progress_log = false;
		parameter_connect.socket_option.tcp_quickack = true;
		parameter_connect.socket_option.tcp_fastopen = true;
		parameter_connect.flag_socket_option = 1;
		psocket->start(parameter_connect);
		echo_and_close(psocket.get());

		// 5) acceptor를 닫는다.
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 2. context로 설정하기
	//-----------------------------------------------------------------
	std::cout << '\n' << "2. context setting"sv << '\n';
	{
		// 1) context를 작성한다. (설정 파일(json 등)에서 읽어 오는 것과 같다)
		context context_setting;
		context_setting["local_endpoint"]["port"] = 20001;
		context_setting["progress_log"] = "false";
		context_setting["tcp_fastopen"] = true;								// - listen socket의 TCP Fast Open (true/false 또는 queue 길이)
		context_setting["socket_option"]["tcp_nodelay"] = true;				// - accept되는 socket의 옵션
		context_setting["socket_option"]["tcp_quickack"] = true;
		context_setting["socket_option"]["tcp_notsent_lowat"] = "64k";		// - "64k", "1m" 같은 표기도 된다.
		context_setting["socket_option"]["busy_poll"] = 50;					// - linux. 올리려면 CAP_NET_ADMIN (실패하면 경고 로그 후 무시)

		// 2) context로 acceptor를 시작한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();
		pacceptor->start(context_setting);

		// 3) 클라이언트도 context로 설정한다.
		std::cout << " - connect to 127.0.0.1:20001 (context \"socket_option\")"sv << '\n';
		context context_connect;
		context_connect["remote_endpoint"]["address"] = "127.0.0.1";
		context_connect["remote_endpoint"]["port"] = 20001;
		context_connect["progress_log"] = "false";
		context_connect["socket_option"]["tcp_quickack"] = true;
		context_connect["socket_option"]["tcp_fastopen"] = true;

		auto psocket = make_own<socket_client>();
		psocket->start(context_connect);
		echo_and_close(psocket.get());

		// 4) acceptor를 닫는다.
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 3. 코드로 설정하기
	//-----------------------------------------------------------------
	std::cout << '\n' << "3. by code"sv << '\n';
	{
		// 1) acceptor : on_accept()에서 accept되는 socket의 socket_option()을 바꾼다. (acceptor_server)
		//    (또는 pacceptor->socket_option(option) : 이후 accept되는 모든 socket에 적용)
		auto pacceptor = make_own<acceptor_server>();
		pacceptor->start(net::acceptor<socket_server>::START_PARAMETER{ net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20002 } });

		// 2) 클라이언트 : start() 전에 socket_option()을 바꾼다.
		std::cout << " - connect to 127.0.0.1:20002 (socket_option() before start)"sv << '\n';
		auto psocket = make_own<socket_client>();
		psocket->socket_option().tcp_nodelay = false;
		psocket->socket_option().tcp_quickack = true;
		psocket->start({{ net::ip::address_v4::loopback(), 20002 }});
		echo_and_close(psocket.get());

		// 3) 옵션을 주지 않은 클라이언트 : 아무것도 적용되지 않는다. (TCP_NODELAY만 예전과 같이 true)
		std::cout << " - connect to 127.0.0.1:20002 (no option)"sv << '\n';
		auto psocket_default = make_own<socket_client>();
		psocket_default->start({{ net::ip::address_v4::loopback(), 20002 }});
		echo_and_close(psocket_default.get());

		// 4) acceptor를 닫는다.
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 4. connector로 설정하기
	//-----------------------------------------------------------------
	std::cout << '\n' << "4. connector"sv << '\n';
	{
		// 1) acceptor를 시작한다.
		auto pacceptor = make_own<net::acceptor<socket_server>>();
		pacceptor->start(net::acceptor<socket_server>::START_PARAMETER{ net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20003 } });

		// 2) connector를 start context의 "socket_option"으로 시작한다. (connector가 할당하는 socket에 적용된다)
		context context_connector;
		context_connector["progress_log"] = "false";
		context_connector["socket_option"]["tcp_quickack"] = true;

		auto pconnector = make_own<net::connector<socket_client>>();
		pconnector->start(context_connector);

		// 3) 접속을 요청한다. => connector의 옵션이 적용된 socket
		std::cout << " - request_connect to 127.0.0.1:20003 (the option of the connector)"sv << '\n';
		auto pconnectable = pconnector->request_connect({ net::ip::address_v4::loopback(), 20003 });
		echo_and_close(dynamic_cast<socket_client*>(pconnectable.get()));

		// 4) 접속 context에 "socket_option"을 주면 그 접속의 socket에 적용된다. (주지 않은 키는 connector의 값)
		std::cout << " - request_connect to 127.0.0.1:20003 (the \"socket_option\" of the connect context)"sv << '\n';
		context context_connect;
		context_connect["remote_endpoint"]["address"] = "127.0.0.1";
		context_connect["remote_endpoint"]["port"] = 20003;
		context_connect["socket_option"]["tcp_nodelay"] = false;
		auto pconnectable2 = pconnector->request_connect(context_connect);
		echo_and_close(dynamic_cast<socket_client*>(pconnectable2.get()));

		// 5) connector와 acceptor를 닫는다.
		pconnector->close_connectable_all();
		pconnector->destroy();
		pacceptor->stop();
	}

	//-----------------------------------------------------------------
	// 5. epoll_wait 자체의 busy poll (linux, kernel 6.9+)
	//
	//    - socket의 busy_poll(SO_BUSY_POLL)은 recv가 NIC 큐를 한 번 보는 것이고, 여기서는 epoll_wait()가 잠들기 전에
	//      NIC 큐를 usecs 동안 돌며 본다. 그 사이 도착한 패킷은 인터럽트와 깨우기 없이 처리된다. (지연 감소, CPU 사용)
	//    - 같은 NAPI ID(한 NIC 큐)의 socket들이 한 epoll에 있어야 효과가 있다. loopback / UDS에는 커널이 아무것도 하지 않는다.
	//      acceptor의 "cpu_affinity"(SO_INCOMING_CPU로 CPU별 io_context)와 함께 쓰면 한 큐의 socket들이 한 epoll에 모인다.
	//    - 켜는 방법
	//      . executor::set_default_busy_poll(usecs, budget, prefer) : 이후 만드는 모든 epoll io_context (system_executor는 처음 쓸 때 만들어지므로 그 전에)
	//      . make_io_context_threads(name, thread_count, cpu, busy_poll_usecs) / make_io_context_threads(name, context) : "busy_poll" (microseconds), "busy_poll_budget", "prefer_busy_poll"
	//      . pexecutor->set_busy_poll(usecs, budget, prefer) (시작 후 변경 / 끄기), get_busy_poll()
	//    - budget : 한 번에 보는 패킷 수 (0 = 커널 기본 8. 64 초과는 CAP_NET_ADMIN), prefer : NIC 인터럽트보다 busy poll을 우선
	//    - windows, io_uring 백엔드, 6.9 미만 커널에서는 경고 후 무시된다. (is_busy_poll_available())
	//-----------------------------------------------------------------
	std::cout << '\n' << "5. busy poll of epoll_wait() (linux 6.9+)"sv << '\n';
	{
		// check) 지원 여부
		if (executor::is_busy_poll_available() == false)
		{
			std::cout << " - not available (windows, or a kernel before 6.9 : sysctl net.core.busy_poll applies instead)"sv << '\n';
		}
		else
		{
			// 1) 전용 io_context를 busy poll 50us로 시작한다. (thread 2개, CPU 고정 없음(-1), busy poll 50us)
			//    (설정 파일로 시작하려면 make_io_context_threads(name, context) : "thread_count", "cpu_affinity", "busy_poll", "busy_poll_budget", "prefer_busy_poll")
			auto pexecutor = executor::make_io_context_threads("busy poll executor", 2, -1, 50);

			// 2) budget과 prefer는 시작 후 set_busy_poll()로. 커널에 설정된 값을 읽어 본다. (EPIOCGPARAMS)
			pexecutor->set_busy_poll(50, 16, true);

			uint32_t usecs = 0; uint16_t budget = 0; bool prefer = false;
			pexecutor->get_busy_poll(usecs, budget, prefer);
			std::cout << " - executor busy poll: "sv << usecs << "us, budget "sv << budget << ", prefer "sv << prefer << '\n';

			// 3) 그 io_context 위에서 acceptor와 클라이언트를 돌린다. (loopback이라 busy poll의 효과는 없다 : 동작 확인)
			auto pacceptor = make_own<net::acceptor<socket_server>>();
			pacceptor->io_context(pexecutor.get());
			net::acceptor<socket_server>::START_PARAMETER parameter;
			parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20004 };
			parameter.enable_progress_log = false;
			pacceptor->start(parameter);

			std::cout << " - connect to 127.0.0.1:20004 (on the busy poll executor)"sv << '\n';
			auto psocket = make_own<socket_client>();
			psocket->io_context(pexecutor.get());
			psocket->start({{ net::ip::address_v4::loopback(), 20004 }});
			echo_and_close(psocket.get());

			// 4) 시작 후 바꾸기 / 끄기
			pexecutor->set_busy_poll(20);
			pexecutor->get_busy_poll(usecs, budget, prefer);
			std::cout << " - set_busy_poll(20): "sv << usecs << "us"sv << '\n';
			pexecutor->set_busy_poll(0);

			// 5) 닫는다.
			pacceptor->stop();
			pexecutor->stop();

			// 6) 기본값 : 이후 만드는 모든 epoll io_context에 적용된다. (서버 시작 전에 한 번 부르는 용도. 여기서는 되돌린다)
			executor::set_default_busy_poll(50);
			auto pexecutor_default = executor::make_io_context_threads("default busy poll executor", 1);
			pexecutor_default->get_busy_poll(usecs, budget, prefer);
			std::cout << " - set_default_busy_poll(50) => a new executor: "sv << usecs << "us"sv << '\n';
			pexecutor_default->stop();
			executor::set_default_busy_poll(0);
		}
	}

	// trace)
	std::cout << '\n' << "stop [tutorial.socket.ex15.socket_option]..."sv << '\n';

	// end) 종료
	return 0;
}
