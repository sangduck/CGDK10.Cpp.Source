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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// I_logFilter
//
// 1. I_logFilter은~
//    설정된 Log만 출력하거나 저장하기 위해 Filtering을 수행하는 Class이다.
//    일반적으로 C_logFile이나 Server MFC control의 CChild_logger_view_ex등에 사용된다.
//
// 2. I_logFilter의 핵심 함수
//		1) Filtering 함수
//
//         bool process_filtering(const sMESSAGE_LOG& _msg) PURE;
//
//         순수가상함수인 process_filtering을 재정의하여 Log를 남길 필요가 있는
//         상황에 true가 리턴되어 오도록 함수를 정의해 주면 된다.
//
//-----------------------------------------------------------------------------
class Nlogger :
	virtual public				Ilogger,
	public						object::Nnameable,
	public						object::Nstate,
	public						object::Ninitializable
{
public:
			Nlogger() noexcept = default;
	virtual ~Nlogger() noexcept {}

protected:
	virtual result_code			_process_initializing(context& _context) override;
};



//-----------------------------------------------------------------------------
//
// Nlogger_async
//
//  - the logs are written on an execution (like the send of net::socket)
//    1) process_log queues the record (lock-free : a node of this logger links the record)
//    2) the logger which queued the first record posts the execution (or it just queues)
//    3) the execution takes every queued record and writes them at once (process_log_batch)
//       until no record is queued
//  - a log record isn't changed once it is traced : the same record is queued to several loggers
//    (each logger links it with its own node, the record isn't copied)
//  - a log is just queued (a null record does nothing : no wait for the write)
//
//-----------------------------------------------------------------------------
class Nlogger_async :
	public						Nlogger,
	virtual public				Iexecutable
{
public:
			Nlogger_async() noexcept = default;
	virtual ~Nlogger_async() noexcept;

	// - node of the queue (reused : a free list)
			struct sNODE
			{
				LOG_RECORD*		precord{ nullptr };
				sNODE*			pnext{ nullptr };
			};

protected:
	virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) override;

	// - writes the records at once (called on the execution : one at a time)
	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) PURE;

	virtual	intptr_t			process_execute(intptr_t _return, size_t _param) override;
			void				_process_post();
			void				_process_wait_complete();
			size_t				_process_take_queued();

	// - lock-free queue (multi producer, single consumer)
	//   1) a producer increases m_count_queued and pushes its node to the stack (m_phead_queued)
	//   2) the producer which increased it from 0 posts the execution : only one execution runs at a time
	//   3) the execution takes the whole stack (FIFO reversed) and decreases m_count_queued by the records taken
	//      it takes again if the count isn't 0
			std::atomic<sNODE*>	m_phead_queued{ nullptr };
			std::atomic<size_t>	m_count_queued{ 0 };
			std::vector<object_ptr<LOG_RECORD>> m_vector_processing;
};


//-----------------------------------------------------------------------------
// logger factory (by the type name of the setting : "type")
//
//  - "file" (or empty) : logger_file, "console" : logger_console, "system" : logger_system,
//    "socket" : logger_socket, "syslog" : logger_syslog (linux)
//  - the others are registered by register_logger_creator
//    (ex) "slack" : registered by including "cgdk/sdk10/server.log.slack.h"
//         (the programs not using it don't link the http/ssl libraries)
//-----------------------------------------------------------------------------
using logger_creator_t = own_ptr<Nlogger>(*)();

void				register_logger_creator(std::string_view _type, logger_creator_t _fn_creator);
[[nodiscard]] own_ptr<Nlogger> create_logger(std::string_view _type);	// - empty if the type is unknown


}
