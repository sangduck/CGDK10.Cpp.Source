//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Lobby Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr)                         *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class user::group: public Igroup_list<socket>
{
private:
	virtual	result_code			on_member_entering(socket* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving(socket* _pmember, uintptr_t _param) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
};
