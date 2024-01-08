#include "pch.h"

namespace CGDK
{


doss::manager_repository::manager_repository()
{
	// Trace) Event Manager를 생성한다.
	LOG_INFO << "(info) doss REPOSITORY_MANAGER is created";
}

doss::manager_repository::~manager_repository() noexcept
{
	// check) Manager가 파괴될땐 분명 Pool이 하나도 없어야 한다.
	CGASSERT_ERROR(m_list_repository.empty());

	// Trace) Event Manager를 파괴힌다.
	LOG_INFO << "(info) doss REPOSITORY_MANAGER is destroyed";
}

void doss::manager_repository::set_instance(own_ptr<manager_repository>&& _pInstance)
{
	scoped_lock(m_lockable_create)
	{
		// 1) 값을 설정한다.
		m_pinstance = std::move(_pInstance);
	}
}

void doss::manager_repository::reset_instance() noexcept
{
	scoped_lock(m_lockable_create)
	{
		// 1) Reset한다.
		m_pinstance.reset();
	}
}

void doss::manager_repository::init_instance()
{
	scoped_lock(m_lockable_create)
	{ 
		// check) 기존에 Instance가 있는지 확인한다.
		RETURN_IF(m_pinstance.exist())

		// 1)새로운 Instance를 생성한다.
		set_instance(make_own<doss::manager_repository>());
	}
}

result_code doss::manager_repository::destroy() noexcept
{
	// 1) 
	scoped_lock(m_lockable_create)
	{ 
	}

	// return)
	return eRESULT::SUCCESS;
}

void doss::manager_repository::register_repository(Irepository* _prepository)
{
	// 1) AddReference한다.
	add_ref();

	scoped_lock(m_list_repository)
	{
		// 2) 같은 것이 있는지 먼저 검사한다.
		auto iter_connective = std::find(m_list_repository.begin(), m_list_repository.end(), _prepository);

		// check) 찾지 못했다면 끝낸다.
		RETURN_IF(iter_connective!=m_list_repository.end());

		// 3) repository id를 얻는다.
		sOBJECT_ID repository_id = m_repository_serial++;;
		
		// 3) set repository_id
		_prepository->set_object_id(repository_id);

		// 4) add repository...
		m_list_repository.push_back(_prepository);
	}

	// 5) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::DOSS::REGISTER, 0, reinterpret_cast<uintptr_t>(_prepository)));
}

void doss::manager_repository::unregister_repository(Irepository* _prepository) noexcept
{
	scoped_lock(m_list_repository)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_connective = std::find(m_list_repository.begin(), m_list_repository.end(), _prepository);

		// check) 해당 Connectie가 없으면 제거한다.
		RETURN_IF(iter_connective == m_list_repository.end());

		// 2) 리스트에서 제거한다.
		m_list_repository.erase(iter_connective);
	}

	// 3) Message 날리기..
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::DOSS::UNREGISTER, 0, reinterpret_cast<uintptr_t>(_prepository)));

	// 4) release한다.
	release();
}

object_ptr<doss::Irepository> doss::manager_repository::find(sREPOSITORY_ID _rid) const noexcept
{
	// declare)
	object_ptr<doss::Irepository> prepository;

	scoped_lock(m_list_repository)
	{
		auto iter = m_list_repository.begin();
		auto iter_end = m_list_repository.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter)->get_repository_id() == _rid)
			{
				prepository = *iter;
				break;
			}
		}
	}

	// return) 
	return prepository;
}

object_ptr<doss::Irepository> doss::manager_repository::find(const std::string_view _name) const noexcept
{
	// declare)
	object_ptr<doss::Irepository> prepository;

	scoped_lock(m_list_repository)
	{
		auto iter = m_list_repository.begin();
		auto iter_end = m_list_repository.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter)->is_this(_name))
			{
				prepository = *iter;
				break;
			}
		}
	}

	// return)
	return prepository;
}



result_code doss::manager_repository::process_message(sMESSAGE& _msg)
{
	// 1) Log Message Dispatch
	TRANSMIT_MESSAGE(_msg);

	// return)
	return eRESULT::BYPASS;
}


}