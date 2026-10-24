//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                  Common                                   *
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

 @class		Ireferenceable
 
*/
//-----------------------------------------------------------------------------
class CGDK::Ireferenceable : virtual public Idisposable
{
// constructor/destructor) 
protected:
			Ireferenceable() noexcept {}
	explicit Ireferenceable(const Ireferenceable&) noexcept {}
			Ireferenceable(Ireferenceable&&) noexcept {}
public:
	virtual	~Ireferenceable() noexcept {}

// public) 
#ifdef NDEBUG
	#define		DEBUG_ON_ADD_REFERENCECOUNT(count)	
	#define		DEBUG_ON_RELEASE_REFERENCECOUNT
#else
	#define		DEBUG_ON_ADD_REFERENCECOUNT(count)	_DEBUG_on_add_reference_count(count)
	#define		DEBUG_ON_RELEASE_REFERENCECOUNT		_DEBUG_on_release_reference_count()
#endif

public:
	//! @brief set reference count(on_final_release/process_dispose is not called) 
			void				reset_reference_count() noexcept { m_counter = 0;}
			void				set_reference_count(int _count) noexcept { m_counter = _count;}

	//! @brief increase reference count 1 @return 변공된 참조계수 값
	//   - relaxed : 참조를 이미 가진 쪽만 add_ref한다. (순서는 release(acq_rel)가 맞춘다. 표준적인 참조계수 방식)
	//     x86은 같은 명령(lock xadd), arm64는 barrier가 없다. 반환값으로 다른 data를 판단하지 않는다.
			int					add_ref() noexcept { DEBUG_ON_ADD_REFERENCECOUNT(1); return m_counter.fetch_add(1, std::memory_order_relaxed) + 1;}
			int					add_ref(int _count) noexcept { DEBUG_ON_ADD_REFERENCECOUNT(_count); CGASSERT_ERROR(_count>=0); return m_counter.fetch_add(_count, std::memory_order_relaxed) + _count;}

	//! @brief 참조가 남아 있을 때만 참조계수를 1 올린다. @return 0이면(해제 중이거나 pool에 돌아간 객체) 올리지 않고 false
	//   - 다른 곳이 참조를 갖지 않은 채 raw pointer로 들고 있는 객체(예 : socket sustain 목록)를 잠시 붙잡을 때 쓴다.
	[[nodiscard]] bool		add_ref_if_alive() noexcept { int count = m_counter.load(std::memory_order_relaxed); while (count > 0) { if (m_counter.compare_exchange_weak(count, count + 1, std::memory_order_relaxed)) { DEBUG_ON_ADD_REFERENCECOUNT(1); return true; } } return false; }

	//! @brief decrease reference and call count on_final_release/process_dispose on reference count being zero.
			int					release() noexcept;
			int					release(int _count) noexcept;

	[[nodiscard]] int			reference_count() const noexcept { return m_counter.load(std::memory_order_acquire);}

			Ireferenceable&		operator ++ () noexcept { add_ref(); return *this;}
			Ireferenceable&		operator -- () noexcept { release(); return *this;}
			void				operator += (int _rhs) noexcept { add_ref(_rhs);}

			Ireferenceable&		operator = (const Ireferenceable&) noexcept { return *this;}
			Ireferenceable&		operator = (Ireferenceable&&) noexcept { return *this; }

			// statistics)
			void				statistics_on_execute_elapsed_time(chrono::tick::duration _elapsed_time) noexcept;
	[[nodiscard]] Ifactory*		get_factory() const noexcept  { return m_pfactory;}

// framework) 
protected:
	// 1) being called on reference count is 0
	virtual	void				on_final_release() noexcept {}

			void				_process_on_final_release() noexcept;

#ifndef NDEBUG
public:
	// debug) 
	virtual	void				_DEBUG_on_add_reference_count(int /*_count*/){}
	virtual	void				_DEBUG_on_release_reference_count() {}
#endif

// implementation) 
private:
			std::atomic<int>	m_counter { 0 };

private:
			Ifactory*			m_pfactory {nullptr};
			template<class> friend class factory::object_pool_tls;
			template<class> friend class Nreferenceable;
			friend class Ischedulable;
};

