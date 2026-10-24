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

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		TLOG_RECORD


*/
//-----------------------------------------------------------------------------
class TLOG_RECORD :
	public						LOG_RECORD,
	public						Ipoolable<TLOG_RECORD>,
	public						factory::_traits_has_pool,			// - the pool sets m_ppool (without it, every record was deleted, not pooled)
	public						factory::_traits_has_on_dealloc
{
public:
	static object_ptr<TLOG_RECORD> _alloc();

	virtual	void				process_dispose() noexcept;

	// - a pooled record is reused as it is : resets every field when it returns to the pool
	//   (or the next log gets the sub logs, the log source... of the previous log)
			void				process_on_pool_dealloc() noexcept;

	// 2) Object
	object_ptr<default_factory_pool_t<TLOG_RECORD>> m_ppool;

	// - max size of the message buffer kept in a pooled record
	static constexpr std::size_t MAX_POOLED_MESSAGE_BUFFER = 1024;
};

extern own_ptr<default_factory_pool_t<TLOG_RECORD>> g_factory_log_record;

inline object_ptr<TLOG_RECORD> TLOG_RECORD::_alloc()
{
	// check) the pool is destroyed at the end of the process (static destruction) : no record (the log is dropped)
	//        (a log while or after the pool was destroyed referenced the null pool)
	if (g_factory_log_record.empty())
		return object_ptr<TLOG_RECORD>();

	return g_factory_log_record->alloc(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
}

inline void TLOG_RECORD::process_on_pool_dealloc() noexcept
{
	this->type = log_type_t::undefined;
	this->level = log_level_t();
	this->result = result_code();
	this->char_encoding = _DEFAULT_CHAR_CHARACTER_SET;
	// - the message buffer is kept for the next log when only this record has it and it is small
	//   (it is emptied : _log_stream writes the header again)
	if (auto& buf = this->buf_message; buf.data() != nullptr && buf.get_bound().lower != nullptr
	 && static_cast<std::size_t>(static_cast<const char*>(buf.get_bound().upper) - static_cast<const char*>(buf.get_bound().lower)) <= MAX_POOLED_MESSAGE_BUFFER
	 && buf.is_source_unique())
	{
		buf.set_size(0);
		buf.set_data(static_cast<char*>(const_cast<void*>(buf.get_bound().lower)));
	}
	else
	{
		this->buf_message = shared_buffer();
	}
	this->origin = log_origin_t();
	this->attribute = log_attribute_t();
	this->source = log_source_t();
	this->destination = log_destination_t();
	this->time_occurred = chrono::time::utc::time_point();
	this->plog_source = nullptr;
	this->m_sub_plog.clear();
	this->m_is_traced.store(false, std::memory_order_relaxed);
}

inline void TLOG_RECORD::process_dispose() noexcept
{
	if (this->m_ppool.exist())
		this->m_ppool->dealloc(this);
	 else
		delete this;
}


}