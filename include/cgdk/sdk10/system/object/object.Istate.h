//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once

//-----------------------------------------------------------------------------
/**

 @class		object::Istate
 @brief		상태 속성을 가지고 설정하는 속성을 가진 인터페이스 클래드
 @todo		
 @ref		
 @details	\n
 객체의 상태값을 가지는 인터페이스 클래스이다.
 상태값은 eOBJECT_STATE 형이다.
 SetStatue()함수로 현재의 상태를 설정할 수 있으며 get_status()함수로 현재 상태를 얻을 수도 있다.
 기본적인 상태는 초기화(initialize)-시작(start)-중지(stop)-소멸(destroy)의 4가지를 기본으로 가지고 있으며 이들간의 전이 상태들도 존재한다.

  1) initialize		eOBJECT_STATE::NONE    ->eOBJECT_STATE::INITIALIZING_PENDING ->eOBJECT_STATE::STOPPED
  2) start			eOBJECT_STATE::STOPPED ->eOBJECT_STATE::START_PENDING        ->eOBJECT_STATE::RUNNING
  3) stop			eOBJECT_STATE::RUNNING ->eOBJECT_STATE::STOP_PENDING		 ->eOBJECT_STATE::STOPPED
  4) Destroy		eOBJECT_STATE::STOPPED ->eOBJECT_STATE::DESTROYING_PENDING   ->eOBJECT_STATE::NONE
  5) pause			eOBJECT_STATE::RUNNING ->eOBJECT_STATE::PAUSE_PENDING		 ->eOBJECT_STATE::PAUSED
  6) continue		eOBJECT_STATE::PAUSED  ->eOBJECT_STATE::CONTINUE_PENDING	 ->eOBJECT_STATE::RUNNING

*/
//-----------------------------------------------------------------------------
class CGDK::object::Istate : public Iattachable<Istate>
{
public:
	virtual ~Istate() noexcept {}

public:
	public:
	[[nodiscard]] bool			is_attached_empty() const noexcept { return this->m_container.empty();}
	[[nodiscard]] bool			is_attached_exist() const noexcept { return !this->m_container.empty();}
	[[nodiscard]] size_t		attached_size() const noexcept { return this->m_container.size();}
	[[nodiscard]] auto			find_attachable(Istate* _child) noexcept -> object_ptr<Istate>;
			int					clear() noexcept;

	virtual	bool				attach_child_object(object_ptr<Istate>&& _pchild) { return this->process_attach(std::move(_pchild));}
	virtual	bool				detach_child_object(Istate* _pchild) noexcept { return this->process_detach(_pchild);}

	[[nodiscard]] virtual auto	get_object_state() const noexcept -> eOBJECT_STATE PURE;
	virtual	auto				set_object_state(eOBJECT_STATE _state) noexcept -> eOBJECT_STATE PURE;
	virtual	bool				set_object_state_if(eOBJECT_STATE _state_compare, eOBJECT_STATE _state) noexcept PURE;

private:
	virtual	bool				process_attach(object_ptr<Istate>&& _child) override;
	virtual	bool				process_detach(Istate* _child) noexcept override;

public:
			struct WAIT_INFO
			{
				object_ptr<Istate> pobject;
				result_code		result;
			};
			std::vector<WAIT_INFO> m_queue_pended;
			circular_list<object_ptr<Istate>> m_container;
};

namespace CGDK::object
{
	result_code initialize	(Istate* _object, context& _context);
	result_code start		(Istate* _object, context& _context);
	result_code stop		(Istate* _object) noexcept;
	result_code destroy		(Istate* _object) noexcept;
	result_code pause		(Istate* _object);
	result_code resume		(Istate* _object);
}