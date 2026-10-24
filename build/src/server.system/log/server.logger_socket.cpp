#include "pch.h"

using namespace CGDK;


logger_socket::logger_socket()
{
}

logger_socket::~logger_socket() noexcept
{
	this->destroy();
}

result_code logger_socket::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	return this->initialize(_initialize_param.address);
}

result_code logger_socket::initialize(const std::string_view _address)
{
	// declare)
	context	context_setting;

	// 1) "host:port" / "[ipv6]:port" -> "remote_endpoint" { "address", "port" } (the setting of the socket)
	//    ("address" at the top was not read by the socket : it was never connected)
	std::string_view address = _address;
	std::string_view port;

	if (address.empty() == false && address.front() == '[')
	{
		const auto pos_close = address.find(']');
		if (pos_close != std::string_view::npos)
		{
			if (pos_close + 1 < address.size() && address[pos_close + 1] == ':')
				port = address.substr(pos_close + 2);
			address = address.substr(1, pos_close - 1);
		}
	}
	else if (const auto pos_colon = address.rfind(':'); pos_colon != std::string_view::npos && address.find(':') == pos_colon)
	{
		port = address.substr(pos_colon + 1);
		address = address.substr(0, pos_colon);
	}

	context_setting["remote_endpoint"sv]["address"sv] = address;
	if (port.empty() == false)
		context_setting["remote_endpoint"sv]["port"sv] = port;

	// return)
	return this->Ninitializable::initialize(context_setting);
}

void logger_socket::process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record)
{
	// 1) get socket
	auto psocket = this->m_psocket_log.get_object_ptr();

	// check)
	if (psocket.empty())
		return;

	// declare) the header of a message : [size(uint32_t)][LOG_(uint32_t)][count(COUNT_T)]
	constexpr std::size_t SIZE_HEADER = sizeof(uint32_t) * 2 + sizeof(COUNT_T);

	// - the filter (once for the batch)
	const auto pfilter = this->_get_filter();

	// 2) the records sent (filtered) and the sizes
	std::vector<std::pair<LOG_RECORD*, std::size_t>> vector_send;
	vector_send.reserve(_vector_plog_record.size());

	for (auto& iter : _vector_plog_record)
	{
		// check) message must be not empty
		if (iter.empty() || iter->buf_message.empty())
			continue;

		// - filtering
		if (pfilter != nullptr && pfilter->process_filtering(iter) == false)
			continue;

		// statistics)
		this->process_count_log(iter->type);

		vector_send.emplace_back(iter.get(), get_size_of(iter));
	}

	// 3) messages (split by MAX_MESSAGE_SIZE : a record larger than it is sent alone)
	try
	{
		std::size_t index = 0;
		while (index < vector_send.size())
		{
			// - the records of this message
			std::size_t size_message = SIZE_HEADER + vector_send[index].second;
			std::size_t index_end = index + 1;
			while (index_end < vector_send.size() && size_message + vector_send[index_end].second <= MAX_MESSAGE_SIZE)
			{
				size_message += vector_send[index_end].second;
				++index_end;
			}

			// - write message
			auto temp_buffer = alloc_shared_buffer(size_message);
			temp_buffer.append<uint32_t>();
			temp_buffer.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
			temp_buffer.append<COUNT_T>(static_cast<COUNT_T>(index_end - index));

			for (std::size_t i = index; i < index_end; ++i)
				temp_buffer.append(object_ptr<LOG_RECORD>(vector_send[i].first));

			// - write message length
			temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

			// - send
			psocket->send(temp_buffer);

			index = index_end;
		}
	}
	catch (...)
	{
		// - the logs of this batch are lost (not logged : the error log comes back to this logger)
	}
}

result_code logger_socket::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_SOCKET["sv << name() << "]"sv;

	// 1) Nlogger ("filter")
	auto result = this->Nlogger::_process_initializing(_context);

	// 2) log 정보를 reset한다.
	this->reset_log_count();

	// 3) 생성한다. (enable_reconnect was called before it was created : a null dereference)
	auto psocket = make_own<net::socket::tcp_client_reconnect<>>();

	// 4) reconnect를 enable한다.
	psocket->enable_reconnect();

	// 5) socket을 시작한다.
	psocket->start(_context);

	// 6) 옮긴다.
	this->m_psocket_log = std::move(psocket);

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_SOCKET["sv << name() << "]"sv;

	// return)
	return result;
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

	// 4) wait for the completion of the queued logs (sent before the disconnection)
	this->_process_wait_complete();

	// 5) log 접속용 Socket을 얻는다.
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
