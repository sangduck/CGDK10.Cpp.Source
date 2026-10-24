//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*            tutorial socket ex16 - emergency acceptor / signals            *
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
// tutorial socket ex16 - emergency acceptor와 signal 처리
//
// 1. 무엇인가?
//    - 서버의 system_executor가 꽉 막혀(모든 thread가 block) 있어도 접속해서 상태를 보거나 조치할 수 있는
//      비상용 관리 채널(emergency acceptor)을 만든다.
//    - emergency acceptor는 평소에는 없고 'emergency signal'을 받았을 때 생성된다.
//    - 같은 manager가 종료 signal(정상 종료)과 강제 종료 signal(dump 후 강제 종료)도 처리한다.
//
// 2. net::emergency::acceptor<TSOCKET>
//    1) acceptor는 system_executor와 완전히 분리된 자체 executor(자체 iocp/epoll과 thread)에서 동작한다.
//    2) 접속한 socket마다 전용 executor(thread 1개)를 가진다. 접속이 끊기면 그 executor는 제거된다.
//       => 한 session이 handler 안에서 멈춰도 acceptor와 다른 session은 영향을 받지 않는다.
//    3) executor의 thread는 최우선(TIME_CRITICAL) 우선순위로 생성된다. (executor_thread_priority()로 변경)
//    4) handler의 예외는 그 socket만 닫는다. acceptor와 executor는 죽지 않는다.
//    5) socket 클래스는 일반 net::acceptor<T>와 같다. (net::socket::tcp<> 등)
//
// 3. net::emergency::manager (process에 하나)
//    1) 종료 signal      (linux : SIGTERM, SIGINT   / windows : ctrl+c)
//       - SETTING::fn_shutdown을 부른다. 여기서 service를 stop/destroy하면 정상 종료된다.
//    2) emergency signal (linux : SIGUSR1           / windows : ctrl+break)
//       - emergency acceptor를 생성해 시작한다. 두 번째 signal은 무시된다.
//    3) 강제 종료 signal  (linux : SIGUSR2           / windows : raise(SIGTERM))
//       - dump를 만들고 모든 접속을 강제로 끊은 뒤 process를 바로 끝낸다.
//    4) signal handler는 flag만 세우고, 실제 처리는 manager의 전용 thread(최우선 우선순위)가 한다.
//    5) trigger(eSIGNAL)로 코드에서도 같은 처리를 할 수 있다. (관리 명령 등)
//    6) emergency acceptor와 executor는 system_executor가 정상 종료(destroy)될 때만 같이 종료된다.
//
//    ex) 서비스에서 쓰기
//        net::emergency::manager::SETTING setting;
//        setting.fn_shutdown = []() { g_pservice->stop(); g_pservice.destroy(); };
//        net::emergency::manager::install<admin_socket>({ { net::ip::tcp::v6(), 30000 } }, std::move(setting));
//
//        $ kill -USR1 <pid>    => emergency acceptor가 30000번 port로 listen
//        $ kill -TERM <pid>    => 정상 종료
//        $ kill -USR2 <pid>    => dump 후 강제 종료
//
// 4. 이 tutorial
//    - signal 대신 trigger()로 같은 처리를 순서대로 해 본다. (끝까지 자동으로 진행된다)
//
//-----------------------------------------------------------------------------
#include "cgdk/sdk10/net.socket.h"

#include <iostream>

using namespace CGDK;

// - 메시지 : [uint32 size(header 포함)][text]
shared_buffer make_text_message(std::string_view _text)
{
	auto buf = alloc_shared_buffer(sizeof(uint32_t) + _text.size());
	buf.append<uint32_t>(static_cast<uint32_t>(sizeof(uint32_t) + _text.size()));
	buf.append_text(_text);
	return buf;
}

std::string_view text_of(const sMESSAGE_NETWORK& _msg)
{
	return std::string_view(reinterpret_cast<const char*>(_msg.buf_message.data()) + sizeof(uint32_t), _msg.buf_message.size() - sizeof(uint32_t));
}

// - 조건이 될 때까지 기다린다. (최대 3초)
template <class TCONDITION>
bool wait_for(TCONDITION&& _condition)
{
	for (int i = 0; i < 300 && !_condition(); ++i)
		std::this_thread::sleep_for(10ms);

	return _condition();
}

// 1) 일반 서버 socket (system_executor에서 동작한다)
class socket_normal : public net::socket::tcp<>
{
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->send(_msg.buf_message);

		return eRESULT::DONE;
	}
};

// 2) emergency 서버 socket (자신만의 executor에서 동작한다)
//    - 실제 서비스에서는 여기서 상태 조회, 접속자 정리, 서비스 중지 같은 관리 명령을 처리한다.
class socket_emergency : public net::socket::tcp<>
{
	virtual void on_connect() override
	{
		// - 이 socket의 executor (접속마다 따로 생성된다)
		auto pnameable = dynamic_cast<object::Inameable*>(this->io_context().get());
		std::cout << "   [emergency] connected (executor : '"sv << ((pnameable != nullptr) ? pnameable->name() : std::string()) << "')"sv << '\n';
	}

	virtual void on_disconnect(uint64_t) override
	{
		std::cout << "   [emergency] disconnected"sv << '\n';
	}

	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		// - 받은 text를 그대로 돌려준다.
		std::cout << "   [emergency] command '"sv << text_of(_msg) << "'"sv << '\n';

		this->send(_msg.buf_message);

		return eRESULT::DONE;
	}
};

// 3) client socket
class socket_client : public net::socket::tcp_client<>
{
public:
	std::atomic<bool>	is_connected{ false };
	std::atomic<bool>	is_failed{ false };
	std::atomic<int>	count_received{ 0 };

	virtual void on_connect() override { this->is_connected = true; }
	virtual void on_fail_connect(uint64_t) override { this->is_failed = true; }
	virtual result_code on_message(sMESSAGE_NETWORK&) override { ++this->count_received; return eRESULT::DONE; }
};

int main()
{
	// trace)
	std::cout << "start [tutorial.socket.ex16.emergency_acceptor]..."sv << '\n';

	// - 종료 signal을 받으면 true가 된다.
	std::atomic<bool> is_shutdown{ false };

	//-----------------------------------------------------------------
	// 1. 일반 서버와 emergency manager를 준비한다.
	//-----------------------------------------------------------------
	std::cout << '\n' << "1. the normal acceptor (port 20000) and the emergency manager"sv << '\n';

	// 1) 일반 acceptor (system_executor에서 동작한다)
	auto pacceptor = make_own<net::acceptor<socket_normal>>();
	{
		net::acceptor<socket_normal>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20000 };
		parameter.enable_progress_log = false;
		pacceptor->start(parameter);
	}

	// 2) emergency manager를 설치한다.
	//    - emergency acceptor : socket_emergency로 20001번 port (emergency signal을 받으면 생성된다)
	//    - fn_shutdown : 종료 signal을 받으면 불린다. (여기서는 아래 main loop를 끝낸다)
	{
		net::emergency::manager::SETTING setting;
		setting.fn_shutdown = [&]() { is_shutdown = true; };

		net::emergency::Nacceptor::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), 20001 };
		parameter.enable_progress_log = false;

		net::emergency::manager::install<socket_emergency>(parameter, std::move(setting));
	}

	// - 아직 emergency acceptor는 없다.
	std::cout << " - emergency acceptor : "sv << (net::emergency::manager::acceptor().exist() ? "exist"sv : "none (waiting for the emergency signal)"sv) << '\n';

	//-----------------------------------------------------------------
	// 2. emergency signal => emergency acceptor가 생성된다.
	//    (실제로는 linux 'kill -USR1 <pid>', windows ctrl+break)
	//-----------------------------------------------------------------
	std::cout << '\n' << "2. the emergency signal"sv << '\n';

	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::EMERGENCY);

	wait_for([]() { auto p = net::emergency::manager::acceptor(); return p.exist() && p->is_listening(); });
	std::cout << " - emergency acceptor : "sv << (net::emergency::manager::acceptor().exist() ? "listening on 20001"sv : "fail"sv) << '\n';

	//-----------------------------------------------------------------
	// 3. emergency acceptor에 접속한다.
	//    - client socket도 별도 executor에서 동작시킨다. (system_executor가 막혀도 응답을 받을 수 있도록)
	//      (socket의 io_context()를 start() 전에 지정하면 그 executor를 쓴다)
	//-----------------------------------------------------------------
	std::cout << '\n' << "3. connect to the emergency acceptor"sv << '\n';

	auto pexecutor_client = executor::make_io_context_threads("tutorial client executor", 1);

	auto pclient = make_own<socket_client>();
	pclient->io_context(pexecutor_client.get());
	pclient->start({{ net::ip::address_v4::loopback(), 20001 }});

	if (wait_for([&]() { return pclient->is_connected.load() || pclient->is_failed.load(); }) == false || pclient->is_failed)
	{
		std::cout << " - fail to connect"sv << '\n';
		return 1;
	}

	pclient->send(make_text_message("status"));
	wait_for([&]() { return pclient->count_received.load() == 1; });
	std::cout << " - reply received : "sv << pclient->count_received.load() << '\n';

	//-----------------------------------------------------------------
	// 4. system_executor를 꽉 막아도 emergency 채널은 동작한다.
	//    - system_executor의 모든 thread를 3초 동안 block시킨다.
	//-----------------------------------------------------------------
	std::cout << '\n' << "4. the system_executor jammed (every thread blocked for 3s)"sv << '\n';
	{
		// - 이 tutorial에서는 thread 감시(deadlock dump)를 끈다.
		api::manager_thread::instance()->deadlock_dump(false);

		const auto count_thread = system_executor::thread_count();
		std::atomic<std::size_t> count_jammed{ 0 };

		for (std::size_t i = 0; i < count_thread; ++i)
		{
			system_executor::post(make_executable([&count_jammed]()
			{
				++count_jammed;
				std::this_thread::sleep_for(3s);
				--count_jammed;
			}));
		}
		wait_for([&]() { return count_jammed.load() == count_thread; });
		std::cout << " - "sv << count_jammed.load() << " threads of the system_executor are blocked"sv << '\n';

		// - emergency acceptor에 명령을 보낸다. => 바로 응답이 온다.
		const auto time_begin = std::chrono::steady_clock::now();
		pclient->send(make_text_message("status while jammed"));
		wait_for([&]() { return pclient->count_received.load() == 2; });
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);
		std::cout << " - reply received : "sv << pclient->count_received.load() << " ("sv << elapsed.count() << "ms)"sv << '\n';

		// - 막힘이 풀릴 때까지 기다린다.
		while (count_jammed.load() != 0)
			std::this_thread::sleep_for(10ms);
	}

	//-----------------------------------------------------------------
	// 5. 종료 signal => fn_shutdown => 정상 종료
	//    (실제로는 linux 'kill -TERM <pid>', windows ctrl+c)
	//-----------------------------------------------------------------
	std::cout << '\n' << "5. the shutdown signal"sv << '\n';

	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::SHUTDOWN);

	// - 실제 서비스의 main loop (종료 signal을 받을 때까지)
	while (is_shutdown == false)
		std::this_thread::sleep_for(10ms);

	std::cout << " - shutdown requested : stop and destroy"sv << '\n';

	// 1) client
	pclient->closesocket();
	std::this_thread::sleep_for(100ms);
	pexecutor_client->stop();

	// 2) 일반 acceptor
	pacceptor->stop();
	pacceptor.destroy();

	// 3) system_executor를 정상 종료한다.
	//    - emergency acceptor와 그 executor들은 여기서 같이 종료된다. (manager가 system_executor의 destroy handler로 등록되어 있다)
	system_executor::destroy_instance();

	std::cout << " - emergency acceptor : "sv << (net::emergency::manager::acceptor().exist() ? "exist"sv : "stopped"sv) << '\n';

	// trace)
	std::cout << '\n' << "stop [tutorial.socket.ex16.emergency_acceptor]..."sv << '\n';

	// end) 종료
	return 0;
}
