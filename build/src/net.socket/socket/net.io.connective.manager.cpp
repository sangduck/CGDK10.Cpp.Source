#include "pch.h"

using namespace CGDK;


net::io::connective::manager::manager() : 
	Nnameable(),
	m_connective_serial(0)
{
}

net::io::connective::manager::~manager() noexcept
{
	// check) manager가 파괴될땐 분명 Pool이 하나도 없어야 한다.
	CGASSERT_ERROR(this->m_list_connective.empty());
}

result_code net::io::connective::manager::destroy() noexcept
{
	// 1) ...
	SCOPED_LOCK(m_lockable_create)
	{ 
	}

	// return)
	return eRESULT::SUCCESS;
}

void net::io::connective::manager::join( net::io::Iconnective* _pconnective)
{
	// 1) add_reference한다.
	this->add_ref();

	SCOPED_UNIQUE_LOCK(this->m_lockable_list_connective)
	{
		// 1) 같은 것이 있는지 먼저 검사한다.
		auto iter_connective = std::find(this->m_list_connective.begin(), this->m_list_connective.end(), _pconnective);

		// check) 찾지 못했다면 끝낸다.
		RETURN_IF(iter_connective != this->m_list_connective.end());

		// 2) connective ID
		_pconnective->m_connective_id = this->m_connective_serial++;

		// 3) Pool 관리 List에 추가한다.
		this->m_list_connective.push_back(_pconnective);
	}

	// notify)
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER, 0, reinterpret_cast<uintptr_t>(_pconnective)));
}

void net::io::connective::manager::leave( net::io::Iconnective* _pconnective) noexcept
{
	SCOPED_UNIQUE_LOCK(this->m_lockable_list_connective)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_connective = std::find(this->m_list_connective.begin(), this->m_list_connective.end(), _pconnective);

		// check) 해당 Connective가 없으면 제거한다.
		RETURN_IF(iter_connective == this->m_list_connective.end());

		// 2) 리스트에서 제거한다.
		this->m_list_connective.erase(iter_connective);
	}

	// notify)
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pconnective)));

	// 4) release
	this->release();
}

void net::io::connective::manager::initialize_instance()
{
	SCOPED_LOCK(m_lockable_create)
	{
		RETURN_IF(m_pinstance.exist());
		
		m_pinstance.exchange_release(_make_own_raw<manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	} 
}
void net::io::connective::manager::set_instance(own_ptr<manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager> pinstance_pre;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void net::io::connective::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}

void net::io::connective::manager::print_statistics() noexcept
{
	try
	{
			// 1) get instance
			auto temp_instance = instance();

			// 2) alloc shared_buffer for log text
			auto buf_output = alloc_shared_buffer((static_cast<size_t>(temp_instance->size()) + 4) * 256);

			buf_output.append_text("\x1b[37m" "-----------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");
			buf_output.append_text(std::format("\x1b[37m" "{:30s} {:>3s} {:>11s}{:>11s}{:>11s}{:>11s}{:>11s}{:>11s}{:>11s}\n" "\x1b[0m"sv,
				"name"sv,
				"id"sv,
				"try"sv,
				"con.succ"sv,
				"con.fail"sv,
				"discon"sv,
				"err.dis"sv,
				"now"sv,
				"sync"sv)
			);
			buf_output.append_text("\x1b[90m" "-----------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

			SCOPED_SHARED_LOCK(temp_instance->m_lockable_list_connective)
			{
				for (auto iter : temp_instance->m_list_connective)
				{
					// check)
					CGASSERT_ERROR(iter != nullptr);

					// check)
					CONTINUE_IF(iter == nullptr);

					// - get statistics
					auto& statistics = iter->statistics_get_connection_info();

					auto name = iter->name();

					std::string_view connective_name = (name.size() < 30) ? std::string_view(name) : std::string_view(name.data(), 30);

					// - "1. name"
					// - "2. id"
					buf_output.append_text(std::format("\x1b[92m" "{:30s} {:>3}" "\x1b[0m"sv, connective_name, iter->get_connective_id()));

					// - "3. try"
					{
						auto count = statistics.count_try.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 12));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "           0" "\x1b[0m"sv);
						}
					}

					// - "4. success_connect"
					{

						auto count = statistics.count_success_connect.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}

					// - "5. count_fail_connect"
					{
						auto count = statistics.count_fail_connect.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}

					// - "6. count_disconnect"
					{
						auto count = statistics.count_disconnect.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}

					// - "7. count_error_disconnect"
					{

						auto count = statistics.count_error_disconnect.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}

					// - "8. count_keep"
					{
						auto count = statistics.count_keep.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}
					// - "9. count_async"
					{
						auto count = statistics.count_async.load();
						if (count != 0)
						{
							buf_output.append_text(CGDK::stream_width(count, 11));
						}
						else
						{
							buf_output.append_text("\x1b[90m" "          0" "\x1b[0m"sv);
						}
					}

					// - new line~
					buf_output.append_text("\n");
				}
			}
			buf_output.append_text("\x1b[37m" "-----------------------------------------------------------------------------------------------------------------\n" "\x1b[0m");

			// 3) 최종 출력
		#if defined(_WIN32)
			DWORD number_written = 0;
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buf_output.data(), buf_output.size<DWORD>(), &number_written, nullptr);
		#else
			fwrite(buf_output.data(), 1, buf_output.size<int>(), stdout);
			fflush(stdout);
		#endif
	}
	catch (...)
	{
	}
}