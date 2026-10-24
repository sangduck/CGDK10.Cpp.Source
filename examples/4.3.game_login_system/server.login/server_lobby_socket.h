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
//-----------------------------------------------------------------------------
//
// ServerLobby::CSocket
//
//-----------------------------------------------------------------------------
namespace ServerLobby
{

class CSocket : 
	public						CGNet::Socket::CTCP<uint32_t>,
	public						ICGServerGroupable<CSocket>,
	public						NCGDispatchable
{
// 1) Constructor/Destructor
public:
			CSocket();
	virtual	~CSocket();

// 2) Override functions
private:
	// - Connected/Disconnected (CGNet::Socket::CTCP<uint32_t>)
	virtual void				OnConnect(CGNet::IO::IConnective* _pConnective) override;
	virtual	void				OnDisconnect(uint32_t _Reason) override;

// 3) Messages
protected:
	// - Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg) override;

public:
			UINT				m_idxList;
			lockable<SERVER_INFO>	m_ServerInfo;
};


}