//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
// ----------------------------------------------------------------------------
//
// instance::instance
//
// ----------------------------------------------------------------------------
class server_admin::Iinstance :
	public observable::list_sender<socket_websocket>,
	public net::io::messageable::Nbase,
	public Ibuffer_serializable,
	virtual public net::io::Isender
{
public:
	virtual ~Iinstance() noexcept;

			auto&				socket() 
			{
				scoped_lock(this->m_cs_instance)
				{
					return this->m_psocket;
				}
			}
			auto&&				move_out_socket()
			{
				scoped_lock(this->m_cs_instance)
				{
					return std::move(this->m_psocket);
				}
			}
			object_ptr<socket_peer> set_socket(object_ptr<socket_peer>&& _psocket) noexcept
			{
				scoped_lock(this->m_cs_instance)
				{
					object_ptr<socket_peer> psocket_old = std::move(this->m_psocket);
					this->m_psocket = std::move(_psocket);

					return psocket_old;
				}
			}
	virtual	iid_t				get_iid() const PURE;
	virtual std::string			get_name() const PURE;
	virtual int					get_instance_type() const PURE;
	virtual bool				process_sservice_disconnected(socket_peer* _psocket);
	virtual	eOBJECT_STATE		set_instance_state(eOBJECT_STATE) PURE;

public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option) override;

			lockable<>			m_cs_instance;
			object_ptr<socket_peer> m_psocket;
};
