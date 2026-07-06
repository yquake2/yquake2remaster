// NodeTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"

#include "Matrix.h"
#include "Model.h"
#include "resource.h"
#include "NodeTreeCtrl.h"
#include "JointTreeCtrl.h"
#include "FrameManager2.h"

#include "NodePropertyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodeTreeCtrl

CNodeTreeCtrl::CNodeTreeCtrl()
{
	m_rootNode = NULL;
}

CNodeTreeCtrl::~CNodeTreeCtrl()
{
}

void CNodeTreeCtrl::DeleteContents()
{
	DeleteAllItems();
	m_rootNode = InsertItem("Nodes", TVI_ROOT, TVI_FIRST);
	SetItemImage(m_rootNode, 0, 0);
}

CModel* CNodeTreeCtrl::GetModel()
{
	return ((FrameManager2*)GetParent())->GetModel();
}

void CNodeTreeCtrl::SetModel(CModel* model)
{
	DeleteContents();
	if (model != NULL)
	{
		model->AddNodes(this, m_rootNode);
	}
}

BEGIN_MESSAGE_MAP(CNodeTreeCtrl, CTreeCtrl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CNodeTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(ID_NODE_TOGGLE_VIS, OnNodeToggleVis)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodeTreeCtrl message handlers

void CNodeTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW *pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	int			nodeNum = 0;
	HTREEITEM	item, parent;

	CModel* model = GetModel();
	if (model == NULL)
	{
		return;
	}
	if (model->GetMesh(0) == NULL)
		return;

	item = GetSelectedItem();
	parent = GetParentItem(item);

	if (item==m_rootNode)
	{
		model->DeSelectAll();
		*pResult = 0;
		return;
	}

	CString cs = GetItemText(item);
	LPSTR foo = cs.GetBuffer(10);

	sscanf(foo, "Node %d", &nodeNum);

	((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument()->UpdateAllViews(NULL, QM_CHANGE_VISUAL, (CObject*)nodeNum);

	model->SelectNode(nodeNum);

	*pResult = 0;
}

void CNodeTreeCtrl::OnContextMenu(CWnd*, CPoint point)
{
	{
		if (point.x == -1 && point.y == -1){
			//keystroke invocation
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			point = rect.TopLeft();
			point.Offset(5, 5);
		}

		CMenu menu;
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_NODE_TREE_CTRL));

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;

		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();

		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}

void CNodeTreeCtrl::OnNodeToggleVis()
{
	HTREEITEM	item, parent;
	int			nodeNum = 0;

	CModel* model = GetModel();
	if (model == NULL)
	{
		return;
	}
	item = GetSelectedItem();
	parent = GetParentItem(item);

	if (item==m_rootNode)
		return;

	CString cs = GetItemText(item);
	LPSTR foo = cs.GetBuffer(10);

	sscanf(foo, "Node %d", &nodeNum);

	for (int i = 0; i < model->GetNumNodes(); i++)
	{
		model->GetMesh(i)->SetGroupColorRGB(	0,	D3DVALUE(255),
											D3DVALUE(255),
											D3DVALUE(255) );
	}

	model->GetMesh(nodeNum)->SetGroupColorRGB( 0,	D3DVALUE(0),
										  		D3DVALUE(0),
										 		D3DVALUE(0) );
}

void CNodeTreeCtrl::OnRclick(NMHDR* pNMHDR, LRESULT* pResult)
{
	POINT point;

	GetCursorPos(&point);
	UINT flags;
	POINT hitpoint = point;
	ScreenToClient(&hitpoint);
	HTREEITEM item = HitTest(hitpoint, &flags);
	SelectItem(item);
	if ((item == m_rootNode) || (item == NULL))
	{
		return;
	}
	CModel* model = GetModel();
	if (model != NULL)
	{
		CMenu menu;
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_NODE_TREE_CTRL));

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		model->FillMenuWithSkins(pPopup);
		CWnd* pWndPopupOwner = this;

		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();


		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
	*pResult = 0;
}

void CNodeTreeCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CTreeCtrl::PreSubclassWindow();
	m_image.Create(IDB_BITMAP3, 13, 2, RGB(255,255,255));
    SetImageList(&(m_image), TVSIL_NORMAL);
}
