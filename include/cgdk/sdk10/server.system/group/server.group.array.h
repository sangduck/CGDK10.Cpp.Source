//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Group Template Classes                           *
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

#include "cgdk/sdk10/server.system/group/server.group.base.h"

namespace CGDK
{
//-----------------------------------------------------------------------------
/**
 @class		array
*///-----------------------------------------------------------------------------
template <class _TMEMBER, class _TPARAM = sMESSAGE, class _TMEMBER_DATA = int, class _TMEMBERBASE = _TMEMBER>
class group::array : 
	virtual public				Igroup_countable<_TMEMBERBASE, _TPARAM>,
	virtual public				Imember_containerable<_TMEMBERBASE>
{
public:
	using group_base_t = Igroup_countable<_TMEMBERBASE, _TPARAM>;
	using member_t = _TMEMBER;
	using member_base_t = _TMEMBERBASE;
	using member_data_t = _TMEMBER_DATA;
	using member_set_t = MEMBER_SET_ARRAY<member_t, member_data_t>;
	using param_t = _TPARAM;
	using container_t = std::vector<member_set_t>;
	using iterator_t = typename container_t::iterator;
	using const_iterator_t = typename container_t::const_iterator;
	using member_snapshot_t = member_snapshot_ptr<member_t>;

public:
			array(size_t _max_member=256);
	virtual	~array() noexcept;

public:
			result_code			enter(member_base_t* _pmember, param_t& _param) = delete;
			result_code			enter(member_base_t* _pmember) = delete;

			object_ptr<member_t> member(member_t* _pmember, int64_t _seat, param_t& _param = param_t());

			object_ptr<member_t> leave_by_seat(int64_t _seat, uintptr_t _param) noexcept;
			object_ptr<member_t> leave_by_seat(int64_t _seat) noexcept { return this->leave_by_seat(_seat, 0); }
			void				leave_all() noexcept { this->leave_all(0);}
			void				leave_all(uintptr_t _param) noexcept;

	[[nodiscard]] object_ptr<member_t> member(int64_t _seat);
	[[nodiscard]] static member_data_t& member_data(const member_t* _pmember);
	[[nodiscard]] static group_member_id_t member_serial(const member_t* _pmember) { return ((member_set_t*)_pmember->member_set())->member_serial();}

			void				lock_member_enter() noexcept { this->Ireferenceable::add_ref(); this->m_lockable_enter.lock();}
			void				unlock_member_enter() noexcept { this->m_lockable_enter.unlock(); this->Ireferenceable::release();}
	[[nodiscard]] lock_object&	member_enter_lock() noexcept { return this->m_lockable_enter;}

	[[nodiscard]] iterator_t	begin() noexcept { return this->m_container_member.begin();}
	[[nodiscard]] const_iterator_t begin() const noexcept { return this->m_container_member.begin();}
	[[nodiscard]] iterator_t	end() noexcept { return this->m_container_member.end();}
	[[nodiscard]] const_iterator_t end() const noexcept { return this->m_container_member.end();}

	//! @brief members as an immutable array (copy-on-write, in seat order, without empty seats). no group lock while the published one is valid. in on_member_entered / on_member_leaving : the live members
	[[nodiscard]] member_snapshot_t get_member_snapshot() const;
	//! @brief calls _function for the members of the snapshot out of the group lock. (members may enter / leave in _function) @return false _function returned false
			bool				for_each_member_snapshot(const std::function<bool (member_t*)>& _function) const;

	//! @brief returns the first member _pred returned true for. _pred runs out of the group lock over the member snapshot (see for_each_member)
	[[nodiscard]] virtual object_ptr<_TMEMBERBASE> find_member(const std::function<bool (_TMEMBERBASE*)>& _pred) override;

	//! @brief calls _function for the members of the snapshot out of the group lock (same as for_each_member_snapshot) @return false _function returned false
	// - _function may lock other members or enter / leave members. a member can leave (or move to another group) before or while _function runs :
	//   lock the member and check is_member_of() when it matters. member_data() / member_serial() are valid only while it is a member.
	virtual	bool				for_each_member(const std::function<bool (_TMEMBERBASE*)>& _function) override;

protected:
	// - lock of the events (lock order : member lock -> enter lock -> group lock)
	//   on_member_entering : in the lock of the member and the enter lock
	//   on_member_entered / on_member_leaving : in the lock of the member and the group lock
	//   on_member_leaved : in the lock of the member (out of the group lock)
	//   member(_pmember, _seat) replacing a member : the old member leaves in the lock of the new member and the enter lock
	//   do not lock another member (or wait for a thread which locks one) in these events. it deadlocks with the enter / leave of other threads.
	//   find_member / for_each_member / get_member_snapshot can be called in them (the live members). send_conditional() of the sender calls _pred in the group lock too.
	virtual	void				on_enable_enter() {}
	virtual	void				on_disable_enter() {}

	virtual	result_code			on_member_entering(member_t* /*_pmember*/, param_t& /*_param*/) { return eRESULT::SUCCESS;}
	virtual	void				on_member_entered(member_t* /*_pmember*/, param_t& /*_param*/) {}
	virtual	uintptr_t			on_member_leaving(member_t* /*_pmember*/, uintptr_t _param) { return _param;}
	virtual	void				on_member_leaved(member_t* /*_pmember*/, uintptr_t /*_result*/)	{}

protected:
	virtual bool				process_is_enable_member_enter() const noexcept override;
	virtual	bool				process_enable_member_enter(bool _enable) override;
	virtual	result_code			process_enter(member_t* _pmember, param_t& _param) override;
	virtual	void				process_leave(member_base_t* _pmember, uintptr_t _param) noexcept override;

	[[nodiscard]] virtual size_t process_get_member_count() const noexcept override { return this->m_count_member; }
	// - re-creates the seats. throws throwable::failure(eRESULT::INVALID_STATE) when the group has members (the seats are in use)
	virtual	void				process_set_member_capacity(size_t _max_count) override;
	[[nodiscard]] virtual size_t process_get_member_capacity() const noexcept override { return this->m_count_max_member; }

private:
	[[nodiscard]] member_snapshot<member_t>* _build_member_snapshot() const;

	virtual	result_code			_process_member_entering(member_t* _pmember, param_t& _param);
	virtual	void				_process_member_entered(member_t* _pmember, param_t& _param);
	virtual	uintptr_t			_process_member_leaving(member_t* _pmember, uintptr_t _param) noexcept;
	virtual	void				_process_member_leaved(member_t* _pmember, uintptr_t _result) noexcept;

private:
			bool				m_enable_enter;
	mutable	lock_object			m_lockable_enter;
protected:
			container_t			m_container_member;
			size_t				m_count_member;
			size_t				m_count_max_member;

	// - published member snapshot (changed in the group lock, read without it)
	mutable	member_snapshot_holder<member_t> m_member_snapshot;
};


}

#include "cgdk/sdk10/server.system/group/server.group.array.inl"