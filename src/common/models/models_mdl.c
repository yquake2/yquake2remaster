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
 * The MDL models file format
 *
 * =======================================================================
 */

#include "models.h"

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

static const byte *
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
void *
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

	Mod_LoadAnimGroupList(pheader, true);
	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, true);

	return extradata;
}
