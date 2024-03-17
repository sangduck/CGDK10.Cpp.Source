#include "pch.h"

namespace CGDK
{
	
doss::statistics::transactable::transactable(const transactable& _rhs) noexcept
{
	this->set(_rhs); 
}

void doss::statistics::transactable::set(const transactable& _rhs) noexcept
{
	this->count_process_request_immidiately.store(_rhs.count_process_request_immidiately.load());
	this->count_process_request_pended.store(_rhs.count_process_request_pended.load());
	this->count_processing_request.store(_rhs.count_processing_request.load());
	this->count_success.store(_rhs.count_success.load());
	this->count_fail.store(_rhs.count_fail.load());
}

doss::statistics::entity::entity(const entity& _rhs) noexcept
{
	this->set(_rhs); 
}

void doss::statistics::entity::set(const entity& _rhs) noexcept
{
	this->count_login_request.store (_rhs.count_login_request.load());
	this->count_login_success.store (_rhs.count_login_success.load());
	this->count_login_fail.store(_rhs.count_login_fail.load());
	this->count_logout_request.store (_rhs.count_logout_request.load());
	this->count_logout_complete.store (_rhs.count_logout_complete.load());
	this->count_checkin_request.store (_rhs.count_checkin_request.load());
	this->count_checkin_success.store (_rhs.count_checkin_success.load());
	this->count_checkin_fail.store (_rhs.count_checkin_fail.load());
	this->count_checkout_request.store (_rhs.count_checkout_request.load());
	this->count_checkout_success.store (_rhs.count_checkout_success.load());
}

doss::statistics::repository::repository(const repository& _rhs) noexcept
{
	this->set(_rhs); 
}

void doss::statistics::repository::set(const repository& _rhs) noexcept
{
	this->m_statistics_transactable_request.store (_rhs.m_statistics_transactable_request.load());
	this->m_statistics_transactable_complete.store (_rhs.m_statistics_transactable_complete.load());
	this->m_statistics_login.store (_rhs.m_statistics_login.load());;
	this->m_statistics_login_request.store (_rhs.m_statistics_login_request.load());
	this->m_statistics_login_success.store (_rhs.m_statistics_login_success.load());
	this->m_statistics_login_fail.store (_rhs.m_statistics_login_fail.load());
	this->m_statistics_logout_request.store (_rhs.m_statistics_logout_request.load());
	this->m_statistics_logout_complete.store (_rhs.m_statistics_logout_complete.load());
	this->m_statistics_checkin.store (_rhs.m_statistics_checkin.load());
	this->m_statistics_checkin_request.store (_rhs.m_statistics_checkin_request.load());
	this->m_statistics_checkin_success.store (_rhs.m_statistics_checkin_success.load());
	this->m_statistics_checkin_fail.store (_rhs.m_statistics_checkin_fail.load());
	this->m_statistics_checkout_request.store (_rhs.m_statistics_checkout_request.load());
	this->m_statistics_checkout_success.store (_rhs.m_statistics_checkout_success.load());
	this->m_statistics_checkout_fail.store (_rhs.m_statistics_checkout_fail.load());
	this->m_statistics_checkout_forcible.store (_rhs.m_statistics_checkout_forcible.load());
	this->m_statistics_create_request.store (_rhs.m_statistics_create_request.load());
	this->m_statistics_create_success.store (_rhs.m_statistics_create_success.load());
	this->m_statistics_create_fail.store (_rhs.m_statistics_create_fail.load());
	this->m_statistics_remove_request.store (_rhs.m_statistics_remove_request.load());
	this->m_statistics_remove_complete.store (_rhs.m_statistics_remove_complete.load());
	this->m_statistics_command_request.store (_rhs.m_statistics_command_request.load());
	this->m_statistics_command_success.store (_rhs.m_statistics_command_success.load());
	this->m_statistics_command_fail.store (_rhs.m_statistics_command_fail.load());
	this->m_statistics_update_request.store (_rhs.m_statistics_update_request.load());
	this->m_statistics_update_success.store (_rhs.m_statistics_update_success.load());
	this->m_statistics_update_fail.store (_rhs.m_statistics_update_fail.load());
}


}