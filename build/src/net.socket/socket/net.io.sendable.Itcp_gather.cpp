#include "pch.h"

using namespace CGDK;

std::size_t net::io::sendable::Itcp_gather::m_s_default_max_depth_of_send_buffer = DEFAULT_MAX_DEPTH_OF_MESSAGE_SEND_QUEUE;
std::size_t net::io::sendable::Itcp_gather::m_s_default_max_queued_bytes = DEFAULT_BYTES_MAX_MESSAGE_SEND_QUEUE;

net::io::sendable::Itcp_gather::Itcp_gather() noexcept :
	m_bytes_max_queued{ net::io::sendable::Itcp_gather::default_maximum_queued_bytes() },
	m_max_depth_of_send_buffer{ net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer() }
{
}

void net::io::sendable::Itcp_gather::maximum_depth_of_send_buffer(std::size_t _depth) noexcept
{
	// check) 
	RETURN_IF(this->m_max_depth_of_send_buffer == _depth);

	// 1) 
	this->m_max_depth_of_send_buffer = _depth;
}

void net::io::sendable::Itcp_gather::maximum_depth_of_send_buffer_bytes(std::size_t _size) noexcept
{
	// check) 
	RETURN_IF(this->m_bytes_max_queued == _size);

	// 1) 
	this->m_bytes_max_queued = _size;
}

void net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer(std::size_t _max) noexcept
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_max == m_s_default_max_depth_of_send_buffer);

	// check) 최소 4 최대 16384로 한다.
	SATURATE(_max, 4, (4096 * 4));

	// 1) 값을 설정한다.
	m_s_default_max_depth_of_send_buffer = _max;

	// log) 
	LOG_INFO << log_level_t::important() << "(info) set " << m_s_default_max_depth_of_send_buffer << " as DEFAULT MAX SENDBUFFER DEPTH of net::io::sendable::Itcp_gather.";
}

std::size_t net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer() noexcept
{
	return m_s_default_max_depth_of_send_buffer;
}

void net::io::sendable::Itcp_gather::default_maximum_queued_bytes(std::size_t _max) noexcept
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_max == m_s_default_max_queued_bytes);

	// check) 최소 256K로 설정한다.
	SATURATE_MIN(_max, (256 * 1024));

	// 1) 값을 설정한다.
	m_s_default_max_queued_bytes = _max;

	// log) 
	LOG_INFO << log_level_t::important() << "(info) set " << m_s_default_max_queued_bytes << " bytes as DEFAULT MAX SUSTAIN QUEUED BYTE of net::io::sendable::Itcp_gather.";
}

std::size_t net::io::sendable::Itcp_gather::default_maximum_queued_bytes() noexcept
{
	return m_s_default_max_queued_bytes;
}
