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

template <class TMEMBER>
class Igroup_list :
	public						CGDK::group::list_sender<TMEMBER>,
	public                      object::Ninitializable_startable,
	public						object::Nnameable,
	public						net::io::messageable::Nbase,
	public						Nmessage_transmitter
{
private:
	virtual	void				on_initializing(context& _context) override;
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroying() override;
	virtual	void				on_destroy() override;
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_stop() override;
};

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_initializing(context& /*_context*/)
{
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_initialize(context& /*_context*/)
{
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_destroying()
{
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_destroy()
{
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_starting(context& /*_context*/)
{
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_start(context& /*_context*/)
{
	// 1) 입장을 받기 시작~
	this->enable_member_enter(true);
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_stopping()
{
	// 1) 입장을 금지
	this->disable_member_enter();

	// 2) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	this->for_each_member([=](TMEMBER* _pmember)->bool
		{
			_pmember->closesocket();
			return true;
		});
}

template <class TMEMBER>
void Igroup_list<TMEMBER>::on_stop()
{
}
