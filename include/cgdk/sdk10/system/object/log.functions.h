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
//
// 1. Default Tracer
//
//-----------------------------------------------------------------------------
// 10) Default Tracer
object_ptr<Ilogger>		get_default_logger();
Ilogger*				_get_default_logger_cached() noexcept;	// - the logger is kept by the cache of this thread
object_ptr<Ilogger>		set_default_logger(object_ptr<Ilogger>&& _plogger);
object_ptr<Ilogger>		reset_default_logger(Ilogger* _plogger = nullptr);
#define	DEFAULT_LOGGER	get_default_logger()


//-----------------------------------------------------------------------------
//
// 2. _log functions
//
//-----------------------------------------------------------------------------
// 1) log function 1
void _log(Ilogger* _plogger, LOG_RECORD* _plog_record) noexcept;
void _log(Ilogger* _plogger, const std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) noexcept;

// 2) log function 2
void _log(Ilogger* _plogger, Ilog_source* _plog_source, log_source_t _source, log_type_t _type, int _level, eCHARACTER_SET _character_set, const std::string_view _message) noexcept;
void _log(Ilogger* _plogger, Ilog_source* _plog_source, log_source_t _source, log_type_t _type, int _level, eCHARACTER_SET _character_set, const std::wstring_view _message) noexcept;

}