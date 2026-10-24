#include "pch.h"


void user::manager::request_account_login(std::string_view /*_id*/, std::string_view /*_password*/, user::socket* _psocket)
{
	// 1) ID와 Password 문자열 자체가 문제가 있는지 없는지 검사한다.(생략)


	// 1) ID와 Password를 비교해서 정당한 login인지 확인한다.(생략:일반적으로 DB에서 Query해낸다.)

	// check) ID와 Password가 바르지 않다면 Exception을 던진다.


	// 2) 이미 Login되어 있는지 확인한다.(같은 ID가 존재하는지 확인한다.)

	// check) 이미 Login된 상태라면 Login된 상대의 접속을 종료한다.

	// -------------------------------------------------------------------------------------
	// 설명) 여기에서 일반적으로 DB 혹은 외부에서 Login정보를 얻어오는 코드를 작성한다.
	//       하지만 여기서는 예시만을 보여주도록 하기 위해 그냥 비동기 함수 호출로 대체한다.
	//
	//   - 반드시 account_uid값에 해당하는 계정의 고유 번호를 부여하여야 한다.
	//   - 여기서는 예제를 보여주기 위해 계속 증가만 하는 g_TT라는 임시값을 사용했다.
	// -------------------------------------------------------------------------------------
	// 임시로 값을 사용한다.(일반적으로 DB에서 계정 고유 값을 얻어 온다.)
	static uint64_t	g_TT = 1;

	// 1) 여기서 일반적으로 ID와 Password를 검사하고 계정의 고유번호를 얻었다고 가정
	uint64_t account_uid = g_TT++;

	// 2) 계정처리 완료 후 호출 (이 때 성공여부와 계정고유번호를 전달해 준다.)
	complete_account_login(0, account_uid, _psocket);
}

void user::manager::complete_account_login(int _result, uint64_t _account_id, user::socket* _psocket)
{
	// check) Result를 확인해서 Success(0)이 아니면 _psocket에 Reject를 전송한다.
	if(_result != 0)
	{
		// Reject 메시지 전송
		_psocket->send(group_immigration::message_RESULT_ACCOUNT_CHECKIN_REJECT(_result));

		// Log) 
		LOG_PROGRESS << " account login failed [reault: "sv << _result << "] [account_id:"sv << _account_id << "]"sv;

		// return) 
		return;
	}

	// Static) 
	static uint32_t	g_serial_checkin = 0;

	try
	{
		// 3) 성공적으로 적절한 Lobby Server를 얻는다.
		auto psocket_server_channel = g_pservice->pgroup_server_channel->get_suitable_channel_server();

		// check) 적당한 Lobby Server가 없으면 Exception을 던진다.
		THROW_IF(psocket_server_channel.empty(), throwable::failure(eRESULT::NOT_EXIST, 0, " Lobby Server is not exist [AccountID: {}]"sv));

		// Declare) 
		object_ptr<account>	paccount;

		// 5) Account ID를 얻는다.
		SCOPED_LOCK(this->m_cs_map_account)
		{
			// - 먼저 동일한 Account ID가 존재하는지 확인한다.
			auto iter = this->m_map_account.find(_account_id);

			// Case) 동일한 Account ID가 존재하지 않는다면 -> 신규로 생성한다.
			if (iter == this->m_map_account.end())
			{
				// - 신규로 생성해서 추가한다.
				paccount = make_object<account>();
			}
			// Case) 이미 같은 Account ID가 존재한다면 -> 그걸 그냥 사용한다.
			else
			{
				// - 기존 것을 사용한다.
				paccount = iter->second;
			}

			paccount->reset();
			paccount->m_account_id.account_uid = _account_id;
			paccount->m_account_id.serial = g_serial_checkin++; // Login ID를 얻는다.
			paccount->m_account_id.key = rand(); // random key를 하나 생성한다.

			// - 추가
			this->m_map_account.insert(std::make_pair(_account_id, paccount));

			// - checkin을 한다.(여기서 Lock count는 무조건 1이 될 것이다.)
			paccount->checkin();

			// - Socket에 Account 설정.
			_psocket->m_paccount = paccount;
		}

		// 5) Login되었다면 Accept 신호를 전송한다.
		_psocket->send(group_immigration::message_RESULT_ACCOUNT_CHECKIN_ACCEPT(paccount->m_account_id, psocket_server_channel->m_server_info));
	}
	catch(...)
	{
		// Log) 
		LOG_FATAL<< "Excp) Exception Occured [AccountID: "sv << _account_id << "] "sv;

		// - Rejct 신호를 전송한다.
		_psocket->send(group_immigration::message_RESULT_ACCOUNT_CHECKIN_REJECT(1));
	}
}

void user::manager::request_account_logout(const ACCOUNT_ID& /*_account_id*/)
{

}

void user::manager::request_account_logout_all()
{

}

int user::manager::request_account_checkin(const ACCOUNT_ID& _account_id)
{
	// check) Account ID가 제대로된 것이 아니라면 그냥 Reset한다.
	ERROR_RETURN_IF(_account_id.valid() == false, 1, LOG_ERROR << std::format("Excp) Account id is invalid [AccountID: {}] ", _account_id.account_uid));

	SCOPED_LOCK(this->m_cs_map_account)
	{
		// 1) 먼저 동일한 Account ID가 존재하는지 확인한다.
		auto iter = this->m_map_account.find(_account_id.account_uid);

		// check) 찾지 못했으면 끝낸다.
		ERROR_RETURN_IF(iter == this->m_map_account.end(), 2, LOG_ERROR << std::format("Excp) Account is not founded [AccountID: {}] ", _account_id.account_uid));

		// check) 나머지 key들도 동일해야 한다.
		ERROR_RETURN_IF(iter->second->m_account_id.account_uid !=_account_id.account_uid || iter->second->m_account_id.key !=_account_id.key, 3, LOG_ERROR << std::format("Excp) Account key is invalid[AccountID: %I64u] own[%u;%u] - compare[{}:{}] ", _account_id.account_uid, iter->second->m_account_id.account_uid, _account_id.account_uid, iter->second->m_account_id.key, _account_id.key));

		// 2) 찾은 Account 객체를 저장.
		iter->second->checkin();
	}

	// return)
	return 0;
}

void user::manager::request_account_checkout(const ACCOUNT_ID& _account_id)
{
	// check) Account ID가 제대로된 것이 아니라면 그냥 Reset한다.
	ERROR_RETURN_IF(_account_id.valid()==false, , LOG_ERROR << std::format("Excp) Account id is invalid [AccountID: {}]", _account_id.account_uid));

	// 1) Check Out처리를 한다.
	SCOPED_LOCK(this->m_cs_map_account)
	{
		// 1) 먼저 동일한 Account ID가 존재하는지 확인한다.
		auto iter = this->m_map_account.find(_account_id.account_uid);

		// check) 찾지 못했으면 끝낸다.
		ERROR_RETURN_IF(iter == this->m_map_account.end(), ,LOG_ERROR << std::format("Excp) Account is not founded [AccountID: %I64u]"sv, _account_id.account_uid));

		// check) 나머지 key들도 동일해야 한다.
		ERROR_RETURN_IF(iter->second->m_account_id.account_uid !=_account_id.account_uid || iter->second->m_account_id.key !=_account_id.key, , LOG_ERROR << std::format("Excp) Account key is invalid[AccountID: {}] own[{};{}] - compare[{}:{}] ", _account_id.account_uid, iter->second->m_account_id.account_uid, _account_id.account_uid, iter->second->m_account_id.key, _account_id.key));

		// 2) 찾은 Account 객체를 저장.
		auto count = iter->second->checkout();

		// check) 만약 count가 0이라면 Log out처리한다.
		if(count == 0)
		{
			// - mapaccount에서 제거한다.
			this->m_map_account.erase(iter);

			// - Log out 처리를 수행한다.
		}
	}
}

void user::manager::request_account_checkout(user::socket* _psocket)
{
	// 1) Check Out처리를 한다.
	SCOPED_LOCK(this->m_cs_map_account)
	{
		// check) 
		ERROR_RETURN_IF(_psocket==nullptr, , LOG_ERROR << "Excp) Socket is null"sv);

		// check) 
		ERROR_RETURN_IF(_psocket->m_paccount.empty(), , LOG_ERROR << "Excp) Account is null"sv);

		// 1) 할당해제.
		auto paccount = _psocket->m_paccount;

		// 2) Socket에서 m_paccount를 Reset한다.
		_psocket->m_paccount.reset();

		// 3) checkout한다.
		request_account_checkout(paccount->m_account_id);
	}
}

int user::manager::get_logined_account()
{
	SCOPED_LOCK (this->m_cs_map_account)
	{
		return (int)this->m_map_account.size();
	}
}

void user::manager::on_start(context& /*_context*/)
{
	// 1) 호출 간격을 10초로 설정
	this->execute_interval(10s);

	// 2) schedulable을 등록한다.
	system_executor::register_schedulable(this);
}

void user::manager::on_stopping()
{
	// 1) schedulable에서 등록해제한다.
	system_executor::unregister_schedulable(this);

	// 2) 모든 Account를 LogOut시킨다.
	this->request_account_logout_all();
}

void user::manager::on_execute()
{

}
