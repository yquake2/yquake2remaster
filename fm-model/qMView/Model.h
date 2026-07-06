// Model.h
#define	MIPLEVELS		16
#define MIP_VERSION		2
#define INTERPOLATION_STEPS 4
#define FRAME_TICK_INCR 100

#define	ANIM_FORWARD	0
#define	ANIM_BACKWARD	1
#define	ANIM_PINGPONG	2
#define	ANIM_PUNCH		3

#define PALTYPE_RAW  0
#define PALTYPE_PCX  1
#define PALTYPE_JASC 2
#define PALTYPE_MS   3
#define PALTYPE_BMP  4

#define PAL_QUAKE		0
#define PAL_QUAKE2		1
#define PAL_HEXEN2		2
#define PAL_FROMFILE	3
#define PAL_NONE		4

#define PAL_SIZE		256

typedef struct
{
	unsigned char r,g,b;
} paletteRGB_t;

typedef struct Placement_s
{
	vec_t origin;
	vec_t direction;
	vec_t up;
} Placement_t;

typedef struct M_SkeletalJoint_s
{
	int children;		// must be the first field
	Placement_t model;	// relative to the model, used for dynamic software rotation
	Placement_t parent;	// relative to the parent joint (or model in case of root joint), used for
						// inverse kinematics
	qboolean inUse;
} M_SkeletalJoint_t;

typedef struct ArrayedListNode_s
{
	int data;
	int next;
	int inUse;
} ArrayedListNode_t;

typedef struct ModelSkeleton_s
{
	M_SkeletalJoint_t rootJoint[8];
	ArrayedListNode_t rootNode[64];
} ModelSkeleton_t;

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} pal_t;

typedef struct
{
	int numFrames;
	int info;
	int* frames;
} frameStruct;

typedef struct miptex_s
{
	//int				version;
	char			name[32];
	unsigned		width[MIPLEVELS/2], height[MIPLEVELS/2];
	unsigned		offsets[MIPLEVELS/2];		// four mip maps stored
	char			animname[32];			// next frame in animation chain
	paletteRGB_t	palette[PAL_SIZE];
	int				flags;
	int				contents;
	int				value;
} miptex_t;

typedef struct miptex_v2_s
{
	//int				version;
	char			name[32];
	unsigned		width[MIPLEVELS], height[MIPLEVELS];
	unsigned		offsets[MIPLEVELS];		// four mip maps stored
	char			animname[32];			// next frame in animation chain
	paletteRGB_t	palette[PAL_SIZE];
	int				flags;
	int				contents;
	int				value;
} miptex_v2_t;

typedef struct
{
  long facesfront;
  long vertices[3];

} itriangle_t;

typedef struct
{
  u_char x;
  u_char y;
  u_char z;
  u_char lightnormalindex;
} trivertx_t;


typedef struct
{
  trivertx_t min;
  trivertx_t max;
  char name[16];
} frameinfo_t;

typedef struct
{
  vec3_t scale;
  vec3_t origin;
  char name[16];
  trivertx_t verts[1];
} frameinfo2_t;

typedef struct
{
	int type;
	int skin_num;
	HTREEITEM pointer;
} skinItem;

typedef struct
{
	int numskins;
	skinItem *skins;
} skinStruct;

// used for tree control maintenance

class CTreeGroup
{
public:
	HTREEITEM head;
	char id[16];
};

class CTreeHead
{
public:
	int			bFrame;
	int			eFrame;
};

/////////////////////////////////////////////////////////////////////////////
// CGeneralPropPage dialog

class CGeneralPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGeneralPropPage)

// Construction
public:
	CGeneralPropPage();
	~CGeneralPropPage();

// Dialog Data
	//{{AFX_DATA(CGeneralPropPage)
	enum { IDD = IDD_PP_GENERAL };
	CString	m_frames;
	CString	m_glcommands;
	CString	m_groups;
	CString	m_skinheight;
	CString	m_skins;
	CString	m_skinwidth;
	CString	m_tris;
	CString	m_stverts;
	CString	m_verts;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGeneralPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGeneralPropPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//
// CSkin
//

typedef struct
{
  long        type;
  long        numgroupskins;
  float       *interval;
  u_char      **skin;
} skin_t;

class CSkin
{
public:
	CSkin();
	~CSkin();

	CSkin* GetNext();
	CSkin* InsertSkin(CSkin* newSkin);
	CSkin* Find(LPCTSTR filename);

	void SetFilename(LPCTSTR filename);
	LPCTSTR GetFilename();
	int GetWidth(LPDIRECT3DRM2 d3drm, CDC* pDC);
	int GetHeight(LPDIRECT3DRM2 d3drm, CDC* pDC);
	LPDIRECT3DRMTEXTURE GetTexture(LPDIRECT3DRM2 d3drm, CDC* pDC);
	CBitmap* GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC);


protected:
	CSkin*					m_parent;
	CSkin*					m_left;
	CSkin*					m_right;

	char*					m_filename;
	CBitmap*				m_bitmap;
	LPDIRECT3DRMTEXTURE2	m_texture;
	int						m_bitDepth;
	int						m_skinWidth;
	int						m_skinHeight;

	LPDIRECTDRAWSURFACE		t_lpDDSkin;
	int						t_pitch;
	byte*					t_skinBits; // temporary during build of bitmap
	byte*					t_textureBits; // temporary during build of texture

	static CRGB16*			s_rgb16;

	void Cache(LPDIRECT3DRM2 d3drm, CDC* pDC);
	void CreateRGB16(CDC* pDC);

	bool CacheFromPCX(CFile* file);
	bool CacheFromM8(CFile* file);
	bool CacheFromM32(CFile* file);
	bool CacheFromTGA(CFile* file);

	bool SetBitmap(int width, int height);
	void SetBitmapRow(int row, byte* buf, pal_t* palette);
	void SetBitmapRow(int row, long* buf);
};

//
// CModel
//

class CModel
{
public:
	virtual void Serialize(CArchive& ar);
	CModel();
	~CModel();

	virtual CBitmap* GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int skinnum, int& width, int& height);
	virtual CSkin* GetSkin(LPCTSTR skinname);
	virtual CSkin* GetSkin(int i);
	virtual void ReplaceSkin(int i, LPCTSTR skinname);
	virtual void AddSkin(LPCTSTR skinname);
	virtual void FillMenuWithSkins(CMenu* menu);
	virtual void ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, int nodenum, int skinnum);

	void SetPunch(bool usePunch);
	int GetPlayMode();
	void SetPlayMode(int playmode);
	void SetTimerData(long delay, bool useTimer);
	void GetTimerData(long* delay, bool* useTimer);
	virtual void RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture);
	virtual void DeleteVisuals(LPDIRECT3DRMFRAME frame);
	void SetFilename(LPCTSTR filename);
	LPCTSTR GetFilename();
	static CModel* Create(LPCTSTR extension);
	virtual void Delete();
	virtual void DeleteMeshs(LPDIRECT3DRMFRAME frame);

	virtual LPDIRECT3DRMMESH GetMesh(int i = -1);
	virtual bool ToggleNodeVisibility(int node);
	virtual void MakeAllNodesVisible(LPDIRECT3DRMFRAME frame);
	virtual HTREEITEM SelectMesh(LPDIRECT3DRMVISUAL selection);
	virtual int SelectNode(int nodeNum);
	virtual void ChangeVisual(LPDIRECT3DRMFRAME frame, int nodeNum);
	virtual void DeSelectAll();
	virtual HRESULT SetGroupQuality(D3DRMRENDERQUALITY value);
	virtual D3DRMRENDERQUALITY GetGroupQuality();
	virtual bool ValidNode();
	virtual long GetNumFrames();
	void SetCurGroup(long group);
	long GetNumGroups();
	virtual long GetNumNodes();
	void SetPingPong(bool pingPong);
	virtual void BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC);
	virtual long* GetCommands();
	virtual void CreateJointVisuals(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame);
	virtual void SetCurJoint(long joint);
	virtual long GetCurJoint();
	virtual long GetNumTris();
	virtual int GetTriCount();
	int	GetCurFrame();
	void SetCurFrame(int curframe);
	double GetCurScale();
	void SetCurScale(double curscale);
	void ResetPlay();
	void SetPlay(bool playing);
	bool Playing();

	virtual void SetBackOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetFrontOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetLeftOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetRightOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetTopOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetBottomOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);
	virtual void SetInitialOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene);

	virtual bool LoadSkin(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, int skinnum, CDC* pDC);

	virtual BOOL ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC);
	virtual BOOL UpdateFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, int direction, bool interOn);
	virtual void UpdateForward();
	virtual void UpdateBackward();

	virtual void Drag(double delta_x, double delta_y);
	virtual void Snap();

	virtual BOOL CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction);

// info window support
	virtual void LoadSkinInfo(CTreeCtrl* pCTree, HTREEITEM rootSkin);
	virtual void AddNodes (CTreeCtrl*pCTree, HTREEITEM rootNode);
	virtual void AddJoints(CTreeCtrl* pCTree, HTREEITEM rootJoint);

protected:
// header info
	char*				m_filename;
	long				m_ident;
	long				m_version;
	long				m_framesize;
	long				m_num_glcmds;
	long				m_num_frames;
	long				m_num_tris;
	long				m_num_xyz;
	long				m_num_st;
	long				m_num_skins;

	long				m_skinwidth;
	long				m_skinheight;
//	skin_t*				m_skins;
	CSkin*				m_skin_head;
	CSkin*				m_curSkin;
	int					m_defaultSkin;

	long				m_curFrame;
	double				m_curscale;
	long				m_curGroup;
	int					m_curstep;

	void*				m_frames;
	void*				m_stverts;
	void*				m_tris;
	long*				m_glcmds;
	long*				m_vertPath;
	D3DRMGROUPINDEX		m_group;
	int					m_pcurframe;

	unsigned long		m_frameTick;
	bool				m_nUseTicker;
	long				m_nTickerDelay;

	bool				m_pingPong;
	bool				m_playing;
	bool				m_usePunch;
	int					m_playMode;

	virtual void Init();

// used for support of frame tree view----------------------------------------------
protected:
	frameStruct			m_frameSels;		// passes back tree selections
	CTreeHead*			m_treeInfo;			// one entry for each animation group
	long				m_numGroups;		// number of treeinfo entries

public:  // called by frame tree ctrl
	virtual void LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame);
	frameStruct* GetFrameSels();
	CTreeHead* GetTreeInfo(int i);

protected:
	void StripGroupName(char *gname);
	int InGroupList(char *test, CTreeGroup* groupList, int numGroups);
//-------------------------------------------------------------------------------------

// skin texturing support

public:
//	virtual void TextureChanged();
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

// property sheet support
public:
	virtual void AddPropPages(CPropertySheet* propSheet);
	virtual void UpdateFromPropPages(CPropertySheet* propSheet);
	virtual void RemovePropPages(CPropertySheet* propSheet);
protected:
	CGeneralPropPage*			m_genPropPage;
};

