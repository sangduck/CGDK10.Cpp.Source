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
// ----------------------------------------------------------------------------
// 2. MUTEX (native)
//    
//    
// 
// ----------------------------------------------------------------------------
namespace CGDK
{
//-----------------------------------------------------------------------------
// 
// CGDK::SCOPED_LOCK - for std::shared_mutex
//
//-----------------------------------------------------------------------------
// 1) unique_lock (unnamed)
template <class Tlockable, class TENABLE = void> class _unique_lock : public std::unique_lock<lock_object>									{};
template <class Tlockable> class _unique_lock<Tlockable, std::enable_if_t<std::is_base_of_v<std::mutex, Tlockable>, void>>					{ public: using type = std::unique_lock<std::mutex>; };
template <class Tlockable> class _unique_lock<Tlockable, std::enable_if_t<std::is_base_of_v<std::recursive_mutex, Tlockable>, void>>		{ public: using type = std::unique_lock<std::recursive_mutex>; };
template <class Tlockable> class _unique_lock<Tlockable, std::enable_if_t<std::is_base_of_v<std::shared_mutex, Tlockable>, void>>			{ public: using type = std::unique_lock<std::shared_mutex>; };
template <class Tlockable> class _unique_lock<Tlockable, std::enable_if_t<std::is_base_of_v<std::timed_mutex, Tlockable>, void>>			{ public: using type = std::unique_lock<std::timed_mutex>; };

// 2) unique_lock (named)
template <class Tlockable, class TENABLE = void> class _unique_lock_named : public std::unique_lock<lock_object> { public: using type = std::unique_lock<lock_object>; };
template <class Tlockable> class _unique_lock_named<Tlockable, std::enable_if_t<std::is_base_of_v<std::mutex, Tlockable>, void>>			{ public: using type = std::unique_lock<std::mutex>; };
template <class Tlockable> class _unique_lock_named<Tlockable, std::enable_if_t<std::is_base_of_v<std::recursive_mutex, Tlockable>, void>>	{ public: using type = std::unique_lock<std::recursive_mutex>; };
template <class Tlockable> class _unique_lock_named<Tlockable, std::enable_if_t<std::is_base_of_v<std::shared_mutex, Tlockable>, void>>		{ public: using type = std::unique_lock<std::shared_mutex>; };
template <class Tlockable> class _unique_lock_named<Tlockable, std::enable_if_t<std::is_base_of_v<std::timed_mutex, Tlockable>, void>>		{ public: using type = std::unique_lock<std::timed_mutex>; };

// 3) shared_lock
template <class Tlockable, class TENABLE = void> class _shared_lock : public std::shared_lock<std::shared_mutex>							{};
template <class Tlockable> class _shared_lock<Tlockable, std::enable_if_t<std::is_base_of_v<std::shared_mutex, Tlockable>, void>>			{ public: using type = std::shared_lock<std::shared_mutex>; };

}

// ----------------------------------------------------------------------------
//  CGDK::SCOPED_LOCK
// ----------------------------------------------------------------------------
// 1) unique lock
#define	SCOPED_UNIQUE_LOCK(cs)				if(typename CGDK::_unique_lock<std::decay_t<decltype(cs)>>::type _UNIQUE_TEMPORARY_NAME(_temp_cs_lock) {const_cast<std::decay_t<decltype(cs)>&>(cs)}; true)
#define	SCOPED_UNIQUE_LOCK_NAMED(cs, name)	if(typename CGDK::_unique_lock_named<std::decay_t<decltype(cs)>>::type name {const_cast<typename std::decay<decltype(cs)>::type&>(cs)}; true)

// 2) shared lock
#define	SCOPED_SHARED_LOCK(cs)				if(typename CGDK::_shared_lock<std::decay_t<decltype(cs)>>::type _UNIQUE_TEMPORARY_NAME(_temp_cs_lock) {const_cast<std::decay_t<decltype(cs)>&>(cs)}; true)
#define	SCOPED_SHARED_LOCK_NAMED(cs, name)	if(typename CGDK::_shared_lock<std::decay_t<decltype(cs)>>::type name {const_cast<std::decay_t<decltype(cs)>&>(cs)}; true)

// 3) lock_guard
#define	SCOPED_LOCK(cs)						SCOPED_UNIQUE_LOCK(cs)
#define	SYNCHRONIZED(cs)					SCOPED_LOCK(cs)

// 4) for capability
#define	SCOPED_LOCK_NAMED(cs, name)			SCOPED_UNIQUE_LOCK_NAMED(cs, name)
#define	SCOPED_LOCK_LEAVE(name)				name.unlock();

// 5) lock_guard
#define	CONDITIONAL_LOCK(cs, name)			if(std::unique_lock<std::mutex> name {cs}; true)