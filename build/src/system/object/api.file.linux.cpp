#include "pch.h"

#if defined(__linux__)

namespace CGDK
{

FILE_HANDLE api::file::open(const std::filesystem::path& _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location) noexcept
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check) 
	RETURN_IF(_filename.empty(), INVALID_FILE_HANDLE);

	// declare) 
	const char* str_access;

	switch (_operation)
	{
	case	eFILE_OPERATION::READ:
			str_access = "r";
			break;

	case	eFILE_OPERATION::WRITE:
			str_access = "w";
			break;

	case	eFILE_OPERATION::APPEND:
			str_access = "a+";
			break;

	case	eFILE_OPERATION::UPDATE:
			str_access = "a+";
			break;

	case	eFILE_OPERATION::READ_ASYNC:
			str_access = "r+";
			break;

	case	eFILE_OPERATION::WRITE_ASYNC:
			str_access = "w";
			break;

	case	eFILE_OPERATION::APPEND_ASYNC:
			str_access = "a+";
			break;

	default:
			return INVALID_FILE_HANDLE;
	}

	// 2) create file
	if (_operation == eFILE_OPERATION::WRITE
	 || _operation == eFILE_OPERATION::APPEND
	 || _operation == eFILE_OPERATION::WRITE_ASYNC
	 || _operation == eFILE_OPERATION::APPEND_ASYNC)
	{
		std::filesystem::create_directories(_filename.parent_path());
	}

	// declare) 
	FILE_HANDLE	hFile = INVALID_FILE_HANDLE;

	// 3) 현재 Directory에 먼저 찾는다.
	if ((_file_location & eFILE_LOCATION::NOW) != eFILE_LOCATION::NONE)
	{
		hFile = fopen(_filename.c_str(), str_access);
	}

	// 2) 파일 열기에 실패한 경우 eFILE_LOCATION::INTERNAL 플래그가 설정되어 있다면 INTERNAL 디렉토리에서 열기를 시도한다.
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::INTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 3) 파일 열기에 실패한 경우 eFILE_LOCATION::EXTERNAL 플래그가 설정되어 있다면 EXTERNAL 디렉토리에서 열기를 시도한다.
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 4) Resource Directory에도 뒤져보는가?
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::RESOURCE) != eFILE_LOCATION::NONE)
	{
		for (auto& iter : *resource::manager::instance())
		{
			// - path + filename
			auto temp_string = NFMT::format("{}/{}", iter, _filename.c_str());

			// - 파일을 open한다.
			hFile = fopen(temp_string.c_str(), str_access);

			// check) 파일을 찾아서 열었다면 여기서 중단!
			BREAK_IF(hFile != INVALID_FILE_HANDLE);
		}
	}

	// Return) 결과 리턴~
	return hFile;
}

void api::file::close(FILE_HANDLE _handle_file)
{
	// check) FileHandle이 INVALID_FILE_HANDLE이면 안됀다.
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE);

	// 1) close한다.
	fclose(_handle_file);
}

size_t api::file::read(FILE_HANDLE _handle_file, size_t _size, void* _data) noexcept
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file != INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// 1) read한다.
	return	fread(_data, 1, _size, _handle_file);
}

size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data) noexcept
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file != INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// 1) write한다.
	return	fwrite(_data, _size, 1, _handle_file);
}

size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin) noexcept
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// declare) 
	int origin;

	switch (_origin)
	{
	case	eFILE_ORIGIN::BEGIN:	origin = SEEK_SET; break;
	case	eFILE_ORIGIN::CURRENT:	origin = SEEK_CUR; break;
	case	eFILE_ORIGIN::END:		origin = SEEK_END; break;
	default:						origin = SEEK_SET; break;
	}

	// 1) file pointer를 설정한다.
	auto result = fseek(_handle_file, static_cast<long>(_offset), origin);

	// Return) 
	return result;
}

size_t api::file::get_length(FILE_HANDLE _handle_file) noexcept
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// 1) 현재 위치 저장해 놓음.
	long pos_now = ftell(_handle_file);

	// 2) File Pointer를 제일 뒤로 이동
	fseek(_handle_file, 0, SEEK_END);

	// 3) 위치를 얻음
	auto length = ftell(_handle_file);

	// check) 0보다 작은 값이 돌아오면 0으로 설정한다.
	SATURATE_MIN(length, 0);

	// 4) 원래 위치로 
	fseek(_handle_file, pos_now, SEEK_SET);

	// Return)
	return static_cast<size_t>(length);
}

size_t api::file::get_length(const std::filesystem::path& _filename) noexcept
{
	// declare) 
	std::error_code ec;

	// 1) get file size
	auto file_size = std::filesystem::file_size(_filename, ec);

	// check) 
	if(ec)
		return static_cast<size_t>(-1); // error, could use errno to find out more
	
	// return)
	return file_size;
}

bool api::file::truncate(FILE_HANDLE _handle_file, size_t _length) noexcept
{
	// 1) truncate
	auto result = ftruncate(fileno(_handle_file), _length);

	// return) ...
	return result == 0;
}

bool api::file::truncate(FILE_HANDLE _handle_file) noexcept
{
	// 1) truncate
	auto result = ftruncate(fileno(_handle_file), 0);

	// return) ...
	return result == 0;
}

std::filesystem::file_time_type api::file::get_last_write_time(const std::filesystem::path& _filename)
{
	return std::filesystem::last_write_time(_filename);
}

void api::file::set_last_write_time(const std::filesystem::path& _filename, std::filesystem::file_time_type _write_time)
{
	// 1) set file modifi time
	std::filesystem::last_write_time(_filename, _write_time);
}


}
#endif