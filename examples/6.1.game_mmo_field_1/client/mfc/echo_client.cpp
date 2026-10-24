#include "pch.h"

bool test_tcp_echo_client::m_enable_relay_echo_test = false;
bool test_tcp_echo_client::m_enable_disconnect_on_response = false;
own_ptr<test_tcp_echo_client> g_ptest_tcp_echo_client;


shared_buffer message_USER_LOGIN_REQUEST(REQUEST_ID _request_id, ACCOUNT_INDEX _account_index)
{
	// 1) buffer를 할당받는다.
	auto buf = alloc_shared_buffer(1024);

	// 2) message header 작성
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::USER_LOGIN_REQUEST);

	// 3) meddsage body 작성
	buf.append<REQUEST_ID>(_request_id);
	buf.append<ACCOUNT_INDEX>(_account_index);

	// 4) message 길이 써넣기
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

shared_buffer message_AREA_ENTER_REQUEST(REQUEST_ID _request_id, AREA_INDEX _area_index)
{
	// 1) buffer를 할당받는다.
	auto buf = alloc_shared_buffer(1024);

	// 2) message header 작성
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::AREA_ENTER_REQUEST);

	// 3) meddsage body 작성
	buf.append<REQUEST_ID>(_request_id);
	buf.append<AREA_INDEX>(_area_index);

	// 4) message 길이 써넣기
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

shared_buffer message_AREA_ENTITY_READY(REQUEST_ID _request_id)
{
	// 1) alloc buffer
	auto buf = alloc_shared_buffer(8 + sizeof(REQUEST_ID));

	// 2) message header
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::AREA_ENTITY_READY);

	// 3) message body
	buf.append<REQUEST_ID>(_request_id);

	// 4) set message length
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

void socket_tcp::on_connect()
{
	// stati
	static ACCOUNT_INDEX temp_account_index = 1000;

	// 1) get temporary account _index
	auto account_index = temp_account_index++;

	// 2) send request login~
	this->send(message_USER_LOGIN_REQUEST(1, account_index));
}

result_code socket_tcp::on_message(sMESSAGE_NETWORK& _msg)
{
	if (test_tcp_echo_client::m_enable_disconnect_on_response == true)
	{
		closesocket();
		return eRESULT::DONE;
	}

	// decalure
	result_code result;

	switch (_msg.buf_message.front<uint32_t>(4))
	{
	case	eMESSAGE::USER_LOGIN_RESPONSE:
			result = this->on_message_USER_LOGIN_RESPONSE(_msg);
			break;

	case	eMESSAGE::AREA_ENTER_RESPONSE:
			result = this->on_message_AREA_ENTOER_RESPONSE(_msg);
			break;

	case	eMESSAGE::AREA_INFO:
			result = this->on_message_AREA_INFO(_msg);
			break;

	case	eMESSAGE::AREA_ENTITY_LIST:
			result = this->on_message_AREA_ENTITY_LIST(_msg);
			break;

	case	eMESSAGE::AREA_ENTITY_EVENT:
			result = this->on_message_AREA_ENTITY_EVENT(_msg);
			break;

	default:
			result = eRESULT::BYPASS;
			break;
	}

	// return) 
	return result;
}

result_code socket_tcp::on_message_USER_LOGIN_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1)  buf
	auto buf = _msg.buf_message + offset(8);

	// 2) extract request id
	/*auto request_id =*/ buf.extract<REQUEST_ID>();

	// 3) extract requst
	auto result = buf.extract<eRESULT>();

	// 4)...
	if (result == eRESULT::SUCCESS)
	{

	}
	else
	{
		this->closesocket();
	}

	// return) 
	return eRESULT::DONE;
}

result_code socket_tcp::on_message_AREA_ENTOER_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 2) request_id
	/*auto request_id =*/ buf.extract<REQUEST_ID>();

	// 3) extract requst
	auto result = buf.extract<eRESULT>();

	// 4)...
	if (result == eRESULT::SUCCESS)
	{
	}
	else
	{
		this->closesocket();
	}


	// return) 
	return eRESULT::BYPASS;
}

result_code socket_tcp::on_message_AREA_INFO(sMESSAGE_NETWORK& _msg)
{
	// 1)  buf
	auto buf = _msg.buf_message + offset(8);

	// 2) extract request id
	auto request_id = buf.extract<REQUEST_ID>();

	try
	{
		// 3) request_id
		auto area_info = buf.extract<sAREA_INFO>();

		// 4) set area_info
		m_area_info = std::move(area_info);

		// 5) ...

		// 6) send
		this->send(message_AREA_ENTITY_READY(request_id));
	}
	catch (...)
	{
		this->closesocket();
	}


	// return) 
	return eRESULT::DONE;
}

result_code socket_tcp::on_message_AREA_ENTITY_LIST(sMESSAGE_NETWORK& _msg)
{
	// 1) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 2) request_id
	/*auto request_id =*/ buf.extract<REQUEST_ID>();

	try
	{
		// 3) get entity count
		auto count = buf.extract<COUNT_T>();

		// 4) extract AREA_ENTITY_INFO and create entity
		for (; count > 0; --count)
		{
			// - get entity
			/*auto entity_info =*/ buf.extract<sAREA_ENTITY_INFO>();

			//// - create entity
			//process_entity_create(entity_info, buf);
		}
	}
	catch (...)
	{
		this->closesocket();
	}

	// return) 
	return eRESULT::DONE;
}

result_code socket_tcp::on_message_AREA_ENTITY_EVENT(sMESSAGE_NETWORK& /*_msg*/)
{
	// return) 
	return eRESULT::DONE;
}

void test_tcp_echo_client::add_connect_test_min(std::size_t _count) noexcept
{
	this->m_connect_test_min += _count;

	if (this->m_connect_test_min > this->m_connect_test_max)
		this->m_connect_test_max = this->m_connect_test_min;
}
void test_tcp_echo_client::sub_connect_test_min(std::size_t _count) noexcept
{
	if (this->m_connect_test_min < _count)
		this->m_connect_test_min = 0;
	else
		this->m_connect_test_min -= _count;
}
void test_tcp_echo_client::set_connect_test_min(std::size_t _count) noexcept
{
	this->m_connect_test_max = _count;
}

void test_tcp_echo_client::add_connect_test_max(std::size_t _count) noexcept
{
	this->m_connect_test_max += _count;
}
void test_tcp_echo_client::sub_connect_test_max(std::size_t _count) noexcept
{
	if (this->m_connect_test_max < 100 + _count)
		this->m_connect_test_max = 100;
	else
		this->m_connect_test_max -= _count;

	if (this->m_connect_test_max < this->m_connect_test_min)
	{
		this->m_connect_test_min = this->m_connect_test_max;
	}
}
void test_tcp_echo_client::set_connect_test_max(std::size_t _count) noexcept
{
	this->m_connect_test_max = _count;
}

void test_tcp_echo_client::enable_connect_test(bool _enable) noexcept
{
	// check)
	RETURN_IF(this->m_enable_connect_test == _enable)

	// 1) set flag
	this->m_enable_connect_test = _enable;
}
void test_tcp_echo_client::enable_traffic_test(bool _enable) noexcept
{
	// check)
	RETURN_IF(this->m_enable_traffic_test == _enable)

	// 1) set flag
		this->m_enable_traffic_test = _enable;

	if (_enable == false)
	{
		this->m_traffic_send_requested = 0;
	}
}

void test_tcp_echo_client::add_traffic_test_count_per_single(std::size_t _count) noexcept
{
	this->m_traffic_test_count_per_single += _count;
}

void test_tcp_echo_client::sub_traffic_test_count_per_single(std::size_t _count) noexcept
{
	if ((this->m_traffic_test_count_per_single + 1)  > _count)
	{
		this->m_traffic_test_count_per_single -= _count;
	}
	else
	{
		this->m_traffic_test_count_per_single  = 1;
	}
}

void test_tcp_echo_client::set_traffic_test_count_per_single(std::size_t _count) noexcept
{
	if (_count > 1)
	{
		this->m_traffic_test_count_per_single = _count;
	}
	else
	{
		this->m_traffic_test_count_per_single = 1;
	}
}

void test_tcp_echo_client::increase_traffic_test_message_size()
{
	RETURN_IF(this->m_traffic_test_selected >= 10);

	++this->m_traffic_test_selected;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}

void test_tcp_echo_client::decrease_traffic_test_message_size()
{
	RETURN_IF(this->m_traffic_test_selected == 0);

	--this->m_traffic_test_selected;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}
void test_tcp_echo_client::set_traffic_test_message_size(std::size_t _index)
{
	RETURN_IF(this->m_traffic_test_selected >= 10);
	RETURN_IF(this->m_traffic_test_selected == _index);

	this->m_traffic_test_selected = _index;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}

void test_tcp_echo_client::request_connect(std::size_t _count)
{
	for (std::size_t i = 0; i < _count; i++)
	{
		this->m_pconnector_socket->request_connect(this->m_endpoint_connect);
	}
}

void test_tcp_echo_client::request_closesocket(std::size_t _count)
{
	std::size_t disconnected = 0;

	SCOPED_LOCK(this->m_pconnector_socket->get_lockable())
	{
		// 주의) closesocket을 할 경우 m_pconnector의 connectable_list에서 제거될 수 있어
		//       iterator 무효화 현상이 발생할 수 있으므로 반드시 iterator 무효화에 대비해 처리해야 한다.

		auto iter = this->m_pconnector_socket->begin();
		auto iter_end = this->m_pconnector_socket->end();

		while (iter != iter_end)
		{
			auto iter_now = iter++;

			if ((*iter_now)->closesocket())
			{
				BREAK_IF((++disconnected) == _count);
			}
		}
	}
}

void test_tcp_echo_client::request_closesocket_all()
{
	this->m_pconnector_socket->close_connectable_all();
}

void test_tcp_echo_client::request_disconnect_socket(std::size_t _count)
{
	std::size_t disconnected = 0;

	SCOPED_LOCK(this->m_pconnector_socket->get_lockable())
	{
		// 주의) disconnect을 할 경우 m_pconnector의 connectable_list에서 제거될 수 있어
		//       iterator 무효화 현상이 발생할 수 있으므로 반드시 iterator 무효화에 대비해 처리해야 한다.

		auto iter = this->m_pconnector_socket->begin();
		auto iter_end = this->m_pconnector_socket->end();

		while (iter != iter_end)
		{
			auto iter_now = iter++;

			if ((*iter_now)->disconnect())
			{
				BREAK_IF((++disconnected) == _count);
			}
		}
	}
}

void test_tcp_echo_client::request_disconnect_socket_all()
{
	m_pconnector_socket->disconnect_connectable_all();
}

void test_tcp_echo_client::request_send()
{
	// 1) ...
	auto value = ++this->m_traffic_send_requested;

	// check) iPre가 1이면 Interlocked걸기.
	if (value == 1)
	{
		object_ptr<test_tcp_echo_client> self(this);

		system_executor::post(make_executable([=]()
		{
			self->process_execute_send();
		}));
	}
}

void test_tcp_echo_client::request_send_immediately(std::size_t _count)
{
	// 1) send
	process_send_message(m_traffic_test_selected, _count);
}

void test_tcp_echo_client::request_send_error_message_zero()
{
	auto tempBuffer = alloc_shared_buffer(256);
	tempBuffer.append<uint32_t>(0);
	tempBuffer.append<uint32_t>(1);

	process_send_message(tempBuffer);
}

void test_tcp_echo_client::on_starting(context& /*_context*/)
{
	// 1) connect test
	this->m_enable_connect_test = false;
	this->m_connect_test_min = 500;
	this->m_connect_test_max = 800;

	// 2) send test
	this->m_traffic_send_requested = 0;
	this->m_enable_traffic_test = false;
	this->m_traffic_test_selected = 0;
	this->m_traffic_test_count_per_single = 200;

	// 3) ready messges for send test
	this->process_ready_send_test_message(this->m_traffic_test_selected);

	// 4) Connector를 생성한다.
	this->m_pconnector_socket = make_own<net::connector<socket_tcp>>();
	this->m_endpoint_connect = net::ip::tcp::endpoint{ net::ip::tcp::resolver().resolve("localhost", "33020") };
	//m_endpoint_connect = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), 33020 };

	// 5) disable test
	enable_connect_test(false);
	enable_traffic_test(false);

	// 6) Connector를 시작한다.
	this->m_pconnector_socket->start();

	// 7) start schedulable
	system_executor::register_schedulable(this);
}

struct TEST_MESSAGE_TYPE
{
	std::size_t size;
	std::size_t count;
};

#define BUFFER_SIZE_SCALE 2
#define TEST_MESSAGE_TYPE_COUNT 12
const TEST_MESSAGE_TYPE g_test_message_info[TEST_MESSAGE_TYPE_COUNT]
{
	{                8, 1024 * 1024 * BUFFER_SIZE_SCALE}, // 0:   8 byte
	{               64,  256 * 1024 * BUFFER_SIZE_SCALE}, // 1:  64 byte
	{              256,   64 * 1024 * BUFFER_SIZE_SCALE}, // 2: 256 byte
	{             1024,   16 * 1024 * BUFFER_SIZE_SCALE}, // 3:  1K byte
	{         4 * 1024,    2 * 1024 * BUFFER_SIZE_SCALE}, // 4:  4K byte
	{        16 * 1024,    1 * 1024 * BUFFER_SIZE_SCALE}, // 5: 16K byte
	{        64 * 1024,         256 * BUFFER_SIZE_SCALE}, // 6: 64K byte
	{       256 * 1024,          64 * BUFFER_SIZE_SCALE}, // 6:256K byte
	{  1 * 1024 * 1024,          16 * BUFFER_SIZE_SCALE}, // 7:  1M byte
	{  4 * 1024 * 1024,           4 * BUFFER_SIZE_SCALE}, // 8:  4M byte
	{ 32 * 1024 * 1024,           1},                     // 9: 32M byte
	{256 * 1024 * 1024,           1}                      // 10:256M byte
};

CGDK::shared_buffer generate_message(size_t _message_size, size_t _count)
{
	// check)
	CGASSERT_ERROR(_message_size > 0);

	auto buffer_temp = alloc_shared_buffer(_message_size);
	buffer_temp.append<uint32_t>(static_cast<uint32_t>(_message_size));
	for (std::size_t k = 0; k < (_message_size - 8); k += sizeof(uint32_t))
		buffer_temp.append<uint32_t>(rand());
	buffer_temp.front<uint32_t>() = static_cast<uint32_t>(buffer_temp.size() + SIZE_OF_CRC);
	buffer_temp.append(crc());

	auto buf_message = alloc_shared_buffer(_message_size * _count);
	for (std::size_t j = 0; j < _count; ++j)
	{
		buf_message += buffer_temp;
	}

	return buf_message;
}

void test_tcp_echo_client::process_ready_send_test_message(std::size_t _index)
{
	// check)
	CGASSERT_ERROR(_index < TEST_MESSAGE_TYPE_COUNT);

	// check)
	THROW_IF(_index >= TEST_MESSAGE_TYPE_COUNT, std::exception());

	// check)
	RETURN_IF(this->m_traffic_test_buf_message[_index].exist());

	// check)
	RETURN_IF(this->m_enable_relay_echo_test == true);

	// 1) get message_buffer
	this->m_traffic_test_buf_message[_index] = generate_message(g_test_message_info[_index].size, g_test_message_info[_index].count);
}

void test_tcp_echo_client::on_stopping()
{
	// 1) disable test
	this->m_enable_connect_test = false;
	this->m_enable_traffic_test = false;

	// 2) unregister...
	system_executor::unregister_schedulable(this);

	// 3) destry
	this->m_pconnector_socket->destroy();
}

void test_tcp_echo_client::process_execute_send()
{
	// check)
	CGASSERT_ERROR(this->m_traffic_test_count_per_single != 0);

	// 1) send
	process_send_message(this->m_traffic_test_selected, this->m_traffic_test_count_per_single);

	// check)
	RETURN_IF(this->m_enable_traffic_test == false);

	// 2) ...
	auto value = --m_traffic_send_requested;

	// check) 0이 아닐 경우 또 걸기...
	if (value != 0)
	{
		object_ptr<test_tcp_echo_client> self(this);

		system_executor::post(make_executable([=]() 
		{
			self->process_execute_send();
		}));
	}
}

intptr_t test_tcp_echo_client::process_execute(intptr_t /*_return*/, std::size_t /*_param*/)
{
	// 1) traffice test
	this->process_traffic_test();

	// 2) connection test
	this->process_connect_test();

	// return) 
	return 0;
}

void test_tcp_echo_client::process_connect_test()
{
	// check) 
	RETURN_IF(m_enable_connect_test == false);

	// 1) disconnect
	std::size_t disconnected = 0;

	SCOPED_LOCK(this->m_pconnector_socket->get_lockable())
	{
		auto count_socket = this->m_pconnector_socket->count();

		if (count_socket > this->m_connect_test_min)
		{
			std::size_t range = count_socket - this->m_connect_test_min;
			std::size_t disconnect = (range) ? rand() % range : 0;

			// 주의) closesocket을 할 경우 m_pconnector의 connectable_list에서 제거될 수 있어
			//       iterator 무효화 현상이 발생할 수 있으므로 반드시 iterator 무효화에 대비해 처리해야 한다.

			auto iter = this->m_pconnector_socket->begin();
			auto iter_end = this->m_pconnector_socket->end();

			while(iter != iter_end)
			{
				auto iter_now = iter++;

				if ((*iter_now)->closesocket())
				{
					BREAK_IF((++disconnected) == disconnect);
				}
			}
		}
	}

	// 2) connect
	{
		auto count_socket = this->m_pconnector_socket->allocated_socket_count() - disconnected;

		std::size_t nConnect;
		if (count_socket < this->m_connect_test_min)
		{
			// - min connect보다 낮을 시 min connect만큼 한꺼번에 접속한다.
			nConnect = this->m_connect_test_min - count_socket;
		}
		else if (count_socket < this->m_connect_test_max)
		{
			auto range = this->m_connect_test_max - count_socket;

			// - 최대 여유 remained/8(12.5%)의 값을 신규로 접속시도한다.
			nConnect = (range) ? rand() % range : 0;
		}
		else
		{
			nConnect = 0;
		}

		process_connect(nConnect);
	}
}

void test_tcp_echo_client::process_traffic_test()
{
	// check)
	RETURN_IF(this->m_enable_traffic_test == false);

	// 1) send 
	this->request_send();
}

void test_tcp_echo_client::process_connect(std::size_t _n)
{
	// 1) 접속한다.
	for (std::size_t i = 0; i < _n; i++)
	{
		this->m_pconnector_socket->request_connect(m_endpoint_connect);
	}
}

void test_tcp_echo_client::process_send_message(std::size_t _index, std::size_t _count)
{
	try
	{
		const auto message_info = g_test_message_info[_index];
		auto buf_message = m_traffic_test_buf_message[_index];

		SCOPED_LOCK(this->m_pconnector_socket->get_lockable())
		{
			for (auto& iter : *this->m_pconnector_socket)
			{
				std::size_t	nSend = _count;
				auto buf_send = buf_message;

				for (; nSend >= message_info.count; nSend -= message_info.count)
				{
					iter->send({ buf_send, message_info.count });
				}

				if (nSend != 0)
				{
					buf_send.set_size(message_info.size * nSend);
					iter->send({ buf_send, nSend });
				}
			}
		}
	}
	catch (...)
	{
	}
}

void test_tcp_echo_client::process_send_message(shared_buffer _buffer)
{
	try
	{
		SCOPED_LOCK(this->m_pconnector_socket->get_lockable())
		{
			for (auto& iter : *this->m_pconnector_socket)
			{
				iter->send(_buffer);
			}
		}
	}
	catch (...)
	{
	}
}