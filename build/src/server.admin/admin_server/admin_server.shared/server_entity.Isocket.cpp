#include "pch.h"


server_entity::Isocket::Isocket() noexcept
{
	set_maximum_message_buffer_size(512 * 1024 * 1024);
	set_maximum_depth_of_send_buffer(65536);
	set_maximum_depth_of_send_buffer_bytes(512 * 1024 * 1024);
}

server_entity::Isocket::~Isocket() noexcept
{
}
