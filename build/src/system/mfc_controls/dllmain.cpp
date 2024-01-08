// dllmain.cpp : DLL의 초기화 루틴을 정의합니다.
//

#include "pch.h"
#include <afxwin.h>
#include <afxdllx.h>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE CGDKcontrolsMFCvs16DLL = { false, nullptr };

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
		if (!AfxInitExtensionModule(CGDKcontrolsMFCvs16DLL, hInstance))
			return 0;

		AfxInitRichEdit2();

		new CDynLinkLibrary(CGDKcontrolsMFCvs16DLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(CGDKcontrolsMFCvs16DLL);
	}

	// return) 
	return 1;
}
