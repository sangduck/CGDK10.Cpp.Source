#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_connective_status, CDialog)

CDialog_connective_status::CDialog_connective_status(net::io::connective::manager* p_pmangerConnective, CWnd* pParent)
	: CDialog(IDD_DIALOG_CONNECTIVE_STATUS, pParent)
	, m_pmangerConnective(p_pmangerConnective)
{
}

CDialog_connective_status::CDialog_connective_status(net::io::Iconnective* _pconnective, CWnd* pParent)
	: CDialog(IDD_DIALOG_CONNECTIVE_STATUS, pParent)
	, m_pconnective(_pconnective)
{
}

CDialog_connective_status::~CDialog_connective_status()
{
	scoped_lock(m_csConnective)
	{
		CGASSERT_ERROR(m_pconnective.empty());
	}
}

void CDialog_connective_status::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL, m_comboUpdateInterval);
	DDX_Control(pDX, IDC_COMBO_ACCEPTOR, m_comboConnective);
}


BEGIN_MESSAGE_MAP(CDialog_connective_status, CDialog)
	ON_BN_CLICKED(IDOK, &CDialog_connective_status::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialog_connective_status::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL, &CDialog_connective_status::OnCbnSelchangeComboUpdateInterval)
	ON_CBN_SELCHANGE(IDC_COMBO_ACCEPTOR, &CDialog_connective_status::OnCbnSelchangeConnective)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CDialog_connective_status::OnBnClickedButtonUpdate)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CDialog_connective_status::OnCGMessageNotify)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_connective_status message handlers
int	CDialog_connective_status::m_s_iUpdateInverval = 2;

BOOL CDialog_connective_status::CreateModaless(net::io::connective::manager* p_pmangerAcceptor, CWnd* p_pParent)
{
	scoped_lock(m_csConnective)
	{
		m_pmangerConnective = p_pmangerAcceptor;
	}

	return Create(IDD_DIALOG_CONNECTIVE_STATUS, p_pParent);
}

BOOL CDialog_connective_status::CreateModaless(net::io::Iconnective* _pconnective, CWnd* p_pParent)
{
	scoped_lock(m_csConnective)
	{
		m_pconnective = _pconnective;
	}

	return Create(IDD_DIALOG_CONNECTIVE_STATUS, p_pParent);
}


BOOL CDialog_connective_status::OnInitDialog()
{
	CDialog::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Connectable Manager를 생성한다.
	m_ChildConnectableManager.CreateChild(nullptr, this);
	m_ChildConnectableManager.Move(10, 135);

	// 2) Window를 Show한다.
	m_ChildConnectableManager.ShowWindow(SW_SHOW);


	//-----------------------------------------------------------------
	// 1. ConnectiveManager의 설정
	//-----------------------------------------------------------------
	scoped_lock(m_csConnective)
	{
		// 1) List를 설정한다.
		if(m_pmangerConnective!=nullptr)
		{
		}
		else if(m_pconnective.exist())
		{
			// - Identifier를 구한다.
			auto pnameable = dynamic_cast<object::Inameable*>(m_pconnective.get());

			// declare) 
			int	index = 0;

			// - Connect를 추가한다.
			if(pnameable != nullptr)
			{
			#ifdef _UNICODE
					TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
					encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH);
					index = m_comboConnective.AddString(text);
					free(text);
			#else
					index = m_comboConnective.AddString(pnameable->name().data());
			#endif
			}
			else
			{
				index = m_comboConnective.AddString(TEXT("NoName"));
			}

			// - Connective를 설정한다.
			m_comboConnective.SetItemDataPtr(index, m_pconnective);
			m_comboConnective.SetCurSel(0);

			// - Reference count를 더한다.
			m_pconnective->add_ref();
		}

		// 2) Connective를 설정한다.
		SetConnective(m_pconnective);
	}


	//-----------------------------------------------------------------
	// 3. start
	//-----------------------------------------------------------------
	// 3) Update Combo의 값을 설정하고 타이머를 설정한다.
	m_comboUpdateInterval.SetCurSel(m_s_iUpdateInverval);
	SetUpdateInterval(m_s_iUpdateInverval);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDialog_connective_status::OnBnClickedOk()
{
	// 1) 타이머를 끈다.
	KillTimer(0);

	// 2) OnOk
	OnOK();
}

void CDialog_connective_status::OnBnClickedCancel()
{
	// 2) 타이머를 끈다.
	KillTimer(0);

	// 2) OnCancel
	OnCancel();
}

void CDialog_connective_status::SetConnective(net::io::Iconnective* _pconnective)
{
	// 1) 설정한다.
	scoped_lock(m_csConnective)
	{
		// 1) Connective를 설정한다.
		m_pconnective = _pconnective;
	}

	// 2) Connectable Manager를 구한다.
	auto pconnectable_manager = dynamic_cast<net::io::Iconnectable_manager*>(_pconnective);

	// 3) Connectable Manager를 설정한다.
	m_ChildConnectableManager.set_connectable_manager(pconnectable_manager);

	// 4) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CDialog_connective_status::ResetConnectiveList()
{
	// 1) Connectable Manager도 먼저 제거한다.
	m_ChildConnectableManager.clear_connectable_list();

	// 2) Connective를 해제
	scoped_lock(m_csConnective)
	{
		m_pmangerConnective.reset();
		m_pconnective.reset();
	}

	// 3) 모든 Connective를 제거한다.
	for(int i=0; i<m_comboConnective.GetCount(); ++i)
	{
		// 1) Connective를 얻는다.
		net::io::Iconnective*	pconnective	 = (net::io::Iconnective*)m_comboConnective.GetItemDataPtr(i);

		// check) Connective가 nullptr이면 안됀다
		CGASSERT_ERROR(pconnective != nullptr);

		// 2) Connective를 Release한다.
		pconnective->release();
	}
	m_comboConnective.Clear();
}


void CDialog_connective_status::UpdateConnectiveStatus()
{
	// declare) 
	object_ptr<net::io::Iconnective> pconnective;

	scoped_lock(m_csConnective)
	{
		pconnective = m_pconnective;
	}

	if(pconnective.exist())
	{
		// 1) IAcceptor Interface를 Query한다.
		auto pacceptor = dynamic_cast<net::io::connective::Iacceptor*>(pconnective.get());

		// 2) IAcceptor를 구했다면 설정한다.
		if(pacceptor!=nullptr)
		{
			GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_ADDRESS)->EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_PORT)->EnableWindow(TRUE);

			if(pacceptor->is_binded())
			{
				// - socket address를 얻는다.
				auto addr_socket = pacceptor->local_endpoint();

				// - socket address가 있다면...
				if(addr_socket.exist())
				{
					if(addr_socket.is_bind_any()==false)
					{
						SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_ADDRESS, addr_socket.to_string<TCHAR>().c_str());
					}
					else
					{
						SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_ADDRESS, TEXT("ADDR_ANY"));
					}

					SetDlgItemInt(IDC_STATIC_ACCEPTOR_BIND_PORT, addr_socket.port());
				}
				else
				{
					SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_ADDRESS, TEXT("ADDR_ANY"));
					SetDlgItemInt(IDC_STATIC_ACCEPTOR_BIND_PORT, 10000);
				}

			}
			else
			{
				SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_ADDRESS, TEXT("Unbinded"));
				SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_PORT, TEXT(""));
			}

			// 4) View 상태를 설정한다.
			//SetViewStatus(pconnective->is_listening());
		}
		// 3) 구하지 못했다면 Acceptor부분을 Reset한다.
		else
		{
			GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_ADDRESS)->EnableWindow(FALSE);
			GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_PORT)->EnableWindow(FALSE);

			SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_ADDRESS, TEXT(""));
			SetDlgItemText(IDC_STATIC_ACCEPTOR_BIND_PORT, TEXT(""));
		}
	}
	else
	{
		SetDlgItemText(IDC_STATIC_PREPARED,			TEXT(""));
		SetDlgItemText(IDC_STATIC_TRY,				TEXT(""));
		SetDlgItemText(IDC_STATIC_CONNECTED,		TEXT(""));
		SetDlgItemText(IDC_STATIC_FAIL_CONNECTED,	TEXT(""));
		SetDlgItemText(IDC_STATIC_DISCONNECTED,		TEXT(""));
		m_ChildConnectableManager.clear_connectable_list();
		return;
	}

	// declare) 
	SetDlgItemInt(IDC_STATIC_PREPARED,				static_cast<uint32_t>(pconnective->statistics_get_now_async_io()));
	SetDlgItemInt(IDC_STATIC_TRY,					static_cast<uint32_t>(pconnective->statistics_get_try()));
	SetDlgItemInt(IDC_STATIC_CONNECTED,				static_cast<uint32_t>(pconnective->statistics_get_success_connect()));
	SetDlgItemInt(IDC_STATIC_FAIL_CONNECTED,		static_cast<uint32_t>(pconnective->statistics_get_fail_connect()));
	SetDlgItemInt(IDC_STATIC_DISCONNECTED,			static_cast<uint32_t>(pconnective->statistics_get_disconnect()));

	m_ChildConnectableManager.UpdateConnectableList();
}

void CDialog_connective_status::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 
	int	selected	 = m_comboUpdateInterval.GetCurSel();

	// 2) 
	SetUpdateInterval(selected);
}

void CDialog_connective_status::OnCbnSelchangeConnective()
{
	// 1) 현재 선택된 Acceptor의 Index를 얻는다.
	int	selected = m_comboConnective.GetCurSel();

	// declare) 
	object_ptr<net::io::Iconnective> pconnective;

	// 2) 새로운 Acceptor를 얻는다.
	scoped_lock(m_csConnective)
	{
		pconnective	 = (net::io::Iconnective*)m_comboConnective.GetItemDataPtr(selected);
	}

	// 3) Socket 상태를 설정한다.
	SetConnective(pconnective);
}

void CDialog_connective_status::OnBnClickedButtonUpdate()
{
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CDialog_connective_status::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// - 상태를 Update한다.
		UpdateConnectiveStatus();
	}

	CDialog::OnTimer(nIDEvent);
}

void CDialog_connective_status::SetUpdateInterval(int p_iInterval)
{
	// 1) 
	m_s_iUpdateInverval = p_iInterval;

	// 2) 
	switch(m_s_iUpdateInverval)
	{
	// No Update
	case	0:
			KillTimer(0);
			break;

	// 1sec
	case	1:
			SetTimer(0, 1000, nullptr);
			break;

	// 2sec
	case	2:
			SetTimer(0, 2000, nullptr);
			break;

	// 5sec
	case	3:
			SetTimer(0, 5000, nullptr);
			break;

	// 10sec
	case	4:
			SetTimer(0, 10000, nullptr);
			break;

	// 30sec
	case	5:
			SetTimer(0, 30000, nullptr);
			break;

	// 1min
	case	6:
			SetTimer(0, 60000, nullptr);
			break;
	};
}


void CDialog_connective_status::SetViewStatus(bool p_bStatus)
{
	GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_ADDRESS)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_PORT)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_PREPARED)->EnableWindow(p_bStatus);

	GetDlgItem(IDC_STATIC_TRY)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_CONNECTED)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_FAIL_CONNECTED)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_DISCONNECTED)->EnableWindow(p_bStatus);

	m_comboUpdateInterval.EnableWindow(p_bStatus);
	GetDlgItem(IDC_BUTTON_UPDATE)->EnableWindow(p_bStatus);
}


LRESULT CDialog_connective_status::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);

				SetConnective(pconnective);
			}
			break;

	case	CGACCEPTOR_READY:
			{
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);

				SetConnective(pconnective);

				EnableWindow(TRUE);
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
				EnableWindow(FALSE);
			}
			break;

	case	CGACCEPTOR_START:
			{
			}
			break;

	case	CGACCEPTOR_STOP:
			{
			}
			break;

	case	CGUPDATE:
			{
				UpdateConnectiveStatus();
			}
			break;
	};

	return	0;
}

void CDialog_connective_status::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	//-----------------------------------------------------------------
	// 1. Connective를 새로 설정한다.
	//-----------------------------------------------------------------
	// 1) Connective를 List에 다시 추가한다.
	scoped_lock(m_csConnective)
	{
		SetConnective(m_pconnective);
	}

	// 2) Timer를 설정한다.
	SetUpdateInterval(m_s_iUpdateInverval);


	//-----------------------------------------------------------------
	// 2. 값을 설정.
	//-----------------------------------------------------------------
	// 1) Acceptor를 List에 추가한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CDialog_connective_status::OnDestroy()
{
	// 1) 모두 Reset한다.
	ResetConnectiveList();

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}


}