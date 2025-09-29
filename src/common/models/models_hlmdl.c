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

		Com_DPrintf("%s: Skin %s: %dx%d\n",
			__func__, in_skins[i].name, width, height);

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
	hlmdl_framegroup_t *seqgroups;
	dmdx_t dmdxheader, *pheader;
	dmdxmesh_t *mesh_nodes, *mesh_tmp;
	const hlmdl_bone_t *bones;
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

	Mod_LoadHLMDLSkinsSize(&pinmodel, buffer, &skinw, &skinh);

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
			int *in_boneids;
			int k;

			Com_Printf("%s: %s: body part '%s' model '%s' mesh %d\n",
				__func__, mod_name, bodyparts[i].name, bodymodels[j].name,
				bodymodels[j].num_mesh);

			mesh_nodes = (hlmdl_bodymesh_t *)((byte *)buffer + bodymodels[j].ofs_mesh);
			for (k = 0; k < bodymodels[j].num_mesh; k++)
			{
				short *trivert;
				int l;

				/*
				Com_Printf("%s: %s: mesh #%d tris %d, ofs: %d, skin: %d, norms: %d\n",
					__func__, mod_name, k, mesh_nodes[k].num_tris,
						mesh_nodes[k].ofs_tris, mesh_nodes[k].skinref, mesh_nodes[k].num_norms);
				*/

				trivert = (short *)((byte *)buffer + mesh_nodes[k].ofs_tris);
				while ((l = *(trivert++)))
				{
					int g, count = l, st_prefix = num_st;
					int *verts = NULL;

					/*
					Com_Printf("%s: %s: tris %d\n",
						__func__, mod_name, l);
					*/

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

						/*
						Com_Printf("%s: %s: tris #%d vert: %d, norm: %d, s: %d, t: %d\n",
							__func__, mod_name, l,
							trivert[0], trivert[1],
							trivert[2], trivert[3]);
						*/
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
			in_boneids = (int *)((byte *)buffer + bodymodels[j].ofs_vert);
			if (!out_vert || !in_boneids ||
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
				int l;

				for (l = 0; l < 3; l++)
				{
					out_vert[num_verts].xyz[l] = LittleFloat(in_verts[k][l]);
				}

				out_boneids[num_verts] = LittleLong(in_boneids[k]);

				num_verts++;

				/*
				 Com_Printf("%s: vert[%03ld]: %.2fx%.2fx%.2fx\n",
					__func__, k,
					in_verts[k][0], in_verts[k][1], in_verts[k][2]);
				*/
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

	total_frames = 0;
	for (i = 0; i < pinmodel.num_seq; i++)
	{
		int j;

		for(j = 0; j < sequences[i].num_frames; j ++)
		{
			daliasxframe_t *frame = (daliasxframe_t *)(
				(byte *)pheader + pheader->ofs_frames + total_frames * pheader->framesize);

			/* limit frame ids to 2**16 */
			snprintf(frame->name, sizeof(frame->name), "%s%d",
				sequences[i].name, j % 0xFF);

			PrepareFrameVertex(out_vert, num_verts, frame);
			total_frames++;
		}
	}
	free(out_vert);
	free(out_boneids);

	Mod_LoadHLMDLSkins(mod_name, pheader, &pinmodel, buffer);

	/* Get bones/sequences */
	bones = (const hlmdl_bone_t *)((const byte *)buffer + pinmodel.ofs_bones);
	int frame_idx = 0;
	for (i = 0; i < pinmodel.num_seq; i++)
	{
		const hlmdl_sequence_t *seq = &sequences[i];
		for (int f = 0; f < seq->num_frames; f++, frame_idx++)
		{
			daliasxframe_t *frame = (daliasxframe_t *)((byte *)pheader +
				pheader->ofs_frames + frame_idx * pheader->framesize);

			/* Name frame as sequenceName_frameIdx */
			snprintf(frame->name, sizeof(frame->name), "%s_%d", seq->name, f);

			Com_Printf("%s: %s, Sequence '%s': frame %d/%d\n",
				__func__, mod_name, seq->name, f, seq->num_frames);

			/* Use standard scale/translation for now */
			VectorSet(frame->scale, 1.0f, 1.0f, 1.0f);
			VectorSet(frame->translate, 0.f, 0.f, 0.f);

			/* Convert bone positions to vertices (same as before) */
			for (int j = 0; j < pinmodel.num_bones; j++)
			{
				dxtrivertx_t *vert = &frame->verts[j];
				const hlmdl_bone_t *bone = &bones[j];

				vec3_t pos = {0};
				vec3_t rotation = {0};
				vec3_t final_pos = {0};

				for (int k = 0; k < 3; k++)
				{
					pos[k] = bone->value[k];
					rotation[k] = bone->value[k + 3];
					if (bone->scale[k] != 0)
						pos[k] *= bone->scale[k];
					if (bone->scale[k + 3] != 0)
						rotation[k] *= bone->scale[k + 3];
				}

				if (bone->parent >= 0)
				{
					vec3_t parent_pos = {0};
					const hlmdl_bone_t *parent = &bones[bone->parent];
					for (int k = 0; k < 3; k++)
					{
						parent_pos[k] = parent->value[k];
						if (parent->scale[k] != 0)
						{
							parent_pos[k] *= parent->scale[k];
						}
					}

					float cr = cos(parent->value[3]);
					float sr = sin(parent->value[3]);
					float cp = cos(parent->value[4]);
					float sp = sin(parent->value[4]);
					float cy = cos(parent->value[5]);
					float sy = sin(parent->value[5]);

					vec3_t rotated_pos;

					/* Rotate around X */
					rotated_pos[0] = pos[0];
					rotated_pos[1] = pos[1] * cr - pos[2] * sr;
					rotated_pos[2] = pos[1] * sr + pos[2] * cr;

					/* Rotate around Y */
					pos[0] = rotated_pos[0] * cp + rotated_pos[2] * sp;
					pos[1] = rotated_pos[1];
					pos[2] = -rotated_pos[0] * sp + rotated_pos[2] * cp;

					/* Rotate around Z */
					rotated_pos[0] = pos[0] * cy - pos[1] * sy;
					rotated_pos[1] = pos[0] * sy + pos[1] * cy;
					rotated_pos[2] = pos[2];

					/* Add parent position */
					VectorAdd(rotated_pos, parent_pos, final_pos);
				}
				else
				{
					VectorCopy(pos, final_pos);
				}

				/* Convert to normalized 8-bit vertex coords */
				float scale = 64.0f; /* Scale factor to fit model size */
				for (int k = 0; k < 3; k++)
				{
					float normalized = (final_pos[k] / scale + 1.0f) * 127.5f;
					vert->v[k] = (byte)normalized;
				}

				/* Calculate vertex normal from bone rotation */
				float cr = cos(rotation[0]);
				float sr = sin(rotation[0]);
				float cp = cos(rotation[1]);
				float sp = sin(rotation[1]);
				float cy = cos(rotation[2]);
				float sy = sin(rotation[2]);

				/* Default normal points up (0,0,1) transformed by bone rotation */
				vec3_t normal = {0, 0, 1};
				vec3_t temp;

				/* Apply rotations in order: Y, P, R */
				/* Yaw */
				temp[0] = normal[0] * cy - normal[1] * sy;
				temp[1] = normal[0] * sy + normal[1] * cy;
				temp[2] = normal[2];

				/* Pitch */
				normal[0] = temp[0] * cp + temp[2] * sp;
				normal[1] = temp[1];
				normal[2] = -temp[0] * sp + temp[2] * cp;

				/* Roll */
				temp[0] = normal[0];
				temp[1] = normal[1] * cr - normal[2] * sr;
				temp[2] = normal[1] * sr + normal[2] * cr;

				/* Store normalized normal vector */
				VectorNormalize(temp);
				for (int k = 0; k < 3; k++)
				{
					vert->normal[k] = (byte)((temp[k] + 1.0f) * 127.5f);
				}
			}
		}
	}

	Mod_LoadHLMDLAnimGroupList(pheader, sequences, pinmodel.num_seq);
	Mod_LoadFixNormals(pheader);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
