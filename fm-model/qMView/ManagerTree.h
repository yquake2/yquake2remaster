// ManagerTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CManagerTree window

class CManagerTree : public CDialogBar
{
// Construction
public:
	CFrameTreeCtrl m_nFrameTree;
	CSkinTreeCtrl m_nSkinTree;
	CManagerTree();
	BOOL Create(CWnd* pParentWnd, UINT nStyle);
	void SetModel(CModel* model);
	void DeleteContents();
	CModel* GetModel();
	CMenu* GetSkinMenu();
	CTreeCtrl* GetSkinTreeCtrl();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CManagerTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CManagerTree();

	// Generated message map functions
protected:
	//{{AFX_MSG(CManagerTree)
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
