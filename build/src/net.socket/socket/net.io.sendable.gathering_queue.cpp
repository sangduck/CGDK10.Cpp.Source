#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

void net::io::sendable::QUEUE_SEND::clear() noexcept
{
	this->array_buffer.clear();
	this->array_node.clear();
	this->bytes_data = 0;
	this->count_message = 0;
}

void net::io::sendable::QUEUE_SEND::reset_and_swap(QUEUE_SEND&& _move) noexcept
{
	this->array_buffer.swap(_move.array_buffer);
	this->array_node.swap(_move.array_node);
	this->bytes_data = _move.bytes_data; _move.bytes_data = 0;
	this->count_message = _move.count_message; _move.count_message = 0;

	// check)
	CGASSERT_ERROR(_move.array_buffer.empty());
	CGASSERT_ERROR(_move.array_node.empty());
}

void net::io::sendable::QUEUE_SEND::push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	if (this->array_buffer.empty() == false)
	{
		// - get pbuffer and pnode
		auto& buffer_now = this->array_buffer.back();

		// - expand only when it follows the last buffer in memory and has the same owner (the owner keeps the memory)
		if ((buffer_now.buf + buffer_now.len) == _buffer.data() && this->array_node.back().powner == _powner)
		{
			// - expand only
			auto size_added = _buffer.size();
			buffer_now.len += static_cast<ULONG>(size_added);
			this->bytes_data += size_added;
			this->count_message += _count_messages;

			// return) 
			return;
		}
	}

	// - set buf data
	this->array_buffer.emplace_back(_buffer.size<ULONG>(), _buffer.data());
	this->array_node.emplace_back(std::move(_powner));
	this->bytes_data += _buffer.size();
	this->count_message += _count_messages;
}

#endif
