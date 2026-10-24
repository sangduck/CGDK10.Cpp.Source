//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / release 2017.01.03                        *
//*                                                                           *
//*                 Distributed Object Storage System Classes                 *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ⓒ 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
// CGServerentity::Centity
//-----------------------------------------------------------------------------
namespace CGDK
{

struct doss::statistics::repository
{
public:
			repository() noexcept {}
			repository(const repository& _rhs) noexcept;

public:
			repository&			operator = (const repository& _rhs) noexcept { set(_rhs); return *this; }

public:
			// - 통계 (relaxed atomic : 트랜잭션마다 여러 i/o thread가 올린다. 읽는 쪽(monitor)은 순서가 필요 없다)
			//   * 트랜잭션 종류마다 cache line을 나눈다. (서로 다른 종류를 처리하는 thread가 같은 cache line을 두드리지 않도록)
			//   * 패딩으로 나눈다. (alignas(64)는 object pool 메모리가 64 byte 정렬을 보장하지 않을 수 있어 쓰지 않는다)
			std::atomic<int64_t> m_statistics_checkin{ 0 };				// - 지금 checkin된 수 (++/--)
			std::atomic<int64_t> m_statistics_checkin_request{ 0 };
			std::atomic<int64_t> m_statistics_checkin_success{ 0 };
			std::atomic<int64_t> m_statistics_checkin_fail{ 0 };
			std::byte			 m_padding_checkin[64];

			std::atomic<int64_t> m_statistics_checkout_request{ 0 };
			std::atomic<int64_t> m_statistics_checkout_success{ 0 };
			std::atomic<int64_t> m_statistics_checkout_fail{ 0 };
			std::atomic<int64_t> m_statistics_checkout_forcible{ 0 };
			std::byte			 m_padding_checkout[64];

			std::atomic<int64_t> m_statistics_create_request{ 0 };
			std::atomic<int64_t> m_statistics_create_success{ 0 };
			std::atomic<int64_t> m_statistics_create_fail{ 0 };
			std::atomic<int64_t> m_statistics_remove_request{ 0 };
			std::atomic<int64_t> m_statistics_remove_complete{ 0 };
			std::byte			 m_padding_create[64];

			std::atomic<int64_t> m_statistics_command_request{ 0 };
			std::atomic<int64_t> m_statistics_command_success{ 0 };
			std::atomic<int64_t> m_statistics_command_fail{ 0 };
			std::byte			 m_padding_command[64];

			std::atomic<int64_t> m_statistics_update_request{ 0 };
			std::atomic<int64_t> m_statistics_update_success{ 0 };
			std::atomic<int64_t> m_statistics_update_fail{ 0 };

			// - 통계 값을 올리고 내린다. (relaxed)
	static	void				increase(std::atomic<int64_t>& _value) noexcept { _value.fetch_add(1, std::memory_order_relaxed); }
	static	void				decrease(std::atomic<int64_t>& _value) noexcept { _value.fetch_sub(1, std::memory_order_relaxed); }

public:
			void				set(const repository& _rhs) noexcept;

};


class doss::statistics::Nrepository
{
public:
	// 1) repository info
			auto& statistics() const noexcept { return m_statistics; }
			auto& get_repository_id() const noexcept { return m_repository_id;}

protected:
	// 2) statistics
			doss::statistics::repository m_statistics;

	// 3) repository id
			sREPOSITORY_ID		m_repository_id;
};



}