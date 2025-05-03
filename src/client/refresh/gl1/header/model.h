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
 * Header for the model stuff.
 *
 * =======================================================================
 */

#ifndef REF_MODEL_H
#define REF_MODEL_H

/* Whole model */

typedef struct model_s
{
	char name[MAX_QPATH];

	int registration_sequence;

	modtype_t type;
	int numframes;

	int flags;

	/* volume occupied by the model graphics */
	vec3_t mins, maxs;
	float radius;

	/* solid volume for clipping */
	qboolean clipbox;
	vec3_t clipmins, clipmaxs;

	/* brush model */
	int firstmodelsurface, nummodelsurfaces;
	int lightmap; /* only for submodels */

	int numsubmodels;
	struct model_s *submodels;

	int numplanes;
	cplane_t *planes;

	int numleafs; /* number of visible leafs, not counting 0 */
	mleaf_t *leafs;

	int numvertexes;
	mvertex_t *vertexes;

	int numedges;
	medge_t *edges;

	int numnodes;
	int firstnode;
	mnode_t *nodes;

	int numtexinfo;
	mtexinfo_t *texinfo;

	int numsurfaces;
	msurface_t *surfaces;

	int numsurfedges;
	int *surfedges;

	unsigned int nummarksurfaces;
	msurface_t **marksurfaces;

	int numvisibility;
	int numclusters;
	dvis_t *vis;

	byte *lightdata;
	int numlightdata;

	/* for alias models and skins */
	struct image_s **skins;
	int numskins;

	int extradatasize;
	void *extradata;

	// submodules
	vec3_t		origin;	// for sounds or lights

	/* octree  */
	bspxlightgrid_t *grid;
} model_t;

void Mod_Init(void);
void Mod_ClearAll(void);
const byte *Mod_ClusterPVS(int cluster, const model_t *model);

void Mod_Modellist_f(void);

void *Hunk_Begin(int maxsize);
void *Hunk_Alloc(int size);
int Hunk_End(void);
void Hunk_Free(void *base);

void Mod_FreeAll(void);

#endif
