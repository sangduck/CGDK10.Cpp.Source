#include "pch.h"

using namespace CGDK;


statistics::unit_traffic::unit_traffic(const unit_traffic& _copy) noexcept :
	tick_snap				(_copy.tick_snap),
	count_received_message	((uint64_t)_copy.count_received_message),
	count_received_bytes	((uint64_t)_copy.count_received_bytes),
	count_sent_message	((uint64_t)_copy.count_sent_message),
	count_sent_bytes		((uint64_t)_copy.count_sent_bytes),
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
	this->count_sent_message.store(_rhs.count_sent_message);
	this->count_sent_bytes.store(_rhs.count_sent_bytes);
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
	this->count_sent_message = _rhs.count_sent_message;
	this->count_sent_bytes = _rhs.count_sent_bytes;
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
	this->count_sent_message = 0;
	this->count_sent_bytes = 0;
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
	this->count_sent_message = _buffer.extract<uint64_t>();
	this->count_sent_bytes = _buffer.extract<uint64_t>();
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
	_buffer.append<uint64_t>(this->count_sent_message);
	_buffer.append<uint64_t>(this->count_sent_bytes);
	_buffer.append<uint64_t>(this->count_send_io);
	_buffer.append<uint64_t>(this->count_receive_io);
	_buffer.append<uint64_t>(this->count_general_io);
	_buffer.append<uint64_t>(this->count_error_on_send);
	_buffer.append<uint64_t>(this->count_error_on_receive);
}

std::size_t	statistics::unit_traffic::process_size_of() const
{
	return get_size_of(this->tick_snap)
		 + get_size_of(this->count_received_message)
		 + get_size_of(this->count_received_bytes)
		 + get_size_of(this->count_sent_message)
		 + get_size_of(this->count_sent_bytes)
		 + get_size_of(this->count_send_io)
		 + get_size_of(this->count_receive_io)
		 + get_size_of(this->count_general_io)
		 + get_size_of(this->count_error_on_send)
		 + get_size_of(this->count_error_on_receive);
}

statistics::unit_traffic_snap& statistics::unit_traffic_snap::operator=(const unit_traffic& _rhs) noexcept
{
	this->tick_snap = _rhs.tick_snap;
	this->count_received_message = _rhs.count_received_message;
	this->count_received_bytes = _rhs.count_received_bytes;
	this->count_sent_message = _rhs.count_sent_message;
	this->count_sent_bytes = _rhs.count_sent_bytes;
	this->count_send_io = _rhs.count_send_io;
	this->count_receive_io = _rhs.count_receive_io;
	this->count_general_io = _rhs.count_general_io;
	this->count_error_on_send = _rhs.count_error_on_send;
	this->count_error_on_receive = _rhs.count_error_on_receive;

	// return)
	return *this;
}
