#include "pch.h"
#include <cstring>
#include <charconv>

#if defined(_WIN32)
	#include <DbgHelp.h>
	#include <shellapi.h>
	#include <shlobj.h>
	#pragma comment(lib, "Dbghelp.lib")
#elif defined(__linux__)
	#include <execinfo.h>
	#include <signal.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
	#include <dlfcn.h>
	#include <cxxabi.h>
	#include <link.h>
	#include <fstream>
	#include <sstream>
	#include <ucontext.h>
	#include <sys/prctl.h>
#endif
	
using namespace CGDK;

#if defined(__linux__)
// runs gcore on this process
//  - Yama 'ptrace_scope = 1' : gcore (a child of this process) cannot attach to this process (its parent)
//    unless this process allows it => allowed only while gcore is running
static int _run_gcore(const std::string& _command) noexcept
{
	// 1) allow to be traced
	::prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);

	// 2) run gcore
	int result = ::system(_command.c_str());

	// 3) restore
	::prctl(PR_SET_PTRACER, 0, 0, 0, 0);

	// return)
	return result;
}
#endif

#if defined(_WIN32)
	#ifdef NDEBUG
		#define	DEFAULT_ENABLE_EXCEPTION_HANDLING	true
	#else
		#define	DEFAULT_ENABLE_EXCEPTION_HANDLING	false
	#endif 
#elif defined(__linux__)
	#define	DEFAULT_ENABLE_EXCEPTION_HANDLING	true
#else
	#error "[CGDK10] Not supported Platform"
#endif



dump_manager::dump_manager(eDUMP_CONTEXT _type_dump, size_t _max_dump_count) noexcept :
		object::Nnameable(),
		m_is_exception_handling(DEFAULT_ENABLE_EXCEPTION_HANDLING),
		m_count_dump_limit(_max_dump_count),
		m_count_dump_now(0),
		m_type_dump(_type_dump),
		m_str_dump_path(".")
{
}

dump_manager::~dump_manager() noexcept
{
}

result_code dump_manager::destroy() noexcept
{
	return result_code(eRESULT::SUCCESS);
}

std::string _generate_dump_filename(std::string_view _str_dump_path, size_t _count_dump_now, std::string_view _prefix)
{
	// 1) 현재의 시간을 얻는다.
	auto time_now = chrono::to_date_time(chrono::time::utc::now());

	// 2) file이름을 설정한다.
#if defined(_WIN32)
	return CGDK::stream_string(
		_str_dump_path, "/", _prefix,
		CGDK::stream_width(time_now.t_year, 4, '0'), "-",
		CGDK::stream_width(time_now.t_month, 2, '0'), "-",
		CGDK::stream_width(time_now.t_day, 2, '0'), "@",
		CGDK::stream_width(time_now.t_hour, 2, '0'), "-",
		CGDK::stream_width(time_now.t_minute, 2, '0'), "-",
		CGDK::stream_width(time_now.t_second, 2, '0'), "@",
		CGDK::stream_width(time_now.t_fraction, 7, '0'), "_",
		static_cast<int>(_count_dump_now),
		".dmp");
#elif defined(__linux__)
	return CGDK::stream_string(
		_str_dump_path, "/", _prefix,
		CGDK::stream_width(time_now.t_year, 4, '0'), "-",
		CGDK::stream_width(time_now.t_month, 2, '0'), "-",
		CGDK::stream_width(time_now.t_day, 2, '0'), "@",
		CGDK::stream_width(time_now.t_hour, 2, '0'), "-",
		CGDK::stream_width(time_now.t_minute, 2, '0'), "-",
		CGDK::stream_width(time_now.t_second, 2, '0'), "@",
		CGDK::stream_width(time_now.t_fraction, 7, '0'), "_",
		static_cast<int>(_count_dump_now));
#endif
}

std::string _generate_crush_dump_filename(std::string_view _str_dump_path, size_t _count_dump_now)
{
	return _generate_dump_filename(_str_dump_path, _count_dump_now, "crash_");
}

#if defined(_WIN32)
uint32_t _to_minidump_type(eDUMP_CONTEXT _type_dump) noexcept
{
	switch(_type_dump)
	{
	case	eDUMP_CONTEXT::DETAIL:
			return MiniDumpWithPrivateReadWriteMemory | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules;

	case	eDUMP_CONTEXT::FULL:
			return MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules;

	default:
			return MiniDumpNormal;
	}
}
#endif


#if defined(_WIN32)

// Windows용 스택 메모리 덤프
std::string _dump_stack_memory_win(void* stack_pointer,  void* /*context_ptr*/, size_t bytes = 512)
{
	if (!stack_pointer)
		return "";

	std::ostringstream ss;
	ss << "\n========== Stack Memory Dump ==========\n";
	ss << "Stack pointer: 0x" << std::hex << (uintptr_t)stack_pointer << "\n";
	ss << std::hex << std::uppercase << std::setfill('0');

	HANDLE process = GetCurrentProcess();
	ULONG64* stack = (ULONG64*)stack_pointer;
	size_t qwords = bytes / 8;

	for (size_t i = 0; i < qwords; i++)
	{
		uintptr_t addr = (uintptr_t)&stack[i];
		ULONG64 value = 0;

		// 안전하게 메모리 읽기
		if (IsBadReadPtr(&stack[i], sizeof(ULONG64)))
			break;

		value = stack[i];

		ss << "0x" << std::setw(16) << addr << ": 0x" << std::setw(16) << value;

		// 심볼 정보 조회
		DWORD64 dwDisplacement = 0;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(process, value, &dwDisplacement, pSymbol))
		{
			ss << "  <- " << pSymbol->Name;
			if (dwDisplacement != 0)
				ss << "+0x" << std::hex << dwDisplacement;
		}

		ss << "\n";

		if ((i + 1) % 4 == 0)
			ss << "\n";
	}

	return ss.str();
}

// Windows용 스택 프레임 분석
std::string _analyze_stack_frames_win(void* base_pointer, void* stack_pointer, HANDLE process)
{
	if (!base_pointer)
		return "";

	std::ostringstream ss;
	ss << "\n========== Stack Frame Analysis ==========\n";
	ss << std::hex << std::uppercase << std::setfill('0');

	struct StackFrame
	{
		StackFrame* rbp;
		void* rip;
	};

	StackFrame* frame = (StackFrame*)base_pointer;
	int frame_num = 0;

	while (frame && frame_num < 32)
	{
		// 안전하게 메모리 읽기
		if (IsBadReadPtr(frame, sizeof(StackFrame)))
			break;

		// 스택 범위 검증
		if ((uintptr_t)frame < (uintptr_t)stack_pointer || 
			(uintptr_t)frame > (uintptr_t)stack_pointer + 1024*1024)
			break;

		ss << "Frame #" << std::dec << frame_num << ":\n";
		ss << "  RBP: 0x" << std::hex << std::setw(16) << (uintptr_t)frame << "\n";
		ss << "  RIP: 0x" << std::setw(16) << (uintptr_t)frame->rip << "\n";

		// 로컬 변수 영역 덤프
		ss << "  Local variables area (RBP-64 to RBP):\n";
		ULONG64* locals = (ULONG64*)((char*)frame - 64);

		if (!IsBadReadPtr(locals, 64))
		{
			for (int i = 0; i < 8; i++)
			{
				ULONG64 value = locals[i];
				uintptr_t addr = (uintptr_t)&locals[i];

				ss << "    [RBP" << std::dec << std::showpos << ((i-8)*8) << std::noshowpos << "] ";
				ss << "0x" << std::hex << std::setw(16) << addr << ": 0x" << std::setw(16) << value;

				// 심볼 정보 확인
				DWORD64 dwDisplacement = 0;
				char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
				PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
				pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
				pSymbol->MaxNameLen = MAX_SYM_NAME;

				if (SymFromAddr(process, value, &dwDisplacement, pSymbol))
				{
					ss << "  <- " << pSymbol->Name;
				}
				else
				{
					// ASCII 문자열로 해석 가능한지 체크
					char* str = (char*)&value;
					bool is_printable = true;
					for (int j = 0; j < 8; j++)
					{
						if (str[j] != 0 && (str[j] < 32 || str[j] > 126))
						{
							is_printable = false;
							break;
						}
					}
					if (is_printable && str[0] != 0)
					{
						ss << "  (\"";
						for (int j = 0; j < 8 && str[j] != 0; j++)
							ss << str[j];
						ss << "\")";
					}
				}

				ss << "\n";
			}
		}

		ss << "\n";

		// 다음 프레임으로
		frame = frame->rbp;
		frame_num++;
	}

	return ss.str();
}

// Windows용 상세 callstack 로그
void _log_callstack_win(std::string_view _message, LPEXCEPTION_POINTERS _pexception)
{
	// 1) log record 생성
	auto plog_record = make_object<LOG_RECORD>();

	// 2) set log record
	plog_record->type = log_type_t::fatal;
	plog_record->level = log_level_t::higher();
	plog_record->result = eRESULT::EXCEPTION;
	plog_record->origin = log_origin_t{ 0 };
	plog_record->time_occurred = chrono::time::utc::now();

	// 3) log message
	plog_record->set_message<char>(_message);

	// 4) Initialize symbol handler
	HANDLE process = GetCurrentProcess();
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	if (!SymInitialize(process, NULL, TRUE))
	{
		plog_record->add_message<char>("Failed to initialize symbol handler");
		_log(DEFAULT_LOGGER, plog_record);
		return;
	}

	// 5) Context 정보가 있으면 스택 분석
	if (_pexception && _pexception->ContextRecord)
	{
		CONTEXT* context = _pexception->ContextRecord;

#if defined(_M_X64)
		void* stack_ptr = (void*)context->Rsp;
		void* base_ptr = (void*)context->Rbp;
#elif defined(_M_IX86)
		void* stack_ptr = (void*)context->Esp;
		void* base_ptr = (void*)context->Ebp;
#elif defined(_M_ARM64)
		void* stack_ptr = (void*)context->Sp;
		void* base_ptr = nullptr;
#else
		void* stack_ptr = nullptr;
		void* base_ptr = nullptr;
#endif

		// 스택 메모리 덤프
		if (stack_ptr)
		{
			std::string stack_dump = _dump_stack_memory_win(stack_ptr, context, 512);
			if (!stack_dump.empty())
				plog_record->add_message<char>(stack_dump);
		}

		// 스택 프레임 분석
		if (base_ptr)
		{
			std::string frame_analysis = _analyze_stack_frames_win(base_ptr, stack_ptr, process);
			if (!frame_analysis.empty())
				plog_record->add_message<char>(frame_analysis);
		}
	}

	// 6) Callstack 캡처
	const int MAX_FRAMES = 62; // StackWalk64의 제한
	void* stack[MAX_FRAMES];
	WORD frames = CaptureStackBackTrace(0, MAX_FRAMES, stack, NULL);

	// 7) 각 프레임 상세 정보 출력
	for (WORD i = 0; i < frames; i++)
	{
		std::stringstream frame_detail;

		DWORD64 address = (DWORD64)stack[i];

		frame_detail << "\n========== Frame #" << std::dec << i << " ==========\n";
		frame_detail << "Address    : 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << address << "\n";

		// 심볼 정보
		DWORD64 dwDisplacement = 0;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(process, address, &dwDisplacement, pSymbol))
		{
			frame_detail << "Symbol     : " << pSymbol->Name << "+0x" << std::hex << dwDisplacement << "\n";
			frame_detail << "Module     : 0x" << std::setw(16) << pSymbol->ModBase << "\n";
		}

		// 소스 파일 정보
		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD dwLineDisplacement = 0;

		if (SymGetLineFromAddr64(process, address, &dwLineDisplacement, &line))
		{
			frame_detail << "Source     :\n";
			frame_detail << "  at " << line.FileName << ":" << std::dec << line.LineNumber << "\n";

			// 소스 코드 읽기 시도 (FILE API 사용)
			FILE* file = nullptr;
			if (fopen_s(&file, line.FileName, "r") == 0 && file != nullptr)
			{
				char source_line[1024];
				int current_line = 1;
				int target_line = line.LineNumber;
				int context_lines = 2;

				while (fgets(source_line, sizeof(source_line), file) != nullptr && current_line <= target_line + context_lines)
				{
					// 개행 제거
					size_t len = strlen(source_line);
					if (len > 0 && source_line[len - 1] == '\n')
						source_line[len - 1] = '\0';

					if (current_line >= target_line - context_lines)
					{
						if (current_line == target_line)
							frame_detail << "  ==> " << current_line << ": " << source_line << "\n";
						else
							frame_detail << "      " << current_line << ": " << source_line << "\n";
					}
					current_line++;
				}

				fclose(file);
			}
		}

		plog_record->add_message<char>(frame_detail.str());
	}

	// 8) Cleanup
	SymCleanup(process);

	// 9) trace log
	_log(DEFAULT_LOGGER, plog_record);
}

bool dump_manager::create_dump(const std::string& _str_dump_file, LPEXCEPTION_POINTERS _pexception)
{
	// check) 
	if(_str_dump_file.empty())
		return false;

	// check) 
	if(_pexception == nullptr)
		return false;
	
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_map_dump);

		//-----------------------------------------------------------------
		// 1. 먼저 이미 발생한 적이 있는 Exception인지 확인한다.
		//-----------------------------------------------------------------
		// 1) 해당하는 주소를 찾는다.
		auto iter_find = this->m_map_dump.find(_pexception->ExceptionRecord->ExceptionAddress);

		// check) 이미 존재하는 것이라면 발생횟수만 증가시키고 끝낸다.
		if(iter_find != this->m_map_dump.end())
		{
			// - 발생횟수만 추가한다.
			++(iter_find->second.count_times);

			// - Dump를 추가한다.
			this->m_list_exception_event.push_back(_pexception->ExceptionRecord->ExceptionAddress);

			// return) false를 리턴한다.
			return false;
		}


		//-----------------------------------------------------------------
		// 2. Exception 기본 처리
		//-----------------------------------------------------------------
		// 1) Exception Count를 증가시킨다.
		++this->m_count_dump_now;

		// check) Exception최대수에 따라 처리한다. (m_count_dump_limit가 0이면 한계가 없다.)
		if(this->m_count_dump_limit > 0 && this->m_count_dump_now > this->m_count_dump_limit)
			return false;

		//-----------------------------------------------------------------
		// 3. Dump정보를 새로 추가한다.
		//-----------------------------------------------------------------
		// declare) 없다면 새로 추가한다.
		sMESSAGE_DUMP tempMessage;

		// 1) Dump Info와 Message를 설정한다.
		tempMessage.dump_info.thread_id			 = GetCurrentThreadId();
		tempMessage.dump_info.time_occurred_first	 = chrono::time::local::now();
		tempMessage.dump_info.time_occurred_last	 = tempMessage.dump_info.time_occurred_first;
		tempMessage.dump_info.count_times		 = 1;
		tempMessage.dump_info.sExceptionRecords	 = *_pexception->ExceptionRecord;
		tempMessage.dump_info.sContextRecord	 = *_pexception->ContextRecord;
		tempMessage.dump_info.dump_filename		 = _str_dump_file;

		// 2) Dump정보를 추가한다.
		this->m_map_dump.emplace(tempMessage.dump_info.sExceptionRecords.ExceptionAddress, tempMessage.dump_info);

		// 3) Dump를 Notify할 Message를 추가한다.
		this->m_list_exception_event.push_back(tempMessage.dump_info.sExceptionRecords.ExceptionAddress);
	}


	//-----------------------------------------------------------------
	// 4. File을 start한다.
	//-----------------------------------------------------------------
	// declare) 
	FILE_HANDLE hFile;

	// 1) File을 연다.
	api::file::create_parent_directories(_str_dump_file);
	hFile = CreateFileA(_str_dump_file.data(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// check) File을 열지 못했나?
	if(hFile==INVALID_CGHANDLE_VALUE)
		return false;


	//-----------------------------------------------------------------
	// 5. Dump를 저장한다.
	//-----------------------------------------------------------------
	// 1) Dump Input값을 설정한다.
	MINIDUMP_EXCEPTION_INFORMATION temp_dump_information;
	temp_dump_information.ThreadId = GetCurrentThreadId();
	temp_dump_information.ExceptionPointers = _pexception;
	temp_dump_information.ClientPointers = TRUE;

	// declare) 
	uint32_t dumptype;

	// 2) ...
	switch(m_type_dump)
	{
	case	eDUMP_CONTEXT::MINI:
			dumptype = MINIDUMP_TYPE(MiniDumpNormal);
			break;

	case	eDUMP_CONTEXT::DETAIL:
			dumptype = MiniDumpWithPrivateReadWriteMemory	// 읽거나 쓸 수 있는 메모리들(Stack/Heap/TLS)
					 | MiniDumpWithDataSegs					// 전역변수
					 | MiniDumpWithHandleData				// 핸들 정보(Socket, File, 등등...)
					 | MiniDumpWithFullMemoryInfo			// 가상 메모리 상의 레이아웃 정보
					 | MiniDumpWithThreadInfo				// 추가적인 Thread 정보 (Thread시간들, 시작주소 등등...)
					 | MiniDumpWithUnloadedModules			// 
					;
			break;

	case	eDUMP_CONTEXT::FULL:
			dumptype = MiniDumpWithFullMemory				// 모든 메모리읽거나 쓸 수 있는 메모리들(Stack/Heap/TLS)
					 | MiniDumpWithFullMemoryInfo			// 가상 메모리 상의 레이아웃 정보
					 | MiniDumpWithHandleData				// 핸들 정보(Socket, File, 등등...)
					 | MiniDumpWithThreadInfo				// 추가적인 Thread 정보 (Thread시간들, 시작주소 등등...)
					 | MiniDumpWithUnloadedModules			// 
					;
			break;

	default:
			dumptype = MINIDUMP_TYPE(MiniDumpNormal);
			break;
	}

	// 2) Dump한다.
	BOOL result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile , static_cast<MINIDUMP_TYPE>(dumptype), &temp_dump_information, NULL, NULL);

	// check) Mini Dump파일 저장에 실패했나?
	EXECUTE_IF(result == FALSE, LOG_FATAL << "(fatal) fail to writing dump on file[" << _str_dump_file << "]");

	// trace) 
	LOG_FATAL << "(fatal) CGDK.system.object: dump file created [" << _str_dump_file << "] ";


	//-----------------------------------------------------------------
	// 6. File을 닫는다.
	//-----------------------------------------------------------------
	// 1) File을 닫는다.
	CloseHandle(hFile);


	// return) 
	return true;
}

void dump_manager::translator_function(uint32_t, LPEXCEPTION_POINTERS _pexception)
{
	// 1) Dump를 뜬다.
	dump_manager::instance()->process_exception(_pexception);
}

void dump_manager::process_exception(LPEXCEPTION_POINTERS _pexception)
{
	// check)
	if(_pexception == nullptr)
		return;

	// check) enable상태가 아니면 설정을 하지 않는다.
	if(this->m_is_exception_handling == false)
		return;

	//-----------------------------------------------------------------
	// 1. Dump file을 생성한다.
	//-----------------------------------------------------------------
	// 1) Dump File을 생성한다.
	this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now), _pexception);

	//-----------------------------------------------------------------
	// 2. 상세 Callstack 로그 남기기
	//-----------------------------------------------------------------
	std::string exception_message = "Exception: ";
	if (_pexception && _pexception->ExceptionRecord)
	{
		DWORD code = _pexception->ExceptionRecord->ExceptionCode;
		switch (code)
		{
		case EXCEPTION_ACCESS_VIOLATION:         exception_message += "EXCEPTION_ACCESS_VIOLATION"; break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    exception_message += "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break;
		case EXCEPTION_BREAKPOINT:               exception_message += "EXCEPTION_BREAKPOINT"; break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:    exception_message += "EXCEPTION_DATATYPE_MISALIGNMENT"; break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:     exception_message += "EXCEPTION_FLT_DENORMAL_OPERAND"; break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       exception_message += "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break;
		case EXCEPTION_FLT_INEXACT_RESULT:       exception_message += "EXCEPTION_FLT_INEXACT_RESULT"; break;
		case EXCEPTION_FLT_INVALID_OPERATION:    exception_message += "EXCEPTION_FLT_INVALID_OPERATION"; break;
		case EXCEPTION_FLT_OVERFLOW:             exception_message += "EXCEPTION_FLT_OVERFLOW"; break;
		case EXCEPTION_FLT_STACK_CHECK:          exception_message += "EXCEPTION_FLT_STACK_CHECK"; break;
		case EXCEPTION_FLT_UNDERFLOW:            exception_message += "EXCEPTION_FLT_UNDERFLOW"; break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:      exception_message += "EXCEPTION_ILLEGAL_INSTRUCTION"; break;
		case EXCEPTION_IN_PAGE_ERROR:            exception_message += "EXCEPTION_IN_PAGE_ERROR"; break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       exception_message += "EXCEPTION_INT_DIVIDE_BY_ZERO"; break;
		case EXCEPTION_INT_OVERFLOW:             exception_message += "EXCEPTION_INT_OVERFLOW"; break;
		case EXCEPTION_INVALID_DISPOSITION:      exception_message += "EXCEPTION_INVALID_DISPOSITION"; break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: exception_message += "EXCEPTION_NONCONTINUABLE_EXCEPTION"; break;
		case EXCEPTION_PRIV_INSTRUCTION:         exception_message += "EXCEPTION_PRIV_INSTRUCTION"; break;
		case EXCEPTION_SINGLE_STEP:              exception_message += "EXCEPTION_SINGLE_STEP"; break;
		case EXCEPTION_STACK_OVERFLOW:           exception_message += "EXCEPTION_STACK_OVERFLOW"; break;
		default:
			exception_message += CGDK::stream_string("Code ", CGDK::stream_hex(code, 8, true, true));
			break;
		}

		if (code == EXCEPTION_ACCESS_VIOLATION && _pexception->ExceptionRecord->NumberParameters >= 2)
		{
			ULONG_PTR access_type = _pexception->ExceptionRecord->ExceptionInformation[0];
			ULONG_PTR address = _pexception->ExceptionRecord->ExceptionInformation[1];
			exception_message += CGDK::stream_string(" (",
				access_type == 0 ? "read " : access_type == 1 ? "write " : "",
				"access at ",
				CGDK::stream_hex(address, 16, true, true),
				")");
		}
	}

	_log_callstack_win(exception_message, _pexception);


	//-----------------------------------------------------------------
	// 3. Exception Throw
	//-----------------------------------------------------------------
	// declare) 
	throwable::system_exception rException;

	// 1) throw 객체를 작성한다.
#if defined(_WIN32)
	if (_pexception && _pexception->ExceptionRecord && _pexception->ContextRecord)
	{
		rException.m_exception_record = *_pexception->ExceptionRecord;
		rException.m_context_record = *_pexception->ContextRecord;
	}
#elif defined(__linux__)

#endif

	// ㅅhrow) 던진다!!!
	throw rException;
}

void dump_manager::process_invalid_parameter(const std::wstring_view /*_expression*/, const std::wstring_view /*_function*/, const std::wstring_view /*_file*/, unsigned int /*_line*/, uintptr_t /*_preserved*/)
{
	// Throw) 던진다!!!
	throw throwable::failure(eRESULT::INVALID_PARAMETER_HANDLE);
}

#elif defined(__linux__)

// 레지스터 정보 출력
std::string _dump_registers(ucontext_t* context)
{
	if (!context)
		return "";

	std::stringstream ss;
	ss << "\n========== CPU Registers ==========\n";

#if defined(__x86_64__)
	// x86_64 아키텍처
	auto& regs = context->uc_mcontext.gregs;
	ss << std::hex << std::uppercase;
	ss << "RAX: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RAX] << "  ";
	ss << "RBX: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RBX] << "\n";
	ss << "RCX: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RCX] << "  ";
	ss << "RDX: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RDX] << "\n";
	ss << "RSI: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RSI] << "  ";
	ss << "RDI: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RDI] << "\n";
	ss << "RBP: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RBP] << "  ";
	ss << "RSP: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RSP] << "\n";
	ss << "R8 : 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R8]  << "  ";
	ss << "R9 : 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R9]  << "\n";
	ss << "R10: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R10] << "  ";
	ss << "R11: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R11] << "\n";
	ss << "R12: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R12] << "  ";
	ss << "R13: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R13] << "\n";
	ss << "R14: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R14] << "  ";
	ss << "R15: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_R15] << "\n";
	ss << "RIP: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_RIP] << "  ";
	ss << "EFLAGS: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)regs[REG_EFL] << "\n";

	// Instruction pointer가 가리키는 곳의 메모리 내용
	ss << "\nInstruction bytes at RIP:\n  ";
	unsigned char* rip = (unsigned char*)regs[REG_RIP];
	for (int i = 0; i < 16; i++)
	{
		ss << std::setw(2) << std::setfill('0') << (int)rip[i] << " ";
		if ((i + 1) % 8 == 0)
			ss << "\n  ";
	}
	ss << "\n";

#elif defined(__i386__)
	// x86 32-bit 아키텍처
	auto& regs = context->uc_mcontext.gregs;
	ss << std::hex << std::uppercase;
	ss << "EAX: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EAX] << "  ";
	ss << "EBX: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EBX] << "\n";
	ss << "ECX: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_ECX] << "  ";
	ss << "EDX: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EDX] << "\n";
	ss << "ESI: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_ESI] << "  ";
	ss << "EDI: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EDI] << "\n";
	ss << "EBP: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EBP] << "  ";
	ss << "ESP: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_ESP] << "\n";
	ss << "EIP: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EIP] << "  ";
	ss << "EFLAGS: 0x" << std::setw(8) << std::setfill('0') << (uint32_t)regs[REG_EFL] << "\n";
#elif defined(__aarch64__)
	// ARM64 아키텍처
	ss << std::hex << std::uppercase;
	for (int i = 0; i < 31; i++)
	{
		ss << "X" << std::dec << i << ": 0x" << std::hex << std::setw(16) << std::setfill('0') 
		   << (uint64_t)context->uc_mcontext.regs[i];
		if (i % 2 == 1)
			ss << "\n";
		else
			ss << "  ";
	}
	ss << "SP: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)context->uc_mcontext.sp << "  ";
	ss << "PC: 0x" << std::setw(16) << std::setfill('0') << (uint64_t)context->uc_mcontext.pc << "\n";
#else
	ss << "Register dump not supported for this architecture\n";
#endif

	return ss.str();
}

// 스택 메모리 덤프
std::string _dump_stack_memory(void* stack_pointer, size_t bytes = 256)
{
	if (!stack_pointer)
		return "";

	std::stringstream ss;
	ss << "\n========== Stack Memory Dump ==========\n";
	ss << "Stack pointer: 0x" << std::hex << (uintptr_t)stack_pointer << "\n";
	ss << std::hex << std::uppercase << std::setfill('0');

	uint64_t* stack = (uint64_t*)stack_pointer;
	size_t qwords = bytes / 8;

	for (size_t i = 0; i < qwords; i++)
	{
		uintptr_t addr = (uintptr_t)&stack[i];
		ss << "0x" << std::setw(16) << addr << ": 0x" << std::setw(16) << stack[i];

		// 포인터처럼 보이는 값인지 확인 (dladdr로 체크)
		Dl_info info;
		if (dladdr((void*)stack[i], &info) && info.dli_sname)
		{
			int status;
			char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
			ss << "  <- " << ((demangled && status == 0) ? demangled : info.dli_sname);
			if (demangled)
				free(demangled);
		}

		ss << "\n";

		if ((i + 1) % 4 == 0)
			ss << "\n";
	}

	return ss.str();
}

// 스택 프레임 분석 (RBP 체인 추적)
std::string _analyze_stack_frames(void* base_pointer, void* stack_pointer)
{
	if (!base_pointer)
		return "";

	std::stringstream ss;
	ss << "\n========== Stack Frame Analysis ==========\n";
	ss << std::hex << std::uppercase << std::setfill('0');

	struct StackFrame
	{
		StackFrame* rbp;
		void* rip;
	};

	StackFrame* frame = (StackFrame*)base_pointer;
	int frame_num = 0;

	while (frame && frame_num < 32)
	{
		// 스택 범위 검증 (간단한 체크)
		if ((uintptr_t)frame < (uintptr_t)stack_pointer || 
			(uintptr_t)frame > (uintptr_t)stack_pointer + 1024*1024)
			break;

		ss << "Frame #" << std::dec << frame_num << ":\n";
		ss << "  RBP: 0x" << std::hex << std::setw(16) << (uintptr_t)frame << "\n";
		ss << "  RIP: 0x" << std::setw(16) << (uintptr_t)frame->rip << "\n";

		// 로컬 변수 영역 덤프 (RBP 기준 아래쪽)
		ss << "  Local variables area (RBP-64 to RBP):\n";
		uint64_t* locals = (uint64_t*)((char*)frame - 64);
		for (int i = 0; i < 8; i++)
		{
			uintptr_t addr = (uintptr_t)&locals[i];
			ss << "    [RBP" << std::dec << std::showpos << ((i-8)*8) << std::noshowpos << "] ";
			ss << "0x" << std::hex << std::setw(16) << addr << ": 0x" << std::setw(16) << locals[i];

			// 심볼 정보 확인
			Dl_info info;
			if (dladdr((void*)locals[i], &info) && info.dli_sname)
			{
				int status;
				char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
				ss << "  <- " << ((demangled && status == 0) ? demangled : info.dli_sname);
				if (demangled)
					free(demangled);
			}
			else
			{
				// ASCII로 해석 가능한지 체크
				char* str = (char*)&locals[i];
				bool is_printable = true;
				for (int j = 0; j < 8; j++)
				{
					if (str[j] != 0 && (str[j] < 32 || str[j] > 126))
					{
						is_printable = false;
						break;
					}
				}
				if (is_printable && str[0] != 0)
				{
					ss << "  (\"";
					for (int j = 0; j < 8 && str[j] != 0; j++)
						ss << str[j];
					ss << "\")";
				}
			}

			ss << "\n";
		}

		ss << "\n";

		// 다음 프레임으로
		frame = frame->rbp;
		frame_num++;
	}

	return ss.str();
}

// 소스 파일에서 특정 라인 주변 코드를 읽는 함수
std::string _read_source_lines(const char* filename, int line_number, int context_lines = 3)
{
	std::ifstream file(filename);
	if (!file.is_open())
		return "";

	std::stringstream result;
	std::string line;
	int current_line = 1;
	int start_line = std::max(1, line_number - context_lines);
	int end_line = line_number + context_lines;

	while (std::getline(file, line) && current_line <= end_line)
	{
		if (current_line >= start_line)
		{
			if (current_line == line_number)
				result << "  ==> " << current_line << ": " << line << "\n";
			else
				result << "      " << current_line << ": " << line << "\n";
		}
		current_line++;
	}

	return result.str();
}

// addr2line을 사용하여 인라인 함수를 포함한 모든 정보를 얻는 함수
struct SourceLocationInfo
{
	std::string function_name;
	std::string file_path;
	int line_number = 0;
	bool is_inline = false;
};

struct SymbolResolveResult
{
	std::vector<SourceLocationInfo> locations;
	std::string diagnostic;
};

std::string _shell_quote(const char* text)
{
	std::string result = "'";
	for (const char* p = text; p != nullptr && *p != '\0'; ++p)
	{
		if (*p == '\'')
			result += "'\\''";
		else
			result += *p;
	}
	result += "'";
	return result;
}

SymbolResolveResult _get_detailed_source_location(uintptr_t addr, const char* executable_path)
{
	SymbolResolveResult result;

	if (executable_path == nullptr || executable_path[0] == '\0')
	{
		result.diagnostic = "addr2line skipped: empty executable path";
		return result;
	}

	char addr_str[64];
	snprintf(addr_str, sizeof(addr_str), "0x%lx", (unsigned long)addr);

	// -i: 인라인 함수 포함, -f: 함수명, -C: demangle, -p: pretty print, -a: 주소 표시
	std::string command = "addr2line -e " + _shell_quote(executable_path) + " -i -f -C -a " + addr_str + " 2>&1";

	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe)
	{
		result.diagnostic = "addr2line popen failed: " + command;
		return result;
	}

	char line[512];
	bool first_entry = true;
	std::string raw_output;

	while (fgets(line, sizeof(line), pipe) != nullptr)
	{
		// 개행 제거
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';

		if (!raw_output.empty())
			raw_output += " | ";
		raw_output += line;

		// 주소 라인 스킵
		if (line[0] == '0' && line[1] == 'x')
			continue;

		SourceLocationInfo info;
		info.is_inline = !first_entry;

		// 함수명 라인
		if (std::string_view(line) != "??")
		{
			info.function_name = line;

			// 다음 라인에서 파일:라인 정보 읽기
			if (fgets(line, sizeof(line), pipe) != nullptr)
			{
				len = strlen(line);
				if (len > 0 && line[len - 1] == '\n')
					line[len - 1] = '\0';

				if (!raw_output.empty())
					raw_output += " | ";
				raw_output += line;

				// "file:line" 또는 "file:?" 형식 파싱
				if (std::string_view(line) != "??:0" && std::string_view(line) != "??:?")
				{
					const std::string_view str_line(line);
					const auto pos_colon = str_line.rfind(':');
					if (pos_colon != std::string_view::npos)
					{
						info.file_path = std::string(str_line.substr(0, pos_colon));
						info.line_number = 0;
						std::from_chars(str_line.data() + pos_colon + 1, str_line.data() + str_line.size(), info.line_number);

						result.locations.push_back(info);
					}
				}
			}
		}

		first_entry = false;
	}

	int exit_status = pclose(pipe);
	if (result.locations.empty())
	{
		std::stringstream ss;
		ss << "addr2line unresolved for " << executable_path << " " << addr_str;
		if (exit_status != -1)
		{
			if (WIFEXITED(exit_status))
				ss << " (exit=" << WEXITSTATUS(exit_status) << ")";
			else if (WIFSIGNALED(exit_status))
				ss << " (signal=" << WTERMSIG(exit_status) << ")";
		}
		if (!raw_output.empty())
			ss << ": " << raw_output;
		result.diagnostic = ss.str();
	}

	return result;
}

// 메모리 맵 정보 조회 (추가 디버깅 정보)
std::string _get_memory_map_info(void* addr)
{
	std::string line;
	std::ifstream maps("/proc/self/maps");
	if (!maps.is_open())
		return line;

	uintptr_t addr_value = (uintptr_t)addr;

	while (std::getline(maps, line))
	{
		uintptr_t start = 0, end = 0;
		const auto pos_dash = line.find('-');
		if (pos_dash != std::string::npos
		 && std::from_chars(line.data(), line.data() + pos_dash, start, 16).ec == std::errc{}
		 && std::from_chars(line.data() + pos_dash + 1, line.data() + line.size(), end, 16).ec == std::errc{})
		{
			if (addr_value >= start && addr_value < end)
				return line;
		}
	}

	line.clear();
	return line;
}

void _log_callstack(std::string_view _message, void** _array_callstack, int _count, ucontext_t* context = nullptr)
{
	// 1) log record를 생성한다.
	auto plog_record = make_object<LOG_RECORD>();

	// 2) set log record
	plog_record->type = log_type_t::fatal;
	plog_record->level = log_level_t::higher();
	plog_record->result = eRESULT::EXCEPTION;
	plog_record->origin = log_origin_t{ 0 };
	plog_record->time_occurred = chrono::time::utc::now();

	// 3) log message
	plog_record->set_message<char>(_message);

	// 4) CPU context 정보가 있으면 스택 분석 추가
	if (context)
	{
		std::string registers = _dump_registers(context);
		if (!registers.empty())
			plog_record->add_message<char>(registers);

		// 스택 메모리 덤프
#if defined(__x86_64__)
		void* stack_ptr = (void*)context->uc_mcontext.gregs[REG_RSP];
		void* base_ptr = (void*)context->uc_mcontext.gregs[REG_RBP];
#elif defined(__i386__)
		void* stack_ptr = (void*)context->uc_mcontext.gregs[REG_ESP];
		void* base_ptr = (void*)context->uc_mcontext.gregs[REG_EBP];
#elif defined(__aarch64__)
		void* stack_ptr = (void*)context->uc_mcontext.sp;
		void* base_ptr = nullptr; // ARM64는 frame pointer 사용이 optional
#else
		void* stack_ptr = nullptr;
		void* base_ptr = nullptr;
#endif

		if (stack_ptr)
		{
			std::string stack_dump = _dump_stack_memory(stack_ptr, 512);
			if (!stack_dump.empty())
				plog_record->add_message<char>(stack_dump);
		}

		if (base_ptr)
		{
			std::string frame_analysis = _analyze_stack_frames(base_ptr, stack_ptr);
			if (!frame_analysis.empty())
				plog_record->add_message<char>(frame_analysis);
		}
	}

	// 4) trace callstack
	if (_count != 0)
	{
		// - get string 
		auto array_string = backtrace_symbols(_array_callstack, _count);

		// check) 
		if (array_string != nullptr)
		{
			// 실행 파일 경로 얻기
			char exe_path[1024];
			ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
			if (len != -1)
				exe_path[len] = '\0';
			else
				exe_path[0] = '\0';

			// - description ex를 추가한다.
			for (int i = 0; i < _count; ++i)
			{
				Dl_info info;
				std::stringstream callstack_detail;

				// dladdr을 사용하여 더 상세한 정보를 얻는다
				if (dladdr(_array_callstack[i], &info))
				{
					// 함수명 demangling
					int status;
					char* demangled = nullptr;
					if (info.dli_sname != nullptr)
					{
						demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
					}

					const char* function_name = (demangled && status == 0) ? demangled : (info.dli_sname ? info.dli_sname : "??");
					const char* file_name = info.dli_fname ? info.dli_fname : "??";

					// 기본 정보 출력
					char header[2048];
					snprintf(header, sizeof(header), "\n========== Frame #%d ==========", i);
					callstack_detail << header << "\n";

					snprintf(header, sizeof(header), "Address    : 0x%016lx", (unsigned long)_array_callstack[i]);
					callstack_detail << header << "\n";

					snprintf(header, sizeof(header), "Binary     : %s (base: 0x%016lx, offset: +0x%lx)",
						file_name,
						(unsigned long)info.dli_fbase,
						(unsigned long)((char*)_array_callstack[i] - (char*)info.dli_fbase));
					callstack_detail << header << "\n";

					if (info.dli_sname)
					{
						snprintf(header, sizeof(header), "Symbol     : %s (at 0x%016lx, offset: +0x%lx)",
							function_name,
							(unsigned long)info.dli_saddr,
							(unsigned long)((char*)_array_callstack[i] - (char*)info.dli_saddr));
						callstack_detail << header << "\n";
					}

					// addr2line으로 상세 소스 위치 얻기 (인라인 함수 포함)
					SymbolResolveResult source_result;
					if (exe_path[0] != '\0')
					{
						auto absolute_addr = (uintptr_t)_array_callstack[i];
						auto relative_addr = (uintptr_t)((char*)_array_callstack[i] - (char*)info.dli_fbase);

						// 실행 파일 내의 심볼인 경우
						if (info.dli_fname && std::string_view(info.dli_fname).find(exe_path) != std::string_view::npos)
						{
							source_result = _get_detailed_source_location(absolute_addr, exe_path);
							if (source_result.locations.empty() && relative_addr != absolute_addr)
								source_result = _get_detailed_source_location(relative_addr, exe_path);
						}
						// 공유 라이브러리인 경우
						else if (info.dli_fname)
						{
							// 공유 라이브러리 베이스 주소 기준 오프셋 계산
							source_result = _get_detailed_source_location(relative_addr, info.dli_fname);
						}
					}

					// 소스 위치 정보 출력
					if (!source_result.locations.empty())
					{
						callstack_detail << "Source     :\n";
						for (size_t j = 0; j < source_result.locations.size(); ++j)
						{
							const auto& loc = source_result.locations[j];
							if (loc.is_inline)
								callstack_detail << "  [inlined] ";
							else
								callstack_detail << "  ";

							callstack_detail << loc.function_name;
							if (!loc.file_path.empty() && loc.line_number > 0)
							{
								callstack_detail << "\n            at " << loc.file_path << ":" << loc.line_number << "\n";

								// 소스 코드 주변 라인 표시
								std::string source_code = _read_source_lines(loc.file_path.c_str(), loc.line_number, 2);
								if (!source_code.empty())
								{
									callstack_detail << source_code;
								}
							}
							else
							{
								callstack_detail << "\n";
							}
						}
					}
					else if (!source_result.diagnostic.empty())
					{
						callstack_detail << "Source     : " << source_result.diagnostic << "\n";
					}

					// 메모리 맵 정보
					std::string mem_map = _get_memory_map_info(_array_callstack[i]);
					if (!mem_map.empty())
					{
						callstack_detail << "Memory Map : " << mem_map << "\n";
					}

					// backtrace_symbols 원본 정보
					callstack_detail << "Raw Info   : " << array_string[i] << "\n";

					// demangled 메모리 해제
					if (demangled)
						free(demangled);
				}
				else
				{
					// dladdr 실패 시 기본 정보만
					char header[256];
					snprintf(header, sizeof(header), "\n========== Frame #%d ==========", i);
					callstack_detail << header << "\n";
					callstack_detail << "Address    : 0x" << std::hex << (unsigned long)_array_callstack[i] << "\n";
					callstack_detail << "Raw Info   : " << array_string[i] << "\n";

					// 메모리 맵 정보
					std::string mem_map = _get_memory_map_info(_array_callstack[i]);
					if (!mem_map.empty())
					{
						callstack_detail << "Memory Map : " << mem_map << "\n";
					}
				}

				plog_record->add_message<char>(callstack_detail.str());
			}
		}

		// - free array_string
		free(array_string);
	}

	// 5) trace log
	_log(DEFAULT_LOGGER, plog_record);
}

bool dump_manager::create_dump(const std::string& _str_dump_file)
{
	// check) 
	if(_str_dump_file.empty())
		return false;
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_map_dump);

		////-----------------------------------------------------------------
		//// 1. 먼저 이미 발생한 적이 있는 Exception인지 확인한다.
		////-----------------------------------------------------------------
		//// 1) 해당하는 주소를 찾는다.
		//auto iter_find = this->m_map_dump.find(_pexception->ExceptionRecord->ExceptionAddress);

		//// check) 이미 존재하는 것이라면 발생횟수만 증가시키고 끝낸다.
		//if (iter_find != this->m_map_dump.end())
		//{
		//	// - 발생횟수만 추가한다.
		//	++(iter_find->second.count_times);

		//	// - Dump를 추가한다.
		//	this->m_list_exception_event.push_back(_pexception->ExceptionRecord->ExceptionAddress);

		//	// return) false를 리턴한다.
		//	return false;
		//}


		//-----------------------------------------------------------------
		// 2. Exception 기본 처리
		//-----------------------------------------------------------------
		// 1) Exception Count를 증가시킨다.
		++this->m_count_dump_now;

		// check) Exception최대수에 따라 처리한다. (m_count_dump_limit가 0이면 한계가 없다.)
		if(this->m_count_dump_limit > 0 && this->m_count_dump_now > this->m_count_dump_limit)
			return false;


		////-----------------------------------------------------------------
		//// 3. Dump정보를 새로 추가한다.
		////-----------------------------------------------------------------
		//// declare) 없다면 새로 추가한다.
		//sMESSAGE_DUMP tempMessage;

		//// 1) Dump Info와 Message를 설정한다.
		//tempMessage.dump_info.thread_id = GetCurrentThreadId();
		//tempMessage.dump_info.time_occurred_first = chrono::time::local::now();
		//tempMessage.dump_info.time_occurred_last = tempMessage.dump_info.time_occurred_first;
		//tempMessage.dump_info.count_times = 1;
		//tempMessage.dump_info.sExceptionRecords = *_pexception->ExceptionRecord;
		//tempMessage.dump_info.sContextRecord = *_pexception->ContextRecord;
		//tempMessage.dump_info.dump_filename = _str_dump_file;

		//// 2) Dump정보를 추가한다.
		//this->m_map_dump.emplace(tempMessage.dump_info.sExceptionRecords.ExceptionAddress, tempMessage.dump_info);

		//// 3) Dump를 Notify할 Message를 추가한다.
		//this->m_list_exception_event.push_back(tempMessage.dump_info.sExceptionRecords.ExceptionAddress);
	}

	// trace) 
	LOG_FATAL << "(fatal) CGDK.system.object: dump file created [" << _str_dump_file << "]";


	//-----------------------------------------------------------------
	// 6. write dump file using gcore
	//-----------------------------------------------------------------
	// 1) make command string - gcore를 사용하여 core dump 생성
	api::file::create_parent_directories(_str_dump_file);
	auto dump_command = std::format("gcore -o {} {} >/dev/null 2>&1", _str_dump_file, getpid());

	// 2) write to dump
	int result = _run_gcore(dump_command);

	// check) dump 생성 실패 확인
	if (result != 0)
	{
		LOG_FATAL << "(fatal) CGDK.system.object: failed to create dump file (gcore returned " << result << ")";

		// gcore가 없거나 실패한 경우, /proc/self/coredump_filter 설정 후 abort() 시도
		// coredump_filter를 설정하여 모든 메모리 영역 포함
		FILE* filter = fopen("/proc/self/coredump_filter", "w");
		if (filter)
		{
			fprintf(filter, "0x7f"); // 모든 메모리 맵 영역 포함
			fclose(filter);
		}

		// core pattern 확인 로그
		FILE* pattern = fopen("/proc/sys/kernel/core_pattern", "r");
		if (pattern)
		{
			char core_pattern[256];
			if (fgets(core_pattern, sizeof(core_pattern), pattern))
			{
				LOG_FATAL << "(fatal) CGDK.system.object: core_pattern = " << core_pattern;
			}
			fclose(pattern);
		}
	}

	// return) 
	return (result == 0);
}

// ucontext를 포함한 상세 signal handler
void dump_manager::translator_function_sa(int _signal, siginfo_t* _info, void* _context)
{
	// check) no instance (not created yet, or already destroyed while the process exits)
	//  - never create it here : allocating memory in a signal handler may deadlock (e.g. with the allocator lock held)
	if (m_s_pinstance.empty())
		return;

	// 1) Dump를 뜬다.
	m_s_pinstance->process_exception_detailed(_signal, _info, (ucontext_t*)_context);
}

void dump_manager::translator_function(int _signal)
{
	// check) no instance (not created yet, or already destroyed while the process exits)
	//  - never create it here : allocating memory in a signal handler may deadlock (e.g. with the allocator lock held)
	if (m_s_pinstance.empty())
		return;

	// 1) Dump를 뜬다.
	m_s_pinstance->process_exception(_signal);
}

// - throws throwable::system_exception from the handler of a crash signal (same as the SE translator of windows)
//   1) synchronous fault (si_code > 0 : SIGSEGV, SIGFPE, SIGBUS, SIGILL ... raised by the faulting instruction)
//      => throws the exception at the faulting instruction (it can be caught by try/catch)
//         * the code must be compiled with '-fnon-call-exceptions' (like '/EHa' of MSVC)
//         * the signal is unblocked before throwing (the handler does not return : the mask is not restored by sigreturn)
//   2) asynchronous (si_code <= 0 : sent by kill(), raise(), tgkill()) => can't throw at an arbitrary point
//      => terminated by the default action of the signal (after the handler returns)
static thread_local bool t_is_signal_exception_thrown = false;

static void _throw_signal_exception(int _signal, siginfo_t* _info)
{
	// check) asynchronous signal : default action
	if (_info == nullptr || _info->si_code <= 0)
	{
		::signal(_signal, SIG_DFL);
		::raise(_signal);	// pending while this handler runs (delivered after it returns)
		return;
	}

	// 1) unblock the signal
	sigset_t set_signal;
	sigemptyset(&set_signal);
	sigaddset(&set_signal, _signal);
	::pthread_sigmask(SIG_UNBLOCK, &set_signal, nullptr);

	// 2) throw
	t_is_signal_exception_thrown = true;
	throw throwable::system_exception(_signal);
}

// - abort() caused by an uncaught exception thrown by _throw_signal_exception() (the dump was created already)
static bool _is_abort_by_signal_exception() noexcept
{
	// - gcc : the exception is 'caught' (__cxa_begin_catch) before std::terminate() when no handler is found
	return t_is_signal_exception_thrown && (std::uncaught_exceptions() > 0 || std::current_exception() != nullptr);
}

void dump_manager::process_exception_detailed(int _signal, siginfo_t* _info, ucontext_t* _context)
{
	// check) enable상태가 아니면 설정을 하지 않는다.
	if(this->m_is_exception_handling == false)
		return;

	// definitions)
	#define TRACE_MAX 1024

	switch (_signal)
	{
	case	SIGHUP:
			{
				// 연결 끊기, 프로세스의 설정 파일을 다시 읽는데 사용된다.
			}
			break;

	case	SIGTRAP:
			{
				// 트랩 추적
			}
			break;

	case	SIGBUS:
			{
				// 버스 에러
				// 
				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				_log_callstack("receive signal'SIGBUS'"sv, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);

			}
			break;

	case	SIGFPE:
			{
				// 부동 소수점 예외

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				_log_callstack("receive signal'SIGFPE'"sv, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);
			}
			break;

	case	SIGILL:
			{
				// 잘못된 명령어

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				_log_callstack("receive signal'SIGILL'"sv, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);
			}
			break;

	case	SIGSEGV:
			{
				// 세그멘테이션 위반

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				_log_callstack("receive signal'SIGSEGV'"sv, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);
			}
			break;

	case	SIGSTKFLT:
			{
				// 코프로세서 스택 실패

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				_log_callstack("receive signal'SIGSTKFLT'"sv, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);
			}
			break;

	case	SIGABRT:
			{
				// 1) dump (not for the uncaught exception of a crash signal : the dump was created already)
				if (_is_abort_by_signal_exception() == false)
				{
					// - create dump
					this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

					// - back trace
					void* array_callstack[TRACE_MAX];
					auto size = backtrace(array_callstack, TRACE_MAX);

					// - trace callstack with context
					_log_callstack("receive signal'SIGABRT'"sv, array_callstack, size, _context);
				}

				// 2) not thrown : terminated by the default action (abort() or kill -ABRT) after this handler returns
				::signal(SIGABRT, SIG_DFL);
				::raise(SIGABRT);
			}
			break;

	default:			{
				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack with context
				char msg[128];
				snprintf(msg, sizeof(msg), "receive unknown signal'%d'", _signal);
				_log_callstack(msg, array_callstack, size, _context);

				// - throw exception 
				_throw_signal_exception(_signal, _info);
			}
			break;
	}
}

void dump_manager::process_exception(int _signal)
{

	// check) enable상태가 아니면 설정을 하지 않는다.
	if(this->m_is_exception_handling == false)
		return;

	// definitions)
	#define TRACE_MAX 1024

	switch (_signal)
	{
	case	SIGHUP:
			{
				// 연결 끊기, 프로세스의 설정 파일을 다시 읽는데 사용된다.
			}
			break;

	case	SIGTRAP:
			{
				// 트랩 추적
			}
			break;

	case	SIGBUS:
			{
				// 버스 에러
				// 
				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack
				_log_callstack("receive signal'SIGBUS'"sv, array_callstack, size);

				// - throw exception 
				throwable::system_exception rException;
				throw rException;

			}
			break;

	case	SIGFPE:
			{
				// 버스 에러

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack
				_log_callstack("receive signal'SIGFPE'"sv, array_callstack, size);

				// - throw exception 
				throwable::system_exception rException;
				throw rException;
			}
			break;

	case	SIGILL:
			{
				// 버스 에러

				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack
				_log_callstack("receive signal'SIGILL'"sv, array_callstack, size);

				// - throw exception 
				throwable::system_exception rException;
				throw rException;
			}
			break;

	case	SIGSEGV:
			{
				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack
				_log_callstack("receive signal'SIGSEGV'"sv, array_callstack, size);

				// - throw exception 
				throwable::system_exception rException;
				throw rException;
			}
			break;

	case	SIGSTKFLT:
			{
				// - create dump
				this->create_dump(_generate_crush_dump_filename(this->m_str_dump_path, this->m_count_dump_now));

				// - back trace
				void* array_callstack[TRACE_MAX];
				auto size = backtrace(array_callstack, TRACE_MAX);

				// - trace callstack
				_log_callstack("receive signal'SIGSTKFLT'"sv, array_callstack, size);

				// - throw exception 
				throwable::system_exception rException;
				throw rException;
			}
			break;

	case	SIGINT:
			{
				// - throw exception
				throwable::system_exception rException;
				throw rException;
			}
			break;

	case	SIGABRT:
			{
				//// - throw exception 
				//throwable::system_exception rException;
				//throw rException;
			}
			break;

	case	SIGKILL:
			{
				// 죽이기. 이 시그널은 잡히지는 않는다.
				this->destroy();
			}
			break;

	case	SIGQUIT:
			{
				// 종료
				this->destroy();
			}
			break;

	case	SIGUSR1:
			{
				// 사용자 정의 signal 1
			}
			break;

	case	SIGUSR2:
			{
				// 사용자 정의 signal 2
				throw throwable::force_raise();
			}
			break;

	case	SIGPIPE:
			{
				// 읽은 것이 없는 파이프에 대한 시그널
			}
			break;

	case	SIGALRM:
			{
				// 경고 클럭
			}
			break;

	case	SIGTERM:
			{
				// 소프트웨어 종료 시그널, 일반적으로 kill 시그널이 전송되기 전에 전송된다. 잡히는 signal이기 때문에 종료되는 것을 트랙할 수 있다.
				this->destroy();
			}
			break;

	case	SIGCHLD:
			{
				// 자식 프로세스의 상태 변화
			}
			break;

	case	SIGCONT:
			{
				// STOP signal 이후 다시 진행할 때 사용.
				//this->resume();
			}
			break;

	case	SIGSTOP:
			{
				// 정지, 이 시그널 역시 잡을 수 없다.
				//this->pause();
			}
			break;

	case	SIGTSTP:
			{
				// 키보드에서 Ctrl+Z를 눌렀을 때 발생한다.
				//this->destroy();
			}
			break;

	case	SIGTTIN:
			{
				// 백그라운드에서 제어터미널 읽기
			}
			break;

	case	SIGTTOU:
			{
				// 백그라운드에서 제어터미널 쓰기
			}
			break;

	case	SIGURG:
			{
				// 소켓에서 긴근한 상태
			}
			break;

	case	SIGXCPU:
			{
				// CPU 시간 제한 초과 setrlimit(2) 매뉴얼 페이지 참조
			}
			break;

	case	SIGXFSZ:
			{
				// 파일 크기제한 초과 setrlimit(2) 매뉴얼 페이지 참조
			}
			break;

	case	SIGVTALRM:
			{
				// 가상 시간 경고 setrlimit(2) 매뉴얼 페이지 참조
			}
			break;

	case	SIGPROF:
			{
				// 프로파일링 타이머 경고 setrlimit(2) 매뉴얼 페이지 참조
			}
			break;

	case	SIGWINCH:
			{
				// 윈도우 크기 변경
			}
			break;

	case	SIGIO:
			{
				// 기술자에서 입출력이 가능함. fcntl(2) 매뉴얼 참조
			}
			break;

	case	SIGPWR:
			{
				// 전원 실패
			}
			break;
	}
}
#else
	#error "[CGDK10] Not supported Platform"
#endif

std::vector<DUMP_INFO> dump_manager::status_snap() const noexcept
{
	// declare) 
	std::vector<DUMP_INFO> vector_temp;

	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_map_dump);

		// - reserve
		vector_temp.reserve(this->m_map_dump.size());

		for (auto& iter : this->m_map_dump)
		{
			vector_temp.push_back(iter.second);
		}
	}

	// return)
	return vector_temp;
}

void dump_manager::initialize_instance()
{
	// lock)
	std::lock_guard<std::mutex> lock(m_s_lockable_create);

	// check) 이미 instance가 존재하면 생성하지 않는다.
	if(m_s_pinstance.exist())
		return;
		
//#if defined(__linux__)
//	// 1) set dump limit as unlimited
//	rlimit core_limits;
//	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
//	setrlimit(RLIMIT_CORE, &core_limits);
//#endif

	m_s_pinstance.exchange_release(_make_own_raw<dump_manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
}

//-----------------------------------------------------------------------------
// initialize with a setting context (ex: "dump" section of the json setting file)
//
//	"dump": {
//		"enable": true,              // crash dump enable (true/false)
//		"path": "dumps/crash",       // dump path (created when the dump is written)
//		"count_limit": 100,          // max dump count (0: unlimited)
//		"dump_type": "mini"          // "mini" / "detail" / "full" / "default"
//	}
//-----------------------------------------------------------------------------
void dump_manager::initialize_instance(context& _context)
{
	// 1) get instance (created if not exist)
	auto pdump_manager = dump_manager::instance();

	// trace)
	LOG_INFO << "  @ dump manager"sv;

	// 2) "enable"
	if (const auto& context_find = _context.find("enable"sv); context_find.is_valid())
	{
		std::string str_value = context_find;

		if (CGDK::__stricmp(str_value.c_str(), "false") == 0 || CGDK::__stricmp(str_value.c_str(), "disable") == 0 || str_value == "0")
		{
			pdump_manager->enable_exception_handling(false);

			// trace)
			LOG_INFO << "  + parameter['enable']: false"sv;
		}
		else if (CGDK::__stricmp(str_value.c_str(), "true") == 0 || CGDK::__stricmp(str_value.c_str(), "enable") == 0 || str_value == "1")
		{
			pdump_manager->enable_exception_handling(true);

			// trace)
			LOG_INFO << "  + parameter['enable']: true"sv;
		}
		else
		{
			// trace)
			LOG_WARNING << "  (!) invalid parameter['enable']: '"sv << str_value << "' (ignored)"sv;
		}
	}

	// 3) "path"
	if (const auto& context_find = _context.find("path"sv, "dump_path"sv); context_find.is_valid())
	{
		std::string str_path = context_find;

		pdump_manager->set_dump_path(str_path);

		// trace)
		LOG_INFO << "  + parameter['path']: "sv << str_path;
	}

	// 4) "count_limit"
	if (const auto& context_find = _context.find("count_limit"sv, "count limit"sv, "countlimit"sv); context_find.is_valid())
	{
		std::string str_value = context_find;

		int64_t count_limit = 0;
		try { count_limit = std::stoll(str_value); } catch (...) { count_limit = -1; }

		if (count_limit >= 0)
		{
			pdump_manager->set_dump_count_limit(static_cast<size_t>(count_limit));

			// trace)
			LOG_INFO << "  + parameter['count_limit']: "sv << count_limit;
		}
		else
		{
			// trace)
			LOG_WARNING << "  (!) invalid parameter['count_limit']: '"sv << str_value << "' (ignored)"sv;
		}
	}

	// 5) "dump_type"
	if (const auto& context_find = _context.find("dump_type"sv, "dump type"sv, "dumptype"sv); context_find.is_valid())
	{
		std::string str_value = context_find;

		if (CGDK::__stricmp(str_value.c_str(), "mini") == 0)
			pdump_manager->set_dump_type(eDUMP_CONTEXT::MINI);
		else if (CGDK::__stricmp(str_value.c_str(), "detail") == 0 || CGDK::__stricmp(str_value.c_str(), "defail") == 0)
			pdump_manager->set_dump_type(eDUMP_CONTEXT::DETAIL);
		else if (CGDK::__stricmp(str_value.c_str(), "full") == 0)
			pdump_manager->set_dump_type(eDUMP_CONTEXT::FULL);
		else if (CGDK::__stricmp(str_value.c_str(), "default") == 0)
			pdump_manager->set_dump_type(eDUMP_CONTEXT::DEFAULT);
		else
		{
			// trace)
			LOG_WARNING << "  (!) invalid parameter['dump_type']: '"sv << str_value << "' (ignored)"sv;
			return;
		}

		// trace)
		LOG_INFO << "  + parameter['dump_type']: '"sv << str_value << "'"sv;
	}
}

void dump_manager::set_instance(own_ptr<dump_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<dump_manager> pinstance_pre;

	{
		// lock)
		std::lock_guard<std::mutex> lock(m_s_lockable_create);

		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void dump_manager::reset_instance() noexcept
{
	// declare)
	own_ptr<dump_manager> pinstance;

	{
		// lock)
		std::lock_guard<std::mutex> lock(m_s_lockable_create);

		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

void dump_manager::print_statistics() noexcept
{
	try
	{
		// declare)
		shared_buffer buf_output;

		// 1) get instance
		auto pdump_manager = dump_manager::instance();

		// 2) dump type
		auto type_dump = pdump_manager->get_dump_type();
		std::string_view str_type = (type_dump == eDUMP_CONTEXT::FULL) ? "full"sv : (type_dump == eDUMP_CONTEXT::DETAIL) ? "detail"sv : "mini"sv;

		{
			// lock)
			std::unique_lock<std::mutex> lock(pdump_manager->m_lockable_map_dump);

			// 3) alloc shared_buffer for log text
			buf_output = alloc_shared_buffer((pdump_manager->m_map_dump.size() + 10) * 384);

			// 4) settings
			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "dump  enable: {}  path: '{}'  type: {}  count: {} / {}\n" "\x1b[0m",
				pdump_manager->m_is_exception_handling ? "true" : "false",
				pdump_manager->m_str_dump_path,
				str_type,
				pdump_manager->m_count_dump_now,
				(pdump_manager->m_count_dump_limit != 0) ? std::to_string(pdump_manager->m_count_dump_limit) : std::string("unlimited"))
			);

			// 5) title
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "{:>10s}{:>7s}  {:23s}  {:23s}  {}\n" "\x1b[0m",
				"thread",
				"times",
				"first",
				"last",
				"file")
			);
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

			// 6) dumps (crash address)
			for (const auto& [paddress, info] : pdump_manager->m_map_dump)
			{
				buf_output.append_text(std::format("{}" "{:>10}{:>7}  {:23.23s}  {:23.23s}  {}" "\x1b[0m\n",
					(info.count_times > 1) ? "\x1b[93m" : "\x1b[92m",
					info.thread_id,
					info.count_times,
					to_string(info.time_occurred_first),
					to_string(info.time_occurred_last),
					info.dump_filename)
				);
			}

			// - no dump
			if (pdump_manager->m_map_dump.empty())
				buf_output.append_text("\x1b[90m" "(no dump)\n" "\x1b[0m"sv);

			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		}

		// 7) output
	#if defined(_WIN32)
		DWORD number_written = 0;
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	#else
		fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	#endif
	}
	catch (...)
	{
	}
}

void dump_manager::dispatch_exception_message_all()
{
	// check) 하나도 없으면 그냥 끝낸다.
	if(m_list_exception_event.empty())
		return;

	// declare) 
	circular_list<void*> temp_list;

	// 1) ExceptionEvent를 복사한다.
	{
		// lock)
		std::lock_guard<std::mutex> lock(this->m_lockable_map_dump);

		// - List를 몽땅 바꾼다~ Swap!!!
		temp_list.swap(m_list_exception_event);

		// check) 하나도 없으면 그냥 끝낸다.
		if(temp_list.empty())
			return;
	}

	// 2) 모든 Message를 전송한다.
	for(auto& iter:temp_list)
	{
		// declare) 
		std::map<void*, DUMP_INFO>::iterator iter_find;

		{
			// lock)
			std::lock_guard<std::mutex> lock(this->m_lockable_map_dump);

			// - 해당하는 Dump를 찾는다.
			iter_find = this->m_map_dump.find(iter);

			// check) 찾지 못했으면 다음... (이럴 리는 없다...)
			CONTINUE_IF(iter_find == this->m_map_dump.end());
		}

		// - Message를 날린다.
		transmit_message(sMESSAGE_DUMP((*iter_find).second));
	}
}

void dump_manager::enable_exception_handling(bool _is_enable) noexcept
{
	this->m_is_exception_handling = _is_enable;
}

void dump_manager::disable_exception_handling() noexcept
{
	this->m_is_exception_handling = false;
}

bool dump_manager::is_exception_handling() const noexcept
{
	return this->m_is_exception_handling;
}

#if defined(_WIN32)
void dump_manager::set_seh_to_thread() noexcept
{
	// check) enable상태가 아니면 설정을 하지 않는다.
	if(this->m_is_exception_handling == false)
		return;

	// 1) Translator를 설정한다.
#if defined(_MSC_VER)
#pragma warning(disable:4535)
#endif

#ifndef	NO_SUPPORT_SEH
	_set_se_translator(&dump_manager::translator_function);
#endif

#if defined(_MSC_VER)
	#pragma warning(default:4535)
#endif
}

void dump_manager::invalid_parameter_handler(wchar_t const* _expression, wchar_t const* _function, wchar_t const* _file, unsigned int _line, uintptr_t _reserved)
{
	dump_manager::instance()->process_invalid_parameter(_expression, _function, _file, _line, _reserved);
}

#elif defined(__linux__)
// - crash signals (process-wide : installed once)
//   * called by the system_executor and every api::Nthread (like the SE translator of windows)
//   * a crash on the thread created before the system_executor is also thrown as throwable::system_exception
void dump_manager::set_signal_handler_crash() noexcept
{
	// check) installed already
	static std::mutex s_lockable_install;
	static bool s_is_installed = false;
	std::lock_guard lock(s_lockable_install);
	if (s_is_installed)
		return;
	s_is_installed = true;

	// 0) create the instance now (the signal handlers never create it)
	dump_manager::instance();

	// sigaction을 사용하여 ucontext 정보를 얻을 수 있도록 설정
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = dump_manager::translator_function_sa;
	sa.sa_flags = SA_SIGINFO;	// SA_SIGINFO: ucontext. (no SA_RESETHAND : every crash is handled and thrown as throwable::system_exception)
	sigemptyset(&sa.sa_mask);

	// 크래시 관련 중요 시그널들은 상세 정보 수집
	sigaction(SIGBUS, &sa, nullptr);    // 버스 에러
	sigaction(SIGFPE, &sa, nullptr);    // 부동 소수점 예외
	sigaction(SIGILL, &sa, nullptr);    // 잘못된 명령어
	sigaction(SIGSEGV, &sa, nullptr);   // 세그멘테이션 위반
	sigaction(SIGSTKFLT, &sa, nullptr); // 코프로세서 스택 실패
	sigaction(SIGABRT, &sa, nullptr);   // Abort
}

// - installs the handler only when the signal has the default disposition
//   (a handler installed by the application before is not overwritten)
static void _set_signal_handler_if_default(int _signal, void (*_handler)(int)) noexcept
{
	struct sigaction sa_old;
	memset(&sa_old, 0, sizeof(sa_old));

	// check) fail to get / not default (SIG_IGN or handler of the application)
	if (sigaction(_signal, nullptr, &sa_old) != 0)
		return;
	if ((sa_old.sa_flags & SA_SIGINFO) != 0 || sa_old.sa_handler != SIG_DFL)
		return;

	signal(_signal, _handler);
}

void dump_manager::set_signal_handler() noexcept
{
	// 1) crash signals (SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGSTKFLT, SIGABRT)
	set_signal_handler_crash();

	// 2) SIGPIPE : ignored (writing to a closed socket must not kill the process)
	_set_signal_handler_if_default(SIGPIPE, SIG_IGN);

	// 3) termination signals (SIGTERM, SIGINT, SIGQUIT, SIGHUP) are NOT handled here.
	//    - they belong to the application (systemd/docker stop sends SIGTERM).
	//      the handler of dump_manager ignored SIGTERM and threw an exception from
	//      an asynchronous signal handler on SIGINT (undefined behavior).
	//    - the default action (terminate) is kept unless the application installs its own handler.

	// 4) others (only when the signal has the default disposition)
	_set_signal_handler_if_default(SIGTRAP, dump_manager::translator_function); // 트랩 추적
	_set_signal_handler_if_default(SIGUSR1, dump_manager::translator_function); // 사용자 정의 signal 1
	_set_signal_handler_if_default(SIGUSR2, dump_manager::translator_function); // 사용자 정의 signal 2
	_set_signal_handler_if_default(SIGALRM, dump_manager::translator_function); // 경고 클럭
	_set_signal_handler_if_default(SIGSYS, dump_manager::translator_function);
	_set_signal_handler_if_default(SIGCHLD, dump_manager::translator_function); // 자식 프로세스의 상태 변화
	_set_signal_handler_if_default(SIGCONT, dump_manager::translator_function); // STOP signal 이후 다시 진행할 때 사용.
	_set_signal_handler_if_default(SIGTSTP, dump_manager::translator_function); // 키보드에서 Ctrl+Z를 눌렀을 때 발생한다.
	_set_signal_handler_if_default(SIGTTIN, dump_manager::translator_function); // 백그라운드에서 제어터미널 읽기
	_set_signal_handler_if_default(SIGTTOU, dump_manager::translator_function); // 백그라운드에서 제어터미널 쓰기
	_set_signal_handler_if_default(SIGURG, dump_manager::translator_function); // 소켓에서 긴근한 상태
	_set_signal_handler_if_default(SIGXCPU, dump_manager::translator_function); // CPU 시간 제한 초과 setrlimit(2) 매뉴얼 페이지 참조
	_set_signal_handler_if_default(SIGXFSZ, dump_manager::translator_function); // 파일 크기제한 초과 setrlimit(2) 매뉴얼 페이지 참조
	_set_signal_handler_if_default(SIGVTALRM, dump_manager::translator_function); // 가상 시간 경고 setrlimit(2) 매뉴얼 페이지 참조
	_set_signal_handler_if_default(SIGPROF, dump_manager::translator_function); // 프로파일링 타이머 경고 setrlimit(2) 매뉴얼 페이지 참조
	_set_signal_handler_if_default(SIGWINCH, dump_manager::translator_function); // 윈도우 크기 변경
	_set_signal_handler_if_default(SIGIO, dump_manager::translator_function); // 기술자에서 입출력이 가능함. fcntl(2) 매뉴얼 참조
	_set_signal_handler_if_default(SIGPWR, dump_manager::translator_function); // 전원 실패
}

#endif

#ifdef _WIN32
void dump_manager::set_invalid_parameter_handle() noexcept
{
	// 1) Set Invalid parameter handler
	_set_invalid_parameter_handler(invalid_parameter_handler);
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#define	SNAP_PROLOG(context)						\
	__asm	push	ebp								\
	__asm	mov		ebp,			esp				\
	__asm	sub		esp,			__LOCAL_SIZE	\
													\
	__asm	mov		context.Eax,	eax				\
	__asm	mov		context.Ebx,	ebx				\
	__asm	mov		context.Ecx,	ecx				\
	__asm	mov		context.Edx,	edx				\
	__asm	mov		context.Edi,	edi				\
	__asm	mov		context.Esi,	esi				\
													\
	__asm	xor		eax,			eax				\
	__asm	mov		ax,				gs				\
	__asm	mov		context.SegGs,	eax				\
	__asm	mov		ax,				fs				\
	__asm	mov		context.SegFs,	eax				\
	__asm	mov		ax,				es				\
	__asm	mov		context.SegEs,	eax				\
	__asm	mov		ax,				ds				\
	__asm	mov		context.SegDs,	eax				\
	__asm	mov		ax,				cs				\
	__asm	mov		context.SegCs,	eax				\
	__asm	mov		ax,				ss				\
	__asm	mov		context.SegSs,	eax				\
													\
	__asm	mov		eax,			DWORD PTR [ebp]	\
	__asm	mov		context.Ebp,	eax				\
													\
	__asm	mov		eax,			ebp				\
													\
	__asm	add		eax,			8				\
	__asm	mov		context.Esp,	eax				\
													\
	__asm	push	esi								\
	__asm	push	edi								\
	__asm	push	ebx								\
	__asm	push	ecx								\
	__asm	push	edx


#define	SNAP_PEPILOG(return_value)					\
	__asm	pop		edx								\
	__asm	pop		ecx								\
	__asm	pop		ebx								\
	__asm	pop		edi								\
	__asm	pop		esi								\
	__asm	mov		eax,			return_value	\
	__asm	mov		esp,			ebp				\
	__asm	pop		ebp								\
	__asm	ret


#define COPYKEYCONTEXTREGISTERS(ctxDest, ctxSrc)	\
	ctxDest.Eax		 = ctxSrc.Eax;					\
	ctxDest.Ebx		 = ctxSrc.Ebx;					\
	ctxDest.Ecx		 = ctxSrc.Ecx;					\
	ctxDest.Edx		 = ctxSrc.Edx;					\
	ctxDest.Edi		 = ctxSrc.Edi;					\
	ctxDest.Esi		 = ctxSrc.Esi;					\
	ctxDest.SegGs	 = ctxSrc.SegGs;				\
	ctxDest.SegFs	 = ctxSrc.SegFs;				\
	ctxDest.SegEs	 = ctxSrc.SegEs;				\
	ctxDest.SegDs	 = ctxSrc.SegDs;				\
	ctxDest.SegCs	 = ctxSrc.SegCs;				\
	ctxDest.SegSs	 = ctxSrc.SegSs;				\
	ctxDest.Ebp		 = ctxSrc.Ebp 

#define	CGEXCEPTION_CALLNEARBACK	5
#define	CGEXCEPTION_CALLFARBACK		6

bool CalculateBeginningOfCallInstruction(uint32_t& _RetAddr)
{
	// declare) 
	bool	bRet	 = true;

	__try
	{
		uint8_t*	pBytes	 = reinterpret_cast<uint8_t*>(_RetAddr);

		if(*(pBytes-CGEXCEPTION_CALLNEARBACK) == 0xE8)
		{
			_RetAddr	 -= CGEXCEPTION_CALLNEARBACK;
		}
		else if(*(pBytes-CGEXCEPTION_CALLFARBACK) == 0xFF)
		{
			_RetAddr	 -= CGEXCEPTION_CALLFARBACK;
		}
		else
		{
			bRet	 = false;
		}
	}
	#pragma warning(disable:6320)
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		bRet	 = false;
	}
	#pragma warning(default:6320)

	// return)
	return	bRet;
}

bool CommonSnapCurrentProcessDump(const std::string_view _str_dump_file, PCONTEXT p_pContext, eDUMP_CONTEXT _type_dump)
{
	//-----------------------------------------------------------------
	// 1. Dump file 이름을 설정한다.
	//-----------------------------------------------------------------
	// declare)
	char			str_tempFileName[MAX_STRING_LENGTH]={};
	static uint32_t	countDumpNow = 0;	

	// 1) DumpfileName
	if(_str_dump_file.empty())
	{
		// - 현재의 시간을 얻는다.
		chrono::time::utc::time_point tempTime = chrono::time::utc::now();
		const std::string& stdDumpPath = dump_manager::instance()->get_dump_path();

		// - Dump file 이름을 설정한다.
		std::snprintf(str_tempFileName, std::size(str_tempFileName), "%s/Snap_%04d-%02d-%02d@%02d-%02d-%02d_%03d.dmp", stdDumpPath.c_str(), tempTime.get_year(), tempTime.get_month(), tempTime.get_day(), tempTime.get_hour(), tempTime.get_minute(), tempTime.get_second(), tempTime.get_millisecond());

		// - Dump file이름을 설정한다.
		_str_dump_file = str_tempFileName;
	}


	//-----------------------------------------------------------------
	// 2. File을 open한다.
	//-----------------------------------------------------------------
	// declare) 
	FILE_HANDLE hFile;

	// 1) File을 연다.
	api::file::create_parent_directories(_str_dump_file);
	hFile = CreateFileA(_str_dump_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// check) File을 열지 못했나?
	if(hFile==INVALID_CGHANDLE_VALUE)
		return false;


	//-----------------------------------------------------------------
	// 3. Dump를 저장한다.
	//-----------------------------------------------------------------
	// 1) Exception Recored...
	EXCEPTION_RECORD temp_exception_record;
	std::memset(&temp_exception_record, 0, sizeof(EXCEPTION_RECORD));
#if defined(_M_X64)
	temp_exception_record.ExceptionAddress	 = reinterpret_cast<void*>(p_pContext->Rip);
#else
	temp_exception_record.ExceptionAddress	 = reinterpret_cast<void*>(p_pContext->Eip);
#endif

	// 2) Exception Pointer
	EXCEPTION_POINTERS temp_exception_pointers;
	std::memset(&temp_exception_pointers, 0, sizeof(EXCEPTION_POINTERS));
	temp_exception_pointers.ContextRecord	 = p_pContext;
	temp_exception_pointers.ExceptionRecord	 = &temp_exception_record;

	// 3) Dump Input값을 설정한다.
	MINIDUMP_EXCEPTION_INFORMATION temp_dump_information;
	std::memset(&temp_dump_information, 0, sizeof(MINIDUMP_EXCEPTION_INFORMATION));
	temp_dump_information.ThreadId			 = GetCurrentThreadId();
	temp_dump_information.ExceptionPointers	 = &temp_exception_pointers;
	temp_dump_information.ClientPointers	 = TRUE;

	// 4) Dump한다.
	auto result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile , _type_dump, &temp_dump_information, NULL, NULL);

	// check) Mini Dump파일 저장에 실패했나?
	EXECUTE_IF(result == FALSE, LOG(log_type_t::fatal) << "(err ) fail to writing dump on file[" << _str_dump_file << "] (" << __FUNCTION__ << ")");

	// trace) 
	LOG_ERROR << "(fatal) CGDK.system.object: dump file created [" << _str_dump_file << "]";


	//-----------------------------------------------------------------
	// 4. File을 닫는다.
	//-----------------------------------------------------------------
	// 1) File을 닫는다.
	CloseHandle(hFile);
	

	// return) 
	return true;
}

__declspec(naked) bool CGDK::snap_dump(std::string_view _str_dump_file, eDUMP_CONTEXT _type_dump)
{
#ifdef _WIN32
#pragma  warning(disable:26494)
	//-----------------------------------------------------------------
	// 1. Snap Pro
	//-----------------------------------------------------------------
	// declare) 
	CONTEXT	contextInitial;
	CONTEXT	context_final;

	// 1) REGISTER를 snap한다~
	SNAP_PROLOG(contextInitial);


	//-----------------------------------------------------------------
	// 2. Dump를 저장한다.
	//-----------------------------------------------------------------
	// 1) context_final을 0으로 지운다.
	std::memset(&context_final, 0, sizeof(CONTEXT));

	// 2) ContextFlags를 설정한다.
	context_final.ContextFlags	 = _type_dump;

	// 3) Thread Context를 얻는다.
	BOOL result = GetThreadContext(get_current_thread(), &context_final);

	// check) 
	if(result==TRUE)
	{
		// 4) 
		COPYKEYCONTEXTREGISTERS(context_final, contextInitial);

		// 5) Return Address를 얻는다.
		uint32_t dwRetAddr = reinterpret_cast<uint32_t>(_returnAddress());

		// 6) Call Instruction의 시작을 계산한다.
		result = CalculateBeginningOfCallInstruction(dwRetAddr);

		// check) 
		if(result==TRUE)
		{
			// 7) EIP를 얻는다.
			context_final.Eip = static_cast<uint32_t>(dwRetAddr);

			// 8) 파일에 기록한다.
			result = CommonSnapCurrentProcessDump(_str_dump_file, &context_final, _type_dump);
		}
	}

	//-----------------------------------------------------------------
	// 3. Snap Pep
	//-----------------------------------------------------------------
	SNAP_PEPILOG(result)
#pragma  warning(default:26494)
#endif
}
#else
bool CGDK::snap_dump([[maybe_unused]] std::string_view _str_dump_file, [[maybe_unused]] eDUMP_CONTEXT _type_dump)
{
	// 1) file name (default : "<dump path>/snap_<time>_<count>")
	auto pdump_manager = dump_manager::instance();
	std::string str_dump_file = _str_dump_file.empty() ? _generate_dump_filename(pdump_manager->get_dump_path(), pdump_manager->dump_count(), "snap_") : std::string(_str_dump_file);

#if defined(_WIN32)
	// 2) create file
	api::file::create_parent_directories(str_dump_file);
	HANDLE handle_file = ::CreateFileA(str_dump_file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	// check)
	if (handle_file == INVALID_HANDLE_VALUE)
	{
		LOG_ERROR << "(err ) fail to create snap dump file[" << str_dump_file << "]";
		return false;
	}

	// 3) write the dump of the current process (all threads, called from this thread : no exception information)
	auto result = ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), handle_file, static_cast<MINIDUMP_TYPE>(_to_minidump_type(_type_dump)), nullptr, nullptr, nullptr);

	// 4) close file
	::CloseHandle(handle_file);

	// check)
	if (result == FALSE)
	{
		LOG_ERROR << "(err ) fail to writing snap dump on file[" << str_dump_file << "]";
		return false;
	}

	// trace)
	LOG_ERROR << "(info) CGDK.system.object: snap dump file created [" << str_dump_file << "]";

	// return)
	return true;
#elif defined(__linux__)
	// 2) make command string ('gcore -o <prefix> <pid>' creates '<prefix>.<pid>')
	api::file::create_parent_directories(str_dump_file);
	auto dbx = CGDK::stream_string("gcore -a -o ", str_dump_file, " ", getpid());

	// 3) write to dump
	auto result = _run_gcore(dbx);

	// trace)
	LOG_ERROR << "(info) CGDK.system.object: snap dump [" << str_dump_file << "." << getpid() << "] result:" << result;

	// return)
	return result == 0;
#else
	return false;
#endif
}
#endif

void CGDK::enable_exception_handling(bool _is_enable)
{
	dump_manager::instance()->enable_exception_handling(_is_enable);
}

void CGDK::disable_exception_handling()
{
	dump_manager::instance()->disable_exception_handling();
}

bool CGDK::is_exception_handling()
{
	return dump_manager::instance()->is_exception_handling();
}
