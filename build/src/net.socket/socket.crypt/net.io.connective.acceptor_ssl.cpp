#include "pch.h"

using namespace CGDK;


result_code net::io::connective::acceptor::Ntcp_ssl::start(context& _context)
{
	// declare)
	result_code result;

	// declare) 
	net::io::connective::acceptor::Ntcp::START_PARAMETER parameter;

	try
	{
		// - ssl_context
		if (_context.type() == context::value_type_t::buffer_)
		{
			// - get parameter
			if (_context.v.value_buffer.size() == sizeof(START_PARAMETER))
			{
				// - get parameters
				auto parameter_nested = _context.v.value_buffer.front<START_PARAMETER>();

				// - set ssl context
				this->m_pssl_context = parameter_nested.pssl_context;

				// trace)
				LOG_INFO << "  + parameter['ssl_context']: 'enabled'"sv;
			}
		}

		// - start
		result = this->net::io::connective::acceptor::Ntcp::start(_context);
	}
	catch (...)
	{
		// - rollback
		this->m_pssl_context.reset();

		// reraise)
		throw;
	}

	// return)
	return result;
}


void net::io::connective::acceptor::Ntcp_ssl::process_on_accept( Iconnectable* _pconnectable )
{
	if(this->m_pssl_context.exist())
	{
		// 1) 설정한다.
		auto pcryptable = dynamic_cast<Issl_cryptable*>(_pconnectable);

		// check) 
		if(pcryptable == nullptr)
			throw throwable::failure(eRESULT::FAIL);

		// 2) 새로운 SSL객체를 생성한다.
		auto pcrypt = this->m_pssl_context->alloc_ssl(static_cast<int>(_pconnectable->native_handle()));

		// 3) 초기화를 한다.
		pcrypt->initialize_handshake_server(pcryptable);

		// 4) handshake 제한 시간을 건다. (ClientHello를 보내지 않는 접속을 끊는다.)
		if (auto psocket_ssl = dynamic_cast<net::io::Nsocket_ssl*>(_pconnectable); psocket_ssl != nullptr)
			psocket_ssl->start_ssl_handshake_timer(this->m_ssl_handshake_timeout);
	}

	// 5) on_accept를 호출한다.
	this->on_accept(_pconnectable);
}

void net::io::connective::acceptor::Ntcp_ssl::process_set_parameters(context& _context)
{
	// 0) "ssl_handshake_timeout" (초)
	if (auto& context_timeout = _context.find("ssl_handshake_timeout"sv); context_timeout.is_valid())
	{
		int64_t seconds = context_timeout;
		this->m_ssl_handshake_timeout = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::seconds(__select_max<int64_t>(seconds, 0)));
	}

	// 설명) _context.find(a, b)는 a 또는 b를 찾는 것이 아니라 [a][b] (중첩)를 찾는다.
	//       따라서 두 이름("certificate_file"/"certificate file")은 각각 찾는다.
	auto find_either = [&_context](std::string_view _key_a, std::string_view _key_b) -> context&
	{
		auto& context_a = _context.find(_key_a);
		return context_a.is_valid() ? context_a : _context.find(_key_b);
	};

	// 1) "certificate_file" file
	auto& context_certificate_file = find_either("certificate_file"sv, "certificate file"sv);

	// 2) "privatekey_file" file
	auto& context_private_key_file = find_either("privatekey_file"sv, "privatekey file"sv);

	// check) 둘 다 없으면 ssl을 사용하지 않는다. (http)
	if (context_certificate_file.is_invalid() && context_private_key_file.is_invalid())
		return;

	// check) 하나만 있으면 설정 오류
	if (context_certificate_file.is_invalid() || context_private_key_file.is_invalid())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("both 'certificate_file' and 'privatekey_file' must be set"));

	// 3) certificate_file와 privatekey_file이 모두 존재하면 ssl_context를 만든다.
	{
		// - get keys
		std::string str_certificate_file = context_certificate_file;
		std::string str_private_key_file = context_private_key_file;

		// - make ssl_context
		this->m_pssl_context = make_object<crypt::ssl_context>(str_certificate_file, str_private_key_file);
	}
}

result_code net::io::connective::acceptor::Ntcp_ssl::_process_starting(context& _context)
{
	// 1) call ...
	net::io::connective::acceptor::Ntcp::_process_starting(_context);

	// 2) get ssl context
	auto pssl_context = this->m_pssl_context;

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
