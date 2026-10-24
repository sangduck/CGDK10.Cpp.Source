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
// logger_socket
//
// 1. logger_socket은~
//     Log처리를 위한 것이다.
//
// 2. Log의 내용
//		0) Date/Time	Log가 남은 시간이다.(자동기록)
//		1) Logger		Log를 남기는 자의 이름이다.(자동 등록)
//		2) type			Log의 종류이다. Error, Progress, Warning, Info, ...
//	    3) Level		Log의 중요도이다.
//		4) Source		Log에 남기는 Event의 주체가 되는 것을 의미한다.
//		5) Dest			Log에 남기는 Event의 대상이 되는 것을 의미한다.
//		6) Where		Log에 남기는 Event가 발생하는 객체를 의미한다.
//		7) Function		Log가 남는 Event를 호출한 함수를 의미한다.
//		8) Message		남길 Log Message를 의미한다.
//
//-----------------------------------------------------------------------------
//  - asynchronous (Nlogger_async) : the logs of a batch are sent in a message (eMESSAGE::SYSTEM::LOG_ : [size][LOG_][count][records])
//    a message is split by MAX_MESSAGE_SIZE
//
//-----------------------------------------------------------------------------
class logger_socket : public Nlogger_async
{
public:
			struct INITIALIZE_PARAMETER
			{
				std::string_view address;
			};

			logger_socket();
	virtual ~logger_socket() noexcept;

			result_code			initialize() { return this->object::Ninitializable::initialize(); }
	virtual	result_code			initialize(context& _context) override { return this->object::Ninitializable::initialize(_context); }
			result_code			initialize(const INITIALIZE_PARAMETER& _initialize_param);
			result_code			initialize(const std::string_view  _address);

	// - the max size of a message (the records more than it are sent in the next message)
	static constexpr std::size_t MAX_MESSAGE_SIZE = 256 * 1024;

protected:
	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) override;

	virtual	result_code			_process_initializing(context& _context) override;
	virtual	result_code			_process_destroying() noexcept override;

			own_ptr<net::socket::tcp_client_reconnect<>> m_psocket_log;
};


}