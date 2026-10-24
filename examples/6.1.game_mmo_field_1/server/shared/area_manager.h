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

class area::manager :
	public	object::Ninitializable_startable,
	public	object::Nnameable,
	public	schedulable::Nexecutable
{
public:
			manager();
	virtual	~manager() noexcept;

			void				register_area(own_ptr<area::group>&& _pgroup_area);
			void				unregister_area(AREA_INDEX _area_index) noexcept;
			object_ptr<area::group> find_area(AREA_INDEX _area_index) const noexcept;

public:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_stop() override;
	virtual	void				on_execute() override;

private:
			lock_object			m_cs_map_area;
			std::map<AREA_INDEX, own_ptr<area::group>> m_map_area;
};
