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
// CService
//
//-----------------------------------------------------------------------------
class CService : 
	public						NCGServerService,
	public						CGSchedulable::CExecutable
{
// 1) Constructor/Destructor
public:
			CService();
	virtual	~CService();

// 2) Publics
			void				UpdatePlayerCount();

// 3) Override functions
private:
	// - Setting/Initialize/Start (NCGServerService)
	virtual void				OnServiceSetting(CGMSG_CONTEXT&) override;
	virtual	void				OnInitializing	(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnInitialize	(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnDestroying	() override;
	virtual	void				OnDestroy		() override;

	// - Start/Stop
	virtual	void				OnStarting		(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStart			(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStopping		() override;
	virtual	void				OnStop			() override;

	// - Executable
	virtual	void				OnExecute		() override;

	// - Messages
	virtual int					OnMessage(CGMSG& _msg) override;

public:
			CCGBuffer			Message_REQUEST_CERTIFICATION();
			CCGBuffer			Message_SEND_LOBBYSERVER_STATUS();

// 3) Implementations
public:
	// - Server Login
			CGPTR_DESTROY<ServerLogin::CSocket>				m_psocketServerLogin;

	// - User
			CGPTR_DESTROY<User::CGroupSocket>				m_pgroupUser;
			CGPTR_DESTROY<User::CGroupImmigrationSocket>	m_pgroupUserImmigration;
			CGPTR_DESTROY<CGNet::CAcceptor<User::CSocket>>	m_pacceptorUser;

	// - Log
			CGPTR_DESTROY<CCGLogFile>						m_pLogFile;

	// - Server Info
			lockable<SERVER_INFO>							m_ServerInfo;
};
