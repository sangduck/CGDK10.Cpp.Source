#include "pch.h"

namespace CGDK
{

shared_buffer server::web::Ientity::m_buffer_context_tail;

// - no tail is sent after the body : the responses have a Content-length and "\r\n\r\n" after the body would be read
//   as the head of the next response on a keep-alive connection. (the check was inverted : the tail was never built
//   and an empty buffer was sent. the sends are removed and 'm_buffer_context_tail' stays empty)
void server::web::Ientity::init_context()
{
}

}