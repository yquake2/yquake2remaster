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
	dmdxmesh_t *mesh_nodes;
	const hlmdl_bone_t *bones;
	void *extradata;
	const hlmdl_sequence_t *sequences;
	size_t i, num_tris, total_frames, framesize;
	hlmdl_bodypart_t *bodyparts;

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
	for (i = 0; i < pinmodel.num_bodyparts; i++)
	{
		hlmdl_bodymodel_t *bodymodels;
		int j;

		Com_Printf("%s: %s: Bodypart %s: nummodels %d, base %d\n",
			__func__, mod_name, bodyparts[i].name, bodyparts[i].num_models,
			bodyparts[i].base);

		bodymodels = (hlmdl_bodymodel_t *)((byte *)buffer + bodyparts[i].ofs_model);
		for (j = 0; j < bodyparts[i].num_models; j++)
		{
			hlmdl_bodymesh_t *mesh_nodes;
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
					Com_Printf("%s: %s: tris %d\n",
						__func__, mod_name, l);

					if (l < 0)
					{
						l = -l;
					}

					for (; l > 0; l--, trivert += 4)
					{
						Com_Printf("%s: %s: tris #%d vert: %d, norm: %d, s: %d, t: %d\n",
							__func__, mod_name, l,
							trivert[0], trivert[1],
							trivert[2], trivert[3]);
					}
				}
			}
		}
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

	num_tris = 0;

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = 0;
	dmdxheader.skinheight = 0;
	dmdxheader.framesize = framesize;

	dmdxheader.num_meshes = 0;
	dmdxheader.num_skins = pinmodel.num_skins;
	dmdxheader.num_xyz = pinmodel.num_bones;
	dmdxheader.num_st = 0;
	dmdxheader.num_tris = num_tris;
	dmdxheader.num_glcmds = 0;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_frames = total_frames;
	dmdxheader.num_animgroup = pinmodel.num_seq;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	/* create single mesh */
	mesh_nodes = (dmdxmesh_t *)((char *)pheader + pheader->ofs_meshes);
	mesh_nodes[0].ofs_tris = 0;
	mesh_nodes[0].num_tris = num_tris;

	in_skins = (hlmdl_texture_t *)((byte *)buffer + pinmodel.ofs_texture);
	for (i = 0; i < pinmodel.num_skins; i++)
	{
		char *skin;

		skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;

		Q_strlcpy(skin, in_skins[i].name, MAX_SKINNAME);

		Com_Printf("%s: Skin %s: %d %dx%d\n",
			__func__, in_skins[i].name, in_skins[i].offset, in_skins[i].width, in_skins[i].height);
	}

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
	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
