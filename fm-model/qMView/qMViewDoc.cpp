// qMViewDoc.cpp : implementation of the CQMViewDoc class
//

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"
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

#include "SkinPageFrm.h"
#include "NodePropertyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQMViewDoc

IMPLEMENT_DYNCREATE(CQMViewDoc, CDocument)

BEGIN_MESSAGE_MAP(CQMViewDoc, CDocument)
	ON_UPDATE_COMMAND_UI_RANGE(IDR_SKIN_FROM_FILE_START, IDR_SKIN_FROM_FILE_END, OnUpdateSkinFromFile)
	ON_COMMAND_RANGE(IDR_SKIN_FROM_FILE_START, IDR_SKIN_FROM_FILE_END, OnSkinFromFile)
	ON_UPDATE_COMMAND_UI_RANGE(IDR_SKIN_SELECT_START, IDR_SKIN_SELECT_END, OnUpdateSkinSelect)
	ON_COMMAND_RANGE(IDR_SKIN_SELECT_START, IDR_SKIN_SELECT_END, OnSkinSelect)
	//{{AFX_MSG_MAP(CQMViewDoc)
	ON_COMMAND(ID_NODE_PROP, OnNodeProp)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPS, OnUpdateViewProps)
	ON_COMMAND(ID_VIEW_PROPS, OnViewProps)
	ON_COMMAND(ID_REPLACE_SKIN, OnReplaceSkin)
	ON_UPDATE_COMMAND_UI(ID_REPLACE_SKIN, OnUpdateReplaceSkin)
	ON_COMMAND(ID_SKIN_IMPORT, OnSkinImport)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQMViewDoc construction/destruction

CQMViewDoc::CQMViewDoc()
{
	// TODO: add one-time construction code here

	m_spec_dwFps = 0;
	m_spec_dwFramesRendered = 0;
	m_spec_dwCurTime = 0;
	m_spec_dwDeltaTime = 0;
	m_spec_dwLastTime = 0;
	m_spec_dwFpsTime = 0;
	m_model = NULL;
}

CQMViewDoc::~CQMViewDoc()
{
}

BOOL CQMViewDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewDoc serialization

void CQMViewDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		CString filename = ar.GetFile()->GetFilePath();
		CString szExt = filename.Right(filename.GetLength() - 1 - filename.ReverseFind('.'));
		if (m_model != NULL)
		{
			UpdateAllViews(NULL, QM_DELETE_MODEL, (CObject*)m_model);
		}
		m_model = CModel::Create(szExt);
		m_model->SetFilename(filename);
		m_model->Serialize(ar);
		UpdateAllViews(NULL, QM_NEW_MODEL, (CObject*)m_model);
		m_spec_dwFps = 0;
		m_spec_dwFramesRendered = 0;
		m_spec_dwCurTime = 0;
		m_spec_dwDeltaTime = 0;
		m_spec_dwLastTime = 0;
		m_spec_dwFpsTime = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewDoc diagnostics

#ifdef _DEBUG
void CQMViewDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CQMViewDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CQMViewDoc commands

LPDIRECT3DRM2 CQMViewDoc::GetD3D()
{
	CMainFrame* mainframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	return mainframe->GetD3D();
}

CModel* CQMViewDoc::GetModel()
{
	return m_model;
}

void CQMViewDoc::DeleteContents()
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_model != NULL)
	{
		UpdateAllViews(NULL, QM_DELETE_MODEL, (CObject*)m_model);
		m_model->Delete();
		m_model = NULL;
	}

	m_spec_dwFps = 0;
	m_spec_dwFramesRendered = 0;
	m_spec_dwCurTime = 0;
	m_spec_dwDeltaTime = 0;
	m_spec_dwLastTime = 0;
	m_spec_dwFpsTime = 0;
	CDocument::DeleteContents();
}

/*
void CQMViewDoc::OnIdle()
{
	LPSTR szText = "xxFPS: xx/20xx";

//	if (m_active)
	{
		m_spec_dwCurTime   = timeGetTime();
	    m_spec_dwDeltaTime = m_spec_dwCurTime - m_spec_dwLastTime;
		m_spec_dwLastTime  = m_spec_dwCurTime;
		m_spec_dwFpsTime  += m_spec_dwDeltaTime;

        if (m_spec_dwFpsTime > 250)
        {
            m_spec_dwFps             = m_spec_dwFramesRendered * 4;
            m_spec_dwFramesRendered  = 0;
            m_spec_dwFpsTime         = 0;

			sprintf(szText, "FPS: %2d/20\n", m_spec_dwFps);
//			m_wndStatusBar.SetPaneText(1, szText, 1);
		}

//		UpdateAllViews(NULL, QM_IDLE, (CObject*)m_model);
		m_spec_dwFramesRendered++;
	}

}
*/

void CQMViewDoc::OnNodeProp()
{
	// TODO: Add your command handler code here
	if (m_model == NULL)
	{
		return;
	}
	CNodePropertyDlg	nodePropDlg;
	nodePropDlg.m_tris.Format("&d", m_model->GetNumTris());
	int ret = nodePropDlg.DoModal();
}

void CQMViewDoc::OnCloseDocument()
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_model != NULL)
	{
		UpdateAllViews(NULL, QM_DELETE_MODEL, (CObject*)m_model);
		m_model->Delete();
		m_model = NULL;
	}
	CDocument::OnCloseDocument();
}

void CQMViewDoc::OnUpdateViewProps(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_model != NULL);
}

void CQMViewDoc::OnViewProps()
{
	// TODO: Add your command handler code here
	if (m_model == NULL)
	{
		return;
	}
	CPropertySheet* propSheet = new CPropertySheet(m_model->GetFilename());
	m_model->AddPropPages(propSheet);
	if (propSheet->DoModal() == IDOK)
	{
		m_model->UpdateFromPropPages(propSheet);
	}
	m_model->RemovePropPages(propSheet);
	delete propSheet;
}

void CQMViewDoc::OnUpdateSkinFromFile(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(true);
}

void CQMViewDoc::OnSkinFromFile(UINT nID)
{
	// TODO: Add your command handler code here
	CMainFrame* mainframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CMenu* popup = mainframe->GetSkinMenu();
	CString filename;
	popup->GetMenuString(nID, filename, MF_BYCOMMAND);
	ReplaceSkin(filename);
}

void CQMViewDoc::OnUpdateReplaceSkin(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(true);
}

void CQMViewDoc::OnReplaceSkin()
{
	// TODO: Add your command handler code here
	CString extensions;
	CString formats;
	extensions.LoadString(ID_FILE_EXTENSIONS);
	formats.LoadString(ID_SUPPORTED_FORMATS);
	CFileDialog fdOpenFile(TRUE,extensions,NULL,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, formats, NULL);

	if (fdOpenFile.DoModal()==IDOK)
	{
		ReplaceSkin(fdOpenFile.GetPathName());
	}
}

void CQMViewDoc::ReplaceSkin(LPCTSTR skinname)
{
	if (m_model == NULL)
	{
		return;
	}
	CMainFrame* mainframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CTreeCtrl* tree = mainframe->GetSkinTreeCtrl();
	if (tree == NULL)
	{
		return;
	}
	HTREEITEM item = tree->GetSelectedItem();
	if (item == NULL)
	{
		return;
	}
	if (tree->GetParentItem(item) == TVI_ROOT)
	{
		return;
	}
	tree->SetItemText(item, skinname);

	int skinnum = tree->GetItemData(item);
	m_model->ReplaceSkin(skinnum, skinname);
	UpdateAllViews(NULL, QM_NEW_SKIN, (CObject*)skinnum);
}


void CQMViewDoc::OnSkinImport()
{
	// TODO: Add your command handler code here
	if (m_model == NULL)
	{
		return;
	}
	CString extensions;
	CString formats;
	extensions.LoadString(ID_FILE_EXTENSIONS);
	formats.LoadString(ID_SUPPORTED_FORMATS);
	CFileDialog fdOpenFile(TRUE,extensions,NULL,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, formats, NULL);

	if (fdOpenFile.DoModal()==IDOK)
	{
		m_model->AddSkin(fdOpenFile.GetPathName());
		CMainFrame* mainframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
		CSkinTreeCtrl* tree = (CSkinTreeCtrl*)mainframe->GetSkinTreeCtrl();
		tree->SetModel(m_model);
		ReplaceSkin(fdOpenFile.GetPathName());
	}
}

void CQMViewDoc::OnUpdateSkinSelect(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(true);
}

void CQMViewDoc::OnSkinSelect(UINT nID)
{
	// TODO: Add your command handler code here
	if (m_model == NULL)
	{
		return;
	}
	int skinnum = nID - IDR_SKIN_SELECT_START;
	UpdateAllViews(NULL, QM_CHANGE_SKIN, (CObject*)skinnum);
}

void CQMViewDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU);
	SetTitle(lpszPathName);
}
