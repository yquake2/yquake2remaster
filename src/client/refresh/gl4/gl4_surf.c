/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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
 * Surface generation and drawing
 *
 * =======================================================================
 */

#include <assert.h>
#include <stddef.h> // ofsetof()

#include "header/local.h"

int c_visible_lightmaps;
int c_visible_textures;
static vec3_t modelorg; /* relative to viewpoint */
static msurface_t *gl4_alpha_surfaces;

gl4lightmapstate_t gl4_lms;

extern gl4image_t gl4textures[MAX_TEXTURES];
extern int numgl4textures;

void GL4_SurfInit(void)
{
	// init the VAO and VBO for the standard vertexdata: 10 floats and 1 uint
	// (X, Y, Z), (S, T), (LMS, LMT), (normX, normY, normZ) ; lightFlags - last two groups for lightmap/dynlights

	glGenVertexArrays(1, &gl4state.vao3D);
	GL4_BindVAO(gl4state.vao3D);

	glGenBuffers(1, &gl4state.vbo3D);
	GL4_BindVBO(gl4state.vbo3D);

	if(gl4config.useBigVBO)
	{
		gl4state.vbo3Dsize = 5*1024*1024; // a 5MB buffer seems to work well?
		gl4state.vbo3DcurOffset = 0;
		glBufferData(GL_ARRAY_BUFFER, gl4state.vbo3Dsize, NULL, GL_STREAM_DRAW); // allocate/reserve that data
	}

	glEnableVertexAttribArray(GL4_ATTRIB_POSITION);
	qglVertexAttribPointer(GL4_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), 0);

	glEnableVertexAttribArray(GL4_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL4_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, texCoord));

	glEnableVertexAttribArray(GL4_ATTRIB_LMTEXCOORD);
	qglVertexAttribPointer(GL4_ATTRIB_LMTEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, lmTexCoord));

	glEnableVertexAttribArray(GL4_ATTRIB_NORMAL);
	qglVertexAttribPointer(GL4_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, normal));

	glEnableVertexAttribArray(GL4_ATTRIB_LIGHTFLAGS);
	qglVertexAttribIPointer(GL4_ATTRIB_LIGHTFLAGS, 1, GL_UNSIGNED_INT, sizeof(mvtx_t), offsetof(mvtx_t, lightFlags));



	// init VAO and VBO for model vertexdata: 9 floats
	// (X,Y,Z), (S,T), (R,G,B,A)

	glGenVertexArrays(1, &gl4state.vaoAlias);
	GL4_BindVAO(gl4state.vaoAlias);

	glGenBuffers(1, &gl4state.vboAlias);
	GL4_BindVBO(gl4state.vboAlias);

	glEnableVertexAttribArray(GL4_ATTRIB_POSITION);
	qglVertexAttribPointer(GL4_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 0);

	glEnableVertexAttribArray(GL4_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL4_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 3*sizeof(GLfloat));

	glEnableVertexAttribArray(GL4_ATTRIB_COLOR);
	qglVertexAttribPointer(GL4_ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 5*sizeof(GLfloat));

	glGenBuffers(1, &gl4state.eboAlias);

	// init VAO and VBO for particle vertexdata: 9 floats
	// (X,Y,Z), (point_size,distace_to_camera), (R,G,B,A)

	glGenVertexArrays(1, &gl4state.vaoParticle);
	GL4_BindVAO(gl4state.vaoParticle);

	glGenBuffers(1, &gl4state.vboParticle);
	GL4_BindVBO(gl4state.vboParticle);

	glEnableVertexAttribArray(GL4_ATTRIB_POSITION);
	qglVertexAttribPointer(GL4_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 0);

	// TODO: maybe move point size and camera origin to UBO and calculate distance in vertex shader
	glEnableVertexAttribArray(GL4_ATTRIB_TEXCOORD); // it's abused for (point_size, distance) here..
	qglVertexAttribPointer(GL4_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 3*sizeof(GLfloat));

	glEnableVertexAttribArray(GL4_ATTRIB_COLOR);
	qglVertexAttribPointer(GL4_ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 5*sizeof(GLfloat));
}

void GL4_SurfShutdown(void)
{
	glDeleteBuffers(1, &gl4state.vbo3D);
	gl4state.vbo3D = 0;
	glDeleteVertexArrays(1, &gl4state.vao3D);
	gl4state.vao3D = 0;

	glDeleteBuffers(1, &gl4state.eboAlias);
	gl4state.eboAlias = 0;
	glDeleteBuffers(1, &gl4state.vboAlias);
	gl4state.vboAlias = 0;
	glDeleteVertexArrays(1, &gl4state.vaoAlias);
	gl4state.vaoAlias = 0;
}

static void
SetLightFlags(msurface_t *surf)
{
	unsigned int lightFlags = 0;
	if (surf->dlightframe == gl4_framecount)
	{
		lightFlags = surf->dlightbits;
	}

	mvtx_t* verts = surf->polys->verts;

	int numVerts = surf->polys->numverts;
	for(int i=0; i<numVerts; ++i)
	{
		verts[i].lightFlags = lightFlags;
	}
}

static void
SetAllLightFlags(msurface_t *surf)
{
	unsigned int lightFlags = 0xffffffff;

	mvtx_t* verts = surf->polys->verts;

	int numVerts = surf->polys->numverts;
	for(int i=0; i<numVerts; ++i)
	{
		verts[i].lightFlags = lightFlags;
	}
}

void
GL4_DrawGLPoly(msurface_t *fa)
{
	mpoly_t *p = fa->polys;

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	GL4_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN);
}

void
GL4_DrawGLFlowingPoly(msurface_t *fa)
{
	mpoly_t *p;
	float sscroll, tscroll;

	p = fa->polys;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	if((gl4state.uni3DData.sscroll != sscroll) || (gl4state.uni3DData.tscroll != tscroll))
	{
		gl4state.uni3DData.sscroll = sscroll;
		gl4state.uni3DData.tscroll = tscroll;
		GL4_UpdateUBO3D();
	}

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	GL4_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN);
}

static void
DrawTriangleOutlines(void)
{
	STUB_ONCE("TODO: Implement for r_showtris support!");
#if 0
	int i, j;
	mpoly_t *p;

	if (!r_showtris->value)
	{
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glColor4f(1, 1, 1, 1);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		msurface_t *surf;

		for (surf = gl4_lms.lightmap_surfaces[i];
				surf != 0;
				surf = surf->lightmapchain)
		{
			p = surf->polys;

			for ( ; p; p = p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					GLfloat vtx[12];
					unsigned int k;

					for (k=0; k<3; k++)
					{
						vtx[0+k] = p->verts [ 0 ][ k ];
						vtx[3+k] = p->verts [ j - 1 ][ k ];
						vtx[6+k] = p->verts [ j ][ k ];
						vtx[9+k] = p->verts [ 0 ][ k ];
					}

					glEnableClientState( GL_VERTEX_ARRAY );

					glVertexPointer( 3, GL_FLOAT, 0, vtx );
					glDrawArrays( GL_LINE_STRIP, 0, 4 );

					glDisableClientState( GL_VERTEX_ARRAY );
				}
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
#endif // 0
}

static void
UpdateLMscales(const hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE], gl4ShaderInfo_t* si)
{
	int i;
	qboolean hasChanged = false;

	for(i=0; i<MAX_LIGHTMAPS_PER_SURFACE; ++i)
	{
		if(hasChanged)
		{
			si->lmScales[i] = lmScales[i];
		}
		else if(   si->lmScales[i].R != lmScales[i].R
		        || si->lmScales[i].G != lmScales[i].G
		        || si->lmScales[i].B != lmScales[i].B
		        || si->lmScales[i].A != lmScales[i].A )
		{
			si->lmScales[i] = lmScales[i];
			hasChanged = true;
		}
	}

	if(hasChanged)
	{
		glUniform4fv(si->uniLmScalesOrTime, MAX_LIGHTMAPS_PER_SURFACE, si->lmScales[0].Elements);
	}
}

static void
RenderBrushPoly(entity_t *currententity, msurface_t *fa)
{
	int map;
	gl4image_t *image;

	c_brush_polys++;

	image = R_TextureAnimation(currententity, fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		GL4_Bind(image->texnum);

		GL4_EmitWaterPolys(fa);

		return;
	}
	else
	{
		GL4_Bind(image->texnum);
	}

	hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE] = {0};
	lmScales[0] = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	GL4_BindLightmap(fa->lightmaptexturenum);

	// Any dynamic lights on this surface?
	for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE && fa->styles[map] != 255; map++)
	{
		lmScales[map].R = r_newrefdef.lightstyles[fa->styles[map]].rgb[0];
		lmScales[map].G = r_newrefdef.lightstyles[fa->styles[map]].rgb[1];
		lmScales[map].B = r_newrefdef.lightstyles[fa->styles[map]].rgb[2];
		lmScales[map].A = 1.0f;
	}

	if (fa->texinfo->flags & SURF_SCROLL)
	{
		GL4_UseProgram(gl4state.si3DlmFlow.shaderProgram);
		UpdateLMscales(lmScales, &gl4state.si3DlmFlow);
		GL4_DrawGLFlowingPoly(fa);
	}
	else
	{
		GL4_UseProgram(gl4state.si3Dlm.shaderProgram);
		UpdateLMscales(lmScales, &gl4state.si3Dlm);
		GL4_DrawGLPoly(fa);
	}

	// Note: lightmap chains are gone, lightmaps are rendered together with normal texture in one pass
}

/*
 * Draw water surfaces and windows.
 * The BSP tree is waled front to back, so unwinding the chain
 * of alpha_surfaces will draw back to front, giving proper ordering.
 */
void
GL4_DrawAlphaSurfaces(void)
{
	msurface_t *s;

	/* go back to the world matrix */
	gl4state.uni3DData.transModelMat4 = gl4_identityMat4;
	GL4_UpdateUBO3D();

	glEnable(GL_BLEND);

	for (s = gl4_alpha_surfaces; s != NULL; s = s->texturechain)
	{
		GL4_Bind(s->texinfo->image->texnum);
		c_brush_polys++;
		float alpha = 1.0f;
		if (s->texinfo->flags & SURF_TRANS33)
		{
			alpha = 0.333f;
		}
		else if (s->texinfo->flags & SURF_TRANS66)
		{
			alpha = 0.666f;
		}

		if(alpha != gl4state.uni3DData.alpha)
		{
			gl4state.uni3DData.alpha = alpha;
			GL4_UpdateUBO3D();
		}

		if (s->flags & SURF_DRAWTURB)
		{
			GL4_EmitWaterPolys(s);
		}
		else if (s->texinfo->flags & SURF_SCROLL)
		{
			GL4_UseProgram(gl4state.si3DtransFlow.shaderProgram);
			GL4_DrawGLFlowingPoly(s);
		}
		else
		{
			GL4_UseProgram(gl4state.si3Dtrans.shaderProgram);
			GL4_DrawGLPoly(s);
		}
	}

	gl4state.uni3DData.alpha = 1.0f;
	GL4_UpdateUBO3D();

	glDisable(GL_BLEND);

	gl4_alpha_surfaces = NULL;
}

static void
DrawTextureChains(entity_t *currententity)
{
	int i;
	msurface_t *s;
	gl4image_t *image;

	c_visible_textures = 0;

	for (i = 0, image = gl4textures; i < numgl4textures; i++, image++)
	{
		if (!image->registration_sequence)
		{
			continue;
		}

		s = image->texturechain;

		if (!s)
		{
			continue;
		}

		c_visible_textures++;

		for ( ; s; s = s->texturechain)
		{
			SetLightFlags(s);
			RenderBrushPoly(currententity, s);
		}

		image->texturechain = NULL;
	}

	// TODO: maybe one loop for normal faces and one for SURF_DRAWTURB ???
}

static void
RenderLightmappedPoly(entity_t *currententity, msurface_t *surf)
{
	int map;
	gl4image_t *image = R_TextureAnimation(currententity, surf->texinfo);

	hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE] = {0};
	lmScales[0] = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	assert((surf->texinfo->flags & (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)) == 0
			&& "RenderLightMappedPoly mustn't be called with transparent, sky or warping surfaces!");

	// Any dynamic lights on this surface?
	for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[map] != 255; map++)
	{
		lmScales[map].R = r_newrefdef.lightstyles[surf->styles[map]].rgb[0];
		lmScales[map].G = r_newrefdef.lightstyles[surf->styles[map]].rgb[1];
		lmScales[map].B = r_newrefdef.lightstyles[surf->styles[map]].rgb[2];
		lmScales[map].A = 1.0f;
	}

	c_brush_polys++;

	GL4_Bind(image->texnum);
	GL4_BindLightmap(surf->lightmaptexturenum);

	if (surf->texinfo->flags & SURF_SCROLL)
	{
		GL4_UseProgram(gl4state.si3DlmFlow.shaderProgram);
		UpdateLMscales(lmScales, &gl4state.si3DlmFlow);
		GL4_DrawGLFlowingPoly(surf);
	}
	else
	{
		GL4_UseProgram(gl4state.si3Dlm.shaderProgram);
		UpdateLMscales(lmScales, &gl4state.si3Dlm);
		GL4_DrawGLPoly(surf);
	}
}

static void
DrawInlineBModel(entity_t *currententity, gl4model_t *currentmodel)
{
	int i;
	cplane_t *pplane;
	float dot;
	msurface_t *psurf;

	R_PushDlights(&r_newrefdef, currentmodel->nodes + currentmodel->firstnode,
			r_dlightframecount, currentmodel->surfaces);

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glEnable(GL_BLEND);
		/* TODO: should I care about the 0.25 part? we'll just set alpha to 0.33 or 0.66 depending on surface flag..
		glColor4f(1, 1, 1, 0.25);
		R_TexEnv(GL_MODULATE);
		*/
	}

	/* draw texture */
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++)
	{
		/* find which side of the node we are on */
		pplane = psurf->plane;

		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		/* draw the polygon */
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & SURF_TRANSPARENT)
			{
				/* add to the translucent chain */
				psurf->texturechain = gl4_alpha_surfaces;
				gl4_alpha_surfaces = psurf;
			}
			else if(!(psurf->flags & SURF_DRAWTURB))
			{
				SetAllLightFlags(psurf);
				RenderLightmappedPoly(currententity, psurf);
			}
			else
			{
				RenderBrushPoly(currententity, psurf);
			}
		}
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glDisable(GL_BLEND);
	}
}

void
GL4_DrawBrushModel(entity_t *e, gl4model_t *currentmodel)
{
	vec3_t mins, maxs;
	int i;
	qboolean rotated;

	if (currentmodel->nummodelsurfaces == 0)
	{
		return;
	}

	gl4state.currenttexture = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, currentmodel->mins, mins);
		VectorAdd(e->origin, currentmodel->maxs, maxs);
	}

	if (r_cull->value && R_CullBox(mins, maxs, frustum))
	{
		return;
	}

	if (gl_zfix->value)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	VectorSubtract(r_newrefdef.vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}



	//glPushMatrix();
	hmm_mat4 oldMat = gl4state.uni3DData.transModelMat4;

	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];
	GL4_RotateForEntity(e);
	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];

	DrawInlineBModel(e, currentmodel);

	// glPopMatrix();
	gl4state.uni3DData.transModelMat4 = oldMat;
	GL4_UpdateUBO3D();

	if (gl_zfix->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

static void
RecursiveWorldNode(entity_t *currententity, mnode_t *node)
{
	int c, side, sidebit;
	cplane_t *plane;
	msurface_t *surf, **mark;
	mleaf_t *pleaf;
	float dot;
	gl4image_t *image;

	if (node->contents == CONTENTS_SOLID)
	{
		return; /* solid */
	}

	if (node->visframe != gl4_visframecount)
	{
		return;
	}

	if (r_cull->value && R_CullBox(node->minmaxs, node->minmaxs + 3, frustum))
	{
		return;
	}

	/* if a leaf node, draw stuff */
	if (node->contents != CONTENTS_NODE)
	{
		pleaf = (mleaf_t *)node;

		/* check for door connected areas */
		// check for door connected areas
		if (!R_AreaVisible(r_newrefdef.areabits, pleaf))
			return;	// not visible

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = gl4_framecount;
				mark++;
			}
			while (--c);
		}

		return;
	}

	/* node is just a decision point, so go down the apropriate
	   sides find which side of the node we are on */
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

	/* recurse down the children, front side first */
	RecursiveWorldNode(currententity, node->children[side]);

	if ((node->numsurfaces + node->firstsurface) > gl4_worldmodel->numsurfaces)
	{
		R_Printf(PRINT_ALL, "Broken node firstsurface\n");
		return;
	}

	/* draw stuff */
	for (c = node->numsurfaces,
		 surf = gl4_worldmodel->surfaces + node->firstsurface;
		 c; c--, surf++)
	{
		if (surf->visframe != gl4_framecount)
		{
			continue;
		}

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
		{
			continue; /* wrong side */
		}

		if (surf->texinfo->flags & SURF_SKY)
		{
			/* just adds to visible sky bounds */
			GL4_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & SURF_TRANSPARENT)
		{
			/* add to the translucent chain */
			surf->texturechain = gl4_alpha_surfaces;
			gl4_alpha_surfaces = surf;
			gl4_alpha_surfaces->texinfo->image = R_TextureAnimation(currententity, surf->texinfo);
		}
		else if (surf->texinfo->flags & SURF_NODRAW)
		{
			/* Surface should be skipped */
			continue;
		}
		else
		{
			// calling RenderLightmappedPoly() here probably isn't optimal, rendering everything
			// through texturechains should be faster, because far less glBindTexture() is needed
			// (and it might allow batching the drawcalls of surfaces with the same texture)
#if 0
			if(!(surf->flags & SURF_DRAWTURB))
			{
				RenderLightmappedPoly(surf);
			}
			else
#endif // 0
			{
				/* the polygon is visible, so add it to the texture sorted chain */
				image = R_TextureAnimation(currententity, surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	/* recurse down the back side */
	RecursiveWorldNode(currententity, node->children[!side]);
}

void
GL4_DrawWorld(void)
{
	entity_t ent;

	if (!r_drawworld->value)
	{
		return;
	}

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	VectorCopy(r_newrefdef.vieworg, modelorg);

	/* auto cycle the world frame for texture animation */
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time * 2);

	gl4state.currenttexture = -1;

	RE_ClearSkyBox();
	RecursiveWorldNode(&ent, gl4_worldmodel->nodes);
	DrawTextureChains(&ent);
	GL4_DrawSkyBox();
	DrawTriangleOutlines();
}

/*
 * Mark the leaves and nodes that are
 * in the PVS for the current cluster
 */
void
GL4_MarkLeaves(void)
{
	const byte *vis;
	byte *fatvis = NULL;
	mnode_t *node;
	int i;
	mleaf_t *leaf;

	if ((gl4_oldviewcluster == gl4_viewcluster) &&
		(gl4_oldviewcluster2 == gl4_viewcluster2) &&
		!r_novis->value &&
		(gl4_viewcluster != -1))
	{
		return;
	}

	/* development aid to let you run around
	   and see exactly where the pvs ends */
	if (r_lockpvs->value)
	{
		return;
	}

	gl4_visframecount++;
	gl4_oldviewcluster = gl4_viewcluster;
	gl4_oldviewcluster2 = gl4_viewcluster2;

	if (r_novis->value || (gl4_viewcluster == -1) || !gl4_worldmodel->vis)
	{
		/* mark everything */
		for (i = 0; i < gl4_worldmodel->numleafs; i++)
		{
			gl4_worldmodel->leafs[i].visframe = gl4_visframecount;
		}

		for (i = 0; i < gl4_worldmodel->numnodes; i++)
		{
			gl4_worldmodel->nodes[i].visframe = gl4_visframecount;
		}

		return;
	}

	vis = GL4_Mod_ClusterPVS(gl4_viewcluster, gl4_worldmodel);

	/* may have to combine two clusters because of solid water boundaries */
	if (gl4_viewcluster2 != gl4_viewcluster)
	{
		int c;

		fatvis = malloc(((gl4_worldmodel->numleafs + 31) / 32) * sizeof(int));
		memcpy(fatvis, vis, (gl4_worldmodel->numleafs + 7) / 8);
		vis = GL4_Mod_ClusterPVS(gl4_viewcluster2, gl4_worldmodel);
		c = (gl4_worldmodel->numleafs + 31) / 32;

		for (i = 0; i < c; i++)
		{
			((int *)fatvis)[i] |= ((int *)vis)[i];
		}

		vis = fatvis;
	}

	for (i = 0, leaf = gl4_worldmodel->leafs;
		 i < gl4_worldmodel->numleafs;
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
				if (node->visframe == gl4_visframecount)
				{
					break;
				}

				node->visframe = gl4_visframecount;
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
