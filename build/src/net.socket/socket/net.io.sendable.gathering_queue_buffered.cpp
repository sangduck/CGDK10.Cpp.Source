#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

void net::io::sendable::QUEUE_SEND_bufferd::clear() noexcept
{
	this->array_buffer.clear();
	this->array_node.clear();

	this->bytes_data = 0;
	this->count_message = 0;
	this->bytes_remained = 0;
}

void net::io::sendable::QUEUE_SEND_bufferd::reset_and_swap(QUEUE_SEND_bufferd&& _move) noexcept
{
	// 1) swap buffer info
	this->array_buffer.swap(_move.array_buffer);
	this->array_node.swap(_move.array_node);

	this->bytes_data = _move.bytes_data; _move.bytes_data = 0;
	this->count_message = _move.count_message; _move.count_message = 0;
	this->bytes_remained = _move.bytes_remained; _move.bytes_remained = 0;
}

void net::io::sendable::QUEUE_SEND_bufferd::push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	// 설명) buffer의 queuing 처리
	//       - queuing된 COUNT_SEND_GATHER_START_DEPTH보다 많고 _buffer 크기가 BYTES_SEND_GATHER_BUFFER_MIN보다 작을 경우 => queuing 처리한다.
	//       - 그것이 아닐 경우 => _buffer를 통째로 복사한다.

	// 1) ...
	auto bytes_requested = _buffer.size();

	if (this->array_buffer.size() > COUNT_SEND_GATHER_START_DEPTH && bytes_requested <= BYTES_SEND_GATHER_BUFFER_MIN)
	{
		// - get pbuffer and pnode
		auto* pbuffer_now = &this->array_buffer.back();

		// - is buffer enough to copy (no => create new)
		if (this->bytes_remained < bytes_requested)
		{
			// - shift count
			auto shift_count = static_cast<uint32_t>(this->array_buffer.size() - COUNT_SEND_GATHER_START_DEPTH - 1) * 2;

			// - saturate_max
			SATURATE_MAX(shift_count, BYTES_SEND_GATHER_SHIFT_MAX);

			// - get buffer size
			auto size_buffer = (BYTES_SEND_GATHER_BUFFER_MIN << shift_count);

			// check)
			CGASSERT_ERROR(size_buffer <= BYTES_SEND_GATHER_BUFFER_MAX);

			// - alloc new buffer
			auto buf_new = alloc_shared_buffer(size_buffer);

			// - queue allocated buffer
			this->array_buffer.emplace_back(buf_new.size<ULONG>(), buf_new.data());
			this->array_node.emplace_back(std::move(buf_new.get_source()));
			this->bytes_remained = size_buffer;

			pbuffer_now = &this->array_buffer.back();
		}

		// check)
		CGASSERT_ERROR(this->bytes_remained >= bytes_requested)

		// - copy~
		memcpy(pbuffer_now->buf + pbuffer_now->len, _buffer.data(), bytes_requested);
		pbuffer_now->len += static_cast<ULONG>(bytes_requested);
		this->bytes_remained -= bytes_requested;
	}
	// - BYTES_SEND_GATHER_BUFFER_MIN보다 크면 버퍼 통째로 추가한다.
	else
	{
		// - 기존 queueing 데이터에 연속된 경우 길이만 늘여주고 끝낸다.
		if (this->array_buffer.empty() == false)
		{
			// - get pbuffer and pnode
			auto& buffer_now = this->array_buffer.back();
			auto& node_now = this->array_node.back();

			if (node_now.powner == _powner && (buffer_now.buf + buffer_now.len) == _buffer.data())
			{
				// - expand only
				buffer_now.len += static_cast<ULONG>(bytes_requested);
				this->bytes_data += bytes_requested;
				this->count_message += _count_messages;

				// check)
				CGASSERT_ERROR(this->bytes_remained == 0);

				// return) 
				return;
			}
		}

		// - buffer에 복사하지 않고 통째로 queuing한다.
		this->array_buffer.emplace_back(_buffer.size<ULONG>(), _buffer.data());
		this->array_node.emplace_back(std::move(_powner));
		this->bytes_remained = 0;
	}

	// - ...
	this->bytes_data += bytes_requested;
	this->count_message += _count_messages;
}

#endif
