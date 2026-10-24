#include "pch.h"


namespace CGDK
{


void server_system::sRANK::clear() noexcept
{
	this->group = RANK_NONE;
	this->field = RANK_NONE;
	this->role = RANK_NONE;
	this->index = RANK_NONE;
}

std::string to_string(server_system::iid_t _iid)
{
	if(_iid == server_system::IID_AUTO)
		return "auto";
	else if(_iid == server_system::IID_NONE)
		return "none";
	else
		return std::to_string(_iid);
}

std::string server_system::sRANK::to_string() const
{
	std::string str_group = (this->group != RANK_NONE) ? std::to_string(this->group) : "-";
	std::string str_field = (this->field != RANK_NONE) ? std::to_string(this->field) : "-";
	std::string str_role = (this->role != RANK_NONE) ? std::to_string(this->role) : "_";
	std::string str_index = (this->index != RANK_NONE) ? std::to_string(this->index) : "-";

	return str_group + "." + str_field + "." + str_role + "." + str_index;
}


}