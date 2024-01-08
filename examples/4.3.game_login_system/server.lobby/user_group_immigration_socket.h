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
// User::CGroupImmigrationSocket
//
//-----------------------------------------------------------------------------
namespace User
{

class CGroupImmigrationSocket :
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
			CGroupImmigrationSocket(const _CHAR* _strName);
	virtual ~CGroupImmigrationSocket();


// 2) Public functions
public:


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

	// 4) Schedulable
	virtual	void				OnExecute		() override;


// 4) Messages
protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// 1) Message 贸府 窃荐
	virtual int					OnMessage(CGMSG& _msg);

	// 2) Message 累己
			int					OnMessage_REQUEST_ACCOUNT_CHECKIN(CGNETMSG& _msg);
			int					OnMessage_RESPONSE_ACCOUNT_CHECKIN(CGNETMSG& _msg);

	// 3) Message 累己
	//		CCGBuffer			Message_ACCEPT_CERTICIATION();
};


}