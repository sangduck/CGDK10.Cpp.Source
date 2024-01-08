#include "pch.h"

int main(int _argc, char* _ppargv[])
{
	// 1) CGCII initilaization (thread count = 4)
	system_executor::initialize_instance(4);

	// 2) run
	return service::run(_argc, _ppargv);
}
