#include "pch.h"


bool CGDK::Iresource::register_()
{
	// declare) 
	auto pmanager = resource::manager::instance();

	// check) pmanager
	CGASSERT_ERROR(pmanager.exist());

	// check) pmanager가 nullptr이면 끝낸다.
	if(pmanager.empty())
		return false;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		// 1) Factory Manager에 등록한다.
		pmanager->register_resource(this);

		// 2) Manager를 등록한다.
		m_pmanager = pmanager;
	}

	// return)
	return true;
}

bool CGDK::Iresource::unregister() noexcept
{
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		// check) 
		RETURN_IF(m_pmanager.empty(), false);

		// 1) UnRegister한다.
		m_pmanager->unregister_resource(this);

		// 2) reset한다.
		m_pmanager.reset();
	}

	// return)
	return true;
}
