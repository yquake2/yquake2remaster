// SkinTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "qMView.h"
#include "Matrix.h"

#include "DDUtil.h"
#include "Model.h"

#include "qMViewDoc.h"
#include "SkinPageFrm.h"

#include "SkinTreeCtrl.h"
#include "FrameTreeCtrl.h"
#include "ManagerTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinTreeCtrl

CSkinTreeCtrl::CSkinTreeCtrl()
{
	m_rootSkin = NULL;
	m_menu = NULL;
}

CSkinTreeCtrl::~CSkinTreeCtrl()
{
	if (m_menu != NULL)
	{
		delete m_menu;
		m_menu = NULL;
	}
}

BEGIN_MESSAGE_MAP(CSkinTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CSkinTreeCtrl)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinTreeCtrl message handlers

void CSkinTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	if ((pNMTreeView->itemOld.hItem != NULL) && (pNMTreeView->itemNew.hItem == pNMTreeView->itemOld.hItem))
	{
		return;
	}
	HTREEITEM item, parent;

	CModel* model = GetModel();
	if (model == NULL)
	{
		return;
	}

	item = pNMTreeView->itemNew.hItem;
	parent =GetParentItem(item);

	if (parent == TVI_ROOT)
	{
		*pResult = 0;
		return;
	}
	CString cs,strPath;

	strPath = cs = GetItemText(item);

	int destChar = cs.ReverseFind('.');
	int strLen = cs.GetLength();

	if (destChar > -1)
	{
		cs = strPath.Right(strLen - destChar - 1);
	}

	((CFrameWnd*)GetParent()->GetParent())->GetActiveDocument()->UpdateAllViews(NULL, QM_NEW_SKIN, (CObject*)GetItemData(item));
	*pResult = 0;
}

void CSkinTreeCtrl::DeleteContents()
{
	DeleteAllItems();
	m_rootSkin = InsertItem("Skins", TVI_ROOT, TVI_FIRST);
	SetItemImage(m_rootSkin, 0, 0);
	if (m_menu != NULL)
	{
		delete m_menu;
		m_menu = NULL;
	}
}

CMenu* CSkinTreeCtrl::GetSkinMenu()
{
	if (m_menu == NULL)
	{
		return NULL;
	}
	return m_menu->GetSubMenu(0);
}

CModel* CSkinTreeCtrl::GetModel()
{
	return ((CManagerTree*)GetParent())->GetModel();
}

void CSkinTreeCtrl::SetModel(CModel* model)
{
	DeleteContents();
	if (model != NULL)
	{
		model->LoadSkinInfo(this, m_rootSkin);

		char tempStr[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, tempStr);
		if (m_menu != NULL)
		{
			delete m_menu;
			m_menu = NULL;
		}
		m_menu = new CMenu();
		m_menu->LoadMenu(IDR_SKIN_POPUP_MENU);
		CMenu* pPopup = m_menu->GetSubMenu(0);

		CString path = model->GetFilename();
		int loc = path.ReverseFind('/');
		int loc2 = path.ReverseFind('\\');
		if (loc2 > loc)
		{
			loc = loc2;
		}
		if (loc != -1)
		{
			path = path.Left(loc + 1);
		}
		SetCurrentDirectory(path);
		int curID = IDR_SKIN_FROM_FILE_START;

		CString extensions;
		extensions.LoadString(ID_FILE_EXTENSIONS);
		CFileFind findFile;
		while (extensions.GetLength() > 0)
		{
			CString thisExtension;
			loc = extensions.Find(';');
			if (loc > -1)
			{
				thisExtension = extensions.Left(loc);
				extensions = extensions.Right(extensions.GetLength() - loc - 1);
			}
			else
			{
				thisExtension = extensions;
				extensions.Empty();
			}
			BOOL process = findFile.FindFile(thisExtension, 0);
			while(process)
			{
				process = findFile.FindNextFile();
				pPopup->AppendMenu(MF_ENABLED | MF_UNCHECKED | MF_STRING, curID++, findFile.GetFilePath());
			}
		}
		SetCurrentDirectory(tempStr);

		Expand(m_rootSkin, TVE_EXPAND);
	}
}

void CSkinTreeCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CTreeCtrl::PreSubclassWindow();
	m_image.Create( IDB_BITMAP2, 13, 1, RGB(255,255,255));
    SetImageList(&(m_image), TVSIL_NORMAL);
}

void CSkinTreeCtrl::OnRclick(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	if (m_menu == NULL)
	{
		*pResult = 0;
		return;
	}

	POINT point;

	GetCursorPos(&point);
	POINT hitpoint = point;
	ScreenToClient(&hitpoint);
	UINT flags;
	HTREEITEM item = HitTest(hitpoint, &flags);
	SelectItem(item);

	{
		CMenu* pPopup = m_menu->GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;

		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();


		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
	*pResult = 0;
}
