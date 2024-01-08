#include "pch.h"


namespace CGDK
{

buffer_view	api::file::read(FILE_HANDLE _handle_file, buffer_view _buffer) noexcept
{
	// 1) read
	auto result = read(_handle_file, _buffer.size(), _buffer.data()); 
	
	// return) 
	return buffer_view{ _buffer.data(), result };
}
buffer_view	api::file::write(FILE_HANDLE _handle_file, buffer_view _buffer) noexcept
{
	// 1) write
	auto result = write(_handle_file, _buffer.size(), _buffer.data());

	// return) 
	return buffer_view{ _buffer.data(), result };
}

shared_buffer api::file::read(const std::string_view _filename, eFILE_LOCATION _file_location) noexcept
{
	// 1) open file
	auto file = open(_filename.data(), eFILE_OPERATION::READ, _file_location);

	// check)
	RETURN_IF(file == INVALID_FILE_HANDLE, shared_buffer());

	// declare)
	shared_buffer buf_read;

	// 2) get file size
	auto file_size = get_length(file);

	// check)
	if (file_size != 0)
	{
		// 3) alloc buffer
		buf_read = alloc_shared_buffer(file_size);

		// 4) write file
		auto result = api::file::read(file, file_size, buf_read.data());

		// 5) set length
		buf_read ^= result;
	}

	// 6) close file
	close(file);

	// return)
	return buf_read;
}

buffer_view api::file::write(const std::string_view _filename, buffer_view _buf_write, eFILE_LOCATION _file_location) noexcept
{
	// 1) open file
	auto file = open(_filename.data(), eFILE_OPERATION::WRITE, _file_location);

	// check)
	RETURN_IF(file == INVALID_FILE_HANDLE, 0);

	// 2) write file
	auto result_size = api::file::write(file, _buf_write.size(), _buf_write.data());

	// 3) close file
	close(file);

	// return)
	return buffer_view(_buf_write.data(), result_size);
}

std::string api::file::make_full_path(const std::string_view _path, const std::string_view _file_name)
{
	// 1) 
	auto file_name = _file_name;

	// 2) path
	std::string temp_str(_path);

	// 3) trim string
	while(temp_str.empty() == false && temp_str.back() == 0)
	{
		temp_str.pop_back();
	}
	
	// 4)
	if (temp_str.back() == '\\')
		temp_str.back() = '/';

	// 5) ...
	if (temp_str.back() == '/')
	{
		if (file_name.front() == '/')
		{
			temp_str.pop_back();
		}
		else if (file_name.front() == '\\')
		{
			file_name.remove_prefix(1);
		}
	}
	else
	{
		if (file_name.front() != '/')
		{
			if (file_name.front() == '\\')
			{
				file_name.remove_prefix(1);
			}

			temp_str += '/';
		}
	}

	// 3) add
	temp_str += file_name;

	// return)
	return temp_str;
}



}