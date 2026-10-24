#pragma once

class manager_user : 
	virtual public				CGDK::Ireferenceable
{
public:
			void				register_user (socket_user* _user);
			void				unregister_user	(socket_user* _user) noexcept;

protected:
			CGDK::lock_object	m_mutex_map_user;
			std::map<uint64_t, CGDK::object_ptr<socket_user>> m_map_channel;
};
