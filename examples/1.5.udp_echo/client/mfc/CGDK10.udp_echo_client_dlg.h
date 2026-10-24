
// tcp_echo_dlg.h: 헤더 파일
//

#pragma once

class Ctcp_echo_client_dlg : public CDialogEx
{
// 생성입니다.
public:
	Ctcp_echo_client_dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCP_ECHOWINDOWSMFC_DIALOG };
#endif

protected:
	HICON m_hIcon;

	virtual BOOL				OnInitDialog();
	virtual void				DoDataExchange(CDataExchange* pDX);
	virtual BOOL				PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg void				OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg HCURSOR				OnQueryDragIcon();
	afx_msg void				OnPaint();
	afx_msg void				OnTimer(UINT_PTR nIDEvent);
	afx_msg void				OnSystemExit();
	afx_msg void				OnAboutCGDKinfo();
	afx_msg void				OnBnClickedOk();
	afx_msg void				OnBnClickedCancel();
	afx_msg void				OnBnClickedButtonBind();
	afx_msg void				OnBnClickedButtonClosesocket();
	afx_msg void				OnBnClickedButtonClosesocketAll();
	afx_msg void				OnBnClickedButtonDestAddressAdd();
	afx_msg void				OnBnClickedButtonDestAddressDel();
	afx_msg void				OnCbnSelchangeComboMessageSize();
	afx_msg void				OnBnClickedCheckSendTestStartstop();
	afx_msg void				OnBnClickedCheckRelayEcho();
	afx_msg void				OnBnClickedButtonSingleSend();
	afx_msg void				OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void				OnEnChangeEditSendPerOnetime();
	afx_msg void				OnBnClickedButtonPool();
	afx_msg void				OnBnClickedButtonThread();
	afx_msg void				OnDeltaposSpinTps1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void				OnDeltaposSpinTps10(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void				OnDeltaposSpinTps100(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void				OnDeltaposSpinTps1000(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void				OnDeltaposSpinTps10000(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void				OnDeltaposSpinTps100000(NMHDR* pNMHDR, LRESULT* pResult);

private:
			lock_object			m_cs_listctrl_sockets;
			CListCtrl			m_listctrl_sockets;
			lock_object			m_cs_listctrl_dest_address;
			CListCtrl			m_listctrl_dest_address;
			CSliderCtrl			m_sliderSendTimes;
			CComboBox			m_comboMessageSize;
			CToolTipCtrl		m_ctrl_tooltip;
			object_ptr<socket_udp> m_socket_selected;
public:
			CChild_connective_info		m_child_connective_info;
			CChild_traffic_info			m_child_traffic_info;
			CDialog_object_factory_status m_dlg_object_factory_status;
			CDialog_thread_status		m_dlg_thread_status;
		
			// stage
			void				list_add_socket_udp(socket_udp* _psocket);
			void				list_remove_socket_udp(socket_udp* _psocket);
			void				list_update_socket_udp(socket_udp* _psocket);
			void				list_update_socket_udp(int _index);
			void				list_update_socket_udp();
			void				_list_update_socket_udp(int _index);
			void				list_set_dest_address(socket_udp* _psocket);
			void				enable_dest_address(BOOL _enable);


			net::sockaddr		ReadBindAddress();
			net::sockaddr		ReadDestinationAddress();
			afx_msg void OnLvnItemchangedListSocket(NMHDR* pNMHDR, LRESULT* pResult);
};
