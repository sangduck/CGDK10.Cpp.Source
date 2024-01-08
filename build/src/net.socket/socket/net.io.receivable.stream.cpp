#include "pch.h"

using namespace CGDK;

std::size_t	g_bytes_message_buffer_default_min_size	 = DEFAULT_BYTES_TCP_MESSAGE_BUFFER;
std::size_t	g_bytes_message_buffer_default_max_size	 = DEFAULT_BYTES_MAX_TCP_MESSAGE_BUFFER;


void net::io::receivable::default_message_buffer_min_size(std::size_t _bytes_size)
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_bytes_size == g_bytes_message_buffer_default_min_size);

	// check) 최소 1Kbyte보다 작으면 안됀다.
	CGASSERT_ERROR(_bytes_size<1024);

	// check) 최대 MAX_TCP_MESSAGE_SIZE(512NBytes)를 넘으면 안됀다.
	CGASSERT_ERROR(_bytes_size <= MAX_TCP_MESSAGE_SIZE);

	// check) 최소 1Kbyte 최대 512Mbyte로 제한한다....
	SATURATE(_bytes_size, 1024, MAX_TCP_MESSAGE_SIZE);

	// 1) 값을 설정한다.
	g_bytes_message_buffer_default_min_size	 = _bytes_size;

	// log) 
	LOG_INFO << "(info) CGDK.net.socket: set 'DEFAULT TCP_RECEIVE BUFFER SIZE' to " << static_cast<int64_t>(g_bytes_message_buffer_default_min_size) << " bytes";
}

void net::io::receivable::default_message_buffer_max_size(std::size_t _bytes_size)
{
	// check) 이전 값과 동일하다면 그냥 끝낸다.
	RETURN_IF(_bytes_size == g_bytes_message_buffer_default_max_size);

	// check) 최소 1Kbyte보다 작으면 안됀다.
	CGASSERT_ERROR(_bytes_size<1024);

	// check) 최대 MAX_TCP_MESSAGE_SIZE(512MByte)를 넘으면 안됀다.
	CGASSERT_ERROR(_bytes_size <= MAX_TCP_MESSAGE_SIZE);

	// check) 최소 1Kbyte 최대 512Mbyte로 제한한다....
	SATURATE(_bytes_size, 1024, MAX_TCP_MESSAGE_SIZE);

	// 1) 값을 설정한다.
	g_bytes_message_buffer_default_max_size	 = _bytes_size;

	// log) 
	LOG_INFO << "(info) CGDK.net.socket: set 'DEFAULT TCP_RECEIVE BUFFER SIZE' to " << static_cast<int64_t>(g_bytes_message_buffer_default_max_size) << " bytes";
}

std::size_t net::io::receivable::default_message_buffer_min_size()
{
	return	g_bytes_message_buffer_default_min_size;
}

std::size_t net::io::receivable::default_message_buffer_max_size()
{
	return	g_bytes_message_buffer_default_max_size;
}

std::size_t net::io::receivable::Nstream::minimum_message_buffer_size(std::size_t _size) noexcept
{
	// 1) 기존의 Min receive shared_buffer 크기를 저장해 놓는다.
	std::size_t	old = this->m_minimum_mesage_buffer_size;

	// 2) 최하크기 이하일 경우 최하크기로 맞춘다.
	SATURATE_MIN(_size, MINIMUM_REMAINED_RECEIVE_BUFFER)

	// check) 이전의 값과 동일하면 그냥 끝낸다.
	RETURN_IF(old==_size, old);

	// 3) 값을 변경한다.
	this->m_minimum_mesage_buffer_size = _size;

	// return) 
	return	old;
}

std::size_t net::io::receivable::Nstream::minimum_message_buffer_size() noexcept
{
	return this->m_minimum_mesage_buffer_size;
}

std::size_t net::io::receivable::Nstream::maximum_message_buffer_size(std::size_t _size) noexcept
{
	// 1) 기존의 Min receive shared_buffer 크기를 저장해 놓는다.
	std::size_t	old = this->m_maximum_message_buffer_size;

	// check) 이전의 값과 동일하면 그냥 끝낸다.
	RETURN_IF(old == _size, old);

	// 3) 값을 변경한다.
	this->m_maximum_message_buffer_size = _size;

	// return) 
	return old;
}

std::size_t net::io::receivable::Nstream::maximum_message_buffer_size() noexcept
{
	return this->m_maximum_message_buffer_size;
}
