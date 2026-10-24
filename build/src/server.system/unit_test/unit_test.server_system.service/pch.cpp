//
// pch.cpp
//

#include "pch.h"

//-----------------------------------------------------------------------------
// main
//  - gtest_main is not used : vcpkg's gtest_main (dll) does not see the tests registered in this executable
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
