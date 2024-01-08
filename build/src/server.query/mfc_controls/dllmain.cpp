#include "pch.h"
#include <afxdllx.h>

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE CGServerQueryMFCControlsDLL = { NULL, NULL };

namespace CGDK
{
	extern bool g_is_initialize;
}

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	CGDK::g_is_initialize = false;

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		if (!AfxInitExtensionModule(CGServerQueryMFCControlsDLL, hInstance))
			return 0;

		AfxInitRichEdit2();

		new CDynLinkLibrary(CGServerQueryMFCControlsDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(CGServerQueryMFCControlsDLL);
	}
	return 1;
}


