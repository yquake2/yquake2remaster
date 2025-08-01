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
 */

#include "header/local.h"

//
// current entity info
//
vec3_t		modelorg;	// modelorg is the viewpoint reletive to
				// the currently rendering entity
vec3_t		r_entorigin;	// the currently rendering entity in world
				// coordinates

static float	entity_rotation[3][3];

typedef enum {touchessolid, drawnode, nodrawnode} solidstate_t;

#define MAX_BMODEL_VERTS	500	// 6K
#define MAX_BMODEL_EDGES	1000	// 12K

static int		numbverts, numbedges;
static mvertex_t	bverts[MAX_BMODEL_VERTS];
static bedge_t		bedges[MAX_BMODEL_EDGES];

//===========================================================================

/*
================
R_EntityRotate
================
*/
static void
R_EntityRotate (vec3_t vec)
{
	vec3_t	tvec;

	VectorCopy (vec, tvec);
	vec[0] = DotProduct (entity_rotation[0], tvec);
	vec[1] = DotProduct (entity_rotation[1], tvec);
	vec[2] = DotProduct (entity_rotation[2], tvec);
}

/*
================
R_RotateBmodel
================
*/
void
R_RotateBmodel(const entity_t *currententity)
{
	float	angle, s, c, temp1[3][3], temp2[3][3], temp3[3][3];

	// TODO: should use a look-up table
	// TODO: should really be stored with the entity instead of being reconstructed
	// TODO: could cache lazily, stored in the entity
	// TODO: share work with R_SetUpAliasTransform

	// yaw
	angle = currententity->angles[YAW];
	angle = angle * M_PI*2 / 360;
	s = sin(angle);
	c = cos(angle);

	temp1[0][0] = c;
	temp1[0][1] = s;
	temp1[0][2] = 0;
	temp1[1][0] = -s;
	temp1[1][1] = c;
	temp1[1][2] = 0;
	temp1[2][0] = 0;
	temp1[2][1] = 0;
	temp1[2][2] = 1;


	// pitch
	angle = currententity->angles[PITCH];
	angle = angle * M_PI*2 / 360;
	s = sin(angle);
	c = cos(angle);

	temp2[0][0] = c;
	temp2[0][1] = 0;
	temp2[0][2] = -s;
	temp2[1][0] = 0;
	temp2[1][1] = 1;
	temp2[1][2] = 0;
	temp2[2][0] = s;
	temp2[2][1] = 0;
	temp2[2][2] = c;

	R_ConcatRotations (temp2, temp1, temp3);

	// roll
	angle = currententity->angles[ROLL];
	angle = angle * M_PI*2 / 360;
	s = sin(angle);
	c = cos(angle);

	temp1[0][0] = 1;
	temp1[0][1] = 0;
	temp1[0][2] = 0;
	temp1[1][0] = 0;
	temp1[1][1] = c;
	temp1[1][2] = s;
	temp1[2][0] = 0;
	temp1[2][1] = -s;
	temp1[2][2] = c;

	R_ConcatRotations (temp1, temp3, entity_rotation);

	//
	// rotate modelorg and the transformation matrix
	//
	R_EntityRotate (modelorg);
	R_EntityRotate (vpn);
	R_EntityRotate (vright);
	R_EntityRotate (vup);

	R_TransformFrustum ();
}

/*
================
R_RecursiveClipBPoly

Clip a bmodel poly down the world bsp tree
================
*/
static void
R_RecursiveClipBPoly(entity_t *currententity, bedge_t *pedges, mnode_t *pnode, msurface_t *psurf)
{
	bedge_t		*psideedges[2], *pnextedge;
	int		i, side, lastside;
	cplane_t	*splitplane, tplane;
	mvertex_t	*pvert, *plastvert, *ptvert;
	mnode_t		*pn;
	mvertex_t	*prevclipvert = NULL;

	psideedges[0] = psideedges[1] = NULL;

	// transform the BSP plane into model space
	// FIXME: cache these?
	splitplane = pnode->plane;
	tplane.dist = splitplane->dist -
			DotProduct(r_entorigin, splitplane->normal);
	tplane.normal[0] = DotProduct (entity_rotation[0], splitplane->normal);
	tplane.normal[1] = DotProduct (entity_rotation[1], splitplane->normal);
	tplane.normal[2] = DotProduct (entity_rotation[2], splitplane->normal);

	// clip edges to BSP plane
	for (; pedges; pedges = pnextedge)
	{
		float  dist, lastdist;
		bedge_t *ptedge;

		pnextedge = pedges->pnext;

		// set the status for the last point as the previous point
		// FIXME: cache this stuff somehow?
		plastvert = pedges->v[0];
		lastdist = DotProduct (plastvert->position, tplane.normal) - tplane.dist;
		if (lastdist >= 0)
			lastside = 0;
		else
			lastside = 1;

		pvert = pedges->v[1];
		dist = DotProduct (pvert->position, tplane.normal) - tplane.dist;
		if (dist >= 0)
			side = 0;
		else
			side = 1;

		if (side != lastside)
		{
			float	frac;

			// clipped
			if (numbverts >= MAX_BMODEL_VERTS)
				return;

			// generate the clipped vertex
			frac = lastdist / (lastdist - dist);
			ptvert = &bverts[numbverts++];
			ptvert->position[0] = plastvert->position[0] +
					frac * (pvert->position[0] -
					plastvert->position[0]);
			ptvert->position[1] = plastvert->position[1] +
					frac * (pvert->position[1] -
					plastvert->position[1]);
			ptvert->position[2] = plastvert->position[2] +
					frac * (pvert->position[2] -
					plastvert->position[2]);

			// split into two edges, one on each side, and remember entering
			// and exiting points
			// FIXME: share the clip edge by having a winding direction flag?
			if (numbedges + 4 > MAX_BMODEL_EDGES)
			{
				Com_Printf("Out of edges for bmodel\n");
				return;
			}

			// outside: last vert, clip vert
			ptedge = &bedges[numbedges++];
			ptedge->pnext = psideedges[lastside];
			psideedges[lastside] = ptedge;
			ptedge->v[0] = plastvert;
			ptedge->v[1] = ptvert;

			// each two clipped verts we get a clipped-off contour;
			// connect the verts by two edges (one per side)
			// going in opposite directions
			// this fixes surface 0 of model *50 (fan) in mine2:
			// teleport 1231 770 -579
			if (prevclipvert)
			{
				ptedge = &bedges[numbedges++];
				ptedge->pnext = psideedges[lastside];
				psideedges[lastside] = ptedge;
				ptedge->v[0] = ptvert;
				ptedge->v[1] = prevclipvert;

				ptedge = &bedges[numbedges++];
				ptedge->pnext = psideedges[side];
				psideedges[side] = ptedge;
				ptedge->v[0] = prevclipvert;
				ptedge->v[1] = ptvert;

				prevclipvert = NULL;
			}
			else
			{
				prevclipvert = ptvert;
			}

			// inside: clip vert, current vert
			ptedge = &bedges[numbedges++];
			ptedge->pnext = psideedges[side];
			psideedges[side] = ptedge;
			ptedge->v[0] = ptvert;
			ptedge->v[1] = pvert;
		}
		else
		{
			// add the edge to the appropriate side
			pedges->pnext = psideedges[side];
			psideedges[side] = pedges;
		}
	}

	// draw or recurse further
	for (i=0 ; i<2 ; i++)
	{
		if (psideedges[i])
		{
			// draw if we've reached a non-solid leaf, done if all that's left is a
			// solid leaf, and continue down the tree if it's not a leaf
			pn = pnode->children[i];

			// we're done with this branch if the node or leaf isn't in the PVS
			if (pn->visframe == r_visframecount)
			{
				if (pn->contents != CONTENTS_NODE)
				{
					if (pn->contents != CONTENTS_SOLID)
					{
						int r_currentbkey;

						if (!R_AreaVisible(r_newrefdef.areabits, (mleaf_t *)pn))
							continue;

						r_currentbkey = ((mleaf_t *)pn)->key;
						R_RenderBmodelFace(currententity, psideedges[i], psurf, r_currentbkey);
					}
				}
				else
				{
					R_RecursiveClipBPoly(currententity, psideedges[i], pnode->children[i],
									  psurf);
				}
			}
		}
	}
}

/*
================
R_DrawSolidClippedSubmodelPolygons

Bmodel crosses multiple leafs
================
*/
void
R_DrawSolidClippedSubmodelPolygons(entity_t *currententity, const model_t *currentmodel, mnode_t *topnode)
{
	int		i;
	msurface_t	*psurf;
	int		numsurfaces;
	medge_t		*pedges;

	// FIXME: use bounding-box-based frustum clipping info?
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	numsurfaces = currentmodel->nummodelsurfaces;
	pedges = currentmodel->edges;

	for (i = 0; i < numsurfaces; i++, psurf++)
	{
		cplane_t *pplane;
		bedge_t  *pbedge;
		vec_t    dot;
		int      j;

		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (( !(psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			((psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			continue;
		}

		// FIXME: use bounding-box-based frustum clipping info?

		// copy the edges to bedges, flipping if necessary so always
		// clockwise winding
		// FIXME: if edges and vertices get caches, these assignments must move
		// outside the loop, and overflow checking must be done here
		numbverts = numbedges = 0;
		pbedge = &bedges[numbedges];
		numbedges += psurf->numedges;

		for (j = 0; j < psurf->numedges; j++)
		{
			int	lindex;

			lindex = currentmodel->surfedges[psurf->firstedge+j];

			if (lindex > 0)
			{
				medge_t  *pedge;

				pedge = &pedges[lindex];
				pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[0]];
				pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[1]];
			}
			else
			{
				medge_t  *pedge;

				lindex = -lindex;
				pedge = &pedges[lindex];
				pbedge[j].v[0] = &r_pcurrentvertbase[pedge->v[1]];
				pbedge[j].v[1] = &r_pcurrentvertbase[pedge->v[0]];
			}

			pbedge[j].pnext = &pbedge[j+1];
		}

		pbedge[j-1].pnext = NULL; // mark end of edges

		if ( !( psurf->texinfo->flags & SURF_TRANSPARENT ))
		{
			R_RecursiveClipBPoly(currententity, pbedge, topnode, psurf);
		}
		else
		{
			R_RenderBmodelFace(currententity, pbedge, psurf, ((mleaf_t *)topnode)->key);
		}
	}
}


/*
================
R_DrawSubmodelPolygons

All in one leaf
================
*/
void
R_DrawSubmodelPolygons(entity_t *currententity, const model_t *currentmodel, int clipflags, mnode_t *topnode)
{
	int			i;
	msurface_t	*psurf;
	int numsurfaces;

	// FIXME: use bounding-box-based frustum clipping info?
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	numsurfaces = currentmodel->nummodelsurfaces;

	for (i=0 ; i<numsurfaces ; i++, psurf++)
	{
		vec_t dot;

		cplane_t *pplane;
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			r_currentkey = ((mleaf_t *)topnode)->key;

			// FIXME: use bounding-box-based frustum clipping info?
			R_RenderFace(currententity, currentmodel, psurf, clipflags, true);
		}
	}
}

/*
================
R_RecursiveWorldNode
================
*/
static void
R_RecursiveWorldNode (entity_t *currententity, const model_t *currentmodel, mnode_t *node,
	int clipflags, qboolean insubmodel)
{
	int c;
	vec3_t acceptpt, rejectpt;
	mleaf_t *pleaf;

	if (node->contents == CONTENTS_SOLID)
	{
		return;		// solid
	}

	if (node->visframe != r_visframecount)
	{
		return;
	}

	if (r_cull->value && R_CullBox(node->minmaxs, node->minmaxs + 3, frustum))
	{
		return;
	}

	// cull the clipping planes if not trivial accept
	// FIXME: the compiler is doing a lousy job of optimizing here; it could be
	//  twice as fast in ASM
	if (clipflags)
	{
		int i;
		for (i=0 ; i<4 ; i++)
		{
			const int *pindex;
			float d;

			if (!(clipflags & (1<<i)))
			{
				continue;	// don't need to clip against it
			}

			// generate accept and reject points
			// FIXME: do with fast look-ups or integer tests based on the sign bit
			// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = node->minmaxs[pindex[0]];
			rejectpt[1] = node->minmaxs[pindex[1]];
			rejectpt[2] = node->minmaxs[pindex[2]];

			d = DotProduct (rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;
			if (d <= 0)
				return;
			acceptpt[0] = node->minmaxs[pindex[3+0]];
			acceptpt[1] = node->minmaxs[pindex[3+1]];
			acceptpt[2] = node->minmaxs[pindex[3+2]];

			d = DotProduct (acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d >= 0)
				clipflags &= ~(1<<i);	// node is entirely on screen
		}
	}

	// if a leaf node, draw stuff
	if (node->contents != CONTENTS_NODE)
	{
		msurface_t **mark;
		pleaf = (mleaf_t *)node;

		if (!R_AreaVisible(r_newrefdef.areabits, pleaf))
			return;	// not visible

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		pleaf->key = r_currentkey;
		r_currentkey++;	// all bmodels in a leaf share the same key
	}
	else
	{
		float dot;
		int side;
		cplane_t *plane;

		// node is just a decision point, so go down the apropriate sides
		// find which side of the node we are on
		plane = node->plane;

		switch (plane->type)
		{
		case PLANE_X:
			dot = modelorg[0] - plane->dist;
			break;
		case PLANE_Y:
			dot = modelorg[1] - plane->dist;
			break;
		case PLANE_Z:
			dot = modelorg[2] - plane->dist;
			break;
		default:
			dot = DotProduct (modelorg, plane->normal) - plane->dist;
			break;
		}

		if (dot >= 0)
			side = 0;
		else
			side = 1;

		// recurse down the children, front side first
		R_RecursiveWorldNode (currententity, currentmodel, node->children[side], clipflags, insubmodel);

		if ((node->numsurfaces + node->firstsurface) > currentmodel->numsurfaces)
		{
			Com_Printf("Broken node firstsurface\n");
			return;
		}

		// draw stuff
		c = node->numsurfaces;

		if (c)
		{
			msurface_t *surf;

			surf = currentmodel->surfaces + node->firstsurface;

			if (dot < -BACKFACE_EPSILON)
			{
				do
				{
					if ((surf->flags & SURF_PLANEBACK) &&
						(surf->visframe == r_framecount))
					{
						R_RenderFace (currententity, currentmodel, surf, clipflags, insubmodel);
					}

					surf++;
				} while (--c);
			}
			else if (dot > BACKFACE_EPSILON)
			{
				do
				{
					if (!(surf->flags & SURF_PLANEBACK) &&
						(surf->visframe == r_framecount))
					{
						R_RenderFace (currententity, currentmodel, surf, clipflags, insubmodel);
					}

					surf++;
				} while (--c);
			}

			// all surfaces on the same node share the same sequence number
			r_currentkey++;
		}

		// recurse down the back side
		R_RecursiveWorldNode (currententity, currentmodel, node->children[!side], clipflags, insubmodel);
	}
}

/*
================
R_RenderWorld
================
*/
void
R_RenderWorld (entity_t *currententity)
{
	const model_t *currentmodel = r_worldmodel;

	if (!r_drawworld->value)
		return;
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	// auto cycle the world frame for texture animation
	currententity->frame = (int)(r_newrefdef.time*2);

	VectorCopy (r_origin, modelorg);
	r_pcurrentvertbase = currentmodel->vertexes;

	R_RecursiveWorldNode (currententity, currentmodel, currentmodel->nodes, ALIAS_XY_CLIP_MASK, false);
}
