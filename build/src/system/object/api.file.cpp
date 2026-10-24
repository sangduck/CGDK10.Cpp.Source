#include "pch.h"

namespace CGDK
{

std::string_view to_string(eFILE_OPERATION _file_operation)
{
	// declare) 
	std::string_view result;

	switch (_file_operation)
	{
	case eFILE_OPERATION::NONE:				result = "none"sv; break;
	case eFILE_OPERATION::READ:				result = "read"sv; break;
	case eFILE_OPERATION::WRITE:			result = "write"sv; break;
	case eFILE_OPERATION::APPEND:			result = "append"sv; break;
	case eFILE_OPERATION::UPDATE:			result = "update"sv; break;
	case eFILE_OPERATION::READ_ASYNC:		result = "read_async"sv; break;
	case eFILE_OPERATION::WRITE_ASYNC:		result = "write_async"sv; break;
	case eFILE_OPERATION::APPEND_ASYNC:		result = "append_async"sv; break;
	default:								result = "-"sv;	break;
	};

	// return)
	return result;
}

std::string_view to_string(eFILE_ORIGIN _file_origin)
{
	// declare) 
	std::string_view result;

	// 1) 
	switch (_file_origin)
	{
	case eFILE_ORIGIN::BEGIN:				result = "begin"sv; break;
	case eFILE_ORIGIN::CURRENT:				result = "current"sv; break;
	case eFILE_ORIGIN::END:					result = "end"sv; break;
	default:								result = "-"sv; break;
	};

	// return)
	return result;
}

std::string_view to_string(eFILE_LOCATION _file_location)
{
	// declare) 
	std::string_view result;

	if (static_cast<uint32_t>(_file_location & eFILE_LOCATION::NOW))
	{

	}

	if (static_cast<uint32_t>(_file_location & eFILE_LOCATION::RESOURCE))
	{

	}

	if (static_cast<uint32_t>(_file_location & eFILE_LOCATION::SYSTEM))
	{

	}

	if (static_cast<uint32_t>(_file_location & eFILE_LOCATION::INTERNAL))
	{

	}

	if (static_cast<uint32_t>(_file_location & eFILE_LOCATION::EXTERNAL))
	{

	}

	// return)
	return result;
}


shared_buffer api::file::read_file(const std::string_view _filename, eFILE_LOCATION _file_location)
{
	// 1) File open
	auto fp = api::file::open(_filename, eFILE_OPERATION::READ, _file_location);

	// check)
	THROW_IF(fp == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, CGDK::stream_string("fail to open json file for read (name:'", _filename, "', location:", to_string(_file_location), ")")));

	// 2) get length 
	auto file_size = get_length(fp);

	// 3) alloc buffer
	auto read_buffer = alloc_shared_buffer(file_size);

	// 4) set size
	read_buffer.resize(file_size);

	// 4) read file
	auto read_bytes = api::file::read(fp, file_size, read_buffer.data());

	// check)
	THROW_IF(read_bytes != file_size, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, CGDK::stream_string("fail to read json file (required:", file_size, ", readed:", read_bytes, ")")));

	// 5) close file
	api::file::close(fp);

	// return) 
	return read_buffer;
}

void api::file::write_file(buffer_view _data, const std::string_view _filename, eFILE_LOCATION _file_location)
{
	// 1) File open
	auto fp = api::file::open(_filename, eFILE_OPERATION::WRITE, _file_location);

	// check)
	THROW_IF(fp == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, CGDK::stream_string("fail to open json file for write (name:'", _filename, "', location:", to_string(_file_location), ")")));

	// 4) read file
	auto write_bytes = api::file::write(fp, _data.size(), _data.data());

	// check)
	THROW_IF(write_bytes != _data.size(), throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, CGDK::stream_string("fail to write json file (required:", _data.size(), ", writed:", write_bytes, ")")));

	// 5) close file
	api::file::close(fp);
}

}
