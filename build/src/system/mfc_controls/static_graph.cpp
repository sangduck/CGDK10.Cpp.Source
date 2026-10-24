#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

// CCGGraphTraffic
IMPLEMENT_DYNAMIC(CStatic_graph, CStatic)

class	CStatic_graph::CGraph
{
public:
	CGraph(int _data_length, int _scaleVertical, COLORREF _colorLine);
	~CGraph();

protected:
	CPen		m_penLine;

	std::vector<int> m_vectorData;
	int			m_indexNow;

	bool		m_bEnable;

public:
	BOOL		m_bAutoScale;
	int			m_scaleVertical;

	void		enable(bool _enable_sustain)	{ m_bEnable=_enable_sustain;}
												  
	int			GetVerticalScale() const		{ return m_scaleVertical;}
	void		SetVerticalScale(int _Scale)	{ m_scaleVertical=_Scale; m_bAutoScale=FALSE;}
												  
	void		DrawGraph(uint32_t _positionView, int _scaleHorizontal, CDC* _pDC, RECT _rRect);
	void		DrawLastSingleLine(uint32_t _positionView, int _scaleHorizontal, CDC* _pDC, RECT _rRect);
	void		AppendData(int p_iSendData);
};


CStatic_graph::CGraph::CGraph(int _data_length, int _scaleVertical, COLORREF _colorLine)
{
	m_bEnable		 = true;
	m_indexNow		 = 0;

	m_scaleVertical	 = _scaleVertical;
	m_bAutoScale	 = FALSE;

	m_vectorData.assign(static_cast<size_t>(_data_length), 0);

	m_penLine.CreatePen(PS_SOLID, 1, _colorLine);
}

CStatic_graph::CGraph::~CGraph()
{
}

void CStatic_graph::CGraph::AppendData(int p_iSendData)
{
	// 1) Data를 써넣는다.
	m_vectorData.at(m_indexNow) = p_iSendData;

	// 2) pointer를 MAX_DATASIZE에서 짜른다.
	m_indexNow	 = (m_indexNow+1)% static_cast<int>(m_vectorData.size());
}

void CStatic_graph::CGraph::DrawGraph(uint32_t _positionView, int _scaleHorizontal, CDC* _pDC, RECT _rRect)
{
	// check)
	RETURN_IF(m_bEnable==false);

	// 1) 그리는데 필요한 값을 얻는다.
	int		sizeView	 = _rRect.right-_rRect.left;
	size_t	dataStart	 = (static_cast<size_t>(_positionView)%m_vectorData.size())- static_cast<size_t>(sizeView)*_scaleHorizontal;

	dataStart			 = (dataStart>=0) ? dataStart : dataStart+m_vectorData.size();

	// 2) send traffic Line을 그린다.
	_pDC->SelectObject(&m_penLine);

	// 3) Line을 그린다.
	_pDC->MoveTo(_rRect.left, _rRect.bottom);
	for(int x=0; x<sizeView; x++, dataStart+=_scaleHorizontal)
	{
		int	value	 = m_vectorData.at(dataStart%m_vectorData.size());
		int	y		 = _rRect.bottom-value/m_scaleVertical-1;

		SATURATE_MIN(y, _rRect.top);
		SATURATE_MAX(y, _rRect.bottom-1);

		_pDC->LineTo(_rRect.left++, y);
	}
}

void CStatic_graph::CGraph::DrawLastSingleLine(uint32_t _positionView, int _scaleHorizontal, CDC* _pDC, RECT _rRect)
{
	// check)
	RETURN_IF(m_bEnable==false);

	// 1) 그리는데 필요한 값을 얻는다.
	size_t	dataStart	 = (static_cast<size_t>(_positionView) - static_cast<size_t>(_scaleHorizontal)*2)%m_vectorData.size();

	dataStart			 = (dataStart>=0) ? dataStart : dataStart+m_vectorData.size();

	// 2) send traffic Line을 그린다.
	_pDC->SelectObject(&m_penLine);


	// 3) start점을 설정한다.
	int	value	 = m_vectorData.at(dataStart++);
	int	y		 = _rRect.bottom-value/m_scaleVertical-1;

	SATURATE_MIN(y, _rRect.top);
	SATURATE_MAX(y, _rRect.bottom-1);

	_pDC->MoveTo(_rRect.right-2, y);

	// 4) Line을 그린다.
	value	 = m_vectorData.at(dataStart%m_vectorData.size());
	y		 = _rRect.bottom-value/m_scaleVertical-1;

	SATURATE_MIN(y, _rRect.top);
	SATURATE_MAX(y, _rRect.bottom-1);

	_pDC->LineTo(_rRect.right-1, y);
}

CStatic_graph::CStatic_graph(int _data_length, int _GridUnit) noexcept
{
	m_scaleHorizontal	 = 1;
	m_positionView		 = 0;

	m_iGridUnit			 = _GridUnit;
	m_iDataSize			 = _data_length;
}

CStatic_graph::~CStatic_graph() noexcept
{
	for(auto& iter:m_vectorGraph)
	{
		delete	iter;
	}
}

int CStatic_graph::GetVerticalScale(int _idxGraph) const
{
	return m_vectorGraph.at(_idxGraph)->GetVerticalScale();
}

void CStatic_graph::SetVerticalScale(int _Scale)
{
	for(auto& iter:m_vectorGraph)
	{
		iter->SetVerticalScale(_Scale);
	}

	InvalidateRect(nullptr);
}

void CStatic_graph::SetVerticalScale(int _idxGraph, int _Scale)
{
	//-----------------------------------------------------------------
	// check) 먼위 밖이면 끝낸다.
	//-----------------------------------------------------------------
	RETURN_IF(static_cast<int>(m_vectorGraph.size()) >= _idxGraph);


	//-----------------------------------------------------------------
	// 1. 지운다.
	//-----------------------------------------------------------------
	m_vectorGraph.at(_idxGraph)->SetVerticalScale(_Scale);
}


int CStatic_graph::AddGraph(int _scaleVertical, COLORREF _colorLine)
{
	//-----------------------------------------------------------------
	// 1. 새로운 Graph를 추가한다.
	//-----------------------------------------------------------------
	// 1) Graph를 생성한다.
	CGraph*	pGraph = new CGraph(m_iDataSize, _scaleVertical, _colorLine);

	// 2) Graph를 넣는다.
	m_vectorGraph.push_back(pGraph);

	// return) 새로 추가한 Graph의 Index를 Return한다.
	return	static_cast<int>(m_vectorGraph.size())-1;
}

void CStatic_graph::RemoveGraph(int _idxGraph)
{
	//-----------------------------------------------------------------
	// check) 먼위 밖이면 끝낸다.
	//-----------------------------------------------------------------
	RETURN_IF(static_cast<int>(m_vectorGraph.size()) >= _idxGraph);


	//-----------------------------------------------------------------
	// 1. 지운다.
	//-----------------------------------------------------------------
	// 1) Graph를 임시로 얻어 놓는다.
	CGraph* pGraph = m_vectorGraph.at(_idxGraph);

	// 2) 지운다.
	delete	pGraph;

	// 3) vector에서 제거한다.
	m_vectorGraph.erase(m_vectorGraph.begin()+_idxGraph);
}

void CStatic_graph::EnableGraph(int _idxGraph, bool _enable_sustain)
{
	RETURN_IF(_idxGraph <0 || _idxGraph>=static_cast<int>(m_vectorGraph.size()));

	m_vectorGraph.at(_idxGraph)->enable(_enable_sustain);
}

void CStatic_graph::AppendData(int _idxGraph, int _data)
{
	RETURN_IF(_idxGraph>=static_cast<int>(m_vectorGraph.size()));

	m_vectorGraph.at(_idxGraph)->AppendData(_data);
}

void CStatic_graph::Scroll(int p_iScroll)
{
	//-----------------------------------------------------------------
	// 1. Scroll한다.
	//-----------------------------------------------------------------
	m_positionView+=p_iScroll;

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 2. 배경을 그릴 준비한다.
	//-----------------------------------------------------------------
	// 1) DC를 얻는다.
	CClientDC dc(this);

	// check) dc가 nullptr이면 그냥 끝낸다.
	RETURN_IF(dc == nullptr);

	// 2) Client 영역을 얻는다.
	CRect rect;
	GetClientRect(&rect);


	//-----------------------------------------------------------------
	// 3. 복사해서 당긴다.
	//-----------------------------------------------------------------
	// 1) 복사해 옮긴다.
	dc.BitBlt(0, 0, rect.right-1, rect.bottom, &dc, 1, 0, SRCCOPY);


	//-----------------------------------------------------------------
	// 4. 마지막 라인을 그린다.
	//-----------------------------------------------------------------
	if((m_positionView + rect.right - 1) % m_iGridUnit != 0)
	{
		// - 제일 마지막 Line만 그려넣는다.
		rect.left	 = rect.right-1;
		dc.FillRect(&rect, &m_brushBackground);

		// - Grid용 Pen을 설정한다.
		dc.SelectObject(&m_penGrid);

		// - 수평선을 그린다.
		for(int y=rect.bottom-1; y>=rect.top; y-=m_iGridUnit)
		{
			dc.SetPixel(rect.right-1, y, m_colorGrid);
		}
	}
	else
	{
		// - Grid용 Pen을 설정한다.
		//if (x % 5)
			dc.SelectObject(&m_penGrid);
		//else
		//	dc.SelectObject(&m_colorGrid_empha);

		// - Line하나만 그리면 끝난다.
		int	x = rect.right-1;

		dc.MoveTo(x, rect.top);
		dc.LineTo(x, rect.bottom);
	}


	//-----------------------------------------------------------------
	// 5. Graph를 그린다.
	//-----------------------------------------------------------------
	for(auto& iter:m_vectorGraph)
	{
		iter->DrawLastSingleLine(m_positionView, m_scaleHorizontal, &dc, rect);
	}
}




BEGIN_MESSAGE_MAP(CStatic_graph, CStatic)
	ON_WM_PAINT()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CStatic_graph message handlers
void CStatic_graph::OnPaint()
{
	//-----------------------------------------------------------------
	// 1. 준비하기
	//-----------------------------------------------------------------
	// 1) Paint DC를 얻는다.(device context for painting)
	CPaintDC dc(this);

	// 2) Client 영역을 얻는다.
	RECT	tempRect;
	GetClientRect(&tempRect);


	// check) 
	if(m_brushBackground.GetSafeHandle() == NULL)
	{
		m_colorGrid = RGB(0x00, 0x20, 0x00);
		m_colorGrid_empha = RGB(0x00, 0x50, 0x00);

		m_brushBackground.CreateStockObject(BLACK_BRUSH);
		m_penGrid.CreatePen(PS_SOLID, 1, m_colorGrid);

		m_brushBackground_Disabled.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
		m_penGrid_Disabled.CreatePen(PS_SOLID, 1, RGB(0xd0, 0xd0, 0xd0));
	}


	//-----------------------------------------------------------------
	// 2. Background를 지운다.
	//-----------------------------------------------------------------
	// check) Disable된 상태라면 그냥 배경만 지우고 끝낸다.
	if(IsWindowEnabled()==FALSE)
	{
		dc.FillRect(&tempRect, &m_brushBackground_Disabled);
		return;
	}

	// 1) 배경을 지운다.
	dc.FillRect(&tempRect, &m_brushBackground);


	//-----------------------------------------------------------------
	// 3. Grid를 그린다.
	//-----------------------------------------------------------------
	// 1) Grid용 Pen을 설정한다.
	dc.SelectObject(&m_penGrid);

	// 2) 수평선을 그린다.
	for(int y=tempRect.bottom-1; y>=tempRect.top; y-=m_iGridUnit)
	{
		dc.MoveTo(0, y);
		dc.LineTo(tempRect.right, y);
	}

	// 3) 수직선을 그린다.
	for(int x = (m_iGridUnit-(m_positionView % m_iGridUnit)); x < tempRect.right; x += m_iGridUnit)
	{
		//if (x % 5)
			dc.SelectObject(&m_penGrid);
		//else
		//	dc.SelectObject(&m_colorGrid_empha);

		dc.MoveTo(x, tempRect.top);
		dc.LineTo(x, tempRect.bottom);
	}


	//-----------------------------------------------------------------
	// 4. Graph를 그린다.
	//-----------------------------------------------------------------
	// 1) Graph Line를 그린다.
	for(auto& iter:m_vectorGraph)
	{
		iter->DrawGraph(m_positionView, m_scaleHorizontal, &dc, tempRect);
	}
}

void CStatic_graph::OnDestroy()
{
	// 1) 부모 Class의 destroy
	__super::OnDestroy();
}


}