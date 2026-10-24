//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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

namespace CGDK
{
// ----------------------------------------------------------------------------
//
// server_system::group_socket
//
// 
// ----------------------------------------------------------------------------
class server_system::host::group_socket : 
	public						CGDK::group::list_sender<server_system::host::socket>,
	public						net::io::messageable::Nbase	
{
private:
	virtual	result_code			on_member_entering(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_entered(socket* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving(socket* _pmember, uintptr_t _param) override;
	virtual	void				on_member_leaved(socket* _pmember, uintptr_t _result) override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_RESULT(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_HOST_UPDATE_STATUS(sMESSAGE_NETWORK& _msg);

			void				_process_remove_instances_in_socket(server_system::host::socket* _pmember);
};


}
