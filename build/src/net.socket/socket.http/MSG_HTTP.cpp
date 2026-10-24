#include "pch.h"

using namespace CGDK;

const std::string_view STRING_TAG_CONTEX_TYPE{ "Content-Type: " };
const std::string_view STRING_TAG_CONTEX_LENGTH{ "Content-Length: " };
const std::string_view STRING_DEFAULT_CONTENT_TYPE{ "text/html; charset=utf-8" };


buffer_view CGDK::append_MESSAGE_HTTP(buffer& _buffer, const sMESSAGE_HTTP_RESPONSE& _data)
{
	// 1) response headers
	// - http version
	_buffer.append_text("HTTP/");
	_buffer.append_text(HTTP_VERSION_MAJOR(_data.version));
	_buffer.append_text(".");
	_buffer.append_text(HTTP_VERSION_MINOR(_data.version));
	_buffer.append_text(" ");
	// - status code
	_buffer.append_text(static_cast<uint32_t>(_data.status_code));
	_buffer.append_text(" ");
	// - status message
	_buffer.append_text((_data.status_message.empty() == false) ? _data.status_message : to_string(_data.status_code));
	_buffer.append_text("\r\n");

	// 2) append headers
	for (auto& iter : _data.headers)
	{
		_buffer.append_text(iter.key);
		_buffer.append_text(": ");
		_buffer.append_text(iter.value);
		_buffer.append_text("\r\n");
	}

	size_t size_contents = 0;

	// 3) get contents length
	for (auto& iter : _data.contents)
		size_contents += iter.size();

	// 3) append 'Content-Type'
	_buffer.append_text(STRING_TAG_CONTEX_TYPE);
	_buffer.append_text((_data.content_type.empty() == false) ? _data.content_type : STRING_DEFAULT_CONTENT_TYPE);
	_buffer.append_text("\r\n");

	// 4) append 'Content-Length'
	_buffer.append_text("Content-Length: ");
	{
		_buffer.append_text(size_contents);
	}
	_buffer.append_text("\r\n");

	// 5) end of header or end of contents
	_buffer.append_text("\r\n");

	// 6) append 'contents'
	if (size_contents != 0)
	{
		for (auto& iter : _data.contents)
			_buffer.append(iter.size(), iter.data());
	}

	// check)
	CGASSERT_ERROR(get_size_of(_data) == _buffer.size());

	// return) 
	return buffer_view(_buffer.data() + _buffer.size(), _buffer.size());
}

std::size_t GET_NUMBER_SIZE(uint64_t _number)
{
	// declare) 
	std::size_t value;

	if (_number < 10)
		value = 1;
	else if (_number < 100)
		value = 2;
	else if (_number < 1000)
		value = 3;
	else if (_number < 10000)
		value = 4;
	else if (_number < 100000)
		value = 5;
	else if (_number < 1000000)
		value = 6;
	else if (_number < 10000000)
		value = 7;
	else if (_number < 100000000)
		value = 8;
	else if (_number < 1000000000)
		value = 9;
	else if (_number < 10000000000)
		value = 10;
	else if (_number < 100000000000)
		value = 11;
	else
		throw throwable::failure(eRESULT::OVERFLOW_ERROR);

	// return) 
	return value;
}

std::size_t CGDK::get_size_of_MESSAGE_HTTP(const sMESSAGE_HTTP_RESPONSE& _data)
{
	// 1) 
	std::size_t	size = 0;

	// 2) header.1
	// - http version
	size = "HTTP/"sv.size()
		 + GET_NUMBER_SIZE(HTTP_VERSION_MAJOR(_data.version))
		 + 1
		 + GET_NUMBER_SIZE(HTTP_VERSION_MINOR(_data.version))
		 + 1
	// - status code
		 + GET_NUMBER_SIZE(static_cast<uint32_t>(_data.status_code))
		 + 1;
	// - status message
	size += (_data.status_message.empty() == false) ? _data.status_message.size() : to_string(_data.status_code).size();
	size += 2;

	// 3) headers
	for (auto& iter : _data.headers)
		size += iter.key.size() + 2 + iter.value.size() + 2;

	// 4) get context_isze
	size_t size_contents = 0;
	for (auto iter : _data.contents)
		size_contents += iter.size();

	// 3) add size of 'Content-Type'
	size += STRING_TAG_CONTEX_TYPE.size();
	size += (_data.content_type.empty() == false) ? _data.content_type.size() : STRING_DEFAULT_CONTENT_TYPE.size();
	size += 2;

	// 4) append 'Content-Length'
	size += "Content-Length: "sv.size()
		+ GET_NUMBER_SIZE(size_contents)
		+ 2;

	// 5) end of header or end of contents
	size += 2;

	// 6) append 'contents'
	size += size_contents;

	// return) 
	return size;
}
