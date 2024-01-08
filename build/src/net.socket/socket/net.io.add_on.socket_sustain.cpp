#include "pch.h"
#include "net.io.add_on.manager_socket_sustain.h"

using namespace CGDK;


net::io::add_on::Nsocket_sustain::Nsocket_sustain() : 
	m_tick_timeout_connect(chrono::tick::duration::zero()),
	m_tick_timeout_disconnect(chrono::tick::duration::zero()),
	m_tick_timeout_receive(chrono::seconds(300)),
	m_tick_timeout_send(chrono::tick::duration::zero())
{
	this->start_socket_sustain();
}

net::io::add_on::Nsocket_sustain::~Nsocket_sustain() 
{
	this->stop_socket_sustain();
}

bool net::io::add_on::Nsocket_sustain::start_socket_sustain()
{
	// 1) SocketSustain에 붙인다.
	object_ptr<manager_socket_sustain> pmanager = manager_socket_sustain::instance();

	// 2) socket을 Add한다.
	auto result = pmanager->add_socket(this);

	// check) 
	RETURN_IF(result ==false, false);

	// 3) 값을 설정한다.
	this->m_pmanager = pmanager;

	// return) 성공...
	return true;
}

bool net::io::add_on::Nsocket_sustain::stop_socket_sustain()
{
	// 1) manager를 얻는다.
	object_ptr<manager_socket_sustain> pmanager = this->m_pmanager;

	// check) ... 
	RETURN_IF(pmanager.empty(), false)

	// 2) socket을 제거한다.
	auto result = pmanager->remove_socket(this);

	// check) 
	RETURN_IF(result == false, false);

	// 3) manager를 reset한다.
	this->m_pmanager.reset();

	// return) 성공...
	return true;
}

void net::io::add_on::Nsocket_sustain::process_socket_sustain()
{
	// 1) 현재의 Tick을 얻는다.
	auto tick_now = chrono::tick::now();

	if(this->socket_state() == eSOCKET_STATE::ESTABLISHED)
	{
		// 2) connect관련 sustain을 수행한다.
		if(m_tick_timeout_connect != chrono::tick::duration::zero())
		{
			// - 접속한 이후의 시간을 구한다.
			auto tick_differ_execute = tick_now - this->statistics_get_tick_connect();

			// check) 접속된 후 m_tick_timeout_connect보다 시간이 더 지났으면 on_timeout_connect를 호출한다.
			if(tick_differ_execute >= this->m_tick_timeout_connect)
			{
				// - on_timeout_send()함수를 호출한다.
				this->on_timeout_connect(tick_differ_execute);
			}
		}

		// 3) receive Auto Closer를 수행한다.
		if(m_tick_timeout_receive != chrono::tick::duration::zero())
		{
			// - 마지막 전송받은 이후 지난 시간을 구한다.
			auto tick_differ_execute = tick_now - this->statistics_get_tick_last_receive();

			// check) 만약 마지막 전송받은 후 지난시간이 m_tick_timeout_receive보다 지났으면 on_timeout_receive()를 호출한다.
			if(tick_differ_execute >= this->m_tick_timeout_receive) 
			{
				// - on_timeout_send()함수를 호출한다.
				this->on_timeout_receive(tick_differ_execute);
			}
		}

		// 4) send Auto Closer를 수행한다.
		if(this->m_tick_timeout_send != chrono::tick::duration::zero())
		{
			// - 마지막 전송한 이후 지난 시간을 구한다.
			auto tick_differ_execute = tick_now - this->statistics_get_tick_last_send();

			// check) 만약 마지막 전송한 후 지난시간이 m_tick_timeout_receive보다 지났으면 on_timeout_send()를 호출한다.
			if(tick_differ_execute >= m_tick_timeout_send)
			{
				// - on_timeout_send()함수를 호출한다.
				this->on_timeout_send(tick_differ_execute);
			}
		}
	}
	else if(this->socket_state() == eSOCKET_STATE::CLOSED)
	{
		// 5) disconnect관련 sustain을 수행한다.
		if(this->m_tick_timeout_disconnect != chrono::tick::duration::zero())
		{
			// - 마지막 받은 receive tick을 얻는다.
			auto tick_last_disconnect = this->statistics_get_tick_disconnect();

			// check) tick_last_disconnect값이 0이면 아직 접속이 되지 않았거나 접속종료가 되지 않았다는 뜻이다.
			if(tick_last_disconnect != chrono::tick::time_point(chrono::tick::duration::zero()))
			{
				// - 접속 종료한 시점 이후의 tick을 구한다.
				auto tick_differ_execute = tick_now - tick_last_disconnect;

				// check) 접속된 후 m_tick_timeout_connect보다 시간이 더 지났으면 on_timeout_connect를 호출한다.

				if(tick_differ_execute >= this->m_tick_timeout_disconnect)
				{
					// - on_timeout_send()함수를 호출한다.
					this->on_timeout_disconnect(tick_differ_execute);
				}
			}
		}
	}
}

void net::io::add_on::Nsocket_sustain::detach_instance() noexcept
{
	manager_socket_sustain::detach_instance();
}
