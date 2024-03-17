#ifndef PCH_H
#define PCH_H

#include "framework.h"

// message heaser
struct PACKET_HEADER_BASE
{
	short seperator_ = 0;
	short size_ = 0;

	operator size_t() const noexcept { return (size_t)size_; }
	static void			_set_message_size(buffer& _buffer) noexcept { _buffer.front<short>(2) = _buffer.size<short>(); }
	static std::size_t	_get_message_size(const buffer_view& _buffer) noexcept { return static_cast<std::size_t>(*_buffer.data<short>(2)); }
	static bool			_validate_message(const std::vector<shared_buffer>& /*_vector_buffer*/) noexcept { return true; }
};

#endif //PCH_H
