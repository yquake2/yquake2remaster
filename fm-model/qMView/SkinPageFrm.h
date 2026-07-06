#if !defined(AFX_SKINPAGEFRM_H__80CBF401_CA1C_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_SKINPAGEFRM_H__80CBF401_CA1C_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SkinPageFrm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSkinPageFrm frame

class CSkinPageFrm : public CView
{
	DECLARE_DYNCREATE(CSkinPageFrm)
protected:
// Attributes
public:
	CSkinPageFrm();           // protected constructor used by dynamic creation
	virtual ~CSkinPageFrm();
	void SetModel(CModel* model);
	CQMViewDoc* GetDocument();

// Operations
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	void DrawSTOverlay(void);
	void AdjustScrollers(int width, int height);
	static void Register();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinPageFrm)
	protected:
	virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	float					m_skinScale;
	CBitmap*				m_skinBitmap;
	long					m_num_tris;
	D3DRMVERTEX*			m_vertexlist;

	// Generated message map functions
	//{{AFX_MSG(CSkinPageFrm)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINPAGEFRM_H__80CBF401_CA1C_11D1_82DF_0080C82BD965__INCLUDED_)
