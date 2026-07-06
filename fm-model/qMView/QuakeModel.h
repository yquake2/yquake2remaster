// QuakeModel.h

typedef struct
{
  long type;
  long numgroupframes;
  trivertx_t groupmin;
  trivertx_t groupmax;
  frameinfo_t *info;
  float *interval;
  trivertx_t **data;
} frame_t;

class CQuakeModel : public CModel
{
public:
	CQuakeModel();
	~CQuakeModel();
	virtual void Serialize(CArchive& ar);
	virtual void BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC);

	virtual void Delete();

	virtual CSkin* GetSkin(int i);
	virtual void ReplaceSkin(int i, LPCTSTR skinname);
	virtual void AddSkin(LPCTSTR skinname);
	virtual void FillMenuWithSkins(CMenu* menu);

	virtual void DeleteMeshs(LPDIRECT3DRMFRAME frame);
	virtual LPDIRECT3DRMMESH GetMesh(int i = -1);
	virtual void SetBackOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetFrontOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetLeftOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetRightOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetTopOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetBottomOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetInitialOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);

	virtual BOOL ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC);

	virtual BOOL CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction);
// frame tree ctrl support
	virtual void LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame);


protected:
	virtual void Init();

	vec3_t			m_scale;
	vec3_t			m_origin;
	scalar_t		m_radius;
	vec3_t			m_eye;
	long			m_sync;
	long			m_flags;
	float			m_size;
	LPDIRECT3DRMMESH	m_mesh;
};

