/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 2005-2015 David HENRY
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
 * The maps file format
 *
 * =======================================================================
 */

#include "../ref_shared.h"
#include "maps.h"

/*
=================
Mod_SetParent
=================
*/
static void
Mod_SetParent(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != CONTENTS_NODE)
	{
		return;
	}

	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_NumberLeafs
=================
*/
static void
Mod_NumberLeafs(mleaf_t *leafs, mnode_t *node, int *r_leaftovis, int *r_vistoleaf,
	int *numvisleafs)
{
	if (node->contents != CONTENTS_NODE)
	{
		const mleaf_t *leaf;
		int leafnum;

		leaf = (mleaf_t *)node;
		leafnum = leaf - leafs;
		if (leaf->contents & CONTENTS_SOLID)
		{
			return;
		}

		r_leaftovis[leafnum] = *numvisleafs;
		r_vistoleaf[*numvisleafs] = leafnum;
		(*numvisleafs) ++;
		return;
	}

	Mod_NumberLeafs(leafs, node->children[0], r_leaftovis, r_vistoleaf,
		numvisleafs);
	Mod_NumberLeafs(leafs, node->children[1], r_leaftovis, r_vistoleaf,
		numvisleafs);
}

static void
Mod_LoadNodes(const char *name, cplane_t *planes, int numplanes, mleaf_t *leafs,
	int numleafs, mnode_t **nodes, int *numnodes, const byte *mod_base,
	const lump_t *l)
{
	int	r_leaftovis[MAX_MAP_LEAFS], r_vistoleaf[MAX_MAP_LEAFS];
	int	i, count, numvisleafs;
	dnode_t	*in;
	mnode_t	*out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_NODES) * sizeof(*out));

	*nodes = out;
	*numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		int j, planenum;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		planenum = LittleLong(in->planenum);
		if (planenum  < 0 || planenum >= numplanes)
		{
			Com_Error(ERR_DROP, "%s: Incorrect %d < %d planenum.",
					__func__, planenum, numplanes);
		}
		out->plane = planes + planenum;

		out->firstsurface = LittleShort(in->firstface) & 0xFFFF;
		out->numsurfaces = LittleShort(in->numfaces) & 0xFFFF;
		out->contents = CONTENTS_NODE; /* differentiate from leafs */

		for (j = 0; j < 2; j++)
		{
			int leafnum;

			leafnum = LittleLong(in->children[j]);

			if (leafnum >= 0)
			{
				if (leafnum  < 0 || leafnum >= *numnodes)
				{
					Com_Error(ERR_DROP, "%s: Incorrect %d nodenum as leaf.",
							__func__, leafnum);
				}

				out->children[j] = *nodes + leafnum;
			}
			else
			{
				leafnum = -1 - leafnum;
				if (leafnum  < 0 || leafnum >= numleafs)
				{
					Com_Error(ERR_DROP, "%s: Incorrect %d leafnum.",
							__func__, leafnum);
				}

				out->children[j] = (mnode_t *)(leafs + leafnum);
			}
		}
	}

	Mod_SetParent(*nodes, NULL); /* sets nodes and leafs */

	numvisleafs = 0;
	Mod_NumberLeafs(leafs, *nodes, r_leaftovis, r_vistoleaf, &numvisleafs);
}

static void
Mod_LoadQNodes(const char *name, cplane_t *planes, int numplanes, mleaf_t *leafs,
	int numleafs, mnode_t **nodes, int *numnodes, const byte *mod_base,
	const lump_t *l)
{
	int	r_leaftovis[MAX_MAP_LEAFS], r_vistoleaf[MAX_MAP_LEAFS];
	int	i, count, numvisleafs;
	dqnode_t	*in;
	mnode_t	*out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_NODES) * sizeof(*out));

	*nodes = out;
	*numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		int j, planenum;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleFloat(in->mins[j]);
			out->minmaxs[3 + j] = LittleFloat(in->maxs[j]);
		}

		planenum = LittleLong(in->planenum);
		if (planenum  < 0 || planenum >= numplanes)
		{
			Com_Error(ERR_DROP, "%s: Incorrect %d < %d planenum.",
					__func__, planenum, numplanes);
		}
		out->plane = planes + planenum;

		out->firstsurface = LittleLong(in->firstface) & 0xFFFFFFFF;
		out->numsurfaces = LittleLong(in->numfaces) & 0xFFFFFFFF;
		out->contents = CONTENTS_NODE; /* differentiate from leafs */

		for (j = 0; j < 2; j++)
		{
			int leafnum;

			leafnum = LittleLong(in->children[j]);

			if (leafnum >= 0)
			{
				if (leafnum  < 0 || leafnum >= *numnodes)
				{
					Com_Error(ERR_DROP, "%s: Incorrect %d nodenum as leaf.",
							__func__, leafnum);
				}

				out->children[j] = *nodes + leafnum;
			}
			else
			{
				leafnum = -1 - leafnum;
				if (leafnum  < 0 || leafnum >= numleafs)
				{
					Com_Error(ERR_DROP, "%s: Incorrect %d leafnum.",
							__func__, leafnum);
				}

				out->children[j] = (mnode_t *)(leafs + leafnum);
			}
		}
	}

	Mod_SetParent(*nodes, NULL); /* sets nodes and leafs */

	numvisleafs = 0;
	Mod_NumberLeafs(leafs, *nodes, r_leaftovis, r_vistoleaf, &numvisleafs);
}

void
Mod_LoadQBSPNodes(const char *name, cplane_t *planes, int numplanes, mleaf_t *leafs,
	int numleafs, mnode_t **nodes, int *numnodes, const byte *mod_base,
	const lump_t *l, int ident)
{
	if (ident == IDBSPHEADER)
	{
		Mod_LoadNodes(name, planes, numplanes, leafs, numleafs, nodes, numnodes,
			mod_base, l);
	}
	else
	{
		Mod_LoadQNodes(name, planes, numplanes, leafs, numleafs, nodes, numnodes,
			mod_base, l);
	}
}

/*
=================
Mod_LoadVertexes

extra for skybox
=================
*/
void
Mod_LoadVertexes(const char *name, mvertex_t **vertexes, int *numvertexes,
	const byte *mod_base, const lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int	i, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_VERTEXES) * sizeof(*out));

	/*
	 * FIXME: Recheck with soft render
	 * Fix for the problem where the games dumped core
	 * when changing levels.
	 */
	memset(out, 0, (count + EXTRA_LUMP_VERTEXES) * sizeof(*out));

	*vertexes = out;
	*numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void
Mod_LoadLighting(byte **lightdata, int *size, const byte *mod_base, const lump_t *l)
{
	if (!l->filelen)
	{
		*lightdata = NULL;
		*size = 0;
		return;
	}

	*size = l->filelen;
	*lightdata = Hunk_Alloc(*size);
	memcpy(*lightdata, mod_base + l->fileofs, *size);
}

void
Mod_LoadSetSurfaceLighting(byte *lightdata, int size, msurface_t *out,
	const byte *styles, int lightofs)
{
	int i;

	/* lighting info */
	for (i = 0; i < MAXLIGHTMAPS; i++)
	{
		out->styles[i] = styles[i];
	}

	i = LittleLong(lightofs);
	if (i == -1 || lightdata == NULL || i >= size)
	{
		out->samples = NULL;
	}
	else
	{
		out->samples = lightdata + i;
	}
}

/*
 * Fills in s->texturemins[] and s->extents[]
 */
void
Mod_CalcSurfaceExtents(const int *surfedges, mvertex_t *vertexes, medge_t *edges,
	msurface_t *s)
{
	double mins[2], maxs[2];
	mtexinfo_t *tex;
	int i;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		int e, j;
		mvertex_t *v;

		e = surfedges[s->firstedge + i];

		if (e >= 0)
		{
			v = &vertexes[edges[e].v[0]];
		}
		else
		{
			v = &vertexes[edges[-e].v[1]];
		}

		for (j = 0; j < 2; j++)
		{
			float val;

			val = (double)v->position[0] * tex->vecs[j][0] +
				  (double)v->position[1] * tex->vecs[j][1] +
				  (double)v->position[2] * tex->vecs[j][2] +
				  (double)tex->vecs[j][3];

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
		int bmins[2], bmaxs[2];

		bmins[i] = floor(mins[i] / (1 << s->lmshift));
		bmaxs[i] = ceil(maxs[i] / (1 << s->lmshift));

		s->texturemins[i] = bmins[i] * (1 << s->lmshift);
		s->extents[i] = (bmaxs[i] - bmins[i]) * (1 << s->lmshift);
		if (s->extents[i] < 16)
		{
			/* take at least one cache block */
			s->extents[i] = 16;
		}
	}
}

static int
Mod_LoadSurfConvertFlags(int flags, maptype_t maptype)
{
	const int *convert;
	int sflags = 0;
	int i;

	switch (maptype)
	{
		case map_heretic2: convert = heretic2_flags; break;
		case map_daikatana: convert = daikatana_flags; break;
		case map_kingpin: convert = kingpin_flags; break;
		case map_anachronox: convert = anachronox_flags; break;
		default: convert = NULL; break;
	}

	if (!convert)
	{
		return flags;
	}

	for (i = 0; i < 32; i++)
	{
		if (flags & (1 << i))
		{
			sflags |= convert[i];
		}
	}

	return sflags;
}

/*
=================
Mod_LoadTexinfo

extra for skybox in soft render
=================
*/
void
Mod_LoadTexinfo(const char *name, mtexinfo_t **texinfo, int *numtexinfo,
	const byte *mod_base, const lump_t *l, findimage_t find_image,
	struct image_s *notexture, maptype_t maptype)
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_TEXINFO) * sizeof(*out));

	*texinfo = out;
	*numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		struct image_s *image;
		int j, next;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		/* Convert flags for game type */
		out->flags = Mod_LoadSurfConvertFlags(LittleLong(in->flags), maptype);

		next = LittleLong(in->nexttexinfo);
		if (next > 0)
		{
			out->next = *texinfo + next;
		}
		else
		{
			/*
			 * Fix for the problem where the game
			 * domed core when loading a new level.
			 */
			out->next = NULL;
		}

		image = GetTexImage(in->texture, find_image);
		if (!image)
		{
			R_Printf(PRINT_ALL, "%s: Couldn't load %s\n",
				__func__, in->texture);
			image = notexture;
		}

		out->image = image;
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = (*texinfo) + i;
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
		{
			out->numframes++;
		}
	}
}

/*
=================
Mod_LoadEdges

extra is used for skybox, which adds 6 surfaces
=================
*/
static void
Mod_LoadEdges(const char *name, medge_t **edges, int *numedges,
	const byte *mod_base, const lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_EDGES) * sizeof(*out));

	*edges = out;
	*numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadQEdges

extra is used for skybox, which adds 6 surfaces
=================
*/
static void
Mod_LoadQEdges(const char *name, medge_t **edges, int *numedges,
	const byte *mod_base, const lump_t *l)
{
	dqedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_EDGES) * sizeof(*out));

	*edges = out;
	*numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned int)LittleLong(in->v[0]);
		out->v[1] = (unsigned int)LittleLong(in->v[1]);
	}
}

void
Mod_LoadQBSPEdges(const char *name, medge_t **edges, int *numedges,
	const byte *mod_base, const lump_t *l, int ident)
{
	if (ident == IDBSPHEADER)
	{
		Mod_LoadEdges(name, edges, numedges, mod_base, l);
	}
	else
	{
		Mod_LoadQEdges(name, edges, numedges, mod_base, l);
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void
Mod_LoadSurfedges(const char *name, int **surfedges, int *numsurfedges,
	const byte *mod_base, const lump_t *l)
{
	const int *in;
	int i, count;
	int *out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_SURFEDGES) * sizeof(*out));	// extra for skybox

	*surfedges = out;
	*numsurfedges = count;

	for ( i=0 ; i<count ; i++)
	{
		out[i] = LittleLong(in[i]);
	}
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *
Mod_PointInLeaf(const vec3_t p, mnode_t *node)
{
	if (!node)
	{
		Com_Error(ERR_DROP, "%s: bad node.", __func__);
		return NULL;
	}

	while (1)
	{
		float d;
		cplane_t *plane;

		if (node->contents != CONTENTS_NODE)
		{
			return (mleaf_t *)node;
		}

		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;

		if (d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return NULL; /* never reached */
}

const void *
Mod_LoadBSPXFindLump(const bspx_header_t *bspx_header, const char *lumpname,
	int *plumpsize, const byte *mod_base)
{
	bspx_lump_t *lump;
	int i, numlumps;

	if (!bspx_header) {
		return NULL;
	}

	numlumps = LittleLong(bspx_header->numlumps);

	lump = (bspx_lump_t*)(bspx_header + 1);
	for (i = 0; i < numlumps; i++, lump++) {
		if (!strncmp(lump->lumpname, lumpname, sizeof(lump->lumpname))) {
			if (plumpsize) {
				*plumpsize = LittleLong(lump->filelen);
			}
			return mod_base + LittleLong(lump->fileofs);
		}
	}

	return NULL;
}

const bspx_header_t *
Mod_LoadBSPX(int filesize, const byte *mod_base)
{
	const bspx_header_t *xheader;
	const dheader_t *header;
	int i, numlumps, xofs;
	bspx_lump_t *lump;

	/* find end of last lump */
	header = (dheader_t*)mod_base;
	xofs = 0;

	numlumps = HEADER_LUMPS;
	if (header->version == BSPDKMVERSION)
	{
		numlumps = 21;
	}

	for (i = 0; i < numlumps; i++) {
		xofs = Q_max(xofs,
			(header->lumps[i].fileofs + header->lumps[i].filelen + 3) & ~3);
	}

	if (xofs + sizeof(bspx_header_t) > filesize) {
		return NULL;
	}

	xheader = (bspx_header_t*)(mod_base + xofs);
	if (LittleLong(xheader->ident) != BSPXHEADER)
	{
		R_Printf(PRINT_ALL, "%s: Incorrect header ident.\n", __func__);
		return NULL;
	}

	numlumps = LittleLong(xheader->numlumps);

	if (numlumps < 0 ||
		(xofs + sizeof(bspx_header_t) + numlumps * sizeof(bspx_lump_t)) > filesize)
	{
		return NULL;
	}

	// byte-swap and check sanity
	lump = (bspx_lump_t*)(xheader + 1); // lumps immediately follow the header
	for (i = 0; i < numlumps; i++, lump++)
	{
		int fileofs, filelen;

		fileofs = LittleLong(lump->fileofs);
		filelen = LittleLong(lump->filelen);
		if (fileofs < 0 || filelen < 0 || (fileofs + filelen) > filesize) {
			return NULL;
		}
	}

	// success
	return xheader;
}

/* Extension to support lightmaps that aren't tied to texture scale. */
int
Mod_LoadBSPXDecoupledLM(const dlminfo_t* lminfos, int surfnum, msurface_t *out)
{
	int lmwidth, lmheight, i;
	const dlminfo_t *lminfo;
	float v0, v1;

	if (lminfos == NULL)
	{
		return -1;
	}

	lminfo = lminfos + surfnum;

	lmwidth = LittleShort(lminfo->lmwidth);
	lmheight = LittleShort(lminfo->lmheight);

	if (lmwidth <= 0 || lmheight <= 0) {
		return -1;
	}

	for (i = 0; i < 2; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			out->lmvecs[i][j] = LittleFloat(lminfo->vecs[i][j]);
		}
	}

	out->extents[0] = (short)(lmwidth - 1);
	out->extents[1] = (short)(lmheight - 1);
	out->lmshift = 0;
	out->texturemins[0] = 0;
	out->texturemins[1] = 0;

	v0 = VectorLength(out->lmvecs[0]);
	out->lmvlen[0] = v0 > 0.0f ? 1.0f / v0 : 0.0f;

	v1 = VectorLength(out->lmvecs[1]);
	out->lmvlen[1] = v1 > 0.0f ? 1.0f / v1 : 0.0f;

	return LittleLong(lminfo->lightofs);
}

static void
Mod_LoadMarksurfaces(const char *name, msurface_t ***marksurfaces, unsigned int *nummarksurfaces,
	msurface_t *surfaces, int numsurfaces, const byte *mod_base, const lump_t *l)
{
	int i, count;
	const short *in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	*marksurfaces = out;
	*nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		int j;

		j = LittleShort(in[i]) & 0xFFFF;

		if ((j < 0) || (j >= numsurfaces))
		{
			Com_Error(ERR_DROP, "%s: bad surface number",
					__func__);
		}

		out[i] = surfaces + j;
	}
}

static void
Mod_LoadQMarksurfaces(const char *name, msurface_t ***marksurfaces, unsigned int *nummarksurfaces,
	msurface_t *surfaces, int numsurfaces, const byte *mod_base, const lump_t *l)
{
	int i, count;
	const int *in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	*marksurfaces = out;
	*nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		int j;
		j = LittleLong(in[i]) & 0xFFFFFFFF;

		if ((j < 0) || (j >= numsurfaces))
		{
			Com_Error(ERR_DROP, "%s: bad surface number",
					__func__);
		}

		out[i] = surfaces + j;
	}
}

void
Mod_LoadQBSPMarksurfaces(const char *name, msurface_t ***marksurfaces, unsigned int *nummarksurfaces,
	msurface_t *surfaces, int numsurfaces, const byte *mod_base, const lump_t *l, int ident)
{
	if (ident == IDBSPHEADER)
	{
		Mod_LoadMarksurfaces(name, marksurfaces, nummarksurfaces,
			surfaces, numsurfaces, mod_base, l);
	}
	else
	{
		Mod_LoadQMarksurfaces(name, marksurfaces, nummarksurfaces,
			surfaces, numsurfaces, mod_base, l);
	}
}

static void
Mod_LoadLeafs(const char *name, mleaf_t **leafs, int *numleafs,
	msurface_t **marksurfaces, unsigned int nummarksurfaces,
	const byte *mod_base, const lump_t *l)
{
	dleaf_t *in;
	mleaf_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_LEAFS) * sizeof(*out));

	*leafs = out;
	*numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned int firstleafface;

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

		out->firstmarksurface = marksurfaces + firstleafface;
		if ((firstleafface + out->nummarksurfaces) > nummarksurfaces)
		{
			Com_Error(ERR_DROP, "%s: wrong marksurfaces position in %s",
				__func__, name);
		}
	}
}

static void
Mod_LoadQLeafs(const char *name, mleaf_t **leafs, int *numleafs,
	msurface_t **marksurfaces, unsigned int nummarksurfaces,
	const byte *mod_base, const lump_t *l)
{
	dqleaf_t *in;
	mleaf_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_LEAFS) * sizeof(*out));

	*leafs = out;
	*numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned int firstleafface;

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

		out->firstmarksurface = marksurfaces + firstleafface;
		if ((firstleafface + out->nummarksurfaces) > nummarksurfaces)
		{
			Com_Error(ERR_DROP, "%s: wrong marksurfaces position in %s",
				__func__, name);
		}
	}
}

void
Mod_LoadQBSPLeafs(const char *name, mleaf_t **leafs, int *numleafs,
	msurface_t **marksurfaces, unsigned int nummarksurfaces,
	const byte *mod_base, const lump_t *l, int ident)
{
	if (ident == IDBSPHEADER)
	{
		Mod_LoadLeafs(name, leafs, numleafs, marksurfaces, nummarksurfaces,
			mod_base, l);
	}
	else
	{
		Mod_LoadQLeafs(name, leafs, numleafs, marksurfaces, nummarksurfaces,
			mod_base, l);
	}
}

/* Need to clean */
struct rctx_s {
	const byte *data;
	int ofs, size;
};

static byte ReadByte(struct rctx_s *ctx)
{
	if (ctx->ofs >= ctx->size)
	{
		ctx->ofs++;
		return 0;
	}
	return ctx->data[ctx->ofs++];
}

static int ReadInt(struct rctx_s *ctx)
{
	int r = (int)ReadByte(ctx)<<0;
		r|= (int)ReadByte(ctx)<<8;
		r|= (int)ReadByte(ctx)<<16;
		r|= (int)ReadByte(ctx)<<24;
	return r;
}

static float ReadFloat(struct rctx_s *ctx)
{
	union {float f; int i;} u;
	u.i = ReadInt(ctx);
	return u.f;
}

bspxlightgrid_t*
Mod_LoadBSPXLightGrid(const bspx_header_t *bspx_header, const byte *mod_base)
{
	vec3_t step, mins;
	int size[3];
	bspxlightgrid_t *grid;
	unsigned int numstyles, numnodes, numleafs, rootnode;
	unsigned int nodestart, leafsamps = 0, i, j, k, s;
	struct bspxlgsamp_s *samp;
	struct rctx_s ctx = {0};

	ctx.data = Mod_LoadBSPXFindLump(bspx_header, "LIGHTGRID_OCTREE", &ctx.size, mod_base);
	if (!ctx.data)
	{
		return NULL;
	}

	for (j = 0; j < 3; j++)
		step[j] = ReadFloat(&ctx);
	for (j = 0; j < 3; j++)
		size[j] = ReadInt(&ctx);
	for (j = 0; j < 3; j++)
		mins[j] = ReadFloat(&ctx);

	numstyles = ReadByte(&ctx);	//urgh, misaligned the entire thing
	rootnode = ReadInt(&ctx);
	numnodes = ReadInt(&ctx);
	nodestart = ctx.ofs;
	ctx.ofs += (3+8)*4*numnodes;
	numleafs = ReadInt(&ctx);
	for (i = 0; i < numleafs; i++)
	{
		unsigned int lsz[3];
		ctx.ofs += 3*4;
		for (j = 0; j < 3; j++)
			lsz[j] = ReadInt(&ctx);
		j = lsz[0]*lsz[1]*lsz[2];
		leafsamps += j;
		while (j --> 0)
		{	//this loop is annonying, memcpy dreams...
			s = ReadByte(&ctx);
			if (s == 255)
				continue;
			ctx.ofs += s*4;
		}
	}

	grid = Hunk_Alloc(sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
	memset(grid, 0xcc, sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
	grid->leafs = (void*)(grid+1);
	grid->nodes = (void*)(grid->leafs + numleafs);
	samp = (void*)(grid->nodes+numnodes);

	for (j = 0; j < 3; j++)
		grid->gridscale[j] = 1/step[j];	//prefer it as a multiply
	VectorCopy(mins, grid->mins);
	VectorCopy(size, grid->count);
	grid->numnodes = numnodes;
	grid->numleafs = numleafs;
	grid->rootnode = rootnode;
	(void)numstyles;

	//rewind to the nodes. *sigh*
	ctx.ofs = nodestart;
	for (i = 0; i < numnodes; i++)
	{
		for (j = 0; j < 3; j++)
			grid->nodes[i].mid[j] = ReadInt(&ctx);
		for (j = 0; j < 8; j++)
			grid->nodes[i].child[j] = ReadInt(&ctx);
	}
	ctx.ofs += 4;
	for (i = 0; i < numleafs; i++)
	{
		for (j = 0; j < 3; j++)
			grid->leafs[i].mins[j] = ReadInt(&ctx);
		for (j = 0; j < 3; j++)
			grid->leafs[i].size[j] = ReadInt(&ctx);

		grid->leafs[i].rgbvalues = samp;

		j = grid->leafs[i].size[0]*grid->leafs[i].size[1]*grid->leafs[i].size[2];
		while (j --> 0)
		{
			s = ReadByte(&ctx);
			if (s == 0xff)
				memset(samp, 0xff, sizeof(*samp));
			else
			{
				for (k = 0; k < s; k++)
				{
					if (k >= 4)
						ReadInt(&ctx);
					else
					{
						samp->map[k].style = ReadByte(&ctx);
						samp->map[k].rgb[0] = ReadByte(&ctx);
						samp->map[k].rgb[1] = ReadByte(&ctx);
						samp->map[k].rgb[2] = ReadByte(&ctx);
					}
				}
				for (; k < 4; k++)
				{
					samp->map[k].style = (byte)~0u;
					samp->map[k].rgb[0] =
					samp->map[k].rgb[1] =
					samp->map[k].rgb[2] = 0;
				}
			}
			samp++;
		}
	}

	if (ctx.ofs != ctx.size)
		grid = NULL;

	return grid;
}
