#include "pch.h"


namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_acceptor_control, CChildDialog)

CChild_acceptor_control::CChild_acceptor_control(net::io::connective::Iacceptor* p_pAcceptor, CWnd* pParent)
	: CChildDialog(IDD_DIALOG_ACCEPTOR_CONTROL, pParent),
	m_pAcceptor(p_pAcceptor),
	m_statusPre(eSOCKET_STATE::MAX),
	m_bEnableControl(TRUE),
	m_port_default(10000)
{
}

CChild_acceptor_control::~CChild_acceptor_control()
{
}

void CChild_acceptor_control::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_BIND_ADDRESS, m_comboBindAddress);
	DDX_Control(pDX, IDC_CHECK_ACCEPTOR_READY, m_checkReady);
	DDX_Control(pDX, IDC_BUTTON_ACCEPTOR_STATUS, m_buttonStatus);
	DDX_Control(pDX, IDC_BUTTON_ACCEPTOR_CLOSESOCKET_ALL, m_buttonDisconnectAll);
	DDX_Control(pDX, IDC_EDIT_BIND_PORT, m_editBindPort);
}

BEGIN_MESSAGE_MAP(CChild_acceptor_control, CChildDialog)
	ON_BN_CLICKED(IDC_CHECK_ACCEPTOR_READY, &CChild_acceptor_control::OnBnClickedCheckAcceptorStartStop)
	ON_BN_CLICKED(IDC_BUTTON_ACCEPTOR_STATUS, &CChild_acceptor_control::OnBnClickedButtonAcceptorStatus)
	ON_BN_CLICKED(IDC_BUTTON_ACCEPTOR_CLOSESOCKET_ALL, &CChild_acceptor_control::OnBnClickedButtonAcceptorDisconnectAll)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_acceptor_control::OnCGMessageNotify)
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

object_ptr<net::io::connective::Iacceptor>	CChild_acceptor_control::GetAcceptor()
{
	scoped_lock(m_csAcceptor)
	{
		return m_pAcceptor;
	}
}


BOOL CChild_acceptor_control::CreateChild(net::io::connective::Iacceptor* p_pAcceptor, CWnd* _pwnd)
{
	// declare) 
	BOOL	result;

	// 1) Acceptor를 설정한다.
	SetAcceptor(p_pAcceptor);

	// 2) 윈도우를 생성한다.
	result	 = Create(IDD_DIALOG_ACCEPTOR_CONTROL, _pwnd);

	// check) 생성에 실패했으면 false를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 
	return	TRUE;
}

// CChild_acceptor_control message handlers
bool CChild_acceptor_control::ListenAcceptor()
{
	// 1) get acceptor
	object_ptr<net::io::connective::Iacceptor> pacceptor= GetAcceptor();

	// check)
	RETURN_IF(pacceptor.empty(), false);

	// declare) 
	net::io::connective::acceptor::Ntcp::START_PARAMETER parameter;

	// 2) 현재 선택된 Item을 얻는다.
	int	cur_sel = m_comboBindAddress.GetCurSel();

	// 3) 선택에 따라 Address를 설정한다.
	if(cur_sel == 0)
	{
		parameter.local_endpoint = net::sockaddr{ net::ip::address_v6::any() };
	}
	else
	{
		parameter.local_endpoint = api::socket::instance().get_local_address(cur_sel - 1);
	}

	// 4) Port를 얻는다.
	uint16_t port = (uint16_t)GetDlgItemInt(IDC_EDIT_BIND_PORT);

	// 5) Port를 설정한다.
	parameter.local_endpoint.port(port);

	// 6) start(=Listen)한다.
	auto result = pacceptor->start(parameter);

	// check) 제대로 start되었는지 확인한다.
	if(result != eRESULT::SUCCESS && result != eRESULT::PENDED)
	{
		AfxMessageBox(TEXT("해당 Port에 start하는데 실패했습니다."), MB_OK);
		return false;
	}

	// return)
	return true;
}

void CChild_acceptor_control::SetConnective(net::io::Iconnective* _pconnective)
{
	// 1) Acceptor인가 확인한다.(IAcceptor Interface를 얻는다.)
	auto pacceptor = dynamic_cast<net::io::connective::Iacceptor*>(_pconnective);

	// 2) Acceptor를 설정한다.
	SetAcceptor(pacceptor);
}

void CChild_acceptor_control::SetAcceptor(net::io::connective::Iacceptor* p_pAcceptor)
{
	scoped_lock(m_csAcceptor)
	{
		// check) 
		RETURN_IF(m_pAcceptor==p_pAcceptor);

		// 1) Acceptor를 설정한다.
		m_pAcceptor = p_pAcceptor;

		// 2) 상태를 바꾼다.
		m_statusPre = eSOCKET_STATE::MAX;
	}

	// 3) Acceptor의 상태를 Update한다.
	UpdateAcceptorStatus();
}

void CChild_acceptor_control::UpdateAcceptorStatus()
{
	// check) 윈도우가 생성되지 않았다면 여기서 끝낸다.
	RETURN_IF(m_hWnd == nullptr);


	//-----------------------------------------------------------------
	// 1. Acceptor를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::connective::Iacceptor>	pacceptor = GetAcceptor();

	// check)
	if(pacceptor.empty() || IsWindowEnabled()==FALSE)
	{
		m_comboBindAddress.EnableWindow(FALSE);
		m_editBindPort.EnableWindow(FALSE);
		m_checkReady.EnableWindow(FALSE);
		m_buttonStatus.EnableWindow(FALSE);
		m_buttonDisconnectAll.EnableWindow(FALSE);	

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. 내용을 Update한다.
	//-----------------------------------------------------------------
	// 1) ...
	m_checkReady.EnableWindow(m_bEnableControl);
	m_buttonStatus.EnableWindow(TRUE);
	m_buttonDisconnectAll.EnableWindow(TRUE);	

	// declare) Acceptor의 상태를 얻어낸다.
	eSOCKET_STATE statusSocket = pacceptor->socket_state();

	if(statusSocket>= eSOCKET_STATE::LISTEN)
	{
		// 2) enable한다.
		m_comboBindAddress.EnableWindow(FALSE);
		m_editBindPort.EnableWindow(FALSE);

		m_checkReady.SetCheck(BST_CHECKED);

		// declare) 
		auto address_socket = pacceptor->local_endpoint();

		// 3) Socket Address를 얻는다.
		if(address_socket.exist())
		{
			if(address_socket.is_unspecified())
			{
				m_comboBindAddress.SetCurSel(0);
			}
			else
			{
				for(int i=1; i<m_comboBindAddress.GetCount(); ++i)
				{
					net::sockaddr* addrLocal = reinterpret_cast<net::sockaddr*>(m_comboBindAddress.GetItemData(i));

					if(*addrLocal == address_socket)
					{
						m_comboBindAddress.SetCurSel(i);
						break;
					}
				}
			}

			SetDlgItemInt(IDC_EDIT_BIND_PORT, address_socket.port());
		}
		else
		{
			m_comboBindAddress.SetCurSel(0);
			SetDlgItemInt(IDC_EDIT_BIND_PORT, 10000);
		}

		// 4) 메시지를 전송한다.
		SendMessageToTarget((WPARAM)CGACCEPTOR_READY, (LPARAM)(net::io::Iconnective*)pacceptor.get());
	}
	else
	{
		// 1) enable한다.
		m_comboBindAddress.EnableWindow(TRUE);
		m_editBindPort.EnableWindow(TRUE);

		m_checkReady.SetCheck(BST_UNCHECKED);

		// 2) CurSel을 ADDR_ANY로...
		m_comboBindAddress.SetCurSel(0);

		// 3) 메시지를 전송한다.
		SendMessageToTarget((WPARAM)CGACCEPTOR_CLOSE, (LPARAM)pacceptor.get());
	}

	// 6) 상태를 Update한다. 
	m_statusPre	 = statusSocket;
}


void CChild_acceptor_control::SetDefaultPort(uint16_t _port)
{
	m_port_default = _port;

	SetDlgItemInt(IDC_EDIT_BIND_PORT, _port);
}

void CChild_acceptor_control::EnableControl(BOOL _flag_enable)
{
	m_bEnableControl = _flag_enable;

	RETURN_IF(GetSafeHwnd() == nullptr);

	m_checkReady.EnableWindow(m_bEnableControl && _flag_enable);
}

BOOL CChild_acceptor_control::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	// 1) Local IP Address setting
	m_comboBindAddress.AddString(TEXT("ADDR_ANY"));
	{
		auto psockAddr = new net::sockaddr{ net::ip::address_v4::loopback(), 0 };

		int index = m_comboBindAddress.AddString(TEXT("localhost"));
		m_comboBindAddress.SetItemData(index, reinterpret_cast<DWORD_PTR>(psockAddr));
	}

	for(int i=0; i<api::socket::instance().get_local_address_count(); ++i)
	{
		auto psockAddr = new net::sockaddr(api::socket::instance().get_local_address(i));

		TCHAR temp_string[256];
		psockAddr->to_string<TCHAR>(temp_string, 256);

		int index = m_comboBindAddress.AddString(temp_string);
		m_comboBindAddress.SetItemData(index, reinterpret_cast<DWORD_PTR>(psockAddr));
	}

	// 2) 현재 Sel을 0번으로 한다.
	m_comboBindAddress.SetCurSel(0);
	
	// check) 
	CGASSERT_ERROR(m_comboBindAddress.GetCurSel()==0);

	// 3) set default port를 설정한다.
	SetDefaultPort(m_port_default);

	// 4) Update한다.
	UpdateAcceptorStatus();

	// 5) ...
	m_checkReady.EnableWindow(m_bEnableControl);

	// 6) 1초마다 상태 체크
	SetTimer(0, 1000, nullptr);

	// return) 
	return TRUE;
}

void CChild_acceptor_control::OnBnClickedCheckAcceptorStartStop()
{
	// declare) 
	object_ptr<net::io::connective::Iacceptor> pacceptor = GetAcceptor();

	// check)
	RETURN_IF(pacceptor.empty());

	// 1) Ready 버튼을 얻는다.
	if(m_checkReady.GetCheck())
	{
		// declare)
		bool result;

		// - Accept를 시작한다.
		result = ListenAcceptor();

		// check) 
		if(result == false)
		{
			// - 다시 Button을 FALSE로 바꾼다.
			m_checkReady.SetCheck(BST_UNCHECKED);

			// return) 
			return;
		}

		// - 메시지를 전송한다.
		SendMessageToTarget((WPARAM)CGACCEPTOR_READY, (LPARAM)(net::io::Iconnective*)pacceptor.get());
	}
	else
	{
		// 1) Accept를 멈춘다.
		pacceptor->stop();

		// 2) 메시지를 전송한다.
		SendMessageToTarget((WPARAM)CGACCEPTOR_CLOSE, (LPARAM)pacceptor.get());
	}

	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_acceptor_control::OnBnClickedButtonAcceptorDisconnectAll()
{
	// declare) 
	object_ptr<net::io::connective::Iacceptor>	pacceptor = GetAcceptor();

	// check) pacceptor를 없으면 끝낸다.
	RETURN_IF(pacceptor.empty());

	// 1) Connectable Manager를 얻는다.
	object_ptr<net::io::Iconnectable_manager>	pmanagerConnectable	 = dynamic_pointer_cast<net::io::Iconnectable_manager>(pacceptor);

	// check) pmanagerConnectable를 nullptr이면 그냥 끝난다.
	RETURN_IF(pmanagerConnectable.empty())

	// 2) Acceptor의 Socket을 모두 접속종료한다.
	pmanagerConnectable->close_connectable_all();
}

void CChild_acceptor_control::OnBnClickedButtonAcceptorStatus()
{
	if(m_dlgConnectiveStatus.m_hWnd==nullptr)
	{
		// declare) 
		object_ptr<net::io::connective::Iacceptor>	pacceptor = GetAcceptor();

		// 1) 
		m_dlgConnectiveStatus.CreateModaless(pacceptor, this);
	}

	m_dlgConnectiveStatus.ShowWindow(SW_SHOW);
}

LRESULT CChild_acceptor_control::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				// 1) 먼저 IConnective로 Casting한다.
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);

				// 2) Acceptor를 설정한다.
				SetConnective(pconnective);
			}
			break;

	case	CGACCEPTOR_READY:
			break;

	case	CGACCEPTOR_CLOSE:
			break;

	case	CGACCEPTOR_START:
			break;

	case	CGACCEPTOR_STOP:
			break;

	case	CGUPDATE:
			{
				UpdateAcceptorStatus();
			}
			break;
	};

	return	0;
}

void CChild_acceptor_control::OnEnable(BOOL is_enable)
{
	// 1) Dialog::OnEnable()
	CChildDialog::OnEnable(is_enable);

	// 2) Acceptor를 얻어놓는다.
	object_ptr<net::io::connective::Iacceptor> pacceptor = GetAcceptor();

	// 3) enable
	BOOL bEnableCombo = is_enable!=FALSE && pacceptor.exist() && (pacceptor->socket_state()!= eSOCKET_STATE::LISTEN);

	// 4) Address & Port
	m_comboBindAddress.EnableWindow(bEnableCombo);
	m_editBindPort.EnableWindow(bEnableCombo);

	// 5) Ready/start Button
	m_checkReady.EnableWindow(m_bEnableControl && is_enable);

	// 6) Status/DisconnectAll Button
	m_buttonStatus.EnableWindow(is_enable);
	m_buttonDisconnectAll.EnableWindow(is_enable);

	// 7) Static
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
}

void CChild_acceptor_control::OnDestroy()
{
	// 1) Acceptor를 Reset한다.
	scoped_lock(m_csAcceptor)
	{
		m_pAcceptor.reset();
	}

	// 2) Combobox의 내용을 지우며 메모리도 할당해제한다.
	for(int i=0; i<m_comboBindAddress.GetCount(); ++i)
	{
		net::sockaddr* psockAddr = reinterpret_cast<net::sockaddr*>(m_comboBindAddress.GetItemData(i));

		SAFE_DELETE(psockAddr);
	}
	m_comboBindAddress.Clear();

	// 3) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CChild_acceptor_control::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Acceptor를 얻는다.
		object_ptr<net::io::connective::Iacceptor>	pacceptor = GetAcceptor();

		// check) pacceptor가 없으면 끝낸다.
		RETURN_IF(pacceptor.empty());

		// declare) Acceptor의 상태를 얻어낸다.
		eSOCKET_STATE statusSocket = pacceptor->socket_state();

		// check) 변화가 없을 경우 끝낸다.
		RETURN_IF(statusSocket == m_statusPre);

		// 1) Update한다.
		UpdateAcceptorStatus();

		// 2) 
		m_statusPre = statusSocket;
	}

	CChildDialog::OnTimer(nIDEvent);
}


}