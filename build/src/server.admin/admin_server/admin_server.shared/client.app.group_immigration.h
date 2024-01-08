//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ⓒ2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// client::group_immigration
//
//-----------------------------------------------------------------------------
class client::app::group_immigration :
	public						CGDK::group::list_sender<socket>,
	public						net::io::messageable::Nbase,
	public						Nmessage_transmitter,
	public						Nobject_state,
	public						Ninitializable,
	public						Nstartable,
	public						Nnameable,
	public						schedulable::Nexecutable
{
public:
			 group_immigration(std::string_view _name);
	virtual ~group_immigration();


private:
	// 1) initialize/destroy
	virtual	void				on_initializing		(context& _context) override;
	virtual	void				on_initialize		(context& _context) override;
	virtual	void				on_destroying		() override;
	virtual	void				on_destroy			() override;

	// 2) start/Stop
	virtual	void				on_starting			(context& _context) override;
	virtual	void				on_start			(context& _context) override;
	virtual	void				on_stopping			() override;
	virtual	void				on_stop				() override;

	// 3) Member Enter/Leave
	virtual	result_code			on_member_entering	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_entered	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_leaved	(socket* _pmember, sMESSAGE& _msg, _In_ uintptr_t _result) override;

	// 4) Schedulable
	virtual	void				on_execute			() override;


public:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// 1) Message 처리 함수
	virtual result_code			on_message		(sMESSAGE_NETWORK& _msg) override;

	// 2) Message별 처리 함수
			result_code			on_message_CERTIFY_CLIENT_REQUEST(sMESSAGE_NETWORK& _msg);
};
