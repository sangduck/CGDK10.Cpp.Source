//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Buffer template build                            *
//*                                                                           *
//*****************************************************************************

#define CGDK_BUFFER_BUILD_EXPLICIT_INSTANTIATION
#include "pch.h"

namespace CGDK
{
	template class _buffer_view<char>;
	template class _basic_buffer<char>;
	template class _shared_buffer<buffer>;
}
