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

class user::account : virtual public Ireferenceable
{
public:
			lock_object			m_cs_acount;
			ACCOUNT_ID			m_account_id;
			int					m_count_lock;

			void reset()	
			{
				SCOPED_LOCK(this->m_cs_acount)
				{
					this->m_account_id.reset();
					this->m_count_lock = 0;
				}
			}

			int checkin()
			{
				SCOPED_LOCK(this->m_cs_acount)
				{
					return ++this->m_count_lock;
				}
			}

			int	checkout()
			{
				SCOPED_LOCK(this->m_cs_acount)
				{
					return --this->m_count_lock;
				}
			}
};

class user::manager : 
	public object::Ninitializable_startable,
	public object::Nnameable,
	public schedulable::Nexecutable
{
public:
			void				request_account_login(std::string_view _id, std::string_view _password, user::socket* _psocket);
			void				request_account_logout(const ACCOUNT_ID& _account_id);
			void				request_account_logout_all();
			int					request_account_checkin(const ACCOUNT_ID& _account_id);
			void				request_account_checkout(const ACCOUNT_ID& _account_id);
			void				request_account_checkout(user::socket* _psocket);
			int					get_logined_account();
			void				complete_account_login(int _result, uint64_t _account_id, user::socket* _psocket);

private:
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_execute () override;

public:
			lock_object			m_cs_map_account;
			std::map<uint64_t, object_ptr<account>> m_map_account;

};
