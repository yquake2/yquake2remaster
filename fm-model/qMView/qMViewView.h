// qMViewView.h : interface of the CQMViewView class
//
/////////////////////////////////////////////////////////////////////////////

class CQMViewView : public CView
{
protected: // create from serialization only
	CQMViewView();
	DECLARE_DYNCREATE(CQMViewView)

// Attributes
public:
	CQMViewDoc* GetDocument();
	LPDIRECT3DRM2 GetD3D();

// Operations
public:
	BOOL CreateDevice();
	void CreateDot(vec3_t origin, int r, int g , int b);

	static void UpdateDrag(LPDIRECT3DRMFRAME frame, void* data, D3DVALUE);

//	GUID* GetGUID();
	BOOL CreateScene();
	D3DVALUE ScaleMesh( LPDIRECT3DRMMESHBUILDER mesh, D3DVALUE dim);

 	void PickMesh(CPoint point);

	void SetColorModel( D3DCOLORMODEL cm )  { m_colormodel=cm; }
	inline COLORREF D3DCOLOR_2_COLORREF(D3DCOLOR d3dclr);
	inline D3DCOLOR COLORREF_2_D3DCOLOR(COLORREF cref);

	D3DCOLORMODEL m_colormodel;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQMViewView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CQMViewView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void DrawOnSkin(UINT nFlags, CPoint point);

// Generated message map functions
protected:
	LPDIRECTDRAWCLIPPER		m_clipper;
	LPDIRECT3DRMDEVICE2		m_device;
	LPDIRECT3DRMFRAME2		m_frame;
	LPDIRECT3DRMFRAME2		m_camera;
	LPDIRECT3DRMVIEWPORT	m_viewport;
	LPDIRECT3DRMFRAME2		m_scene;
	bool					m_transtate;
	bool					m_curstate;
	bool					m_d3drm_up;
	LPDIRECT3DRM2			m_d3drm;
	double					m_globalField;

// mouse movements
	BOOL					m_drag;
	BOOL					m_r_drag;

	BOOL					m_has_moved;
	BOOL					m_r_has_moved;

	int						m_last_x;
	int						m_last_y;
	int						m_r_last_x;
	int						m_r_last_y;

// locks
	bool					m_lock_rot_x;
	bool					m_lock_rot_y;
	bool					m_lock_rot_z;
	bool					m_lock_scale_x;
	bool					m_lock_scale_y;
	bool					m_lock_scale_z;
	bool					m_lock_trans_x;
	bool					m_lock_trans_y;
	bool					m_lock_trans_z;

	bool					m_interOn;

	void CreateOriginVisual();
	int PickFace(const CPoint &point);

	//{{AFX_MSG(CQMViewView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnAllLock();
	afx_msg void OnAllUnlock();
	afx_msg void OnGotoposBack();
	afx_msg void OnGotoposBottom();
	afx_msg void OnGotoposFront();
	afx_msg void OnGotoposLeft();
	afx_msg void OnGotoposRight();
	afx_msg void OnGotoposTop();
	afx_msg void OnLockRotX();
	afx_msg void OnUpdateLockRotX(CCmdUI* pCmdUI);
	afx_msg void OnLockRotY();
	afx_msg void OnUpdateLockRotY(CCmdUI* pCmdUI);
	afx_msg void OnLockRotZ();
	afx_msg void OnUpdateLockRotZ(CCmdUI* pCmdUI);
	afx_msg void OnLockScaleX();
	afx_msg void OnUpdateLockScaleX(CCmdUI* pCmdUI);
	afx_msg void OnLockScaleY();
	afx_msg void OnUpdateLockScaleY(CCmdUI* pCmdUI);
	afx_msg void OnLockScaleZ();
	afx_msg void OnUpdateLockScaleZ(CCmdUI* pCmdUI);
	afx_msg void OnLockTranX();
	afx_msg void OnUpdateLockTranX(CCmdUI* pCmdUI);
	afx_msg void OnLockTranY();
	afx_msg void OnUpdateLockTranY(CCmdUI* pCmdUI);
	afx_msg void OnLockTranZ();
	afx_msg void OnUpdateLockTranZ(CCmdUI* pCmdUI);
	afx_msg void OnRenderTrans();
	afx_msg void OnUpdateRenderTrans(CCmdUI* pCmdUI);
	afx_msg void OnRenderTexture();
	afx_msg void OnUpdateRenderTexture(CCmdUI* pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnGotoposWeapon();
	afx_msg void OnTickerOptions();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnAnimateStepBack();
	afx_msg void OnAnimateStepFore();
	afx_msg void OnAnimIntertoggle();
	afx_msg void OnUpdateAnimIntertoggle(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in qMViewView.cpp
inline CQMViewDoc* CQMViewView::GetDocument()
   { return (CQMViewDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
inline D3DCOLOR CQMViewView::COLORREF_2_D3DCOLOR(COLORREF cref)
{
	D3DVALUE r=D3DVALUE(GetRValue(cref))/D3DVALUE(255);
	D3DVALUE g=D3DVALUE(GetGValue(cref))/D3DVALUE(255);
	D3DVALUE b=D3DVALUE(GetBValue(cref))/D3DVALUE(255);
	return D3DRMCreateColorRGB( r, g, b );
}

inline COLORREF CQMViewView::D3DCOLOR_2_COLORREF(D3DCOLOR d3dclr)
{
	D3DVALUE red=D3DVALUE(255)*D3DRMColorGetRed(d3dclr);
	D3DVALUE green=D3DVALUE(255)*D3DRMColorGetGreen( d3dclr );
	D3DVALUE blue=D3DVALUE(255)*D3DRMColorGetBlue( d3dclr );
	return RGB((int)red,(int)green,(int)blue);
}

