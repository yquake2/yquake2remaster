// SkinPageFrm.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "Matrix.h"

#include "DDUtil.h"
#include "Model.h"

#include "qMView.h"
#include "qMViewDoc.h"
#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "SkinPageFrm.h"
#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "ManagerTree.h"
#include "FrameManager2.h"

#include "MainFrm.h"

#include "TickerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning(disable : 4244)		// truncation from double to float

/////////////////////////////////////////////////////////////////////////////
// CSkinPageFrm

IMPLEMENT_DYNCREATE(CSkinPageFrm, CView)

CSkinPageFrm::CSkinPageFrm()
{
	m_skinScale = 1.0;
	m_skinBitmap = NULL;
	m_vertexlist = NULL;
	m_num_tris = 0;
}

CSkinPageFrm::~CSkinPageFrm()
{
}

CQMViewDoc* CSkinPageFrm::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CQMViewDoc)));
	return (CQMViewDoc*)m_pDocument;
}

void CSkinPageFrm::SetModel(CModel* model)
{
	if (model == NULL)
	{
		return;
	}
	int skinwidth;
	int skinheight;
	CDC *pDC =  GetDC();
	m_skinBitmap = model->GetBitmap(GetDocument()->GetD3D(), pDC, 0, skinwidth, skinheight);
	ReleaseDC(pDC);
	if (m_vertexlist != NULL)
	{
		delete m_vertexlist;
		m_vertexlist = NULL;
	}
	m_num_tris = model->GetTriCount();
	long* command = model->GetCommands();

	int curvert = m_num_tris * 3;

	m_vertexlist = new D3DRMVERTEX[curvert + 1];
	if (m_vertexlist == NULL)
	{
		m_num_tris = 0;
		return;
	}

	unsigned* vertorder = new unsigned[curvert + 1];
	if (vertorder == NULL)
	{
		delete m_vertexlist;
		m_vertexlist = NULL;
		m_num_tris = 0;
		return;
	}

	vec5_t* vertlist = (vec5_t*)malloc(sizeof(vec5_t) * curvert);
	if (vertlist == NULL)
	{
		delete vertorder;
		delete m_vertexlist;
		m_vertexlist = NULL;
		m_num_tris = 0;
		return;
	}
	curvert=0;

	//do the gl commands <?>
	while (*command)
	{
		int num_verts;
		int command_type;
		bool ODD = true;
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command++;
		  command_type = 0;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command++);
		  command_type = 1;
		}

		int vert_index;
		vec5_t p1;
		for (int i = 0; i < num_verts; i++)
		{
		  //grab the floating point s and t
		  p1.s = (*((float *)command++)) * skinwidth;
		  p1.t = (*((float *)command++)) * skinheight;

		  //grab the vertex index
		  vert_index = *command++;

		  vertlist[i] = p1;
		}

		switch (command_type)
		{
			case 0:
			  //tristrip
			  for (i=0;i<num_verts-2;i++)
			  {
					if (ODD == true)
					{
						for (int j = 2; j > -1; j--)
						{
							m_vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s);
							m_vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t);
							curvert++;
						}
					}
					else
					{
						for (int j = 0; j < 3; j++)
						{
							m_vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s);
							m_vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t);
							curvert++;
						}
					}
					ODD = !ODD;
			  }
			  break;

			case 1:
			  //trifan
			  for (i=0;i<num_verts-2;i++)
			  {
					for (int j = 2; j > -1; j--)
					{
						int x;
						if (j == 0)
							x = 0;
						else
							x = i;

						m_vertexlist[curvert].tu = D3DVALUE(vertlist[x+j].s);
						m_vertexlist[curvert].tv = D3DVALUE(vertlist[x+j].t);
						curvert++;
					}
			  }
			  break;
		}
	}
    delete vertorder;
	free(vertlist);

	Invalidate(true);
}

BEGIN_MESSAGE_MAP(CSkinPageFrm, CView)
	//{{AFX_MSG_MAP(CSkinPageFrm)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_DRAWITEM()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinPageFrm message handlers

extern BOOL overlayOn;

void CSkinPageFrm::AdjustScrollers(int width, int height)
{
	SetScrollRange(SB_HORZ, 0, width, true);
	SetScrollRange(SB_VERT, 0, height, true);

	SetScrollPos(SB_HORZ, 0);
	SetScrollPos(SB_VERT, 0);
}

BOOL CSkinPageFrm::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_CHILD  | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_HSCROLL | WS_VSCROLL
		| FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;

	return CView::PreCreateWindow(cs);
}

void CSkinPageFrm::Register()
{
	LPCTSTR lpszChildClass =
			AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
			LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_MAGNIFIER)),
			0,
			LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_SKIN)));
}

extern BOOL overlayOn;

void CSkinPageFrm::DrawSTOverlay(void)
{
	CDC* pDC = GetDC();

	CPen penStroke;
	if (!penStroke.CreatePen(PS_SOLID, 1, RGB(255,255,255)))
		return;

	CPen* pOldPen = pDC->SelectObject(&penStroke);

	for (int i = 0; i < m_num_tris; i++)
	{
		pDC->MoveTo((int) m_vertexlist[i*3].tu * m_skinScale, (int) m_vertexlist[i*3].tv * m_skinScale);
		pDC->LineTo((int) m_vertexlist[i*3+1].tu * m_skinScale, (int) m_vertexlist[i*3+1].tv * m_skinScale);

		pDC->MoveTo((int) m_vertexlist[i*3+1].tu * m_skinScale, (int) m_vertexlist[i*3+1].tv * m_skinScale);
		pDC->LineTo((int) m_vertexlist[i*3+2].tu * m_skinScale, (int) m_vertexlist[i*3+2].tv * m_skinScale);

		pDC->MoveTo((int) m_vertexlist[i*3+2].tu * m_skinScale, (int) m_vertexlist[i*3+2].tv * m_skinScale);
		pDC->LineTo((int) m_vertexlist[i*3].tu * m_skinScale, (int) m_vertexlist[i*3].tv * m_skinScale);
	}

	pDC->SelectObject(pOldPen);

	return;
}

void CSkinPageFrm::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_skinScale < 4)
	{
		m_skinScale += (float) 0.2;
		Invalidate(true);
	}

	CView::OnLButtonDown(nFlags, point);
}

void CSkinPageFrm::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_skinScale > 1)
	{
		m_skinScale -= (float) 0.2;
		Invalidate(true);
	}

	CView::OnRButtonDown(nFlags, point);
}

void CSkinPageFrm::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDC *pDC = GetDC();

	pDC->OffsetViewportOrg(nPos, 0);
	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSkinPageFrm::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (pScrollBar != NULL)
	{
		pScrollBar->SetScrollPos(pScrollBar->GetScrollPos() + 10);
	}
}

void CSkinPageFrm::OnDraw(CDC* pDC)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_skinBitmap == NULL)
	{
		return;
	}
	CDC dcCompatible;
	dcCompatible.CreateCompatibleDC(pDC);

	BITMAP bmInfo;

	m_skinBitmap->GetObject(sizeof(bmInfo), &bmInfo);

	dcCompatible.SelectObject(m_skinBitmap);

	BOOL res = pDC->StretchBlt(0, 0, (int)bmInfo.bmWidth*m_skinScale, (int)bmInfo.bmHeight*m_skinScale, &dcCompatible, 0, 0, bmInfo.bmWidth, bmInfo.bmHeight, SRCCOPY);
	dcCompatible.DeleteDC();

	if (overlayOn)
		DrawSTOverlay();

	AdjustScrollers((int)bmInfo.bmWidth*m_skinScale, (int)bmInfo.bmHeight*m_skinScale);
}

void CSkinPageFrm::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	// TODO: Add your specialized code here and/or call the base class
	CModel* theModel;

	switch(lHint)
	{
	case QM_IDLE:
		break;
	case QM_NEW_SKIN:
		theModel = GetDocument()->GetModel();
		SetModel(theModel);
		if (theModel != NULL)
		{
			int skinwidth;
			int skinheight;
			m_skinBitmap = theModel->GetBitmap(GetDocument()->GetD3D(), GetDC(), (int)pHint, skinwidth, skinheight);
		}
		break;
		break;
	case QM_CHANGE_SKIN:
		theModel = GetDocument()->GetModel();
		if (theModel != NULL)
		{
			int skinwidth;
			int skinheight;
			m_skinBitmap = theModel->GetBitmap(GetDocument()->GetD3D(), GetDC(), (int)pHint, skinwidth, skinheight);
		}
		break;
	case QM_NEW_MODEL:
		theModel = (CModel*)pHint;
		SetModel(theModel);
		break;
	}
}

int CSkinPageFrm::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

void CSkinPageFrm::OnDestroy()
{
	CView::OnDestroy();

	// TODO: Add your message handler code here

	if (m_vertexlist != NULL)
	{
		delete m_vertexlist;
		m_vertexlist = NULL;
		m_num_tris = 0;
	}
}
