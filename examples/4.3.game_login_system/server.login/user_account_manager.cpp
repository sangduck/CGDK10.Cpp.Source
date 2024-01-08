#include "StdAfx.h"


User::CAccountManager::CAccountManager(const _CHAR* _Name) :
	CGNameable::NString(_Name)
{
}

User::CAccountManager::~CAccountManager()
{
}

void User::CAccountManager::RequestAccountLogin(const wchar_t* _pstrID, const wchar_t* _pstrPassword, User::CSocket* _pSocket)
{
	// 1) ID와 Password 문자열 자체가 문제가 있는지 없는지 검사한다.(생략)


	// 1) ID와 Password를 비교해서 정당한 login인지 확인한다.(생략:일반적으로 DB에서 Query해낸다.)

	// check) ID와 Password가 바르지 않다면 Exception을 던진다.


	// 2) 이미 Login되어 있는지 확인한다.(같은 ID가 존재하는지 확인한다.)

	// check) 이미 Login된 상태라면 Login된 상대의 접속을 종료한다.


	// 3) 성공적으로 적절한 Lobby Server를 얻는다.
	auto	psocketServerLobby	 = g_pService->GetSuitableLobbyServer();

	// check) 적당한 Lobby Server가 없으면 Exception을 던진다.
	THROW_IF(psocketServerLobby.empty(), CGException::CThrow_NotExist());

	// Declare) 
	ACCOUNT_ID	idAccount;

	// 4) Account ID를 얻는다.


	// 5) Login되었다면 Accept 신호를 전송한다.
	_pSocket->Send(CGroupImmigrationSocket::Message_RESULT_ACCOUNT_LOGIN_ACCEPT(idAccount, psocketServerLobby->m_ServerInfo));
}

void User::CAccountManager::RequestAccountLogout(const wchar_t* _pstrID)
{

}

void User::CAccountManager::RequestAccountLogoutAll()
{

}

void User::CAccountManager::RequestAccountCheckin(ACCOUNT_ID _idAccount)
{
}

void User::CAccountManager::RequestAccountCheckout(ACCOUNT_ID _idAccount)
{
}
 
void User::CAccountManager::OnInitializing(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CAccountManager::OnInitialize(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CAccountManager::OnDestroying()
{
}

void User::CAccountManager::OnDestroy()
{
}

void User::CAccountManager::OnStarting(CGMSG_CONTEXT& /*_msg*/)
{
}

void User::CAccountManager::OnStart(CGMSG_CONTEXT& _msg)
{
	// 1) 1분마다 한번씩 호출되도록 Schedulable의 간격을 설정한다.
	SetExecuteInterval(tick::minutes(10));

	// 2) Scheduler에  등록한다.
	REGISTER_SCHEDULABLE(this);
}

void User::CAccountManager::OnStopping()
{
	// 1) 실행 등록 해제한다.
	UNREGISTER_SCHEDULABLE(this);

	// 2) 모든 Account를 LogOut시킨다.
	RequestAccountLogoutAll();
}

void User::CAccountManager::OnStop()
{

}

void User::CAccountManager::OnExecute()
{

}
