#include "pch.h"

#if defined(__linux__)
	#include <sys/sysinfo.h>
#endif


namespace CGDK
{

extern bool g_is_initialize;
extern int g_statistics_time;


factory::manager::manager() noexcept:
	CGDK::object::Nnameable("factory_manager"sv),
	m_list_registered(),
	m_list_factory(),
	m_factory_serial(0),
	m_size_sustain_threshold{0,0,0,0},
	m_flag_sustain(true)
{
// temporary)
#if defined(__linux__) && !defined(__ANDROID__)/* && !defined(WINAPI_FAMILY)*/
	// declare) 
	uint64_t size_max_memory = 0;

	// declare) 
	struct sysinfo temp_system_info;

	// 1) 물리 메모리량을 얻는다.
	if (sysinfo(&temp_system_info) == 0)
	{
		size_max_memory = temp_system_info.totalram;
	}

	// 2) Bytes로 바꾼다.
	this->m_size_sustain_threshold[0] = static_cast<size_t>(size_max_memory * 500 / 1024);
	this->m_size_sustain_threshold[1] = static_cast<size_t>(size_max_memory * 600 / 1024);
	this->m_size_sustain_threshold[2] = static_cast<size_t>(size_max_memory * 700 / 1024);
	this->m_size_sustain_threshold[3] = size_max_memory;

#elif (!defined(_MSC_VER) || defined(_M_X64)) && !defined(__ANDROID__)
	// declare) 
	ULONGLONG size_max_memory = 0;

	// 1) 물리 메모리량을 얻는다.
	if(GetPhysicallyInstalledSystemMemory(&size_max_memory) == FALSE)
	{
		// - 만약에 실패하면 기본 Default값은 8G로 한다.
		size_max_memory = 8 * 1024 * 1024;
	}

	// 2) Bytes로 바꾼다.
	size_max_memory *= 1024;

	this->m_size_sustain_threshold[0] = static_cast<size_t>(size_max_memory * 500 / 1024);
	this->m_size_sustain_threshold[1] = static_cast<size_t>(size_max_memory * 600 / 1024);
	this->m_size_sustain_threshold[2] = static_cast<size_t>(size_max_memory * 700 / 1024);
	this->m_size_sustain_threshold[3] = size_max_memory;
#else
	this->m_size_sustain_threshold[0] = static_cast<size_t>(1400ull * 1024ull * 1024ull);	// 1.4G
	this->m_size_sustain_threshold[1] = static_cast<size_t>(1600ull * 1024ull * 1024ull);	// 1.6G
	this->m_size_sustain_threshold[2] = static_cast<size_t>(1800ull * 1024ull * 1024ull);	// 1.8G
	this->m_size_sustain_threshold[3] = static_cast<size_t>(2048ull * 1024ull * 1024ull);	// 2.0G
#endif
}

factory::manager::~manager() noexcept
{
	// locked) 
	{
		// lock) 
		std::lock_guard<std::shared_mutex> cs(const_cast<std::shared_mutex&>(this->m_lockable_list_factory));

		// check) manager가 파괴될땐 분명 Factory이 하나도 없어야 한다.
		CGASSERT_ERROR(this->m_list_registered.empty());
	}
}

result_code factory::manager::destroy() noexcept
{
	// 주의1) destroy함수는 own_ptr<T>가 소멸될 때와 이 객체가 소멸될 때 각각 1번씩 총 2번이 호출된다는 점에 주의할 필요가 있다.

	// 주의2) close후 destory 처리
	//  - factory각 stack된 객체들을 delete하며 레퍼런스된 객체들이 해제 되며 이미 소멸된 factory를 접근하는 문제가 있을 수 있다.
	//  - 따라서 반드시 먼저 factory의 close()함수를 호출해 일단 모든 객체를 소멸시킨 후 factory 객체를 소멸시켜야 한다.

	// 1) close and destroy all factory
	{
		// declare) 
		std::list<object_ptr<Ifactory>> list_factory;

		// 주의) 생성된 역순으로 파괴해야하므로 역순 복사한다.

		// - copy registered factory (reverse copy)
		scoped_unique_lock(this->m_lockable_list_factory)
		{
			for (auto& iter : this->m_list_factory)
			{
				list_factory.push_front(iter);
			}
		}

		// - close all registered factory
		for (auto& iter : list_factory)
		{
			iter->close();
		}

		// - destroy all registered factory
		for (auto& iter : list_factory)
		{
			iter->destroy();
		}
	}

	// 2) clear factory
	{
		// declare) 
		std::list<object_ptr<Ifactory>> list_factory_nested;

		// - 먼저 move
		scoped_unique_lock(this->m_lockable_list_factory)
		{
			list_factory_nested = std::move(this->m_list_factory);
		}

		// - clear (안해도 자동으로 cleanr되지만...)
		list_factory_nested.clear();
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}


bool factory::manager::process_register_factory(Ifactory* _pfactory)
{
	scoped_unique_lock(this->m_lockable_list_factory)
	{
		// 1) Factory에서 이미 등록되어 있는지 참는다.
		const auto	iter_factory = std::find(this->m_list_registered.begin(), this->m_list_registered.end(), _pfactory);

		// check) 이미 등록되어 있으면 끝낸다.
		RETURN_IF(iter_factory != this->m_list_registered.end(), false);

		// 2) 등록한다.
		this->m_list_registered.push_back(_pfactory);
		this->m_list_factory.emplace_back(_pfactory);

		// 3) set factory id
		_pfactory->m_statistics.id = this->m_factory_serial++;

		// 4) add_reference한다.
		this->add_ref();
	}

	// 4) View에 통보한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::FACTORY::FACTORY_REGISTER, 0, reinterpret_cast<uintptr_t>(_pfactory)));
	
	// return)
	return true;
}

bool factory::manager::process_unregister_factory(Ifactory* _pfactory) noexcept
{
	// 주의)
	//   여기로 넘어온 _pfactory는 이미 Reference count가 0이 되여 delete되어 소멸과정에서 넘어온 값일 수 있다.
	//   따라서 이 값에 다시 Reference count를 변경시키게하는 object_ptr<T>에 넣는다든지 add_ref()를 한다든지 release()를 수행해서는 안됀다!

	// check)
	RETURN_IF(_pfactory == nullptr, false);

	try
	{
		// declare) for hold
		object_ptr<Ifactory> pfactory;

		scoped_unique_lock(this->m_lockable_list_factory)
		{
			// 1) Factory 관리 List에서 떼낸다.
			auto iter_registered = std::find(this->m_list_registered.begin(), this->m_list_registered.end(), _pfactory);

			// check) 등록되어 있지 않으면 끝낸다.
			RETURN_IF(iter_registered == this->m_list_registered.end(), false);

			// 2) 찾은 Factory을 제거한다.
			this->m_list_registered.erase(iter_registered);

			// 3) Factory 관리 List에서 떼낸다.
			auto iter_factory = std::find(this->m_list_factory.begin(), this->m_list_factory.end(), _pfactory);

			// 4) 찾은 Factory을 제거한다.
			if (iter_factory != this->m_list_factory.end())
			{
				// - 일단 옮긴후에..
				pfactory = std::move(*iter_factory);

				// - 제거
				this->m_list_factory.erase(iter_factory);
			}
		}

		// 5) View에 통보한다.
		this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::FACTORY::FACTORY_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pfactory)));
	}
	catch(...)
	{
	}

	// 4) Release한다.
	this->release();

	// return) 
	return true;
}

object_ptr<Ifactory> factory::manager::find(const std::string_view _name) const noexcept
{
	scoped_shared_lock(this->m_lockable_list_factory)
	{
		// 1) 해당 Factory을 찾는다.
		for(auto& iter : this->m_list_factory)
		{
			if(iter->is_this(_name))
			{
				return iter;
			}
		}
	}

	// return) 실패!
	return object_ptr<Ifactory>();
}

object_ptr<Ifactory> factory::manager::find(Ifactory* _pfactory) const noexcept
{
	// locked) 
	{
		// lock) 
		std::shared_lock<std::shared_mutex> cs(const_cast<std::shared_mutex&>(this->m_lockable_list_factory));

		// 1) 찾는다.
		const auto iter_find = std::find(this->m_list_factory.begin(), this->m_list_factory.end(), _pfactory);

		// check) 
		RETURN_IF(iter_find == this->m_list_factory.end(), object_ptr<Ifactory>());

		// return) 
		return *iter_find;
	}
}

object_ptr<Ifactory> factory::manager::find(int _id_factory) const noexcept
{
	scoped_shared_lock(this->m_lockable_list_factory)
	{
		// 1) 해당 Factory을 찾는다.
		for(auto& iter : this->m_list_factory)
		{
			if(iter->id()== _id_factory)
			{
				return iter;
			}
		}
	}

	// return) 실패!
	return object_ptr<Ifactory>();
}

void factory::manager::sustain()
{
	// check) enable sustain 상태가 아니면 false를 리턴한다.
	RETURN_IF(this->is_enable_sustain() == false);

	// 1) memory Usage를 얻는다.
	const auto size_memeory_usage = static_cast<size_t>(api::get_memory_usage());

	// declare) 
	eFACTORY_GC_LEVEL level;

	// 2) 메모리의 상황에 따라 sustain의 level을 결정한다.
	if (size_memeory_usage > this->m_size_sustain_threshold[2])
	{
		level = eFACTORY_GC_LEVEL::STACKED_ALL;
	}
	else if (size_memeory_usage > this->m_size_sustain_threshold[1])
	{
		level = eFACTORY_GC_LEVEL::EXCESS_ALL;
	}
	else if (size_memeory_usage > this->m_size_sustain_threshold[0])
	{
		level = eFACTORY_GC_LEVEL::HEAVY;
	}
	else
	{
		level = eFACTORY_GC_LEVEL::NORMAL;
	}

	// 3) Object Factory을 sustain한다.
	this->sustain(level);
}

void factory::manager::sustain(eFACTORY_GC_LEVEL _level)
{
	// declare) 
	std::vector<object_ptr<Ifactory>> vector_factory;

	// 1) 일단 Factory의 리스트를 먼저 복사한다.
	scoped_shared_lock(this->m_lockable_list_factory)
	{
		// check) 
		RETURN_IF(this->m_list_factory.empty());

		// - reserve
		vector_factory.reserve(this->m_list_factory.size());

		// - copy factory list
		for (const auto& iter : this->m_list_factory)
		{
			vector_factory.push_back(iter);
		}
	}

	// 2) sustain을 수행한다.
	for(auto& iter: vector_factory)
	{
		// - sustain
		iter->sustain(_level);
	}

	// 3) statistics store
	//for (auto& iter : vector_factory)
	//{
	//	// - get index
	//	auto index = g_statistics_time;

	//	// - sustain
	//	iter->statistics_store(index);
	//}

	// 4) increase sustain index;
	++m_s_sustain_serial_now;
}

void factory::manager::sustain_tls()
{
	// 1) get instance
	auto pinstance = instance();

	// declare) 
	std::vector<object_ptr<Ifactory>> vector_factory;

	// 2) 일단 Factory의 리스트를 먼저 복사한다.
	scoped_shared_lock(pinstance->m_lockable_list_factory)
	{
		// check) 
		RETURN_IF(pinstance->m_list_factory.empty());

		// - reserve
		vector_factory.reserve(pinstance->m_list_factory.size());

		// - copy factory list
		for (const auto& iter : pinstance->m_list_factory)
		{
			vector_factory.push_back(iter);
		}
	}

	// 3) sustain을 수행한다.
	for (auto& iter : vector_factory)
	{
		iter->process_tls_garbage_collection();
	}

	// 4) update sustain serial
	m_s_sustain_serial = m_s_sustain_serial_now;
}


void factory::manager::enable_sustain(bool _state) noexcept
{
	// check) 
	RETURN_IF(_state == this->m_flag_sustain);

	// 1) sustain
	this->m_flag_sustain = _state;

	// Message) View에 통보한다.
	this->transmit_message(sMESSAGE(eMESSAGE::FACTORY::FACTORY_UPDATE));
}

void factory::manager::initialize_instance()
{
	scoped_lock(m_s_lockable_create)
	{ 
		// check) 
		RETURN_IF(m_s_pinstance.exist());
		
		// 1) create instance
		m_s_pinstance = _make_own_raw<manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}

void factory::manager::set_instance(own_ptr<manager>&& _pinstance)
{
	// declare)
	own_ptr<manager> pinstance_pre;

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

void factory::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	scoped_lock(m_s_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

void factory::manager::print_statistics(bool _using_only) noexcept
{
	try
	{
		auto pinstance = instance();

		// 2) alloc shared_buffer for log text
		auto buf_output = alloc_shared_buffer((static_cast<size_t>(pinstance->size()) + 4) * 512);

		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		buf_output.append_text("\x1b[37m" "{:30s} {:>3s} {:11s} {:11s}{:>10s}{:>10s}{:>10s}{:>13s}{:>13s}{:>13s}{:>10s}{:>10s}{:>13s}{:>13s}{:>9s}{:>9s}{:>9s}\n" "\x1b[0m"sv,
			"name"sv,
			"id"sv,
			"type"sv,
			"GC_type"sv,
			"in_using"sv,
			"stk.pool"sv,
			"stk.tls"sv,
			"alloc.new"sv,
			"alloc.pool"sv,
			"alloc.tls"sv,
			"thr.pool"sv,
			"thr.tls"sv,
			"free.del"sv,
			"free.tls"sv,
			"r.pool"sv,
			"r.tls"sv,
			"r.D/C"sv
		);
		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		// 1) 일단 Factory의 리스트를 먼저 복사한다.
		scoped_shared_lock(pinstance->m_lockable_list_factory)
		{
			for (auto& iter : pinstance->m_list_factory)
			{
				// check)
				CGASSERT_ERROR(iter != nullptr);

				// check)
				CONTINUE_IF(iter == nullptr);

				// - get statistics
				auto& statistics = iter->statistics();

				std::string_view factory_name = (statistics.name.size() < 30) ? std::string_view(statistics.name) : std::string_view(statistics.name.c_str(), 30);

				uint64_t alloc_create = statistics.now.alloc_create;
				bool is_unused = false;

				std::string_view str_gc_type;

				if(statistics.type >= eFACTORY_TYPE::POOL_MEMORYBLOCK && statistics.type <= eFACTORY_TYPE::POOL_USER)
					str_gc_type = to_string<char>(statistics.gc_type);
				else
					str_gc_type = ""sv;

				// "1. name" "2. id" "3. type" 
				if (statistics.type == eFACTORY_TYPE::POOL_USER || statistics.type == eFACTORY_TYPE::ALLOCATOR_USER)
				{
					if (alloc_create != 0)
					{
						buf_output.append_text("\x1b[92m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string<char>(statistics.type), str_gc_type);
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text("\x1b[32m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string<char>(statistics.type), str_gc_type);
						is_unused = true;
					}
				}
				else
				{
					if (alloc_create != 0)
					{
						buf_output.append_text("\x1b[37m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string<char>(statistics.type), str_gc_type);
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text("\x1b[90m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string<char>(statistics.type), str_gc_type);
						is_unused = true;
					}
				}

				if (iter->type() <= eFACTORY_TYPE::POOL_USER)
				{
					// - "4. in_using"
					{
						int64_t count = statistics.now.in_using;

						if (count != 0)
						{
							buf_output.append_text(" {:>9}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv);
						}
					}

					// - "5. stk.pool"
					{
						if (iter->stacked_count() != 0)
						{
							buf_output.append_text(" {:>9}", iter->stacked_count());
						}
						else
						{
							buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv);
						}
					}

					// - "6. stk.tls"
					{
						int64_t count = statistics.now.tls_stack_depth;

						if (is_unused == false)
						{
							buf_output.append_text(" {:>9}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv, count);
						}
					}

					// - "7. alloc.new"
					{
						auto count = alloc_create;

						if (count != 0)
						{
							buf_output.append_text(" {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "8. alloc.pool"
					{
						uint64_t count = statistics.now.alloc_stack; // alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls

						if (count != 0)
						{
							buf_output.append_text(" {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "9. alloc.tls"
					{
						int64_t count = statistics.now.alloc_stack_tls;

						if (count != 0)
						{
							buf_output.append_text(" {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "9. thr.pool"
					{
						int64_t existing = statistics.now.existing;
						auto existing_limits = statistics.now.existing_limits;
						if (existing_limits != INT64_MAX && is_unused == false)
						{
							if (existing <= existing_limits)
							{
								buf_output.append_text(" {:>9}"sv, existing_limits);
							}
							else
							{
								buf_output.append_text("\x1b[31m" " {:>9}" "\x1b[0m"sv, existing_limits);
							}
						}
						else
						{
							buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv, existing_limits);
							existing_limits = 0;
						}
					}

					// 10. threshold.tls
					{
						int64_t count = statistics.now.tls_stack_depth_threshold;

						if (count != INT64_MAX && is_unused == false)
						{
							buf_output.append_text(" {:>9}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv, count);
						}
					}

					// - "11. free.del"
					{
						uint64_t count = statistics.now.free_delete;

						if (count != 0)
						{
							buf_output.append_text(" {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "12. free.tls"
					{
						int64_t count = statistics.now.free_stack_tls;

						if (count != 0)
						{
							buf_output.append_text(" {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "12. ratio.pool"
					{
						char temp_buf_reuse_reate[64]{ 0 };
						std::string_view str_reuse_reate;

						int64_t	f_allocated_total = alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls;
						if (f_allocated_total)
						{
							auto length = __sprintf(temp_buf_reuse_reate, "%6.3f%%"sv, (float)statistics.now.alloc_stack / (float)f_allocated_total * 100.0f);

							if (length > 0)
							{
								str_reuse_reate = std::string_view{ temp_buf_reuse_reate , static_cast<size_t>(length) };
							}
						}
						else
						{
							str_reuse_reate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
						}

						buf_output.append_text(" {:>8}"sv, str_reuse_reate);
					}

					// - "13. ratio.tls"
					{
						char temp_buf_reuse_reate[64]{ 0 };
						std::string_view str_reuse_reate;

						int64_t	f_allocated_total = alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls;
						if (f_allocated_total)
						{
							auto length = __sprintf(temp_buf_reuse_reate, "%6.3f%%"sv, (float)statistics.now.alloc_stack_tls / (float)f_allocated_total * 100.0f);

							if (length > 0)
							{
								str_reuse_reate = std::string_view{ temp_buf_reuse_reate , static_cast<size_t>(length) };
							}
						}
						else
						{
							str_reuse_reate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
						}

						buf_output.append_text(" {:>8}"sv, str_reuse_reate);
					}

					// - "14. ratio.D/C"
					{
						char temp_buf_destroy_reate[64]{ 0 };
						std::string_view str_destroy_reate;
						if (alloc_create != 0)
						{
							auto length = __sprintf(temp_buf_destroy_reate, "%6.3f%%"sv, (float)statistics.now.free_delete / alloc_create * 100.0f);

							if (length > 0)
							{
								str_destroy_reate = std::string_view{ temp_buf_destroy_reate , static_cast<size_t>(length) };
							}
						}
						else
						{
							str_destroy_reate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
						}

						buf_output.append_text(" {:>8}"sv, str_destroy_reate);
					}
				}
				else
				{
					// - "4. in using"
					{
						int64_t count = statistics.now.in_using;

						if (count != 0)
						{
							buf_output.append_text(" {:>9}"sv, count);
						}
						else
						{
							buf_output.append_text( "\x1b[90m" "         0" "\x1b[0m"sv );
						}
					}

					// - "7. alloc.new"
					{
						// check)
						CGASSERT_ERROR(statistics.now.alloc_stack == 0);
						CGASSERT_ERROR(statistics.now.alloc_stack_tls == 0);

						uint64_t count = alloc_create;

						if (count != 0)
						{
							buf_output.append_text("          " "          " " {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          " "          " "            0" "\x1b[0m"sv);
						}
					}

					// - "11. free.del"
					{
						uint64_t count = statistics.now.free_delete;

						if (count != 0)
						{
							buf_output.append_text("             " "             " "          " "          " " {:>12}"sv, count);
						}
						else
						{
							buf_output.append_text("\x1b[90m" "             " "             " "          " "          " "            0" "\x1b[0m"sv);
						}
					}
				}

				buf_output.append<char>('\n');
			}
		}
		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		// 2) 최종 출력
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


}