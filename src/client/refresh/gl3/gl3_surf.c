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
static msurface_t *gl3_alpha_surfaces;

gl3lightmapstate_t gl3_lms;

extern gl3image_t gl3textures[MAX_TEXTURES];
extern int numgl3textures;

void
GL3_SurfInit(void)
{
	// init the VAO and VBO for the standard vertexdata: 10 floats and 1 uint
	// (X, Y, Z), (S, T), (LMS, LMT), (normX, normY, normZ) ; lightFlags - last two groups for lightmap/dynlights

	glGenVertexArrays(1, &gl3state.vao3D);
	GL3_BindVAO(gl3state.vao3D);

	glGenBuffers(1, &gl3state.vbo3D);
	GL3_BindVBO(gl3state.vbo3D);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), 0);

	glEnableVertexAttribArray(GL3_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL3_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, texCoord));

	glEnableVertexAttribArray(GL3_ATTRIB_LMTEXCOORD);
	qglVertexAttribPointer(GL3_ATTRIB_LMTEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, lmTexCoord));

	glEnableVertexAttribArray(GL3_ATTRIB_NORMAL);
	qglVertexAttribPointer(GL3_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(mvtx_t), offsetof(mvtx_t, normal));

	glEnableVertexAttribArray(GL3_ATTRIB_LIGHTFLAGS);
	qglVertexAttribIPointer(GL3_ATTRIB_LIGHTFLAGS, 1, GL_UNSIGNED_INT, sizeof(mvtx_t), offsetof(mvtx_t, lightFlags));

	glGenBuffers(1, &gl3state.ebo3D);

	// init VAO and VBO for model vertexdata: 9 floats
	// (X,Y,Z), (S,T), (R,G,B,A)

	glGenVertexArrays(1, &gl3state.vaoAlias);
	GL3_BindVAO(gl3state.vaoAlias);

	glGenBuffers(1, &gl3state.vboAlias);
	GL3_BindVBO(gl3state.vboAlias);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 0);

	glEnableVertexAttribArray(GL3_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL3_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 3*sizeof(GLfloat));

	glEnableVertexAttribArray(GL3_ATTRIB_COLOR);
	qglVertexAttribPointer(GL3_ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 5*sizeof(GLfloat));

	glGenBuffers(1, &gl3state.eboAlias);

	// init VAO and VBO for particle vertexdata: 9 floats
	// (X,Y,Z), (point_size,distace_to_camera), (R,G,B,A)

	glGenVertexArrays(1, &gl3state.vaoParticle);
	GL3_BindVAO(gl3state.vaoParticle);

	glGenBuffers(1, &gl3state.vboParticle);
	GL3_BindVBO(gl3state.vboParticle);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 0);

	// TODO: maybe move point size and camera origin to UBO and calculate distance in vertex shader
	glEnableVertexAttribArray(GL3_ATTRIB_TEXCOORD); // it's abused for (point_size, distance) here..
	qglVertexAttribPointer(GL3_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 3*sizeof(GLfloat));

	glEnableVertexAttribArray(GL3_ATTRIB_COLOR);
	qglVertexAttribPointer(GL3_ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 5*sizeof(GLfloat));
}

void GL3_SurfShutdown(void)
{
	glDeleteBuffers(1, &gl3state.ebo3D);
	gl3state.ebo3D = 0;
	glDeleteBuffers(1, &gl3state.vbo3D);
	gl3state.vbo3D = 0;
	glDeleteVertexArrays(1, &gl3state.vao3D);
	gl3state.vao3D = 0;

	glDeleteBuffers(1, &gl3state.eboAlias);
	gl3state.eboAlias = 0;
	glDeleteBuffers(1, &gl3state.vboAlias);
	gl3state.vboAlias = 0;
	glDeleteVertexArrays(1, &gl3state.vaoAlias);
	gl3state.vaoAlias = 0;
}

static void
SetLightFlags(msurface_t *surf)
{
	unsigned int lightFlags = 0;
	if (surf->dlightframe == r_framecount)
	{
		lightFlags = surf->dlightbits;
	}

	mvtx_t* verts = surf->polys->verts;

	int numVerts = surf->polys->numverts;
	for (int i=0; i<numVerts; ++i)
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
	for (int i=0; i<numVerts; ++i)
	{
		verts[i].lightFlags = lightFlags;
	}
}

static void
GL3_DrawGLPoly(const msurface_t *fa, gl3drawCmd_t drawCmd)
{
	const mpoly_t *p = fa->polys;

	GL3_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN, drawCmd);
}

static void
GL3_DrawGLFlowingPoly(const msurface_t *fa, gl3drawCmd_t drawCmd)
{
	const mpoly_t *p;
	float sscroll, tscroll;

	p = fa->polys;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	drawCmd.sscroll = sscroll;
	drawCmd.tscroll = tscroll;
	drawCmd.flags |= DCFlag_UseScroll;

	GL3_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN, drawCmd);
}

#define LINE_VTX_COUNT (256 * 6)

static void
DrawTriangleOutlines(void)
{
	static mvtx_t vtx[LINE_VTX_COUNT];
	const msurface_t *surf;
	size_t i, curr_vtx;

	if (!r_showtris->value)
	{
		return;
	}

	GL3_Draw3DBatchesNow();

	glDisable(GL_DEPTH_TEST);
	glUseProgram(gl3state.si3DcolorOnly.shaderProgram);

	gl3state.uniCommonData.color = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	GL3_UpdateUBOCommon();
	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	curr_vtx = 0;

	memset(vtx, 0, sizeof(vtx));
	for (i = 0, surf = r_worldmodel->surfaces; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		const mpoly_t *p;

		if (surf->visframe != r_framecount)
		{
			continue;
		}

		for (p = surf->polys; p != NULL; p = p->chain)
		{
			size_t j;

			for (j = 2; j < p->numverts; j++)
			{
				size_t k;

				if (curr_vtx > (LINE_VTX_COUNT - 6))
				{
					glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx, GL_STREAM_DRAW);
					glDrawArrays(GL_LINES, 0, curr_vtx);
					curr_vtx = 0;
					memset(vtx, 0, sizeof(vtx));
				}

				for (k = 0; k < 3; k++)
				{
					vtx[curr_vtx + 0].pos[k] = p->verts[0].pos[k];
					vtx[curr_vtx + 1].pos[k] = p->verts[j - 1].pos[k];

					vtx[curr_vtx + 2].pos[k] = p->verts[j - 1].pos[k];
					vtx[curr_vtx + 3].pos[k] = p->verts[j].pos[k];

					vtx[curr_vtx + 4].pos[k] = p->verts[j].pos[k];
					vtx[curr_vtx + 5].pos[k] = p->verts[0].pos[k];
				}

				curr_vtx += 6;
			}
		}
	}

	if (curr_vtx)
	{
		glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx, GL_STREAM_DRAW);
		glDrawArrays(GL_LINES, 0, curr_vtx);
	}

	glEnable(GL_DEPTH_TEST);
}

static void
RenderBrushPoly(const entity_t *currententity, msurface_t *fa, gl3drawCmd_t drawCmd)
{
	const gl3image_t *image;

	c_brush_polys++;

	image = R_TextureAnimation(currententity, fa->texinfo);
	drawCmd.texnum = image->texnum;

	if (fa->flags & SURF_DRAWTURB)
	{
		GL3_EmitWaterPolys(fa, drawCmd);
		return;
	}

	drawCmd.lmtexnum = fa->lightmaptexturenum;

	// Any dynamic lights on this surface?
	// TODO: maybe put lightstyles into a uniform (it's just 256 vec4)
	//       and put fa->styles[] into the 3d draw vertex?
	memcpy(drawCmd.styles, fa->styles, sizeof(fa->styles));
	drawCmd.flags |= DCFlag_UseLmStyles;

	if (fa->texinfo->flags & SURF_SCROLL)
	{
		GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3DlmFlow);
		GL3_DrawGLFlowingPoly(fa, drawCmd);
	}
	else
	{
		GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3Dlm);
		GL3_DrawGLPoly(fa, drawCmd);
	}

	// Note: lightmap chains are gone, lightmaps are rendered together with normal texture in one pass
}

/*
 * Draw water surfaces and windows.
 * The BSP tree is walked front to back, so unwinding the chain
 * of alpha_surfaces will draw back to front, giving proper ordering.
 */
void
GL3_DrawAlphaSurfaces(void)
{
	msurface_t *s;

	/* go back to the world matrix */
	gl3drawCmd_t drawCmd = GL3_CreateDrawCmd();
	drawCmd.flags |= DCFlag_Blend;

	for (s = gl3_alpha_surfaces; s != NULL; s = s->texturechain)
	{
		drawCmd.texnum = s->texinfo->image->texnum;
		c_brush_polys++;
		if (s->texinfo->flags & SURF_TRANS33)
		{
			drawCmd.alpha = 0.333f;
		}
		else if (s->texinfo->flags & SURF_TRANS66)
		{
			drawCmd.alpha = 0.666f;
		}
		else
		{
			drawCmd.alpha = 1.0f;
		}

		if (s->flags & SURF_DRAWTURB)
		{
			GL3_EmitWaterPolys(s, drawCmd);
		}
		else if (s->texinfo->flags & SURF_SCROLL)
		{
			GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3DtransFlow);
			GL3_DrawGLFlowingPoly(s, drawCmd);
		}
		else
		{
			GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3Dtrans);
			GL3_DrawGLPoly(s, drawCmd);
		}
	}

	gl3_alpha_surfaces = NULL;
}

static void
DrawTextureChains(const entity_t *currententity)
{
	int i;
	msurface_t *s;
	gl3image_t *image;

	c_visible_textures = 0;

	gl3drawCmd_t drawCmd = GL3_CreateDrawCmd();

	for (i = 0, image = gl3textures; i < numgl3textures; i++, image++)
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
			RenderBrushPoly(currententity, s, drawCmd);
		}

		image->texturechain = NULL;
	}

	// TODO: maybe one loop for normal faces and one for SURF_DRAWTURB ???
}

static void
RenderLightmappedPoly(const entity_t *currententity, msurface_t *surf, gl3drawCmd_t drawCmd)
{
	const gl3image_t *image = R_TextureAnimation(currententity, surf->texinfo);

	assert((surf->texinfo->flags & (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)) == 0
			&& "RenderLightMappedPoly mustn't be called with transparent, sky or warping surfaces!");

	// Any dynamic lights on this surface?
	memcpy(drawCmd.styles, surf->styles, sizeof(surf->styles));
	drawCmd.flags |= DCFlag_UseLmStyles;

	c_brush_polys++;

	drawCmd.texnum = image->texnum;
	drawCmd.lmtexnum = surf->lightmaptexturenum;

	if (surf->texinfo->flags & SURF_SCROLL)
	{
		GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3DlmFlow);
		GL3_DrawGLFlowingPoly(surf, drawCmd);
	}
	else
	{
		GL3_SetDrawCmdShader(&drawCmd, &gl3state.si3Dlm);
		GL3_DrawGLPoly(surf, drawCmd);
	}
}

static void
DrawInlineBModel(const entity_t *currententity, model_t *currentmodel, gl3drawCmd_t drawCmd)
{
	msurface_t *psurf;
	size_t i;

	/* calculate dynamic lighting for bmodel */
	R_PushDlights(&r_newrefdef, currentmodel->nodes + currentmodel->firstnode,
			r_dlightframecount, currentmodel->surfaces);

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		drawCmd.flags |= DCFlag_Blend;
		/* TODO: should I care about the 0.25 part? we'll just set alpha to 0.33 or 0.66 depending on surface flag..
		glColor4f(1, 1, 1, 0.25);
		R_TexEnv(GL_MODULATE);
		*/
	}

	/* draw texture */
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++)
	{
		cplane_t *pplane;
		float dot;

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
				psurf->texturechain = gl3_alpha_surfaces;
				gl3_alpha_surfaces = psurf;
			}
			else if (!(psurf->flags & SURF_DRAWTURB))
			{
				SetAllLightFlags(psurf);
				RenderLightmappedPoly(currententity, psurf, drawCmd);
			}
			else
			{
				RenderBrushPoly(currententity, psurf, drawCmd);
			}
		}
	}

}

void
GL3_DrawBrushModel(entity_t *e, model_t *currentmodel)
{
	vec3_t mins, maxs;
	qboolean rotated;

	if (currentmodel->nummodelsurfaces == 0)
	{
		return;
	}

	gl3drawCmd_t drawCmd = GL3_CreateDrawCmd();
	if (e->flags & RF_TRANSLUCENT)
		drawCmd.flags |= DCFlag_DisableDepthMask;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		int i;

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

	if (r_cull->value && R_CullBox(mins, maxs))
	{
		return;
	}

	if (r_zfix->value)
	{
		drawCmd.flags |= DCFlag_PolyOffsetFill;
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

	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];
	GL3_RotateForEntity(e, &drawCmd);
	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];

	DrawInlineBModel(e, currentmodel, drawCmd);
}

static void
R_RenderFace(entity_t *currententity, msurface_t *surf, int clipflags)
{
	if (surf->texinfo->flags & SURF_SKY)
	{
		/* just adds to visible sky bounds */
		RE_AddSkySurface(surf);
	}
	else if (surf->texinfo->flags & SURF_TRANSPARENT)
	{
		/* add to the translucent chain */
		surf->texturechain = gl3_alpha_surfaces;
		gl3_alpha_surfaces = surf;
		gl3_alpha_surfaces->texinfo->image = R_TextureAnimation(currententity, surf->texinfo);
	}
	else if (surf->texinfo->flags & SURF_NODRAW)
	{
		/* Surface should be skipped */
	}
	else
	{
		// calling RenderLightmappedPoly() here probably isn't optimal, rendering everything
		// through texturechains should be faster, because far less glBindTexture() is needed
		// (and it might allow batching the drawcalls of surfaces with the same texture)
#if 0
		if (!(surf->flags & SURF_DRAWTURB))
		{
			RenderLightmappedPoly(surf);
		}
		else
#endif // 0
		{
			gl3image_t *image;

			/* the polygon is visible, so add it to the texture sorted chain */
			image = R_TextureAnimation(currententity, surf->texinfo);
			surf->texturechain = image->texturechain;
			image->texturechain = surf;
		}
	}
}

void
GL3_DrawWorld(void)
{
	entity_t ent;

	if ((!r_drawworld->value) || (r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		return;
	}

	VectorCopy(r_newrefdef.vieworg, modelorg);

	/* auto cycle the world frame for texture animation */
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time * 2);
	ent.model = r_worldmodel;
	VectorCopy(r_newrefdef.vieworg, ent.origin);

	gl3state.currenttexture = -1;

	RE_ClearSkyBox();
	R_RecursiveWorldNode(&ent, r_worldmodel->nodes, ALIAS_XY_CLIP_MASK,
		R_RenderFace);
	DrawTextureChains(&ent);
	GL3_DrawSkyBox();
	DrawTriangleOutlines();

	// make sure all this is drawed
	GL3_Draw3DBatchesNow();
}
