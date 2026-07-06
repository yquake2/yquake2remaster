// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"

#include "Model.h"
#include "qMView.h"
#include "QMViewDoc.h"
#include "qMViewView.h"

#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "SkinPageFrm.h"
#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "ManagerTree.h"
#include "FrameManager2.h"
#include "MainFrm.h"

#include "NodePropertyDlg.h"
//#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_COMMAND_EX(CG_ID_VIEW_FRAMEMANAGER2, OnBarCheck)
	ON_UPDATE_COMMAND_UI(CG_ID_VIEW_FRAMEMANAGER2, OnUpdateControlBarMenu)
	ON_COMMAND_EX(CG_ID_VIEW_FRAMETREEDLG, OnBarCheck)
	ON_UPDATE_COMMAND_UI(CG_ID_VIEW_FRAMETREEDLG, OnUpdateControlBarMenu)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_ANIMATE_PLAY, OnAnimatePlay)
	ON_COMMAND(ID_ANIMATE_STOP, OnAnimateStop)
	ON_COMMAND(ID_ANIMATE_TYPE_BACK, OnAnimateTypeBack)
	ON_UPDATE_COMMAND_UI(ID_ANIMATE_TYPE_BACK, OnUpdateAnimateTypeBack)
	ON_COMMAND(ID_ANIMATE_TYPE_FRONT, OnAnimateTypeFront)
	ON_UPDATE_COMMAND_UI(ID_ANIMATE_TYPE_FRONT, OnUpdateAnimateTypeFront)
	ON_COMMAND(ID_ANIMATE_TYPE_PINGPONG, OnAnimateTypePingpong)
	ON_UPDATE_COMMAND_UI(ID_ANIMATE_TYPE_PINGPONG, OnUpdateAnimateTypePingpong)
	ON_COMMAND(ID_RENDER_FLAT, OnRenderFlat)
	ON_UPDATE_COMMAND_UI(ID_RENDER_FLAT, OnUpdateRenderFlat)
	ON_COMMAND(ID_RENDER_GOURAUD, OnRenderGouraud)
	ON_UPDATE_COMMAND_UI(ID_RENDER_GOURAUD, OnUpdateRenderGouraud)
	ON_COMMAND(ID_RENDER_WIREFRAME, OnRenderWireframe)
	ON_UPDATE_COMMAND_UI(ID_RENDER_WIREFRAME, OnUpdateRenderWireframe)
	ON_COMMAND(ID_NODE_TOGGLE_VIS, OnNodeToggleVis)
	ON_COMMAND(ID_SKIN_SHOWOVERLAY, OnSkinShowoverlay)
	ON_UPDATE_COMMAND_UI(ID_SKIN_SHOWOVERLAY, OnUpdateSkinShowoverlay)
	ON_COMMAND(ID_MODE_CAMERA, OnModeCamera)
	ON_UPDATE_COMMAND_UI(ID_MODE_CAMERA, OnUpdateModeCamera)
	ON_COMMAND(ID_MODE_SKELETAL, OnModeSkeletal)
	ON_UPDATE_COMMAND_UI(ID_MODE_SKELETAL, OnUpdateModeSkeletal)
	ON_COMMAND(ID_SKELETON_SNAP, OnSkeletonSnap)
	ON_COMMAND(ID_JOINT_MANUAL_ANGLES, OnJointManualAngles)
	ON_COMMAND(ID_USE_JOINTS, OnUseJoints)
	ON_UPDATE_COMMAND_UI(ID_USE_JOINTS, OnUpdateUseJoints)
	ON_UPDATE_COMMAND_UI(ID_JOINT_MANUAL_ANGLES, OnUpdateJointManualAngles)
	ON_UPDATE_COMMAND_UI(ID_JOINT_CONSTRAINTS, OnUpdateJointConstraints)
	ON_COMMAND(ID_JOINT_CONSTRAINTS, OnJointConstraints)
	ON_COMMAND(ID_WINDOW_SKIN, OnWindowSkin)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_SKIN, OnUpdateWindowSkin)
	ON_COMMAND(ID_WINDOW_FRAME, OnWindowFrame)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_FRAME, OnUpdateWindowFrame)
	ON_UPDATE_COMMAND_UI(ID_ANIMATE_STOP, OnUpdateAnimateStop)
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_ANIMATE_PLAY, OnUpdateAnimatePlay)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)

	ON_UPDATE_COMMAND_UI_RANGE(ID_RENDER_FLAT, ID_RENDER_GOURAUD, OnUpdateRender)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ANIMATE_PLAY, ID_ANIMATE_STOP, OnUpdateAnim)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ANIMATE_TYPE_BACK, ID_ANIMATE_TYPE_PINGPONG, OnUpdateAnimType)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_FPS,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/estruction

CMainFrame::CMainFrame()
{
	m_hItemFirstSel = NULL;
	m_jointDlg = NULL;
	m_jointConstraintDlg = NULL;
	m_wndFrameTreeDlg = NULL;
	m_wndFrameManager2 = NULL;
	m_model = NULL;
	m_currentView = ID_MODELVIEW;
	m_skinView = NULL;
	m_modelView = NULL;

	m_manipulationType = MANIPULATE_CAMERA;
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::PickNode(HTREEITEM item)
{
	m_wndFrameManager2->PickNode(item);
}

void CMainFrame::SetModel(CModel* model)
{
	m_wndFrameTreeDlg->SetModel(model);
	m_wndFrameManager2->SetModel(model);
	m_model = model;
	m_manipulationType = MANIPULATE_CAMERA;
}

LPDIRECT3DRM2 CMainFrame::GetD3D()
{
	return ((CQMViewView*)m_modelView)->GetD3D();
}

CMenu* CMainFrame::GetSkinMenu()
{
	return m_wndFrameTreeDlg->GetSkinMenu();
}

CTreeCtrl* CMainFrame::GetSkinTreeCtrl()
{
	return m_wndFrameTreeDlg->GetSkinTreeCtrl();
}

CTreeCtrl* CMainFrame::GetNodeTreeCtrl()
{
	return m_wndFrameManager2->GetNodeTreeCtrl();
}

void CMainFrame::DeleteContents()
{
	m_wndFrameTreeDlg->DeleteContents();
	m_wndFrameManager2->DeleteContents();
}

CModel* CMainFrame::GetModel()
{
	return ((CQMViewDoc*)GetActiveDocument())->GetModel();
}

int CMainFrame::GetManipulationType()
{
	return m_manipulationType;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_QMVIEWTYPE))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);

	if (!m_wndPosToolBar.Create(this) ||
		!m_wndPosToolBar.LoadToolBar(IDR_VIEWPOSBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndPosToolBar.SetBarStyle(m_wndPosToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndPosToolBar.EnableDocking(CBRS_ALIGN_ANY);

	DockControlBar(&m_wndPosToolBar, AFX_IDW_DOCKBAR_LEFT);

	if (!m_wndAnimToolBar.Create(this) ||
		!m_wndAnimToolBar.LoadToolBar(IDR_ANIMBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndAnimToolBar.SetBarStyle(m_wndAnimToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndAnimToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndAnimToolBar, AFX_IDW_DOCKBAR_TOP);

	m_wndFrameManager2 = new FrameManager2;
	if (!m_wndFrameManager2->Create(this, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE))
	{
		TRACE0("Failed to create dialog bar m_wndFrameManager2\n");
		return -1;		// fail to create
	}

	m_wndFrameTreeDlg = new CManagerTree();
	if (!m_wndFrameTreeDlg->Create(this, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE))
	{
		TRACE0("Failed to create dialog bar m_wndFrameTreeDlg\n");
		return -1;		// fail to create
	}

	// TODO: Add a menu item that will toggle the visibility of the
	// dialog bar named "Frame Manager2":
	//   1. In ResourceView, open the menu resource that is used by
	//      the CMainFrame class
	//   2. Select the View submenu
	//   3. Double-click on the blank item at the bottom of the submenu
	//   4. Assign the new item an ID: CG_ID_VIEW_FRAMEMANAGER2
	//   5. Assign the item a Caption: Frame Manager2

	// TODO: Change the value of CG_ID_VIEW_FRAMEMANAGER2 to an appropriate value:
	//   1. Open the file resource.h
	// CG: The following block was inserted by the 'Dialog Bar' component

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL overlayOn = false;

//Added so toolbar can function
void CMainFrame::OnAnimatePlay()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetPlay(true);
	GetActiveDocument()->UpdateAllViews(NULL, QM_RUN_ANIMATION, NULL);
}

void CMainFrame::OnAnimateStop()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetPlay(false);
	GetActiveDocument()->UpdateAllViews(NULL, QM_STOP_ANIMATION, NULL);
}

void CMainFrame::OnAnimateTypeBack()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetPlayMode(ANIM_BACKWARD);
	m_model->SetPingPong(false);
}

void CMainFrame::OnUpdateAnimateTypeBack(CCmdUI* pCmdUI)
{
	if (m_model != NULL)
	{
		pCmdUI->SetCheck((m_model->GetPlayMode()==ANIM_BACKWARD));
	}
}

void CMainFrame::OnAnimateTypeFront()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetPlayMode(ANIM_FORWARD);
	m_model->SetPingPong(false);
}

void CMainFrame::OnUpdateAnimateTypeFront(CCmdUI* pCmdUI)
{
	if (m_model != NULL)
	{
		pCmdUI->SetCheck((m_model->GetPlayMode()==ANIM_FORWARD));
	}
}

void CMainFrame::OnAnimateTypePingpong()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetPingPong(true);
}

void CMainFrame::OnUpdateAnimateTypePingpong(CCmdUI* pCmdUI)
{
	if (m_model != NULL)
	{
		pCmdUI->SetCheck((m_model->GetPlayMode()==ANIM_PINGPONG));
	}
}

void CMainFrame::OnRenderFlat()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetGroupQuality(D3DRMRENDER_FLAT);
//	m_model->ShowFrame();
}

void CMainFrame::OnUpdateRenderFlat(CCmdUI* pCmdUI)
{
	if (m_model)
	{
		D3DRMRENDERQUALITY meshquality = m_model->GetGroupQuality();
		pCmdUI->SetCheck(meshquality==D3DRMRENDER_FLAT);
	}
}

void CMainFrame::OnRenderGouraud()
{
	if (m_model)
	{
		m_model->SetGroupQuality(D3DRMRENDER_GOURAUD);
//		m_model->ShowFrame();
	}
}

void CMainFrame::OnUpdateRenderGouraud(CCmdUI* pCmdUI)
{
	if (m_model)
	{
		D3DRMRENDERQUALITY meshquality = m_model->GetGroupQuality();
		pCmdUI->SetCheck( meshquality==D3DRMRENDER_GOURAUD );
	}
}

void CMainFrame::OnRenderWireframe()
{
	if (m_model == NULL)
	{
		return;
	}
	m_model->SetGroupQuality(D3DRMRENDER_WIREFRAME);
//	m_model->ShowFrame();
}

void CMainFrame::OnUpdateRenderWireframe(CCmdUI* pCmdUI)
{
	if (m_model)
	{
		D3DRMRENDERQUALITY meshquality = m_model->GetGroupQuality();
		pCmdUI->SetCheck( meshquality==D3DRMRENDER_WIREFRAME );
	}
}

void CMainFrame::OnUpdateRender(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(TRUE);
}

void CMainFrame::OnUpdateAnimType(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(TRUE);
}

void CMainFrame::OnUpdateAnim(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(TRUE);
}

void CMainFrame::OnNodeToggleVis()
{
	HTREEITEM	item, parent;
	int			nodeNum = 0;
	CTreeCtrl*	pCTree = (CTreeCtrl*)m_wndFrameManager2->GetDlgItem(IDC_NODETREE);

	item = pCTree->GetSelectedItem();
	parent = pCTree->GetParentItem(item);

	if (parent == TVI_ROOT)
	{
		GetActiveDocument()->UpdateAllViews(NULL, QM_ALLNODES_VISIBLE, (CObject*)m_model);
		return;
	}

	CString cs = pCTree->GetItemText(item);
	LPSTR foo = cs.GetBuffer(10);

	sscanf(foo, "Node %d", &nodeNum);

	m_model->SelectMesh(NULL);

	if (m_model->ToggleNodeVisibility(nodeNum))
	{
		GetActiveDocument()->UpdateAllViews(NULL, QM_ADD_VISUAL, (CObject*)nodeNum);
		m_model->GetMesh(nodeNum)->SetGroupColorRGB(0, 255, 0, 0);
		pCTree->SetItemImage(item, 1, 1);
	}
	else
	{
		GetActiveDocument()->UpdateAllViews(NULL, QM_DELETE_VISUAL, (CObject*)nodeNum);
		pCTree->SetItemImage(item, 2, 2);
	}
}

void CMainFrame::OnSkinShowoverlay()
{
	overlayOn = !overlayOn;
	Invalidate(true);
}

void CMainFrame::OnUpdateSkinShowoverlay(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(overlayOn);
}

void CMainFrame::OnModeCamera()
{
	m_manipulationType = MANIPULATE_CAMERA;
}

void CMainFrame::OnUpdateModeCamera(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_manipulationType==MANIPULATE_CAMERA);
}

void CMainFrame::OnModeSkeletal()
{
	if (m_model == NULL)
	{
		return;
	}
	m_manipulationType = MANIPULATE_SKELETON;
}

void CMainFrame::OnUpdateModeSkeletal(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_manipulationType==MANIPULATE_SKELETON);
}

void CMainFrame::OnSkeletonSnap()
{
	if (m_model != NULL)
	{
		m_model->Snap();
		UpdateVisuals();
//		m_model->ShowFrame();
	}
}

void CMainFrame::OnJointManualAngles()
{
	if (m_jointDlg == NULL)
	{
		m_jointDlg = new CJointAnglesDlg;
		m_jointDlg->Create(IDD_JOINT_ANGLES, this);
	}
	else
	{
		m_jointDlg->DestroyWindow();
		delete m_jointDlg;
		m_jointDlg = NULL;
	}
}

void CMainFrame::OnUseJoints()
{
	if (m_model != NULL)
	{
		m_model->ToggleJointOn();
	}
}

void CMainFrame::OnUpdateUseJoints(CCmdUI* pCmdUI)
{
	if (m_model != NULL)
	{
		pCmdUI->SetRadio(m_model->IsJointOn());
	}
}

void CMainFrame::DestroyJointDialogs()
{
	if (m_jointDlg != NULL)
	{
		m_jointDlg->DestroyWindow();
		delete m_jointDlg;
		m_jointDlg = NULL;
	}
	if (m_jointConstraintDlg != NULL)
	{
		m_jointConstraintDlg->DestroyWindow();
		delete m_jointConstraintDlg;
		m_jointConstraintDlg = NULL;
	}
}

void CMainFrame::UpdateVisuals()
{
	if (m_jointDlg != NULL)
	{
		m_jointDlg->UpdateVisuals();
	}
}

void CMainFrame::OnUpdateJointManualAngles(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_jointDlg != NULL);
}

void CMainFrame::OnUpdateJointConstraints(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_jointConstraintDlg != NULL);
}

void CMainFrame::OnJointConstraints()
{
	// TODO: Add your command handler code here
	if (m_jointConstraintDlg == NULL)
	{
		m_jointConstraintDlg = new CJointConstraintDlg;
		m_jointConstraintDlg->Create(IDD_JOINT_LOCK_ANGLES, this);
	}
	else
	{
		m_jointConstraintDlg->DestroyWindow();
		delete m_jointConstraintDlg;
		m_jointConstraintDlg = NULL;
	}
}

void CMainFrame::OnWindowFrame()
{
	// TODO: Add your command handler code here
	if (m_currentView == ID_MODELVIEW)
	{
		return;
	}
	if (m_modelView == NULL)
	{
		MessageBox("Logic error -- no model view");
		return;
	}
	m_modelView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
	m_skinView->SetDlgCtrlID(ID_SKINVIEW);
	m_modelView->ShowWindow(SW_SHOW);
	m_skinView->ShowWindow(SW_HIDE);
	SetActiveView(m_modelView);
	m_currentView = ID_MODELVIEW;
	RecalcLayout();
}

void CMainFrame::OnUpdateWindowSkin(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_currentView == ID_SKINVIEW);
}

void CMainFrame::OnWindowSkin()
{
	// TODO: Add your command handler code here
	if (m_currentView == ID_SKINVIEW)
	{
		return;
	}
	m_skinView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
	m_modelView->SetDlgCtrlID(ID_MODELVIEW);
	m_skinView->ShowWindow(SW_SHOW);
	m_modelView->ShowWindow(SW_HIDE);
	SetActiveView(m_skinView);
	m_currentView = ID_SKINVIEW;
	RecalcLayout();
}

void CMainFrame::OnUpdateWindowFrame(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_currentView == ID_MODELVIEW);
}


void CMainFrame::OnUpdateAnimateStop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_model == NULL)
	{
		return;
	}
	pCmdUI->SetCheck(!m_model->Playing());
}

void CMainFrame::OnUpdateAnimatePlay(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_model == NULL)
	{
		return;
	}
	pCmdUI->SetCheck(m_model->Playing());
}

void CMainFrame::OnDestroy()
{
	CFrameWnd::OnDestroy();

	// TODO: Add your message handler code here
	if (m_jointDlg != NULL)
	{
		m_jointDlg->DestroyWindow();
		delete m_jointDlg;
		m_jointDlg = NULL;
	}
	if (m_jointConstraintDlg != NULL)
	{
		m_jointConstraintDlg->DestroyWindow();
		delete m_jointConstraintDlg;
		m_jointConstraintDlg = NULL;
	}
	if (m_wndFrameTreeDlg != NULL)
	{
		m_wndFrameTreeDlg->DestroyWindow();
		delete m_wndFrameTreeDlg;
		m_wndFrameTreeDlg = NULL;
	}
	if (m_wndFrameManager2 != NULL)
	{
		m_wndFrameManager2->DestroyWindow();
		delete m_wndFrameManager2;
		m_wndFrameManager2 = NULL;
	}
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class

	m_modelView = (CView*)CreateView(pContext, AFX_IDW_PANE_FIRST);
	m_skinView = new CSkinPageFrm;
	m_skinView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0,0,0,0), this, ID_SKINVIEW, NULL);
	CDocument* pDoc = pContext->m_pCurrentDoc;
	pDoc->AddView(m_skinView);
	m_modelView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
	m_skinView->SetDlgCtrlID(ID_SKINVIEW);
	m_modelView->ShowWindow(SW_SHOW);
	m_skinView->ShowWindow(SW_HIDE);
	SetActiveView(m_modelView);
	m_currentView = ID_MODELVIEW;
	return true;
}
