// dllmain.cpp : DLL의 초기화 루틴을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include <afxwin.h>
#include <afxdllx.h>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE CGDKserversystemcontrolsMFCvs16DLL = { false, nullptr };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		if (!AfxInitExtensionModule(CGDKserversystemcontrolsMFCvs16DLL, hInstance))
			return 0;

		AfxInitRichEdit2();

		new CDynLinkLibrary(CGDKserversystemcontrolsMFCvs16DLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(CGDKserversystemcontrolsMFCvs16DLL);
	}
	return 1;
}
