#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CComboBox_connective, CComboBox)

CComboBox_connective::CComboBox_connective()
{
}

CComboBox_connective::~CComboBox_connective()
{
}


BEGIN_MESSAGE_MAP(CComboBox_connective, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, &CComboBox_connective::OnCbnSelchange)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CComboBox_connective::OnCGMessageNotify)
END_MESSAGE_MAP()

void CComboBox_connective::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	// 1) 부모 클래스의 Activate!!!
	__super::OnActivate(nState, pWndOther, bMinimized);
	
	// 2) Updae한다.
	UpdateConnectiveList();
}

// CComboBox_connective message handlers
void CComboBox_connective::SetConnectiveManager(net::io::connective::manager* _pconnective_manager)
{
	scoped_lock(m_csmangerConnective)
	{
		// check) 같은 것이면 종료한다.
		RETURN_IF(m_pmangerConnective==_pconnective_manager);

		m_pmangerConnective	 = _pconnective_manager;
	}

	// 1) setting한다.
	UpdateConnectiveList();
}

object_ptr<net::io::connective::manager> CComboBox_connective::get_connective_manager()
{
	scoped_lock(m_csmangerConnective)
	{
		// return) 
		return	m_pmangerConnective;
	}
}

void CComboBox_connective::UpdateConnectiveList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)


	//-----------------------------------------------------------------
	// 1. Socket을 얻는다.
	//    - Multi-Thread환경이므로 먼저 pConnector의 포인터를 받은 다음
	//      처리한다.
	//-----------------------------------------------------------------
	// 1) Connective Manager를 얻는다.
	object_ptr<net::io::connective::manager> pmanger_connective = get_connective_manager();

	// 2) 먼저 ComboBox를 clear한다.
	Clear();

	// check) Connective가 없으면 끝낸다.
	RETURN_IF(pmanger_connective.empty());


	//-----------------------------------------------------------------
	// 2. xxx
	//-----------------------------------------------------------------
	scoped_lock(pmanger_connective->get_lockable())
	{
		auto iter = pmanger_connective->begin();
		auto iter_end = pmanger_connective->end();

		for(; iter!= iter_end; ++iter)
		{
			// - Nameable을 얻는다.
			auto pnameable = dynamic_cast<object::Inameable*>(*iter);

			// declare) 
			int	index = 0;

			// - Connective를 추가한다.
			if(pnameable != nullptr)
			{
			#ifdef _UNICODE
					TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
					encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH);
					index = AddString(text);
					free(text);
			#else
					index = AddString(pnameable->name().data());
			#endif
			}
			else
			{
				index = AddString(TEXT("NoName"));
			}

			// - Item Data를 설정한다.
			SetItemData(index, (DWORD_PTR)*iter);
		}
	}
}

void CComboBox_connective::OnDestroy()
{
	// 1) 파괴한다~~
	scoped_lock(m_csmangerConnective)
	{
		m_pmangerConnective.reset();
	}

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CComboBox_connective::OnCbnSelchange()
{
	// declare) 
	object_ptr<net::io::Iconnective> pconnective;

	scoped_lock(m_csmangerConnective)
	{
		// check) 
		RETURN_IF(m_pmangerConnective.empty());

		// 1) 현재 선택된 Acceptor의 Index를 얻는다.
		int	selected = GetCurSel();

		// check) 
		RETURN_IF(selected < 0);

		// 2) pconnective를 얻는다.
		pconnective = (net::io::Iconnective*)GetItemData(selected);
	}

	// check) 
	RETURN_IF(pconnective.empty());

	// 3) Acceptor를 설정한다.
	SendMessageToTarget((WPARAM)CGACCEPTOR_SET_CONNECTIVE, reinterpret_cast<LPARAM>(pconnective.get()));
}

BOOL CComboBox_connective::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	// 1) 선택정보를 전송한다.
	OnCbnSelchange();

	// 2) Default
	return CComboBox::OnChildNotify(message, wParam, lParam, pLResult);
}

BOOL CComboBox_connective::create(uint32_t dwStyle, const RECT& rect, CWnd* pParentWnd, uint32_t nID)
{
	// declare) 
	BOOL	result;

	// 1) create한다...
	result	 = CComboBox::Create(dwStyle, rect, pParentWnd, nID);

	// check) 
	RETURN_IF(result==FALSE, FALSE);

	// return) 
	return	result;
}

void CComboBox_connective::AddTargetWnd(CWnd* _pwnd)
{
	scoped_lock(m_cslistWndTarget)
	{
		// 1) Target을 찾는다.
		circular_list<CWnd*>::iterator iter	 = std::find(m_listWndTarget.begin(), m_listWndTarget.end(), _pwnd);

		// check) 이미 존재한다면 그냥 끝낸다.
		RETURN_IF(iter!=m_listWndTarget.end());

		// 2) 새로운 Target을 추가한다.
		m_listWndTarget.push_back(_pwnd);
	}
}

void CComboBox_connective::RemoveTargetWnd(CWnd* _pwnd)
{
	scoped_lock(m_cslistWndTarget)
	{
		// 1) Target을 찾는다.
		auto iter	 = std::find(m_listWndTarget.begin(), m_listWndTarget.end(), _pwnd);

		// check) 해당 Target이 없으면 그냥 끝낸다.
		RETURN_IF(iter==m_listWndTarget.end());

		// 2) 해당 Target을 지운다.
		m_listWndTarget.erase(iter);
	}
}

void CComboBox_connective::SendMessageToTarget(WPARAM p_dwMessage, LPARAM _lparam)
{
	// check) 
	RETURN_IF(m_listWndTarget.empty());

	// declare) 
	std::vector<CWnd*> vectorTempTarget;

	// 1) Reserve한다.
	vectorTempTarget.reserve(m_listWndTarget.size());

	// 2) Assign
	scoped_lock(m_cslistWndTarget)
	{
		auto iter = m_listWndTarget.begin();
		auto iter_end = m_listWndTarget.end();

		for(; iter!= iter_end; ++iter)
		{
			vectorTempTarget.push_back(*iter);
		}
	}

	// 3) 전송한다.
	auto iter = vectorTempTarget.begin();
	auto iter_end = vectorTempTarget.end();

	for(; iter!= iter_end; ++iter)
	{
		(*iter)->SendMessage(eMESSAGE::WINDOWS::NOTIFY, p_dwMessage, _lparam);
	}
}

LRESULT CComboBox_connective::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				//auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);
				//set_connection_info(pconnective);
			}
			break;

	case	CGACCEPTOR_READY:
			{
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
				UpdateConnectiveList();
			}
			break;
	};

	// return) 
	return	0;
}

object_ptr<net::io::Iconnective> CComboBox_connective::GetCurrentConnective()
{
	scoped_lock(m_csmangerConnective)
	{
		// 1) 현재 선택된 Acceptor의 Index를 얻는다.
		int	selected	 = GetCurSel();

		// check) 
		RETURN_IF(selected<0, object_ptr<net::io::Iconnective>());

		// 2) 새로운 Acceptor를 얻는다.
		object_ptr<net::io::Iconnective>	pconnective		 = (net::io::Iconnective*)GetItemData(selected);

		// return) 
		return	pconnective;
	}
}


}