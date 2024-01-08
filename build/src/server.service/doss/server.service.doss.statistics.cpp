#include "pch.h"

namespace CGDK
{
	
doss::statistics::transactable::transactable(const transactable& _rhs) noexcept
{
	set(_rhs); 
}

void doss::statistics::transactable::set(const transactable& _rhs) noexcept
{
	count_process_request_immidiately.store(_rhs.count_process_request_immidiately);
	count_process_request_pended.store(_rhs.count_process_request_pended);
	count_processing_request.store(_rhs.count_processing_request);
	count_success.store(_rhs.count_success);
	count_fail.store(_rhs.count_fail);
}

doss::statistics::entity::entity(const entity& _rhs) noexcept
{
	set(_rhs); 
}

void doss::statistics::entity::set(const entity& _rhs) noexcept
{
	count_login_request.store			(_rhs.count_login_request);
	count_login_success.store			(_rhs.count_login_success);
	count_login_fail.store				(_rhs.count_login_fail);
	count_logout_request.store			(_rhs.count_logout_request);
	count_logout_complete.store			(_rhs.count_logout_complete);
	count_checkin_request.store			(_rhs.count_checkin_request);
	count_checkin_success.store			(_rhs.count_checkin_success);
	count_checkin_fail.store			(_rhs.count_checkin_fail);
	count_checkout_request.store		(_rhs.count_checkout_request);
	count_checkout_success.store		(_rhs.count_checkout_success);
}

doss::statistics::repository::repository(const repository& _rhs) noexcept
{
	set(_rhs); 
}

void doss::statistics::repository::set(const repository& _rhs) noexcept
{
	m_statistics_transactable_request.store	(_rhs.m_statistics_transactable_request);;
	m_statistics_transactable_complete.store(_rhs.m_statistics_transactable_complete);;
	m_statistics_login.store				(_rhs.m_statistics_login);;
	m_statistics_login_request.store		(_rhs.m_statistics_login_request);
	m_statistics_login_success.store		(_rhs.m_statistics_login_success);
	m_statistics_login_fail.store			(_rhs.m_statistics_login_fail);
	m_statistics_logout_request.store		(_rhs.m_statistics_logout_request);
	m_statistics_logout_complete.store		(_rhs.m_statistics_logout_complete);
	m_statistics_checkin.store				(_rhs.m_statistics_checkin);
	m_statistics_checkin_request.store		(_rhs.m_statistics_checkin_request);
	m_statistics_checkin_success.store		(_rhs.m_statistics_checkin_success);
	m_statistics_checkin_fail.store			(_rhs.m_statistics_checkin_fail);
	m_statistics_checkout_request.store		(_rhs.m_statistics_checkout_request);
	m_statistics_checkout_success.store		(_rhs.m_statistics_checkout_success);
	m_statistics_checkout_fail.store		(_rhs.m_statistics_checkout_fail);
	m_statistics_checkout_forcible.store	(_rhs.m_statistics_checkout_forcible);
	m_statistics_create_request.store		(_rhs.m_statistics_create_request);
	m_statistics_create_success.store		(_rhs.m_statistics_create_success);
	m_statistics_create_fail.store			(_rhs.m_statistics_create_fail);
	m_statistics_remove_request.store		(_rhs.m_statistics_remove_request);
	m_statistics_remove_complete.store		(_rhs.m_statistics_remove_complete);
	m_statistics_command_request.store	(_rhs.m_statistics_command_request);
	m_statistics_command_success.store	(_rhs.m_statistics_command_success);
	m_statistics_command_fail.store		(_rhs.m_statistics_command_fail);
	m_statistics_update_request.store		(_rhs.m_statistics_update_request);
	m_statistics_update_success.store		(_rhs.m_statistics_update_success);
	m_statistics_update_fail.store			(_rhs.m_statistics_update_fail);
}


}