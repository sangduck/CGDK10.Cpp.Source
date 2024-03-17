#include "pch.h"

extern	HINSTANCE	g_hInstance;

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connective_info, CChildDialog)

CChild_connective_info::CChild_connective_info(const statistics::unit_connective* _pinfo, CWnd* pParent)
	: CChildDialog(IDD_DIALOG_CONNECTIVE_INFO, pParent),
	m_pstatistics(_pinfo),
	m_countTry(0),
	m_count_connect(0),
	m_count_disconnect(0)
{
}

CChild_connective_info::~CChild_connective_info()
{
}

void CChild_connective_info::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CUSTOM_TCP_CONNECT,	m_graph_tcp_connect);
	DDX_Control(pDX, IDC_SLIDER_CONNECTION,		m_slider_scale_connection);
}


BEGIN_MESSAGE_MAP(CChild_connective_info, CChildDialog)
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connective_info::OnCGMessageNotify)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_TRY, &CChild_connective_info::OnBnClickedCheckGraphTry)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_CONNECT, &CChild_connective_info::OnBnClickedCheckGraphConnect)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_DISCONNECT, &CChild_connective_info::OnBnClickedCheckGraphDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CChild_connective_info::OnBnClickedResetStatistics)
END_MESSAGE_MAP()

BOOL CChild_connective_info::CreateChild(const statistics::unit_connective* _pinfo, CWnd* _pwnd)
{
	// 1) Statistics의 포인터를 저장한다.
	set_connection_info(_pinfo);

	// 2) 윈도우를 생성한다.
	auto result = Create(IDD_DIALOG_CONNECTIVE_INFO, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공~
	return TRUE;
}

BOOL CChild_connective_info::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. Graph설정.
	//-----------------------------------------------------------------
	// 1) Graph
	m_graph_tcp_connect.AddGraph(128, RGB(0xff, 0x00, 0x00));
	m_graph_tcp_connect.AddGraph(128, RGB(0x00, 0xff, 0x00));
	m_graph_tcp_connect.AddGraph(128, RGB(0x00, 0xff, 0xff));
	m_graph_tcp_connect.AddGraph(128, RGB(0xff, 0xff, 0x00));

	m_graph_tcp_connect.EnableGraph(1, false);
	m_graph_tcp_connect.EnableGraph(2, false);
	m_graph_tcp_connect.EnableGraph(3, false);

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

void CChild_connective_info::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pscroll_bar)
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

void CChild_connective_info::UpdateConnectiveInfo()
{
	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)

	// check) 
	RETURN_IF(this->m_pstatistics == nullptr);

	// - 현재 Tick과 Tick Gap을 구한다.
	chrono::tick::time_point tick_now = chrono::tick::now();
	chrono::milliseconds	 tick_differ = std::chrono::duration_cast<chrono::milliseconds>(tick_now - this->m_tick_last);

	// - 최종 시간을 last에 넣는다.
	this->m_tick_last = tick_now;


	//-----------------------------------------------------------------
	// 1. Traffice Info 계산하기.
	//-----------------------------------------------------------------
	// 1) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
	LONGLONG countTry			 = this->m_pstatistics->count_try.load() - this->m_statistics_offet_count_try;
	LONGLONG countConnect		 = this->m_pstatistics->count_success_connect.load() - this->m_statistics_offet_success_connect;
	LONGLONG countDisconnect	 = this->m_pstatistics->count_disconnect.load() - this->m_statistics_offet_disconnect;
	LONGLONG countFailConnect	 = this->m_pstatistics->count_fail_connect.load() - this->m_statistics_offet_fail_connect;
	LONGLONG countErrorDisconnect= this->m_pstatistics->count_error_disconnect.load() - this->m_statistics_offet_error_disconnect;

	// 2) 초당 전송률을 구한다.
	float fTry_PerSec			 = (countTry > this->m_countTry) ? static_cast<float>(countTry - this->m_countTry)*1000/tick_differ.count() : 0;
	float fConnected_PerSec		 = (countConnect > this->m_count_connect) ? static_cast<float>(countConnect - this->m_count_connect)*1000/tick_differ.count() : 0;
	float fDisconnected_PerSec	 = (countDisconnect > this->m_count_disconnect) ? static_cast<float>(countDisconnect - this->m_count_disconnect)*1000/tick_differ.count() : 0;

	// 3) 이전 값을 현재 값으로 Update한다.(다음 Frame에서의 계산을 위해)
	this->m_countTry			 = countTry;
	this->m_count_connect		 = countConnect;
	this->m_count_disconnect	 = countDisconnect;


	//-----------------------------------------------------------------
	// 2.control에 표시하기.
	//-----------------------------------------------------------------
	TCHAR		temp_string[256]={};
	LONGLONG	countKeep = this->m_pstatistics->count_keep.load();

	// 1) Connection Info
	m_graph_tcp_connect.AppendData(0, static_cast<int>(countKeep));
	m_graph_tcp_connect.AppendData(1, static_cast<int>(fTry_PerSec));
	m_graph_tcp_connect.AppendData(2, static_cast<int>(fConnected_PerSec));
	m_graph_tcp_connect.AppendData(3, static_cast<int>(fDisconnected_PerSec));
	m_graph_tcp_connect.Scroll(1);

	// 2) Connection Info
	__to_string(temp_string, countKeep);			this->SetDlgItemText(IDC_STATIC_KEEP_CONNECT,	temp_string);
	__to_string(temp_string, countTry);				this->SetDlgItemText(IDC_STATIC_TRY,			temp_string);
	__to_string(temp_string, countConnect);			this->SetDlgItemText(IDC_STATIC_CONNECTED,		temp_string);
	__to_string(temp_string, countDisconnect);		this->SetDlgItemText(IDC_STATIC_DISCONNECTED,	temp_string);
	__to_string(temp_string, countFailConnect);		this->SetDlgItemText(IDC_STATIC_FAIL_CONNECT,	temp_string);
	__to_string(temp_string, countErrorDisconnect);	this->SetDlgItemText(IDC_STATIC_ERROR_CLOSE,	temp_string);

	// 3) 초당 전송률 표시.
	__sprintf(temp_string, TEXT("%.2f"), fTry_PerSec);			this->SetDlgItemText(IDC_STATIC_TRY_PERSEC,				temp_string);
	__sprintf(temp_string, TEXT("%.2f"), fConnected_PerSec);	this->SetDlgItemText(IDC_STATIC_CONNECTED_PERSEC,		temp_string);
	__sprintf(temp_string, TEXT("%.2f"), fDisconnected_PerSec);	this->SetDlgItemText(IDC_STATIC_DISCONNECTED_PERSEC,	temp_string);
}

void CChild_connective_info::OnTimer(UINT_PTR nIDEvent)
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

void CChild_connective_info::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	GetDlgItem(IDC_STATIC_KEEP_CONNECT)->EnableWindow(is_enable);

	GetDlgItem(IDC_CHECK_GRAPH_TRY)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_GRAPH_CONNECT)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_GRAPH_DISCONNECT)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_TRY)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_CONNECTED)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DISCONNECTED)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_TRY_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_CONNECTED_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DISCONNECTED_PERSEC)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_FAIL_CONNECT)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_ERROR_CLOSE)->EnableWindow(is_enable);

	m_graph_tcp_connect.EnableWindow(is_enable);
	GetDlgItem(IDC_SLIDER_CONNECTION)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T4)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T5)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T6)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T7)->EnableWindow(is_enable);
	
	// Graph를 다시 그린다.
	m_graph_tcp_connect.InvalidateRect(nullptr);
}

void CChild_connective_info::OnDestroy()
{
	// 1) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_connective_info::set_connection_info(const statistics::unit_connective* _statistics_acceptor)
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

void CChild_connective_info::set_connection_info(const net::io::Iconnective* _pconnective)
{
	// check) 
	RETURN_IF(_pconnective == nullptr);

	// 1) Statistics정보를 출력한다.
	set_connection_info(&_pconnective->statistics_get_connection_info());
}

void CChild_connective_info::Ready()
{
	EnableWindow(TRUE);
}

void CChild_connective_info::close()
{
	EnableWindow(FALSE);
}

void CChild_connective_info::start()
{
}

void CChild_connective_info::stop()
{
}

LRESULT CChild_connective_info::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
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

void CChild_connective_info::OnBnClickedCheckGraphTry()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	iStatus	 = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_TRY))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(iStatus == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graph_tcp_connect.EnableGraph(1, iStatus==BST_CHECKED);
	m_graph_tcp_connect.Invalidate(FALSE);
}

void CChild_connective_info::OnBnClickedCheckGraphConnect()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	iStatus	 = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_CONNECT))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(iStatus == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graph_tcp_connect.EnableGraph(2, iStatus==BST_CHECKED);
	m_graph_tcp_connect.Invalidate(FALSE);
}

void CChild_connective_info::OnBnClickedCheckGraphDisconnect()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	iStatus	 = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_DISCONNECT))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(iStatus == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graph_tcp_connect.EnableGraph(3, iStatus==BST_CHECKED);
	m_graph_tcp_connect.Invalidate(FALSE);
}

void CChild_connective_info::OnBnClickedResetStatistics()
{
	// 1) reset
	this->m_statistics_offet_count_try = this->m_pstatistics->count_try.load() ;
	this->m_statistics_offet_success_connect = this->m_pstatistics->count_success_connect.load();
	this->m_statistics_offet_disconnect = this->m_pstatistics->count_disconnect.load();
	this->m_statistics_offet_fail_connect = this->m_pstatistics->count_fail_connect.load();
	this->m_statistics_offet_error_disconnect = this->m_pstatistics->count_error_disconnect.load();
	this->m_tick_last = chrono::tick::now();

	// 2) 최종 시간을 last에 넣는다.
	this->UpdateConnectiveInfo();
}


}