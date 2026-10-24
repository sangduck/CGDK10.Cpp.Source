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
	return get_size_of(this->tick_snap)
		+ get_size_of(this->count_try)
		+ get_size_of(this->count_success_connect)
		+ get_size_of(this->count_fail_connect)
		+ get_size_of(this->count_disconnect)
		+ get_size_of(this->count_error_disconnect)
		+ get_size_of(this->count_keep)
		+ get_size_of(this->count_async);
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
	// 1) Traffic Info¸¦ resetÇÑ´Ù.
	this->m_statistics_connection.reset();
	this->m_count_async = 0;
}
