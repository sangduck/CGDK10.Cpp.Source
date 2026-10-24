#include "pch.h"

using namespace CGDK;


class logger_acceptor::socket :
	public			net::socket::tcp<>
{
public:
			socket() noexcept {}
	virtual ~socket() noexcept {}

protected:
	virtual void			on_connect() override;
	virtual void			on_disconnect(uint64_t _reason) override;

	virtual result_code		on_message(sMESSAGE_NETWORK& _msg) override;
};

void logger_acceptor::socket::on_connect()
{
}

void logger_acceptor::socket::on_disconnect(uint64_t /*_reason*/)
{
}

result_code logger_acceptor::socket::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	return eRESULT::BYPASS;
}

logger_acceptor::logger_acceptor() :
	net::io::connective::acceptor::Ntcp()
{
}

logger_acceptor::~logger_acceptor() noexcept
{
	destroy();
}

result_code logger_acceptor::initialize(const INITIALIZE_PARAMETER& _initialize_param)
{
	// declare)
	context context_setting;

	// 1) 값을 설정한다.
	context_setting["address"sv] = _initialize_param.address;

	// return) 
	return Ninitializable::initialize(context_setting);
}

result_code logger_acceptor::initialize(const std::string_view _address)
{
	// declare)
	context context_setting;

	// 1) 값을 설정한다.
	context_setting["address"sv] = _address;

	// return) 
	return Ninitializable::initialize(context_setting);
}

void logger_acceptor::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_plog_record.empty(),);

	// check) 
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// 1) get socket_log temporary
	auto psocket = this->m_psocket_log.get_object_ptr();

	// check) 
	RETURN_IF(psocket.empty());

	try
	{
		// 4) alloc memory shared_buffer
		shared_buffer temp_buffer = alloc_shared_buffer(256 + sizeof(COUNT_T) + get_size_of(_plog_record));

		// 5) write message
		temp_buffer.append<uint32_t>();
		temp_buffer.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
		auto& ref_count = temp_buffer.append<COUNT_T>();

		// declare)
		int count = 0;

		// 6) append LOG_RECORD
		if (auto& iter = _plog_record; true)
		{
			// check) 
			RETURN_IF(iter->buf_message.empty());

			// - filtering
			if (auto pfilter = this->_get_filter(); pfilter != nullptr)
			{
				RETURN_IF(pfilter->process_filtering(iter) == false);
			}

			// - log Count
			process_count_log(iter->type);

			// - append LOG_RECORD
			temp_buffer.append(iter);

			// - increase count
			++count;
		}

		// 7) write LOG_RECORD count
		ref_count = count;

		// 8) write message length
		temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

		// 9) send
		psocket->send(temp_buffer);
	}
	catch (std::exception& _e)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing log to socket("sv << _e.what() << "), logger: '"sv << this->name() << "'"sv;
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing by unknowned exception on writing log to socket, logger: '"sv << this->name() << "'"sv;
	}
}

result_code logger_acceptor::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_ACCEPTOR["sv << name() << "]"sv;

	// 1) Log 정보를 reset한다.
	this->reset_log_count();

	// declare)  
	own_ptr<net::socket::tcp_client_reconnect<>> psocket;

	// 2) Reconnect를 enable한다.
	psocket->enable_reconnect();
	
	// 3) 생성한다.
	psocket = make_own<net::socket::tcp_client_reconnect<>>();

	// 4) call on_start
	this->on_start(_context);

	// 5) Socket을 시작한다.
	psocket->start(_context);

	// 6) 옮긴다.
	this->m_psocket_log = std::move(psocket);

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_ACCEPTOR["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_acceptor::_process_destroying() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) on_destroying 함수를 호출한다.
	try
	{
		this->on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 3) Log 접속용 Socket을 얻는다.
	auto psocket = this->m_psocket_log.get_object_ptr();

	// 4) Socket이 존재하면 접속 종료처리를 한다.
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

object_ptr<net::io::Iconnectable> logger_acceptor::process_alloc_connectable()
{
	return make_object<socket>();

}

bool logger_acceptor::add_connectable(net::io::Iconnectable* /*_pconnectable*/) noexcept
{
	return true;
}

bool logger_acceptor::remove_connectable(net::io::Iconnectable* /*_pconnectable*/) noexcept
{
	return true;
}

void logger_acceptor::disconnect_connectable_all() noexcept
{
}

void logger_acceptor::close_connectable_all(uint64_t /*_flag*/) noexcept
{
}

size_t logger_acceptor::count() const noexcept
{
	return 0;
}

object_ptr<net::io::Iconnectable_manager::CONNECTABLE_ITERATOR>	logger_acceptor::alloc_iterator()
{
	return object_ptr<net::io::Iconnectable_manager::CONNECTABLE_ITERATOR>();
}
