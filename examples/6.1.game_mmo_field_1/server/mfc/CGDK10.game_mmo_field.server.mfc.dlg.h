#pragma once

class CCGDK10_game_mmo_field_server_mfc_dlg : public CDialogEx, virtual public Imessageable
{
public:
	CCGDK10_game_mmo_field_server_mfc_dlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CGDK10GAME_MMO_FIELDSERVERMFC_DIALOG };
#endif

protected:
	virtual void				DoDataExchange(CDataExchange* pDX);
	virtual BOOL				OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()

	// 1) system message
	afx_msg void				OnPaint();
	afx_msg HCURSOR				OnQueryDragIcon();
	afx_msg void				OnTimer(UINT_PTR nIDEvent);

	// 2) control events
	afx_msg void				OnBnClickedOk();
	afx_msg void				OnBnClickedCancel();
	afx_msg void				OnBnClickedButtonPoolStatus();
	afx_msg void				OnBnClickedButtonThreadInfo();

	// 3) menu events
	afx_msg void				OnSystemServiceStart();
	afx_msg void				OnSystemServiceCancel();
	afx_msg void				OnSystemServiceStop();
	afx_msg void				OnSystemExit();

	// 4) CGDK.message
	afx_msg LRESULT				on_notify_message_update(WPARAM _wParam, LPARAM _lParam);

private:
	// 1) message를 받았을 때 불려지는 함수.
	virtual result_code			process_message(sMESSAGE& _msg) override;

	// 2) message
			void				on_service_initializing_pending();
			void				on_service_destroying_pending();
			void				on_service_running();
			void				on_service_start_pending();
			void				on_service_stop_pending();
			void				on_service_stopped();
			void				on_service_update();

protected:
	// 1) MFC resource & controls
			HICON				m_hIcon;

	// 2) server sockets
			CChild_acceptor_control m_child_acceptor_user;
			lock_object			m_lockable_list_area_group;
			CListCtrl			m_list_area_group;

	// 3) traffic info
			CChild_traffic_info	m_child_traffic_info;

	// 3) logger view
			CChild_logger_view	m_child_logger_view;

	// 4) factory & thread status
			CDialog_object_factory_status m_dlg_factory_status;
			CDialog_thread_status m_dlg_thread_status;

	// 5) service
	void				request_service_initialize();
	void				request_service_start();
	void				request_service_cancel();
	void				request_service_stop();
	void				request_service_destroy();
	void				enable_server_controls(BOOL _bEnable);

// declare)
	struct EVENT_AREA_GROUP_NOTIFY
	{
	public:
		uint32_t				message;
		object_ptr<Igroup_user>	psource;
	};
	lock_object			m_cs_vector_event_area_group;
	std::vector<EVENT_AREA_GROUP_NOTIFY> m_vector_event_area_group_queuing;
	std::vector<EVENT_AREA_GROUP_NOTIFY> m_vector_event_area_group_processing;

	int					list_add_area_group(Igroup_user* _parea_group);
	void				list_remove_area_group(Igroup_user* _parea_group);
	void				list_update_area_group(Igroup_user* _parea_group);
	void				list_update_area_group(int _index);
	void				list_update_event_area_group();

};
