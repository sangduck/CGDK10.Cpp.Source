#include "pch.h"

using namespace CGDK;


net::io::add_on::Nkeep_alive::Nkeep_alive() noexcept : m_ping_key{0,0}
{
	// 1) ..
	this->m_nested.m_pParent = this;

	// 2) 10초마다 Ping을 날린다.
	this->m_nested.m_tick_ping_interval = chrono::seconds(10);

	// 3) TimeOut함수.(12초)
	this->m_nested.m_tick_timeout = chrono::seconds(12);
}


bool net::io::add_on::Nkeep_alive::start_keep_alive()
{
	// 1) Random하게 Key값 얻기.
	this->m_ping_key[0] = rand();
	this->m_ping_key[1] = rand();

	// 2) 기타값을 설정.
	this->m_nested.m_tick_tat = chrono::tick::duration::zero();

	// 3) Progress를 붙인다.
	system_executor::register_schedulable(&m_nested);

	// 4) messageable을 붙인다.
	CGASSERT_ERROR(false);
	//AttachMessageable(&m_nested);

	// return) 성공...
	return true;
}


bool net::io::add_on::Nkeep_alive::stop_keep_alive() noexcept
{
	// 1) Progress를 떼낸다.
	system_executor::unregister_schedulable(&m_nested);

	// 2) messageable을 떼낸다.
	CGASSERT_ERROR(false);
	//detachMessageable(&m_nested);

	// return) 성공...
	return true;
}


bool net::io::add_on::Nkeep_alive::send_ping_req() noexcept
{
	// 1) shared_buffer를 할당한다.
	shared_buffer temp_buffer = alloc_shared_buffer(128);

	// 2) message를 만든다.
	temp_buffer.append<uint32_t>(24);
	temp_buffer.append<uint32_t>(eMESSAGE::NETWORK::PING_REQ);
	temp_buffer.append<uint32_t>(m_ping_key[0]);
	temp_buffer.append<uint32_t>(m_ping_key[1]);
	temp_buffer.append<chrono::tick::time_point>(chrono::tick::now());

	// declare)
	net::sockaddr address_peer;

	// 3) Ping을 날린다.
	return this->process_sendable(temp_buffer, temp_buffer.move_out_source(), 1, address_peer);
}

intptr_t net::io::add_on::Nkeep_alive::nested_update()
{
	// declare) 
	chrono::tick::time_point tick_now = chrono::tick::now();
	chrono::tick::time_point tick_threshold;

	//  1) TimeOut Check한다.
	auto tick_last_received = this->statistics_get_tick_last_receive();

	tick_threshold = tick_last_received + this->m_nested.m_tick_timeout;
	if(tick_now > tick_threshold)
	{
		// - OnTimeOutKeepAlive()함수를 호출한다.
		this->on_keep_alive_time_out();

		// return) 
		return intptr_t(1);
	}

	//  2) Ping Check한다.
	tick_threshold = tick_last_received + this->m_nested.m_tick_ping_interval;

	if(tick_now > tick_threshold)
	{
		this->send_ping_req();
	}

	// return) 
	return intptr_t(0);
}


result_code net::io::add_on::Nkeep_alive::nested_process_message(sMESSAGE& _msg)
{
	// 1) casting
	sMESSAGE_NETWORK& msg_network = (sMESSAGE_NETWORK&)_msg;

	// 2) message를 얻는다.
	uint32_t message = msg_network.buf_message.front<uint32_t>(4);

	// 3) ...
	switch(message)
	{
	// case 1) binding의 응답이 왔을 때...(당연히 Hole Punched로 처리한다.)
	case	eMESSAGE::NETWORK::PING_ACK:
			{
				uint32_t key_a = msg_network.buf_message.front<uint32_t>(8);
				uint32_t key_b = msg_network.buf_message.front<uint32_t>(12);

				// check) Key가 다르면 false를 Return한다.
				RETURN_IF(key_a != this->m_ping_key[0] || key_b != m_ping_key[1], false);

				// 1) Turn Arround Time을 Update한다.
				this->m_nested.m_tick_tat = chrono::tick::now() - msg_network.buf_message.front<chrono::tick::time_point>(16);
			}
			return eRESULT::DONE;

	// case 2) ...
	case	eMESSAGE::NETWORK::PING_REQ:
			{
				// 1) message에 전송받을 것을 ACK로만 바꿔서 바로 전송.
				msg_network.buf_message.front<uint32_t>(4) = eMESSAGE::NETWORK::PING_ACK;

				// 2) Ping을 날린다.
				this->process_sendable(msg_network.buf_message, msg_network.buf_message.get_source(), 1, msg_network.address);
			}
			return eRESULT::DONE;
	};

	// return) message를 처리하지 못했으므로 false를 Return한다.
	return eRESULT::BYPASS;
}

void net::io::add_on::Nkeep_alive::on_keep_alive_time_out()
{
}
