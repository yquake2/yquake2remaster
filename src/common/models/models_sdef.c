/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * The SiN models file formats
 *
 * =======================================================================
 */

#include "models.h"

/*
=================
Mod_LoadFrames_SAM

Load the SiN sam animation format frames
=================
*/
static void
Mod_LoadFrames_SAM(dmdx_t *pheader, sin_sam_header_t **anims, def_entry_t *animations,
	int anim_num, vec3_t translate, float scale)
{
	dmdxframegroup_t *framegroups;
	int curr_frame = 0, i;

	framegroups = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);

	for (i = 0; i < anim_num; i++)
	{
		sin_frame_t *pinframe;
		int k;

		/* create single animation group by default*/
		Q_strlcpy(framegroups[i].name, animations[i].name,
			sizeof(framegroups[i].name));
		framegroups[i].ofs = curr_frame;
		framegroups[i].num = anims[i]->num_frames;

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
				Mod_ConvertNormalMDL(verts[j].lightnormalindex,
					poutframe->verts[j].normal);
			}

			curr_frame ++;
		}

		Mod_UpdateMinMaxByFrames(pheader,
			framegroups[i].ofs, framegroups[i].ofs + framegroups[i].num,
			framegroups[i].mins, framegroups[i].maxs);
	}
}

static const namesconvert_t sdef_names[] = {
	{"crouch_death", "crdeath"},
	{"crouch_idle", "crstnd"},
	{"crouch_pain", "crpain"},
	{"death", "death"},
	{"down_idle", "idle"},
	{"fire", "attack"},
	{"idle", "idle"},
	{"jump", "jump"},
	{"melee", "melee"},
	{"pain", "pain"},
	{"run", "run"},
	{"step", "walk"},
	{"twitch", "idle"},
	{"up_idle", "idle"},
	{"walk", "walk"},
	{NULL, NULL}
};

static void *
Mod_LoadModel_SDEF_Text(const char *mod_name, char *curr_buff)
{
	char models_path[MAX_QPATH];
	char base_model[MAX_QPATH * 2] = {0};
	def_entry_t *skinnames = NULL;
	def_entry_t *animations = NULL;
	int actions_num, skinnames_num, i, base_size = -1;
	dmdx_t dmdxheader, *pheader = NULL;
	sin_sbm_header_t *base;
	sin_sam_header_t **anim;
	void *extradata = NULL;
	vec3_t translate = {0, 0, 0};
	float scale;
	int framescount = 0;
	int animation_num = 0;
	int num_tris = 0;
	sin_trigroup_t *trigroup;
	char prevvalue[MAX_SKINNAME] = {0};

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
			Q_strlcpy(models_path, COM_Parse(&curr_buff), sizeof(models_path));
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
				prevvalue[0] = 0;
			}
			else if (!Q_stricmp(ext, "sam"))
			{
				def_entry_t *tmp;

				actions_num ++;
				tmp = realloc(animations, actions_num * sizeof(def_entry_t));
				YQ2_COM_CHECK_OOM(tmp, "realloc()", actions_num * sizeof(def_entry_t))
				if (!tmp)
				{
					actions_num --;
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					break;
				}

				animations = tmp;
				snprintf(animations[actions_num - 1].value,
					sizeof(animations[actions_num - 1].value),
					"%s/%s", models_path, token);
				if (prevvalue[0])
				{
					Q_strlcpy(animations[actions_num - 1].name, prevvalue,
						sizeof(animations[actions_num - 1].name));
				}
				else
				{
					sprintf(animations[actions_num - 1].name,
						"optional%d", actions_num);
				}
				prevvalue[0] = 0;
			}
			else if (!Q_stricmp(ext, "tga"))
			{
				def_entry_t *tmp;

				skinnames_num ++;
				tmp = realloc(skinnames, skinnames_num * sizeof(def_entry_t));
				YQ2_COM_CHECK_OOM(tmp, "realloc()", skinnames_num * sizeof(def_entry_t))
				if (!tmp)
				{
					skinnames_num --;
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					break;
				}
				skinnames = tmp;
				snprintf(skinnames[skinnames_num - 1].value,
					sizeof(skinnames[skinnames_num - 1].value),
					"%s/%s", models_path, token);
				if (prevvalue[0])
				{
					Q_strlcpy(skinnames[skinnames_num - 1].name, prevvalue,
						sizeof(skinnames[skinnames_num - 1].name));
				}
				else
				{
					sprintf(skinnames[skinnames_num - 1].name,
						"optional%d", skinnames_num);
				}
				prevvalue[0] = 0;
			}
			else
			{
				Q_strlcpy(prevvalue, token, sizeof(prevvalue));
			}
		}
	}

	if (*base_model)
	{
		base_size = FS_LoadFile(base_model, (void **)&base);
	}

	if (base_size <= 0)
	{
		Com_DPrintf("%s: %s No base model for %s\n",
			__func__, mod_name, base_model);

		if (skinnames)
		{
			free(skinnames);
		}

		if (animations)
		{
			free(animations);
		}

		return NULL;
	}

	Mod_LittleHeader((int *)base, sizeof(sin_sbm_header_t) / sizeof(int),
		(int *)base);

	if ((base->ident != SBMHEADER) || (base->version != MDSINVERSION))
	{
		FS_FreeFile(base);
		free(skinnames);
		free(animations);
		Com_DPrintf("%s: %s, Incorrect ident or version for %s\n",
			__func__, mod_name, base_model);
		return NULL;
	}

	trigroup = (sin_trigroup_t *)((char*)base + sizeof(sin_sbm_header_t));

	for(i = 0; i < base->num_groups; i ++)
	{
		num_tris += LittleLong(trigroup[i].num_tris);
	}

	anim = malloc(actions_num * sizeof(*anim));
	if (!anim)
	{
		FS_FreeFile(base);
		free(skinnames);
		free(animations);
		YQ2_COM_CHECK_OOM(anim, "malloc()", actions_num * sizeof(*anim))
		return NULL;
	}

	for (i = 0; i < actions_num; i++)
	{
		int anim_size;

		anim_size = FS_LoadFile(animations[i].value, (void **)&anim[animation_num]);
		if (anim_size <= 0)
		{
			Com_DPrintf("%s: %s empty animation %s\n",
				__func__, mod_name, animations[i].value);
			continue;
		}

		if (anim[animation_num]->num_xyz != base->num_xyz)
		{
			Com_DPrintf("%s: %s, incorrect count tris in animation in %s\n",
				__func__, mod_name, animations[i].value);
			continue;
		}

		Mod_LittleHeader((int *)anim[animation_num], sizeof(sin_sam_header_t) / sizeof(int),
			(int *)anim[animation_num]);

		if ((anim[animation_num]->ident != SAMHEADER) ||
			(anim[animation_num]->version != MDSINVERSION))
		{
			FS_FreeFile(base);
			Com_DPrintf("%s: %s, Incorrect ident or version for %s\n",
				__func__, mod_name, base_model);
			return NULL;
		}

		framescount += anim[animation_num]->num_frames;
		animation_num ++;
	}

	if (!animation_num)
	{
		Com_DPrintf("%s: %s no animation found\n",
			__func__, mod_name);
		free(anim);
		free(skinnames);
		free(animations);
		FS_FreeFile(base);
		return NULL;
	}

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
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = animation_num;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	for (i = 0; i < pheader->num_skins; i ++)
	{
		Q_strlcpy((char*)pheader + pheader->ofs_skins +  i * MAX_SKINNAME,
			skinnames[i].value, MAX_SKINNAME);
	}

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

	Mod_LoadFrames_SAM(pheader, anim, animations, animation_num, translate, scale);
	Mod_LoadModel_AnimGroupNamesFix(pheader, sdef_names);
	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);

	for (i = 0; i < animation_num; i++)
	{
		FS_FreeFile(anim[i]);
	}

	free(anim);
	free(skinnames);
	free(animations);

	FS_FreeFile(base);
	return extradata;
}

void *
Mod_LoadModel_SDEF(const char *mod_name, const void *buffer, int modfilelen)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1);
	if (!text)
	{
		YQ2_COM_CHECK_OOM(text, "malloc()", modfilelen + 1)
		return NULL;
	}

	memcpy(text, buffer, modfilelen);
	text[modfilelen] = 0;
	extradata = Mod_LoadModel_SDEF_Text(mod_name, text);
	free(text);

	return extradata;
}
