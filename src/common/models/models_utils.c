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
 * The common model file format
 *
 * =======================================================================
 */

#include "models.h"

static const float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

/* compressed vertex normals used by mdl and md2 model formats */
void
Mod_ConvertNormalMDL(byte in_normal, signed char *normal)
{
	const float *norm;
	int n;

	norm = r_avertexnormals[in_normal % NUMVERTEXNORMALS];

	for (n = 0; n < 3; n ++)
	{
		normal[n] = norm[n] * 127.f;
	}
}

/*
=================
Mod_LoadAnimGroupList

Generate animations groups from frames
=================
*/
void
Mod_LoadAnimGroupList(dmdx_t *pheader)
{
	char newname[16] = {0}, oldname[16] = {0};
	int i, oldframe = 0, currgroup = 0;
	dmdxframegroup_t *pframegroup;

	pframegroup = (dmdxframegroup_t *)((char *)pheader + pheader->ofs_animgroup);

	for (i = 0; i < pheader->num_frames; i++)
	{
		daliasxframe_t *poutframe;

		poutframe = (daliasxframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		if (poutframe->name[0])
		{
			size_t j;

			Q_strlcpy(newname, poutframe->name, sizeof(poutframe->name));
			for (j = strlen(newname) - 1; j > 0; j--)
			{
				if ((newname[j] >= '0' && newname[j] <= '9') ||
					(newname[j] == '_'))
				{
					newname[j] = 0;
					continue;
				}
				break;
			}
		}
		else
		{
			*newname = 0;
		}

		if (strcmp(newname, oldname))
		{
			if ((i != oldframe) && (currgroup < pheader->num_animgroup))
			{
				Q_strlcpy(pframegroup[currgroup].name, oldname,
					sizeof(pframegroup[currgroup].name));
				pframegroup[currgroup].ofs = oldframe;
				pframegroup[currgroup].num = i - oldframe;
				currgroup++;
			}
			strcpy(oldname, newname);
			oldframe = i;
		}
	}

	if (currgroup < pheader->num_animgroup)
	{
		Q_strlcpy(pframegroup[currgroup].name, oldname,
			sizeof(pframegroup[currgroup].name));
		pframegroup[currgroup].ofs = oldframe;
		pframegroup[currgroup].num = i - oldframe;
		currgroup++;
	}

	pheader->num_animgroup = currgroup;

	for (i = 0; i < pheader->num_animgroup; i++)
	{
		Mod_UpdateMinMaxByFrames(pheader,
			pframegroup[i].ofs, pframegroup[i].ofs + pframegroup[i].num,
			pframegroup[i].mins, pframegroup[i].maxs);
	}
}

/*
 * verts as bytes
 */
void
Mod_LoadFrames_VertMD2(dxtrivertx_t *vert, const byte *in)
{
	int k;

	for (k=0; k < 3; k++)
	{
		vert->v[k] = in[k] * 0xFF;
	}
}

/*
 * Calculate offsets and allocate memory for model
 */
dmdx_t *
Mod_LoadAllocate(const char *mod_name, dmdx_t *dmdxheader, void **extradata)
{
	dmdx_t *pheader = NULL;

	dmdxheader->ident = IDALIASHEADER;
	dmdxheader->version = 0;
	/* just skip header */
	dmdxheader->ofs_meshes = sizeof(dmdx_t);
	dmdxheader->ofs_skins = dmdxheader->ofs_meshes + dmdxheader->num_meshes * sizeof(dmdxmesh_t);
	dmdxheader->ofs_st = dmdxheader->ofs_skins + dmdxheader->num_skins * MAX_SKINNAME;
	dmdxheader->ofs_tris = dmdxheader->ofs_st + dmdxheader->num_st * sizeof(dstvert_t);
	dmdxheader->ofs_frames = dmdxheader->ofs_tris + dmdxheader->num_tris * sizeof(dtriangle_t);
	dmdxheader->ofs_glcmds = dmdxheader->ofs_frames + dmdxheader->num_frames * dmdxheader->framesize;
	dmdxheader->ofs_imgbit = dmdxheader->ofs_glcmds + dmdxheader->num_glcmds * sizeof(int);
	dmdxheader->ofs_animgroup = dmdxheader->ofs_imgbit + (
		dmdxheader->skinwidth * dmdxheader->skinheight *
		dmdxheader->num_skins * dmdxheader->num_imgbit / 8
	);
	dmdxheader->ofs_end = dmdxheader->ofs_animgroup + dmdxheader->num_animgroup * sizeof(dmdxframegroup_t);

	*extradata = Hunk_Begin(dmdxheader->ofs_end);
	pheader = Hunk_Alloc(dmdxheader->ofs_end);

	memcpy(pheader, dmdxheader, sizeof(dmdx_t));

	return pheader;
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

		/* fix skin backslash */
		Q_replacebackslash(skin);

		Com_DPrintf("%s: %s #%d: Should load %s '%s'\n",
			__func__, mod_name, i, internal ? "internal": "external", skin);
	}

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
