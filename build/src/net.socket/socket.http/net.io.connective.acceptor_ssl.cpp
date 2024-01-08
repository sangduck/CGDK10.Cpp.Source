#include "pch.h"

using namespace CGDK;


result_code net::io::connective::acceptor::Ntcp_secure::start(context& _context)
{
	// declare)
	result_code result;

	// declare) 
	net::io::connective::acceptor::Ntcp::START_PARAMETER parameter;

	try
	{
		// - ssl_context
		if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
		{
			// - get parameter
			if (_context.v.value_buffer.size() == sizeof(START_PARAMETER))
			{
				// - get parameters
				auto parameter_nested = _context.v.value_buffer.front<START_PARAMETER>();

				// - set ssl context
				m_pssl_context = parameter_nested.pssl_context;

				// trace)
				LOG_INFO << "  + parameter['ssl_context']: 'enabled'"sv;
			}
		}

		// - start
		result = net::io::connective::acceptor::Ntcp::start(_context);
	}
	catch (...)
	{
		// - rollback
		m_pssl_context.reset();

		// reraise)
		throw;
	}

	// return)
	return result;
}


void net::io::connective::acceptor::Ntcp_secure::process_on_accept( Iconnectable* _pconnectable )
{
	if(m_pssl_context.exist())
	{
		// 1) 설정한다.
		auto pcryptable = dynamic_cast<Icryptable*>(_pconnectable);

		// check) 
		THROW_IF(pcryptable == nullptr, throwable::failure(eRESULT::FAIL));

		// 2) 새로운 SSL객체를 생성한다.
		auto pcrypt = m_pssl_context->alloc_ssl(static_cast<int>(_pconnectable->native_handle()));

		// 3) 초기화를 한다.
		pcrypt->initialize_handshake_server(pcryptable);
	}

	// 4) on_accept를 호출한다.
	on_accept(_pconnectable); 
}

void net::io::connective::acceptor::Ntcp_secure::process_set_parameters(context& _context)
{
	// 1) "Certificate file" file
	std::string str_certificate_file = (std::string)_context.find("certificate_file", "certificate file");

	// 2) "privatekey file" file
	std::string str_private_key_file = (std::string)_context.find("privatekey_file", "privatekey file");

	// 3) nullptr이 아니면 생성...
	if (str_certificate_file.empty() == false && str_private_key_file.empty() == false)
	{
		m_pssl_context = make_object<crypt::ssl_context>(str_certificate_file, str_private_key_file);
	}
}

result_code net::io::connective::acceptor::Ntcp_secure::_process_starting(context& _context)
{
	// 1) call ...
	net::io::connective::acceptor::Ntcp::_process_starting(_context);

	// 2) get ssl context
	auto pssl_context = m_pssl_context;

	// log)
	if (pssl_context.exist())
	{
		LOG_INFO << "  - 'https' protocol ready"sv;
		LOG_INFO << "     + certification file: "sv << pssl_context->get_filename_cert();
		LOG_INFO << "     + private key file file: "sv << pssl_context->get_filename_private_key();
	}
	else
	{
		LOG_INFO << "  - 'http' protocol ready"sv;
	}

	// return)
	return eRESULT::SUCCESS;
}
