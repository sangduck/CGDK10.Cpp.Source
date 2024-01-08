// CGChildBinderControl.cpp : implementation file
//

#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_binder_control, CDialog)

CChild_binder_control::CChild_binder_control(CWnd* pParent)
	: CChildDialog(IDD_DIALOG_BINDER_CONTROL, pParent)
{
	m_port_default			 = 20000;
}

CChild_binder_control::~CChild_binder_control()
{
}

void CChild_binder_control::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_BIND_ADDRESS, m_comboBindAddress);
	DDX_Control(pDX, IDC_CHECK_BIND, m_checkBind);
	DDX_Control(pDX, IDC_BUTTON_BINDER_STATUS, m_buttonStatus);
}


BEGIN_MESSAGE_MAP(CChild_binder_control, CChildDialog)
	ON_BN_CLICKED(IDC_CHECK_BIND, &CChild_binder_control::OnBnClickedCheckBind)
	ON_BN_CLICKED(IDC_BUTTON_BINDER_STATUS, &CChild_binder_control::OnBnClickedButtonBinderStatus)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_binder_control::OnCGMessageNotify)
END_MESSAGE_MAP()


// CChild_binder_control message handlers

BOOL CChild_binder_control::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. 설정
	//-----------------------------------------------------------------
	// 1) Local IP Address setting
	m_comboBindAddress.AddString(TEXT("ADDR_ANY"));

	{
		// - alloc sockaddr
		net::sockaddr* psock_addr = new net::sockaddr(net::ip::address_v4::loopback(), 0);

		// - ..
		int index = m_comboBindAddress.AddString(TEXT("localhost"));
		m_comboBindAddress.SetItemData(index, reinterpret_cast<DWORD_PTR>(psock_addr));
	}

	for(int i=0; i<api::socket::instance().get_local_address_count(); ++i)
	{
		net::sockaddr* psock_addr = new net::sockaddr();

		*psock_addr = api::socket::instance().get_local_address(i);

		TCHAR temp_string[256];
		psock_addr->to_string<TCHAR>(temp_string, 256);

		int index = m_comboBindAddress.AddString(temp_string);
		m_comboBindAddress.SetItemData(index, reinterpret_cast<DWORD_PTR>(psock_addr));
	}
	m_comboBindAddress.SetCurSel(0);
	SetDefaultPort(m_port_default);


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChild_binder_control::OnBnClickedCheckBind()
{
	// 1) Ready 버튼을 얻는다.
	if(m_checkBind.GetCheck())
	{
		// declare)
		bool	result;

		// - Accept를 시작한다.
		result	 = Bind();

		// check) 
		if(result==false)
		{
			// - 다시 Button을 FALSE로 바꾼다.
			m_checkBind.SetCheck(BST_UNCHECKED);

			// return) 
			return;
		}

		// - 메시지를 전송한다.
		//SendMessageToTarget((WPARAM)CGACCEPTOR_READY, (LPARAM)(net::io::Iconnective*)m_pAcceptor.get());
	}
	else
	{
		// 1) Accept를 멈춘다.
		//pbind->CloCloseSocket();

		// 2) 메시지를 전송한다.
		//SendMessageToTarget((WPARAM)CGACCEPTOR_CLOSE, (LPARAM)m_pAcceptor.get());
	}

	// 3) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_binder_control::OnBnClickedButtonBinderStatus()
{
}

void CChild_binder_control::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Update한다.
		UpdateBinderStatus();
	}

	CChildDialog::OnTimer(nIDEvent);
}

void CChild_binder_control::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	// TODO: Add your message handler code here
}

void CChild_binder_control::OnDestroy()
{
	// 1) Acceptor를 Reset한다.
	scoped_lock(m_csBinder)
	{
		m_pBinder.reset();
	}

	// 2) Combobox의 내용을 지우며 메모리도 할당해제한다.
	for(int i=0; i<m_comboBindAddress.GetCount(); ++i)
	{
		net::sockaddr* psock_addr = reinterpret_cast<net::sockaddr*>(m_comboBindAddress.GetItemData(i));

		SAFE_DELETE(psock_addr);
	}
	m_comboBindAddress.Clear();

	// 3) 부모 Class의 on_destroy
	__super::OnDestroy();
}

BOOL CChild_binder_control::CreateChild(net::io::Ibinder* _pbinder, CWnd* _pwnd)
{
	// declare) 
	BOOL	result;

	// 1) Acceptor를 설정한다.
	SetBinder(_pbinder);

	// 2) 윈도우를 생성한다.
	result	 = Create(IDD_DIALOG_BINDER_CONTROL, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 
	return	TRUE;
}

void CChild_binder_control::SetDefaultPort(uint16_t _port)
{
	m_port_default	 = _port;

	SetDlgItemInt(IDC_EDIT_BIND_PORT, _port);
}

void CChild_binder_control::SetBinder(net::io::Ibinder* _pbinder)
{
	// 1) Acceptor를 설정한다.
	scoped_lock(m_csBinder)
	{
		m_pBinder = _pbinder;
	}

	// 2) Acceptor의 상태를 Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_binder_control::UpdateBinderStatus()
{
	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)

	// check) 윈도우가 생성되지 않았다면 여기서 끝낸다.
	RETURN_IF(m_hWnd == nullptr);


	//-----------------------------------------------------------------
	// 1. Show/Hide상태를 결정한다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Ibinder> pbind;

	// 1) m_pBinder를 얻는다.
	scoped_lock(m_csBinder)
	{
		pbind	 = m_pBinder;
	}

	// 2) pbind가 있는가?
	BOOL	result	 = pbind.exist();

	// 1) Window를 enable/Disable한다.
	m_comboBindAddress.EnableWindow(result);
	GetDlgItem(IDC_EDIT_BIND_PORT)->EnableWindow(result);
	m_checkBind.EnableWindow(result);
	m_buttonStatus.EnableWindow(result);

	// check) result가 FASLE면 끝낸다.
	RETURN_IF(result == FALSE);


	//-----------------------------------------------------------------
	// 2. 내용을 Update한다.
	//-----------------------------------------------------------------
	auto psocket = dynamic_cast<net::io::Isocket*>(pbind.get());

	if(psocket != nullptr && psocket->is_binded())
	{
		// 1) enable한다.
		m_comboBindAddress.EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_BIND_PORT)->EnableWindow(FALSE);

		m_checkBind.SetCheck(BST_CHECKED);

		// 2) Socket의 Address를 얻는다.
		auto address_socket = psocket->local_endpoint();

		// 3) socket address가 있다면...
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

					if(*addrLocal==address_socket)
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

	}
	else
	{
		// 1) enable한다.
		m_comboBindAddress.EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_BIND_PORT)->EnableWindow(TRUE);

		m_checkBind.SetCheck(BST_UNCHECKED);

		// 2) CurSel을 ADDR_ANY로...
		m_comboBindAddress.SetCurSel(0);
	}
}


LRESULT CChild_binder_control::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
{
	switch(_wparam)
	{
	case	CGBINDER_SET:
			{
				// 1) 먼저 IConnective로 Casting한다.
				net::io::Ibinder*	pbind	 = (net::io::Ibinder*)_lparam;

				// 2) Acceptor를 설정한다.
				SetBinder(pbind);
			}
			break;

	case	CGBINDER_OPEN:
			{
			}
			break;

	case	CGUPDATE:
			{
				UpdateBinderStatus();
			}
			break;
	};

	return	0;
}




// CChild_acceptor_control message handlers
bool CChild_binder_control::Bind()
{
	// declare) 
	object_ptr<net::io::Ibinder> pbind;

	// 1) m_pBinder를 얻는다.
	scoped_lock(m_csBinder)
	{
		pbind = m_pBinder;
	}

	// check)
	RETURN_IF(pbind.empty(), false);


	//-----------------------------------------------------------------
	// 1. 먼저 Address와 Port를 구한다.
	//-----------------------------------------------------------------
	net::sockaddr addr_bind;

	// 1) 현재 선택된 Item을 얻는다.
	int	iCurSel = m_comboBindAddress.GetCurSel();

	// 2) 선택에 따라 Address를 설정한다.
	if(iCurSel==0)
	{
		addr_bind = net::sockaddr{ net::ip::address_v4::any(), 0 };
	}
	else
	{
		addr_bind = api::socket::instance().get_local_address(iCurSel-1);
	}

	// 3) Port를 얻는다.
	uint16_t wPort = (uint16_t)GetDlgItemInt(IDC_EDIT_BIND_PORT);

	// 4) Port를 설정한다.
	addr_bind.u_v4.sin_port = htons(wPort);
	addr_bind.u_v4.sin_family = AF_INET;


	//-----------------------------------------------------------------
	// 2. 시작한다.
	//-----------------------------------------------------------------
	// 1) Listen한다.
	auto result = pbind->bind(addr_bind);

	// check) 제대로 Bind와 Listen되었는지 확인한다.
	if(result == FALSE)
	{
		AfxMessageBox(TEXT("해당 Port에 Bind와 Listen에 실패했습니다."), MB_OK);
		return false;
	}

	// return)
	return true;
}


}