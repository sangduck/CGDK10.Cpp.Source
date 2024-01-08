#include "pch.h"

using namespace CGDK;


CGDK::server_admin::Isocket::Isocket()
{
	this->maximum_depth_of_send_buffer(1024);
}

CGDK::server_admin::Isocket::~Isocket() noexcept
{
}
