#include "pch.h"

namespace CGDK
{

FILE_HANDLE api::file::open(std::string_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	// check)
	CGASSERT_ERROR(_filename != nullptr);

	// check) _filename이 nullptr이면 안됀다.
	RETURN_IF(_filename == nullptr, INVALID_FILE_HANDLE);

	// Declare) 
	std::string_view strAccess;

	switch (_operation)
	{
	case	eFILE_OPERATION::READ:
			strAccess = "r";
			break;

	case	eFILE_OPERATION::WRITE:
			strAccess = "w";
			break;

	case	eFILE_OPERATION::APPEND:
			strAccess = "r+";
			break;

	case	eFILE_OPERATION::READ_ASYNC:
			strAccess = "r";
			break;

	case	eFILE_OPERATION::WRITE_ASYNC:
			strAccess = "w";
			break;

	case	eFILE_OPERATION::APPEND_ASYNC:
			strAccess = "r+";
			break;

	default:
			CGASSERT_ERROR(false);
			throw CGException::CThrow_Error(eRESULT::NOT_DEFINED);
			break;
	}

	// Declare) 
	FILE_HANDLE	hFile = INVALID_FILE_HANDLE;

	// 2) 현재 Directory에 먼저 찾는다.
	if ((_file_location & eFILE_LOCATION::NOW) != eFILE_LOCATION::NONE)
	{
		hFile = fopen(_filename.data(), strAccess);
	}

	// 2) 파일 열기에 실패한 경우 eFILE_LOCATION::INTERNAL 플래그가 설정되어 있다면 INTERNAL 디렉토리에서 열기를 시도한다.
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::INTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 3) 파일 열기에 실패한 경우 eFILE_LOCATION::EXTERNAL 플래그가 설정되어 있다면 EXTERNAL 디렉토리에서 열기를 시도한다.
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
	{
	}

	// 3) Resource Directory에도 뒤져보는가?
	if (hFile == INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::RESOURCE) != eFILE_LOCATION::NONE)
	{
		for (auto& iter : *resource::manager::get_instance())
		{
			// Declare) 
			char	temp_string[MAX_STRING_LENGTH];

			// - String을 만든다.
			__sprintf(temp_string, "%s/%s", iter.c_str(), _filename);

			// - open file
			hFile = fopen(temp_string, strAccess);

			// check) 파일을 찾아서 열었다면 여기서 중단!
			BREAK_IF(hFile != INVALID_FILE_HANDLE);
		}
	}

	// Return) 결과 리턴~
	return hFile;
}

FILE_HANDLE api::file::open(std::wstring_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	// Declare) 
	char	temp[MAX_STRING_LENGTH]{};

	// 1) 문자열을 Converting한다.
	__convert_string(temp, _filename.data());

	// 2) open
	return	api::file::open(temp, _operation, _file_location);
}

FILE_HANDLE api::file::open(std::u16string_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	// Declare) 
	wchar_t	temp[MAX_STRING_LENGTH]{};

	// 1) 문자열을 Converting한다.
	__convert_string(temp, _filename.data());

	// 2) open
	return	api::file::open(temp, _operation, _file_location);
}

FILE_HANDLE api::file::open(std::u32string_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	return	api::file::open(reinterpret_cast<const char*>(_filename.data()), _operation, _file_location);
}

void api::file::close(FILE_HANDLE _handle_file)
{
	// check) FileHandle이 INVALID_FILE_HANDLE이면 안됀다.
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE);

	// 1) close한다.
	fclose(_handle_file);
}

size_t api::file::Read(FILE_HANDLE _handle_file, size_t _size, void* _data)
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file!=INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);

	// 1) Read한다.
	return	fread(_handle_file, static_cast<int>(_size), 1, _data);
}

size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data)
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file!=INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);

	// 1) Read한다.
	return	fwrite(_handle_file, static_cast<int>(_size), 1, , _data);
}

size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin)
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// Declare) 
	int origin;

	switch (_origin)
	{
	case	eFILE_ORIGIN::BEGIN:	origin = SEEK_SET; break;
	case	eFILE_ORIGIN::CURRENT:	origin = SEEK_CUR; break;
	case	eFILE_ORIGIN::END:		origin = SEEK_END; break;
	default:						origin = SEEK_SET; break;
	}

	// 1) File Pointer를 설정한다.
	auto result = fseek(_handle_file, static_cast<long>(_offset), origin);

	// Return) 
	return	result;
}

size_t api::file::get_length(FILE_HANDLE _handle_file)
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// check)
	CGASSERT_ERROR(false);

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

bool api::file::truncate(FILE_HANDLE _handle_file, size_t _length)
{
	// 1) truncate
	auto result = ftruncate(fileno(_handle_file), _length);

	// return) ...
	return result == 0;
}

bool api::file::truncate(FILE_HANDLE _handle_file)
{
	// 1) truncate
	auto result = ftruncate(fileno(_handle_file), 0);

	// return) ...
	return result == 0;
}


}