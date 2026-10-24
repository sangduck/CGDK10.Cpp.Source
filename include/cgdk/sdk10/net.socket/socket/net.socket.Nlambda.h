//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
// net::socket::Nlambda
//
//    1) 기본적인 tcp 소켓으로 send/receive 등을 수행할 수 있다.
//
// ----------------------------------------------------------------------------
template <class TSELF>
class net::socket::Nlambda :
	virtual public				net::io::Iconnect_notify,
	virtual public				net::io::Imessageable
{
public:
	using function_on_request_connect_t	 = std::function<void(TSELF*, const net::sockaddr&)>;
	using function_on_connect_t			 = std::function<void(TSELF*)>;
	using function_on_fail_connect_t	 = std::function<void(TSELF*, uint64_t)>;
	using function_on_disconnect_t		 = std::function<void(TSELF*, uint64_t)>;
	using function_on_message_t			 = std::function<result_code(TSELF*, sMESSAGE_NETWORK&)>;

			void				set_on_request_connect(function_on_request_connect_t&& _function) noexcept { this->m_function_on_request_connect = std::move(_function);}
			void				set_on_connect(function_on_connect_t&& _function) noexcept { this->m_function_on_connect = std::move(_function); }
			void				set_on_fail_connect(function_on_fail_connect_t&& _function) noexcept { this->m_function_on_fail_connect = std::move(_function); }
			void				set_on_disconnect(function_on_disconnect_t&& _function) noexcept { this->m_function_on_disconnect = std::move(_function); }
			void				set_on_message(function_on_message_t&& _function) noexcept { this->m_function_on_message = std::move(_function); }

protected:
	virtual void				on_request_connect(const net::sockaddr& _address) override
	{
		if (this->m_function_on_request_connect)
			this->m_function_on_request_connect(dynamic_cast<TSELF*>(this), _address);
	}
	virtual void				on_connect() override
	{
		if (this->m_function_on_connect)
			this->m_function_on_connect(dynamic_cast<TSELF*>(this));
	}
	virtual void				on_disconnect(uint64_t _disconnect_reason) override
	{
		if (this->m_function_on_disconnect)
			this->m_function_on_disconnect(dynamic_cast<TSELF*>(this), _disconnect_reason);
	}
	virtual void				on_fail_connect(uint64_t _disconnect_reason) override
	{
		if (this->m_function_on_fail_connect)
			this->m_function_on_fail_connect(dynamic_cast<TSELF*>(this), _disconnect_reason);
	}
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		if (this->m_function_on_message)
			return this->m_function_on_message(dynamic_cast<TSELF*>(this), _msg);

		return eRESULT::BYPASS;
	}

			function_on_request_connect_t m_function_on_request_connect;
			function_on_connect_t		m_function_on_connect;
			function_on_fail_connect_t	m_function_on_fail_connect;
			function_on_disconnect_t	m_function_on_disconnect;
			function_on_message_t		m_function_on_message;

};


}