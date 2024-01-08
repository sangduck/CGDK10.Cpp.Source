// dllmain.cpp : DLL의 초기화 루틴을 정의합니다.
//

#include "pch.h"
#include <afxwin.h>
#include <afxdllx.h>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE CGDKnetworksocketcontrolsMFCvs16DLL = { false, nullptr };

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
		if (!AfxInitExtensionModule(CGDKnetworksocketcontrolsMFCvs16DLL, hInstance))
			return 0;

		AfxInitRichEdit2();

		new CDynLinkLibrary(CGDKnetworksocketcontrolsMFCvs16DLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(CGDKnetworksocketcontrolsMFCvs16DLL);
	}
	return 1;
}
