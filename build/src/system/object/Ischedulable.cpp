#include "pch.h"

namespace CGDK
{

Ischedulable::Ischedulable() noexcept
{
	this->m_statistics_schedulable.id = reinterpret_cast<uint64_t>(this);
}

}