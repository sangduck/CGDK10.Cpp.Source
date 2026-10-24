//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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

class server_system::host::Ninstance : 
	virtual public				Imessageable,
	virtual public				net::io::Isender,
	public						Ibuffer_serializable
{
public:
			sHOST				info;
			sINSTANCE_KEY		instance_key;

public:
			auto				get_iid() const noexcept { return this->info.setting.iid; }
			auto 				get_sender() const noexcept { return this->m_psender; }
			auto 				set_sender(net::io::Isender* _psender) noexcept { this->m_psender = _psender; }
			void				reset_sender() noexcept { this->m_psender.reset(); }

			// - host system status (HOST_UPDATE_STATUS. the last one only)
			void				update_system_status(const SYSTEM::sSTATUS& _status)
			{
				SCOPED_LOCK(this->m_lockable_instance)
				{
					this->info.status.vector_status.assign(1, _status);
				}
			}
			[[nodiscard]] std::optional<SYSTEM::sSTATUS> get_system_status() const
			{
				SCOPED_LOCK(this->m_lockable_instance)
				{
					if (this->info.status.vector_status.empty())
						return std::nullopt;
					return this->info.status.vector_status.back();
				}
				return std::nullopt;
			}
			// - host_agent web ui status (HOST_UPDATE_STATUS. 이전 host_agent는 보내지 않는다.)
			void				update_web_status(const sHOST_WEB_STATUS& _status)
			{
				SCOPED_LOCK(this->m_lockable_instance)
				{
					this->m_web_status = _status;
				}
			}
			[[nodiscard]] std::optional<sHOST_WEB_STATUS> get_web_status() const
			{
				SCOPED_LOCK(this->m_lockable_instance)
				{
					return this->m_web_status;
				}
				return std::nullopt;
			}
			[[nodiscard]] std::string get_name() const
			{
				SCOPED_LOCK(this->m_lockable_instance)
				{
					return this->info.setting.name;
				}
				return std::string();
			}

public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option) override;

protected:
	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;

	mutable lock_object			m_lockable_instance;
			object_ptr<net::io::Isender> m_psender;
			std::optional<sHOST_WEB_STATUS> m_web_status;
};

} 
