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
		case map_quake1:
			/* fall through */
		case map_hexen2:
			/* fall through */
		case map_halflife1:
			return flags == 1 ? SURF_WARP: 0;
		case map_heretic2: convert = heretic2_flags; break;
		case map_daikatana: convert = daikatana_flags; break;
		case map_kingpin: convert = kingpin_flags; break;
		case map_anachronox: convert = anachronox_flags; break;
		case map_sin: convert = sin_flags; break;
		case map_quake2: convert = quake2_flags; break;
		case map_quake3: convert = quake3_flags; break;
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
 * Convert Quake 1 games flags to Quake 2 content flags
 */
static int
ModLoadContentQuake1(int flags)
{
	switch (flags)
	{
		case CONTENTS_Q1_EMPTY: return 0;
		case CONTENTS_Q1_SOLID: return CONTENTS_SOLID;
		case CONTENTS_Q1_WATER: return CONTENTS_WATER;
		case CONTENTS_Q1_SLIME: return CONTENTS_SLIME;
		case CONTENTS_Q1_LAVA: return CONTENTS_LAVA;
		case CONTENTS_Q1_SKY: return 0;
		case CONTENTS_Q1_CLIP: return CONTENTS_SOLID;
		case CONTENTS_Q1_ORIGIN: return CONTENTS_ORIGIN;
		case CONTENTS_Q1_CURRENT_0: return CONTENTS_CURRENT_0;
		case CONTENTS_Q1_CURRENT_90: return CONTENTS_CURRENT_90;
		case CONTENTS_Q1_CURRENT_180: return CONTENTS_CURRENT_180;
		case CONTENTS_Q1_CURRENT_270: return CONTENTS_CURRENT_270;
		case CONTENTS_Q1_CURRENT_UP: return CONTENTS_CURRENT_UP;
		case CONTENTS_Q1_CURRENT_DOWN: return CONTENTS_CURRENT_DOWN;
		case CONTENTS_HL_TRANSLUCENT: return CONTENTS_TRANSLUCENT;
		case CONTENTS_HL_LADDER: return CONTENTS_LADDER;
		/* Ignored:
		 * CONTENTS_HL_FLYFIELD,
		 * CONTENTS_HL_GRAVITY_FLYFIELD,
		 * CONTENTS_HL_FOG. */
		default: return 0;
	}
}

/*
 * Convert other games flags to Quake 2 content flags
 */
static int
Mod_LoadContentConvertFlags(int flags, maptype_t maptype)
{
	const int *convert;

	switch (maptype)
	{
		case map_quake1:
			/* fall through */
		case map_hexen2:
			/* fall through */
		case map_halflife1:
			return ModLoadContentQuake1(flags);
		case map_quake2: convert = quake2_contents_flags; break;
		case map_quake3: convert = quake3_contents_flags; break;
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
	size_t i, count;
	const int *in;
	int *out;

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
	const dplane_t *in;
	dplane_t *out;
	size_t i, count;

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
Mod_Load2QBSP_IBSP46_PLANES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dq3plane_t *in;
	dplane_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq3plane_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dplane_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}

		out->dist = LittleFloat(in->dist);
		out->type = PLANE_ANYZ; /* calculate distance each time */

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_VERTEXES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dvertex_t *in;
	dvertex_t *out;
	size_t i, count;

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
	const dnode_t *in;
	size_t i, count;

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
Mod_Load2QBSP_IBSP29_NODES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	dqnode_t *out;
	const dq1node_t *in;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq1node_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqnode_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

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
			out->children[j] = LittleShort(in->children[j]);
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
	const dqnode_t *in;
	dqnode_t *out;
	size_t i, count;

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
Mod_Load2QBSP_IBSP46_NODES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dq3node_t *in;
	dqnode_t *out;
	size_t i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq3node_t *)(inbuf + lumps[inlumppos].fileofs);
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
		out->firstface = 0;
		out->numfaces = 0;

		for (j = 0; j < 2; j++)
		{
			out->children[j] = LittleLong(in->children[j]);
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_MATERIALS_TEXINFO(xtexinfo_t *out, size_t count)
{
	size_t i;

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

/*
 * Only bspx maps in ReRelease has SURF_NODRAW correctly set,
 * other maps could by set flag by mistake or some custom unknown logic
 */
static void
Mod_Load2QBSP_TEXINFO_NOBSPX(byte *outbuf, dheader_t *outheader)
{
	xtexinfo_t *out;
	size_t i, count;

	count = outheader->lumps[LUMP_TEXINFO].filelen / sizeof(xtexinfo_t);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);
	for (i = 0; i < count; i++)
	{
		out->flags &= ~SURF_NODRAW;
		out ++;
	}
}

static void
Mod_Load2QBSP_IBSP_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const texinfo_t *in;
	xtexinfo_t *out;
	size_t i, count;

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
		Q_strlcpy(out->texture, in->texture,
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
Mod_Load2QBSP_IBSP_XTEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const xtexinfo_t *in;
	xtexinfo_t *out;
	size_t i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (xtexinfo_t *)(inbuf + lumps[inlumppos].fileofs);
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
		Q_strlcpy(out->material, in->material,
			Q_min(sizeof(out->material), sizeof(in->material)));
		Q_strlcpy(out->texture, in->texture,
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
Mod_Load2QBSP_IBSP29_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dq1mipheader_t *miptextures;
	size_t i, count, tex_count, tex_offs;
	const dq1texinfo_t *in;
	xtexinfo_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq1texinfo_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs);
	tex_count = lumps[LUMP_BSP29_MIPTEX].filelen;
	tex_offs = lumps[LUMP_BSP29_MIPTEX].fileofs;
	miptextures = (dq1mipheader_t *)(inbuf + tex_offs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = Mod_LoadSurfConvertFlags(LittleLong(in->animated), maptype);
		out->nexttexinfo = -1;
		snprintf(out->texture, sizeof(out->texture), "#%d", LittleLong(in->texture_id));
		if (in->texture_id < tex_count)
		{
			int texture_offset;

			texture_offset = LittleLong(miptextures->offset[in->texture_id]);
			if (texture_offset > 0)
			{
				const dq1miptex_t *texture;

				texture = (dq1miptex_t *)(inbuf + tex_offs + texture_offset);

				if (!strncmp(texture->name, "sky", 3))
				{
					out->flags = SURF_SKY;
				}
				else if ((maptype == map_quake1 || maptype == map_hexen2) &&
					texture->name[0] == '*')
				{
					out->flags = SURF_WARP;
				}
				else if ((maptype == map_halflife1) && texture->name[0] == '!')
				{
					out->flags = SURF_WARP;
				}
			}
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_RBSP_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const texrinfo_t *in;
	xtexinfo_t *out;
	size_t i, count;

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
		Q_strlcpy(out->texture, in->texture,
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
Mod_Load2QBSP_IBSP46_TEXINFO(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dshader_t *in;
	xtexinfo_t *out;
	int i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dshader_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (xtexinfo_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		memset(out->vecs, 0, sizeof(out->vecs));

		out->flags = Mod_LoadSurfConvertFlags(LittleLong(in->surface_flags), maptype);
		out->nexttexinfo = -1;
		Q_strlcpy(out->texture, in->shader,
			Q_min(sizeof(out->texture), sizeof(in->shader)));

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const dface_t *in;
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
Mod_Load2QBSP_IBSP29_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const q1face_t *in;
	dqface_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (q1face_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqface_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->planenum = LittleShort(in->planenum);
		out->side = LittleShort(in->side);
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->texinfo = LittleShort(in->texinfo);
		/* use static light for all styles */
		memset(out->styles, 0, sizeof(out->styles));
		out->lightofs = LittleLong(in->lightofs) * 3;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP29_LIGHTING(byte *outbuf, const dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const byte *in;
	byte *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (byte *)(inbuf + lumps[inlumppos].fileofs);
	out = (byte *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		size_t j;

		for (j = 0; j < 3; j++)
		{
			*out = *in;
			out++;
		}
		in++;
	}
}

static void
Mod_Load2QBSP_RBSP_FACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const drface_t *in;
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
	size_t i, count;
	const dqface_t *in;
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
	size_t i, count;
	const dleaf_t *in;
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

		out->contents = Mod_LoadContentConvertFlags(LittleLong(in->contents), maptype);
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
Mod_Load2QBSP_IBSP29_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const dq1leaf_t *in;
	dqleaf_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq1leaf_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		out->contents = Mod_LoadContentConvertFlags(LittleLong(in->type), maptype);
		out->cluster = 0;
		out->area = 0;

		/* make unsigned long from signed short */
		out->firstleafface = LittleShort(in->firstleafface) & 0xFFFF;
		out->numleaffaces = LittleShort(in->numleaffaces) & 0xFFFF;
		out->firstleafbrush = i;
		out->numleafbrushes = 1;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_DKBSP_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const ddkleaf_t *in;
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

		out->contents = Mod_LoadContentConvertFlags(LittleLong(in->contents), maptype);
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
	size_t i, count;
	const dqleaf_t *in;
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

		out->contents = Mod_LoadContentConvertFlags(LittleLong(in->contents), maptype);
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

#define BSP46_PATCH_STEPS 4
#define BSP46_LIGHTMAP_SIZE (128 * 128 * 3)
#define BSP46_TEXCOORD_SCALE 64.0f

static qboolean
Mod_Load2QBSP_IBSP46_SurfaceIsPatch(const dq3surface_t *surface)
{
	return LittleLong(surface->type) == 2;
}

static size_t
Mod_Load2QBSP_IBSP46_PatchCellCount(const dq3surface_t *surface)
{
	int width, height;

	if (!Mod_Load2QBSP_IBSP46_SurfaceIsPatch(surface))
	{
		return 0;
	}

	width = LittleLong(surface->patch_width);
	height = LittleLong(surface->patch_height);
	if ((width < 3) || (height < 3) || !(width & 1) || !(height & 1))
	{
		return 0;
	}

	return ((width - 1) / 2) * ((height - 1) / 2);
}

static size_t
Mod_Load2QBSP_IBSP46_SurfaceVertexCount(const dq3surface_t *surface)
{
	return Mod_Load2QBSP_IBSP46_PatchCellCount(surface) *
		(BSP46_PATCH_STEPS + 1) * (BSP46_PATCH_STEPS + 1);
}

static int
Mod_Load2QBSP_IBSP46_Lightofs(const dq3surface_t *surface, const lump_t *lumps)
{
	int lightmapnum, numlightmaps;

	if (lumps[LUMP_BSP46_LIGHTMAPS].filelen % BSP46_LIGHTMAP_SIZE)
	{
		return -1;
	}

	lightmapnum = LittleLong(surface->lightmapnum);
	if (lightmapnum < 0)
	{
		return -1;
	}

	numlightmaps = lumps[LUMP_BSP46_LIGHTMAPS].filelen / BSP46_LIGHTMAP_SIZE;
	if (lightmapnum >= numlightmaps)
	{
		return -1;
	}

	return lightmapnum * BSP46_LIGHTMAP_SIZE;
}

static qboolean
Mod_Load2QBSP_IBSP46_SolveTexVec(const vec3_t p0, const vec3_t p1,
	const vec3_t p2, float tc0, float tc1, float tc2, float out[4])
{
	double a[3][4];
	vec3_t dp1, dp2, normal;
	int i, j, pivot;

	VectorSubtract(p1, p0, dp1);
	VectorSubtract(p2, p0, dp2);
	CrossProduct(dp1, dp2, normal);
	if (VectorNormalize(normal) == 0.0f)
	{
		return false;
	}

	for (i = 0; i < 3; i++)
	{
		a[0][i] = dp1[i];
		a[1][i] = dp2[i];
		a[2][i] = normal[i];
	}

	a[0][3] = (tc1 - tc0) * BSP46_TEXCOORD_SCALE;
	a[1][3] = (tc2 - tc0) * BSP46_TEXCOORD_SCALE;
	a[2][3] = 0.0;

	for (i = 0; i < 3; i++)
	{
		pivot = i;
		for (j = i + 1; j < 3; j++)
		{
			if (fabs(a[j][i]) > fabs(a[pivot][i]))
			{
				pivot = j;
			}
		}

		if (fabs(a[pivot][i]) < 0.0001)
		{
			return false;
		}

		if (pivot != i)
		{
			for (j = i; j < 4; j++)
			{
				double tmp;

				tmp = a[i][j];
				a[i][j] = a[pivot][j];
				a[pivot][j] = tmp;
			}
		}

		for (j = i + 1; j < 3; j++)
		{
			double scale;
			int k;

			scale = a[j][i] / a[i][i];
			for (k = i; k < 4; k++)
			{
				a[j][k] -= a[i][k] * scale;
			}
		}
	}

	for (i = 2; i >= 0; i--)
	{
		double value;

		value = a[i][3];
		for (j = i + 1; j < 3; j++)
		{
			value -= a[i][j] * out[j];
		}

		out[i] = value / a[i][i];
	}

	out[3] = tc0 * BSP46_TEXCOORD_SCALE - DotProduct(out, p0);
	return true;
}

static size_t
Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(const dq3surface_t *surface)
{
	int type, numindexes, numverts;

	type = LittleLong(surface->type);
	if (type == 2)
	{
		return Mod_Load2QBSP_IBSP46_PatchCellCount(surface) *
			BSP46_PATCH_STEPS * BSP46_PATCH_STEPS * 2;
	}

	if ((type != 1) && (type != 3))
	{
		return 0;
	}

	numindexes = LittleLong(surface->numindexes);
	if (numindexes >= 3)
	{
		return numindexes / 3;
	}

	numverts = LittleLong(surface->numverts);
	if (numverts >= 3)
	{
		return numverts - 2;
	}

	return 0;
}

static size_t
Mod_Load2QBSP_IBSP46_TotalVertexes(const byte *inbuf, const lump_t *lumps)
{
	const dq3surface_t *surfaces;
	size_t i, count, vertexes;

	if (lumps[LUMP_BSP46_SURFACES].filelen % sizeof(dq3surface_t))
	{
		return 0;
	}

	count = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	surfaces = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);

	vertexes = 0;
	for (i = 0; i < count; i++)
	{
		vertexes += Mod_Load2QBSP_IBSP46_SurfaceVertexCount(&surfaces[i]);
	}

	return vertexes;
}

static size_t
Mod_Load2QBSP_IBSP46_SurfaceFirstFace(const dq3surface_t *surfaces, int surface_index)
{
	size_t firstface;
	int i;

	firstface = 0;
	for (i = 0; i < surface_index; i++)
	{
		firstface += Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&surfaces[i]);
	}

	return firstface;
}

static size_t
Mod_Load2QBSP_IBSP46_TotalFaces(const byte *inbuf, const lump_t *lumps)
{
	const dq3surface_t *surfaces;
	size_t i, count, faces;

	if (lumps[LUMP_BSP46_SURFACES].filelen % sizeof(dq3surface_t))
	{
		return 0;
	}

	count = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	surfaces = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);

	faces = 0;
	for (i = 0; i < count; i++)
	{
		faces += Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&surfaces[i]);
	}

	return faces;
}

static size_t
Mod_Load2QBSP_IBSP46_TotalLeafFaces(const byte *inbuf, const lump_t *lumps)
{
	const dq3surface_t *surfaces;
	const dq3leaf_t *leafs;
	const int *leafsurfaces;
	size_t total;
	int i, j, count_leafs, count_leafsurfaces, count_surfaces;

	if ((lumps[LUMP_BSP46_SURFACES].filelen % sizeof(dq3surface_t)) ||
		(lumps[LUMP_BSP46_LEAFS].filelen % sizeof(dq3leaf_t)) ||
		(lumps[LUMP_BSP46_LEAFSURFACES].filelen % sizeof(int)))
	{
		return 0;
	}

	count_surfaces = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	count_leafs = lumps[LUMP_BSP46_LEAFS].filelen / sizeof(dq3leaf_t);
	count_leafsurfaces = lumps[LUMP_BSP46_LEAFSURFACES].filelen / sizeof(int);
	surfaces = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);
	leafs = (dq3leaf_t *)(inbuf + lumps[LUMP_BSP46_LEAFS].fileofs);
	leafsurfaces = (int *)(inbuf + lumps[LUMP_BSP46_LEAFSURFACES].fileofs);

	total = 0;
	for (i = 0; i < count_leafs; i++)
	{
		int firstleafface, numleaffaces;

		firstleafface = LittleLong(leafs[i].firstleafface);
		numleaffaces = LittleLong(leafs[i].numleaffaces);
		if ((firstleafface < 0) || (numleaffaces < 0) ||
			((firstleafface + numleaffaces) > count_leafsurfaces))
		{
			continue;
		}

		for (j = 0; j < numleaffaces; j++)
		{
			int surface_index;

			surface_index = LittleLong(leafsurfaces[firstleafface + j]);
			if ((surface_index < 0) || (surface_index >= count_surfaces))
			{
				continue;
			}

			total += Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&surfaces[surface_index]);
		}
	}

	return total;
}

static void
Mod_Load2QBSP_IBSP46_LEAFS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	int i, count, count_leafbrush, count_brush, count_shader, count_leafsurface;
	const int *in_leafsurface;
	const int *in_leafbrush;
	const dq3brush_t *in_brush;
	const dshader_t *in_shader;
	const dq3surface_t *in_surface;
	const dq3leaf_t *in;
	dqleaf_t *out;
	int count_surface;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq3leaf_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dqleaf_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	count_leafbrush = lumps[LUMP_BSP46_LEAFBRUSHES].filelen / sizeof(int);
	in_leafbrush = (int *)(inbuf + lumps[LUMP_BSP46_LEAFBRUSHES].fileofs);
	count_leafsurface = lumps[LUMP_BSP46_LEAFSURFACES].filelen / sizeof(int);
	in_leafsurface = (int *)(inbuf + lumps[LUMP_BSP46_LEAFSURFACES].fileofs);

	count_brush = lumps[LUMP_BSP46_BRUSHES].filelen / sizeof(dq3brush_t);
	in_brush = (dq3brush_t *)(inbuf + lumps[LUMP_BSP46_BRUSHES].fileofs);

	count_shader = lumps[LUMP_BSP46_SHADERS].filelen / sizeof(dshader_t);
	in_shader = (dshader_t *)(inbuf + lumps[LUMP_BSP46_SHADERS].fileofs);
	count_surface = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	in_surface = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j, brush_index, brushleaf_index, shader_index;
		int firstleafface, numleaffaces, face_index;
		int firstleafbrush, numleafbrushes, leafbrush_index;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(in->mins[j]);
			out->maxs[j] = LittleLong(in->maxs[j]);
		}

		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);

		firstleafface = LittleLong(in->firstleafface);
		numleaffaces = LittleLong(in->numleaffaces);
		if ((firstleafface < 0) || (numleaffaces < 0) ||
			((firstleafface + numleaffaces) > count_leafsurface))
		{
			Com_Error(ERR_DROP, "%s: Incorrect leafface range %d + %d > %d",
				__func__, firstleafface, numleaffaces, count_leafsurface);
			return;
		}

		out->firstleafface = 0;
		for (j = 0; j < i; j++)
		{
			int prev_first, prev_count, k;

			prev_first = LittleLong(((const dq3leaf_t *)(inbuf +
				lumps[inlumppos].fileofs))[j].firstleafface);
			prev_count = LittleLong(((const dq3leaf_t *)(inbuf +
				lumps[inlumppos].fileofs))[j].numleaffaces);
			if ((prev_first < 0) || (prev_count < 0) ||
				((prev_first + prev_count) > count_leafsurface))
			{
				Com_Error(ERR_DROP, "%s: Incorrect leafface range %d + %d > %d",
					__func__, prev_first, prev_count, count_leafsurface);
				return;
			}

			for (k = 0; k < prev_count; k++)
			{
				face_index = LittleLong(in_leafsurface[prev_first + k]);
				if ((face_index < 0) || (face_index >= count_surface))
				{
					Com_Error(ERR_DROP, "%s: Incorrect leafface index %d",
						__func__, face_index);
					return;
				}

				out->firstleafface +=
					Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&in_surface[face_index]);
			}
		}

		out->numleaffaces = 0;
		for (j = 0; j < numleaffaces; j++)
		{
			face_index = LittleLong(in_leafsurface[firstleafface + j]);
			if ((face_index < 0) || (face_index >= count_surface))
			{
				Com_Error(ERR_DROP, "%s: Incorrect leafface index %d",
					__func__, face_index);
				return;
			}

			out->numleaffaces +=
				Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&in_surface[face_index]);
		}

		out->firstleafbrush = LittleLong(in->firstleafbrush) & 0xFFFFFFFF;
		out->numleafbrushes = LittleLong(in->numleafbrushes) & 0xFFFFFFFF;
		out->contents = 0;

		/* get content flags */
		firstleafbrush = LittleLong(in->firstleafbrush);
		numleafbrushes = LittleLong(in->numleafbrushes);
		if ((firstleafbrush < 0) || (numleafbrushes < 0) ||
			((firstleafbrush + numleafbrushes) > count_leafbrush))
		{
			Com_Error(ERR_DROP, "%s: Incorrect brushleaf range %d + %d > %d",
				__func__, firstleafbrush, numleafbrushes, count_leafbrush);
			return;
		}

		for (leafbrush_index = 0; leafbrush_index < numleafbrushes; leafbrush_index++)
		{
			brushleaf_index = firstleafbrush + leafbrush_index;
			brush_index = LittleLong(in_leafbrush[brushleaf_index]);
			if ((brush_index < 0) || (brush_index >= count_brush))
			{
				Com_Error(ERR_DROP, "%s: Incorrect brush index %d > %d",
					__func__, brush_index, count_brush);
				return;
			}

			shader_index = LittleLong(in_brush[brush_index].shader_index) & 0xFFFFFFFF;
			if (shader_index >= count_shader)
			{
				Com_Error(ERR_DROP, "%s: Incorrect shader index %d > %d",
					__func__, shader_index, count_shader);
				return;
			}

			out->contents |= Mod_LoadContentConvertFlags(
				LittleLong(in_shader[shader_index].content_flags), maptype);
		}

		if (!(out->contents & (CONTENTS_SOLID | CONTENTS_LAVA | CONTENTS_SLIME |
								CONTENTS_WATER | CONTENTS_MIST)))
		{
			out->contents = 0;
		}

		if ((i == 0) && !out->contents)
		{
			out->contents = CONTENTS_SOLID;
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_LEAFFACES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const short *in;
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
	size_t i, count;
	const int *in;
	int *out;

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
	size_t i, count;
	const short *in;
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
	const dedge_t *in;
	dqedge_t *out;
	size_t i, count;

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
	const dqedge_t *in;
	dqedge_t *out;
	size_t i, count;

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
Mod_Load2QBSP_IBSP46_VERTEXES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const q3drawvert_t *in;
	size_t i, count;
	dvertex_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (q3drawvert_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dvertex_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			out->point[j] = LittleFloat(in->xyz[j]);
		}

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_MODELS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dmodel_t *in;
	dmodel_t *out;
	size_t i, count;

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
Mod_Load2QBSP_IBSP29_MODELS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const dq1model_t *in;
	dmodel_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq1model_t *)(inbuf + lumps[inlumppos].fileofs);
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

		out->headnode = LittleLong(in->headnode[0]);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP29_H2MODELS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const dh2model_t *in;
	dmodel_t *out;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dh2model_t *)(inbuf + lumps[inlumppos].fileofs);
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

		out->headnode = LittleLong(in->headnode[0]);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP46_MODELS(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	size_t i, count;
	const dq3surface_t *surfaces;
	const dq3model_t *in;
	dmodel_t *out;
	int count_surfaces;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq3model_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dmodel_t *)(outbuf + outheader->lumps[outlumppos].fileofs);
	count_surfaces = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	surfaces = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);

	for (i = 0; i < count; i++)
	{
		int j, firstface, numfaces;

		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
			out->origin[j] = (out->mins[j] + out->maxs[j]);
		}

		firstface = LittleLong(in->firstface);
		numfaces = LittleLong(in->numfaces);
		if ((firstface < 0) || (numfaces < 0) ||
			((firstface + numfaces) > count_surfaces))
		{
			Com_Error(ERR_DROP, "%s: Incorrect BSP46 model face range %d + %d > %d",
				__func__, firstface, numfaces, count_surfaces);
			return;
		}

		out->headnode = 0;
		out->firstface = Mod_Load2QBSP_IBSP46_SurfaceFirstFace(surfaces, firstface);
		out->numfaces = Mod_Load2QBSP_IBSP46_SurfaceFirstFace(surfaces,
			firstface + numfaces) - out->firstface;

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_BRUSHES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dbrush_t *in;
	dbrush_t *out;
	size_t i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dbrush_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dbrush_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		out->firstside = LittleLong(in->firstside) & 0xFFFFFFFF;
		out->numsides = LittleLong(in->numsides) & 0xFFFFFFFF;
		out->contents = Mod_LoadContentConvertFlags(LittleLong(in->contents), maptype);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP46_BRUSHES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dshader_t *in_shader;
	const dq3brush_t *in;
	dbrush_t *out;
	int count_shader;
	size_t i, count;

	count = lumps[inlumppos].filelen / rule_size;
	in = (dq3brush_t *)(inbuf + lumps[inlumppos].fileofs);
	out = (dbrush_t *)(outbuf + outheader->lumps[outlumppos].fileofs);

	count_shader = lumps[LUMP_BSP46_SHADERS].filelen / sizeof(dshader_t);
	in_shader = (dshader_t *)(inbuf + lumps[LUMP_BSP46_SHADERS].fileofs);

	for (i = 0; i < count; i++)
	{
		unsigned shader_index;

		out->firstside = LittleLong(in->firstside) & 0xFFFFFFFF;
		out->numsides = LittleLong(in->numsides) & 0xFFFFFFFF;
		out->contents = 0;

		/* get content flags */
		shader_index = LittleLong(in->shader_index) & 0xFFFFFFFF;
		if (shader_index >= count_shader)
		{
			Com_Error(ERR_DROP, "%s: Incorrect shader index %d > %d",
				__func__, shader_index, count_shader);
			return;
		}

		out->contents = Mod_LoadContentConvertFlags(
			LittleLong(in_shader[shader_index].content_flags), maptype);

		out++;
		in++;
	}
}

static void
Mod_Load2QBSP_IBSP_BRUSHSIDES(byte *outbuf, dheader_t *outheader,
	const byte *inbuf, const lump_t *lumps, size_t rule_size,
	maptype_t maptype, int outlumppos, int inlumppos)
{
	const dbrushside_t *in;
	dqbrushside_t *out;
	size_t i, count;

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
	const drbrushside_t *in;
	dqbrushside_t *out;
	size_t i, count;

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
	const dqbrushside_t *in;
	dqbrushside_t *out;
	size_t i, count;

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
	const darea_t *in;
	size_t i, count;
	darea_t *out;

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

static size_t
Mod_Load2QBSP_IBSP29_AdditionalSize(const lump_t *lumps)
{
	size_t result_size;

	/* lights is white only in quake 1, extend to rgb */
	result_size = (lumps[LUMP_BSP29_LIGHTING].filelen * 3 + 3) & ~3;

	/* set brush one to one to leaf */
	result_size += (lumps[LUMP_BSP29_LEAFS].filelen * sizeof(int) /
		sizeof(dq1leaf_t) + 3) & ~3;
	result_size += (lumps[LUMP_BSP29_LEAFS].filelen * sizeof(dbrush_t) /
		sizeof(dq1leaf_t) + 3) & ~3;
	result_size += (lumps[LUMP_BSP29_LEAFS].filelen * sizeof(dqbrushside_t) /
		sizeof(dq1leaf_t) + 3) & ~3;

	return result_size;
}

static size_t
Mod_Load2QBSP_AREAS_AdditionalSize(const lump_t *lumps)
{
	size_t result_size;

	/* Quake 1/Quake 3 does not have any areas, marks as single area */
	result_size = (sizeof(darea_t) + 3) & ~3;
	result_size += (sizeof(dareaportal_t) + 3) & ~3;

	return result_size;
}

static size_t
Mod_Load2QBSP_IBSP46_AdditionalSize(const byte *inbuf, const lump_t *lumps)
{
	size_t result_size, face_count, leafface_count, vertex_count, surface_count;

	face_count = Mod_Load2QBSP_IBSP46_TotalFaces(inbuf, lumps);
	leafface_count = Mod_Load2QBSP_IBSP46_TotalLeafFaces(inbuf, lumps);
	vertex_count = Mod_Load2QBSP_IBSP46_TotalVertexes(inbuf, lumps);
	surface_count = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);

	result_size = surface_count * sizeof(xtexinfo_t);
	result_size += lumps[LUMP_BSP46_LIGHTMAPS].filelen;
	result_size += vertex_count * sizeof(dvertex_t);
	result_size += face_count * sizeof(dqface_t);
	result_size += (face_count * 3 + 1) * sizeof(dqedge_t);
	result_size += face_count * 3 * sizeof(int);
	result_size += leafface_count * sizeof(int);

	return result_size;
}

static size_t
Mod_Load2QBSP_IBSP29_SetLumps(size_t ofs, const lump_t *lumps, lump_t *outlumps)
{
	/* Lighting, lights is white only in quake 1, extend to rgb */
	outlumps[LUMP_LIGHTING].fileofs = ofs;
	outlumps[LUMP_LIGHTING].filelen =
		(lumps[LUMP_BSP29_LIGHTING].filelen * 3 + 3) & ~3;
	ofs += outlumps[LUMP_LIGHTING].filelen;

	/* LeafBrushes */
	outlumps[LUMP_LEAFBRUSHES].fileofs = ofs;
	outlumps[LUMP_LEAFBRUSHES].filelen = lumps[LUMP_BSP29_LEAFS].filelen *
		sizeof(int) / sizeof(dq1leaf_t);
	ofs += outlumps[LUMP_LEAFBRUSHES].filelen;

	/* Brushes */
	outlumps[LUMP_BRUSHES].fileofs = ofs;
	outlumps[LUMP_BRUSHES].filelen = lumps[LUMP_BSP29_LEAFS].filelen *
		sizeof(dbrush_t) / sizeof(dq1leaf_t);
	ofs += outlumps[LUMP_BRUSHES].filelen;

	/* BrusheSides */
	outlumps[LUMP_BRUSHSIDES].fileofs = ofs;
	outlumps[LUMP_BRUSHSIDES].filelen = lumps[LUMP_BSP29_LEAFS].filelen *
		sizeof(dqbrushside_t) / sizeof(dq1leaf_t);
	ofs += outlumps[LUMP_BRUSHSIDES].filelen;
	return ofs;
}

static size_t
Mod_Load2QBSP_AREAS_SetLumps(size_t ofs, const lump_t *lumps, lump_t *outlumps)
{
	/* Areas */
	outlumps[LUMP_AREAS].fileofs = ofs;
	outlumps[LUMP_AREAS].filelen =
		(sizeof(darea_t) + 3) & ~3;
	ofs += outlumps[LUMP_AREAS].filelen;

	/* Areas Portals */
	outlumps[LUMP_AREAPORTALS].fileofs = ofs;
	outlumps[LUMP_AREAPORTALS].filelen =
		(sizeof(dareaportal_t) + 3) & ~3;
	ofs += outlumps[LUMP_AREAPORTALS].filelen;

	return ofs;
}

static size_t
Mod_Load2QBSP_IBSP46_SetLumps(size_t ofs, const byte *inbuf,
	const lump_t *lumps, lump_t *outlumps)
{
	size_t face_count, leafface_count, vertex_count, surface_count;
	int i;

	face_count = Mod_Load2QBSP_IBSP46_TotalFaces(inbuf, lumps);
	leafface_count = Mod_Load2QBSP_IBSP46_TotalLeafFaces(inbuf, lumps);
	vertex_count = Mod_Load2QBSP_IBSP46_TotalVertexes(inbuf, lumps);
	surface_count = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);

	outlumps[LUMP_TEXINFO].filelen += surface_count * sizeof(xtexinfo_t);
	outlumps[LUMP_VERTEXES].filelen += vertex_count * sizeof(dvertex_t);
	outlumps[LUMP_LIGHTING].filelen = lumps[LUMP_BSP46_LIGHTMAPS].filelen;
	outlumps[LUMP_FACES].filelen = face_count * sizeof(dqface_t);
	outlumps[LUMP_EDGES].filelen = (face_count * 3 + 1) * sizeof(dqedge_t);
	outlumps[LUMP_SURFEDGES].filelen = face_count * 3 * sizeof(int);
	outlumps[LUMP_LEAFFACES].filelen = leafface_count * sizeof(int);

	ofs = sizeof(dheader_t);
	for (i = 0; i < HEADER_LUMPS; i++)
	{
		if (outlumps[i].filelen)
		{
			outlumps[i].fileofs = ofs;
			ofs += outlumps[i].filelen;
		}
	}

	return ofs;
}

static void
Mod_Load2QBSP_IBSP29_Fix(const char *name, maptype_t maptype, const lump_t *lumps,
	const dheader_t *outheader, const byte *inbuf, byte *outbuf)
{
	int num_texinfo, i, num_leafs;
	dqbrushside_t *out_brushsides;
	xtexinfo_t *out_texinfo;
	dbrush_t *out_brushes;
	int *out_leafbrushes;
	dqleaf_t *out_leafs;

	/* fix textures path */
	num_texinfo = outheader->lumps[LUMP_TEXINFO].filelen / sizeof(xtexinfo_t);
	out_texinfo = (xtexinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);
	num_leafs = outheader->lumps[LUMP_LEAFS].filelen / sizeof(dqleaf_t);
	out_leafs = (dqleaf_t *)(outbuf + outheader->lumps[LUMP_LEAFS].fileofs);
	out_brushes = (dbrush_t *)(outbuf + outheader->lumps[LUMP_BRUSHES].fileofs);
	out_brushsides = (dqbrushside_t *)(outbuf + outheader->lumps[LUMP_BRUSHSIDES].fileofs);
	out_leafbrushes = (int *)(outbuf + outheader->lumps[LUMP_LEAFBRUSHES].fileofs);

	for (i = 0; i < num_texinfo; i++)
	{
		char texturename[80];

		snprintf(texturename, sizeof(texturename), "%s%s", name,
			out_texinfo[i].texture);
		Q_strlcpy(out_texinfo[i].texture, texturename, sizeof(out_texinfo[i].texture));
	}

	/* lights is white only in quake 1, extend to rgb */
	Mod_Load2QBSP_IBSP29_LIGHTING(outbuf, outheader, inbuf, lumps, sizeof(char), maptype,
		LUMP_LIGHTING, LUMP_BSP29_LIGHTING);

	/* Convert each Quake 1 leaf to a default leafbrush (one brush per leaf) */
	for (i = 0; i < num_leafs; i++)
	{
		/* Each leaf references its own brush */
		out_leafbrushes[i] = i;
	}

	/* Quake 1 leafs does not have plane and texinfo, use zero for now */
	for (i = 0; i < num_leafs; i++)
	{
		out_brushsides[i].planenum = 0;
		out_brushsides[i].texinfo = 0;
	}

	/* Brushes: convert each Quake 1 leaf to a default brush */
	for (i = 0; i < num_leafs; i++)
	{
		out_brushes[i].firstside = i; /* Each brush starts at its own brushside */
		out_brushes[i].numsides = 1;  /* One side per brush (per leaf) */
		out_brushes[i].contents = out_leafs[i].contents;
	}
}

static void
Mod_Load2QBSP_AREAS_Fix(const char *name, maptype_t maptype, const lump_t *lumps,
	const dheader_t *outheader, const byte *inbuf, byte *outbuf)
{
	size_t size;
	byte *out;

	/* Areas */
	size = outheader->lumps[LUMP_AREAS].filelen;
	out = (byte *)(outbuf + outheader->lumps[LUMP_AREAS].fileofs);
	memset(out, 0, size);

	/* Areas Portals */
	size = outheader->lumps[LUMP_AREAPORTALS].filelen;
	out = (byte *)(outbuf + outheader->lumps[LUMP_AREAPORTALS].fileofs);
	memset(out, 0, size);
}

static void
Mod_Load2QBSP_IBSP46_EvalPatchPoint(const q3drawvert_t *drawverts,
	int firstvert, int width, int cell_x, int cell_y, int step_x, int step_y,
	dvertex_t *out)
{
	float bu[3], bv[3], u, v;
	int i, j, k;

	u = (float)step_x / BSP46_PATCH_STEPS;
	v = (float)step_y / BSP46_PATCH_STEPS;
	bu[0] = (1.0f - u) * (1.0f - u);
	bu[1] = 2.0f * u * (1.0f - u);
	bu[2] = u * u;
	bv[0] = (1.0f - v) * (1.0f - v);
	bv[1] = 2.0f * v * (1.0f - v);
	bv[2] = v * v;

	VectorClear(out->point);
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			const q3drawvert_t *vert;
			float weight;

			vert = &drawverts[firstvert + (cell_y * 2 + j) * width + cell_x * 2 + i];
			weight = bu[i] * bv[j];
			for (k = 0; k < 3; k++)
			{
				out->point[k] += LittleFloat(vert->xyz[k]) * weight;
			}
		}
	}
}

static void
Mod_Load2QBSP_IBSP46_SurfaceTexinfo(xtexinfo_t *out, const xtexinfo_t *shader,
	const dq3surface_t *surface, const q3drawvert_t *drawverts,
	const int *drawindexes, int count_vertexes, int count_indexes)
{
	int i, firstvert, firstindex, numindexes, type, width;
	int verts[3];
	vec3_t points[3];
	float svec[4], tvec[4];

	memcpy(out, shader, sizeof(*out));
	out->nexttexinfo = -1;

	firstvert = LittleLong(surface->firstvert);
	firstindex = LittleLong(surface->firstindex);
	numindexes = LittleLong(surface->numindexes);
	type = LittleLong(surface->type);
	width = LittleLong(surface->patch_width);

	if ((type != 2) && (numindexes >= 3) &&
		(firstindex >= 0) && ((firstindex + 2) < count_indexes))
	{
		for (i = 0; i < 3; i++)
		{
			verts[i] = firstvert + LittleLong(drawindexes[firstindex + i]);
		}
	}
	else if ((type == 2) && (width >= 3))
	{
		verts[0] = firstvert;
		verts[1] = firstvert + 1;
		verts[2] = firstvert + width;
	}
	else
	{
		verts[0] = firstvert;
		verts[1] = firstvert + 1;
		verts[2] = firstvert + 2;
	}

	for (i = 0; i < 3; i++)
	{
		int j;

		if ((verts[i] < 0) || (verts[i] >= count_vertexes))
		{
			memset(out->vecs, 0, sizeof(out->vecs));
			out->vecs[0][0] = 1.0f;
			out->vecs[1][1] = 1.0f;
			return;
		}

		for (j = 0; j < 3; j++)
		{
			points[i][j] = LittleFloat(drawverts[verts[i]].xyz[j]);
		}
	}

	if (!Mod_Load2QBSP_IBSP46_SolveTexVec(points[0], points[1], points[2],
		LittleFloat(drawverts[verts[0]].st[0]),
		LittleFloat(drawverts[verts[1]].st[0]),
		LittleFloat(drawverts[verts[2]].st[0]), svec) ||
		!Mod_Load2QBSP_IBSP46_SolveTexVec(points[0], points[1], points[2],
		LittleFloat(drawverts[verts[0]].st[1]),
		LittleFloat(drawverts[verts[1]].st[1]),
		LittleFloat(drawverts[verts[2]].st[1]), tvec))
	{
		memset(out->vecs, 0, sizeof(out->vecs));
		out->vecs[0][0] = 1.0f;
		out->vecs[1][1] = 1.0f;
		return;
	}

	for (i = 0; i < 4; i++)
	{
		out->vecs[0][i] = svec[i];
		out->vecs[1][i] = tvec[i];
	}
}

static void
Mod_Load2QBSP_IBSP46_Fix(const char *name, maptype_t maptype, const lump_t *lumps,
	const dheader_t *outheader, const byte *inbuf, byte *outbuf)
{
	const dq3surface_t *surfaces;
	const q3drawvert_t *drawverts;
	const dq3leaf_t *leafs;
	const int *drawindexes, *leafsurfaces;
	dvertex_t *out_vertexes;
	xtexinfo_t *out_texinfo;
	dqface_t *out_faces;
	dqedge_t *out_edges;
	int *out_surfedges, *out_leaffaces;
	size_t out_face, out_leafface, out_vertex;
	int i, count_surfaces, count_vertexes, count_indexes;
	int count_leafs, count_leafsurfaces, count_shaders;

	if ((lumps[LUMP_BSP46_SURFACES].filelen % sizeof(dq3surface_t)) ||
		(lumps[LUMP_BSP46_DRAWVERTS].filelen % sizeof(q3drawvert_t)) ||
		(lumps[LUMP_BSP46_DRAWINDEXES].filelen % sizeof(int)) ||
		(lumps[LUMP_BSP46_LEAFS].filelen % sizeof(dq3leaf_t)) ||
		(lumps[LUMP_BSP46_LEAFSURFACES].filelen % sizeof(int)) ||
		(lumps[LUMP_BSP46_LIGHTMAPS].filelen % BSP46_LIGHTMAP_SIZE) ||
		(lumps[LUMP_BSP46_SHADERS].filelen % sizeof(dshader_t)))
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 lump sizes",
			__func__, name);
		return;
	}

	count_surfaces = lumps[LUMP_BSP46_SURFACES].filelen / sizeof(dq3surface_t);
	count_vertexes = lumps[LUMP_BSP46_DRAWVERTS].filelen / sizeof(q3drawvert_t);
	count_indexes = lumps[LUMP_BSP46_DRAWINDEXES].filelen / sizeof(int);
	count_leafs = lumps[LUMP_BSP46_LEAFS].filelen / sizeof(dq3leaf_t);
	count_leafsurfaces = lumps[LUMP_BSP46_LEAFSURFACES].filelen / sizeof(int);
	count_shaders = lumps[LUMP_BSP46_SHADERS].filelen / sizeof(dshader_t);

	surfaces = (dq3surface_t *)(inbuf + lumps[LUMP_BSP46_SURFACES].fileofs);
	drawverts = (q3drawvert_t *)(inbuf + lumps[LUMP_BSP46_DRAWVERTS].fileofs);
	drawindexes = (int *)(inbuf + lumps[LUMP_BSP46_DRAWINDEXES].fileofs);
	leafs = (dq3leaf_t *)(inbuf + lumps[LUMP_BSP46_LEAFS].fileofs);
	leafsurfaces = (int *)(inbuf + lumps[LUMP_BSP46_LEAFSURFACES].fileofs);

	out_vertexes = (dvertex_t *)(outbuf + outheader->lumps[LUMP_VERTEXES].fileofs);
	out_texinfo = (xtexinfo_t *)(outbuf + outheader->lumps[LUMP_TEXINFO].fileofs);
	out_faces = (dqface_t *)(outbuf + outheader->lumps[LUMP_FACES].fileofs);
	out_edges = (dqedge_t *)(outbuf + outheader->lumps[LUMP_EDGES].fileofs);
	out_surfedges = (int *)(outbuf + outheader->lumps[LUMP_SURFEDGES].fileofs);
	out_leaffaces = (int *)(outbuf + outheader->lumps[LUMP_LEAFFACES].fileofs);
	memcpy(outbuf + outheader->lumps[LUMP_LIGHTING].fileofs,
		inbuf + lumps[LUMP_BSP46_LIGHTMAPS].fileofs,
		lumps[LUMP_BSP46_LIGHTMAPS].filelen);
	for (i = 0; i < count_surfaces; i++)
	{
		int texinfo;

		texinfo = LittleLong(surfaces[i].texinfo);
		if ((texinfo < 0) || (texinfo >= count_shaders))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 surface texinfo %d",
				__func__, name, texinfo);
			return;
		}

		Mod_Load2QBSP_IBSP46_SurfaceTexinfo(&out_texinfo[count_shaders + i],
			&out_texinfo[texinfo], &surfaces[i], drawverts, drawindexes,
			count_vertexes, count_indexes);
	}

	memset(out_edges, 0, sizeof(*out_edges));
	out_face = 0;
	out_vertex = count_vertexes;
	for (i = 0; i < count_surfaces; i++)
	{
		const dq3surface_t *surface;
		size_t num_tris, tri;
		int firstvert, numverts, firstindex, numindexes, texinfo, type;
		int lightofs;

		surface = &surfaces[i];
		num_tris = Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(surface);
		if (!num_tris)
		{
			continue;
		}

		firstvert = LittleLong(surface->firstvert);
		numverts = LittleLong(surface->numverts);
		firstindex = LittleLong(surface->firstindex);
		numindexes = LittleLong(surface->numindexes);
		texinfo = count_shaders + i;
		type = LittleLong(surface->type);
		lightofs = Mod_Load2QBSP_IBSP46_Lightofs(surface, lumps);

		if ((firstvert < 0) || (numverts < 0) ||
			((firstvert + numverts) > count_vertexes) ||
			(texinfo < 0) ||
			(texinfo >= (int)(outheader->lumps[LUMP_TEXINFO].filelen / sizeof(xtexinfo_t))))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 surface %d",
				__func__, name, i);
			return;
		}

		if (type == 2)
		{
			int width, height, cell_x, cell_y, step_x, step_y;
			size_t base_vertex;

			width = LittleLong(surface->patch_width);
			height = LittleLong(surface->patch_height);
			if ((width < 3) || (height < 3) || !(width & 1) || !(height & 1) ||
				(width * height > numverts))
			{
				Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 patch surface %d",
					__func__, name, i);
				return;
			}

			for (cell_y = 0; cell_y < (height - 1) / 2; cell_y++)
			{
				for (cell_x = 0; cell_x < (width - 1) / 2; cell_x++)
				{
					base_vertex = out_vertex;
					for (step_y = 0; step_y <= BSP46_PATCH_STEPS; step_y++)
					{
						for (step_x = 0; step_x <= BSP46_PATCH_STEPS; step_x++, out_vertex++)
						{
							Mod_Load2QBSP_IBSP46_EvalPatchPoint(drawverts, firstvert,
								width, cell_x, cell_y, step_x, step_y,
								&out_vertexes[out_vertex]);
						}
					}

					for (step_y = 0; step_y < BSP46_PATCH_STEPS; step_y++)
					{
						for (step_x = 0; step_x < BSP46_PATCH_STEPS; step_x++)
						{
							int patch_verts[4];
							int tri_verts[2][3];
							int tri_index;

							patch_verts[0] = base_vertex + step_y *
								(BSP46_PATCH_STEPS + 1) + step_x;
							patch_verts[1] = patch_verts[0] + 1;
							patch_verts[2] = patch_verts[0] + BSP46_PATCH_STEPS + 1;
							patch_verts[3] = patch_verts[2] + 1;

							tri_verts[0][0] = patch_verts[0];
							tri_verts[0][1] = patch_verts[2];
							tri_verts[0][2] = patch_verts[1];
							tri_verts[1][0] = patch_verts[1];
							tri_verts[1][1] = patch_verts[2];
							tri_verts[1][2] = patch_verts[3];

							for (tri_index = 0; tri_index < 2; tri_index++, out_face++)
							{
								size_t edge_index, surfedge_index;

								edge_index = out_face * 3 + 1;
								surfedge_index = out_face * 3;

								out_edges[edge_index + 0].v[0] = tri_verts[tri_index][0];
								out_edges[edge_index + 0].v[1] = tri_verts[tri_index][1];
								out_edges[edge_index + 1].v[0] = tri_verts[tri_index][1];
								out_edges[edge_index + 1].v[1] = tri_verts[tri_index][2];
								out_edges[edge_index + 2].v[0] = tri_verts[tri_index][2];
								out_edges[edge_index + 2].v[1] = tri_verts[tri_index][0];

								out_surfedges[surfedge_index + 0] = edge_index + 0;
								out_surfedges[surfedge_index + 1] = edge_index + 1;
								out_surfedges[surfedge_index + 2] = edge_index + 2;

								out_faces[out_face].planenum = 0;
								out_faces[out_face].side = 0;
								out_faces[out_face].firstedge = surfedge_index;
								out_faces[out_face].numedges = 3;
								out_faces[out_face].texinfo = texinfo;
								memset(out_faces[out_face].styles, 255,
									sizeof(out_faces[out_face].styles));
								out_faces[out_face].styles[0] = 0;
								out_faces[out_face].lightofs = lightofs;
							}
						}
					}
				}
			}

			continue;
		}

		if (numindexes >= 3 &&
			((firstindex < 0) || ((firstindex + (int)(num_tris * 3)) > count_indexes)))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 indexes for surface %d",
				__func__, name, i);
			return;
		}

		for (tri = 0; tri < num_tris; tri++, out_face++)
		{
			int j, verts[3];
			size_t edge_index, surfedge_index;

			if (numindexes >= 3)
			{
				for (j = 0; j < 3; j++)
				{
					verts[j] = firstvert + LittleLong(drawindexes[firstindex + tri * 3 + j]);
				}
			}
			else
			{
				verts[0] = firstvert;
				verts[1] = firstvert + tri + 1;
				verts[2] = firstvert + tri + 2;
			}

			for (j = 0; j < 3; j++)
			{
				if ((verts[j] < 0) || (verts[j] >= count_vertexes))
				{
					Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 vertex %d",
						__func__, name, verts[j]);
					return;
				}
			}

			edge_index = out_face * 3 + 1;
			surfedge_index = out_face * 3;

			out_edges[edge_index + 0].v[0] = verts[0];
			out_edges[edge_index + 0].v[1] = verts[1];
			out_edges[edge_index + 1].v[0] = verts[1];
			out_edges[edge_index + 1].v[1] = verts[2];
			out_edges[edge_index + 2].v[0] = verts[2];
			out_edges[edge_index + 2].v[1] = verts[0];

			out_surfedges[surfedge_index + 0] = edge_index + 0;
			out_surfedges[surfedge_index + 1] = edge_index + 1;
			out_surfedges[surfedge_index + 2] = edge_index + 2;

			out_faces[out_face].planenum = 0;
			out_faces[out_face].side = 0;
			out_faces[out_face].firstedge = surfedge_index;
			out_faces[out_face].numedges = 3;
			out_faces[out_face].texinfo = texinfo;
			memset(out_faces[out_face].styles, 255, sizeof(out_faces[out_face].styles));
			out_faces[out_face].styles[0] = 0;
			out_faces[out_face].lightofs = lightofs;
		}
	}

	out_leafface = 0;
	for (i = 0; i < count_leafs; i++)
	{
		int j, firstleafface, numleaffaces;

		firstleafface = LittleLong(leafs[i].firstleafface);
		numleaffaces = LittleLong(leafs[i].numleaffaces);
		if ((firstleafface < 0) || (numleaffaces < 0) ||
			((firstleafface + numleaffaces) > count_leafsurfaces))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 leaffaces",
				__func__, name);
			return;
		}

		for (j = 0; j < numleaffaces; j++)
		{
			int surface_index;
			size_t firstface, num_tris, tri;

			surface_index = LittleLong(leafsurfaces[firstleafface + j]);
			if ((surface_index < 0) || (surface_index >= count_surfaces))
			{
				Com_Error(ERR_DROP, "%s: Map %s has incorrect BSP46 surface index %d",
					__func__, name, surface_index);
				return;
			}

			firstface = Mod_Load2QBSP_IBSP46_SurfaceFirstFace(surfaces, surface_index);
			num_tris = Mod_Load2QBSP_IBSP46_SurfaceTriangleCount(&surfaces[surface_index]);
			for (tri = 0; tri < num_tris; tri++, out_leafface++)
			{
				out_leaffaces[out_leafface] = firstface + tri;
			}
		}
	}

	{
		dqnode_t *out_nodes;
		int count_nodes;

		count_nodes = outheader->lumps[LUMP_NODES].filelen / sizeof(dqnode_t);
		out_nodes = (dqnode_t *)(outbuf + outheader->lumps[LUMP_NODES].fileofs);
		for (i = 0; i < count_nodes; i++)
		{
			out_nodes[i].firstface = 0;
			out_nodes[i].numfaces = 0;
		}

		if (count_nodes > 0)
		{
			out_nodes[0].numfaces = outheader->lumps[LUMP_FACES].filelen / sizeof(dqface_t);
		}
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

/* Quake 1 based games */

/* Quake 1 */
static const rule_t idq1bsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{-1, 0, NULL}, /* Wall Textures */
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{-1, 0, NULL}, /* Visability has different structure */
	{LUMP_NODES, sizeof(dq1node_t), Mod_Load2QBSP_IBSP29_NODES},
	{LUMP_TEXINFO, sizeof(dq1texinfo_t), Mod_Load2QBSP_IBSP29_TEXINFO},
	{LUMP_FACES, sizeof(q1face_t), Mod_Load2QBSP_IBSP29_FACES},
	{-1, 0, NULL}, /* LIGHTING has different structure */
	{-1, 0, NULL}, /* clipnode_t */
	{LUMP_LEAFS, sizeof(dq1leaf_t), Mod_Load2QBSP_IBSP29_LEAFS},
	{LUMP_LEAFFACES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{LUMP_EDGES, sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dq1model_t), Mod_Load2QBSP_IBSP29_MODELS},
};

/* Hexen 2 */
static const rule_t idh2bsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{-1, 0, NULL}, /* Wall Textures */
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{-1, 0, NULL}, /* Visability has different structure */
	{LUMP_NODES, sizeof(dq1node_t), Mod_Load2QBSP_IBSP29_NODES},
	{LUMP_TEXINFO, sizeof(dq1texinfo_t), Mod_Load2QBSP_IBSP29_TEXINFO},
	{LUMP_FACES, sizeof(q1face_t), Mod_Load2QBSP_IBSP29_FACES},
	{-1, 0, NULL}, /* LIGHTING has different structure */
	{-1, 0, NULL}, /* clipnode_t */
	{LUMP_LEAFS, sizeof(dq1leaf_t), Mod_Load2QBSP_IBSP29_LEAFS},
	{LUMP_LEAFFACES, sizeof(short), Mod_Load2QBSP_IBSP_LEAFFACES},
	{LUMP_EDGES, sizeof(dedge_t), Mod_Load2QBSP_IBSP_EDGES},
	{LUMP_SURFEDGES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dh2model_t), Mod_Load2QBSP_IBSP29_H2MODELS},
};

/* Quake 2 based games */
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
	{-1, 0, NULL}, /* LUMP_POP */
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
	{-1, 0, NULL}, /* LUMP_POP */
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
	{-1, 0, NULL}, /* LUMP_POP */
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
	{-1, 0, NULL}, /* LUMP_POP */
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

/* Quake 3 based games */
static const rule_t idq3bsplumps[HEADER_Q3LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_TEXINFO, sizeof(dshader_t), Mod_Load2QBSP_IBSP46_TEXINFO},
	{LUMP_PLANES, sizeof(dq3plane_t), Mod_Load2QBSP_IBSP46_PLANES},
	{LUMP_NODES, sizeof(dq3node_t), Mod_Load2QBSP_IBSP46_NODES},
	{LUMP_LEAFS, sizeof(dq3leaf_t), Mod_Load2QBSP_IBSP46_LEAFS},
	{-1, 0, NULL}, /* LUMP_BSP46_LEAFSURFACES */
	{LUMP_LEAFBRUSHES, sizeof(int), Mod_Load2QBSP_IBSP_CopyLong},
	{LUMP_MODELS, sizeof(dq3model_t), Mod_Load2QBSP_IBSP46_MODELS},
	{LUMP_BRUSHES, sizeof(dq3brush_t), Mod_Load2QBSP_IBSP46_BRUSHES},
	{LUMP_BRUSHSIDES, sizeof(dqbrushside_t), Mod_Load2QBSP_QBSP_BRUSHSIDES},
	{LUMP_VERTEXES, sizeof(q3drawvert_t), Mod_Load2QBSP_IBSP46_VERTEXES},
	{-1, 0, NULL}, /* LUMP_BSP46_DRAWINDEXES */
	{-1, 0, NULL}, /* LUMP_BSP46_FOGS */
	{-1, 0, NULL}, /* LUMP_BSP46_SURFACES */
	{-1, 0, NULL}, /* LUMP_BSP46_LIGHTMAPS */
	{-1, 0, NULL}, /* LUMP_BSP46_LIGHTGRID */
	{-1, 0, NULL}, /* LUMP_BSP46_VISIBILITY */
};

/* custom format with extended texture name */
static const rule_t xbsplumps[HEADER_LUMPS] = {
	{LUMP_ENTITIES, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_PLANES, sizeof(dplane_t), Mod_Load2QBSP_IBSP_PLANES},
	{LUMP_VERTEXES, sizeof(dvertex_t), Mod_Load2QBSP_IBSP_VERTEXES},
	{LUMP_VISIBILITY, sizeof(char), Mod_Load2QBSP_IBSP_Copy},
	{LUMP_NODES, sizeof(dqnode_t), Mod_Load2QBSP_QBSP_NODES},
	{LUMP_TEXINFO, sizeof(xtexinfo_t), Mod_Load2QBSP_IBSP_XTEXINFO},
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
	{-1, 0, NULL}, /* LUMP_POP */
	{LUMP_AREAS, sizeof(darea_t), Mod_Load2QBSP_IBSP_AREAS},
	{LUMP_AREAPORTALS, sizeof(dareaportal_t), Mod_Load2QBSP_IBSP_Copy},
};

/* keep it same as xbsplumps size */
static const char *lump_names[HEADER_LUMPS] = {
	"ENTITIES",
	"PLANES",
	"VERTEXES",
	"VISIBILITY",
	"NODES",
	"TEXINFO",
	"FACES",
	"LIGHTING",
	"LEAFS",
	"LEAFFACES",
	"LEAFBRUSHES",
	"EDGES",
	"SURFEDGES",
	"MODELS",
	"BRUSHES",
	"BRUSHSIDES",
	"POP",
	"AREAS",
	"AREAPORTALS"
};

static const char*
Mod_MaptypeName(maptype_t maptype)
{
	const char* maptypename;

	switch(maptype)
	{
		case map_quake1: maptypename = "Quake"; break;
		case map_hexen2: maptypename = "Hexen II"; break;
		case map_halflife1: maptypename = "Half Life"; break;
		case map_quake2: maptypename = "Quake II"; break;
		case map_quake2rr: maptypename = "Quake II ReRelease"; break;
		case map_quake3: maptypename = "Quake III Arena"; break;
		case map_heretic2: maptypename = "Heretic II"; break;
		case map_daikatana: maptypename = "Daikatana"; break;
		case map_kingpin: maptypename = "Kingpin"; break;
		case map_anachronox: maptypename = "Anachronox"; break;
		case map_sin: maptypename = "SiN"; break;
		default: maptypename = "Unknown"; break;
	}

	return maptypename;
}

static qboolean
Mod_HasBSPXHeader(const byte *inbuf, const lump_t *lumps, int numlumps, int filesize)
{
	size_t xofs, s;

	/* find end of last lump */
	xofs = 0;
	for (s = 0; s < numlumps; s++)
	{
		xofs = Q_max(xofs,
			(lumps[s].fileofs + lumps[s].filelen + 3) & ~3);
	}

	if (xofs + sizeof(bspx_header_t) < filesize)
	{
		const bspx_header_t* xheader;

		xheader = (bspx_header_t*)(inbuf + xofs);
		if (LittleLong(xheader->ident) == BSPXHEADER)
		{
			return true;
		}
	}

	return false;
}

static int
Mod_Load2QBSP_IBSP_TEXINFO_Flags(const byte *inbuf, const lump_t *lumps,
	size_t rule_size, int inlumppos)
{
	const texinfo_t *in;
	size_t i, count;
	int inflags;

	inflags = 0;
	count = lumps[inlumppos].filelen / rule_size;
	in = (texinfo_t *)(inbuf + lumps[inlumppos].fileofs);

	for (i = 0; i < count; i++)
	{
		inflags |= LittleLong(in->flags);
		in++;
	}

	return inflags;
}

static maptype_t
Mod_LoadGetRules(int ident, int version, const byte *inbuf, const lump_t *lumps,
	size_t filesize, const rule_t **rules, int *numlumps, int *numrules)
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
			int flags;

			*rules = idq2bsplumps;
			*numrules = *numlumps = HEADER_LUMPS;

			if (Mod_HasBSPXHeader(inbuf, lumps, *numlumps, filesize))
			{
				return map_quake2rr;
			}

			flags = Mod_Load2QBSP_IBSP_TEXINFO_Flags(inbuf, lumps,
				idq2bsplumps[LUMP_TEXINFO].size, LUMP_TEXINFO);

			/* used only quake2 flags */
			if (!(flags & ~QUAKE2_ALLFLAGS))
			{
				return map_quake2;
			}

			/* has heretic2 specific flags and no unused flags */
			if (((flags & HERETIC2_FLAGS) == HERETIC2_FLAGS) &&
				!(flags & ~HERETIC2_ALLFLAGS))
			{
				/* heretic 2 has 24, 25 set as material */
				return map_heretic2;
			}

			/* has kingpin specific flags and no unused flags */
			if (((flags & KINGPIN_FLAGS) == KINGPIN_FLAGS) &&
				!(flags & ~KINGPIN_ALLFLAGS))
			{
				/* kingpin has 19 - 27 set as material */
				return map_kingpin;
			}

			return map_quake2rr;
		}
		else if (version == BSPHL1VERSION)
		{
			*numrules = *numlumps = HEADER_Q1LUMPS;
			*rules = idq1bsplumps;
			return map_halflife1;
		}
		else if (version == BSPQ1VERSION)
		{
			size_t num_nodes, num_models, i;
			const dq1model_t *in_models;
			qboolean hexen2map = false;

			*numrules = *numlumps = HEADER_Q1LUMPS;

			if (lumps[LUMP_BSP29_MODELS].filelen % sizeof(dh2model_t) != 0)
			{
				/* for sure not hexen map */
				*rules = idq1bsplumps;
				return map_quake1;
			}

			if (lumps[LUMP_BSP29_MODELS].fileofs >= filesize)
			{
				/* incorrect lump fileofs */
				*rules = NULL;
				return map_quake2rr;
			}

			/* revalidate one more time */
			num_nodes = lumps[LUMP_BSP29_NODES].filelen / sizeof(dq1node_t);
			num_models = lumps[LUMP_BSP29_MODELS].filelen / sizeof(dq1model_t);
			in_models = (const dq1model_t *)(inbuf + lumps[LUMP_BSP29_MODELS].fileofs);

			for (i = 0; i < num_models; i++)
			{
				int headnode;

				/* check to incorrect nodes */
				headnode = LittleLong(in_models[i].headnode[0]);
				if (headnode >= num_nodes)
				{
					hexen2map = true;
					break;
				}
			}

			if (hexen2map)
			{
				*rules = idh2bsplumps;
				return map_hexen2;
			}
			else
			{
				*rules = idq1bsplumps;
				return map_quake1;
			}
		}
		else if (version == BSPQ3VERSION)
		{
			*rules = idq3bsplumps;
			*numrules = *numlumps = HEADER_Q3LUMPS;
			return map_quake3;
		}
	}
	else if (ident == QBSPHEADER && version == BSPVERSION)
	{
		if (lumps[LUMP_TEXINFO].filelen % sizeof(texinfo_t) == 0)
		{
			*rules = qbsplumps;
		}
		else
		{
			*rules = xbsplumps;
		}

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

static size_t
Mod_Load2QBSPValidateRules(const char *name, const rule_t *rules, size_t numrules,
	lump_t *lumps, int ident, int version, size_t filesize, maptype_t maptype)
{
	qboolean error = false;
	size_t result_size;

	result_size = sizeof(dheader_t);

	if (rules)
	{
		int s;

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

				if (rules[s].pos >= HEADER_LUMPS)
				{
					Com_Printf("%s: Map %s lump #%d: incorrect lump id #%d\n",
						__func__, name, s, rules[s].pos);
					error = true;
				}

				else if (rules[s].pos >= 0)
				{
					result_size += (
						xbsplumps[rules[s].pos].size * lumps[s].filelen / rules[s].size
					);

					Com_DPrintf("%s: #%s Lump: " YQ2_COM_PRIdS " elmenents\n",
						__func__, lump_names[rules[s].pos], lumps[s].filelen / rules[s].size);
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
				version, Mod_MaptypeName(maptype),
				result_size);

	if (error || !rules)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect lumps",
			__func__, name);
		return 0;
	}

	return result_size;
}

static size_t
Mod_Load2QBSPSizeByRules(const rule_t *rules, size_t numrules, dheader_t *outheader, lump_t *lumps)
{
	size_t ofs;
	int s;

	ofs = sizeof(dheader_t);

	/* mark offsets for all lumps */
	for (s = 0; s < numrules; s++)
	{
		if (rules[s].size && (rules[s].pos >= 0))
		{
			size_t pos;

			pos = rules[s].pos;
			outheader->lumps[pos].fileofs = ofs;
			outheader->lumps[pos].filelen = (
				xbsplumps[pos].size * lumps[s].filelen / rules[s].size
			);
			ofs += outheader->lumps[pos].filelen;
		}
	}

	return ofs;
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
	int s, numlumps, numrules;
	byte *outbuf;
	maptype_t detected_maptype;
	qboolean bspx_map = false;
	int ident, version;
	int *inlumps;
	size_t ofs, xofs;

	ident = LittleLong(((int *)inbuf)[0]);
	version = LittleLong(((int *)inbuf)[1]);
	inlumps = (int*)inbuf + 2; /* skip ident + version */
	if (ident == BSPQ1VERSION || ident == BSPHL1VERSION)
	{
		version = ident;
		ident = IDBSPHEADER;
		inlumps = (int*)inbuf + 1; /* version */
	}

	for (s = 0; s < sizeof(lumpsmem) / sizeof(int); s++)
	{
		lumpsmem[s] = LittleLong(((int *)inlumps)[s]);
	}
	lumps = (lump_t *)&lumpsmem;

	detected_maptype = Mod_LoadGetRules(ident, version, inbuf, lumps, filesize,
		&rules, &numlumps, &numrules);
	if (detected_maptype != map_quake2rr)
	{
		/* Use detected maptype only if for sure know */
		*maptype = detected_maptype;
	}

	result_size = Mod_Load2QBSPValidateRules(name, rules, numrules, lumps,
		ident, version, filesize, *maptype);

	/* find end of last lump */
	xofs = 0;
	for (s = 0; s < numlumps; s++)
	{
		xofs = Q_max(xofs,
			(lumps[s].fileofs + lumps[s].filelen + 3) & ~3);
	}

	if (xofs + sizeof(bspx_header_t) < filesize)
	{
		const bspx_header_t* xheader;

		xheader = (bspx_header_t*)(inbuf + xofs);
		if (LittleLong(xheader->ident) == BSPXHEADER)
		{
			result_size += (filesize - xofs);
			result_size += 4;
			bspx_map = true;
		}
		else
		{
			/* Have some other data at the end of file, just skip it */
			xofs = filesize;
		}
	}

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1))
	{
		result_size += Mod_Load2QBSP_IBSP29_AdditionalSize(lumps);
	}

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1) ||
		(detected_maptype == map_quake3))
	{
		result_size += Mod_Load2QBSP_AREAS_AdditionalSize(lumps);
	}

	if (detected_maptype == map_quake3)
	{
		result_size += Mod_Load2QBSP_IBSP46_AdditionalSize(inbuf, lumps);
	}

	outbuf = malloc(result_size);
	if (!outbuf)
	{
		Com_Error(ERR_DROP, "%s: Map %s is huge",
			__func__, name);
		return NULL;
	}

	outheader = (dheader_t*)outbuf;
	memset(outheader, 0, sizeof(dheader_t));
	outheader->ident = QBSPHEADER;
	outheader->version = BSPVERSION;

	ofs = Mod_Load2QBSPSizeByRules(rules, numrules, outheader, lumps);

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1))
	{
		ofs = Mod_Load2QBSP_IBSP29_SetLumps(ofs, lumps, outheader->lumps);
	}

	if (detected_maptype == map_quake3)
	{
		ofs = Mod_Load2QBSP_IBSP46_SetLumps(ofs, inbuf, lumps, outheader->lumps);
	}

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1) ||
		(detected_maptype == map_quake3))
	{
		ofs = Mod_Load2QBSP_AREAS_SetLumps(ofs, lumps, outheader->lumps);
	}

	if (filesize > xofs)
	{
		bspx_header_t *bspx_header;
		bspx_lump_t *lump;
		int numbspxlumps, i;

		ofs = ((ofs + 3) & ~3);
		bspx_header = (bspx_header_t *)(outbuf + ofs);

		/* copy BSPX */
		memcpy(bspx_header, inbuf + xofs, (filesize - xofs));

		/* fix positions */
		numbspxlumps = LittleLong(bspx_header->numlumps);
		if ((numbspxlumps * sizeof(*lump)) >= (filesize - xofs))
		{
			Com_Error(ERR_DROP, "%s: Map %s has incorrect bspx lumps",
				__func__, name);
			return NULL;
		}

		lump = (bspx_lump_t*)(bspx_header + 1);
		for (i = 0; i < numbspxlumps; i++, lump++)
		{
			size_t fileofs;

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
			return NULL;
		}

		rules[s].func(outbuf, outheader, inbuf, lumps, rules[s].size, *maptype,
			rules[s].pos, s);
	}

	if (!bspx_map)
	{
		Mod_Load2QBSP_TEXINFO_NOBSPX(outbuf, outheader);
	}

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1) ||
		(detected_maptype == map_quake3))
	{
		Mod_Load2QBSP_AREAS_Fix(name, *maptype, lumps, outheader, inbuf, outbuf);
	}

	if (detected_maptype == map_quake3)
	{
		Mod_Load2QBSP_IBSP46_Fix(name, *maptype, lumps, outheader, inbuf, outbuf);
	}

	if ((detected_maptype == map_quake1) ||
		(detected_maptype == map_hexen2) ||
		(detected_maptype == map_halflife1))
	{
		Mod_Load2QBSP_IBSP29_Fix(name, *maptype, lumps, outheader, inbuf, outbuf);
	}

	*out_len = result_size;
	return outbuf;
}

