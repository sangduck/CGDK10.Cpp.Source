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
	scoped_lock(m_lockable_create)
	{ 
	}

	// return)
	return eRESULT::SUCCESS;
}

void net::io::connective::manager::join( net::io::Iconnective* _pconnective)
{
	// 1) add_reference한다.
	this->add_ref();

	scoped_unique_lock(this->m_lockable_list_connective)
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
	scoped_unique_lock(this->m_lockable_list_connective)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_connective = std::find(this->m_list_connective.begin(), this->m_list_connective.end(), _pconnective);

		// check) 해당 Connectie가 없으면 제거한다.
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
	scoped_lock(m_lockable_create)
	{
		RETURN_IF(m_pinstance.exist());
		
		m_pinstance = _make_own_raw<manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	} 
}
void net::io::connective::manager::set_instance(own_ptr<manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager> pinstance_pre;

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

void net::io::connective::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	scoped_lock(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}
