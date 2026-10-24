#include "pch.h"

using namespace CGDK;

#ifdef NDEBUG
	#define	DEFAULT_ENABLE_EXCEPTION_HANDLING	true
#else
	#define	DEFAULT_ENABLE_EXCEPTION_HANDLING	false
#endif 


executor::manager::manager() noexcept
{
	this->name("executor manager"sv);
}

executor::manager::~manager() noexcept
{
}

void executor::manager::initialize_instance()
{
	// lock)
	std::lock_guard<std::mutex> lock(m_s_lockable_create);

	// check)
	if(m_s_pinstance.exist())
		return;
		
	m_s_pinstance.exchange_release(_make_own_raw<executor::manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
}

void executor::manager::set_instance(own_ptr<manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager> pinstance_pre;

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

void executor::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	{
		// lock)
		std::lock_guard<std::mutex> lock(m_s_lockable_create);

		// 1) move instance
		pinstance = std::move(m_s_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

void executor::manager::print_statistics(bool _using_only) noexcept
{
	try
	{
		// 1) get instance
		auto pinstance = instance();

		// 2) alloc shared_buffer for log text
		auto buf_output = alloc_shared_buffer((static_cast<size_t>(pinstance->size()) + 4) * 256);

		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		buf_output.append_text(std::format("\x1b[37m" "{:30s} {:8s}{:>5s}{:>9s}{:>13s}{:>13s}\n" "\x1b[0m",
			"name",
			"type",
			"id",
			"thread",
			"executed",
			"pended")
		);
		buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		SCOPED_SHARED_LOCK(pinstance->m_lockable_list_executor)
		{
			for (auto& iter : pinstance->m_list_executor)
			{
				// check)
				CGASSERT_ERROR(iter != nullptr);

				// check)
				CONTINUE_IF(iter == nullptr);

				// - get statistics
				auto& statistics = iter->statistics();

				std::string_view executor_name = (statistics.name.size() < 30) ? std::string_view(statistics.name) : std::string_view(statistics.name.c_str(), 30);

				auto alloc_execute = statistics.now.count_execute.load();

				if (statistics.type == eEXECUTOR_TYPE::USER)
				{
					// "1. name" "2. type"
					if (alloc_execute != 0)
					{
						buf_output.append_text(std::format("\x1b[92m" "{:30s} {:8s}" "\x1b[0m"sv, executor_name, to_string(statistics.type)));
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text(std::format("\x1b[32m" "{:30s} {:8s}" "\x1b[0m"sv, executor_name, to_string(statistics.type)));
					}

					// - "3. id"
					{
						auto count = statistics.id;

						if (count != 0)
						{
							buf_output.append_text(std::format(" {:>4}"sv, count));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "    0" "\x1b[0m"sv);
						}
					}

					// - "4. thread_count"
					{
						if (statistics.now.thread_count != 0)
						{
							buf_output.append_text(std::format(" {:>8}", statistics.now.thread_count.load()));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "        0" "\x1b[0m"sv);
						}
					}

					// - "5. executed"
					{
						auto count = statistics.now.count_execute.load();

						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 10));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "6. pended"
					{
						auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

						if (pqueue != nullptr)
						{
							auto count = pqueue->pended_count();

							if (count != 0)
							{
								buf_output.append_text(CGDK::stream_width(count, 13));
							}
							else
							{
								buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
							}
						}
					}

					// - new line~
					buf_output.append_text("\n"sv);
				}
				else
				{
					// "1. name" "2. type"
					if (alloc_execute != 0)
					{
						buf_output.append_text(std::format("\x1b[37m" "{:30s} {:8s}" "\x1b[0m"sv, executor_name, to_string(statistics.type)));
					}
					else
					{
						if (_using_only == true)
							continue;

						buf_output.append_text(std::format("\x1b[90m" "{:30s} {:8s}" "\x1b[0m"sv, executor_name, to_string(statistics.type)));
					}

					// - "3. id"
					{
						auto count = statistics.id;

						if (count != 0)
						{
							buf_output.append_text(std::format(" {:>4}"sv, count));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "    0" "\x1b[0m"sv);
						}
					}

					// - "4. thread_count"
					{
						if (statistics.now.thread_count != 0)
						{
							buf_output.append_text(CGDK::stream_width(statistics.now.thread_count.load(), 9));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "        0" "\x1b[0m"sv);
						}
					}

					// - "5. executed"
					{
						auto count = statistics.now.count_execute.load();

						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 13));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
						}
					}

					// - "6. pended"
					{
						auto* pqueue = dynamic_cast<executor::Iqueue*>(iter);

						if (pqueue != nullptr)
						{
							auto count = pqueue->pended_count();

							if (count != 0)
							{
								buf_output.append_text(CGDK::stream_width(count, 13));
							}
							else
							{
								buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
							}
						}
					}

					// - new line~
					buf_output.append_text("\n");
				}
			}
		}
		buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

		// 3) 최종 출력
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

result_code executor::manager::destroy() noexcept
{
	return result_code(eRESULT::SUCCESS);
}

bool executor::manager::process_register_executor(Iexecutor* _pexecutor)
{
	SCOPED_UNIQUE_LOCK(this->m_lockable_list_executor)
	{
		// 1) 이미 등록되어 이는지 확인한다.
		auto iter_thread = std::find(this->m_list_executor.begin(), this->m_list_executor.end(), _pexecutor);

		// check) 이미 등록되어 있으면 끝낸다.
		if(iter_thread != this->m_list_executor.end())
			return false;

		// 2) set executor id
		_pexecutor->m_statistics.id = m_executor_serial++;

		// 3) Thread를 List에 추가된다.
		this->m_list_executor.push_back(_pexecutor);
	}

	// 3) view에 통보한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::EXECUTOR_REGISTER, 0, reinterpret_cast<uintptr_t>(_pexecutor)));

	// return) 
	return true;
}

bool executor::manager::process_unregister_executor(Iexecutor* _pexecutor) noexcept
{
	SCOPED_UNIQUE_LOCK(this->m_lockable_list_executor)
	{ 
		// 1) Thread를 List에서 찾는다.
		auto iter_thread = std::find(this->m_list_executor.begin(), this->m_list_executor.end(), _pexecutor);

		// check) 찾지 못했으면 그냥 끝낸다.
		if(iter_thread == this->m_list_executor.end())
			return false;

		// 2) Thread를 지운다.
		this->m_list_executor.erase(iter_thread);
	}

	// 3) view에 통보한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::EXECUTOR_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pexecutor)));

	// return) 
	return true;
}
