#include "pch.h"

//using namespace CGDK;
//
//#include <fcntl.h>
//
//#define	MAX_FILE_BUFFER		MEMORYPOOL_MAX_SIZE_OF_MEMORY_BLOCK
//#define	MAX_WRITING_RETRY	8
//
//#if defined(__linux__)
//	#define	IO_SIGNAL		SIGUSR1
//#endif
//
//#elif defined(__linux__)
//void _process_signal_handler(int /*_signal*/, siginfo_t* _signal_info, void* /*_context*/)
//{
//	// check) ASYNC IO로 인한 signal일 경우만 처리한다.
//	RETURN_IF(_signal_info->si_code != SI_ASYNCIO);
//
//	// check) IO_SIGNAL일 경우만 처리한다.
//	RETURN_IF(_signal_info->si_signo != IO_SIGNAL);
//
//	// 1) Casting한다.
//	auto pexecutable = (Iexecutable*)_signal_info->si_value.sival_ptr;
//
//	// check) 
//	CGASSERT_ERROR(pexecutable != nullptr);
//
//	// 2) 처리한다.
//	pexecutable->process_execute((_signal_info->si_errno == 0) ? 1 : 0, 0);
//
//	// - Release
//	pexecutable->release();
//}
//
//#else
//
//
//archive::file_record::file_record() :
//	m_writing_count{ 0 },
//#if defined(_WIN32)
//	m_handle_file(INVALID_FILE_HANDLE)
//#elif defined(__linux__)
//	m_handle_file(0)
//#else
//	#error	"[CGDK10] Not supported Platform"
//#endif
//
//{
//#if defined(__linux__)
//	bzero(&this->m_aio_control_block, sizeof(aiocb));
//#endif
//
//}
//
//archive::file_record::~file_record() noexcept
//{
//}
//
//result_code archive::file_record::start(std::string_view _filename, bool _is_create_new)
//{
//	// declare)
//	context context_setting;
//
//	// 1) parameter를 setting한다.
//	context_setting["filename"sv] = (std::string_view)_filename;
//	context_setting["create_new"sv] = _is_create_new;
//
//	// 2)  start한다.
//	return this->start(context_setting);
//}
//
//result_code archive::file_record::_process_initializing(context& _context)
//{
//	// 1) call 'on_initializing'
//	this->on_initializing(_context);
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_initialize(context& _context)
//{
//	// 1) call 'on_initialize'
//	this->on_initialize(_context);
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_destroying() noexcept
//{
//	// 1) call 'on_destroying'
//	this->on_destroying();
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//result_code archive::file_record::_process_destroy() noexcept
//{
//	// 1) call 'on_destroy'
//	this->on_destroy();
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_starting(context& _context)
//{
//	// 1) call 'on_starting'
//	this->on_starting(_context);
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_start(context& _context)
//{
//	// declare) 
//	std::string	filename;
//	bool is_create_new = false;
//
//	// 1) filename을 읽어온다.
//	{
//		// declare) 
//		std::string str_filename;
//
//		// - "filename"키에 대응하는 값을 얻는다.
//		str_filename = (std::string)_context["filename"sv];
//
//		// - "filename"키에 대응하는 값을 얻었다면...
//		if (str_filename.empty() == false)
//		{
//			// - Filename을 설정한다.
//			filename = str_filename;
//
//			LOG_INFO << "  + parameter['filename']: "sv << filename;
//		}
//	}
//
//	// 2) new을 읽어온다.
//	{
//		// declare) 
//		std::string str_option;
//
//		// - "create_new"키에 대응하는 값을 얻는다.
//		str_option = (std::string)_context.find("create_new"sv, "create new"sv, "createnew"sv);
//
//		// - "create_new"키에 대응하는 값을 얻었다면...
//		if (str_option.empty() == false)
//		{
//			// - IPv6로 설정한다.
//			if (CGDK::__stricmp(str_option, "yes") == 0
//			 || CGDK::__stricmp(str_option, "true") == 0)
//			{
//				is_create_new = true;
//
//				// trace)
//				LOG_INFO << "  + parameter['create_new']: true"sv;
//			}
//		}
//	}
//	
//	// check) filename 이름이 nullptr이면 안 된다.
//	THROW_IF(filename.empty(), throwable::failure(), LOG_ERROR << "  (err ) filename is not assigned"sv);
//
//	// 2) File을 open처리한다.
//	SCOPED_LOCK(this->m_lockable_buffer)
//	{
//		// - create directory
//		{
//			// declare)
//			std::filesystem::path temp_path(filename);
//
//			// - create directory
//			std::filesystem::create_directories(temp_path.parent_path());
//		}
//
//	#if defined(_WIN32)
//		// declare) 
//		FILE_HANDLE	file_handle;
//
//		// declare) 
//		DWORD file_open_flag = (is_create_new == false) ? OPEN_ALWAYS : CREATE_ALWAYS;
//
//		// - file을 open한다. (open Always로 한다.)
//		file_handle = ::CreateFileA(filename.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, file_open_flag, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
//
//		// check) file 열기에 실패했다면...
//		THROW_IF(file_handle == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN), LOG_ERROR << "  (err ) fail to open file ('"sv << filename << "')"sv);
//
//		// - 열었던 File이 있다면 닫고 정리한다.
//		_close_file(file_handle);
//
//		// - Invalid Handle로 설정한다.
//		this->m_handle_file = INVALID_FILE_HANDLE;
//
//		// - open한 File을 setting하고 다른 변수들을 초기화한다.
//		this->m_queue_buffer.push_back(alloc_shared_buffer(MAX_FILE_BUFFER));
//
//		// - FIle 이름을 복사한다.
//		this->m_file_name = filename;
//
//	#elif defined(__linux__)
//		// declare) 
//		int	file_handle;
//
//		// declare) 
//		int	file_open_flag = (is_create_new == false) ? (O_CREAT | O_APPEND) : (O_CREAT | O_TRUNC);
//
//		// 1) File을 open한다. (open Always로 한다.)
//		file_handle = ::open(filename.c_str(), file_open_flag, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//
//		// check) 파일 열기에 실패했다면...
//		THROW_IF(file_handle < 0, throwable::failure(eRESULT::FAIL_TO_OPEN), LOG_ERROR << "  (err ) fail to open file ('"sv << filename << "')"sv);
//
//		// - 열었던 File이 있다면 닫고 정리한다.
//		_close_file(file_handle);
//
//		// - Invalid Handle로 설정한다.
//		this->m_handle_file = 0;
//
//		// - open한 File을 setting하고 다른 변수들을 초기화한다.
//		this->m_queue_buffer.push_back(alloc_shared_buffer(MAX_FILE_BUFFER));
//
//		// - FIle 이름을 복사한다.
//		this->m_file_name = filename;
//
//		// declare)
//		struct sigaction sa;
//
//		sa.sa_handler = nullptr;
//		sa.sa_sigaction = _process_signal_handler;
//		sigemptyset(&sa.sa_mask);
//		sa.sa_flags = SA_RESTART | SA_SIGINFO;
//
//		// - signal을 등록한다.
//		auto result = sigaction(IO_SIGNAL, &sa, NULL);
//
//		// check) 파일 열기에 실패했다면...
//		THROW_IF(result < 0, throwable::failure(eRESULT::FAIL_TO_OPEN), LOG_ERROR << "  (err ) fail to open file ('"sv << filename << "')"sv);
//
//	#else
//			#error	"[CGDK10] Not supported Platform"
//	#endif
//	}
//
//	// 2) 
//	this->on_start(_context);
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_stopping() noexcept
//{
//#if defined(_WIN32)
//	// check) File 이름이 null이면 안 된다.
//	if(this->m_handle_file!= INVALID_FILE_HANDLE && m_file_name.empty() == false)
//	{
//		// 1) filename을 clear
//		this->m_file_name.clear();
//
//		// 2) 다 저장할 때까지 기다린다.(최대 100초간)
//		for(int i=0; i<10000; ++i)
//		{
//			// check) 
//			SCOPED_LOCK(this->m_lockable_buffer)
//			{
//				BREAK_IF(this->m_queue_buffer.front().size() == 0 && this->m_handle_file == INVALID_FILE_HANDLE);
//			}
//
//			// - 10ms동안 Sleep한다.
//			std::this_thread::sleep_for(1ms);
//		}
//
//		// 3) clear~
//		SCOPED_LOCK(this->m_lockable_buffer)
//		{
//			this->m_queue_buffer.clear();
//		}
//
//		// check) HANDL은 반드시 INVALID_FILE_HANDLE여야 한다.
//		CGASSERT_ERROR(this->m_handle_file == INVALID_FILE_HANDLE);
//	}
//#elif defined(__linux__)
//	// check) File 이름이 null이면 안 된다.
//	if(this->m_handle_file!= 0 && this->m_file_name.empty() == false)
//	{
//		// 1) 먼저 이름을 NULL로한다.
//		this->m_file_name.clear();
//
//		// 2) 다 저장할 때까지 기다린다.(최대 100초간)
//		for(int i=0; i<100000; ++i)
//		{
//			// check) 
//			SCOPED_LOCK(this->m_lockable_buffer)
//			{
//				BREAK_IF(this->m_queue_buffer.front().size() == 0 && this->m_handle_file == 0);
//			}
//
//			// - 10ms동안 Sleep한다.
//			std::this_thread::sleep_for(1ms);
//		}
//
//		// 3) clear~
//		SCOPED_LOCK(this->m_lockable_buffer)
//		{
//			this->m_queue_buffer.clear();
//		}
//
//		// check) HANDL은 반드시 0여야 한다.
//		CGASSERT_ERROR(this->m_handle_file == 0);
//	}
//#else
//		#error "[CGDK10] Not supported Platform"
//#endif
//
//	try
//	{
//		// 1) call 'on_stopping'
//		this->on_stopping();
//	}
//	catch (...)
//	{
//		CGASSERT_ERROR(false);
//	}
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code archive::file_record::_process_stop() noexcept
//{
//	try
//	{
//		// 1) call 'on_stop'
//		this->on_stop();
//	}
//	catch (...)
//	{
//		CGASSERT_ERROR(false);
//	}
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//bool archive::file_record::process_write(shared_buffer& _buffer)
//{
//	// check)  buffer.size()가 0Byte면 쓰지 않는다.
//	RETURN_IF(_buffer.size() == 0, true);
//
//	// check) buffer.data()가 nullptr이면 그냥 끝낸다.
//	RETURN_IF(_buffer.data() == nullptr, false);
//
//	// check) File을 열지 않았으면 끝낸다.
//	RETURN_IF(this->m_file_name.empty(), false);
//
//	SCOPED_LOCK(this->m_lockable_buffer)
//	{
//		// check) buffer가 완전히 비어 있으면 그냥 끝낸다.
//		RETURN_IF(this->m_queue_buffer.empty(), false);
//
//		if(this->m_buffer_writing.size() != 0)
//		{
//			// check) 
//			if(this->m_queue_buffer.back().size() + _buffer.size() >= MAX_FILE_BUFFER)
//			{
//				// - Queuing한다.
//				this->m_queue_buffer.push_back(alloc_shared_buffer(MAX_FILE_BUFFER));
//			}
//
//			// - 마지막 buffer를 얻는다.
//			shared_buffer& temp_buffer = this->m_queue_buffer.back();
//
//			// - Data를 push_back한다.
//			temp_buffer.append(_buffer.size(), _buffer.data());
//
//			// return) 끝낸다.
//			return true;
//		}
//
//		// 2) Writing Buffer를 설정한다.
//		this->m_buffer_writing = _buffer;
//		this->m_writing_count = 0;
//	}
//
//	try
//	{
//	#if defined(_WIN32)
//		// 2) File을 open한다. (open Always로 한다.)
//		this->m_handle_file = CreateFileA(m_file_name.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
//
//		// check) 파일을 열지 못했으면 끝낸다.
//		THROW_IF(this->m_handle_file == INVALID_FILE_HANDLE, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to open file"sv), LOG_ERROR << "(err ) CGDK.system.object: fail to open log file '{"sv << m_file_name << "}'"sv);
//
//		// declare) 
//		int result;
//
//		// 3) executor를 만든다.
//		auto pexecutor = system_executor::executor();
//
//		// 4) executor를 설정한다.(생성하면 기본 executor로 설정한다.)
//		result = pexecutor->attach(this->m_handle_file);
//
//		// check) result가 FALSE이면 Bind 실패다!!!
//		THROW_IF(result == FALSE, throwable::failure(eRESULT::FAIL ,0 ,"fail to attach to executor "sv), LOG_ERROR << "(err ) CGDK.system.object: fail to attaching to executor"sv);
//
//		// 5) Overlapped Structure를 재설정한다.
//		OVERLAPPED*	temp_overlapped	 = (OVERLAPPED*)this;
//
//		// 6) Overlapped를 초기화한다.
//		temp_overlapped->Offset = UINT_MAX;
//		temp_overlapped->OffsetHigh = UINT_MAX;
//
//		// 7) Reference Count를 1 증가시킨다.
//		this->add_ref();
//
//		// 8) write한다.
//		result = WriteFile(this->m_handle_file, _buffer.data(), static_cast<DWORD>(_buffer.size()), nullptr, temp_overlapped);
//
//		// check) 실패했을 경우 파일을 닫고 끝낸다.
//		THROW_IF(result ==FALSE && ::GetLastError()!= ERROR_IO_PENDING, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write to file!! "sv), this->release());
//
//	#elif defined(__linux__)
//		// 2) File을 open한다. (open Always로 한다.)
//		this->m_handle_file = ::open(m_file_name.c_str(), O_WRONLY | O_APPEND);
//
//		// check) 파일을 열지 못했으면 끝낸다.
//		THROW_IF(this->m_handle_file < 0, throwable::failure(eRESULT::FAIL_TO_OPEN, 0, "fail to open file "sv), LOG_ERROR << "(err ) CGDK.system.log: fail to open log file '"sv << m_file_name << "'"sv);
//
//		// 3) aoi 정보를 작성한다.
//		this->m_aio_control_block.aio_fildes						 = this->m_handle_file;	// int
//		this->m_aio_control_block.aio_offset						 = 0;				// off_t (append이므로 0임. O_APPEND 옵션을 설정해 파일이 open함.)
//		this->m_aio_control_block.aio_buf							 = _buffer.data();	// volatile void*
//		this->m_aio_control_block.aio_nbytes						 = _buffer.size();	// size_t
//		this->m_aio_control_block.aio_reqprio						 = 0;				// int
//		this->m_aio_control_block.aio_sigevent.sigev_notify			 = SIGEV_SIGNAL;
//		this->m_aio_control_block.aio_sigevent.sigev_signo			 = IO_SIGNAL;
//		this->m_aio_control_block.aio_sigevent.sigev_value.sival_ptr = (Iexecutable*)this;
//		this->m_aio_control_block.aio_lio_opcode					 = LIO_WRITE;		// int
//		
//		// 4) Reference Count를 1 증가시킨다.
//		this->add_ref();
//
//		// 5) write를 한다.
//		auto result = ::aio_write(&this->m_aio_control_block);
//
//		// check) 실패했을 경우...
//		if (result < 0)
//		{
//			// log) 
//			LOG_ERROR << "(err ) file_record['" << name() << "'] fail to write [error_code: "sv << api::get_last_error() << "]"sv;
//
//			// - Release
//			release();
//
//			// throw)
//			throw throwable::failure(eRESULT::FAIL);
//		}
//	#else
//			#error "[CGDK10] Not supported Platform"
//	#endif
//	}
//	catch(...)
//	{
//		// - File을 닫는다.
//		_close_file(this->m_handle_file);
//
//		// - Buffer를 free한다.
//		this->m_buffer_writing.clear();
//
//		// return) false를 리턴한다.
//		return false;
//	}
//
//	// return) 
//	return true;
//}
//
//bool archive::file_record::set_filename(const std::string_view _filename)
//{
//#if defined(_WIN32)
//	// check) 기존의 파일이름과 같은지 확인한다.
//	RETURN_IF(this->m_file_name == _filename, true);
//
//	// 2) File을 열어 본다. (open Always로 한다.)
//	auto hFile = ::CreateFileA(_filename.data(), FILE_GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
//
//	// check) 파일을 열지 못하면 끝낸다.
//	RETURN_IF(hFile == INVALID_FILE_HANDLE, false);
//
//	// 3) File을 닫는다.
//	_close_file(hFile);
//
//	// 4) Filename을 복사해 넣는다.
//	this->m_file_name = _filename;
//#elif defined(__linux__)
//	// check) 기존의 파일이름과 같은지 확인한다.
//	RETURN_IF(this->m_file_name == _filename, true);
//
//	// 2) File을 open한다. (open Always로 한다.)
//	auto hFile = ::open(m_file_name.c_str(), O_WRONLY | O_APPEND);
//
//	// check) 파일 열기에 실패했다면...
//	THROW_IF(hFile < 0, throwable::failure(eRESULT::FAIL_TO_OPEN), LOG_ERROR << "  (err ) fail to open file ('"sv << m_file_name << "')"sv);
//
//	// 3) File을 바로 닫는다.
//	::close(hFile);
//
//	// 4) Filename을 복사해 넣는다.
//	this->m_file_name = _filename;
//
//	// Temp) 
//	_close_file(this->m_handle_file);
//#else
//	#error	"[CGDK10] Not supported Platform"
//#endif 
//
//	// return)
//	return true;
//}
//
//intptr_t archive::file_record::process_execute(intptr_t _return, size_t _param)
//{
//	try
//	{
//		//-----------------------------------------------------------------
//		// 1. 계속 저장될 내용이 있는지 확인하고 없으면 종료한다.
//		//    만약 저장에 실패했을 경우 다시 저장을 시도한다.(최대 MAX_WRITING_RETRY번)
//		//-----------------------------------------------------------------
//		// check)
//		CGASSERT_ERROR(_return == 0 || this->m_buffer_writing.size() == _param)
//
//		// 1) shared_buffer를 작성.
//		shared_buffer temp_buffer = this->m_buffer_writing ^ _param;
//
//		// 2) complete 처리를 한다.
//		this->on_write_complete(temp_buffer);
//
//		SCOPED_LOCK(this->m_lockable_buffer)
//		{
//			// 3.A) 쓰기에 성공했을 경우! (다음 것을 쓰거나 종료한다.)
//			if(_return != 0)
//			{
//				// 3) 제일 앞의 shared_buffer를 얻는다.
//				this->m_buffer_writing = this->m_queue_buffer.front();
//				this->m_writing_count	= 0;
//
//				// 4) buffering된 것이 하나도 없다면 그냥 파일을 닫고 끝낸다.
//				if(this->m_buffer_writing.size() == 0)
//				{
//					// - file을 닫는다.
//					_close_file(this->m_handle_file);
//
//					// return) S_FALSE
//					return intptr_t(1);
//				}
//
//				// 5) buffer가 비었다면 새 버퍼를 생성해 추가한다.
//				if(this->m_queue_buffer.size() == 1)
//				{
//					// - get front
//					this->m_queue_buffer.front() = alloc_shared_buffer(MAX_FILE_BUFFER);
//
//					// check)
//					CGASSERT_ERROR((this->m_buffer_writing.size() == 0));
//				}
//				else
//				{
//					this->m_queue_buffer.pop_front();
//				}
//			}
//			// 3.B) 쓰기에 실패했을 경우 (다시 한번 시도한다.)
//			else
//			{
//				// - 재시도횟수를 증가시킨다.
//				++this->m_writing_count;
//
//				// check)
//				CGASSERT_ERROR(this->m_writing_count < MAX_WRITING_RETRY);
//
//				// check) 
//				THROW_IF(this->m_writing_count >= MAX_WRITING_RETRY, throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write to file!! "sv));
//			}
//		}
//
//
//		//-----------------------------------------------------------------
//		// 2. Buffer에 있던 데이터들의 write를 수행한다.
//		//-----------------------------------------------------------------
//	#if defined(_WIN32)
//		// 1) Overlapped Structure를 재설정한다.
//		OVERLAPPED*	temp_overlapped = (OVERLAPPED*)this;
//
//		// 2) Overlapped를 초기화한다.
//		temp_overlapped->Offset = UINT_MAX;
//		temp_overlapped->OffsetHigh = UINT_MAX;
//
//		// 3) Reference Count를 1 증가시킨다.
//		this->add_ref();
//
//		// 4) Buffing된 Data를 write한다.
//		auto result = WriteFile(this->m_handle_file, this->m_buffer_writing.data(), static_cast<DWORD>(this->m_buffer_writing.size()), nullptr, temp_overlapped);
//
//		// check) 실패했을 경우 파일을 닫고 끝낸다.
//		if (result == FALSE && ::GetLastError() != ERROR_IO_PENDING)
//		{
//			// - rollback
//			this->release();
//
//			// throw) 
//			throw throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write to file!! "sv);
//		}
//	#elif defined(__linux__)
//		// 1) aoi 정보를 재설정한다.
//		this->m_aio_control_block.aio_fildes						 = this->m_handle_file;				// int
//		this->m_aio_control_block.aio_offset						 = 0;								// off_t (append이므로 0임. O_APPEND 옵션을 설정해 파일이 open함.)
//		this->m_aio_control_block.aio_buf							 = this->m_buffer_writing.data();	// volatile void*
//		this->m_aio_control_block.aio_nbytes						 = this->m_buffer_writing.size();	// size_t
//		this->m_aio_control_block.aio_reqprio						 = 0;								// int
//		this->m_aio_control_block.aio_sigevent.sigev_notify			 = SIGEV_SIGNAL;
//		this->m_aio_control_block.aio_sigevent.sigev_signo			 = IO_SIGNAL;
//		this->m_aio_control_block.aio_sigevent.sigev_value.sival_ptr = (Iexecutable*)this;
//		this->m_aio_control_block.aio_lio_opcode					 = LIO_WRITE;						// int
//
//		// 2) reference count를 1 증가시킨다.
//		add_ref();
//
//		// 3) write를 한다.
//		auto result = ::aio_write(&this->m_aio_control_block);
//
//		// check) 실패했을 경우 파일을 닫고 끝낸다.
//		if (result < 0)
//		{
//			// - rollback
//			this->release();
//
//			// throw)
//			throw throwable::failure(eRESULT::FAIL_TO_OPERATION, 0, "fail to write to file!! "sv);
//		}
//	#else
//		#error	"[CGDK10] Not supported Platform"
//	#endif
//	}
//	catch(...)
//	{
//		// - File을 닫는다.
//		_close_file(this->m_handle_file);
//
//		// - Release
//		this->release();
//
//		// - Buffer를 free한다.
//		this->m_buffer_writing.clear();
//	}
//
//	// return) success
//	return intptr_t(0);
//}
//
//#if defined(_WIN32)
//void archive::file_record::_close_file(FILE_HANDLE& _file_handle) noexcept
//{
//	SAFE_CLOSEHANDLE(_file_handle);
//}
//#elif defined(__linux__)
//void archive::file_record::_close_file(int& _file_handle) noexcept
//{
//	if(_file_handle>0)
//	{
//		::close(_file_handle);
//		_file_handle = 0;
//	}
//}
//
//#else
//	#error	"[CGDK10] Not supported Platform"
//#endif
