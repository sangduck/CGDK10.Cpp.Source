#include "pch.h"

namespace CGDK
{

void Ilogger::detach_from_parent() noexcept
{
	// 1) get parent logger
	auto pparent = m_pparent;

	// 2) detach from parents
	if (pparent != nullptr)
	{
		pparent->detach(this);
	}
}

void Ilogger::process_count_log(log_type_t _type)
{
	// declare) 
	uint32_t type = static_cast<uint32_t>(_type) & 0xffff;

	scoped_lock(m_lockable_count_log)
	{
		// - log의 갯수를 계산한다.
		++m_count_log_total;

		// - log type의 수를 증가시킨다.
		if (type >= (uint32_t)log_type_t::info && type < (uint32_t)log_type_t::max_)
		{
			++m_count_log.at(type);
		}
	}
}


}