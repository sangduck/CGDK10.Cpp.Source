#include "pch.h"

namespace CGDK
{

#define	FILE_WRITE_BUFFER_SIZE	(  64*1024)
#define FILE_READ_BUFFER_SIZE	(1024*1024)

//result_code archive::file::start(const std::string_view _filename, eFILE_OPERATION _file_operation, eFILE_LOCATION _file_Location, bool _is_instant_open)
//{
//	// declare) 
//	context temp_context;
//
//	// 1) set file name
//	temp_context["filename"sv] = _filename;
//
//	// 2) file operation
//	{
//		// declare) 
//		std::string_view str_file_operation;
//
//		switch (_file_operation)
//		{
//		case	eFILE_OPERATION::NONE:
//				str_file_operation = "none"sv;
//				break;
//
//		case	eFILE_OPERATION::READ:
//				str_file_operation = "read"sv;
//				break;
//
//		case	eFILE_OPERATION::WRITE:
//				str_file_operation = "write"sv;
//				break;
//
//		case	eFILE_OPERATION::APPEND:
//				str_file_operation = "append"sv;
//				break;
//
//		case	eFILE_OPERATION::READ_ASYNC:
//				str_file_operation = "read_async"sv;
//				break;
//
//		case	eFILE_OPERATION::WRITE_ASYNC:
//				str_file_operation = "write_async"sv;
//				break;
//
//		case	eFILE_OPERATION::APPEND_ASYNC:
//				str_file_operation = "append_async"sv;
//				break;
//
//		default:
//				str_file_operation = "none"sv;
//				break;
//		};
//
//		temp_context["file_operation"sv] = str_file_operation;
//	}
//
//	// 3) file location
//	{
//		// declare) 
//		std::string_view str_file_location;
//
//		switch (_file_Location)
//		{
//		case	eFILE_LOCATION::NONE:
//				str_file_location = "none"sv;
//				break;
//
//		case	eFILE_LOCATION::NOW:
//				str_file_location = "now"sv;
//				break;
//
//		case	eFILE_LOCATION::RESOURCE:
//				str_file_location = "resource"sv;
//				break;
//
//		case	eFILE_LOCATION::SYSTEM:
//				str_file_location = "system"sv
//				break;
//
//		case	eFILE_LOCATION::INTERNAL:
//				str_file_location = "internal"sv;
//				break;
//
//		case	eFILE_LOCATION::EXTERNAL:
//				str_file_location = "external"sv;
//				break;
//
//		default:
//				str_file_location = "none"sv;
//				break;
//		};
//
//		temp_context["file_location"] = str_file_location;
//	}
//
//	// 4) instant open
//	{
//		temp_context["instant_open"sv] = (_is_instant_open == true) ? "true"sv : "false"sv;
//	}
//
//	// 5) start
//	return start(temp_context);
//}

uint32_t archive::file::set_status(uint32_t _status)
{
	// 1) 이전 상태를 저장해 놓는다.
	uint32_t status_pre = this->m_status;

	// 2) 상태를 바꾼다.
	this->m_status = _status;

	// return) 이전 상태를 되돌린다.
	return status_pre;
}

result_code archive::file::_process_initializing(context& _context)
{
	// 1) call 'on_initializing'
	this->on_initializing(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_initialize(context& _context)
{
	// 1) call 'on_initialize'
	this->on_initialize(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_destroying() noexcept
{
	// 1) call 'on_destroying'
	this->on_destroying();

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_destroy() noexcept
{
	// 1) call 'on_destroying'
	this->on_destroy();

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_starting(context& _context)
{
	// 1) call 'on_starting'
	this->on_starting(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_start(context& _context)
{
	// declare) 
	START_PARAMETER parameter;

	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		// - get parameter
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - "filename"
		if (parameter.filename.empty() == false)
		{
			LOG_INFO << "  + parameter['filename']: "sv << parameter.filename;
		}

		// - "file_operation"
		if (parameter.file_operation != eFILE_OPERATION::NONE)
		{
			// declare) 
			std::string_view str_file_operation;
			
			switch (parameter.file_operation)
			{
			case	eFILE_OPERATION::NONE:
					str_file_operation = "none"sv;
					break;
			
			case	eFILE_OPERATION::READ:
					str_file_operation = "read"sv;
					break;
			
			case	eFILE_OPERATION::WRITE:
					str_file_operation = "write"sv;
					break;
			
			case	eFILE_OPERATION::APPEND:
					str_file_operation = "append"sv;
					break;
			
			case	eFILE_OPERATION::READ_ASYNC:
					str_file_operation = "read_async"sv;
					break;
			
			case	eFILE_OPERATION::WRITE_ASYNC:
					str_file_operation = "write_async"sv;
					break;
			
			case	eFILE_OPERATION::APPEND_ASYNC:
					str_file_operation = "append_async"sv;
					break;
			
			default:
					str_file_operation = "none";
					break;
			};
			
			LOG_INFO << "  + parameter['file_operation']: "sv << str_file_operation;
		}

		// - "file_location"
		if (parameter.file_location != eFILE_LOCATION::NONE)
		{
			// declare) 
			std::string_view str_file_location;
			
			switch (parameter.file_location)
			{
			case	eFILE_LOCATION::NONE:
					str_file_location = "none"sv;
					break;
			
			case	eFILE_LOCATION::NOW:
					str_file_location = "now"sv;
					break;
			
			case	eFILE_LOCATION::RESOURCE:
					str_file_location = "resource"sv;
					break;
			
			case	eFILE_LOCATION::SYSTEM:
					str_file_location = "system"sv;
					break;
			
			case	eFILE_LOCATION::INTERNAL:
					str_file_location = "internal"sv;
					break;
			
			case	eFILE_LOCATION::EXTERNAL:
					str_file_location = "external"sv;
					break;
			
			default:
					str_file_location = "none";
					break;
			};

			LOG_INFO << "  + parameter['file_location']: "sv << str_file_location;
		}

		// - "create_new"
		LOG_INFO << "  + parameter['create_new']: "sv << ((parameter.is_create_new) ? "true"sv : "false"sv);

		// - "is_instant_open"
		LOG_INFO << "  + parameter['instant_open']: "sv << ((parameter.is_instant_open) ? "true"sv : "false"sv);
	}
	else
	{
		// 1) filename을 읽어온다.
		{
			// declare) 
			std::string str_filename;

			// - "filename"키에 대응하는 값을 얻는다.
			str_filename = (std::string)_context.find("filename"sv, "file_name"sv, "file name"sv);

			// - "filename"키에 대응하는 값을 얻었다면...
			if (str_filename.empty() == false)
			{
				// - Filename을 설정한다.
				parameter.filename = str_filename;

				LOG_INFO << "  + parameter['filename']: "sv << parameter.filename;
			}
		}

		// - operation을 읽어온다.
		{
			// declare) 
			std::string str_operation;

			// - "file_operation"키에 대응하는 값을 얻는다.
			str_operation = (std::string)_context.find("file_operation"sv, "file operation"sv, "fileoperation"sv);

			// - "file_operation"키에 대응하는 값을 얻었다면...
			if (str_operation.empty() == false)
			{
				// - IPv6로 설정한다.
				if (__stricmp(str_operation, "read"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::READ;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: read"sv;
				}
				else if (__stricmp(str_operation, "write"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::WRITE;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: write"sv;
				}
				else if (__stricmp(str_operation, "append"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::APPEND;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: append"sv;
				}
				else if (__stricmp(str_operation, "read_async"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::READ_ASYNC;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: read_async"sv;
				}
				else if (__stricmp(str_operation, "write_async"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::WRITE_ASYNC;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: write_async"sv;
				}
				else if (__stricmp(str_operation, "append_async"sv) == 0)
				{
					parameter.file_operation = eFILE_OPERATION::APPEND_ASYNC;

					// trace)
					LOG_INFO << "  + parameter['file_operation']: append_async"sv;
				}
			}
		}

		// - file_location을 읽어온다.
		{
			// declare) 
			std::string str_file_location;

			// - "file_location"키에 대응하는 값을 얻는다.
			str_file_location = (std::string)_context.find("file_location"sv, "file location"sv, "filelocation"sv);

			// - "file_location"키에 대응하는 값을 얻었다면...
			if (str_file_location.empty() == false)
			{
				// - IPv6로 설정한다.
				if (__stricmp(str_file_location, "now"sv) == 0)
				{
					parameter.file_location = eFILE_LOCATION::NOW;

					// trace)
					LOG_INFO << "  + parameter['file_location']: now"sv;
				}
				else if (__stricmp(str_file_location, "resource"sv) == 0)
				{
					parameter.file_location = eFILE_LOCATION::RESOURCE;

					// trace)
					LOG_INFO << "  + parameter['file_location']: resource"sv;
				}
				else if (__stricmp(str_file_location, "system"sv) == 0)
				{
					parameter.file_location = eFILE_LOCATION::SYSTEM;

					// trace)
					LOG_INFO << "  + parameter['file_location']: system"sv;
				}
				else if (__stricmp(str_file_location, "internal"sv) == 0)
				{
					parameter.file_location = eFILE_LOCATION::INTERNAL;

					// trace)
					LOG_INFO << "  + parameter['file_location']: internal"sv;
				}
				else if (__stricmp(str_file_location, "external"sv) == 0)
				{
					parameter.file_location = eFILE_LOCATION::EXTERNAL;

					// trace)
					LOG_INFO << "  + parameter['file_location']: external"sv;
				}
			}
		}

		// - create_new을 읽어온다.
		{
			// declare) 
			std::string str_option;

			// - "create_new"키에 대응하는 값을 얻는다.
			str_option = (std::string)_context.find("create_new"sv, "create new"sv, "createnew"sv);

			// - "create_new"키에 대응하는 값을 얻었다면...
			if (str_option.empty() == false)
			{
				// - IPv6로 설정한다.
				if (__stricmp(str_option, "yes"sv) == 0
					|| __stricmp(str_option, "true"sv) == 0)
				{
					parameter.is_create_new = true;

					// trace)
					LOG_INFO << "  + parameter['create_new']: true"sv;
				}
			}
		}


		// - instant open
		{
			// declare) 
			std::string str_instant_open;

			// - "instant_open"키에 대응하는 값을 얻는다.
			str_instant_open = (std::string)_context.find("instant_open"sv, "instant open"sv);

			// - "instant_open"키에 대응하는 값을 얻었다면...
			if (str_instant_open.empty() == false)
			{
				if (__stricmp(str_instant_open, "true"sv) == 0)
				{
					parameter.is_instant_open = true;

					// trace)
					LOG_INFO << "  + parameter['instant_open']: true"sv;
				}
				else if (__stricmp(str_instant_open, "false"sv) == 0)
				{
					parameter.is_instant_open = false;

					// trace)
					LOG_INFO << "  + parameter['instant_open']: false"sv;
				}
				else if (__stricmp(str_instant_open, "1"sv) == 0)
				{
					parameter.is_instant_open = true;

					// trace)
					LOG_INFO << "  + parameter['instant_open']: true"sv;
				}
				else if (__stricmp(str_instant_open, "0"sv) == 0)
				{
					parameter.is_instant_open = false;

					// trace)
					LOG_INFO << "  + parameter['instant_open']: false"sv;
				}
				else if (__stricmp(str_instant_open, "default"sv) == 0)
				{
					parameter.is_instant_open = false;

					// trace)
					LOG_INFO << "  + parameter['instant_open']: default(false)"sv;
				}
			}
		}
	}
	
	// 6) get executor
#if defined(__linux__)
	this->m_pexecutor = system_executor::executor();
#endif

	// 7) file open
	_file_open(parameter.filename, parameter.file_operation, parameter.file_location, parameter.is_create_new, parameter.is_instant_open);

	// 8) call 'on_start;
	on_start(_context);

	// return)
	return result_code(eRESULT::SUCCESS);
}

void archive::file::_file_open(std::string_view _filename, eFILE_OPERATION _file_operation, eFILE_LOCATION	_file_location, bool _create_new, bool _instant_open)
{
	// 1) 이전에 파일이 open되어 있으면 먼저 닫는다.
	_file_close();

	// declare)
	uint32_t status = 0;

	// 2) status
	switch(_file_operation)
	{
	case	eFILE_OPERATION::READ:
	case	eFILE_OPERATION::READ_ASYNC:
			status = CGARCHIVE_STATUS_LOADING;
			break;

	case	eFILE_OPERATION::WRITE:
	case	eFILE_OPERATION::WRITE_ASYNC:
	case	eFILE_OPERATION::APPEND:
	case	eFILE_OPERATION::APPEND_ASYNC:
			status = CGARCHIVE_STATUS_LOADING;
			break;

	default:
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	};

	// check) _filename 이름이 nullptr이면 안됀다.
	THROW_IF(_filename.empty(), throwable::failure(), LOG_ERROR << "  (err ) _filename is not assigned"sv);

	// check) _filename 이름이 nullptr이면 안됀다.
	THROW_IF(_file_operation == eFILE_OPERATION::NONE, throwable::failure(), LOG_ERROR << "  (err ) _file_operation is not assigned"sv);

	// 3) create new first
	if(_create_new)
	{
		// - CREATE_NEW로 일단 연다.
		auto hFile_temp = api::file::open(_filename, eFILE_OPERATION::WRITE, _file_location);

		// - 그냥 바로 닫는다.
		api::file::close(hFile_temp);
	}

	// 4) File을 연다.
	auto hFile = api::file::open(_filename, _file_operation, _file_location);

	// check) pAsset이면 Exception을 던진다.	

	// check) 
	if (hFile == INVALID_FILE_HANDLE)
	{
		// log) 
		LOG_ERROR << "(err ) CGDK.system.object: fail to open file '" << _filename << "' error message: " << api::get_last_error_string();

		// return)
		throw throwable::failure(eRESULT::FAIL_TO_OPEN);
	}

	// 5) Asset을 설정..
	this->m_handle_file = hFile;
	this->m_status = status;
	this->m_file_operation = _file_operation;
	this->m_file_location = _file_location;
	this->m_instant_open = _instant_open;

	// 7) 파일 길이를 구한다.
#if defined(_WIN32)
	this->m_file_length = api::file::get_length(hFile);
#elif defined(__ANDROID__) || defined(__linux__)
	this->m_processing = false;
#endif

	// 8) File name을 저장해 놓는다.
	this->m_file_name = _filename;

	// 9) close file if instant open is 'true'
	if (_instant_open)
	{
		// - close file
		api::file::close(this->m_handle_file);

		// - reset m_handle_file
		this->m_handle_file = INVALID_FILE_HANDLE;
	}

	// 10) 읽어들일 버퍼를 함당한다. (버퍼 Pointer는 0!)
#if defined(_WIN32)
	((size_t&)Pointer) = (_file_operation == eFILE_OPERATION::APPEND || _file_operation == eFILE_OPERATION::APPEND_ASYNC) ? this->m_file_length : 0;
#endif
	this->m_buffer_sended = alloc_shared_buffer(FILE_READ_BUFFER_SIZE);

	// check)
	CGASSERT_ERROR(this->m_buffer_sended.get_source().exist())
}

void archive::file::_file_close()
{
	// check)
	RETURN_IF(this->m_file_name.empty());

	if(this->m_file_operation >= eFILE_OPERATION::READ_ASYNC)
	{
		// 1) 저장된 데이터를 모두 저장할 때까지 대기한다.
	#if defined(_WIN32)
		// check) File 이름이 null이면 안됀다.
		if(this->m_file_name.empty() == false)
		{
			// 2) 다 저장할 때까지 기다린다.(최대 100초간)
			for(int i=0; i<10000; ++i)
			{
				// check) 
				scoped_lock(this->m_lockable_buffer)
				{
					BREAK_IF(this->m_handle_file == INVALID_FILE_HANDLE);

					BREAK_IF(this->m_list_buffer_queuing.empty() && this->m_buffer_sended.empty());
				}

				// - 10ms동안 Sleep한다.
				std::this_thread::sleep_for(1ms);
			}

			// 3) clear~
			scoped_lock(this->m_lockable_buffer)
			{
				this->m_list_buffer_queuing.clear();
			}

			// 4) filename을 clear
			this->m_file_name.clear();
		}
	#elif defined(__linux__)
		// check) File 이름이 null이면 안됀다.
		if(this->m_file_name.empty() == false)
		{
			// 2) 다 저장할 때까지 기다린다.(최대 100sec)
			for(int i=0; i<10000; ++i)
			{
				// check) 
				scoped_lock(this->m_lockable_buffer)
				{
					BREAK_IF(this->m_handle_file == INVALID_FILE_HANDLE);

					BREAK_IF(this->m_list_buffer_queuing.empty());
				}

				// - sleep for 10ms
				std::this_thread::sleep_for(1ms);
			}

			// 3) clear queue
			scoped_lock(this->m_lockable_buffer)
			{
				this->m_list_buffer_queuing.clear();
			}

			// 4) clear filename
			this->m_file_name.clear();
		}
	#else
			#error	"[CGDK10] Not supported UWP Platform"
	#endif

	#if defined(_WIN32)
		// 2) detach executor
		{
			// - get executor
			auto pexecutor = system_executor::executor();

			// - detach executor
			if (pexecutor.exist())
			{
				pexecutor->detach(this->m_handle_file);
			}
		}
	#endif
	}

	// 3) close file and set 'm_handle_file' to 'INVALID_FILE_HANDLE'
	if(this->m_handle_file != INVALID_FILE_HANDLE)
	{
		api::file::close(this->m_handle_file);
		this->m_handle_file = INVALID_FILE_HANDLE;
	}

	// 4) reset file info
	this->m_file_name.clear();

	// 5) clear all buffers
	scoped_lock(this->m_lockable_buffer)
	{
		this->m_buffer_sended.clear();
		this->m_list_buffer_queuing.clear();
	#if defined(_WIN32)
		((size_t&)Pointer) = 0;
		OffsetHigh = 0;
	#endif
	}
}

result_code archive::file::_process_stopping() noexcept
{
	// 1) close file
	this->_file_close();

	// 2) call 'on_stopping'
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code archive::file::_process_stop() noexcept
{
	// 1) get executor
#if defined(__linux__)
	this->m_pexecutor.reset();
#endif

	// 2) call 'on_stop'
	try
	{
		this->on_stop();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool archive::file::process_read(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(this->m_file_name.empty(), false);

	// declare)
	bool result;

	if (this->m_file_operation == eFILE_OPERATION::READ_ASYNC || this->m_file_operation == eFILE_OPERATION::APPEND_ASYNC)
	{
		result = this->process_read_async(_buffer);
	}
	else
	{
		result = this->process_read_sync(_buffer);
	}

	// return)
	return result;
}

#if defined(_WIN32)
bool archive::file::process_read_async(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(this->m_file_name.empty(), false);

	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(this->m_handle_file != INVALID_FILE_HANDLE);

	// check) _buffer.size_이 0이면 안됀다.
	CGASSERT_ERROR(_buffer.size()!= 0);

	// check) _buffer의 크기가 0이 아닌지 확인한다.
	RETURN_IF(_buffer.size() == 0, false);

	// declare)
	shared_buffer buf_temp = _buffer;

	// 1) 버퍼에 남은 데이터 크기가 읽으려는 데이터 크기보다 작으면 새로 읽어들인다.
	if (buf_temp.size() > this->m_buffer_sended.size())
	{
		// - 먼저 남은 데이터 복사
		if (this->m_buffer_sended.size() != 0)
		{
			// - m_buffer_sended에 남은 데이터를 일단 _buffer에 복사한다.
			memcpy(buf_temp.data(), this->m_buffer_sended.data(), this->m_buffer_sended.size());
			buf_temp += offset(this->m_buffer_sended.size());

			// - m_buffer_sended를 클리어~
			this->m_buffer_sended.clear();
		}

		// - 남은 파일의 길이를 계산한다.
		size_t size_remained = this->m_file_length - this->get_position();

		// check) 파일의 끝이면 여기서 끝낸다.
		if (size_remained == 0)
		{
			_buffer.set_size(static_cast<uint32_t>(buf_temp.data() - _buffer.data()));
			return _buffer.size();
		}

		// - read로 설정
		this->m_is_write = false;

		// Case A) 읽어들이려는 데이터가 FILE_READ_BUFFER_SIZE보다 작거나 남은 파일 크기보다 작을 경우
		if (buf_temp.size() < FILE_READ_BUFFER_SIZE && buf_temp.size() <size_remained)
		{
			// - 새 버퍼를 할당받는다.
			//   버퍼는 무족건 FILE_READ_BUFFER_SIZE크기로 할당받는다.
			//   같은 버퍼 크기로만 할당받는 것이 메모리풀의 잉어 데이터 갯수를 줄일 수 있기 때문이다.
			this->m_buffer_sended = alloc_shared_buffer(FILE_READ_BUFFER_SIZE);

			// - 새로 읽어 들일 데이터의 크기를 계산한다.
			//   (남은 파일의 길이와 FILE_READ_BUFFER_SIZE 중 작은 것으로...)
			auto size_bytes = __select_min(size_remained, static_cast<size_t>(FILE_READ_BUFFER_SIZE));

			// - Read한다.
			auto result = api::file::read(this->m_handle_file, size_bytes, this->m_buffer_sended.data());

			// check) 읽어들인 데이터의 크기(result)가 읽으려는 데이터의 크기와 다르면 Exception을 던진다.
			THROW_IF(result != size_bytes, throwable::failure(eRESULT::DOMAIN_ERROR, 0, "fail to read file (" __FILE__ ")"sv), LOG_ERROR << "(err ) CGDK.system.object: fail to read file " << this->m_file_name);

			// - 읽어들인 만큼 다시 포인터를 충전!
			this->m_buffer_sended.set_size(size_bytes);
		}
		// Case B) 읽어들이려는 데이터가 FILE_READ_BUFFER_SIZE보다 클 경우.
		//         - 한번에 파일에서 바로 해당 버퍼로 읽어들인다.
		else
		{
			// - shared_buffer 크기만큼 통채로 Read한다.
			auto result = api::file::read(this->m_handle_file, buf_temp.size(), buf_temp.data());

			// - 읽어들인 만큼...
			buf_temp += offset(result);
		}
	}

	// check) 복사할 크기를 계산한다. (복사할 버퍼의 크기(buf_temp.size_, 저장된 데이터의 크기(m_bufferSenede.size_))
	auto size_copy = __select_min(buf_temp.size(), this->m_buffer_sended.size());

	// 2) shared_buffer에 데이터를 복사한다.
	memcpy(buf_temp.data(), this->m_buffer_sended.data(), size_copy);

	// 3) 읽어 들인만큼 m_buffer_sended 버퍼의 포인터를 이동시킨다.
	this->m_buffer_sended	+= offset(size_copy);

	// 4) 읽어들인 데이터의 길이를 계산한다.
	auto bytes_read = (buf_temp.data() - _buffer.data()) + size_copy;

	// 5) 읽어들인 데이터의 길이를 _buffer.size_에 써넣는다.
	_buffer.set_size(bytes_read);

	// return) 읽은 Byte수를 return한다.
	return true;
}

bool archive::file::process_read_sync(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(this->m_file_name.empty(), false);

	// check) Debug를 위한 잘못된 값을 검사.
	CGASSERT_ERROR(this->m_handle_file != INVALID_FILE_HANDLE);

	// check) _buffer.size_이 0이면 안됀다.
	CGASSERT_ERROR(_buffer.size() != 0);

	// check) _buffer의 크기가 0이 아닌지 확인한다.
	RETURN_IF(_buffer.size() == 0, false);

	// 1) flag 설정
	this->m_is_write = false;

	// 2) read한다.
	auto bytes_read = api::file::read(this->m_handle_file, _buffer.size(), _buffer.data());

	// check) 하나도 잃지 못했으면 error 처리한다.
	RETURN_IF(bytes_read == 0, false);

	// 3) 읽어들인 데이터의 길이를 _buffer.size_에 써넣는다.
	_buffer.set_size(static_cast<uint32_t>(bytes_read));

	// 4) on write를 호출한다.
	this->on_read_complete(_buffer);

	// 5) offset을 더해준다.
#if defined(_WIN32)
	((size_t&)Pointer)	+= bytes_read;
#endif

	// return)
	return true;
}
#elif defined(__ANDROID__) || defined(__linux__)
bool archive::file::process_read_async(shared_buffer& /*_buffer*/)
{
	return false;
}

bool archive::file::process_read_sync(shared_buffer& /*_buffer*/)
{
	return false;
}
#endif


bool archive::file::process_write(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, false);

	// declare)
	bool result;

	if (this->m_file_operation == eFILE_OPERATION::WRITE_ASYNC || this->m_file_operation == eFILE_OPERATION::APPEND_ASYNC)
	{
		result = this->process_write_async(_buffer);
	}
	else
	{
		result = this->process_write_sync(_buffer);
	}

	// return)
	return result;
}

bool archive::file::process_write_sync(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(_buffer.size() == 0, 0);

	scoped_lock(this->m_lockable_buffer)
	{
		// 1) flag 설정
		this->m_is_write = true;

		// 2) open file if instant open is 'true'
		if (this->m_instant_open == true)
		{
			// check)
			CGASSERT_ERROR(this->m_handle_file == INVALID_FILE_HANDLE)

			// - open file
			this->m_handle_file = api::file::open(this->m_file_name, this->m_file_operation, this->m_file_location);

			// check) 
			if (this->m_handle_file == INVALID_FILE_HANDLE)
			{
				// log) 
				LOG_DEBUG << "(err ) CGDK.system.object: fail to open file file " << this->m_file_name << " (error message: " << api::get_last_error_string() << ")";

				// return)
				return false;
			}
		}

		// 3) write
		auto bytes_write = api::file::write(this->m_handle_file, _buffer.size(), _buffer.data());

		// check) 하나도 잃지 못했으면 error 처리한다.
		RETURN_IF(bytes_write == 0, false);

		// 4) on write를 호출한다.
		this->on_write_complete(_buffer ^ bytes_write);

		// 5) write한만큼 이동
	#if defined(_WIN32)
		((size_t&)Pointer) += bytes_write;
	#endif
		_buffer += offset(bytes_write);

		// 6) close handle if instant open flag set
		if (this->m_instant_open == true)
		{
			if (this->m_handle_file != INVALID_FILE_HANDLE)
			{
				// - close file
				api::file::close(this->m_handle_file);

				// - open file
				this->m_handle_file = INVALID_FILE_HANDLE;
			}
		}
	}

	// return) 
	return true;
}

#if defined(_WIN32)
bool archive::file::process_write_async(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(_buffer.size() == 0, 0);

	// 설명) 이미 Writing중인지를 확인한다.
	scoped_lock(this->m_lockable_buffer)
	{
		// 1) send중이면 queuing만 하고 끝낸다.
		if(this->m_buffer_sended.size() != 0)
		{
			// - add new buffer
			if(this->m_list_buffer_queuing.empty() || (FILE_WRITE_BUFFER_SIZE - this->m_list_buffer_queuing.back().size()) < _buffer.size())
			{
				this->m_list_buffer_queuing.emplace_back(alloc_shared_buffer(FILE_WRITE_BUFFER_SIZE));
			}

			// - append buffer
			this->m_list_buffer_queuing.back().append(_buffer.size(), _buffer.data());

			// return) 
			return _buffer.size();
		}

		// 2) sended buffer에 저장후 전송한다.
		this->m_buffer_sended = _buffer;
	}

	// 1) open file if instant open is 'true'
	if (this->m_instant_open == true)
	{
		// check)
		CGASSERT_ERROR(this->m_handle_file == INVALID_FILE_HANDLE)

		// - open file
		this->m_handle_file = api::file::open(this->m_file_name, this->m_file_operation, this->m_file_location);

		// check) 
		if (this->m_handle_file == INVALID_FILE_HANDLE)
		{
			// log) 
			LOG_DEBUG << "(err ) CGDK.system.object: fail to open file file " << this->m_file_name << " (error message: " << api::get_last_error_string() << ")";

			// return)
			return false;
		}
	}

	// 2) write
	auto writed = this->request_process_write_async(_buffer);

	// 3) write한만큼 이동
	_buffer += offset(writed);

	// return) 
	return true;
}

size_t archive::file::request_process_write_async(shared_buffer _buf_write)
{
	// 1) completor를 add_ref한다.
	this->add_ref();

	// 2) flag 설정
	this->m_is_write = true;

	// declare) 
	uint32_t dwBytes = 0;

	// 3) overlapped로 File에 write한다.
	auto result = ::WriteFile(this->m_handle_file, _buf_write.data(), static_cast<DWORD>(_buf_write.size()), reinterpret_cast<LPDWORD>(&dwBytes), static_cast<LPOVERLAPPED>(this));

	// check) Error가 발생했고 그것이 IO Pending이 아니라면 Error가 발생한 것이다.
	if(result == FALSE && ::GetLastError() != ERROR_IO_PENDING)
	{
		// - Release한다.
		this->release();

		// trace) 
		LOG_DEBUG << "(err ) fail to writing to file[position: " << get_position() << ", size: " << _buf_write.size() << "]";
	}

	// return)
	return _buf_write.size();
}

intptr_t archive::file::process_execute([[maybe_unused]] intptr_t _return, size_t _param)
{
	// check)
	CGASSERT_ERROR(/*_return == 0 ||*/ this->m_buffer_sended.size() == _param)

	shared_buffer temp_buffer = this->m_buffer_sended ^ _param;

	// 1) offset을 증가시킨다.
	((size_t&)Pointer) += _param;

	// 2) write일 경우 증가시킨다.
	if (this->m_is_write)
	{
		this->m_file_length += _param;
	}

	// 3) call on_send_complete
	this->on_write_complete(temp_buffer);

	// 4) queueing된 것을 확인한다.
	scoped_lock(this->m_lockable_buffer)
	{
		// check) queuing된 것이 없으면 그냥 끝낸다.
		if (this->m_list_buffer_queuing.empty())
		{
			// - Sended buffer를 clear한다.
			this->m_buffer_sended.clear();

			// - close file if instant open is 'true'
			if (this->m_instant_open == true)
			{
				if(this->m_handle_file != INVALID_FILE_HANDLE)
				{
					// - close file
					api::file::close(this->m_handle_file);

					// - open file
					this->m_handle_file = INVALID_FILE_HANDLE;
				}

				// check) 
				return false;
			}

			// return) 
			return 0;
		}

		// 3) Queuing된 것을 하나 끄집어 낸다.
		this->m_buffer_sended	 = this->m_list_buffer_queuing.front();
		this->m_list_buffer_queuing.pop_front();
	}

	// 5) queuing된 것이 있으면 처리한다.
	this->request_process_write_async(this->m_buffer_sended);

	// return)
	return 0;
}

#elif defined(__ANDROID__) || defined(__linux__)
bool archive::file::process_write_async(shared_buffer& _buffer)
{
	// check)
	RETURN_IF(_buffer.size() == 0, true);

	// check)
	CGASSERT_ERROR(_buffer.data() != nullptr);

	scoped_lock(this->m_lockable_buffer)
	{
		// 1) queueing
		if(m_processing == true)
		{
			// - add new buffer
			if(this->m_list_buffer_queuing.empty() || (FILE_WRITE_BUFFER_SIZE - this->m_list_buffer_queuing.back().size()) < _buffer.size())
			{
				this->m_list_buffer_queuing.emplace_back(alloc_shared_buffer(FILE_WRITE_BUFFER_SIZE));
			}

			// - append buffer
			this->m_list_buffer_queuing.back().append(_buffer.size(), _buffer.data());

			// - write한만큼 이동
			_buffer += offset(_buffer.size());

			// return) 
			return true;
		}

		// 2) queuing
		this->m_buffer_sended = _buffer;

		// 3) set flag
		this->m_processing = true;
	}

	// 3) open file 
	{
		// - open file
		auto result = this->_process_open_file_handle();

		// check)
		RETURN_IF(result == false, false);
	}

	// 4) post execute
	{
		// - requeest post execute
		auto result = m_pexecutor->post(this);

		// check)
		RETURN_IF(result == false, false);
	}

	// 5) write한만큼 이동
	_buffer += offset(_buffer.size());

	// return) 
	return true;
}

intptr_t archive::file::process_execute([[maybe_unused]] intptr_t _return, [[maybe_unused]] size_t _param)
{
	// check)
	RETURN_IF(this->m_handle_file == INVALID_FILE_HANDLE, 0);

	try
	{
		// 1) sended buffer부터 먼저 전송처리한다.
		if(this->m_buffer_sended.size() != 0)
		{
			// - seend buffer
			this->_process_send_buffer(this->m_buffer_sended);

			// - reeset
			this->m_buffer_sended.clear();
		}

		// 2) queued buffer list를 전송처리한다.
		scoped_lock(this->m_lockable_buffer)
		{
			// check)
			if (this->m_list_buffer_queuing.empty())
			{
				// - close file handle
				this->_process_close_file_handle();

				// - reset flag
				this->m_processing = false;

				// return)
				return 0;
			}

			// - m_list_buffer_processing와 m_list_buffer_queuing를 교환한다.
			this->m_list_buffer_processing.swap(this->m_list_buffer_queuing);

			// check)
			CGASSERT_ERROR(this->m_list_buffer_queuing.empty());
		}

		// 3) send queued buffer
		for (auto& iter : this->m_list_buffer_processing)
		{
			// - seend buffer
			this->_process_send_buffer(iter);

			// - call on_send_complete
			this->on_write_complete(iter);
		}

		// 4) clear 'm_list_buffer_processing'
		this->m_list_buffer_processing.clear();

		// 5) queueing된 것을 확인한다.
		scoped_lock(this->m_lockable_buffer)
		{
			// check)
			if (this->m_list_buffer_queuing.empty())
			{
				// - close file handle
				this->_process_close_file_handle();

				// - reset flag
				this->m_processing = false;

				// return) 
				return 0;
			}
		}

		// 5) post execute
		auto result = this->m_pexecutor->post(this);

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL_TO_OPERATION));
	}
	catch (...)
	{
		// - close file handle
		this->_process_close_file_handle();

		// - reset flag
		this->m_processing = false;
	}

	// return) 
	return 0;
}

bool archive::file::_process_open_file_handle()
{
	// check) 
	RETURN_IF(this->m_instant_open == false, true);

	// check)
	CGASSERT_ERROR(this->m_handle_file == INVALID_FILE_HANDLE)

	// - open file
	this->m_handle_file = api::file::open(this->m_file_name, this->m_file_operation, this->m_file_location);

	// check) 
	RETURN_IF(this->m_handle_file == INVALID_FILE_HANDLE, false);

	// return)
	return true;
}

void archive::file::_process_close_file_handle()
{
	// check) close file if instant open is 'true'
	RETURN_IF(this->m_instant_open == false);

	// check) 
	RETURN_IF(this->m_handle_file == INVALID_FILE_HANDLE);

	// 1) close file
	api::file::close(this->m_handle_file);

	// 2) reset file handle
	this->m_handle_file = INVALID_FILE_HANDLE;
}

void archive::file::_process_send_buffer(buffer_view _buf_processing)
{
	while (_buf_processing.size() != 0)
	{
		// - write
		auto result_size = fwrite(_buf_processing.data(), 1, _buf_processing.size(), this->m_handle_file);

		// check) 
		THROW_IF(result_size == 0, throwable::failure(eRESULT::FAIL_TO_OPERATION));

		// - process
		_buf_processing += offset(result_size);
	}
}

#else
	#error "unsupported API"
#endif


}
