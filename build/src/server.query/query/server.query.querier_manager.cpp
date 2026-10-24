#include "pch.h"

using namespace CGDK;


query::manager_querier::manager_querier() : m_db_querier_serial(0)
{
}

query::manager_querier::~manager_querier()
{
	// check) 
	CGASSERT_ERROR(m_list_querier.empty());
}

void query::manager_querier::initialize_instance()
{
	// SCOPED_LOCK) 
	std::lock_guard cs(m_lockable_create);

	// check) 기존에 Instance가 있는지 확인한다.
	RETURN_IF(m_pinstance.exist())

	// 1)새로운 Instance를 생성한다.
	set_instance(_make_own_raw<query::manager_querier>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
}

void query::manager_querier::set_instance(own_ptr<manager_querier>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_querier> pinstance_pre;

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

void query::manager_querier::reset_instance() noexcept
{
	// lock) 
	std::lock_guard cs(m_lockable_create);

	m_pinstance.reset();
}

void query::manager_querier::print_statistics() noexcept
{
	try
	{
		// 1) get instance
		auto temp_instance = instance();

		// declare)
		shared_buffer buf_output;

		SCOPED_LOCK(temp_instance->m_lockable_list_querier)
		{
			// 2) alloc shared_buffer for log text
			buf_output = alloc_shared_buffer((static_cast<size_t>(temp_instance->m_list_querier.size()) + 6) * 256);

			// 3) title
			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
			buf_output.append_text(std::format("\x1b[37m" "{:30s}{:>5s}{:>13s}{:>13s}{:>9s}{:>9s}{:>12s}{:>10s}{:>10s}\n" "\x1b[0m",
				"name",
				"id",
				"try",
				"success",
				"fail",
				"retry",
				"processing",
				"session",
				"using")
			);
			buf_output.append_text("\x1b[90m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);

			// 4) queriers
			for (auto& iter : temp_instance->m_list_querier)
			{
				// check)
				CGASSERT_ERROR(iter != nullptr);

				// check)
				CONTINUE_IF(iter == nullptr);

				// - "1. name"
				std::string str_name;
				if (auto* pnameable = dynamic_cast<object::Inameable*>(iter); pnameable != nullptr)
					str_name = std::string(pnameable->name());
				if (str_name.empty())
					str_name = std::format("querier {}", iter->m_id_querier);
				if (str_name.size() > 30)
					str_name.resize(30);

				// - statistics
				auto count_try = iter->statistics_get_query_try();
				auto count_fail = iter->statistics_get_query_fail();

				// - color : processed(green) / failed(yellow) / not used(gray)
				std::string_view color = (count_fail != 0) ? "\x1b[93m"sv : (count_try != 0) ? "\x1b[92m"sv : "\x1b[90m"sv;

				buf_output.append_text(std::format("{}" "{:30s}{:>5}{:>13}{:>13}{:>9}{:>9}{:>12}{:>10}{:>10}" "\x1b[0m\n",
					color,
					str_name,
					iter->m_id_querier,
					count_try,
					iter->statistics_get_query_success(),
					count_fail,
					iter->statistics_get_query_retry(),
					iter->statistics_get_query_processing(),
					iter->statistics_get_exist_session(),
					iter->statistics_get_using_session())
				);
			}

			// - no querier
			if (temp_instance->m_list_querier.empty())
				buf_output.append_text("\x1b[90m" "(no querier)\n" "\x1b[0m"sv);

			buf_output.append_text("\x1b[37m" "---------------------------------------------------------------------------------------------------------------------------------\n" "\x1b[0m"sv);
		}

		// 5) output
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

result_code query::manager_querier::destroy() noexcept
{
	// lock)
	std::lock_guard cs(m_lockable_create);

	// return)
	return eRESULT::SUCCESS;
}

void query::manager_querier::register_querier(Iquerier* _pquerier)
{
	// 1) add_reference한다.
	add_ref();

	SCOPED_LOCK(m_lockable_list_querier)
	{
		// 2) 같은 것이 있는지 먼저 검사한다.
		auto iter_connective = std::find(m_list_querier.begin(), m_list_querier.end(), _pquerier);

		// check) 찾지 못했다면 끝낸다.
		RETURN_IF(iter_connective != m_list_querier.end());

		// 3) connective ID
		_pquerier->m_id_querier = m_db_querier_serial++;

		// 4) Pool 관리 List에 추가한다.
		m_list_querier.push_back(_pquerier);
	}

	// 5) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::REGISTER, 0, reinterpret_cast<uintptr_t>(_pquerier)));
}

void query::manager_querier::unregister_querier(Iquerier* _pquerier) noexcept
{
	SCOPED_LOCK(m_lockable_list_querier)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_connective = std::find(m_list_querier.begin(), m_list_querier.end(), _pquerier);

		// check) 해당 connective가 없으면 제거한다.
		RETURN_IF(iter_connective == m_list_querier.end());

		// 2) 리스트에서 제거한다.
		m_list_querier.erase(iter_connective);
	}

	// 3) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::QUERY::UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pquerier)));

	// 4) Release한다.
	release();
}

result_code query::manager_querier::process_message(sMESSAGE& _msg)
{
	// 1) Log Message Dispatch
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}
