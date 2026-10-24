//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            Server Log Classes                             *
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

@class		Ilogger


*/
//-----------------------------------------------------------------------------
class CGDK::Ilogger :
	virtual public				Ireferenceable
{
public:
			Ilogger() noexcept {}
	virtual	~Ilogger() noexcept	{}

public:
			void				trace(object_ptr<LOG_RECORD>&& _plog_record);

			void				set_filter(Ilog_filter* _plog_filter);
	[[nodiscard]] object_ptr<Ilog_filter> get_filter() const;
	[[nodiscard]] size_t		get_log_count() const noexcept { return this->m_count_log_total.load(std::memory_order_relaxed); }
	[[nodiscard]] size_t		get_log_count(log_type_t _type) const noexcept { return this->m_count_log.at(static_cast<int>(_type)).load(std::memory_order_relaxed); }
			void				reset_log_count() noexcept;
	[[nodiscard]] lock_object&	get_count_lock() noexcept { return this->m_lockable_count_log; }

	[[nodiscard]] std::array<size_t, MAX_LOG_TYPE> _log_count() const noexcept;

			void				detach_from_parent() noexcept;

protected:
	virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) PURE;
			void				process_count_log(log_type_t _type) noexcept;

	// - the filter now (no lock : the loggers read it for every log)
	//   a filter set is kept until this logger is destroyed (a logger may use the old one while it is changed)
	[[nodiscard]] Ilog_filter*	_get_filter() const noexcept { return this->m_pfilter_now.load(std::memory_order_acquire); }

protected:
			mutable lock_object	m_lockable_filter;
			object_ptr<Ilog_filter>	m_plog_filter;
			std::vector<object_ptr<Ilog_filter>> m_vector_filter_old;
			std::atomic<Ilog_filter*> m_pfilter_now{ nullptr };

	// - counters are atomic (relaxed) : m_lockable_count_log is only for the readers which want a consistent snapshot
			std::atomic<size_t>	m_count_log_total{ 0 };
			std::array<std::atomic<size_t>, MAX_LOG_TYPE> m_count_log{};
			lock_object			m_lockable_count_log;
			object::Iattachable<Ilogger>* m_pparent{ nullptr };
			friend class logger_hub;
};
