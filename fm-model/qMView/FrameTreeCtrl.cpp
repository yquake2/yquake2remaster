// FrameTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"
#include "Matrix.h"
#include "Model.h"
#include "FrameTreeCtrl.h"
#include "SkinTreeCtrl.h"
#include "ManagerTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFrameTreeCtrl

CFrameTreeCtrl::CFrameTreeCtrl()
{
	m_rootFrame = NULL;
}

CFrameTreeCtrl::~CFrameTreeCtrl()
{
}

void CFrameTreeCtrl::DeleteContents()
{
	DeleteAllItems();
	m_rootFrame = InsertItem("Animations", TVI_ROOT, TVI_FIRST);
	SetItemImage(m_rootFrame, 0, 0);
}

CModel* CFrameTreeCtrl::GetModel()
{
	return ((CManagerTree*)GetParent())->GetModel();
}

void CFrameTreeCtrl::SetModel(CModel* model)
{
	DeleteContents();
	if (model != NULL)
	{
		model->LoadFrameInfo(this, m_rootFrame);
	}
}

BEGIN_MESSAGE_MAP(CFrameTreeCtrl, CTreeCtrlEx)
	//{{AFX_MSG_MAP(CFrameTreeCtrl)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFrameTreeCtrl message handlers

void CFrameTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CTreeCtrlEx::OnLButtonDown(nFlags, point);

	CModel* model = GetModel();
	if (model == NULL)
	{
		return;
	}

	int count = GetSelectedCount();

	if (count > 0)
	{
		model->SetCurGroup(-1);
		HTREEITEM thisSel = GetFirstSelectedItem();
		if (thisSel == m_rootFrame)
		{
			model->SetPunch(false);
			model->SetCurFrame(0);
			((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument()->UpdateAllViews(NULL, QM_UPDATE_MODEL, NULL);
			return;
		}

		model->SetPunch(true);

		frameStruct* frameSels = model->GetFrameSels();
		if (frameSels->frames != NULL)
		{
			delete frameSels->frames;
			frameSels->frames = NULL;
		}
		frameSels->frames = new int[model->GetNumFrames()];
		frameSels->numFrames = 0;
		frameSels->info = 0;


		while (thisSel)
		{
			if (GetParentItem(thisSel) == m_rootFrame)
			{
				int i = GetItemData(thisSel);
				model->SetCurGroup(i);
				count -= 1;
				for (int j = model->GetTreeInfo(i)->bFrame; j <= model->GetTreeInfo(i)->eFrame; j++)
				{
					frameSels->frames[frameSels->numFrames] = j;
					frameSels->numFrames++;
					count++;
				}
			}
			else
			{
				frameSels->frames[frameSels->numFrames] = GetItemData(thisSel);
				frameSels->numFrames++;
			}
			thisSel = GetNextSelectedItem(thisSel);
		}
		model->SetCurFrame(frameSels->frames[0]);
		((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument()->UpdateAllViews(NULL, QM_UPDATE_MODEL, NULL);
	}
	else
		model->SetPunch(false);
}

void CFrameTreeCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CTreeCtrlEx::PreSubclassWindow();
 	m_image.Create(IDB_BITMAP1, 13, 1, RGB(255,255,255));
	SetImageList(&(m_image), TVSIL_NORMAL);
}
