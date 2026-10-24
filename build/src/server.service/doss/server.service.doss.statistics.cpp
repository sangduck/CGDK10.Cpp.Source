#include "pch.h"

namespace CGDK
{
	
doss::statistics::repository::repository(const repository& _rhs) noexcept
{
	this->set(_rhs); 
}

void doss::statistics::repository::set(const repository& _rhs) noexcept
{
	// - snapshot (relaxed : 필드 사이의 일관성은 원래 없다)
	this->m_statistics_checkin.store(_rhs.m_statistics_checkin.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkin_request.store(_rhs.m_statistics_checkin_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkin_success.store(_rhs.m_statistics_checkin_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkin_fail.store(_rhs.m_statistics_checkin_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkout_request.store(_rhs.m_statistics_checkout_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkout_success.store(_rhs.m_statistics_checkout_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkout_fail.store(_rhs.m_statistics_checkout_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_checkout_forcible.store(_rhs.m_statistics_checkout_forcible.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_create_request.store(_rhs.m_statistics_create_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_create_success.store(_rhs.m_statistics_create_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_create_fail.store(_rhs.m_statistics_create_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_remove_request.store(_rhs.m_statistics_remove_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_remove_complete.store(_rhs.m_statistics_remove_complete.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_command_request.store(_rhs.m_statistics_command_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_command_success.store(_rhs.m_statistics_command_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_command_fail.store(_rhs.m_statistics_command_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_update_request.store(_rhs.m_statistics_update_request.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_update_success.store(_rhs.m_statistics_update_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
	this->m_statistics_update_fail.store(_rhs.m_statistics_update_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
}


}