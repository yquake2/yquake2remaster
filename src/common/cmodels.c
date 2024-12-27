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
 * The common models file format
 *
 * =======================================================================
 */

#include "header/common.h"
#include "header/cmodel.h"

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
		Com_DPrintf("%s: Map %s has too small visibility lump\n",
			__func__, name);
		*vis = NULL;
		*numvisibility = 0;
		return;
	}

	*numvisibility = l->filelen;

	out = Hunk_Alloc((l->filelen + 63) & ~63);
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
===================
Mod_DecompressVis
===================
*/
void
Mod_DecompressVis(const byte *in, byte *out, const byte* numvisibility, int row)
{
	byte *out_p;

	out_p = out;

	if (!in || !numvisibility)
	{
		/* no vis info, so make all visible */
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}

		return;
	}

	do
	{
		int c;

		if (((in + 2) < numvisibility) && *in)
		{
			*out_p++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;

		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf("%s: warning: Vis decompression overrun\n", __func__);
		}

		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	}
	while (out_p - out < row);
}

float
Mod_RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	int i;
	vec3_t corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength(corner);
}
