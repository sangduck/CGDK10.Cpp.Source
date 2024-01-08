#include "pch.h"

#if defined(_WIN32)
#include <DbgHelp.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

using namespace CGDK;

#ifdef NDEBUG
	#define	DEFAULT_ENABLE_SEH	true
#else
	#define	DEFAULT_ENABLE_SEH	false
#endif 


dump_manager::dump_manager(eDUMP_CONTEXT _type_dump, size_t _max_dump_count) noexcept :
		object::Nnameable(),
		m_is_enable_seh(DEFAULT_ENABLE_SEH),
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

bool dump_manager::create_dump(const std::string_view _str_dump_file, LPEXCEPTION_POINTERS _pexception)
{
	// check) 
	RETURN_IF(_str_dump_file.empty(), false);

	// check) 
	RETURN_IF(_pexception==nullptr, false);
	
#if defined(_MSC_VER)
	scoped_lock(m_lockable_map_dump)
	{
		//-----------------------------------------------------------------
		// 1. 먼저 이미 발생한 적이 있는 Exception인지 확인한다.
		//-----------------------------------------------------------------
		// 1) 해당하는 주소를 찾는다.
		auto iter_find = m_map_dump.find(_pexception->ExceptionRecord->ExceptionAddress);

		// check) 이미 존재하는 것이라면 발생횟수만 증가시키고 끝낸다.
		if(iter_find!=m_map_dump.end())
		{
			// - 발생횟수만 추가한다.
			++(iter_find->second.count_times);

			// - Dump를 추가한다.
			m_list_exception_event.push_back(_pexception->ExceptionRecord->ExceptionAddress);

			// return) false를 리턴한다.
			return false;
		}


		//-----------------------------------------------------------------
		// 2. Exception 기본 처리
		//-----------------------------------------------------------------
		// 1) Exception Count를 증가시킨다.
		++m_count_dump_now;

		// check) Exception최대수에 따라 처리한다. (m_count_dump_limit가 0보다 작으면 한계가 없다.)
		RETURN_IF(m_count_dump_limit>=0 && m_count_dump_now>m_count_dump_limit, false);


		//-----------------------------------------------------------------
		// 3. Dump정보를 새로 추가한다.
		//-----------------------------------------------------------------
		// declare) 없다면 새로 추가한다.
		sMESSAGE_DUMP tempMessage;

		// 1) Dump Info와 Message를 설정한다.
		tempMessage.dump_info.thread_id			 = GetCurrentThreadId();
		tempMessage.dump_info.time_occure_first	 = chrono::time::local::now();
		tempMessage.dump_info.time_occure_last	 = tempMessage.dump_info.time_occure_first;
		tempMessage.dump_info.count_times		 = 1;
		tempMessage.dump_info.sExceptionRecords	 = *_pexception->ExceptionRecord;
		tempMessage.dump_info.sContextRecord	 = *_pexception->ContextRecord;
		tempMessage.dump_info.dump_filename		 = _str_dump_file;

		// 2) Dump정보를 추가한다.
		m_map_dump.emplace(tempMessage.dump_info.sExceptionRecords.ExceptionAddress, tempMessage.dump_info);

		// 3) Dump를 Notify할 Message를 추가한다.
		m_list_exception_event.push_back(tempMessage.dump_info.sExceptionRecords.ExceptionAddress);
	}


	//-----------------------------------------------------------------
	// 4. File을 start한다.
	//-----------------------------------------------------------------
	// declare) 
	FILE_HANDLE hFile;

	// 1) File을 연다.
	hFile = CreateFileA(_str_dump_file.data(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// check) File을 열지 못했나?
	RETURN_IF(hFile==INVALID_CGHANDLE_VALUE, false);


	//-----------------------------------------------------------------
	// 5. Dump를 저장한다.
	//-----------------------------------------------------------------
	// 1) Dump Input값을 설정한다.
	MINIDUMP_EXCEPTION_INFORMATION	temp_dump_information;
	temp_dump_information.ThreadId			 = GetCurrentThreadId();
	temp_dump_information.ExceptionPointers	 = _pexception;
	temp_dump_information.ClientPointers	 = TRUE;

	// declare) 
	uint32_t dumptype;

	// 2) ...
	switch(m_type_dump)
	{
	case	eDUMP_CONTEXT::MINI:
			dumptype = MINIDUMP_TYPE(MiniDumpNormal);
			break;

	case	eDUMP_CONTEXT::DETAIL:
			dumptype = MiniDumpWithPrivateReadWriteMemory	// 읽거나 쓸수 있는 메모리들(Stack/Heap/TLS)
					 | MiniDumpWithDataSegs					// 전역변수
					 | MiniDumpWithHandleData				// 핸들 정보(Socket, File, 등등...)
					 | MiniDumpWithFullMemoryInfo			// 가상 메모리 상의 레이아웃 정보
					 | MiniDumpWithThreadInfo				// 추가적인 Thread 정보 (Thread시간들, 시작주소 등등...)
					 | MiniDumpWithUnloadedModules			// 
					;
			break;

	case	eDUMP_CONTEXT::FULL:
			dumptype = MiniDumpWithFullMemory				// 모든 메모리읽거나 쓸수 있는 메모리들(Stack/Heap/TLS)
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
	BOOL	result	 = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile , static_cast<MINIDUMP_TYPE>(dumptype), &temp_dump_information, NULL, NULL);

	// check) Mini Dump파일 저장에 실패했나?
	EXECUTE_IF(result == FALSE, LOG(log_type_t::error) << "(err ) fail to writing dump on file["sv << _str_dump_file  <<"] ("sv << __FUNCTION__ << ")"sv);


	//-----------------------------------------------------------------
	// 6. File을 닫는다.
	//-----------------------------------------------------------------
	// 1) File을 닫는다.
	CloseHandle(hFile);
#elif defined(__linux__)
	scoped_lock(m_lockable_map_dump)
	{

	}

#else
#endif

	// return) 
	return true;
}

void dump_manager::process_exception(LPEXCEPTION_POINTERS _pexception)
{
	// check)
	RETURN_IF(_pexception == nullptr);
	

	//-----------------------------------------------------------------
	// 1. Dump file을 생성한다.
	//-----------------------------------------------------------------
	// 1) 현재의 시간을 얻는다.
	auto time_now = chrono::to_date_time(chrono::time::utc::now());

	// 2) file이름을 설정한다.
	char tempFileName[MAX_STRING_LENGTH] {0};
	__sprintf(tempFileName, "%s/crash(%04d-%02d-%02d@%02d-%02d-%02d@%07d)[%d].dmp", 
		m_str_dump_path.c_str(),
		time_now.t_year,
		time_now.t_month,
		time_now.t_day,
		time_now.t_hour,
		time_now.t_minute,
		time_now.t_second,
		time_now.t_fraction,
		static_cast<int>(m_count_dump_now));
	
	// 3) Dump File을 생성한다.
	create_dump(tempFileName, _pexception);


	//-----------------------------------------------------------------
	// 2. Exception Throw
	//-----------------------------------------------------------------
#if defined(_MSC_VER)
	// declare) 
	throwable::seh_exception rException;

	// 1) Throw를 한다.
	rException.m_exception_record = *_pexception->ExceptionRecord;
	rException.m_context_record = *_pexception->ContextRecord;

	// Throw) 던진다!!!
	throw rException;
#endif
}

void dump_manager::process_invalid_parameter(const std::wstring_view /*_expression*/, const std::wstring_view /*_function*/, const std::wstring_view /*_file*/, unsigned int /*_line*/, uintptr_t /*_preserved*/)
{
	// Throw) 던진다!!!
	throw throwable::failure(eRESULT::INVALID_PARAMETER_HANDLE);
}


std::vector<DUMP_INFO> dump_manager::status_snap() const noexcept
{
	// declare) 
	std::vector<DUMP_INFO> vector_temp;

	scoped_lock(m_lockable_map_dump)
	{
		// - reserve
		vector_temp.reserve(m_map_dump.size());

		for (auto& iter : m_map_dump)
		{
			vector_temp.push_back(iter.second);
		}
	}

	// return)
	return vector_temp;
}

void dump_manager::initialize_instance()
{
	scoped_lock(m_s_lockable_create)
	{
		RETURN_IF(m_s_pinstance.exist());
		
		m_s_pinstance = _make_own_raw<dump_manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}

void dump_manager::set_instance(own_ptr<dump_manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<dump_manager> pinstance_pre;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_s_pinstance);

		// 2) set new instance
		m_s_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void dump_manager::reset_instance() noexcept
{
	// declare)
	own_ptr<dump_manager> pinstance;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

void dump_manager::print_statistics() noexcept
{
	//try
	//{
	//	// 1) get instance
	//	auto instance = instance();

	//	// 2) alloc shared_buffer for log text
	//	auto buf_output = alloc_shared_buffer((static_cast<size_t>(instance->size()) + 4) * 256);

	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");
	//	buf_output.append_text("\x1b[37m" "{:30s} {:8s}{:>5s}{:>9s}{:>13s}{:>13s}\n" "\x1b[0m",
	//		"name",
	//		"type",
	//		"id",
	//		"thread",
	//		"executed",
	//		"pended"
	//	);
	//	buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	scoped_shared_lock(instance->m_lockable_list_executor)
	//	{
	//		for (auto& iter : instance->m_list_executor)
	//		{
	//			// check)
	//			CGASSERT_ERROR(iter != nullptr);

	//			// check)
	//			CONTINUE_IF(iter == nullptr);

	//			// - get statistics
	//			auto& statistics = iter->statistics();

	//			std::string_view executor_name = (statistics.name.size() < 30) ? std::string_view(statistics.name) : std::string_view(statistics.name.c_str(), 30);

	//			auto alloc_execute = statistics.now.count_execute.load();

	//			if (statistics.type == eEXECUTOR_TYPE::USER)
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[92m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[32m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>9}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "         0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//			else
	//			{
	//				// "1. name" "2. type"
	//				if (alloc_execute != 0)
	//				{
	//					buf_output.append_text("\x1b[37m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}
	//				else
	//				{
	//					if (_using_only == true)
	//						continue;

	//					buf_output.append_text("\x1b[90m" "{:30s} {:8s}" "\x1b[0m", executor_name, to_string<char>(statistics.type));
	//				}

	//				// - "3. id"
	//				{
	//					auto count = statistics.id;

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>4}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "    0" "\x1b[0m");
	//					}
	//				}

	//				// - "4. thread_count"
	//				{
	//					if (statistics.now.thread_count != 0)
	//					{
	//						buf_output.append_text(" {:>8}", statistics.now.thread_count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "        0" "\x1b[0m");
	//					}
	//				}

	//				// - "5. executed"
	//				{
	//					auto count = statistics.now.count_execute.load();

	//					if (count != 0)
	//					{
	//						buf_output.append_text(" {:>12}", count);
	//					}
	//					else
	//					{
	//						buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//					}
	//				}

	//				// - "6. pended"
	//				{
	//					auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

	//					if (pqueue != nullptr)
	//					{
	//						auto count = pqueue->pended_count();

	//						if (count != 0)
	//						{
	//							buf_output.append_text(" {:>12}", count);
	//						}
	//						else
	//						{
	//							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m");
	//						}
	//					}
	//				}

	//				// - new line~
	//				buf_output.append_text("\n");
	//			}
	//		}
	//	}
	//	buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

	//	// 3) 최종 출력
	//#if defined(_WIN32)
	//	DWORD number_written = 0;
	//	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
	//#else
	//	fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
	//#endif
	//}
	//catch (...)
	//{
	//}
}

void dump_manager::dispatch_exception_message_all()
{
	// check) 하나도 없으면 그냥 끝낸다.
	RETURN_IF(m_list_exception_event.empty());

	// declare) 
	circular_list<void*> temp_list;

	// 1) ExceptionEvent를 복사한다.
	scoped_lock(m_lockable_map_dump)
	{
		// - List를 몽땅 바꾼다~ Swap!!!
		temp_list.swap(m_list_exception_event);

		// check) 하나도 없으면 그냥 끝낸다.
		RETURN_IF(temp_list.empty());
	}

	// 2) 모든 Message를 전송한다.
	for(auto& iter:temp_list)
	{
		// declare) 
		std::map<void*, DUMP_INFO>::iterator iter_find;

		scoped_lock(m_lockable_map_dump)
		{
			//std::scoped_lock cs(m_lockable_map_dump);

			// - 해당하는 Dump를 찾는다.
			iter_find = m_map_dump.find(iter);

			// check) 찾지 못했으면 다음... (이럴 리는 없다...)
			CONTINUE_IF(iter_find == m_map_dump.end());
		}

		// - Message를 날린다.
		transmit_message(sMESSAGE_DUMP((*iter_find).second));
	}
}

void dump_manager::enable_seh(bool _is_enable) noexcept
{
	m_is_enable_seh = _is_enable;
}

void dump_manager::disable_seh() noexcept
{
	m_is_enable_seh = false;
}

bool dump_manager::seh_enabled() const noexcept
{
	return m_is_enable_seh;
}

void dump_manager::translator_function(uint32_t, LPEXCEPTION_POINTERS _pexception)
{
	// 1) Dump를 뜬다.
	dump_manager::instance()->process_exception(_pexception);
}

void dump_manager::set_seh_to_thread() noexcept
{
	// check) enable상태가 아니면 설정을 하지 않는다.
	RETURN_IF(m_is_enable_seh == false);

	// 1) Translator를 설정한다.
#if defined(_MSC_VER)
	#if defined(_MSC_VER)
		#pragma warning(disable:4535)
	#endif

	#ifndef	NO_SUPPORT_SEH
	_set_se_translator(&dump_manager::translator_function);
	#endif

	#if defined(_MSC_VER)
		#pragma warning(default:4535)
	#endif
#endif
}

void dump_manager::invalid_parameter_handler(wchar_t const* _expression, wchar_t const* _function, wchar_t const* _file, unsigned int _line, uintptr_t _reserved)
{
	dump_manager::instance()->process_invalid_parameter(_expression, _function, _file, _line, _reserved);
}

void dump_manager::set_invalid_parameter_handle() noexcept
{
#ifdef _WIN32
	// 1) Set Invalid parameter handler
	_set_invalid_parameter_handler(invalid_parameter_handler);
#else
#endif
}

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
		__sprintf(str_tempFileName, "%s/Snap(%04d-%02d-%02d@%02d-%02d-%02d@%03d).dmp", stdDumpPath.c_str(), tempTime.get_year(), tempTime.get_month(), tempTime.get_day(), tempTime.get_hour(), tempTime.get_minute(), tempTime.get_second(), tempTime.get_millisecond());

		// - Dump file이름을 설정한다.
		_str_dump_file = str_tempFileName;
	}


	//-----------------------------------------------------------------
	// 2. File을 open한다.
	//-----------------------------------------------------------------
	// declare) 
	FILE_HANDLE hFile;

	// 1) File을 연다.
	hFile = CreateFileA(_str_dump_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// check) File을 열지 못했나?
	RETURN_IF(hFile==INVALID_CGHANDLE_VALUE, false);


	//-----------------------------------------------------------------
	// 3. Dump를 저장한다.
	//-----------------------------------------------------------------
	// 1) Exception Recored...
	EXCEPTION_RECORD temp_exception_record;
	__zero_memory(&temp_exception_record, sizeof(EXCEPTION_RECORD));
#if defined(_M_X64)
	temp_exception_record.ExceptionAddress	 = reinterpret_cast<void*>(p_pContext->Rip);
#else
	temp_exception_record.ExceptionAddress	 = reinterpret_cast<void*>(p_pContext->Eip);
#endif

	// 2) Exception Pointer
	EXCEPTION_POINTERS temp_exception_pointers;
	__zero_memory(&temp_exception_pointers, sizeof(EXCEPTION_POINTERS));
	temp_exception_pointers.ContextRecord	 = p_pContext;
	temp_exception_pointers.ExceptionRecord	 = &temp_exception_record;

	// 3) Dump Input값을 설정한다.
	MINIDUMP_EXCEPTION_INFORMATION temp_dump_information;
	__zero_memory(&temp_dump_information, sizeof(MINIDUMP_EXCEPTION_INFORMATION));
	temp_dump_information.ThreadId			 = GetCurrentThreadId();
	temp_dump_information.ExceptionPointers	 = &temp_exception_pointers;
	temp_dump_information.ClientPointers	 = TRUE;

	// 4) Dump한다.
	auto result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile , _type_dump, &temp_dump_information, NULL, NULL);

	// check) Mini Dump파일 저장에 실패했나?
	EXECUTE_IF(result == FALSE, LOG(log_type_t::ERROR) << "(err ) fail to writing dump on file["sv << _str_dump_file << "] ("sv << __FUNCTION__ << ")"sv);

	// trace) 
	LOG_ERROR << "(err ) CGDK.system.object: dump is created [" << _str_dump_file << "]";


	//-----------------------------------------------------------------
	// 4. File을 닫는다.
	//-----------------------------------------------------------------
	// 1) File을 닫는다.
	CloseHandle(hFile);
	

	// return) 
	return true;
}


__declspec(naked) bool snap_dump(const std::wstring_view _str_dump_file, eDUMP_CONTEXT _type_dump)
{
#pragma  warning(disable:26494)
	// 1) String을 Convert한다.

	// declare) 
	char str_buffer[MAX_STRING_LENGTH];

	// 2) snap_dump
	auto result	 = snap_dump((_str_dump_file.empty() == false) ? encode_string(str_buffer, _str_dump_file) : nullptr, _type_dump);

	// return) Snap Pep
	SNAP_PEPILOG(result)
#pragma  warning(default:26494)
}

__declspec(naked) bool snap_dump(const std::string_view _str_dump_file, eDUMP_CONTEXT _type_dump)
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
	__zero_memory(&context_final, sizeof(CONTEXT));

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
bool CGDK::snap_dump(const std::string_view /*_str_dump_file*/, eDUMP_CONTEXT /*_type_dump*/)
{
	return false;
}

bool CGDK::snap_dump(const std::wstring_view /*_str_dump_file*/, eDUMP_CONTEXT /*_type_dump*/)
{
	return false;
}
#endif

void CGDK::enable_seh(bool _is_enable)
{
	dump_manager::instance()->enable_seh(_is_enable);
}

void CGDK::disable_seh()
{
	dump_manager::instance()->disable_seh();
}

bool CGDK::seh_enabled()
{
	return dump_manager::instance()->seh_enabled();
}

void CGDK::set_seh_to_thread() noexcept
{
#if defined(_MSC_VER)
	// 1) SEH Exception Handler를 설정한다.
	dump_manager::instance()->set_seh_to_thread();

	// 2) Invalid parameter Handler를 설정한다.
	dump_manager::instance()->set_invalid_parameter_handle();
#endif
}
