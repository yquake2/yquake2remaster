// FrameTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFrameTreeCtrl window

#include "TreeCtrlEx.h"

class CFrameTreeCtrl : public CTreeCtrlEx
{
// Construction
public:
	CFrameTreeCtrl();
	void SetModel(CModel* model);
	void DeleteContents();
	CModel* GetModel();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFrameTreeCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFrameTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFrameTreeCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	CImageList		m_image;
	HTREEITEM		m_rootFrame;
};

/////////////////////////////////////////////////////////////////////////////
