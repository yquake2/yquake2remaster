/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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

static YQ2_ALIGNAS_TYPE(int) byte mod_novis[MAX_MAP_LEAFS / 8];

static model_t *models_known;
static int mod_numknown = 0;
static int mod_max = 0;
static int mod_loaded = 0;
static int models_known_max = 0;
int registration_sequence;

const byte *
Mod_ClusterPVS(int cluster, const model_t *model)
{
	if ((cluster == -1) || !model->vis)
	{
		return mod_novis;
	}

	return Mod_DecompressVis((byte *)model->vis +
			model->vis->bitofs[cluster][DVIS_PVS],
			(byte *)model->vis + model->numvisibility,
			(model->vis->numclusters + 7) >> 3);
}


//===============================================================================

static void
Mod_Reallocate (void)
{
	if ((models_known_max >= (mod_max * 4)) && (models_known != NULL))
	{
		return;
	}

	if (models_known)
	{
		// Always up if already allocated
		models_known_max *= 2;
		// free up
		Mod_FreeAll();
		Mod_FreeModelsKnown();
	}

	if (models_known_max < (mod_max * 4))
	{
		// double is not enough?
		models_known_max = ROUNDUP(mod_max * 4, 16);
	}

	R_Printf(PRINT_ALL, "Reallocate space for %d models.\n", models_known_max);

	models_known = calloc(models_known_max, sizeof(model_t));
}

void
Mod_Init(void)
{
	mod_max = 0;
	memset(mod_novis, 0xff, sizeof(mod_novis));
	mod_numknown = 0;
	mod_loaded = 0;

	models_known_max = MAX_MOD_KNOWN;
	models_known = NULL;

	Mod_Reallocate ();
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
			out->mins[j] = in->mins[j] - 1;
			out->maxs[j] = in->maxs[j] + 1;
			out->origin[j] = in->origin[j];
		}

		out->radius = Mod_RadiusFromBounds(out->mins, out->maxs);
		out->firstnode = in->headnode;
		out->firstmodelsurface = in->firstface;
		out->nummodelsurfaces = in->numfaces;
		// visleafs
		out->numleafs = 0;
		//  check limits
		if (out->firstnode >= loadmodel->numnodes)
		{
			Com_Error(ERR_DROP, "%s: Inline model %i has bad firstnode",
					__func__, i);
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
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + EXTRA_LUMP_FACES) * sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	lminfos = Mod_LoadBSPXFindLump(bspx_header, "DECOUPLED_LM", &lminfosize, mod_base);
	if ((lminfos != NULL) &&
		(lminfosize / sizeof(dlminfo_t) != loadmodel->numsurfaces))
	{
		R_Printf(PRINT_ALL, "%s: [%s] decoupled_lm size " YQ2_COM_PRIdS " does not match surface count %d\n",
			__func__, loadmodel->name, lminfosize / sizeof(dlminfo_t), loadmodel->numsurfaces);
		lminfos = NULL;
	}

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		int side, ti, planenum, lightofs;

		out->firstedge = in->firstedge;
		out->numedges = in->numedges;

		if (out->numedges < 3)
		{
			Com_Error(ERR_DROP, "%s: Surface with %d edges",
					__func__, out->numedges);
		}
		out->flags = 0;
		out->polys = NULL;

		planenum = in->planenum;
		side = in->side;

		if (side)
		{
			out->flags |= SURF_PLANEBACK;
		}

		if (planenum < 0 || planenum >= loadmodel->numplanes)
		{
			Com_Error(ERR_DROP, "%s: Incorrect %d planenum.",
					__func__, planenum);
		}
		out->plane = loadmodel->planes + planenum;

		ti = LittleLong(in->texinfo);

		if ((ti < 0) || (ti >= loadmodel->numtexinfo))
		{
			Com_Error(ERR_DROP, "%s: bad texinfo number",
					__func__);
		}

		out->texinfo = loadmodel->texinfo + ti;

		lightofs = Mod_LoadBSPXDecoupledLM(lminfos, surfnum, out);
		if (lightofs < 0) {
			memcpy(out->lmvecs, out->texinfo->vecs, sizeof(out->lmvecs));
			out->lmshift = DEFAULT_LMSHIFT;
			out->lmvlen[0] = 1.0f;
			out->lmvlen[1] = 1.0f;

			Mod_CalcSurfaceExtents(loadmodel->surfedges, loadmodel->vertexes,
				loadmodel->edges, out);

			lightofs = in->lightofs;
		}

		Mod_LoadSetSurfaceLighting(loadmodel->lightdata, loadmodel->numlightdata,
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

			R_SubdivideSurface(loadmodel->surfedges, loadmodel->vertexes,
				loadmodel->edges, out); /* cut up polygon for warps */
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

	if (mod != models_known)
	{
		Com_Error(ERR_DROP, "%s: Loaded a brush model after the world", __func__);
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

	mod->extradata = Hunk_Begin(hunkSize);
	mod->type = mod_brush;

	if (bspx_header)
	{
		mod->grid = Mod_LoadBSPXLightGrid(bspx_header, mod_base);
	}
	else
	{
		mod->grid = NULL;
	}

	/* load into heap */
	Mod_LoadVertexes(mod->name, &mod->vertexes, &mod->numvertexes, mod_base,
		&header->lumps[LUMP_VERTEXES]);
	Mod_LoadQBSPEdges(mod->name, &mod->edges, &mod->numedges,
		mod_base, &header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(mod->name, &mod->surfedges, &mod->numsurfedges,
		mod_base, &header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting(&mod->lightdata, &mod->numlightdata, mod_base,
		&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(mod->name, &mod->planes, &mod->numplanes,
		mod_base, &header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(mod->name, &mod->texinfo, &mod->numtexinfo,
		mod_base, &header->lumps[LUMP_TEXINFO], (findimage_t)Vk_FindImage,
		r_notexture);

	LM_BeginBuildingLightmaps(mod);
	Mod_LoadQFaces(mod, mod_base, &header->lumps[LUMP_FACES], bspx_header);
	LM_EndBuildingLightmaps();

	Mod_LoadQBSPMarksurfaces(mod->name, &mod->marksurfaces, &mod->nummarksurfaces,
		mod->surfaces, mod->numsurfaces, mod_base, &header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility(mod->name, &mod->vis, &mod->numvisibility, mod_base,
		&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadQBSPLeafs(mod->name, &mod->leafs, &mod->numleafs,
		mod->marksurfaces, mod->nummarksurfaces, mod_base,
		&header->lumps[LUMP_LEAFS]);
	Mod_LoadQBSPNodes(mod->name, mod->planes, mod->numplanes, mod->leafs,
		mod->numleafs, &mod->nodes, &mod->numnodes, mod_base,
		&header->lumps[LUMP_NODES], header->ident);
	Mod_LoadSubmodels(mod, mod_base, &header->lumps[LUMP_MODELS]);
	mod->numframes = 2; /* regular and alternate animation */
}

/* Temporary solution, need to use load file dirrectly */
static int
Mod_ReadFile(const char *path, void **buffer)
{
	char *data;
	int size;

	size = ri.FS_LoadFile(path, (void **)&data);
	if (size <= 0)
	{
		return size;
	}

	*buffer = malloc(size);
	memcpy(*buffer, data, size);
	ri.FS_FreeFile((void *)data);

	return size;
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
		Com_Error(ERR_DROP, "%s: NULL name", __func__);
	}

	/* inline models are grabbed only from worldmodel */
	if (name[0] == '*' && parent_model)
	{
		i = (int)strtol(name + 1, (char **)NULL, 10);

		if (i < 1 || i >= parent_model->numsubmodels)
		{
			Com_Error(ERR_DROP, "%s: bad inline model number",
					__func__);
		}

		return &parent_model->submodels[i];
	}

	/* search the currently loaded models */
	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
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
	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			break; /* free spot */
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == models_known_max)
		{
			Com_Error(ERR_DROP, "%s: mod_numknown == models_known_max", __func__);
		}

		mod_numknown++;
	}

	strcpy(mod->name, name);

	/* load the file */
	modfilelen = ri.Mod_LoadFile(mod->name, &buf);

	if (!buf)
	{
		if (crash)
		{
			Com_Error(ERR_DROP, "%s: %s not found",
					__func__, mod->name);
		}

		if (r_validation->value > 0)
		{
			R_Printf(PRINT_ALL, "%s: Can't load %s\n", __func__, mod->name);
		}

		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	/* update count of loaded models */
	mod_loaded ++;
	if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "%s: Load %s[%d]\n", __func__, mod->name, mod_loaded);
	}

	/* fill it in */

	/* call the apropriate loader */
	switch (LittleLong(*(unsigned *)buf))
	{
		case MDAHEADER:
			/* fall through */
		case SDEFHEADER:
			/* fall through */
		case MDXHEADER:
			/* fall through */
		case DKMHEADER:
			/* fall through */
		case RAVENFMHEADER:
			/* fall through */
		case IDALIASHEADER:
			/* fall through */
		case IDMDLHEADER:
			/* fall through */
		case ID3HEADER:
			/* fall through */
		case MDR_IDENT:
			/* fall through */
		case IDMD5HEADER:
			/* fall through */
		case IDSPRITEHEADER:
			{
				mod->extradata = Mod_LoadModel(mod->name, buf, modfilelen,
					mod->mins, mod->maxs,
					(struct image_s ***)&mod->skins, &mod->numskins,
					(findimage_t)Vk_FindImage, (loadimage_t)Vk_LoadPic, Mod_ReadFile,
					&(mod->type));
				if (!mod->extradata)
				{
					Com_Error(ERR_DROP, "%s: Failed to load %s",
						__func__, mod->name);
				}
			};
			break;

		/* should have only preloaded formats for maps */
		case QBSPHEADER:
			Mod_LoadBrushModel(mod, buf, modfilelen);
			break;

		default:
			Com_Error(ERR_DROP, "%s: unknown fileid for %s",
					__func__, mod->name);
			break;
	}

	mod->radius = Mod_RadiusFromBounds(mod->mins, mod->maxs);
	if (mod->extradata)
	{
		mod->extradatasize = Hunk_End();
	}
	else
	{
		mod->extradatasize = 0;
	}

	ri.FS_FreeFile(buf);

	return mod;
}

static void
Mod_Free(model_t *mod)
{
	if (!mod->extradata)
	{
		/* looks as empty model */
		memset (mod, 0, sizeof(*mod));
		return;
	}

	if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "%s: Unload %s[%d]\n", __func__, mod->name, mod_loaded);
	}

	Hunk_Free(mod->extradata);
	memset(mod, 0, sizeof(*mod));
	mod_loaded --;
	if (mod_loaded < 0)
	{
		ri.Sys_Error (ERR_DROP, "%s: Broken unload", __func__);
	}
}

void
Mod_FreeAll(void)
{
	int i;

	for (i = 0; i < mod_numknown; i++)
	{
		if (models_known[i].extradatasize)
		{
			Mod_Free(&models_known[i]);
		}
	}
}

void
Mod_FreeModelsKnown(void)
{
	free(models_known);
	models_known = NULL;
}

/*
 * Specifies the model that will be used as the world
 */
void
RE_BeginRegistration(const char *model)
{
	char fullname[MAX_QPATH];
	const cvar_t *flushmap;

	Mod_Reallocate();

	registration_sequence++;
	r_oldviewcluster = -1; /* force markleafs */

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	/* explicitly free the old map if different
	   this guarantees that mod_known[0] is the
	   world map */
	flushmap = ri.Cvar_Get("flushmap", "0", 0);

	if (strcmp(models_known[0].name, fullname) || flushmap->value)
	{
		Mod_Free(&models_known[0]);
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
				(findimage_t)Vk_FindImage, (loadimage_t)Vk_LoadPic,
				mod->extradata, mod->type);
		}
	}

	return mod;
}

static qboolean
Mod_HasFreeSpace(void)
{
	int i, used;
	model_t *mod;

	used = 0;

	for (i=0, mod=models_known ; i<mod_numknown ; i++, mod++)
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
	return (mod_loaded + mod_max) < models_known_max;
}

void
Mod_Modellist_f(void)
{
	int		i, total, used;
	model_t	*mod;
	qboolean	freeup;

	total = 0;
	used = 0;

	R_Printf(PRINT_ALL,"Loaded models:\n");
	for (i=0, mod=models_known ; i < mod_numknown ; i++, mod++)
	{
		char *in_use = "";

		if (mod->registration_sequence == registration_sequence)
		{
			in_use = "*";
			used ++;
		}

		if (!mod->name[0])
			continue;
		R_Printf(PRINT_ALL, "%8i : %s %s r: %.2f #%d\n",
			 mod->extradatasize, mod->name, in_use, mod->radius, mod->numsubmodels);
		total += mod->extradatasize;
	}
	R_Printf(PRINT_ALL, "Total resident: %i in %d models\n", total, mod_loaded);
	// update statistics
	freeup = Mod_HasFreeSpace();
	R_Printf(PRINT_ALL, "Used %d of %d models%s.\n", used, mod_max, freeup ? ", has free space" : "");
}

void
RE_EndRegistration(void)
{
	int i;
	model_t *mod;

	if (Mod_HasFreeSpace() && Vk_ImageHasFreeSpace())
	{
		// should be enough space for load next maps
		return;
	}

	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
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

	Vk_FreeUnusedImages();
}
