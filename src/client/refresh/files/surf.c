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

int r_viewcluster, r_oldviewcluster;
static int r_viewcluster2, r_oldviewcluster2;
int r_visframecount; /* bumped when going to a new PVS */
int r_currentkey;
static cplane_t frustum[4];
clipplane_t view_clipplanes[4];
int *pfrustum_indexes[4];
static int r_frustum_indexes[4 * 6];

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
R_CullBox(vec3_t mins, vec3_t maxs)
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
	float fov_x, float fov_y)
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
	float dist[64] = {0};
	float frac;
	mpoly_t *poly;
	vec3_t total;
	float total_s, total_t;
	vec3_t normal;

	VectorCopy(warpface->plane->normal, normal);

	if (numverts > MAX_SUBDIVIDE_VERTS)
	{
		Com_Error(ERR_DROP, "%s: numverts = %i", __func__, numverts);
		return;
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

/*
 * Mark the leaves and nodes that are
 * in the PVS for the current cluster
 */
void
R_MarkLeaves(const model_t *r_worldmodel)
{
	const byte *vis;
	byte *fatvis = NULL;
	mnode_t *node;
	int i;
	mleaf_t *leaf;

	if ((r_oldviewcluster == r_viewcluster) &&
		(r_oldviewcluster2 == r_viewcluster2) &&
		!r_novis->value &&
		(r_viewcluster != -1))
	{
		return;
	}

	/* development aid to let you run around
	   and see exactly where the pvs ends */
	if (r_lockpvs->value)
	{
		return;
	}

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || (r_viewcluster == -1) || !r_worldmodel->vis)
	{
		/* mark everything */
		for (i = 0; i < r_worldmodel->numleafs; i++)
		{
			r_worldmodel->leafs[i].visframe = r_visframecount;
		}

		for (i = 0; i < r_worldmodel->numnodes; i++)
		{
			r_worldmodel->nodes[i].visframe = r_visframecount;
		}

		return;
	}

	vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel);

	/* may have to combine two clusters because of solid water boundaries */
	if (r_viewcluster2 != r_viewcluster)
	{
		int c;

		fatvis = malloc(((r_worldmodel->numleafs + 31) / 32) * sizeof(int));
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;

		for (i = 0; i < c; i++)
		{
			((int *)fatvis)[i] |= ((int *)vis)[i];
		}

		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs;
		 i < r_worldmodel->numleafs;
		 i++, leaf++)
	{
		int cluster;

		cluster = leaf->cluster;

		if (cluster == -1)
		{
			continue;
		}

		if (vis[cluster >> 3] & (1 << (cluster & 7)))
		{
			node = (mnode_t *)leaf;

			do
			{
				if (node->visframe == r_visframecount)
				{
					break;
				}

				node->visframe = r_visframecount;
				node = node->parent;
			}
			while (node);
		}
	}

	/* clean combined buffer */
	if (fatvis)
	{
		free(fatvis);
	}
}

void
R_SetClusters(const model_t *r_worldmodel, const vec3_t r_origin)
{
	/* current viewcluster */
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		const mleaf_t *leaf;

		if (!r_worldmodel)
		{
			Com_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel->nodes);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
	}
}

static qboolean
R_CullAliasMeshModel(dmdx_t *paliashdr, int frame, int oldframe, vec3_t e_angles,
	vec3_t e_origin, vec3_t bbox[8])
{
	int i;
	vec3_t mins, maxs;
	vec3_t vectors[3];
	vec3_t thismins, oldmins, thismaxs, oldmaxs;
	daliasxframe_t *pframe, *poldframe;
	vec3_t angles;

	pframe = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames +
			frame * paliashdr->framesize);

	poldframe = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames +
			oldframe * paliashdr->framesize);

	/* compute axially aligned mins and maxs */
	if (pframe == poldframe)
	{
		for (i = 0; i < 3; i++)
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i] * 0xFFFF;
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i] * 0xFFFF;

			oldmins[i] = poldframe->translate[i];
			oldmaxs[i] = oldmins[i] + poldframe->scale[i] * 0xFFFF;

			if (thismins[i] < oldmins[i])
			{
				mins[i] = thismins[i];
			}
			else
			{
				mins[i] = oldmins[i];
			}

			if (thismaxs[i] > oldmaxs[i])
			{
				maxs[i] = thismaxs[i];
			}
			else
			{
				maxs[i] = oldmaxs[i];
			}
		}
	}

	/* compute a full bounding box */
	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		if (i & 1)
		{
			tmp[0] = mins[0];
		}
		else
		{
			tmp[0] = maxs[0];
		}

		if (i & 2)
		{
			tmp[1] = mins[1];
		}
		else
		{
			tmp[1] = maxs[1];
		}

		if (i & 4)
		{
			tmp[2] = mins[2];
		}
		else
		{
			tmp[2] = maxs[2];
		}

		VectorCopy(tmp, bbox[i]);
	}

	/* rotate the bounding box */
	VectorCopy(e_angles, angles);
	angles[YAW] = -angles[YAW];
	AngleVectors(angles, vectors[0], vectors[1], vectors[2]);

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		VectorCopy(bbox[i], tmp);

		bbox[i][0] = DotProduct(vectors[0], tmp);
		bbox[i][1] = -DotProduct(vectors[1], tmp);
		bbox[i][2] = DotProduct(vectors[2], tmp);

		VectorAdd(e_origin, bbox[i], bbox[i]);
	}

	int p, f, aggregatemask = ~0;

	for (p = 0; p < 8; p++)
	{
		int mask = 0;

		for (f = 0; f < 4; f++)
		{
			float dp = DotProduct(frustum[f].normal, bbox[p]);

			if ((dp - frustum[f].dist) < 0)
			{
				mask |= (1 << f);
			}
		}

		aggregatemask &= mask;
	}

	if (aggregatemask)
	{
		return true;
	}

	return false;
}

qboolean
R_CullAliasModel(const model_t *currentmodel, vec3_t bbox[8], entity_t *e)
{
	dmdx_t *paliashdr;

	paliashdr = (dmdx_t *)currentmodel->extradata;
	if (!paliashdr)
	{
		Com_Printf("%s %s: Model is not fully loaded\n",
				__func__, currentmodel->name);
		return true;
	}

	if ((e->frame >= paliashdr->num_frames) || (e->frame < 0))
	{
		Com_DPrintf("%s %s: no such frame %d\n",
				__func__, currentmodel->name, e->frame);
		e->frame = 0;
	}

	if ((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0))
	{
		Com_DPrintf("%s %s: no such oldframe %d\n",
				__func__, currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	return R_CullAliasMeshModel(paliashdr, e->frame, e->oldframe,
		e->angles, e->origin, bbox);
}

void
R_TransformFrustum(vec3_t modelorg, vec3_t vright, vec3_t vup, vec3_t vpn)
{
	vec3_t v, v2;
	size_t i;

	for (i = 0; i < 4; i++)
	{
		v[0] = screenedge[i].normal[2];
		v[1] = -screenedge[i].normal[0];
		v[2] = screenedge[i].normal[1];

		v2[0] = v[1] * vright[0] + v[2] * vup[0] + v[0] * vpn[0];
		v2[1] = v[1] * vright[1] + v[2] * vup[1] + v[0] * vpn[1];
		v2[2] = v[1] * vright[2] + v[2] * vup[2] + v[0] * vpn[2];

		VectorCopy(v2, view_clipplanes[i].normal);

		view_clipplanes[i].dist = DotProduct(modelorg, v2);
	}
}

void
R_SetUpFrustumIndexes(void)
{
	int i, *pindex;

	pindex = r_frustum_indexes;

	for (i = 0; i < 4; i++)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			if (view_clipplanes[i].normal[j] < 0)
			{
				pindex[j] = j;
				pindex[j + 3] = j + 3;
			}
			else
			{
				pindex[j] = j + 3;
				pindex[j + 3] = j;
			}
		}

		/* FIXME: do just once at start */
		pfrustum_indexes[i] = pindex;
		pindex += 6;
	}
}

void
SetupScreenEdge(void)
{
	/*
	 * at Z = 1.0, this many X is visible
	 * 2.0 = 90 degrees
	 */
	float verticalFieldOfView, horizontalFieldOfView;
	int i;

	horizontalFieldOfView = 2 * tan((float)r_newrefdef.fov_x / 360 * M_PI);
	verticalFieldOfView = 2 * tan((float)r_newrefdef.fov_y / 360 * M_PI);

	/* left side clip */
	screenedge[0].normal[0] = -1.0 / (XCENTERING * horizontalFieldOfView);
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;

	/* right side clip */
	screenedge[1].normal[0] = 1.0 / ((1.0 - XCENTERING) * horizontalFieldOfView);
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;

	/* top side clip */
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = -1.0 / (YCENTERING * verticalFieldOfView);
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;

	/* bottom side clip */
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = 1.0 / ((1.0 - YCENTERING) * verticalFieldOfView);
	screenedge[3].normal[2] = 1;
	screenedge[3].type = PLANE_ANYZ;

	for (i = 0; i < 4; i++)
	{
		VectorNormalize(screenedge[i].normal);
	}
}
