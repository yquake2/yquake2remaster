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

#include "../ref_shared.h"

static void
Mod_LoadLimits(const char *mod_name, void *extradata, modtype_t type)
{
	if (type == mod_alias)
	{
		dmdxframegroup_t *framegroups;
		dmdxmesh_t *mesh_nodes;
		int i, num_glcmds = 0;
		dmdx_t *pheader;

		pheader = (dmdx_t *)extradata;

		if (pheader->skinheight > MAX_LBM_WIDTH)
		{
			Com_Printf("%s: model %s has a skin taller %d than %d\n",
					__func__, mod_name, pheader->skinheight, MAX_LBM_WIDTH);
		}

		if (pheader->skinwidth > MAX_LBM_WIDTH)
		{
			Com_Printf("%s: model %s has a skin wider %d than %d\n",
					__func__, mod_name, pheader->skinwidth, MAX_LBM_WIDTH);
		}

		mesh_nodes = (dmdxmesh_t *)((char*)pheader + pheader->ofs_meshes);
		for (i = 0; i < pheader->num_meshes; i++)
		{
			Com_DPrintf("%s: model %s mesh #%d: %d commands, %d tris\n",
				__func__, mod_name, i, mesh_nodes[i].num_glcmds, mesh_nodes[i].num_tris);
			num_glcmds += mesh_nodes[i].num_glcmds;
		}

		framegroups = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);
		for (i = 0; i < pheader->num_animgroup; i++)
		{
			Com_DPrintf(
				"%s: model %s animation group #%d: '%s' %d:%d, box %.2fx%.2fx%.2f:%.2fx%.2fx%.2f\n",
				__func__, mod_name, i, framegroups[i].name, framegroups[i].ofs,
				framegroups[i].num,
				framegroups[i].mins[0], framegroups[i].mins[1], framegroups[i].mins[2],
				framegroups[i].maxs[0], framegroups[i].maxs[1], framegroups[i].maxs[2]);
		}

		Com_DPrintf(
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
		const dmdx_t *pheader;

		pheader = (dmdx_t *)extradata;
		if (!pheader->num_frames)
		{
			return;
		}

		Mod_UpdateMinMaxByFrames(pheader, 0, pheader->num_frames, mins, maxs);

		Com_DPrintf("Model %s box: %.1fx%.1fx%.1f -> %.1fx%.1fx%.1f\n",
			mod_name,
			mins[0], mins[1], mins[2],
			maxs[0], maxs[1], maxs[2]);
	}
}

static void
Mod_AllocateSkins(const char *mod_name, struct image_s ***skins, int *numskins,
	void *extradata, modtype_t type)
{
	if (type == mod_sprite)
	{
		dsprite_t	*sprout;

		sprout = (dsprite_t *)extradata;

		*numskins = sprout->numframes;
		*skins = malloc(Q_max(*numskins, 1) * sizeof(struct image_s *));
	}
	else if (type == mod_alias)
	{
		const dmdx_t *pheader;

		pheader = (dmdx_t *)extradata;

		*numskins = pheader->num_skins;
		*skins = malloc(Q_max(*numskins, 1) * sizeof(struct image_s *));
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
	findimage_t find_image, modtype_t *type)
{
	int ident;

	/* code needs at least 2 ints for detect file type */
	if (!buffer || modfilelen < (sizeof(unsigned) * 2))
	{
		return NULL;
	}

	ident = LittleLong(*(unsigned *)buffer);

	if (ident == IDALIASHEADER || ident == IDSPRITEHEADER)
	{
		void *extradata = NULL;
		char *data;

		extradata = Hunk_Begin(modfilelen);
		data = Hunk_Alloc(modfilelen);
		memcpy(data, buffer, modfilelen);

		switch (ident)
		{
			case IDALIASHEADER: *type = mod_alias; break;
			case IDSPRITEHEADER: *type = mod_sprite; break;
		}

		Mod_AllocateSkins(mod_name, skins, numskins, extradata, *type);
		Mod_LoadMinMaxUpdate(mod_name, mins, maxs, extradata, *type);
		Mod_ReLoadSkins(mod_name, *skins, find_image, extradata, *type);
		/* should use data, in other case compiler could skip memcpy
		 * for optimizaed code */
		Mod_LoadLimits(mod_name, data, *type);

		return extradata;
	}

	return NULL;
}

/*
=================
Mod_ReLoad

Reload images in SP2/MD2 (mark registration_sequence)
=================
*/
int
Mod_ReLoadSkins(const char *name, struct image_s **skins, findimage_t find_image,
	void *extradata, modtype_t type)
{
	if (type == mod_sprite)
	{
		dsprite_t	*sprout;
		int	i;

		sprout = (dsprite_t *)extradata;
		for (i = 0; i < sprout->numframes; i++)
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
		int i;

		pheader = (dmdx_t *)extradata;

		for (i = 0; i < pheader->num_skins; i++)
		{
			char *skin;

			skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;
			skins[i] = find_image(skin, it_skin);

			if (!skins[i] && !strchr(skin, '/') && !strchr(skin, '\\'))
			{
				char skin_path[MAX_QPATH * 2] = {0};

				Q_strlcpy(skin_path, name, sizeof(skin_path));
				strcpy(strrchr(skin_path, '/') + 1, skin);

				Com_DPrintf("Model %s: No original skin found, %s is used\n",
					name, skin_path);
				skins[i] = find_image(skin_path, it_skin);
			}
		}

		return  pheader->num_frames;
	}

	/* Unknow format, no images associated with it */
	return 0;
}
