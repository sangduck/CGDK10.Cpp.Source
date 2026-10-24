#include "pch.h"


void socket_user::set_account(object_ptr<account>&& _paccount)
{
	// check)
	CGASSERT_ERROR(_paccount.exist());

	// check)
	THROW_IF(_paccount.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "account is nulltpr"sv));

	SCOPED_LOCK(this->m_cs_account)
	{
		// check)
		CGASSERT_ERROR(this->m_paccount.empty());

		// check)
		THROW_IF(m_paccount.exist(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "aleady account exist"sv));

		// 1) set...
		this->m_paccount = std::move(_paccount);
	}
}

object_ptr<account> socket_user::reset_account() noexcept
{
	// delcare)
	object_ptr<account>	paccount;

	// 1) move account object
	SCOPED_LOCK(this->m_cs_account)
	{
		paccount = std::move(this->m_paccount);
	}

	// return)
	return paccount;
}

object_ptr<account> socket_user::get_account() const noexcept
{
	// delcare)
	object_ptr<account>	paccount;

	// 1) move account object
	SCOPED_LOCK(this->m_cs_account)
	{
		paccount = this->m_paccount;
	}

	// return)
	return paccount;
}

object_ptr<area::entity_player> socket_user::get_area_entity() const noexcept
{
	// 1) get account first
	auto paccount = this->get_account();

	// check)
	RETURN_IF(paccount.empty(), object_ptr<area::entity_player>());

	// 2) get entity
	return paccount->parea_entity;
}


void socket_user::on_connect()
{
	g_pservice->pgroup_user_immigration->enter(this);
}
void socket_user::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) 먼저 group에서 빠진다.
	this->leave_group();

	// 2) logout처리를 한다.
	{
		g_pservice->m_paccount_manager->logout(this);
	}
}
result_code socket_user::on_message(sMESSAGE_NETWORK & _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return) 
	return eRESULT::DONE;
}
