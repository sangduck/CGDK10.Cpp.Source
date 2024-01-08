#include "pch.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_CGDK_component_info, CDialogEx)

CDialog_CGDK_component_info::CDialog_CGDK_component_info(CWnd* pnode_parent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_CGDK_VERSION_INFO, pnode_parent),
	m_is_enable_control(TRUE),
	m_pvector_components(nullptr)
{
}

CDialog_CGDK_component_info::CDialog_CGDK_component_info(const std::vector<CGDK::sCOMPONENT_LICENSE>& _vector_components, CWnd* pnode_parent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_CGDK_VERSION_INFO, pnode_parent),
	m_is_enable_control(TRUE),
	m_pvector_components(&_vector_components)
{
}

CDialog_CGDK_component_info::~CDialog_CGDK_component_info()
{
}

void CDialog_CGDK_component_info::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CGDK_COMPONENTS, this->m_listctrl_components);
	DDX_Control(pDX, IDC_EXPLANATION_1, this->m_rich_explanation_1);
	DDX_Control(pDX, IDC_EXPLANATION_2, this->m_rich_explanation_2);
	DDX_Control(pDX, IDC_EXPLANATION_3, this->m_rich_explanation_3);
}

BEGIN_MESSAGE_MAP(CDialog_CGDK_component_info, CDialogEx)
END_MESSAGE_MAP()


// CDialog_CGDK_component_info 메시지 처리기입니다.
BOOL CDialog_CGDK_component_info::CreateModaless(const std::vector<CGDK::sCOMPONENT_LICENSE>& _vector_components, CWnd* _pparent)
{
	// 1) Manager를 설정한다.
	m_pvector_components = &_vector_components;

	// 2) 생성한다.
	return Create(IDD_DIALOG_CGDK_VERSION_INFO, _pparent);
}

void CDialog_CGDK_component_info::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd() == nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	//GetDlgItem(IDC_BUTTON_POOL_SHRINK)->EnableWindow(is_enable & _is_enable);
	//GetDlgItem(IDC_BUTTON_POOL_ENABLE_SUSTAIN)->EnableWindow(is_enable & _is_enable);
	//GetDlgItem(IDC_BUTTON_POOL_DISABLE_SUSTAIN)->EnableWindow(is_enable & _is_enable);
	//GetDlgItem(IDC_CHECK_ENABLE_AUTO_SUSTAIN)->EnableWindow(is_enable & _is_enable);
	this->m_listctrl_components.EnableWindow(is_enable & _is_enable);
}

void InsertText(CRichEditCtrl* _pEdit, CString text, COLORREF color, bool bold, bool italic)
{
	CHARFORMAT cf = {};
	int txtLen = _pEdit->GetTextLength();

	cf.cbSize = sizeof(cf);
	cf.dwMask = (bold ? CFM_BOLD : 0) | (italic ? CFM_ITALIC : 0) | CFM_COLOR;
	cf.dwEffects = (bold ? CFE_BOLD : 0) | (italic ? CFE_ITALIC : 0) |~CFE_AUTOCOLOR;
	cf.crTextColor = color;

	_pEdit->SetSel(txtLen, -1);
	_pEdit->ReplaceSel(text);
	_pEdit->SetSel(txtLen, _pEdit->GetTextLength());
	_pEdit->SetSelectionCharFormat(cf);
}
BOOL CDialog_CGDK_component_info::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 1) 
	this->m_listctrl_components.InsertColumn(0, TEXT("Component name"),	LVCFMT_LEFT,  170, 0);
	this->m_listctrl_components.InsertColumn(1, TEXT("version"),		LVCFMT_LEFT,  100, 1);
	this->m_listctrl_components.InsertColumn(2, TEXT("build Time"),		LVCFMT_LEFT,  120, 2);
	this->m_listctrl_components.InsertColumn(3, TEXT("Config"),			LVCFMT_RIGHT,  70, 3);
	this->m_listctrl_components.InsertColumn(4, TEXT("compiler"),		LVCFMT_RIGHT, 110, 4);
	this->m_listctrl_components.InsertColumn(5, TEXT("platform"),		LVCFMT_RIGHT,  80, 5);
	this->m_listctrl_components.InsertColumn(6, TEXT("Expire date"),	LVCFMT_LEFT,  100, 6);
	this->m_listctrl_components.InsertColumn(7, TEXT("Licence Info"),	LVCFMT_LEFT,  140, 7);
	this->m_listctrl_components.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) 
	if(m_pvector_components != nullptr)
	{
		// declare) 
		TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
		LV_ITEM list_item;

		list_item.mask = LVIF_TEXT;
		list_item.state = 0;
		list_item.cchTextMax = 256;

		for(auto& iter: *m_pvector_components)
		{
			// - 
			list_item.iItem = this->m_listctrl_components.GetItemCount();

			// - Component name
			list_item.iSubItem = 0;
		#ifdef _UNICODE
			list_item.pszText = (TCHAR*)encode_string(text, iter.component_info.name.c_str(), MAX_STRING_LENGTH).data();
		#else
			list_item.pszText = const_cast<TCHAR*>(iter.name.c_str());
		#endif
			this->m_listctrl_components.InsertItem(&list_item);

			// - version
			list_item.iSubItem = 1;
			list_item.pszText = text;
			wsprintf(text, TEXT("%d.%d.%d"), iter.component_info.version_package, iter.component_info.version_sub, iter.component_info.version_build);
			this->m_listctrl_components.SetItem(&list_item);

			// - build Time
			auto strBuildTime = to_string<TCHAR>(iter.component_info.time_build);
			list_item.iSubItem = 2;
			list_item.pszText = const_cast<TCHAR*>(strBuildTime.data());
			this->m_listctrl_components.SetItem(&list_item);

			// - configuration
			list_item.iSubItem = 3;
			list_item.pszText = const_cast<TCHAR*>(to_string<TCHAR>(iter.component_info.configuration).data());
			this->m_listctrl_components.SetItem(&list_item);

			// - compiler
			list_item.iSubItem = 4;
			list_item.pszText = const_cast<TCHAR*>(to_string<TCHAR>(iter.component_info.compiler).data());
			this->m_listctrl_components.SetItem(&list_item);

			// - platform
			list_item.iSubItem = 5;
			list_item.pszText = const_cast<TCHAR*>(to_string<TCHAR>(iter.component_info.platform).data());
			this->m_listctrl_components.SetItem(&list_item);

			// - Expire time
			list_item.iSubItem = 6;
			if(iter.component_info.time_expire != chrono::time::utc::time_point::min())
			{
				auto time_temp = chrono::to_date_time(iter.component_info.time_expire);

				wsprintf(text, TEXT("%u/%02u/%02u"), time_temp.t_year, time_temp.t_month, time_temp.t_day);
				list_item.pszText = text;
			}
			else
			{
				list_item.pszText = const_cast<LPWSTR>(TEXT("unlimited"));
			}
			this->m_listctrl_components.SetItem(&list_item);

			// - License
			list_item.iSubItem = 7;
			switch(iter.license_type)
			{
			case	CGDK::eLICENSE_TYPE::FREE:
					wsprintf(text, TEXT("free"));
					break;

			case	CGDK::eLICENSE_TYPE::INDIE:
					wsprintf(text, TEXT("indie licence"));
					break;

			case	CGDK::eLICENSE_TYPE::LIMITED_PROJECT:
				#ifdef _USE_CHAR_STRING
					wsprintf(text, TEXT("limiited [pj: %hs]"), iter.license_project.c_str());
				#elif _USE_WCHAR_STRING
					wsprintf(text, TEXT("limiited [pj: %ls]"), iter.license_project.c_str());
				#else
					#error
				#endif
					break;

			case	CGDK::eLICENSE_TYPE::LIMITED_COMPPANY:
				#ifdef _USE_CHAR_STRING
					wsprintf(text, TEXT("limiited [co: %hs]"), iter.license_company.c_str());
				#elif _USE_WCHAR_STRING
					wsprintf(text, TEXT("limiited [co: %ls]"), iter.license_company.c_str());
				#else
					#error
				#endif
					break;

			case	CGDK::eLICENSE_TYPE::UNLIMITED:
					wsprintf(text, TEXT("unlimited"));
					break;

			case	CGDK::eLICENSE_TYPE::DEVELOPER:
					wsprintf(text, TEXT("developer edition"));
					break;
			}
			//chrono::time::utc::time_point	license_time_expire;
			list_item.pszText = text;
			this->m_listctrl_components.SetItem(&list_item);
		}

		free(text);
	}

	// return) 
	return TRUE;
}


}