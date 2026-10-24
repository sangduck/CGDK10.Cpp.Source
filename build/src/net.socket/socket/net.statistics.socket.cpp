#include "pch.h"

using namespace CGDK;


net::io::statistics::Nsocket::Nsocket():
	m_statistics_traffic(),
	m_count_total_connect_try(0),
	m_count_total_connect(0),
	m_count_connect_try(0),
	m_tick_disconnect(chrono::tick::time_point(chrono::tick::duration::zero()))
{
	this->reset_statistics();
}

net::io::statistics::Nsocket::~Nsocket() noexcept
{
}

void net::io::statistics::Nsocket::reset_statistics() noexcept
{
	// 1) Traffic Info를 reset한다.
	m_statistics_traffic.reset();

	// 2) I/O관련...
	this->m_count_async_con_dis = 0;
	this->m_count_async_sending = 0;
	this->m_count_async_receiving = 0;
	this->m_is_error_disconnect = false;

	// 3) 전송시간 관련
	this->m_tick_last_receive = chrono::tick::time_point(chrono::tick::duration::zero());
	this->m_tick_last_send = chrono::tick::time_point(chrono::tick::duration::zero());
	this->m_tick_wait_time = chrono::tick::time_point(chrono::tick::duration::zero());
	this->m_tick_connect_try = chrono::tick::time_point(chrono::tick::duration::zero());

	// 4) set 'enable socket logger'
	this->m_enable_socket_debug_logging = net::io::is_default_socket_debug_logging_enabled();
}

void net::io::statistics::Nsocket::sTRAFFIC_TOTAL_TLS::flush() noexcept
{
	// check)
	if (this->count_pending == 0)
		return;

	// 1) add to the total (relaxed : the order with other data is not needed)
	auto& total = m_statistics_traffic_total;
	auto f_add = [](std::atomic<uint64_t>& _dest, uint64_t& _value) noexcept
	{
		if (_value != 0)
		{
			_dest.fetch_add(_value, std::memory_order_relaxed);
			_value = 0;
		}
	};
	f_add(total.count_received_message, this->count_received_message);
	f_add(total.count_received_bytes, this->count_received_bytes);
	f_add(total.count_sent_message, this->count_sent_message);
	f_add(total.count_sent_bytes, this->count_sent_bytes);
	f_add(total.count_send_io, this->count_send_io);
	f_add(total.count_receive_io, this->count_receive_io);
	f_add(total.count_general_io, this->count_general_io);

	// 2) reset
	this->count_pending = 0;
}
