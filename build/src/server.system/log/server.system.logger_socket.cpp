#include "pch.h"

using namespace CGDK;


logger_socket::logger_socket()
{
}

logger_socket::logger_socket(const std::string_view _name) :
	Nlogger(_name)
{
}

logger_socket::~logger_socket() noexcept
{
	this->destroy();
}

result_code logger_socket::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	// declare)
	context	context_setting;

	// 1) 값을 설정한다.
	context_setting["address"sv] = _initialize_param.address;

	// return) 
	return this->Ninitializable::initialize(context_setting);
}

result_code logger_socket::initialize(const std::string_view _address)
{
	// declare)
	context	context_setting;

	// 1) 값을 설정한다.
	context_setting["address"sv] = _address;

	// return) 
	return this->Ninitializable::initialize(context_setting);
}

void logger_socket::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _vector_log_record must not empty
	RETURN_IF(_vector_log_record.empty());

	// check) 
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// 1) get socke
	auto psocket = this->m_psocket_log.get_object_ptr();

	// check) 
	RETURN_IF(psocket.empty());

	// declare) 
	size_t required_buffer_size = 8 + sizeof(COUNT_T);

	// 2) log_records들의 크기를 얻는다.
	for (auto& iter : _vector_log_record)
	{
		// check)
		CONTINUE_IF(iter.empty());

		required_buffer_size += get_size_of(iter);
	}

	// 3) alloc memory shared_buffer
	auto temp_buffer = alloc_shared_buffer(required_buffer_size);

	// 4) write message
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
	auto& ref_count = temp_buffer.append<COUNT_T>();

	// declare)
	int count = 0;

	// 5) append LOG_RECORD
	for (const auto& iter : _vector_log_record)
	{
		// check) message must be not empty
		CONTINUE_IF(iter->buf_message.empty());

		// - filtering
		if (this->m_plog_filter.exist())
		{
			CONTINUE_IF(this->m_plog_filter->process_filtering(iter) == false);
		}

		// statistics) 
		this->process_count_log(iter->type);

		// - append log_record
		temp_buffer.append(iter);

		// - increase count
		++count;
	}

	// 6) write LOG_RECORD count
	ref_count = count;

	// 7) write message length
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// 8) send
	psocket->send(temp_buffer);
}

result_code logger_socket::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SOCKET["sv << name() << "]"sv;

	// 1) log 정보를 reset한다.
	this->reset_log_count();

	// declare)  
	own_ptr<net::socket::tcp_client_reconnect<>> psocket;

	// 2) reconnect를 enable한다.
	psocket->enable_reconnect();
	
	// 3) 생성한다.
	psocket = make_own<net::socket::tcp_client_reconnect<>>();

	// 4) socket을 시작한다.
	psocket->start(_context);

	// 5) 옮긴다.
	this->m_psocket_log = std::move(psocket);

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_SOCKET["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_socket::_process_destroying() noexcept
{
	// 1) default tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	this->detach_from_parent();

	// 3) on_destroying 함수를 호출한다.
	try
	{
		this->on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 4) log 접속용 Socket을 얻는다.
	auto psocket = this->m_psocket_log.get_object_ptr();

	// 5) socket이 존재하면 접속 종료처리를 한다.
	if(psocket.exist())
	{
		// - destroy처리....
		psocket->disable_reconnect();

		// - 접속을 종료처리한다.
		psocket->disconnect();
	}

	// return)
	return eRESULT::SUCCESS;
}
