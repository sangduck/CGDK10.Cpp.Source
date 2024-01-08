//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Login Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once
#include "afxcmn.h"

// CLoginServerDlg 대화 상자
class CLoginServerDlg : 
	public						CDialogEx,
	public						CGNet::IO::Messageable::NBase
{
public:
			CLoginServerDlg(CWnd* pParent = nullptr);

protected:
	virtual void				DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL				OnInitDialog();

public:
	// Declare) 
	DECLARE_MESSAGE_MAP()

	// 1)  System Message
	afx_msg void				OnPaint();
	afx_msg HCURSOR				OnQueryDragIcon();
	afx_msg void				OnTimer(UINT_PTR nIDEvent);

	// 2) Control events
	afx_msg void				OnBnClickedButtonPoolStatus();
	afx_msg void				OnBnClickedButtonThreadInfo();
	afx_msg void				OnBnClickedOk();
	afx_msg void				OnBnClickedCancel();

	// 3) Menu events
	afx_msg void				OnSystemServiceStart();
	afx_msg void				OnSystemServiceCancel();
	afx_msg void				OnSystemServiceStop();
	afx_msg void				OnSystemExit();
	afx_msg void				OnSettingConfigures();
	afx_msg void				OnSettingEnginetunning();
	afx_msg void				OnAboutSystemInfo();
	afx_msg void				OnAboutApplicationInfo();

	// 4) CGNetMessage
	afx_msg LRESULT				OnNotifyMessage_Update(WPARAM _wParam, LPARAM _lParam);

private:
	// 1) Message를 받았을 때 불려지는 함수.
	virtual int					OnMessage(CGMSG& _msg) override;

	// 2) Message
			void				OnServiceInitializingPending();
			void				OnServiceDestroyingPending();
			void				OnServiceRunning();
			void				OnServiceStartPending();
			void				OnServiceStopPending();
			void				OnServiceStopped();
			void				OnServiceUpdate();

	// 3) 
			void				OnLobbyServerConnect(ServerLobby::CSocket* _pSocket);
			void				OnLobbyServerDisconnect(ServerLobby::CSocket* _pSocket);
			void				OnLobbyServerUpdate(ServerLobby::CSocket* _pSocket);


protected:
	// 1) MFC Resource & Controls
			HICON				m_hIcon;

	// 2) User
			CCGChildAcceptorControl		m_childAcceptorUser;
			CCGChildConnectiveInfo		m_childConnectiveUser;

	// 3) Server Login
			CCGChildAcceptorControl		m_childAcceptorServerLobby;
			lockable<CListCtrl>			m_listServerLobby;

	// 4) Log view
			CCGChildLogView				m_childLogView;

	// 5) Pool & Thread status
			CCGDialogObjectPoolStatus	m_dlgPoolStatus;
			CCGDialogThreadStatus		m_dlgThreadStatus;

	// 6) ...
			void				RequestServiceInitialize();
			void				RequestServiceStart();
			void				RequestServiceCancel();
			void				RequestServiceStop();
			void				RequestServiceDestroy();

			void				EnableServerControls(BOOL _bEnable);

			int					List_LobbyServerAdd		(ServerLobby::CSocket* _pSocket);
			void				List_LobbyServerRemove	(ServerLobby::CSocket* _pSocket);
			void				List_LobbyServerUpdate	(ServerLobby::CSocket* _pSocket);
			void				List_LobbyServerUpdate	(int _Index);
			void				List_LobbyServerUpdateAll();

			void				UpdateServerInfo();
};
