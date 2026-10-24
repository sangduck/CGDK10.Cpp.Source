#include "pch.h"

namespace CGDK
{

FILE_HANDLE api::file::open(std::string_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	// Declare) 
	wchar_t	temp[MAX_STRING_LENGTH];

	// 1) 문자열을 Converting한다.
	__convert_string(temp, _filename);

	// 2) open
	return	open(temp, _operation, _file_location);
}

FILE_HANDLE api::file::open(std::wstring_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
{
	// check)
	CGASSERT_ERROR(_filename.empty()==false);

	// check) _filename이 nullptr이면 안 된다.
	RETURN_IF(_filename.empty(), INVALID_FILE_HANDLE);

	// Declare) 
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	DWORD dwCreationDisposition;

	switch(_operation)
	{
	case	eFILE_OPERATION::READ:
	case	eFILE_OPERATION::READ_ASYNC:
		{
			dwDesiredAccess			 = GENERIC_READ;
			dwShareMode				 = FILE_SHARE_READ;
			dwCreationDisposition	 = OPEN_EXISTING;
		}
		break;

	case	eFILE_OPERATION::WRITE:
	case	eFILE_OPERATION::WRITE_ASYNC:
		{
			dwDesiredAccess			 = GENERIC_WRITE;
			dwShareMode				 = FILE_SHARE_WRITE;
			dwCreationDisposition	 = CREATE_NEW;
		}
		break;

	case	eFILE_OPERATION::APPEND:
	case	eFILE_OPERATION::APPEND_ASYNC:
		{
			dwDesiredAccess			 = GENERIC_WRITE | GENERIC_READ;
			dwShareMode				 = FILE_SHARE_WRITE;
			dwCreationDisposition	 = CREATE_ALWAYS;
		}
		break;

	default:
		CGASSERT_ERROR(false);
		throw	CGException::CThrow_Error(eRESULT::NOT_DEFINED);
		break;
	}

	// Declare) 
	FILE_HANDLE	hFile	 = INVALID_FILE_HANDLE;

	// 2) 현재 Directory에 먼저 찾는다.
	if((_file_location & eFILE_LOCATION::NOW)!=eFILE_LOCATION::NONE)
	{
		hFile	 = CreateFile2(_filename.data(), dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);
	}

	// 3) Resource Directory에도 뒤져보는가?
	if(hFile==INVALID_FILE_HANDLE && (_file_location & eFILE_LOCATION::RESOURCE)!=eFILE_LOCATION::NONE)
	{
		for(auto& iter: *resource::manager::get_instance())
		{
			// Declare) 
			wchar_t	temp_string[MAX_STRING_LENGTH];

			// - String을 만든다.
			std::snprintf(temp_string, std::size(temp_string), std::string_view(L"%hs/%s").data(), iter.c_str(), _filename);

			// - 읽어들인다.
			hFile	 = CreateFile2(temp_string, dwDesiredAccess, dwShareMode, dwCreationDisposition, nullptr);

			// check) 파일을 찾아서 열었다면 여기서 중단!
			BREAK_IF(hFile!=INVALID_FILE_HANDLE);
		}
	}

	// Return) 결과 리턴~
	return	hFile;
}

void api::file::close(FILE_HANDLE _handle_file)
{
	// check) FileHandle이 INVALID_FILE_HANDLE이면 안 된다.
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE);

	// 1) close한다.
	CloseHandle(_handle_file);
}

size_t api::file::read(FILE_HANDLE _handle_file, size_t _size, void* _data)
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file!=INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);

	// Declare) 
	size_t dwData = 0;

	// 1) Read한다.
	auto result = ::ReadFile(_handle_file, _data, static_cast<DWORD>(_size), (LPDWORD)&dwData, NULL);

	// check) 읽기에 실패했으면 0을 리턴한다.
	RETURN_IF(result==FALSE, 0);

	// Return) 읽은 Byte수를 return한다.
	return	dwData;
}

size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data)
{
	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(_handle_file!=INVALID_FILE_HANDLE);

	// check)
	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);

	// Declare) 
	size_t dwData = 0;

	// 1) Read한다.
	auto result = ::WriteFile(_handle_file, _data, static_cast<DWORD>(_size), (LPDWORD)&dwData, NULL);

	// check) 읽기에 실패했으면 0을 리턴한다.
	RETURN_IF(result==FALSE, 0);

	// Return) 읽은 Byte수를 return한다.
	return dwData;
}

size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin)
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	// Declare)
	LARGE_INTEGER posNew;
	LARGE_INTEGER posResult;

	posNew.QuadPart = static_cast<LONGLONG>(_offset);

	// 1) File Pointer를 설정한다.
	auto result = SetFilePointerEx(_handle_file, posNew, &posResult, (DWORD)_origin);

	// check) 
	RETURN_IF(result == FALSE, 0);

	// Return) 
	return static_cast<size_t>(posResult.QuadPart);
}

size_t api::file::get_length(FILE_HANDLE _handle_file)
{
	// check) 
	RETURN_IF(_handle_file == INVALID_FILE_HANDLE, 0);

	LARGE_INTEGER posResult;

	// 1) 파일의 크기를 구한다.
	auto result = GetFileSizeEx(_handle_file, &posResult);

	// check) 
	RETURN_IF(result==FALSE, 0);

	// Return) 
	return static_cast<size_t>(posResult.QuadPart);
}



















//
//
//FILE_HANDLE api::file::open(std::string_view _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
//{
//	// check)
//	CGASSERT_ERROR(_filename!=nullptr);
//
//	// check) _filename이 nullptr이면 안 된다.
//	RETURN_IF(_filename==nullptr, INVALID_FILE_HANDLE);
//
//	// Declare) 
//	FILE_HANDLE			hFile	 = INVALID_FILE_HANDLE;
//	std::string_view	strMode	 = nullptr;
//
//	// 1) ...
//	switch(_operation)
//	{
//	case	eFILE_OPERATION::READ:
//	case	eFILE_OPERATION::READ_ASYNC:
//			{
//				strMode	 = "rb";
//			}
//			break;
//
//	case	eFILE_OPERATION::WRITE:
//	case	eFILE_OPERATION::WRITE_ASYNC:
//			{
//				strMode	 = "wb";
//			}
//			break;
//
//	case	eFILE_OPERATION::APPEND:
//	case	eFILE_OPERATION::APPEND_ASYNC:
//			{
//				strMode	 = "a+";
//			}
//			break;
//
//	default:
//			CGASSERT_ERROR(false);
//			break;
//	}
//
//	// 2) File을 open한다.
//	auto	result	 = fopen_s(&hFile, _filename, strMode);
//
//	// check) File의 open에 실패한 경우 Exception을 던진다.
//	if(result!=0)
//	{
//		if(hFile==INVALID_FILE_HANDLE)
//		{
//			for(auto& iter: *resource::manager::get_instance())
//			{
//				// Declare) 
//				char	temp_string[MAX_STRING_LENGTH];
//
//				std::snprintf(temp_string, std::size(temp_string), std::string_view("%ls/%s").data(), iter.c_str(), _filename);
//
//				// - 읽어들인다.
//				result	 = fopen_s(&hFile, temp_string, strMode);
//
//				// check) 성공했으면 여기서 중단.
//				BREAK_IF(hFile!=INVALID_FILE_HANDLE);
//			}
//		}
//	}
//
//	// Return) 
//	return	hFile;
//}
//


//FILE_HANDLE api::file::open(const wchar_t* _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location)
//{
//	// Declare) 
//	char	temp[MAX_STRING_LENGTH];
//
//	// 1) 문자열을 Converting한다.
//	__convert_string(temp, _filename);
//
//	// 2) open
//	return	open(temp, _operation, _file_location);
//}
//
//void api::file::close(FILE_HANDLE _handle_file)
//{
//	// check) 
//	RETURN_IF(_handle_file==INVALID_FILE_HANDLE);
//
//	// 1) close한다.
//	fclose(_handle_file);
//}
//
//size_t api::file::Read(FILE_HANDLE _handle_file, size_t _size, void* _data)
//{
//	// check)
//	CGASSERT_ERROR(_data!=nullptr);
//
//	// check)
//	RETURN_IF(_data==nullptr || _size==0, 0);
//
//	// check)
//	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);
//
//	// Declare) 
//	size_t	sizeResult;
//
//	// 1) File을 읽는다.
//	sizeResult	 = fread(_data, 1, _size, _handle_file);
//
//	// Return) 
//	return	sizeResult;
//}
//
//size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data)
//{
//	// check)
//	CGASSERT_ERROR(_data!=nullptr);
//
//	// check)
//	RETURN_IF(_data==nullptr, 0);
//
//	// check)
//	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);
//
//	// Declare) 
//	size_t	sizeResult;
//
//	// 1) File을 읽는다.
//	sizeResult	 = fwrite(_data, 1, _size, _handle_file);
//
//	// Return) 
//	return	sizeResult;
//}
//
//size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin)
//{
//	// check) 
//	CGASSERT_ERROR(_handle_file!=INVALID_FILE_HANDLE);
//
//	// check) 
//	RETURN_IF(_handle_file==INVALID_FILE_HANDLE, 0);
//
//	// Declare) 
//	size_t	result	 = 0;
//
//	// 1) seek
//	result	 = fseek(_handle_file, static_cast<long>(_offset), static_cast<int>(_origin));
//
//	// Return) 
//	return	result;
//}
//
//
//


}