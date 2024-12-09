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
		case map_quake2: convert = quake2_flags; break;
		default: convert = NULL; break;
	}

	return Mod_LoadConvertFlags(flags, convert);
}

/*
 * Convert other games flags to Quake 2 material name
 */
static void
Mod_LoadMaterialConvertFlags(int flags, maptype_t maptype, char *value)
{
	const char **material = NULL;
	int i;

	if (maptype == map_heretic2)
	{
		const char *surface_materials[] = {
			"gravel",
			"metal",
			"stone",
			"wood",
		};

		flags = (flags >> 24) & 0x3;

		strcpy(value, surface_materials[flags]);
		return;
	}
	else if (maptype == map_sin)
	{
		const char *surface_materials[] = {
			"",
			"wood",
			"metal",
			"stone",
			"concrete",
			"dirt",
			"flesh",
			"grill",
			"glass",
			"fabric",
			"monitor",
			"gravel",
			"vegetation",
			"paper",
			"dust",
			"water",
		};

		flags = (flags >> 27) & 0xf;

		strcpy(value, surface_materials[flags]);
		return;
	}

	switch (maptype)
	{
		case map_anachronox: material = anachronox_material; break;
		case map_daikatana: material = daikatana_material; break;
		case map_kingpin: material = kingpin_material; break;
		default: break;
	}

	if (!material)
	{
		return;
	}

	for (i = 0; i < 32; i++)
	{
		if (flags & (1 << i) && material[i])
		{
			strcpy(value, material[i]);
			return;
		}
	}
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
Mod_Load2QBSP_IBSP_Copy(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	memcpy(outbuf + outheader->lumps[outlumppos].fileofs,
		inbuf + lumps[inlumppos].fileofs,
		lumps[inlumppos].filelen);
}

static void
Mod_Load2QBSP_IBSP_CopyLong(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	int *in, *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (int *)(inbuf + lumps[inlumppos].fileofs);
	out = (int *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleLong(*in);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_PLANES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dplane_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dplane_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dplane_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_IBSP_VERTEXES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dvertex_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dvertex_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dvertex_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_IBSP_NODES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dqnode_t *out;
	dnode_t *in;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dnode_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqnode_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->planenum = LittleLong(in->planenum) & 0xFFFFFFFF;
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
Mod_Load2QBSP_QBSP_NODES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dqnode_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dqnode_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqnode_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
		}

		out->planenum = LittleLong(in->planenum) & 0xFFFFFFFF;
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
Mod_Load2QBSP_MATERIALS_TEXINFO(xtexinfo_t *out, int count)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (out->texture[0])
		{
			char material_path[80];
			byte *raw;
			int len;

			snprintf(material_path, sizeof(material_path), "textures/%s.mat", out->texture);

			/* load the file */
			len = FS_LoadFile(material_path, (void **)&raw);
			if (len > 0)
			{
				int j;

				j = Q_min(sizeof(out->material) - 1, len);
				memcpy(out->material, raw, j);
				out->material[j] = 0;

				FS_FreeFile(raw);
			}
		}

		out ++;
	}
}

static void
Mod_Load2QBSP_IBSP_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	texinfo_t *in;
	xtexinfo_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (texinfo_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j, inflags;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		inflags = LittleLong(in->flags);
		out->flags = Mod_LoadSurfConvertFlags(inflags, maptype);
		out->nexttexinfo = LittleLong(in->nexttexinfo);
		memset(out->material, 0, sizeof(out->material));
		Mod_LoadMaterialConvertFlags(inflags, maptype, out->material);
		strncpy(out->texture, in->texture,
			Q_min(sizeof(out->texture), sizeof(in->texture)));

		/* Fix backslashes */
		Q_replacebackslash(out->texture);

		out++;
		in++;
	}

	Mod_Load2QBSP_MATERIALS_TEXINFO(
		(xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs), count);
}

static void
Mod_Load2QBSP_RBSP_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	texrinfo_t *in;
	xtexinfo_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (texrinfo_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j, inflags;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		inflags = LittleLong(in->flags);
		out->flags = Mod_LoadSurfConvertFlags(inflags, maptype);
		out->nexttexinfo = LittleLong(in->nexttexinfo);
		memset(out->material, 0, sizeof(out->material));
		Mod_LoadMaterialConvertFlags(inflags, maptype, out->material);
		strncpy(out->texture, in->texture,
			Q_min(sizeof(out->texture), sizeof(in->texture)));

		/* Fix backslashes */
		Q_replacebackslash(out->texture);

		out++;
		in++;
	}

	Mod_Load2QBSP_MATERIALS_TEXINFO(
		(xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs), count);
}

static void
Mod_Load2QBSP_IBSP_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	dface_t *in;
	dqface_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dface_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum) & 0xFFFF;
		out->side = LittleShort(in->side) & 0xFFFF;
		out->firstedge = LittleLong(in->firstedge) & 0xFFFFFFFF;
		out->numedges = LittleShort(in->numedges) & 0xFFFF;
		out->texinfo = LittleShort(in->texinfo);
		memcpy(out->styles, in->styles, Q_min(sizeof(out->styles), sizeof(in->styles)));
		out->lightofs = LittleLong(in->lightofs);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_RBSP_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	drface_t *in;
	dqface_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (drface_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum) & 0xFFFF;
		out->side = LittleShort(in->side) & 0xFFFF;
		out->firstedge = LittleLong(in->firstedge) & 0xFFFFFFFF;
		out->numedges = LittleShort(in->numedges) & 0xFFFF;
		out->texinfo = LittleShort(in->texinfo);
		memcpy(out->styles, in->styles, Q_min(sizeof(out->styles), sizeof(in->styles)));
		out->lightofs = LittleLong(in->lightofs);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_QBSP_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	dqface_t *in;
	dqface_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dqface_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleLong(in->planenum) & 0xFFFFFFFF;
		out->side = LittleLong(in->side) & 0xFFFFFFFF;
		out->firstedge = LittleLong(in->firstedge) & 0xFFFFFFFF;
		out->numedges = LittleLong(in->numedges) & 0xFFFFFFFF;
		out->texinfo = LittleLong(in->texinfo);
		memcpy(out->styles, in->styles, Q_min(sizeof(out->styles), sizeof(in->styles)));
		out->lightofs = LittleLong(in->lightofs);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	dleaf_t *in;
	dqleaf_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dleaf_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_DKBSP_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	ddkleaf_t *in;
	dqleaf_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (ddkleaf_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_QBSP_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	dqleaf_t *in;
	dqleaf_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dqleaf_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_IBSP_LEAFFACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	short *in;
	int *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (short *)(inbuf + lumps[inlumppos].fileofs);
	out = (int *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleShort(*in) & 0xFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_QBSP_LEAFFACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	int *in, *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (int *)(inbuf + lumps[inlumppos].fileofs);
	out = (int *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleLong(*in) & 0xFFFFFFFF;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFBRUSHES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count;
	short *in;
	int *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (short *)(inbuf + lumps[inlumppos].fileofs);
	out = (int *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		*out = LittleShort(*in);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_EDGES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dedge_t *in;
	dqedge_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dedge_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqedge_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_QBSP_EDGES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dqedge_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dqedge_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqedge_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->v[0] = (unsigned int)LittleLong(in->v[0]);
		out->v[1] = (unsigned int)LittleLong(in->v[1]);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_MODELS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dmodel_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dmodel_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dmodel_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_IBSP_BRUSHES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dbrush_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dbrush_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dbrush_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
Mod_Load2QBSP_IBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dbrushside_t *in;
	dqbrushside_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dbrushside_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqbrushside_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum) & 0xFFFF;
		out->texinfo = LittleShort(in->texinfo);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_RBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	drbrushside_t *in;
	dqbrushside_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (drbrushside_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqbrushside_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum) & 0xFFFF;
		out->texinfo = LittleShort(in->texinfo);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_QBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	dqbrushside_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dqbrushside_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqbrushside_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleLong(in->planenum) & 0xFFFFFFFF;
		out->texinfo = LittleLong(in->texinfo);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_AREAS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	darea_t *in, *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (darea_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (darea_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->numareaportals = LittleLong(in->numareaportals);
		out->firstareaportal = LittleLong(in->firstareaportal);

		out++;
		in++;
	}
}

typedef void (*funcrule_t)(byte *outbuf, dheader_t *outheader, const byte *inbuf,
	const lump_t *lumps, const size_t size, maptype_t maptype,
	int outlumppos, int inlumppos);

typedef struct
{
	int pos; /* Lump position in relation to Q2 in final size */
	size_t size;
	funcrule_t func;
} rule_t;

static const rule_t idq2bsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{LUMP_TEXINFO, sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{LUMP_FACES, sizeof(dface_t), Mod_Load2QBSP_IBSP_FACES},
	{LUMP_LIGHTING, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_LEAFS, sizeof(dleaf_t), Mod_Load2QBSP_IBSP_LEAFS},
	{LUMP_LEAFFACES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{LUMP_LEAFBRUSHES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{LUMP_EDGES, sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{LUMP_BRUSHES, sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(dbrushside_t), Mod_Load2QBSP_IBSP_BRUSHSIDES},
	{-1, 0, NULL}, // LUMP_POP
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

static const rule_t dkbsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{LUMP_TEXINFO, sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{LUMP_FACES, sizeof(dface_t), Mod_Load2QBSP_IBSP_FACES},
	{LUMP_LIGHTING, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_LEAFS, sizeof(ddkleaf_t), Mod_Load2QBSP_DKBSP_LEAFS},
	{LUMP_LEAFFACES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{LUMP_LEAFBRUSHES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{LUMP_EDGES, sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{LUMP_BRUSHES, sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(dbrushside_t), Mod_Load2QBSP_IBSP_BRUSHSIDES},
	{-1, 0, NULL}, // LUMP_POP
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

static const rule_t rbsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dnode_t), Mod_Load2QBSP_IBSP_NODES},
	{LUMP_TEXINFO, sizeof(texrinfo_t), Mod_Load2QBSP_RBSP_TEXINFO},
	{LUMP_FACES, sizeof(drface_t), Mod_Load2QBSP_RBSP_FACES},
	{LUMP_LIGHTING, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_LEAFS, sizeof(dleaf_t), Mod_Load2QBSP_IBSP_LEAFS},
	{LUMP_LEAFFACES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{LUMP_LEAFBRUSHES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFBRUSHES},
	{LUMP_EDGES, sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{LUMP_BRUSHES, sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(drbrushside_t), Mod_Load2QBSP_RBSP_BRUSHSIDES},
	{-1, 0, NULL}, // LUMP_POP
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

static const rule_t qbsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dqnode_t), Mod_Load2QBSP_QBSP_NODES},
	{LUMP_TEXINFO, sizeof(texinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{LUMP_FACES, sizeof(dqface_t), Mod_Load2QBSP_QBSP_FACES},
	{LUMP_LIGHTING, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_LEAFS, sizeof(dqleaf_t), Mod_Load2QBSP_QBSP_LEAFS},
	{LUMP_LEAFFACES, sizeof(int), Mod_Load2QBSP_QBSP_LEAFFACES},
	{LUMP_LEAFBRUSHES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_EDGES, sizeof(dqedge_t), Mod_Load2QBSP_QBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{LUMP_BRUSHES, sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(dqbrushside_t), Mod_Load2QBSP_QBSP_BRUSHSIDES},
	{-1, 0, NULL}, // LUMP_POP
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

/* custom format with extended texture name */
static const rule_t xbsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dqnode_t), Mod_Load2QBSP_QBSP_NODES},
	{LUMP_TEXINFO, sizeof(xtexinfo_t), Mod_Load2QBSP_IBSP_TEXINFO},
	{LUMP_FACES, sizeof(dqface_t), Mod_Load2QBSP_QBSP_FACES},
	{LUMP_LIGHTING, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_LEAFS, sizeof(dqleaf_t), Mod_Load2QBSP_QBSP_LEAFS},
	{LUMP_LEAFFACES, sizeof(int), Mod_Load2QBSP_QBSP_LEAFFACES},
	{LUMP_LEAFBRUSHES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_EDGES, sizeof(dqedge_t), Mod_Load2QBSP_QBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dmodel_t), Mod_Load2QBSP_IBSP_MODELS},
	{LUMP_BRUSHES, sizeof(dbrush_t), Mod_Load2QBSP_IBSP_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(dqbrushside_t), Mod_Load2QBSP_QBSP_BRUSHSIDES},
	{-1, 0, NULL}, // LUMP_POP
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
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
Mod_LoadGetRules(int ident, int version, const lump_t *lumps, const rule_t **rules,
	int *numlumps, int *numrules)
{
	/*
	 * numlumps is count lumps in format,
	 * numrules is what could checked and converted
	 */
	if (ident == IDBSPHEADER)
	{
		if (version == BSPDKMVERSION)
		{
			/* SiN demos used same version ids as Daikatana */
			if ((lumps[LUMP_TEXINFO].filelen % sizeof(texrinfo_t) == 0) &&
				(lumps[LUMP_FACES].filelen % sizeof(drface_t) == 0))
			{
				*rules = rbsplumps;
				*numrules = *numlumps = HEADER_LUMPS;
				return map_sin;
			}
			else
			{
				if (lumps[LUMP_LEAFS].filelen % sizeof(ddkleaf_t) == 0)
				{
					*rules = dkbsplumps;
				}
				else
				{
					*rules = idq2bsplumps;
				}

				*numrules = HEADER_LUMPS;
				*numlumps = HEADER_DKLUMPS;
				return map_daikatana;
			}
		}
		else if (version == BSPVERSION)
		{
			*rules = idq2bsplumps;
			*numrules = *numlumps = HEADER_LUMPS;
			return map_quake2rr;
		}
	}
	else if (ident == QBSPHEADER && version == BSPVERSION)
	{
		*rules = qbsplumps;
		*numrules = *numlumps = HEADER_LUMPS;
		return map_quake2rr;
	}
	else if (ident == RBSPHEADER && version == BSPSINVERSION)
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
	int lumpsmem[64];
	const rule_t *rules = NULL;
	size_t result_size;
	dheader_t *outheader;
	lump_t *lumps;
	int s, xofs, numlumps, numrules;
	qboolean error = false;
	byte *outbuf;
	maptype_t detected_maptype;
	int ident, version;
	int *inlumps;

	ident = LittleLong(((int *)inbuf)[0]);
	version = LittleLong(((int *)inbuf)[1]);
	inlumps = (int*)inbuf + 2; /* skip ident + version */
	if (ident == BSPQ1VERSION)
	{
		ident = IDBSPHEADER;
		version = BSPQ1VERSION;
		inlumps = (int*)inbuf + 1; /* version */
	}

	for (s = 0; s < sizeof(lumpsmem) / sizeof(int); s++)
	{
		lumpsmem[s] = LittleLong(((int *)inlumps)[s]);
	}
	lumps = (lump_t *)&lumpsmem;

	result_size = sizeof(dheader_t);

	detected_maptype = Mod_LoadGetRules(ident, version, lumps,
		&rules, &numlumps, &numrules);
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
				if (lumps[s].filelen % rules[s].size)
				{
					Com_Printf("%s: Map %s lump #%d: incorrect size %d / " YQ2_COM_PRIdS "\n",
						__func__, name, s, lumps[s].filelen, rules[s].size);
					error = true;
				}

				if ((lumps[s].fileofs + lumps[s].filelen) > filesize)
				{
					Com_Printf("%s: Map %s lump #%d: incorrect size %d or offset %d\n",
						__func__, name, s, lumps[s].fileofs, lumps[s].filelen);
					error = true;
				}

				if (rules[s].pos >= 0)
				{
					result_size += (
						xbsplumps[rules[s].pos].size * lumps[s].filelen / rules[s].size
					);
				}
			}
		}
	}

	Com_Printf("Map %s %c%c%c%c with version %d (%s): " YQ2_COM_PRIdS " bytes\n",
				name,
				(ident >> 0) & 0xFF,
				(ident >> 8) & 0xFF,
				(ident >> 16) & 0xFF,
				(ident >> 24) & 0xFF,
				version, Mod_MaptypeName(*maptype),
				result_size);

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
			(lumps[s].fileofs + lumps[s].filelen + 3) & ~3);
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
		if (rules[s].size && (rules[s].pos >= 0))
		{
			int pos;

			pos = rules[s].pos;
			outheader->lumps[pos].fileofs = ofs;
			outheader->lumps[pos].filelen = (
				xbsplumps[pos].size * lumps[s].filelen / rules[s].size
			);
			ofs += outheader->lumps[pos].filelen;
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

		rules[s].func(outbuf, outheader, inbuf, lumps, rules[s].size, *maptype,
			rules[s].pos, s);
	}

	*out_len = result_size;
	return outbuf;
}

