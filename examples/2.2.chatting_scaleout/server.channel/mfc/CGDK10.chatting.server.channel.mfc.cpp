#include "pch.h"
#include "framework.h"
#include "CGDK10.chatting.server.channel.mfc.h"
#include "CGDK10.chatting.server.channel.dlg.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(channel_server_mfc_app, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

channel_server_mfc_app::channel_server_mfc_app()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

channel_server_mfc_app theApp;

BOOL channel_server_mfc_app::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	CShellManager *pShellManager = new CShellManager;

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("로컬 애플리케이션 마법사에서 생성된 애플리케이션"));

	channel_server_dlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	return FALSE;
}

