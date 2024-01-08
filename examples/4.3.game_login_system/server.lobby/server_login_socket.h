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
// ServerLogin::CSocket
//
//-----------------------------------------------------------------------------
namespace ServerLogin
{

class CSocket : 
	public						CGNet::IO::Connector::NTCPReconnect,
	public						CGNet::Socket::CTCP<uint32_t>,
	public						CGSchedulable::CExecutable
{
// 1) Constructor/Destructor
public:
			CSocket(const _CHAR* _Name);
	virtual	~CSocket();

// 2) Publics
			void				RequestAccountCheckin(const ACCOUNT_ID& _idAccount, User::CSocket* _pSource);

// 3) Override functions
private:
	// - Connected/Disconnected (CGNet::Socket::CTCP<uint32_t>)
	virtual void				OnConnect(CGNet::IO::IConnective* _pConnective) override;
	virtual	void				OnDisconnect(uint32_t _Reason) override;

	// - Schedulable (CGSchedulable::CExecutable)
	virtual	void				OnExecute() override;

// 4) Messages
protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// - Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg) override;

	// - Message별 처리 함수
			int					OnMessage_ACCEPT_CERTIFICATION(CGNETMSG& _msg);
			int					OnMessage_RESPONSE_ACCOUNT_CHECKIN(CGNETMSG& _msg);

	// - Message
	static	CCGBuffer			Message_REQUEST_ACCOUNT_CHECKIN(const ACCOUNT_ID& _idAccount, uint32_t _idRequest);
	static	CCGBuffer			Message_REQUEST_ACCOUNT_CHECKOUT(const ACCOUNT_ID& _idAccount);

public:
			UINT				m_idxList;
			CGPTR<CManagerRegister<User::CSocket>>	m_request;
};


}