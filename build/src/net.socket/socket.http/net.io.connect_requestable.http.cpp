#include "pch.h"

using namespace CGDK;


bool net::io::connect_requestable::Nhttp::connect(const std::string_view _url)
{
	// 1) create http_request
	auto phttp_request = make_object<net::http_request>();

	// 2) set url
	phttp_request->set_request(_url);

	// 3) return
	return connect(phttp_request);
}

void net::io::connect_requestable::Nhttp::set_request(Ihttp_request* _prequest) noexcept
{
	// lock) 
	std::lock_guard cs(this->m_lock_prequest);

	// check)
	CGASSERT_ERROR(this->m_prequest.empty());

	// 1) set ssl context (m_flag == true -> set ssl_context, m_flag == false -> reset ssl_context)
	prepare_ssl(_prequest->m_is_crypt, _prequest->m_host_name);

	// 1) HTTPRequest객체를 Set 한다.
	this->m_prequest = _prequest;
}
void net::io::connect_requestable::Nhttp::reset_request() noexcept
{
	// lock) 
	std::lock_guard cs(this->m_lock_prequest);

	// check)
	if(this->m_prequest.empty())
		return;

	// 1) reset Ihttp_request object
	this->m_prequest.reset();

	// 2) reset ssl context 
	this->m_pssl_context.reset();
}
object_ptr<net::Ihttp_request> net::io::connect_requestable::Nhttp::move_request() noexcept
{
	// lock) 
	std::lock_guard cs(this->m_lock_prequest);

	return std::move(this->m_prequest);
}

bool net::io::connect_requestable::Nhttp::connect(Ihttp_request* _prequest)
{
	// check) _prequest must not be nullptr
	CGASSERT_ERROR(_prequest != nullptr);

	// check) _prequest must not be nullptr
	if(_prequest == nullptr)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// check) check address validation.
	CGASSERT_ERROR(_prequest->m_address_host.address().is_unspecified() == false);

	// check) throw exception on invalid address
	if(_prequest->m_address_host.address().is_unspecified())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// declare) 
	bool result = false;

	try
	{
		// 1) HTTP Request 객체를 Set한다.
		set_request(_prequest);

		// 2) binder를 먼저 얻는다.
		auto pbinder = dynamic_cast<net::io::Ibinder*>(this);
		
		// 3) bind socket
		if (pbinder != nullptr)
			pbinder->bind();
		
		// 4) connect한다.
		result = process_request_connecting(_prequest->m_address_host);

		// check) throw exception on failed
		if(result == false)
			throw throwable::failure(eRESULT::FAIL_TO_CONNECT);
	}
	catch(...)
	{
		// - reset Ihttp_reques object
		reset_request();

		// - socket을 닫는다.
		closesocket();
	}

	// return)
	return result;
}
