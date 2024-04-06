/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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

#include "header/common.h"
#include "header/cmodel.h"
#include "header/flags.h"

/*
=================
Mod_LoadSurfedges

calculate the size that Hunk_Alloc(), called by Mod_Load*() from Mod_LoadBrushModel(),
will use (=> includes its padding), so we'll know how big the hunk needs to be
extra is used for skybox, which adds 6 surfaces
=================
*/
int
Mod_CalcLumpHunkSize(const lump_t *l, int inSize, int outSize, int extra)
{
	if (l->filelen % inSize)
	{
		// Mod_Load*() will error out on this because of "funny size"
		// don't error out here because in Mod_Load*() it can print the functionname
		// (=> tells us what kind of lump) before shutting down the game
		return 0;
	}

	int count = l->filelen / inSize + extra;
	int size = count * outSize;

	// round to cacheline, like Hunk_Alloc() does
	size = (size + 32) & ~31;
	return size;
}

/*
=================
Mod_LoadVisibility
=================
*/
void
Mod_LoadVisibility(const char *name, dvis_t **vis, int *numvisibility,
	const byte *mod_base, const lump_t *l)
{
	dvis_t	*out;
	int	i;

	if (!l->filelen)
	{
		Com_Printf("%s: Map %s has too small visibility lump",
			__func__, name);
		*vis = NULL;
		return;
	}

	*numvisibility = l->filelen;

	out = Hunk_Alloc(l->filelen);
	*vis = out;
	memcpy(out, mod_base + l->fileofs, l->filelen);

	out->numclusters = LittleLong(out->numclusters);

	for (i = 0; i < out->numclusters; i++)
	{
		out->bitofs[i][0] = LittleLong(out->bitofs[i][0]);
		out->bitofs[i][1] = LittleLong(out->bitofs[i][1]);
	}
}

/*
=================
Mod_LoadPlanes

extra is used for skybox, which adds 12 surfaces
=================
*/
void
Mod_LoadPlanes(const char *name, cplane_t **planes, int *numplanes,
	const byte *mod_base, const lump_t *l)
{
	int i;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s has funny lump size",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s with no planes", __func__, name);
	}

	out = Hunk_Alloc((count + EXTRA_LUMP_PLANES) * sizeof(*out));

	*planes = out;
	*numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		int bits, j;

		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
			{
				bits |= 1<<j;
			}
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

static const size_t idbsplumps[HEADER_LUMPS] = {
	sizeof(char), // LUMP_ENTITIES
	sizeof(dplane_t), // LUMP_PLANES
	sizeof(dvertex_t), // LUMP_VERTEXES
	sizeof(char), // LUMP_VISIBILITY
	sizeof(dnode_t), // LUMP_NODES
	sizeof(texinfo_t), // LUMP_TEXINFO
	sizeof(dface_t), // LUMP_FACES
	sizeof(char), // LUMP_LIGHTING
	sizeof(dleaf_t), // LUMP_LEAFS
	sizeof(short), // LUMP_LEAFFACES
	sizeof(short), // LUMP_LEAFBRUSHES
	sizeof(dedge_t), // LUMP_EDGES
	sizeof(int), // LUMP_SURFEDGES
	sizeof(dmodel_t), // LUMP_MODELS
	sizeof(dbrush_t), // LUMP_BRUSHES
	sizeof(dbrushside_t), // LUMP_BRUSHSIDES
	0, // LUMP_POP
	sizeof(darea_t), // LUMP_AREAS
	sizeof(dareaportal_t), // LUMP_AREAPORTALS
};

static const size_t qbsplumps[HEADER_LUMPS] = {
	sizeof(char), // LUMP_ENTITIES
	sizeof(dplane_t), // LUMP_PLANES
	sizeof(dvertex_t), // LUMP_VERTEXES
	sizeof(char), // LUMP_VISIBILITY
	sizeof(dqnode_t), // LUMP_NODES
	sizeof(texinfo_t), // LUMP_TEXINFO
	sizeof(dqface_t), // LUMP_FACES
	sizeof(char), // LUMP_LIGHTING
	sizeof(dqleaf_t), // LUMP_LEAFS
	sizeof(int), // LUMP_LEAFFACES
	sizeof(int), // LUMP_LEAFBRUSHES
	sizeof(dqedge_t), // LUMP_EDGES
	sizeof(int), // LUMP_SURFEDGES
	sizeof(dmodel_t), // LUMP_MODELS
	sizeof(dbrush_t), // LUMP_BRUSHES
	sizeof(dqbrushside_t), // LUMP_BRUSHSIDES
	0, // LUMP_POP
	sizeof(darea_t), // LUMP_AREAS
	sizeof(dareaportal_t), // LUMP_AREAPORTALS
};

static const char*
Mod_MaptypeName(maptype_t maptype)
{
	const char* maptypename;

	switch(maptype)
	{
		case map_quake2: maptypename = "Quake2"; break;
		case map_heretic2: maptypename = "Heretic 2"; break;
		case map_daikatana: maptypename = "Daikatana"; break;
		case map_kingpin: maptypename = "Kingpin"; break;
		case map_anachronox: maptypename = "Anachronox"; break;
		case map_sin: maptypename = "SiN"; break;
		default: maptypename = "Unknown"; break;
	}

	return maptypename;
}

maptype_t
Mod_LoadValidateLumps(const char *name, const dheader_t *header)
{
	const size_t *rules = NULL;
	qboolean error = false;
	maptype_t maptype;

	if ((header->ident == IDBSPHEADER) ||
		(header->ident == RBSPHEADER))
	{
		rules = idbsplumps;
		if (header->version == BSPDKMVERSION)
		{
			maptype = map_daikatana;
		}
		else
		{
			maptype = map_quake2;
		}
	}
	else if (header->ident == QBSPHEADER)
	{
		rules = qbsplumps;
		maptype = map_quake2;
	}
	else
	{
		rules = NULL;
		maptype = map_quake2;
	}

	if (rules)
	{
		int s;
		for (s = 0; s < HEADER_LUMPS; s++)
		{
			if (rules[s])
			{
				if ((maptype == map_daikatana) &&
					(s == LUMP_LEAFS) &&
					(header->lumps[s].filelen % sizeof(ddkleaf_t) == 0))
				{
					/* Small hack for daikatana,
					 * bsp could have two different sizes of LUMP_LEAFS
					 */
					continue;
				}
				else if (header->lumps[s].filelen % rules[s])
				{
					Com_Printf("%s: Map %s lump #%d: incorrect size %d / " YQ2_COM_PRIdS "\n",
						__func__, name, s, header->lumps[s].filelen, rules[s]);
					error = true;
				}
			}
		}
	}

	Com_Printf("Map %s %c%c%c%c with version %d (%s)\n",
				name,
				(header->ident >> 0) & 0xFF,
				(header->ident >> 8) & 0xFF,
				(header->ident >> 16) & 0xFF,
				(header->ident >> 24) & 0xFF,
				header->version, Mod_MaptypeName(maptype));

	if (error)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect lumps",
			__func__, name);
	}

	return maptype;
}

/*
 * Convert Other games flags to Quake 2 surface flags
 */
int
Mod_LoadSurfConvertFlags(int flags, maptype_t maptype)
{
	const int *convert;
	int sflags = 0;
	int i;

	switch (maptype)
	{
		case map_heretic2: convert = heretic2_flags; break;
		case map_daikatana: convert = daikatana_flags; break;
		case map_kingpin: convert = kingpin_flags; break;
		case map_anachronox: convert = anachronox_flags; break;
		default: convert = NULL; break;
	}

	if (!convert)
	{
		return flags;
	}

	for (i = 0; i < 32; i++)
	{
		if (flags & (1 << i))
		{
			sflags |= convert[i];
		}
	}

	return sflags;
}
