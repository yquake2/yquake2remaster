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
 * The collision model. Slaps "boxes" through the world and checks if
 * they collide with the world model, entities or other boxes.
 *
 * =======================================================================
 */

#include <stdint.h>

#include "header/common.h"
#include "header/cmodel.h"

typedef struct
{
	cplane_t	*plane;
	int			children[2]; /* negative numbers are leafs */
} cnode_t;

typedef struct
{
	cplane_t	*plane;
	mapsurface_t	*surface;
} cbrushside_t;

typedef struct
{
	int			contents;
	int			cluster;
	int			area;
	unsigned int	firstleafbrush;
	unsigned int	numleafbrushes;
} cleaf_t;

typedef struct
{
	int			contents;
	unsigned int	numsides;
	unsigned int	firstbrushside;
	int			checkcount;	/* to avoid repeated testings */
} cbrush_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum; /* if two areas have equal floodnums, they are connected */
	int		floodvalid;
} carea_t;

typedef struct
{
	char name[MAX_QPATH];
	unsigned checksum;
	byte *cache; /* raw converted map */
	size_t cache_size;

	cleaf_t *map_leafs;
	int emptyleaf;
	int numleafs;

	unsigned int *map_leafbrushes;
	int numleafbrushes;

	cplane_t *map_planes; /* extra 6 for box hull */
	int numplanes;

	cbrush_t *map_brushes;
	int numbrushes;

	cbrushside_t *map_brushsides;
	int numbrushsides;

	mapsurface_t *map_surfaces;
	int	numtexinfo;

	/* TODO: big amount code expect static submodels */
	cmodel_t map_cmodels[MAX_MAP_MODELS];
	int numcmodels;

	cnode_t *map_nodes; /* extra 6 for box hull */
	int numnodes;

	carea_t	*map_areas;
	int numareas;

	qboolean *portalopen;
	const dareaportal_t *map_areaportals;
	int numareaportals;

	dvis_t *map_vis;
	int numclusters;
	int numvisibility;

	const char *map_entitystring;
	int numentitychars;

	int extradatasize;
	void *extradata;
} model_t;

#define MAX_MOD_KNOWN 8

/* Just empty model for cinematic */
static model_t empty_model;
/* Loaded models */
static model_t models[MAX_MOD_KNOWN];
static int model_num = 0;
static model_t *cmod = models;

// DG: is casted to int32_t* in SV_FatPVS() so align accordingly
static byte *pvsrow = NULL;
static byte *phsrow = NULL;
static size_t pxsrow_len = 0;
static cbrush_t *box_brush;
static cleaf_t *box_leaf;
static cplane_t *box_planes;
static cvar_t *map_noareas;
static cvar_t *r_maptype;
static cvar_t *r_game;
static int box_headnode;
static int checkcount;
static int floodvalid;
static int leaf_topnode;
static int trace_contents;
static mapsurface_t nullsurface;
static qboolean trace_ispoint; /* optimized case */
static trace_t trace_trace;
static vec3_t trace_start, trace_end;
static vec3_t trace_mins, trace_maxs;
static vec3_t trace_extents;

#ifndef DEDICATED_ONLY
int		c_pointcontents;
int		c_traces, c_brush_traces;
#endif

/* 1/32 epsilon to keep floating point happy */
#define DIST_EPSILON (0.03125f)

static void
FloodArea_r(carea_t *area, int floodnum)
{
	int i;
	const dareaportal_t *p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
		{
			return;
		}

		Com_Error(ERR_DROP, "%s: reflooded", __func__);
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &cmod->map_areaportals[area->firstareaportal];

	for (i = 0; i < area->numareaportals; i++, p++)
	{
		if (LittleLong(p->portalnum) >= cmod->numareaportals)
		{
			Com_Error(ERR_DROP, "%s: No such portal open %d > %d",
				__func__, LittleLong(p->portalnum), cmod->numareaportals);
		}

		if (LittleLong(p->otherarea) >= cmod->numareas)
		{
			Com_Error(ERR_DROP, "%s: No such area %d > %d",
				__func__, LittleLong(p->otherarea), cmod->numareas);
		}

		if (cmod->portalopen[LittleLong(p->portalnum)])
		{
			FloodArea_r(&cmod->map_areas[LittleLong(p->otherarea)], floodnum);
		}
	}
}

static void
FloodAreaConnections(void)
{
	int i, floodnum;

	/* all current floods are now invalid */
	floodvalid++;
	floodnum = 0;

	/* area 0 is not used */
	for (i = 1; i < cmod->numareas; i++)
	{
		carea_t *area;

		area = &cmod->map_areas[i];

		if (area->floodvalid == floodvalid)
		{
			continue; /* already flooded into */
		}

		floodnum++;
		FloodArea_r(area, floodnum);
	}
}

void
CM_SetAreaPortalState(int portalnum, qboolean open)
{
	if (portalnum > cmod->numareaportals)
	{
		Com_Printf("%s: areaportal > numareaportals %d > %d\n",
			__func__, portalnum, cmod->numareaportals);
		return;
	}

	cmod->portalopen[portalnum] = open;
	FloodAreaConnections();
}

qboolean
CM_AreasConnected(int area1, int area2)
{
	if (map_noareas->value)
	{
		return true;
	}

	if ((area1 > cmod->numareas) || (area2 > cmod->numareas))
	{
		Com_Error(ERR_DROP, "%s: area > numareas", __func__);
	}

	if (cmod->map_areas[area1].floodnum == cmod->map_areas[area2].floodnum)
	{
		return true;
	}

	return false;
}

/*
 * Writes a length byte followed by a bit vector of all the areas
 * that area in the same flood as the area parameter
 *
 * This is used by the client refreshes to cull visibility
 */
int
CM_WriteAreaBits(byte *buffer, int area)
{
	int bytes;

	bytes = (cmod->numareas + 7) >> 3;

	if (map_noareas->value)
	{
		/* for debugging, send everything */
		memset(buffer, 255, bytes);
	}
	else
	{
		int floodnum, i;

		memset(buffer, 0, bytes);

		floodnum = cmod->map_areas[area].floodnum;

		for (i = 0; i < cmod->numareas; i++)
		{
			if ((cmod->map_areas[i].floodnum == floodnum) || !area)
			{
				buffer[i >> 3] |= 1 << (i & 7);
			}
		}
	}

	return bytes;
}

/*
 * Writes the portal state to a savegame file
 */
void
CM_WritePortalState(FILE *f)
{
	fwrite(cmod->portalopen, sizeof(qboolean) * cmod->numareaportals, 1, f);
}

/*
 * Reads the portal state from a savegame file
 * and recalculates the area connections
 */
void
CM_ReadPortalState(fileHandle_t f)
{
	FS_Read(cmod->portalopen, sizeof(qboolean) * cmod->numareaportals, f);
	FloodAreaConnections();
}

/*
 * Returns true if any leaf under headnode has a cluster that
 * is potentially visible
 */
qboolean
CM_HeadnodeVisible(int nodenum, byte *visbits)
{
	const cnode_t *node;

	if (nodenum < 0)
	{
		int leafnum1;
		int cluster;

		leafnum1 = -1 - nodenum;
		cluster = cmod->map_leafs[leafnum1].cluster;

		if (cluster == -1)
		{
			return false;
		}

		if (visbits[cluster >> 3] & (1 << (cluster & 7)))
		{
			return true;
		}

		return false;
	}

	node = &cmod->map_nodes[nodenum];

	if (CM_HeadnodeVisible(node->children[0], visbits))
	{
		return true;
	}

	return CM_HeadnodeVisible(node->children[1], visbits);
}

/*
 * Set up the planes and nodes so that the six floats of a bounding box
 * can just be stored out and get a proper clipping hull structure.
 */
static void
CM_InitBoxHull(void)
{
	cplane_t *p;
	int i;

	box_headnode = cmod->numnodes;
	box_planes = &cmod->map_planes[cmod->numplanes];

	if ((cmod->numnodes <= 0) ||
		(cmod->numbrushes <= 0) ||
		(cmod->numleafbrushes <= 0) ||
		(cmod->numbrushsides <= 0) ||
		(cmod->numplanes <= 0))
	{
		Com_Printf("%s: Not enough room for box tree\n", __func__);
		return;
	}

	box_brush = &cmod->map_brushes[cmod->numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = cmod->numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &cmod->map_leafs[cmod->numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = cmod->numleafbrushes;
	box_leaf->numleafbrushes = 1;

	cmod->map_leafbrushes[cmod->numleafbrushes] = cmod->numbrushes;

	for (i = 0; i < 6; i++)
	{
		cbrushside_t *s;
		cnode_t *c;
		int side;

		side = i & 1;

		/* brush sides */
		s = &cmod->map_brushsides[cmod->numbrushsides + i];
		s->plane = cmod->map_planes + (cmod->numplanes + i * 2 + side);
		s->surface = &nullsurface;

		/* nodes */
		c = &cmod->map_nodes[box_headnode + i];
		c->plane = cmod->map_planes + (cmod->numplanes + i * 2);
		c->children[side] = -1 - cmod->emptyleaf;

		if (i != 5)
		{
			c->children[side ^ 1] = box_headnode + i + 1;
		}

		else
		{
			c->children[side ^ 1] = -1 - cmod->numleafs;
		}

		/* planes */
		p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + (i >> 1);
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = -1;
	}
}

/*
 * To keep everything totally uniform, bounding boxes are turned into
 * small BSP trees instead of being compared directly.
 */
int
CM_HeadnodeForBox(vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}

static int
CM_PointLeafnum_r(vec3_t p, int num)
{
	float d;
	cnode_t *node;
	cplane_t *plane;

	if (!cmod->numnodes)
	{
		return 0;
	}

	while (num >= 0 && num < (cmod->numnodes + EXTRA_LUMP_NODES))
	{
		node = cmod->map_nodes + num;
		plane = node->plane;

		if (plane->type < 3)
		{
			d = p[plane->type] - plane->dist;
		}

		else
		{
			d = DotProduct(plane->normal, p) - plane->dist;
		}

		if (d < 0)
		{
			num = node->children[1];
		}

		else
		{
			num = node->children[0];
		}
	}

#ifndef DEDICATED_ONLY
	c_pointcontents++; /* optimize counter */
#endif

	return -1 - num;
}

int
CM_PointLeafnum(vec3_t p)
{
	if (!cmod->numplanes || !cmod->numnodes || !cmod->map_nodes)
	{
		return 0; /* sound may call this without map loaded */
	}

	return CM_PointLeafnum_r(p, 0);
}

/*
 * Fills in a list of all the leafs touched
 */
static void
CM_BoxLeafnums_r(int nodenum, vec3_t leaf_mins, vec3_t leaf_maxs,
	int *leaf_list, int *leaf_count, int leaf_maxcount)
{
	while (1)
	{
		const cplane_t *plane;
		cnode_t *node;
		int s;

		if (nodenum < 0)
		{
			if ((*leaf_count) >= leaf_maxcount)
			{
				return;
			}

			leaf_list[(*leaf_count)++] = -1 - nodenum;
			return;
		}

		if (nodenum >= (cmod->numnodes + EXTRA_LUMP_NODES))
		{
			return;
		}

		node = &cmod->map_nodes[nodenum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);

		if (s == 1)
		{
			nodenum = node->children[0];
		}
		else if (s == 2)
		{
			nodenum = node->children[1];
		}
		else
		{
			/* go down both */
			if (leaf_topnode == -1)
			{
				leaf_topnode = nodenum;
			}

			CM_BoxLeafnums_r(node->children[0], leaf_mins, leaf_maxs, leaf_list,
				leaf_count, leaf_maxcount);
			nodenum = node->children[1];
		}
	}
}

static int
CM_BoxLeafnums_headnode(vec3_t leaf_mins, vec3_t leaf_maxs, int *leaf_list,
		int leaf_maxcount, int headnode, int *topnode)
{
	int leaf_count = 0;

	leaf_topnode = -1;

	CM_BoxLeafnums_r(headnode, leaf_mins, leaf_maxs, leaf_list,
		&leaf_count, leaf_maxcount);

	if (topnode)
	{
		*topnode = leaf_topnode;
	}

	return leaf_count;
}

int
CM_BoxLeafnums(vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	if (!cmod->numcmodels)
	{
		return 0;
	}

	return CM_BoxLeafnums_headnode(mins, maxs, list,
			listsize, cmod->map_cmodels[0].headnode, topnode);
}

int
CM_PointContents(vec3_t p, int headnode)
{
	int l;

	if (!cmod->numnodes) /* map not loaded */
	{
		return 0;
	}

	l = CM_PointLeafnum_r(p, headnode);

	return cmod->map_leafs[l].contents;
}

/*
 * Handles offseting and rotation of the end points for moving and
 * rotating entities
 */
int
CM_TransformedPointContents(vec3_t p, int headnode,
		vec3_t origin, vec3_t angles)
{
	vec3_t p_l;
	vec3_t temp;
	vec3_t forward, right, up;
	int l;

	/* subtract origin offset */
	VectorSubtract(p, origin, p_l);

	/* rotate start and end into the models frame of reference */
	if ((headnode != box_headnode) &&
		(angles[0] || angles[1] || angles[2]))
	{
		AngleVectors(angles, forward, right, up);

		VectorCopy(p_l, temp);
		p_l[0] = DotProduct(temp, forward);
		p_l[1] = -DotProduct(temp, right);
		p_l[2] = DotProduct(temp, up);
	}

	l = CM_PointLeafnum_r(p_l, headnode);

	if (!cmod->map_leafs || l >= (cmod->numleafs + EXTRA_LUMP_LEAFS) || l < 0)
	{
		return 0;
	}

	return cmod->map_leafs[l].contents;
}

static void
CM_ClipBoxToBrush(vec3_t mins, vec3_t maxs, vec3_t p1,
		vec3_t p2, trace_t *trace, const cbrush_t *brush)
{
	cbrushside_t *side, *leadside;
	float enterfrac, leavefrac;
	const cplane_t *clipplane;
	qboolean getout, startout;
	cplane_t *plane;
	float d1, d2;
	float dist;
	vec3_t ofs;
	int i, j;
	float f;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides || !cmod->map_brushsides)
	{
		return;
	}

#ifndef DEDICATED_ONLY
	c_brush_traces++;
#endif

	getout = false;
	startout = false;
	leadside = NULL;

	for (i = 0; i < brush->numsides; i++)
	{
		if ((brush->firstbrushside + i) >= (cmod->numbrushsides + EXTRA_LUMP_BRUSHSIDES))
		{
			Com_DPrintf("%s: Incorrect brushside %d\n",
				__func__, brush->firstbrushside + i);
			break;
		}

		side = &cmod->map_brushsides[brush->firstbrushside + i];
		plane = side->plane;

		if (!trace_ispoint)
		{
			/* general box case
			   push the plane out
			   apropriately for mins/maxs */
			for (j = 0; j < 3; j++)
			{
				if (plane->normal[j] < 0)
				{
					ofs[j] = maxs[j];
				}

				else
				{
					ofs[j] = mins[j];
				}
			}

			dist = DotProduct(ofs, plane->normal);
			dist = plane->dist - dist;
		}

		else
		{
			/* special point case */
			dist = plane->dist;
		}

		d1 = DotProduct(p1, plane->normal) - dist;
		d2 = DotProduct(p2, plane->normal) - dist;

		if (d2 > 0)
		{
			getout = true; /* endpoint is not in solid */
		}

		if (d1 > 0)
		{
			startout = true;
		}

		/* if completely in front of face, no intersection */
		if ((d1 > 0) && (d2 >= d1))
		{
			return;
		}

		if ((d1 <= 0) && (d2 <= 0))
		{
			continue;
		}

		/* crosses face */
		if (d1 > d2)
		{
			/* enter */
			f = (d1 - DIST_EPSILON) / (d1 - d2);

			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}

		else
		{
			/* leave */
			f = (d1 + DIST_EPSILON) / (d1 - d2);

			if (f < leavefrac)
			{
				leavefrac = f;
			}
		}
	}

	if (!startout)
	{
		/* original point was inside brush */
		trace->startsolid = true;

		if (!getout)
		{
			trace->allsolid = true;
		}

		return;
	}

	if (enterfrac < leavefrac)
	{
		if ((enterfrac > -1) && (enterfrac < trace->fraction))
		{
			if (enterfrac < 0)
			{
				enterfrac = 0;
			}

			if (clipplane == NULL)
			{
				Com_Error(ERR_FATAL, "%s: clipplane was NULL!\n", __func__);
			}

			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

static void
CM_TestBoxInBrush(vec3_t mins, vec3_t maxs, vec3_t p1,
		trace_t *trace, const cbrush_t *brush)
{
	int i, j;
	cplane_t *plane;
	vec3_t ofs;
	cbrushside_t *side;

	if (!brush->numsides || !cmod->map_brushsides)
	{
		return;
	}

	for (i = 0; i < brush->numsides; i++)
	{
		float d1, dist;

		if ((brush->firstbrushside + i) >= (cmod->numbrushsides + EXTRA_LUMP_BRUSHSIDES))
		{
			Com_DPrintf("%s: Incorrect brushside %d\n",
				__func__, brush->firstbrushside + i);
			break;
		}

		side = &cmod->map_brushsides[brush->firstbrushside + i];
		plane = side->plane;

		/* general box case
		   push the plane out
		   apropriately for mins/maxs */
		for (j = 0; j < 3; j++)
		{
			if (plane->normal[j] < 0)
			{
				ofs[j] = maxs[j];
			}
			else
			{
				ofs[j] = mins[j];
			}
		}

		dist = DotProduct(ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct(p1, plane->normal) - dist;

		/* if completely in front of face, no intersection */
		if (d1 > 0)
		{
			return;
		}
	}

	/* inside this brush */
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}

static void
CM_TraceToLeaf(int leafnum)
{
	const cleaf_t *leaf;
	int k, maxleaf;

	if (leafnum >= (cmod->numleafs + EXTRA_LUMP_LEAFS) || leafnum < 0)
	{
		Com_DPrintf("%s: Incorrect leaf number!\n", __func__);
	}

	leaf = &cmod->map_leafs[leafnum];

	if (!(leaf->contents & trace_contents) || !cmod->numleafbrushes)
	{
		return;
	}

	maxleaf = leaf->firstleafbrush + leaf->numleafbrushes;
	if (maxleaf >= (cmod->numleafbrushes + EXTRA_LUMP_LEAFBRUSHES))
	{
		Com_Error(ERR_FATAL, "%s: broken leaf! %d > %d\n",
			__func__, maxleaf, cmod->numleafbrushes + EXTRA_LUMP_LEAFBRUSHES);
	}

	/* trace line against all brushes in the leaf */
	for (k = 0; k < leaf->numleafbrushes; k++)
	{
		int brushnum;
		cbrush_t *b;

		brushnum = cmod->map_leafbrushes[leaf->firstleafbrush + k];

		if (brushnum < 0 || brushnum >= (cmod->numbrushes + EXTRA_LUMP_BRUSHES))
		{
			Com_Error(ERR_FATAL, "%s: incorrect brushnum in leaf!\n", __func__);
		}

		b = &cmod->map_brushes[brushnum];

		if (b->checkcount == checkcount)
		{
			continue; /* already checked this brush in another leaf */
		}

		b->checkcount = checkcount;

		if (!(b->contents & trace_contents))
		{
			continue;
		}

		CM_ClipBoxToBrush(trace_mins, trace_maxs, trace_start,
				trace_end, &trace_trace, b);

		if (!trace_trace.fraction)
		{
			return;
		}
	}
}

static void
CM_TestInLeaf(int leafnum)
{
	const cleaf_t *leaf;
	int k, maxleaf;

	if (leafnum > (cmod->numleafs + EXTRA_LUMP_LEAFS) || leafnum < 0)
	{
		Com_DPrintf("%s: Incorrect leaf number!\n", __func__);
	}

	leaf = &cmod->map_leafs[leafnum];

	if (!(leaf->contents & trace_contents) || !cmod->numleafbrushes)
	{
		return;
	}

	maxleaf = leaf->firstleafbrush + leaf->numleafbrushes;
	if (maxleaf >= (cmod->numleafbrushes + EXTRA_LUMP_LEAFBRUSHES))
	{
		Com_Error(ERR_FATAL, "%s: broken leaf! %d > %d\n",
			__func__, maxleaf, cmod->numleafbrushes + EXTRA_LUMP_LEAFBRUSHES);
	}

	/* trace line against all brushes in the leaf */
	for (k = 0; k < leaf->numleafbrushes; k++)
	{
		int brushnum;
		cbrush_t *b;

		brushnum = cmod->map_leafbrushes[leaf->firstleafbrush + k];

		if (brushnum < 0 || brushnum >= (cmod->numbrushes + EXTRA_LUMP_BRUSHES))
		{
			Com_Error(ERR_FATAL, "%s: incorrect brushnum in leaf!\n", __func__);
		}

		b = &cmod->map_brushes[brushnum];

		if (b->checkcount == checkcount)
		{
			continue; /* already checked this brush in another leaf */
		}

		b->checkcount = checkcount;

		if (!(b->contents & trace_contents))
		{
			continue;
		}

		CM_TestBoxInBrush(trace_mins, trace_maxs, trace_start, &trace_trace, b);

		if (!trace_trace.fraction)
		{
			return;
		}
	}
}

static void
CM_RecursiveHullCheck(int num, float p1f, float p2f, const vec3_t p1, const vec3_t p2)
{
	cnode_t *node;
	cplane_t *plane;
	float t1, t2, offset;
	float frac, frac2;
	float idist;
	int i;
	vec3_t mid;
	int side;
	float midf;

	if (trace_trace.fraction <= p1f)
	{
		return; /* already hit something nearer */
	}

	/* if < 0, we are in a leaf node */
	if (num < 0)
	{
		CM_TraceToLeaf(-1 - num);
		return;
	}

	if (!cmod->numnodes || (num >= (cmod->numnodes + EXTRA_LUMP_NODES)))
	{
		/* Not fully loaded? */
		return;
	}

	/* find the point distances to the seperating plane
	   and the offset for the size of the box */
	node = cmod->map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}

	else
	{
		t1 = DotProduct(plane->normal, p1) - plane->dist;
		t2 = DotProduct(plane->normal, p2) - plane->dist;

		if (trace_ispoint)
		{
			offset = 0;
		}

		else
		{
			offset = (float)fabs(trace_extents[0] * plane->normal[0]) +
					 (float)fabs(trace_extents[1] * plane->normal[1]) +
					 (float)fabs(trace_extents[2] * plane->normal[2]);
		}
	}

	/* see which sides we need to consider */
	if ((t1 >= offset) && (t2 >= offset))
	{
		CM_RecursiveHullCheck(node->children[0], p1f, p2f, p1, p2);
		return;
	}

	if ((t1 < -offset) && (t2 < -offset))
	{
		CM_RecursiveHullCheck(node->children[1], p1f, p2f, p1, p2);
		return;
	}

	/* put the crosspoint DIST_EPSILON pixels on the near side */
	if (t1 < t2)
	{
		idist = 1.0f / (t1 - t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON) * idist;
		frac = (t1 - offset + DIST_EPSILON) * idist;
	}

	else if (t1 > t2)
	{
		idist = 1.0 / (t1 - t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON) * idist;
		frac = (t1 + offset + DIST_EPSILON) * idist;
	}

	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	/* move up to the node */
	if (frac < 0)
	{
		frac = 0;
	}

	if (frac > 1)
	{
		frac = 1;
	}

	midf = p1f + (p2f - p1f) * frac;

	for (i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	CM_RecursiveHullCheck(node->children[side], p1f, midf, p1, mid);

	/* go past the node */
	if (frac2 < 0)
	{
		frac2 = 0;
	}

	if (frac2 > 1)
	{
		frac2 = 1;
	}

	midf = p1f + (p2f - p1f) * frac2;

	for (i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);
	}

	CM_RecursiveHullCheck(node->children[side ^ 1], midf, p2f, mid, p2);
}

trace_t
CM_BoxTrace(const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs,
		int headnode, int brushmask)
{
	checkcount++; /* for multi-check avoidance */

#ifndef DEDICATED_ONLY
	c_traces++; /* for statistics, may be zeroed */
#endif

	/* fill in a default trace */
	memset(&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!cmod->numnodes)  /* map not loaded */
	{
		return trace_trace;
	}

	trace_contents = brushmask;
	VectorCopy(start, trace_start);
	VectorCopy(end, trace_end);
	VectorCopy(mins, trace_mins);
	VectorCopy(maxs, trace_maxs);

	/* check for position test special case */
	if ((start[0] == end[0]) && (start[1] == end[1]) && (start[2] == end[2]))
	{
		int leafs[1024];
		int i, numleafs;
		vec3_t c1, c2;
		int topnode;

		VectorAdd(start, mins, c1);
		VectorAdd(start, maxs, c2);

		for (i = 0; i < 3; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode(c1, c2, leafs, 1024,
				headnode, &topnode);

		for (i = 0; i < numleafs; i++)
		{
			CM_TestInLeaf(leafs[i]);

			if (trace_trace.allsolid)
			{
				break;
			}
		}

		VectorCopy(start, trace_trace.endpos);
		return trace_trace;
	}

	/* check for point special case */
	if ((mins[0] == 0) && (mins[1] == 0) && (mins[2] == 0) &&
		(maxs[0] == 0) && (maxs[1] == 0) && (maxs[2] == 0))
	{
		trace_ispoint = true;
		VectorClear(trace_extents);
	}

	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	/* general sweeping through world */
	CM_RecursiveHullCheck(headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy(end, trace_trace.endpos);
	}
	else
	{
		int i;

		for (i = 0; i < 3; i++)
		{
			trace_trace.endpos[i] = start[i] + trace_trace.fraction *
									(end[i] - start[i]);
		}
	}

	return trace_trace;
}

/*
 * Handles offseting and rotation of the end points for moving and
 * rotating entities
 */
trace_t
CM_TransformedBoxTrace(const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs, int headnode, int brushmask,
		const vec3_t origin, const vec3_t angles)
{
	vec3_t forward, right, up;
	vec3_t start_l, end_l;
	qboolean rotated;
	trace_t trace;
	vec3_t temp;

	/* subtract origin offset */
	VectorSubtract(start, origin, start_l);
	VectorSubtract(end, origin, end_l);

	/* rotate start and end into the models frame of reference */
	if ((headnode != box_headnode) &&
		(angles[0] || angles[1] || angles[2]))
	{
		rotated = true;
	}

	else
	{
		rotated = false;
	}

	if (rotated)
	{
		AngleVectors(angles, forward, right, up);

		VectorCopy(start_l, temp);
		start_l[0] = DotProduct(temp, forward);
		start_l[1] = -DotProduct(temp, right);
		start_l[2] = DotProduct(temp, up);

		VectorCopy(end_l, temp);
		end_l[0] = DotProduct(temp, forward);
		end_l[1] = -DotProduct(temp, right);
		end_l[2] = DotProduct(temp, up);
	}

	/* sweep the box through the model */
	trace = CM_BoxTrace(start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && (trace.fraction != 1.0))
	{
		vec3_t a;

		VectorNegate(angles, a);
		AngleVectors(a, forward, right, up);

		VectorCopy(trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct(temp, forward);
		trace.plane.normal[1] = -DotProduct(temp, right);
		trace.plane.normal[2] = DotProduct(temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

static void
CMod_LoadSubmodels(const char *name, cmodel_t *map_cmodels, int *numcmodels, int numnodes,
	const byte *cmod_base, const lump_t *l)
{
	dmodel_t *in;
	cmodel_t *out;
	int i, j, count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s has funny lump size", __func__, name);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s with no models", __func__, name);
	}

	if (count > MAX_MAP_MODELS)
	{
		Com_Error(ERR_DROP, "%s: Map %s has too many models", __func__, name);
	}

	*numcmodels = count;

	out = map_cmodels;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			/* spread the mins / maxs by a pixel */
			out->mins[j] = in->mins[j] - 1;
			out->maxs[j] = in->maxs[j] + 1;
			out->origin[j] = in->origin[j];
		}

		out->headnode = in->headnode;

		/* check limits */
		if (out->headnode >= numnodes)
		{
			Com_Error(ERR_DROP, "%s: Inline model %i has bad firstnode",
					__func__, i);
		}
	}
}

static void
CMod_LoadSurfaces(const char *name, mapsurface_t **map_surfaces, int *numtexinfo,
	const byte *cmod_base, const lump_t *l)
{
	xtexinfo_t *in;
	mapsurface_t *out;
	int i, count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size", __func__);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map with no surfaces", __func__);
	}

	*numtexinfo = count;
	out = *map_surfaces = Hunk_Alloc(count * sizeof(*out));

	for (i = 0; i < count; i++, in++, out++)
	{
		Q_strlcpy(out->c.name, in->texture, sizeof(out->c.name));
		Q_strlcpy(out->rname, in->texture, sizeof(out->rname));
		out->c.flags = in->flags;
		Q_strlcpy(out->c.material, in->material, sizeof(out->c.material));
	}
}

static void
CMod_LoadNodes(const char *name, cnode_t **map_nodes, int *numnodes,
	cplane_t *map_planes, const byte *cmod_base, const lump_t *l)
{
	dqnode_t *in;
	int child;
	cnode_t *out;
	int i, j, count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size " YQ2_COM_PRIdS,
			__func__, name, sizeof(*in));
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s with no nodes", __func__, name);
	}

	out = *map_nodes = Hunk_Alloc((count + EXTRA_LUMP_NODES) * sizeof(*out));

	*numnodes = count;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->plane = map_planes + in->planenum;

		for (j = 0; j < 2; j++)
		{
			child = in->children[j];
			out->children[j] = child;
		}
	}
}

static void
CMod_LoadBrushes(const char* name, cbrush_t **map_brushes, int *numbrushes,
	const byte *cmod_base, const lump_t *l)
{
	dbrush_t *in;
	cbrush_t *out;
	int i, count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size", __func__, name);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s has no brushes", __func__, name);
	}

	out = *map_brushes = Hunk_Alloc((count + EXTRA_LUMP_BRUSHES) * sizeof(*out));

	*numbrushes = count;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->firstbrushside = in->firstside;
		out->numsides = in->numsides;
		out->contents = in->contents;
	}
}

static void
CMod_LoadLeafs(const char *name, cleaf_t **map_leafs, int *numleafs, int *emptyleaf,
	int *numclusters, const byte *cmod_base, const lump_t *l)
{
	int i;
	cleaf_t *out;
	dqleaf_t *in;
	int count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size", __func__);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map with no leafs", __func__);
	}

	out = *map_leafs = Hunk_Alloc((count + EXTRA_LUMP_LEAFS) * sizeof(*out));
	*numleafs = count;
	*numclusters = 0;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->contents = in->contents;
		out->cluster = in->cluster;
		out->area = in->area;
		out->firstleafbrush = in->firstleafbrush;
		out->numleafbrushes = in->numleafbrushes;

		if (out->cluster >= *numclusters)
		{
			*numclusters = out->cluster + 1;
		}
	}

	if ((*map_leafs)[0].contents != CONTENTS_SOLID)
	{
		Com_Error(ERR_DROP, "%s: Map leaf 0 is not CONTENTS_SOLID", __func__);
	}

	*emptyleaf = -1;

	for (i = 1; i < count; i++)
	{
		if (!(*map_leafs)[i].contents)
		{
			*emptyleaf = i;
			break;
		}
	}

	if (*emptyleaf == -1)
	{
		Com_Error(ERR_DROP, "%s: Map does not have an empty leaf", __func__);
	}
}

static void
CMod_LoadLeafBrushes(const char *name, unsigned int **map_leafbrushes,
	int *numleafbrushes, const byte *cmod_base, const lump_t *l)
{
	unsigned int *out, *in;
	int count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size", __func__, name);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s with no leaf brushes", __func__, name);
	}

	out = *map_leafbrushes = Hunk_Alloc((count + EXTRA_LUMP_LEAFBRUSHES) * sizeof(*out));
	*numleafbrushes = count;

	memcpy(out, in, count * sizeof(*out));
}

static void
CMod_LoadBrushSides(const char *name, cbrushside_t **map_brushsides, int *numbrushsides,
	cplane_t *map_planes, int numplanes, mapsurface_t *map_surfaces, int numtexinf,
	const byte *cmod_base, const lump_t *l)
{
	int i;
	cbrushside_t *out;
	dqbrushside_t *in;
	int count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size", __func__, name);
	}

	count = l->filelen / sizeof(*in);

	/* need to save space for box planes */
	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s with no brush sides", __func__, name);
	}

	out = *map_brushsides = Hunk_Alloc((count + EXTRA_LUMP_BRUSHSIDES) * sizeof(*out));
	*numbrushsides = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		int j, num;

		num = in->planenum;
		j = in->texinfo;

		if (j >= numtexinf || num > numplanes)
		{
			Com_Error(ERR_DROP, "%s: Bad brushside texinfo", __func__);
		}

		out->plane = map_planes + num;
		out->surface = (j >= 0) ? &map_surfaces[j] : &nullsurface;
	}
}

static void
CMod_LoadAreas(const char *name, carea_t **map_areas, int *numareas,
	const byte *cmod_base, const lump_t *l)
{
	int i;
	carea_t *out;
	darea_t *in;
	int count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size",
			__func__, name);
	}

	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Com_Error(ERR_DROP, "%s: Map %s has no areas",
			__func__, name);
	}

	out = *map_areas = Hunk_Alloc(count * sizeof(*out));
	*numareas = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->numareaportals = in->numareaportals;
		out->firstareaportal = in->firstareaportal;
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

static void
CMod_LoadAreaPortals(const char *name, const dareaportal_t **map_areaportals, qboolean **portalopen,
	int *numareaportals, const byte *cmod_base, const lump_t *l)
{
	const dareaportal_t *in;
	int count;

	in = (void *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: Map %s funny lump size", __func__, name);
	}

	count = l->filelen / sizeof(*in);

	if (count <= 0)
	{
		Com_Error(ERR_DROP, "%s: Map %s has too small areas", __func__, name);
	}

	*map_areaportals = in;

	*portalopen = Hunk_Alloc(count * sizeof(qboolean));
	*numareaportals = count;

	memset(*portalopen, 0, count * sizeof(qboolean));
}

static void
CMod_LoadEntityString(const char *name, const char **map_entitystring, int *numentitychars,
	const byte *cmod_base, const lump_t *l)
{
	if (sv_entfile->value)
	{
		char *buffer = NULL, entname[256];
		size_t nameLen;
		int bufLen = -1;

		nameLen = strlen(name);
		if (strcmp(name + nameLen - 4, ".bsp") || nameLen > (MAX_QPATH - 1))
		{
			Com_Printf("%s: unsupported map format '%s'\n", __func__, name);
		}
		else
		{
			char namewe[MAX_QPATH];

			int crc = 0;

			if (l->filelen > 0) {
				crc = CRC_Block(cmod_base + l->fileofs, l->filelen - 1);
			}

			memset(namewe, 0, sizeof(namewe));
			memcpy(namewe, name, nameLen - 4);

			snprintf(entname, sizeof(entname) -1, "%s@%04x.ent", namewe, crc);

			bufLen = FS_LoadFile(entname, (void **)&buffer);
			if (buffer == NULL)
			{
				Com_Printf("No fixes found as '%s'\n", entname);

				snprintf(entname, sizeof(entname) -1, "%s.ent", namewe);
				bufLen = FS_LoadFile(entname, (void **)&buffer);
				if (buffer != NULL && bufLen > 1)
				{
					Com_Printf("Have used '%s' file without content hash.\n", entname);
				}
			}
		}

		if (buffer != NULL && bufLen > 1)
		{
			char *entity;
			*numentitychars = bufLen;

			entity = Hunk_Alloc(bufLen);
			memcpy(entity, buffer, bufLen);
			FS_FreeFile(buffer);

			Com_Printf (".ent file %s loaded.\n", entname);
			*map_entitystring = entity;
			return;
		}
		else if (bufLen != -1)
		{
			/* If the .ent file is too small or large, don't load. */
			Com_Printf("%s: .ent file %s too small.\n", __func__, entname);
			FS_FreeFile(buffer);
		}
	}

	*numentitychars = l->filelen;

	if (l->filelen == 0)
	{
		Com_Error(ERR_DROP, "%s: Map %s has too small entity lump", __func__, name);
	}

	*map_entitystring = (const char *)cmod_base + l->fileofs;
}

static void
CM_ModFree(model_t *cmod)
{
	if (cmod->extradata && cmod->extradatasize)
	{
		Hunk_Free(cmod->extradata);
	}

	memset(cmod, 0, sizeof(model_t));
}

void
CM_ModInit(void)
{
	memset(models, 0, sizeof(models));

	/* init buffers for PVS/PHS buffers*/
	pvsrow = NULL;
	phsrow = NULL;
	pxsrow_len = 0;

	map_noareas = Cvar_Get("map_noareas", "0", 0);
	r_maptype = Cvar_Get("maptype", "0", CVAR_ARCHIVE);
	r_game = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);
}

void
CM_ModFreeAll(void)
{
	int i;

	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		CM_ModFree(&models[i]);
	}

	/* Free up buffer for PVS/PHS */
	if (pvsrow)
	{
		free(pvsrow);
		pvsrow = NULL;
	}

	if (phsrow)
	{
		free(phsrow);
		phsrow = NULL;
	}
	pxsrow_len = 0;

	Com_Printf("Server models free up\n");
}

static void
CM_LoadCachedMap(const char *name, model_t *mod)
{
	size_t length, hunkSize;
	byte *cmod_base, *filebuf;
	maptype_t maptype;
	dheader_t *header;
	int filelen;

	filelen = FS_LoadFile(name, (void **)&filebuf);

	if (!filebuf || filelen <= 0)
	{
		Com_Printf("%s: Couldn't load %s\n", __func__, name);
		return;
	}

	mod->checksum = LittleLong(Com_BlockChecksum(filebuf, filelen));

	/* Can't detect will use provided */
	maptype = r_maptype->value;

	cmod_base = Mod_Load2QBSP(name, (byte *)filebuf, filelen, &length, &maptype);
	FS_FreeFile(filebuf);

	header = (dheader_t *)cmod_base;

	/* load into heap */
	Q_strlcpy(mod->name, name, sizeof(mod->name));

	hunkSize = length; /* allocate memory for future maps cache */
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_TEXINFO],
		sizeof(xtexinfo_t), sizeof(mapsurface_t), EXTRA_LUMP_TEXINFO);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFS],
		sizeof(dqleaf_t), sizeof(cleaf_t), 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFBRUSHES],
		sizeof(int), sizeof(int), EXTRA_LUMP_LEAFBRUSHES);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_PLANES],
		sizeof(dplane_t), sizeof(cplane_t), EXTRA_LUMP_PLANES);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_BRUSHES],
		sizeof(dbrush_t), sizeof(cbrush_t), EXTRA_LUMP_BRUSHES);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_BRUSHSIDES],
		sizeof(dqbrushside_t), sizeof(cbrushside_t), EXTRA_LUMP_BRUSHSIDES);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_NODES],
		sizeof(dqnode_t), sizeof(cnode_t), EXTRA_LUMP_NODES);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_AREAS],
		sizeof(darea_t), sizeof(carea_t), 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_AREAPORTALS],
		sizeof(dareaportal_t), sizeof(dareaportal_t), 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_AREAPORTALS],
		sizeof(dareaportal_t), sizeof(qboolean), 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_VISIBILITY],
		1, 1, 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_ENTITIES],
		1, 1, MAX_MAP_ENTSTRING);

	mod->extradata = Hunk_Begin(hunkSize);

	mod->cache = Hunk_Alloc(length);
	memcpy(mod->cache, cmod_base, length);
	mod->cache_size = length;

	CMod_LoadSurfaces(mod->name, &mod->map_surfaces, &mod->numtexinfo,
		mod->cache, &header->lumps[LUMP_TEXINFO]);
	CMod_LoadLeafs(mod->name, &mod->map_leafs, &mod->numleafs, &mod->emptyleaf,
		&mod->numclusters, mod->cache, &header->lumps[LUMP_LEAFS]);
	CMod_LoadLeafBrushes(mod->name, &mod->map_leafbrushes, &mod->numleafbrushes,
		mod->cache, &header->lumps[LUMP_LEAFBRUSHES]);
	Mod_LoadPlanes(mod->name, &mod->map_planes, &mod->numplanes,
		mod->cache, &header->lumps[LUMP_PLANES]);
	CMod_LoadBrushes(mod->name, &mod->map_brushes, &mod->numbrushes,
		mod->cache, &header->lumps[LUMP_BRUSHES]);
	CMod_LoadBrushSides(mod->name, &mod->map_brushsides, &mod->numbrushsides,
		mod->map_planes, mod->numplanes, mod->map_surfaces, mod->numtexinfo,
		mod->cache, &header->lumps[LUMP_BRUSHSIDES]);
	CMod_LoadNodes(mod->name, &mod->map_nodes, &mod->numnodes,
		mod->map_planes, mod->cache, &header->lumps[LUMP_NODES]);
	CMod_LoadSubmodels(mod->name, mod->map_cmodels, &mod->numcmodels, mod->numnodes,
		mod->cache, &header->lumps[LUMP_MODELS]);
	CMod_LoadAreas(mod->name, &mod->map_areas, &mod->numareas, mod->cache,
		&header->lumps[LUMP_AREAS]);
	CMod_LoadAreaPortals(mod->name, &mod->map_areaportals,
		&mod->portalopen, &mod->numareaportals,
		mod->cache, &header->lumps[LUMP_AREAPORTALS]);
	Mod_LoadVisibility(mod->name, &mod->map_vis, &mod->numvisibility,
		mod->cache, &header->lumps[LUMP_VISIBILITY]);

	if (!mod->map_vis)
	{
		Com_DPrintf("%s: Map %s has no visual clusters.",
			__func__, name);
	}
	else if (mod->numclusters != mod->map_vis->numclusters)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect number of clusters %d != %d",
			__func__, name, mod->numclusters, mod->map_vis->numclusters);
	}

	/* From kmquake2: adding an extra parameter for .ent support. */
	CMod_LoadEntityString(mod->name, &mod->map_entitystring, &mod->numentitychars,
		mod->cache, &header->lumps[LUMP_ENTITIES]);
	mod->extradatasize = Hunk_End();
	Com_DPrintf("Allocated %d from expected " YQ2_COM_PRIdS " hunk size\n",
		mod->extradatasize, hunkSize);

	free(cmod_base);

	if ((mod->numleafs > pxsrow_len) || !pvsrow || !phsrow)
	{
		/* reallocate buffers for PVS/PHS buffers*/
		pxsrow_len = (mod->numleafs + 63) & ~63;
		pvsrow = realloc(pvsrow, pxsrow_len / 8);
		phsrow = realloc(phsrow, pxsrow_len / 8);
		Com_Printf("Allocated " YQ2_COM_PRIdS " bit leafs of PVS/PHS buffer\n",
			pxsrow_len);
	}
}

const byte *
CM_GetRawMap(int *len)
{
	if (!cmod)
	{
		return NULL;
	}

	*len = cmod->cache_size;
	return cmod->cache;
}

/*
 * Loads in the map and all submodels
 */
cmodel_t *
CM_LoadMap(const char *name, qboolean clientload, unsigned *checksum)
{
	int i, sec_start;

	if (!name[0])
	{
		memset(&empty_model, 0, sizeof(empty_model));
		*checksum = 0;
		cmod = &empty_model;
		return &cmod->map_cmodels[0]; /* cinematic servers won't have anything at all */
	}

	if (cmod && !strcmp(cmod->name, name)
		&& (clientload || !Cvar_VariableValue("flushmap")))
	{
		*checksum = cmod->checksum;

		if (!clientload)
		{
			memset(cmod->portalopen, 0, sizeof(qboolean) * cmod->numareaportals);
			FloodAreaConnections();
		}

		return &cmod->map_cmodels[0]; /* still have the right version */
	}

	sec_start = Sys_Milliseconds();
	cmod = NULL;
	/* search already loaded */
	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		if (!strcmp(models[i].name, name))
		{
			model_num = i;
			cmod = &models[model_num];
			Com_DPrintf("%s: Loaded cached map: %s: %d Kb\n",
				__func__, name, cmod->extradatasize / 1024);
			break;
		}
	}

	/* first empty model */
	if (!cmod)
	{
		for (i = 0; i < MAX_MOD_KNOWN; i++)
		{
			/* first free momdel */
			if (!models[i].name[0])
			{
				model_num = i;
				cmod = &models[model_num];
				Com_DPrintf("%s: Used free space for map: %s\n",
					__func__, name);
				break;
			}
		}
	}

	if (!cmod)
	{
		model_num = (model_num + 1) % MAX_MOD_KNOWN;
		cmod = &models[model_num];
		Com_DPrintf("%s: No free space. Clean up random for map: %s: %d Kb\n",
			__func__, name, cmod->extradatasize / 1024);
		/* free old stuff */
		CM_ModFree(cmod);
	}

	/* need to load map */
	if (!cmod->extradatasize)
	{
		CM_LoadCachedMap(name, cmod);
	}

	*checksum = cmod->checksum;

	CM_InitBoxHull();

	memset(cmod->portalopen, 0, sizeof(qboolean) * cmod->numareaportals);
	FloodAreaConnections();

	Com_DPrintf("%s: Loaded map: %s: %d Kb in %.2fs\n",
		__func__, name, cmod->extradatasize / 1024,
		(Sys_Milliseconds() - sec_start) / 1000.0);
	return cmod->map_cmodels;
}

cmodel_t *
CM_InlineModel(const char *name)
{
	int num;

	if (!name || (name[0] != '*'))
	{
		Com_Error(ERR_DROP, "%s: bad name", __func__);
	}

	num = (int)strtol(name + 1, (char **)NULL, 10);

	if ((num < 1) || (num >= cmod->numcmodels))
	{
		Com_Error(ERR_DROP, "%s: bad number", __func__);
	}

	return &cmod->map_cmodels[num];
}

int
CM_NumClusters(void)
{
	return cmod->numclusters;
}

int
CM_NumInlineModels(void)
{
	return cmod->numcmodels;
}

const char *
CM_EntityString(int *size)
{
	*size = cmod->numentitychars;
	return cmod->map_entitystring;
}

int
CM_LeafContents(int leafnum)
{
	if ((leafnum < 0) || (leafnum >= (cmod->numleafs + EXTRA_LUMP_LEAFS)))
	{
		Com_Error(ERR_DROP, "%s: bad number", __func__);
	}

	return cmod->map_leafs[leafnum].contents;
}

int
CM_LeafCluster(int leafnum)
{
	if (!cmod->map_leafs)
	{
		return 0;
	}

	if ((leafnum < 0) || (leafnum >= (cmod->numleafs + EXTRA_LUMP_LEAFS)))
	{
		Com_Error(ERR_DROP, "%s: bad number", __func__);
	}

	return cmod->map_leafs[leafnum].cluster;
}

int
CM_LeafArea(int leafnum)
{
	if (!cmod->map_leafs)
	{
		return 0;
	}

	if ((leafnum < 0) || (leafnum >= (cmod->numleafs + EXTRA_LUMP_LEAFS)))
	{
		Com_Error(ERR_DROP, "%s: bad number", __func__);
	}

	return cmod->map_leafs[leafnum].area;
}

int
CM_MapSurfacesNum(void)
{
	return cmod->numtexinfo;
}

mapsurface_t*
CM_MapSurfaces(int surfnum)
{
	if (surfnum > cmod->numtexinfo)
	{
		return &nullsurface;
	}
	return cmod->map_surfaces + surfnum;
}

static byte *
CM_Cluster(int cluster, int type, byte *buffer)
{
	if (!buffer)
	{
		Com_Error(ERR_DROP, "%s: incrorrect init of PVS/PHS", __func__);
	}

	if (!cmod->map_vis)
	{
		Mod_DecompressVis(NULL, buffer, NULL, (cmod->numclusters + 7) >> 3);
	}
	else if (cluster == -1)
	{
		memset(buffer, 0, (cmod->numclusters + 7) >> 3);
	}
	else
	{
		if (cluster < 0 || cluster >= cmod->numclusters)
		{
			Com_Error(ERR_DROP, "%s: bad cluster", __func__);
		}

		Mod_DecompressVis((byte *)cmod->map_vis +
				cmod->map_vis->bitofs[cluster][type], buffer,
				(byte *)cmod->map_vis + cmod->numvisibility,
				(cmod->numclusters + 7) >> 3);
	}

	return buffer;
}

byte *
CM_ClusterPVS(int cluster)
{
	return CM_Cluster(cluster, DVIS_PVS, pvsrow);
}

byte *
CM_ClusterPHS(int cluster)
{
	return CM_Cluster(cluster, DVIS_PHS, phsrow);
}

/*
 * Return file content for most case but could cache converted file in future
 */
int
CM_LoadFile(const char *path, void **buffer)
{
	int i;

	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		/* we have already cached */
		if (!strcmp(path, models[i].name) &&
			models[i].cache &&
			models[i].extradatasize)
		{
			*buffer = Z_Malloc(models[i].cache_size);
			memcpy(*buffer, models[i].cache, models[i].cache_size);
			return models[i].cache_size;
		}
	}

	Com_Printf("%s: render asked about '%s' but have not loaded it yet?\n",
		__func__, path);
	return -1;
}
