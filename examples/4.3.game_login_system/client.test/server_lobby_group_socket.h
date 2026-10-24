#pragma once

namespace ServerLobby
{

class CGroupSocket : 
	public						CGServerGroup::CListSender<CSocket>, 
	public						CGNet::IO::Messageable::NBase,
	public						CGExecutor::CSchedulable,
	public						NCGDispatchable
{ 
// 1) Constructor/Destructor
public:
			CGroupSocket();
	virtual ~CGroupSocket();

// 2) Override functions
private:
	// - Member Enter/Leave (CGServerGroup::CListSender<CSocket>)
	virtual	eOBJECT_RESULT		OnMemberEntering(CSocket* _pMember, CGMSG& _msg);
	virtual	void				OnMemberEntered(CSocket* _pMember, CGMSG& _msg);
	virtual	uintptr_t			OnMemberLeaving(CSocket* _pMember);
	virtual	void				OnMemberLeaved(CSocket* _pMember, uintptr_t _Result);

	// - Schedulable (CGSchedulable::CExecutable)
	virtual	void				OnExecute(DWORD p_dwParam);

// 3) Messages
protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// - Message 처리 함수
	virtual int					OnMessage(CGMSG& _msg) override;

	// - Message별 처리 함수
};


}