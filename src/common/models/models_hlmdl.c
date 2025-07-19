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
	const hlmdl_sequence_t *sequences;
	void *extradata;
	size_t i, num_tris, framesize;
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
	dmdxheader.num_xyz = 0;
	dmdxheader.num_st = 0;
	dmdxheader.num_tris = num_tris;
	dmdxheader.num_glcmds = 0;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_frames = pinmodel.num_seq;  /* Each sequence becomes a frame */
	dmdxheader.num_animgroup = 0;

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
	sequences = (const hlmdl_sequence_t *)((const byte *)buffer + pinmodel.ofs_seq);
	for (i = 0; i < pinmodel.num_seq; i++)
	{
		daliasxframe_t *frame = (daliasxframe_t *)((byte *)pheader +
			pheader->ofs_frames + i * pheader->framesize);

		Com_Printf("%s: %s, Sequence '%s': %d frames\n",
			__func__, mod_name, sequences[i].name, sequences[i].num_frames);

		/* Set frame name from sequence */
		Q_strlcpy(frame->name, sequences[i].name, sizeof(frame->name));

		/* Use standard scale/translation for now */
		VectorSet(frame->scale, 1.0f, 1.0f, 1.0f);
		VectorSet(frame->translate, 0.f, 0.f, 0.f);

		/* Convert bone positions to vertices */
		for (int j = 0; j < pinmodel.num_bones; j++)
		{
			dxtrivertx_t *vert = &frame->verts[j];
			const hlmdl_bone_t *bone = &bones[j];

			/* Initialize vertex from bone position */
			vec3_t pos = {0};

			/* Get base bone position */
			for (int k = 0; k < 3; k++)
			{
				pos[k] = bone->value[k];

				/* Apply bone controller scaling */
				if (bone->scale[k] != 0)
				{
					pos[k] *= bone->scale[k];
				}
			}

			/* Apply parent bone transformations */
			if (bone->parent >= 0)
			{
				const hlmdl_bone_t *parent = &bones[bone->parent];
				vec3_t parentPos;

				/* Get parent position */
				VectorSet(parentPos,
					parent->value[0],
					parent->value[1],
					parent->value[2]);


				/* Add parent offset */
				VectorAdd(pos, parentPos, pos);
			}

			/* Convert to 8-bit vertex coords */
			vert->v[0] = (byte)((pos[0] / 255.0f) * 255);
			vert->v[1] = (byte)((pos[1] / 255.0f) * 255);
			vert->v[2] = (byte)((pos[2] / 255.0f) * 255);

			/* Set normal to point up */
			memset(vert->normal, 0, sizeof(vert->normal));
		}
	}

	Mod_LoadAnimGroupList(pheader);

	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
