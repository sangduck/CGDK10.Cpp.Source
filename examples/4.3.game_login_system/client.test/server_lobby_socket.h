//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                        Ver 5.0 / Bulid 2010.02.28                         *
//*                                                                           *
//*                        Client Classes Tutorial (4)                        *
//*                                 Room Client                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@paran.com           *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                   (C) 2008 CGLabs All right reserved.                     *
//*                           http://www.CGCII.co.kr                          *
//*                           http://www.CGLabs.co.kr                         *
//*                                                                           *
//*****************************************************************************

#pragma once
// ----------------------------------------------------------------------------
//
// CSocket
//
// ----------------------------------------------------------------------------
namespace ServerLobby
{

class CSocket : 
	public						CGNet::IO::Connector::NTCP,
	public						CGNet::Socket::CTCP<>,
	public						ICGServerGroupable<CSocket>,
	public						NCGDispatchableSingle
{
public:
// 1) Constructor/Destructor
			CSocket()			{}
	virtual ~CSocket()			{}

// 2) Override functions
private:
	virtual void				OnConnect(CGNet::IO::IConnective* _pConnective) override;
	virtual	void				OnFailConnect(CGNet::IO::IConnective* _pConnective, uint32_t _Reason) override;
	virtual void				OnDisconnect(uint32_t _Reason) override;

// 3) Messages
protected:
	// - Message를 받았을 때 불려지는 함수.
	virtual int					OnMessage(CGMSG& _msg) override;

public:
	// - Message...
			CCGBuffer			Message_REQUEST_ACCOUNT_CHECKIN();

public:
	// 1) Account ID
			ACCOUNT_ID			m_idAccount;

	// 2) Socket
			CGPTR<ServerLogin::CSocket>	m_psocketLoginServer;
};


}