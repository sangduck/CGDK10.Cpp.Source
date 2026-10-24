#include "pch.h"

//-----------------------------------------------------------------------------
// main
//  - gtest_main is not used : vcpkg's gtest_main (dll) does not see the tests registered in this executable
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	auto result = RUN_ALL_TESTS();

	// - destroy the time_event::entity_manager explicitly
	//   (known issue) if it is destroyed as a static object, it can be destroyed after the system_executor/executor::manager
	//   (both are 'init_seg(lib)' and the order depends on the link order) and crashes on exit (Debug)
	CGDK::time_event::entity_manager::reset_instance();
	return result;
}
