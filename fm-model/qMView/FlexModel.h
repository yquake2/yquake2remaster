// FlexModel.h
//

#define MAX_LBM_HEIGHT		480
#define	MAX_FM_TRIANGLES	2048
#define MAX_FM_VERTS		2048
#define MAX_FM_FRAMES		2048
#define MAX_FM_SKINS		64
#define	MAX_FM_SKINNAME		64
#define MAX_FM_MESH_NODES	16		// also defined in game/qshared.h
#define MAX_JOINTS_PER_SKELETON	8	// arbitrary small number
#define MAX_JOINT_NODES_PER_SKELETON (MAX_JOINTS_PER_SKELETON - 1)
#define JOINT_VISUALS		10
#define SKELETON_COUNT		256

class CNode
{
public:
	void Init();
	void Delete();

	void DeleteMesh();
	LPDIRECT3DRMMESH GetMesh();

	void SetSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin, int skinnum);
	void ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin);
	void LoadSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin, int skinnum);

	CBitmap* GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int& width, int& height);
	void AddVisual(LPDIRECT3DRMFRAME frame);
	bool IsVisible();
	void SetVisible(bool visible);
	int GetNumTris();
	void SetTris(byte* buf, int numTris);
	void SetVerts(byte* buf, int numVerts);
	void SetGlcmds(long* glcmds, int num);
	void SetTexture(bool useTexture);

	HRESULT SetQuality(D3DRMRENDERQUALITY value);
	void SetTransparency(LPDIRECT3DRM2 d3drm, CDC* pDC, bool trans);
	HRESULT SetColorRGB(D3DVALUE red, D3DVALUE green, D3DVALUE blue);
	void RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture);

	void BuildMesh(LPDIRECT3DRM2 d3drm, frameinfo2_t* frameinfo1, int skinwidth, int shinheight);
	bool ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, vec3a_t* rotVerts, frameinfo2_t* frameinfo1);
	void ShowInterFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, frameinfo2_t* frameinfo1, frameinfo2_t* frameinfo2, int step, int numsteps);

	HTREEITEM				m_treeitem;
//	char*					m_skinbuf;
//	int						m_skinheight;
//	int						m_skinwidth;
//	LPDIRECT3DRMTEXTURE		m_texture;
protected:
	int*					m_vertPath;
	byte*					m_tris;
	byte*					m_verts;
	long*					m_start_glcmds;
	short					m_num_glcmds;
	LPDIRECT3DRMMESH		m_mesh;
	bool					m_visible;
	int						m_numTris;
	int						m_numVerts;

	long					m_group;
	int						m_defaultSkin;
	CSkin*					m_curSkin;
};

typedef struct
{
	LPDIRECT3DRMMESH	dir, up, origin, right;
} jointTri;

typedef struct
{
	D3DVECTOR	min, max;
} rangeVector_t;

typedef struct
{
	short	index_xyz[3];
	short	index_st[3];
} fmtriangle_t;

typedef struct
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
} fmtrivertx_t;

typedef struct
{
	float			scale[3];		// multiply byte verts by this
	float			translate[3];	// then add this
	char			name[16];		// frame name from grabbing
	fmtrivertx_t	verts[1];		// variable sized
} fmaliasframe_t;

typedef struct
{
	int				start_frame;
	int				num_frames;
	int				degrees;
	char			*mat;
	char			*ccomp;
	unsigned char	*cbase;
	float			*cscale;
	float			*coffset;
	float			trans[3];
	float			scale[3];
	float			bmin[3];
	float			bmax[3];
	float			*complerp;
} fmgroup_t;

typedef struct M_SkeletalCluster_s
{
	int children;		// must be the first field
	int numVerticies;
	int *verticies;
	qboolean inUse;
} M_SkeletalCluster_t;

typedef struct
{
	char		blockid[32];
	int			blocktype;
} fmblock_t;

// flex model itself

class CFlexModel : public CModel
{
public:
	CFlexModel();
	~CFlexModel();
	virtual void Delete();

	virtual CBitmap* GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int skinnum, int& width, int& height);
	virtual void DeleteVisuals(LPDIRECT3DRMFRAME frame);
	virtual void DeleteMeshs(LPDIRECT3DRMFRAME frame);
	virtual void RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture);
	virtual void CreateJointVisuals(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame);

	virtual LPDIRECT3DRMMESH GetMesh(int i);
	virtual void MakeAllNodesVisible(LPDIRECT3DRMFRAME frame);
	virtual bool ToggleNodeVisibility(int node);
	virtual HTREEITEM SelectMesh(LPDIRECT3DRMVISUAL selection);
	virtual int SelectNode(int nodeNum);
	virtual void ChangeVisual(LPDIRECT3DRMFRAME frame, int nodeNum);
	virtual void DeSelectAll();
	virtual HRESULT SetGroupQuality(D3DRMRENDERQUALITY value);
	virtual void BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC);
	virtual bool ValidNode();
	virtual long GetNumNodes();
	virtual long GetNumTris();

	CSkin* GetSkin(int i);
	virtual void ReplaceSkin(int i, LPCTSTR skinname);
	virtual void AddSkin(LPCTSTR skinname);
	virtual void FillMenuWithSkins(CMenu* menu);
	virtual void ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, int nodenum, int skinnum);

	virtual BOOL ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC);

	virtual void Drag(double delta_x, double delta_y);
	virtual void Snap();

	virtual BOOL CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction);

	// Ctrl window support
	virtual void AddJoints(CTreeCtrl* pCTree, HTREEITEM rootJoint);
	virtual void AddNodes (CTreeCtrl*pCTree, HTREEITEM rootNode);
	virtual void LoadSkinInfo(CTreeCtrl* pCTree, HTREEITEM rootSkin);
	virtual bool LoadSkin(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, int skinnum, CDC* pDC);
// frame tree ctrl support
	virtual void LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame);

protected:
	fmgroup_t				m_compdata;
	char*					m_framenames;
	byte*					m_lightnormalindex;

	char**					m_skin_names;

	int						m_num_mesh_nodes;
	CNode*					m_nodes;
	int						m_curNode;

	int						m_skeletalType;
	ModelSkeleton_t*		m_skeletons;
	jointTri				m_joint_visuals[JOINT_VISUALS];
	int						m_referenceType;


	virtual void Init();
	void UpdateJointVisuals(int jointNum);
	void FillRawVerts(vec3a_t *foo);

	void CreateJointVisual(LPDIRECT3DRM2 d3drm, Placement_t model, int jointNum, LPDIRECT3DRMFRAME frame);

// File read support
	static fmblock_t		m_fmblocks[];

	virtual void Serialize(CArchive& ar);
	void SerializeHeader(int version, int length, char* buffer);
	void SerializeSkin(int version, int length, char* buffer);
	void SerializeSt(int version, int length, char* buffer);
	void SerializeTris(int version, int length, char* buffer);
	void SerializeFrames(int version, int length, char* buffer);
	void SerializeGLCmds(int version, int length, char* buffer);
	void SerializeMeshNodes(int version, int length, char* buffer);
	void SerializeShortFrames(int version, int length, char* buffer);
	void SerializeNormal(int version, int length, char* buffer);
	void SerializeComp(int version, int length, char* buffer);
	void SerializeSkeleton(int version, int length, char* buffer);
	void SerializeReferences(int version, int length, char* buffer);

// Ctrl Window Support
	void AddNode (CTreeCtrl* pCTree, HTREEITEM rootNode, int nodeNum);
	void RotateModelSegment(M_SkeletalJoint_t *joint, vec3a_t *modelVerticies, vec3a_t angles, M_SkeletalCluster_t *modelCluster);

// skin texturing support

public:
	virtual void SetTransparency(LPDIRECT3DRM2 d3drm, CDC* pDC, bool trans);

	// joint support
public:
	virtual bool IsJointOn();
	virtual void ToggleJointOn();
	virtual void GetModelAngles(float* x, float* y, float* z);
	virtual void SetModelAngles(float x, float y, float z);
	virtual void GetConstraintAngleMaxs(float* x, float* y, float* z);
	virtual void GetConstraintAngleMins(float* x, float* y, float* z);
	virtual void SetConstraintAngleMaxs(float x, float y, float z);
	virtual void SetConstraintAngleMins(float x, float y, float z);
	virtual void SetCurJoint(long joint);
	virtual long GetCurJoint();

protected:
	bool					m_jointsOn;
	long					m_curJoint;
	rangeVector_t*			m_jointConstraintAngles;
	D3DVECTOR*				m_modelJointAngles;
	M_SkeletalCluster_t*	m_skeletalClusters;
	int						m_numClusters;

	void ConstrainJoint();

	// property sheet support
public:
	virtual void AddPropPages(CPropertySheet* propSheet);
	virtual void UpdateFromPropPages(CPropertySheet* propSheet);
	virtual void RemovePropPages(CPropertySheet* propSheet);
protected:
	// specific prop page declaration
};

