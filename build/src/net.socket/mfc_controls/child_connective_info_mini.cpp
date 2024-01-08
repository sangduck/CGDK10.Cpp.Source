#include "pch.h"

extern	HINSTANCE	g_hInstance;

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connective_info_mini, CChildDialog)

CChild_connective_info_mini::CChild_connective_info_mini(const statistics::unit_connective* _pinfo, CWnd* pParent)
	: CChildDialog(IDD_DIALOG_CONNECTIVE_INFO_MINI, pParent),
	m_pstatistics(_pinfo),
	m_count_connect(0),
	m_count_disconnect(0)
{
}

CChild_connective_info_mini::~CChild_connective_info_mini()
{
}

void CChild_connective_info_mini::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CUSTOM_TCP_CONNECT,	m_graph_tcp_connect);
	DDX_Control(pDX, IDC_SLIDER_CONNECTION,		m_slider_scale_connection);
}


BEGIN_MESSAGE_MAP(CChild_connective_info_mini, CChildDialog)
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connective_info_mini::OnCGMessageNotify)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_CONNECT, &CChild_connective_info_mini::OnBnClickedCheckGraphConnect)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_DISCONNECT, &CChild_connective_info_mini::OnBnClickedCheckGraphDisconnect)
END_MESSAGE_MAP()

BOOL CChild_connective_info_mini::CreateChild(const statistics::unit_connective* _pinfo, CWnd* _pwnd)
{
	// 1) Statistics의 포인터를 저장한다.
	set_connection_info(_pinfo);

	// 2) 윈도우를 생성한다.
	auto result = Create(IDD_DIALOG_CONNECTIVE_INFO_MINI, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공~
	return	TRUE;
}

BOOL CChild_connective_info_mini::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. Graph설정.
	//-----------------------------------------------------------------
	// 1) Graph
	m_graph_tcp_connect.AddGraph(128, RGB(0xff, 0x00, 0x00));
	m_graph_tcp_connect.AddGraph(128, RGB(0x00, 0xff, 0xff));
	m_graph_tcp_connect.AddGraph(128, RGB(0xff, 0xff, 0x00));

	m_graph_tcp_connect.EnableGraph(1, false);
	m_graph_tcp_connect.EnableGraph(2, false);

	// 2) Slider
	m_slider_scale_connection.SetRange(0, 6);
	m_slider_scale_connection.SetPos(4);		// 128/8=16해서 얻은 값

	// 3) Tick 초기화.
	m_tick_last = chrono::tick::now();


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_connective_info_mini::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pscroll_bar)
{
	switch(pscroll_bar->GetDlgCtrlID())
	{
	case	IDC_SLIDER_CONNECTION:
			{
				int	value	 = m_slider_scale_connection.GetPos();

				int	iScale	 = 8;
				for(; value>0; --value)
				{
					iScale	*=2;
				}
				
				m_graph_tcp_connect.SetVerticalScale(iScale);
			}
			break;
	};

	CChildDialog::OnVScroll(nSBCode, nPos, pscroll_bar);
}

void CChild_connective_info_mini::UpdateConnectiveInfo()
{
	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)

	// check) 
	RETURN_IF(m_pstatistics == nullptr);

	// - 현재 Tick과 Tick Gap을 구한다.
	auto tick_now	 = chrono::tick::now();
	auto tick_differ = std::chrono::duration_cast<chrono::milliseconds>(tick_now - m_tick_last);

	// - 최종 시간을 last에 넣는다.
	m_tick_last = tick_now;


	//-----------------------------------------------------------------
	// 1. Traffice Info 계산하기.
	//-----------------------------------------------------------------
	// 1) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
	uint64_t countConnect		  = m_pstatistics->count_success_connect.load();
	uint64_t countDisconnect	  = m_pstatistics->count_disconnect.load();
	uint64_t countErrorDisconnect = m_pstatistics->count_error_disconnect.load();

	// 2) 초당 전송률을 구한다.
	float	Connected_PerSec	 = ((float)(countConnect - m_count_connect))*1000/tick_differ.count();
	float	Disconnected_PerSec	 = ((float)(countDisconnect - m_count_disconnect))*1000/tick_differ.count();

	// 3) 이전 값을 현재 값으로 Update한다.(다음 Frame에서의 계산을 위해)
	m_count_connect				 = countConnect;
	m_count_disconnect			 = countDisconnect;


	//-----------------------------------------------------------------
	// 2.control에 표시하기.
	//-----------------------------------------------------------------
	TCHAR		tempString[256]={};
	LONGLONG	countKeep = m_pstatistics->count_keep;

	// 1) Connection Info
	m_graph_tcp_connect.AppendData(0, static_cast<int>(countKeep));
	m_graph_tcp_connect.AppendData(1, static_cast<int>(Connected_PerSec));
	m_graph_tcp_connect.AppendData(2, static_cast<int>(Disconnected_PerSec));
	m_graph_tcp_connect.Scroll(1);

	// 2) Connection Info
	__to_string(tempString, countKeep);				SetDlgItemText(IDC_STATIC_KEEP_CONNECT,			tempString);
	__to_string(tempString, countConnect);			SetDlgItemText(IDC_STATIC_CONNECTED,			tempString);
	__to_string(tempString, countDisconnect);		SetDlgItemText(IDC_STATIC_DISCONNECTED,			tempString);
	__to_string(tempString, countErrorDisconnect);	SetDlgItemText(IDC_STATIC_ERROR_CLOSE,			tempString);

	// 3) 초당 전송률 표시.
	__sprintf(tempString, TEXT("%.2f"), Connected_PerSec);		SetDlgItemText(IDC_STATIC_CONNECTED_PERSEC,		tempString);
	__sprintf(tempString, TEXT("%.2f"), Disconnected_PerSec);	SetDlgItemText(IDC_STATIC_DISCONNECTED_PERSEC,	tempString);
}

void CChild_connective_info_mini::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Update한다.
		UpdateConnectiveInfo();
	}

	CChildDialog::OnTimer(nIDEvent);
}

void CChild_connective_info_mini::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	GetDlgItem(IDC_STATIC_KEEP_CONNECT)->EnableWindow(is_enable);

	GetDlgItem(IDC_CHECK_GRAPH_CONNECT)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_GRAPH_DISCONNECT)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_CONNECTED)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DISCONNECTED)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_CONNECTED_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DISCONNECTED_PERSEC)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_ERROR_CLOSE)->EnableWindow(is_enable);

	m_graph_tcp_connect.EnableWindow(is_enable);
	GetDlgItem(IDC_SLIDER_CONNECTION)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	
	// Graph를 다시 그린다.
	m_graph_tcp_connect.InvalidateRect(nullptr);
}

void CChild_connective_info_mini::OnDestroy()
{
	// 1) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_connective_info_mini::set_connection_info(const statistics::unit_connective* _statistics_acceptor)
{
	// 1) 값을 설정한다.
	scoped_lock(m_lockable_statistics)
	{
		m_pstatistics = _statistics_acceptor;
	}

	// 2) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_connective_info_mini::set_connection_info(const net::io::Iconnective* _pconnective)
{
	// check) 
	RETURN_IF(_pconnective == nullptr);

	// 1) Statistics정보를 출력한다.
	set_connection_info(&_pconnective->statistics_get_connection_info());
}

void CChild_connective_info_mini::Ready()
{
	EnableWindow(TRUE);
}

void CChild_connective_info_mini::close()
{
	EnableWindow(FALSE);
}

void CChild_connective_info_mini::start()
{
}

void CChild_connective_info_mini::stop()
{
}

LRESULT CChild_connective_info_mini::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);
				set_connection_info(pconnective);
			}
			break;

	case	CGACCEPTOR_READY:
			{
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);
				set_connection_info(pconnective);

				Ready();
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
				close();
			}
			break;

	case	CGACCEPTOR_START:
			{
				start();
			}
			break;

	case	CGACCEPTOR_STOP:
			{
				stop();
			}
			break;

	case	CGUPDATE:
			{
				UpdateConnectiveInfo();
			}
			break;
	};

	return	0;
}

void CChild_connective_info_mini::OnBnClickedCheckGraphConnect()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_CONNECT))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graph_tcp_connect.EnableGraph(2, status == BST_CHECKED);
	m_graph_tcp_connect.Invalidate(FALSE);
}

void CChild_connective_info_mini::OnBnClickedCheckGraphDisconnect()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_DISCONNECT))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graph_tcp_connect.EnableGraph(3, status == BST_CHECKED);
	m_graph_tcp_connect.Invalidate(FALSE);
}


}