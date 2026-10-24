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
// ServerLobby::CGroup
//
//-----------------------------------------------------------------------------
namespace ServerLobby
{

class CGroupSocket :
	public						CGServerGroup::CListSender<CSocket>, 
	public						CGNet::IO::Messageable::NBase,
	public						NCGObjectState,
	public						NCGInitializable,
	public						NCGStartable,
	public						CGNameable::NString,
	public						CGSchedulable::CExecutable
{
// 1) Constructor/Destructor
public:
			CGroupSocket(const _CHAR* _strName);
	virtual ~CGroupSocket();

public:

// 2) Public
			CGPTR<CSocket>		GetSuitableLobbyServer();

// 3) Override functions
private:
	// - Initialize/Start (NCGInitializable/NCGStartable)
	virtual	void				OnInitializing	(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnInitialize	(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStarting		(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStart			(CGMSG_CONTEXT& _msg) override;

	// - Stop/Destroy (NCGInitializable/NCGStartable)
	virtual	void				OnStopping		() override;
	virtual	void				OnStop			() override;
	virtual	void				OnDestroying	() override;
	virtual	void				OnDestroy		() override;

	// - Member Enter/Leave (CGServerGroup::CListSender<CSocket>)
	virtual	eOBJECT_RESULT		OnMemberEntering(CSocket* _pMember, CGMSG& _msg) override;
	virtual	void				OnMemberEntered	(CSocket* _pMember, CGMSG& _msg) override;
	virtual	uintptr_t			OnMemberLeaving	(CSocket* _pMember) override;
	virtual	void				OnMemberLeaved	(CSocket* _pMember, _In_ uintptr_t _Result) override;

	// - Schedulable (CGSchedulable::CExecutable)
	virtual	void				OnExecute		() override;

// 4) Messages
public:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// - Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg);

	// - Message별 처리 함수
			int					OnMessage_SEND_LOBBYSERVER_STATUS(CGNETMSG& _msg);
			int					OnMessage_REQUEST_ACCOUNT_CHECKIN(CGNETMSG& _msg);
			int					OnMessage_REQUEST_ACCOUNT_CHECKOUT(CGNETMSG& _msg);

	// - Message
	static	CCGBuffer			Message_RESPONSE_ACCOUNT_CHECKIN(int _Result, uint32_t _idTransaction, const ACCOUNT_ID& _idAccount);
};


}