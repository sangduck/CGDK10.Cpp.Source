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
@class		LOG_RECORD
*/
//-----------------------------------------------------------------------------
class CGDK::LOG_RECORD : virtual public Ireferenceable, public Ibuffer_serializable, public factory::_traits_system
{
public:
	virtual ~LOG_RECORD() noexcept {}

			void				clear() noexcept;
			auto				clone(eCHARACTER_SET _character_set = eCHARACTER_SET::NONE) const -> object_ptr<LOG_RECORD>;

	template<class T>
			void				set_message(const std::basic_string_view<T> _message, eCHARACTER_SET _character_set = _DEFAULT_CHAR_CHARACTER_SET);
	template<class T>
			void				add_message(const std::basic_string_view<T>, eCHARACTER_SET _character_set = _DEFAULT_CHAR_CHARACTER_SET);
			void				add_sub_log(object_ptr<LOG_RECORD>&& _sub_log);
			void				remove_front_sub_log() noexcept;
	[[nodiscard]] const std::vector<object_ptr<LOG_RECORD>>& sub_plog() const noexcept { return this->m_sub_plog; }

	// - a log record isn't changed once it is traced : the loggers read it on their threads without lock
	//   (set_message, add_message, add_sub_log, remove_front_sub_log and clear are only for a record not traced yet)
	[[nodiscard]] bool			is_traced() const noexcept { return this->m_is_traced.load(std::memory_order_relaxed); }
			void				_set_traced() noexcept { if (this->m_is_traced.load(std::memory_order_relaxed) == false) this->m_is_traced.store(true, std::memory_order_relaxed); }

	[[nodiscard]] size_t		line_count() const noexcept;
	[[nodiscard]] bool			empty() const noexcept { return buf_message.empty(); }

			auto				operator >> (std::ostream& _stream_out) const -> const LOG_RECORD& { stream_out(_stream_out); return *this; }
			void				stream_out(std::ostream& _stream_out, eCHARACTER_SET _character_set = _DEFAULT_CHAR_CHARACTER_SET) const;
			size_t				get_log_message(shared_buffer& _dst, eCHARACTER_SET _character_set_dst, size_t _offset_src) const;

protected:
	virtual	void				process_serialize_in(CGDK::buffer_view& _buffer) override;
	virtual	void				process_serialize_out(CGDK::buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;

public:
	log_type_t					type{ log_type_t::undefined };
	log_level_t					level;
	result_code					result;

	eCHARACTER_SET				char_encoding{ _DEFAULT_CHAR_CHARACTER_SET };
	shared_buffer				buf_message;

	log_origin_t				origin;
	log_attribute_t				attribute;
	log_source_t				source;
	log_destination_t			destination;
	chrono::time::utc::time_point time_occurred;

	object_ptr<Ilog_source>		plog_source;

protected:
	std::vector<object_ptr<LOG_RECORD>> m_sub_plog;
	std::atomic<bool>			m_is_traced{ false };

public:
	// - link of the lock-free queue of logger_hub_archive (valid only while the record is queued)
	LOG_RECORD*					_pnext_queued{ nullptr };

	static auto					_alloc() -> object_ptr< LOG_RECORD>;
	static void					append_text_date(shared_buffer& _buf_dst, chrono::time::utc::time_point _time_occurred, eCHARACTER_SET _character_set);
	static size_t				get_buffer_size(buffer_view _buf_source, eCHARACTER_SET _character_set_src, eCHARACTER_SET _character_set_dst) noexcept;
};


//-----------------------------------------------------------------------------
/**
@class		sMESSAGE_LOG
*/
//-----------------------------------------------------------------------------
struct CGDK::sMESSAGE_LOG : public sMESSAGE
{
public:
	sMESSAGE_LOG() noexcept : sMESSAGE(eMESSAGE::SYSTEM::LOG_) {}
	sMESSAGE_LOG(LOG_RECORD* _plog_record) noexcept : sMESSAGE(eMESSAGE::SYSTEM::LOG_), plog_record(_plog_record) {}
	sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set, const std::string_view _message);
	sMESSAGE_LOG(log_type_t _type, log_level_t _level, eCHARACTER_SET _character_set, const std::string_view _message, const chrono::time::utc::time_point& _time_occurred);

public:
	object_ptr<LOG_RECORD>	plog_record;
};
