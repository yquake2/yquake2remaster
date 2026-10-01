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

/* DOOM lumps names */
static const char *const doom_lumps[] = {
	"THINGS",
	"LINEDEFS",
	"SIDEDEFS",
	"VERTEXES",
	"SEGS",
	"SSECTORS",
	"NODES",
	"SECTORS",
	"REJECT",
	"BLOCKMAP",
	"BEHAVIOR"
};

/* Doom map conversion rules mapping nearest Doom lump types to Quake 2 BSP lump types */
static const rule_t doombsplumps[11] = {
	{-1, 0, NULL}, /* THINGS */
	{-1, 0, NULL}, /* LINEDEFS */
	{-1, 0, NULL}, /* SIDEDEFS */
	{-1, 0, NULL}, /* VERTEXES */
	{-1, 0, NULL}, /* SEGS */
	{-1, 0, NULL}, /* SSECTORS */
	{-1, 0, NULL}, /* NODES */
	{-1, 0, NULL}, /* SECTORS */
	{-1, 0, NULL}, /* REJECT */
	{-1, 0, NULL}, /* BLOCKMAP */
	{-1, 0, NULL}  /* BEHAVIOR */
};

typedef struct { short x, y; } doom_vertex_t;
typedef struct
{
	short textureoffset, rowoffset;
	char toptexture[8], bottomtexture[8], midtexture[8];
	short sector;
} doom_sidedef_t;
typedef struct { short v1, v2, flags, special, tag, sidenum[2]; } doom_linedef_t;
typedef struct
{
	short floorheight, ceilingheight;
	char floorpic[8], ceilingpic[8];
	short lightlevel, special, tag;
} doom_sector_t;
typedef struct { short numsegs, firstseg; } doom_subsector_t;
typedef struct { short v1, v2, angle, linedef, side, offset; } doom_seg_t;
typedef struct
{
	short x, y, dx, dy;
	short bbox[2][4];
	unsigned short children[2];
} doom_node_t;
typedef struct { short x, y, angle, type, options; } doom_thing_t;

typedef struct
{
	dplane_t *planes;
	size_t num_planes, planes_capacity;
	size_t *plane_hash;
	size_t plane_hash_capacity, plane_hash_count;
	xtexinfo_t *texinfo;
	size_t num_texinfo, texinfo_capacity;
	dvertex_t *vertexes;
	size_t num_vertexes, vertexes_capacity;
	dqedge_t *edges;
	size_t num_edges, edges_capacity;
	int *surfedges;
	size_t num_surfedges, surfedges_capacity;
	dqface_t *faces;
	size_t num_faces, faces_capacity;
	dqleaf_t *leafs;
	size_t num_leafs;
	int *leaffaces;
	size_t num_leaffaces, leaffaces_capacity;
	int *leafbrushes;
	size_t num_leafbrushes, leafbrushes_capacity;
	dqnode_t *nodes;
	size_t num_nodes;
	dbrush_t *brushes;
	size_t num_brushes, brushes_capacity;
	dqbrushside_t *brushsides;
	size_t num_brushsides, brushsides_capacity;
} doom_bsp_t;

static qboolean
Mod_DoomAppend(void **items, size_t *count, size_t *capacity,
	size_t item_size, const void *item, size_t *index)
{
	void *new_items;
	size_t new_capacity;

	if (*count == *capacity)
	{
		new_capacity = *capacity ? *capacity * 2 : 16;
		if ((new_capacity < *capacity) || (new_capacity > ((size_t)-1 / item_size)))
		{
			return false;
		}

		new_items = realloc(*items, new_capacity * item_size);
		if (!new_items)
		{
			return false;
		}
		*items = new_items;
		*capacity = new_capacity;
	}

	if (index)
	{
		*index = *count;
	}
	memcpy((byte *)*items + *count * item_size, item, item_size);
	(*count)++;
	return true;
}

static void
Mod_DoomFree(doom_bsp_t *bsp)
{
	free(bsp->planes);
	free(bsp->plane_hash);
	free(bsp->texinfo);
	free(bsp->vertexes);
	free(bsp->edges);
	free(bsp->surfedges);
	free(bsp->faces);
	free(bsp->leafs);
	free(bsp->leaffaces);
	free(bsp->leafbrushes);
	free(bsp->nodes);
	free(bsp->brushes);
	free(bsp->brushsides);
	memset(bsp, 0, sizeof(*bsp));
}

static size_t
Mod_DoomPlaneHash(const dplane_t *plane)
{
	size_t hash;
	unsigned int bits;
	int i;

	hash = 2166136261u;
	for (i = 0; i < 4; i++)
	{
		const float *value;

		value = i < 3 ? &plane->normal[i] : &plane->dist;
		memcpy(&bits, value, sizeof(bits));
		hash = (hash ^ bits) * 16777619u;
	}
	return hash;
}

static qboolean
Mod_DoomResizePlaneHash(doom_bsp_t *bsp, size_t capacity)
{
	size_t *hash_table;
	size_t i;

	hash_table = calloc(capacity, sizeof(*hash_table));
	if (!hash_table)
	{
		return false;
	}

	for (i = 0; i < bsp->num_planes; i += 2)
	{
		size_t slot;

		slot = Mod_DoomPlaneHash(&bsp->planes[i]) & (capacity - 1);
		while (hash_table[slot])
		{
			slot = (slot + 1) & (capacity - 1);
		}
		hash_table[slot] = i + 1;
	}

	free(bsp->plane_hash);
	bsp->plane_hash = hash_table;
	bsp->plane_hash_capacity = capacity;
	return true;
}

static qboolean
Mod_DoomEnsurePlaneHash(doom_bsp_t *bsp)
{
	size_t capacity;

	if (bsp->plane_hash_capacity &&
		((bsp->plane_hash_count + 1) * 2 <= bsp->plane_hash_capacity))
	{
		return true;
	}

	capacity = bsp->plane_hash_capacity ? bsp->plane_hash_capacity * 2 : 256;
	if (capacity < bsp->plane_hash_capacity)
	{
		return false;
	}
	return Mod_DoomResizePlaneHash(bsp, capacity);
}

static qboolean
Mod_DoomReadLump(const char *mapname, const byte *inbuf, size_t filesize,
	const lump_t *lumps, int lumpnum, const char *lumpname, size_t item_size,
	const byte **data, size_t *count)
{
	if ((lumps[lumpnum].fileofs > filesize) ||
		(lumps[lumpnum].filelen > filesize - lumps[lumpnum].fileofs))
	{
		Com_Printf("%s: Map %s Doom lump %s has invalid bounds (offset %u, length %u, file size "
			YQ2_COM_PRIdS ")\n", __func__, mapname, lumpname,
			lumps[lumpnum].fileofs, lumps[lumpnum].filelen, filesize);
		return false;
	}
	if (item_size && (lumps[lumpnum].filelen % item_size))
	{
		Com_Printf("%s: Map %s Doom lump %s length %u is not a multiple of record size "
			YQ2_COM_PRIdS "\n", __func__, mapname, lumpname,
			lumps[lumpnum].filelen, item_size);
		return false;
	}

	*data = inbuf + lumps[lumpnum].fileofs;
	*count = item_size ? lumps[lumpnum].filelen / item_size : lumps[lumpnum].filelen;
	return true;
}

static qboolean
Mod_DoomAddPlane(doom_bsp_t *bsp, const vec3_t normal, float dist, size_t *index)
{
	dplane_t plane, opposite;
	size_t plane_index, slot;
	qboolean reverse;
	int i;

	memset(&plane, 0, sizeof(plane));
	VectorCopy(normal, plane.normal);
	plane.dist = dist;
	plane.type = PLANE_ANYZ;
	reverse = false;
	for (i = 0; i < 3; i++)
	{
		if (plane.normal[i] != 0.0f)
		{
			reverse = plane.normal[i] < 0.0f;
			break;
	}
	}
	if (reverse)
	{
		VectorScale(plane.normal, -1.0f, plane.normal);
		plane.dist = -plane.dist;
	}
	for (i = 0; i < 3; i++)
	{
		if (plane.normal[i] == 0.0f)
		{
			plane.normal[i] = 0.0f;
		}
	}
	if (!Mod_DoomEnsurePlaneHash(bsp))
	{
		return false;
	}

	slot = Mod_DoomPlaneHash(&plane) & (bsp->plane_hash_capacity - 1);
	while (bsp->plane_hash[slot])
	{
		const dplane_t *existing;

		plane_index = bsp->plane_hash[slot] - 1;
		existing = &bsp->planes[plane_index];
		if ((existing->normal[0] == plane.normal[0]) &&
			(existing->normal[1] == plane.normal[1]) &&
			(existing->normal[2] == plane.normal[2]) &&
			(existing->dist == plane.dist))
		{
			*index = plane_index + (reverse ? 1 : 0);
			return true;
		}
		slot = (slot + 1) & (bsp->plane_hash_capacity - 1);
	}

	if (bsp->num_planes + 2 > MAX_MAP_PLANES)
	{
		return false;
	}
	plane_index = bsp->num_planes;
	if (!Mod_DoomAppend((void **)&bsp->planes, &bsp->num_planes,
		&bsp->planes_capacity, sizeof(plane), &plane, NULL))
	{
		return false;
	}
	opposite = plane;
	VectorScale(plane.normal, -1.0f, opposite.normal);
	opposite.dist = -plane.dist;
	if (!Mod_DoomAppend((void **)&bsp->planes, &bsp->num_planes,
		&bsp->planes_capacity, sizeof(opposite), &opposite, NULL))
	{
		return false;
	}
	bsp->plane_hash[slot] = plane_index + 1;
	bsp->plane_hash_count++;
	*index = plane_index + (reverse ? 1 : 0);
	return true;
}

static void
Mod_DoomTextureName(char *out, size_t out_size, const char in[8], const char *category)
{
	char name[9];
	size_t len;

	memcpy(name, in, 8);
	name[8] = 0;
	len = 8;
	while (len && (!name[len - 1] || (name[len - 1] == ' ')))
	{
		name[--len] = 0;
	}
	if (!len || ((len == 1) && (name[0] == '-')))
	{
		Q_strlcpy(out, "missing", out_size);
	}
	else if (category)
	{
		/* match the path Doom WAD flats/patches are extracted under */
		snprintf(out, out_size, "%s/%s", category, name);
	}
	else
	{
		Q_strlcpy(out, name, out_size);
	}
}

static qboolean
Mod_DoomAddTexinfo(doom_bsp_t *bsp, const char *texture,
	const float vecs[2][4], size_t *index)
{
	xtexinfo_t texinfo;

	if (bsp->num_texinfo >= MAX_MAP_TEXINFO)
	{
		return false;
	}
	memset(&texinfo, 0, sizeof(texinfo));
	memcpy(texinfo.vecs, vecs, sizeof(texinfo.vecs));
	Q_strlcpy(texinfo.texture, texture, sizeof(texinfo.texture));
	texinfo.nexttexinfo = -1;
	return Mod_DoomAppend((void **)&bsp->texinfo, &bsp->num_texinfo,
		&bsp->texinfo_capacity, sizeof(texinfo), &texinfo, index);
}

static qboolean
Mod_DoomAddFace(doom_bsp_t *bsp, size_t leaf_index,
	const float (*points)[3], size_t num_points, const char *texture,
	const float vecs[2][4])
{
	dqface_t face;
	dvertex_t vertex;
	dqedge_t edge;
	vec3_t edge1, edge2, normal;
	float dist;
	size_t texinfo_index, plane_index, face_index, point_index, first_vertex;

	if ((leaf_index >= bsp->num_leafs) || (num_points < 3) ||
		(num_points > 0xFFFFFFFFu) ||
		(bsp->num_faces >= MAX_MAP_FACES) ||
		(bsp->num_vertexes + num_points > MAX_MAP_VERTS) ||
		(bsp->num_edges + num_points > MAX_MAP_EDGES) ||
		(bsp->num_surfedges + num_points > MAX_MAP_SURFEDGES) ||
		(bsp->num_leaffaces >= MAX_MAP_LEAFFACES))
	{
		return false;
	}
	VectorSubtract(points[1], points[0], edge1);
	VectorSubtract(points[2], points[0], edge2);
	CrossProduct(edge1, edge2, normal);
	if (VectorNormalize(normal) == 0.0f)
	{
		return true;
	}
	dist = DotProduct(normal, points[0]);
	if (!Mod_DoomAddPlane(bsp, normal, dist, &plane_index) ||
		!Mod_DoomAddTexinfo(bsp, texture, vecs, &texinfo_index))
	{
		return false;
	}

	memset(&face, 0, sizeof(face));
	face.planenum = plane_index;
	face.firstedge = bsp->num_surfedges;
	face.numedges = num_points;
	face.texinfo = texinfo_index;
	memset(face.styles, 255, sizeof(face.styles));
	face.styles[0] = 0;
	face.lightofs = -1;
	first_vertex = bsp->num_vertexes;
	for (point_index = 0; point_index < num_points; point_index++)
	{
		size_t vertex_index, edge_index;
		int surfedge;

		VectorCopy(points[point_index], vertex.point);
		if (!Mod_DoomAppend((void **)&bsp->vertexes, &bsp->num_vertexes,
			&bsp->vertexes_capacity, sizeof(vertex), &vertex, &vertex_index))
		{
			return false;
		}
		edge.v[0] = vertex_index;
		edge.v[1] = first_vertex + ((point_index + 1) % num_points);
		if (!Mod_DoomAppend((void **)&bsp->edges, &bsp->num_edges,
			&bsp->edges_capacity, sizeof(edge), &edge, &edge_index))
		{
			return false;
		}
		surfedge = edge_index;
		if (!Mod_DoomAppend((void **)&bsp->surfedges, &bsp->num_surfedges,
			&bsp->surfedges_capacity, sizeof(surfedge), &surfedge, NULL))
		{
			return false;
		}
	}

	if (!Mod_DoomAppend((void **)&bsp->faces, &bsp->num_faces,
		&bsp->faces_capacity, sizeof(face), &face, &face_index))
	{
		return false;
	}
	{
		int leafface;

		leafface = face_index;
		return Mod_DoomAppend((void **)&bsp->leaffaces, &bsp->num_leaffaces,
			&bsp->leaffaces_capacity, sizeof(leafface), &leafface, NULL);
	}
}

static qboolean
Mod_DoomAddBrush(doom_bsp_t *bsp, size_t leaf_index,
	const float (*points)[2], size_t num_points, float min_z, float max_z)
{
	dbrush_t brush;
	dqbrushside_t brushside;
	float area;
	size_t point_index, firstside;

	if ((leaf_index >= bsp->num_leafs) || (num_points < 3) ||
		(max_z <= min_z) || (bsp->num_brushes >= MAX_MAP_BRUSHES) ||
		(bsp->num_brushsides + num_points + 2 > MAX_MAP_BRUSHSIDES) ||
		(bsp->num_leafbrushes >= MAX_MAP_LEAFBRUSHES))
	{
		return false;
	}
	area = 0.0f;
	for (point_index = 0; point_index < num_points; point_index++)
	{
		size_t next;

		next = (point_index + 1) % num_points;
		area += points[point_index][0] * points[next][1] -
			points[next][0] * points[point_index][1];
	}
	if (area == 0.0f)
	{
		return true;
	}

	firstside = bsp->num_brushsides;
	for (point_index = 0; point_index < num_points; point_index++)
	{
		size_t start_index, end_index, plane_index;
		vec3_t normal;
		float length, dist;

		if (area > 0.0f)
		{
			start_index = point_index;
			end_index = (point_index + 1) % num_points;
		}
		else
		{
			start_index = num_points - 1 - point_index;
			end_index = (start_index + num_points - 1) % num_points;
		}
		normal[0] = points[end_index][1] - points[start_index][1];
		normal[1] = points[start_index][0] - points[end_index][0];
		normal[2] = 0.0f;
		length = VectorNormalize(normal);
		if (length == 0.0f)
		{
			return false;
		}
		dist = normal[0] * points[start_index][0] +
			normal[1] * points[start_index][1];
		if (!Mod_DoomAddPlane(bsp, normal, dist, &plane_index))
		{
			return false;
		}
		brushside.planenum = plane_index;
		brushside.texinfo = 0;
		if (!Mod_DoomAppend((void **)&bsp->brushsides, &bsp->num_brushsides,
			&bsp->brushsides_capacity, sizeof(brushside), &brushside, NULL))
		{
			return false;
		}
	}

	{
		vec3_t normal;
		size_t plane_index;

		VectorSet(normal, 0.0f, 0.0f, -1.0f);
		if (!Mod_DoomAddPlane(bsp, normal, -min_z, &plane_index))
		{
			return false;
		}
		brushside.planenum = plane_index;
		brushside.texinfo = 0;
		if (!Mod_DoomAppend((void **)&bsp->brushsides, &bsp->num_brushsides,
			&bsp->brushsides_capacity, sizeof(brushside), &brushside, NULL))
		{
			return false;
		}
		VectorSet(normal, 0.0f, 0.0f, 1.0f);
		if (!Mod_DoomAddPlane(bsp, normal, max_z, &plane_index))
		{
			return false;
		}
		brushside.planenum = plane_index;
		if (!Mod_DoomAppend((void **)&bsp->brushsides, &bsp->num_brushsides,
			&bsp->brushsides_capacity, sizeof(brushside), &brushside, NULL))
		{
			return false;
		}
	}

	brush.firstside = firstside;
	brush.numsides = num_points + 2;
	brush.contents = CONTENTS_SOLID;
	if (!Mod_DoomAppend((void **)&bsp->brushes, &bsp->num_brushes,
		&bsp->brushes_capacity, sizeof(brush), &brush, NULL))
	{
		return false;
	}
	{
		int leafbrush;

		leafbrush = bsp->num_brushes - 1;
		return Mod_DoomAppend((void **)&bsp->leafbrushes, &bsp->num_leafbrushes,
			&bsp->leafbrushes_capacity, sizeof(leafbrush), &leafbrush, NULL);
	}
}

static qboolean
Mod_DoomAddWallBrush(doom_bsp_t *bsp, size_t leaf_index,
	float x1, float y1, float x2, float y2, float min_z, float max_z)
{
	float points[4][2];
	float dx, dy, length, ox, oy;

	dx = x2 - x1;
	dy = y2 - y1;
	length = sqrtf(dx * dx + dy * dy);
	if ((length == 0.0f) || (max_z <= min_z))
	{
		return true;
	}
	ox = dy / length * 2.0f;
	oy = -dx / length * 2.0f;
	points[0][0] = x1 + ox; points[0][1] = y1 + oy;
	points[1][0] = x2 + ox; points[1][1] = y2 + oy;
	points[2][0] = x2 - ox; points[2][1] = y2 - oy;
	points[3][0] = x1 - ox; points[3][1] = y1 - oy;
	return Mod_DoomAddBrush(bsp, leaf_index,
		(const float (*)[2])points, ARRLEN(points), min_z, max_z);
}

static qboolean
Mod_DoomAddWallFace(doom_bsp_t *bsp, size_t leaf_index,
	const char texture[8], short textureoffset, short rowoffset,
	short segoffset, short side, float x1, float y1, float x2, float y2,
	float min_z, float max_z)
{
	float points[4][3];
	float vecs[2][4] = {{0}};
	char texture_name[64];
	float dx, dy, length, direction;

	if (max_z <= min_z)
	{
		return true;
	}
	/* wall texture names refer to TEXTURE1/TEXTURE2 composed textures, not raw patches */
	Mod_DoomTextureName(texture_name, sizeof(texture_name), texture, NULL);
	if (!strcmp(texture_name, "missing"))
	{
		return true;
	}
	dx = x2 - x1;
	dy = y2 - y1;
	length = sqrtf(dx * dx + dy * dy);
	if (length == 0.0f)
	{
		return true;
	}

	direction = side ? -1.0f : 1.0f;
	vecs[0][0] = dx / length * direction;
	vecs[0][1] = dy / length * direction;
	vecs[0][3] = -(float)(textureoffset + segoffset);
	vecs[1][2] = 1.0f;
	vecs[1][3] = -(float)rowoffset;
	if (side)
	{
		points[0][0] = x1; points[0][1] = y1; points[0][2] = min_z;
		points[1][0] = x1; points[1][1] = y1; points[1][2] = max_z;
		points[2][0] = x2; points[2][1] = y2; points[2][2] = max_z;
		points[3][0] = x2; points[3][1] = y2; points[3][2] = min_z;
	}
	else
	{
		points[0][0] = x1; points[0][1] = y1; points[0][2] = min_z;
		points[1][0] = x2; points[1][1] = y2; points[1][2] = min_z;
		points[2][0] = x2; points[2][1] = y2; points[2][2] = max_z;
		points[3][0] = x1; points[3][1] = y1; points[3][2] = max_z;
	}
	return Mod_DoomAddFace(bsp, leaf_index,
		(const float (*)[3])points, ARRLEN(points), texture_name, vecs);
}

static int
Mod_DoomChild(unsigned short child, size_t num_nodes, size_t num_subsectors,
	qboolean *valid)
{
	if (child & 0x8000)
	{
		size_t subsector;

		subsector = child & 0x7FFF;
		if (subsector >= num_subsectors)
		{
			*valid = false;
			return -1;
		}
		return -(int)(subsector + 2);
	}
	if (child >= num_nodes)
	{
		*valid = false;
		return -1;
	}
	return child;
}

static void
Mod_DoomSetLump(byte *outbuf, dheader_t *header, int lumpnum,
	const void *data, size_t count, size_t item_size, size_t *offset)
{
	if (!count)
	{
		return;
	}
	*offset = (*offset + 3) & ~(size_t)3;
	header->lumps[lumpnum].fileofs = *offset;
	header->lumps[lumpnum].filelen = count * item_size;
	memcpy(outbuf + *offset, data, count * item_size);
	*offset += count * item_size;
}

static byte *
Mod_Load2QBSP_Doom(const char *name, const byte *inbuf, size_t filesize,
	const lump_t *lumps, size_t *out_len)
{
	const byte *raw_things, *raw_lines, *raw_sides, *raw_vertexes;
	const byte *raw_segs, *raw_subsectors, *raw_nodes, *raw_sectors;
	size_t num_things, num_lines, num_sides, num_vertexes;
	size_t num_segs, num_subsectors, num_nodes, num_sectors;
	const doom_thing_t *things;
	const doom_linedef_t *lines;
	const doom_sidedef_t *sides;
	const doom_vertex_t *vertexes;
	const doom_seg_t *segs;
	const doom_subsector_t *subsectors;
	const doom_node_t *nodes;
	const doom_sector_t *sectors;
	doom_bsp_t bsp;
	dqedge_t edge_zero;
	byte *entities = NULL, *outbuf = NULL;
	size_t entity_len, entity_capacity, output_size, offset;
	size_t subsector_index, node_index, thing_index;
	float world_min[3], world_max[3], world_min_z, world_max_z;
	qboolean valid;
	char failure_reason[160];

	memset(&bsp, 0, sizeof(bsp));
	Q_strlcpy(failure_reason, "validating required Doom lumps", sizeof(failure_reason));
	if (!Mod_DoomReadLump(name, inbuf, filesize, lumps, 0, "THINGS", sizeof(doom_thing_t),
		&raw_things, &num_things) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 1, "LINEDEFS", sizeof(doom_linedef_t),
		&raw_lines, &num_lines) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 2, "SIDEDEFS", sizeof(doom_sidedef_t),
		&raw_sides, &num_sides) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 3, "VERTEXES", sizeof(doom_vertex_t),
		&raw_vertexes, &num_vertexes) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 4, "SEGS", sizeof(doom_seg_t),
		&raw_segs, &num_segs) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 5, "SSECTORS", sizeof(doom_subsector_t),
		&raw_subsectors, &num_subsectors) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 6, "NODES", sizeof(doom_node_t),
		&raw_nodes, &num_nodes) ||
		!Mod_DoomReadLump(name, inbuf, filesize, lumps, 7, "SECTORS", sizeof(doom_sector_t),
		&raw_sectors, &num_sectors))
	{
		goto fail;
	}
	if (!num_lines || !num_sides || !num_vertexes || !num_segs ||
		!num_subsectors || !num_sectors || (num_nodes > MAX_MAP_NODES) ||
		(!num_nodes && (num_subsectors != 1)) || (num_subsectors > 32768) ||
		(num_subsectors + 1 > MAX_MAP_LEAFS))
	{
		snprintf(failure_reason, sizeof(failure_reason),
			"required lump empty or map counts unsupported (lines %d, sides %d, vertexes %d, segs %d, subsectors %d, nodes %d, sectors %d)",
			(int)num_lines, (int)num_sides, (int)num_vertexes, (int)num_segs,
			(int)num_subsectors, (int)num_nodes, (int)num_sectors);
		goto fail;
	}

	things = (const doom_thing_t *)raw_things;
	lines = (const doom_linedef_t *)raw_lines;
	sides = (const doom_sidedef_t *)raw_sides;
	vertexes = (const doom_vertex_t *)raw_vertexes;
	segs = (const doom_seg_t *)raw_segs;
	subsectors = (const doom_subsector_t *)raw_subsectors;
	nodes = (const doom_node_t *)raw_nodes;
	sectors = (const doom_sector_t *)raw_sectors;

	Q_strlcpy(failure_reason, "calculating map bounds", sizeof(failure_reason));
	world_min[0] = world_min[1] = world_min[2] = 1.0e30f;
	world_max[0] = world_max[1] = world_max[2] = -1.0e30f;
	for (thing_index = 0; thing_index < num_vertexes; thing_index++)
	{
		float x, y;

		x = LittleShort(vertexes[thing_index].x);
		y = LittleShort(vertexes[thing_index].y);
		world_min[0] = Q_min(world_min[0], x);
		world_min[1] = Q_min(world_min[1], y);
		world_max[0] = Q_max(world_max[0], x);
		world_max[1] = Q_max(world_max[1], y);
	}
	world_min_z = 1.0e30f;
	world_max_z = -1.0e30f;
	for (thing_index = 0; thing_index < num_sectors; thing_index++)
	{
		float floor_height, ceiling_height;

		floor_height = LittleShort(sectors[thing_index].floorheight);
		ceiling_height = LittleShort(sectors[thing_index].ceilingheight);
		world_min_z = Q_min(world_min_z, Q_min(floor_height, ceiling_height));
		world_max_z = Q_max(world_max_z, Q_max(floor_height, ceiling_height));
		if (ceiling_height <= floor_height)
		{
			continue;
		}
	}
	if (world_min_z > world_max_z)
	{
		Q_strlcpy(failure_reason, "no sectors have an open floor-to-ceiling interval",
			sizeof(failure_reason));
		goto fail;
	}
	world_min[2] = world_min_z - 64.0f;
	world_max[2] = world_max_z + 64.0f;

	Q_strlcpy(failure_reason, "allocating BSP nodes and leaves", sizeof(failure_reason));
	bsp.num_leafs = num_subsectors + 1;
	bsp.num_nodes = num_nodes ? num_nodes : 1;
	bsp.leafs = calloc(bsp.num_leafs, sizeof(*bsp.leafs));
	bsp.nodes = calloc(bsp.num_nodes, sizeof(*bsp.nodes));
	if (!bsp.leafs || !bsp.nodes)
	{
		Q_strlcpy(failure_reason, "allocating BSP node/leaf arrays", sizeof(failure_reason));
		goto fail;
	}
	memset(&edge_zero, 0, sizeof(edge_zero));
	if (!Mod_DoomAppend((void **)&bsp.edges, &bsp.num_edges,
		&bsp.edges_capacity, sizeof(edge_zero), &edge_zero, NULL))
	{
		Q_strlcpy(failure_reason, "allocating BSP edge sentinel", sizeof(failure_reason));
		goto fail;
	}
	bsp.leafs[0].contents = CONTENTS_SOLID;
	bsp.leafs[0].cluster = -1;

	Q_strlcpy(failure_reason, "converting Doom BSP nodes", sizeof(failure_reason));
	for (node_index = 0; node_index < bsp.num_nodes; node_index++)
	{
		dqnode_t *out_node;
		doom_node_t source_node;
		vec3_t normal;
		size_t plane_index;
		int child;

		out_node = &bsp.nodes[node_index];
		if (num_nodes)
		{
			float x, y, dx, dy, length;

			source_node = nodes[node_index];
			x = LittleShort(source_node.x);
			y = LittleShort(source_node.y);
			dx = LittleShort(source_node.dx);
			dy = LittleShort(source_node.dy);
			normal[0] = dy;
			normal[1] = -dx;
			normal[2] = 0.0f;
			length = VectorNormalize(normal);
			if ((length == 0.0f) || !Mod_DoomAddPlane(&bsp, normal,
				normal[0] * x + normal[1] * y, &plane_index))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"node %d has an invalid partition or plane allocation failed",
					(int)node_index);
				goto fail;
			}

			valid = true;
			child = Mod_DoomChild(LittleShort(source_node.children[0]),
				num_nodes, num_subsectors, &valid);
			if (!valid)
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"node %d has invalid child 0 (%u)", (int)node_index,
					(unsigned)LittleShort(source_node.children[0]));
				goto fail;
			}
			out_node->children[0] = child;
			child = Mod_DoomChild(LittleShort(source_node.children[1]),
				num_nodes, num_subsectors, &valid);
			if (!valid)
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"node %d has invalid child 1 (%u)", (int)node_index,
					(unsigned)LittleShort(source_node.children[1]));
				goto fail;
			}
			out_node->children[1] = child;
		}
		else
		{
			VectorSet(normal, 0.0f, 0.0f, 1.0f);
			if (!Mod_DoomAddPlane(&bsp, normal, 0.0f, &plane_index))
			{
				goto fail;
			}
			out_node->children[0] = -2;
			out_node->children[1] = -2;
		}
		out_node->planenum = plane_index;
		for (thing_index = 0; thing_index < 3; thing_index++)
		{
			out_node->mins[thing_index] = world_min[thing_index];
			out_node->maxs[thing_index] = world_max[thing_index];
		}
	}

	Q_strlcpy(failure_reason, "converting Doom subsectors", sizeof(failure_reason));
	for (subsector_index = 0; subsector_index < num_subsectors; subsector_index++)
	{
		doom_subsector_t subsector;
		dqleaf_t *leaf;
		float (*polygon)[2];
		size_t num_polygon_points, seg_index, leaf_index;
		int firstseg, numsegs, sector_index;
		float floor_height, ceiling_height, area_sum;
		char floor_texture[64], ceiling_texture[64];
		float floor_vecs[2][4] = {{0}}, ceiling_vecs[2][4] = {{0}};
		qboolean has_polygon_area;

		subsector = subsectors[subsector_index];
		firstseg = LittleShort(subsector.firstseg);
		numsegs = LittleShort(subsector.numsegs);
		if ((firstseg < 0) || (numsegs < 1) ||
			((size_t)firstseg + numsegs > num_segs))
		{
			snprintf(failure_reason, sizeof(failure_reason),
				"subsector %d has invalid seg range %d + %d (count %d)",
				(int)subsector_index, firstseg, numsegs, (int)num_segs);
			goto fail;
		}
		polygon = malloc((size_t)numsegs * 2 * sizeof(*polygon));
		if (!polygon)
		{
			goto fail;
		}

		sector_index = -1;
		num_polygon_points = 0;
		for (seg_index = (size_t)firstseg;
			seg_index < (size_t)firstseg + numsegs; seg_index++)
		{
			doom_seg_t seg;
			doom_linedef_t line;
			doom_sidedef_t side;
			int v1, v2, linedef_index, side_index, side_lump_index;
			int current_sector;
			size_t duplicate_index;
			qboolean duplicate_point;

			seg = segs[seg_index];
			v1 = LittleShort(seg.v1);
			v2 = LittleShort(seg.v2);
			linedef_index = LittleShort(seg.linedef);
			side_index = LittleShort(seg.side);
			if ((v1 < 0) || (v2 < 0) || (v1 >= num_vertexes) ||
				(v2 >= num_vertexes) || (linedef_index < 0) ||
				(linedef_index >= num_lines) || (side_index < 0) || (side_index > 1))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"subsector %d seg %d has invalid indices v=%d,%d line=%d side=%d",
					(int)subsector_index, (int)seg_index, v1, v2,
					linedef_index, side_index);
				free(polygon);
				goto fail;
			}
			line = lines[linedef_index];
			side_lump_index = LittleShort(line.sidenum[side_index]);
			if ((side_lump_index < 0) || (side_lump_index >= num_sides))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"subsector %d seg %d references sidedef %d (count %d)",
					(int)subsector_index, (int)seg_index, side_lump_index,
					(int)num_sides);
				free(polygon);
				goto fail;
			}
			side = sides[side_lump_index];
			current_sector = LittleShort(side.sector);
			if ((current_sector < 0) || (current_sector >= num_sectors) ||
				((sector_index >= 0) && (sector_index != current_sector)))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"subsector %d has inconsistent sector %d (expected %d)",
					(int)subsector_index, current_sector, sector_index);
				free(polygon);
				goto fail;
			}
			sector_index = current_sector;
			{
				int endpoints[2], endpoint_index;

				endpoints[0] = v1;
				endpoints[1] = v2;
				for (endpoint_index = 0; endpoint_index < 2; endpoint_index++)
				{
					int vertex_index;

					vertex_index = endpoints[endpoint_index];
					duplicate_point = false;
					for (duplicate_index = 0; duplicate_index < num_polygon_points;
						duplicate_index++)
					{
						if ((polygon[duplicate_index][0] == LittleShort(vertexes[vertex_index].x)) &&
							(polygon[duplicate_index][1] == LittleShort(vertexes[vertex_index].y)))
						{
							duplicate_point = true;
							break;
						}
					}
					if (!duplicate_point)
					{
						polygon[num_polygon_points][0] = LittleShort(vertexes[vertex_index].x);
						polygon[num_polygon_points][1] = LittleShort(vertexes[vertex_index].y);
						num_polygon_points++;
					}
				}
			}
		}
		if ((num_polygon_points > 1) &&
			(polygon[0][0] == polygon[num_polygon_points - 1][0]) &&
			(polygon[0][1] == polygon[num_polygon_points - 1][1]))
		{
			num_polygon_points--;
		}
		if (!num_polygon_points || (sector_index < 0))
		{
			snprintf(failure_reason, sizeof(failure_reason),
				"subsector %d has no polygon points or no sector",
				(int)subsector_index);
			free(polygon);
			goto fail;
		}

		{
			float center_x, center_y;
			size_t point_index;

			center_x = center_y = 0.0f;
			for (point_index = 0; point_index < num_polygon_points; point_index++)
			{
				center_x += polygon[point_index][0];
				center_y += polygon[point_index][1];
			}
			center_x /= num_polygon_points;
			center_y /= num_polygon_points;

			for (point_index = 1; point_index < num_polygon_points; point_index++)
			{
				float point_x, point_y, point_angle;
				size_t sorted_index;

				point_x = polygon[point_index][0];
				point_y = polygon[point_index][1];
				point_angle = atan2f(point_y - center_y, point_x - center_x);
				sorted_index = point_index;
				while (sorted_index > 0 &&
					atan2f(polygon[sorted_index - 1][1] - center_y,
						polygon[sorted_index - 1][0] - center_x) > point_angle)
				{
					polygon[sorted_index][0] = polygon[sorted_index - 1][0];
					polygon[sorted_index][1] = polygon[sorted_index - 1][1];
					sorted_index--;
				}
				polygon[sorted_index][0] = point_x;
				polygon[sorted_index][1] = point_y;
			}
		}

		area_sum = 0.0f;
		for (seg_index = 0; seg_index < num_polygon_points; seg_index++)
		{
			size_t next;

			next = (seg_index + 1) % num_polygon_points;
			area_sum += polygon[seg_index][0] * polygon[next][1] -
				polygon[next][0] * polygon[seg_index][1];
		}
		has_polygon_area = area_sum != 0.0f;
		if (area_sum < 0.0f)
		{
			for (seg_index = 0; seg_index < num_polygon_points / 2; seg_index++)
			{
				float tmp[2];
				size_t opposite;

				opposite = num_polygon_points - 1 - seg_index;
				tmp[0] = polygon[seg_index][0];
				tmp[1] = polygon[seg_index][1];
				polygon[seg_index][0] = polygon[opposite][0];
				polygon[seg_index][1] = polygon[opposite][1];
				polygon[opposite][0] = tmp[0];
				polygon[opposite][1] = tmp[1];
			}
		}

		leaf_index = subsector_index + 1;
		leaf = &bsp.leafs[leaf_index];
		if (!has_polygon_area)
		{
			leaf->contents = CONTENTS_SOLID;
		}
		leaf->cluster = -1;
		leaf->area = 0;
		leaf->firstleafface = bsp.num_leaffaces;
		leaf->firstleafbrush = bsp.num_leafbrushes;
		leaf->mins[0] = leaf->mins[1] = leaf->mins[2] = 1.0e30f;
		leaf->maxs[0] = leaf->maxs[1] = leaf->maxs[2] = -1.0e30f;
		for (seg_index = 0; seg_index < num_polygon_points; seg_index++)
		{
			leaf->mins[0] = Q_min(leaf->mins[0], polygon[seg_index][0]);
			leaf->mins[1] = Q_min(leaf->mins[1], polygon[seg_index][1]);
			leaf->maxs[0] = Q_max(leaf->maxs[0], polygon[seg_index][0]);
			leaf->maxs[1] = Q_max(leaf->maxs[1], polygon[seg_index][1]);
		}
		floor_height = LittleShort(sectors[sector_index].floorheight);
		ceiling_height = LittleShort(sectors[sector_index].ceilingheight);
		if (ceiling_height < floor_height)
		{
			snprintf(failure_reason, sizeof(failure_reason),
				"subsector %d references inverted sector %d (floor %.0f, ceiling %.0f)",
				(int)subsector_index, sector_index, floor_height, ceiling_height);
			free(polygon);
			goto fail;
		}
		if (ceiling_height == floor_height)
		{
			leaf->contents = CONTENTS_SOLID;
		}
		leaf->mins[2] = floor_height;
		leaf->maxs[2] = ceiling_height;
		Mod_DoomTextureName(floor_texture, sizeof(floor_texture), sectors[sector_index].floorpic, "flat");
		Mod_DoomTextureName(ceiling_texture, sizeof(ceiling_texture), sectors[sector_index].ceilingpic, "flat");
		floor_vecs[0][0] = 1.0f / 64.0f;
		floor_vecs[1][1] = 1.0f / 64.0f;
		ceiling_vecs[0][0] = 1.0f / 64.0f;
		ceiling_vecs[1][1] = -1.0f / 64.0f;

		{
			float (*floor_points)[3], (*ceiling_points)[3];

			floor_points = malloc(num_polygon_points * sizeof(*floor_points));
			ceiling_points = malloc(num_polygon_points * sizeof(*ceiling_points));
			if (!floor_points || !ceiling_points)
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"allocating floor/ceiling polygons for subsector %d",
					(int)subsector_index);
				free(floor_points);
				free(ceiling_points);
				free(polygon);
				goto fail;
			}
			for (seg_index = 0; seg_index < num_polygon_points; seg_index++)
			{
				size_t reverse_index;

				reverse_index = num_polygon_points - 1 - seg_index;
				floor_points[seg_index][0] = polygon[seg_index][0];
				floor_points[seg_index][1] = polygon[seg_index][1];
				floor_points[seg_index][2] = floor_height;
				ceiling_points[seg_index][0] = polygon[reverse_index][0];
				ceiling_points[seg_index][1] = polygon[reverse_index][1];
				ceiling_points[seg_index][2] = ceiling_height;
			}
			if (has_polygon_area && (ceiling_height > floor_height) &&
				(!Mod_DoomAddFace(&bsp, leaf_index,
					(const float (*)[3])floor_points, num_polygon_points,
					floor_texture, floor_vecs) ||
					!Mod_DoomAddFace(&bsp, leaf_index,
						(const float (*)[3])ceiling_points, num_polygon_points,
						ceiling_texture, ceiling_vecs)))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"generating floor/ceiling surfaces for subsector %d",
					(int)subsector_index);
				free(floor_points);
				free(ceiling_points);
				free(polygon);
				goto fail;
			}
			if (has_polygon_area &&
				(!Mod_DoomAddBrush(&bsp, leaf_index,
				(const float (*)[2])polygon, num_polygon_points,
				world_min_z - 64.0f, floor_height) ||
				!Mod_DoomAddBrush(&bsp, leaf_index,
					(const float (*)[2])polygon, num_polygon_points,
					ceiling_height, world_max_z + 64.0f)))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"generating floor/ceiling collision brushes for subsector %d",
					(int)subsector_index);
				free(floor_points);
				free(ceiling_points);
				free(polygon);
				goto fail;
			}
			free(floor_points);
			free(ceiling_points);
		}

		for (seg_index = (size_t)firstseg;
			seg_index < (size_t)firstseg + numsegs; seg_index++)
		{
			doom_seg_t seg;
			doom_linedef_t line;
			doom_sidedef_t side;
			int line_index, side_index, side_lump_index, back_side_index;
			int back_sector_index;
			float x1, y1, x2, y2, back_floor, back_ceiling;
			float lower_top, upper_bottom, opening_floor, opening_ceiling;
			qboolean two_sided;

			seg = segs[seg_index];
			line_index = LittleShort(seg.linedef);
			side_index = LittleShort(seg.side);
			line = lines[line_index];
			side_lump_index = LittleShort(line.sidenum[side_index]);
			side = sides[side_lump_index];
			x1 = LittleShort(vertexes[LittleShort(seg.v1)].x);
			y1 = LittleShort(vertexes[LittleShort(seg.v1)].y);
			x2 = LittleShort(vertexes[LittleShort(seg.v2)].x);
			y2 = LittleShort(vertexes[LittleShort(seg.v2)].y);
			back_side_index = LittleShort(line.sidenum[side_index ^ 1]);
			two_sided = ((LittleShort(line.flags) & 4) != 0) &&
				(back_side_index >= 0) && (back_side_index < num_sides);
			back_sector_index = two_sided ? LittleShort(sides[back_side_index].sector) : -1;
			if (two_sided && ((back_sector_index < 0) || (back_sector_index >= num_sectors)))
			{
					snprintf(failure_reason, sizeof(failure_reason),
						"subsector %d seg %d references back sector %d (count %d)",
						(int)subsector_index, (int)seg_index, back_sector_index,
						(int)num_sectors);
				free(polygon);
				goto fail;
			}

			back_floor = two_sided ? LittleShort(sectors[back_sector_index].floorheight) : floor_height;
			back_ceiling = two_sided ? LittleShort(sectors[back_sector_index].ceilingheight) : ceiling_height;
			opening_floor = Q_max(floor_height, back_floor);
			opening_ceiling = Q_min(ceiling_height, back_ceiling);
			lower_top = two_sided ? Q_min(back_floor, ceiling_height) : ceiling_height;
			upper_bottom = two_sided ? Q_max(back_ceiling, floor_height) : floor_height;

			if (!two_sided)
			{
				if (!Mod_DoomAddWallFace(&bsp, leaf_index, side.midtexture,
					LittleShort(side.textureoffset), LittleShort(side.rowoffset),
					LittleShort(seg.offset), side_index, x1, y1, x2, y2,
					floor_height, ceiling_height) ||
					!Mod_DoomAddWallBrush(&bsp, leaf_index, x1, y1, x2, y2,
						floor_height, ceiling_height))
				{
					snprintf(failure_reason, sizeof(failure_reason),
						"generating one-sided wall at subsector %d seg %d",
						(int)subsector_index, (int)seg_index);
					free(polygon);
					goto fail;
				}
			}
			else
			{
				if (back_floor > floor_height)
				{
					if (!Mod_DoomAddWallFace(&bsp, leaf_index, side.bottomtexture,
						LittleShort(side.textureoffset), LittleShort(side.rowoffset),
						LittleShort(seg.offset), side_index, x1, y1, x2, y2,
						floor_height, lower_top) ||
						!Mod_DoomAddWallBrush(&bsp, leaf_index, x1, y1, x2, y2,
							floor_height, lower_top))
					{
						snprintf(failure_reason, sizeof(failure_reason),
							"generating lower wall at subsector %d seg %d (faces %d, brushes %d, sides %d, planes %d, texinfo %d)",
							(int)subsector_index, (int)seg_index,
							(int)bsp.num_faces, (int)bsp.num_brushes,
							(int)bsp.num_brushsides, (int)bsp.num_planes,
							(int)bsp.num_texinfo);
						free(polygon);
						goto fail;
					}
				}
				if (ceiling_height > back_ceiling)
				{
					if (!Mod_DoomAddWallFace(&bsp, leaf_index, side.toptexture,
						LittleShort(side.textureoffset), LittleShort(side.rowoffset),
						LittleShort(seg.offset), side_index, x1, y1, x2, y2,
						upper_bottom, ceiling_height) ||
						!Mod_DoomAddWallBrush(&bsp, leaf_index, x1, y1, x2, y2,
							upper_bottom, ceiling_height))
					{
						snprintf(failure_reason, sizeof(failure_reason),
							"generating upper wall at subsector %d seg %d (faces %d, brushes %d, sides %d, planes %d, texinfo %d)",
							(int)subsector_index, (int)seg_index,
							(int)bsp.num_faces, (int)bsp.num_brushes,
							(int)bsp.num_brushsides, (int)bsp.num_planes,
							(int)bsp.num_texinfo);
						free(polygon);
						goto fail;
					}
				}
				if ((LittleShort(line.flags) & 1) && (opening_ceiling > opening_floor) &&
					!Mod_DoomAddWallBrush(&bsp, leaf_index, x1, y1, x2, y2,
						opening_floor, opening_ceiling))
				{
					snprintf(failure_reason, sizeof(failure_reason),
						"generating blocking mid-wall at subsector %d seg %d",
						(int)subsector_index, (int)seg_index);
					free(polygon);
					goto fail;
				}
			}
		}

		leaf->numleaffaces = bsp.num_leaffaces - leaf->firstleafface;
		leaf->numleafbrushes = bsp.num_leafbrushes - leaf->firstleafbrush;
		free(polygon);
	}

	if (!bsp.num_faces || !bsp.num_brushes || !bsp.num_brushsides ||
		!bsp.num_texinfo || !bsp.num_planes)
	{
		Q_strlcpy(failure_reason,
			"generated BSP is missing faces, brushes, texinfo, or planes",
			sizeof(failure_reason));
		goto fail;
	}
	bsp.nodes[bsp.num_nodes - 1].firstface = 0;
	bsp.nodes[bsp.num_nodes - 1].numfaces = bsp.num_faces;

	Q_strlcpy(failure_reason, "converting Doom player starts", sizeof(failure_reason));
	if (num_things > (((size_t)-1 - 128) / 128))
	{
		Q_strlcpy(failure_reason, "player-start entity buffer size overflow",
			sizeof(failure_reason));
		goto fail;
	}
	entity_capacity = 128 + num_things * 128;
	entities = malloc(entity_capacity);
	if (!entities)
	{
		Q_strlcpy(failure_reason, "allocating entity string", sizeof(failure_reason));
		goto fail;
	}
	entity_len = (size_t)snprintf((char *)entities, entity_capacity,
		"{\n\"classname\" \"worldspawn\"\n}\n");
	for (thing_index = 0; thing_index < num_things; thing_index++)
	{
		int thing_type, x, y, angle, subsector_number, sector_number;
		float z;

		thing_type = LittleShort(things[thing_index].type);
		if (thing_type != 1)
		{
			continue;
		}
		x = LittleShort(things[thing_index].x);
		y = LittleShort(things[thing_index].y);
		angle = LittleShort(things[thing_index].angle);
		subsector_number = 0;
		if (num_nodes)
		{
			int current_node;
		size_t depth;

			current_node = num_nodes - 1;
			for (depth = 0; depth <= num_nodes; depth++)
			{
				const doom_node_t *node;
				float dx, dy, line_dx, line_dy;
				int child;

				if ((current_node < 0) || ((size_t)current_node >= num_nodes))
				{
					snprintf(failure_reason, sizeof(failure_reason),
						"player thing %d has invalid BSP node %d",
						(int)thing_index, current_node);
					goto fail;
				}
				node = &nodes[current_node];
				dx = x - LittleShort(node->x);
				dy = y - LittleShort(node->y);
				line_dx = LittleShort(node->dx);
				line_dy = LittleShort(node->dy);
				child = LittleShort(node->children[(line_dx * dy - line_dy * dx) < 0.0f ? 0 : 1]);
				if (child & 0x8000)
				{
					subsector_number = child & 0x7FFF;
					break;
				}
				current_node = child;
			}
		}
		if ((subsector_number < 0) || ((size_t)subsector_number >= num_subsectors))
		{
			snprintf(failure_reason, sizeof(failure_reason),
				"player thing %d resolves to invalid subsector %d",
				(int)thing_index, subsector_number);
			goto fail;
		}
		{
			int firstseg;
			doom_seg_t spawn_seg;
			doom_linedef_t spawn_line;
			int spawn_side;

			firstseg = LittleShort(subsectors[subsector_number].firstseg);
			spawn_seg = segs[firstseg];
			spawn_line = lines[LittleShort(spawn_seg.linedef)];
			spawn_side = LittleShort(spawn_line.sidenum[LittleShort(spawn_seg.side)]);
			sector_number = LittleShort(sides[spawn_side].sector);
		}
		z = LittleShort(sectors[sector_number].floorheight) + 24.0f;
		if (entity_len >= entity_capacity)
		{
			snprintf(failure_reason, sizeof(failure_reason),
				"entity string exceeds capacity at player thing %d",
				(int)thing_index);
			goto fail;
		}
		{
			int written;

			written = snprintf((char *)entities + entity_len,
				entity_capacity - entity_len,
				"{\n\"classname\" \"info_player_start\"\n"
				"\"origin\" \"%d %d %.0f\"\n\"angle\" \"%d\"\n}\n",
				x, y, z, angle);
			if ((written < 0) || ((size_t)written >= entity_capacity - entity_len))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"entity string overflow at player thing %d", (int)thing_index);
				goto fail;
			}
			entity_len += written;
		}
	}
	entity_len++;

	Q_strlcpy(failure_reason, "serializing generated QBSP", sizeof(failure_reason));
	{
		darea_t area;
		dareaportal_t portal;
		dmodel_t model;
		const void *lump_data[HEADER_LUMPS] = {0};
		size_t lump_count[HEADER_LUMPS] = {0};
		size_t lump_size[HEADER_LUMPS] = {0};
		dheader_t *header;
		int lump_index;

		memset(&area, 0, sizeof(area));
		memset(&portal, 0, sizeof(portal));
		memset(&model, 0, sizeof(model));
		for (thing_index = 0; thing_index < 3; thing_index++)
		{
			model.mins[thing_index] = world_min[thing_index];
			model.maxs[thing_index] = world_max[thing_index];
			model.origin[thing_index] = (world_min[thing_index] + world_max[thing_index]) * 0.5f;
		}
		model.headnode = bsp.num_nodes - 1;
		model.numfaces = bsp.num_faces;

		lump_data[LUMP_ENTITIES] = entities;
		lump_count[LUMP_ENTITIES] = entity_len;
		lump_size[LUMP_ENTITIES] = 1;
		lump_data[LUMP_PLANES] = bsp.planes;
		lump_count[LUMP_PLANES] = bsp.num_planes;
		lump_size[LUMP_PLANES] = sizeof(dplane_t);
		lump_data[LUMP_VERTEXES] = bsp.vertexes;
		lump_count[LUMP_VERTEXES] = bsp.num_vertexes;
		lump_size[LUMP_VERTEXES] = sizeof(dvertex_t);
		lump_data[LUMP_NODES] = bsp.nodes;
		lump_count[LUMP_NODES] = bsp.num_nodes;
		lump_size[LUMP_NODES] = sizeof(dqnode_t);
		lump_data[LUMP_TEXINFO] = bsp.texinfo;
		lump_count[LUMP_TEXINFO] = bsp.num_texinfo;
		lump_size[LUMP_TEXINFO] = sizeof(xtexinfo_t);
		lump_data[LUMP_FACES] = bsp.faces;
		lump_count[LUMP_FACES] = bsp.num_faces;
		lump_size[LUMP_FACES] = sizeof(dqface_t);
		lump_data[LUMP_LEAFS] = bsp.leafs;
		lump_count[LUMP_LEAFS] = bsp.num_leafs;
		lump_size[LUMP_LEAFS] = sizeof(dqleaf_t);
		lump_data[LUMP_LEAFFACES] = bsp.leaffaces;
		lump_count[LUMP_LEAFFACES] = bsp.num_leaffaces;
		lump_size[LUMP_LEAFFACES] = sizeof(int);
		lump_data[LUMP_LEAFBRUSHES] = bsp.leafbrushes;
		lump_count[LUMP_LEAFBRUSHES] = bsp.num_leafbrushes;
		lump_size[LUMP_LEAFBRUSHES] = sizeof(int);
		lump_data[LUMP_EDGES] = bsp.edges;
		lump_count[LUMP_EDGES] = bsp.num_edges;
		lump_size[LUMP_EDGES] = sizeof(dqedge_t);
		lump_data[LUMP_SURFEDGES] = bsp.surfedges;
		lump_count[LUMP_SURFEDGES] = bsp.num_surfedges;
		lump_size[LUMP_SURFEDGES] = sizeof(int);
		lump_data[LUMP_MODELS] = &model;
		lump_count[LUMP_MODELS] = 1;
		lump_size[LUMP_MODELS] = sizeof(dmodel_t);
		lump_data[LUMP_BRUSHES] = bsp.brushes;
		lump_count[LUMP_BRUSHES] = bsp.num_brushes;
		lump_size[LUMP_BRUSHES] = sizeof(dbrush_t);
		lump_data[LUMP_BRUSHSIDES] = bsp.brushsides;
		lump_count[LUMP_BRUSHSIDES] = bsp.num_brushsides;
		lump_size[LUMP_BRUSHSIDES] = sizeof(dqbrushside_t);
		lump_data[LUMP_AREAS] = &area;
		lump_count[LUMP_AREAS] = 1;
		lump_size[LUMP_AREAS] = sizeof(darea_t);
		lump_data[LUMP_AREAPORTALS] = &portal;
		lump_count[LUMP_AREAPORTALS] = 1;
		lump_size[LUMP_AREAPORTALS] = sizeof(dareaportal_t);

		output_size = sizeof(dheader_t);
		for (lump_index = 0; lump_index < HEADER_LUMPS; lump_index++)
		{
			size_t bytes;

			if (lump_count[lump_index] > ((size_t)-1 / (lump_size[lump_index] ? lump_size[lump_index] : 1)))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"output lump %d size overflows", lump_index);
				goto fail;
			}
			bytes = lump_count[lump_index] * lump_size[lump_index];
			if (output_size > ((size_t)-1 - bytes - 3))
			{
				snprintf(failure_reason, sizeof(failure_reason),
					"output lump %d exceeds addressable size", lump_index);
				goto fail;
			}
			output_size = (output_size + 3) & ~(size_t)3;
			output_size += bytes;
		}
		if (output_size > 0xFFFFFFFFu)
		{
			Q_strlcpy(failure_reason, "generated QBSP exceeds 32-bit lump offsets",
				sizeof(failure_reason));
			goto fail;
		}
		outbuf = calloc(1, output_size);
		if (!outbuf)
		{
			Q_strlcpy(failure_reason, "allocating generated QBSP buffer",
				sizeof(failure_reason));
			goto fail;
		}
		header = (dheader_t *)outbuf;
		header->ident = QBSPHEADER;
		header->version = BSPVERSION;
		offset = sizeof(*header);
		for (lump_index = 0; lump_index < HEADER_LUMPS; lump_index++)
		{
			Mod_DoomSetLump(outbuf, header, lump_index, lump_data[lump_index],
				lump_count[lump_index], lump_size[lump_index], &offset);
		}
	}

	*out_len = output_size;
	free(entities);
	Mod_DoomFree(&bsp);
	return outbuf;

fail:
	free(outbuf);
	free(entities);
	Mod_DoomFree(&bsp);
	Com_Error(ERR_DROP, "%s: Map %s has invalid or unsupported Doom geometry: %s",
		__func__, name, failure_reason);
	return NULL;
}

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
		case map_doom: maptypename = "Doom"; break;
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
	else if (ident == PWADHEADER && version == 0)
	{
		*rules = doombsplumps;
		*numrules = *numlumps = ARRLEN(doom_lumps);
		return map_doom;
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
				rules ? result_size : filesize);

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
	if (detected_maptype == map_doom)
	{
		*maptype = map_doom;
		return Mod_Load2QBSP_Doom(name, inbuf, filesize, lumps, out_len);
	}

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

/*
 * Combine separate Doom map lumps into a single file structure expected by Mod_Load2QBSP:
 * - IDENT (4 bytes)
 * - VERSION (4 bytes)
 * - Lump list with offset and length for each lump
 * - Lump contents
 */
int
Mod_CombineLumps(const char *name, void **buffer)
{
	char dir[MAX_QPATH];
	const char *basename;
	byte *lumpdata[ARRLEN(doom_lumps)];
	int lumpsize[ARRLEN(doom_lumps)];
	qboolean lumpexists[ARRLEN(doom_lumps)];
	int lumppos[ARRLEN(doom_lumps)];
	size_t i;
	int num_found = 0;
	size_t data_ofs;
	size_t total_size;
	byte *outbuf;
	lump_t *outlumps;
	qboolean has_essential_lump = false;

	if (!name || !name[0] || !buffer)
	{
		return -1;
	}

	*buffer = NULL;

	COM_StripExtension(name, dir);
	basename = COM_SkipPath(dir);
	if (!basename || !basename[0])
	{
		return -1;
	}

	for (i = 0; i < ARRLEN(doom_lumps); i++)
	{
		char lumppath[MAX_QPATH];
		int len;

		lumpdata[i] = NULL;
		lumpsize[i] = 0;
		lumpexists[i] = false;
		lumppos[i] = 0;

		Com_sprintf(lumppath, sizeof(lumppath), "%s/%s.lmp", dir, doom_lumps[i]);
		len = FS_LoadFile(lumppath, (void **)&lumpdata[i]);

		if (len >= 0)
		{
			lumpexists[i] = true;
			lumpsize[i] = len;
			num_found++;

			if (!strcmp(doom_lumps[i], "VERTEXES") ||
				!strcmp(doom_lumps[i], "LINEDEFS") ||
				!strcmp(doom_lumps[i], "SECTORS") ||
				!strcmp(doom_lumps[i], "NODES"))
			{
				has_essential_lump = true;
			}
		}
	}

	if (num_found == 0 || !has_essential_lump)
	{
		for (i = 0; i < ARRLEN(doom_lumps); i++)
		{
			if (lumpdata[i])
			{
				FS_FreeFile(lumpdata[i]);
			}
		}
		return -1;
	}

	/*
	 * Structure:
	 * 1. ident (4 bytes) + version (4 bytes)
	 * 2. lump_t array [ARRLEN(doom_lumps)]
	 * 3. Lump contents
	 */
	data_ofs = sizeof(int) * 2 + sizeof(lump_t) * ARRLEN(doom_lumps);
	data_ofs = (data_ofs + 3) & ~3;

	for (i = 0; i < ARRLEN(doom_lumps); i++)
	{
		if (lumpexists[i] && lumpsize[i] > 0)
		{
			data_ofs = (data_ofs + 3) & ~3;
			lumppos[i] = (int)data_ofs;
			data_ofs += lumpsize[i];
		}
		else
		{
			lumppos[i] = 0;
			lumpsize[i] = 0;
		}
	}

	data_ofs = (data_ofs + 3) & ~3;
	total_size = data_ofs;

	outbuf = Z_Malloc(total_size);
	if (!outbuf)
	{
		for (i = 0; i < ARRLEN(doom_lumps); i++)
		{
			if (lumpdata[i])
			{
				FS_FreeFile(lumpdata[i]);
			}
		}
		return -1;
	}

	/* Write IDENT and VERSION */
	((int *)outbuf)[0] = LittleLong(PWADHEADER);
	((int *)outbuf)[1] = LittleLong(0);

	/* Write lump list: offset and length for each lump */
	outlumps = (lump_t *)((int *)outbuf + 2);
	for (i = 0; i < ARRLEN(doom_lumps); i++)
	{
		outlumps[i].fileofs = LittleLong(lumppos[i]);
		outlumps[i].filelen = LittleLong(lumpsize[i]);
	}

	/* Copy content of each lump */
	for (i = 0; i < ARRLEN(doom_lumps); i++)
	{
		if (lumpexists[i] && lumpsize[i] > 0 && lumpdata[i])
		{
			memcpy(outbuf + lumppos[i], lumpdata[i], lumpsize[i]);
		}
	}

	for (i = 0; i < ARRLEN(doom_lumps); i++)
	{
		if (lumpdata[i])
		{
			FS_FreeFile(lumpdata[i]);
		}
	}

	*buffer = outbuf;
	return total_size;
}
