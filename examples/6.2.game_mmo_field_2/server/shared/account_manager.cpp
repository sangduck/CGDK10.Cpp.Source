#include "pch.h"


object_ptr<account>	account_manager::login(socket_user* _psocket, ACCOUNT_INDEX _account_index)
{
	// check)
	CGASSERT_ERROR(_account_index != 0);

	// check)
	THROW_IF(_account_index == 0, throwable::failure(eRESULT::INVALID_VALUE, 0, "'accont_index' is invalid(zero)!"sv));

	// declare) 
	object_ptr<account> paccount;
	object_ptr<socket_user> psocket_old;

	SCOPED_LOCK(m_cs_map_account)
	{
		// 1) get exist account
		auto iter_find = this->m_map_account.find(_account_index);

		// 2-1) 없다.(정상 상황)
		if (iter_find == this->m_map_account.end())
		{
			// - process load account info
			auto paccount_new = this->process_get_account(_account_index);

			// - copy~ (paacount_new는 move를 할 것이기 때문에 여기서 일단 복사...)
			paccount = paccount_new;

			// - 추기한다.
			this->m_map_account.insert({ _account_index, std::move(paccount_new) });
		}
		// 2-2) 이미 존재하는 접속 중인 경우? (기존 socket의 접속을 끊는다.)
		else
		{
			// - get account
			paccount = iter_find->second;

			// check) aleady logined?
			CGASSERT_ERROR(paccount->psocket != _psocket);

			// - old socket move~
			psocket_old = std::move(paccount->psocket);
		}

		// - create entity
		this->process_prepare_login(paccount);

		// - set new socket
		paccount->psocket = _psocket;
	}

	// 3) close old_socket
	if (psocket_old.exist())
	{
		// - 모든 group을 먼저 나오게 한다.
		psocket_old->leave_group();

		// - 접속을 끊는다.
		psocket_old->closesocket();
	}

	// return)
	return paccount;
}

void account_manager::logout(socket_user* _psocket) noexcept
{
	// check)
	CGASSERT_ERROR(_psocket != nullptr);

	// check)
	RETURN_IF(_psocket == nullptr,)

	SCOPED_LOCK(m_cs_map_account)
	{
		// declare) 
		auto paccont = _psocket->reset_account();

		// check) logout
		RETURN_IF(paccont.empty(), );

		// 1) get account_index
		auto account_index = paccont->account_info.account_index;

		// check)
		RETURN_IF(account_index == 0, )

		// check)
		RETURN_IF(_psocket != paccont->psocket)

		// 2) get exist account
		auto iter_find = this->m_map_account.find(account_index);

		// check)
		RETURN_IF(iter_find == this->m_map_account.end(),);

		// 3) remove
		this->m_map_account.erase(iter_find);

		// 4) reset account
		//_paccount
	}
}

void account_manager::on_starting(context& /*_context*/)
{
}

void account_manager::on_start(context& /*_context*/)
{

}

void account_manager::on_stopping()
{

}

void account_manager::on_stop()
{

}

own_ptr<account> account_manager::process_get_account(ACCOUNT_INDEX _account_index)
{
	// - 새로운 account 객체를 생성한다.
	auto paccount_new = make_own<account>();

	// - temporary..
	paccount_new->account_info.account_index = _account_index;
	paccount_new->area_index_now = 0;
	//paccount_new->parea_entity; // own_ptr<area::entity_player> 
	//paccount_new->psocket; // object_ptr<socket_user> 

	// - set entity tag info
	paccount_new->account_info.tag.name = std::move("TEST player"s);

	// reutnr)
	return paccount_new;
}

void account_manager::process_prepare_login(account* _paccount)
{
	// 설명) 로그인의 처리로 사용자의 인증과 사용자 정보를 모두 받았다는 것을 전재로...
	//       account 정보를 준비하는 과정을 말한다.
	//       일단은 entity가 하나뿐이라는 가정하에 설정했음.
	//       한명이 여러 개의 entity를 가질 경우 여기서 처리

	// check)
	CGASSERT_ERROR(_paccount != nullptr);

	// check)
	CGASSERT_ERROR(_paccount->account_info.account_index != 0);

	{
		// - area_entity를 생성한다.
		auto pentity = make_own<area::entity_player>();

		// - area entity를 초기화한다.
		{
			pentity->area_entity_info.id = 0;
			pentity->area_entity_info.owner = _paccount->account_info.account_index;
			pentity->area_entity_info.type = eAREA_ENTITY_TYPE::CHARACTER;
		}

		// - set area entity to account
		pentity->paccount = _paccount;
		_paccount->parea_entity = std::move(pentity);
	}
}
