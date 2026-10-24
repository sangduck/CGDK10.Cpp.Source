#include "pch.h"

namespace CGDK
{

void _print_type_pool_not_user(shared_buffer& buf_output, const Ifactory* iter, bool is_unused);

void factory::manager::print_statistics(bool _using_only) noexcept
{
	try
	{
		auto pinstance = instance();

		// 2) alloc shared_buffer for log text
		auto buf_output = alloc_shared_buffer((static_cast<size_t>(pinstance->size()) + 4) * 512);

		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		buf_output.append_text(std::format("\x1b[37m" "{:30s} {:>3s} {:11s} {:11s}{:>10s}{:>10s}{:>10s}{:>13s}{:>13s}{:>13s}{:>10s}{:>10s}{:>13s}{:>13s}{:>9s}{:>9s}{:>9s}\n" "\x1b[0m"sv,
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
			"r.D/C"sv)
		);
		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		// 1) 일단 Factory의 리스트를 먼저 복사한다.
		SCOPED_SHARED_LOCK(pinstance->m_lockable_list_factory)
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
					str_gc_type = to_string(statistics.gc_type);
				else
					str_gc_type = ""sv;

				// "1. name" "2. id" "3. type" 
				if (statistics.type == eFACTORY_TYPE::POOL_USER || statistics.type == eFACTORY_TYPE::ALLOCATOR_USER)
				{
					if (alloc_create != 0)
					{
						buf_output.append_text(std::format("\x1b[92m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string(statistics.type), str_gc_type));
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text(std::format("\x1b[32m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string(statistics.type), str_gc_type));
						is_unused = true;
					}
				}
				else
				{
					if (alloc_create != 0)
					{
						buf_output.append_text(std::format("\x1b[37m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string(statistics.type), str_gc_type));
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text(std::format("\x1b[90m" "{:30s} {:>3} {:11s} {:11s}" "\x1b[0m"sv, factory_name, statistics.id, to_string(statistics.type), str_gc_type));
						is_unused = true;
					}
				}

				if (iter->type() <= eFACTORY_TYPE::POOL_USER)
				{
					_print_type_pool_not_user(buf_output, iter, is_unused);
				}
				else
				{
					// - "4. in using"
					{
						int64_t count = statistics.now.in_using;

						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 10));
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
							buf_output.append_text(std::format("          " "          " " {:>12}"sv, count));
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
							buf_output.append_text(std::format("             " "             " "          " "          " " {:>12}"sv, count));
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