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

void net::io::add_on::Nsocket_sustain::set_sustain_timeout(const sTIMEOUT& _timeout, sTIMEOUT_COUNT* _pcount) noexcept
{
	this->m_tick_timeout_connecting = _timeout.connecting;
	this->m_tick_timeout_certify = _timeout.certify;
	this->m_tick_timeout_receive = _timeout.idle;
	this->m_tick_ping_interval = _timeout.ping_interval;
	this->m_pcount_timeout = _pcount;
}

void net::io::add_on::Nsocket_sustain::_close_by_sustain_timeout(std::atomic<uint64_t> sTIMEOUT_COUNT::* _pcount)
{
	// statistics)
	if (this->m_pcount_timeout != nullptr && _pcount != nullptr)
		(this->m_pcount_timeout->*_pcount).fetch_add(1, std::memory_order_relaxed);

	// - close
	this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_SUSTAIN_TIMEOUT);
}

bool net::io::add_on::Nsocket_sustain::_is_sustain_target() noexcept
{
	// - 이 class와 기반의 값만 읽는다. (가상 함수를 부르지 않는다)
	const auto state_now = this->socket_state();

	// - 접속 중이 아니면 '처음 본 시각'을 지운다. (대상이 아니면 process_socket_sustain()이 불리지 않으므로 여기서 지운다)
	if (state_now != eSOCKET_STATE::SYN)
		this->m_tick_connecting_start = chrono::tick::time_point();

	switch (state_now)
	{
	// 1) 접속 중
	case	eSOCKET_STATE::SYN:
			return this->m_tick_timeout_connecting != chrono::tick::duration::zero();

	// 2) 접속됨
	case	eSOCKET_STATE::ESTABLISHED:
	case	eSOCKET_STATE::CERTIFIED:
	case	eSOCKET_STATE::LOGINED:
			return this->m_tick_timeout_certify != chrono::tick::duration::zero()
				|| this->m_tick_timeout_connect != chrono::tick::duration::zero()
				|| this->m_tick_timeout_receive != chrono::tick::duration::zero()
				|| this->m_tick_timeout_send != chrono::tick::duration::zero()
				|| this->m_tick_ping_interval != chrono::tick::duration::zero();

	// 3) 접속 종료됨 (접속 종료한 적이 있을 때만)
	case	eSOCKET_STATE::CLOSED:
			return this->m_tick_timeout_disconnect != chrono::tick::duration::zero()
				&& this->statistics_get_tick_disconnect() != chrono::tick::time_point(chrono::tick::duration::zero());

	// 4) 그 외 (접속 종료 중 등) : 할 것이 없다.
	default:
			return false;
	}
}

void net::io::add_on::Nsocket_sustain::process_socket_sustain()
{
	// 1) 현재의 Tick을 얻는다.
	const auto tick_now = chrono::tick::now();
	const auto state_now = this->socket_state();

	// 2) 접속 중(SYN) : 접속 시간 제한
	//    - 접속 요청 시각부터 센다. (statistics_on_connect_try)
	//    - 기록되지 않은 경로(ex. connector)는 접속 중인 것을 처음 본 시각부터 센다. (sustain 주기만큼 늦을 수 있다)
	if (state_now == eSOCKET_STATE::SYN)
	{
		if (this->m_tick_timeout_connecting != chrono::tick::duration::zero())
		{
			auto tick_start = this->statistics_get_tick_connect_try();

			if (tick_start == chrono::tick::time_point())
			{
				if (this->m_tick_connecting_start == chrono::tick::time_point())
					this->m_tick_connecting_start = tick_now;

				tick_start = this->m_tick_connecting_start;
			}

			if (tick_now - tick_start >= this->m_tick_timeout_connecting)
			{
				this->m_tick_connecting_start = chrono::tick::time_point();
				this->_close_by_sustain_timeout(&sTIMEOUT_COUNT::connecting);
			}
		}

		// return)
		return;
	}

	// - 접속 중이 아니다.
	this->m_tick_connecting_start = chrono::tick::time_point();

	// - connected (ESTABLISHED, CERTIFIED(ex. http after the first request), LOGINED)
	if (state_now == eSOCKET_STATE::ESTABLISHED || state_now == eSOCKET_STATE::CERTIFIED || state_now == eSOCKET_STATE::LOGINED)
	{
		// 3) 접속 후 CERTIFIED가 되지 않았다. (ex. http : 첫 요청이 끝나지 않았다)
		if (this->m_tick_timeout_certify != chrono::tick::duration::zero() && state_now == eSOCKET_STATE::ESTABLISHED && tick_now - this->statistics_get_tick_connect() >= this->m_tick_timeout_certify)
		{
			this->_close_by_sustain_timeout(&sTIMEOUT_COUNT::certify);
			return;
		}

		// 4) 접속된 후 m_tick_timeout_connect보다 시간이 더 지났으면 on_timeout_connect를 호출한다.
		if (this->m_tick_timeout_connect != chrono::tick::duration::zero())
		{
			const auto tick_differ_execute = tick_now - this->statistics_get_tick_connect();

			if (tick_differ_execute >= this->m_tick_timeout_connect)
				this->on_timeout_connect(tick_differ_execute);
		}

		// 5) 받은 것이 없다.
		{
			// - 마지막 전송받은 이후 지난 시간을 구한다.
			const auto tick_differ_execute = tick_now - this->statistics_get_tick_last_receive();

			// - m_tick_timeout_receive(idle)보다 지났으면 on_timeout_receive()를 호출한다. (기본 : 끊는다)
			if (this->m_tick_timeout_receive != chrono::tick::duration::zero() && tick_differ_execute >= this->m_tick_timeout_receive)
				this->on_timeout_receive(tick_differ_execute);

			// - keepalive : ping_interval 동안 받은 것이 없으면 PING을 보내고(ping_interval마다 한 번), 두 배의 시간 동안 받은 것이 없으면(PONG도 없음) 끊는다.
			if (this->m_tick_ping_interval != chrono::tick::duration::zero() && tick_differ_execute >= this->m_tick_ping_interval && this->socket_state() != eSOCKET_STATE::CLOSED)
			{
				if (tick_differ_execute >= this->m_tick_ping_interval * 2)
				{
					this->_close_by_sustain_timeout(&sTIMEOUT_COUNT::idle);
					return;
				}

				if (tick_now - this->m_tick_last_ping >= this->m_tick_ping_interval && this->process_keepalive_ping())
					this->m_tick_last_ping = tick_now;
			}
		}

		// 6) send Auto Closer를 수행한다.
		if (this->m_tick_timeout_send != chrono::tick::duration::zero())
		{
			// - 마지막 전송한 이후 지난 시간을 구한다.
			const auto tick_differ_execute = tick_now - this->statistics_get_tick_last_send();

			// - 만약 마지막 전송한 후 지난시간이 m_tick_timeout_send보다 지났으면 on_timeout_send()를 호출한다.
			if (tick_differ_execute >= this->m_tick_timeout_send)
				this->on_timeout_send(tick_differ_execute);
		}
	}
	else if (state_now == eSOCKET_STATE::CLOSED)
	{
		// 7) disconnect관련 sustain을 수행한다.
		if (this->m_tick_timeout_disconnect != chrono::tick::duration::zero())
		{
			// - 마지막 접속 종료 tick을 얻는다. (0이면 아직 접속이 되지 않았거나 접속종료가 되지 않았다는 뜻이다)
			const auto tick_last_disconnect = this->statistics_get_tick_disconnect();

			if (tick_last_disconnect != chrono::tick::time_point(chrono::tick::duration::zero()))
			{
				// - 접속 종료한 시점 이후의 tick을 구한다.
				const auto tick_differ_execute = tick_now - tick_last_disconnect;

				if (tick_differ_execute >= this->m_tick_timeout_disconnect)
					this->on_timeout_disconnect(tick_differ_execute);
			}
		}
	}
}

void net::io::add_on::Nsocket_sustain::detach_instance() noexcept
{
	manager_socket_sustain::detach_instance();
}
