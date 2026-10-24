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
// ServerLobby::CGroupImmigration
//
//-----------------------------------------------------------------------------
namespace ServerLobby
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

// 2) Override functions
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

	// - Execute function (CGSchedulable::CExecutable)
	virtual	void				OnExecute		() override;


// 3) Messages
public:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// - Message 贸府 窃荐
	virtual int					OnMessage(CGMSG& _msg);

	// - Message 累己
			int					OnMessage_REQUEST_CERTIFICATION(CGNETMSG& _msg);

	// - Message 累己
			CCGBuffer			Message_ACCEPT_CERTICIATION();
};


}