#include "pch.h"
#include "Dialog_amount.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialogAmount, CGDIALOG)

CDialogAmount::CDialogAmount(CWnd* pnode_parent /*=NULL*/) noexcept
	: CGDIALOG(CDialogAmount::IDD, pnode_parent)
{

}

CDialogAmount::~CDialogAmount() noexcept
{

}

void CDialogAmount::DoDataExchange(CDataExchange* pDX)
{
	CGDIALOG::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogAmount, CGDIALOG)
	ON_BN_CLICKED(IDOK, &CDialogAmount::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialogAmount::OnBnClickedCancel)
END_MESSAGE_MAP()


// CDialogAmount message handlers


void CDialogAmount::OnBnClickedOk()
{
	m_amount = GetDlgItemInt(IDC_EDIT_AMOUNT);

	CGDIALOG::OnOK();
}


void CDialogAmount::OnBnClickedCancel()
{
	CGDIALOG::OnCancel();
}


}