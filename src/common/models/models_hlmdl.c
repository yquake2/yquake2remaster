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

/*
=================
Mod_LoadModel_HLMDL
=================
*/
void *
Mod_LoadModel_HLMDL(const char *mod_name, const void *buffer, int modfilelen)
{
	const hlmdl_header_t pinmodel;
	hlmdl_framegroup_t *seqgroups;
	dmdx_t dmdxheader, *pheader;
	hlmdl_texture_t *in_skins;
	dmdxmesh_t *mesh_nodes, *mesh_tmp;
	void *extradata;
	const hlmdl_sequence_t *sequences;
	size_t i, framesize;
	int total_frames;
	hlmdl_bodypart_t *bodyparts;
	dstvert_t *st_tmp = NULL;
	dtriangle_t *tri_tmp = NULL;
	int num_st = 0, st_size = 0;
	int num_tris = 0, tri_size = 0;

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
	seqgroups = (hlmdl_framegroup_t *)((byte *)buffer + pinmodel.ofs_seqgroup);
	for (i = 0; i < pinmodel.num_seqgroups; i++)
	{
		Com_Printf("%s: %s: Seqgroup  %s: %s\n",
			__func__, mod_name, seqgroups[i].label, seqgroups[i].name);
	}

	bodyparts = (hlmdl_bodypart_t *)((byte *)buffer + pinmodel.ofs_bodyparts);
	mesh_tmp = calloc(pinmodel.num_bodyparts, sizeof(*mesh_tmp));
	for (i = 0; i < pinmodel.num_bodyparts; i++)
	{
		hlmdl_bodymodel_t *bodymodels;
		int j;

		/* TODO: convert submodels to additional meshes? */
		mesh_tmp[i].ofs_tris = num_tris;

		Com_Printf("%s: %s: Bodypart %s: nummodels %d, base %d\n",
			__func__, mod_name, bodyparts[i].name, bodyparts[i].num_models,
			bodyparts[i].base);

		bodymodels = (hlmdl_bodymodel_t *)((byte *)buffer + bodyparts[i].ofs_model);
		for (j = 0; j < bodyparts[i].num_models; j++)
		{
			hlmdl_bodymesh_t *mesh_nodes;
			vec3_t *in_verts;
			int k;

			Com_Printf("%s: %s: body part '%s' model '%s' mesh %d\n",
				__func__, mod_name, bodyparts[i].name, bodymodels[j].name,
				bodymodels[j].num_mesh);

			mesh_nodes = (hlmdl_bodymesh_t *)((byte *)buffer + bodymodels[j].ofs_mesh);
			for (k = 0; k < bodymodels[j].num_mesh; k++)
			{
				short *trivert;
				int l;

				Com_Printf("%s: %s: mesh #%d tris %d, ofs: %d, skin: %d, norms: %d\n",
					__func__, mod_name, k, mesh_nodes[k].num_tris,
						mesh_nodes[k].ofs_tris, mesh_nodes[k].skinref, mesh_nodes[k].num_norms);

				trivert = (short *)((byte *)buffer + mesh_nodes[k].ofs_tris);
				while ((l = *(trivert++)))
				{
					int g, count = l, st_prefix = num_st;
					int *verts = NULL;

					Com_Printf("%s: %s: tris %d\n",
						__func__, mod_name, l);

					if (count < 0)
					{
						count = -count;
					}

					verts = malloc(count * sizeof(*verts));

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
						st_tmp[num_st].s = trivert[2] * 256;
						st_tmp[num_st].t = trivert[3] * 256;
						num_st++;

						verts[g] = trivert[0];

						Com_Printf("%s: %s: tris #%d vert: %d, norm: %d, s: %d, t: %d\n",
							__func__, mod_name, l,
							trivert[0], trivert[1],
							trivert[2], trivert[3]);
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
			for (k = 0; k < bodymodels[j].num_verts; k++)
			{
				Com_Printf("%s: vert[%03ld]: %.2fx%.2fx%.2fx\n",
					__func__, i, in_verts[k][0], in_verts[k][1], in_verts[k][2]);
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
	framesize += pinmodel.num_bones * sizeof(dxtrivertx_t);

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = 256;
	dmdxheader.skinheight = 256;
	dmdxheader.framesize = framesize;

	dmdxheader.num_meshes = pinmodel.num_bodyparts;
	dmdxheader.num_skins = pinmodel.num_skins;
	dmdxheader.num_xyz = pinmodel.num_bones;
	dmdxheader.num_st = num_st;
	dmdxheader.num_tris = num_tris;
	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	dmdxheader.num_glcmds = (10 * num_tris) + 1 * pinmodel.num_bodyparts;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_frames = 0; /* total_frames; */
	dmdxheader.num_animgroup = pinmodel.num_seq;

	Com_DPrintf("%s: %s has %d frames\n",
		__func__, mod_name, total_frames);

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

	in_skins = (hlmdl_texture_t *)((byte *)buffer + pinmodel.ofs_texture);
	for (i = 0; i < pinmodel.num_skins; i++)
	{
		char *skin;

		skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;

		Q_strlcpy(skin, in_skins[i].name, MAX_SKINNAME);

		Com_Printf("%s: Skin %s: %d %dx%d\n",
			__func__, in_skins[i].name, in_skins[i].offset, in_skins[i].width, in_skins[i].height);
	}

	Mod_LoadHLMDLAnimGroupList(pheader, sequences, pinmodel.num_seq);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
