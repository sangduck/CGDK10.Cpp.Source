// dllmain.cpp : DLL의 초기화 루틴을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include <afxwin.h>
#include <afxdllx.h>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE CGDKserverwebcontrolsMFCvs16DLL = { false, nullptr };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		if (!AfxInitExtensionModule(CGDKserverwebcontrolsMFCvs16DLL, hInstance))
			return 0;

		AfxInitRichEdit2();

		new CDynLinkLibrary(CGDKserverwebcontrolsMFCvs16DLL);

		if (!AfxSocketInit())
		{
			return FALSE;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(CGDKserverwebcontrolsMFCvs16DLL);
	}
	return 1;
}
