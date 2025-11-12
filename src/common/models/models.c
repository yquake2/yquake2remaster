/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 2005-2015 David HENRY
 * Copyright (c) 2011 Sajt (https://icculus.org/qshed/qwalk/)
 * Copyright (c) 1998 Trey Harrison (SiN View)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * The models file format
 *
 * =======================================================================
 */

#include "models.h"

/*
=================
Mod_LoadSTvertList

load base s and t vertices (not used in gl version)
=================
*/
static void
Mod_LoadSTvertList(dmdx_t *pheader, const dstvert_t *pinst)
{
	dstvert_t *poutst;
	int i;

	poutst = (dstvert_t *)((byte *)pheader + pheader->ofs_st);

	for (i = 0; i < pheader->num_st; i++)
	{
		poutst[i].s = LittleShort(pinst[i].s);
		poutst[i].t = LittleShort(pinst[i].t);
	}
}

/*
=================
Mod_LoadCmdList

Load the glcmds
=================
*/
static void
Mod_LoadCmdList(const char *mod_name, dmdx_t *pheader, const int *pincmd)
{
	int *poutcmd;
	int i;

	poutcmd = (int *)((char*)pheader + pheader->ofs_glcmds);
	for (i = 0; i < pheader->num_glcmds; i++)
	{
		poutcmd[i] = LittleLong(pincmd[i]);
	}

	if (poutcmd[pheader->num_glcmds-1] != 0)
	{
		Com_Printf("%s: Entity %s has possible last element issues with %d verts.\n",
			__func__, mod_name, poutcmd[pheader->num_glcmds-1]);
	}
}

/*
 * verts as compressed int
 */
static void
Mod_LoadFrames_VertDKM2(dxtrivertx_t *vert, int in)
{
	unsigned xyz;

	xyz = LittleLong(in) & 0xFFFFFFFF;
	vert->v[0] = ((xyz & 0xFFE00000) >> 21) & 0x7FF;
	vert->v[0] *= ((float)0xFFFF / 0x7FF);
	vert->v[1] = ((xyz & 0x1FF800) >> 11) & 0x3FF;
	vert->v[1] *= ((float)0xFFFF / 0x3FF);
	vert->v[2] = xyz & 0x7FF;
	vert->v[2] *= ((float)0xFFFF / 0x7FF);
}

/*
 * verts as short
 */
static void
Mod_LoadFrames_VertA(dxtrivertx_t *vert, const short *in)
{
	int k;

	for (k=0; k < 3; k++)
	{
		vert->v[k] = LittleShort(in[k]);
	}
}

/*
=================
Mod_LoadFrames

Load the Quake2 md2 default format frames
=================
*/
static qboolean
Mod_LoadFrames_MD2(dmdx_t *pheader, byte *src, size_t inframesize, vec3_t translate)
{
	qboolean normalfix = true;
	int i;

	for (i = 0; i < pheader->num_frames; i++)
	{
		daliasframe_t *pinframe;
		daliasxframe_t *poutframe;
		int j;

		pinframe = (daliasframe_t *) (src + i * inframesize);
		poutframe = (daliasxframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]) / 0xFF;
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
			poutframe->translate[j] += translate[j];
		}

		/* verts are all 8 bit, so no swapping needed */
		for (j=0; j < pheader->num_xyz; j ++)
		{
			Mod_LoadFrames_VertMD2(poutframe->verts + j, pinframe->verts[j].v);

			if (pinframe->verts[j].lightnormalindex)
			{
				/* normal is set? */
				normalfix = false;
			}

			Mod_ConvertNormalMDL(pinframe->verts[j].lightnormalindex,
				poutframe->verts[j].normal);
		}
	}

	return normalfix;
}

/*
=================
Mod_LoadFrames_MD2A

Load the Anachronox md2 format frame
=================
*/
static void
Mod_LoadFrames_MD2A(dmdx_t *pheader, byte *src, size_t inframesize,
	vec3_t translate, int resolution)
{
	int i;

	for (i = 0; i < pheader->num_frames; i++)
	{
		daliasframe_t *pinframe;
		daliasxframe_t *poutframe;
		byte *inverts;
		int j;

		pinframe = (daliasframe_t *) (src + i * inframesize);
		poutframe = (daliasxframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
			poutframe->translate[j] += translate[j];
		}

		inverts = (byte*)pinframe->verts;
		switch(resolution)
		{
			case 0:
				/* Code will multiply vertex values by 255 */
				for (j = 0; j < 3; j++)
				{
					poutframe->scale[j] /= 0xFF;
				}

				/* verts are all 8 bit, so no swapping needed */
				for (j=0; j < pheader->num_xyz; j ++)
				{
					Mod_LoadFrames_VertMD2(poutframe->verts + j, inverts);
					/* 3 bytes vert + 2 bytes */
					inverts += 5;
					/* norm convert logic is unknown */
					memset(poutframe->verts[j].normal, 0,
						sizeof(poutframe->verts[j].normal));
				}
				break;

			case 1:
				/* Code will multiply vertex for normilize */
				poutframe->scale[0] *= (0x7FF / (float)0xFFFF);
				poutframe->scale[1] *= (0x3FF / (float)0xFFFF);
				poutframe->scale[2] *= (0x7FF / (float)0xFFFF);

				/* verts are 32 bit and swap are inside vonvert code*/
				for (j=0; j < pheader->num_xyz; j ++)
				{
					short tmp;

					Mod_LoadFrames_VertDKM2(poutframe->verts + j, *((int *)inverts));

					/* int vert + 2 bytes */
					inverts += 6;

					/* DKM2 has opposite vert list in packed format */
					tmp = poutframe->verts[j].v[0];
					poutframe->verts[j].v[0] = poutframe->verts[j].v[2];
					poutframe->verts[j].v[2] = tmp;

					/* norm convert logic is unknown */
					memset(poutframe->verts[j].normal, 0,
						sizeof(poutframe->verts[j].normal));
				}
				break;

			case 2:
				/* verts are all short, swapped inside func */
				for (j=0; j < pheader->num_xyz; j ++)
				{
					Mod_LoadFrames_VertA(poutframe->verts + j, (short*)inverts);
					/* 6 bytes vert + 2 bytes */
					inverts += 8;
					/* norm convert logic is unknown */
					memset(poutframe->verts[j].normal, 0,
						sizeof(poutframe->verts[j].normal));
				}
				break;

			default:
				/* should never happen */
				break;
		}
	}
}

/*
=================
Mod_LoadMD2TriangleList

Load triangle lists
=================
*/
static void
Mod_LoadMD2TriangleList(dmdx_t *pheader, const dtriangle_t *pintri)
{
	dtriangle_t *pouttri;
	int i;

	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	for (i = 0; i < pheader->num_tris; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}
}

/*
=================
Mod_LoadMDXTriangleList

Load MDX triangle lists
=================
*/
static void
Mod_LoadMDXTriangleList(const char *mod_name, dmdx_t *pheader, const dtriangle_t *pintri,
	const int *glcmds, int num_glcmds)
{
	const dtriangle_t *pouttriofs;
	dmdxmesh_t *mesh_nodes;
	dtriangle_t *pouttri;
	const int *glcmds_end;
	dstvert_t *stvert;
	int m, *mesh_ids;

	pouttriofs = pouttri = (dtriangle_t *) ((char *)pheader + pheader->ofs_tris);
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	stvert = (dstvert_t *)((byte *)pheader + pheader->ofs_st);
	mesh_ids = (int *)calloc(pheader->num_xyz, sizeof(int));
	if (!mesh_ids)
	{
		YQ2_COM_CHECK_OOM(mesh_ids, "calloc()", pheader->num_xyz * sizeof(int))
		return;
	}

	glcmds_end = glcmds + num_glcmds;

	while (1)
	{
		int count, mesh_id;

		/* get the vertex count and primitive type */
		count = LittleLong(*glcmds++);

		if (!count || glcmds >= glcmds_end)
		{
			break; /* done */
		}

		if (count < 0)
		{
			count = -count;
		}

		/* num meshes should be same as subobjects */
		mesh_id = LittleLong(*glcmds++) % pheader->num_meshes;

		do
		{
			int index_xyz;
			vec2_t st;

			memcpy(&st, glcmds, sizeof(st));
			index_xyz = LittleLong(glcmds[2]);

			mesh_ids[index_xyz] = mesh_id;
			stvert[index_xyz].s = LittleFloat(st[0]) * pheader->skinwidth;
			stvert[index_xyz].t = LittleFloat(st[1]) * pheader->skinheight;
			glcmds += 3;
		}
		while (--count);
	}

	for (m = 0; m < pheader->num_meshes; m++)
	{
		int i;

		mesh_nodes[m].ofs_tris = pouttri - pouttriofs;

		for (i = 0; i < pheader->num_tris; i++)
		{
			int j, index[3];

			for (j = 0; j < 3; j++)
			{
				index[j] = LittleShort(pintri[i].index_xyz[j]);
			}

			/* sanity check for verts */
			if (mesh_ids[index[0]] != mesh_ids[index[1]] ||
				mesh_ids[index[1]] != mesh_ids[index[2]])
			{
				Com_DPrintf(
					"%s: %s: Mesh detect could be wrong (%d != %d != %d)\n",
					__func__, mod_name,
					mesh_ids[index[0]], mesh_ids[index[1]], mesh_ids[index[2]]);
			}

			/* use only first vert for mesh detect */
			if (mesh_ids[LittleShort(pintri[i].index_xyz[0])] == m)
			{
				for (j = 0; j < 3; j++)
				{
					pouttri->index_xyz[j] = index[j];
					/* ST is always zero in input */
					pouttri->index_st[j] = index[j];
				}
				pouttri++;
			}
		}

		mesh_nodes[m].num_tris = pouttri - pouttriofs - mesh_nodes[m].ofs_tris;
	}

	free(mesh_ids);
}

/*
=================
Mod_LoadDKMTriangleList

Load DKM triangle lists
=================
*/
static void
Mod_LoadDKMTriangleList(dmdx_t *pheader, const dkmtriangle_t *pintri)
{
	const dtriangle_t *pouttriofs;
	dmdxmesh_t *mesh_nodes;
	dtriangle_t *pouttri;
	int m;

	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	pouttriofs = pouttri = (dtriangle_t *) ((char *)pheader + pheader->ofs_tris);

	for (m = 0; m < pheader->num_meshes; m++)
	{
		int i;

		mesh_nodes[m].ofs_tris = pouttri - pouttriofs;

		for (i = 0; i < pheader->num_tris; i++)
		{
			if (pintri[i].mesh_id == m)
			{
				int j;

				for (j = 0; j < 3; j++)
				{
					pouttri->index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
					pouttri->index_st[j] = LittleShort(pintri[i].index_st[j]);
				}
				pouttri++;
			}
		}

		mesh_nodes[m].num_tris = pouttri - pouttriofs - mesh_nodes[m].ofs_tris;
	}
}

/*
=================
Mod_LoadDKMAnimGroupList

Load DKM animation group lists
=================
*/
static void
Mod_LoadDKMAnimGroupList(dmdx_t *pheader, const byte *iframegroup)
{
	dmdxframegroup_t *pframegroup;
	int i;

	pframegroup = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);

	for (i = 0; i < pheader->num_animgroup; i++)
	{
		memcpy(pframegroup[i].name, iframegroup, 16);
		iframegroup += 16;
		pframegroup[i].ofs = LittleLong(((int*)iframegroup)[0]);
		pframegroup[i].num = LittleLong(((int*)iframegroup)[1]) + 1;
		pframegroup[i].num -= pframegroup[i].ofs;
		iframegroup += 8;

		Mod_UpdateMinMaxByFrames(pheader,
			pframegroup[i].ofs, pframegroup[i].ofs + pframegroup[i].num,
			pframegroup[i].mins, pframegroup[i].maxs);
	}
}

/*
=================
Mod_DkmLoadFrames

Load the DKM v2 frames
=================
*/
static void
Mod_LoadFrames_DKM2(dmdx_t *pheader, const byte *src, size_t inframesize, vec3_t translate)
{
	int i, outframesize;

	outframesize = sizeof(daliasxframe_t) + (pheader->num_xyz - 1) * sizeof(dxtrivertx_t);

	for (i = 0; i < pheader->num_frames; i ++)
	{
		daliasxframe_t *poutframe;
		daliasframe_t *pinframe;
		dxtrivertx_t *outverts;
		byte *inverts;
		int j;

		pinframe = (daliasframe_t *)(src + i * inframesize);
		poutframe = (daliasxframe_t *)((byte *)pheader
			+ pheader->ofs_frames + i * outframesize);

		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
			poutframe->translate[j] += translate[j];
		}

		poutframe->scale[0] *= (0x7FF / (float)0xFFFF);
		poutframe->scale[1] *= (0x3FF / (float)0xFFFF);
		poutframe->scale[2] *= (0x7FF / (float)0xFFFF);

		inverts = (byte *)&pinframe->verts;
		outverts = poutframe->verts;

		/* dkm vert version 2 has unalligned by int size struct */
		for(j = 0; j < pheader->num_xyz; j++)
		{
			Mod_LoadFrames_VertDKM2(outverts + j, *((int *)inverts));
			inverts += sizeof(int);
			/* norm convert logic is unknown */
			memset(outverts[j].normal, 0, sizeof(outverts[j].normal));
			inverts ++;
		}
	}
}

/* Genetate normals based on MD5_ComputeNormals code */
void
Mod_LoadFixNormals(dmdx_t *pheader)
{
	int i, outframesize;
	vec3_t *normals;
	dtriangle_t *pouttri;

	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	outframesize = sizeof(daliasxframe_t) + (pheader->num_xyz - 1) * sizeof(dxtrivertx_t);
	normals = calloc(pheader->num_xyz, sizeof(vec3_t));
	if (!normals)
	{
		YQ2_COM_CHECK_OOM(normals, "calloc()", pheader->num_xyz * sizeof(vec3_t))
		return;
	}

	for (i = 0; i < pheader->num_frames; i ++)
	{
		daliasxframe_t *poutframe;
		dxtrivertx_t *outverts;
		int t;

		poutframe = (daliasxframe_t *)((byte *)pheader
			+ pheader->ofs_frames + i * outframesize);
		outverts = poutframe->verts;

		for(t = 0; t < pheader->num_tris; t ++)
		{
			vec3_t v[3], d1, d2, norm;
			int j;

			/* get verts */
			for (j = 0; j < 3; j ++)
			{
				dxtrivertx_t *dv;
				int k;

				dv = outverts + pouttri[t].index_xyz[j];

				/* convert to vec3 */
				for (k = 0; k < 3; k++)
				{
					v[j][k] = dv->v[k];
				}
			}

			VectorSubtract(v[1], v[0], d1);
			VectorSubtract(v[2], v[0], d2);

			/* scale result before cross product */
			for (j = 0; j < 3; j++)
			{
				d1[j] *= poutframe->scale[j];
				d2[j] *= poutframe->scale[j];
			}

			CrossProduct(d1, d2, norm);
			VectorNormalize(norm);

			/* FIXME: this should be weighted by each vertex angle. */
			for (j = 0; j < 3; j++)
			{
				int index;

				index = pouttri[t].index_xyz[j];
				VectorAdd(normals[index], norm, normals[index]);
			}
		}

		/* save normals */
		for(t = 0; t < pheader->num_xyz; t++)
		{
			int j;

			VectorNormalize(normals[t]);
			/* FIXME: QSS does not have such invert */
			VectorInverse(normals[t]);

			for (j = 0; j < 3; j ++)
			{
				poutframe->verts[t].normal[j] = normals[t][j] * 127;
			}
		}
	}
	free(normals);
}

/*
=================
Mod_LoadModel_MD3
=================
*/
static void *
Mod_LoadModel_MD3(const char *mod_name, const void *buffer, int modfilelen)
{
	int framesize, num_xyz = 0, num_tris = 0, num_glcmds = 0, num_skins = 0,
		meshofs = 0;
	dmdx_t dmdxheader, *pheader;
	md3_header_t pinmodel;
	void *extradata;
	dmdxmesh_t *mesh_nodes;
	dtriangle_t *tris;
	dstvert_t *st;
	dmdx_vert_t *vertx;
	char *skin;
	int i;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	Mod_LittleHeader((int *)buffer, sizeof(pinmodel) / sizeof(int),
		(int *)&pinmodel);

	if (pinmodel.version != ID3_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ID3_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_meshes < 0)
	{
		Com_Printf("%s: model %s file has incorrect meshes count %d\n",
				__func__, mod_name, pinmodel.num_meshes);
		return NULL;
	}

	if (pinmodel.num_frames < 0)
	{
		Com_Printf("%s: model %s file has incorrect frames count %d\n",
				__func__, mod_name, pinmodel.num_frames);
		return NULL;
	}

	meshofs = pinmodel.ofs_meshes;

	for (i = 0; i < pinmodel.num_meshes; i++)
	{
		const md3_mesh_t *md3_mesh = (md3_mesh_t*)((byte*)buffer + meshofs);

		num_xyz += LittleLong(md3_mesh->num_xyz);
		num_tris += LittleLong(md3_mesh->num_tris);
		num_skins += LittleLong(md3_mesh->num_shaders);

		if (pinmodel.num_frames != LittleLong(md3_mesh->num_frames))
		{
			Com_Printf("%s: model %s broken mesh %d\n",
					__func__, mod_name, i);
			return NULL;
		}

		meshofs += LittleLong(md3_mesh->ofs_end);
	}

	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	num_glcmds = (10 * num_tris) + 1 * pinmodel.num_meshes;

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * num_xyz;

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.framesize = framesize;
	dmdxheader.skinheight = 256;
	dmdxheader.skinwidth = 256;
	dmdxheader.num_skins = num_skins;
	dmdxheader.num_glcmds = num_glcmds;
	dmdxheader.num_frames = pinmodel.num_frames;
	dmdxheader.num_xyz = num_xyz;
	dmdxheader.num_meshes = pinmodel.num_meshes;
	dmdxheader.num_st = num_tris * 3;
	dmdxheader.num_tris = num_tris;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = pinmodel.num_frames;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	mesh_nodes = (dmdxmesh_t *)((byte *)pheader + pheader->ofs_meshes);
	tris = (dtriangle_t*)((byte *)pheader + pheader->ofs_tris);
	st = (dstvert_t*)((byte *)pheader + pheader->ofs_st);
	vertx = malloc(pinmodel.num_frames * pheader->num_xyz * sizeof(dmdx_vert_t));
	if (!vertx)
	{
		YQ2_COM_CHECK_OOM(vertx, "malloc()",
			pinmodel.num_frames * pheader->num_xyz * sizeof(dmdx_vert_t))
		return NULL;
	}

	skin = (char *)pheader + pheader->ofs_skins;

	num_xyz = 0;
	num_tris = 0;
	meshofs = pinmodel.ofs_meshes;
	for (i = 0; i < pinmodel.num_meshes; i++)
	{
		const md3_mesh_t *md3_mesh;
		md3_vertex_t *md3_vertex;
		const float *fst;
		const int *p;
		int j;

		md3_mesh = (md3_mesh_t*)((byte*)buffer + meshofs);
		fst = (const float*)((byte*)buffer + meshofs + LittleLong(md3_mesh->ofs_st));

		/* load shaders */
		for (j = 0; j < LittleLong(md3_mesh->num_shaders); j++)
		{
			const md3_shader_t *md3_shader = (md3_shader_t*)((byte*)buffer + meshofs + LittleLong(md3_mesh->ofs_shaders)) + j;

			memcpy(skin, md3_shader->name, Q_min(sizeof(md3_shader->name), MAX_SKINNAME));
			skin += MAX_SKINNAME;
		}

		for (j = 0; j < LittleLong(md3_mesh->num_xyz); j++)
		{
			st[j + num_xyz].s = LittleFloat(fst[j * 2 + 0]) * pheader->skinwidth;
			st[j + num_xyz].t = LittleFloat(fst[j * 2 + 1]) * pheader->skinheight;
		}

		/* load triangles */
		p = (const int*)((byte*)buffer + meshofs + LittleLong(md3_mesh->ofs_tris));

		mesh_nodes[i].ofs_tris = num_tris;
		mesh_nodes[i].num_tris = LittleLong(md3_mesh->num_tris);

		for (j = 0; j < mesh_nodes[i].num_tris; j++)
		{
			int k;

			for (k = 0; k < 3; k++)
			{
				int vert_id;

				/* index */
				vert_id = LittleLong(p[j * 3 + k]) + num_xyz;
				tris[num_tris + j].index_xyz[k] = vert_id;
				tris[num_tris + j].index_st[k] = vert_id;
			}
		}

		md3_vertex = (md3_vertex_t*)((byte*)buffer + meshofs + LittleLong(md3_mesh->ofs_verts));

		for (j = 0; j < pinmodel.num_frames; j ++)
		{
			int k, vert_pos;

			vert_pos = num_xyz + j * pheader->num_xyz;

			for (k = 0; k < LittleLong(md3_mesh->num_xyz); k++, md3_vertex++)
			{
				double npitch, nyaw;
				short normalpitchyaw;

				vertx[vert_pos + k].xyz[0] = (signed short)LittleShort(md3_vertex->origin[0]) * (1.0f / 64.0f);
				vertx[vert_pos + k].xyz[1] = (signed short)LittleShort(md3_vertex->origin[1]) * (1.0f / 64.0f);
				vertx[vert_pos + k].xyz[2] = (signed short)LittleShort(md3_vertex->origin[2]) * (1.0f / 64.0f);

				/* decompress the vertex normal */
				normalpitchyaw = LittleShort(md3_vertex->normalpitchyaw);
				npitch = (normalpitchyaw & 255) * (2 * M_PI) / 256.0;
				nyaw = ((normalpitchyaw >> 8) & 255) * (2 * M_PI) / 256.0;

				vertx[vert_pos + k].norm[0] = (float)(sin(npitch) * cos(nyaw));
				vertx[vert_pos + k].norm[1] = (float)(sin(npitch) * sin(nyaw));
				vertx[vert_pos + k].norm[2] = (float)cos(npitch);
			}
		}

		meshofs += LittleLong(md3_mesh->ofs_end);
		num_xyz += LittleLong(md3_mesh->num_xyz);
		num_tris += LittleLong(md3_mesh->num_tris);
	}

	byte *inframe = (unsigned char*)buffer + pinmodel.ofs_frames;
	for (i = 0; i < pheader->num_frames; i ++)
	{
		daliasxframe_t *frame = (daliasxframe_t *)(
			(byte *)pheader + pheader->ofs_frames + i * pheader->framesize);
		const md3_frameinfo_t *md3_frameinfo = (md3_frameinfo_t*)inframe;

		if (md3_frameinfo->name[0])
		{
			Q_strlcpy(frame->name, md3_frameinfo->name, sizeof(frame->name));
		}
		else
		{
			/* limit frame ids to 2**16 */
			snprintf(frame->name, sizeof(frame->name), "frame%d", i % 0xFFFF);
		}

		PrepareFrameVertex(vertx + i * pheader->num_xyz,
			pheader->num_xyz, frame);

		inframe += sizeof(md3_frameinfo_t);
	}
	free(vertx);

	Mod_LoadAnimGroupList(pheader);
	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}

/*
=================
Mod_LoadModel_MD2A

Anachronox Model
=================
*/
static void *
Mod_LoadModel_MD2A(const char *mod_name, const void *buffer, int modfilelen)
{
	vec3_t translate = {0, 0, 0};
	dmdx_t dmdxheader, *pheader;
	const dtriangle_t *pintri;
	const dstvert_t *pinst;
	dmdxmesh_t *mesh_nodes;
	const int *pincmd;
	dmdla_t pinmodel;
	void *extradata;
	int framesize;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	Mod_LittleHeader((int *)buffer, sizeof(pinmodel) / sizeof(int),
		(int *)&pinmodel);

	if (pinmodel.version != ALIAS_ANACHRONOX_VERSION &&
		pinmodel.version != ALIAS_ANACHRONOX_VERSION_OLD)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ALIAS_ANACHRONOX_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		Com_Printf("%s: model %s file has incorrect skins count %d\n",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.resolution < 0 || pinmodel.resolution > 2)
	{
		Com_Printf("%s: model %s file has incorrect vert type %d\n",
				__func__, mod_name, pinmodel.resolution);
		return NULL;
	}

	if (pinmodel.num_xyz <= 0)
	{
		Com_Printf("%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_xyz > MAX_VERTS)
	{
		Com_Printf("%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (pinmodel.num_st <= 0)
	{
		Com_Printf("%s: model %s has no st vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_tris <= 0)
	{
		Com_Printf("%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_frames <= 0)
	{
		Com_Printf("%s: model %s has no frames\n",
				__func__, mod_name);
		return NULL;
	}

	framesize = sizeof(daliasxframe_t) +
		(pinmodel.num_xyz - 1) * sizeof(dxtrivertx_t);

	/* Copy values as we have mostly same data format */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = pinmodel.skinwidth;
	dmdxheader.skinheight = pinmodel.skinheight;
	dmdxheader.framesize = framesize;
	dmdxheader.num_meshes = 1;
	dmdxheader.num_skins = pinmodel.num_skins;
	dmdxheader.num_xyz = pinmodel.num_xyz;
	dmdxheader.num_st = pinmodel.num_st;
	dmdxheader.num_tris = pinmodel.num_tris;
	dmdxheader.num_glcmds = pinmodel.num_glcmds;
	dmdxheader.num_frames = pinmodel.num_frames;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = pinmodel.num_frames;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = pheader->num_tris;
	mesh_nodes[0].ofs_glcmds = 0;
	mesh_nodes[0].num_glcmds = pheader->num_glcmds;

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (dstvert_t *)((byte *)buffer + pinmodel.ofs_st);
	Mod_LoadSTvertList(pheader, pinst);

	//
	// load triangle lists
	//
	pintri = (dtriangle_t *)((byte *)buffer + pinmodel.ofs_tris);
	Mod_LoadMD2TriangleList(pheader, pintri);

	//
	// load the frames
	//
	Mod_LoadFrames_MD2A(pheader, (byte *)buffer + pinmodel.ofs_frames,
		pinmodel.framesize, translate, pinmodel.resolution);
	Mod_LoadAnimGroupList(pheader);
	Mod_LoadFixNormals(pheader);

	//
	// load the glcmds
	//
	pincmd = (int *)((byte *)buffer + pinmodel.ofs_glcmds);
	Mod_LoadCmdList(mod_name, pheader, pincmd);

	// register all skins
	memcpy((char *)pheader + pheader->ofs_skins, (char *)buffer + pinmodel.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);

	return extradata;
}

/*
=================
Mod_LoadModel_MD2
=================
*/
static void *
Mod_LoadModel_MD2(const char *mod_name, const void *buffer, int modfilelen)
{
	vec3_t translate = {0, 0, 0};
	dmdx_t dmdxheader, *pheader;
	const dtriangle_t *pintri;
	dmdxmesh_t *mesh_nodes;
	const dstvert_t *pinst;
	qboolean normalfix;
	const int *pincmd;
	dmdl_t pinmodel;
	void *extradata;
	int framesize;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	Mod_LittleHeader((int *)buffer, sizeof(pinmodel) / sizeof(int),
		(int *)&pinmodel);

	if (pinmodel.version != ALIAS_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ALIAS_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		Com_Printf("%s: model %s file has incorrect skins count %d\n",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.framesize != (
		sizeof(daliasframe_t) + (pinmodel.num_xyz - 1) * sizeof(dtrivertx_t)))
	{
		Com_Printf("%s: model %s has incorrect framesize\n",
				__func__, mod_name);
		return NULL;
	}

	framesize = sizeof(daliasxframe_t) +
		(pinmodel.num_xyz - 1) * sizeof(dxtrivertx_t);

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = pinmodel.skinwidth;
	dmdxheader.skinheight = pinmodel.skinheight;
	dmdxheader.framesize = framesize;

	dmdxheader.num_meshes = 1;
	dmdxheader.num_skins = pinmodel.num_skins;
	dmdxheader.num_xyz = pinmodel.num_xyz;
	dmdxheader.num_st = pinmodel.num_st;
	dmdxheader.num_tris = pinmodel.num_tris;
	dmdxheader.num_glcmds = pinmodel.num_glcmds;
	dmdxheader.num_frames = pinmodel.num_frames;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = pinmodel.num_frames;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = pheader->num_tris;
	mesh_nodes[0].ofs_glcmds = 0;
	mesh_nodes[0].num_glcmds = pheader->num_glcmds;

	if (pheader->num_xyz <= 0)
	{
		Com_Printf("%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_xyz > MAX_VERTS)
	{
		Com_Printf("%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (pheader->num_st <= 0)
	{
		Com_Printf("%s: model %s has no st vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_tris <= 0)
	{
		Com_Printf("%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_frames <= 0)
	{
		Com_Printf("%s: model %s has no frames\n",
				__func__, mod_name);
		return NULL;
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (dstvert_t *)((byte *)buffer + pinmodel.ofs_st);
	Mod_LoadSTvertList(pheader, pinst);

	//
	// load triangle lists
	//
	pintri = (dtriangle_t *)((byte *)buffer + pinmodel.ofs_tris);
	Mod_LoadMD2TriangleList(pheader, pintri);

	//
	// load the frames
	//
	normalfix = Mod_LoadFrames_MD2(pheader, (byte *)buffer + pinmodel.ofs_frames,
		pinmodel.framesize, translate);

	//
	// Update animation groups by frames
	//
	Mod_LoadAnimGroupList(pheader);

	//
	// load the glcmds
	//
	pincmd = (int *)((byte *)buffer + pinmodel.ofs_glcmds);
	Mod_LoadCmdList(mod_name, pheader, pincmd);

	/* register all skins */
	memcpy((char *)pheader + pheader->ofs_skins, (char *)buffer + pinmodel.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);

	if (normalfix)
	{
		/* look like normals is zero, lets fix it */
		Mod_LoadFixNormals(pheader);
	}

	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}

static void
Mod_LoadModel_FlexNamesFix(dmdx_t *pheader)
{
	dmdxframegroup_t *pframegroup;
	int i;

	pframegroup = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);
	for (i = 0; i < pheader->num_animgroup; i++)
	{
		/* replace frame group started with atack* to attack */
		if (!memcmp(pframegroup[i].name, "atack", 5))
		{
			strcpy(pframegroup[i].name, "attack");
		}
		/* replace frame group started with death* to death */
		else if (!memcmp(pframegroup[i].name, "death", 5))
		{
			strcpy(pframegroup[i].name, "death");
		}
	}

}


/*
=============
Mod_LoadModel_Flex
=============
*/
static void *
Mod_LoadModel_Flex(const char *mod_name, const void *buffer, int modfilelen)
{
	char *src = (char *)buffer;
	int inframesize = 0;
	void *extradata = NULL;
	dmdx_t *pheader = NULL;

	while (modfilelen > 0)
	{
		char blockname[32];
		int version, size;

		memcpy(blockname, src, sizeof(blockname));

		src += sizeof(blockname);
		version = *(int*)src;
		src += sizeof(version);
		size = *(int*)src;
		src += sizeof(size);
		modfilelen = modfilelen - sizeof(blockname) - sizeof(version) - sizeof(size);

		if (Q_strncasecmp(blockname, "header", sizeof(blockname)) == 0)
		{
			const fmheader_t *header = (fmheader_t *)src;
			dmdx_t dmdxheader;
			int framesize;

			if (sizeof(fmheader_t) > size)
			{
				Com_Printf("%s: Too short header\n", __func__);
				return NULL;
			}

			if (version != 2)
			{
				Com_Printf("%s: Invalid %s version %d\n",
					__func__, blockname, version);
				return NULL;
			}

			inframesize = LittleLong(header->framesize);
			/* has same frame structure */
			if (inframesize < (
				sizeof(daliasframe_t) + (LittleLong(header->num_xyz) - 1) * sizeof(dtrivertx_t)))
			{
				Com_Printf("%s: model %s has incorrect framesize\n",
						__func__, mod_name);
				return NULL;
			}

			framesize = sizeof(daliasxframe_t) +
				(LittleLong(header->num_xyz) - 1) * sizeof(dxtrivertx_t);

			/* copy back all values */
			memset(&dmdxheader, 0, sizeof(dmdxheader));
			dmdxheader.skinwidth = LittleLong(header->skinwidth);
			dmdxheader.skinheight = LittleLong(header->skinheight);
			dmdxheader.framesize = framesize;

			dmdxheader.num_skins = LittleLong(header->num_skins);
			dmdxheader.num_xyz = LittleLong(header->num_xyz);
			dmdxheader.num_st = LittleLong(header->num_st);
			dmdxheader.num_tris = LittleLong(header->num_tris);
			dmdxheader.num_glcmds = LittleLong(header->num_glcmds);
			dmdxheader.num_frames = LittleLong(header->num_frames);
			dmdxheader.num_meshes = LittleLong(header->num_mesh_nodes);
			dmdxheader.num_imgbit = 0;
			dmdxheader.num_animgroup = dmdxheader.num_frames;

			if (dmdxheader.num_xyz <= 0)
			{
				Com_Printf("%s: model %s has no vertices\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_xyz > MAX_VERTS)
			{
				Com_Printf("%s: model %s has too many vertices\n",
						__func__, mod_name);
			}

			if (dmdxheader.num_st <= 0)
			{
				Com_Printf("%s: model %s has no st vertices\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_tris <= 0)
			{
				Com_Printf("%s: model %s has no triangles\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_frames <= 0)
			{
				Com_Printf("%s: model %s has no frames\n",
						__func__, mod_name);
				return NULL;
			}

			pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);
		}
		else
		{
			if (!pheader)
			{
				Com_Printf("%s: %s has broken header.\n",
					__func__, mod_name);
				return NULL;
			}
			else if (Q_strncasecmp(blockname, "skin", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_skins * MAX_SKINNAME))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}
				memcpy((char*) pheader + pheader->ofs_skins, src, size);
			}
			else if (Q_strncasecmp(blockname, "st coord", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_st * sizeof(dstvert_t)))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadSTvertList(pheader, (dstvert_t *)src);
			}
			else if (Q_strncasecmp(blockname, "tris", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_tris * sizeof(dtriangle_t)))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadMD2TriangleList(pheader, (dtriangle_t *) src);
			}
			else if (Q_strncasecmp(blockname, "frames", sizeof(blockname)) == 0)
			{
				vec3_t translate = {0, 0, 0};

				if (version != 1)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}

				if (size < (pheader->num_frames *
					(sizeof(daliasframe_t) + (pheader->num_xyz - 1) * sizeof(dtrivertx_t))))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadFrames_MD2(pheader, (byte *)src, inframesize, translate);
				Mod_LoadAnimGroupList(pheader);
				Mod_LoadModel_FlexNamesFix(pheader);
			}
			else if (Q_strncasecmp(blockname, "glcmds", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_glcmds * sizeof(int)))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadCmdList (mod_name, pheader, (int *)src);
			}
			else if (Q_strncasecmp(blockname, "mesh nodes", sizeof(blockname)) == 0)
			{
				int num_mesh_nodes;

				num_mesh_nodes = (pheader->ofs_skins - sizeof(*pheader)) / sizeof(dmdxmesh_t);

				if (version != 3)
				{
					Com_Printf("%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}

				/* 516 mesh node size */
				if (size != (num_mesh_nodes * 516))
				{
					Com_Printf("%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				if (num_mesh_nodes > 0)
				{
					dmdxmesh_t *mesh_nodes;
					char *in_mesh = src;
					int i;

					mesh_nodes = (dmdxmesh_t *)((char*)pheader + sizeof(*pheader));
					for (i = 0; i < num_mesh_nodes; i++)
					{
						int j, min = 256 * 8, max = 0;

						for (j = 0; j < 256; j++)
						{
							if (in_mesh[j])
							{
								if (min > (j * 8))
								{
									int k, v;

									v = in_mesh[j] & 0xFF;

									for (k = 0; k < 8; k ++)
									{
										if ((v & 1))
										{
											min = j * 8 + k;
											break;
										}
										v >>= 1;
									}
								}
								break;
							}
						}

						for (j = (min / 8) - 1; j < 256; j++)
						{
							if (in_mesh[j])
							{
								int v;

								v = in_mesh[j] & 0xFF;
								max = j * 8;

								while (v)
								{
									max ++;
									v >>= 1;
								}
							}
						}

						/* save mesh triangle */
						mesh_nodes[i].ofs_tris = min;
						mesh_nodes[i].num_tris = max - min;

						/* 256 bytes of tri data */
						/* 256 bytes of vert data */
						/* 2 bytes of start */
						/* 2 bytes of number commands */
						in_mesh += 512;
						mesh_nodes[i].ofs_glcmds = LittleShort(*(short *)in_mesh);
						in_mesh += 2;
						mesh_nodes[i].num_glcmds = LittleShort(*(short *)in_mesh);
						in_mesh += 2;
					}
				}
			}
			else if (Q_strncasecmp(blockname, "normals", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "short frames", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "comp data", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "skeleton", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "references", sizeof(blockname)) == 0)
			{
				/* skipped block */
			}
			else
			{
				Com_Printf("%s: %s Unknown block %s\n",
					__func__, mod_name, blockname);
				return NULL;
			}
		}
		modfilelen -= size;
		src += size;
	}

	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}

static void *
Mod_LoadModel_DKM(const char *mod_name, const void *buffer, int modfilelen)
{
	dmdx_t dmdxheader, *pheader = NULL;
	dkm_header_t header = {0};
	void *extradata = NULL;

	if (sizeof(dkm_header_t) > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	Mod_LittleHeader((int *)buffer, sizeof(dkm_header_t) / sizeof(int),
		(int *)&header);

	if (header.version != DKM1_VERSION && header.version != DKM2_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, header.version, DKM2_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, header.ofs_end);
		return NULL;
	}

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = 256;
	dmdxheader.skinheight = 256;
	if (header.version != DKM2_VERSION)
	{
		/* has same frame structure */
		if (header.framesize < (
			sizeof(daliasframe_t) + (header.num_xyz - 1) * sizeof(dtrivertx_t)))
		{
			Com_Printf("%s: model %s has incorrect framesize\n",
					__func__, mod_name);
			return NULL;
		}
	}
	else
	{
		if (header.framesize < (
			sizeof(daliasframe_t) + (header.num_xyz - 1) * (sizeof(int) + sizeof(byte))))
		{
			Com_Printf("%s: model %s has incorrect framesize\n",
					__func__, mod_name);
			return NULL;
		}
	}

	dmdxheader.framesize = sizeof(daliasxframe_t) - sizeof(dxtrivertx_t);
	dmdxheader.framesize += header.num_xyz * sizeof(dxtrivertx_t);

	dmdxheader.num_meshes = header.num_surf;
	dmdxheader.num_skins = header.num_skins;
	dmdxheader.num_xyz = header.num_xyz;
	dmdxheader.num_st = header.num_st;
	dmdxheader.num_tris = header.num_tris;
	dmdxheader.num_frames = header.num_frames;
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	dmdxheader.num_glcmds = (10 * dmdxheader.num_tris) + 1 * dmdxheader.num_meshes;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = header.num_animgroup;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	memcpy((byte*)pheader + pheader->ofs_skins, (byte *)buffer + header.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	Mod_LoadSTvertList(pheader,
		(dstvert_t *)((byte *)buffer + header.ofs_st));
	if (header.version == DKM1_VERSION)
	{
		Mod_LoadFrames_MD2(pheader, (byte *)buffer + header.ofs_frames,
			header.framesize, header.translate);
	}
	else
	{
		Mod_LoadFrames_DKM2(pheader, (byte *)buffer + header.ofs_frames,
			header.framesize, header.translate);
	}

	Mod_LoadDKMTriangleList(pheader,
		(dkmtriangle_t *)((byte *)buffer + header.ofs_tris));
	Mod_LoadDKMAnimGroupList(pheader,
		(byte *)buffer + header.ofs_animgroup);

	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixNormals(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}

static void *
Mod_LoadModel_MDX(const char *mod_name, const void *buffer, int modfilelen)
{
	dmdx_t dmdxheader, *pheader = NULL;
	vec3_t translate = {0, 0, 0};
	mdx_header_t header = {0};
	void *extradata = NULL;

	if (sizeof(mdx_header_t) > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	Mod_LittleHeader((int *)buffer, sizeof(mdx_header_t) / sizeof(int),
		(int *)&header);

	if (header.version != MDX_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, header.version, MDX_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, header.ofs_end);
		return NULL;
	}

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = header.skinwidth;
	dmdxheader.skinheight = header.skinwidth;
	/* has same frame structure */
	if (header.framesize < (
		sizeof(daliasframe_t) + (header.num_xyz - 1) * sizeof(dtrivertx_t)))
	{
		Com_Printf("%s: model %s has incorrect framesize\n",
				__func__, mod_name);
		return NULL;
	}

	dmdxheader.framesize = sizeof(daliasxframe_t) - sizeof(dxtrivertx_t);
	dmdxheader.framesize += header.num_xyz * sizeof(dxtrivertx_t);

	dmdxheader.num_meshes = header.num_subobj;
	dmdxheader.num_skins = header.num_skins;
	dmdxheader.num_xyz = header.num_xyz;
	dmdxheader.num_st = header.num_xyz;
	dmdxheader.num_tris = header.num_tris;
	dmdxheader.num_frames = header.num_frames;
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	dmdxheader.num_glcmds = (10 * dmdxheader.num_tris) + 1 * dmdxheader.num_meshes;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = header.num_frames;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	memcpy((byte*)pheader + pheader->ofs_skins, (byte *)buffer + header.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	Mod_LoadMDXTriangleList(mod_name, pheader,
		(dtriangle_t *)((byte *)buffer + header.ofs_tris),
		(int*)((byte *)buffer + header.ofs_glcmds),
		header.num_glcmds);
	Mod_LoadFrames_MD2(pheader, (byte *)buffer + header.ofs_frames,
		header.framesize, translate);
	Mod_LoadAnimGroupList(pheader);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}

/*
=================
Mod_LoadModelFile
=================
*/
void *
Mod_LoadModelFile(const char *mod_name, const void *buffer, int modfilelen)
{
	void *extradata = NULL;

	/* code needs at least 2 ints for detect file type */
	if (!buffer || modfilelen < (sizeof(unsigned) * 2))
	{
		return NULL;
	}

	switch (LittleLong(*(unsigned *)buffer))
	{
		case MDAHEADER:
			extradata = Mod_LoadModel_MDA(mod_name, buffer, modfilelen);
			break;

		case SDEFHEADER:
			extradata = Mod_LoadModel_SDEF(mod_name, buffer, modfilelen);
			break;

		case MDXHEADER:
			extradata = Mod_LoadModel_MDX(mod_name, buffer, modfilelen);
			break;

		case DKMHEADER:
			extradata = Mod_LoadModel_DKM(mod_name, buffer, modfilelen);
			break;

		case RAVENFMHEADER:
			extradata = Mod_LoadModel_Flex(mod_name, buffer, modfilelen);
			break;

		case IDALIASHEADER:
			{
				/* next short after file type */
				short version;

				version = LittleShort(((short*)buffer)[2]);
				if (version == ALIAS_ANACHRONOX_VERSION ||
					version == ALIAS_ANACHRONOX_VERSION_OLD)
				{
					extradata = Mod_LoadModel_MD2A(mod_name, buffer, modfilelen);
				}
				else
				{
					extradata = Mod_LoadModel_MD2(mod_name, buffer, modfilelen);
				}
			}
			break;

		case IDMDLHEADER:
			extradata = Mod_LoadModel_MDL(mod_name, buffer, modfilelen);
			break;

		case IDHLMDLHEADER:
			extradata = Mod_LoadModel_HLMDL(mod_name, buffer, modfilelen);
			break;

		case ID3HEADER:
			extradata = Mod_LoadModel_MD3(mod_name, buffer, modfilelen);
			break;

		case MDR_IDENT:
			extradata = Mod_LoadModel_MDR(mod_name, buffer, modfilelen);
			break;

		case IDMD5HEADER:
			extradata = Mod_LoadModel_MD5(mod_name, buffer, modfilelen);
			break;

		case IDSPRITEHEADER:
			extradata = Mod_LoadSprite_SP2(mod_name, buffer, modfilelen);
			break;

		case IDBKHEADER:
			extradata = Mod_LoadSprite_BK(mod_name, buffer, modfilelen);
			break;

		case IDQ1SPRITEHEADER:
			extradata = Mod_LoadSprite_SPR(mod_name, buffer, modfilelen);
			break;
	}

	return extradata;
}
