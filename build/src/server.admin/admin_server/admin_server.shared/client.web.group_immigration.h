//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
// ----------------------------------------------------------------------------
//
// socket::group_immigration
//
// ----------------------------------------------------------------------------
class client::web::group_immigration :
// inherited classes).
	public						CGDK::group::list_sender<socket>,
	public						Nnameable,
	public                      Nobject_startable,
	virtual public				Nmessage_transmitter_single,
	public						net::io::messageable::Nbase
{
// constructor/destructor)
public:
	explicit group_immigration(const std::string_view _name);
	virtual	~group_immigration();

// framework) 
private:
	virtual	void				on_initialize		(context& _context) override;
	virtual	void				on_destroy			() override;
	virtual	void				on_start			(context& _context) override;
	virtual	void				on_stop				() override;

	virtual	result_code			on_member_entering	(socket* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_entered	(socket* _pmember, sMESSAGE& _msg);
	virtual	uintptr_t			on_member_leaving	(socket* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_leaved	(socket* _pmember, sMESSAGE& _msg, uintptr_t _result);

// message) 
private:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
};
