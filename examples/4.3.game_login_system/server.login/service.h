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
public:
			CGPTR<ServerLobby::CSocket>	GetSuitableLobbyServer();

// 2) Override functions
private:
	// - Setting/Initialize/Start (NCGServerService)
	virtual void				OnServiceSetting(CGMSG_CONTEXT&) override;
	virtual	void				OnInitializing(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnInitialize(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStarting(CGMSG_CONTEXT& _msg) override;
	virtual	void				OnStart(CGMSG_CONTEXT& _msg) override;

	// - Stop/Destroy
	virtual	void				OnStopping() override;
	virtual	void				OnStop() override;
	virtual	void				OnDestroying() override;
	virtual	void				OnDestroy() override;

	// - Executable
	virtual	void				OnExecute() override;

	// - Messages
	virtual int					OnMessage(CGMSG& _msg) override;


// 3) Implementations
public:
	// - Account Manager
			CGPTR_DESTROY<User::CAccountManager>					m_pmanagerAccount;

	// - Server Login
			CGPTR_DESTROY<ServerLobby::CGroupSocket>				m_pgroupServerLobby;
			CGPTR_DESTROY<ServerLobby::CGroupImmigrationSocket>		m_pgroupServerLobbyImmigration;
			CGPTR_DESTROY<CGNet::CAcceptor<ServerLobby::CSocket>>	m_pacceptorServerLobby;

	// - User
			CGPTR_DESTROY<User::CGroupImmigrationSocket>			m_pgroupUserImmigration;
			CGPTR_DESTROY<CGNet::CAcceptor<User::CSocket>>			m_pacceptorUser;

	// - Log file
			CGPTR_DESTROY<CCGLogFile>	m_pLogFile;
};
