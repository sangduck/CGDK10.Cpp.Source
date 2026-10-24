#include "pch.h"

namespace CGDK
{


doss::depot::group_immigration::group_immigration()
{
}

doss::depot::group_immigration::~group_immigration()
{
}

void doss::depot::group_immigration::on_initializing(context& /*_msg*/)
{
}

void doss::depot::group_immigration::on_initialize(context& /*_msg*/)
{
}

void doss::depot::group_immigration::on_destroying()
{
	this->m_pgroup_next.reset();
}

void doss::depot::group_immigration::on_destroy()
{
}

void doss::depot::group_immigration::on_starting(context& /*_msg*/)
{
}

void doss::depot::group_immigration::on_start(context& /*_msg*/)
{
	this->enable_member_enter();
}

void doss::depot::group_immigration::on_stopping()
{
	this->disable_member_enter();
}

void doss::depot::group_immigration::on_stop()
{
}

result_code doss::depot::group_immigration::on_member_entering(depot::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	return eRESULT::SUCCESS;
}

void doss::depot::group_immigration::on_member_entered(depot::socket* _pmember, sMESSAGE& /*_msg*/)
{
	_pmember->register_messageable(this);
}

uintptr_t doss::depot::group_immigration::on_member_leaving(depot::socket* _pmember, uintptr_t /*_param*/)
{
	// 1) ...
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void doss::depot::group_immigration::on_member_leaved(socket* /*_pmember*/, uintptr_t /*_result*/)
{
}

BEGIN_CGMESSAGE_MAP(doss::depot::group_immigration)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::REQUEST_CERTIFY, on_message_SERVER_DOSS_REQUEST_CERTIFY, sMESSAGE_NETWORK)
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code doss::depot::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code doss::depot::group_immigration::on_message_SERVER_DOSS_REQUEST_CERTIFY(sMESSAGE_NETWORK& _msg)
{
	// 1) Source를 얻는다.
	auto psocket = _msg.get_source<depot::socket>();

	// 2) depot Group에 들어간다.
	auto result = m_pgroup_next->enter(psocket, _msg);

	// check) Enter에 실패했을 경우 접속을 끊어 버린다.
	if (result != eRESULT::SUCCESS)
	{
		psocket->closesocket();
	}

	// return) 
	return eRESULT::DONE;
}


}