#include "pch.h"

using namespace CGDK;


uint64_t hto64(uint64_t _value)
{
	return static_cast<uint64_t>((htonl(static_cast<uint32_t>(_value >> 32)))) | (static_cast<uint64_t>(htonl(static_cast<uint32_t>(_value))) << 32);
}

void sWEBSOCKET_MESSAGE::add_text(std::string_view _string) noexcept
{
	// 1) get buffer size
	auto size = _string.length() * 4;

	// 1) alloc buffer
	auto buf_conv = alloc_shared_buffer(size);

	// 2) UTF-8로 변환하고 그 길이를 구한다.
	auto str_result = encode_string(buf_conv.data<char8_t>(), _string, size);

	// 3) set
	method = eHTTP_METHOD::TEXT_;
	messages.push_back(buf_conv ^ (str_result.size()));
}

void sWEBSOCKET_MESSAGE::add_text(std::u8string_view _string) noexcept
{
	// 1)
	method = eHTTP_METHOD::TEXT_; 
	messages.push_back(to_basic_buffer<char>(_string));
}

void _append_length(buffer& _buffer, std::size_t _length)
{
	// 2) 길이를 설정한다.
	if (_length < 126)
	{
		_buffer.append<uint8_t>(static_cast<uint8_t>(_length));
	}
	else if (_length <= 0xffff)
	{
		_buffer.append<uint8_t>(126);
		_buffer.append<uint16_t>(htons(static_cast<uint16_t>(_length)));
	}
	else
	{
		auto value = hto64(_length);

		_buffer.append<uint8_t>(127);
		_buffer.append<uint64_t>(value);
	}
}


buffer_view sWEBSOCKET_MESSAGE::process_append(buffer& _buffer) const
{
	// declare)
	uint8_t opcode;

	switch(method)
	{
	// case) binary - FIN:0, RSV:0, Opcode:2
	case	eHTTP_METHOD::BINARY_:
			opcode = 130;
			break;
	
	// case) text - FIN:0, RSV:0, Opcode:1
	case	eHTTP_METHOD::TEXT_:
			opcode = 129;
			break;
	
	// default) ...
	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	}

	// 1) get message_length
	size_t message_length = 0;
	for (auto& iter : messages)
	{
		message_length = iter.size();
	}

	// 2) append data
	_buffer.append<uint8_t>(opcode);	
	_append_length(_buffer, message_length);
	for (auto& iter : messages)
	{
		_buffer.append(iter.size(), iter.data());
	}

	// return) 
	return buffer_view(_buffer.data(), _buffer.size());
}

void sWEBSOCKET_MESSAGE::process_extract(buffer_view& /*_buffer*/) const
{

}

void sWEBSOCKET_MESSAGE::process_front(buffer_view& _buffer, int& _offset) const
{
	// 1) ...
	buffer_view buf_temp = _buffer + offset(_offset);

	// 2) ...
	process_extract(buf_temp);
}

size_t _get_length(size_t _length)
{
	// 2) 길이를 설정한다.
	if (_length < 126)
	{
		return sizeof(uint8_t);
	}
	else if (_length > 0xffff)
	{
		return sizeof(uint8_t) + sizeof(uint64_t);
	}
	else
	{
		return sizeof(uint8_t) + sizeof(uint16_t);
	}
}

std::size_t sWEBSOCKET_MESSAGE::process_get_size_of() const noexcept
{
	// declare) 
	std::size_t	size = 0;

	// 1) get message length
	size_t message_length = 0;

	for (auto& iter : messages)
	{
		message_length = iter.size();
	}

	switch(method)
	{
	case	eHTTP_METHOD::BINARY_:
			size = message_length + sizeof(uint8_t) + _get_length(message_length);
			break;
	
	case	eHTTP_METHOD::TEXT_:
			size = message_length + sizeof(uint8_t) + _get_length(message_length);
			break;
	
	default:
			break;
	}
	
	// return) 
	return size;
}
