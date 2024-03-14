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
static void
Mod_LoadFrames_MD2(dmdx_t *pheader, byte *src, size_t inframesize, vec3_t translate)
{
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
		// verts are all 8 bit, so no swapping needed
		for (j=0; j < pheader->num_xyz; j ++)
		{
			Mod_LoadFrames_VertMD2(poutframe->verts + j, pinframe->verts[j].v);
			poutframe->verts[j].lightnormalindex = pinframe->verts[j].lightnormalindex;
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
					poutframe->verts[j].lightnormalindex = 0;
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
					poutframe->verts[j].lightnormalindex = 0;
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
					poutframe->verts[j].lightnormalindex = 0;
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
Mod_LoadDTriangleList

Load triangle lists
=================
*/
static void
Mod_LoadDTriangleList(dmdx_t *pheader, const dtriangle_t *pintri)
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
Mod_LoadDTriangleList

Load DKM triangle lists
=================
*/
static void
Mod_LoadDkmTriangleList(dmdx_t *pheader, const dkmtriangle_t *pintri)
{
	dtriangle_t *pouttri;
	int i;

	pouttri = (dtriangle_t *) ((char *)pheader + pheader->ofs_tris);

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
Mod_LoadDKMCmdList

Load the DKM glcmds
=================
*/
static void
Mod_LoadDKMCmdList(const char *mod_name, dmdx_t *pheader, int *pincmd)
{
	const int *pendcmd;
	int *poutcmd;
	int i;

	poutcmd = (int *)((char*)pheader + pheader->ofs_glcmds);
	pendcmd = poutcmd + pheader->num_glcmds;
	/* read command count */
	i = LittleLong(*pincmd++);
	*poutcmd++ = i;

	while (i != 0)
	{
		if (i < 0)
		{
			i = -i;
		}

		/* skip unused surf_id and skin index */
		pincmd += 2;

		while (i)
		{
			poutcmd[0] = LittleLong(pincmd[1]);
			poutcmd[1] = LittleLong(pincmd[2]);
			poutcmd[2] = LittleLong(pincmd[0]);
			poutcmd += 3;
			pincmd += 3;
			i --;
		}

		/* read command count */
		i = LittleLong(*pincmd++);
		*poutcmd++ = i;

		if (pendcmd < poutcmd)
		{
			R_Printf(PRINT_ALL, "%s: Entity %s has possible broken glcmd.\n",
				__func__, mod_name);
			break;
		}
	}

	memset (poutcmd, 0, (pendcmd - poutcmd) * sizeof(int));
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

	for (i=0 ; i<pheader->num_frames ; i++)
	{
		daliasframe_t *pinframe;
		daliasxframe_t *poutframe;
		int j;
		dxtrivertx_t	*outverts;
		byte	*inverts;

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
		for(j=0; j < pheader->num_xyz; j++)
		{
			Mod_LoadFrames_VertDKM2(outverts + j, *((int *)inverts));
			inverts += sizeof(int);
			outverts[j].lightnormalindex = *inverts;
			inverts ++;
		}
	}
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
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
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
	num_glcmds = (
		(3 * num_tris) * sizeof(int) * 3 + /* 3 vert */
		(num_tris * sizeof(int)) + /* triangles count */
		sizeof(int) /* final zero */) / sizeof(int);
	num_frames = LittleLong(pinmodel->num_frames);

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * (num_xyz - 1);

	/* validate */
	if (num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices",
				__func__, mod_name);
		return NULL;
	}

	if (num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
				__func__, mod_name);
	}

	if (num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles",
				__func__, mod_name);
		return NULL;
	}

	if (num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames",
				__func__, mod_name);
		return NULL;
	}

	/* get full count of frames */
	{
		const byte *curr_pos;
		int i;

		curr_pos = (byte*)buffer + sizeof(mdl_header_t);

		// register all skins
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
				R_Printf(PRINT_ALL, "%s: model %s has unsupported skin type %d",
						__func__, mod_name, skin_type);
				return NULL;
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

			/* skip bboxmin, bouding box min */
			curr_pos += sizeof(dtrivertx_t);
			/* skip bboxmax, bouding box max */
			curr_pos += sizeof(dtrivertx_t);
			/* name */
			curr_pos += sizeof(char) * 16;
			/* first vertex list */
			curr_pos += sizeof(dtrivertx_t) * num_xyz;

			/* next frames in frame group is unsupported */
			curr_pos += (frames_skip - 1) * (
				/* bouding box */
				sizeof(dtrivertx_t) * 2 +
				/* name */
				16 +
				/* verts */
				sizeof(dtrivertx_t) * num_xyz
			);

			frame_count += frames_skip;
		}
	}

	/* generate offsets */
	ofs_meshes = sizeof(*pheader); // just skip header and go
	ofs_skins = ofs_meshes + num_meshes * sizeof(dmdxmesh_t);
	ofs_st = ofs_skins + num_skins * MAX_SKINNAME;
	ofs_tris = ofs_st + num_st * sizeof(dstvert_t);
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
	pheader->num_st = num_st;
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

		mdl_triangle_t *triangles;
		const mdl_texcoord_t *texcoords;

		curr_pos = (byte*)buffer + sizeof(mdl_header_t);

		// register all skins
		for (i = 0; i < num_skins; ++i)
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
				R_Printf(PRINT_ALL, "%s: model %s has unsupported skin type %d",
						__func__, mod_name, skin_type);
				return NULL;
			}

			/* copy 8bit image */
			memcpy((byte*)pheader + pheader->ofs_imgbit +
					(skinwidth * skinheight * i),
					curr_pos,
					skinwidth * skinheight);
			curr_pos += skinwidth * skinheight;
		}

		/* texcoordinates */
		{
			dstvert_t *poutst = (dstvert_t *) ((byte *)pheader + ofs_st);

			texcoords = (mdl_texcoord_t *)curr_pos;
			curr_pos += sizeof(mdl_texcoord_t) * num_st;

			for(i = 0; i < num_st; i++)
			{
				/* Compute texture coordinates */
				poutst[i].s = LittleLong(texcoords[i].s);
				poutst[i].t = LittleLong(texcoords[i].t);

				if (texcoords[i].onseam)
				{
					poutst[i].s += skinwidth * 0.5f; /* Backface */
				}
			}
		}

		/* triangles */
		{
			dtriangle_t *pouttri = (dtriangle_t *) ((byte *)pheader + ofs_tris);

			triangles = (mdl_triangle_t *) curr_pos;
			curr_pos += sizeof(mdl_triangle_t) * num_tris;

			for (i = 0; i < num_tris; i++)
			{
				int j;

				for (j = 0; j < 3; j++)
				{
					pouttri[i].index_xyz[j] = LittleLong(triangles[i].vertex[j]);
					pouttri[i].index_st[j] = pouttri[i].index_xyz[j];
				}
			}
		}

		frame_count = 0;

		/* register all frames */
		for (i = 0; i < num_frames; ++i)
		{
			int frame_type, j, k, frames_skip;
			daliasframe_t *frame;
			dxtrivertx_t* poutvertx;
			dtrivertx_t *pinvertx;
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
				// verts are all 8 bit, so no swapping needed
				for (j=0; j < num_xyz; j ++)
				{
					Mod_LoadFrames_VertMD2(poutvertx + j, pinvertx[j].v);
					poutvertx[j].lightnormalindex = pinvertx[j].lightnormalindex;
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
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be %ld)",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i=0 ; i < sizeof(pinmodel) / sizeof(int) ; i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ID3_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, pinmodel.version, ID3_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_meshes < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect meshes count %d",
				__func__, mod_name, pinmodel.num_meshes);
		return NULL;
	}

	if (pinmodel.num_frames < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect frames count %d",
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

		/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
		num_glcmds += (10 * LittleLong(md3_mesh->num_tris)) + 1;

		if (pinmodel.num_frames != LittleLong(md3_mesh->num_frames))
		{
			R_Printf(PRINT_ALL, "%s: model %s broken mesh %d",
					__func__, mod_name, i);
			return NULL;
		}

		meshofs += LittleLong(md3_mesh->ofs_end);
	}

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

			strncpy(skin, md3_shader->name, MAX_SKINNAME - 1);
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

		for (j = 0; j < LittleLong(md3_mesh->num_tris); j++)
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

		strncpy(frame->name, md3_frameinfo->name, sizeof(frame->name) - 1);
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
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be %ld)",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_ANACHRONOX_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, pinmodel.version, ALIAS_ANACHRONOX_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect skins count %d",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.resolution < 0 || pinmodel.resolution > 2)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect vert type %d",
				__func__, mod_name, pinmodel.resolution);
		return NULL;
	}

	if (pinmodel.num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
				__func__, mod_name);
	}

	if (pinmodel.num_st <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no st vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles",
				__func__, mod_name);
		return NULL;
	}

	if (pinmodel.num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames",
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
	Mod_LoadDTriangleList(pheader, pintri);

	//
	// load the frames
	//
	Mod_LoadFrames_MD2Anox(pheader, (byte *)buffer + pinmodel.ofs_frames,
		pinmodel.framesize, translate, pinmodel.resolution);

	//
	// load the glcmds
	//
	pincmd = (int *)((byte *)buffer + pinmodel.ofs_glcmds);
	Mod_LoadCmdList(mod_name, pheader, pincmd);

	// register all skins
	memcpy((char *)pheader + pheader->ofs_skins, (char *)buffer + pinmodel.ofs_skins,
		pheader->num_skins*MAX_SKINNAME);

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
	const dstvert_t *pinst;
	const int *pincmd;
	dmdl_t pinmodel;
	dmdx_t *pheader;
	dmdxmesh_t *mesh_nodes;
	void *extradata;
	int i, framesize;

	if (modfilelen < sizeof(pinmodel))
	{
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be %ld)",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, pinmodel.version, ALIAS_VERSION);
		return NULL;
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
		return NULL;
	}

	if (pinmodel.num_skins < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect skins count %d",
				__func__, mod_name, pinmodel.num_skins);
		return NULL;
	}

	if (pinmodel.framesize != (
		sizeof(daliasframe_t) + (pinmodel.num_xyz - 1) * sizeof(dtrivertx_t)))
	{
		R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize",
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
		R_Printf(PRINT_ALL, "%s: model %s has no vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
				__func__, mod_name);
	}

	if (pheader->num_st <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no st vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames",
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
	Mod_LoadDTriangleList(pheader, pintri);

	//
	// load the frames
	//
	Mod_LoadFrames_MD2(pheader, (byte *)buffer + pinmodel.ofs_frames,
		pinmodel.framesize, translate);

	//
	// load the glcmds
	//
	pincmd = (int *)((byte *)buffer + pinmodel.ofs_glcmds);
	Mod_LoadCmdList(mod_name, pheader, pincmd);

	/* register all skins */
	memcpy((char *)pheader + pheader->ofs_skins, (char *)buffer + pinmodel.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);

	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}

static void
Mod_LoadSkinList_MD2(const char *mod_name, const void *buffer, int modfilelen,
	char **skins, int *numskins)
{
	dmdl_t pinmodel;
	int i;

	if (modfilelen < sizeof(pinmodel))
	{
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be %ld)",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, pinmodel.version, ALIAS_VERSION);
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
	}

	if (pinmodel.num_skins < 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s file has incorrect skins count %d",
				__func__, mod_name, pinmodel.num_skins);
	}

	*numskins = pinmodel.num_skins;
	*skins = malloc(pinmodel.num_skins * MAX_SKINNAME);

	memcpy(*skins, (char *)buffer + pinmodel.ofs_skins,
		pinmodel.num_skins * MAX_SKINNAME);
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
				R_Printf(PRINT_ALL, "%s: Too short header", __func__);
				return NULL;
			}

			if (version != 2)
			{
				R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
					__func__, blockname, version);
				return NULL;
			}

			inframesize = LittleLong(header->framesize);
			/* has same frame structure */
			if (inframesize < (
				sizeof(daliasframe_t) + (LittleLong(header->num_xyz) - 1) * sizeof(dtrivertx_t)))
			{
				R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize",
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
				R_Printf(PRINT_ALL, "%s: model %s has no vertices",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_xyz > MAX_VERTS)
			{
				R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
						__func__, mod_name);
			}

			if (dmdxheader.num_st <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no st vertices",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_tris <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no triangles",
						__func__, mod_name);
				return NULL;
			}

			if (dmdxheader.num_frames <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no frames",
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
				R_Printf(PRINT_ALL, "%s: %s has broken header.",
					__func__, mod_name);
				return NULL;
			}
			else if (Q_strncasecmp(blockname, "skin", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_skins * MAX_SKINNAME))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}
				memcpy((char*) pheader + pheader->ofs_skins, src, size);
			}
			else if (Q_strncasecmp(blockname, "st coord", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_st * sizeof(dstvert_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadSTvertList(pheader, (dstvert_t *)src);
			}
			else if (Q_strncasecmp(blockname, "tris", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_tris * sizeof(dtriangle_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadDTriangleList (pheader, (dtriangle_t *) src);
			}
			else if (Q_strncasecmp(blockname, "frames", sizeof(blockname)) == 0)
			{
				vec3_t translate = {0, 0, 0};

				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}

				if (size < (pheader->num_frames *
					(sizeof(daliasframe_t) + (pheader->num_xyz - 1) * sizeof(dtrivertx_t))))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadFrames_MD2(pheader, (byte *)src, inframesize, translate);
			}
			else if (Q_strncasecmp(blockname, "glcmds", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_glcmds * sizeof(int)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
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
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				/* 516 mesh node size */
				if (size != (num_mesh_nodes * 516))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
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
									int k, v = in_mesh[j];

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
								int v = in_mesh[j];

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
	dmdxmesh_t *mesh_nodes;
	int i;

	if (sizeof(dkm_header_t) > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small",
				__func__, mod_name, modfilelen);
	}

	/* byte swap the header fields and sanity check */
	for (i=0 ; i<sizeof(dkm_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong(((int *)buffer)[i]);

	if (header.ident != DKMHEADER)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong ident (%i should be %i)",
				__func__, mod_name, header.ident, DKMHEADER);
		return NULL;
	}

	if (header.version != DKM1_VERSION && header.version != DKM2_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, header.version, DKM2_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
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
			R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize",
					__func__, mod_name);
			return NULL;
		}
	}
	else
	{
		if (header.framesize < (
			sizeof(daliasframe_t) + (header.num_xyz - 1) * (sizeof(int) + sizeof(byte))))
		{
			R_Printf(PRINT_ALL, "%s: model %s has incorrect framesize",
					__func__, mod_name);
			return NULL;
		}
	}

	dmdxheader.framesize = sizeof(daliasxframe_t) - sizeof(dxtrivertx_t);
	dmdxheader.framesize += header.num_xyz * sizeof(dxtrivertx_t);

	dmdxheader.num_meshes = 1;
	dmdxheader.num_skins = header.num_skins;
	dmdxheader.num_xyz = header.num_xyz;
	dmdxheader.num_st = header.num_st;
	dmdxheader.num_tris = header.num_tris;
	dmdxheader.num_glcmds = header.num_glcmds;
	dmdxheader.num_frames = header.num_frames;

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

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = pheader->num_tris;
	mesh_nodes[0].ofs_glcmds = 0;
	mesh_nodes[0].num_glcmds = pheader->num_glcmds;

	memcpy((byte*)pheader + pheader->ofs_skins, (byte *)buffer + header.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	Mod_LoadSTvertList(pheader,
		(dstvert_t *)((byte *)buffer + header.ofs_st));
	Mod_LoadDKMCmdList(mod_name, pheader,
		(int *)((byte *)buffer + header.ofs_glcmds));
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

	Mod_LoadDkmTriangleList(pheader,
		(dkmtriangle_t *)((byte *)buffer + header.ofs_tris));

	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

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
		R_Printf(PRINT_ALL, "%s has wrong version number (%i should be %i)",
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

		if (pheader->skinheight > MAX_LBM_HEIGHT)
		{
			R_Printf(PRINT_ALL, "%s: model %s has a skin taller %d than %d\n",
					__func__, mod_name, pheader->skinheight, MAX_LBM_HEIGHT);
		}

		if (pheader->skinwidth > MAX_LBM_HEIGHT)
		{
			R_Printf(PRINT_ALL, "%s: model %s has a skin wider %d than %d\n",
					__func__, mod_name, pheader->skinwidth, MAX_LBM_HEIGHT);
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

static void
Mod_LoadSkinList(const char *mod_name, const void *buffer, int modfilelen,
	char **skins, int *numskins)
{
	switch (LittleLong(*(unsigned *)buffer))
	{
		case IDALIASHEADER:
			Mod_LoadSkinList_MD2(mod_name, buffer, modfilelen,
				skins, numskins);
			break;
	}
}

/*
=================
Mod_LoadModel
=================
*/
void *
Mod_LoadModel(const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s ***skins, int *numskins,
	findimage_t find_image, loadimage_t load_image, modtype_t *type)
{
	void *extradata = NULL;

	/* code needs at least 2 ints for detect file type */
	if (!buffer || modfilelen < (sizeof(unsigned) * 2))
	{
		return NULL;
	}

	switch (LittleLong(*(unsigned *)buffer))
	{
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

		case IDMD5HEADER:
			extradata = Mod_LoadModel_MD5(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;

		case IDSPRITEHEADER:
			extradata = Mod_LoadSprite_SP2(mod_name, buffer, modfilelen,
				skins, numskins, type);
			break;
	}

	if (extradata)
	{
		Mod_LoadMinMaxUpdate(mod_name, mins, maxs, extradata, *type);
		Mod_ReLoadSkins(*skins, find_image, load_image, extradata, *type);
		Mod_LoadLimits(mod_name, extradata, *type);
	}

	return extradata;
}

/* Add md5 to full file name */
static void
Mod_LoadFileInsertMD5(char *newname, const char *oldname, int size)
{
	const char *filename;

	filename = COM_SkipPath(oldname);
	memset(newname, 0, size);
	memcpy(newname, oldname, strlen(oldname) - strlen(filename));
	Q_strlcat(newname, "md5/", size);
	Q_strlcat(newname, filename, size);
}

static int
Mod_LoadFileMD5Merge(const char *namewe, void **buffer)
{
	int fullsize, filesize_anim, filesize, filesize_skins;
	char *final_buffer = NULL, *skins_list = NULL;
	void *anim_buffer = NULL, *skins_buffer = NULL;
	qboolean md5path = false;
	char newname[256];

	/* search mesh file */
	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".md5mesh", sizeof(newname));
	filesize = ri.FS_LoadFile(newname, buffer);

	/* check overwrite file */
	if (filesize <= 0)
	{
		char md5modelname[256];

		Mod_LoadFileInsertMD5(md5modelname, newname, sizeof(md5modelname));

		filesize = ri.FS_LoadFile(md5modelname, buffer);
		/* no replace file */
		if (filesize <= 0)
		{
			return filesize;
		}

		md5path = true;
		strcpy(newname, md5modelname);
	}

	/* search animation file */
	memcpy(newname + strlen(newname) - strlen("mesh"), "anim", strlen("anim"));
	filesize_anim = ri.FS_LoadFile(newname, &anim_buffer);
	if (filesize_anim <= 0)
	{
		ri.FS_FreeFile(*buffer);
		return filesize;
	}

	/* search skins list */
	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".md2", sizeof(newname));
	filesize_skins = ri.FS_LoadFile(newname, &skins_buffer);
	if (filesize_skins > 0)
	{
		char *skins = NULL;
		int numskins = 0, i;

		Mod_LoadSkinList(newname, skins_buffer, filesize_skins,
			&skins, &numskins);
		ri.FS_FreeFile(skins_buffer);

		/*
		 * 20 -> numSkins <num> | skin <num> "MAX_SKINNAME" + md5
		 */
		skins_list = malloc((numskins + 1) * (MAX_SKINNAME + 20));
		sprintf(skins_list, "\nnumSkins %d\n", numskins);
		for(i = 0; i < numskins; i++)
		{
			const char *skinname = skins + MAX_SKINNAME * i;

			if (!md5path)
			{
				sprintf(skins_list + strlen(skins_list), "skin %d \"%s\"\n",
					i, skinname);
			}
			else
			{
				char md5skinname[256];

				Mod_LoadFileInsertMD5(md5skinname, skinname, sizeof(md5skinname));

				sprintf(skins_list + strlen(skins_list), "skin %d \"%s\"\n",
					i, md5skinname);
			}
		}
	}

	/* prepare final file */
	fullsize = filesize + filesize_anim + 1;
	if (skins_list)
	{
		fullsize += strlen(skins_list);
	}

	/* allocate new buffer, ERR_FATAL on alloc fail */
	final_buffer = ri.FS_AllocFile(fullsize);

	/* copy combined information */
	memcpy(final_buffer, *buffer, filesize);
	if (skins_list)
	{
		memcpy(final_buffer + filesize, skins_list, strlen(skins_list));
		filesize += strlen(skins_list);
		free(skins_list);
	}
	final_buffer[filesize] = 0;
	memcpy(final_buffer + filesize + 1, anim_buffer, filesize_anim);

	/* Remove old buffers */
	ri.FS_FreeFile(anim_buffer);
	ri.FS_FreeFile(*buffer);

	*buffer = final_buffer;
	return fullsize;
}

static int
Mod_LoadFileWithoutExt(const char *namewe, void **buffer, const char* ext)
{
	char newname[256];
	size_t tlen;

	*buffer = NULL;

	tlen = strlen(namewe);

	if (!strcmp(ext, "fm") ||
		!strcmp(ext, "dkm") ||
		!strcmp(ext, "md2") ||
		!strcmp(ext, "md3") ||
		!strcmp(ext, "md5mesh") ||
		!strcmp(ext, "mdl"))
	{
		int filesize;

		/* Check ReRelease / Doom 3 / Quake 4 model */
		filesize = Mod_LoadFileMD5Merge(namewe, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as md5 (Doom 3)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake 3 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcpy(newname + tlen, ".md3", sizeof(newname));
		filesize = ri.FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as md3 (Quake 3)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Heretic2 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcat(newname, ".fm", sizeof(newname));
		filesize = ri.FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as fm (Heretic 2)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake 2 model */
		Q_strlcpy(newname + tlen, ".md2", sizeof(newname));
		filesize = ri.FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as md2 (Quake 2/Anachronox)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Daikatana model */
		Q_strlcpy(newname + tlen, ".dkm", sizeof(newname));
		filesize = ri.FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as dkm (Daikatana)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake model */
		Q_strlcpy(newname + tlen, ".mdl", sizeof(newname));
		filesize = ri.FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			R_Printf(PRINT_DEVELOPER, "%s: %s loaded as mdl (Quake 1)\n",
				__func__, namewe);
			return filesize;
		}
	}

	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".", sizeof(newname));
	Q_strlcat(newname, ext, sizeof(newname));

	return ri.FS_LoadFile(newname, buffer);
}

/*
=================
Mod_LoadFile
=================
*/
int
Mod_LoadFile(const char *name, void **buffer)
{
	char namewe[256];
	const char* ext;
	int filesize, len;
	size_t tlen;

	if (!name)
	{
		return -1;
	}

	ext = COM_FileExtension(name);
	if(!ext[0])
	{
		/* file has no extension */
		return -1;
	}

	len = strlen(name);
	if (len < 5)
	{
		return -1;
	}

	/* Remove the extension */
	tlen = len - (strlen(ext) + 1);
	memset(namewe, 0, 256);
	memcpy(namewe, name, tlen);

	filesize = Mod_LoadFileWithoutExt(namewe, buffer, ext);
	if (filesize > 0)
	{
		return filesize;
	}

	/* Replacement of ReRelease models */
	if (!strcmp(namewe, "models/monsters/soldierh/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/soldier/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/gladb/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/gladiatr/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/boss5/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/boss1/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/bitch2/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/bitch/tris",
			buffer, ext);
	}

	return filesize;
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
		int	i;

		pheader = (dmdx_t *)extradata;
		if (pheader->ofs_imgbit && load_image)
		{
			byte* images = (byte *)pheader + pheader->ofs_imgbit;
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
			for (i=0; i < pheader->num_skins; i++)
			{
				skins[i] = find_image((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
			}
		}
		return  pheader->num_frames;
	}

	/* Unknow format, no images associated with it */
	return 0;
}
