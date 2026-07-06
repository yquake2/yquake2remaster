// QuakeModel.cpp

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"
#include "Model.h"
#include "FrameManager2.h"
#include "QuakeModel.h"

//
// CQuakeModel
//

CQuakeModel::CQuakeModel()
{
}

CQuakeModel::~CQuakeModel()
{
}

void CQuakeModel::Init()
{
	m_mesh = NULL;
	m_scale.x = 0;
	m_scale.y = 0;
	m_scale.z = 0;
	m_origin.x = 0;
	m_origin.y = 0;
	m_origin.z = 0;
	m_radius = 0;
	m_eye.x = 0;
	m_eye.y = 0;
	m_eye.z = 0;
	m_sync = 0;
	m_flags = 0;
	m_size = 0;
	CModel::Init();
}

void CQuakeModel::DeleteMeshs(LPDIRECT3DRMFRAME frame)
{
	if (m_mesh != NULL)
	{
		frame->DeleteVisual(m_mesh);
		m_mesh->Release();
		m_mesh = NULL;
	}
}

void CQuakeModel::Delete()
{
	if (m_frames!=NULL)
	{
		for (int i=0;i<m_num_frames;i++)
		{
			if (((frame_t*)m_frames)[i].data!=NULL)
			{
				for (int j=0;j<((frame_t*)m_frames)[i].numgroupframes;j++)
				{
					if (((frame_t*)m_frames)[i].data[j]!=NULL) free(((frame_t*)m_frames)[i].data[j]);
				}
				free(((frame_t*)m_frames)[i].data);
			}
			if (((frame_t*)m_frames)[i].info!=NULL) free(((frame_t*)m_frames)[i].info);
			if (((frame_t*)m_frames)[i].interval!=NULL) free(((frame_t*)m_frames)[i].interval);
		}
		free(m_frames);
	}
	CModel::Delete();
}

void CQuakeModel::SetBackOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(1),
						D3DVALUE(0), D3DVALUE(-1), D3DVALUE(0) );
}

void CQuakeModel::SetFrontOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(-1), D3DVALUE(0),
						D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0) );
}

void CQuakeModel::SetLeftOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(-1), D3DVALUE(0),
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(1) );
}

void CQuakeModel::SetRightOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(-1), D3DVALUE(0),
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(-1) );
}

void CQuakeModel::SetTopOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(1),
						D3DVALUE(0), D3DVALUE(1), D3DVALUE(0) );
}

void CQuakeModel::SetBottomOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
		frame->SetOrientation( scene,
						D3DVALUE(0), D3DVALUE(0), D3DVALUE(-1),
						D3DVALUE(0), D3DVALUE(1), D3DVALUE(0) );
}

void CQuakeModel::SetInitialOrientation(LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene)
{
			frame->SetOrientation( scene,
					D3DVALUE(0), D3DVALUE(-1), D3DVALUE(0),
					D3DVALUE(-1), D3DVALUE(0), D3DVALUE(0) );
}

typedef struct
{
  long onseam;
  long s;
  long t;
} stvert_t;

void CQuakeModel::Serialize(CArchive& ar)
{
	m_curFrame = 0;
	m_curscale = -150;
	m_playing = false;

	ar >> m_ident >> m_version;
	ar.Read(&m_scale, sizeof(m_scale));
	ar.Read(&m_origin, sizeof(m_origin));
	ar.Read(&m_radius, sizeof(m_radius));
	ar.Read(&m_eye, sizeof(m_eye));
	ar >> m_num_skins >> m_skinwidth >>
		m_skinheight >> m_num_xyz >> m_num_tris >> m_num_frames >>
		m_sync >> m_flags >> m_size;
	m_num_st = m_num_xyz;
	if (m_ident != 0x4F504449)
	{
		AfxMessageBox("Invalid File");
		return;
	}

	if (m_version != 6)
	{
		AfxMessageBox("Invalid MDL Version");
		return;
	}

	CFile* file = ar.GetFile();
	AfxMessageBox("not currently reading quake models properly");
	if (true)
	{
		return;
	}
/*	file->Seek(0x54, CFile::begin);

	m_skins = (skin_t *)malloc(m_num_skins*sizeof(skin_t));
	if (m_skins==NULL)
	{
		AfxMessageBox("Could not allocate skin page");
		return;
	}
	memset(m_skins,0,m_num_skins*sizeof(skin_t));
	for (int i=0;i<m_num_skins;i++)
	{
		file->Read(&(m_skins[i].type),sizeof(long));

		if (m_skins[i].type==0)
		{
			m_skins[i].numgroupskins = 1;

			m_skins[i].skin    = (u_char **)malloc(sizeof(unsigned char *));
			if (m_skins[i].skin==NULL)
			{
				AfxMessageBox("Could not allocate skin page buffer");
				return;
			}
			m_skins[i].skin[0] = (u_char *)malloc(m_skinwidth*m_skinheight);
			if (m_skins[i].skin[0]==NULL)
			{
				AfxMessageBox("Could not do something");
				return;
			}
			memset(m_skins[i].skin[0],0,m_skinwidth*m_skinheight);

			file->Read(m_skins[i].skin[0],m_skinwidth*m_skinheight);

			m_skins[i].interval = (float *)malloc(sizeof(float));
			if (m_skins[i].interval==NULL)
			{
				AfxMessageBox("Could not allocate intervals");
				return;
			}
			*(m_skins[i].interval) = (float)0;

		}
		else
		{
			file->Read(&(m_skins[i].numgroupskins),sizeof(long));

			m_skins[i].interval = (float *)malloc(sizeof(float)*(m_skins[i].numgroupskins));
			if (m_skins[i].interval==NULL) AfxMessageBox("Could not load");
			memset(m_skins[i].interval,0,sizeof(float)*m_skins[i].numgroupskins);
			for (int j=0;j<m_skins[i].numgroupskins;j++)
			{
				file->Read(&(m_skins[i].interval[j]),sizeof(float));
			}

			m_skins[i].skin = (u_char **)malloc(sizeof(unsigned char *)*(m_skins[i].numgroupskins));
			if (m_skins[i].skin==NULL) AfxMessageBox("Could not load");
			memset(m_skins[i].skin,0,sizeof(unsigned char *)*(m_skins[i].numgroupskins));
			for (j=0;j<m_skins[i].numgroupskins;j++)
			{
				m_skins[i].skin[j] = (unsigned char *)malloc(m_skinwidth*m_skinheight);
				if (m_skins[i].skin[j]==NULL) AfxMessageBox("Could not load");
				memset(m_skins[i].skin[j],0,m_skinwidth*m_skinheight);
				file->Read(m_skins[i].skin[j],m_skinwidth*m_skinheight);
			}
		}
	}

*/	if (m_stverts != NULL)
	{
		free(m_stverts);
	}
	m_stverts = (void*)malloc(m_num_st*sizeof(stvert_t));
	if (m_stverts==NULL) AfxMessageBox("1");
	memset(m_stverts,0,m_num_st*sizeof(stvert_t));
	file->Read(m_stverts,m_num_st*sizeof(stvert_t));
	m_tris = (void*)malloc(m_num_tris*sizeof(itriangle_t));
	if (m_tris==NULL) AfxMessageBox("2");
	memset(m_tris,0,m_num_tris*sizeof(itriangle_t));
	file->Read(m_tris,m_num_tris*sizeof(itriangle_t));

	m_frames = (void*)malloc(m_num_frames*sizeof(frame_t));
	if (m_frames==NULL) AfxMessageBox("3");
	memset(m_frames,0,m_num_frames*sizeof(frame_t));

	for (int i=0;i<m_num_frames;i++)
	{
		file->Read(&(((frame_t*)m_frames)[i].type),sizeof(long));
		if (((frame_t*)m_frames)[i].type==0)
		{
			((frame_t*)m_frames)[i].numgroupframes = 1;

			((frame_t*)m_frames)[i].info = (frameinfo_t *)malloc(sizeof(frameinfo_t));
			if (((frame_t*)m_frames)[i].info==NULL) AfxMessageBox("4");

			file->Read(((frame_t*)m_frames)[i].info,sizeof(frameinfo_t));
			((frame_t*)m_frames)[i].data = (trivertx_t **)malloc(sizeof(u_char *));
			if (((frame_t*)m_frames)[i].data==NULL) AfxMessageBox("5");

			((frame_t*)m_frames)[i].data[0] = (trivertx_t *)malloc(m_num_xyz*sizeof(trivertx_t));
			if (((frame_t*)m_frames)[i].data[0]==NULL) AfxMessageBox("6");
			memset(((frame_t*)m_frames)[i].data[0],0,m_num_xyz*sizeof(trivertx_t));
			file->Read(((frame_t*)m_frames)[i].data[0],m_num_xyz*sizeof(trivertx_t));

			((frame_t*)m_frames)[i].interval = (float *)malloc(sizeof(float));
			if (((frame_t*)m_frames)[i].interval==NULL) AfxMessageBox("7");
			*(((frame_t*)m_frames)[i].interval) = (float)0;
		}
		else
		{
			file->Read(&(((frame_t*)m_frames)[i].numgroupframes),sizeof(long));
			file->Read(&(((frame_t*)m_frames)[i].groupmin),sizeof(trivertx_t));
			file->Read(&(((frame_t*)m_frames)[i].groupmax),sizeof(trivertx_t));

			((frame_t*)m_frames)[i].data = (trivertx_t **)malloc(sizeof(u_char *)*(((frame_t*)m_frames)[i].numgroupframes));
			if (((frame_t*)m_frames)[i].data==NULL) AfxMessageBox("8");
			memset(((frame_t*)m_frames)[i].data,0,sizeof(u_char *)*(((frame_t*)m_frames)[i].numgroupframes));

			((frame_t*)m_frames)[i].info = (frameinfo_t *)malloc(sizeof(frameinfo_t)*(((frame_t*)m_frames)[i].numgroupframes));
			if (((frame_t*)m_frames)[i].info==NULL) AfxMessageBox("9");
			memset(((frame_t*)m_frames)[i].info,0,sizeof(frameinfo_t)*(((frame_t*)m_frames)[i].numgroupframes));

			((frame_t*)m_frames)[i].interval = (float *)malloc(sizeof(float)*(((frame_t*)m_frames)[i].numgroupframes));
			if (((frame_t*)m_frames)[i].interval==NULL) AfxMessageBox("10");
			memset(((frame_t*)m_frames)[i].interval,0,sizeof(float)*((frame_t*)m_frames)[i].numgroupframes);
			for (int j=0;j<((frame_t*)m_frames)[i].numgroupframes;j++)
			{
				file->Read(&(((frame_t*)m_frames)[i].interval[j]),sizeof(float));
			}

			for (j=0;j<((frame_t*)m_frames)[i].numgroupframes;j++)
			{
				file->Read(&(((frame_t*)m_frames)[i].info[j]),sizeof(frameinfo_t));

				((frame_t*)m_frames)[i].data[j] = (trivertx_t *)malloc(m_num_xyz*sizeof(trivertx_t));
				if (((frame_t*)m_frames)[i].data[j]==NULL) AfxMessageBox("11");
				memset(((frame_t*)m_frames)[i].data[j],0,m_num_xyz*sizeof(trivertx_t));
				file->Read(((frame_t*)m_frames)[i].data[j],m_num_xyz*sizeof(trivertx_t));
			}
		}
	}
}

void CQuakeModel::BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC)
{
	D3DVECTOR			p[3], n1, n2, n3;
	int					i,j;
	trivertx_t			*triverts;
	int					v1, curvert;
	vec3_t				pt1;
	long				s, t;
	D3DRMVERTEX			*vertexlist;
	unsigned			*vertorder;
	unsigned			numread = 0;

	vertexlist = new D3DRMVERTEX[m_num_tris*3+1];
	if (vertexlist == NULL)
	{
		AfxMessageBox("Cannot make vertexlist");
		return;
	}

	curvert = m_num_tris * 3;

	vertorder = new unsigned[m_num_tris*3+1];
	if (vertorder == NULL)
	{
		AfxMessageBox("Cannot make vertorder");
		return;
	}

	curvert = m_num_tris * 3;

	for (i = 0; i < (m_num_tris * 3); i++)
	{
		curvert--;
		vertorder[i] = curvert;
	}

	curvert=0;

	d3drm->CreateMesh(&m_mesh);

	triverts = ((frame_t*)m_frames)[0].data[0];

	for (i = 0; i < m_num_tris; i++)
	{
		for (j = 2; j >= 0; j--)
		{
			v1 = ((itriangle_t*)m_tris)[i].vertices[j];

			pt1.x = triverts[v1].x;
			pt1.y = triverts[v1].y;
			pt1.z = triverts[v1].z;

			s = ((stvert_t*)m_stverts)[v1].s;
			t = ((stvert_t*)m_stverts)[v1].t;

			if (s > m_skinwidth)
				AfxMessageBox("Invalid skin vertex (S)");

			if (t > m_skinheight)
				AfxMessageBox("Invalid skin vertex (T)");

			if( (((stvert_t*)m_stverts)[v1].onseam) && (!((itriangle_t*)m_tris)[i].facesfront))
				s += m_skinwidth / 2;

			p[j].x = ( m_scale.x * pt1.x ) + m_origin.x;
			p[j].y = ( m_scale.y * pt1.y ) + m_origin.y;
			p[j].z = ( m_scale.z * pt1.z ) + m_origin.z;

			vertexlist[curvert].position.x = -p[j].x;
			vertexlist[curvert].position.y = -p[j].y;
			vertexlist[curvert].position.z = -p[j].z;

			vertexlist[curvert].tu = D3DVALUE(s) / m_skinwidth;
			vertexlist[curvert].tv = D3DVALUE(t) / m_skinheight;
			vertexlist[curvert].color = D3DCOLOR(0);

			if (curvert+1 < m_num_tris*3) curvert++;
		}

		D3DRMVectorSubtract(&n2, &p[2], &p[1]);
		D3DRMVectorSubtract(&n1, &p[1], &p[0]);

		D3DRMVectorCrossProduct(&n3, &n1, &n2);
		D3DRMVectorNormalize(&n3);

		for (j = 0; j < 3; j++)
		{
			vertexlist[curvert-j+1].normal.x = -n3.x;
			vertexlist[curvert-j+1].normal.y = -n3.y;
			vertexlist[curvert-j+1].normal.z = -n3.z;
		}

		numread++;
		//statusDlg->m_nProgressBar3.SetPos(numread);
	}

	m_mesh->AddGroup( m_num_tris * 3, m_num_tris, 3, vertorder, &m_group );
	m_mesh->SetVertices(m_group, 0, m_num_tris * 3, vertexlist );

//	m_mesh->SetGroupTexture(m_group, m_texture);
//	m_mesh->SetGroupMapping(m_group, D3DRMMAP_PERSPCORRECT);
//	m_mesh->SetGroupQuality(m_group, D3DRMRENDER_FLAT);

//	ddrval = frame->AddVisual(m_mesh);
//	if (ddrval != D3DRM_OK)
//		AfxMessageBox(CDDHelper::TraceError(ddrval));

	if (vertexlist) delete vertexlist;
	if (vertorder) delete vertorder;

}

BOOL CQuakeModel::ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	long			s,t;
	D3DVECTOR		p[3], v;
	int				curvert = 0;
	trivertx_t		*triverts;
	int				v1;
	D3DRMVERTEX		*vertexlist;
	int				i,j;

	vertexlist = new D3DRMVERTEX[m_num_tris*3+1];
	if (vertexlist == NULL)
		return AfxMessageBox("Cannot make vertexlist");

	m_mesh->GetVertices(m_group, 0, m_num_tris*3, vertexlist);
	triverts = ((frame_t*)m_frames)[m_curFrame].data[0];

	for (i = 0; i < m_num_tris; i++)
	{
		for (j = 2; j >= 0; j--)
		{
			v1 = ((itriangle_t*)m_tris)[i].vertices[j];

			v.x = triverts[v1].x;
			v.y = triverts[v1].y;
			v.z = triverts[v1].z;

			s = ((stvert_t*)m_stverts)[v1].s;
			t = ((stvert_t*)m_stverts)[v1].t;

			if (s > m_skinwidth)
				s = m_skinwidth;;

			if (t > m_skinheight)
				t = m_skinheight;

			if( (((stvert_t*)m_stverts)[v1].onseam) && (!((itriangle_t*)m_tris)[i].facesfront))
				s += m_skinwidth / 2;

			p[j].x = ( m_scale.x * v.x ) + m_origin.x;
			p[j].y = ( m_scale.y * v.y ) + m_origin.y;
			p[j].z = ( m_scale.z * v.z ) + m_origin.z;

			vertexlist[curvert].position.x = -p[j].x;
			vertexlist[curvert].position.y = -p[j].y;
			vertexlist[curvert].position.z = -p[j].z;

			vertexlist[curvert].tu = D3DVALUE(s) / m_skinwidth;
			vertexlist[curvert].tv = D3DVALUE(t) / m_skinheight;

			curvert++;
		}
	}
	m_mesh->SetVertices(m_group, 0, m_num_tris*3, vertexlist );

	delete (vertexlist);

	return TRUE;
}

BOOL CQuakeModel::CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction)
{
	D3DVECTOR		delta;
	D3DVECTOR		d1,d2;
	D3DVECTOR		p[3];
	int				curvert = 0;
	trivertx_t		*triverts, *triverts2;
	int				v1;
	D3DRMVERTEX		*vertexlist;
	int				lastframe, i, j;

	long			s, t;

	vertexlist = new D3DRMVERTEX[m_num_tris*3+1];
	if (vertexlist == NULL)
		return AfxMessageBox("Cannot make vertexlist");
	if (direction == ANIM_FORWARD)
	{
		if (m_curGroup == -1)
		{
			if (m_curFrame + 1 > m_num_frames-1)
				lastframe = 0;
			else
				lastframe = m_curFrame + 1;
		}
		else
		{
			if (m_curFrame + 1 > m_treeInfo[m_curGroup].eFrame)
				lastframe = m_treeInfo[m_curGroup].bFrame;
			else
				lastframe = m_curFrame + 1;
		}
	}
	//mesh->GetVertices( group, 0, mainModel.header->numtris*3, vertexlist);

	triverts  = ((frame_t*)m_frames)[m_curFrame].data[0];
	triverts2 = ((frame_t*)m_frames)[lastframe].data[0];

	for (i = 0; i < m_num_tris; i++)
	{
		for (j = 2; j >= 0; j--)
		{

			v1 = ((itriangle_t*)m_tris)[i].vertices[j];

			s = ((stvert_t*)m_stverts)[v1].s;
			t = ((stvert_t*)m_stverts)[v1].t;

			if (s > m_skinwidth)
				AfxMessageBox("Invalid skin vertex (S)");

			if (t > m_skinheight)
				AfxMessageBox("Invalid skin vertex (T)");

			if( (((stvert_t*)m_stverts)[v1].onseam) && (!((itriangle_t*)m_tris)[i].facesfront))
				s += m_skinwidth / 2;

			d1.x = triverts[v1].x;
			d1.y = triverts[v1].y;
			d1.z = triverts[v1].z;

			d2.x = triverts2[v1].x;
			d2.y = triverts2[v1].y;
			d2.z = triverts2[v1].z;

			delta.x = (d1.x - d2.x) / numsteps;
			delta.y = (d1.y - d2.y) / numsteps;
			delta.z = (d1.z - d2.z) / numsteps;

			d1.x -= delta.x * step;
			d1.y -= delta.y * step;
			d1.z -= delta.z * step;

			p[j].x = ( m_scale.x * d1.x ) + m_origin.x;
			p[j].y = ( m_scale.y * d1.y ) + m_origin.y;
			p[j].z = ( m_scale.z * d1.z ) + m_origin.z;

			vertexlist[curvert].position.x = -p[j].x;
			vertexlist[curvert].position.y = -p[j].y;
			vertexlist[curvert].position.z = -p[j].z;

			vertexlist[curvert].tu = D3DVALUE(s) / m_skinwidth;
			vertexlist[curvert].tv = D3DVALUE(t) / m_skinheight;

			curvert++;
		}
	}

	m_mesh->SetVertices(m_group, 0, m_num_tris*3, vertexlist );

	delete (vertexlist);

	if (step + 1 > numsteps) m_curstep = 0;
	else m_curstep++;

	return TRUE;
}

void CQuakeModel::FillMenuWithSkins(CMenu* menu)
{
}

void CQuakeModel::AddSkin(LPCTSTR skinname)
{
}

void CQuakeModel::ReplaceSkin(int i, LPCTSTR skinname)
{
}

CSkin* CQuakeModel::GetSkin(int i)
{
	return NULL;
}

LPDIRECT3DRMMESH CQuakeModel::GetMesh(int i)
{
	return m_mesh;
}

// info window support

void CQuakeModel::LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame)
{
	int				thisgroup = 0;
	char			name[16], *newname;
	HTREEITEM		point;
	int groupcount = 0;

	if (m_num_frames < 1) return;

	m_numGroups = 0;

	m_treeInfo = new CTreeHead[m_num_frames];
	if (m_treeInfo == NULL) AfxMessageBox("Null tree");

	CTreeGroup* groupList = new CTreeGroup[m_num_frames];
	if (groupList == NULL) AfxMessageBox("Null groupList");

	for (int i = 0; i < m_num_frames; i++)
	{
		if (((frame_t*)m_frames)[i].type == 1)
		{
			AfxMessageBox("Tricky Frames not supported!\n");
		}
		else
		{
			strcpy(name , ((frame_t*)m_frames)[i].info->name);
			newname = (char *)&name;
			StripGroupName(newname);

			if ((thisgroup = InGroupList(newname, groupList, m_numGroups)) == -1)
			{
				point = treeDlg->InsertItem(newname, rootFrame, TVI_LAST);
				groupList[thisgroup].head = point;
				treeDlg->SetItemData(point, m_numGroups);
				m_treeInfo[m_numGroups].bFrame = i;
				m_treeInfo[m_numGroups].eFrame = i;
				point = treeDlg->InsertItem(((frame_t*)m_frames)[i].info->name, point, TVI_LAST);
				treeDlg->SetItemData(point, i);
				m_numGroups++;
			}
			else
			{
				if (i > m_treeInfo[thisgroup].eFrame)
					m_treeInfo[thisgroup].eFrame = i;

				point = treeDlg->InsertItem(((frame_t*)m_frames)[i].info->name, groupList[thisgroup].head, TVI_LAST);
				treeDlg->SetItemData(point, i);
			}
			treeDlg->SetItemImage(point, 1, 1);
		}
	}
	if (groupList)
	{
		delete groupList;
		groupList = NULL;
	}

	treeDlg->Expand(rootFrame, TVE_EXPAND);
}

