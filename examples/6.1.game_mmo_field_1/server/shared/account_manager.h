//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once

class account_manager :
	public	object::Ninitializable_startable,
	public	object::Nnameable
{
public:
			object_ptr<account>	login(socket_user* _psocket, ACCOUNT_INDEX);
			void				logout(socket_user* _psocket) noexcept;

private:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_stop() override;

			own_ptr<account>	process_get_account(ACCOUNT_INDEX _account_index);
			void				process_prepare_login(account* _paccount);

			lock_object			m_cs_map_account;
			std::map<ACCOUNT_INDEX, own_ptr<account>> m_map_account;
};
