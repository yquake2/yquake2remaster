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
			out->normal[j] = in->normal[j];
			if (out->normal[j] < 0)
			{
				bits |= 1<<j;
			}
		}

		out->dist = in->dist;
		out->type = in->type;
		out->signbits = bits;
	}
}

/*
 * Convert Other games flags to Quake 2 flags
 */
static int
Mod_LoadConvertFlags(int flags, const int *convert)
{
	int sflags = 0;
	int i;

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

/*
 * Convert other games flags to Quake 2 surface flags
 */
static int
Mod_LoadSurfConvertFlags(int flags, maptype_t maptype)
{
	const int *convert;

	switch (maptype)
	{
		case map_heretic2: convert = heretic2_flags; break;
		case map_daikatana: convert = daikatana_flags; break;
		case map_kingpin: convert = kingpin_flags; break;
		case map_anachronox: convert = anachronox_flags; break;
		case map_sin: convert = sin_flags; break;
		default: convert = NULL; break;
	}

	return Mod_LoadConvertFlags(flags, convert);
}

/*
 * Convert other games flags to Quake 2 context flags
 */
static int
Mod_LoadContextConvertFlags(int flags, maptype_t maptype)
{
	const int *convert;

	switch (maptype)
	{
		case map_quake2: convert = quake2_contents_flags; break;
		case map_heretic2: convert = heretic2_contents_flags; break;
		case map_daikatana: convert = daikatana_contents_flags; break;
		case map_kingpin: convert = kingpin_contents_flags; break;
		case map_anachronox: convert = anachronox_contents_flags; break;
		case map_sin: convert = sin_contents_flags; break;
		default: convert = NULL; break;
	}

	return Mod_LoadConvertFlags(flags, convert);
}

static void
Mod_Load2QBSP_IBSP_ENTITIES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	memcpy(outbuf + outheader->lumps[LUMP_ENTITIES].fileofs,
		inbuf + inheader->lumps[LUMP_ENTITIES].fileofs,
		inheader->lumps[LUMP_ENTITIES].filelen);
}

static void
Mod_Load2QBSP_IBSP_PLANES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dplane_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_PLANES].filelen / rule_size;
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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dvertex_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_VERTEXES].filelen / rule_size;
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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	memcpy(outbuf + outheader->lumps[LUMP_VISIBILITY].fileofs,
		inbuf + inheader->lumps[LUMP_VISIBILITY].fileofs,
		inheader->lumps[LUMP_VISIBILITY].filelen);
}

static void
Mod_Load2QBSP_IBSP_NODES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dqnode_t *out;
	dnode_t *in;
	int i, count;

	count = inheader->lumps[LUMP_NODES].filelen / rule_size;
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
Mod_Load2QBSP_QBSP_NODES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dqnode_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_NODES].filelen / rule_size;
	in = (dqnode_t *)(inbuf + inheader->lumps[LUMP_NODES].fileofs);
	out = (dqnode_t *)(outbuf + outheader->lumps[LUMP_NODES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
		}

		out->planenum = LittleLong(in->planenum);
		out->firstface = LittleLong(in->firstface) & 0xFFFFFFFF;
		out->numfaces = LittleLong(in->numfaces) & 0xFFFFFFFF;

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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	texinfo_t *in;
	xtexinfo_t *out;
	int i, count;

	count = inheader->lumps[LUMP_TEXINFO].filelen / rule_size;
	in = (texinfo_t *)(inbuf + inheader->lumps[LUMP_TEXINFO].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = Mod_LoadSurfConvertFlags(LittleLong(in->flags), maptype);
		out->nexttexinfo = LittleLong(in->nexttexinfo);
		strncpy(out->texture, in->texture,
			Q_min(sizeof(out->texture), sizeof(in->texture)));

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_RBSP_TEXINFO(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	texrinfo_t *in;
	xtexinfo_t *out;
	int i, count;

	count = inheader->lumps[LUMP_TEXINFO].filelen / rule_size;
	in = (texrinfo_t *)(inbuf + inheader->lumps[LUMP_TEXINFO].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = Mod_LoadSurfConvertFlags(LittleLong(in->flags), maptype);
		out->nexttexinfo = LittleLong(in->nexttexinfo);
		/* TODO: Need to use longer texture path */
		strncpy(out->texture, in->texture,
			Q_min(sizeof(out->texture), sizeof(in->texture)));

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_FACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	dface_t *in;
	dqface_t *out;

	count = inheader->lumps[LUMP_FACES].filelen / rule_size;
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
Mod_Load2QBSP_RBSP_FACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	drface_t *in;
	dqface_t *out;

	count = inheader->lumps[LUMP_FACES].filelen / rule_size;
	in = (drface_t *)(inbuf + inheader->lumps[LUMP_FACES].fileofs);
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
Mod_Load2QBSP_QBSP_FACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	dqface_t *in;
	dqface_t *out;

	count = inheader->lumps[LUMP_FACES].filelen / rule_size;
	in = (dqface_t *)(inbuf + inheader->lumps[LUMP_FACES].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[LUMP_FACES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleLong(in->planenum);
		out->side = LittleLong(in->side);
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleLong(in->numedges);
		out->texinfo = LittleLong(in->texinfo);
		memcpy(out->styles, in->styles, Q_min(sizeof(out->styles), sizeof(in->styles)));
		out->lightofs = LittleLong(in->lightofs);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LIGHTING(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	memcpy(outbuf + outheader->lumps[LUMP_LIGHTING].fileofs,
		inbuf + inheader->lumps[LUMP_LIGHTING].fileofs,
		inheader->lumps[LUMP_LIGHTING].filelen);
}

static void
Mod_Load2QBSP_IBSP_LEAFS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	dleaf_t *in;
	dqleaf_t *out;

	count = inheader->lumps[LUMP_LEAFS].filelen / rule_size;
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

		out->contents = Mod_LoadContextConvertFlags(LittleLong(in->contents), maptype);
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
Mod_Load2QBSP_DKBSP_LEAFS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	ddkleaf_t *in;
	dqleaf_t *out;

	count = inheader->lumps[LUMP_LEAFS].filelen / rule_size;
	in = (ddkleaf_t *)(inbuf + inheader->lumps[LUMP_LEAFS].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[LUMP_LEAFS].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->contents = Mod_LoadContextConvertFlags(LittleLong(in->contents), maptype);
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
Mod_Load2QBSP_QBSP_LEAFS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	dqleaf_t *in;
	dqleaf_t *out;

	count = inheader->lumps[LUMP_LEAFS].filelen / rule_size;
	in = (dqleaf_t *)(inbuf + inheader->lumps[LUMP_LEAFS].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[LUMP_LEAFS].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
		}

		out->contents = Mod_LoadContextConvertFlags(LittleLong(in->contents), maptype);
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);

		/* make unsigned long */
		out->firstleafface = LittleLong(in->firstleafface) & 0xFFFFFFFF;
		out->numleaffaces = LittleLong(in->numleaffaces) & 0xFFFFFFFF;
		out->firstleafbrush = LittleLong(in->firstleafbrush) & 0xFFFFFFFF;
		out->numleafbrushes = LittleLong(in->numleafbrushes) & 0xFFFFFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFFACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	short *in;
	int *out;

	count = inheader->lumps[LUMP_LEAFFACES].filelen / rule_size;
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
Mod_Load2QBSP_QBSP_LEAFFACES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	int *in, *out;

	count = inheader->lumps[LUMP_LEAFFACES].filelen / rule_size;
	in = (int *)(inbuf + inheader->lumps[LUMP_LEAFFACES].fileofs);
	out = (int *)(outbuf + outheader->lumps[LUMP_LEAFFACES].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleLong(*in) & 0xFFFFFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFBRUSHES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	short *in;
	int *out;

	count = inheader->lumps[LUMP_LEAFBRUSHES].filelen / rule_size;
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
Mod_Load2QBSP_QBSP_LEAFBRUSHES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	int *in, *out;

	count = inheader->lumps[LUMP_LEAFBRUSHES].filelen / rule_size;
	in = (int *)(inbuf + inheader->lumps[LUMP_LEAFBRUSHES].fileofs);
	out = (int *)(outbuf + outheader->lumps[LUMP_LEAFBRUSHES].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleLong(*in);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_EDGES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dedge_t *in;
	dqedge_t *out;
	int i, count;

	count = inheader->lumps[LUMP_EDGES].filelen / rule_size;
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
Mod_Load2QBSP_QBSP_EDGES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dqedge_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_EDGES].filelen / rule_size;
	in = (dqedge_t *)(inbuf + inheader->lumps[LUMP_EDGES].fileofs);
	out = (dqedge_t *)(outbuf + outheader->lumps[LUMP_EDGES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->v[0] = (unsigned int)LittleLong(in->v[0]);
		out->v[1] = (unsigned int)LittleLong(in->v[1]);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_SURFEDGES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	int i, count;
	int *in, *out;

	count = inheader->lumps[LUMP_SURFEDGES].filelen / rule_size;
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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dmodel_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_MODELS].filelen / rule_size;
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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dbrush_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHES].filelen / rule_size;
	in = (dbrush_t *)(inbuf + inheader->lumps[LUMP_BRUSHES].fileofs);
	out = (dbrush_t *)(outbuf + outheader->lumps[LUMP_BRUSHES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->firstside = LittleLong(in->firstside) & 0xFFFFFFFF;
		out->numsides = LittleLong(in->numsides) & 0xFFFFFFFF;
		out->contents = Mod_LoadContextConvertFlags(LittleLong(in->contents), maptype);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dbrushside_t *in;
	dqbrushside_t *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHSIDES].filelen / rule_size;
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
Mod_Load2QBSP_RBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	drbrushside_t *in;
	dqbrushside_t *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHSIDES].filelen / rule_size;
	in = (drbrushside_t *)(inbuf + inheader->lumps[LUMP_BRUSHSIDES].fileofs);
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
Mod_Load2QBSP_QBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dqbrushside_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_BRUSHSIDES].filelen / rule_size;
	in = (dqbrushside_t *)(inbuf + inheader->lumps[LUMP_BRUSHSIDES].fileofs);
	out = (dqbrushside_t *)(outbuf + outheader->lumps[LUMP_BRUSHSIDES].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleLong(in->planenum);
		out->texinfo = LittleLong(in->texinfo);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_AREAS(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	darea_t *in, *out;
	int i, count;

	count = inheader->lumps[LUMP_AREAS].filelen / rule_size;
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
	const dheader_t *inheader, size_t rule_size, maptype_t maptype)
{
	dareaportal_t *in, *out;
	int count;

	count = inheader->lumps[LUMP_AREAPORTALS].filelen / rule_size;
	in = (dareaportal_t *)(inbuf + inheader->lumps[LUMP_AREAPORTALS].fileofs);
	out = (dareaportal_t *)(outbuf + outheader->lumps[LUMP_AREAPORTALS].fileofs);
	memcpy(out, in, sizeof(*in) * count);
}

typedef void (*funcrule_t)(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const dheader_t *inheader, const size_t size, maptype_t maptype);

typedef struct
{
	size_t size;
	funcrule_t func;
} rule_t;

static const rule_t idbsplumps[HEADER_LUMPS] = {
	{sizeof(char), Mod_Load2QBSP_IBSP_ENTITIES},
	{sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{sizeof(char), Mod_Load2QBSP_IBSP_VISIBILITY},
	{sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{sizeof(dface_t), Mod_Load2QBSP_IBSP_FACES},
	{sizeof(char), Mod_Load2QBSP_IBSP_LIGHTING},
	{sizeof(dleaf_t), Mod_Load2QBSP_IBSP_LEAFS},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{sizeof(int), Mod_Load2QBSP_IBSP_SURFEDGES},
	{sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{sizeof(dbrushside_t), Mod_Load2QBSP_IBSP_BRUSHSIDES},
	{0, NULL}, // LUMP_POP
	{sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_AREAPORTALS},
};

static const rule_t dkbsplumps[HEADER_LUMPS] = {
	{sizeof(char), Mod_Load2QBSP_IBSP_ENTITIES},
	{sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{sizeof(char), Mod_Load2QBSP_IBSP_VISIBILITY},
	{sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{sizeof(dface_t), Mod_Load2QBSP_IBSP_FACES},
	{sizeof(char), Mod_Load2QBSP_IBSP_LIGHTING},
	{sizeof(ddkleaf_t), Mod_Load2QBSP_DKBSP_LEAFS},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{sizeof(int), Mod_Load2QBSP_IBSP_SURFEDGES},
	{sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{sizeof(dbrushside_t), Mod_Load2QBSP_IBSP_BRUSHSIDES},
	{0, NULL}, // LUMP_POP
	{sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_AREAPORTALS},
};

static const rule_t rbsplumps[HEADER_LUMPS] = {
	{sizeof(char), Mod_Load2QBSP_IBSP_ENTITIES},
	{sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{sizeof(char), Mod_Load2QBSP_IBSP_VISIBILITY},
	{sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{sizeof(texrinfo_t), Mod_Load2QBSP_RBSP_TEXINFO},
	{sizeof(drface_t), Mod_Load2QBSP_RBSP_FACES},
	{sizeof(char), Mod_Load2QBSP_IBSP_LIGHTING},
	{sizeof(dleaf_t), Mod_Load2QBSP_IBSP_LEAFS},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{sizeof(int), Mod_Load2QBSP_IBSP_SURFEDGES},
	{sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{sizeof(drbrushside_t), Mod_Load2QBSP_RBSP_BRUSHSIDES},
	{0, NULL}, // LUMP_POP
	{sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_AREAPORTALS},
};

static const rule_t qbsplumps[HEADER_LUMPS] = {
	{sizeof(char), Mod_Load2QBSP_IBSP_ENTITIES},
	{sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{sizeof(char), Mod_Load2QBSP_IBSP_VISIBILITY},
	{sizeof(dqnode_t), Mod_Load2QBSP_QBSP_NODES},
	{sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{sizeof(dqface_t), Mod_Load2QBSP_QBSP_FACES},
	{sizeof(char), Mod_Load2QBSP_IBSP_LIGHTING},
	{sizeof(dqleaf_t), Mod_Load2QBSP_QBSP_LEAFS},
	{sizeof(int), Mod_Load2QBSP_QBSP_LEAFFACES},
	{sizeof(int), Mod_Load2QBSP_QBSP_LEAFBRUSHES},
	{sizeof(dqedge_t), Mod_Load2QBSP_QBSP_EDGES},
	{sizeof(int), Mod_Load2QBSP_IBSP_SURFEDGES},
	{sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{sizeof(dqbrushside_t), Mod_Load2QBSP_QBSP_BRUSHSIDES},
	{0, NULL}, // LUMP_POP
	{sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_AREAPORTALS},
};

/* custom format with extended texture name */
static const rule_t xbsplumps[HEADER_LUMPS] = {
	{sizeof(char), Mod_Load2QBSP_IBSP_ENTITIES},
	{sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{sizeof(char), Mod_Load2QBSP_IBSP_VISIBILITY},
	{sizeof(dqnode_t), Mod_Load2QBSP_QBSP_NODES},
	{sizeof(xtexinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{sizeof(dqface_t), Mod_Load2QBSP_QBSP_FACES},
	{sizeof(char), Mod_Load2QBSP_IBSP_LIGHTING},
	{sizeof(dqleaf_t), Mod_Load2QBSP_QBSP_LEAFS},
	{sizeof(int), Mod_Load2QBSP_QBSP_LEAFFACES},
	{sizeof(int), Mod_Load2QBSP_QBSP_LEAFBRUSHES},
	{sizeof(dqedge_t), Mod_Load2QBSP_QBSP_EDGES},
	{sizeof(int), Mod_Load2QBSP_IBSP_SURFEDGES},
	{sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{sizeof(dqbrushside_t), Mod_Load2QBSP_QBSP_BRUSHSIDES},
	{0, NULL}, // LUMP_POP
	{sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_AREAPORTALS},
};

static const char*
Mod_MaptypeName(maptype_t maptype)
{
	const char* maptypename;

	switch(maptype)
	{
		case map_quake2rr: maptypename = "Quake2 ReRelease"; break;
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
Mod_LoadGetRules(const dheader_t *header, const rule_t **rules, int *numlumps, int *numrules)
{
	/*
	 * numlumps is count lumps in format,
	 * numrules is what could checked and converted
	 */
	if (header->ident == IDBSPHEADER)
	{
		if (header->version == BSPDKMVERSION)
		{
			/* SiN demos used same version ids as Daikatana */
			if ((header->lumps[LUMP_TEXINFO].filelen % sizeof(texrinfo_t) == 0) &&
				(header->lumps[LUMP_FACES].filelen % sizeof(drface_t) == 0))
			{
				*rules = rbsplumps;
				*numrules = *numlumps = HEADER_LUMPS;
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

				*numrules = HEADER_LUMPS;
				*numlumps = HEADER_DKLUMPS;
				return map_daikatana;
			}
		}
		else if (header->version == BSPVERSION)
		{
			*rules = idbsplumps;
			*numrules = *numlumps = HEADER_LUMPS;
			return map_quake2rr;
		}
	}
	else if (header->ident == QBSPHEADER && header->version == BSPVERSION)
	{
		*rules = qbsplumps;
		*numrules = *numlumps = HEADER_LUMPS;
		return map_quake2rr;
	}
	else if (header->ident == RBSPHEADER && header->version == BSPSINVERSION)
	{
		*rules = rbsplumps;
		*numrules = *numlumps = HEADER_LUMPS;
		return map_sin;
	}

	*rules = NULL;
	return map_quake2rr;
}

byte *
Mod_Load2QBSP(const char *name, byte *inbuf, size_t filesize, size_t *out_len,
	maptype_t *maptype)
{
	/* max lump count * lumps + ident + version */
	int headermem[64];
	const rule_t *rules = NULL;
	size_t result_size;
	dheader_t *header, *outheader;
	int s, xofs, numlumps, numrules;
	qboolean error = false;
	byte *outbuf;
	maptype_t detected_maptype;

	for (s = 0; s < sizeof(headermem) / sizeof(int); s++)
	{
		headermem[s] = LittleLong(((int *)inbuf)[s]);
	}
	header = (dheader_t *)&headermem;

	result_size = sizeof(dheader_t);

	detected_maptype = Mod_LoadGetRules(header, &rules, &numlumps, &numrules);
	if (detected_maptype != map_quake2rr)
	{
		/* Use detected maptype only if for sure know */
		*maptype  = detected_maptype;
	}

	if (rules)
	{
		for (s = 0; s < numrules; s++)
		{
			if (rules[s].size)
			{
				if (header->lumps[s].filelen % rules[s].size)
				{
					Com_Printf("%s: Map %s lump #%d: incorrect size %d / " YQ2_COM_PRIdS "\n",
						__func__, name, s, header->lumps[s].filelen, rules[s].size);
					error = true;
				}

				result_size += (
					xbsplumps[s].size * header->lumps[s].filelen / rules[s].size
				);
			}
		}
	}

	Com_Printf("Map %s %c%c%c%c with version %d (%s)\n",
				name,
				(header->ident >> 0) & 0xFF,
				(header->ident >> 8) & 0xFF,
				(header->ident >> 16) & 0xFF,
				(header->ident >> 24) & 0xFF,
				header->version, Mod_MaptypeName(*maptype));

	if (error || !rules)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect lumps",
			__func__, name);
	}

	/* find end of last lump */
	xofs = 0;
	for (s = 0; s < numlumps; s++)
	{
		xofs = Q_max(xofs,
			(header->lumps[s].fileofs + header->lumps[s].filelen + 3) & ~3);
	}

	if (xofs + sizeof(bspx_header_t) < filesize)
	{
		bspx_header_t* xheader;

		xheader = (bspx_header_t*)(inbuf + xofs);
		if (LittleLong(xheader->ident) == BSPXHEADER)
		{
			result_size += (filesize - xofs);
			result_size += 4;
		}
		else
		{
			/* Have some other data at the end of file, just skip it */
			xofs = filesize;
		}
	}

	outbuf = malloc(result_size);
	outheader = (dheader_t*)outbuf;
	memset(outheader, 0, sizeof(dheader_t));
	outheader->ident = QBSPHEADER;
	outheader->version = BSPVERSION;
	int ofs = sizeof(dheader_t);

	/* mark offsets for all lumps */
	for (s = 0; s < numrules; s++)
	{
		if (rules[s].size)
		{
			outheader->lumps[s].fileofs = ofs;
			outheader->lumps[s].filelen = (
				xbsplumps[s].size * header->lumps[s].filelen / rules[s].size
			);
			ofs += outheader->lumps[s].filelen;
		}
	}

	if (filesize > xofs)
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
		if ((numlumps * sizeof(*lump)) >= (filesize - xofs))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect bspx lumps",
				__func__, name);
		}

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

	/* convert lumps to QBSP for all lumps */
	for (s = 0; s < numrules; s++)
	{
		if (!rules[s].size)
		{
			continue;
		}

		if (!rules[s].func)
		{
			Com_Error(ERR_DROP, "%s: Map %s does not have convert function for %d",
				__func__, name, s);
		}

		rules[s].func(outbuf, outheader, inbuf, header, rules[s].size, *maptype);
	}

	*out_len = result_size;
	return outbuf;
}
