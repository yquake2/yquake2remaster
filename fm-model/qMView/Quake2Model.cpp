// Quake2Model
//

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"
#include "Model.h"
#include "FrameManager2.h"
#include "Quake2Model.h"

//
// CQuake2Model
//

CQuake2Model::CQuake2Model()
{
}

CQuake2Model::~CQuake2Model()
{
}

void CQuake2Model::Init()
{
	m_mesh = NULL;
	m_skinname = NULL;
	CModel::Init();
}

void CQuake2Model::DeleteMeshs(LPDIRECT3DRMFRAME frame)
{
	if (m_mesh != NULL)
	{
		frame->DeleteVisual(m_mesh);
		m_mesh->Release();
		m_mesh = NULL;
	}
}

void CQuake2Model::Delete()
{
	if (m_skinname != NULL)
	{
		free (m_skinname);
		m_skinname = NULL;
	}
	CModel::Delete();
}

void CQuake2Model::Serialize(CArchive& ar)
{
	long				ofs_skins;	// each skin is a MAX_SKINNAME string
	long				ofs_st;		// byte offset from start for stverts
	long				ofs_tris;		// offset for dtriangles
	long				ofs_frames;	// offset for first frame
	long				ofs_glcmds;
	long				ofs_end;		// end of file
	m_curFrame = 0;
	m_curscale = -150;
	m_playing = FALSE;

	ar >> m_ident >> m_version >> m_skinwidth >> m_skinheight >>
		m_framesize >> m_num_skins >> m_num_xyz >> m_num_st >>
		m_num_tris >> m_num_glcmds >> m_num_frames >> ofs_skins >>
		ofs_st >> ofs_tris >> ofs_frames >> ofs_glcmds >>
		ofs_end;

	if (m_glcmds)
	{
		free(m_glcmds);
		m_glcmds = NULL;
	}

	CFile* file = ar.GetFile();
	file->Seek(ofs_glcmds, CFile::begin);
	m_glcmds = (long*)malloc(m_num_glcmds * sizeof(long));
	file->Read(m_glcmds, m_num_glcmds * sizeof(long));

	if (m_frames != NULL)
	{
		free(m_frames);
		m_frames = NULL;
	}
	file->Seek(ofs_frames, CFile::begin);
	m_frames = (void*)malloc(m_framesize * m_num_frames);
	file->Read(m_frames, m_framesize * m_num_frames);

	file->Seek(ofs_skins, CFile::begin);
	char tempskinname[64];
	file->Read(tempskinname, 64);
	CString skinname = tempskinname;
	int pos = skinname.ReverseFind('.');
	if (pos > -1)
	{
		skinname = skinname.Left(pos);
	}
	skinname += ".pcx";
	CString filePath = m_filename;
	int destChar = filePath.ReverseFind('\\');
	if (destChar > -1)
	{
		filePath = filePath.Left(destChar);
	}

	while(true)
	{
		destChar = skinname.Find("/");
		if (destChar < 0)
		{
			break;
		}
		skinname.SetAt(destChar, '\\');
	}

	CString namePart;
	CString partialPath = skinname;
	destChar = partialPath.ReverseFind('\\');
	if (destChar > -1)
	{
		namePart = partialPath.Right(partialPath.GetLength() - destChar);
		partialPath = partialPath.Left(destChar);
	}

	if (partialPath.CompareNoCase(filePath.Right(partialPath.GetLength())) == 0)
	{
		partialPath = filePath;
	}
	partialPath = partialPath + namePart;

	m_skinname = (char*)malloc(partialPath.GetLength() + 1);
	strcpy(m_skinname, partialPath);
}

void CQuake2Model::BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC)
{
	int realvert = 0;

	D3DRMVERTEX* vertexlist = new D3DRMVERTEX[m_num_tris*3+1];
	if (vertexlist == NULL)
	{
		AfxMessageBox("Cannot make vertexlist");
		return;
	}

	if (m_vertPath != NULL)
	{
		delete m_vertPath;
	}
	m_vertPath = new long[m_num_tris*3+1];

	int curvert = m_num_tris * 3;

	unsigned* vertorder = new unsigned[m_num_tris*3+1];
	if (vertorder == NULL)
	{
		AfxMessageBox("Cannot make vertorder");
		return;
	}

	for (int i = 0; i < (m_num_tris * 3); i++)
	{
		curvert--;
		vertorder[i] = curvert;
	}

	curvert=0;

	d3drm->CreateMesh(&m_mesh);

	vec5_t* vertlist = (vec5_t*)malloc(sizeof(vec5_t)*m_num_tris*3);

	frameinfo2_t* frameinfo1 = (frameinfo2_t*)m_frames;
	long* command = m_glcmds;
	BOOL ODD = FALSE;
	int		     num_verts;
	int			 command_type;

	//do the gl commands <?>
	while (*command)
	{
		ODD = TRUE;
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command;
		  command_type = 0;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command);
		  command_type = 1;
		}

		command++;

		for (i=0; i < num_verts; i++)
		{
		  vec5_t       p1;
		  //grab the floating point s and t
		  p1.s = (*((float *)command)) * m_skinwidth; command++;
		  p1.t = (*((float *)command)) * m_skinheight; command++;

		  //grab the vertex index
		  int vert_index = *command; command++;

		  p1.z = -((frameinfo1->verts[vert_index].x * frameinfo1->scale.x) + frameinfo1->origin.x);
		  p1.y =  ((frameinfo1->verts[vert_index].y * frameinfo1->scale.y) + frameinfo1->origin.y);
		  p1.x = -((frameinfo1->verts[vert_index].z * frameinfo1->scale.z) + frameinfo1->origin.z);

		  vertlist[i] = p1;
		}

		D3DVECTOR 	 p[3], n1, n2, n3;
		switch (command_type)
		{
			case 0:
			  //tristrip
			  for (i=0;i<num_verts-2;i++)
			  {

					for (int j = 0; j < 3; j++)
					{
						p[j].x = vertlist[i+j].x;
						p[j].y = vertlist[i+j].y;
						p[j].z = vertlist[i+j].z;
					}

					D3DRMVectorSubtract(&n1, &p[1], &p[0]);
					D3DRMVectorSubtract(&n2, &p[2], &p[1]);

					D3DRMVectorCrossProduct(&n3, &n1, &n2);
					D3DRMVectorNormalize(&n3);

					if (ODD == TRUE)
					{
						for (int j = 2; j > -1; j--)
						{
							vertexlist[curvert].position.x = vertlist[i+j].x;
							vertexlist[curvert].position.y = vertlist[i+j].y;
							vertexlist[curvert].position.z = vertlist[i+j].z;
							vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s) / m_skinwidth;
							vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t) / m_skinheight;
							vertexlist[curvert].color = D3DCOLOR(2);
							m_vertPath[curvert] = realvert+i+j;
							curvert++;
						}
						ODD = FALSE;
					}
					else
					{
						for (int j = 0; j < 3; j++)
						{
							vertexlist[curvert].position.x = vertlist[i+j].x;
							vertexlist[curvert].position.y = vertlist[i+j].y;
							vertexlist[curvert].position.z = vertlist[i+j].z;
							vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s) / m_skinwidth;
							vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t) / m_skinheight;
							vertexlist[curvert].color = D3DCOLOR(2);
							m_vertPath[curvert] = realvert+i+j;
							curvert++;
						}
						ODD = TRUE;
					}
			  }
			  break;

			case 1:
			  //trifan
			  for (i=0;i<num_verts-2;i++)
			  {
					int x;
					for (int j = 2; j > -1; j--)
					{
						if (j == 0)
							x = 0;
						else
							x = i;

						p[j].x = vertlist[x+j].x;
						p[j].y = vertlist[x+j].y;
						p[j].z = vertlist[x+j].z;
					}

					D3DRMVectorSubtract(&n1, &p[1], &p[0]);
					D3DRMVectorSubtract(&n2, &p[2], &p[1]);

					D3DRMVectorCrossProduct(&n3, &n1, &n2);
					D3DRMVectorNormalize(&n3);

					for (j = 2; j > -1; j--)
					{
						if (j == 0)
							x = 0;
						else
							x = i;

						vertexlist[curvert].position.x = vertlist[x+j].x;
						vertexlist[curvert].position.y = vertlist[x+j].y;
						vertexlist[curvert].position.z = vertlist[x+j].z;
						vertexlist[curvert].tu = D3DVALUE(vertlist[x+j].s) / m_skinwidth;
						vertexlist[curvert].tv = D3DVALUE(vertlist[x+j].t) / m_skinheight;
						vertexlist[curvert].color = D3DCOLOR(2);
						m_vertPath[curvert] = realvert+x+j;
						curvert++;
					}
			  }
			  break;
		}

		realvert += num_verts;
	}

	m_mesh->AddGroup( m_num_tris * 3, m_num_tris, 3, vertorder, &m_group );
	m_mesh->SetVertices(m_group, 0, m_num_tris * 3, vertexlist );

	frame->SetOrientation( scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(1),
				D3DVALUE(1), D3DVALUE(0), D3DVALUE(0) );

	if (vertexlist) delete vertexlist;
	if (vertorder)	delete vertorder;
	if (vertlist)	free(vertlist);
}

BOOL CQuake2Model::ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	int				curvert = 0;
	D3DRMVERTEX		*vertexlist;
	vec5_t			*vertlist;
	int				i;
	vec5_t			p1;
	frameinfo2_t	*frameinfo1;
	long			*command;
	int				num_verts,vert_index;
	int				command_type;

	vertexlist = new D3DRMVERTEX[m_num_tris*3+1];
	if (vertexlist == NULL)
		return AfxMessageBox("Cannot make vertexlist");

	vertlist = (vec5_t *)malloc(sizeof(vec5_t)*m_num_tris*3);

	frameinfo1 = (frameinfo2_t*)((char*)m_frames + m_curFrame * m_framesize);
	command = m_glcmds;

	curvert = 0;
	//do the gl commands <?>
	while (*command)
	{
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command;
		  command_type = 0;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command);
		  command_type = 1;
		}

		command++;

		for (i=0; i < num_verts; i++)
		{
			//grab the floating point s and t
			p1.s = (*((float *)command)) * m_skinwidth; command++;
			p1.t = (*((float *)command)) * m_skinheight; command++;

			//grab the vertex index
			vert_index = *command;
			command++;

			p1.z = -((frameinfo1->verts[vert_index].x * frameinfo1->scale.x) + frameinfo1->origin.x);
			p1.y =  ((frameinfo1->verts[vert_index].y * frameinfo1->scale.y) + frameinfo1->origin.y);
			p1.x = -((frameinfo1->verts[vert_index].z * frameinfo1->scale.z) + frameinfo1->origin.z);

			vertlist[curvert] = p1;
			curvert++;
		}

	}

	for (i = 0; i < m_num_tris*3; i++)
	{
		vertexlist[i].position.x = vertlist[m_vertPath[i]].x;
		vertexlist[i].position.y = vertlist[m_vertPath[i]].y;
		vertexlist[i].position.z = vertlist[m_vertPath[i]].z;
		vertexlist[i].tu = D3DVALUE(vertlist[m_vertPath[i]].s) / m_skinwidth;
		vertexlist[i].tv = D3DVALUE(vertlist[m_vertPath[i]].t) / m_skinheight;
	}

	m_mesh->SetVertices(m_group, 0, m_num_tris*3, vertexlist );

	free(vertlist);

	delete (vertexlist);

	return TRUE;
}

BOOL CQuake2Model::CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction)
{
	D3DVECTOR		delta;
	int				curvert = 0;
	D3DRMVERTEX		*vertexlist;
	int				lastframe, i;

	vec5_t			p1,p2;
	frameinfo2_t	*frameinfo1, *frameinfo2;
	long			*command;
	int				num_verts,vert_index;
	int				command_type;
	vec5_t			*vertlist;
	BOOL			ODD;

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

	curvert = 0;
	vertlist = (vec5_t *)malloc(sizeof(vec5_t)*m_num_tris*3);

	frameinfo1 = (frameinfo2_t*)((char*)m_frames + m_curFrame * m_framesize);
	frameinfo2 = (frameinfo2_t*)((char*)m_frames + lastframe * m_framesize);
	command = m_glcmds;

	//do the gl commands <?>
	while (*command)
	{
		ODD = TRUE;
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command;
		  command_type = 0;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command);
		  command_type = 1;
		}

		command++;

		for (i=0; i < num_verts; i++)
		{
		  //grab the floating point s and t
		  p1.s = (*((float *)command)) * m_skinwidth; command++;
		  p1.t = (*((float *)command)) * m_skinheight; command++;

		  //grab the vertex index
		  vert_index = *command; command++;

		  p1.z = -((frameinfo1->verts[vert_index].x * frameinfo1->scale.x) + frameinfo1->origin.x);
		  p1.y =  ((frameinfo1->verts[vert_index].y * frameinfo1->scale.y) + frameinfo1->origin.y);
		  p1.x = -((frameinfo1->verts[vert_index].z * frameinfo1->scale.z) + frameinfo1->origin.z);

		  p2.z = -((frameinfo2->verts[vert_index].x * frameinfo2->scale.x) + frameinfo2->origin.x);
		  p2.y =  ((frameinfo2->verts[vert_index].y * frameinfo2->scale.y) + frameinfo2->origin.y);
		  p2.x = -((frameinfo2->verts[vert_index].z * frameinfo2->scale.z) + frameinfo2->origin.z);

		  delta.x = (p2.x - p1.x) / numsteps;
		  delta.y = (p2.y - p1.y) / numsteps;
		  delta.z = (p2.z - p1.z) / numsteps;

		  p1.x += delta.x * step;
		  p1.y += delta.y * step;
		  p1.z += delta.z * step;

		  vertlist[curvert] = p1;
		  curvert++;
		}

	}

	for (i = 0; i < m_num_tris*3; i++)
	{
		vertexlist[i].position.x = vertlist[m_vertPath[i]].x;
		vertexlist[i].position.y = vertlist[m_vertPath[i]].y;
		vertexlist[i].position.z = vertlist[m_vertPath[i]].z;
		vertexlist[i].tu = D3DVALUE(vertlist[m_vertPath[i]].s) / m_skinwidth;
		vertexlist[i].tv = D3DVALUE(vertlist[m_vertPath[i]].t) / m_skinheight;
	}

	m_mesh->SetVertices(m_group, 0, m_num_tris*3, vertexlist );

	free(vertlist);
	delete (vertexlist);

	if (step + 1 > numsteps) m_curstep = 0;
	else m_curstep++;

	return TRUE;
}

LPDIRECT3DRMMESH CQuake2Model::GetMesh(int i)
{
	return m_mesh;
}

CSkin* CQuake2Model::GetSkin(int i)
{
	return CModel::GetSkin(m_skinname);
}

void CQuake2Model::FillMenuWithSkins(CMenu* menu)
{
	if (m_skinname != NULL)
	{
		menu->AppendMenu(MF_ENABLED | MF_UNCHECKED | MF_STRING, IDR_SKIN_SELECT_START, m_skinname);
	}
}

void CQuake2Model::AddSkin(LPCTSTR skinname)
{
}

void CQuake2Model::ReplaceSkin(int i, LPCTSTR skinname)
{
	if (m_skinname != NULL)
	{
		free(m_skinname);
		m_skinname = NULL;
	}
	m_skinname = (char*)malloc(strlen(skinname) + 1);
	strcpy(m_skinname, skinname);
}

// frame tree ctrl support

void CQuake2Model::LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame)
{
	if (m_num_frames > 0)
	{
		m_numGroups = 0;

		m_treeInfo = new CTreeHead[m_num_frames];
		if (m_treeInfo == NULL) AfxMessageBox("Null tree");

		CTreeGroup* groupList = new CTreeGroup[m_num_frames];
		if (groupList == NULL) AfxMessageBox("Null groupList");

		for (int i = 0; i < m_num_frames; i++)
		{
			frameinfo2_t* frameinfo1 = (frameinfo2_t*)((char*)m_frames + i * m_framesize);
			char	name[16];
			strcpy(name , frameinfo1->name);
			char* newname = name;
			StripGroupName(newname);
			int thisgroup = InGroupList(newname, groupList, m_numGroups);
			if (thisgroup == -1)
			{
				thisgroup = m_numGroups;
				groupList[thisgroup].head = treeDlg->InsertItem(newname, rootFrame, TVI_LAST);
				treeDlg->SetItemData(groupList[thisgroup].head, thisgroup);
				m_treeInfo[m_numGroups].bFrame = i;
				m_numGroups++;
			}
			m_treeInfo[thisgroup].eFrame = i;
			HTREEITEM item = treeDlg->InsertItem(frameinfo1->name, groupList[thisgroup].head);
			treeDlg->SetItemData(item, i);
			treeDlg->SetItemImage(item, 1, 1);
		}
		if (groupList)
		{
			delete groupList;
			groupList = NULL;
		}
		treeDlg->Expand(rootFrame, TVE_EXPAND);
	}
}

