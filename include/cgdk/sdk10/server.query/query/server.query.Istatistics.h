//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                               server.query                                *
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
//-----------------------------------------------------------------------------
//
// query::statistics
//
//
//-----------------------------------------------------------------------------
extern	int						g_statistics_db_time;

namespace query
{
namespace statistics
{

struct unit_query
{
public:
	std::atomic<uint64_t>		query_try = 0;
	std::atomic<uint64_t>		query_success = 0;
	std::atomic<uint64_t>		query_fail = 0;
	std::atomic<uint64_t>		query_retry = 0;

	std::atomic<uint64_t>		alloc_session = 0;
	std::atomic<uint64_t>		free_session = 0;

	std::atomic<uint64_t>		create_session = 0;
	std::atomic<uint64_t>		destroy_session = 0;

	// - gauges : the current numbers (query_try - success - fail, alloc - free, create - destroy). reset() does not clear them
	std::atomic<int64_t>		query_processing = 0;
	std::atomic<int64_t>		using_session = 0;
	std::atomic<int64_t>		exist_session = 0;

public:
	unit_query&	operator =(const unit_query& rhs) noexcept
	{
		query_try.store(rhs.query_try.load(std::memory_order_relaxed), std::memory_order_relaxed);
		query_success.store(rhs.query_success.load(std::memory_order_relaxed), std::memory_order_relaxed);
		query_fail.store(rhs.query_fail.load(std::memory_order_relaxed), std::memory_order_relaxed);
		query_retry.store(rhs.query_retry.load(std::memory_order_relaxed), std::memory_order_relaxed);
		alloc_session.store(rhs.alloc_session.load(std::memory_order_relaxed), std::memory_order_relaxed);
		free_session.store(rhs.free_session.load(std::memory_order_relaxed), std::memory_order_relaxed);
		create_session.store(rhs.create_session.load(std::memory_order_relaxed), std::memory_order_relaxed);
		destroy_session.store(rhs.destroy_session.load(std::memory_order_relaxed), std::memory_order_relaxed);
		query_processing.store(rhs.query_processing.load(std::memory_order_relaxed), std::memory_order_relaxed);
		using_session.store(rhs.using_session.load(std::memory_order_relaxed), std::memory_order_relaxed);
		exist_session.store(rhs.exist_session.load(std::memory_order_relaxed), std::memory_order_relaxed);

		return *this;
	}

	void reset() noexcept
	{
		// - atomic을 memset하지 않는다. (다른 thread가 올리는 중일 수 있다)
		query_try.store(0, std::memory_order_relaxed);
		query_success.store(0, std::memory_order_relaxed);
		query_fail.store(0, std::memory_order_relaxed);
		query_retry.store(0, std::memory_order_relaxed);
		alloc_session.store(0, std::memory_order_relaxed);
		free_session.store(0, std::memory_order_relaxed);
		create_session.store(0, std::memory_order_relaxed);
		destroy_session.store(0, std::memory_order_relaxed);

		// - the gauges are not cleared. (they are the current numbers. clearing them while some are in use makes them wrong for good)
	}
};

class Nquerier
{
public:																				  
			void				reset_statistics() noexcept							{ this->m_statistics_querier.reset();}
																					  
			const auto&			statistics_get_query_info() const noexcept			{ return this->m_statistics_querier;}
																					  
			uint64_t			statistics_get_query_try() const noexcept			{ return this->m_statistics_querier.query_try.load(std::memory_order_relaxed);}
			uint64_t			statistics_get_query_success() const noexcept		{ return this->m_statistics_querier.query_success.load(std::memory_order_relaxed);}
			uint64_t			statistics_get_query_fail() const noexcept			{ return this->m_statistics_querier.query_fail.load(std::memory_order_relaxed);}
			uint64_t			statistics_get_query_retry() const noexcept			{ return this->m_statistics_querier.query_retry.load(std::memory_order_relaxed);}
			// - processing/using/exist : the gauges (the current numbers). exact while counting, not cleared by reset_statistics()
			uint64_t			statistics_get_query_processing() const noexcept	{ const auto count = this->m_statistics_querier.query_processing.load(std::memory_order_relaxed); return (count > 0) ? static_cast<uint64_t>(count) : 0;}
			uint64_t			statistics_get_alloc_session() const noexcept		{ return this->m_statistics_querier.alloc_session.load(std::memory_order_relaxed);}
			uint64_t			statistics_get_free_session() const noexcept		{ return this->m_statistics_querier.free_session.load(std::memory_order_relaxed);}
			uint64_t			statistics_get_using_session() const noexcept		{ const auto count = this->m_statistics_querier.using_session.load(std::memory_order_relaxed); return (count > 0) ? static_cast<uint64_t>(count) : 0;}
			uint64_t			statistics_get_exist_session() const noexcept		{ const auto count = this->m_statistics_querier.exist_session.load(std::memory_order_relaxed); return (count > 0) ? static_cast<uint64_t>(count) : 0;}
																					  
			void				statistics_on_query_try() noexcept					{ this->m_statistics_querier.query_try.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.query_try.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.query_processing.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.query_processing.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_query_success() noexcept				{ this->m_statistics_querier.query_success.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.query_success.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.query_processing.fetch_sub(1, std::memory_order_relaxed); m_statistics_querier_total.query_processing.fetch_sub(1, std::memory_order_relaxed);}
			void				statistics_on_query_fail() noexcept					{ this->m_statistics_querier.query_fail.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.query_fail.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.query_processing.fetch_sub(1, std::memory_order_relaxed); m_statistics_querier_total.query_processing.fetch_sub(1, std::memory_order_relaxed);}
			void				statistics_on_query_retry() noexcept				{ this->m_statistics_querier.query_retry.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.query_retry.fetch_add(1, std::memory_order_relaxed);}
																					  
			void				statistics_on_alloc_session() noexcept				{ this->m_statistics_querier.alloc_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.alloc_session.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.using_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.using_session.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_free_session() noexcept				{ this->m_statistics_querier.free_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.free_session.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.using_session.fetch_sub(1, std::memory_order_relaxed); m_statistics_querier_total.using_session.fetch_sub(1, std::memory_order_relaxed);}
																					  
			void				statistics_on_create_session() noexcept				{ this->m_statistics_querier.create_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.create_session.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.exist_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.exist_session.fetch_add(1, std::memory_order_relaxed);}
			void				statistics_on_create_session(uint64_t _count) noexcept{	this->m_statistics_querier.create_session.fetch_add(_count, std::memory_order_relaxed); m_statistics_querier_total.create_session.fetch_add(_count, std::memory_order_relaxed); this->m_statistics_querier.exist_session.fetch_add(static_cast<int64_t>(_count), std::memory_order_relaxed); m_statistics_querier_total.exist_session.fetch_add(static_cast<int64_t>(_count), std::memory_order_relaxed);}
			void				statistics_on_destroy_session() noexcept			{ this->m_statistics_querier.destroy_session.fetch_add(1, std::memory_order_relaxed); m_statistics_querier_total.destroy_session.fetch_add(1, std::memory_order_relaxed); this->m_statistics_querier.exist_session.fetch_sub(1, std::memory_order_relaxed); m_statistics_querier_total.exist_session.fetch_sub(1, std::memory_order_relaxed);}
			void				statistics_on_destroy_session(uint64_t _count) noexcept{ this->m_statistics_querier.destroy_session.fetch_add(_count, std::memory_order_relaxed); m_statistics_querier_total.destroy_session.fetch_add(_count, std::memory_order_relaxed); this->m_statistics_querier.exist_session.fetch_sub(static_cast<int64_t>(_count), std::memory_order_relaxed); m_statistics_querier_total.exist_session.fetch_sub(static_cast<int64_t>(_count), std::memory_order_relaxed);}

	static	const unit_query&	statistics_get_total_query_info() noexcept			{ return m_statistics_querier_total;}

// implementation) 
private:
			unit_query			m_statistics_querier;
	static	unit_query			m_statistics_querier_total;
};


}
}


}