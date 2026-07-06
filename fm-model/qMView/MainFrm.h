// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#define	MANIPULATE_CAMERA	0
#define MANIPULATE_SKELETON 1

class CMainFrame : public CFrameWnd
{
	DECLARE_DYNCREATE(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	void UpdateVisuals();
	CModel* GetModel();
	CMenu* GetSkinMenu();
	CTreeCtrl* GetSkinTreeCtrl();
	CTreeCtrl* GetNodeTreeCtrl();
	LPDIRECT3DRM2 GetD3D();
// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual	~CMainFrame();

	void SetModel(CModel* model);
	void DeleteContents();
	void PickNode(HTREEITEM item);
	int GetManipulationType();

	void DestroyJointDialogs();

public:  // control bar embedded members
	HTREEITEM	m_hItemFirstSel;

// Generated message map functions
protected:
	CToolBar				m_wndToolBar;
	CToolBar				m_wndPosToolBar;
	CToolBar				m_wndAnimToolBar;
	CStatusBar				m_wndStatusBar;

	CJointAnglesDlg*		m_jointDlg;
	CJointConstraintDlg*	m_jointConstraintDlg;
	CManagerTree*			m_wndFrameTreeDlg;
	FrameManager2*			m_wndFrameManager2;
	CModel*					m_model;

	int						m_currentView;
	CView*					m_skinView;
	CView*					m_modelView;

	int						m_manipulationType;

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAnimatePlay();
	afx_msg void OnAnimateStop();
	afx_msg void OnAnimateTypeBack();
	afx_msg void OnUpdateAnimateTypeBack(CCmdUI* pCmdUI);
	afx_msg void OnAnimateTypeFront();
	afx_msg void OnUpdateAnimateTypeFront(CCmdUI* pCmdUI);
	afx_msg void OnAnimateTypePingpong();
	afx_msg void OnUpdateAnimateTypePingpong(CCmdUI* pCmdUI);
	afx_msg void OnRenderFlat();
	afx_msg void OnUpdateRenderFlat(CCmdUI* pCmdUI);
	afx_msg void OnRenderGouraud();
	afx_msg void OnUpdateRenderGouraud(CCmdUI* pCmdUI);
	afx_msg void OnRenderWireframe();
	afx_msg void OnUpdateRenderWireframe(CCmdUI* pCmdUI);
	afx_msg void OnNodeToggleVis();
	afx_msg void OnSkinShowoverlay();
	afx_msg void OnUpdateSkinShowoverlay(CCmdUI* pCmdUI);
	afx_msg void OnModeCamera();
	afx_msg void OnUpdateModeCamera(CCmdUI* pCmdUI);
	afx_msg void OnModeSkeletal();
	afx_msg void OnUpdateModeSkeletal(CCmdUI* pCmdUI);
	afx_msg void OnSkeletonSnap();
	afx_msg void OnJointManualAngles();
	afx_msg void OnUseJoints();
	afx_msg void OnUpdateUseJoints(CCmdUI* pCmdUI);
	afx_msg void OnUpdateJointManualAngles(CCmdUI* pCmdUI);
	afx_msg void OnUpdateJointConstraints(CCmdUI* pCmdUI);
	afx_msg void OnJointConstraints();
	afx_msg void OnWindowSkin();
	afx_msg void OnUpdateWindowSkin(CCmdUI* pCmdUI);
	afx_msg void OnWindowFrame();
	afx_msg void OnUpdateWindowFrame(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimateStop(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateAnimatePlay(CCmdUI* pCmdUI);
	//}}AFX_MSG

	afx_msg void OnUpdateAnim(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRender(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimType(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
