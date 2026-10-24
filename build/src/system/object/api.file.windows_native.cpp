#include "pch.h"

#if defined(_WIN32)

namespace CGDK
{

FILE_HANDLE api::file::open(const std::filesystem::path& _filename, eFILE_OPERATION _file_operation, eFILE_LOCATION _file_location) noexcept
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check) _filename이 nullptr이면 안 된다.
	RETURN_IF(_filename.empty(), INVALID_FILE_HANDLE);

	// declare) 
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	DWORD dwCreationDisposition;
	DWORD dwFlagsAndAttributes;

	switch(_file_operation)
	{
	case	eFILE_OPERATION::READ:
			{
				dwDesiredAccess			 = GENERIC_READ;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = OPEN_EXISTING;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL;
			}
			break;

	case	eFILE_OPERATION::WRITE:
			{
				dwDesiredAccess			 = GENERIC_WRITE;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = CREATE_ALWAYS;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL;
			}
			break;

	case	eFILE_OPERATION::APPEND:
			{
				dwDesiredAccess			 = GENERIC_READ | FILE_APPEND_DATA;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = OPEN_ALWAYS;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL;
			}
			break;

	case	eFILE_OPERATION::UPDATE:
			{
				dwDesiredAccess			 = GENERIC_READ | GENERIC_WRITE;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = OPEN_ALWAYS;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL;
			}
			break;

	case	eFILE_OPERATION::READ_ASYNC:
			{
				dwDesiredAccess			 = GENERIC_READ;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = OPEN_EXISTING;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
			}
			break;

	case	eFILE_OPERATION::WRITE_ASYNC:
			{
				dwDesiredAccess			 = GENERIC_WRITE;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = CREATE_ALWAYS;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
			}
			break;

	case	eFILE_OPERATION::APPEND_ASYNC:
			{
				dwDesiredAccess			 = GENERIC_READ | FILE_APPEND_DATA;
				dwShareMode				 = FILE_SHARE_READ | FILE_SHARE_WRITE;
				dwCreationDisposition	 = OPEN_ALWAYS;
				dwFlagsAndAttributes	 = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
			}
			break;

	default:
			return INVALID_FILE_HANDLE;
	}

	// 2) create file
	if (dwDesiredAccess & GENERIC_WRITE
	 || dwDesiredAccess & FILE_APPEND_DATA)
	{
		// - get parent_path
		auto parent_path = _filename.parent_path();

		if (parent_path.empty() == false
			&& parent_path.string() != "."
			&& parent_path.string() != "/"
			)
		{
			std::error_code error_code;
			std::filesystem::create_directories(parent_path, error_code);
		}
	}

	// declare) 
	FILE_HANDLE	handle_file = INVALID_FILE_HANDLE;

	// 3) 현재 Directory에 먼저 찾는다.
	if((_file_location & eFILE_LOCATION::NOW) != eFILE_LOCATION::NONE)
	{
#ifdef _UNICODE
		// - create file
		handle_file = CreateFile(_filename.c_str(), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, 0);
#else
		// - create file
		handle_file = CreateFileW(_filename.c_str(), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, 0);
#endif
	}

	// 4) 파일 열기에 실패한 경우 eFILE_LOCATION::INTERNAL 플래그가 설정되어 있다면 INTERNAL 디렉터리에서 열기를 시도한다.
	if (handle_file == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::INTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 5) 파일 열기에 실패한 경우 eFILE_LOCATION::EXTERNAL 플래그가 설정되어 있다면 EXTERNAL 디렉터리에서 열기를 시도한다.
	if (handle_file == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 6) Resource Directory에도 뒤져보는가?
	if(handle_file ==INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::RESOURCE)!=eFILE_LOCATION::NONE)
	{
		for(auto& iter: *resource::manager::instance())
		{
			// - Resource path를 기준으로 파일 경로를 만든다.
			auto temp_path = std::filesystem::path(iter) / _filename;

			// - 읽어들인다.
			handle_file = CreateFileW(temp_path.c_str(), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, 0);

			// check) 파일을 찾아서 열었다면 여기서 중단!
			BREAK_IF(handle_file !=INVALID_FILE_HANDLE);
		}
	}

#if defined(_WIN32)
	if (handle_file != INVALID_FILE_HANDLE)
	{
		try
		{
			// 4) executor에 붙인다.
			if (_file_operation == eFILE_OPERATION::READ_ASYNC || _file_operation == eFILE_OPERATION::WRITE_ASYNC || _file_operation == eFILE_OPERATION::APPEND_ASYNC)
			{
				// - executor를 얻는다.
				auto pexecutor = system_executor::executor();

				// check) pexecutor가 nullptr이면 안 된다.
				THROW_IF(pexecutor.empty(), throwable::failure(eRESULT::FAIL, 0, "executor is nullptr"sv));

				// - executor에 붙인다.(생성하면 기본 Executor로 설정한다.)
				int result = pexecutor->attach(handle_file);

				// check) result가 FALSE이면 Bind 실패다!!!
				THROW_IF(result == FALSE, throwable::failure(eRESULT::FAIL, 0, "fail to attach to executor"sv));
			}
		}
		catch (...)
		{
			// - closesocket
			CloseHandle(handle_file);

			// - reset file handle
			handle_file = INVALID_FILE_HANDLE;
		}
	}
#endif

	// return) 결과 리턴~
	return handle_file;
}

void api::file::close(FILE_HANDLE _handle_file) noexcept
{
	// check) FileHandle이 INVALID_FILE_HANDLE이면 안 된다.
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE);

	// 1) close한다.
	CloseHandle(_handle_file);
}

size_t api::file::read(FILE_HANDLE _handle_file, size_t _size, void* _data) noexcept
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file != INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// declare) 
	size_t dwData = 0;

	// 1) Read한다.
	auto result = ::ReadFile(_handle_file, _data, static_cast<DWORD>(_size), (LPDWORD)&dwData, NULL);

	// check) 읽기에 실패했으면 0을 리턴한다.
	RETURN_IF(result == FALSE, 0);

	// return) 읽은 Byte수를 return한다.
	return dwData;
}

size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data) noexcept
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file != INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// declare) 
	size_t dwData = 0;

	// 1) Read한다.
	auto result = ::WriteFile(_handle_file, _data, static_cast<DWORD>(_size), (LPDWORD)&dwData, NULL);

	// check) 읽기에 실패했으면 0을 리턴한다.
	RETURN_IF(result == FALSE, 0);

	// return) 읽은 Byte수를 return한다.
	return dwData;
}

size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin) noexcept
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// 1) File Pointer를 설정한다.
	auto result = ::SetFilePointer(_handle_file, static_cast<long>(_offset), nullptr, (DWORD)_origin);

	// return) 
	return result;
}

//size_t api::file::get_length(FILE_HANDLE _handle_file) noexcept
//{
//	// check) 
//	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);
//
//	// return)
//	return ::GetFileSize(_handle_file, nullptr);
//}

size_t api::file::get_length(FILE_HANDLE _handle_file) noexcept
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// declare) 
	LARGE_INTEGER fie_size{0};

	// 1) get file size
	auto result = ::GetFileSizeEx(_handle_file, &fie_size);

	// check)
	if(result == 0)
		return static_cast<size_t>(-1); // error, could use errno to find out more

	// return)
	return fie_size.QuadPart;
}

size_t api::file::get_length(const std::filesystem::path& _filename) noexcept
{
	// declare) 
	std::error_code ec;

	// 1) get file size
	auto file_size = std::filesystem::file_size(_filename, ec);

	// check) 
	if (ec)
		return static_cast<size_t>(-1); // error, could use errno to find out more

	// return)
	return file_size;
}

bool api::file::truncate(FILE_HANDLE _handle_file, size_t _length) noexcept
{
	// 1) 현재의 위치를 얻는다.
	auto pos_now = ::SetFilePointer(_handle_file, 0, nullptr, FILE_CURRENT);

	// 2) File Pointer를 설정한다.
	auto result = ::SetFilePointer(_handle_file, static_cast<DWORD>(_length), nullptr, FILE_BEGIN);

	// check)
	RETURN_IF(result == INVALID_SET_FILE_POINTER, false);

	// 3) 파일 크기를 설정한다.
	auto result_2 = ::SetEndOfFile(_handle_file);

	// check)
	RETURN_IF(result_2 == FALSE, false);

	// 4) File Pointer를 설정한다.
	::SetFilePointer(_handle_file, pos_now, nullptr, FILE_BEGIN);

	// return)
	return true;
}

bool api::file::truncate(FILE_HANDLE _handle_file) noexcept
{
	return ::SetEndOfFile(_handle_file);
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