#include "pch.h"

namespace CGDK
{


void Ifactory::process_set_name(std::string_view _name)
{
    this->m_statistics.name = _name;
}

void Ifactory::process_register_factory()
{
	// declare) 
	object_ptr<factory::manager> pmanager = factory::manager::instance();

	// check) pmanager
	CGASSERT_ERROR(pmanager.exist());

	// check) pmanager가 nullptr이면 끝낸다.
	if(pmanager.empty())
		return;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		// 1) Factory Manager에 등록한다.
		auto result = pmanager->process_register_factory(this);

		// check) 
		if(result == false)
			return;

		// 2) Manager를 등록한다.
		this->m_pmanager = pmanager;
	}
}

void Ifactory::process_unregister_factory() noexcept
{
	// declare) 
	object_ptr<factory::manager> pmanager;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager);

		// check) 
		if(this->m_pmanager.empty())
			return;

		// 1) UnRegister한다.
		this->m_pmanager->process_unregister_factory(this);

		// 2) reset한다.
		pmanager = std::move(this->m_pmanager);
	}
}

void Ifactory::process_reset_statistics() noexcept
{
	// 1) reset now statistics
	this->m_statistics.now.reset();
}

size_t Ifactory::_get_shrink_count(eFACTORY_GC_LEVEL _level) const noexcept
{
	size_t amount_shrink = 0;

	switch(_level)
	{
	// - 최대개수를 초과하는 객체수의 1/16만을 모두 제거한다.
	case	eFACTORY_GC_LEVEL::NORMAL:
			if(this->m_statistics.now.existing > this->m_statistics.now.existing_limits)
			{
				amount_shrink = static_cast<size_t>((this->m_statistics.now.existing - this->m_statistics.now.existing_limits)>>4);
			}
			break;

	// - 최대개수를 초과하는 객체수의 1/4만을 모두 제거한다.
	case	eFACTORY_GC_LEVEL::HEAVY:
			if(this->m_statistics.now.existing > this->m_statistics.now.existing_limits)
			{
				amount_shrink = static_cast<size_t>((this->m_statistics.now.existing - this->m_statistics.now.existing_limits)>>2);
			}
			break;

	// - 최대개수를 초과하는 객체 개수 모두 만큼을 제거한다.
	case	eFACTORY_GC_LEVEL::EXCESS_ALL:
			if(this->m_statistics.now.existing > this->m_statistics.now.existing_limits)
			{
				amount_shrink = static_cast<size_t>(this->m_statistics.now.existing - this->m_statistics.now.existing_limits);
			}
			break;

	// - Stack된 것은 모조리 제거한다.
	case	eFACTORY_GC_LEVEL::STACKED_ALL:
			{
				amount_shrink = this->stacked_count();
			}
			break;

	// error) 
	default:
			CGASSERT_ERROR(false);
			break;
	};

	// return)
	return amount_shrink;
}


}