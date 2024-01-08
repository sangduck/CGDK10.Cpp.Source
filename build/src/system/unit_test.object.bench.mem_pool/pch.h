#pragma once

#include "gtest/gtest.h"

//#define _TC_MALLOC

#if defined(_TC_MALLOC)
	#include <gperftools/tcmalloc.h>
#endif

// configure) 
#define _CGDK_NO_ASSERT_ON_BOUND_CHECK

#include "cgdk/sdk10/system.object.h"
