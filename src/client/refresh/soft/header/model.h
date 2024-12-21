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

#ifndef __MODEL__
#define __MODEL__

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

//
// in memory representation
//

//===================================================================

//
// Whole model
//

typedef struct model_s
{
	char		name[MAX_QPATH];

	int		registration_sequence;

	modtype_t	type;
	int		numframes;

	int		flags;

	//
	// volume occupied by the model graphics
	//
	vec3_t		mins, maxs;
	float		radius;

	//
	// solid volume for clipping (sent from server)
	//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

	//
	// brush model
	//
	int		firstmodelsurface, nummodelsurfaces;

	int		numsubmodels;
	struct model_s	*submodels;

	int		numplanes;
	cplane_t	*planes;

	int		numleafs;	// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int		numvertexes;
	mvertex_t	*vertexes;

	int		numedges;
	medge_t		*edges;

	int		numnodes;
	int		firstnode;
	mnode_t		*nodes;

	int		numtexinfo;
	mtexinfo_t	*texinfo;

	int		numsurfaces;
	msurface_t	*surfaces;

	int		numsurfedges;
	int		*surfedges;

	unsigned int nummarksurfaces;
	msurface_t **marksurfaces;

	int numvisibility;
	int numclusters;
	dvis_t *vis;

	byte *lightdata;
	int numlightdata;

	// for alias models and sprites
	struct image_s **skins;
	int numskins;

	void		*extradata;
	int		extradatasize;

	// submodules
	vec3_t		origin;	// for sounds or lights

	/* octree  */
	bspxlightgrid_t *grid;
} model_t;

//============================================================================

void	Mod_Init(void);

const byte *Mod_ClusterPVS(int cluster, const model_t *model);

void Mod_Modellist_f(void);
void Mod_FreeAll(void);

extern	int	registration_sequence;

#endif	// __MODEL__
