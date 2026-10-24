 #include "pch.h"

#if defined(__ANDROID__)

namespace CGDK
{

FILE_HANDLE api::file::open(const std::string& _filename, eFILE_OPERATION _operation, eFILE_LOCATION _file_location) noexcept
{
	// check)
	CGASSERT_ERROR(_filename.empty() == false);

	// check) _filename이 nullptr이면 안 된다.
	RETURN_IF(_filename.empty(), INVALID_FILE_HANDLE);


	//-----------------------------------------------------------------
	// 설명) 
	// Android의 경우 읽을 때는 AAssetManager를 통해 파일을 오픈하고
	// 읽어야 하며
	// 쓸 때는 standard의 fopen, fread 등을 통해 Access해야 한다.
	//
	//-----------------------------------------------------------------
	// declare) 
	FILE_HANDLE	hFile = INVALID_FILE_HANDLE;

	switch(_operation)
	{
	case	eFILE_OPERATION::READ:
			{
				// 1) Activity를 얻는다.
				auto pActive = resource::manager::instance()->get_activity();

				// check) pNativeActivity가 nullptr이면 Exception을 던진다.
				if(pActive==nullptr)
					return hFile;

				// declare)
				AAsset* pAsset = nullptr;

				// 2) 파일명 그대로 열기를 시도한다.
				if ((_file_location & eFILE_LOCATION::NOW)!=eFILE_LOCATION::NONE)
					pAsset = AAssetManager_open(pActive->assetManager, _filename, AASSET_MODE_STREAMING);

				// 3) File의 open이 여기서도 실패한 경우 eFILE_LOCATION::INTERNAL을 시도해 본다.
				if(pAsset==nullptr && (_file_location & eFILE_LOCATION::INTERNAL)!=eFILE_LOCATION::NONE)
				{
					// - Activity를 얻는다.
					auto pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if(pActive != nullptr && pActive->internalDataPath!=nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						snprintf(temp_string, MAX_STRING_LENGTH, "%s/%s", pActive->internalDataPath, _filename);

						// - 파일을 연다.
						pAsset = AAssetManager_open(pActive->assetManager, temp_string, AASSET_MODE_STREAMING);
					}
				}

				// 4) File의 open이 여기서도 실패한 경우 eFILE_LOCATION::EXTERNAL을 시도해 본다.
				if (pAsset==nullptr && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
				{
					// - Activity를 얻는다.
					auto pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if(pActive != nullptr && pActive->externalDataPath!=nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), pActive->externalDataPath, _filename);

						// - 파일을 연다.
						pAsset = AAssetManager_open(pActive->assetManager, temp_string, AASSET_MODE_STREAMING);
					}
				}

				// 5) Resource Path에서 열기를 시도한다.
				if(pAsset == nullptr && (_file_location & eFILE_LOCATION::RESOURCE)!=eFILE_LOCATION::NONE)
				{
					for(auto& iter: *resource::manager::instance())
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), iter.c_str(), _filename);

						// - 파일을 연다.
						pAsset = AAssetManager_open(pActive->assetManager, temp_string, AASSET_MODE_STREAMING);

						// check) 성공했으면 여기서 중단.
						BREAK_IF(pAsset != nullptr);
					}
				}

				// 6) 값을 대입한다.
				hFile = pAsset;
			}
			break;

	case	eFILE_OPERATION::WRITE:
			{
				// declare)
				::FILE*	pFile = nullptr;

				// 1) 파일명 그대로 열기를 시도한다.
				if ((_file_location & eFILE_LOCATION::NOW) != eFILE_LOCATION::NONE)
					pFile = std::fopen(_filename, "wb");

				// 2) 파일 열기에 실패한 경우 eFILE_LOCATION::INTERNAL 플래그가 설정되어 있다면 INTERNAL 디렉터리에서 열기를 시도한다.
				if(pFile==nullptr && (_file_location & eFILE_LOCATION::INTERNAL)!=eFILE_LOCATION::NONE)
				{
					// 1) Activity를 얻는다.
					auto pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if(pActive != nullptr && pActive->internalDataPath!=nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						snprintf(temp_string, MAX_STRING_LENGTH, "%s/%s", pActive->internalDataPath, _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "wb");
					}
				}

				// 3) 파일 열기에 실패한 경우 eFILE_LOCATION::EXTERNAL 플래그가 설정되어 있다면 EXTERNAL 디렉터리에서 열기를 시도한다.
				if (pFile == nullptr && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
				{
					// 1) Activity를 얻는다.
					auto pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if(pActive != nullptr && pActive->externalDataPath!=nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), pActive->externalDataPath, _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "wb");
					}
				}

				// 4) 파일 열기에 실패한 경우 eFILE_LOCATION::RESOURCE 플래그가 설정되어 있다면 등록된 Resource 디렉터리에서 열기를 시도한다.
				if(pFile == nullptr && (_file_location & eFILE_LOCATION::RESOURCE)!=eFILE_LOCATION::NONE)
				{
					for(auto& iter: *resource::manager::instance())
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), iter.c_str(), _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "wb");

						// check) 성공했으면 여기서 중단.
						BREAK_IF(pFile != nullptr);
					}
				}

				// 5) 값을 대입한다.
				hFile = pFile;
			}
			break;

	case	eFILE_OPERATION::APPEND:
			{
				// declare)
				::FILE*	pFile = nullptr;

				// 1) 파일명 그대로 열기를 시도한다.
				if((_file_location & eFILE_LOCATION::NOW)!=eFILE_LOCATION::NONE)
					pFile = std::fopen(_filename, "a+");

				// 2) 파일 열기에 실패한 경우 eFILE_LOCATION::INTERNAL 플래그가 설정되어 있다면 INTERNAL 디렉터리에서 열기를 시도한다.
				if (pFile == nullptr && (_file_location & eFILE_LOCATION::INTERNAL) != eFILE_LOCATION::NONE)
				{
					// - Activity를 얻는다.
					auto	pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if (pActive != nullptr && pActive->internalDataPath != nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						snprintf(temp_string, MAX_STRING_LENGTH, "%s/%s", pActive->internalDataPath, _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "a+");
					}
				}

				// 3) 파일 열기에 실패한 경우 eFILE_LOCATION::EXTERNAL 플래그가 설정되어 있다면 EXTERNAL 디렉터리에서 열기를 시도한다.
				if (pFile == nullptr && (_file_location & eFILE_LOCATION::EXTERNAL) != eFILE_LOCATION::NONE)
				{
					// - Activity를 얻는다.
					auto pActive = resource::manager::instance()->get_activity();

					// check) pNativeActivity가 nullptr이면 Exception을 던진다.
					if (pActive != nullptr && pActive->externalDataPath != nullptr)
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), pActive->externalDataPath, _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "a+");
					}
				}

				// 4) 파일 열기에 실패한 경우 eFILE_LOCATION::RESOURCE 플래그가 설정되어 있다면 등록된 Resource 디렉터리에서 열기를 시도한다.
				if(pFile == nullptr && (_file_location & eFILE_LOCATION::RESOURCE)!=eFILE_LOCATION::NONE)
				{
					for(auto& iter: *resource::manager::instance())
					{
						// declare) 
						char temp_string[MAX_STRING_LENGTH];

						// - Path를 작성한다.
						std::snprintf(temp_string, std::size(temp_string), std::string_view("%s/%s").data(), iter.c_str(), _filename);

						// - 읽어들인다.
						pFile = std::fopen(temp_string, "a+");

						// check) 성공했으면 여기서 중단.
						BREAK_IF(pFile!=nullptr);
					}
				}

				// 2) 값을 대입한다.
				hFile = pFile;
			}
			break;

	default:
			CGASSERT_ERROR(false);
			break;
	}

	return	hFile;
}

size_t api::file::read(FILE_HANDLE _handle_file, size_t _size, void* _data) noexcept
{
	// check)
	CGASSERT_FATAL(_data!=nullptr);

	// check)
	if(_data == nullptr || _size == 0)
		return 0;

	// check)
	if(_handle_file.hRead == nullptr)
		return 0;

	// declare)
	size_t size_result;

	// 1) File을 읽는다.
	size_result = AAsset_read(_handle_file.hRead, _data, _size);

	return size_result;
}

size_t api::file::write(FILE_HANDLE _handle_file, size_t _size, const void* _data) noexcept
{
	// check)
	CGASSERT_FATAL(_data != nullptr);

	// check)
	if(_data == nullptr)
		return 0;

	// check)
	if(_handle_file.hWrite == nullptr)
		return 0;

	// 1) File을 읽는다.
	auto size_result = std::fwrite(_data, 1, _size, _handle_file);

	// Return) 
	return size_result;
}

size_t api::file::seek(FILE_HANDLE _handle_file, size_t _offset, eFILE_ORIGIN _origin) noexcept
{
	// declare) 
	size_t result = 0;

	if(_handle_file.hRead!=nullptr)
	{
	#if INTPTR_MAX == INT32_MAX
		result = AAsset_seek(_handle_file.hRead, _offset, static_cast<int>(_origin));
	#elif INTPTR_MAX == INT64_MAX
		result = AAsset_seek64(_handle_file.hRead, _offset, static_cast<int>(_origin));
	#else
		#error "Environment not 32 or 64-bit!"
	#endif
	}
	else if(_handle_file.hWrite!=nullptr)
	{
		result = std::fseek(_handle_file.hWrite, _offset, static_cast<int>(_origin));
	}

	// Return) 
	return result;
}

size_t api::file::get_length(FILE_HANDLE _handle_file) noexcept
{
	// declare) 
	size_t result = 0;

	if(_handle_file.hRead!=nullptr)
	{
	#if INTPTR_MAX == INT32_MAX
		result = AAsset_getLength(_handle_file.hRead);
	#elif INTPTR_MAX == INT64_MAX
		result = AAsset_getLength64(_handle_file.hRead);
	#else
		#error "Environment not 32 or 64-bit!"
	#endif
	}
	else if(_handle_file.hWrite != nullptr)
	{
		fpos_t	pos_now = 0;
		fgetpos(_handle_file.hWrite, &pos_now);

		std::fseek(_handle_file.hWrite, 0, SEEK_END);
		result = ftell(_handle_file.hWrite);

		std::fseek(_handle_file.hWrite, pos_now, SEEK_SET);
	}

	// return) 
	return result;
}

size_t api::file::truncate(FILE_HANDLE /*_handle_file*/, size_t /*_length*/) noexcept
{
	// yet...
	CGASSERT_ERROR(false);

	// return) 
	return 0;
}

std::filesystem::file_time_type api::file::get_last_write_time(const std::string& _filename)
{
	//return std::filesystem::last_write_time(_filename);

	// yet...
	CGASSERT_ERROR(false);

	// return) 
	return false;
}

void api::file::set_last_write_time(const std::string& _filename, std::filesystem::file_time_type _write_time)
{
	//// 1) set file modifi time
	//std::filesystem::last_write_time(_filename, _write_time);

	// yet...
	CGASSERT_ERROR(false);
}


//void CGArchive::CFile::OpenForLoad(const std::string_view _filename)
//{
//	// 1) Activity를 얻는다.
//	auto	pActive	 = resource::manager::instance()->get_activity();
//
//	// check) pNativeActivity가 nullptr이면 Exception을 던진다.
//	THROW_IF(pActive==nullptr, CGException::CThrow_Error(FAIL_TO_CREATE));
//
//	// 2) 파일을 연다~
//	auto* pAsset = AAssetManager_open(pActive->assetManager, _filename, AASSET_MODE_STREAMING);
//
//	// 3) 읽지 못했다면 Resource Path에서 읽기를 시도한다.
//	if(pAsset==nullptr)
//	{
//		for(auto& iter: *resource::manager::instance())
//		{
//			// declare) 
//			char	temp_string[MAX_STRING_LENGTH];
//
//#if defined(_USE_CHAR_STRING)
//			snprintf(temp_string, MAX_STRING_LENGTH, "%s/%s", iter.c_str(), _filename);
//#elif defined(_USE_UCS16_ENCODING)
//			snprintf(temp_string, MAX_STRING_LENGTH, "%ls/%s", iter.c_str(), _filename);
//#else
//#error "Unsupported Platform"
//#endif
//
//			// - 읽어들인다.
//			pAsset = AAssetManager_open(pActive->assetManager, temp_string, AASSET_MODE_STREAMING);
//
//			// check) 성공했으면 여기서 중단.
//			BREAK_IF(pAsset==nullptr);
//		}
//	}
//
//	// check) pAsset이면 Exception을 던진다.	
//	THROW_IF(pAsset==nullptr, CGException::CThrow_Error(FAIL_TO_CREATE));
//
//	// 4) Asset을 설정..
//	m_handle_file		 = pAsset;
//	m_status	 = CGARCHIVE_STATUS_LOADING;
//
//	// 5) 파일 길이를 구한다.
//	m_file_length = static_cast<size_t>(AAsset_getLength (pAsset));
//
//	// 6) File name을 저장해 놓는다.
//#if defined(_USE_CHAR_STRING)
//	std::char_traits<char>::copy(m_file_name, _filename, MAX_LENGTH_OF_FILENAME);
//#else
//	encode_string(m_file_name, _filename);
//#endif
//}
////m_status	 = ((_dwDesiredAccess & GENERIC_READ)==GENERIC_READ) ? CGARCHIVE_STATUS_LOADING : CGARCHIVE_STATUS_SAVING;
//
//void CGArchive::CFile::OpenForLoad(const wchar_t* _filename)
//{
//	// declare) 
//	char	str_buffer[MAX_STRING_LENGTH];
//
//	// 1) Load한다.
//	OpenForLoad(str_buffer);
//}
//
//void CGArchive::CFile::OpenForSave(const std::string_view _filename)
//{
//	//// 1) File을 open한다.
//	//bool	bResult	 = ProcessOpen(_filename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
//
//	//// check) File을 제대로 열지 못했으면 Return한다.
//	//RETURN_IF(bResult==false, false);
//
//	//// 2) executor에 붙인다.
//	//object_ptr<executor::CCompletionPort>	pExecutor = executor::Default::instance()->executor();
//
//	//// 3) executor를 설정한다.(생성하면 기본 executor로 설정한다.)
//	//bResult	 = pExecutor->attach(m_handle_file);
//
//	//// check) File Time을 쓰지 못하면 되돌린다.
//	//RETURN_IF(bResult==false, false);
//
//	//// 4) position을 0으로 설정한다.
//	//m_dwPosition		 = 0;
//	//m_bufferSent.len	 = 0;
//
//	//// Return) 
//	//return true;
//}
//
//void CGArchive::CFile::OpenForSave(const wchar_t* _filename)
//{
//	// declare) 
//	char	str_buffer[MAX_STRING_LENGTH];
//
//	OpenForSave(encode_string(str_buffer, _filename));
//}
//
//void CGArchive::CFile::CloseF()
//{
//	// check)
//	RETURN_IF(m_handle_file==nullptr);
//
//	// 1) 완료될 때까지 대기한다.
//
//	// 2) File을 닫는다.
//	AAsset_close(m_handle_file);
//
//	// 3) m_handle_file을 nullptr로 한다.
//	m_handle_file	 = nullptr;
//
//	// 4) FileName을 clear한다.
//	m_file_name[0] = 0;
//}
//
//size_t CGArchive::CFile::ProcessRead(buffer& _buffer)
//{
//	// check) Debug를 위한 잘못된 값을 검사.
//	CGASSERT_ERROR(m_handle_file!=nullptr);
//
//	// 1) Read한다.
//	auto	iResult	 = AAsset_read(m_handle_file, _buffer.buf, _buffer.len);
//
//	// check) iResult가 0보다 작으면 Exception을 던진다.
//	THROW_IF(iResult<0, CGException::CThrow_DomainError("fail to read file (" __FILE__) ")"), LOG_ERROR(DEFAULT_LOGGER, "(err ) CGDK.system.object: fail to read file {} ({})", m_file_name, __FUNCTION__));
//
//	// 2) 읽은 값을 len값에 써넣는다.
//	_buffer.len	 = static_cast<uint32_t>(iResult);
//
//	// Return) 읽은 Byte수를 return한다.
//	return	static_cast<size_t>(iResult);
//}
//
//size_t CGArchive::CFile::ProcessWrite(const buffer& _buffer)
//{
//	//// check)
//	//RETURN_IF(_buffer.len==0, 0);
//
//	//// 설명) 이미 Writing중인지를 확인한다.
//	//SCOPED_LOCK(m_lockable_buffer)
//	//{
//	//	// 1) send중이면 Queuing만 하고 끝낸다.
//	//	if(m_bufferSent.len!=0)
//	//	{
//	//		if(m_listQueued.empty() || (MAX_WRITE_BUFFER-m_listQueued.back().len)<_buffer.len)
//	//		{
//	//			m_listQueued.push_back(buffer(MEM_POOL_ALLOC(MAX_WRITE_BUFFER)));
//	//		}
//
//	//		m_listQueued.back().append(_buffer);
//
//	//		// Return) 
//	//		return	_buffer.len;
//	//	}
//
//	//	// 2) Sent buffer에 저장 후 전송한다.
//	//	m_bufferSent	 = _buffer;
//	//}
//
//	//// 2) write한다.
//	//return	RequestProcessWrite(_buffer.buf, _buffer.len);
//
//	return	0;
//}
//
//size_t CGArchive::CFile::RequestProcessWrite(_In_reads_bytes_(_Bytes) const void* _pData, size_t _Bytes)
//{
//	//// 1) WriteFileBlock에 값을 써넣는다.
//	//Offset			 = static_cast<uint32_t>(m_dwPosition);
//	//OffsetHigh		 = 0;
//
//	//// 2) Offset을 연장한다.
//	//m_dwPosition	+= _Bytes;
//
//	//// 3) Completor를 add_ref한다.
//	//add_ref();
//
//	//// declare) 
//	//uint32_t	dwBytes	 = 0;
//
//	//// 4) Overlapped로 File에 write한다.
//	//BOOL	result	 = WriteFile(m_handle_file, _pData, static_cast<uint32_t>(_Bytes), reinterpret_cast<LPDWORD>(&dwBytes), static_cast<LPOVERLAPPED>(this));
//
//	//// check) Error가 발생했고 그것이 IO Pending이 아니라면 Error가 발생한 것이다.
//	//if(result == FALSE && GetLastError()!=ERROR_IO_PENDING)
//	//{
//	//	// - position을 다시 원상복구시킨다.
//	//	m_dwPosition	-= _Bytes;
//
//	//	// - Release한다.
//	//	release();
//
//	//	// trace) 
//	//	LOG_ERROR(DEFAULT_LOGGER, "(err ) fail to writing to file[position :{}, size: {}] ({})", Offset, static_cast<int64_t>(_Bytes), __FUNCTION__);
//	//}
//
//	//// Return) 읽은 Byte수를 return한다.
//	//return	_Bytes;
//
//	return	0;
//}
//
//intptr_t CGArchive::CFile::process_execute(intptr_t /*_return*/, size_t /*_param*/)
//{
//	//SCOPED_LOCK(m_lockable_buffer)
//	//{
//	//	// 1) Sent buffer를 clear한다.
//	//	m_bufferSent.clear();
//
//	//	// check) Queuing된 것이 없으면 그냥 끝낸다.
//	//	RETURN_IF(m_listQueued.empty(), intptr_t(1));
//
//	//	// 2) Queuing된 것을 하나 끄집어 낸다.
//	//	m_bufferSent	 = m_listQueued.front();
//	//	m_listQueued.pop_front();
//	//}
//
//	//// 3) 전송한다.
//	//RequestProcessWrite(m_bufferSent.buf, static_cast<uint32_t>(m_bufferSent.len));
//
//	// Return) Success!
//	return	intptr_t(0);
//}


}

#endif