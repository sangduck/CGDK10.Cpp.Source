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
// net::acceptor<T> for normal socket
//
//-----------------------------------------------------------------------------
template <typename TSOCKET>
class net::acceptor<TSOCKET, typename std::enable_if<!std::is_base_of<net::io::Nhttp_server, TSOCKET>::value>::type> :
	public						net::io::connective::acceptor::Ntcp,
	public						Nmessage_transmitter,
	public						net::io::connectable_manager::list<TSOCKET>
{
public:
			acceptor() 
			{
				std::string temp_name("acceptor<");
				temp_name += typeid(TSOCKET).name();
				temp_name += ">"sv;

				this->name(temp_name);
			}
	virtual	~acceptor() noexcept {}

protected:
	virtual	object_ptr<net::io::Iconnectable> process_alloc_connectable() override { return alloc_object<TSOCKET>(); }
	virtual result_code			_process_destroy() noexcept override { auto result = this->net::io::connective::acceptor::Ntcp::_process_destroy(); this->reset_message_transmitter(); return result; }
};


//-----------------------------------------------------------------------------
//
// net::acceptor_lambda<T> - 
//
//-----------------------------------------------------------------------------
template <>
class net::acceptor<net::io::socket::Ntcp> :
	public						net::io::connective::acceptor::Ntcp,
	public						Nmessage_transmitter,
	public						net::io::connectable_manager::list<net::io::socket::Ntcp>
{
public:
	using function_alloc_socket = std::function<object_ptr<net::io::socket::Ntcp>(void)>;

	acceptor(function_alloc_socket&& _function) : m_function_alloc_socket(_function){}
	virtual	~acceptor() noexcept {}

protected:
	virtual	object_ptr<net::io::Iconnectable> process_alloc_connectable() override { return this->m_function_alloc_socket();}
			function_alloc_socket m_function_alloc_socket;

};

}