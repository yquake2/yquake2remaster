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

#include "header/local.h"

static byte *mod_novis = NULL;
static size_t mod_novis_len = 0;

static model_t mod_known[MAX_MOD_KNOWN];
static int mod_numknown = 0;
static int mod_max = 0;
int registration_sequence;

//===============================================================================

static qboolean
Mod_HasFreeSpace(void)
{
	int		i, used;
	model_t	*mod;

	used = 0;

	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->s.name[0])
			continue;
		if (mod->s.registration_sequence == registration_sequence)
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
	if (!mod_novis)
	{
		Com_Error(ERR_DROP, "%s: incrorrect init of PVS/PHS", __func__);
		return mod_novis;
	}

	if (!model->s.vis)
	{
		Mod_DecompressVis(NULL, mod_novis, NULL,
			(model->s.numclusters + 7) >> 3);
		return mod_novis;
	}

	if (cluster == -1)
	{
		memset(mod_novis, 0, (model->s.numclusters + 7) >> 3);
		return mod_novis;
	}

	if (cluster < 0 || cluster >= model->s.numvisibility)
	{
		Com_Error(ERR_DROP, "%s: bad cluster", __func__);
		return mod_novis;
	}

	Mod_DecompressVis((byte *)model->s.vis +
			model->s.vis->bitofs[cluster][DVIS_PVS], mod_novis,
			(byte *)model->s.vis + model->s.numvisibility,
			(model->s.numclusters + 7) >> 3);
	return mod_novis;
}

void
Mod_Modellist_f(void)
{
	int i, total, used;
	model_t *mod;
	qboolean freeup;

	total = 0;
	used = 0;
	Com_Printf("Loaded models:\n");

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		char *in_use = "";

		if (mod->s.registration_sequence == registration_sequence)
		{
			in_use = "*";
			used ++;
		}

		if (!mod->s.name[0])
		{
			continue;
		}

		Com_Printf("%8i : %s %s r: %.2f #%d\n",
			 mod->s.extradatasize, mod->s.name, in_use, mod->s.radius, mod->numsubmodels);
		total += mod->s.extradatasize;
	}

	Com_Printf("Total resident: %i\n", total);
	// update statistics
	freeup = Mod_HasFreeSpace();
	Com_Printf("Used %d of %d models%s.\n", used, mod_max, freeup ? ", has free space" : "");
}

void
Mod_Init(void)
{
	mod_max = 0;
	mod_novis = NULL;
	mod_novis_len = 0;
}

static void
Mod_LoadSubmodels(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dmodel_t *in;
	model_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->s.name);
		return;
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		if (i == 0)
		{
			/* copy parent as template for first model */
			memcpy(out, loadmodel, sizeof(*out));
		}
		else
		{
			/* copy first as template for model */
			memmove(out, loadmodel->submodels, sizeof(*out));
		}

		Com_sprintf (out->s.name, sizeof(out->s.name), "*%d", i);

		for (j = 0; j < 3; j++)
		{
			/* spread the mins / maxs by a pixel */
			out->s.mins[j] = in->mins[j] - 1;
			out->s.maxs[j] = in->maxs[j] + 1;
			out->s.origin[j] = in->origin[j];
		}

		out->s.radius = Mod_RadiusFromBounds(out->s.mins, out->s.maxs);
		out->s.firstnode = in->headnode;
		out->s.firstmodelsurface = in->firstface;
		out->s.nummodelsurfaces = in->numfaces;
		/* visleafs */
		out->s.numleafs = 0;
		/* check limits */
		if (out->s.firstnode >= loadmodel->s.numnodes)
		{
			Com_Error(ERR_DROP, "%s: Inline model %i has bad firstnode",
					__func__, i);
			return;
		}
	}
}

static void
Mod_LoadQFaces(model_t *loadmodel, const byte *mod_base, const lump_t *l,
	const bspx_header_t *bspx_header)
{
	int i, count, surfnum, lminfosize;
	const dlminfo_t *lminfos;
	msurface_t *out;
	dqface_t *in;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->s.name);
		return;
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_FACES) * sizeof(*out));

	loadmodel->s.surfaces = out;
	loadmodel->s.numsurfaces = count;

	lminfos = Mod_LoadBSPXFindLump(bspx_header, "DECOUPLED_LM", &lminfosize, mod_base);
	if ((lminfos != NULL) &&
		(lminfosize / sizeof(dlminfo_t) != loadmodel->s.numsurfaces))
	{
		Com_Printf("%s: [%s] decoupled_lm size " YQ2_COM_PRIdS " does not match surface count %d\n",
			__func__, loadmodel->s.name, lminfosize / sizeof(dlminfo_t), loadmodel->s.numsurfaces);
		lminfos = NULL;
	}

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		unsigned int planenum;
		int	ti, side, lightofs;

		out->firstedge = in->firstedge;
		out->numedges = in->numedges;

		if (out->numedges < 3)
		{
			Com_Error(ERR_DROP, "%s: Surface with %d edges",
					__func__, out->numedges);
			return;
		}

		out->flags = 0;
		out->polys = NULL;

		planenum = in->planenum;
		side = in->side;

		if (side)
		{
			out->flags |= SURF_PLANEBACK;
		}

		if (planenum >= loadmodel->s.numplanes)
		{
			Com_Error(ERR_DROP, "%s: Incorrect %d planenum.",
					__func__, planenum);
			return;
		}
		out->plane = loadmodel->s.planes + planenum;

		ti = in->texinfo;

		if ((ti < 0) || (ti >= loadmodel->s.numtexinfo))
		{
			Com_Error(ERR_DROP, "%s: bad texinfo number",
					__func__);
			return;
		}

		out->texinfo = loadmodel->s.texinfo + ti;

		lightofs = Mod_LoadBSPXDecoupledLM(lminfos, surfnum, out);
		if (lightofs < 0) {
			memcpy(out->lmvecs, out->texinfo->vecs, sizeof(out->lmvecs));
			out->lmshift = DEFAULT_LMSHIFT;
			out->lmvlen[0] = 1.0f;
			out->lmvlen[1] = 1.0f;

			Mod_CalcSurfaceExtents(loadmodel->s.surfedges, loadmodel->s.numsurfedges,
				loadmodel->s.vertexes, loadmodel->s.edges, out);

			lightofs = in->lightofs;
		}

		Mod_LoadSetSurfaceLighting(loadmodel->s.lightdata, loadmodel->s.numlightdata,
			out, in->styles, lightofs);

		/* set the drawing flags */
		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;

			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}

			R_SubdivideSurface(loadmodel->s.surfedges, loadmodel->s.vertexes,
				loadmodel->s.edges, out); /* cut up polygon for warps */
		}

		if (r_fixsurfsky->value)
		{
			if (out->texinfo->flags & SURF_SKY)
			{
				out->flags |= SURF_DRAWSKY;
			}
		}

		LM_CreateLightmapsPoligon(loadmodel, out);
	}
}

static void
Mod_LoadBrushModel(model_t *mod, const void *buffer, int modfilelen)
{
	int lightgridsize = 0, hunkSize;
	const bspx_header_t *bspx_header;
	dheader_t *header;
	byte *mod_base;

	if (mod != mod_known)
	{
		Com_Error(ERR_DROP, "%s: Loaded a brush model after the world", __func__);
		return;
	}

	mod_base = (byte *)buffer;
	header = (dheader_t *)mod_base;

	/* check for BSPX extensions */
	bspx_header = Mod_LoadBSPX(modfilelen, (byte*)mod_base);

	// calculate the needed hunksize from the lumps
	hunkSize = Mod_CalcNonModelLumpHunkSize(mod_base, header);

	hunkSize += Mod_CalcLumpHunkSize(&header->lumps[LUMP_MODELS],
		sizeof(dmodel_t), sizeof(model_t), 0);

	/* Get size of octree on disk, need to recheck real size */
	if (Mod_LoadBSPXFindLump(bspx_header, "LIGHTGRID_OCTREE", &lightgridsize, mod_base))
	{
		hunkSize += lightgridsize * 4;
	}

	mod->s.extradata = Hunk_Begin(hunkSize);
	mod->s.type = mod_brush;

	Mod_LoadBSPXSections(bspx_header, mod_base, &mod->s);

	/* load into heap */
	Mod_LoadVertexes(mod->s.name, &mod->s.vertexes, &mod->s.numvertexes, mod_base,
		&header->lumps[LUMP_VERTEXES]);
	Mod_LoadQBSPEdges(mod->s.name, &mod->s.edges, &mod->s.numedges,
		mod_base, &header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(mod->s.name, &mod->s.surfedges, &mod->s.numsurfedges,
		mod_base, &header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting(&mod->s.lightdata, &mod->s.numlightdata, mod_base,
		&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(mod->s.name, &mod->s.planes, &mod->s.numplanes,
		mod_base, &header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(mod->s.name, &mod->s.texinfo, &mod->s.numtexinfo,
		mod_base, &header->lumps[LUMP_TEXINFO], (findimage_t)R_FindImage,
		r_notexture);

	LM_BeginBuildingLightmaps(mod);
	Mod_LoadQFaces(mod, mod_base, &header->lumps[LUMP_FACES], bspx_header);
	LM_EndBuildingLightmaps();

	Mod_LoadQBSPMarksurfaces(mod->s.name, &mod->s.marksurfaces, &mod->s.nummarksurfaces,
		mod->s.surfaces, mod->s.numsurfaces, mod_base, &header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility(mod->s.name, &mod->s.vis, &mod->s.numvisibility, mod_base,
		&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadQBSPLeafs(mod->s.name, &mod->s.leafs, &mod->s.numleafs,
		mod->s.marksurfaces, mod->s.nummarksurfaces, &mod->s.numclusters,
		mod_base, &header->lumps[LUMP_LEAFS]);
	Mod_LoadQBSPNodes(mod->s.name, mod->s.planes, mod->s.numplanes, mod->s.leafs,
		mod->s.numleafs, &mod->s.nodes, &mod->s.numnodes, mod->s.mins, mod->s.maxs,
		mod_base, &header->lumps[LUMP_NODES], header->ident);
	Mod_LoadSubmodels(mod, mod_base, &header->lumps[LUMP_MODELS]);
	mod->s.numframes = 2; /* regular and alternate animation */

	if (mod->s.vis && mod->s.numclusters != mod->s.vis->numclusters)
	{
		Com_Error(ERR_DROP, "%s: Map %s has incorrect number of clusters %d != %d",
			__func__, mod->s.name, mod->s.numclusters, mod->s.vis->numclusters);
		return;
	}

	if ((mod->s.numleafs > mod_novis_len) || !mod_novis)
	{
		byte *tmp;

		/* reallocate buffers for PVS/PHS buffers*/
		mod_novis_len = (mod->s.numleafs + 63) & ~63;
		tmp = realloc(mod_novis, mod_novis_len / 8);
		YQ2_COM_CHECK_OOM(tmp, "realloc()", mod_novis_len / 8)
		if (!tmp)
		{
			mod_novis_len = 0;
			/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
			return;
		}

		mod_novis = tmp;
		Com_Printf("Allocated " YQ2_COM_PRIdS " bit leafs of PVS/PHS buffer\n",
			mod_novis_len);
	}
}

/*
 * Loads in a model for the given name
 */
static model_t *
Mod_ForName(const char *name, model_t *parent_model, qboolean crash)
{
	char filename[256] = {0}, *tag;
	int i, modfilelen;
	model_t *mod;
	void *buf;

	if (!name[0])
	{
		Com_Error(ERR_DROP, "%s: NULL name", __func__);
		return NULL;
	}

	/* inline models are grabbed only from worldmodel */
	if (name[0] == '*' && parent_model)
	{
		i = (int)strtol(name + 1, (char **)NULL, 10);

		if (i < 1 || i >= parent_model->numsubmodels)
		{
			Com_Error(ERR_DROP, "%s: bad inline model number",
					__func__);
			return NULL;
		}

		return &parent_model->submodels[i];
	}

	/* search the currently loaded models */
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->s.name[0])
		{
			continue;
		}

		if (!strcmp(mod->s.name, name))
		{
			return mod;
		}
	}

	/* find a free model slot spot */
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->s.name[0])
		{
			break; /* free spot */
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
			Com_Error(ERR_DROP, "%s: mod_numknown == MAX_MOD_KNOWN", __func__);
			return NULL;
		}

		mod_numknown++;
	}

	Q_strlcpy(mod->s.name, name, sizeof(mod->s.name));

	/* Anachronox has tags in model path*/
	Q_strlcpy(filename, name, sizeof(filename));
	tag = strstr(filename, ".mda!");
	if (tag)
	{
		tag += 4; /* strlen(.mda) */
		*tag = 0;
		tag ++;
	}

	/* load the file */
	modfilelen = ri.Mod_LoadFile(filename, &buf);

	if (!buf)
	{
		if (crash)
		{
			Com_Error(ERR_DROP, "%s: %s not found",
					__func__, mod->s.name);
			return NULL;
		}

		if (r_validation->value > 0)
		{
			Com_Printf("%s: Can't load %s\n", __func__, mod->s.name);
		}

		memset(mod->s.name, 0, sizeof(mod->s.name));
		return NULL;
	}

	/* call the apropriate loader */
	switch (LittleLong(*(unsigned *)buf))
	{
		case IDALIASHEADER:
			/* fall through */
		case IDSPRITEHEADER:
			{
				mod->s.extradata = Mod_LoadModel(mod->s.name, buf, modfilelen,
					mod->s.mins, mod->s.maxs,
					(struct image_s ***)&mod->skins, &mod->numskins,
					(findimage_t)R_FindImage, &(mod->s.type));
				if (!mod->s.extradata)
				{
					Com_Error(ERR_DROP, "%s: Failed to load %s",
						__func__, mod->s.name);
					return NULL;
				}
			};
			break;

		/* should have only preloaded formats for maps */
		case QBSPHEADER:
			Mod_LoadBrushModel(mod, buf, modfilelen);
			break;

		default:
			Com_Error(ERR_DROP, "%s: unknown fileid for %s",
					__func__, mod->s.name);
			return NULL;
	}

	mod->s.radius = Mod_RadiusFromBounds(mod->s.mins, mod->s.maxs);
	if (mod->s.extradata)
	{
		mod->s.extradatasize = Hunk_End();
	}
	else
	{
		mod->s.extradatasize = 0;
	}

	ri.FS_FreeFile(buf);

	return mod;
}

static void
Mod_Free(model_t *mod)
{
	if (!mod->s.extradata)
	{
		/* looks as empty model */
		memset (mod, 0, sizeof(*mod));
		return;
	}

	if (r_validation->value > 0)
	{
		Com_Printf("%s: Unload %s\n", __func__, mod->s.name);
	}

	Hunk_Free(mod->s.extradata);

	if (mod->s.type == mod_alias || mod->s.type == mod_sprite)
	{
		/* skins are allocated separately */
		free(mod->skins);
	}

	memset(mod, 0, sizeof(*mod));
}

void
Mod_FreeAll(void)
{
	int i;

	for (i = 0; i < mod_numknown; i++)
	{
		if (mod_known[i].s.extradatasize)
		{
			Mod_Free(&mod_known[i]);
		}
	}

	/* Free PVS buffer */
	if (mod_novis)
	{
		free(mod_novis);
		mod_novis = NULL;
	}
	mod_novis_len = 0;
}

/*
 * Specifies the model that will be used as the world
 */
void
RI_BeginRegistration(const char *model)
{
	char fullname[MAX_QPATH];
	const cvar_t *flushmap;

	registration_sequence++;
	r_oldviewcluster = -1; /* force markleafs */

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	/* explicitly free the old map if different
	   this guarantees that mod_known[0] is the
	   world map */
	flushmap = ri.Cvar_Get("flushmap", "0", 0);

	if (strcmp(mod_known[0].s.name, fullname) || flushmap->value)
	{
		Mod_Free(&mod_known[0]);
	}

	r_worldmodel = Mod_ForName(fullname, NULL, true);

	r_viewcluster = -1;
}

struct model_s *
RI_RegisterModel(const char *name)
{
	model_t *mod;

	mod = Mod_ForName(name, r_worldmodel, false);

	if (mod)
	{
		mod->s.registration_sequence = registration_sequence;

		/* register any images used by the models */
		if (mod->s.type == mod_brush)
		{
			int i;

			for (i = 0; i < mod->s.numtexinfo; i++)
			{
				mod->s.texinfo[i].image->registration_sequence =
					registration_sequence;
			}
		}
		else
		{
			/* numframes is unused for SP2 but lets set it also  */
			mod->s.numframes = Mod_ReLoadSkins(name, (struct image_s **)mod->skins,
				(findimage_t)R_FindImage, mod->s.extradata, mod->s.type);
		}
	}

	return mod;
}

void
RI_EndRegistration(void)
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
		if (!mod->s.name[0])
		{
			continue;
		}

		if (mod->s.registration_sequence != registration_sequence)
		{
			/* don't need this model */
			ri.Mod_FreeFile(mod->s.name);
			Mod_Free(mod);
		}
	}

	R_FreeUnusedImages();
}
