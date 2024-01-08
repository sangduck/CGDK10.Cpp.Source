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
//-----------------------------------------------------------------------------
//
// User::CSocket
//
//-----------------------------------------------------------------------------
namespace User
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

public:

// 2) Override functions
private:
	// 1) 접속/접속종료 되었을 때 불려지는 함수.
	virtual void				OnConnect(CGNet::IO::IConnective* _pConnective) override;
	virtual	void				OnDisconnect(uint32_t _Reason) override;

// 3) Messages
protected:
	// 1) Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg) override;

public:
			int					m_idxList;

};


}