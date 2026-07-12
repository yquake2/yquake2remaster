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
#include <stdlib.h>

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

	if (gl3config.useBigVBO)
	{
		gl3state.vbo3Dsize = 5*1024*1024; // a 5MB buffer seems to work well?
		gl3state.vbo3DcurOffset = 0;
		glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW); // allocate/reserve that data
	}

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
GL3_DrawGLPoly(const msurface_t *fa)
{
	const mpoly_t *p = fa->polys;

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	GL3_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN);
}

static void
GL3_DrawGLFlowingPoly(const msurface_t *fa)
{
	const mpoly_t *p;
	float sscroll, tscroll;

	p = fa->polys;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	if ((gl3state.uni3DData.sscroll != sscroll) || (gl3state.uni3DData.tscroll != tscroll))
	{
		gl3state.uni3DData.sscroll = sscroll;
		gl3state.uni3DData.tscroll = tscroll;
		GL3_UpdateUBO3D();
	}

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	GL3_BufferAndDraw3D(p->verts, p->numverts, GL_TRIANGLE_FAN);
}

static void
DrawTriangleOutlines(void)
{
	const msurface_t *surf;
	size_t i;

	if (!r_showtris->value)
	{
		return;
	}

	glDisable(GL_DEPTH_TEST);
	GL3_UseProgram(gl3state.si3DcolorOnly.shaderProgram);

	gl3state.uniCommonData.color = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	GL3_UpdateUBOCommon();

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
				mvtx_t vtx[4];
				size_t k;

				for (k = 0; k < 3; k++)
				{
					vtx[0].pos[k] = p->verts[0].pos[k];
					vtx[1].pos[k] = p->verts[j - 1].pos[k];
					vtx[2].pos[k] = p->verts[j].pos[k];
					vtx[3].pos[k] = p->verts[0].pos[k];
				}

				// set other fields to 0
				for (k = 0; k < 4; k++)
				{
					vtx[k].texCoord[0] = 0;
					vtx[k].texCoord[1] = 0;
					vtx[k].lmTexCoord[0] = 0;
					vtx[k].lmTexCoord[1] = 0;
					vtx[k].normal[0] = 0;
					vtx[k].normal[1] = 0;
					vtx[k].normal[2] = 0;
					vtx[k].lightFlags = 0;
				}

				GL3_BufferAndDraw3D(vtx, 4, GL_LINE_STRIP);
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
}

static void
UpdateLMscales(const hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE], gl3ShaderInfo_t* si)
{
	int i;
	qboolean hasChanged = false;

	for (i=0; i<MAX_LIGHTMAPS_PER_SURFACE; ++i)
	{
		if (hasChanged)
		{
			si->lmScales[i] = lmScales[i];
		}
		else if (  si->lmScales[i].R != lmScales[i].R
		        || si->lmScales[i].G != lmScales[i].G
		        || si->lmScales[i].B != lmScales[i].B
		        || si->lmScales[i].A != lmScales[i].A )
		{
			si->lmScales[i] = lmScales[i];
			hasChanged = true;
		}
	}

	if (hasChanged)
	{
		glUniform4fv(si->uniLmScalesOrTime, MAX_LIGHTMAPS_PER_SURFACE, si->lmScales[0].Elements);
	}
}

typedef struct
{
	qboolean active;
	int lightmaptexturenum;
	qboolean flowing;
	float sscroll;
	float tscroll;
	hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE];
	size_t vertex_count;
	size_t draw_count;
	size_t vertex_capacity;
	size_t draw_capacity;
	mvtx_t *verts;
	GLint *firsts;
	GLsizei *counts;
} gl3_chain_batch_t;

static void
BuildLMScales(const msurface_t *surf, hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE])
{
	int map;

	memset(lmScales, 0, sizeof(hmm_vec4) * MAX_LIGHTMAPS_PER_SURFACE);
	lmScales[0] = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Any dynamic lights on this surface?
	for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[map] != 255; map++)
	{
		lmScales[map].R = r_newrefdef.lightstyles[surf->styles[map]].rgb[0];
		lmScales[map].G = r_newrefdef.lightstyles[surf->styles[map]].rgb[1];
		lmScales[map].B = r_newrefdef.lightstyles[surf->styles[map]].rgb[2];
		lmScales[map].A = 1.0f;
	}
}

static qboolean
BatchMatchesSurface(const msurface_t *surf, const gl3_chain_batch_t *batch,
	qboolean flowing, float sscroll, float tscroll,
	const hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE])
{
	int i;

	if (!batch->active)
	{
		return true;
	}

	if (batch->lightmaptexturenum != surf->lightmaptexturenum)
	{
		return false;
	}

	if (batch->flowing != flowing)
	{
		return false;
	}

	if (batch->flowing && ((batch->sscroll != sscroll) || (batch->tscroll != tscroll)))
	{
		return false;
	}

	for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
	{
		if (batch->lmScales[i].R != lmScales[i].R ||
			batch->lmScales[i].G != lmScales[i].G ||
			batch->lmScales[i].B != lmScales[i].B ||
			batch->lmScales[i].A != lmScales[i].A)
		{
			return false;
		}
	}

	return true;
}

static void
DrawBatchRanges(const GLint *firsts, const GLsizei *counts, size_t draw_count)
{
#if defined(YQ2_GL3_GLES3)
	size_t i;

	for (i = 0; i < draw_count; ++i)
	{
		glDrawArrays(GL_TRIANGLE_FAN, firsts[i], counts[i]);
	}
#else
	glMultiDrawArrays(GL_TRIANGLE_FAN, firsts, counts, (GLsizei)draw_count);
#endif
}

static void
FlushBatch(const gl3image_t *image, gl3_chain_batch_t *batch)
{
	if (!batch->active || batch->vertex_count == 0 || batch->draw_count == 0)
	{
		return;
	}

	GL3_Bind(image->texnum);
	GL3_BindLightmap(batch->lightmaptexturenum);

	if (batch->flowing)
	{
		GL3_UseProgram(gl3state.si3DlmFlow.shaderProgram);
		UpdateLMscales(batch->lmScales, &gl3state.si3DlmFlow);

		if ((gl3state.uni3DData.sscroll != batch->sscroll) ||
			(gl3state.uni3DData.tscroll != batch->tscroll))
		{
			gl3state.uni3DData.sscroll = batch->sscroll;
			gl3state.uni3DData.tscroll = batch->tscroll;
			GL3_UpdateUBO3D();
		}
	}
	else
	{
		GL3_UseProgram(gl3state.si3Dlm.shaderProgram);
		UpdateLMscales(batch->lmScales, &gl3state.si3Dlm);
	}

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	if (!gl3config.useBigVBO)
	{
		glBufferData(GL_ARRAY_BUFFER, sizeof(mvtx_t) * batch->vertex_count, batch->verts, GL_STREAM_DRAW);
		DrawBatchRanges(batch->firsts, batch->counts, batch->draw_count);
	}
	else
	{
		int curOffset = gl3state.vbo3DcurOffset;
		int neededSize = (int)(batch->vertex_count * sizeof(mvtx_t));

#if !defined(YQ2_GL3_GLES3)
		GLint *drawFirsts = NULL;
#endif

		if (curOffset + neededSize > gl3state.vbo3Dsize)
		{
			glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW);
			curOffset = 0;
		}

		GLbitfield accessBits = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		void *data = glMapBufferRange(GL_ARRAY_BUFFER, curOffset, neededSize, accessBits);
		if (!data)
		{
			Com_Error(ERR_FATAL, "%s: failed to map VBO for batched draw\n", __func__);
			return;
		}

		memcpy(data, batch->verts, neededSize);
		glUnmapBuffer(GL_ARRAY_BUFFER);

#if defined(YQ2_GL3_GLES3)
		for (size_t i = 0; i < batch->draw_count; i++)
		{
			glDrawArrays(GL_TRIANGLE_FAN, batch->firsts[i] + (curOffset / sizeof(mvtx_t)), batch->counts[i]);
		}
#else
		drawFirsts = malloc(sizeof(GLint) * batch->draw_count);
		if (!drawFirsts)
		{
			Com_Error(ERR_FATAL, "%s: failed to allocate multi-draw offsets\n", __func__);
			return;
		}

		for (size_t i = 0; i < batch->draw_count; i++)
		{
			drawFirsts[i] = batch->firsts[i] + (curOffset / sizeof(mvtx_t));
		}

		DrawBatchRanges(drawFirsts, batch->counts, batch->draw_count);
		free(drawFirsts);
#endif

		gl3state.vbo3DcurOffset = curOffset + neededSize;
	}

	batch->active = false;
	batch->vertex_count = 0;
	batch->draw_count = 0;
}

static void
AppendSurfaceToBatch(const msurface_t *surf, gl3_chain_batch_t *batch)
{
	const mpoly_t *p = surf->polys;
	size_t needed_vertices = batch->vertex_count + p->numverts;
	size_t needed_draws = batch->draw_count + 1;
	size_t new_capacity;
	mvtx_t *new_verts;
	GLint *new_firsts;
	GLsizei *new_counts;

	if (batch->vertex_capacity < needed_vertices)
	{
		new_capacity = batch->vertex_capacity ? batch->vertex_capacity : p->numverts;
		while (new_capacity < needed_vertices)
		{
			new_capacity *= 2;
		}

		new_verts = realloc(batch->verts, sizeof(mvtx_t) * new_capacity);
		if (!new_verts)
		{
			Com_Error(ERR_FATAL, "%s: failed to allocate batch vertices\n", __func__);
			return;
		}

		batch->verts = new_verts;
		batch->vertex_capacity = new_capacity;
	}

	if (batch->draw_capacity < needed_draws)
	{
		new_capacity = batch->draw_capacity ? batch->draw_capacity : 4;
		while (new_capacity < needed_draws)
		{
			new_capacity *= 2;
		}

		new_firsts = realloc(batch->firsts, sizeof(GLint) * new_capacity);
		new_counts = realloc(batch->counts, sizeof(GLsizei) * new_capacity);
		if (!new_firsts || !new_counts)
		{
			Com_Error(ERR_FATAL, "%s: failed to allocate batch draw ranges\n", __func__);
			return;
		}

		batch->firsts = new_firsts;
		batch->counts = new_counts;
		batch->draw_capacity = new_capacity;
	}

	batch->firsts[batch->draw_count] = (GLint)batch->vertex_count;
	batch->counts[batch->draw_count] = (GLsizei)p->numverts;
	memcpy(&batch->verts[batch->vertex_count], p->verts, sizeof(mvtx_t) * p->numverts);
	batch->vertex_count = needed_vertices;
	batch->draw_count = needed_draws;
	batch->active = true;
}

static void
RenderBrushPoly(const entity_t *currententity, msurface_t *fa)
{
	const gl3image_t *image;

	c_brush_polys++;

	image = R_TextureAnimation(currententity, fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		GL3_Bind(image->texnum);

		GL3_EmitWaterPolys(fa);

		return;
	}
	else
	{
		GL3_Bind(image->texnum);
	}

	hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE] = {0};
	BuildLMScales(fa, lmScales);

	GL3_BindLightmap(fa->lightmaptexturenum);

	if (fa->texinfo->flags & SURF_SCROLL)
	{
		GL3_UseProgram(gl3state.si3DlmFlow.shaderProgram);
		UpdateLMscales(lmScales, &gl3state.si3DlmFlow);
		GL3_DrawGLFlowingPoly(fa);
	}
	else
	{
		GL3_UseProgram(gl3state.si3Dlm.shaderProgram);
		UpdateLMscales(lmScales, &gl3state.si3Dlm);
		GL3_DrawGLPoly(fa);
	}

	// Note: lightmap chains are gone, lightmaps are rendered together with normal texture in one pass
}

/*
 * Draw water surfaces and windows.
 * The BSP tree is waled front to back, so unwinding the chain
 * of alpha_surfaces will draw back to front, giving proper ordering.
 */
void
GL3_DrawAlphaSurfaces(void)
{
	msurface_t *s;

	/* go back to the world matrix */
	gl3state.uni3DData.transModelMat4 = gl3_identityMat4;
	GL3_UpdateUBO3D();

	glEnable(GL_BLEND);

	for (s = gl3_alpha_surfaces; s != NULL; s = s->texturechain)
	{
		GL3_Bind(s->texinfo->image->texnum);
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

		if (alpha != gl3state.uni3DData.alpha)
		{
			gl3state.uni3DData.alpha = alpha;
			GL3_UpdateUBO3D();
		}

		if (s->flags & SURF_DRAWTURB)
		{
			GL3_EmitWaterPolys(s);
		}
		else if (s->texinfo->flags & SURF_SCROLL)
		{
			GL3_UseProgram(gl3state.si3DtransFlow.shaderProgram);
			GL3_DrawGLFlowingPoly(s);
		}
		else
		{
			GL3_UseProgram(gl3state.si3Dtrans.shaderProgram);
			GL3_DrawGLPoly(s);
		}
	}

	gl3state.uni3DData.alpha = 1.0f;
	GL3_UpdateUBO3D();

	glDisable(GL_BLEND);

	gl3_alpha_surfaces = NULL;
}

static void
DrawTextureChains(const entity_t *currententity)
{
	int i;
	msurface_t *s;
	gl3image_t *image;

	c_visible_textures = 0;

	for (i = 0, image = gl3textures; i < numgl3textures; i++, image++)
	{
		gl3_chain_batch_t batch = {0};
		qboolean batch_ready = false;

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
			float sscroll = 0.0f;
			float tscroll = 0.0f;
			qboolean flowing;
			hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE];

			if (s->flags & SURF_DRAWTURB)
			{
				FlushBatch(image, &batch);
				SetLightFlags(s);
				RenderBrushPoly(currententity, s);
				continue;
			}

			SetLightFlags(s);
			flowing = (s->texinfo->flags & SURF_SCROLL) != 0;
			if (flowing)
			{
				R_FlowingScroll(&r_newrefdef, s->texinfo->flags, &sscroll, &tscroll);
			}

			BuildLMScales(s, lmScales);

			if (!batch_ready || !BatchMatchesSurface(s, &batch, flowing, sscroll, tscroll, lmScales))
			{
				FlushBatch(image, &batch);
				batch.active = true;
				batch.lightmaptexturenum = s->lightmaptexturenum;
				batch.flowing = flowing;
				batch.sscroll = sscroll;
				batch.tscroll = tscroll;
				memcpy(batch.lmScales, lmScales, sizeof(batch.lmScales));
				batch_ready = true;
			}

			AppendSurfaceToBatch(s, &batch);
		}

		FlushBatch(image, &batch);
		image->texturechain = NULL;
	}
}

static void
RenderLightmappedPoly(const entity_t *currententity, msurface_t *surf)
{
	int map;
	const gl3image_t *image = R_TextureAnimation(currententity, surf->texinfo);

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

	GL3_Bind(image->texnum);
	GL3_BindLightmap(surf->lightmaptexturenum);

	if (surf->texinfo->flags & SURF_SCROLL)
	{
		GL3_UseProgram(gl3state.si3DlmFlow.shaderProgram);
		UpdateLMscales(lmScales, &gl3state.si3DlmFlow);
		GL3_DrawGLFlowingPoly(surf);
	}
	else
	{
		GL3_UseProgram(gl3state.si3Dlm.shaderProgram);
		UpdateLMscales(lmScales, &gl3state.si3Dlm);
		GL3_DrawGLPoly(surf);
	}
}

static void
DrawInlineBModel(const entity_t *currententity, model_t *currentmodel)
{
	msurface_t *psurf;
	size_t i;

	/* calculate dynamic lighting for bmodel */
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
GL3_DrawBrushModel(entity_t *e, model_t *currentmodel)
{
	vec3_t mins, maxs;
	qboolean rotated;

	if (currentmodel->nummodelsurfaces == 0)
	{
		return;
	}

	gl3state.currenttexture = -1;

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
	hmm_mat4 oldMat = gl3state.uni3DData.transModelMat4;

	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];
	GL3_RotateForEntity(e);
	e->angles[0] = -e->angles[0];
	e->angles[2] = -e->angles[2];

	DrawInlineBModel(e, currentmodel);

	// glPopMatrix();
	gl3state.uni3DData.transModelMat4 = oldMat;
	GL3_UpdateUBO3D();

	if (r_zfix->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
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
}
