#pragma once


// CDialogAmount dialog
namespace CGDK
{

class CDialogAmount : public CGDIALOG
{
	DECLARE_DYNAMIC(CDialogAmount)

public:
			CDialogAmount(CWnd* pnode_parent = NULL) noexcept;   // standard constructor
	virtual ~CDialogAmount() noexcept;

// Dialog Data
	enum { IDD = IDD_DIALOG_AMOUNT };

public:
		int		m_amount;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};


}