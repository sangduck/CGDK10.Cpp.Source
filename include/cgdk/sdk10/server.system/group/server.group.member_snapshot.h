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

namespace CGDK
{
namespace group
{
//-----------------------------------------------------------------------------
/**
 @class		member_snapshot
 @brief		immutable copy of the member list of a group (copy-on-write)
 @details	- one allocation : header + member pointer array.
			- every element owns one reference of the member. (released when the snapshot is freed)
			- never modified after it is built. shared between threads by reference count.
*///-----------------------------------------------------------------------------
template <class TMEMBER>
class member_snapshot
{
public:
	using member_t = TMEMBER;

public:
	[[nodiscard]] static member_snapshot* create(size_t _capacity)
	{
		void* pmemory = ::operator new(sizeof(member_snapshot) + sizeof(member_t*) * ((_capacity != 0) ? _capacity : 1));
		return new (pmemory) member_snapshot(_capacity);
	}

			void				add_ref() noexcept { this->m_count_ref.fetch_add(1, std::memory_order_relaxed); }
			void				release() noexcept { if (this->m_count_ref.fetch_sub(1, std::memory_order_acq_rel) == 1) destroy(this); }

	//! @brief adds a member while building (before it is shared). takes one reference of the member.
			void				push_back(member_t* _pmember) noexcept { CGASSERT_ERROR(this->m_count < this->m_capacity); _pmember->add_ref(); this->data()[this->m_count++] = _pmember; }

	[[nodiscard]] size_t		size() const noexcept { return this->m_count; }
	[[nodiscard]] bool			empty() const noexcept { return this->m_count == 0; }
	[[nodiscard]] member_t* const* begin() const noexcept { return this->data(); }
	[[nodiscard]] member_t* const* end() const noexcept { return this->data() + this->m_count; }
	[[nodiscard]] member_t*		operator[](size_t _index) const noexcept { return this->data()[_index]; }

private:
	explicit					member_snapshot(size_t _capacity) noexcept : m_capacity(_capacity) {}
								~member_snapshot() noexcept = default;

	static	void				destroy(member_snapshot* _psnapshot) noexcept
	{
		for (auto* iter : *_psnapshot)
			iter->release();

		_psnapshot->~member_snapshot();
		::operator delete(_psnapshot);
	}

	[[nodiscard]] member_t**	data() noexcept { return reinterpret_cast<member_t**>(this + 1); }
	[[nodiscard]] member_t* const* data() const noexcept { return reinterpret_cast<member_t* const*>(this + 1); }

			std::atomic<int>	m_count_ref { 1 };
			size_t				m_count { 0 };
			size_t				m_capacity { 0 };
};

//-----------------------------------------------------------------------------
/**
 @class		member_snapshot_ptr
 @brief		reference of member_snapshot (empty handle = no member)
*///-----------------------------------------------------------------------------
template <class TMEMBER>
class member_snapshot_ptr
{
public:
	using member_t = TMEMBER;
	using snapshot_t = member_snapshot<TMEMBER>;

public:
	constexpr					member_snapshot_ptr() noexcept {}
								member_snapshot_ptr(const member_snapshot_ptr& _rhs) noexcept : m_psnapshot(_rhs.m_psnapshot) { if (this->m_psnapshot != nullptr) this->m_psnapshot->add_ref(); }
								member_snapshot_ptr(member_snapshot_ptr&& _rhs) noexcept : m_psnapshot(_rhs.m_psnapshot) { _rhs.m_psnapshot = nullptr; }
								~member_snapshot_ptr() noexcept { if (this->m_psnapshot != nullptr) this->m_psnapshot->release(); }

			member_snapshot_ptr& operator=(const member_snapshot_ptr& _rhs) noexcept { member_snapshot_ptr temp(_rhs); this->swap(temp); return *this; }
			member_snapshot_ptr& operator=(member_snapshot_ptr&& _rhs) noexcept { member_snapshot_ptr temp(std::move(_rhs)); this->swap(temp); return *this; }

	//! @brief takes a reference which is already owned. (no add_ref)
	[[nodiscard]] static member_snapshot_ptr adopt(snapshot_t* _psnapshot) noexcept { member_snapshot_ptr temp; temp.m_psnapshot = _psnapshot; return temp; }
	[[nodiscard]] snapshot_t*	detach() noexcept { auto p = this->m_psnapshot; this->m_psnapshot = nullptr; return p; }
			void				swap(member_snapshot_ptr& _rhs) noexcept { std::swap(this->m_psnapshot, _rhs.m_psnapshot); }

	[[nodiscard]] snapshot_t*	get() const noexcept { return this->m_psnapshot; }
	[[nodiscard]] explicit		operator bool() const noexcept { return this->m_psnapshot != nullptr; }
	[[nodiscard]] size_t		size() const noexcept { return (this->m_psnapshot != nullptr) ? this->m_psnapshot->size() : 0; }
	[[nodiscard]] bool			empty() const noexcept { return this->size() == 0; }
	[[nodiscard]] member_t* const* begin() const noexcept { return (this->m_psnapshot != nullptr) ? this->m_psnapshot->begin() : nullptr; }
	[[nodiscard]] member_t* const* end() const noexcept { return (this->m_psnapshot != nullptr) ? this->m_psnapshot->end() : nullptr; }

private:
			snapshot_t*			m_psnapshot { nullptr };
};

//-----------------------------------------------------------------------------
/**
 @class		member_snapshot_holder
 @brief		published member snapshot of a group (lazy copy-on-write)
 @details	- writer : changes the member list in a 'write scope' (in the group lock). the scope drops the published snapshot. (O(1))
			- reader : get(). returns the published snapshot without the group lock.
			           if there is none, builds one from the live container in the group lock and publishes it.
			           in a write scope of this thread (on_member_entered/on_member_leaving...) it returns a private snapshot of
			           the live container and does not publish it. (same members as the stage 1 snapshot)
			- m_lockable_snapshot is a leaf lock. (only pointer copy and add_ref in it)
*///-----------------------------------------------------------------------------
template <class TMEMBER>
class member_snapshot_holder
{
public:
	using member_t = TMEMBER;
	using snapshot_t = member_snapshot<TMEMBER>;
	using snapshot_ptr_t = member_snapshot_ptr<TMEMBER>;

public:
								member_snapshot_holder() noexcept {}
								member_snapshot_holder(const member_snapshot_holder&) = delete;
								~member_snapshot_holder() noexcept { if (this->m_psnapshot != nullptr) this->m_psnapshot->release(); }
			member_snapshot_holder& operator=(const member_snapshot_holder&) = delete;

public:
	//! @brief returns the member snapshot. @param _lockable group lock (m_lockable_group) @param _build builds a snapshot from the live container (called in _lockable)
	template <class TLOCKABLE, class FBUILD>
	[[nodiscard]] snapshot_ptr_t get(TLOCKABLE& _lockable, FBUILD&& _build) const
	{
		// 1) published snapshot
		{
			auto psnapshot = this->_acquire();

			if (psnapshot)
				return psnapshot;
		}

		// 2) build in the group lock
		std::lock_guard lock(_lockable);

		// check) in a write scope of this thread -> live members, not published
		if (this->m_depth_write != 0)
			return snapshot_ptr_t::adopt(_build());

		// check) published by other thread while waiting
		{
			auto psnapshot = this->_acquire();

			if (psnapshot)
				return psnapshot;
		}

		// 3) build and publish (in the group lock : a write scope can not drop it before it is published)
		auto psnapshot = snapshot_ptr_t::adopt(_build());
		this->_exchange(psnapshot.get());
		return psnapshot;
	}

	//! @brief drops the published snapshot (in the group lock)
			void				invalidate() noexcept { this->_exchange(nullptr); }

	//! @brief scope of member list change (in the group lock)
	class write_scope
	{
	public:
		explicit				write_scope(member_snapshot_holder& _holder) noexcept : m_holder(_holder) { if (this->m_holder.m_depth_write++ == 0) this->m_holder.invalidate(); }
								~write_scope() noexcept { --this->m_holder.m_depth_write; }
								write_scope(const write_scope&) = delete;
			write_scope&		operator=(const write_scope&) = delete;
	private:
			member_snapshot_holder& m_holder;
	};

	//! @brief is this thread in a write scope (call in the group lock)
	[[nodiscard]] bool			is_writing() const noexcept { return this->m_depth_write != 0; }

private:
	[[nodiscard]] snapshot_ptr_t _acquire() const noexcept
	{
		std::lock_guard lock(this->m_lockable_snapshot);

		if (this->m_psnapshot != nullptr)
			this->m_psnapshot->add_ref();

		return snapshot_ptr_t::adopt(this->m_psnapshot);
	}

	// - publishes _psnapshot (nullptr : drop). the old one is released out of m_lockable_snapshot
			void				_exchange(snapshot_t* _psnapshot) const noexcept
	{
		if (_psnapshot != nullptr)
			_psnapshot->add_ref();

		snapshot_t* pold;
		{
			std::lock_guard lock(this->m_lockable_snapshot);
			pold = this->m_psnapshot;
			this->m_psnapshot = _psnapshot;
		}

		if (pold != nullptr)
			pold->release();
	}

	mutable	std::mutex			m_lockable_snapshot;
	mutable	snapshot_t*			m_psnapshot { nullptr };
			int					m_depth_write { 0 };	// guarded by the group lock
};


}
}
