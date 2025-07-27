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
 * Surface logic
 *
 * =======================================================================
 */

#include "../ref_shared.h"

#define SUBDIVIDE_SIZE 64.0f
#define MAX_SUBDIVIDE_VERTS 60

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
struct image_s *
R_TextureAnimation(const entity_t *currententity, const mtexinfo_t *tex)
{
	int c;

	if (!tex->next)
	{
		return tex->image;
	}

	if (!currententity)
	{
		return tex->image;
	}

	c = currententity->frame % tex->numframes;
	while (c && tex)
	{
		tex = tex->next;
		c--;
	}

	return tex ? tex->image : NULL;
}

qboolean
R_AreaVisible(const byte *areabits, const mleaf_t *pleaf)
{
	int area;

	// check for door connected areas
	if (!areabits)
	{
		return true;
	}

	area = pleaf->area;

	if ((areabits[area >> 3] & (1 << (area & 7))))
	{
		return true;
	}

	return false; // not visible
}

/*
 * Returns true if the box is completely outside the frustom
 */
qboolean
R_CullBox(vec3_t mins, vec3_t maxs, const cplane_t *frustum)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (BOX_ON_PLANE_SIDE(mins, maxs, frustum + i) == 2)
		{
			return true;
		}
	}

	return false;
}

static int
R_SignbitsForPlane(const cplane_t *out)
{
	int bits, j;

	/* for fast box on planeside test */
	bits = 0;

	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
		{
			bits |= 1 << j;
		}
	}

	return bits;
}

void
R_SetFrustum(vec3_t vup, vec3_t vpn, vec3_t vright, vec3_t r_origin,
	float fov_x, float fov_y, cplane_t *frustum)
{
	int i;

	/* rotate VPN right by FOV_X/2 degrees */
	RotatePointAroundVector(frustum[0].normal, vup, vpn,
			-(90 - fov_x / 2));
	/* rotate VPN left by FOV_X/2 degrees */
	RotatePointAroundVector(frustum[1].normal,
			vup, vpn, 90 - fov_x / 2);
	/* rotate VPN up by FOV_X/2 degrees */
	RotatePointAroundVector(frustum[2].normal,
			vright, vpn, 90 - fov_y / 2);
	/* rotate VPN down by FOV_X/2 degrees */
	RotatePointAroundVector(frustum[3].normal, vright, vpn,
			-(90 - fov_y / 2));

#if defined(__GNUC__)
#       pragma GCC unroll 4
#endif
	for (i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = R_SignbitsForPlane(&frustum[i]);
	}
}

static void
R_BoundPoly(int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int i, j;
	float *v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;

	for (i = 0; i < numverts; i++)
	{
		for (j = 0; j < 3; j++, v++)
		{
			if (*v < mins[j])
			{
				mins[j] = *v;
			}

			if (*v > maxs[j])
			{
				maxs[j] = *v;
			}
		}
	}
}

static void
R_SubdividePolygon(int numverts, float *verts, msurface_t *warpface)
{
	int i, j, k;
	vec3_t mins, maxs;
	float *v;
	vec3_t front[64], back[64];
	int f, b;
	float dist[64];
	float frac;
	mpoly_t *poly;
	vec3_t total;
	float total_s, total_t;
	vec3_t normal;

	VectorCopy(warpface->plane->normal, normal);

	if (numverts > MAX_SUBDIVIDE_VERTS)
	{
		Com_Error(ERR_DROP, "%s: numverts = %i", __func__, numverts);
	}

	R_BoundPoly(numverts, verts, mins, maxs);

	for (i = 0; i < 3; i++)
	{
		float m;

		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor(m / SUBDIVIDE_SIZE + 0.5);

		if (maxs[i] - m < 8)
		{
			continue;
		}

		if (m - mins[i] < 8)
		{
			continue;
		}

		/* cut it */
		v = verts + i;

		for (j = 0; j < numverts; j++, v += 3)
		{
			dist[j] = *v - m;
		}

		/* wrap cases */
		dist[j] = dist[0];
		v -= i;
		VectorCopy(verts, v);

		f = b = 0;
		v = verts;

		for (j = 0; j < numverts; j++, v += 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy(v, front[f]);
				f++;
			}

			if (dist[j] <= 0)
			{
				VectorCopy(v, back[b]);
				b++;
			}

			if ((dist[j] == 0) || (dist[j + 1] == 0))
			{
				continue;
			}

			if ((dist[j] > 0) != (dist[j + 1] > 0))
			{
				/* clip point */
				frac = dist[j] / (dist[j] - dist[j + 1]);

				for (k = 0; k < 3; k++)
				{
					front[f][k] = back[b][k] = v[k] + frac * (v[3 + k] - v[k]);
				}

				f++;
				b++;
			}
		}

		R_SubdividePolygon(f, front[0], warpface);
		R_SubdividePolygon(b, back[0], warpface);
		return;
	}

	/* add a point in the center to help keep warp valid */
	poly = Hunk_Alloc(sizeof(mpoly_t) + ((numverts - 4) + 2) * sizeof(mvtx_t));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts + 2;
	VectorClear(total);
	total_s = 0;
	total_t = 0;

	for (i = 0; i < numverts; i++, verts += 3)
	{
		float s, t;

		VectorCopy(verts, poly->verts[i + 1].pos);
		s = DotProduct(verts, warpface->texinfo->vecs[0]);
		t = DotProduct(verts, warpface->texinfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd(total, verts, total);

		poly->verts[i + 1].texCoord[0] = s;
		poly->verts[i + 1].texCoord[1] = t;
		VectorCopy(normal, poly->verts[i + 1].normal);
		poly->verts[i + 1].lightFlags = 0;
	}

	VectorScale(total, (1.0 / numverts), poly->verts[0].pos);
	poly->verts[0].texCoord[0] = total_s / numverts;
	poly->verts[0].texCoord[1] = total_t / numverts;
	VectorCopy(normal, poly->verts[0].normal);

	/* copy first vertex to last */
	memcpy(&poly->verts[i + 1], &poly->verts[1], sizeof(mvtx_t));
}

/*
 * Breaks a polygon up along axial 64 unit
 * boundaries so that turbulent and sky warps
 * can be done reasonably.
 */
void
R_SubdivideSurface(const int *surfedges, mvertex_t *vertexes, medge_t *edges,
	msurface_t *fa)
{
	const float *vec;
	vec3_t verts[64];
	int numverts;
	int i;

	/* convert edges back to a normal polygon */
	numverts = 0;

	for (i = 0; i < fa->numedges; i++)
	{
		int	lindex;

		lindex = surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			vec = vertexes[edges[lindex].v[0]].position;
		}
		else
		{
			vec = vertexes[edges[-lindex].v[1]].position;
		}

		VectorCopy(vec, verts[numverts]);
		numverts++;
	}

	R_SubdividePolygon(numverts, verts[0], fa);
}
