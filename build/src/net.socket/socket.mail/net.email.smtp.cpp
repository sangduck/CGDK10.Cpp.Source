#include "pch.h"

using namespace CGDK;

shared_buffer CGDK::net::email::smtp::m_s_buf_DATA;
shared_buffer CGDK::net::email::smtp::m_s_buf_QUIT;
shared_buffer CGDK::net::email::smtp::m_s_buf_email_DATA_tail;

net::email::smtp::smtp() :
	m_max_concurrent_socket(8)
{
	// 1) create static message
	auto buf_DATA = alloc_shared_buffer(1024);
	buf_DATA.append_text<char>("DATA\r\n");
	m_s_buf_DATA = std::move(buf_DATA);

	auto buf_QUIT = alloc_shared_buffer(1024);
	buf_QUIT.append_text<char>("QUIT\r\n");
	m_s_buf_QUIT = std::move(buf_QUIT);

	// declare)
	auto buf_email_DATA_tail = alloc_shared_buffer(32);
	buf_email_DATA_tail.append_text<char>(".\r\n");
	m_s_buf_email_DATA_tail = std::move(buf_email_DATA_tail);
}

void net::email::smtp::send(const email::sREQUEST& _email_info)
{
	send(email::sREQUEST(_email_info));
}

void net::email::smtp::send(email::sREQUEST&& _email_send_request)
{
	// check) check receiver
	CGASSERT_ERROR(_email_send_request.to.empty() == false);

	// check) 
	THROW_IF(_email_send_request.to.empty(), throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) 'email receiver' is empty"sv));

	// declare) 
	object_ptr<socket> psocket;

	// 1) get socket
	scoped_lock(m_cs_email_request)
	{
		if (m_list_socket_queue.empty() == false)
		{
			psocket = m_list_socket_queue.front();
		}
		else
		{
			// - push email_info
			if (m_list_socket_exist.size() > m_max_concurrent_socket)
			{
				// - queueing 
				m_list_send_email.push_back(_email_send_request);

				// return) 
				return;
			}

			// - create new
			psocket = make_object<socket>();

			// - set...s
			psocket->m_psmtp = this;

			// - queueing
			m_list_socket_exist.push_back(psocket);
		}
	}

	// 2) call on_email~ function
	on_email_sending(_email_send_request);

	// 3) request connect
	if (psocket->is_disconnected())
	{
		psocket->start(m_connect_parameter);
	}

	// 4) request send
	psocket->request_send(std::move(_email_send_request));
}

result_code net::email::smtp::_process_start(context& _context)
{
	const size_t DEFAULT_MAX_CONCURRENT_SOCKET = 8;

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start 'email SMTP sender'['"sv << name() << "']"sv;

	// 1) get address
	auto temp_connect_param = CGDK::get_parameter_connection(_context, true);

	// check)
	if (temp_connect_param.remote_endpoint.data().is_bind_any())
	{
		// log)
		LOG_ERROR << "  (excp) email 'remote_endpoint' is empty."sv;

		// raise) 
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) email 'remote_endpoint' is empty."sv);
	}

	// 2) it set port or remote_endpoint to smpt default port(=25) if port of remote_endpoint is 0
	if (temp_connect_param.remote_endpoint.port() == 0)
	{
		temp_connect_param.remote_endpoint.port(25);

		// log)
		LOG_INFO << "  - it set port of remote_endpoinrt as default value (="sv << temp_connect_param.remote_endpoint.port() << ")"sv;
	}

	if (temp_connect_param.bind_endpoint.port() != 0)
	{
		// - set as 0
		temp_connect_param.bind_endpoint.port(0);

		// log)
		LOG_INFO << "  ! bind_endpoint.port is ignored.";
	}

	// 2) get 'max_concurrent_socket'
	size_t max_concurrent_socket = DEFAULT_MAX_CONCURRENT_SOCKET;
	{
		// - get 'max_concurrent_socket'
		if (auto& context_find = _context.find("max_concurrent_socket"sv, "max concurrent socket"sv); context_find.is_valid())
		{
			// - get value
			max_concurrent_socket = context_find;

			// - saturate value
			if (max_concurrent_socket == 0)
			{
				// - sturate min 1
				max_concurrent_socket = DEFAULT_MAX_CONCURRENT_SOCKET;

				// log)
				LOG_INFO << "  + parameter['max_concurrent_socket']: default"sv;

				// log)
				LOG_INFO << "  - 'max_concurrent_socket' set as default value. (="sv << max_concurrent_socket << ")"sv;
			}
			else if (max_concurrent_socket > 8192)
			{
				// - sturate max 8192
				max_concurrent_socket = 8192;

				// log)
				LOG_INFO << "  - 'max_concurrent_socket' is too big. saturate is saturate (="sv << max_concurrent_socket << ")"sv;
			}
			else
			{
				// log)
				LOG_INFO << "  + parameter['max_concurrent_socket']: "sv << max_concurrent_socket;
			}
		}
	}

	// 3) email dmoain
	std::string email_server_domain;
	shared_buffer buf_EHLO;
	{
		// - get 'max_concurrent_socket'
		if (auto& context_find = _context.find("email_server_domain"sv, "email server domain"sv, "email_domain"sv, "email domain"sv); context_find.is_valid())
		{
			// - get email server domain
			email_server_domain = static_cast<std::string>(context_find);

			LOG_INFO << "  + parameter['email_server_domain']: '"sv << email_server_domain << "'"sv;

			// - create buffers
			buf_EHLO = alloc_shared_buffer(16 + email_server_domain.size() * sizeof(char));
			buf_EHLO.append_text<char>("EHLO ");
			buf_EHLO.append_text<char>(email_server_domain);
			buf_EHLO.append_text<char>("\r\n");
		}
		else
		{
			// log)
			LOG_ERROR << "  (excp) email 'remote_endpoint' is empty."sv;

			// raise) 
			throw throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) 'email_server_domain' is empty."sv);
		}
	}

	// 4) set...
	m_connect_parameter = std::move(temp_connect_param);
	m_email_server_domain = std::move(email_server_domain);
	m_buf_EHLO = std::move(buf_EHLO);
	m_max_concurrent_socket = max_concurrent_socket;

	// 5) call 'on_start'
	this->on_start(_context);

	// trace)
	LOG_PROGRESS << "@ <END> start 'email SMTP sender'['"sv << name() << "']"sv;

	// return) 
	return eRESULT::SUCCESS;
}

result_code net::email::smtp::_process_stopping() noexcept
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop 'email SMTP sender'['"sv << name() << "']"sv;

	// 1) call 'on_stopping'
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// 2) close socket all
	auto list_socket_exist = m_list_socket_exist;
	for (auto& iter : list_socket_exist)
	{
		iter->closesocket();
	}

	// return) 
	return eRESULT::SUCCESS;
}

result_code net::email::smtp::_process_stop() noexcept
{
	// 1) call 'on_stop'
	try
	{
		this->on_stop();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// 2) clear socket
	scoped_lock(m_cs_email_request)
	{
		m_list_socket_queue.clear();
		m_list_socket_queue.clear();
	}

	// trace)
	LOG_PROGRESS << "@ <END> stop 'email SMTP sender'['"sv << name() << "']"sv;

	// return) 
	return eRESULT::SUCCESS;
}

void net::email::smtp::process_complete_send(socket* _psocket)
{
	// 1) call 'on_email_sended'
	on_email_sended(_psocket->m_email_smtp_result);

	// declare) 
	email::sREQUEST email_send_request;

	scoped_lock(m_cs_email_request)
	{
		// check) push socket if email is empty
		if (m_list_send_email.empty())
		{
			// - queueing 
			m_list_socket_queue.emplace_back(_psocket);

			// - clear
			_psocket->m_email_smtp_result.clear();

			// return) 
			return;
		}

		// 2) pop queued email info
		email_send_request = std::move(m_list_send_email.front());
		m_list_send_email.pop_front();
	}

	// 3) request send email
	_psocket->request_send(std::move(email_send_request));
}

void net::email::smtp::process_socket_closed(socket* _psocket)
{
	scoped_lock(m_cs_email_request)
	{
		m_list_socket_queue.remove(_psocket);
		m_list_socket_exist.remove(_psocket);
	}
}