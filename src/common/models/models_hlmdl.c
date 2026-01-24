/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 2005-2015 David HENRY
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
 * The Half-Life MDL models file format
 *
 * =======================================================================
 */

#include "models.h"
#include <math.h>

/* bone controllers */
typedef struct
{
	int bone;	/* -1 == 0 */
	int type;	/* X, Y, Z, XR, YR, ZR, M */
	float start;
	float end;
	int rest;	/* byte index value at rest */
	int index;	/* 0-3 user set controller, 4 mouth */
} hlmdl_bonecontroller_t;

typedef float bonematrix_t[3][4];

typedef unsigned short hlmdl_anim_t[6];

// animation frames
typedef union
{
	struct {
		byte valid;
		byte total;
	} num;
	short		value;
} hlmdl_animvalue_t;

// Minimal math helpers copied from engine mathlib
static void
AngleQuaternion(const vec3_t angles, vec4_t quaternion)
{
	float angle, sr, sp, sy, cr, cp, cy;

	angle = angles[2] * 0.5;
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[1] * 0.5;
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[0] * 0.5;
	sr = sinf(angle);
	cr = cosf(angle);

	quaternion[0] = sr*cp*cy-cr*sp*sy; // X
	quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy+sr*sp*sy; // W
}

static void
QuaternionMatrix(const vec4_t quaternion, float (*matrix)[4])
{
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

static void
QuaternionSlerp(const vec4_t p, vec4_t q, float t, vec4_t qt)
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}

	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			((float*)q)[i] = -((float*)q)[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if ((1.0f + cosom) > 0.00000001f)
	{
		if ((1.0f - cosom) > 0.00000001f)
		{
			omega = acosf(cosom);
			sinom = sinf(omega);
			sclp = sinf((1.0f - t) * omega) / sinom;
			sclq = sinf(t * omega ) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}

		for (i = 0; i < 4; i++)
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sinf( (1.0f - t) * 0.5f * M_PI);
		sclq = sinf( t * 0.5f * M_PI);

		for (i = 0; i < 3; i++)
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

static void
VectorTransform(const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

// Calculate quaternion from anim (port of StudioModel functions, assumes panim points to array per bone)
static void
CalcBoneQuaternion(int frame, float s, hlmdl_bone_t *pbone, hlmdl_anim_t *panim, float *q_out)
{
	vec4_t q1, q2;
	vec3_t angle1, angle2;
	hlmdl_animvalue_t *panimvalue;
	int j;

	for (j = 0; j < 3; j++)
	{
		// default
		angle1[j] = angle2[j] = pbone->value[j + 3];
		if (panim && (*panim)[j + 3] != 0)
		{
			panimvalue = (hlmdl_animvalue_t *)((byte*)panim + (*panim)[j + 3]);
			int k = frame;
			if (panimvalue->num.total < panimvalue->num.valid)
			{
				k = 0;
			}

			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue = (hlmdl_animvalue_t *)((byte*)panimvalue + (panimvalue->num.valid + 1) * sizeof(hlmdl_animvalue_t));
				if (panimvalue->num.total < panimvalue->num.valid)
				{
					k = 0;
				}
			}

			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
					{
						angle2[j] = angle1[j];
					}
					else
					{
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
					}
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;

				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}

			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}
		// no controllers support; m_adj assumed 0
	}

	if (angle1[0] != angle2[0] || angle1[1] != angle2[1] || angle1[2] != angle2[2])
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q_out);
	}
	else
	{
		AngleQuaternion(angle1, q_out);
	}
}

// Calculate bone position from anim
static void
CalcBonePosition(int frame, float s, hlmdl_bone_t *pbone, hlmdl_anim_t *panim, float *pos)
{
	hlmdl_animvalue_t *panimvalue;
	int j;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j];

		if (panim && (*panim)[j] != 0)
		{
			panimvalue = (hlmdl_animvalue_t *)((byte*)panim + (*panim)[j]);
			int k = frame;
			if (panimvalue->num.total < panimvalue->num.valid)
			{
				k = 0;
			}

			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue = (hlmdl_animvalue_t *)((byte*)panimvalue + (panimvalue->num.valid + 1) * sizeof(hlmdl_animvalue_t));
				if (panimvalue->num.total < panimvalue->num.valid)
				{
					k = 0;
				}
			}

			if (panimvalue->num.valid > k)
			{
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
	}
}

/*
=================
Mod_LoadHLMDLAnimGroupList

Load HLMDL animation group lists
=================
*/
static void
Mod_LoadHLMDLAnimGroupList(dmdx_t *pheader, const hlmdl_sequence_t *sequences, int num_seq)
{
	dmdxframegroup_t *pframegroup;
	int i, frame_offset;

	pframegroup = (dmdxframegroup_t*)((char *)pheader + pheader->ofs_animgroup);
	frame_offset = 0;

	/* Create animation group for each sequence */
	for (i = 0; i < num_seq; i++)
	{
		/* Copy sequence name as group name (truncate if needed) */
		Q_strlcpy(pframegroup[i].name, sequences[i].name, sizeof(pframegroup[i].name));

		/* Set frame range for this group */
		pframegroup[i].ofs = frame_offset;
		pframegroup[i].num = sequences[i].num_frames;

		Mod_UpdateMinMaxByFrames(pheader,
			pframegroup[i].ofs, pframegroup[i].ofs + pframegroup[i].num,
			pframegroup[i].mins, pframegroup[i].maxs);

		frame_offset += pframegroup[i].num;
	}
}

static void
Mod_LoadHLMDLSkinsSize(const hlmdl_header_t *pinmodel, const byte *buffer,
	int *w, int *h)
{
	hlmdl_texture_t *in_skins;
	size_t i;

	*w = 8;
	*h = 8;

	in_skins = (hlmdl_texture_t *)((byte *)buffer + pinmodel->ofs_texture);
	for (i = 0; i < pinmodel->num_skins; i++)
	{
		size_t width, height;

		width = LittleLong(in_skins[i].width);
		height = LittleLong(in_skins[i].height);

		if (*w < width)
		{
			*w = width;
		}

		if (*h < height)
		{
			*h = height;
		}
	}
}

static void
Mod_LoadHLMDLSkins(const char *mod_name, dmdx_t *pheader, const hlmdl_header_t *pinmodel,
	const byte *buffer)
{
	hlmdl_texture_t *in_skins;
	size_t size;
	int i;
	byte *img_cache;

	size = pheader->skinwidth * pheader->skinheight * 4;
	img_cache = malloc(size);
	if (!img_cache)
	{
		YQ2_COM_CHECK_OOM(img_cache, "malloc()", size)
		return;
	}

	in_skins = (hlmdl_texture_t *)((byte *)buffer + pinmodel->ofs_texture);
	for (i = 0; i < pinmodel->num_skins; i++)
	{
		char *skin;
		byte *pal, *src;
		int y, width, height;

		skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;

		snprintf(skin, MAX_SKINNAME, "%s#%d.lmp", mod_name, i);

		width = LittleLong(in_skins[i].width);
		height = LittleLong(in_skins[i].height);

		src = (byte *)buffer + in_skins[i].offset;
		pal = src + width * height;

		for (y = 0; y < pheader->skinheight; y++)
		{
			size_t x;

			for (x = 0; x < pheader->skinwidth; x++)
			{
				byte *dst, val;

				dst = img_cache + (y * pheader->skinwidth + x) * 4;
				val = src[
					(x * width / pheader->skinwidth) +
					(y * height / pheader->skinheight) * width
				];

				dst[0] = pal[val * 3 + 0];
				dst[1] = pal[val * 3 + 1];
				dst[2] = pal[val * 3 + 2];
				dst[3] = 255;
			}
		}

		/* copy image */
		memcpy((byte*)pheader + pheader->ofs_imgbit + (size * i), img_cache, size);
	}

	free(img_cache);
}

/*
=================
Mod_LoadModel_HLMDL
=================
*/
void *
Mod_LoadModel_HLMDL(const char *mod_name, const void *buffer, int modfilelen)
{
	const hlmdl_header_t pinmodel;
	dmdx_t dmdxheader, *pheader;
	dmdxmesh_t *mesh_nodes, *mesh_tmp;
	void *extradata;
	const hlmdl_sequence_t *sequences;
	size_t i, framesize;
	int total_frames, skinw, skinh;
	hlmdl_bodypart_t *bodyparts;
	dstvert_t *st_tmp = NULL;
	dtriangle_t *tri_tmp = NULL;
	dmdx_vert_t *out_vert = NULL;
	int *out_boneids = NULL;
	int num_st = 0, st_size = 0;
	int num_tris = 0, tri_size = 0;
	int num_verts = 0, verts_size = 0;
	bonematrix_t *bonetransform = NULL;
	vec4_t *quaternion = NULL;
	vec3_t *bonepos = NULL;
	dmdx_vert_t *temp_verts = NULL;
	hlmdl_bone_t *pbones;

	Mod_LittleHeader((int *)buffer, sizeof(pinmodel) / sizeof(int),
		(int *)&pinmodel);

	if (pinmodel.version != HLMDL_VERSION)
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

	if (pinmodel.ofs_texture < 0)
	{
		Com_Printf("%s: model %s incorrect texture possition\n",
				__func__, mod_name);
		return NULL;
	}

	if ((pinmodel.ofs_texture + sizeof(hlmdl_texture_t) * pinmodel.num_skins) > pinmodel.ofs_end)
	{
		Com_Printf("%s: model %s incorrect texture size\n",
				__func__, mod_name);
		return NULL;
	}

	sequences = (const hlmdl_sequence_t *)((const byte *)buffer + pinmodel.ofs_seq);

	Mod_LoadHLMDLSkinsSize(&pinmodel, buffer, &skinw, &skinh);

	bodyparts = (hlmdl_bodypart_t *)((byte *)buffer + pinmodel.ofs_bodyparts);
	mesh_tmp = calloc(pinmodel.num_bodyparts, sizeof(*mesh_tmp));

	for (i = 0; i < pinmodel.num_bodyparts; i++)
	{
		hlmdl_bodymodel_t *bodymodels;
		int j;

		/* TODO: convert submodels to additional meshes? */
		mesh_tmp[i].ofs_tris = num_tris;

		bodymodels = (hlmdl_bodymodel_t *)((byte *)buffer + bodyparts[i].ofs_model);
		for (j = 0; j < bodyparts[i].num_models; j++)
		{
			hlmdl_bodymesh_t *mesh_nodes;
			vec3_t *in_verts;
			byte *in_boneids;
			int k;

			mesh_nodes = (hlmdl_bodymesh_t *)((byte *)buffer + bodymodels[j].ofs_mesh);
			for (k = 0; k < bodymodels[j].num_mesh; k++)
			{
				short *trivert;
				int l;

				trivert = (short *)((byte *)buffer + mesh_nodes[k].ofs_tris);
				while ((l = *(trivert++)))
				{
					int g, count = l, st_prefix = num_st;
					int *verts = NULL;

					if (count < 0)
					{
						count = -count;
					}

					verts = malloc(count * sizeof(*verts));
					YQ2_COM_CHECK_OOM(verts, "malloc()", count * sizeof(*verts))
					if (!verts)
					{
						/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
						break;
					}

					if (!st_tmp || (num_st + count) >= st_size)
					{
						dstvert_t *tmp = NULL;

						st_size = num_st + count * 2;
						tmp = realloc(st_tmp, st_size * sizeof(*st_tmp));
						YQ2_COM_CHECK_OOM(tmp, "realloc()", st_size * sizeof(*st_tmp))
						if (!tmp)
						{
							/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
							st_size = num_st;
							break;
						}

						st_tmp = tmp;
					}

					if (!tri_tmp || (num_tris + count) >= tri_size)
					{
						dtriangle_t *tmp = NULL;

						tri_size = num_tris + count * 2;
						tmp = realloc(tri_tmp, tri_size * sizeof(*tri_tmp));
						YQ2_COM_CHECK_OOM(tmp, "realloc()", tri_size * sizeof(*tri_tmp))
						if (!tmp)
						{
							/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
							tri_size = num_tris;
							break;
						}

						tri_tmp = tmp;
					}

					for (g = 0; g < count; g++, trivert += 4)
					{
						st_tmp[num_st].s = trivert[2]; // sizew
						st_tmp[num_st].t = trivert[3]; // sizeh
						num_st++;

						verts[g] = trivert[0];
					}

					/* Reconstruct triangles */
					if (l > 0)
					{
						/* Triangle strip */
						for (g = 0; g < count - 2; g++)
						{
							dtriangle_t *tri = &tri_tmp[num_tris++];

							if (g % 2 == 0)
							{
								tri->index_xyz[0] = verts[g];
								tri->index_xyz[1] = verts[g + 1];
								tri->index_xyz[2] = verts[g + 2];
							}
							else
							{
								tri->index_xyz[0] = verts[g + 1];
								tri->index_xyz[1] = verts[g];
								tri->index_xyz[2] = verts[g + 2];
							}
							tri->index_st[0] = st_prefix + g;
							tri->index_st[1] = st_prefix + g + 1;
							tri->index_st[2] = st_prefix + g + 2;
						}
					}
					else
					{
						// Triangle fan
						for (g = 1; g < count - 1; g++)
						{
							dtriangle_t *tri = &tri_tmp[num_tris++];

							tri->index_xyz[0] = verts[0];
							tri->index_xyz[1] = verts[g];
							tri->index_xyz[2] = verts[g + 1];

							tri->index_st[0] = st_prefix + 0;
							tri->index_st[1] = st_prefix + g;
							tri->index_st[2] = st_prefix + g + 1;
						}
					}

					free(verts);
				}
			}

			in_verts = (vec3_t *)((byte *)buffer + bodymodels[j].ofs_vert);
			in_boneids = NULL;
			if (bodymodels[j].ofs_vertinfo > 0 &&
				bodymodels[j].ofs_vertinfo + bodymodels[j].num_verts <= modfilelen)
			{
				in_boneids = (byte*)((byte *)buffer + bodymodels[j].ofs_vertinfo);
			}

			if (!out_vert ||
				(num_verts + bodymodels[j].num_verts) >= verts_size)
			{
				dmdx_vert_t *tmp_verts = NULL;
				int *tmp_boneids = NULL;

				verts_size = num_verts + bodymodels[j].num_verts * 2;
				tmp_verts = realloc(out_vert, verts_size * sizeof(*out_vert));
				YQ2_COM_CHECK_OOM(tmp_verts, "realloc()", verts_size * sizeof(*out_vert))
				if (!tmp_verts)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					verts_size = num_verts;
					break;
				}

				tmp_boneids = realloc(out_boneids, verts_size * sizeof(*out_boneids));
				YQ2_COM_CHECK_OOM(tmp_boneids, "realloc()", verts_size * sizeof(*out_boneids))
				if (!tmp_boneids)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					verts_size = num_verts;
					break;
				}

				out_vert = tmp_verts;
				out_boneids = tmp_boneids;
			}

			for (k = 0; k < bodymodels[j].num_verts; k++)
			{
				int l, bone = 0;

				for (l = 0; l < 3; l++)
				{
					out_vert[num_verts].xyz[l] = LittleFloat(in_verts[k][l]);
				}

				if (in_boneids)
				{
					bone = in_boneids[k];
				}

				out_boneids[num_verts] = bone;

				num_verts++;
			}
		}

		mesh_tmp[i].num_tris = num_tris - mesh_tmp[i].ofs_tris;
	}

	/* Calculate total number of frames (sum of all sequences' num_frames) */
	total_frames = 0;
	for (i = 0; i < pinmodel.num_seq; i++)
	{
		total_frames += sequences[i].num_frames;
	}

	/* Calculate frame size */
	framesize = sizeof(daliasxframe_t) - sizeof(dxtrivertx_t);
	framesize += num_verts * sizeof(dxtrivertx_t);

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = skinw;
	dmdxheader.skinheight = skinh;
	dmdxheader.framesize = framesize;

	dmdxheader.num_meshes = pinmodel.num_bodyparts;
	dmdxheader.num_skins = pinmodel.num_skins;
	dmdxheader.num_xyz = num_verts;
	dmdxheader.num_st = num_st;
	dmdxheader.num_tris = num_tris;
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	dmdxheader.num_glcmds = (10 * num_tris) + 1 * pinmodel.num_bodyparts;
	dmdxheader.num_imgbit = 32;
	dmdxheader.num_frames = total_frames;
	dmdxheader.num_animgroup = pinmodel.num_seq;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	memcpy(mesh_nodes, mesh_tmp, pinmodel.num_bodyparts * sizeof(*mesh_tmp));
	free(mesh_tmp);

	memcpy((char *)pheader + pheader->ofs_st, st_tmp,
		num_st * sizeof(dstvert_t));
	free(st_tmp);

	memcpy((char *)pheader + pheader->ofs_tris, tri_tmp,
		num_tris * sizeof(dtriangle_t));
	free(tri_tmp);

	if (pinmodel.num_bones > 0)
	{
		bonetransform = malloc(sizeof(bonematrix_t) * pinmodel.num_bones);
		YQ2_COM_CHECK_OOM(bonetransform, "malloc()", sizeof(bonematrix_t) * pinmodel.num_bones)
		quaternion = malloc(sizeof(vec4_t) * pinmodel.num_bones);
		YQ2_COM_CHECK_OOM(quaternion, "malloc()", sizeof(vec4_t) * pinmodel.num_bones)
		bonepos = malloc(sizeof(vec3_t) * pinmodel.num_bones);
		YQ2_COM_CHECK_OOM(bonepos, "malloc()", sizeof(vec3_t) * pinmodel.num_bones)
	}

	temp_verts = malloc(sizeof(dmdx_vert_t) * num_verts);
	YQ2_COM_CHECK_OOM(temp_verts, "malloc()", sizeof(dmdx_vert_t) * num_verts)
	if (!temp_verts)
	{
		/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
		return NULL;
	}

	total_frames = 0;
	pbones = (hlmdl_bone_t *)((byte*)buffer + pinmodel.ofs_bones);
	for (i = 0; i < pinmodel.num_seq; i++)
	{
		const hlmdl_sequence_t *pseq = &sequences[i];
		hlmdl_anim_t *panim = NULL;
		int j;

		if (pseq->seqgroup == 0)
		{
			panim = (hlmdl_anim_t *)((byte*)buffer + pseq->animindex);
		}
		else
		{
			/* sequence group from other file is unsupported */
		}

		for(j = 0; j < pseq->num_frames; j ++)
		{
			daliasxframe_t *frame = (daliasxframe_t *)(
				(byte *)pheader + pheader->ofs_frames + total_frames * pheader->framesize);

			/* name of frames will be duplicated */
			Q_strlcpy(frame->name, sequences[i].name, sizeof(frame->name));

			/* compute bone transforms */
			if (pinmodel.num_bones > 0 && bonetransform && quaternion && bonepos)
			{
				int v, b;

				memset(bonetransform, 0, sizeof(bonematrix_t) * pinmodel.num_bones);
				memset(quaternion, 0, sizeof(vec4_t) * pinmodel.num_bones);
				memset(bonepos, 0, sizeof(vec3_t) * pinmodel.num_bones);

				for (b = 0; b < pinmodel.num_bones; ++b)
				{
					float bonematrix[3][4];
					int parent;

					CalcBoneQuaternion(j, 0.0f, &pbones[b],
						panim ? &panim[b] : NULL, quaternion[b]);
					CalcBonePosition(j, 0.0f, &pbones[b],
						panim ? &panim[b] : NULL, bonepos[b]);

					QuaternionMatrix(quaternion[b], bonematrix);
					bonematrix[0][3] = bonepos[b][0];
					bonematrix[1][3] = bonepos[b][1];
					bonematrix[2][3] = bonepos[b][2];

					parent = pbones[b].parent;
					if (parent == -1)
					{
						memcpy(bonetransform[b], bonematrix, sizeof(float) * 12);
					}
					else
					{
						R_ConcatTransforms(bonetransform[parent], bonematrix, bonetransform[b]);
					}
				}

				/* transform verts */
				for (v = 0; v < num_verts; ++v)
				{
					int bone = out_boneids[v];
					if (bone < 0 || bone >= pinmodel.num_bones)
					{
						bone = 0;
					}

					VectorTransform(out_vert[v].xyz, bonetransform[bone], temp_verts[v].xyz);
					VectorClear(out_vert[v].norm);
				}

				PrepareFrameVertex(temp_verts, num_verts, frame);
			}
			else
			{
				/* no bones, just copy */
				PrepareFrameVertex(out_vert, num_verts, frame);
			}

			total_frames++;
		}
	}
	free(out_vert);
	free(out_boneids);
	free(bonetransform);
	free(quaternion);
	free(bonepos);
	free(temp_verts);

	Mod_LoadHLMDLSkins(mod_name, pheader, &pinmodel, buffer);
	Mod_LoadHLMDLAnimGroupList(pheader, sequences, pinmodel.num_seq);
	Mod_LoadFixNormals(pheader);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
