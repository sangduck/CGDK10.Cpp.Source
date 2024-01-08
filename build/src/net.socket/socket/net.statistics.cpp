#include "pch.h"

using namespace CGDK;


void statistics::unit_connective::process_serialize_in(buffer_view& _buffer)
{
	this->tick_snap = _buffer.extract<chrono::tick::time_point>();
	this->count_try = _buffer.extract<uint64_t>();
	this->count_success_connect = _buffer.extract<uint64_t>();
	this->count_fail_connect = _buffer.extract<uint64_t>();
	this->count_disconnect = _buffer.extract<uint64_t>();
	this->count_error_disconnect = _buffer.extract<uint64_t>();
	this->count_keep = _buffer.extract<uint64_t>();
	this->count_async = _buffer.extract<uint64_t>();
}
void statistics::unit_connective::process_serialize_out(buffer& _buffer)
{
	_buffer.append<chrono::tick::time_point>(this->tick_snap);
	_buffer.append<uint64_t>(this->count_try);
	_buffer.append<uint64_t>(this->count_success_connect);
	_buffer.append<uint64_t>(this->count_fail_connect);
	_buffer.append<uint64_t>(this->count_disconnect);
	_buffer.append<uint64_t>(this->count_error_disconnect);
	_buffer.append<uint64_t>(this->count_keep);
	_buffer.append<uint64_t>(this->count_async);
}
std::size_t statistics::unit_connective::unit_connective::process_size_of() const
{
	return sizeof(chrono::tick::time_point)
		+ sizeof(chrono::tick::time_point)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t);
}

statistics::unit_connective::unit_connective(const statistics::unit_connective& _copy) noexcept:
	tick_snap				(_copy.tick_snap),
	count_try				((uint64_t)_copy.count_try),
	count_success_connect	((uint64_t)_copy.count_success_connect),
	count_fail_connect		((uint64_t)_copy.count_fail_connect),
	count_disconnect		((uint64_t)_copy.count_disconnect),
	count_error_disconnect	((uint64_t)_copy.count_error_disconnect),
	count_keep				((uint64_t)_copy.count_keep),
	count_async				((uint64_t)_copy.count_async)
{
}

statistics::unit_connective& statistics::unit_connective::operator =(const statistics::unit_connective& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_try.store(_rhs.count_try);
	this->count_success_connect.store(_rhs.count_success_connect);
	this->count_fail_connect.store(_rhs.count_fail_connect);
	this->count_disconnect.store(_rhs.count_disconnect);
	this->count_error_disconnect.store(_rhs.count_error_disconnect);
	this->count_keep.store(_rhs.count_keep);
	this->count_async.store(_rhs.count_async);

	// return)
	return *this;
}

statistics::unit_connective& statistics::unit_connective::operator =(const statistics::unit_connective_snap& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_try = _rhs.count_try;
	this->count_success_connect = _rhs.count_success_connect;
	this->count_fail_connect = _rhs.count_fail_connect;
	this->count_disconnect = _rhs.count_disconnect;
	this->count_error_disconnect = _rhs.count_error_disconnect;
	this->count_keep = _rhs.count_keep;
	this->count_async = _rhs.count_async;

	// return)
	return *this;
}

void statistics::unit_connective::reset() noexcept
{
	this->tick_snap = chrono::tick::time_point::min();
	this->count_try = 0;
	this->count_success_connect = 0;
	this->count_fail_connect = 0;
	this->count_disconnect = 0;
	this->count_error_disconnect = 0;
	this->count_keep = 0;
	this->count_async = 0;
}

statistics::unit_connective_snap& statistics::unit_connective_snap::operator =(const unit_connective& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_try = _rhs.count_try;
	this->count_success_connect = _rhs.count_success_connect;
	this->count_fail_connect = _rhs.count_fail_connect;
	this->count_disconnect = _rhs.count_disconnect;
	this->count_error_disconnect = _rhs.count_error_disconnect;
	this->count_keep = _rhs.count_keep;
	this->count_async = _rhs.count_async;

	// return)
	return *this;
}

statistics::unit_traffic::unit_traffic(const unit_traffic& _copy) noexcept :
	tick_snap				(_copy.tick_snap),
	count_received_message	((uint64_t)_copy.count_received_message),
	count_received_bytes	((uint64_t)_copy.count_received_bytes),
	count_sended_message	((uint64_t)_copy.count_sended_message),
	count_sended_bytes		((uint64_t)_copy.count_sended_bytes),
	count_send_io			((uint64_t)_copy.count_send_io),
	count_receive_io		((uint64_t)_copy.count_receive_io),
	count_general_io		((uint64_t)_copy.count_general_io),
	count_error_on_send		((uint64_t)_copy.count_error_on_send),
	count_error_on_receive	((uint64_t)_copy.count_error_on_receive)
{
}

statistics::unit_traffic& statistics::unit_traffic::operator=(const unit_traffic& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_received_message.store(_rhs.count_received_message);
	this->count_received_bytes.store(_rhs.count_received_bytes);
	this->count_sended_message.store(_rhs.count_sended_message);
	this->count_sended_bytes.store(_rhs.count_sended_bytes);
	this->count_send_io.store(_rhs.count_send_io);
	this->count_receive_io.store(_rhs.count_receive_io);
	this->count_general_io.store(_rhs.count_general_io);
	this->count_error_on_send.store(_rhs.count_error_on_send);
	this->count_error_on_receive.store(_rhs.count_error_on_receive);

	// return)
	return	*this;
}

statistics::unit_traffic& statistics::unit_traffic::operator=(const unit_traffic_snap& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_received_message = _rhs.count_received_message;
	this->count_received_bytes = _rhs.count_received_bytes;
	this->count_sended_message = _rhs.count_sended_message;
	this->count_sended_bytes = _rhs.count_sended_bytes;
	this->count_send_io = _rhs.count_send_io;
	this->count_receive_io = _rhs.count_receive_io;
	this->count_general_io = _rhs.count_general_io;
	this->count_error_on_send = _rhs.count_error_on_send;
	this->count_error_on_receive = _rhs.count_error_on_receive;

	// return)
	return	*this;
}

void statistics::unit_traffic::reset() noexcept
{
	this->tick_snap = chrono::tick::time_point::min();
	this->count_received_message = 0;
	this->count_received_bytes = 0;
	this->count_sended_message = 0;
	this->count_sended_bytes = 0;
	this->count_send_io = 0;
	this->count_receive_io = 0;
	this->count_general_io = 0;
	this->count_error_on_send = 0;
	this->count_error_on_receive = 0;
}

void statistics::unit_traffic::process_serialize_in(buffer_view& _buffer)
{
	this->tick_snap = _buffer.extract<chrono::tick::time_point>();
	this->count_received_message = _buffer.extract<uint64_t>();
	this->count_received_bytes = _buffer.extract<uint64_t>();
	this->count_sended_message = _buffer.extract<uint64_t>();
	this->count_sended_bytes = _buffer.extract<uint64_t>();
	this->count_send_io = _buffer.extract<uint64_t>();
	this->count_receive_io = _buffer.extract<uint64_t>();
	this->count_general_io = _buffer.extract<uint64_t>();
	this->count_error_on_send = _buffer.extract<uint64_t>();
	this->count_error_on_receive = _buffer.extract<uint64_t>();
}

void statistics::unit_traffic::process_serialize_out(buffer& _buffer)
{
	_buffer.append<chrono::tick::time_point>(this->tick_snap);
	_buffer.append<uint64_t>(this->count_received_message);
	_buffer.append<uint64_t>(this->count_received_bytes);
	_buffer.append<uint64_t>(this->count_sended_message);
	_buffer.append<uint64_t>(this->count_sended_bytes);
	_buffer.append<uint64_t>(this->count_send_io);
	_buffer.append<uint64_t>(this->count_receive_io);
	_buffer.append<uint64_t>(this->count_general_io);
	_buffer.append<uint64_t>(this->count_error_on_send);
	_buffer.append<uint64_t>(this->count_error_on_receive);
}

std::size_t	statistics::unit_traffic::process_size_of() const
{
	return sizeof(chrono::tick::time_point)
		+ sizeof(chrono::tick::time_point)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t)
		+ sizeof(uint64_t);
}

statistics::unit_traffic_snap& statistics::unit_traffic_snap::operator=(const unit_traffic& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_received_message = _rhs.count_received_message;
	this->count_received_bytes = _rhs.count_received_bytes;
	this->count_sended_message = _rhs.count_sended_message;
	this->count_sended_bytes = _rhs.count_sended_bytes;
	this->count_send_io = _rhs.count_send_io;
	this->count_receive_io = _rhs.count_receive_io;
	this->count_general_io = _rhs.count_general_io;
	this->count_error_on_send = _rhs.count_error_on_send;
	this->count_error_on_receive = _rhs.count_error_on_receive;

	// return)
	return *this;
}

net::io::statistics::Nconnective::Nconnective() noexcept : 
	m_statistics_connection(),
	m_connective_id(0),
	m_time_create(),
	m_time_start(),
	m_time_stop(),
	m_count_async(0)
{
	this->reset_statistics();
}

net::io::statistics::Nconnective::~Nconnective() noexcept
{
}

void net::io::statistics::Nconnective::reset_statistics() noexcept
{
	// 1) Traffic Info를 reset한다.
	this->m_statistics_connection.reset();
	this->m_count_async = 0;
}

net::io::statistics::Nsocket::Nsocket():
	m_statistics_traffic(),
	m_count_total_connect_try(0),
	m_count_total_connect(0),
	m_count_connect_try(0),
	m_tick_disconnect(chrono::tick::duration::zero())
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

	// 4) set 'enable socket logger'
	this->m_enable_socket_debug_logging = net::io::is_default_socket_debug_logging_enabled();
}
