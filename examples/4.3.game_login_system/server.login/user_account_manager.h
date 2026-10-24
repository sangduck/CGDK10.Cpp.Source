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
namespace User
{

class CAccountManager : 
	public						NCGObjectState,
	public						NCGInitializable,
	public						NCGStartable,
	public						CGNameable::NString,
	public						CGSchedulable::CExecutable
{
// 1) Constructor/Destructor
public:
			CAccountManager(const _CHAR* _Name);
	virtual	~CAccountManager();

// 2) 
			void				RequestAccountLogin(const wchar_t* _pstrID, const wchar_t* _pstrPassword, User::CSocket* _pSocket);
			void				RequestAccountLogout(const wchar_t* _pstrID);
			void				RequestAccountLogoutAll();

			void				RequestAccountCheckin(ACCOUNT_ID _idAccount);
			void				RequestAccountCheckout(ACCOUNT_ID _idAccount);

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

	// - Schedulable
	virtual	void				OnExecute		() override;

public:
			//lockable<std::map<std::string, CGPTR<>
};


}