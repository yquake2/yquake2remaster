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
#include "mesh.h"

#include <stddef.h>

/* quaternion helpers for rotating vectors */
static void
QuatMul(const vec4_t a, const vec4_t b, vec4_t out)
{
	/* out = a * b */
	vec3_t av = { a[0], a[1], a[2] };
	vec3_t bv = { b[0], b[1], b[2] };
	float aw = a[3];
	float bw = b[3];
	vec3_t crossv;

	CrossProduct(av, bv, crossv);

	out[3] = aw * bw - (av[0]*bv[0] + av[1]*bv[1] + av[2]*bv[2]);
	out[0] = aw * bv[0] + bw * av[0] + crossv[0];
	out[1] = aw * bv[1] + bw * av[1] + crossv[1];
	out[2] = aw * bv[2] + bw * av[2] + crossv[2];
}

static void
QuatRotateConj(const vec4_t q, const vec3_t v, vec3_t out)
{
	/* rotate vector v by inverse of quaternion q (i.e. q_conj * v * q) */
	vec4_t vq = { v[0], v[1], v[2], 0.0f };
	vec4_t qconj = { -q[0], -q[1], -q[2], q[3] };
	vec4_t tmp, res;

	QuatMul(qconj, vq, tmp);
	QuatMul(tmp, (vec4_t){ q[0], q[1], q[2], q[3] }, res);

	out[0] = res[0];
	out[1] = res[1];
	out[2] = res[2];
}

static void
QuatRotate(const vec4_t q, const vec3_t v, vec3_t out)
{
	/* rotate vector v by quaternion q: out = q * v * q_conj */
	vec4_t vq = { v[0], v[1], v[2], 0.0f };
	vec4_t qconj = { -q[0], -q[1], -q[2], q[3] };
	vec4_t tmp, res;

	QuatMul(q, vq, tmp);
	QuatMul(tmp, qconj, res);

	out[0] = res[0];
	out[1] = res[1];
	out[2] = res[2];
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
		const daliasframe_t *pinframe;
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
	vec3_t translate, int resolution, vec3_t scale)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (!scale[i])
		{
			scale[i] = 1.0;
		}
	}

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
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]) / scale[j];
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]) / scale[j];
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

			if (index_xyz < 0 || index_xyz >= pheader->num_xyz)
			{
				free(mesh_ids);
				Com_DPrintf("%s: %s invalid mesh id %d", __func__, mod_name, index_xyz);
				return;
			}

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
				if (index[j] < 0 || index[j] >= pheader->num_xyz)
				{
					free(mesh_ids);
					Com_DPrintf("%s: %s invalid mesh id %d", __func__, mod_name, index[j]);
					return;
				}
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
		for (j = 0; j < pheader->num_xyz; j++)
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

		for (t = 0; t < pheader->num_tris; t ++)
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
		for (t = 0; t < pheader->num_xyz; t++)
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

static const namesconvert_t flex_names[] = {
	/* replace frame group started with atack* to attack */
	{"atack",  "attack"},
	/* replace frame group started with elf:attck* to attack */
	{"attck", "attack"},
	/* replace frame group started with elf:runatk* to attack */
	{"runatk", "attack"},
	/* replace frame group started with death* to death */
	{"death", "death"},
	/* replace frame group started with elf:walk* to walk */
	{"walk", "walk"},
	/* replace frame group started with run* to run */
	{"run", "run"},
	/* replace frame group started with breath to idle */
	{"breath", "idle"},
	/* replace frame group started with shoot to attack */
	{"shoot", "attack"},
	/* replace frame group started with rolla to crwalk */
	{"rolla", "crwalk"},
	/* replace frame group started with 4swim to swim */
	{"4swim", "swim"},
	{NULL, NULL}
};

static const namesconvert_t quake2_names[] = {
	/* infinity */
	{"powa", "pow"},
	{"powb", "pow"},
	{NULL, NULL}
};

static const namesconvert_t dkm_names[] = {
	{"atak", "attack"},
	{"die", "death"},
	{"fly", "fly"},
	{"hover", "hover"},
	{"run", "run"},
	{"stand", "stand"},
	{"swim", "swim"},
	{"walk", "walk"},
	{"amba", "idle"},
	/* hack for protopod, looks as can't move, and should be attack */
	{"hatcha", "run"},
	{NULL, NULL}
};

static const namesconvert_t anox_names[] = {
	{"amb", "idle"}, /* ambient */
	{"atak", "attack"},
	{"die", "death"},
	{"run", "run"},
	{"walk", "walk"},
	{NULL, NULL}
};

static const namesconvert_t kingpin_names[] = {
	{"walk", "walk"},
	{"crch_astand", "crstnd"},
	{"crch_death", "crdeath"},
	{"crch_dth", "crdeath"},
	{"cr_death", "crdeath"},
	{"crch_walk", "crwalk"},
	{"crch_walk", "crwalk"},
	{"crouch_death", "crdeath"},
	{"crouch_pain", "crpain"},
	{"crouch_walk", "crwalk"},
	{"cr_pain", "crpain"},
	{"death", "death"},
	{"idle", "idle"},
	{"jump", "jump"},
	{"melee", "melee"},
	{"nw_pain", "pain"},
	{"pain", "pain"},
	{"p_pain", "pain"},
	{"run", "run"},
	{"stand_crouch", "crstnd"},
	{"stand", "stand"},
	{"walk", "walk"},
	{NULL, NULL}
};

void
Mod_LoadModel_AnimGroupNamesFix(dmdx_t *pheader, const namesconvert_t *names)
{
	dmdxframegroup_t *pframegroup;
	size_t i;

	pframegroup = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);
	for (i = 0; i < pheader->num_animgroup; i++)
	{
		const namesconvert_t *curr;

		curr = names;
		do
		{
			size_t len;

			len = strlen(curr->prefix);
			if (!memcmp(pframegroup[i].name, curr->prefix, len))
			{
				Q_strlcpy(pframegroup[i].name, curr->name,
					sizeof(pframegroup[i].name));
				break;
			}

			curr++;
		}
		while (curr->prefix);
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

	Mod_LoadAnimGroupList(pheader, true);
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
		pinmodel.framesize, translate, pinmodel.resolution, pinmodel.lod_scale);
	/* Anachronox has gaps in frame sequence numbers, skip check and expect
	 * prefix before number as separate group */
	Mod_LoadAnimGroupList(pheader, false);
	Mod_LoadModel_AnimGroupNamesFix(pheader, anox_names);
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
	Mod_LoadAnimGroupList(pheader, true);
	Mod_LoadModel_AnimGroupNamesFix(pheader, quake2_names);

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

static const char *
Mod_LoadModel_FlexSection(const void *in_buffer, size_t modfilelen, const char *section_name,
	int *section_version, size_t *section_size)
{
	const char *src = (const char *)in_buffer;

	while (modfilelen > 0)
	{
		char blockname[32];
		int version, size;

		if (modfilelen < (int)(sizeof(blockname) + sizeof(version) + sizeof(size)))
		{
			Com_Printf("%s: Invalid mod file len\n", __func__);
			return NULL;
		}

		memcpy(blockname, src, sizeof(blockname));

		src += sizeof(blockname);
		version = *(int*)src;
		src += sizeof(version);
		size = *(int*)src;
		src += sizeof(size);
		modfilelen = modfilelen - sizeof(blockname) - sizeof(version) - sizeof(size);

		if (size < 0 || size > modfilelen)
		{
			Com_Printf("%s: Invalid chunk size\n", __func__);
			return NULL;
		}

		if (Q_strncasecmp(blockname, section_name, sizeof(blockname)) == 0)
		{
			*section_size = size;
			*section_version = version;
			return src;
		}

		modfilelen -= size;
		src += size;
	}

	*section_size = 0;
	return NULL;
}

static qboolean
Mod_LoadModel_FlexSkins(dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "skin", &version, &size);
	if (src && size)
	{
		if (version != 1)
		{
			Com_Printf("%s: Invalid skin version %d\n",
				__func__, version);
			return false;
		}

		if (size != (pheader->num_skins * MAX_SKINNAME))
		{
			Com_Printf("%s: Invalid skin size\n",
				__func__);
			return false;
		}
		memcpy((char*) pheader + pheader->ofs_skins, src, size);
	}

	return true;
}

static qboolean
Mod_LoadModel_FlexSTCoord(dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "st coord", &version, &size);
	if (src && size)
	{
		if (version != 1)
		{
			Com_Printf("%s: Invalid st coord version %d\n",
				__func__, version);
			return false;
		}

		if (size != (pheader->num_st * sizeof(dstvert_t)))
		{
			Com_Printf("%s: Invalid st coord size\n",
				__func__);
			return false;
		}

		Mod_LoadSTvertList(pheader, (dstvert_t *)src);
	}

	return true;
}

static qboolean
Mod_LoadModel_FlexTris(dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "tris", &version, &size);
	if (src && size)
	{
		if (version != 1)
		{
			Com_Printf("%s: Invalid tris version %d\n",
				__func__, version);
			return false;
		}

		if (size != (pheader->num_tris * sizeof(dtriangle_t)))
		{
			Com_Printf("%s: Invalid tris size\n",
				__func__);
			return false;
		}

		Mod_LoadMD2TriangleList(pheader, (dtriangle_t *) src);
	}

	return true;
}

static qboolean
Mod_LoadModel_FlexFrames(size_t inframesize, dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "frames", &version, &size);
	if (src && size)
	{
		vec3_t translate = {0, 0, 0};

		if (version != 1)
		{
			Com_Printf("%s: Invalid frames version %d\n",
				__func__, version);
			return false;
		}

		if (size < (pheader->num_frames *
			(sizeof(daliasframe_t) + (pheader->num_xyz - 1) * sizeof(dtrivertx_t))))
		{
			Com_Printf("%s: Invalid frames size\n",
				__func__);
			return false;
		}

		Mod_LoadFrames_MD2(pheader, (byte *)src, inframesize, translate);
		Mod_LoadAnimGroupList(pheader, true);
		Mod_LoadModel_AnimGroupNamesFix(pheader, flex_names);
	}

	return true;
}

static qboolean
Mod_LoadModel_FlexGLCmd(const char * mod_name, dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "glcmds", &version, &size);
	if (src && size)
	{
		if (version != 1)
		{
			Com_Printf("%s: Invalid glcmds version %d\n",
				__func__, version);
			return false;
		}

		if (size != (pheader->num_glcmds * sizeof(int)))
		{
			Com_Printf("%s: Invalid glcmds size\n",
				__func__);
			return false;
		}

		Mod_LoadCmdList(mod_name, pheader, (int *)src);
	}

	return true;
}

static qboolean
Mod_LoadModel_FlexMeshNodes(dmdx_t *pheader, const void *buffer, int modfilelen)
{
	const char *src = NULL;
	size_t size;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "mesh nodes", &version, &size);
	if (src && size)
	{
		int num_mesh_nodes;

		num_mesh_nodes = (pheader->ofs_skins - sizeof(*pheader)) / sizeof(dmdxmesh_t);

		if (version != 3)
		{
			Com_Printf("%s: Invalid mesh nodes version %d\n",
				__func__, version);
			return false;
		}

		/* 516 mesh node size */
		if (size != (num_mesh_nodes * 516))
		{
			Com_Printf("%s: Invalid mesh nodes size\n",
				__func__);
			return false;
		}

		if (num_mesh_nodes > 0)
		{
			dmdxmesh_t *mesh_nodes;
			const char *in_mesh = src;
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

	return true;
}

/*
=============
Mod_LoadModel_Flex
=============
*/
static void *
Mod_LoadModel_Flex(const char *mod_name, const void *buffer, int modfilelen)
{
	dmdx_t dmdxheader, *pheader = NULL;
	size_t size, inframesize = 0;
	void *extradata = NULL;
	const char *src = NULL;
	int version;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "header", &version, &size);
	if (!src || !size)
	{
		Com_Printf("%s: No header found\n", __func__);
		return NULL;
	}
	else
	{
		const fmheader_t *header = (fmheader_t *)src;
		int framesize;

		if (sizeof(fmheader_t) > size)
		{
			Com_Printf("%s: Too short header\n", __func__);
			return NULL;
		}

		if (version != 2)
		{
			Com_Printf("%s: Invalid header version %d\n",
				__func__, version);
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
	}

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "skeleton", &version, &size);
	if (src && size)
	{
		int skeleton_type, skeleton_joints_num, *skeleton_data;

		if (version != 1)
		{
			Com_Printf("%s: Invalid skeleton version %d\n",
				__func__, version);
			return NULL;
		}

		if (size < (int)(2 * sizeof(int))) {
			Com_Printf("%s: Invalid skeleton size\n",
				__func__);
			return NULL;
		}

		skeleton_data = (int *)src;
		skeleton_type = LittleLong(*skeleton_data);
		skeleton_data++;
		skeleton_joints_num = LittleLong(*skeleton_data);
		Com_DPrintf("%s: %s in skeleton has 0x%02x type and %d bones\n",
			__func__, mod_name, skeleton_type, skeleton_joints_num);
		dmdxheader.num_joints = skeleton_joints_num;

		typedef struct M_SkeletalCluster_s
		{
			int numVerticies;
			int *verticies;
		} M_SkeletalCluster_t;

		typedef struct M_SkeletalJoint_s
		{
			vec3_t origin;
			vec3_t direction;
			vec3_t up;
		} M_SkeletalJoint_t;

		typedef struct ModelSkeleton_s
		{
			M_SkeletalJoint_t rootJoint[8];
		} ModelSkeleton_t;

		int		i, j, k;
		int		*basei;
		int		runningTotalVertices = 0;
		int		indexBase = 0;
		float	*basef;
		M_SkeletalCluster_t *m_skeletalClusters;
		ModelSkeleton_t* m_skeletons;

		basei = (int *)src;

		skeleton_type = *basei;

		skeleton_joints_num = *(++basei);

		m_skeletalClusters = malloc(sizeof(M_SkeletalCluster_t) * skeleton_joints_num);

		for (i = skeleton_joints_num - 1; i >= 0; --i)
		{
			runningTotalVertices += *(++basei);
			m_skeletalClusters[i].numVerticies = runningTotalVertices;
			m_skeletalClusters[i].verticies = (int*)malloc(m_skeletalClusters[i].numVerticies * sizeof(int));
		}

		for (j = skeleton_joints_num - 1; j >= 0; --j)
		{
			for (i = indexBase; i < m_skeletalClusters[j].numVerticies; ++i)
			{
				++basei;

				for (k = 0; k <= j; ++ k)
				{
					m_skeletalClusters[k].verticies[i] = *basei;
				}
			}

			indexBase = m_skeletalClusters[j].numVerticies;
		}

		if (*(++basei))
		{
			basef = (float *)++basei;

			m_skeletons = (ModelSkeleton_t*) malloc(dmdxheader.num_frames * sizeof(ModelSkeleton_t));

			for (i = 0; i < dmdxheader.num_frames; ++i)
			{
				for (j = 0; j < skeleton_joints_num; ++j)
				{
					m_skeletons[i].rootJoint[j].origin[0] = *(basef++);
					m_skeletons[i].rootJoint[j].origin[1] = *(basef++);
					m_skeletons[i].rootJoint[j].origin[2] = *(basef++);

					m_skeletons[i].rootJoint[j].direction[0] = *(basef++);
					m_skeletons[i].rootJoint[j].direction[1] = *(basef++);
					m_skeletons[i].rootJoint[j].direction[2] = *(basef++);

					m_skeletons[i].rootJoint[j].up[0] = *(basef++);
					m_skeletons[i].rootJoint[j].up[1] = *(basef++);
					m_skeletons[i].rootJoint[j].up[2] = *(basef++);
				}
			}

			printf("used %d vs %d\n", (char*)basef - src, size);
		}
	}

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);
	if (!pheader)
	{
		Com_Printf("%s: %s has broken header.\n",
			__func__, mod_name);
		return NULL;
	}

	if (!Mod_LoadModel_FlexSkins(pheader, buffer, modfilelen) ||
		!Mod_LoadModel_FlexSTCoord(pheader, buffer, modfilelen) ||
		!Mod_LoadModel_FlexTris(pheader, buffer, modfilelen) ||
		!Mod_LoadModel_FlexFrames(inframesize, pheader, buffer, modfilelen) ||
		!Mod_LoadModel_FlexGLCmd(mod_name, pheader, buffer, modfilelen) ||
		!Mod_LoadModel_FlexMeshNodes(pheader, buffer, modfilelen))
	{
		return NULL;
	}

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "skeleton", &version, &size);
	if (src && size)
	{
		/* TODO: reload skeleton */
#if 0
	/* If we saw a skeleton block, convert and store boneposes in dmdx format
	   (matches how MD5 loader fills pheader->ofs_baseframe_joints). */
	if (skeleton_block && pheader && pheader->num_joints > 0 && pheader->num_frames > 0 && pheader->ofs_baseframe_joints)
	{
		const byte *p = skeleton_block;
		const byte *end = skeleton_block + skeleton_block_size;
		int framesWritten = 0;
		int f, b, k;
		int joint_count = 0;

		if (p + 2 * sizeof(int) <= end)
		{
			int tmp;
			int skeleton_type;

			/* first int = skeleton_type, second = joint count */
			memcpy(&tmp, p, sizeof(tmp));
			skeleton_type = LittleLong(tmp);
			p += sizeof(int);

			memcpy(&tmp, p, sizeof(tmp));
			joint_count = LittleLong(tmp);
			if (joint_count != pheader->num_joints || joint_count < 0)
			{
				printf("%s: Invalid %s joint count %d (header: num_bones=%d, skeleton_type=%d)\n",
					__func__, "skeleton", joint_count, pheader->num_joints, skeleton_type);
				goto flex_skel_done;
			}
			p += sizeof(int);

			/* First pass: count weights per-vertex */
			{
				const byte *cluster_start = p;
				int *vert_counts = NULL;
				int total_weights_check = 0;
				int vi;

				vert_counts = (int *)calloc(pheader->num_xyz, sizeof(int));
				YQ2_COM_CHECK_OOM(vert_counts, "calloc()", pheader->num_xyz * sizeof(int));
				if (!vert_counts)
					goto flex_skel_done;

				printf("%s: Flex skeleton has %d bones, %d vertices\n",
					__func__, joint_count, pheader->num_xyz);
				for (b = 0; b < joint_count; b++)
				{
					int cluster_verts;

					if (p + sizeof(int) > end)
					{
						free(vert_counts);
						goto flex_skel_done;
					}

					memcpy(&tmp, p, sizeof(tmp));
					cluster_verts = LittleLong(tmp);
					p += sizeof(int);

					if (cluster_verts < 0 || p + ((size_t)cluster_verts * sizeof(int)) > end)
					{
						free(vert_counts);
						goto flex_skel_done;
					}

					for (vi = 0; vi < cluster_verts; vi++)
					{
						int idx;
						memcpy(&tmp, p, sizeof(tmp));
						idx = LittleLong(tmp);
						p += sizeof(int);

						if (idx < 0 || idx >= pheader->num_xyz)
						{
							free(vert_counts);
							goto flex_skel_done;
						}

						vert_counts[idx]++;
						total_weights_check++;
					}
				}

				/* Verify total weights matches header (if available) */
				if (pheader->num_weights != total_weights_check)
				{
					printf("%s: Flex total weights %d != header.num_weights %d\n",
						__func__, total_weights_check, pheader->num_weights);
					/* proceed using counted total */
				}
				printf("%s: Flex skeleton has %d bones, %d total weights\n",
					__func__, joint_count, total_weights_check);

				/* build bindings start offsets, but only if counts match header to avoid overflow */
				{
					dmdx_vertex_t *bindings = (dmdx_vertex_t *)((byte *)pheader + pheader->ofs_mesh_verteces);

					if (total_weights_check <= 0 || (pheader->num_weights != 0 && pheader->num_weights != total_weights_check))
					{
						/* mismatch or zero: do not write weights to avoid corrupting hunk
						   print raw cluster header for debugging */
						Com_Printf("%s: skeleton influence count mismatch or zero: counted=%d header=%d - skipping influence write\n",
							__func__, total_weights_check, pheader->num_weights);

						/* zero bindings */
						for (vi = 0; vi < pheader->num_xyz; vi++)
						{
							bindings[vi].start = 0;
							bindings[vi].count = 0;
						}

						/* dump first 32 ints of cluster_start for inspection */
						{
							int dump_n = 32;
							int di;
							const int *ints = (const int *)cluster_start;
							int available = ((end - cluster_start) / sizeof(int));
							if (dump_n > available) dump_n = available;
							Com_Printf("%s: skeleton raw ints (first %d):", __func__, dump_n);
							for (di = 0; di < dump_n; di++)
							{
								Com_Printf(" %d", LittleLong(ints[di]));
							}
							Com_Printf("\n");
						}
					}
					else
					{
						int running = 0;
						for (vi = 0; vi < pheader->num_xyz; vi++)
						{
							bindings[vi].start = running;
							bindings[vi].count = vert_counts[vi];
							running += vert_counts[vi];
						}

						/* second pass: fill weights */
						{
							dmdx_weight_t *weights = (dmdx_weight_t *)((byte *)pheader + pheader->ofs_weights);
							int *written = (int *)calloc(pheader->num_xyz, sizeof(int));
							const byte *pp = cluster_start;

							if (!written)
							{
								free(vert_counts);
								goto flex_skel_done;
							}

							for (b = 0; b < joint_count; b++)
							{
								int cluster_verts;

								memcpy(&tmp, pp, sizeof(tmp));
								cluster_verts = LittleLong(tmp);
								pp += sizeof(int);

								for (vi = 0; vi < cluster_verts; vi++)
								{
									int idx, dst;
									memcpy(&tmp, pp, sizeof(tmp));
									idx = LittleLong(tmp);
									pp += sizeof(int);

									dst = bindings[idx].start + written[idx];
									written[idx]++;

									weights[dst].joint = b;
									weights[dst].bias = 1.0f;
									weights[dst].pos[0] = 0.0f;
									weights[dst].pos[1] = 0.0f;
									weights[dst].pos[2] = 0.0f;
								}
							}

							free(written);
						}
					}
				}

				free(vert_counts);

				/* p already advanced past clusters in the first pass; continue */
			}
		}

		if (p + sizeof(int) <= end)
		{
			framesWritten = LittleLong(*(const int *)p);
			p += sizeof(int);
		}

		if (framesWritten)
		{
			/* expected size: pheader->num_frames * joint_count * (origin+dir+up) * sizeof(float)
			   where each of origin/dir/up is 3 floats => 9 floats per joint per frame */
			const ptrdiff_t needed = (ptrdiff_t)pheader->num_frames * joint_count * 9 * sizeof(float);
			ptrdiff_t remaining = end - p;
			if (remaining < needed)
			{
				Com_Printf("%s: skeleton block too small: block_size=%d, offset=%td, remaining=%td, framesWritten=%d, joint_count=%d, pheader->num_frames=%d, pheader->num_joints=%d, expected_bytes=%td\n",
					__func__, skeleton_block_size, (p - skeleton_block), remaining, framesWritten, joint_count, pheader->num_frames, pheader->num_joints, needed);
				goto flex_skel_done;
			}

			printf("%s: skeleton block has %d frames, %d bones, %td bytes of data\n",
				__func__, framesWritten, joint_count, needed);
			dmdx_baseframe_joint_t *boneposes = (dmdx_baseframe_joint_t *)((byte *)pheader + pheader->ofs_baseframe_joints);

			for (f = 0; f < pheader->num_frames; f++)
			{
				daliasxframe_t *frame = (daliasxframe_t *)((byte *)pheader + pheader->ofs_frames + f * pheader->framesize);

				for (b = 0; b < pheader->num_joints; b++)
				{
					vec3_t pos, dir, up;
					float tmpf;

					/* read origin (3 floats) */
					for (k = 0; k < 3; k++)
					{
						if (p + sizeof(float) > end) goto flex_skel_done;
						memcpy(&tmpf, p, sizeof(float)); p += sizeof(float);
						pos[k] = tmpf * frame->scale[k] + frame->translate[k];
					}

					/* populate bones bind pose from first frame (if bones area exists) */
					if (pheader->num_joints > 0 && pheader->ofs_joints && pheader->ofs_baseframe_joints)
					{
						dmdx_joint_t *bones = (dmdx_joint_t *)((byte *)pheader + pheader->ofs_joints);
						dmdx_baseframe_joint_t *boneposes = (dmdx_baseframe_joint_t *)((byte *)pheader + pheader->ofs_baseframe_joints);
						int bi;

						for (bi = 0; bi < pheader->num_joints; bi++)
						{
							bones[bi].parent = -1;
							VectorCopy(boneposes[bi].pos, bones[bi].pos);
							bones[bi].orient[0] = boneposes[bi].orient[0];
							bones[bi].orient[1] = boneposes[bi].orient[1];
							bones[bi].orient[2] = boneposes[bi].orient[2];
							bones[bi].orient[3] = boneposes[bi].orient[3];
						}
					}

					/* compute accurate influence offsets from bind-frame vertex positions */
					if (pheader->num_weights > 0 && pheader->ofs_weights && pheader->ofs_mesh_verteces)
					{
						dmdx_vertex_t *bindings = (dmdx_vertex_t *)((byte *)pheader + pheader->ofs_mesh_verteces);
						dmdx_joint_t *bones = (dmdx_joint_t *)((byte *)pheader + pheader->ofs_joints);
						dmdx_weight_t *weights = (dmdx_weight_t *)((byte *)pheader + pheader->ofs_weights);
						daliasxframe_t *frame0 = (daliasxframe_t *)((byte *)pheader + pheader->ofs_frames);
						int vi, iw;

						for (vi = 0; vi < pheader->num_xyz; vi++)
						{
							vec3_t vworld;

							for (iw = 0; iw < 3; iw++)
							{
								vworld[iw] = frame0->verts[vi].v[iw] * frame0->scale[iw] + frame0->translate[iw];
							}

							for (iw = bindings[vi].start; iw < bindings[vi].start + bindings[vi].count; iw++)
							{
								int joint = weights[iw].joint;
								vec3_t local;
								vec3_t diff;

								VectorSubtract(vworld, bones[joint].pos, diff);
								QuatRotateConj(bones[joint].orient, diff, local);

								weights[iw].pos[0] = local[0];
								weights[iw].pos[1] = local[1];
								weights[iw].pos[2] = local[2];
							}
						}

						/* DEBUG: dump expected vs actual for first few vertices */
						{
							int dump_n = Q_min(16, pheader->num_xyz);
							int vj;
							printf("%s: Dumping first %d vertices bind/influence info for model %s\n",
								__func__, dump_n, mod_name);

							for (vj = 0; vj < dump_n; vj++)
							{
								vec3_t vworld;
								int a;

								for (a = 0; a < 3; a++)
									vworld[a] = frame0->verts[vj].v[a] * frame0->scale[a] + frame0->translate[a];

								Com_Printf("  vert %d: world=(%.3f %.3f %.3f) bind_start=%d bind_count=%d\n",
									vj, vworld[0], vworld[1], vworld[2], bindings[vj].start, bindings[vj].count);

								for (a = bindings[vj].start; a < bindings[vj].start + bindings[vj].count; a++)
								{
									int bone = weights[a].joint;
									vec3_t recon, offs_world;

									/* reconstruct world pos from bone bind + rotated offset */
									QuatRotate(bones[bone].orient, weights[a].pos, offs_world);
									VectorAdd(bones[bone].pos, offs_world, recon);

									printf("    inf %d: bone=%d weight=%.3f offset=(%.3f %.3f %.3f) recon=(%.3f %.3f %.3f) bone_bind=(%.3f %.3f %.3f)\n",
										a - bindings[vj].start, bone, weights[a].bias,
										weights[a].pos[0], weights[a].pos[1], weights[a].pos[2],
										recon[0], recon[1], recon[2], bones[bone].pos[0], bones[bone].pos[1], bones[bone].pos[2]);
								}
							}
						}
					}

					/* read direction (3 floats) */
					for (k = 0; k < 3; k++)
					{
						if (p + sizeof(float) > end) goto flex_skel_done;
						memcpy(&tmpf, p, sizeof(float)); p += sizeof(float);
						dir[k] = tmpf * frame->scale[k] + frame->translate[k];
					}

					/* read up (3 floats) */
					for (k = 0; k < 3; k++)
					{
						if (p + sizeof(float) > end) goto flex_skel_done;
						memcpy(&tmpf, p, sizeof(float)); p += sizeof(float);
						up[k] = tmpf * frame->scale[k] + frame->translate[k];
					}

					/* build orthonormal basis from dir and up */
					vec3_t dn, un, right, tmpv;
					VectorCopy(dir, dn); VectorNormalize(dn);
					VectorCopy(up, un); VectorNormalize(un);
					CrossProduct(dn, un, right);
					if (VectorNormalize(right) == 0.0f)
					{
						right[0] = 1.0f; right[1] = 0.0f; right[2] = 0.0f;
					}
					CrossProduct(right, dn, tmpv);
					VectorCopy(tmpv, un);
					VectorNormalize(un);

					/* create 3x4 matrix with columns (right, up, dir) */
					float m[3][4];
					for (k = 0; k < 3; k++)
					{
						m[k][0] = right[k];
						m[k][1] = un[k];
						m[k][2] = dn[k];
						m[k][3] = 0.0f;
					}

					vec4_t quat;
					Mod_Mat3x4ToQuat(m, quat);

					dmdx_baseframe_joint_t *pose = &boneposes[f * pheader->num_joints + b];
					VectorCopy(pos, pose->pos);
					pose->orient[0] = quat[0];
					pose->orient[1] = quat[1];
					pose->orient[2] = quat[2];
					pose->orient[3] = quat[3];
				}
			}
		}

	}
flex_skel_done:
#endif
	}

	/* Skipped blocks:
	 * "normals",
	 * "short frames",
	 * "comp data",
	 * "references".
	 */

	Mod_LoadFixImages(mod_name, pheader, false);

#if 0
	/* Additional diagnostics: report how many vertices got bindings and any unbound verts */
	if (pheader && pheader->num_xyz > 0)
	{
		dmdx_vertex_t *bindings = (dmdx_vertex_t *)((byte *)pheader + pheader->ofs_mesh_verteces);
		int vi;
		int bound_vertices = 0;
		int total_weights_counted = 0;
		int unbound_list[32];
		int unbound_n = 0;

		for (vi = 0; vi < pheader->num_xyz; vi++)
		{
			total_weights_counted += bindings[vi].count;
			if (bindings[vi].count > 0)
			{
				bound_vertices++;
				/* sanity check binding ranges */
				if (bindings[vi].start < 0 || bindings[vi].start + bindings[vi].count > pheader->num_weights)
				{
					Com_Printf("%s: binding out of range for vert %d: start=%d count=%d num_weights=%d\n",
						__func__, vi, bindings[vi].start, bindings[vi].count, pheader->num_weights);
				}
			}
			else
			{
				if (unbound_n < (int)(sizeof(unbound_list)/sizeof(unbound_list[0])))
					unbound_list[unbound_n] = vi;
				unbound_n++;
			}
		}

		Com_Printf("%s: model %s loaded: bones=%d weights(header)=%d weights(counted)=%d verts=%d bound=%d unbound=%d ofs_weights=%d ofs_mesh_verteces=%d ofs_baseframe_joints=%d\n",
			__func__, mod_name, pheader->num_joints, pheader->num_weights, total_weights_counted,
			pheader->num_xyz, bound_vertices, pheader->num_xyz - bound_vertices,
			pheader->ofs_weights, pheader->ofs_mesh_verteces, pheader->ofs_baseframe_joints);

		if (unbound_n > 0)
		{
			int dump_n = Q_min(unbound_n, 20);
			int j;
			Com_Printf("%s: unbound vertex indices (first %d):", __func__, dump_n);
			for (j = 0; j < dump_n; j++)
			{
				Com_Printf(" %d", unbound_list[j]);
			}
			Com_Printf("\n");
		}
	}

	printf("%s: model %s loaded with %d bones and %d weights\n",
		__func__, mod_name, pheader ? pheader->num_joints : 0, pheader ? pheader->num_weights : 0);
#endif
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

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = 256;
	dmdxheader.skinheight = 256;
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
	Mod_LoadModel_AnimGroupNamesFix(pheader, dkm_names);

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

	/* has same frame structure */
	if (header.framesize < (
		sizeof(daliasframe_t) + (header.num_xyz - 1) * sizeof(dtrivertx_t)))
	{
		Com_Printf("%s: model %s has incorrect framesize\n",
				__func__, mod_name);
		return NULL;
	}

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = header.skinwidth;
	dmdxheader.skinheight = header.skinwidth;
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
	Mod_LoadAnimGroupList(pheader, true);
	Mod_LoadModel_AnimGroupNamesFix(pheader, kingpin_names);
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

