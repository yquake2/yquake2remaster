// SkinTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSkinTreeCtrl window

class CSkinTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CSkinTreeCtrl();
	void DeleteContents();
	void SetModel(CModel* model);
	CModel* GetModel();
	CMenu* GetSkinMenu();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinTreeCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSkinTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinTreeCtrl)
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	CImageList				m_image;
	HTREEITEM				m_rootSkin;
	CMenu*					m_menu;
};

/////////////////////////////////////////////////////////////////////////////
