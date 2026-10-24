#include "pch.h"

namespace CGDK
{

shared_buffer server::web::Ientity::m_buffer_context_tail;

void server::web::Ientity::init_context()
{
	// check)
	RETURN_IF(m_buffer_context_tail.empty());

	// 1) context tail
	m_buffer_context_tail = alloc_shared_buffer(8);
	m_buffer_context_tail.append_text("\r\n\r\n");
}

}