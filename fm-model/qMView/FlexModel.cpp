// FlexModel.cpp
//

#include "stdafx.h"
#include "vector.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"
#include "Model.h"
#include "FrameManager2.h"
#include "FlexModel.h"


//
// CNode
//

void CNode::Init()
{
	m_group = 0;
	m_treeitem = NULL;
	m_mesh = NULL;
	m_visible = true;
	m_numTris = 0;
	m_numVerts = 0;
	m_vertPath = NULL;
	m_tris = NULL;
	m_verts = NULL;
	m_start_glcmds = NULL;
	m_num_glcmds = 0;
	m_defaultSkin = 0;
	m_curSkin = NULL;
}

void CNode::Delete()
{
	DeleteMesh();
	if (m_tris != NULL)
	{
		free(m_tris);
		m_tris = NULL;
	}
	if (m_verts != NULL)
	{
		free(m_verts);
		m_verts = NULL;
	}
	if (m_vertPath != NULL)
	{
		delete m_vertPath;
		m_vertPath = NULL;
	}
}

void CNode::DeleteMesh()
{
	if (m_mesh != NULL)
	{
		m_mesh->Release();
		m_mesh = NULL;
	}
}

LPDIRECT3DRMMESH CNode::GetMesh()
{
	return m_mesh;
}

CBitmap* CNode::GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int& width, int& height)
{
	width = m_curSkin->GetWidth(d3drm, pDC);
	height = m_curSkin->GetHeight(d3drm, pDC);
	return m_curSkin->GetBitmap(d3drm, pDC);
}

void CNode::ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin)
{
	if (m_curSkin == skin)
	{
		return;
	}
//	if (m_curSkin != NULL)
//	{
//		m_curSkin->GetTexture(d3drm, pDC)->Release();
//	}
	m_mesh->SetGroupTexture(m_group, skin->GetTexture(d3drm, pDC));
	m_mesh->SetGroupMapping(m_group, D3DRMMAP_PERSPCORRECT);
	m_mesh->SetGroupQuality(m_group, D3DRMRENDER_FLAT);
	m_curSkin = skin;
}

void CNode::LoadSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin, int skinnum)
{
//	if (m_defaultSkin == skinnum)
	{
		if (m_curSkin != skin)
		{
			ChangeSkin(d3drm, pDC, skin);
		}
	}
}

bool CNode::IsVisible()
{
	return m_visible;
}

void CNode::SetVisible(bool visible)
{
	m_visible = visible;
}

int CNode::GetNumTris()
{
	return m_numTris;
}

void CNode::SetTris(byte* buf, int numTris)
{
	m_numTris = numTris;
	m_tris = (byte*)malloc(numTris);
	memcpy(m_tris, buf, numTris);
}

void CNode::SetVerts(byte* buf, int numVerts)
{
	m_numVerts = numVerts;
	m_verts = (byte*)malloc(numVerts);
	memcpy(m_verts, buf, numVerts);
}

void CNode::SetGlcmds(long* glcmds, int num)
{
	m_start_glcmds = glcmds;
	m_num_glcmds = num;
}

void CNode::RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture)
{
	if (useTexture)
	{
		m_mesh->SetGroupTexture(m_group, m_curSkin->GetTexture(d3drm, pDC));
	}
	else
	{
		m_mesh->SetGroupTexture(m_group, NULL);
	}
}

void CNode::SetTransparency(LPDIRECT3DRM2 d3drm, CDC* pDC, bool trans)
{
	m_curSkin->GetTexture(d3drm, pDC)->SetDecalTransparency(trans);
}

HRESULT CNode::SetQuality(D3DRMRENDERQUALITY value)
{
	return m_mesh->SetGroupQuality(m_group, value);
}

HRESULT CNode::SetColorRGB(D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
	return m_mesh->SetGroupColorRGB(m_group, red, green, blue);
}

void CNode::BuildMesh(LPDIRECT3DRM2 d3drm, frameinfo2_t* frameinfo1, int skinwidth, int skinheight)
{
	D3DRMVERTEX* vertexlist = new D3DRMVERTEX[m_numTris * 3 + 1];
	if (vertexlist == NULL)
	{
		AfxMessageBox("Cannot make vertexlist");
		return;
	}

	int curvert=0;

	d3drm->CreateMesh(&m_mesh);

	vec5_t* vertlist = (vec5_t*)malloc(sizeof(vec5_t) * m_numTris * 3);
	m_vertPath = new int[m_numTris * 3];

	long* command = m_start_glcmds;
	int cur_glcmnd = m_num_glcmds;

	//do the gl commands
	int	realvert = 0;
	int realTris = 0;
	while (*command && cur_glcmnd)
	{
		cur_glcmnd--;

		int num_verts;
		int command_type;
		bool odd = true;

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

		for (int i = 0; i < num_verts; i++)
		{
		  vec5_t       p1;
		  //grab the floating point s and t
		  p1.s = (*((float *)command)) * skinwidth; command++;
		  p1.t = (*((float *)command)) * skinheight; command++;

		  //grab the vertex index
		  int vert_index = *command; command++;

		  p1.z = -((frameinfo1->verts[vert_index].x * frameinfo1->scale.x) + frameinfo1->origin.x);
		  p1.y =  ((frameinfo1->verts[vert_index].y * frameinfo1->scale.y) + frameinfo1->origin.y);
		  p1.x = -((frameinfo1->verts[vert_index].z * frameinfo1->scale.z) + frameinfo1->origin.z);

		  vertlist[i] = p1;
		}

		D3DVECTOR 	 p[3];
		switch (command_type)
		{
			case 0:
			  //tristrip
			  for (i = 0;i < num_verts - 2; i++)
			  {

					for (int j = 0; j < 3; j++)
					{
						p[j].x = vertlist[i+j].x;
						p[j].y = vertlist[i+j].y;
						p[j].z = vertlist[i+j].z;
					}

					if (odd)
					{
						for (j = 2; j > -1; j--)
						{
							vertexlist[curvert].position.x = vertlist[i+j].x;
							vertexlist[curvert].position.y = vertlist[i+j].y;
							vertexlist[curvert].position.z = vertlist[i+j].z;
							vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s) / skinwidth;
							vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t) / skinheight;
							m_vertPath[curvert] = realvert+i+j;
							curvert++;
						}
						realTris++;
					}
					else
					{
						for (j = 0; j < 3; j++)
						{
							vertexlist[curvert].position.x = vertlist[i+j].x;
							vertexlist[curvert].position.y = vertlist[i+j].y;
							vertexlist[curvert].position.z = vertlist[i+j].z;
							vertexlist[curvert].tu = D3DVALUE(vertlist[i+j].s) / skinwidth;
							vertexlist[curvert].tv = D3DVALUE(vertlist[i+j].t) / skinheight;
							m_vertPath[curvert] = realvert+i+j;
							curvert++;
						}
						realTris++;
					}
					odd = !odd;
			  }
			  break;

			case 1:
			  //trifan
			  for (i = 0; i < num_verts - 2; i++)
			  {
					for (int j = 2; j > -1; j--)
					{
						int x;
						if (j == 0)
							x = 0;
						else
							x = i;

						p[j].x = vertlist[x+j].x;
						p[j].y = vertlist[x+j].y;
						p[j].z = vertlist[x+j].z;
					}

					for (j = 2; j > -1; j--)
					{
						int x;
						if (j == 0)
							x = 0;
						else
							x = i;

						vertexlist[curvert].position.x = vertlist[x+j].x;
						vertexlist[curvert].position.y = vertlist[x+j].y;
						vertexlist[curvert].position.z = vertlist[x+j].z;
						vertexlist[curvert].tu = D3DVALUE(vertlist[x+j].s) / skinwidth;
						vertexlist[curvert].tv = D3DVALUE(vertlist[x+j].t) / skinheight;
						m_vertPath[curvert] = realvert + x + j;
						curvert++;
					}
					realTris++;
			  }
			  break;
		}

		realvert += num_verts;
	}

    int tempvert = curvert;

	unsigned* vertorder = new unsigned[curvert + 1];
	if (vertorder == NULL)
	{
		AfxMessageBox("Cannot make vertorder");
		return;
	}

	for (int i = 0; i < (curvert); i++)
	{
		tempvert--;
		vertorder[i] = tempvert;
	}

	m_numTris = realTris;
	m_numVerts = curvert;

	m_mesh->AddGroup(curvert, realTris, 3, vertorder, &m_group);
	m_mesh->SetVertices(m_group, 0, curvert, vertexlist );

	if (vertexlist) delete vertexlist;
	if (vertorder)	delete vertorder;
	if (vertlist)	free(vertlist);

	return;
}

void CNode::SetSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, CSkin* skin, int skinnum)
{
	m_defaultSkin = skinnum;
	ChangeSkin(d3drm, pDC, skin);
}

void CNode::AddVisual(LPDIRECT3DRMFRAME frame)
{
	HRESULT ddrval = frame->AddVisual(m_mesh);
	if (ddrval != D3DRM_OK)
	{
		AfxMessageBox(CDDHelper::TraceError(ddrval));
		return;
	}
}

bool CNode::ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, vec3a_t* rotVerts, frameinfo2_t* frameinfo1)
{

	D3DRMVERTEX* vertexlist = new D3DRMVERTEX[m_numTris * 3 + 1];
	if (vertexlist == NULL)
	{
		AfxMessageBox("Cannot make vertexlist");
		return false;
	}

	vec5_t* vertlist = (vec5_t*)malloc(sizeof(vec5_t) * m_numTris * 3);

	long* command = m_start_glcmds;
	int cur_glcmnd = m_num_glcmds;

	int	realvert = 0;
	int skinwidth = m_curSkin->GetWidth(d3drm, pDC);
	int skinheight = m_curSkin->GetHeight(d3drm, pDC);

	//do the gl commands <?>
	while (*command && cur_glcmnd)
	{
		cur_glcmnd--;

		int num_verts;
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command);
		}
		command++;

		vec5_t	p1;
		for (int i=0; i < num_verts; i++)
		{
		  //grab the floating point s and t
		  p1.s = (*((float *)command)) * skinwidth; command++;
		  p1.t = (*((float *)command)) * skinheight; command++;

		  //grab the vertex index
		  int vert_index = *command; command++;

		  p1.z = -((rotVerts[vert_index][0] * frameinfo1->scale.x) + frameinfo1->origin.x);
		  p1.y =  ((rotVerts[vert_index][1] * frameinfo1->scale.y) + frameinfo1->origin.y);
		  p1.x = -((rotVerts[vert_index][2] * frameinfo1->scale.z) + frameinfo1->origin.z);

		  vertlist[realvert] = p1;
		  realvert++;
		}
	}

	for (int i = 0; i < m_numVerts; i++)
	{
		vertexlist[i].position.x = vertlist[m_vertPath[i]].x;
		vertexlist[i].position.y = vertlist[m_vertPath[i]].y;
		vertexlist[i].position.z = vertlist[m_vertPath[i]].z;
		vertexlist[i].tu = D3DVALUE(vertlist[m_vertPath[i]].s) / skinwidth;
		vertexlist[i].tv = D3DVALUE(vertlist[m_vertPath[i]].t) / skinheight;
	}

	m_mesh->SetVertices(m_group, 0, m_numVerts, vertexlist);

	free(vertlist);
	delete (vertexlist);

	return true;
}

void CNode::ShowInterFrame(LPDIRECT3DRM2 d3drm, CDC* pDC, frameinfo2_t* frameinfo1, frameinfo2_t* frameinfo2, int step, int numsteps)
{
	D3DRMVERTEX* vertexlist = new D3DRMVERTEX[m_numTris * 3 + 1];
	if (vertexlist == NULL)
	{
		AfxMessageBox("Cannot make vertexlist");
		return;
	}

	vec5_t* vertlist = (vec5_t*)malloc(sizeof(vec5_t) * m_numTris * 3);

	long* command = m_start_glcmds;
	int cur_glcmnd = m_num_glcmds;
	int	realvert = 0;
	int skinwidth = m_curSkin->GetWidth(d3drm, pDC);
	int skinheight = m_curSkin->GetHeight(d3drm, pDC);

	//do the gl commands <?>
	while (*command && cur_glcmnd)
	{
		cur_glcmnd--;

		int	num_verts;
		if (*command>0)
		{
		  //triangle strip
		  num_verts = *command;
		}
		else
		{
		  //triangle fan
		  num_verts = -(*command);
		}

		command++;

		for (int i = 0; i < num_verts; i++)
		{
			vec5_t p1;
			vec5_t p2;
			//grab the floating point s and t
			p1.s = (*((float *)command)) * skinwidth; command++;
			p1.t = (*((float *)command)) * skinheight; command++;

			//grab the vertex index
			long vert_index = *command; command++;

			p1.z = -((frameinfo1->verts[vert_index].x * frameinfo1->scale.x) + frameinfo1->origin.x);
			p1.y =  ((frameinfo1->verts[vert_index].y * frameinfo1->scale.y) + frameinfo1->origin.y);
			p1.x = -((frameinfo1->verts[vert_index].z * frameinfo1->scale.z) + frameinfo1->origin.z);

			p2.z = -((frameinfo2->verts[vert_index].x * frameinfo2->scale.x) + frameinfo2->origin.x);
			p2.y =  ((frameinfo2->verts[vert_index].y * frameinfo2->scale.y) + frameinfo2->origin.y);
			p2.x = -((frameinfo2->verts[vert_index].z * frameinfo2->scale.z) + frameinfo2->origin.z);

			D3DVECTOR delta;
			delta.x = (p2.x - p1.x) / numsteps;
			delta.y = (p2.y - p1.y) / numsteps;
			delta.z = (p2.z - p1.z) / numsteps;

			p1.x += delta.x * step;
			p1.y += delta.y * step;
			p1.z += delta.z * step;

			vertlist[realvert] = p1;
			realvert++;
		}
	}

	for (int i = 0; i < m_numVerts; i++)
	{
		vertexlist[i].position.x = vertlist[m_vertPath[i]].x;
		vertexlist[i].position.y = vertlist[m_vertPath[i]].y;
		vertexlist[i].position.z = vertlist[m_vertPath[i]].z;
		vertexlist[i].tu = D3DVALUE(vertlist[m_vertPath[i]].s) / skinwidth;
		vertexlist[i].tv = D3DVALUE(vertlist[m_vertPath[i]].t) / skinheight;
	}

	m_mesh->SetVertices(m_group, 0, m_numVerts, vertexlist);

	free(vertlist);
	delete (vertexlist);
}

//
// CFlexModel
//
bool CFlexModel::LoadSkin(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, int skinnum, CDC* pDC)
{
	CSkin* skin = GetSkin(skinnum);
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].LoadSkin(d3drm, pDC, skin, skinnum);
	}
	return true;
}

void CFlexModel::CreateJointVisuals(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame)
{
	if (m_numClusters)
	{
		for (int i = 0; i < m_numClusters; i++)
		{
			CreateJointVisual(d3drm, m_skeletons[0].rootJoint[i].model, i, frame);
		}
	}
}

void CFlexModel::CreateJointVisual(LPDIRECT3DRM2 d3drm, Placement_t model, int jointNum, LPDIRECT3DRMFRAME frame)
{
	D3DRMVERTEX			vertexlist[12];
	unsigned			vertorder[24];

	frameinfo2_t *frameinfo1 = (frameinfo2_t *)m_frames;

	vertexlist[0].position.z = -(((model.origin[0] - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((model.origin[0] + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((model.origin[2] + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((model.origin[2] - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((model.origin[1] + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((model.origin[1] - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertorder[0] = 1;
	vertorder[1] = 4;
	vertorder[2] = 0;

	vertorder[3] = 4;
	vertorder[4] = 3;
	vertorder[5] = 0;

	vertorder[6] = 1;
	vertorder[7] = 2;
	vertorder[8] = 4;

	vertorder[9] = 2;
	vertorder[10] = 3;
	vertorder[11] = 4;

	//////////////////

	vertorder[12] = 3;
	vertorder[13] = 5;
	vertorder[14] = 0;

	vertorder[15] = 5;
	vertorder[16] = 1;
	vertorder[17] = 0;

	vertorder[18] = 3;
	vertorder[19] = 2;
	vertorder[20] = 5;

	vertorder[21] = 2;
	vertorder[22] = 1;
	vertorder[23] = 5;

	d3drm->CreateMesh(&m_joint_visuals[jointNum].origin);

	m_joint_visuals[jointNum].origin->AddGroup( 24, 8, 3, vertorder, &m_group );
	m_joint_visuals[jointNum].origin->SetVertices(m_group, 0, 23, vertexlist );

	m_joint_visuals[jointNum].origin->SetGroupQuality(m_group, D3DRMRENDER_FLAT );
	m_joint_visuals[jointNum].origin->SetGroupColorRGB(m_group, D3DVALUE( 255 ), D3DVALUE( 0 ), D3DVALUE( 0 ));

	HRESULT ddrval = frame->AddVisual(m_joint_visuals[jointNum].origin);
	if (ddrval != D3DRM_OK)
		AfxMessageBox(CDDHelper::TraceError(ddrval));

	/**************************************************************************/

	D3DVECTOR dir, up, org;

	dir.x = model.direction[0];
	dir.y = model.direction[1];
	dir.z = model.direction[2];

	up.x = model.up[0];
	up.y = model.up[1];
	up.z = model.up[2];

	org.x = model.origin[0];
	org.y = model.origin[1];
	org.z = model.origin[2];

	D3DRMVectorSubtract(&dir, &dir, &org);
	D3DRMVectorNormalize(&dir);

	dir.x *= 5;
	dir.y *= 5;
	dir.z *= 5;

	dir.x += org.x;
	dir.y += org.y;
	dir.z += org.z;

	D3DRMVectorSubtract(&up, &up, &org);
	D3DRMVectorNormalize(&up);

	up.x *= 5;
	up.y *= 5;
	up.z *= 5;

	up.x += org.x;
	up.y += org.y;
	up.z += org.z;

	vertexlist[0].position.z = -(((dir.x - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((dir.x + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((dir.z + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((dir.z - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((dir.y + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((dir.y - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	d3drm->CreateMesh(&m_joint_visuals[jointNum].dir);

	m_joint_visuals[jointNum].dir->AddGroup( 24, 8, 3, vertorder, &m_group );
	m_joint_visuals[jointNum].dir->SetVertices(m_group, 0, 23, vertexlist );

	m_joint_visuals[jointNum].dir->SetGroupQuality(m_group, D3DRMRENDER_FLAT );
	m_joint_visuals[jointNum].dir->SetGroupColorRGB(m_group, D3DVALUE( 0 ), D3DVALUE( 255 ), D3DVALUE( 0 ));

	ddrval = frame->AddVisual(m_joint_visuals[jointNum].dir );
	if (ddrval != D3DRM_OK)
		AfxMessageBox(CDDHelper::TraceError(ddrval));

	/**************************************************************************/

	vertexlist[0].position.z = -(((up.x - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((up.x + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((up.z + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((up.z - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((up.y + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((up.y - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	d3drm->CreateMesh(&m_joint_visuals[jointNum].up);

	m_joint_visuals[jointNum].up->AddGroup( 24, 8, 3, vertorder, &m_group );
	m_joint_visuals[jointNum].up->SetVertices(m_group, 0, 23, vertexlist );

	m_joint_visuals[jointNum].up->SetGroupQuality(m_group, D3DRMRENDER_FLAT );
	m_joint_visuals[jointNum].up->SetGroupColorRGB(m_group, D3DVALUE( 0 ), D3DVALUE( 0 ), D3DVALUE( 255 ));

	ddrval = frame->AddVisual(m_joint_visuals[jointNum].up );
	if (ddrval != D3DRM_OK)
		AfxMessageBox(CDDHelper::TraceError(ddrval));
}

CFlexModel::CFlexModel()
{
}

CFlexModel::~CFlexModel()
{
}

void CFlexModel::Init()
{
	m_compdata.start_frame =0;
	m_compdata.num_frames = 0;
	m_compdata.degrees = 0;
	m_compdata.mat = NULL;
	m_compdata.ccomp = NULL;
	m_compdata.cbase = NULL;
	m_compdata.cscale = NULL;
	m_compdata.coffset = NULL;
	m_compdata.complerp = NULL;
	m_compdata.trans[0] = 0;
	m_compdata.scale[0] = 0;
	m_compdata.bmin[0] = 0;
	m_compdata.bmax[0] = 0;
	m_compdata.trans[1] = 0;
	m_compdata.scale[1] = 0;
	m_compdata.bmin[1] = 0;
	m_compdata.bmax[1] = 0;
	m_compdata.trans[2] = 0;
	m_compdata.scale[2] = 0;
	m_compdata.bmin[2] = 0;
	m_compdata.bmax[2] = 0;

	m_curFrame = 0;
	m_framenames = NULL;

	m_lightnormalindex = NULL;
	m_skeletalType = 0;
	m_skeletons = NULL;
	m_referenceType = -1;
	for (int i = 0; i < JOINT_VISUALS; i++)
	{
		m_joint_visuals[i].origin = NULL;
		m_joint_visuals[i].dir = NULL;
		m_joint_visuals[i].up = NULL;
	}

	m_num_mesh_nodes = 0;
	m_curNode = -1;
	m_nodes = NULL;

	m_skin_names = NULL;
	CModel::Init();
// joint support
	m_jointsOn = false;
	m_curJoint = -1;
	m_jointConstraintAngles = NULL;
	m_modelJointAngles = NULL;
	m_skeletalClusters = NULL;
	m_numClusters = 0;
}

void CFlexModel::DeleteMeshs(LPDIRECT3DRMFRAME frame)
{
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].DeleteMesh();
	}
	m_curNode = -1;
}

void CFlexModel::DeleteVisuals(LPDIRECT3DRMFRAME frame)
{
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		LPDIRECT3DRMMESH mesh = m_nodes[i].GetMesh();
		if (mesh != NULL)
		{
			frame->DeleteVisual(mesh);
		}
	}
	for (i = 0; i < m_numClusters; i++)
	{
		if (m_joint_visuals[i].origin != NULL)
		{
			frame->DeleteVisual(m_joint_visuals[i].origin);
			m_joint_visuals[i].origin->Release();
			m_joint_visuals[i].origin = NULL;
		}
		if (m_joint_visuals[i].dir != NULL)
		{
			frame->DeleteVisual(m_joint_visuals[i].dir);
			m_joint_visuals[i].dir->Release();
			m_joint_visuals[i].dir = NULL;
		}
		if (m_joint_visuals[i].up != NULL)
		{
			frame->DeleteVisual(m_joint_visuals[i].up);
			m_joint_visuals[i].up->Release();
			m_joint_visuals[i].up = NULL;
		}
	}
}

void CFlexModel::Delete()
{
	if (m_skin_names)
	{
		for (int i = 0; i < m_num_skins; i++)
		{
			free(m_skin_names[i]);
			m_skin_names[i] = NULL;
		}
		free (m_skin_names);
		m_skin_names = NULL;
	}

	if (m_nodes != NULL)
	{
		for (int i = 0; i < m_num_mesh_nodes; i++)
		{
			m_nodes[i].Delete();
		}
		delete m_nodes;
		m_nodes = NULL;
	}
	if (m_compdata.mat)
	{
		free (m_compdata.mat);
		m_compdata.mat = NULL;
	}

	if (m_compdata.ccomp)
	{
		free (m_compdata.ccomp);
		m_compdata.ccomp = NULL;
	}

	if (m_compdata.cbase)
	{
		free (m_compdata.cbase);
		m_compdata.cbase = NULL;
	}

	if (m_compdata.cscale)
	{
		free (m_compdata.cscale);
		m_compdata.cscale = NULL;
	}

	if (m_compdata.coffset)
	{
		free (m_compdata.coffset);
		m_compdata.coffset = NULL;
	}

	if (m_compdata.complerp)
	{
		free (m_compdata.complerp);
		m_compdata.complerp = NULL;
	}

	if (m_framenames != NULL)
	{
		free(m_framenames);
		m_framenames = NULL;
	}

	if (m_lightnormalindex)
	{
		free (m_lightnormalindex);
		m_lightnormalindex = NULL;
	}

	if (m_skeletons)
	{
		free (m_skeletons);
		m_skeletons = NULL;
	}

	for (int i = 0; i < m_numClusters; i++)
	{
		delete (m_skeletalClusters[i].verticies);
		m_skeletalClusters[i].verticies = NULL;
	}
	if (m_skeletalClusters != NULL)
	{
		delete m_skeletalClusters;
		m_skeletalClusters = NULL;
		m_numClusters = 0;
	}
	if (m_jointConstraintAngles != NULL)
	{
		delete m_jointConstraintAngles;
		m_jointConstraintAngles = NULL;
	}
	if (m_modelJointAngles != NULL)
	{
		delete m_modelJointAngles;
		m_modelJointAngles = NULL;
	}
	m_num_mesh_nodes = -1;
	CModel::Delete();
}

HRESULT CFlexModel::SetGroupQuality(D3DRMRENDERQUALITY value)
{
	HRESULT retval;
	if (m_num_mesh_nodes > 0)
	{
		if (m_curNode < 0)
		{
			for (int i = 0; i < m_num_mesh_nodes;i++)
			{
				retval = m_nodes[i].SetQuality(value);
			}
		}
		else
		{
			retval = m_nodes[m_curNode].SetQuality(value);
		}
	}
	else
	{
		retval = CModel::SetGroupQuality(value);
	}
	return retval;
}

LPDIRECT3DRMMESH CFlexModel::GetMesh(int i)
{
	if (i < 0)
	{
		if (m_curNode < 0)
		{
			return m_nodes[0].GetMesh();
		}
		return m_nodes[m_curNode].GetMesh();
	}
	return m_nodes[i].GetMesh();
}

void CFlexModel::Drag(double delta_x, double delta_y)
{
	if (GetKeyState(VK_SHIFT)&0x8000)
	{
		m_modelJointAngles[m_curJoint].y += (float) delta_x / 100;

		if (m_modelJointAngles[m_curJoint].y > PI*2)
			m_modelJointAngles[m_curJoint].y = (float) PI*2 - m_modelJointAngles[m_curJoint].y;
		else if (m_modelJointAngles[m_curJoint].y < 0)
			m_modelJointAngles[m_curJoint].y = (float) PI*2 + m_modelJointAngles[m_curJoint].y;
	}
	else
	{
		m_modelJointAngles[m_curJoint].x += (float) delta_x / 100;

		m_modelJointAngles[m_curJoint].z += (float) delta_y / 100;

		if (m_modelJointAngles[m_curJoint].x > PI*2)
			m_modelJointAngles[m_curJoint].x = (float) PI*2 - m_modelJointAngles[m_curJoint].x;
		else if (m_modelJointAngles[m_curJoint].x < 0)
			m_modelJointAngles[m_curJoint].x = (float) PI*2 + m_modelJointAngles[m_curJoint].x;

		if (m_modelJointAngles[m_curJoint].z > PI*2)
			m_modelJointAngles[m_curJoint].z = (float) PI*2 - m_modelJointAngles[m_curJoint].z;
		else if (m_modelJointAngles[m_curJoint].z < 0)
			m_modelJointAngles[m_curJoint].z = (float) PI*2 + m_modelJointAngles[m_curJoint].z;
	}
//	ShowFrame();
//	ConstrainJoint();
}

void CFlexModel::DeSelectAll()
{
	m_curNode = -1;
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		GetMesh(i)->SetGroupColorRGB(	0,	D3DVALUE(255),
											D3DVALUE(255),
											D3DVALUE(255) );
	}
}

void CFlexModel::ChangeVisual(LPDIRECT3DRMFRAME frame, int nodeNum)
{
	if (nodeNum != m_curNode)
	{
		if (m_curNode != -1 && !m_nodes[m_curNode].IsVisible())
		{
			frame->DeleteVisual(m_nodes[m_curNode].GetMesh());
		}

		if (nodeNum != -1 && !m_nodes[nodeNum].IsVisible())
		{
			frame->AddVisual(m_nodes[nodeNum].GetMesh());
		}
	}

}

int CFlexModel::SelectNode(int nodeNum)
{
	int retval = m_curNode;
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].SetColorRGB(D3DVALUE(255), D3DVALUE(255), D3DVALUE(255));
	}

	m_nodes[nodeNum].SetColorRGB(D3DVALUE(255), D3DVALUE(0), D3DVALUE(0));
	m_curNode = nodeNum;
	return retval;
}

HTREEITEM CFlexModel::SelectMesh(LPDIRECT3DRMVISUAL selection)
{
	HTREEITEM retval = NULL;
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		if (m_nodes[i].GetMesh() == selection)
		{
			m_curNode = i;
			m_nodes[i].SetColorRGB(255, 0, 0);
			retval = m_nodes[i].m_treeitem;
		}
		else
		{
			m_nodes[i].SetColorRGB(255, 255, 255);
		}
	}
	return retval;
}

bool CFlexModel::ToggleNodeVisibility(int node)
{
	m_nodes[node].SetVisible(!m_nodes[node].IsVisible());
	return m_nodes[node].IsVisible ();
}

void CFlexModel::MakeAllNodesVisible(LPDIRECT3DRMFRAME frame)
{
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		if (!m_nodes[i].IsVisible())
		{
			m_nodes[i].SetVisible(true);
			frame->AddVisual(m_nodes[i].GetMesh());
		}
	}
}

void CFlexModel::SetCurJoint(long joint)
{
	m_curJoint = joint;
}

bool CFlexModel::ValidNode()
{
	return m_curNode >= 0;
}

long CFlexModel::GetNumNodes()
{
	return m_num_mesh_nodes;
}

long CFlexModel::GetNumTris()
{
	return m_nodes[m_curNode].GetNumTris();
}

long CFlexModel::GetCurJoint()
{
	return m_curJoint;
}

CBitmap* CFlexModel::GetBitmap(LPDIRECT3DRM2 d3drm, CDC* pDC, int skinnum, int& width, int& height)
{
	if (skinnum >= 0)
	{
		CSkin* skin = GetSkin(skinnum);
		width = skin->GetWidth(d3drm, pDC);
		height = skin->GetHeight(d3drm, pDC);
		return skin->GetBitmap(d3drm, pDC);
	}
	if (m_curNode > -1)
	{
		return m_nodes[m_curNode].GetBitmap(d3drm, pDC, width, height);
	}
	width = 0;
	height = 0;
	return NULL;
}

void CFlexModel::Snap()
{
	if (m_curJoint < 0)
	{
		return;
	}
	m_modelJointAngles[m_curJoint].x = 0;
	m_modelJointAngles[m_curJoint].y = 0;
	m_modelJointAngles[m_curJoint].z = 0;
}

void CFlexModel::LoadSkinInfo(CTreeCtrl* pCTree, HTREEITEM rootSkin)
{
	for (int i = 0; i < m_num_skins; i++)
	{
		HTREEITEM point = pCTree->InsertItem(m_skin_names[i], rootSkin, TVI_LAST);
		pCTree->SetItemImage(point, 0, 0);
		pCTree->SetItemData(point, i);
	}
}

void CFlexModel::UpdateJointVisuals(int jointNum)
{
	D3DRMVERTEX			vertexlist[12];

	Placement_t			model = m_skeletons[m_curFrame].rootJoint[jointNum].model;

	frameinfo2_t		*frameinfo1 = (frameinfo2_t *)((char *)m_frames + m_framesize * m_curFrame);

	vertexlist[0].position.z = -(((model.origin[0] - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((model.origin[0] + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((model.origin[2] + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((model.origin[1]) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((model.origin[2] - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((model.origin[1] + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((model.origin[0]) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((model.origin[1] - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((model.origin[2]) * frameinfo1->scale.z) + frameinfo1->origin.z);

	m_joint_visuals[jointNum].origin->SetVertices( 0, 0, 23, vertexlist );

	/**************************************************************************/

	D3DVECTOR dir, up, org;

	dir.x = model.direction[0];
	dir.y = model.direction[1];
	dir.z = model.direction[2];

	up.x = model.up[0];
	up.y = model.up[1];
	up.z = model.up[2];

	org.x = model.origin[0];
	org.y = model.origin[1];
	org.z = model.origin[2];

	D3DRMVectorSubtract(&dir, &dir, &org);
	D3DRMVectorNormalize(&dir);

	dir.x *= 8;
	dir.y *= 8;
	dir.z *= 8;

	dir.x += org.x;
	dir.y += org.y;
	dir.z += org.z;

	D3DRMVectorSubtract(&up, &up, &org);
	D3DRMVectorNormalize(&up);

	up.x *= 8;
	up.y *= 8;
	up.z *= 8;

	up.x += org.x;
	up.y += org.y;
	up.z += org.z;

	vertexlist[0].position.z = -(((dir.x - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((dir.x + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((dir.z + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((dir.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((dir.z - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((dir.y + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((dir.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((dir.y - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((dir.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	m_joint_visuals[jointNum].dir->SetVertices( 0, 0, 23, vertexlist );

	/**************************************************************************/

	vertexlist[0].position.z = -(((up.x - 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[0].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[0].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[2].position.z = -(((up.x + 1) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[2].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[2].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[1].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[1].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[1].position.x = -(((up.z + 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[3].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[3].position.y =  (((up.y) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[3].position.x = -(((up.z - 1) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[4].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[4].position.y =  (((up.y + 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[4].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	vertexlist[5].position.z = -(((up.x) * frameinfo1->scale.x) + frameinfo1->origin.x);
	vertexlist[5].position.y =  (((up.y - 1) * frameinfo1->scale.y) + frameinfo1->origin.y);
	vertexlist[5].position.x = -(((up.z) * frameinfo1->scale.z) + frameinfo1->origin.z);

	m_joint_visuals[jointNum].up->SetVertices( 0, 0, 23, vertexlist );
}

void CFlexModel::FillRawVerts(vec3a_t *foo)
{
	frameinfo2_t	*frameinfo1 = (frameinfo2_t *)((char *)m_frames + m_framesize * m_curFrame);

	for (int i = 0; i < m_num_xyz; i++)
	{
		foo[i][0] = frameinfo1->verts[i].x;
		foo[i][1] = frameinfo1->verts[i].y;
		foo[i][2] = frameinfo1->verts[i].z;
	}
}

BOOL CFlexModel::ShowFrame(LPDIRECT3DRM2 d3drm, CDC* pDC)
{
	for (int i = 0; i < m_numClusters; i++)
		UpdateJointVisuals(i);

	if (m_num_mesh_nodes > 0)
	{
		vec3a_t angles;

		if (m_curJoint > -1)
		{
			angles[0] = m_modelJointAngles[m_curJoint].x;
			angles[1] = m_modelJointAngles[m_curJoint].y;
			angles[2] = m_modelJointAngles[m_curJoint].z;
		}
		else
		{
			angles[0] = 0;
			angles[1] = 0;
			angles[2] = 0;
		}

		vec3a_t* rotVerts = (vec3a_t *) malloc(sizeof(vec3a_t)*m_num_tris*3);

		FillRawVerts(rotVerts);

		if (m_skeletons && m_jointsOn)
			RotateModelSegment((M_SkeletalJoint_s *) &m_skeletons[m_curFrame].rootJoint[m_curJoint], rotVerts, angles, &m_skeletalClusters[m_curJoint]);

		frameinfo2_t* frameinfo1 = (frameinfo2_t*)((char *)m_frames + m_framesize * GetCurFrame());
		for (i = 0; i < m_num_mesh_nodes; i++)
		{
			m_nodes[i].ShowFrame(d3drm, pDC, rotVerts, frameinfo1);
		}

		free (rotVerts);
		return true;
	}

	return TRUE;
}

BOOL CFlexModel::CalcInterpolate(LPDIRECT3DRM2 d3drm, CDC* pDC, int step, int numsteps, int intertype, int direction)
{
	int				curvert = 0;
	int				lastframe, i;

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
			{
				lastframe = m_treeInfo[m_curGroup].bFrame;
			}
			else
			{
				lastframe = m_curFrame + 1;
			}
		}
	}

	frameinfo2_t* frameinfo1 = (frameinfo2_t*)((char*)m_frames + m_framesize * m_curFrame);
	frameinfo2_t* frameinfo2 = (frameinfo2_t*)((char*)m_frames + m_framesize * lastframe);
	if (m_num_mesh_nodes > 0)
	{
		for (i=0; i < m_num_mesh_nodes; i++)
		{
			m_nodes[i].ShowInterFrame(d3drm, pDC, frameinfo1, frameinfo2, step, numsteps);
		}
	}
	else
	{
		m_nodes[0].ShowInterFrame(d3drm, pDC, frameinfo1, frameinfo2, step, numsteps);
	}

	if (step + 1 > numsteps) m_curstep = 0;
	else m_curstep++;

	return true;
}

// Node Tree Ctrl Support
void CFlexModel::AddNode (CTreeCtrl*pCTree, HTREEITEM rootNode, int nodeNum)
{
	CString		foo;
	foo.Format("Node %d", nodeNum);
	HTREEITEM point = pCTree->InsertItem(foo, rootNode, TVI_LAST);
	m_nodes[nodeNum].m_treeitem = point;
	pCTree->SetItemData(point, nodeNum);
	pCTree->SetItemImage(point, 1, 1);
	pCTree->Expand(rootNode, TVE_EXPAND);
}

void CFlexModel::AddNodes(CTreeCtrl* pCTreeCtrl, HTREEITEM rootNode)
{
	if (m_num_mesh_nodes > 0)
	{
		for (int i = 0; i < m_num_mesh_nodes; i++)
		{
			m_nodes[i].SetVisible(true);
			AddNode(pCTreeCtrl, rootNode, i);
		}
	}
}

void CFlexModel::FillMenuWithSkins(CMenu* menu)
{
	int curID = IDR_SKIN_SELECT_START;
	for (int i = 0; i < m_num_skins; i++)
	{
		menu->AppendMenu(MF_ENABLED | MF_UNCHECKED | MF_STRING, curID++, m_skin_names[i]);
	}
}

void CFlexModel::ChangeSkin(LPDIRECT3DRM2 d3drm, CDC* pDC, int nodenum, int skinnum)
{
	CSkin* skin = CModel::GetSkin(m_skin_names[skinnum]);
	m_nodes[nodenum].ChangeSkin(d3drm, pDC, skin);
}

void CFlexModel::AddSkin(LPCTSTR skinname)
{
	char** holdSkins = m_skin_names;
	m_skin_names = (char**)malloc(sizeof(char*) * (m_num_skins + 1));
	for (int i = 0; i < m_num_skins; i++)
	{
		m_skin_names[i] = holdSkins[i];
	}
	free(holdSkins);
	CString result = skinname;

	while(true)
	{
		int destChar = result.Find("/");
		if (destChar < 0)
		{
			break;
		}
		result.SetAt(destChar, '\\');
	}

	m_skin_names[m_num_skins] = (char*)malloc(result.GetLength() + 1);
	strcpy(m_skin_names[m_num_skins], result);
	m_num_skins++;
}

void CFlexModel::ReplaceSkin(int i, LPCTSTR skinname)
{
	if (m_skin_names[i] != NULL)
	{
		free(m_skin_names[i]);
		m_skin_names[i] = NULL;
	}
	m_skin_names[i] = (char*)malloc(strlen(skinname) + 1);
	strcpy(m_skin_names[i], skinname);
}

CSkin* CFlexModel::GetSkin(int i)
{
	return CModel::GetSkin(m_skin_names[i]);
}

void CFlexModel::BuildMesh(LPDIRECT3DRM2 d3drm, LPDIRECT3DRMFRAME frame, LPDIRECT3DRMFRAME scene, CDC* pDC)
{
	CSkin* skin = GetSkin(0);

	if (m_num_mesh_nodes > 0)
	{
		for (int i = 0; i < m_num_mesh_nodes; i++)
		{
			m_curNode = i;
			m_nodes[i].BuildMesh(d3drm, (frameinfo2_t*)m_frames, skin->GetWidth(d3drm, pDC), skin->GetHeight(d3drm, pDC));
			m_nodes[i].SetSkin(d3drm, pDC, skin, 0);
			m_nodes[i].AddVisual(frame);
		}
		frame->SetOrientation(scene,
				D3DVALUE(0), D3DVALUE(0), D3DVALUE(1),
				D3DVALUE(1), D3DVALUE(0), D3DVALUE(0));
	}
}

void CFlexModel::RotateModelSegment(M_SkeletalJoint_t *joint, vec3a_t *modelVerticies, vec3a_t angles, M_SkeletalCluster_t *modelCluster)
{
	int i;
	matrix3_t rotation, rotation2, toWorld, partialBackToLocal;
	vec3a_t localAngles;

	localAngles[0] = 0;//angles[0];
	localAngles[1] = 0;//angles[1];
	localAngles[2] = 0;//angles[2];

	memset(rotation, 0, sizeof(rotation));

	CMatrix::Matrix3FromAngles(angles, rotation);

	//D3DVECTOR dir, up;
	vec3a_t	  fdir, fup;

	fdir[0] = joint->model.direction[0];
	fdir[1] = joint->model.direction[1];
	fdir[2] = joint->model.direction[2];

	Vec3Normalize(fdir);

	fup[0] = joint->model.up[0];
	fup[1] = joint->model.up[1];
	fup[2] = joint->model.up[2];

	Vec3Normalize(fup);

	localAngles[ROLL] += (float) CMatrix::Matricies3FromDirAndUp(fdir, fup, toWorld, partialBackToLocal);

	CMatrix::Matrix3MultByMartrix3(rotation, toWorld, rotation2);
	CMatrix::Matrix3MultByMartrix3(partialBackToLocal, rotation2, rotation);

	for(i = 0; i < modelCluster->numVerticies; ++i)
	{
		CMatrix::RotatePointAboutLocalOrigin_d(rotation, joint->model.origin, modelVerticies[modelCluster->verticies[i]]);
	}
}

// frame tree ctrl support

void CFlexModel::LoadFrameInfo(CTreeCtrl* treeDlg, HTREEITEM rootFrame)
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
			frameinfo2_t* frameinfo1 = (frameinfo2_t *)((char *)m_frames + m_framesize * i);
			char	name[18];
			memcpy(name , frameinfo1->name, 16);
			name[16] = '\0';
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
		treeDlg->Expand(rootFrame, TVE_EXPAND);
		if (groupList)
		{
			delete groupList;
			groupList = NULL;
		}

	}
}

// file serialization support

#define FM_HEADER_NAME	"header"
#define FM_HEADER_VER	2
#define FM_SKIN_NAME	"skin"
#define FM_SKIN_VER		1
#define FM_ST_NAME		"st coord"
#define FM_ST_VER		1
#define FM_TRI_NAME		"tris"
#define FM_TRI_VER		1
#define FM_FRAME_NAME	"frames"
#define FM_FRAME_VER	1
#define FM_SHORT_FRAME_NAME	"short frames"
#define FM_SHORT_FRAME_VER	1
#define FM_NORMAL_NAME	"normals"
#define FM_NORMAL_VER	1
#define FM_COMP_NAME	"comp data"
#define FM_COMP_VER	1
#define FM_GLCMDS_NAME	"glcmds"
#define FM_GLCMDS_VER	1
#define FM_MESH_NAME	"mesh nodes"
#define FM_MESH_VER		3
#define FM_SKELETON_NAME "skeleton"
#define FM_SKELETON_VER	1
#define FM_REFERENCES_NAME "references"
#define FM_REFERENCES_VER 1
#define FRAME_NAME_LEN (16)

enum
{
	FM_BLOCK_HEADER,
	FM_BLOCK_SKIN,
	FM_BLOCK_ST,
	FM_BLOCK_TRIS,
	FM_BLOCK_FRAMES,
	FM_BLOCK_GLCMDS,
	FM_BLOCK_MESHNODES,
	FM_BLOCK_SHORTFRAMES,
	FM_BLOCK_NORMAL,
	FM_BLOCK_COMP,
	FM_BLOCK_SKELETON,
	FM_BLOCK_REFERENCES,
};

fmblock_t CFlexModel::m_fmblocks[] =
{
	{FM_HEADER_NAME,			FM_BLOCK_HEADER},
	{FM_SKIN_NAME,				FM_BLOCK_SKIN},
	{FM_ST_NAME,				FM_BLOCK_ST},
	{FM_TRI_NAME,				FM_BLOCK_TRIS},
	{FM_FRAME_NAME	,			FM_BLOCK_FRAMES},
	{FM_GLCMDS_NAME,			FM_BLOCK_GLCMDS},
	{FM_MESH_NAME,				FM_BLOCK_MESHNODES},
	{FM_SHORT_FRAME_NAME,		FM_BLOCK_SHORTFRAMES},
	{FM_NORMAL_NAME,			FM_BLOCK_NORMAL},
	{FM_COMP_NAME,				FM_BLOCK_COMP},
	{FM_SKELETON_NAME,			FM_BLOCK_SKELETON},
	{FM_REFERENCES_NAME,		FM_BLOCK_REFERENCES},
	{"",						-1}
};

void CFlexModel::Serialize(CArchive& ar)
{
	DWORD filesize = ar.GetFile()->GetLength();
	char* buffer =  new char[filesize];
	ar.Read(buffer, filesize);

	char* back = buffer;

	while(filesize > 0)
	{
		int		version;
		int		size;

		char* blockname = buffer;

		int i = 0;
		while (m_fmblocks[i].blocktype >= 0)
		{
			if (strcmpi(buffer, m_fmblocks[i].blockid)==0)
			{
				break;
			}
			i++;
		}
		buffer += sizeof(m_fmblocks[0].blockid);
		version = *(int*)buffer;
		buffer += sizeof(version);
		size = *(int*)buffer;
		buffer += sizeof(size);
		filesize = filesize - sizeof(m_fmblocks[0].blockid) - sizeof(version) - sizeof(size);


		CString foo;
		switch (m_fmblocks[i].blocktype)
		{
		case FM_BLOCK_HEADER:
			SerializeHeader(version, size, buffer);
			break;
		case FM_BLOCK_SKIN:
			SerializeSkin(version, size, buffer);
			break;
		case FM_BLOCK_ST:
			SerializeSt(version, size, buffer);
			break;
		case FM_BLOCK_TRIS:
			SerializeTris(version, size, buffer);
			break;
		case FM_BLOCK_FRAMES:
			SerializeFrames(version, size, buffer);
			break;
		case FM_BLOCK_GLCMDS:
			SerializeGLCmds(version, size, buffer);
			break;
		case FM_BLOCK_MESHNODES:
			SerializeMeshNodes(version, size, buffer);
			break;
		case FM_BLOCK_SHORTFRAMES:
			SerializeShortFrames(version, size, buffer);
			break;
		case FM_BLOCK_NORMAL:
			SerializeNormal(version, size, buffer);
			break;
		case FM_BLOCK_COMP:
			SerializeComp(version, size, buffer);
			break;
		case FM_BLOCK_SKELETON:
			SerializeSkeleton(version, size, buffer);
			break;
		case FM_BLOCK_REFERENCES:
			SerializeReferences(version, size, buffer);
			break;
		default:
			foo.Format("Unknown block %s\n", blockname);
			AfxMessageBox(foo);
		}
		filesize -= size;
		buffer += size;
	}

	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].SetVisible(true);
	}

	m_curNode = -1;
	if (back)
	{
		delete(back);
		back = buffer = NULL;
	}
}

void CFlexModel::SerializeHeader(int version, int length, char *buffer)
{
	if (version != FM_HEADER_VER)
	{
		AfxMessageBox("Invalid header version for block\n");
		return;
	}

	int* offset = (int*)buffer;
	m_skinwidth = *offset++;
	m_skinheight = *offset++;
	m_framesize = *offset++;
	m_num_skins = *offset++;
	m_num_xyz = *offset++;
	m_num_st = *offset++;
	m_num_tris = *offset++;
	m_num_glcmds = *offset++;
	m_num_frames = *offset++;
	m_num_mesh_nodes = *offset++;

	if ((char*)offset != (buffer + length))
	{
		AfxMessageBox("Invalid header read");
		return;
	}

	if (m_skinheight > MAX_LBM_HEIGHT)
	{
		AfxMessageBox("Model has a skin taller than max LBM height\n");
		return;
	}

	if (m_num_xyz <= 0)
	{
		AfxMessageBox("Model has no verts\n");
		return;
	}

	if (m_num_xyz > MAX_FM_VERTS)
	{
		AfxMessageBox("Model has too many vertices\n");
		return;
	}

	if (m_num_st <= 0)
	{
		AfxMessageBox("Model has no st verts\n");
		return;
	}

	if (m_num_tris <= 0)
	{
		AfxMessageBox("Model has no tris\n");
		return;
	}

	if (m_num_frames <= 0)
	{
		AfxMessageBox("Model has no frames\n");
		return;
	}
}

void CFlexModel::SerializeSkin(int version, int length, char *buffer)
{
	if (m_skin_names != NULL)
	{
		AfxMessageBox("Duplicate skin block!\n");
		return;
	}
	if (version != FM_SKIN_VER)
	{
		AfxMessageBox("Invalid skin version!\n");
		return;
	}

	CString filePath = m_filename;
	int destChar = filePath.ReverseFind('\\');
	if (destChar > -1)
	{
		filePath = filePath.Left(destChar);
	}

	m_skin_names = (char**)malloc(sizeof(char*) * m_num_skins);
	char* strname = buffer;

	for (int i = 0; i < m_num_skins; i++)
	{
		CString result = strname;
		strname += 64;

		while(true)
		{
			destChar = result.Find("/");
			if (destChar < 0)
			{
				break;
			}
			result.SetAt(destChar, '\\');
		}

		CString namePart;
		CString partialPath = result;
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

		m_skin_names[i] = (char*)malloc(partialPath.GetLength() + 1);
		strcpy(m_skin_names[i], partialPath);
	}
}


void CFlexModel::SerializeSt(int version, int length, char *buffer)
{
	// ignoring data
}

void CFlexModel::SerializeTris(int version, int length, char *buffer)
{
	// ignoring data
}

void CFlexModel::SerializeFrames(int version, int length, char *buffer)
{
	int				i,j;
	fmaliasframe_t	*pinframe, *poutframe;

	if (m_frames != NULL)
	{
		AfxMessageBox("Duplicate frames block!\n");
		return;
	}
	if (version != FM_FRAME_VER)
	{
		AfxMessageBox("Invalid frames version\n");
		false;
	}

	m_frames = (void*) malloc(m_num_frames * m_framesize);

	for (i=0 ; i<m_num_frames ; i++)
	{
		pinframe = (fmaliasframe_t *) ((byte *)buffer + i * m_framesize);
		poutframe = (fmaliasframe_t *) ((byte *)m_frames + i * m_framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = pinframe->scale[j];
			poutframe->translate[j] = pinframe->translate[j];
		}

		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts, m_num_xyz*sizeof(fmtrivertx_t));
	}
}

void CFlexModel::SerializeGLCmds(int version, int length, char *buffer)
{
	if (m_glcmds != NULL)
	{
		AfxMessageBox("Duplicate glCmds block!\n");
		return;
	}
	if (version != FM_GLCMDS_VER)
	{
		AfxMessageBox("Invalid glcmds version\n");
		return;
	}

	int* poutcmd = (int *) malloc(sizeof(int) * m_num_glcmds);
	m_glcmds = (long*)poutcmd;
	int* pincmd = (int *) ((byte *)buffer);
	for (int i=0 ; i<m_num_glcmds ; i++)
		poutcmd[i] = (pincmd[i]);
}

typedef struct
{
	byte	tris[MAX_FM_TRIANGLES>>3];
	byte	verts[MAX_FM_VERTS>>3];
	short	start_glcmds, num_glcmds;
} fmmeshnode_t;

void CFlexModel::SerializeMeshNodes(int version, int length, char *buffer)
{
	int				i;
	fmmeshnode_t	*node;

	if (m_nodes != NULL)
	{
		AfxMessageBox("Duplicate mesh node block!\n");
		return;
	}
	if (version != FM_MESH_VER)
	{
		AfxMessageBox("Invalid mesh version\n");
		return;
	}

	if (m_num_mesh_nodes)
	{
		m_nodes = new CNode[m_num_mesh_nodes];
		for(i=0,node = (fmmeshnode_t *)buffer;i<m_num_mesh_nodes;i++,node++)
		{
			m_nodes[i].Init();

			int numtris = m_num_tris;
//			if (numtris > (MAX_FM_TRIANGLES>>3))
//			{
//				numtris = MAX_FM_TRIANGLES>>3;
//			}
			m_nodes[i].SetTris(node->tris, numtris);

			int numverts = m_num_xyz;
//			if (numverts > (MAX_FM_VERTS>>3))
//			{
//				numverts = MAX_FM_VERTS>>3;
//			}
			m_nodes[i].SetVerts(node->verts, numverts);
			m_nodes[i].SetGlcmds((long*)((long*)m_glcmds + node->start_glcmds), node->num_glcmds);
			}
	}
}

void CFlexModel::SerializeShortFrames(int version, int length, char *buffer)
{
	if (m_framenames != NULL)
	{
		AfxMessageBox("Duplicate short frames block!\n");
		return;
	}
	AfxMessageBox("This is short frames\n");
	if (version != FM_SHORT_FRAME_VER)
	{
		AfxMessageBox("Invalid shortframes version\n");
		return;
	}

//??	m_frames=NULL;
	m_framenames=(char *) (m_num_frames*FRAME_NAME_LEN);
	memcpy(m_framenames,buffer,m_num_frames*FRAME_NAME_LEN);
}

void CFlexModel::SerializeNormal(int version, int length, char *buffer)
{
	if (m_lightnormalindex != NULL)
	{
		AfxMessageBox("Duplicate light normal block!\n");
		return;
	}
	if (version != FM_NORMAL_VER)
	{
		AfxMessageBox("Invalid normal version\n");
		return;
	}
	m_lightnormalindex=(byte *)malloc(m_num_xyz*sizeof(byte));
	memcpy(m_lightnormalindex,buffer,m_num_xyz*sizeof(byte));
}

void CFlexModel::SerializeComp(int version, int length, char *buff)
{
	fmgroup_t *g;
	char *buffer;

	AfxMessageBox("This is compressed\n");

	buffer= (char *)buff;
	g=&m_compdata;

	if (version != FM_COMP_VER)
	{
		AfxMessageBox("Invalid comp version\n");
		return;
	}

	g->start_frame=*(int *)buffer;
	buffer+=sizeof(int);
	g->num_frames= *(int *)buffer;
	buffer+=sizeof(int);
	g->degrees=*(int *)buffer;
	buffer+=sizeof(int);

	g->mat=		(char *)malloc(m_num_xyz*3*g->degrees*sizeof(char));
	g->ccomp=	(char *)malloc(g->num_frames*g->degrees*sizeof(char));
	g->cbase=	(unsigned char*)malloc(m_num_xyz*3*sizeof(unsigned char));
	g->cscale=	(float *)malloc(g->degrees*sizeof(float));
	g->coffset=	(float *)malloc(g->degrees*sizeof(float));
	g->complerp=(float *)malloc(g->degrees*sizeof(float));

	memcpy(g->mat,buffer,m_num_xyz*3*g->degrees*sizeof(char));
	buffer+=m_num_xyz*3*g->degrees*sizeof(char);
	memcpy(g->ccomp,buffer,g->num_frames*g->degrees*sizeof(char));
	buffer+=g->num_frames*g->degrees*sizeof(char);
	memcpy(g->cbase,buffer,m_num_xyz*3*sizeof(unsigned char));
	buffer+=m_num_xyz*3*sizeof(unsigned char);
	memcpy(g->cscale,buffer,g->degrees*sizeof(float));
	buffer+=g->degrees*sizeof(float);
	memcpy(g->coffset,buffer,g->degrees*sizeof(float));
	buffer+=g->degrees*sizeof(float);
	memcpy(g->trans,buffer,3*sizeof(float));
	buffer+=3*sizeof(float);
	memcpy(g->scale,buffer,3*sizeof(float));
	buffer+=3*sizeof(float);
	memcpy(g->bmin,buffer,3*sizeof(float));
	buffer+=3*sizeof(float);
	memcpy(g->bmax,buffer,3*sizeof(float));
	buffer+=3*sizeof(float);
}

void CFlexModel::SerializeSkeleton(int version, int length, char *buffer)
{
	int		i, j, k;
	int		*basei;
	int		runningTotalVertices = 0;
	int		indexBase = 0;
	float	*basef;

	if (m_skeletons != NULL)
	{
		AfxMessageBox("Duplicate skeleton block!\n");
		return;
	}
	if (version != FM_SKELETON_VER)
	{
		AfxMessageBox("Invalid skeleton version\n");
		return;
	}

	basei = (int *)buffer;

	m_skeletalType = *basei;

	m_numClusters = *(++basei);

	m_jointConstraintAngles = new rangeVector_t[m_numClusters];
	m_modelJointAngles = new D3DVECTOR[m_numClusters];
	m_skeletalClusters = new M_SkeletalCluster_t[m_numClusters];

	for(i = m_numClusters - 1; i >= 0; --i)
	{
		m_jointConstraintAngles[i].min.x = 0;
		m_jointConstraintAngles[i].min.y = 0;
		m_jointConstraintAngles[i].min.z = 0;
		m_jointConstraintAngles[i].max.x = 0;
		m_jointConstraintAngles[i].max.y = 0;
		m_jointConstraintAngles[i].max.z = 0;
		m_modelJointAngles[i].x = 0;
		m_modelJointAngles[i].y = 0;
		m_modelJointAngles[i].z = 0;

		runningTotalVertices += *(++basei);
		m_skeletalClusters[i].numVerticies = runningTotalVertices;
		m_skeletalClusters[i].verticies = (int*)malloc(m_skeletalClusters[i].numVerticies*sizeof(int));
	}

	for(j = m_numClusters - 1; j >= 0; --j)
	{
		for(i = indexBase; i < m_skeletalClusters[j].numVerticies; ++i)
		{
			++basei;

			for(k = 0; k <= j; ++ k)
			{
				m_skeletalClusters[k].verticies[i] = *basei;
			}
		}

		indexBase = m_skeletalClusters[j].numVerticies;
	}

	if(*(++basei))
	{
		basef = (float *)++basei;

		m_skeletons = (ModelSkeleton_t*) malloc(m_num_frames*sizeof(ModelSkeleton_t));

		for (i = 0; i < m_num_frames; ++i)
		{
			for(j = 0; j < m_numClusters; ++j)
			{
				m_skeletons[i].rootJoint[j].model.origin[0] = *(basef++);
				m_skeletons[i].rootJoint[j].model.origin[1] = *(basef++);
				m_skeletons[i].rootJoint[j].model.origin[2] = *(basef++);

				m_skeletons[i].rootJoint[j].model.direction[0] = *(basef++);
				m_skeletons[i].rootJoint[j].model.direction[1] = *(basef++);
				m_skeletons[i].rootJoint[j].model.direction[2] = *(basef++);

				m_skeletons[i].rootJoint[j].model.up[0] = *(basef++);
				m_skeletons[i].rootJoint[j].model.up[1] = *(basef++);
				m_skeletons[i].rootJoint[j].model.up[2] = *(basef++);
			}
		}

	}
	else
	{
		m_num_xyz -= m_numClusters*3;
	}
}

void CFlexModel::SerializeReferences(int version, int length, char *buffer)
{
	if (version != FM_REFERENCES_VER)
	{
		AfxMessageBox("Invalid reference version\n");
		return;
	}

	int* basei = (int*)buffer;
	m_referenceType = *basei++;
	/* remainder of this data structure is hard coded logic for specific models -- just ignore */
}

// skin texture support

void CFlexModel::SetTransparency(LPDIRECT3DRM2 d3drm, CDC* pDC, bool trans)
{
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].SetTransparency(d3drm, pDC, trans);
	}
}

// joint support

void CFlexModel::ToggleJointOn()
{
	m_jointsOn = !m_jointsOn;
}

bool CFlexModel::IsJointOn()
{
	return m_jointsOn;
}

void CFlexModel::ConstrainJoint()
{
	if (m_jointConstraintAngles[m_curJoint].max.x || m_jointConstraintAngles[m_curJoint].min.x)
	{
		if (m_modelJointAngles[m_curJoint].x > m_jointConstraintAngles[m_curJoint].max.x)
			m_modelJointAngles[m_curJoint].x = m_jointConstraintAngles[m_curJoint].max.x;

		if (m_modelJointAngles[m_curJoint].x < m_jointConstraintAngles[m_curJoint].min.x)
			m_modelJointAngles[m_curJoint].x = m_jointConstraintAngles[m_curJoint].min.x;
	}

	if (m_jointConstraintAngles[m_curJoint].max.y || m_jointConstraintAngles[m_curJoint].min.y)
	{
		if (m_modelJointAngles[m_curJoint].y > m_jointConstraintAngles[m_curJoint].max.y)
			m_modelJointAngles[m_curJoint].y = m_jointConstraintAngles[m_curJoint].max.y;

		if (m_modelJointAngles[m_curJoint].y < m_jointConstraintAngles[m_curJoint].min.y)
			m_modelJointAngles[m_curJoint].y = m_jointConstraintAngles[m_curJoint].min.y;
	}

	if (m_jointConstraintAngles[m_curJoint].max.z || m_jointConstraintAngles[m_curJoint].min.z)
	{
		if (m_modelJointAngles[m_curJoint].z > m_jointConstraintAngles[m_curJoint].max.z)
			m_modelJointAngles[m_curJoint].z = m_jointConstraintAngles[m_curJoint].max.z;

		if (m_modelJointAngles[m_curJoint].z < m_jointConstraintAngles[m_curJoint].min.z)
			m_modelJointAngles[m_curJoint].z = m_jointConstraintAngles[m_curJoint].min.z;
	}

//	ShowFrame();
}

void CFlexModel::GetModelAngles(float* x, float* y, float* z)
{
	*x = m_modelJointAngles[m_curJoint].x;
	*y = m_modelJointAngles[m_curJoint].y;
	*z = m_modelJointAngles[m_curJoint].z;
}

void CFlexModel::SetModelAngles(float x, float y, float z)
{
	m_modelJointAngles[m_curJoint].x = x;
	m_modelJointAngles[m_curJoint].y = y;
	m_modelJointAngles[m_curJoint].z = z;

	if (m_modelJointAngles[m_curJoint].y > PI*2)
		m_modelJointAngles[m_curJoint].y = (float) PI*2 - m_modelJointAngles[m_curJoint].y;
	else if (m_modelJointAngles[m_curJoint].y < 0)
		m_modelJointAngles[m_curJoint].y = (float) PI*2 + m_modelJointAngles[m_curJoint].y;

	if (m_modelJointAngles[m_curJoint].x > PI*2)
		m_modelJointAngles[m_curJoint].x = (float) PI*2 - m_modelJointAngles[m_curJoint].x;
	else if (m_modelJointAngles[m_curJoint].x < 0)
		m_modelJointAngles[m_curJoint].x = (float) PI*2 + m_modelJointAngles[m_curJoint].x;

	if (m_modelJointAngles[m_curJoint].z > PI*2)
		m_modelJointAngles[m_curJoint].z = (float) PI*2 - m_modelJointAngles[m_curJoint].z;
	else if (m_modelJointAngles[m_curJoint].z < 0)
		m_modelJointAngles[m_curJoint].z = (float) PI*2 + m_modelJointAngles[m_curJoint].z;
}

void CFlexModel::RenderTexture(LPDIRECT3DRM2 d3drm, CDC* pDC, bool useTexture)
{
	for (int i = 0; i < m_num_mesh_nodes; i++)
	{
		m_nodes[i].RenderTexture(d3drm, pDC, useTexture);
	}
}

void CFlexModel::GetConstraintAngleMaxs(float* x, float* y, float* z)
{
	*x = m_jointConstraintAngles[m_curJoint].max.x;
	*y = m_jointConstraintAngles[m_curJoint].max.y;
	*z = m_jointConstraintAngles[m_curJoint].max.z;
}

void CFlexModel::GetConstraintAngleMins(float* x, float* y, float* z)
{
	*x = m_jointConstraintAngles[m_curJoint].min.x;
	*y = m_jointConstraintAngles[m_curJoint].min.y;
	*z = m_jointConstraintAngles[m_curJoint].min.z;
}

void CFlexModel::SetConstraintAngleMaxs(float x, float y, float z)
{
	m_jointConstraintAngles[m_curJoint].max.x = x;
	m_jointConstraintAngles[m_curJoint].max.y = y;
	m_jointConstraintAngles[m_curJoint].max.z = z;
	ConstrainJoint();
}

void CFlexModel::SetConstraintAngleMins(float x, float y, float z)
{
	m_jointConstraintAngles[m_curJoint].min.x = x;
	m_jointConstraintAngles[m_curJoint].min.y = y;
	m_jointConstraintAngles[m_curJoint].min.z = z;
	ConstrainJoint();
}

// joint tree control support

void CFlexModel::AddJoints(CTreeCtrl* pCTree, HTREEITEM rootJoint)
{
	if (m_skeletons != NULL)
	{
		CString name;
		for (int i = 0; i < m_numClusters; i++)
		{
			name.Format("Joint %d", i);
			HTREEITEM item = pCTree->InsertItem(name, rootJoint, TVI_LAST);
			pCTree->SetItemData(item, i);
			pCTree->SetItemImage(item, 1, 1);
		}
		pCTree->Expand(rootJoint, TVE_EXPAND);
	}
}

// property page support

void CFlexModel::AddPropPages(CPropertySheet* propSheet)
{
	CModel::AddPropPages(propSheet);
	// add private pages
}

void CFlexModel::UpdateFromPropPages(CPropertySheet* propSheet)
{
	// update data from private pages (some could be after)
	CModel::UpdateFromPropPages(propSheet);
	// update data from private pages (some could be before)
}

void CFlexModel::RemovePropPages(CPropertySheet* propSheet)
{
	// remove private pages
	CModel::RemovePropPages(propSheet);
}

