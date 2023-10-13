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

static void
BSPX_LightGridValue(const bspxlightgrid_t *grid, const lightstyle_t *lightstyles,
	const vec3_t point, vec3_t res_diffuse)
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

static int
R_RecursiveLightPoint(const msurface_t *surfaces, const mnode_t *node,
	const lightstyle_t *lightstyles, const vec3_t start, const vec3_t end,
	vec3_t pointcolor, vec3_t lightspot, float modulate)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	const msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;

	if (node->contents != CONTENTS_NODE)
	{
		return -1;     /* didn't hit anything */
	}

	/* calculate mid point */
	plane = node->plane;
	front = DotProduct(start, plane->normal) - plane->dist;
	back = DotProduct(end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
	{
		return R_RecursiveLightPoint(surfaces, node->children[side],
			lightstyles, start, end, pointcolor, lightspot, modulate);
	}

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	/* go down front side */
	r = R_RecursiveLightPoint(surfaces, node->children[side],
		lightstyles, start, mid, pointcolor, lightspot, modulate);
	if (r >= 0)
	{
		return r;     /* hit something */
	}

	if ((back < 0) == side)
	{
		return -1;     /* didn't hit anuthing */
	}

	/* check for impact on this node */
	VectorCopy(mid, lightspot);

	surf = surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
		{
			continue; /* no lightmaps */
		}

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];

		if ((s < surf->texturemins[0]) ||
			(t < surf->texturemins[1]))
		{
			continue;
		}

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if ((ds > surf->extents[0]) || (dt > surf->extents[1]))
		{
			continue;
		}

		if (!surf->samples)
		{
			return 0;
		}

		ds >>= surf->lmshift;
		dt >>= surf->lmshift;

		lightmap = surf->samples;
		VectorCopy(vec3_origin, pointcolor);

		lightmap += 3 * (dt * ((surf->extents[0] >> surf->lmshift) + 1) + ds);

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			const float *rgb;
			int j;

			rgb = lightstyles[surf->styles[maps]].rgb;

			/* Apply light level to models */
			for (j = 0; j < 3; j++)
			{
				float	scale;

				scale = rgb[j] * modulate;
				pointcolor[j] += lightmap[j] * scale * (1.0 / 255);
			}

			lightmap += 3 * ((surf->extents[0] >> surf->lmshift) + 1) *
						((surf->extents[1] >> surf->lmshift) + 1);
		}

		return 1;
	}

	/* go down back side */
	return R_RecursiveLightPoint(surfaces, node->children[!side],
		lightstyles, mid, end, pointcolor, lightspot, modulate);
}

void
R_LightPoint(const bspxlightgrid_t *grid, const entity_t *currententity,
	refdef_t *refdef, const msurface_t *surfaces, const mnode_t *nodes,
	vec3_t p, vec3_t color, float modulate, vec3_t lightspot)
{
	vec3_t end, dist, pointcolor = {0, 0, 0};
	float r;
	int lnum;
	dlight_t *dl;

	if (!currententity)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	if (grid)
	{
		BSPX_LightGridValue(grid, refdef->lightstyles,
			currententity->origin, color);
		return;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = R_RecursiveLightPoint(surfaces, nodes, refdef->lightstyles,
		p, end, pointcolor, lightspot, modulate);

	if (r == -1)
	{
		VectorCopy(vec3_origin, color);
	}
	else
	{
		VectorCopy(pointcolor, color);
	}

	/* add dynamic lights */
	dl = refdef->dlights;

	for (lnum = 0; lnum < refdef->num_dlights; lnum++, dl++)
	{
		float	add;

		VectorSubtract(currententity->origin,
				dl->origin, dist);
		add = dl->intensity - VectorLength(dist);
		add *= (1.0f / 256.0f);

		if (add > 0)
		{
			VectorMA(color, add, dl->color, color);
		}
	}

	VectorScale(color, modulate, color);
}

void
R_SetCacheState(msurface_t *surf, refdef_t *refdef)
{
	int maps;

	for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
		 maps++)
	{
		surf->cached_light[maps] =
			refdef->lightstyles[surf->styles[maps]].white;
	}
}
