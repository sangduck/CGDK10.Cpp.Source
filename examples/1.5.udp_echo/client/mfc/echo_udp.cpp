#include "pch.h"
#include "echo_udp.h"

bool test_udp_echo_client::m_enable_relay_echo_test = false;
own_ptr<test_udp_echo_client> g_ptest_udp_echo_client;


result_code socket_udp::on_message(sMESSAGE_NETWORK& _msg)
{
	if (test_udp_echo_client::m_enable_relay_echo_test == true)
	{
		this->send_to(_msg.buf_message, _msg.address);
	}

	return eRESULT::DONE;
}

void test_udp_echo_client::enable_traffic_test(bool _enable) noexcept
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

void test_udp_echo_client::add_traffic_test_count_per_single(int64_t _count) noexcept
{
	this->m_traffic_test_count_per_single += _count;
}

void test_udp_echo_client::sub_traffic_test_count_per_single(int64_t _count) noexcept
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

void test_udp_echo_client::set_traffic_test_count_per_single(int64_t _count) noexcept
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

void test_udp_echo_client::increase_traffic_test_message_size()
{
	RETURN_IF(this->m_traffic_test_selected >= 10);

	++this->m_traffic_test_selected;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}

void test_udp_echo_client::decrease_traffic_test_message_size()
{
	RETURN_IF(this->m_traffic_test_selected == 0);

	--this->m_traffic_test_selected;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}
void test_udp_echo_client::set_traffic_test_message_size(std::size_t _index)
{
	RETURN_IF(this->m_traffic_test_selected >= 10);
	RETURN_IF(this->m_traffic_test_selected == _index);

	this->m_traffic_test_selected = _index;

	this->process_ready_send_test_message(this->m_traffic_test_selected);
}

bool test_udp_echo_client::request_bind(socket_udp* _psocket_udp, net::sockaddr _addr_bind)
{
	// hold)
	object_ptr<test_udp_echo_client> pthis = this;

	// 1) bind
	scoped_lock(this->m_cs_set_sockets)
	{
		// - insert
		auto iter_insert = this->m_set_sockets.insert(_psocket_udp);

		// check) 
		if (iter_insert.second == false)
			return false;
	}

	// 2) bind
	auto result = _psocket_udp->bind(_addr_bind);

	// return) 
	return result;
}

void test_udp_echo_client::request_closesocket(socket_udp* _psocket_udp)
{
	scoped_lock(this->m_cs_set_sockets)
	{
		// - erase
		this->m_set_sockets.erase(_psocket_udp);
	}

	_psocket_udp->closesocket();
}

void test_udp_echo_client::request_closesocket_all()
{
	// hold)
	object_ptr<test_udp_echo_client> pthis = this;

	// post!
	system_executor::post(make_executable([=]()
		{
			std::set<object_ptr<socket_udp>> set_sockets;

			scoped_lock(this->m_cs_set_sockets)
			{
				set_sockets = std::move(this->m_set_sockets);
			}

			for (auto& iter : set_sockets)
			{
				iter->closesocket();
			}
		}));
}

void test_udp_echo_client::request_send()
{
	// 1) ...
	auto value = ++this->m_traffic_send_requested;

	// check) iPre가 1이면 Interlocked걸기.
	if (value == 1)
	{
		object_ptr<test_udp_echo_client> self(this);

		system_executor::post(make_executable([=]()
		{
			self->process_execute_send();
		}));
	}
}

void test_udp_echo_client::request_send_immidiately(int64_t _count)
{
	// 1) send
	this->process_send_message(m_traffic_test_selected, _count);
}

void test_udp_echo_client::on_starting(context& /*_context*/)
{
	// 1) send test
	this->m_traffic_send_requested = 0;
	this->m_enable_traffic_test = false;
	this->m_traffic_test_selected = 0;
	this->m_traffic_test_count_per_single = 200;

	// 2) ready messges for send test
	this->process_ready_send_test_message(m_traffic_test_selected);

	// 3) disable test
	this->enable_traffic_test(false);

	// 4) start schedulable
	system_executor::register_schedulable(this);
}

struct TEST_MESSAGE_TYPE
{
	std::size_t size;
	int64_t count;
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

CGDK::shared_buffer generate_message(size_t _message_size)
{
	// check)
	CGASSERT_ERROR(_message_size > 0);

	auto buffer_temp = alloc_shared_buffer(_message_size);
	buffer_temp.append<uint32_t>(static_cast<uint32_t>(_message_size));
	for (std::size_t k = 0; k < (_message_size - 8); k += sizeof(uint32_t))
		buffer_temp.append<uint32_t>(rand());
	buffer_temp.front<uint32_t>() = static_cast<uint32_t>(buffer_temp.size() + SIZE_OF_CRC);
	buffer_temp.append(crc());

	return buffer_temp;
}

void test_udp_echo_client::process_ready_send_test_message(std::size_t _index)
{
	// check)
	CGASSERT_ERROR(_index < TEST_MESSAGE_TYPE_COUNT);

	// check)
	THROW_IF(_index >= TEST_MESSAGE_TYPE_COUNT, std::exception());

	// check)
	RETURN_IF(this->m_traffic_test_buf_message[_index].exist());

	// 1) get message_buffer
	this->m_traffic_test_buf_message[_index] = generate_message(g_test_message_info[_index].size);
}

void test_udp_echo_client::on_stopping()
{
	// 1) disable test
	this->m_enable_traffic_test = false;

	// 2) unregister...
	system_executor::unregister_schedulable(this);
}

void test_udp_echo_client::process_execute_send()
{
	// check)
	CGASSERT_ERROR(this->m_traffic_test_count_per_single != 0);

	// 1) send
	process_send_message(this->m_traffic_test_selected, this->m_traffic_test_count_per_single);

	// check)
	RETURN_IF(this->m_enable_traffic_test == false);

	// 2) ...
	auto value = --this->m_traffic_send_requested;

	// check) 0이 아닐 경우 또 걸기...
	if (value != 0)
	{
		object_ptr<test_udp_echo_client> pthis(this);

		system_executor::post(make_executable([=]() 
		{
			pthis->process_execute_send();
		}));
	}
}

intptr_t test_udp_echo_client::process_execute(intptr_t /*_return*/, std::size_t /*_param*/)
{
	// 1) traffice test
	this->process_traffic_test();

	// return) 
	return 0;
}

void test_udp_echo_client::process_traffic_test()
{
	// check)
	RETURN_IF(this->m_enable_traffic_test == false);

	// 1) send 
	this->request_send();
}

void test_udp_echo_client::process_send_message(std::size_t _index, int64_t _count)
{
	try
	{
		// check) 
		if (_index >= 11)
			return;

		// check) 
		if (_count <= 0)
			return;

		//const auto message_info = g_test_message_info[_index];
		auto buf_message = this->m_traffic_test_buf_message[_index];

		std::vector<object_ptr< socket_udp>> vector_socket;

		scoped_lock(this->m_cs_set_sockets)
		{
			vector_socket.reserve(this->m_set_sockets.size());

			for (auto& iter : this->m_set_sockets)
				vector_socket.push_back(iter);
		}

		for (auto& iter: vector_socket)
		{
			for(int i = 0; i < _count; ++i)
				for(auto& iter_dest : iter->m_vector_socket_destination)
					iter->send_to(buf_message, iter_dest);
		}
	}
	catch (...)
	{
	}
}

void test_udp_echo_client::process_send_message(shared_buffer _buffer)
{
	try
	{
		scoped_lock(this->m_cs_set_sockets)
		{
			for (auto& iter : this->m_set_sockets)
				for (auto& iter_dest : iter->m_vector_socket_destination)
					iter->send_to(_buffer, iter_dest);
		}
	}
	catch (...)
	{
	}
}