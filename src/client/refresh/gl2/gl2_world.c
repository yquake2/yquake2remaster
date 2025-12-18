/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_world.c -- NEW WORLD RENDERING CODE

#include <assert.h>
#include "r_local.h"

static vec3_t		modelorg; // relative to viewpoint
static msurface_t	*r_alpha_surfaces = NULL; //transparent surfs
static byte			fatvis[MAX_MAP_LEAFS_QBSP / 8]; // markleaves

extern void R_LightMap_TexCoordsForSurf(msurface_t* surf, polyvert_t* vert, vec3_t pos);
extern void R_BeginLinesRendering(qboolean dt);
extern void R_EndLinesRendering();

extern int registration_sequence; // experimental nature heh

#define MAX_INDICES 16384 // build indices untill this

// gfx world stores the current state for batching drawcalls, if any 
// of them change we draw what we have so far and begin building indices again
typedef struct gfx_world_s
{
	int			tex_diffuse;
	int			tex_lm;

	vec3_t		styles[MAX_LIGHTMAPS_PER_SURFACE];
	vec3_t		new_styles[MAX_LIGHTMAPS_PER_SURFACE];

	vec4_t		color;
	 
	unsigned int uniformflags; //Flags that require a uniform change. 
							   //Currently SURF_DRAWTURB, SURF_FLOWING, SURF_SCROLLX, SURF_SCROLLY, SURF_SCROLLFLIP, and probably sky

	GLuint		vbo;
	int			totalVertCount;

	int			numIndices;
	GLuint		indicesBuf[MAX_INDICES];

	qboolean	isRenderingWorld;
	int			registration_sequence; // hackery
} gfx_world_t;

static gfx_world_t gfx_world = { 0 };



/*
===============
R_BuildPolygonFromSurface

Does also calculate proper lightmap coordinates for poly
===============
*/
void R_BuildPolygonFromSurface(model_t* mod, msurface_t* surf)
{
	int i, lnumverts;
	medge_t* pedges, * r_pedge;
	float* vec;
	poly_t* poly;
	vec3_t total;
	vec3_t normal;

	// reconstruct the polygon
	pedges = mod->edges;
	lnumverts = surf->numedges;

	VectorClear(total);

	/* draw texture */
	poly = Hunk_Alloc(sizeof(poly_t) + (lnumverts - 4) * sizeof(polyvert_t));
	poly->next = surf->polys;
	poly->flags = surf->flags;
	surf->polys = poly;
	poly->numverts = lnumverts;

	VectorCopy(surf->plane->normal, normal);

	if (surf->flags & SURF_PLANEBACK)
	{
		// if for some reason the normal sticks to the back of 
		// the plane, invert it so it'surf usable for the shader
		for (i = 0; i < 3; ++i)
			normal[i] = -normal[i];
	}

	float alpha = 1.0f;
	if (surf->texinfo->flags & SURF_TRANS66)
		alpha = 0.66f;
	else if (surf->texinfo->flags & SURF_TRANS33)
		alpha = 0.33;

	for (i = 0; i < lnumverts; i++)
	{
		polyvert_t* vert;
		float s, t;
		int lindex;

		vert = &poly->verts[i];

		lindex = mod->surfedges[surf->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = mod->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = mod->vertexes[r_pedge->v[1]].position;
		}

		//
		// diffuse texture coordinates
		//
		s = DotProduct(vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
		s /= surf->texinfo->image->width;

		t = DotProduct(vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];
		t /= surf->texinfo->image->height;

		VectorAdd(total, vec, total);
		VectorCopy(vec, vert->pos);
		Vector2Set(vert->texCoord, s, t);

		VectorCopy(normal, vert->normal);
		vert->alpha = alpha;

		R_LightMap_TexCoordsForSurf(surf, vert, vec);
	}
}

// ==============================================================================
// VERTEX BUFFER MANAGMENT
// ==============================================================================

/*
=================
R_DestroyWorldVertexBuffer

Delete world's vbo
=================
*/
static void R_DestroyWorldVertexBuffer()
{
	if (!gfx_world.vbo)
		return;

	glDeleteBuffers(1, &gfx_world.vbo);

	ri.Printf(PRINT_LOW, "Freed world surface cache.\n");
	memset(&gfx_world, 0, sizeof(gfx_world));
}

/*
=================
R_BuildVertexBufferForWorld

upload all world surfaces as one vertex buffer
=================
*/
static void R_BuildVertexBufferForWorld()
{
	msurface_t	*surf;
	poly_t		*p, *bp;
	int			i, j, v, bufsize;
	GLint		ongpusize = 0;

	polyvert_t* tempVerts;

	if (!r_worldmodel)
		return;

	// walk all surfaces, set their firstvert and count the vertices to build a temp buffer
	gfx_world.totalVertCount = 0;
	for (surf = r_worldmodel->surfaces, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		surf->firstvert = gfx_world.totalVertCount;
		surf->numverts = 0;

		// warps have polygon chains
		for (bp = surf->polys; bp; bp = bp->next)
		{
			p = bp;
			surf->numverts += p->numverts;
		}
		gfx_world.totalVertCount += surf->numverts;

//		ri.Printf(PRINT_ALL, "%i verts in %i surf\n", surf->numverts, i);
	}

	bufsize = sizeof(polyvert_t) * gfx_world.totalVertCount; // this must match the vbo size on gpu, otherwise we probably failed to upload due to vram limit

	// create temp buffer
	tempVerts = malloc(bufsize);
	if (!tempVerts)
		ri.Error(ERR_FATAL, "failed to allocate %i kb for gfx_world", bufsize / 1024);

	for (surf = r_worldmodel->surfaces, v = 0, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		// copy verts to temp buffer
		for (bp = surf->polys; bp; bp = bp->next)
		{
			p = bp;
			for (j = 0; j < p->numverts; j++)
			{
				memcpy(&tempVerts[v], &p->verts[j], sizeof(polyvert_t));
				v++;
			}
		}

	}

	// upload vertices to gpu
	glGenBuffers(1, &gfx_world.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &tempVerts[0].pos[0], GL_STATIC_DRAW);

	free(tempVerts);

	// check if its all right
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &ongpusize);
	if (ongpusize != bufsize)
	{
		R_DestroyWorldVertexBuffer();
		ri.Error(ERR_FATAL, "failed to allocate vertex buffer for gfx_world (probably out of VRAM)\n");
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ri.Printf(PRINT_LOW, "World surface cache is %i kb (%i verts in %i surfaces)\n", bufsize / 1024, gfx_world.totalVertCount, r_worldmodel->numsurfaces);
}

// ==============================================================================
// RENDERING
// ==============================================================================

/*
===============
R_World_BeginRendering

Binds the world's VBO, shader and assign attrib locations for rendering the bsp
No UBOs in GL2.1 so we have to bind attrib locations each time we start rendering world, binding diferent VBO invalidates locations
===============
*/
static void R_World_BeginRendering()
{
	int attrib;

	// bind shader and buffer and assign attrib locations

	R_BindProgram(GLPROG_WORLD);
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);

	attrib = R_GetProgAttribLoc(VALOC_POS);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), NULL);
	}

	attrib = R_GetProgAttribLoc(VALOC_TEXCOORD);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, texCoord));
	}

	attrib = R_GetProgAttribLoc(VALOC_LMTEXCOORD);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, lmTexCoord));
	}

	attrib = R_GetProgAttribLoc(VALOC_NORMAL);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, normal));
	}

	attrib = R_GetProgAttribLoc(VALOC_ALPHA);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 1, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, alpha));
	}

	R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);
	R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
	R_ProgUniform2f(LOC_FLOWSTRENGTH, 0.f, 0.f);

	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	gfx_world.uniformflags = 0; //Always force a uniform update if needed
	gfx_world.isRenderingWorld = true;
}

/*
===============
R_World_EndRendering
===============
*/
static void R_World_EndRendering()
{
	if (!gfx_world.isRenderingWorld)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	R_UnbindProgram();

	gfx_world.isRenderingWorld = false;
}

/*
=======================
R_World_DrawAndFlushBufferedGeo

This should be also called at the end of rendering to make sure we drawn everything
=======================
*/
static void R_World_DrawAndFlushBufferedGeo()
{
	if (!gfx_world.numIndices || !gfx_world.isRenderingWorld)
		return;

	rperf.brush_drawcalls++;

	R_MultiTextureBind(TMU_DIFFUSE, gfx_world.tex_diffuse);
	R_MultiTextureBind(TMU_LIGHTMAP, gfx_world.tex_lm);

	R_ProgUniform3fv(LOC_LIGHTSTYLES, MAX_LIGHTMAPS_PER_SURFACE, gfx_world.styles[0]);

//	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawElements(GL_TRIANGLES, gfx_world.numIndices, GL_UNSIGNED_INT, &gfx_world.indicesBuf[0]);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	gfx_world.numIndices = 0;
}


// ==============================================================================
// BATCHING
// ==============================================================================

/*
===============
R_World_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t* R_World_TextureAnimation(mtexinfo_t* tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = pCurrentRefEnt->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
=======================
R_World_GrabSurfaceTextures

Grabs the diffuse and lightmap textures for given surface
=======================
*/
static void R_World_GrabSurfaceTextures(const msurface_t* surf, int *outDiffuse, int *outLM)
{
	image_t* image;
	qboolean lightmapped = true;

	image = R_World_TextureAnimation(surf->texinfo);
	if (!image)
		image = r_texture_missing;

	if (surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
		lightmapped = false;

	*outDiffuse = r_lightmap->value ? r_texture_white->texnum : image->texnum;
	*outLM = (r_fullbright->value || !lightmapped) ? r_texture_white->texnum : (gl_state.lightmap_textures + surf->lightMapTextureNum);

	if (*outDiffuse == r_texture_white->texnum && *outLM == r_texture_white->texnum)
		*outDiffuse = image->texnum; // never let the world render completly white
}

/*
=======================
R_World_UpdateLightStylesForSurf

Update lightstyles for surface, pass NULL to disable lightstyles, this is slightly
diferent from R_LightMap_UpdateLightStylesForSurf and returns true when style has changed
=======================
*/
static qboolean R_World_UpdateLightStylesForSurf(msurface_t* surf)
{
	qboolean hasChanged = false;
	int i;

	if (surf == NULL)
	{
		VectorSet(gfx_world.new_styles[0], 1.0f, 1.0f, 1.0f);
		for (i = 1; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorClear(gfx_world.new_styles[i]);
		goto change;
	}

	if (!r_dynamic->value) // r_dynamic 0 disables lightstyles and dlights
	{
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorSet(gfx_world.new_styles[i], 1.0f, 1.0f, 1.0f);
	}
	else
	{
		// add light styles
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[i] != (MAX_LIGHTSTYLES - 1); i++)
		{
			gfx_world.new_styles[i][0] = r_newrefdef.lightstyles[surf->styles[i]].rgb[0];
			gfx_world.new_styles[i][1] = r_newrefdef.lightstyles[surf->styles[i]].rgb[1];
			gfx_world.new_styles[i][2] = r_newrefdef.lightstyles[surf->styles[i]].rgb[2];
		}
	}

change:
	for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
	{
		if (!VectorCompare(gfx_world.new_styles[i], gfx_world.styles[i]))
		{
			hasChanged = true;
		}
	}

	return hasChanged;
}

/*
=======================
R_World_NewDrawSurface
=======================
*/
static void R_World_NewDrawSurface(msurface_t* surf, qboolean lightmapped)
{
	int			tex_diffuse, tex_lightmap;
	int			numTris, numIndices, i;
	unsigned int* dst_indices;
	qboolean	stylechanged;

	R_World_GrabSurfaceTextures(surf, &tex_diffuse, &tex_lightmap);

	// fullbright surfaces (water, translucent) have no styles
	stylechanged = R_World_UpdateLightStylesForSurf((r_fullbright->value > 0.0f || !lightmapped) ? NULL : surf);

	numTris = surf->numedges - 2;
	numIndices = numTris * 3;

	//These flags are ones that are relevant to uniforms.
	unsigned int uniformflags = surf->texinfo->flags & (SURF_WARP | SURF_FLOWING | SURF_SCROLLX | SURF_SCROLLY | SURF_SCROLLFLIP);

	//
	// draw everything we have buffered so far and begin building again 
	// when any of the textures change or surface lightstyles change
	//
	if (gfx_world.tex_diffuse != tex_diffuse || gfx_world.tex_lm != tex_lightmap || gfx_world.uniformflags != uniformflags || gfx_world.numIndices + numIndices >= MAX_INDICES  || stylechanged )
	{
		R_World_DrawAndFlushBufferedGeo();
		//Only when starting a new batch should uniforms be updated, since they will persist across all of the batch's draws
		
		if (surf->texinfo->flags & SURF_WARP)
			R_ProgUniform1f(LOC_WARPSTRENGTH, 1.f);
		else
			R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
		
		float scrollx = 0, scrolly = 0;
		if (surf->texinfo->flags & SURF_FLOWING)
			scrollx = -64;
		else if (surf->texinfo->flags & SURF_SCROLLX)
			scrollx = -32;
		if (surf->texinfo->flags & SURF_SCROLLY)
			scrolly = -32;
		if (surf->texinfo->flags & SURF_SCROLLFLIP)
		{
			scrollx = -scrollx; scrolly = -scrolly;
		}

		R_ProgUniform2f(LOC_FLOWSTRENGTH, scrollx, scrolly);
	}

	if (stylechanged)
	{
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorCopy(gfx_world.new_styles[i], gfx_world.styles[i]);
	}


	dst_indices = gfx_world.indicesBuf + gfx_world.numIndices;
	for (i = 0; i < numTris; i++) // q2pro code
	{
		dst_indices[0] = surf->firstvert;
		dst_indices[1] = surf->firstvert + (i + 1);
		dst_indices[2] = surf->firstvert + (i + 2);
		dst_indices += 3;
	}

	rperf.brush_tris += numTris;
	rperf.brush_polys++;

	gfx_world.numIndices += numIndices;
	gfx_world.tex_diffuse = tex_diffuse;
	gfx_world.tex_lm = tex_lightmap;
	gfx_world.uniformflags = uniformflags;
}

/*
=================
R_World_LightInlineModel

calculate dynamic lighting for brushmodel, adopts Spike's fix from QuakeSpasm
note: assumes pCurrentRefEnt & pCurrentModel are properly set
=================
*/
void R_World_LightInlineModel()
{
	dlight_t	*light;
	vec3_t		lightorg;
	int			i;

//	if (!pCurrentModel || pCurrentModel->type != MOD_BRUSH)
//		return; // this can never happen

	light = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, light++)
	{
		VectorSubtract(light->origin, pCurrentRefEnt->origin, lightorg);
		R_MarkLights(light, lightorg, (1 << i), (pCurrentModel->nodes + pCurrentModel->firstnode));
	}
}

/*
=================
R_DrawBrushModel_Internal

This is a copy of R_DrawInlineBModel adapted for batching
=================
*/
static void R_DrawBrushModel_Internal()
{
	int			i;
	cplane_t	*pplane;
	msurface_t* psurf;
	vec3_t		color;
	float		dot, alpha = 1.0f;

	// setup RF_COLOR and RF_TRANSLUCENT
	if (pCurrentRefEnt->renderfx & RF_COLOR)
		VectorCopy(pCurrentRefEnt->renderColor, color);
	else
		VectorSet(color, 1.0f, 1.0f, 1.0f);

	if ((pCurrentRefEnt->renderfx & RF_TRANSLUCENT) && pCurrentRefEnt->alpha != 1.0f)
	{
		if (pCurrentRefEnt->alpha <= 0.0f)
			return; // completly gone

		R_Blend(true);
		alpha = pCurrentRefEnt->alpha;
	}

	// light bmodel
	R_World_LightInlineModel();

	//
	// draw brushmodel
	//
	R_World_BeginRendering();
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], alpha);

	psurf = &pCurrentModel->surfaces[pCurrentModel->firstmodelsurface];
	for (i = 0; i < pCurrentModel->nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		pplane = psurf->plane;
		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
			{
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else
				R_World_NewDrawSurface(psurf, true);
		}
	}
	R_World_DrawAndFlushBufferedGeo();

	if (alpha != 1.0f)
		R_Blend(false);

	R_World_EndRendering();
}


/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel(rentity_t* e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (pCurrentModel->nummodelsurfaces == 0)
		return;

	pCurrentRefEnt = e;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - pCurrentModel->radius;
			maxs[i] = e->origin[i] + pCurrentModel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, pCurrentModel->mins, mins);
		VectorAdd(e->origin, pCurrentModel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;


	VectorSubtract(r_newrefdef.view.origin, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

#ifndef FIX_SQB
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
#endif
	R_RotateForEntity(e);
#ifndef FIX_SQB
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
#endif

	R_DrawBrushModel_Internal();
}

/*
================
R_World_DrawSortedByDiffuseMap
================
*/
static void R_World_DrawSortedByDiffuseMap()
{
	int		i;
	msurface_t* s;
	image_t* image;

	rperf.brush_textures = 0;

	R_World_BeginRendering();

	//
	// surfaces that aren't transparent or warped.
	//
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.brush_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
			{ 
				R_World_NewDrawSurface(s, true);
			}
		}
	}
	R_World_DrawAndFlushBufferedGeo();

	//
	// non-transparent warp surfaces. 
	// [ISB] To avoid a context change by changing shaders, and the overhead of updating uniforms,
	// the world shader always performs warps but the strength is zeroed to cancel it out. 
	//
	R_ProgUniform1f(LOC_WARPSTRENGTH, 1.f);
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
			{
				// draw surf
				R_World_NewDrawSurface(s, false);
			}
		}

		if (!r_showtris->value) // texture chains are used in r_showtris
			image->texturechain = NULL;
	}
	R_World_DrawAndFlushBufferedGeo();
	R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
	R_World_EndRendering();
}

/*
================
R_World_DrawAlphaSurfaces_NEW

Draw water surfaces and windows without writing to the depth buffer.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_World_DrawAlphaSurfaces()
{
	msurface_t* surf;
	
	// go back to the world matrix
	memcpy(r_local_matrix, mat4_identity, sizeof(mat4_t));

	//dont write z and enable blending
	R_WriteToDepthBuffer(false);
	R_Blend(true);

	R_World_BeginRendering();
	for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
	{
		R_World_NewDrawSurface(surf, false);
	}
	R_World_DrawAndFlushBufferedGeo();
	R_World_EndRendering();

	R_WriteToDepthBuffer(true);
	R_Blend(false);

	r_alpha_surfaces = NULL;
}

// ==============================================================================
// BSP TREE
// ==============================================================================

/*
===============
R_World_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
Note: a camera may be in two PVS areas hence there ate two clusters
===============
*/
void R_World_MarkLeaves()
{
	byte* vis;
	mnode_t* node;
	int		i, c;
	mleaf_t* leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where the pvs ends
	if (r_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i = 0; i < r_worldmodel->numleafs; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i = 0; i < r_worldmodel->numnodes; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_BSP_ClusterPVS(r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_BSP_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;
		for (i = 0; i < c; i++)
			((int*)fatvis)[i] |= ((int*)vis)[i];
		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster >> 3] & (1 << (cluster & 7)))
		{
			node = (mnode_t*)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}


/*
================
R_World_RecursiveNode

Builts two chains for solid and transparent geometry which we later use
to draw world, discards nodes which are not in PVS or frustum
================
*/
static void R_World_RecursiveNode(mnode_t* node)
{
	int			c, side, sidebit;
	cplane_t* plane;
	msurface_t* surf, ** mark;
	mleaf_t* pleaf;
	float		dot;
	image_t* image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBox(node->mins, node->maxs))
		return;

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t*)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (!(r_newrefdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))))
				return;	// not visible
		}

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

		return;
	}

	//
	// node is just a decision point, so go down the apropriate sides
	//

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
		dot = DotProduct(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_World_RecursiveNode(node->children[side]);

	// draw stuff
	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{
			//
			// add to visible sky bounds
			//
			R_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
		{
			//
			// add surface to the translucent chain
			//
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			//
			// add surface to the texture chain
			//
			// FIXME: this is a hack for animation
			image = R_World_TextureAnimation(surf->texinfo);
			surf->texturechain = image->texturechain;
			image->texturechain = surf;
		}
	}

	// recurse down the back side
	R_World_RecursiveNode(node->children[!side]);
}


/*
================
R_World_DrawTriangleOutlines

r_showtris

todo: get rid of im rendering calls
================
*/
static void R_World_DrawTriangleOutlines()
{
	int		i, j;
	msurface_t* surf;
	poly_t* p;
	image_t* image;

	if (!r_showtris->value)
		return;

	R_WriteToDepthBuffer(false);
	R_BeginLinesRendering(r_showtris->value >= 2 ? true : false);

	if (r_showtris->value > 1)
	{
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, -1);
		glLineWidth(2);
	}

	R_BindProgram(GLPROG_DEBUGLINE);
	R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);

	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		surf = image->texturechain;
		if (!surf)
			continue;

		for (; surf; surf = surf->texturechain)
		{
			if (r_showtris->value > 2)
			{
				if (surf->flags & SURF_DRAWTURB)
					R_ProgUniform4f(LOC_COLOR4, 0.0f, 0.2f, 1.0f, 1.0f);
				else if (surf->flags & SURF_UNDERWATER)
					R_ProgUniform4f(LOC_COLOR4, 0.0f, 1.0f, 0.0f, 1.0f);
				else
					R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);
			}

			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					glBegin(GL_LINE_STRIP);
					glVertex3fv(p->verts[0].pos);
					glVertex3fv(p->verts[j - 1].pos);
					glVertex3fv(p->verts[j].pos);
					glVertex3fv(p->verts[0].pos);
					glEnd();
				}
			}
		}
		image->texturechain = NULL;
	}

	// draw transparent
	if (r_alpha_surfaces)
	{
		R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 0.0f, 1.0f);
		for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
		{
			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					glBegin(GL_LINE_STRIP);
					glVertex3fv(p->verts[0].pos);
					glVertex3fv(p->verts[j - 1].pos);
					glVertex3fv(p->verts[j].pos);
					glVertex3fv(p->verts[0].pos);
					glEnd();
				}
			}
		}
	}

	R_UnbindProgram();

	if (r_showtris->value > 1)
	{
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0.0f, 0.0f);
	}

	R_EndLinesRendering();
	R_WriteToDepthBuffer(true);
}

/*
================
R_DrawWorld

Draws world and skybox
================
*/
void R_DrawWorld()
{
	rentity_t	ent;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.view.flags & RDF_NOWORLDMODEL)
		return;

	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time * 2);
	VectorSet(ent.renderColor, 1.0f, 1.0f, 1.0f);
	ent.alpha = 1.0f;

	pCurrentRefEnt = &ent;
	pCurrentModel = r_worldmodel;

	VectorCopy(r_newrefdef.view.origin, modelorg);

	R_ClearSkyBox();

	// build texture chains
	R_World_RecursiveNode(r_worldmodel->nodes);

	//no local transform needed for world. 
	memcpy(r_local_matrix, mat4_identity, sizeof(mat4_t));


	//
	// DRAW THE WORLD
	// 
	
	// should move this to Mod_LoadFaces or Mod_LoadBSP
	if (registration_sequence != gfx_world.registration_sequence)
	{
		R_DestroyWorldVertexBuffer();
		gfx_world.registration_sequence = registration_sequence;
	}

	if (!gfx_world.vbo) 
	{
		R_BuildVertexBufferForWorld();
	}

	R_World_DrawSortedByDiffuseMap();

	// 
	// DRAW SKYBOX
	//
	R_DrawSkyBox();

	R_World_DrawTriangleOutlines();
}
