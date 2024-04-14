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

static const size_t dkbsplumps[HEADER_LUMPS] = {
	sizeof(char), // LUMP_ENTITIES
	sizeof(dplane_t), // LUMP_PLANES
	sizeof(dvertex_t), // LUMP_VERTEXES
	sizeof(char), // LUMP_VISIBILITY
	sizeof(dnode_t), // LUMP_NODES
	sizeof(texinfo_t), // LUMP_TEXINFO
	sizeof(dface_t), // LUMP_FACES
	sizeof(char), // LUMP_LIGHTING
	sizeof(ddkleaf_t), // LUMP_LEAFS
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

static const size_t rbsplumps[HEADER_LUMPS] = {
	sizeof(char), // LUMP_ENTITIES
	sizeof(dplane_t), // LUMP_PLANES
	sizeof(dvertex_t), // LUMP_VERTEXES
	sizeof(char), // LUMP_VISIBILITY
	sizeof(dnode_t), // LUMP_NODES
	sizeof(texrinfo_t), // LUMP_TEXINFO
	sizeof(drface_t), // LUMP_FACES
	sizeof(char), // LUMP_LIGHTING
	sizeof(dleaf_t), // LUMP_LEAFS
	sizeof(short), // LUMP_LEAFFACES
	sizeof(short), // LUMP_LEAFBRUSHES
	sizeof(dedge_t), // LUMP_EDGES
	sizeof(int), // LUMP_SURFEDGES
	sizeof(dmodel_t), // LUMP_MODELS
	sizeof(dbrush_t), // LUMP_BRUSHES
	sizeof(drbrushside_t), // LUMP_BRUSHSIDES
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

static maptype_t
Mod_LoadGetRules(const dheader_t *header, const size_t **rules)
{
	if (header->ident == IDBSPHEADER)
	{
		if (header->version == BSPDKMVERSION)
		{
			/* SiN demos used same version ids as Daikatana */
			if ((header->lumps[LUMP_TEXINFO].filelen % sizeof(texrinfo_t) == 0) &&
				(header->lumps[LUMP_FACES].filelen % sizeof(drface_t) == 0))
			{
				*rules = rbsplumps;
				return map_sin;
			}
			else
			{
				if (header->lumps[LUMP_LEAFS].filelen % sizeof(ddkleaf_t) == 0)
				{
					*rules = dkbsplumps;
				}
				else
				{
					*rules = idbsplumps;
				}

				return map_daikatana;
			}
		}
		else if (header->version == BSPVERSION)
		{
			*rules = idbsplumps;
			return map_quake2;
		}
	}
	else if (header->ident == QBSPHEADER && header->version == BSPVERSION)
	{
		*rules = qbsplumps;
		return map_quake2;
	}
	else if (header->ident == RBSPHEADER && header->version == BSPSINVERSION)
	{
		*rules = rbsplumps;
		return map_sin;
	}

	*rules = NULL;
	return map_quake2;
}

maptype_t
Mod_LoadValidateLumps(const char *name, const dheader_t *header)
{
	const size_t *rules = NULL;
	qboolean error = false;
	maptype_t maptype;

	maptype = Mod_LoadGetRules(header, &rules);

	if (rules)
	{
		int s;
		for (s = 0; s < HEADER_LUMPS; s++)
		{
			if (rules[s])
			{
				if (header->lumps[s].filelen % rules[s])
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

	if (error || !rules)
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

static void
Mod_Load2QBSP_IBSP_ENTITIES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader)
{
	memcpy(outbuf + outheader->lumps[LUMP_ENTITIES].fileofs,
		inbuf + inheader->lumps[LUMP_ENTITIES].fileofs,
		inheader->lumps[LUMP_ENTITIES].filelen);
}

static void
Mod_Load2QBSP_IBSP_PLANES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dplane_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_PLANES].filelen / rules[LUMP_PLANES];
	in = (dplane_t *)(inbuf + inheader->lumps[LUMP_PLANES].fileofs);
	out = (dplane_t *)(outbuf + outheader->lumps[LUMP_PLANES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_VERTEXES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dvertex_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_VERTEXES].filelen / rules[LUMP_VERTEXES];
	in = (dvertex_t *)(inbuf + inheader->lumps[LUMP_VERTEXES].fileofs);
	out = (dvertex_t *)(outbuf + outheader->lumps[LUMP_VERTEXES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->point[j] = LittleFloat(in->point[j]);
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_VISIBILITY(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader)
{
	memcpy(outbuf + outheader->lumps[LUMP_VISIBILITY].fileofs,
		inbuf + inheader->lumps[LUMP_VISIBILITY].fileofs,
		inheader->lumps[LUMP_VISIBILITY].filelen);
}

static void
Mod_Load2QBSP_IBSP_NODES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dqnode_t *out;
	dnode_t *in;
	int i, count;

	count = inheader->lumps[LUMP_NODES].filelen / rules[LUMP_NODES];
	in = (dnode_t *)(inbuf + inheader->lumps[LUMP_NODES].fileofs);
	out = (dqnode_t *)(outbuf + outheader->lumps[LUMP_NODES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->planenum = LittleLong(in->planenum);
		out->firstface = LittleShort(in->firstface) & 0xFFFF;
		out->numfaces = LittleShort(in->numfaces) & 0xFFFF;

		for (j = 0; j < 2; j++)
		{
			out->children[j] = LittleLong(in->children[j]);
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_TEXINFO(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	texinfo_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_TEXINFO].filelen / rules[LUMP_TEXINFO];
	in = (texinfo_t *)(inbuf + inheader->lumps[LUMP_TEXINFO].fileofs);
	out = (texinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = LittleLong(in->flags);
		out->nexttexinfo = LittleLong(in->nexttexinfo);
		strncpy(out->texture, in->texture,
			Q_min(sizeof(out->texture), sizeof(in->texture)));

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_FACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	int i, count;
	dface_t *in;
	dqface_t *out;

	count = inheader->lumps[LUMP_FACES].filelen / rules[LUMP_FACES];
	in = (dface_t *)(inbuf + inheader->lumps[LUMP_FACES].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[LUMP_FACES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum);
		out->side = LittleShort(in->side);
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->texinfo = LittleShort(in->texinfo);
		memcpy(out->styles, in->styles, Q_min(sizeof(out->styles), sizeof(in->styles)));
		out->lightofs = LittleLong(in->lightofs);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LIGHTING(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader)
{
	memcpy(outbuf + outheader->lumps[LUMP_LIGHTING].fileofs,
		inbuf + inheader->lumps[LUMP_LIGHTING].fileofs,
		inheader->lumps[LUMP_LIGHTING].filelen);
}

static void
Mod_Load2QBSP_IBSP_LEAFS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	int i, count;
	dleaf_t *in;
	dqleaf_t *out;

	count = inheader->lumps[LUMP_LEAFS].filelen / rules[LUMP_LEAFS];
	in = (dleaf_t *)(inbuf + inheader->lumps[LUMP_LEAFS].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[LUMP_LEAFS].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		/* make unsigned long from signed short */
		out->firstleafface = LittleShort(in->firstleafface) & 0xFFFF;
		out->numleaffaces = LittleShort(in->numleaffaces) & 0xFFFF;
		out->firstleafbrush = LittleShort(in->firstleafbrush) & 0xFFFF;
		out->numleafbrushes = LittleShort(in->numleafbrushes) & 0xFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFFACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	int i, count;
	short *in;
	int *out;

	count = inheader->lumps[LUMP_LEAFFACES].filelen / rules[LUMP_LEAFFACES];
	in = (short *)(inbuf + inheader->lumps[LUMP_LEAFFACES].fileofs);
	out = (int *)(outbuf + outheader->lumps[LUMP_LEAFFACES].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleShort(*in) & 0xFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFBRUSHES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	int i, count;
	short *in;
	int *out;

	count = inheader->lumps[LUMP_LEAFBRUSHES].filelen / rules[LUMP_LEAFBRUSHES];
	in = (short *)(inbuf + inheader->lumps[LUMP_LEAFBRUSHES].fileofs);
	out = (int *)(outbuf + outheader->lumps[LUMP_LEAFBRUSHES].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleShort(*in);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_EDGES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dedge_t *in;
	dqedge_t *out;
	int i, count;

	count = inheader->lumps[LUMP_EDGES].filelen / rules[LUMP_EDGES];
	in = (dedge_t *)(inbuf + inheader->lumps[LUMP_EDGES].fileofs);
	out = (dqedge_t *)(outbuf + outheader->lumps[LUMP_EDGES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_SURFEDGES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	int i, count;
	int *in, *out;

	count = inheader->lumps[LUMP_SURFEDGES].filelen / rules[LUMP_SURFEDGES];
	in = (int *)(inbuf + inheader->lumps[LUMP_SURFEDGES].fileofs);
	out = (int *)(outbuf + outheader->lumps[LUMP_SURFEDGES].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleLong(*in);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_MODELS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dmodel_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_MODELS].filelen / rules[LUMP_MODELS];
	in = (dmodel_t *)(inbuf + inheader->lumps[LUMP_MODELS].fileofs);
	out = (dmodel_t *)(outbuf + outheader->lumps[LUMP_MODELS].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->headnode = LittleLong(in->headnode);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_BRUSHES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dbrush_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHES].filelen / rules[LUMP_BRUSHES];
	in = (dbrush_t *)(inbuf + inheader->lumps[LUMP_BRUSHES].fileofs);
	out = (dbrush_t *)(outbuf + outheader->lumps[LUMP_BRUSHES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->firstside = LittleLong(in->firstside) & 0xFFFFFFFF;
		out->numsides = LittleLong(in->numsides) & 0xFFFFFFFF;
		out->contents = LittleLong(in->contents);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dbrushside_t *in;
	dqbrushside_t *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHSIDES].filelen / rules[LUMP_BRUSHSIDES];
	in = (dbrushside_t *)(inbuf + inheader->lumps[LUMP_BRUSHSIDES].fileofs);
	out = (dqbrushside_t *)(outbuf + outheader->lumps[LUMP_BRUSHSIDES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum);
		out->texinfo = LittleShort(in->texinfo);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_AREAS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	darea_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_AREAS].filelen / rules[LUMP_AREAS];
	in = (darea_t *)(inbuf + inheader->lumps[LUMP_AREAS].fileofs);
	out = (darea_t *)(outbuf + outheader->lumps[LUMP_AREAS].fileofs);

	for (i = 0; i < count; i++)
	{
		out->numareaportals = LittleLong(in->numareaportals);
		out->firstareaportal = LittleLong(in->firstareaportal);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_AREAPORTALS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t *rules)
{
	dareaportal_t *in, *out;
	int count;

	count = inheader->lumps[LUMP_AREAPORTALS].filelen / rules[LUMP_AREAPORTALS];
	in = (dareaportal_t *)(inbuf + inheader->lumps[LUMP_AREAPORTALS].fileofs);
	out = (dareaportal_t *)(outbuf + outheader->lumps[LUMP_AREAPORTALS].fileofs);
	memcpy(out, in, sizeof(*in) * count);
}

byte *
Mod_Load2QBSP(const char *name, byte *inbuf, size_t filesize, size_t *out_len,
	maptype_t *maptype)
{
	const size_t *rules = NULL;
	size_t result_size;
	dheader_t header, *outheader;
	int s, xofs, numlumps;
	qboolean error = false;
	byte *outbuf;

	for (s = 0; s < sizeof(dheader_t) / 4; s++)
	{
		((int *)&header)[s] = LittleLong(((int *)inbuf)[s]);
	}

	result_size = sizeof(dheader_t);

	*maptype = Mod_LoadGetRules(&header, &rules);

	if (rules)
	{
		for (s = 0; s < HEADER_LUMPS; s++)
		{
			if (rules[s])
			{
				if (header.lumps[s].filelen % rules[s])
				{
					Com_Printf("%s: Map %s lump #%d: incorrect size %d / " YQ2_COM_PRIdS "\n",
						__func__, name, s, header.lumps[s].filelen, rules[s]);
					error = true;
				}

				result_size += (
					qbsplumps[s] * header.lumps[s].filelen / rules[s]
				);
			}
		}
	}

	Com_Printf("Map %s %c%c%c%c with version %d (%s)\n",
				name,
				(header.ident >> 0) & 0xFF,
				(header.ident >> 8) & 0xFF,
				(header.ident >> 16) & 0xFF,
				(header.ident >> 24) & 0xFF,
				header.version, Mod_MaptypeName(*maptype));

	if (error || !rules)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect lumps",
			__func__, name);
	}

	/* find end of last lump */
	xofs = 0;

	numlumps = HEADER_LUMPS;
	if ((header.version == BSPDKMVERSION) &&
		(*maptype == map_daikatana))
	{
		numlumps = 21;
	}

	for (s = 0; s < numlumps; s++)
	{
		xofs = Q_max(xofs,
			(header.lumps[s].fileofs + header.lumps[s].filelen + 3) & ~3);
	}

	if (xofs + sizeof(bspx_header_t) < filesize)
	{
		result_size += (filesize - xofs);
	}

	result_size += 4;
	outbuf = malloc(result_size);
	outheader = (dheader_t*)outbuf;
	outheader->ident = QBSPHEADER;
	outheader->version = BSPVERSION;
	int ofs = sizeof(dheader_t);

	/* mark offsets for all lumps */
	for (s = 0; s < HEADER_LUMPS; s++)
	{
		if (rules[s])
		{
			outheader->lumps[s].fileofs = ofs;
			outheader->lumps[s].filelen = (
				qbsplumps[s] * header.lumps[s].filelen / rules[s]
			);
			ofs += outheader->lumps[s].filelen;
		}
	}

	if ((filesize - xofs) > 0)
	{
		bspx_header_t *bspx_header;
		bspx_lump_t *lump;
		int numlumps, i;

		ofs = ((ofs + 3) & ~3);
		bspx_header = (bspx_header_t *)(outbuf + ofs);

		/* copy BSPX */
		memcpy(bspx_header, inbuf + xofs, (filesize - xofs));

		/* fix positions */
		numlumps = LittleLong(bspx_header->numlumps);

		lump = (bspx_lump_t*)(bspx_header + 1);
		for (i = 0; i < numlumps; i++, lump++)
		{
			int fileofs;

			/* move fileofs to correct place */
			fileofs = LittleLong(lump->fileofs);
			fileofs += (ofs - xofs);
			lump->fileofs = LittleLong(fileofs);
		}
	}

	Mod_Load2QBSP_IBSP_ENTITIES(outbuf, outheader, inbuf, &header);
	Mod_Load2QBSP_IBSP_PLANES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_VERTEXES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_VISIBILITY(outbuf, outheader, inbuf, &header);
	Mod_Load2QBSP_IBSP_NODES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_TEXINFO(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_FACES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_LIGHTING(outbuf, outheader, inbuf, &header);
	Mod_Load2QBSP_IBSP_LEAFS(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_LEAFFACES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_LEAFBRUSHES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_EDGES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_SURFEDGES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_MODELS(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_BRUSHES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_BRUSHSIDES(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_AREAS(outbuf, outheader, inbuf, &header, rules);
	Mod_Load2QBSP_IBSP_AREAPORTALS(outbuf, outheader, inbuf, &header, rules);

	*out_len = result_size;
	return outbuf;
}
