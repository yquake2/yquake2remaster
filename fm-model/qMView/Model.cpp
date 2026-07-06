// Model.cpp

#include "stdafx.h"
#include "ddutil.h"
#include "resource.h"
#include "Matrix.h"
#include "Model.h"
#include "FlexModel.h"
#include "QuakeModel.h"
#include "Quake2Model.h"
#include "FrameManager2.h"

extern pal_t qpal[];
extern pal_t h2pal[];

//
// CModel
//

CModel::CModel()
{
}

CModel::~CModel()
{
}

void CModel::Init()
{
	m_skin_head = NULL;
	m_curSkin = NULL;
	m_defaultSkin = 0;

	m_framesize = 0;
	m_curFrame = 0;
	m_filename = NULL;
	m_curscale = -150;
	m_playing = false;
	m_num_glcmds = 0;
	m_glcmds = NULL;
	m_vertPath = NULL;
	m_num_frames = 0;
	m_frames = NULL;
	m_ident = 0;
	m_version = 0;
	m_num_skins = 0;
	m_pingPong = false;
	m_curGroup  = -1;
	m_num_st = 0;
	m_stverts = NULL;
	m_num_tris = 0;
	m_tris = NULL;
	m_num_xyz = 0;
	m_group = -1;
	m_frameTick = 0;
	m_curstep = 0;
	m_pcurframe = 0;
	m_nUseTicker = true;
	m_nTickerDelay = 100;
	m_playMode = ANIM_FORWARD;
	m_usePunch = false;
	// used by frame tree ctrl
	m_numGroups = 0;
	m_treeInfo = NULL;
	m_frameSels.frames = NULL;
	m_frameSels.numFrames = 0;
	m_frameSels.info = 0;
	m_genPropPage = NULL;
}

void CModel::SetPlayMode(int playmode)
{
	m_playMode = playmode;
}

int CModel::GetPlayMode()
{
	return m_playMode;
}

void CModel::SetTimerData(long delay, bool useTimer)
{
	m_nUseTicker = useTimer;
	m_nTickerDelay = delay;
}

void CModel::GetTimerData(long* delay, bool* useTimer)
{
	*delay = m_nTickerDelay;
	*useTimer = m_nUseTicker;
}

CBitmap* CModel::GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int skinnum, int& width, int& height)
{
	width = m_curSkin->GetWidth(d3drm, pDC);
	height = m_curSkin->GetHeight(d3drm, pDC);
	return m_curSkin->GetBitmap(d3drm, pDC);
}

void CModel::Delete()
{
	if (m_skin_head != NULL)
	{
		delete m_skin_head;
		m_skin_head = NULL;
	}
	if (m_frames)
	{
		free(m_frames);
		m_frames = NULL;
	}
	if (m_tris != NULL)
	{
		free(m_tris);
		m_tris = NULL;
	}
	if (m_stverts != NULL)
	{
		free(m_stverts);
		m_stverts = NULL;
	}
	if (m_filename != NULL)
	{
		free(m_filename);
		m_filename = NULL;
	}
	if (m_glcmds)
	{
		free(m_glcmds);
		m_glcmds = NULL;
	}

	if (m_vertPath)
	{
		delete m_vertPath;
		m_vertPath = NULL;
	}
	// used by frame tree ctrl
	if (m_treeInfo != NULL)
	{
		delete m_treeInfo;
		m_treeInfo = NULL;
	}
	if (m_frameSels.frames != NULL)
	{
		free (m_frameSels.frames);
		m_frameSels.frames = NULL;
		m_frameSels.numFrames = 0;
	}
	delete this;
}

void CModel::Drag(double delta_x, double delta_y)
{
}

void CModel::RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture)
{
	LPDIRECT3DRMMESH mesh = GetMesh();
	if (mesh == NULL)
	{
		return;
	}
	if ((useTexture) && (m_curSkin != NULL))
	{
		mesh->SetGroupTexture(m_group, m_curSkin->GetTexture(d3drm, pDC));
	}
	else
	{
		mesh->SetGroupTexture(m_group, NULL);
	}
}

HRESULT CModel::SetGroupQuality(D3DRMRENDERQUALITY value)
{
	LPDIRECT3DRMMESH mesh = GetMesh();
	if (mesh == NULL)
	{
		return -1;
	}
	return mesh->SetGroupQuality(m_group, value);
}

D3DRMRENDERQUALITY CModel::GetGroupQuality()
{
	LPDIRECT3DRMMESH mesh = GetMesh();
	if (mesh == NULL)
	{
		return -1;
	}
	return mesh->GetGroupQuality(m_group);
}

void CModel::SetPingPong(bool pingPong)
{
	m_pingPong = pingPong;
}

void CModel::Snap()
{
}

long CModel::GetNumGroups()
{
	return m_numGroups;
}

long CModel::GetNumNodes()
{
	return 0;
}

void CModel::DeSelectAll()
{
}

void CModel::ChangeVisual(LPDIRECT3DRMFRAME frame, int nodeNum)
{
}

int CModel::SelectNode(int nodeNum)
{
	return -1;
}

HTREEITEM CModel::SelectMesh(LPDIRECT3DRMVISUAL selection)
{
	return NULL;
}

bool CModel::ValidNode()
{
	return false;
}

void CModel::SetCurGroup(long group)
{
	m_curGroup = group;
}

long CModel::GetNumFrames()
{
	return m_num_frames;
}

LPCTSTR CModel::GetFilename()
{
	return m_filename;
}

void CModel::SetFilename(LPCTSTR filename)
{
	CString fileName = filename;
	while (true)
	{
		int destChar = fileName.Find("/");
		if (destChar < 0)
		{
			break;
		}
		fileName.SetAt(destChar, '\\');
	}

	if (m_filename != NULL)
	{
		free(m_filename);
	}
	m_filename = (char*)malloc(fileName.GetLength() + 1);
	strcpy(m_filename, fileName);
}

LPDIRECT3DRMMESH CModel::GetMesh(int i)
{
	return NULL;
}

void CModel::MakeAllNodesVisible(LPDIRECT3DRMFRAME frame)
{
}

bool CModel::ToggleNodeVisibility(int node)
{
	return false;
}

long CModel::GetNumTris()
{
	return m_num_tris;
}

double CModel::GetCurScale()
{
	return m_curscale;
}

void CModel::SetCurScale(double curscale)
{
	m_curscale = curscale;
}

int CModel::GetTriCount()
{
	return m_num_tris;
}

int CModel::GetCurFrame()
{
	return m_curFrame;
}

void CModel::SetCurFrame(int curframe)
{
	m_curFrame = curframe;
}

void CModel::ResetPlay()
{
	m_playing = !m_playing;
}

void CModel::SetPlay(bool playing)
{
	m_playing = playing;
}

bool CModel::Playing()
{
	return m_playing;
}

CModel* CModel::Create(LPCTSTR extension)
{
	CModel* retval;
	if (!stricmp(extension, "md2"))
	{
		retval = new CQuake2Model();
	}
	else if (!stricmp(extension, "fm"))
	{
		retval = new CFlexModel();
	}
	else
	{
		retval = new CQuakeModel();
	}
	retval->Init();
	return retval;
}

void CModel::SetBackOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(-1),
						D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0) );
}

void CModel::SetFrontOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(1),
						D3DVALUE(1), D3DVALUE(0), D3DVALUE(0) );
}

void CModel::SetLeftOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
				D3DVALUE(1), D3DVALUE(0), D3DVALUE(0),
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(-1) );
}

void CModel::SetRightOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
				D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0),
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(1) );
}

void CModel::SetTopOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
				D3DVALUE(0), D3DVALUE(1), D3DVALUE(0),
				D3DVALUE(1), D3DVALUE(0), D3DVALUE(0) );
}

void CModel::SetBottomOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
				D3DVALUE(0), D3DVALUE(1), D3DVALUE(0),
				D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0) );
}

void CModel::SetInitialOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
}

long* CModel::GetCommands()
{
	return m_glcmds;
}

void CModel::SetPunch(bool usePunch)
{
	m_usePunch = usePunch;
}

BOOL CModel::ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	return false;
}

void CModel::UpdateForward()
{
	m_curFrame++;
	int upperLimit;
	int lowerLimit;
	if (m_curGroup == -1)
	{
		upperLimit = m_num_frames - 1;
		lowerLimit = 0;
	}
	else
	{
		upperLimit = m_treeInfo[m_curGroup].eFrame;
		lowerLimit = m_treeInfo[m_curGroup].bFrame;
	}
	if (m_curFrame > upperLimit)
	{
		if (m_pingPong)
		{
			m_curFrame -= 2;
		}
		else
		{
			m_curFrame = lowerLimit;
		}
	}
}

void CModel::UpdateBackward()
{
	m_curFrame--;
	int upperLimit;
	int lowerLimit;
	if (m_curGroup == -1)
	{
		upperLimit = m_num_frames - 1;
		lowerLimit = 0;
	}
	else
	{
		upperLimit = m_treeInfo[m_curGroup].eFrame;
		lowerLimit = m_treeInfo[m_curGroup].bFrame;
	}
	if (m_curFrame < lowerLimit)
	{
		if (m_pingPong)
		{
			m_curFrame += 2;
		}
		else
		{
			m_curFrame = upperLimit;
		}
	}
}

BOOL CModel::UpdateFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, int direction, bool interOn)
{
	if (m_num_frames < 2) return TRUE;

	unsigned long thisTick = GetTickCount();

	if (m_nUseTicker)
	{
		if (thisTick < m_frameTick) return TRUE;
	}

	m_frameTick = thisTick + FRAME_TICK_INCR;

	if (interOn && m_curstep != 0)
	{
		return CalcInterpolate(d3drm, pDC, m_curstep, INTERPOLATION_STEPS, 0, direction);
	}
	else m_curstep = 1;

	if (m_usePunch)
	{
		if (m_pingPong)
		{
			if (direction == ANIM_FORWARD)
			{
				if (m_pcurframe > m_frameSels.numFrames-2)
					direction = m_playMode = ANIM_BACKWARD;
			}
			else if (direction == ANIM_BACKWARD)
			{
				if (m_pcurframe <= 0)
					direction = m_playMode = ANIM_FORWARD;
			}
		}

		if (direction == ANIM_FORWARD)
		{
			if (m_pcurframe > m_frameSels.numFrames-2)
				m_pcurframe = 0;
			else
				m_pcurframe++;

			m_curFrame = m_frameSels.frames[m_pcurframe];

			if (m_curFrame < 0)
			{
				m_curFrame = 0;
			}
		}
		else if (direction == ANIM_BACKWARD)
		{
			if (m_pcurframe < 0)
				m_pcurframe = m_frameSels.numFrames-1;
			else
				m_pcurframe--;

			m_curFrame = m_frameSels.frames[m_pcurframe];

			if (m_curFrame < 0)
			{
				m_curFrame = 0;
			}
		}
	}
	if (!m_usePunch || (m_frameSels.numFrames > 1))
	{

		if (direction == ANIM_FORWARD)
		{
			UpdateForward();
		}
		else
		{
			UpdateBackward();
		}
	}
	return ShowFrame(d3drm, pDC);
}

BOOL CModel::CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction)
{
	return false;
}

void CModel::BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC)
{
}

void CModel::DeleteMeshs(LPDIRECT3DRMFRAME frame)
{
}

void CModel::DeleteVisuals(LPDIRECT3DRMFRAME frame)
{
}

CSkin* CModel::GetSkin(int i)
{
	return NULL;
}

CSkin* CModel::GetSkin(LPCTSTR skinname)
{
	CSkin* skin = NULL;
	if (m_num_skins > 0)
	{
		if (m_skin_head == NULL)
		{
			skin = new CSkin();
			skin->SetFilename(skinname);
			m_skin_head = skin;
		}
		else
		{
			skin = m_skin_head->Find(skinname);
			if (skin == NULL)
			{
				skin = new CSkin();
				skin->SetFilename(skinname);
				m_skin_head->InsertSkin(skin);
			}
		}
	}
	return skin;
}

void CModel::FillMenuWithSkins(CMenu* menu)
{
}

void CModel::ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, int nodenum, int skinnum)
{
}

void CModel::AddSkin(LPCTSTR skinname)
{
}

void CModel::ReplaceSkin(int i, LPCTSTR skinname)
{
}

bool CModel::LoadSkin(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, int skinnum, CDC* pDC)
{
	m_defaultSkin = skinnum;
	m_curSkin = GetSkin(skinnum);
	return true;
}

void CModel::CreateJointVisuals(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame)
{
}

void CModel::Serialize(CArchive & ar)
{
}

// info window support
void CModel::AddJoints(CTreeCtrl* pCTree, HTREEITEM rootJoint)
{
}

void CModel::AddNodes(CTreeCtrl* pCTree, HTREEITEM rootNode)
{
}

void CModel::LoadSkinInfo(CTreeCtrl* pCTree, HTREEITEM rootSkin)
{
	for (int i = 0; i < m_num_skins; i++)
	{
		CString name;
		name.Format("skin%d", i);
		HTREEITEM point = pCTree->InsertItem(name, rootSkin, TVI_LAST);
		pCTree->SetItemImage(point, 0, 0);
		pCTree->SetItemData(point, i);
	}
}

// frame tree ctrl support

void CModel::LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame)
{
}

frameStruct* CModel::GetFrameSels()
{
	return &m_frameSels;
}

void CModel::StripGroupName(char *gname)
{
	int len = strlen(gname);
	char newname[16];

	memset(newname, 0, sizeof(newname));

	for (int i = 0; i < len; i++)
	{
		if (isdigit((int)gname[i])) break;
		else newname[i] = gname[i];
	}

	strcpy(gname, newname);
}

int CModel::InGroupList(char *test, CTreeGroup* groupList, int numGroups)
{
	for (int i = 0;	i < numGroups; i++)
	{
		if (!strcmp(test, groupList[i].id)) return i;
	}

	strcpy(groupList[numGroups].id, test);
	return -1;
}

CTreeHead* CModel::GetTreeInfo(int i)
{
	if (i == 0)
	{
		return m_treeInfo;
	}
	return &m_treeInfo[i];
}

// skin texturing support

//void CModel::TextureChanged()
//{
//}

void CModel::SetTransparency(LPDIRECT3DRM2 d3drm, CDC* pDC, bool trans)
{
	if (m_curSkin != NULL)
	{
		m_curSkin->GetTexture(d3drm, pDC)->SetDecalTransparency(trans);
	}
}

// joint support

void CModel::ToggleJointOn()
{
}

bool CModel::IsJointOn()
{
	return false;
}

void CModel::GetModelAngles(float* x, float* y, float* z)
{
	*x = 0;
	*y = 0;
	*z = 0;
}

void CModel::SetModelAngles(float x, float y, float z)
{
}

void CModel::GetConstraintAngleMaxs(float* x, float* y, float* z)
{
	*x = 0;
	*y = 0;
	*z = 0;
}

void CModel::GetConstraintAngleMins(float* x, float* y, float* z)
{
	*x = 0;
	*y = 0;
	*z = 0;
}

void CModel::SetConstraintAngleMaxs(float x, float y, float z)
{
}

void CModel::SetConstraintAngleMins(float x, float y, float z)
{
}

void CModel::SetCurJoint(long joint)
{
}

long CModel::GetCurJoint()
{
	return -1;
}

// property page support

void CModel::AddPropPages(CPropertySheet* propSheet)
{
	m_genPropPage = new CGeneralPropPage();
	m_genPropPage->m_frames.Format("%d", m_num_frames);
	m_genPropPage->m_glcommands.Format("%d", m_num_glcmds);
	m_genPropPage->m_groups.Format("%d", m_numGroups);
	m_genPropPage->m_skinheight.Format("%d", m_skinheight);
	m_genPropPage->m_skins.Format("%d", m_num_skins);
	m_genPropPage->m_skinwidth.Format("%d", m_skinwidth);
	m_genPropPage->m_tris.Format("%d", m_num_tris);
	m_genPropPage->m_verts.Format("%d", m_num_xyz);
	m_genPropPage->m_stverts.Format("%d", m_num_st);
	propSheet->AddPage(m_genPropPage);
}

void CModel::UpdateFromPropPages(CPropertySheet* propSheet)
{
}

void CModel::RemovePropPages(CPropertySheet* propSheet)
{
	delete m_genPropPage;
	m_genPropPage = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CGeneralPropPage property page

IMPLEMENT_DYNCREATE(CGeneralPropPage, CPropertyPage)

CGeneralPropPage::CGeneralPropPage() : CPropertyPage(CGeneralPropPage::IDD)
{
	//{{AFX_DATA_INIT(CGeneralPropPage)
	m_frames = _T("");
	m_glcommands = _T("");
	m_groups = _T("");
	m_skinheight = _T("");
	m_skins = _T("");
	m_skinwidth = _T("");
	m_tris = _T("");
	m_stverts = _T("");
	m_verts = _T("");
	//}}AFX_DATA_INIT
}

CGeneralPropPage::~CGeneralPropPage()
{
}

void CGeneralPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGeneralPropPage)
	DDX_Text(pDX, IDC_PPGEN_FRAMES, m_frames);
	DDX_Text(pDX, IDC_PPGEN_GLCOMMANDS, m_glcommands);
	DDX_Text(pDX, IDC_PPGEN_GROUPS, m_groups);
	DDX_Text(pDX, IDC_PPGEN_SKINHEIGHT, m_skinheight);
	DDX_Text(pDX, IDC_PPGEN_SKINS, m_skins);
	DDX_Text(pDX, IDC_PPGEN_SKINWIDTH, m_skinwidth);
	DDX_Text(pDX, IDC_PPGEN_TRIS, m_tris);
	DDX_Text(pDX, IDC_PPGEN_STVERTS, m_stverts);
	DDX_Text(pDX, IDC_PPGEN_VERTS, m_verts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGeneralPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGeneralPropPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeneralPropPage message handlers


//
// CSkin
//

CRGB16* CSkin::s_rgb16 = NULL;

CSkin::CSkin()
{
	m_parent = NULL;
	m_left = NULL;
	m_right = NULL;
	m_filename = NULL;
	m_bitmap = NULL;
	m_texture = NULL;
	m_bitDepth = 0;
	m_skinWidth = 0;
	m_skinHeight = 0;
	t_skinBits = NULL;
	t_textureBits = NULL;
	t_lpDDSkin = NULL;
	t_pitch = 0;
}

CSkin::~CSkin()
{
	if (t_skinBits != NULL)
	{
		free(t_skinBits);
		t_skinBits = NULL;
	}
	if (m_left != NULL)
	{
		delete m_left;
		m_left = NULL;
	}
	if (m_right != NULL)
	{
		delete m_right;
		m_right = NULL;
	}
	m_parent = NULL;
	if (m_filename != NULL)
	{
		free (m_filename);
		m_filename = NULL;
	}
	if (m_bitmap != NULL)
	{
		delete m_bitmap;
		m_bitmap = NULL;
	}
//	if (m_texture != NULL)
//	{
//		m_texture->Release();
//		m_texture = NULL;
//	}
	if (s_rgb16 != NULL)
	{
		delete s_rgb16;
		s_rgb16 = NULL;
	}
}

CSkin* CSkin::GetNext()
{
	CSkin* retval;

	if (m_left != NULL)
	{
		retval = m_left;
	}
	else if (m_right != NULL)
	{
		retval = m_right;
	}
	else
	{
		retval = m_parent;
		CSkin* curSkin = this;
		while (retval != NULL)
		{
			if ((retval->m_right != NULL) && (retval->m_right != curSkin))
			{
				break;
			}
			curSkin = retval;
			retval = retval->m_parent;
		}
	}
	if (retval != NULL)
	{
		while (retval->m_left != NULL)
		{
			retval = retval->m_left;
		}
	}
	return retval;
}

CSkin* CSkin::Find(LPCTSTR filename)
{
	CSkin* curSkin = this;
	while (curSkin != NULL)
	{
		int result = stricmp(filename, curSkin->m_filename);
		if (result == 0)
		{
			break;
		}
		if (result < 0)
		{
			curSkin = curSkin->m_left;
		}
		else
		{
			curSkin = curSkin->m_right;
		}
	}
	return curSkin;
}

CSkin* CSkin::InsertSkin(CSkin* newSkin)
{
	CSkin* parent = this;
	while(parent != NULL)
	{
		int result = stricmp(newSkin->m_filename, parent->m_filename);
		if (result == 0)
		{
			return parent;
		}
		if (result < 0)
		{
			if (parent->m_left == NULL)
			{
				parent->m_left = newSkin;
				newSkin->m_parent = parent;
				return NULL;
			}
			parent = parent->m_left;
		}
		else
		{
			if (parent->m_right == NULL)
			{
				parent->m_right = newSkin;
				newSkin->m_parent = parent;
				return NULL;
			}
			parent = parent->m_right;
		}
	}
	return this;  // impossible for properly constructed tree
}

void CSkin::SetFilename(LPCTSTR filename)
{
	CFile* file = new CFile(filename, CFile::modeRead);
	if (file == NULL)
	{
		CString message;
		message.Format("%s not found", filename);
		AfxMessageBox(message);
	}
	delete file;

	if (m_filename != NULL)
	{
		free(m_filename);
		m_filename = NULL;
	}
	m_filename = (char*)malloc(strlen(filename) + 1);
	strcpy(m_filename, filename);
}

LPCTSTR CSkin::GetFilename()
{
	return m_filename;
}

int CSkin::GetWidth(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	Cache(d3drm, pDC);
	return m_skinWidth;
}

int CSkin::GetHeight(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	Cache(d3drm, pDC);
	return m_skinHeight;
}

LPDIRECT3DRMTEXTURE CSkin::GetTexture(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	Cache(d3drm, pDC);
	return m_texture;
}

CBitmap* CSkin::GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	Cache(d3drm, pDC);
	return m_bitmap;
}

void CSkin::Cache(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	if (m_bitmap != NULL)
	{
		return;
	}
	if ((d3drm == NULL) || (pDC == NULL))
	{
		AfxMessageBox("Fatal error trying to cache skin");
	}
	int bits = pDC->GetDeviceCaps(BITSPIXEL);
	m_bitDepth = bits;
	if (bits == 16)
	{
		CreateRGB16(pDC);
	}

	CString ext;
	CString path = m_filename;
	int loc = path.ReverseFind('.');
	if (loc != -1)
	{
		ext = path.Right(path.GetLength() - loc - 1);
	}
	CFile* file = new CFile(m_filename, CFile::modeRead);
	if (file == NULL)
	{
		CString message;
		message.Format("%s not found", m_filename);
		AfxMessageBox(message);
		return;
	}
	if (!stricmp(ext, "pcx") )
	{
		if (!CacheFromPCX(file))
		{
			AfxMessageBox("Could not load PCX");
		}
	}
	else if (!stricmp(ext, "m8"))
	{
		if (!CacheFromM8(file))
		{
			AfxMessageBox("Could not load M8");
		}
	}
	else if (!stricmp(ext, "m32"))
	{
		if (!CacheFromM32(file))
		{
			AfxMessageBox("Could not load M32");
		}
	}
	else if (!stricmp(ext, "tga"))
	{
		if (!CacheFromTGA(file))
		{
			AfxMessageBox("Could not load Targa");
		}
	}
	else
	{
		AfxMessageBox("Could not load skin");
		delete file;
		return;
	}
	delete file;

	if ((m_bitmap != NULL) && (t_skinBits != NULL))
	{
		m_bitmap->SetBitmapBits(m_skinWidth * m_skinHeight * m_bitDepth / 8, t_skinBits);
		free(t_skinBits);
		t_skinBits = NULL;
	}

	if (t_lpDDSkin != NULL)
	{
		t_lpDDSkin->Unlock(NULL);
		d3drm->CreateTextureFromSurface(t_lpDDSkin, &m_texture);
		t_lpDDSkin->Release();
		t_lpDDSkin = NULL;
	}
}

void CSkin::CreateRGB16(CDC* pDC)
{
	if (s_rgb16 != NULL)
	{
		return;
	}
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	IDirectDrawSurface* surface = 0;

	// Create the primary surface.
	HRESULT hr = CDDHelper::lpDD->CreateSurface(&ddsd, &surface, 0);
	s_rgb16 = CDDHelper::CreateRGB16(surface);

	surface->Release();
}

bool CSkin::CacheFromPCX(CFile* file)
{
	file->Seek(8, CFile::begin);
	short width;
	short height;
	file->Read(&width, sizeof(width));
	file->Read(&height, sizeof(height));
	width++;
	height++;
	if (!SetBitmap(width, height))
	{
		return false;
	}

	file->Seek(file->GetLength() - 768, CFile::begin);
	pal_t palette[256];
	file->Read(palette, sizeof(pal_t) * 256);

	file->Seek(128, CFile::begin);
	byte* buf = (byte*)malloc(width);
	for (int y = 0; y < height; y++)
	{
		int x = 0;
		while (x < width)
		{
			byte curByte;
			file->Read(&curByte, 1);
			if ((curByte & 0xC0) == 0xC0)
			{
				int runLen = curByte & 0x3F;
				file->Read(&curByte, 1);
				for(; (runLen > 0) && (x < width); runLen--, x++)
				{
					buf[x] = curByte;
				}
			}
			else
			{
				buf[x] = curByte;
				x++;
			}
		}
		SetBitmapRow(y, buf, palette);
	}
	free(buf);
	return true;
}

bool CSkin::CacheFromM8(CFile* file)
{
	int ver;
	file->Read(&ver, sizeof(ver));
	char name[32];
	file->Read(name, sizeof(name));

	unsigned width[MIPLEVELS];
	unsigned height[MIPLEVELS];
	unsigned offsets[MIPLEVELS];
	int arraySize = sizeof(unsigned) * MIPLEVELS;
	if (ver <= 1)
	{
		arraySize /= 2;
	}
	file->Read(width, arraySize);
	file->Read(height, arraySize);
	file->Read(offsets, arraySize);

	char			animname[32];
	pal_t			palette[256];
	int				flags;
	int				contents;
	int				value;
	file->Read(animname, sizeof(animname));
	file->Read(palette, sizeof(palette));
	file->Read(&flags, sizeof(flags));
	file->Read(&contents, sizeof(contents));
	file->Read(&value, sizeof(value));

	if (!SetBitmap(width[0], height[0]))
	{
		return false;
	}

	file->Seek(offsets[0], CFile::begin);
	byte* buf = (byte*)malloc(width[0]);
	for (int y = 0; y < (int)height[0]; y++)
	{
		file->Read(buf, width[0]);
		SetBitmapRow(y, buf, palette);
	}
	free(buf);
	return true;
}

bool CSkin::CacheFromM32(CFile* file)
{
	int ver;
	file->Read(&ver, sizeof(ver));
	char name[128];
	file->Read(name, sizeof(name));

	if (ver >= 3)
	{
		char			animname[128];
		char			altname[128];

		file->Read(&altname, sizeof(altname));
		file->Read(&animname, sizeof(animname));
	}

	unsigned width[MIPLEVELS];
	unsigned height[MIPLEVELS];
	unsigned offsets[MIPLEVELS];
	int arraySize = sizeof(unsigned) * MIPLEVELS;
	file->Read(width, arraySize);
	file->Read(height, arraySize);
	file->Read(offsets, arraySize);

	int				flags;
	int				contents;
	int				value;

	if (ver < 3)
	{
		char			animname[32];

		file->Read(animname, sizeof(animname));
	}

	file->Read(&flags, sizeof(flags));
	file->Read(&contents, sizeof(contents));
	file->Read(&value, sizeof(value));

	float scale_x;
	float scale_y;
	file->Read(&scale_x, sizeof(scale_x));
	file->Read(&scale_y, sizeof(scale_y));

	if (ver >= 3)
	{
		int				mip_scale;
		int				unused[20];

		file->Read(&mip_scale, sizeof(mip_scale));
		file->Read(&unused, sizeof(unused));
	}

	if (!SetBitmap(width[0], height[0]))
	{
		return false;
	}

	file->Seek(offsets[0], CFile::begin);
	long* buf = (long*)malloc(width[0] * sizeof(long));
	for (int y = 0; y < (int)height[0]; y++)
	{
		file->Read(buf, width[0] * 4);
		SetBitmapRow(y, buf);
	}
	free(buf);
	return true;
}

bool CSkin::CacheFromTGA(CFile* file)
{
	// read header
	byte IDLength;
	file->Read(&IDLength, sizeof(IDLength));
	byte ColorMapType;
	file->Read(&ColorMapType, sizeof(ColorMapType));
	byte ImageType;
	file->Read(&ImageType, sizeof(ImageType));
	short CMapStart;
	file->Read(&CMapStart, sizeof(CMapStart));
	short CMapLength;
	file->Read(&CMapLength, sizeof(CMapLength));
	byte CMapDepth;
	file->Read(&CMapDepth, sizeof(CMapDepth));
	short XOffset;
	file->Read(&XOffset, sizeof(XOffset));
	short YOffset;
	file->Read(&YOffset, sizeof(YOffset));
	short width;
	file->Read(&width, sizeof(width));
	short height;
	file->Read(&height, sizeof(height));
	byte PixelDepth;
	file->Read(&PixelDepth, sizeof(PixelDepth));
	byte ImageDescriptor;
	file->Read(&ImageDescriptor, sizeof(ImageDescriptor));
	if ((ImageType != 2) && (ImageType != 10))
	{
		AfxMessageBox ("Only type 2 and 10 targa RGB images supported");
		return false;
	}
	if (ColorMapType || ((PixelDepth != 32) && (PixelDepth != 24)))
	{
		AfxMessageBox ("Only 32 or 24 bit images supported (no colormaps)");
		return false;
	}
	if (!SetBitmap(width, height))
	{
		return false;
	}

	char* id = (char*)malloc(IDLength + 1);
	id[IDLength] = '\0';
	if (IDLength > 0)
	{
		file->Read(id, IDLength);
	}
	byte* buf = (byte*)malloc(width * sizeof(long));

	if (ImageType == 2)
	{
		int srcRow;
		switch (PixelDepth)
		{
		case 24:
			srcRow = 3 * width;
			break;
		case 32:
			srcRow = 4 * width;
			break;
		}
		byte* src = (byte*)malloc(srcRow);
		for(int y = 0; y < height; y++)
		{
			file->Read(src, srcRow);
			byte* srcPtr = src;
			byte* destPtr = buf;
			for(int x = 0; x < width; x++)
			{
				switch (PixelDepth)
				{
				case 24:
					*destPtr++ = srcPtr[2];
					*destPtr++ = srcPtr[1];
					*destPtr++ = srcPtr[0];
					*destPtr++ = 255;
					srcPtr += 3;
					break;
				case 32:
					*destPtr++ = srcPtr[2];
					*destPtr++ = srcPtr[1];
					*destPtr++ = srcPtr[0];
					*destPtr++ = srcPtr[3];
					srcPtr += 4;
					break;
				}
			}
			SetBitmapRow(height - y - 1, (long*)buf);
		}
		free(src);
	}
	else if (ImageType == 10)
	{
		byte packetHeader;
		int packetSize = 0;
		byte blue = 0;
		byte green = 0;
		byte red = 0;
		byte alpha = 255;
		bool readEachByte = true;
		for(int y = 0; y < height; y++)
		{
			byte* destPtr = buf;
			for(int x = 0; x < width; )
			{
				while(packetSize > 0)
				{
					if (readEachByte)
					{
						file->Read(&blue, 1);
						file->Read(&green, 1);
						file->Read(&red, 1);
						if (PixelDepth == 32)
						{
							file->Read(&alpha, 1);
						}
						else
						{
							alpha = 255;
						}
					}
					*destPtr++ = red;
					*destPtr++ = green;
					*destPtr++ = blue;
					*destPtr++ = alpha;
					x++;
					packetSize--;
					if(x == width)
					{
						break;
					}
				}
				if (x == width)
				{
					break;
				}
				file->Read(&packetHeader, 1);
				packetSize = 1 + (packetHeader & 0x7f);
				readEachByte = !(packetHeader & 0x80);
				if (!readEachByte)
				{									// run-length packet
					file->Read(&blue, 1);
					file->Read(&green, 1);
					file->Read(&red, 1);
					if (PixelDepth == 32)
					{
						file->Read(&alpha, 1);
					}
					else
					{
						alpha = 255;
					}
				}
			}
			SetBitmapRow(height - y - 1, (long*)buf);
		}
	}
	free(buf);
	free(id);
	return true;
}

bool CSkin::SetBitmap(int width, int height)
{
	if (((width != 1024) && (width != 512) && (width != 256) && (width != 128) &&
		(width != 64) && (width != 32) && (width != 16) && (width != 8)) ||
		((height != 1024) && (height != 512) && (height != 256) && (height != 128) &&
		(height != 64) && (height != 32) && (height != 16) && (height != 8)))
	{
		AfxMessageBox("Improper skin dimensions");
		return false;
	}
	// create surface and lockdown buffer
	DDSURFACEDESC	ddsd;
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsd.ddpfPixelFormat.dwRBitMask = 0xff0000;
	ddsd.ddpfPixelFormat.dwGBitMask = 0xff00;
	ddsd.ddpfPixelFormat.dwBBitMask = 0xff;
	ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

	if (CDDHelper::lpDD->CreateSurface(&ddsd, &t_lpDDSkin, NULL) != DD_OK)
	{
		AfxMessageBox("Fail on CreateSurface 1");
		return false;
	}

	ddsd.dwSize = sizeof(ddsd);
	t_lpDDSkin->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
	t_textureBits = (byte*)ddsd.lpSurface;
	t_pitch = ddsd.lPitch;

	if (m_bitmap != NULL)
	{
		delete m_bitmap;
		m_bitmap = NULL;
	}
	if (t_skinBits != NULL)
	{
		free(t_skinBits);
		t_skinBits = NULL;
	}
	m_skinWidth = width;
	m_skinHeight = height;

	// create bitmap and buffer
	m_bitmap = new CBitmap();
	m_bitmap->CreateBitmap(width, height, 1, m_bitDepth, NULL);
	t_skinBits = (byte*)malloc(width * height * m_bitDepth / 8);
	return true;
}

void CSkin::SetBitmapRow(int row, byte* buf, pal_t* palette)
{
	if (t_skinBits == NULL)
	{
		return;
	}
	byte* base = (byte*)(t_skinBits + (row * m_skinWidth * m_bitDepth / 8));
	byte* src = buf;
	int x;
	switch (m_bitDepth)
	{
	case 16:
		if (s_rgb16 == NULL)
		{
			return;
		}
		for (x = 0; x < m_skinWidth; x++)
		{
			((short*)base)[x] = (short)(((long)(palette[src[x]].r >> 3) << s_rgb16->Position.rgbRed) |
				((long)(palette[src[x]].g >> 3) << s_rgb16->Position.rgbGreen) |
				((long)(palette[src[x]].b >> 3) << s_rgb16->Position.rgbBlue));
		}
		break;
	case 24:
		for (x = 0; x < m_skinWidth; x++)
		{
			*base++ = palette[src[x]].b;
			*base++ = palette[src[x]].g;
			*base++ = palette[src[x]].r;
		}
		break;
	case 32:
		for (x = 0; x < m_skinWidth; x++)
		{
			*base++ = palette[src[x]].b;
			*base++ = palette[src[x]].g;
			*base++ = palette[src[x]].r;
			*base++ = 0;
		}
		break;
	}

	// fill in texture

	src = (byte*)buf;
	base = (byte*)(t_textureBits + (row * t_pitch));
	for (x = 0; x < m_skinWidth; x++)
	{
		base[2] = palette[src[x]].r;
		base[1] = palette[src[x]].g;
		base[0] = palette[src[x]].b;
		base[3] = 0;
		base += 4;
	}
}

void CSkin::SetBitmapRow(int row, long* buf)
{
	if (t_skinBits == NULL)
	{
		return;
	}
	byte* base = (byte*)(t_skinBits + (row * m_skinWidth * m_bitDepth / 8));
	byte* src = (byte*)buf;
	int x;
	switch (m_bitDepth)
	{
	case 16:
		if (s_rgb16 == NULL)
		{
			return;
		}
		for (x = 0; x < m_skinWidth; x++)
		{
			((short*)base)[x] = (short)(((long)(src[0] >> 3) << s_rgb16->Position.rgbRed) |
				((long)(src[1] >> 3) << s_rgb16->Position.rgbGreen) |
				((long)(src[2] >> 3) << s_rgb16->Position.rgbBlue));
			src += 4;
		}
		break;
	case 24:
		for (x = 0; x < m_skinWidth; x++)
		{
			*base++ = src[2];
			*base++ = src[1];
			*base++ = src[0];
			src += 4;
		}
		break;
	case 32:
		for (x = 0; x < m_skinWidth; x++)
		{
			*base++ = src[2];
			*base++ = src[1];
			*base++ = src[0];
			*base++ = src[3];
			src += 4;
		}
		break;
	}

	// fill in texture

	src = (byte*)buf;
	base = (byte*)(t_textureBits + (row * t_pitch));
	for (x = 0; x < m_skinWidth; x++)
	{
		base[2] = *src++;
		base[1] = *src++;
		base[0] = *src++;
		base[3] = *src++;
		base += 4;
	}
}
