//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Login Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class server::socket :
	public						net::socket::tcp<>,
	public						Igroupable<socket>,
	public						Nmessage_transmitter
{
private:
	virtual void				on_connect() override;
	virtual	void				on_disconnect(uint64_t _reason) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

public:
			lock_object			m_lock_server_info;
			SERVER_INFO			m_server_info;
			std::string			m_strServerName;
			UINT				m_idxList{0xffffffff};
};
