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

static const float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

/* compressed vertex normals used by mdl and md2 model formats */
static void
Mod_ConvertNormalMDL(byte in_normal, signed char *normal)
{
	const float *norm;
	int n;

	norm = r_avertexnormals[in_normal % NUMVERTEXNORMALS];

	for (n = 0; n < 3; n ++)
	{
		normal[n] = norm[n] * 127.f;
	}
}

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
=================
Mod_LoadAnimGroupList

Generate animations groups from frames
=================
*/
void
Mod_LoadAnimGroupList(dmdx_t *pheader)
{
	char newname[16] = {0}, oldname[16] = {0};
	int i, oldframe = 0, currgroup = 0;
	dmdxframegroup_t *pframegroup;

	pframegroup = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);

	for (i = 0; i < pheader->num_frames; i++)
	{
		daliasxframe_t *poutframe;

		poutframe = (daliasxframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		if (poutframe->name[0])
		{
			size_t j;

			Q_strlcpy(newname, poutframe->name, sizeof(poutframe->name));
			for (j = strlen(newname) - 1; j > 0; j--)
			{
				if ((newname[j] >= '0' && newname[j] <= '9') ||
					(newname[j] == '_'))
				{
					newname[j] = 0;
					continue;
				}
				break;
			}
		}
		else
		{
			*newname = 0;
		}

		if (strcmp(newname, oldname))
		{
			if ((i != oldframe) && (currgroup < pheader->num_animgroup))
			{
				Q_strlcpy(pframegroup[currgroup].name, oldname,
					sizeof(pframegroup[currgroup].name));
				pframegroup[currgroup].ofs = oldframe;
				pframegroup[currgroup].num = i - oldframe;
				currgroup++;
			}
			strcpy(oldname, newname);
			oldframe = i;
		}
	}

	if (currgroup < pheader->num_animgroup)
	{
		Q_strlcpy(pframegroup[currgroup].name, oldname,
			sizeof(pframegroup[currgroup].name));
		pframegroup[currgroup].ofs = oldframe;
		pframegroup[currgroup].num = i - oldframe;
		currgroup++;
	}

	pheader->num_animgroup = currgroup;

	for (i = 0; i < pheader->num_animgroup; i++)
	{
		Mod_UpdateMinMaxByFrames(pheader,
			pframegroup[i].ofs, pframegroup[i].ofs + pframegroup[i].num,
			pframegroup[i].mins, pframegroup[i].maxs);
	}
}

/*
 * verts as bytes
 */
static void
Mod_LoadFrames_VertMD2(dxtrivertx_t *vert, const byte *in)
{
	int k;

	for (k=0; k < 3; k++)
	{
		vert->v[k] = in[k] * 0xFF;
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
Mod_LoadFrames_SAM

Load the SiN sam animation format frames
=================
*/
static void
Mod_LoadFrames_SAM(dmdx_t *pheader, sin_sam_header_t **anims, def_entry_t *animations,
	int anim_num, vec3_t translate, float scale)
{
	dmdxframegroup_t *framegroups;
	int curr_frame = 0, i;

	framegroups = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);

	for (i = 0; i < anim_num; i++)
	{
		sin_frame_t *pinframe;
		int k;

		/* create single animation group by default*/
		Q_strlcpy(framegroups[i].name, animations[i].name,
			sizeof(framegroups[i].name));
		framegroups[i].ofs = curr_frame;
		framegroups[i].num = anims[i]->num_frames;

		pinframe = (sin_frame_t *) ((char *)anims[i] + anims[i]->ofs_frames);

		for (k = 0; k < anims[i]->num_frames; k++)
		{
			daliasxframe_t *poutframe;
			dtrivertx_t *verts;
			int j;

			poutframe = (daliasxframe_t *) ((byte *)pheader
				+ pheader->ofs_frames + curr_frame * pheader->framesize);

			memcpy(poutframe->name, anims[i]->name, sizeof(poutframe->name));

			for (j = 0; j < 3; j++)
			{
				poutframe->scale[j] = LittleFloat(pinframe[k].scale[j]) * scale / 0xFF;
				poutframe->translate[j] = LittleFloat(pinframe[k].translate[j]);
				poutframe->translate[j] += translate[j];
			}

			verts = (dtrivertx_t*)((char *)(pinframe + k) + pinframe[k].ofs_verts);

			/* verts are all 8 bit, so no swapping needed */
			for (j=0; j < pheader->num_xyz; j ++)
			{
				Mod_LoadFrames_VertMD2(poutframe->verts + j, verts[j].v);
				Mod_ConvertNormalMDL(verts[j].lightnormalindex,
					poutframe->verts[j].normal);
			}

			curr_frame ++;
		}

		Mod_UpdateMinMaxByFrames(pheader,
			framegroups[i].ofs, framegroups[i].ofs + framegroups[i].num,
			framegroups[i].mins, framegroups[i].maxs);
	}
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
static void
Mod_LoadFixNormals(dmdx_t *pheader)
{
	int i, outframesize;
	vec3_t *normals;
	dtriangle_t *pouttri;

	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	outframesize = sizeof(daliasxframe_t) + (pheader->num_xyz - 1) * sizeof(dxtrivertx_t);
	normals = calloc(pheader->num_xyz, sizeof(vec3_t));

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

void
Mod_LoadFixImages(const char* mod_name, dmdx_t *pheader, qboolean internal)
{
	int i;

	if (!pheader)
	{
		return;
	}

	for (i = 0; i < pheader->num_skins; i++)
	{
		char *skin;

		skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;
		skin[MAX_SKINNAME - 1] = 0;

		/* fix skin backslash */
		Q_replacebackslash(skin);

		Com_DPrintf("%s: %s #%d: Should load %s '%s'\n",
			__func__, mod_name, i, internal ? "internal": "external", skin);
	}

}

/*
 * Calculate offsets and allocate memory for model
 */
dmdx_t *
Mod_LoadAllocate(const char *mod_name, dmdx_t *dmdxheader, void **extradata)
{
	dmdx_t *pheader = NULL;

	dmdxheader->ident = IDALIASHEADER;
	dmdxheader->version = 0;
	/* just skip header */
	dmdxheader->ofs_meshes = sizeof(dmdx_t);
	dmdxheader->ofs_skins = dmdxheader->ofs_meshes + dmdxheader->num_meshes * sizeof(dmdxmesh_t);
	dmdxheader->ofs_st = dmdxheader->ofs_skins + dmdxheader->num_skins * MAX_SKINNAME;
	dmdxheader->ofs_tris = dmdxheader->ofs_st + dmdxheader->num_st * sizeof(dstvert_t);
	dmdxheader->ofs_frames = dmdxheader->ofs_tris + dmdxheader->num_tris * sizeof(dtriangle_t);
	dmdxheader->ofs_glcmds = dmdxheader->ofs_frames + dmdxheader->num_frames * dmdxheader->framesize;
	dmdxheader->ofs_imgbit = dmdxheader->ofs_glcmds + dmdxheader->num_glcmds * sizeof(int);
	dmdxheader->ofs_animgroup = dmdxheader->ofs_imgbit + (
		dmdxheader->skinwidth * dmdxheader->skinheight *
		dmdxheader->num_skins * dmdxheader->num_imgbit / 8
	);
	dmdxheader->ofs_end = dmdxheader->ofs_animgroup + dmdxheader->num_animgroup * sizeof(dmdxframegroup_t);

	*extradata = Hunk_Begin(dmdxheader->ofs_end);
	pheader = Hunk_Alloc(dmdxheader->ofs_end);

	memcpy(pheader, dmdxheader, sizeof(dmdx_t));

	return pheader;
}

/* get full count of frames */
static int
Mod_LoadMDLCountFrames(const char *mod_name, const byte *buffer,
	int num_skins, int skinwidth, int skinheight, int num_st, int num_tris,
	int num_frames, int num_xyz)
{
	const byte *curr_pos;
	int i, frame_count;

	curr_pos = (byte*)buffer + sizeof(mdl_header_t);

	/* check all skins */
	for (i = 0; i < num_skins; ++i)
	{
		int skin_type;

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		curr_pos += sizeof(int);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
				__func__, mod_name, skin_type);
			return -1;
		}

		curr_pos += skinwidth * skinheight;
	}

	/* texcoordinates */
	curr_pos += sizeof(mdl_texcoord_t) * num_st;

	/* triangles */
	curr_pos += sizeof(mdl_triangle_t) * num_tris;

	frame_count = 0;

	/* register all frames */
	for (i = 0; i < num_frames; ++i)
	{
		int frame_type, frames_skip;

		/* Read frame data */
		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		frame_type = LittleLong(((int *)curr_pos)[0]);
		curr_pos += sizeof(int);

		frames_skip = 1;

		if (frame_type)
		{
			frames_skip = LittleLong(((int *)curr_pos)[0]);
			/* skip count of frames */
			curr_pos += sizeof(int);
			/* skip bboxmin, bouding box min */
			curr_pos += sizeof(dtrivertx_t);
			/* skip bboxmax, bouding box max */
			curr_pos += sizeof(dtrivertx_t);

			/* skip intervals */
			curr_pos += frames_skip * sizeof(float);
		}

		/* next frames in frame group is unsupported */
		curr_pos += frames_skip * (
			/* bouding box */
			sizeof(dtrivertx_t) * 2 +
			/* name */
			16 +
			/* verts */
			sizeof(dtrivertx_t) * num_xyz
		);

		frame_count += frames_skip;
	}

	return frame_count;
}

const byte *
Mod_LoadMDLSkins(const char *mod_name, dmdx_t *pheader, const byte *curr_pos)
{
	int i;

	/* register all skins */
	for (i = 0; i < pheader->num_skins; ++i)
	{
		char *out_pos;
		int skin_type;

		out_pos = (char*)pheader + pheader->ofs_skins;
		snprintf(out_pos + MAX_SKINNAME * i, MAX_SKINNAME, "%s#%d.lmp", mod_name, i);

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		curr_pos += sizeof(int);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
					__func__, mod_name, skin_type);
			return NULL;
		}

		/* copy 8bit image */
		memcpy((byte*)pheader + pheader->ofs_imgbit +
				(pheader->skinwidth * pheader->skinheight * i),
				curr_pos,
				pheader->skinwidth * pheader->skinheight);
		curr_pos += pheader->skinwidth * pheader->skinheight;
	}

	return curr_pos;
}

/*
=================
Mod_LoadModel_MDL
=================
*/
static void *
Mod_LoadModel_MDL(const char *mod_name, const void *buffer, int modfilelen)
{
	const mdl_header_t *pinmodel;
	dmdx_t dmdxheader, *pheader;
	dmdxmesh_t *mesh_nodes;
	void *extradata;
	int version;

	/* local copy of all values */
	int skinwidth, skinheight, framesize;
	int num_meshes, num_skins, num_xyz, num_st, num_tris, num_glcmds,
		num_frames, frame_count, num_animgroup;

	pinmodel = (mdl_header_t *)buffer;

	version = LittleLong(pinmodel->version);
	if (version != MDL_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, version, MDL_VERSION);
		return NULL;
	}

	/* generate all offsets and sizes */
	num_meshes = 1;
	num_skins = LittleLong(pinmodel->num_skins);
	skinwidth = LittleLong(pinmodel->skinwidth);
	skinheight = LittleLong(pinmodel->skinheight);
	num_xyz = LittleLong(pinmodel->num_xyz);
	num_st = num_xyz;
	num_tris = LittleLong(pinmodel->num_tris);
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	num_glcmds = (10 * num_tris) + 1;
	num_frames = LittleLong(pinmodel->num_frames);
	num_animgroup = num_frames;

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * (num_xyz - 1);

	/* validate */
	if (num_xyz <= 0)
	{
		Com_Printf("%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (num_xyz > MAX_VERTS)
	{
		Com_Printf("%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (num_tris <= 0)
	{
		Com_Printf("%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (num_frames <= 0)
	{
		Com_Printf("%s: model %s has no frames\n",
				__func__, mod_name);
		return NULL;
	}

	frame_count = Mod_LoadMDLCountFrames(mod_name, buffer, num_skins,
		skinwidth, skinheight, num_st, num_tris, num_frames, num_xyz);

	if (frame_count <= 0)
	{
		Com_Printf("%s: model %s has issues with frame count\n",
				__func__, mod_name);
		return NULL;
	}

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = skinwidth;
	dmdxheader.skinheight = skinheight;
	dmdxheader.framesize = framesize;

	dmdxheader.num_meshes = num_meshes;
	dmdxheader.num_skins = num_skins;
	dmdxheader.num_xyz = num_xyz;
	dmdxheader.num_st = num_st * 2;
	dmdxheader.num_tris = num_tris;
	dmdxheader.num_glcmds = num_glcmds;
	dmdxheader.num_imgbit = 8;
	dmdxheader.num_frames = frame_count;
	dmdxheader.num_animgroup = num_animgroup;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = num_tris;

	{
		int i;
		const byte *curr_pos;

		curr_pos = (byte*)buffer + sizeof(mdl_header_t);

		curr_pos = Mod_LoadMDLSkins(mod_name, pheader, curr_pos);
		if (!curr_pos)
		{
			return NULL;
		}

		/* texcoordinates */
		{
			const mdl_texcoord_t *texcoords;
			dstvert_t *poutst;

			poutst = (dstvert_t *) ((byte *)pheader + pheader->ofs_st);

			texcoords = (mdl_texcoord_t *)curr_pos;
			curr_pos += sizeof(mdl_texcoord_t) * num_st;

			for(i = 0; i < num_st; i++)
			{
				int s, t;

				/* Compute texture coordinates */
				s = LittleLong(texcoords[i].s);
				t = LittleLong(texcoords[i].t);

				poutst[i * 2].s = s;
				poutst[i * 2].t = t;
				poutst[i * 2 + 1].s = s;
				poutst[i * 2 + 1].t = t;

				if (texcoords[i].onseam)
				{
					/* Backface */
					poutst[i * 2 + 1].s += skinwidth >> 1;
				}
			}
		}

		/* triangles */
		{
			const mdl_triangle_t *triangles;
			dtriangle_t *pouttri;

			pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

			triangles = (mdl_triangle_t *) curr_pos;
			curr_pos += sizeof(mdl_triangle_t) * num_tris;

			for (i = 0; i < num_tris; i++)
			{
				int j;

				for (j = 0; j < 3; j++)
				{
					int index;

					index = LittleLong(triangles[i].vertex[j]);
					pouttri[i].index_xyz[j] = index;
					pouttri[i].index_st[j] = index * 2;

					if (!triangles[i].facesfront)
					{
						pouttri[i].index_st[j] ++;
					}
				}
			}
		}

		frame_count = 0;

		/* register all frames */
		for (i = 0; i < num_frames; ++i)
		{
			int frame_type, k, frames_skip;
			vec3_t scale, translate;

			scale[0] = LittleFloat(pinmodel->scale[0]) / 0xFF;
			scale[1] = LittleFloat(pinmodel->scale[1]) / 0xFF;
			scale[2] = LittleFloat(pinmodel->scale[2]) / 0xFF;

			translate[0] = LittleFloat(pinmodel->translate[0]);
			translate[1] = LittleFloat(pinmodel->translate[1]);
			translate[2] = LittleFloat(pinmodel->translate[2]);

			/* Read frame data */
			/* skip type / int */
			/* 0 = simple, !0 = group */
			frame_type = LittleLong(((int *)curr_pos)[0]);
			curr_pos += sizeof(int);

			frames_skip = 1;

			if (frame_type)
			{
				frames_skip = LittleLong(((int *)curr_pos)[0]);
				/* skip count of frames */
				curr_pos += sizeof(int);
				/* skip bboxmin, bouding box min */
				curr_pos += sizeof(dtrivertx_t);
				/* skip bboxmax, bouding box max */
				curr_pos += sizeof(dtrivertx_t);

				/* skip intervals */
				curr_pos += frames_skip * sizeof(float);
			}

			for (k = 0; k < frames_skip; k ++)
			{
				dxtrivertx_t* poutvertx;
				dtrivertx_t *pinvertx;
				daliasframe_t *frame;
				int j;

				frame = (daliasframe_t *) ((byte *)pheader + pheader->ofs_frames +
					frame_count * framesize);
				frame->scale[0] = scale[0];
				frame->scale[1] = scale[1];
				frame->scale[2] = scale[2];

				frame->translate[0] = translate[0];
				frame->translate[1] = translate[1];
				frame->translate[2] = translate[2];

				/* skip bboxmin, bouding box min */
				curr_pos += sizeof(dtrivertx_t);
				/* skip bboxmax, bouding box max */
				curr_pos += sizeof(dtrivertx_t);

				memcpy(&frame->name, curr_pos, sizeof(char) * 16);
				curr_pos += sizeof(char) * 16;

				poutvertx = (dxtrivertx_t*)&frame->verts[0];
				pinvertx = (dtrivertx_t*)curr_pos;

				/* verts are all 8 bit, so no swapping needed */
				for (j=0; j < num_xyz; j ++)
				{
					Mod_LoadFrames_VertMD2(poutvertx + j, pinvertx[j].v);
					Mod_ConvertNormalMDL(pinvertx[j].lightnormalindex,
						poutvertx[j].normal);
				}

				curr_pos += sizeof(dtrivertx_t) * num_xyz;
				frame_count++;
			}
		}
	}

	Mod_LoadAnimGroupList(pheader);
	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, true);

	return extradata;
}

/* glcmds generation */
static int
Mod_LoadCmdStripLength(int starttri, int startv, dtriangle_t *triangles, int num_tris,
	byte *used, int *strip_xyz, int *strip_st, int *strip_tris)
{
	int m1, m2;
	int st1, st2;
	int j;
	dtriangle_t *last, *check;
	int k;
	int stripcount;

	used[starttri] = 2;

	last = &triangles[starttri];

	strip_xyz[0] = last->index_xyz[(startv  )%3];
	strip_xyz[1] = last->index_xyz[(startv+1)%3];
	strip_xyz[2] = last->index_xyz[(startv+2)%3];
	strip_st[0] = last->index_st[(startv  )%3];
	strip_st[1] = last->index_st[(startv+1)%3];
	strip_st[2] = last->index_st[(startv+2)%3];

	strip_tris[0] = starttri;
	stripcount = 1;

	m1 = last->index_xyz[(startv+2)%3];
	st1 = last->index_st[(startv+2)%3];
	m2 = last->index_xyz[(startv+1)%3];
	st2 = last->index_st[(startv+1)%3];

	/* look for a matching triangle */
nexttri:
	for (j = starttri + 1, check = &triangles[starttri + 1]; j < num_tris; j++, check++)
	{
		for (k = 0; k < 3; k++)
		{
			if ((check->index_xyz[k] != m1) ||
				(check->index_st[k] != st1) ||
				(check->index_xyz[(k+1)%3] != m2) ||
				(check->index_st[(k+1)%3] != st2))
			{
				continue;
			}

			/* this is the next part of the fan */

			/* if we can't use this triangle, this tristrip is done */
			if (used[j])
			{
				goto done;
			}

			/* the new edge */
			if (stripcount & 1)
			{
				m2 = check->index_xyz[(k+2)%3];
				st2 = check->index_st[(k+2)%3];
			}
			else
			{
				m1 = check->index_xyz[(k+2)%3];
				st1 = check->index_st[(k+2)%3];
			}

			strip_xyz[stripcount+2] = check->index_xyz[(k+2)%3];
			strip_st[stripcount+2] = check->index_st[(k+2)%3];
			strip_tris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	/* clear the temp used flags */
	for (j = starttri + 1; j < num_tris; j++)
	{
		if (used[j] == 2)
		{
			used[j] = 0;
		}
	}

	return stripcount;
}

static int
Mod_LoadCmdFanLength(int starttri, int startv, dtriangle_t *triangles, int num_tris,
	byte *used, int *strip_xyz, int *strip_st, int *strip_tris)
{
	int m1, m2;
	int st1, st2;
	int j;
	dtriangle_t *last, *check;
	int k;
	int stripcount;

	used[starttri] = 2;

	last = &triangles[starttri];

	strip_xyz[0] = last->index_xyz[(startv  )%3];
	strip_xyz[1] = last->index_xyz[(startv+1)%3];
	strip_xyz[2] = last->index_xyz[(startv+2)%3];
	strip_st[0] = last->index_st[(startv  )%3];
	strip_st[1] = last->index_st[(startv+1)%3];
	strip_st[2] = last->index_st[(startv+2)%3];

	strip_tris[0] = starttri;
	stripcount = 1;

	m1 = last->index_xyz[(startv+0)%3];
	st1 = last->index_st[(startv+0)%3];
	m2 = last->index_xyz[(startv+2)%3];
	st2 = last->index_st[(startv+2)%3];


	/* look for a matching triangle */
nexttri:
	for (j = starttri + 1, check = &triangles[starttri + 1]; j < num_tris; j++, check++)
	{
		for (k = 0; k < 3; k++)
		{
			if ((check->index_xyz[k] != m1) ||
				(check->index_st[k] != st1) ||
				(check->index_xyz[(k+1)%3] != m2) ||
				(check->index_st[(k+1)%3] != st2))
			{
				continue;
			}

			/* this is the next part of the fan */

			/* if we can't use this triangle, this tristrip is done */
			if (used[j])
			{
				goto done;
			}

			/* the new edge */
			m2 = check->index_xyz[(k+2)%3];
			st2 = check->index_st[(k+2)%3];

			strip_xyz[stripcount+2] = m2;
			strip_st[stripcount+2] = st2;
			strip_tris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	/* clear the temp used flags */
	for (j = starttri + 1; j < num_tris; j++)
	{
		if (used[j] == 2)
		{
			used[j] = 0;
		}
	}

	return stripcount;
}

int
Mod_LoadCmdCompress(const dstvert_t *texcoords, dtriangle_t *triangles, int num_tris,
	int *commands, int skinwidth, int skinheight)
{
	int i, j, numcommands = 0, startv, len, bestlen, type, besttype = -1;
	int *strip_xyz, *strip_st, *strip_tris;
	int *best_xyz, *best_st, *best_tris;
	byte *used;

	used = (byte*)calloc(num_tris, sizeof(*used));
	best_xyz = (int*)calloc(num_tris * 3, sizeof(*best_xyz));
	best_st = (int*)calloc(num_tris * 3, sizeof(*best_xyz));
	best_tris = (int*)calloc(num_tris * 3, sizeof(*best_xyz));
	strip_xyz = (int*)calloc(num_tris * 3, sizeof(*strip_xyz));
	strip_st = (int*)calloc(num_tris * 3, sizeof(*strip_xyz));
	strip_tris = (int*)calloc(num_tris * 3, sizeof(*strip_xyz));

	for (i = 0; i < num_tris; i++)
	{
		/* pick an unused triangle and start the trifan */
		if (used[i])
		{
			continue;
		}

		bestlen = 0;
		for (type = 0; type < 2; type++)
		{
			for (startv = 0; startv < 3; startv++)
			{
				if (type == 1)
				{
					len = Mod_LoadCmdStripLength(i, startv, triangles, num_tris,
						used, strip_xyz, strip_st, strip_tris);
				}
				else
				{
					len = Mod_LoadCmdFanLength(i, startv, triangles, num_tris,
						used, strip_xyz, strip_st, strip_tris);
				}

				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;

					for (j = 0; j < bestlen + 2; j++)
					{
						best_st[j] = strip_st[j];
						best_xyz[j] = strip_xyz[j];
					}

					for (j = 0; j < bestlen; j++)
					{
						best_tris[j] = strip_tris[j];
					}
				}
			}
		}

		/* mark the tris on the best strip/fan as used */
		for (j = 0; j < bestlen; j++)
		{
			used[best_tris[j]] = 1;
		}

		if (besttype == 1)
		{
			commands[numcommands++] = (bestlen+2);
		}
		else
		{
			commands[numcommands++] = -(bestlen+2);
		}

		for (j = 0; j < bestlen + 2; j++)
		{
			vec2_t cmdst;

			/* st */
			cmdst[0] = (texcoords[best_st[j]].s + 0.5f) / skinwidth;
			cmdst[1] = (texcoords[best_st[j]].t + 0.5f) / skinheight;
			memcpy(commands + numcommands, &cmdst, sizeof(cmdst));
			numcommands += 2;

			/* vert id */
			commands[numcommands++] = best_xyz[j];
		}
	}

	commands[numcommands++] = 0; /* end of list marker */

	free(best_xyz);
	free(best_st);
	free(best_tris);
	free(strip_xyz);
	free(strip_st);
	free(strip_tris);
	free(used);

	return numcommands;
}

static int *
Mod_LoadSTLookup(dmdx_t *pheader)
{
	const dstvert_t* st;
	int *st_lookup;
	int k;

	st = (dstvert_t*)((byte *)pheader + pheader->ofs_st);

	st_lookup = calloc(pheader->num_st, sizeof(int));

	for(k = 1; k < pheader->num_st; k++)
	{
		int j;

		st_lookup[k] = k;

		for(j = 0; j < k; j++)
		{
			if ((st[j].s == st[k].s) && (st[j].t == st[k].t))
			{
				/* same value */
				st_lookup[k] = j;
				break;
			}
		}
	}
	return st_lookup;
}

static void
Mod_LoadTrisCompress(dmdx_t *pheader)
{
	int *st_lookup;
	dtriangle_t *tris;
	int i;

	st_lookup = Mod_LoadSTLookup(pheader);

	tris = (dtriangle_t*)((byte *)pheader + pheader->ofs_tris);
	for (i = 0; i < pheader->num_tris; i++)
	{
		int k;

		for (k = 0; k < 3; k++)
		{
			tris->index_st[k] = st_lookup[tris->index_st[k]];
		}

		tris++;
	}

	free(st_lookup);
}

void
Mod_LoadCmdGenerate(dmdx_t *pheader)
{
	dmdxmesh_t *mesh_nodes;
	const int *baseglcmds;
	int num_tris, i;
	int *pglcmds;

	Mod_LoadTrisCompress(pheader);

	baseglcmds = pglcmds = (int *)((byte *)pheader + pheader->ofs_glcmds);
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);

	num_tris = 0;
	for (i = 0; i < pheader->num_meshes; i++)
	{
		mesh_nodes[i].ofs_glcmds = pglcmds - baseglcmds;

		/* write glcmds */
		mesh_nodes[i].num_glcmds = Mod_LoadCmdCompress(
			(dstvert_t*)((byte *)pheader + pheader->ofs_st),
			(dtriangle_t*)((byte *)pheader + pheader->ofs_tris) + num_tris,
			mesh_nodes[i].num_tris,
			pglcmds,
			pheader->skinwidth, pheader->skinheight);

		pglcmds += mesh_nodes[i].num_glcmds;
		num_tris += mesh_nodes[i].num_tris;
	}

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

	for (i=0 ; i < sizeof(pinmodel) / sizeof(int) ; i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

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
	int i, framesize;
	void *extradata;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

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
	int i, framesize;
	dmdl_t pinmodel;
	void *extradata;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

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
	int i;

	if (sizeof(dkm_header_t) > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	for (i=0 ; i<sizeof(dkm_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong(((int *)buffer)[i]);

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
	int i;

	if (sizeof(mdx_header_t) > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	for (i=0 ; i<sizeof(mdx_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong(((int *)buffer)[i]);

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

static void *
Mod_LoadModel_SDEF_Text(const char *mod_name, char *curr_buff)
{
	char models_path[MAX_QPATH];
	char base_model[MAX_QPATH * 2] = {0};
	def_entry_t *skinnames = NULL;
	def_entry_t *animations = NULL;
	int actions_num, skinnames_num, i, base_size = -1;
	dmdx_t dmdxheader, *pheader = NULL;
	sin_sbm_header_t *base;
	sin_sam_header_t **anim;
	void *extradata = NULL;
	vec3_t translate = {0, 0, 0};
	float scale;
	int framescount = 0;
	int animation_num = 0;
	int num_tris = 0;
	sin_trigroup_t *trigroup;
	char prevvalue[MAX_SKINNAME] = {0};

	actions_num = 0;
	skinnames_num = 0;
	scale = 1.0f;
	memset(models_path, 0, sizeof(models_path));

	if (strcmp(COM_Parse(&curr_buff), "SDEF"))
	{
		return NULL;
	}

	while (curr_buff)
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!*token)
		{
			continue;
		}

		/* found start of comment */
		else if (!strncmp(token, "/*", 2))
		{
			token = COM_Parse(&curr_buff);

			/* search end of comment */
			while (curr_buff && strncmp(token, "*/", 2))
			{
				token = COM_Parse(&curr_buff);
			}
		}
		else if (!strcmp(token, "path"))
		{
			Q_strlcpy(models_path, COM_Parse(&curr_buff), sizeof(models_path));
		}
		else if (!strcmp(token, "scale"))
		{
			scale = (float)strtod(COM_Parse(&curr_buff), (char **)NULL);
		}
		else if (!strcmp(token, "id") ||
				 !strcmp(token, "group") ||
				 !strcmp(token, "!init:") ||
				 !strcmp(token, "!main:") ||
				 !strcmp(token, "client") ||
				 !strcmp(token, "server"))
		{
			/* Just skipped for now */
			curr_buff = strchr(curr_buff, '\n');
			if (curr_buff && *curr_buff == '\n')
			{
				curr_buff ++;
			}
		}
		else
		{
			const char *ext;

			ext = COM_FileExtension(token);
			if (!Q_stricmp(ext, "sbm"))
			{
				snprintf(base_model, sizeof(base_model),
					"%s/%s", models_path, token);
				prevvalue[0] = 0;
			}
			else if (!Q_stricmp(ext, "sam"))
			{
				actions_num ++;
				animations = realloc(animations, actions_num * sizeof(def_entry_t));
				snprintf(animations[actions_num - 1].value,
					sizeof(animations[actions_num - 1].value),
					"%s/%s", models_path, token);
				if (prevvalue[0])
				{
					Q_strlcpy(animations[actions_num - 1].name, prevvalue,
						sizeof(animations[actions_num - 1].name));
				}
				else
				{
					sprintf(animations[actions_num - 1].name,
						"optional%d", actions_num);
				}
				prevvalue[0] = 0;
			}
			else if (!Q_stricmp(ext, "tga"))
			{
				skinnames_num ++;
				skinnames = realloc(skinnames, skinnames_num * sizeof(def_entry_t));
				snprintf(skinnames[skinnames_num - 1].value,
					sizeof(skinnames[skinnames_num - 1].value),
					"%s/%s", models_path, token);
				if (prevvalue[0])
				{
					Q_strlcpy(skinnames[skinnames_num - 1].name, prevvalue,
						sizeof(skinnames[skinnames_num - 1].name));
				}
				else
				{
					sprintf(skinnames[skinnames_num - 1].name,
						"optional%d", skinnames_num);
				}
				prevvalue[0] = 0;
			}
			else
			{
				Q_strlcpy(prevvalue, token, sizeof(prevvalue));
			}
		}
	}

	if (*base_model)
	{
		base_size = FS_LoadFile(base_model, (void **)&base);
	}

	if (base_size <= 0)
	{
		Com_DPrintf("%s: %s No base model for %s\n",
			__func__, mod_name, base_model);

		if (skinnames)
		{
			free(skinnames);
		}

		if (animations)
		{
			free(animations);
		}

		return NULL;
	}

	/* Convert from LittleLong */
	for (i = 0; i < sizeof(sin_sbm_header_t) / 4; i ++)
	{
		((int *)base)[i] = LittleLong(((int *)base)[i]);
	}

	if ((base->ident != SBMHEADER) || (base->version != MDSINVERSION))
	{
		FS_FreeFile(base);
		free(skinnames);
		free(animations);
		Com_DPrintf("%s: %s, Incorrect ident or version for %s\n",
			__func__, mod_name, base_model);
		return NULL;
	}

	trigroup = (sin_trigroup_t *)((char*)base + sizeof(sin_sbm_header_t));

	for(i = 0; i < base->num_groups; i ++)
	{
		num_tris += LittleLong(trigroup[i].num_tris);
	}

	anim = malloc(actions_num * sizeof(*anim));

	for (i = 0; i < actions_num; i++)
	{
		int anim_size, j;

		anim_size = FS_LoadFile(animations[i].value, (void **)&anim[animation_num]);
		if (anim_size <= 0)
		{
			Com_DPrintf("%s: %s empty animation %s\n",
				__func__, mod_name, animations[i].value);
			continue;
		}

		if (anim[animation_num]->num_xyz != base->num_xyz)
		{
			Com_DPrintf("%s: %s, incorrect count tris in animation in %s\n",
				__func__, mod_name, animations[i].value);
			continue;
		}

		/* Convert from LittleLong */
		for (j = 0; j < sizeof(sin_sam_header_t) / 4; j ++)
		{
			((int *)anim[animation_num])[j] = LittleLong(
				((int *)anim[animation_num])[j]);
		}

		if ((anim[animation_num]->ident != SAMHEADER) ||
			(anim[animation_num]->version != MDSINVERSION))
		{
			FS_FreeFile(base);
			Com_DPrintf("%s: %s, Incorrect ident or version for %s\n",
				__func__, mod_name, base_model);
			return NULL;
		}

		framescount += anim[animation_num]->num_frames;
		animation_num ++;
	}

	if (!animation_num)
	{
		Com_DPrintf("%s: %s no animation found\n",
			__func__, mod_name);
		free(anim);
		free(skinnames);
		free(animations);
		FS_FreeFile(base);
		return NULL;
	}

	dmdxheader.skinwidth = 256;
	dmdxheader.skinheight = 256;

	dmdxheader.framesize = sizeof(daliasxframe_t) - sizeof(dxtrivertx_t);
	dmdxheader.framesize += base->num_xyz * sizeof(dxtrivertx_t);

	dmdxheader.num_meshes = base->num_groups;
	dmdxheader.num_skins = skinnames_num;
	dmdxheader.num_xyz = base->num_xyz;
	dmdxheader.num_st = base->num_st;
	dmdxheader.num_tris = num_tris;
	dmdxheader.num_frames = framescount;
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	dmdxheader.num_glcmds = (10 * dmdxheader.num_tris) + 1 * dmdxheader.num_meshes;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = animation_num;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	for (i = 0; i < pheader->num_skins; i ++)
	{
		Q_strlcpy((char*)pheader + pheader->ofs_skins +  i * MAX_SKINNAME,
			skinnames[i].value, MAX_SKINNAME);
	}

	/* st */
	st_vert_t *st_in = (st_vert_t *)((char *)base + base->ofs_st);
	dstvert_t *st_out = (dstvert_t *)((char *)pheader + pheader->ofs_st);
	for (i = 0; i < pheader->num_st; i ++)
	{
		st_out[i].s = (int)(LittleFloat(st_in[i].s) * pheader->skinwidth + 256) % 256;
		st_out[i].t = (int)(LittleFloat(st_in[i].t) * pheader->skinheight + 256) % 256;
	}

	/* tris and mesh */
	sin_trigroup_t *trigroup_in = (sin_trigroup_t *)((char*)base + sizeof(sin_sbm_header_t));
	dtriangle_t *pouttriofs = (dtriangle_t *) ((char *)pheader + pheader->ofs_tris);
	dmdxmesh_t *mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);

	int tris_ofs = 0;
	for(i = 0; i < base->num_groups; i ++)
	{
		sin_triangle_t *tri_in;
		int j;

		mesh_nodes[i].ofs_tris = tris_ofs;
		mesh_nodes[i].num_tris = LittleLong(trigroup_in[i].num_tris);

		/* offset from current group */
		tri_in = (sin_triangle_t *)((char*)(trigroup_in + i) + LittleLong(trigroup_in[i].ofs_tris));
		for (j = 0; j < mesh_nodes[i].num_tris; j++)
		{
			int k;

			for (k = 0; k < 3; k++)
			{
				pouttriofs[tris_ofs + j].index_st[k] = LittleShort(tri_in[j].index_st[k]);
				pouttriofs[tris_ofs + j].index_xyz[k] = LittleShort(tri_in[j].index_xyz[k]);
			}
		}
		tris_ofs += mesh_nodes[i].num_tris;
	}

	Mod_LoadFrames_SAM(pheader, anim, animations, animation_num, translate, scale);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	for (i = 0; i < animation_num; i++)
	{
		FS_FreeFile(anim[i]);
	}

	free(anim);
	free(skinnames);
	free(animations);

	FS_FreeFile(base);
	return extradata;
}

static void *
Mod_LoadModel_SDEF(const char *mod_name, const void *buffer, int modfilelen)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1);
	memcpy(text, buffer, modfilelen);
	text[modfilelen] = 0;
	extradata = Mod_LoadModel_SDEF_Text(mod_name, text);
	free(text);

	return extradata;
}

typedef struct {
	char *map;
	char *alphafunc;
	char *depthwrite;
	char *uvgen;
	char *blendmode;
	char *depthfunc;
	char *cull;
	char *rgbgen;
	char *uvmod;
} mda_pass_t;

typedef struct {
	mda_pass_t *passes;
	size_t pass_count;
} mda_skin_t;

typedef struct {
	char *name;
	char *evaluate;
	mda_skin_t *skins;
	size_t skin_count;
} mda_profile_t;

typedef struct {
	char *basemodel;
	vec3_t headtri;
	mda_profile_t *profiles;
	size_t profile_count;
} mda_model_t;

static void
Mod_LoadModel_MDA_Parse_SkipComment(char **curr_buff)
{
	size_t linesize;

	/* skip comment */
	linesize = strcspn(*curr_buff, "\n\r");
	*curr_buff += linesize;
}

static void
Mod_LoadModel_MDA_Parse_Pass(const char *mod_name, char **curr_buff, char *curr_end,
	mda_pass_t *pass)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "map"))
		{
			token = COM_Parse(curr_buff);
			pass->map = strdup(token);
			Q_replacebackslash(pass->map);
		}
		else if (!strcmp(token, "uvmod"))
		{
			size_t linesize;
			char *value;

			linesize = strcspn(*curr_buff, "\n\r");
			value = malloc(linesize + 1);
			memcpy(value, *curr_buff, linesize);
			value[linesize] = 0;
			*curr_buff += linesize;

			pass->uvmod = value;
		}
		else if (!strcmp(token, "alphafunc") ||
				 !strcmp(token, "depthwrite") ||
				 !strcmp(token, "uvgen") ||
				 !strcmp(token, "blendmode") ||
				 !strcmp(token, "depthfunc") ||
				 !strcmp(token, "cull") ||
				 !strcmp(token, "rgbgen"))
		{
			char token_section[MAX_TOKEN_CHARS];

			Q_strlcpy(token_section, token, sizeof(token_section));
			token = COM_Parse(curr_buff);

			if (!strcmp(token_section, "alphafunc"))
			{
				pass->alphafunc = strdup(token);
			}
			else if (!strcmp(token_section, "depthwrite"))
			{
				pass->depthwrite = strdup(token);
			}
			else if (!strcmp(token_section, "uvgen"))
			{
				pass->uvgen = strdup(token);
			}
			else if (!strcmp(token_section, "blendmode"))
			{
				pass->blendmode = strdup(token);
			}
			else if (!strcmp(token_section, "depthfunc"))
			{
				pass->depthfunc = strdup(token);
			}
			else if (!strcmp(token_section, "cull"))
			{
				pass->cull = strdup(token);
			}
			else if (!strcmp(token_section, "rgbgen"))
			{
				pass->rgbgen = strdup(token);
			}
		}
	}
}

static void
Mod_LoadModel_MDA_Parse_Skin(const char *mod_name, char **curr_buff, char *curr_end,
	mda_skin_t *skin)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "pass"))
		{
			mda_pass_t *pass;

			skin->pass_count++;
			skin->passes = realloc(skin->passes, skin->pass_count * sizeof(mda_pass_t));
			pass = &skin->passes[skin->pass_count - 1];
			memset(pass, 0, sizeof(*pass));

			token = COM_Parse(curr_buff);
			if (!token || token[0] != '{')
			{
				return;
			}

			Mod_LoadModel_MDA_Parse_Pass(mod_name, curr_buff, curr_end, pass);
		}
	}
}

static void
Mod_LoadModel_MDA_Parse_Profile(const char *mod_name, char **curr_buff, char *curr_end,
	mda_profile_t *profile)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "skin")) {
			mda_skin_t *skin;

			profile->skin_count++;
			profile->skins = realloc(profile->skins, profile->skin_count * sizeof(mda_skin_t));
			skin = &profile->skins[profile->skin_count - 1];
			memset(skin, 0, sizeof(*skin));

			token = COM_Parse(curr_buff);
			if (!token || token[0] != '{')
			{
				return;
			}

			Mod_LoadModel_MDA_Parse_Skin(mod_name, curr_buff, curr_end, skin);
		}
		else if (!strcmp(token, "evaluate"))
		{
			profile->evaluate = strdup(COM_Parse(curr_buff));
		}
	}
}

static void
Mod_LoadModel_MDA_Parse(const char *mod_name, char *curr_buff, char *curr_end,
	mda_model_t *mda)
{
	while (curr_buff)
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			continue;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(&curr_buff);

		}

		/* found basemodel */
		else if (!strcmp(token, "basemodel"))
		{
			token = COM_Parse(&curr_buff);
			if (!token)
			{
				return;
			}
			mda->basemodel = strdup(token);
			Q_replacebackslash(mda->basemodel);
		}
		else if (!strcmp(token, "headtri"))
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				token = COM_Parse(&curr_buff);
				mda->headtri[i] = (float)strtod(token, (char **)NULL);
			}
		}
		else if (!strcmp(token, "profile"))
		{
			mda_profile_t *profile;

			token = COM_Parse(&curr_buff);
			mda->profile_count++;
			mda->profiles = realloc(mda->profiles, mda->profile_count * sizeof(mda_profile_t));
			profile = &mda->profiles[mda->profile_count - 1];
			memset(profile, 0, sizeof(*profile));

			if (!token || token[0] == '{')
			{
				profile->name = strdup("");
			}
			else
			{
				profile->name = strdup(token);
				token = COM_Parse(&curr_buff);
				if (!token || token[0] != '{')
				{
					return;
				}
			}

			Mod_LoadModel_MDA_Parse_Profile(mod_name, &curr_buff, curr_end, profile);
		}
	}
}

static void
Mod_LoadModel_MDA_Free(mda_model_t *mda)
{
	size_t i;

	free(mda->basemodel);
	for (i = 0; i < mda->profile_count; i++) {
		mda_profile_t *profile;
		size_t j;

		profile = &mda->profiles[i];
		free(profile->name);
		free(profile->evaluate);
		for (j = 0; j < profile->skin_count; j++)
		{
			mda_skin_t *skin;
			size_t k;

			skin = &profile->skins[j];
			for (k = 0; k < skin->pass_count; k++)
			{
				mda_pass_t *pass;

				pass = &skin->passes[k];
				free(pass->map);
				free(pass->alphafunc);
				free(pass->depthwrite);
				free(pass->uvgen);
				free(pass->blendmode);
				free(pass->depthfunc);
				free(pass->cull);
				free(pass->rgbgen);
				free(pass->uvmod);
			}
			free(skin->passes);
		}
		free(profile->skins);
	}
	free(mda->profiles);
}

static void *
Mod_LoadModel_MDA_Text(const char *mod_name, char *curr_buff, size_t len)
{
	mda_model_t mda = {0};
	const char *profile_name;

	profile_name = strrchr(mod_name, '!');
	if (profile_name)
	{
		/* skip ! */
		profile_name += 1;
	}
	else
	{
		profile_name = "";
	}

	Mod_LoadModel_MDA_Parse(mod_name, curr_buff, curr_buff + len, &mda);

	if (mda.basemodel)
	{
		void *extradata, *base;
		int base_size;

		base_size = FS_LoadFile(mda.basemodel, (void **)&base);
		if (base_size <= 0)
		{
			Com_DPrintf("%s: %s No base model for %s\n",
				__func__, mod_name, mda.basemodel);
			Mod_LoadModel_MDA_Free(&mda);
			return NULL;
		}

		/* little bit recursive load */
		extradata = Mod_LoadModelFile(mod_name, base, base_size);
		FS_FreeFile(base);

		/* check skin path */
		if (extradata && mda.profile_count &&
			(((dmdx_t *)extradata)->ident == IDALIASHEADER))
		{
			mda_profile_t *profile = NULL;

			if (profile_name)
			{
				size_t i;

				for (i = 0; i < mda.profile_count; i++)
				{
					if (!strcmp(mda.profiles[i].name, profile_name))
					{
						profile = &mda.profiles[i];
					}
				}
			}

			if (!profile)
			{
				/* use first in list */
				profile = &mda.profiles[0];
			}

			if (profile)
			{
				size_t skins_count, i;
				dmdx_t *pheader;

				pheader = (dmdx_t *)extradata;
				skins_count = Q_min(profile->skin_count, pheader->num_skins);

				for (i = 0; i < skins_count; i++)
				{
					if (profile->skins[i].pass_count)
					{
						char *skin;

						/* Update included model with skin path */
						skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;
						if (!strchr(skin, '/') && !strchr(skin, '\\'))
						{
							char skin_path[MAX_QPATH * 2] = {0};
							char *base_skin;

							base_skin = profile->skins[i].passes[0].map;
							Q_strlcpy(skin_path, base_skin, sizeof(skin_path));
							strcpy(strrchr(skin_path, '/') + 1, skin);
							Q_strlcpy(skin, skin_path, MAX_SKINNAME);
						}
					}
				}
			}
		}

		Mod_LoadModel_MDA_Free(&mda);

		return extradata;
	}

	Mod_LoadModel_MDA_Free(&mda);
	return NULL;
}

static void *
Mod_LoadModel_MDA(const char *mod_name, const void *buffer, int modfilelen)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1 - 4);
	memcpy(text, (char *)buffer + 4, modfilelen - 4);
	text[modfilelen - 4] = 0;

	extradata = Mod_LoadModel_MDA_Text(mod_name, text, modfilelen - 4);

	free(text);

	return extradata;
}

/*
=================
Mod_LoadSprite_SP2

support for .sp2 sprites
=================
*/
static void *
Mod_LoadSprite_SP2(const char *mod_name, const void *buffer, int modfilelen)
{
	const dsprite_t *sprin;
	dsprite_t *sprout;
	int i, numframes;
	void *extradata;

	sprin = (dsprite_t *)buffer;
	numframes = LittleLong(sprin->numframes);

	extradata = Hunk_Begin(modfilelen);
	sprout = Hunk_Alloc(modfilelen);

	sprout->ident = LittleLong(sprin->ident);
	sprout->version = LittleLong(sprin->version);
	sprout->numframes = numframes;

	if (sprout->version != SPRITE_VERSION)
	{
		Com_Printf("%s has wrong version number (%i should be %i)\n",
				mod_name, sprout->version, SPRITE_VERSION);
		return NULL;
	}

	/* byte swap everything */
	for (i = 0; i < sprout->numframes; i++)
	{
		sprout->frames[i].width = LittleLong(sprin->frames[i].width);
		sprout->frames[i].height = LittleLong(sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong(sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong(sprin->frames[i].origin_y);
		memcpy(sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
	}

	for (i = 0; i < sprout->numframes; i++)
	{
		Com_DPrintf("%s: %s #%d: Should load external '%s'\n",
			__func__, mod_name, i,
			sprout->frames[i].name);
	}

	return extradata;
}

/*
=================
Mod_LoadSprite_SPR

support for .spr sprites
=================
*/
static void *
Mod_LoadSprite_SPR(const char *mod_name, const void *buffer, int modfilelen)
{
	const dq1sprite_t pinsprite;
	const byte *curr_pos;
	dsprite_t *sprout;
	void *extradata;
	int i, size;

	if (modfilelen < sizeof(pinsprite))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinsprite));
		return NULL;
	}

	for (i = 0; i < sizeof(pinsprite) / sizeof(int); i++)
	{
		((int *)&pinsprite)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinsprite.version != IDQ1SPRITE_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinsprite.version, ALIAS_VERSION);
		return NULL;
	}

	size = sizeof(dsprite_t) + sizeof(dsprframe_t) * (pinsprite.nframes - 1);
	extradata = Hunk_Begin(size);
	sprout = Hunk_Alloc(size);

	sprout->ident = IDSPRITEHEADER;
	sprout->version = SPRITE_VERSION;
	sprout->numframes = pinsprite.nframes;

	curr_pos = (byte*)buffer + sizeof(dq1sprite_t);
	for (i = 0; i < pinsprite.nframes; i++)
	{
		int skin_type;

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
				__func__, mod_name, skin_type);
			return NULL;
		}

		snprintf(sprout->frames[i].name, sizeof(sprout->frames[i].name),
			"%s#%d.lmp", mod_name, i);
		sprout->frames[i].origin_x = LittleLong(((int *)curr_pos)[1]);
		sprout->frames[i].origin_y = LittleLong(((int *)curr_pos)[2]);
		sprout->frames[i].width = LittleLong(((int *)curr_pos)[3]);
		sprout->frames[i].height = LittleLong(((int *)curr_pos)[4]);

		curr_pos += sizeof(int) * 5 + (
			sprout->frames[i].width * sprout->frames[i].height
		);

		if (curr_pos > ((byte *)buffer + modfilelen))
		{
			Com_Printf("%s: model %s is too short\n",
				__func__, mod_name);
			return NULL;
		}
	}

	for (i = 0; i < sprout->numframes; i++)
	{
		Com_DPrintf("%s: %s #%d: Should load internal '%s'\n",
			__func__, mod_name, i,
			sprout->frames[i].name);
	}

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

		case IDQ1SPRITEHEADER:
			extradata = Mod_LoadSprite_SPR(mod_name, buffer, modfilelen);
			break;
	}

	return extradata;
}

static byte *
Mod_LoadSPRImage(const char *mod_name, int texture_index, byte *buffer, int modfilelen,
	int *width, int *height)
{
	const dq1sprite_t pinsprite;
	const byte *curr_pos;
	int i, size;
	byte *pic;

	if (modfilelen < sizeof(pinsprite))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinsprite));
		return NULL;
	}

	for (i = 0; i < sizeof(pinsprite) / sizeof(int); i++)
	{
		((int *)&pinsprite)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinsprite.version != IDQ1SPRITE_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinsprite.version, ALIAS_VERSION);
		return NULL;
	}

	if (pinsprite.nframes <= texture_index)
	{
		Com_Printf("%s: Sprite %s has %d only textures\n",
			__func__, mod_name, pinsprite.nframes);
		return NULL;
	}

	curr_pos = (byte*)buffer + sizeof(dq1sprite_t);
	for (i = 0; i < pinsprite.nframes; i++)
	{
		int skin_type;

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
				__func__, mod_name, skin_type);
			return NULL;
		}

		*width = LittleLong(((int *)curr_pos)[3]);
		*height = LittleLong(((int *)curr_pos)[4]);
		size = (*width) * (*height);

		if (i == texture_index)
		{
			curr_pos += sizeof(int) * 5;

			pic = malloc(size);
			memcpy(pic, curr_pos, size);

			Com_DPrintf("%s Loaded embeded %s#%d image %dx%d\n",
				__func__, mod_name, texture_index, *width, *height);

			return pic;
		}

		curr_pos += sizeof(int) * 5 + size;

		if (curr_pos > ((byte *)buffer + modfilelen))
		{
			Com_Printf("%s: sprite %s is too short\n",
				__func__, mod_name);
			return NULL;
		}
	}
	return NULL;
}

static byte *
Mod_LoadBSPImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	int miptex_offset, miptex_size, texture_offset,
		image_offset, size, ident;
	byte *pic;
	dq1mipheader_t *miptextures;
	dq1miptex_t *texture;

	/* get BSP format ident */
	ident = LittleLong(*(unsigned *)raw);

	miptex_offset = LittleLong(
		((int *)raw)[LUMP_BSP29_MIPTEX * 2 + 1]); /* text info lump pos */
	miptex_size = LittleLong(
		((int *)raw)[LUMP_BSP29_MIPTEX * 2 + 2]); /* text info lump size */

	if (miptex_offset >= len)
	{
		Com_Printf("%s: Map %s has broken miptex lump\n",
			__func__, mod_name);
		return NULL;
	}

	miptextures = (dq1mipheader_t *)(raw + miptex_offset);

	if (miptextures->numtex < texture_index)
	{
		Com_Printf("%s: Map %s has %d only textures\n",
			__func__, mod_name, miptextures->numtex);
		return NULL;
	}

	texture_offset = LittleLong(miptextures->offset[texture_index]);
	if (texture_offset < 0)
	{
		Com_Printf("%s: Map %s image is not attached\n",
			__func__, mod_name);
		return NULL;
	}

	if (texture_offset > miptex_size)
	{
		Com_Printf("%s: Map %s has wrong texture position\n",
			__func__, mod_name);
		return NULL;
	}

	texture = (dq1miptex_t *)(raw + miptex_offset + texture_offset);
	*width = LittleLong(texture->width);
	*height = LittleLong(texture->height);
	image_offset = LittleLong(texture->offset1);
	size = (*width) * (*height);

	if (image_offset <= 0)
	{
		Com_Printf("%s: Map %s is external image\n",
			__func__, mod_name);
		return NULL;
	}

	if ((size < 0) ||
		(image_offset > miptex_size) ||
		((image_offset + size) > miptex_size))
	{
		Com_Printf("%s: Map %s has wrong texture image position\n",
			__func__, mod_name);
		return NULL;
	}

	if (ident == BSPHL1VERSION)
	{
		byte *src, *dst, *palette;
		size_t i, palette_offset;

		/*
		 * Half-Life 1 stored custom palette information for each mip texture,
		 * so we seek past the last mip texture (and past the 256 2-byte denominator)
		 * to grab the RGB values for this texture
		 */
		palette_offset = LittleLong(texture->offset8);
		palette_offset += (*width >> 3) * (*height >> 3) + 2;

		if ((palette_offset > miptex_size) ||
			((palette_offset + 768) > miptex_size))
		{
			Com_Printf("%s: Map %s has wrong palette image position\n",
				__func__, mod_name);
			return NULL;
		}

		palette = (raw + miptex_offset + texture_offset + palette_offset);

		*bitsPerPixel = 32;
		dst = pic = malloc(size * 4);

		src = (raw + miptex_offset + texture_offset + image_offset);
		for (i = 0; i < size; i++)
		{
			byte value;

			value = src[i];
			dst[0] = palette[value * 3 + 0];
			dst[1] = palette[value * 3 + 1];
			dst[2] = palette[value * 3 + 2];
			dst[3] = value == 255 ? 0 : 255;
			dst += 4;
		}
	}
	else
	{
		*bitsPerPixel = 8;
		pic = malloc(size);
		memcpy(pic, (raw + miptex_offset + texture_offset + image_offset), size);
	}

	Com_DPrintf("%s Loaded embeded %s image %dx%d\n",
		__func__, texture->name, *width, *height);

	return pic;
}

static byte *
Mod_LoadMDLImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	byte *images, *pic;
	dmdx_t *pheader;
	size_t size;

	if (len < sizeof(dmdx_t))
	{
		Com_DPrintf("%s: Short file %s in cache\n",
			__func__, mod_name);
		return NULL;
	}

	pheader = (dmdx_t *)raw;
	if (pheader->ident != IDALIASHEADER)
	{
		Com_DPrintf("%s: Incorrect file %s in cache\n",
			__func__, mod_name);
		return NULL;
	}

	if (pheader->num_skins <= texture_index)
	{
		Com_Printf("%s: Map %s has %d only textures\n",
			__func__, mod_name, pheader->num_skins);
		return NULL;
	}

	size = (pheader->skinheight * pheader->skinwidth * pheader->num_imgbit / 8);

	images = (byte *)pheader + pheader->ofs_imgbit;
	images += (texture_index * size);

	pic = malloc(size);
	memcpy(pic, images, size);

	*width = pheader->skinwidth;
	*height = pheader->skinheight;
	*bitsPerPixel = pheader->num_imgbit;

	Com_DPrintf("%s Loaded embeded %s image %dx%d\n",
		__func__, mod_name, *width, *height);

	return pic;
}

byte *
Mod_LoadEmbdedImage(const char *mod_name, int texture_index, byte *raw, int len,
	int *width, int *height, int *bitsPerPixel)
{
	if (len < sizeof(unsigned))
	{
		return NULL;
	}

	switch (LittleLong(*(unsigned *)raw))
	{
		case IDALIASHEADER:
			return Mod_LoadMDLImage(mod_name, texture_index, raw, len,
				width, height, bitsPerPixel);
		case BSPQ1VERSION:
			/* fall through */
		case BSPHL1VERSION:
			return Mod_LoadBSPImage(mod_name, texture_index, raw, len,
				width, height, bitsPerPixel);
		case IDQ1SPRITEHEADER:
			*bitsPerPixel = 8;
			return Mod_LoadSPRImage(mod_name, texture_index, raw, len,
				width, height);
		default:
			return NULL;
	}
}
