#include "pch.h"

using namespace CGDK;


query::manager_querier::manager_querier() : m_db_querier_serial(0)
{
#if defined(_WIN32)
	// 1) coUninitialize
	::CoUninitialize();
#elif defined(__linux__)
#else
	#error "CGDK Not supported platform"
#endif
}

query::manager_querier::~manager_querier()
{
	// check) 
	CGASSERT_ERROR(m_list_querier.empty());
}

void query::manager_querier::initialize_instance()
{
	{ 
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_create);

		// check) 기존에 Instance가 있는지 확인한다.
		RETURN_IF(m_pinstance.exist())

		// 1) CoInitialize한다.
	#if defined(_WIN32)
		//auto	result	 = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		auto result = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);

		// check) 
		THROW_IF(
			result != S_OK &&
			result != S_FALSE,		// S_FALSE indicates that the COM library is already initialized.
			throwable::failure(eRESULT::BAD_FUNCTION_CALL));
	#elif defined(__linux__)
	#else
		#error "CGDK Not supported platform"
	#endif

		// 1)새로운 Instance를 생성한다.
		set_instance(_make_own_raw<query::manager_querier>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}

void query::manager_querier::set_instance(own_ptr<manager_querier>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_querier> pinstance_pre;

	scoped_lock(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void query::manager_querier::reset_instance() noexcept
{
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_create);

		m_pinstance.reset();
	}
}


result_code query::manager_querier::destroy() noexcept
{
	{ 
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_create);
	}

	// return)
	return eRESULT::SUCCESS;
}

void query::manager_querier::register_querier(Iquerier* _pquerier)
{
	// 1) add_reference한다.
	add_ref();

	scoped_lock(m_lockable_list_querier)
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
	scoped_lock(m_lockable_list_querier)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_connective = std::find(m_list_querier.begin(), m_list_querier.end(), _pquerier);

		// check) 해당 connectie가 없으면 제거한다.
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
