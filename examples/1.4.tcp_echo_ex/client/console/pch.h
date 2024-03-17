#ifndef PCH_H
#define PCH_H

// 1) CGDK header files
#include "cgdk/sdk10/net.socket.h"

// 2) C++ standard header files
#include <iostream>
#if defined(_WIN32)
	#include <conio.h>
#endif

// 3) using
using namespace CGDK;

// 4) message heaser
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
