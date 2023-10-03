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
 * Model loading and caching. Includes the .bsp file format
 *
 * =======================================================================
 */

// models are the only shared resource between a client and server running
// on the same machine.

#include <limits.h>
#include "header/local.h"

static YQ2_ALIGNAS_TYPE(int) byte mod_novis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 512
static model_t	mod_known[MAX_MOD_KNOWN];
static int	mod_numknown;
static int	mod_max = 0;

int	registration_sequence;

//===============================================================================

static qboolean
Mod_HasFreeSpace(void)
{
	int		i, used;
	model_t	*mod;

	used = 0;

	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence == registration_sequence)
		{
			used ++;
		}
	}

	if (mod_max < used)
	{
		mod_max = used;
	}

	// should same size of free slots as currently used
	return (mod_numknown + mod_max) < MAX_MOD_KNOWN;
}

const byte *
Mod_ClusterPVS(int cluster, const model_t *model)
{
	if ((cluster == -1) || !model->vis)
	{
		return mod_novis;
	}

	return Mod_DecompressVis((byte *)model->vis +
			model->vis->bitofs[cluster][DVIS_PVS],
			(model->vis->numclusters + 7) >> 3);
}

void
Mod_Modellist_f(void)
{
	int i, total, used;
	model_t *mod;
	qboolean freeup;

	total = 0;
	used = 0;
	R_Printf(PRINT_ALL, "Loaded models:\n");

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		char *in_use = "";

		if (mod->registration_sequence == registration_sequence)
		{
			in_use = "*";
			used ++;
		}

		if (!mod->name[0])
		{
			continue;
		}

		R_Printf(PRINT_ALL, "%8i : %s %s r: %f #%d\n",
			 mod->extradatasize, mod->name, in_use, mod->radius, mod->numsubmodels);
		total += mod->extradatasize;
	}

	R_Printf(PRINT_ALL, "Total resident: %i\n", total);
	// update statistics
	freeup = Mod_HasFreeSpace();
	R_Printf(PRINT_ALL, "Used %d of %d models%s.\n", used, mod_max, freeup ? ", has free space" : "");
}

void
Mod_Init(void)
{
	mod_max = 0;
	memset(mod_novis, 0xff, sizeof(mod_novis));
}

static void
Mod_LoadSubmodels(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dmodel_t *in;
	model_t	*out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		if (i == 0)
		{
			// copy parent as template for first model
			memcpy(out, loadmodel, sizeof(*out));
		}
		else
		{
			// copy first as template for model
			memcpy(out, loadmodel->submodels, sizeof(*out));
		}

		Com_sprintf (out->name, sizeof(out->name), "*%d", i);

		for (j = 0; j < 3; j++)
		{
			/* spread the mins / maxs by a pixel */
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->radius = Mod_RadiusFromBounds(out->mins, out->maxs);
		out->firstnode = LittleLong(in->headnode);
		out->firstmodelsurface = LittleLong(in->firstface);
		out->nummodelsurfaces = LittleLong(in->numfaces);
		// visleafs
		out->numleafs = 0;
		//  check limits
		if (out->firstnode >= loadmodel->numnodes)
		{
			ri.Sys_Error(ERR_DROP, "%s: Inline model %i has bad firstnode",
					__func__, i);
		}
	}
}

/*
 * Fills in s->texturemins[] and s->extents[]
 */
static void
Mod_CalcSurfaceExtents(model_t *loadmodel, msurface_t *s)
{
	float mins[2], maxs[2], val;
	int i;
	mtexinfo_t *tex;
	int bmins[2], bmaxs[2];

	mins[0] = mins[1] = (float)INT_MAX; // Set maximum values for world range
	maxs[0] = maxs[1] = (float)INT_MIN; // Set minimal values for world range

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		int e, j;
		mvertex_t *v;

		e = loadmodel->surfedges[s->firstedge + i];

		if (e >= 0)
		{
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		}
		else
		{
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		}

		for (j = 0; j < 2; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				  v->position[1] * tex->vecs[j][1] +
				  v->position[2] * tex->vecs[j][2] +
				  tex->vecs[j][3];

			if (val < mins[j])
			{
				mins[j] = val;
			}

			if (val > maxs[j])
			{
				maxs[j] = val;
			}
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if (s->extents[i] < 16)
			s->extents[i] = 16;	// take at least one cache block
		if ( !(tex->flags & (SURF_WARP|SURF_SKY)) && s->extents[i] > 256)
		{
			ri.Sys_Error(ERR_DROP, "%s: Bad surface extents", __func__);
		}
	}
}

static void
Mod_LoadFaces(model_t *loadmodel, const byte *mod_base, const lump_t *l,
	const bspx_header_t *bspx_header)
{
	int i, count, surfnum;
	msurface_t *out;
	dface_t *in;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + 6) * sizeof(*out));	// extra for skybox

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		int	side, ti, planenum;

		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);

		if (out->numedges < 3)
		{
			ri.Sys_Error(ERR_DROP, "%s: Surface with %d edges",
					__func__, out->numedges);
		}
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);

		if (side)
		{
			out->flags |= SURF_PLANEBACK;
		}

		if (planenum < 0 || planenum >= loadmodel->numplanes)
		{
			ri.Sys_Error(ERR_DROP, "%s: Incorrect %d planenum.",
					__func__, planenum);
		}
		out->plane = loadmodel->planes + planenum;

		ti = LittleShort(in->texinfo);

		if ((ti < 0) || (ti >= loadmodel->numtexinfo))
		{
			ri.Sys_Error(ERR_DROP, "%s: bad texinfo number",
					__func__);
		}

		out->texinfo = loadmodel->texinfo + ti;
		out->lmshift = DEFAULT_LMSHIFT;

		Mod_CalcSurfaceExtents (loadmodel, out);

		// lighting is saved as its with 24 bit color
		for (i=0 ; i<MAXLIGHTMAPS ; i++)
		{
			out->styles[i] = in->styles[i];
		}

		i = LittleLong(in->lightofs);

		if (i == -1)
		{
			out->samples = NULL;
		}
		else
		{
			out->samples = loadmodel->lightdata + i;
		}

		/* set the drawing flags flag */
		if (!out->texinfo->image)
			continue;

		if (r_fixsurfsky->value)
		{
			if (out->texinfo->flags & SURF_SKY)
			{
				out->flags |= SURF_DRAWSKY;
				continue;
			}
		}

		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;

			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}

		//==============
		// this marks flowing surfaces as turbulent.
		if (out->texinfo->flags & SURF_FLOWING)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}
		//==============
	}
}

static void
Mod_LoadQFaces(model_t *loadmodel, const byte *mod_base, const lump_t *l,
	const bspx_header_t *bspx_header)
{
	int i, count, surfnum;
	msurface_t *out;
	dqface_t *in;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + 6) * sizeof(*out));	// extra for skybox

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		int	side, ti, planenum;

		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleLong(in->numedges);

		if (out->numedges < 3)
		{
			ri.Sys_Error(ERR_DROP, "%s: Surface with %d edges",
					__func__, out->numedges);
		}
		out->flags = 0;

		planenum = LittleLong(in->planenum);
		side = LittleLong(in->side);

		if (side)
		{
			out->flags |= SURF_PLANEBACK;
		}

		if (planenum < 0 || planenum >= loadmodel->numplanes)
		{
			ri.Sys_Error(ERR_DROP, "%s: Incorrect %d planenum.",
					__func__, planenum);
		}
		out->plane = loadmodel->planes + planenum;

		ti = LittleLong(in->texinfo);

		if ((ti < 0) || (ti >= loadmodel->numtexinfo))
		{
			ri.Sys_Error(ERR_DROP, "%s: bad texinfo number",
					__func__);
		}

		out->texinfo = loadmodel->texinfo + ti;
		out->lmshift = DEFAULT_LMSHIFT;

		Mod_CalcSurfaceExtents (loadmodel, out);

		// lighting is saved as its with 24 bit color
		for (i=0 ; i<MAXLIGHTMAPS ; i++)
		{
			out->styles[i] = in->styles[i];
		}

		i = LittleLong(in->lightofs);

		if (i == -1)
		{
			out->samples = NULL;
		}
		else
		{
			out->samples = loadmodel->lightdata + i;
		}

		if (!out->texinfo->image)
			continue;

		if (r_fixsurfsky->value)
		{
			if (out->texinfo->flags & SURF_SKY)
			{
				out->flags |= SURF_DRAWSKY;
				continue;
			}
		}

		/* set the drawing flags */
		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;

			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}

		//==============
		// this marks flowing surfaces as turbulent.
		if (out->texinfo->flags & SURF_FLOWING)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}
		//==============
	}
}

static void
Mod_LoadLeafs(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dleaf_t *in;
	mleaf_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned firstleafface;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		// make unsigned long from signed short
		firstleafface = LittleShort(in->firstleafface) & 0xFFFF;
		out->nummarksurfaces = LittleShort(in->numleaffaces) & 0xFFFF;

		out->firstmarksurface = loadmodel->marksurfaces + firstleafface;
		if ((firstleafface + out->nummarksurfaces) > loadmodel->nummarksurfaces)
		{
			ri.Sys_Error(ERR_DROP, "%s: wrong marksurfaces position in %s",
				__func__, loadmodel->name);
		}
	}
}

static void
Mod_LoadQLeafs(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dqleaf_t *in;
	mleaf_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned firstleafface;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		out->contents = LittleLong(in->contents);
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);

		// make unsigned long from signed short
		firstleafface = LittleLong(in->firstleafface) & 0xFFFFFFFF;
		out->nummarksurfaces = LittleLong(in->numleaffaces) & 0xFFFFFFFF;

		out->firstmarksurface = loadmodel->marksurfaces + firstleafface;
		if ((firstleafface + out->nummarksurfaces) > loadmodel->nummarksurfaces)
		{
			ri.Sys_Error(ERR_DROP, "%s: wrong marksurfaces position in %s",
				__func__, loadmodel->name);
		}
	}
}

static void
Mod_LoadMarksurfaces(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	int i, count;
	short *in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		int j;

		j = LittleShort(in[i]);

		if ((j < 0) || (j >= loadmodel->numsurfaces))
		{
			ri.Sys_Error(ERR_DROP, "%s: bad surface number",
					__func__);
		}

		out[i] = loadmodel->surfaces + j;
	}
}

static void
Mod_LoadQMarksurfaces(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	int i, count;
	int *in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		int j;
		j = LittleLong(in[i]);

		if ((j < 0) || (j >= loadmodel->numsurfaces))
		{
			ri.Sys_Error(ERR_DROP, "%s: bad surface number",
					__func__);
		}

		out[i] = loadmodel->surfaces + j;
	}
}

static void
Mod_LoadBrushModel(model_t *mod, const void *buffer, int modfilelen)
{
	const bspx_header_t	*bspx_header;
	byte		*mod_base;
	dheader_t	*header;
	int			i;

	if (mod != mod_known)
	{
		ri.Sys_Error(ERR_DROP, "%s: Loaded a brush model after the world", __func__);
	}

	header = (dheader_t *)buffer;

	i = LittleLong(header->ident);

	if (i != IDBSPHEADER && i != QBSPHEADER)
	{
		ri.Sys_Error(ERR_DROP, "%s: %s has wrong ident (%i should be %i)",
				__func__, mod->name, i, IDBSPHEADER);
	}

	i = LittleLong(header->version);

	if (i != BSPVERSION && i != BSPDKMVERSION)
	{
		ri.Sys_Error(ERR_DROP, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod->name, i, BSPVERSION);
	}

	/* swap all the lumps */
	mod_base = (byte *)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		((int *)header)[i] = LittleLong(((int *)header)[i]);
	}

	// calculate the needed hunksize from the lumps
	int hunkSize = 0;
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_VERTEXES], sizeof(dvertex_t), sizeof(mvertex_t), 8);
	if (header->ident == IDBSPHEADER)
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_EDGES], sizeof(dedge_t), sizeof(medge_t), 13);
	}
	else
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_EDGES], sizeof(dqedge_t), sizeof(medge_t), 13);
	}
	hunkSize += sizeof(medge_t) + 31; // for count+1 in Mod_LoadEdges()
	int surfEdgeCount = (header->lumps[LUMP_SURFEDGES].filelen+sizeof(int)-1)/sizeof(int);
	if(surfEdgeCount < MAX_MAP_SURFEDGES) // else it errors out later anyway
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_SURFEDGES], sizeof(int), sizeof(int), 24);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_TEXINFO], sizeof(texinfo_t), sizeof(mtexinfo_t), 6);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LIGHTING], 1, 1, 0);
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_PLANES], sizeof(dplane_t), sizeof(cplane_t)*2, 6);
	if (header->ident == IDBSPHEADER)
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_FACES], sizeof(dface_t), sizeof(msurface_t), 6);
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFFACES], sizeof(short), sizeof(msurface_t *), 0); // yes, out is indeed a pointer!
	}
	else
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_FACES], sizeof(dqface_t), sizeof(msurface_t), 6);
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFFACES], sizeof(int), sizeof(msurface_t *), 0); // yes, out is indeed a pointer!
	}
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_VISIBILITY], 1, 1, 0);
	if (header->ident == IDBSPHEADER)
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFS], sizeof(dleaf_t), sizeof(mleaf_t), 0);
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_NODES], sizeof(dnode_t), sizeof(mnode_t), 0);
	}
	else
	{
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_LEAFS], sizeof(dqleaf_t), sizeof(mleaf_t), 0);
		hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_NODES], sizeof(dqnode_t), sizeof(mnode_t), 0);
	}
	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_MODELS], sizeof(dmodel_t), sizeof(model_t), 0);

	hunkSize += 5000000; // and 5MB extra just in case
	mod->extradata = Hunk_Begin(hunkSize);
	mod->type = mod_brush;


	/* check for BSPX extensions */
	bspx_header = Mod_LoadBSPX(modfilelen, (byte*)header);

	/* load into heap */
	Mod_LoadVertexes(mod->name, &mod->vertexes, &mod->numvertexes, mod_base,
		&header->lumps[LUMP_VERTEXES], 8);
	if (header->ident == IDBSPHEADER)
	{
		Mod_LoadEdges(mod->name, &mod->edges, &mod->numedges,
			mod_base, &header->lumps[LUMP_EDGES], 13);
	}
	else
	{
		Mod_LoadQEdges(mod->name, &mod->edges, &mod->numedges,
			mod_base, &header->lumps[LUMP_EDGES], 13);
	}
	Mod_LoadSurfedges(mod->name, &mod->surfedges, &mod->numsurfedges,
		mod_base, &header->lumps[LUMP_SURFEDGES], 24);
	Mod_LoadLighting(&mod->lightdata, mod_base, &header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(mod->name, &mod->planes, &mod->numplanes,
		mod_base, &header->lumps[LUMP_PLANES], 6);
	Mod_LoadTexinfo(mod->name, &mod->texinfo, &mod->numtexinfo,
		mod_base, &header->lumps[LUMP_TEXINFO], (findimage_t)R_FindImage,
		r_notexture_mip, 6);
	if (header->ident == IDBSPHEADER)
	{
		Mod_LoadFaces(mod, mod_base, &header->lumps[LUMP_FACES], bspx_header);
		Mod_LoadMarksurfaces(mod, mod_base, &header->lumps[LUMP_LEAFFACES]);
	}
	else
	{
		Mod_LoadQFaces(mod, mod_base, &header->lumps[LUMP_FACES], bspx_header);
		Mod_LoadQMarksurfaces(mod, mod_base, &header->lumps[LUMP_LEAFFACES]);
	}
	Mod_LoadVisibility(&mod->vis, mod_base, &header->lumps[LUMP_VISIBILITY]);
	if (header->ident == IDBSPHEADER)
	{
		Mod_LoadLeafs(mod, mod_base, &header->lumps[LUMP_LEAFS]);
		Mod_LoadNodes(mod->name, mod->planes, mod->numplanes, mod->leafs,
			mod->numleafs, &mod->nodes, &mod->numnodes, mod_base,
			&header->lumps[LUMP_NODES]);
	}
	else
	{
		Mod_LoadQLeafs(mod, mod_base, &header->lumps[LUMP_LEAFS]);
		Mod_LoadQNodes(mod->name, mod->planes, mod->numplanes, mod->leafs,
			mod->numleafs, &mod->nodes, &mod->numnodes, mod_base,
			&header->lumps[LUMP_NODES]);
	}
	Mod_LoadSubmodels(mod, mod_base, &header->lumps[LUMP_MODELS]);
	mod->numframes = 2; /* regular and alternate animation */

	R_InitSkyBox(mod);
}

/*
 * Loads in a model for the given name
 */
static model_t *
Mod_ForName(const char *name, model_t *parent_model, qboolean crash)
{
	model_t *mod;
	void *buf;
	int i, modfilelen;

	if (!name[0])
	{
		ri.Sys_Error(ERR_DROP, "%s: NULL name", __func__);
	}

	/* inline models are grabbed only from worldmodel */
	if (name[0] == '*' && parent_model)
	{
		i = (int)strtol(name + 1, (char **)NULL, 10);

		if (i < 1 || i >= parent_model->numsubmodels)
		{
			ri.Sys_Error(ERR_DROP, "%s: bad inline model number",
					__func__);
		}

		return &parent_model->submodels[i];
	}

	/* search the currently loaded models */
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			continue;
		}

		if (!strcmp(mod->name, name))
		{
			return mod;
		}
	}

	/* find a free model slot spot */
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			break; /* free spot */
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
			ri.Sys_Error(ERR_DROP, "%s: mod_numknown == MAX_MOD_KNOWN", __func__);
		}

		mod_numknown++;
	}

	strcpy(mod->name, name);

	/* load the file */
	modfilelen = Mod_LoadFile (mod->name, &buf);

	if (!buf)
	{
		if (crash)
		{
			ri.Sys_Error(ERR_DROP, "%s: %s not found",
					__func__, mod->name);
		}

		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	/* call the apropriate loader */
	switch (LittleLong(*(unsigned *)buf))
	{
		case DKMHEADER:
			/* fall through */
		case RAVENFMHEADER:
			/* fall through */
		case IDALIASHEADER:
			/* fall through */
		case IDMDLHEADER:
			{
				mod->extradata = Mod_LoadAliasModel(mod->name, buf, modfilelen,
					mod->mins, mod->maxs,
					(struct image_s **)mod->skins, (findimage_t)R_FindImage,
					&(mod->type));
				if (!mod->extradata)
				{
					ri.Sys_Error(ERR_DROP, "%s: Failed to load %s",
						__func__, mod->name);
				}
			};
			break;

		case IDSPRITEHEADER:
			{
				mod->extradata = Mod_LoadSP2(mod->name, buf, modfilelen,
					(struct image_s **)mod->skins, (findimage_t)R_FindImage,
					&(mod->type));
				if (!mod->extradata)
				{
					ri.Sys_Error(ERR_DROP, "%s: Failed to load %s",
						__func__, mod->name);
				}
			}
			break;

		case IDBSPHEADER:
			/* fall through */
		case QBSPHEADER:
			Mod_LoadBrushModel(mod, buf, modfilelen);
			break;

		default:
			ri.Sys_Error(ERR_DROP, "%s: unknown fileid for %s",
					__func__, mod->name);
			break;
	}

	mod->radius = Mod_RadiusFromBounds(mod->mins, mod->maxs);
	mod->extradatasize = Hunk_End();

	ri.FS_FreeFile(buf);

	return mod;
}

void
Mod_Free(model_t *mod)
{
	Hunk_Free(mod->extradata);
	memset(mod, 0, sizeof(*mod));
}

void
Mod_FreeAll(void)
{
	int i;

	for (i = 0; i < mod_numknown; i++)
	{
		if (mod_known[i].extradatasize)
		{
			Mod_Free(&mod_known[i]);
		}
	}
}

/*
 * Specifies the model that will be used as the world
 */
void
RE_BeginRegistration(const char *model)
{
	char fullname[MAX_QPATH];
	cvar_t *flushmap;

	registration_sequence++;
	r_oldviewcluster = -1; /* force markleafs */

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	D_FlushCaches ();
	/* explicitly free the old map if different
	   this guarantees that mod_known[0] is the
	   world map */
	flushmap = ri.Cvar_Get("flushmap", "0", 0);

	if (strcmp(mod_known[0].name, fullname) || flushmap->value)
	{
		Mod_Free(&mod_known[0]);
	}

	r_worldmodel = Mod_ForName(fullname, NULL, true);

	r_viewcluster = -1;
}

struct model_s *
RE_RegisterModel(const char *name)
{
	model_t *mod;

	mod = Mod_ForName(name, r_worldmodel, false);

	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		/* register any images used by the models */
		if (mod->type == mod_brush)
		{
			int i;

			for (i = 0; i < mod->numtexinfo; i++)
			{
				mod->texinfo[i].image->registration_sequence =
					registration_sequence;
			}
		}
		else
		{
			/* numframes is unused for SP2 but lets set it also  */
			mod->numframes = Mod_ReLoadSkins((struct image_s **)mod->skins,
				(findimage_t)R_FindImage, mod->extradata, mod->type);
		}
	}

	return mod;
}

void
RE_EndRegistration(void)
{
	int i;
	model_t *mod;

	if (Mod_HasFreeSpace() && R_ImageHasFreeSpace())
	{
		// should be enough space for load next maps
		return;
	}

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			continue;
		}

		if (mod->registration_sequence != registration_sequence)
		{
			/* don't need this model */
			Mod_Free(mod);
		}
	}

	R_FreeUnusedImages();
}
