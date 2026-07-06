// QuakeModel2.h

class CQuake2Model : public CModel
{
public:
	CQuake2Model();
	~CQuake2Model();

	virtual void Delete();

	virtual CSkin* GetSkin(int i);
	virtual void ReplaceSkin(int i, LPCTSTR skinname);
	virtual void AddSkin(LPCTSTR skinname);
	virtual void FillMenuWithSkins(CMenu* menu);

	virtual void DeleteMeshs(LPDIRECT3DRMFRAME frame);
	virtual LPDIRECT3DRMMESH GetMesh(int i = -1);
	virtual void Serialize(CArchive& ar);
	// tris and sts are not read in - only first skin name read in

	virtual void BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC);
	virtual BOOL ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC);

	virtual BOOL CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction);
// frame tree control support
	virtual void LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame);

protected:
	char*			m_skinname;		// eventually goes away

	virtual void Init();

	LPDIRECT3DRMMESH	m_mesh;
};

