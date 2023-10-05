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
 * Light logic
 *
 * =======================================================================
 */

#include "../ref_shared.h"

static int
BSPX_LightGridSingleValue(const bspxlightgrid_t *grid, const lightstyle_t *lightstyles, int x, int y, int z, vec3_t res_diffuse)
{
	int i;
	unsigned int node;
	struct bspxlgsamp_s *samp;

	node = grid->rootnode;
	while (!(node & LGNODE_LEAF))
	{
		struct bspxlgnode_s *n;
		if (node & LGNODE_MISSING)
			return 0;	//failure
		n = grid->nodes + node;
		node = n->child[
				((x>=n->mid[0])<<2)|
				((y>=n->mid[1])<<1)|
				((z>=n->mid[2])<<0)];
	}

	{
		struct bspxlgleaf_s *leaf = &grid->leafs[node & ~LGNODE_LEAF];
		x -= leaf->mins[0];
		y -= leaf->mins[1];
		z -= leaf->mins[2];
		if (x >= leaf->size[0] ||
			y >= leaf->size[1] ||
			z >= leaf->size[2])
			return 0;	//sample we're after is out of bounds...

		i = x + leaf->size[0]*(y + leaf->size[1]*z);
		samp = leaf->rgbvalues + i;

		//no hdr support
		for (i = 0; i < 4; i++)
		{
			if (samp->map[i].style == ((byte)(~0u)))
				break;	//no more
			res_diffuse[0] += samp->map[i].rgb[0] * lightstyles[samp->map[i].style].rgb[0] / 255.0;
			res_diffuse[1] += samp->map[i].rgb[1] * lightstyles[samp->map[i].style].rgb[1] / 255.0;
			res_diffuse[2] += samp->map[i].rgb[2] * lightstyles[samp->map[i].style].rgb[2] / 255.0;
		}
	}
	return 1;
}

void
BSPX_LightGridValue(const bspxlightgrid_t *grid, const lightstyle_t *lightstyles, const vec3_t point, vec3_t res_diffuse)
{
	int tile[3];
	int i;
	int s;

	VectorSet(res_diffuse, 0, 0, 0);	//assume worst

	for (i = 0; i < 3; i++)
		tile[i] = (point[i] - grid->mins[i]) * grid->gridscale[i];

	for (i = 0, s = 0; i < 8; i++)
		s += BSPX_LightGridSingleValue(grid, lightstyles,
			tile[0]+!!(i&1),
			tile[1]+!!(i&2),
			tile[2]+!!(i&4), res_diffuse);

	VectorScale(res_diffuse, 1.0/s, res_diffuse);	//average the successful ones
}
