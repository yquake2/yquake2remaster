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

static void *
Mod_LoadModelFile(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, readfile_t read_file, modtype_t *type);

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
		R_Printf(PRINT_ALL, "%s: Entity %s has possible last element issues with %d verts.\n",
			__func__, mod_name, poutcmd[pheader->num_glcmds-1]);
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
Mod_LoadFrames_VertAnox(dxtrivertx_t *vert, const short *in)
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

	for (i=0 ; i < pheader->num_frames ; i++)
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

			R_ConvertNormalMDL(pinframe->verts[j].lightnormalindex,
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
Mod_LoadFrames_SAM(dmdx_t *pheader, sin_sam_header_t **anims, int anim_num,
	vec3_t translate, float scale)
{
	int curr_frame = 0, i;

	for (i = 0; i < anim_num; i++)
	{
		sin_frame_t *pinframe;
		int k;

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
				R_ConvertNormalMDL(verts[j].lightnormalindex,
					poutframe->verts[j].normal);
			}

			curr_frame ++;
		}
	}
}

/*
=================
Mod_LoadFrames_MD2Anox

Load the Anachronox md2 format frame
=================
*/
static void
Mod_LoadFrames_MD2Anox(dmdx_t *pheader, byte *src, size_t inframesize,
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
					Mod_LoadFrames_VertAnox(poutframe->verts + j, (short*)inverts);
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
				R_Printf(PRINT_DEVELOPER,
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
Mod_LoadMD2TriangleList

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

		R_Printf(PRINT_DEVELOPER, "%s: %s #%d: Should load %s '%s'\n",
			__func__, mod_name, i, internal ? "internal": "external", skin);
	}

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
			R_Printf(PRINT_ALL, "%s: model %s has unsupported skin type %d\n",
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
		snprintf(out_pos + MAX_SKINNAME * i, MAX_SKINNAME, "%s#%d.tga", mod_name, i);

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		curr_pos += sizeof(int);
		if (skin_type)
		{
			R_Printf(PRINT_ALL, "%s: model %s has unsupported skin type %d\n",
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
Mod_LoadModel_MDL(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	const mdl_header_t		*pinmodel;
	int		version;
	dmdx_t		*pheader;
	void	*extradata;
	dmdxmesh_t *mesh_nodes;

	/* local copy of all values */
	int skinwidth, skinheight, framesize;
	int num_meshes, num_skins, num_xyz, num_st, num_tris, num_glcmds,
		num_frames, frame_count;
	int ofs_meshes, ofs_skins, ofs_st, ofs_tris, ofs_frames, ofs_glcmds,
		ofs_imgbit, ofs_end;

	pinmodel = (mdl_header_t *)buffer;

	version = LittleLong(pinmodel->version);
	if (version != MDL_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
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

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * (num_xyz - 1);

	/* validate */
	if (num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames\n",
				__func__, mod_name);
		return NULL;
	}

	frame_count = Mod_LoadMDLCountFrames(mod_name, buffer, num_skins,
		skinwidth, skinheight, num_st, num_tris, num_frames, num_xyz);

	if (frame_count <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has issues with frame count\n",
				__func__, mod_name);
		return NULL;
	}

	/* generate offsets */
	ofs_meshes = sizeof(*pheader); // just skip header and go
	ofs_skins = ofs_meshes + num_meshes * sizeof(dmdxmesh_t);
	ofs_st = ofs_skins + num_skins * MAX_SKINNAME;
	ofs_tris = ofs_st + num_st * sizeof(dstvert_t) * 2;
	ofs_glcmds = ofs_tris + num_tris * sizeof(dtriangle_t);
	ofs_frames = ofs_glcmds + num_glcmds * sizeof(int);
	ofs_imgbit = ofs_frames + framesize * frame_count;
	/* one less as single vertx in frame by default */
	ofs_end = ofs_imgbit + (skinwidth * skinheight * num_skins);

	*numskins = num_skins;
	extradata = Hunk_Begin(ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	/* copy back all values */
	memset(pheader, 0, sizeof(*pheader));
	pheader->skinwidth = skinwidth;
	pheader->skinheight = skinheight;
	pheader->framesize = framesize;

	pheader->num_meshes = num_meshes;
	pheader->num_skins = num_skins;
	pheader->num_xyz = num_xyz;
	pheader->num_st = num_st * 2;
	pheader->num_tris = num_tris;
	pheader->num_glcmds = num_glcmds;
	pheader->num_imgbit = 8;
	pheader->num_frames = frame_count;

	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_imgbit = ofs_imgbit;
	pheader->ofs_end = ofs_end;

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

			poutst = (dstvert_t *) ((byte *)pheader + ofs_st);

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

			pouttri = (dtriangle_t *) ((byte *)pheader + ofs_tris);

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

				frame = (daliasframe_t *) ((byte *)pheader + ofs_frames + frame_count * framesize);
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
					R_ConvertNormalMDL(pinvertx[j].lightnormalindex,
						poutvertx[j].normal);
				}

				curr_pos += sizeof(dtrivertx_t) * num_xyz;
				frame_count++;
			}
		}
	}

	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, true);

	*type = mod_alias;

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
Mod_LoadModel_MD3(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	int framesize, ofs_skins, ofs_frames, ofs_glcmds, ofs_meshes, ofs_tris,
		ofs_st, ofs_end;
	int num_xyz = 0, num_tris = 0, num_glcmds = 0, num_skins = 0, meshofs = 0;
	dmdx_t *pheader = NULL;
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
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i=0 ; i < sizeof(pinmodel) / sizeof(int) ; i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ID3_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ID3_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_meshes < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect meshes count %d\n",
				__func__, mod_name, pinmodel.num_meshes);
		return NULL;
	}

	if (pinmodel.num_frames < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect frames count %d\n",
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
			R_Printf(PRINT_ALL, "%s: model %s broken mesh %d\n",
					__func__, mod_name, i);
			return NULL;
		}

		meshofs += LittleLong(md3_mesh->ofs_end);
	}

	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	num_glcmds = (10 * num_tris) + 1 * pinmodel.num_meshes;

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * num_xyz;
	ofs_skins = sizeof(dmdx_t);
	ofs_frames = ofs_skins + num_skins * MAX_SKINNAME;
	ofs_glcmds = ofs_frames + framesize * pinmodel.num_frames;
	ofs_meshes = ofs_glcmds + num_glcmds * sizeof(int);
	ofs_tris = ofs_meshes + pinmodel.num_meshes * sizeof(dmdxmesh_t);
	ofs_st = ofs_tris + num_tris * sizeof(dtriangle_t);
	ofs_end = ofs_st + num_tris * 3 * sizeof(dstvert_t);

	*numskins = num_skins;
	extradata = Hunk_Begin(ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	pheader->framesize = framesize;
	pheader->skinheight = 256;
	pheader->skinwidth = 256;
	pheader->num_skins = *numskins;
	pheader->num_glcmds = num_glcmds;
	pheader->num_frames = pinmodel.num_frames;
	pheader->num_xyz = num_xyz;
	pheader->num_meshes = pinmodel.num_meshes;
	pheader->num_st = num_tris * 3;
	pheader->num_tris = num_tris;
	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_end = ofs_end;

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
			strncpy(frame->name, md3_frameinfo->name, sizeof(frame->name) - 1);
		}
		else
		{
			snprintf(frame->name, 15, "%d", i);
		}

		PrepareFrameVertex(vertx + i * pheader->num_xyz,
			pheader->num_xyz, frame);

		inframe += sizeof(md3_frameinfo_t);
	}
	free(vertx);

	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}

/*
=================
Mod_LoadModel_MD2Anox

ANACHRONOX Model
=================
*/
static void *
Mod_LoadModel_MD2Anox(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	int framesize, ofs_meshes, ofs_skins, ofs_st, ofs_tris, ofs_glcmds,
		ofs_frames, ofs_end;
	vec3_t translate = {0, 0, 0};
	const dtriangle_t *pintri;
	const dstvert_t *pinst;
	dmdxmesh_t *mesh_nodes;
	const int *pincmd;
	dmdla_t pinmodel;
	void *extradata;
	dmdx_t *pheader;
	int i;

	if (modfilelen < sizeof(pinmodel))
	{
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_ANACHRONOX_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ALIAS_ANACHRONOX_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect skins count %d\n",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.resolution < 0 || pinmodel.resolution > 2)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect vert type %d\n",
				__func__, mod_name, pinmodel.resolution);
		return NULL;
	}

	if (pinmodel.num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (pinmodel.num_st <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no st vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames\n",
				__func__, mod_name);
		return NULL;
	}

	framesize = sizeof(daliasxframe_t) +
		(pinmodel.num_xyz - 1) * sizeof(dxtrivertx_t);
	ofs_meshes = sizeof(*pheader); // just skip header and go
	ofs_skins = ofs_meshes + 1 * sizeof(dmdxmesh_t);
	ofs_st = ofs_skins + pinmodel.num_skins * MAX_SKINNAME;
	ofs_tris = ofs_st + pinmodel.num_st * sizeof(dstvert_t);
	ofs_glcmds = ofs_tris + pinmodel.num_tris * sizeof(dtriangle_t);
	ofs_frames = ofs_glcmds + pinmodel.num_glcmds * sizeof(int);
	ofs_end = ofs_frames + framesize * pinmodel.num_frames;

	*numskins = pinmodel.num_skins;
	extradata = Hunk_Begin(ofs_end +
		Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	/* Copy values as we have mostly same data format */
	memset(pheader, 0, sizeof(*pheader));
	pheader->skinwidth = pinmodel.skinwidth;
	pheader->skinheight = pinmodel.skinheight;
	pheader->framesize = framesize;

	pheader->num_meshes = 1;
	pheader->num_skins = pinmodel.num_skins;
	pheader->num_xyz = pinmodel.num_xyz;
	pheader->num_st = pinmodel.num_st;
	pheader->num_tris = pinmodel.num_tris;
	pheader->num_glcmds = pinmodel.num_glcmds;
	pheader->num_frames = pinmodel.num_frames;

	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_end = ofs_end;

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
	Mod_LoadFrames_MD2Anox(pheader, (byte *)buffer + pinmodel.ofs_frames,
		pinmodel.framesize, translate, pinmodel.resolution);

	Mod_LoadFixNormals(pheader);

	//
	// load the glcmds
	//
	pincmd = (int *)((byte *)buffer + pinmodel.ofs_glcmds);
	Mod_LoadCmdList(mod_name, pheader, pincmd);

	// register all skins
	memcpy((char *)pheader + pheader->ofs_skins, (char *)buffer + pinmodel.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);

	*type = mod_alias;

	return extradata;
}

/*
=================
Mod_LoadModel_MD2
=================
*/
static void *
Mod_LoadModel_MD2(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	int ofs_meshes, ofs_skins, ofs_st, ofs_tris, ofs_glcmds, ofs_frames, ofs_end;
	vec3_t translate = {0, 0, 0};
	const dtriangle_t *pintri;
	dmdxmesh_t *mesh_nodes;
	const dstvert_t *pinst;
	qboolean normalfix;
	const int *pincmd;
	dmdl_t pinmodel;
	dmdx_t *pheader;
	void *extradata;
	int i, framesize;

	if (modfilelen < sizeof(pinmodel))
	{
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ALIAS_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect skins count %d\n",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.framesize != (
		sizeof(daliasframe_t) + (pinmodel.num_xyz - 1) * sizeof(dtrivertx_t)))
	{
		R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize\n",
				__func__, mod_name);
		return NULL;
	}

	framesize = sizeof(daliasxframe_t) +
		(pinmodel.num_xyz - 1) * sizeof(dxtrivertx_t);
	ofs_meshes = sizeof(*pheader); // just skip header and go
	ofs_skins = ofs_meshes + 1 * sizeof(dmdxmesh_t);
	ofs_st = ofs_skins + pinmodel.num_skins * MAX_SKINNAME;
	ofs_tris = ofs_st + pinmodel.num_st * sizeof(dstvert_t);
	ofs_glcmds = ofs_tris + pinmodel.num_tris * sizeof(dtriangle_t);
	ofs_frames = ofs_glcmds + pinmodel.num_glcmds * sizeof(int);
	ofs_end = ofs_frames + framesize * pinmodel.num_frames;

	*numskins = pinmodel.num_skins;
	extradata = Hunk_Begin(ofs_end +
		Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	/* Copy values as we have mostly same data format */
	memset(pheader, 0, sizeof(*pheader));
	pheader->skinwidth = pinmodel.skinwidth;
	pheader->skinheight = pinmodel.skinheight;
	pheader->framesize = framesize;

	pheader->num_meshes = 1;
	pheader->num_skins = pinmodel.num_skins;
	pheader->num_xyz = pinmodel.num_xyz;
	pheader->num_st = pinmodel.num_st;
	pheader->num_tris = pinmodel.num_tris;
	pheader->num_glcmds = pinmodel.num_glcmds;
	pheader->num_frames = pinmodel.num_frames;

	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_end = ofs_end;

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = pheader->num_tris;
	mesh_nodes[0].ofs_glcmds = 0;
	mesh_nodes[0].num_glcmds = pheader->num_glcmds;

	if (pheader->num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices\n",
				__func__, mod_name);
	}

	if (pheader->num_st <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no st vertices\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles\n",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames\n",
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

	*type = mod_alias;

	return extradata;
}

/*
=============
Mod_LoadModel_Flex
=============
*/
static void *
Mod_LoadModel_Flex(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins,
	modtype_t *type)
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
				R_Printf(PRINT_ALL, "%s: Too short header\n", __func__);
				return NULL;
			}

			if (version != 2)
			{
				R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
					__func__, blockname, version);
				return NULL;
			}

			inframesize = LittleLong(header->framesize);
			/* has same frame structure */
			if (inframesize < (
				sizeof(daliasframe_t) + (LittleLong(header->num_xyz) - 1) * sizeof(dtrivertx_t)))
			{
				R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize\n",
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

			// just skip header and meshes
			dmdxheader.ofs_meshes = sizeof(dmdxheader);
			dmdxheader.ofs_skins = dmdxheader.ofs_meshes + sizeof(dmdxmesh_t) * dmdxheader.num_meshes;
			dmdxheader.ofs_st = dmdxheader.ofs_skins + dmdxheader.num_skins * MAX_SKINNAME;
			dmdxheader.ofs_tris = dmdxheader.ofs_st + dmdxheader.num_st * sizeof(dstvert_t);
			dmdxheader.ofs_frames = dmdxheader.ofs_tris + dmdxheader.num_tris * sizeof(dtriangle_t);
			dmdxheader.ofs_glcmds = dmdxheader.ofs_frames + dmdxheader.num_frames * dmdxheader.framesize;
			dmdxheader.ofs_end = dmdxheader.ofs_glcmds + dmdxheader.num_glcmds * sizeof(int);

			if (dmdxheader.num_xyz <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no vertices\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_xyz > MAX_VERTS)
			{
				R_Printf(PRINT_ALL, "%s: model %s has too many vertices\n",
						__func__, mod_name);
			}

			if (dmdxheader.num_st <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no st vertices\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_tris <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no triangles\n",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_frames <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no frames\n",
						__func__, mod_name);
				return NULL;
			}

			*numskins = dmdxheader.num_skins;
			extradata = Hunk_Begin(dmdxheader.ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
			pheader = Hunk_Alloc(dmdxheader.ofs_end);
			*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

			memcpy(pheader, &dmdxheader, sizeof(*pheader));
		}
		else {
			if (!pheader)
			{
				R_Printf(PRINT_ALL, "%s: %s has broken header.\n",
					__func__, mod_name);
				return NULL;
			}
			else if (Q_strncasecmp(blockname, "skin", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_skins * MAX_SKINNAME))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}
				memcpy((char*) pheader + pheader->ofs_skins, src, size);
			}
			else if (Q_strncasecmp(blockname, "st coord", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_st * sizeof(dstvert_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadSTvertList(pheader, (dstvert_t *)src);
			}
			else if (Q_strncasecmp(blockname, "tris", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_tris * sizeof(dtriangle_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
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
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}

				if (size < (pheader->num_frames *
					(sizeof(daliasframe_t) + (pheader->num_xyz - 1) * sizeof(dtrivertx_t))))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadFrames_MD2(pheader, (byte *)src, inframesize, translate);
			}
			else if (Q_strncasecmp(blockname, "glcmds", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_glcmds * sizeof(int)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
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
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d\n",
						__func__, blockname, version);
					return NULL;
				}

				/* 516 mesh node size */
				if (size != (num_mesh_nodes * 516))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size\n",
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
				R_Printf(PRINT_ALL, "%s: %s Unknown block %s\n",
					__func__, mod_name, blockname);
				return NULL;
			}
		}
		modfilelen -= size;
		src += size;
	}

	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}

static void *
Mod_LoadModel_DKM(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	dmdx_t dmdxheader, *pheader = NULL;
	dkm_header_t header = {0};
	void *extradata = NULL;
	int i;

	if (sizeof(dkm_header_t) > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	for (i=0 ; i<sizeof(dkm_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong(((int *)buffer)[i]);

	if (header.version != DKM1_VERSION && header.version != DKM2_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, header.version, DKM2_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d\n",
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
			R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize\n",
					__func__, mod_name);
			return NULL;
		}
	}
	else
	{
		if (header.framesize < (
			sizeof(daliasframe_t) + (header.num_xyz - 1) * (sizeof(int) + sizeof(byte))))
		{
			R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize\n",
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

	/* just skip header */
	dmdxheader.ofs_meshes = sizeof(dmdxheader);
	dmdxheader.ofs_skins = dmdxheader.ofs_meshes + dmdxheader.num_meshes * sizeof(dmdxmesh_t);
	dmdxheader.ofs_st = dmdxheader.ofs_skins + dmdxheader.num_skins * MAX_SKINNAME;
	dmdxheader.ofs_tris = dmdxheader.ofs_st + dmdxheader.num_st * sizeof(dstvert_t);
	dmdxheader.ofs_frames = dmdxheader.ofs_tris + dmdxheader.num_tris * sizeof(dtriangle_t);
	dmdxheader.ofs_glcmds = dmdxheader.ofs_frames + dmdxheader.num_frames * dmdxheader.framesize;
	dmdxheader.ofs_end = dmdxheader.ofs_glcmds + dmdxheader.num_glcmds * sizeof(int);

	*numskins = dmdxheader.num_skins;
	extradata = Hunk_Begin(dmdxheader.ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(dmdxheader.ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	memcpy(pheader, &dmdxheader, sizeof(dmdxheader));

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

	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixNormals(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}

static void *
Mod_LoadModel_MDX(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	dmdx_t dmdxheader, *pheader = NULL;
	vec3_t translate = {0, 0, 0};
	mdx_header_t header = {0};
	void *extradata = NULL;
	int i;

	if (sizeof(mdx_header_t) > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small\n",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	for (i=0 ; i<sizeof(mdx_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong(((int *)buffer)[i]);

	if (header.version != MDX_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, header.version, MDX_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d\n",
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
		R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize\n",
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

	/* just skip header */
	dmdxheader.ofs_meshes = sizeof(dmdxheader);
	dmdxheader.ofs_skins = dmdxheader.ofs_meshes + dmdxheader.num_meshes * sizeof(dmdxmesh_t);
	dmdxheader.ofs_st = dmdxheader.ofs_skins + dmdxheader.num_skins * MAX_SKINNAME;
	dmdxheader.ofs_tris = dmdxheader.ofs_st + dmdxheader.num_st * sizeof(dstvert_t);
	dmdxheader.ofs_frames = dmdxheader.ofs_tris + dmdxheader.num_tris * sizeof(dtriangle_t);
	dmdxheader.ofs_glcmds = dmdxheader.ofs_frames + dmdxheader.num_frames * dmdxheader.framesize;
	dmdxheader.ofs_end = dmdxheader.ofs_glcmds + dmdxheader.num_glcmds * sizeof(int);

	*numskins = dmdxheader.num_skins;
	extradata = Hunk_Begin(dmdxheader.ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(dmdxheader.ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	memcpy(pheader, &dmdxheader, sizeof(dmdxheader));

	memcpy((byte*)pheader + pheader->ofs_skins, (byte *)buffer + header.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	Mod_LoadMDXTriangleList(mod_name, pheader,
		(dtriangle_t *)((byte *)buffer + header.ofs_tris),
		(int*)((byte *)buffer + header.ofs_glcmds),
		header.num_glcmds);
	Mod_LoadFrames_MD2(pheader, (byte *)buffer + header.ofs_frames,
		header.framesize, translate);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}

static void *
Mod_LoadModel_SDEF_Text(const char *mod_name, char *curr_buff, readfile_t read_file,
	struct image_s ***skins, int *numskins)
{
	char models_path[MAX_QPATH];
	char base_model[MAX_QPATH * 2];
	char skinnames[255][MAX_SKINNAME];
	char animations[255][MAX_QPATH * 2];
	int actions_num, skinnames_num, i, base_size;
	sin_sbm_header_t *base;
	sin_sam_header_t *anim[255];
	void *extradata = NULL;
	vec3_t translate = {0, 0, 0};
	float scale;

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
			strncpy(models_path, COM_Parse(&curr_buff), sizeof(models_path));
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
			}
			else if (!Q_stricmp(ext, "sam"))
			{
				if (actions_num >= 255)
				{
					R_Printf(PRINT_DEVELOPER, "%s: %s has huge list of animations %s\n",
						__func__, mod_name, base_model);
					continue;
				}
				snprintf(animations[actions_num], sizeof(animations[actions_num]),
					"%s/%s", models_path, token);
				actions_num ++;
			}
			else if (!Q_stricmp(ext, "tga"))
			{
				if (skinnames_num >= 255)
				{
					R_Printf(PRINT_DEVELOPER, "%s: %s has huge list of skinss %s\n",
						__func__, mod_name, base_model);
					continue;
				}
				snprintf(skinnames[skinnames_num], sizeof(skinnames[skinnames_num]),
					"%s/%s", models_path, token);
				skinnames_num ++;
			}
		}
	}

	base_size = read_file(base_model, (void **)&base);
	if (base_size <= 0)
	{
		R_Printf(PRINT_DEVELOPER, "%s: %s No base model for %s\n",
			__func__, mod_name, base_model);
		return NULL;
	}

	/* Convert from LittleLong */
	for (i = 0; i < sizeof(sin_sbm_header_t) / 4; i ++)
	{
		((int *)base)[i] = LittleLong(((int *)base)[i]);
	}

	if ((base->ident != SBMHEADER) || (base->version != MDSINVERSION))
	{
		free(base);
		R_Printf(PRINT_DEVELOPER, "%s: %s, Incorrect ident or version for %s\n",
			__func__, mod_name, base_model);
		return NULL;
	}

	int num_tris = 0;
	{
		sin_trigroup_t *trigroup = (sin_trigroup_t *)((char*)base + sizeof(sin_sbm_header_t));

		for(i = 0; i < base->num_groups; i ++)
		{
			num_tris += LittleLong(trigroup[i].num_tris);
		}
	}

	int framescount = 0;
	int animation_num = 0;

	for (i = 0; i < actions_num; i++)
	{
		int anim_size, j;

		anim_size = read_file(animations[i], (void **)&anim[animation_num]);
		if (anim_size <= 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s empty animation %s\n",
				__func__, mod_name, animations[i]);
			continue;
		}

		if (anim[animation_num]->num_xyz != base->num_xyz)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s, incorrect count tris in animation in %s\n",
				__func__, mod_name, animations[i]);
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
			free(base);
			R_Printf(PRINT_DEVELOPER, "%s: %s, Incorrect ident or version for %s\n",
				__func__, mod_name, base_model);
			return NULL;
		}

		framescount += anim[animation_num]->num_frames;
		animation_num ++;
	}

	if (!animation_num)
	{
		R_Printf(PRINT_DEVELOPER, "%s: %s no animation found\n",
			__func__, mod_name);
		free(base);
		return NULL;
	}

	dmdx_t dmdxheader, *pheader = NULL;
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

	/* just skip header */
	dmdxheader.ofs_meshes = sizeof(dmdxheader);
	dmdxheader.ofs_skins = dmdxheader.ofs_meshes + dmdxheader.num_meshes * sizeof(dmdxmesh_t);
	dmdxheader.ofs_st = dmdxheader.ofs_skins + dmdxheader.num_skins * MAX_SKINNAME;
	dmdxheader.ofs_tris = dmdxheader.ofs_st + dmdxheader.num_st * sizeof(dstvert_t);
	dmdxheader.ofs_frames = dmdxheader.ofs_tris + dmdxheader.num_tris * sizeof(dtriangle_t);
	dmdxheader.ofs_glcmds = dmdxheader.ofs_frames + dmdxheader.num_frames * dmdxheader.framesize;
	dmdxheader.ofs_end = dmdxheader.ofs_glcmds + dmdxheader.num_glcmds * sizeof(int);

	*numskins = dmdxheader.num_skins;
	extradata = Hunk_Begin(dmdxheader.ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(dmdxheader.ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	memcpy(pheader, &dmdxheader, sizeof(dmdxheader));

	memcpy((byte*)pheader + pheader->ofs_skins, (byte *)skinnames,
		pheader->num_skins * MAX_SKINNAME);

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

	Mod_LoadFrames_SAM(pheader, anim, animation_num, translate, scale);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	for (i = 0; i < animation_num; i++)
	{
		free(anim[i]);
	}

	free(base);
	return extradata;
}

static void *
Mod_LoadModel_SDEF(const char *mod_name, const void *buffer, int modfilelen,
	readfile_t read_file, struct image_s ***skins, int *numskins, modtype_t *type)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1);
	memcpy(text, buffer, modfilelen);
	text[modfilelen] = 0;
	extradata = Mod_LoadModel_SDEF_Text(mod_name, text, read_file, skins, numskins);
	free(text);

	*type = mod_alias;

	return extradata;
}

static void *
Mod_LoadModel_MDA_Text(const char *mod_name, char *curr_buff,
	readfile_t read_file, struct image_s ***skins, int *numskins, modtype_t *type)
{
	char base_model[MAX_QPATH * 2] = {0};

	while (curr_buff)
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!*token)
		{
			continue;
		}

		/* found basemodel */
		else if (!strncmp(token, "basemodel", 9))
		{
			token = COM_Parse(&curr_buff);
			if (!token)
			{
				return NULL;
			}
			strncpy(base_model, token, sizeof(base_model) - 1);
			/* other fields is unused for now */
			break;
		}
	}

	if (base_model[0])
	{
		void *extradata, *base;
		int base_size;
		char *curr;

		curr = base_model;
		while (*curr)
		{
			if (*curr == '\\')
			{
				*curr = '/';
			}
			curr++;
		}

		base_size = read_file(base_model, (void **)&base);
		if (base_size <= 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s No base model for %s\n",
				__func__, mod_name, base_model);
			return NULL;
		}

		/* little bit recursive load */
		extradata = Mod_LoadModelFile(mod_name, base, base_size,
			skins, numskins,
			read_file, type);

		free(base);
		return extradata;
	}

	return NULL;
}

static void *
Mod_LoadModel_MDA(const char *mod_name, const void *buffer, int modfilelen,
	readfile_t read_file, struct image_s ***skins, int *numskins, modtype_t *type)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1 - 4);
	memcpy(text, (char *)buffer + 4, modfilelen - 4);
	text[modfilelen - 4] = 0;

	extradata = Mod_LoadModel_MDA_Text(mod_name, text, read_file, skins,
		numskins, type);

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
Mod_LoadSprite_SP2 (const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins,
	modtype_t *type)
{
	const dsprite_t *sprin;
	dsprite_t *sprout;
	int i, numframes;
	void *extradata;

	sprin = (dsprite_t *)buffer;
	numframes = LittleLong(sprin->numframes);

	*numskins = numframes;
	extradata = Hunk_Begin(modfilelen + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	sprout = Hunk_Alloc(modfilelen);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	sprout->ident = LittleLong(sprin->ident);
	sprout->version = LittleLong(sprin->version);
	sprout->numframes = numframes;

	if (sprout->version != SPRITE_VERSION)
	{
		R_Printf(PRINT_ALL, "%s has wrong version number (%i should be %i)\n",
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
		R_Printf(PRINT_DEVELOPER, "%s: %s #%d: Should load external '%s'\n",
			__func__, mod_name, i,
			sprout->frames[i].name);
	}

	*type = mod_sprite;

	return extradata;
}

static void
Mod_LoadLimits(const char *mod_name, void *extradata, modtype_t type)
{
	if (type == mod_alias)
	{
		dmdxmesh_t *mesh_nodes;
		int num_mesh_nodes, i, num_glcmds = 0;
		dmdx_t *pheader;

		pheader = (dmdx_t *)extradata;

		if (pheader->skinheight > MAX_LBM_WIDTH)
		{
			R_Printf(PRINT_ALL, "%s: model %s has a skin taller %d than %d\n",
					__func__, mod_name, pheader->skinheight, MAX_LBM_WIDTH);
		}

		if (pheader->skinwidth > MAX_LBM_WIDTH)
		{
			R_Printf(PRINT_ALL, "%s: model %s has a skin wider %d than %d\n",
					__func__, mod_name, pheader->skinwidth, MAX_LBM_WIDTH);
		}

		num_mesh_nodes = pheader->num_meshes;
		mesh_nodes = (dmdxmesh_t *)((char*)pheader + pheader->ofs_meshes);

		for (i = 0; i < num_mesh_nodes; i++)
		{
			R_Printf(PRINT_DEVELOPER, "%s: model %s mesh #%d: %d commands, %d tris\n",
				__func__, mod_name, i, mesh_nodes[i].num_glcmds, mesh_nodes[i].num_tris);
			num_glcmds += mesh_nodes[i].num_glcmds;
		}
		R_Printf(PRINT_DEVELOPER,
			"%s: model %s num tris %d / num vert %d / commands %d of %d / frames %d\n",
			__func__, mod_name, pheader->num_tris, pheader->num_xyz, num_glcmds,
			pheader->num_glcmds, pheader->num_frames);
	}
}

static void
Mod_LoadMinMaxUpdate(const char *mod_name, vec3_t mins, vec3_t maxs, void *extradata, modtype_t type)
{
	if (type == mod_alias)
	{
		daliasxframe_t *frame;
		const dmdx_t *pheader;
		int i;

		pheader = (dmdx_t *)extradata;
		if (!pheader->num_frames)
		{
			mins[0] = -32;
			mins[1] = -32;
			mins[2] = -32;

			maxs[0] = 32;
			maxs[1] = 32;
			maxs[2] = 32;

			return;
		}

		mins[0] = 9999;
		mins[1] = 9999;
		mins[2] = 9999;

		maxs[0] = -9999;
		maxs[1] = -9999;
		maxs[2] = -9999;

		for (i = 0; i < pheader->num_frames; i++)
		{
			int j;

			frame = (daliasxframe_t *) ((byte *)extradata
				+ pheader->ofs_frames + i * pheader->framesize);

			for (j = 0; j < 3; j++)
			{
				float curr;

				curr = frame->translate[j];

				if (mins[j] > curr)
				{
					mins[j] = curr;
				}

				curr += frame->scale[j] * 0xFFFF;

				if (maxs[j] < curr)
				{
					maxs[j] = curr;
				}
			}
		}

		R_Printf(PRINT_DEVELOPER, "Model %s: %.1fx%.1fx%.1f -> %.1fx%.1fx%.1f\n",
			mod_name,
			mins[0], mins[1], mins[2],
			maxs[0], maxs[1], maxs[2]);
	}
}

/*
=================
Mod_LoadModel
=================
*/
static void *
Mod_LoadModelFile(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, readfile_t read_file, modtype_t *type)
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
			extradata = Mod_LoadModel_MDA(mod_name, buffer, modfilelen,
				read_file, skins, numskins, type);
			break;

		case SDEFHEADER:
			extradata = Mod_LoadModel_SDEF(mod_name, buffer, modfilelen,
				read_file, skins, numskins, type);
			break;

		case MDXHEADER:
			extradata = Mod_LoadModel_MDX(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case DKMHEADER:
			extradata = Mod_LoadModel_DKM(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case RAVENFMHEADER:
			extradata = Mod_LoadModel_Flex(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case IDALIASHEADER:
			{
				/* next short after file type */
				short version;

				version = LittleShort(((short*)buffer)[2]);
				if (version == ALIAS_ANACHRONOX_VERSION)
				{
					extradata = Mod_LoadModel_MD2Anox(mod_name, buffer, modfilelen,
						skins, numskins, type);
				}
				else
				{
					extradata = Mod_LoadModel_MD2(mod_name, buffer, modfilelen,
						skins, numskins, type);
				}
			}
			break;

		case IDMDLHEADER:
			extradata = Mod_LoadModel_MDL(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case ID3HEADER:
			extradata = Mod_LoadModel_MD3(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case MDR_IDENT:
			extradata = Mod_LoadModel_MDR(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case IDMD5HEADER:
			extradata = Mod_LoadModel_MD5(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case IDSPRITEHEADER:
			extradata = Mod_LoadSprite_SP2(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;
	}

	return extradata;
}

/*
=================
Mod_LoadModel
=================
*/
void *
Mod_LoadModel(const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s ***skins, int *numskins,
	findimage_t find_image, loadimage_t load_image, readfile_t read_file,
	modtype_t *type)
{
	void *extradata;

	extradata = Mod_LoadModelFile(mod_name, buffer, modfilelen, skins, numskins,
		read_file, type);

	if (extradata)
	{
		Mod_LoadMinMaxUpdate(mod_name, mins, maxs, extradata, *type);
		Mod_ReLoadSkins(*skins, find_image, load_image, extradata, *type);
		Mod_LoadLimits(mod_name, extradata, *type);
	}

	return extradata;
}

/*
=================
Mod_ReLoad

Reload images in SP2/MD2 (mark registration_sequence)
=================
*/
int
Mod_ReLoadSkins(struct image_s **skins, findimage_t find_image, loadimage_t load_image,
	void *extradata, modtype_t type)
{
	if (type == mod_sprite)
	{
		dsprite_t	*sprout;
		int	i;

		sprout = (dsprite_t *)extradata;
		for (i=0; i < sprout->numframes; i++)
		{
			skins[i] = find_image(sprout->frames[i].name, it_sprite);
			if (!skins[i])
			{
				/* SKIN NAME + sprites prefix */
				char newname[MAX_QPATH + 16];

				/* heretic2 sprites have no "sprites/" prefix */
				snprintf(newname, sizeof(newname) - 1,
					"sprites/%s", sprout->frames[i].name);
				skins[i] = find_image(newname, it_sprite);
			}
		}
		return sprout->numframes;
	}
	else if (type == mod_alias)
	{
		dmdx_t *pheader;

		pheader = (dmdx_t *)extradata;
		if (pheader->ofs_imgbit && load_image)
		{
			byte* images = (byte *)pheader + pheader->ofs_imgbit;
			int i;

			for (i = 0; i < pheader->num_skins; i++)
			{
				skins[i] = load_image(
					(char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME,
					images, pheader->skinwidth, pheader->skinwidth,
					pheader->skinheight, pheader->skinheight,
					pheader->skinheight * pheader->skinwidth,
					it_skin, pheader->num_imgbit);
				images += (pheader->skinheight * pheader->skinwidth * pheader->num_imgbit / 8);
			}
		}
		else
		{
			int i;

			for (i = 0; i < pheader->num_skins; i++)
			{
				skins[i] = find_image((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
			}
		}
		return  pheader->num_frames;
	}

	/* Unknow format, no images associated with it */
	return 0;
}
