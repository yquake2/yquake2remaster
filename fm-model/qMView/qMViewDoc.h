// qMViewDoc.h : interface of the CQMViewDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CQMViewDoc : public CDocument
{
protected: // create from serialization only
	CQMViewDoc();
	DECLARE_DYNCREATE(CQMViewDoc)

// Attributes
public:

// Operations
public:
//	virtual void OnIdle();
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
	CModel* GetModel();
	LPDIRECT3DRM2 GetD3D();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQMViewDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CQMViewDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DWORD		m_spec_dwFps;
	DWORD		m_spec_dwFramesRendered;
	DWORD		m_spec_dwCurTime;
	DWORD		m_spec_dwDeltaTime;
	DWORD		m_spec_dwLastTime;
	DWORD		m_spec_dwFpsTime;

// Generated message map functions
protected:
	void ReplaceSkin(LPCTSTR skinname);
	afx_msg void OnSkinFromFile(UINT nID);
	afx_msg void OnUpdateSkinFromFile(CCmdUI* pCmdUI);
	afx_msg void OnSkinSelect(UINT nID);
	afx_msg void OnUpdateSkinSelect(CCmdUI* pCmdUI);
	//{{AFX_MSG(CQMViewDoc)
	afx_msg void OnNodeProp();
	afx_msg void OnUpdateViewProps(CCmdUI* pCmdUI);
	afx_msg void OnViewProps();
	afx_msg void OnReplaceSkin();
	afx_msg void OnUpdateReplaceSkin(CCmdUI* pCmdUI);
	afx_msg void OnSkinImport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CModel*				m_model;
};

/////////////////////////////////////////////////////////////////////////////
