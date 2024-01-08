#pragma once

namespace ServerLogin
{

class CGroupImmigrationSocket : 
	public						CGServerGroup::CListSender<CSocket>, 
	public						CGNet::IO::Messageable::NBase,
	public						CGExecutor::CSchedulable,
	public						NCGDispatchable
{ 
// 1) Constructor/Destructor)
public:
			CGroupImmigrationSocket();
	virtual ~CGroupImmigrationSocket();

// 2) Override functions
private:
	// - Member Enter/Leave (CGServerGroup::CListSender<CSocket>)
	virtual	eOBJECT_RESULT		OnMemberEntering(CSocket* _pMember, CGMSG& _msg) override;
	virtual	void				OnMemberEntered(CSocket* _pMember, CGMSG& _msg) override;
	virtual	uintptr_t			OnMemberLeaving(CSocket* _pMember) override;
	virtual	void				OnMemberLeaved(CSocket* _pMember, uintptr_t _Result) override;

	// - Schedulable (CGSchedulable::CExecutable)
	virtual	void				OnExecute(DWORD p_dwParam);

// 3) Messages
protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// - Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg) override;

	// - Message별 처리 함수
			int					OnMessage_RESPONSE_ACCOUNT_LOGIN(CGNETMSG& _msg);

public:
	static	CCGBuffer			Message_REQUEST_ACCOUNT_LOGIN(const wchar_t* _strName, const wchar_t* _strPassword);
	static	CCGBuffer			Message_REQUEST_ACCOUNT_LOGIN(const wchar_t* _DeviceID);
};


}