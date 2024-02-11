#include "pch.h"
#include "TLOG_RECORD.h"
#include <iostream>


//-----------------------------------------------------------------
// 1. Init_seg(lib)
//-----------------------------------------------------------------
// Static) 
#if defined(_MSC_VER)
	#pragma warning(disable:4073)
	#pragma init_seg(lib)
#endif

namespace CGDK
{
	// 1) CGDK initialize
	struct CGDK_initialize
	{
	#if !defined(NDEBUG) && defined(_MSC_VER) && defined(_WIN32) 
		_CrtMemState memory_state_begin{};
		_CrtMemState memory_state_end{};
		_CrtMemState memory_state_differ{};
	#endif

		CGDK_initialize()
		{
			setlocale(LC_ALL, "");

		#if !defined(NDEBUG) && defined(_MSC_VER) && defined(_WIN32) 
			_CrtMemCheckpoint(&memory_state_begin);
		#endif
		}
		~CGDK_initialize()
		{
		#if !defined(NDEBUG) && defined(_MSC_VER) && defined(_WIN32) 
			// 1) get memory state
			_CrtMemCheckpoint(&memory_state_end);

			// 2) check difference
			if (_CrtMemDifference(&memory_state_differ, &memory_state_begin, &memory_state_end))
			{
				_CrtMemDumpStatistics(&memory_state_differ);
			}
		#endif
		}

	};
	CGDK_initialize					__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 0)	g_CGDK_initialize;
	bool							g_is_initialize = true;

	// 3) time
	int								g_statistics_time = 0;

	// 4) factory manager
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 1)	factory::manager::m_s_lockable_create;
	own_ptr<factory::manager>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 2)	factory::manager::m_s_pinstance;
	std::size_t						factory::manager::m_s_sustain_serial_now;
	thread_local std::size_t		factory::manager::m_s_sustain_serial;

	// 5) log_record pool and memory block factory
	factory::memory_block			__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 3)	factory::memory_block::m_s_instance_pool;
	own_ptr<default_factory_pool_t<TLOG_RECORD>> __ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 4) g_factory_log_record = _make_own_raw<default_factory_pool_t<TLOG_RECORD>>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL CGNEW_DEBUG_INFO_COMMA "CGDK::TLOG_RECORD", eFACTORY_TYPE::POOL_SYSTEM);
 
	// 6) message logger
	std::shared_mutex				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 5)	g_cs_map_message_type_info;
	std::map<uint64_t, std::string> __ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 6)	g_map_message_type_info;

	// 7) default trace
	std::shared_mutex				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 8)	g_cs_default_logger;
	object_ptr<Ilogger>				__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 9)	g_pdefault_logger;

	// 8) executor default
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 10)	resource::manager::m_s_lockable_create;
	own_ptr<resource::manager>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 11)	resource::manager::m_s_pinstance;

	// 9) default crc
	edc::crc						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 12)	edc::default_crc;

	// 10) this thread object
	TLS	api::Nthread*				g_pthread_now = nullptr;

	// 11) thread manager (Manager의 초기화를 가장 먼저 해야함)
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 21)	api::manager_thread::m_s_lockable_create;
	own_ptr<api::manager_thread>	__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 22)	api::manager_thread::m_s_pinstance;

	// 12) exception manager (Manager의 초기화를 가장 먼저 해야함)
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 23)	dump_manager::m_s_lockable_create;
	own_ptr<dump_manager>			__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 24)	dump_manager::m_s_pinstance;

	// 13) executor manager
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 25)	executor::manager::m_s_lockable_create;
	own_ptr<executor::manager>		__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 26)	executor::manager::m_s_pinstance;

	// 14) executor default
	std::mutex						__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 27)	system_executor::m_s_lockable_create;
	system_executor					__ATTRIBUTE_PRIORITY(__STATIC_PRIORITY_LEVEL1 + 28)	system_executor::m_s_instance;
}

#if defined(_MSC_VER)
	#pragma warning(default:4073)
#endif


//-----------------------------------------------------------------
// 3. CGDK Info
//-----------------------------------------------------------------
namespace CGDK
{
	// * CGDK Component Info
	CGDK_INFO_CLASS	g_info_system_object;
}