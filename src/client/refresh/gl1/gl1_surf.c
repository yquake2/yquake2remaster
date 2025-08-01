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
 * Surface generation and drawing
 *
 * =======================================================================
 */

#include <assert.h>

#include "header/local.h"

typedef struct
{
	int top, bottom, left, right;
} lmrect_t;

int c_visible_lightmaps;
int c_visible_textures;
static vec3_t modelorg; /* relative to viewpoint */
msurface_t *r_alpha_surfaces;

gllightmapstate_t gl_lms;
extern int cur_lm_copy;
byte minlight[256];

void LM_InitBlock(void);
void LM_UploadBlock(qboolean dynamic);
qboolean LM_AllocBlock(int w, int h, int *x, int *y);

static void
R_DrawGLPoly(msurface_t *fa)
{
	int i, nv;
	mvtx_t *v;
	float sscroll, tscroll;

	v = fa->polys->verts;
	nv = fa->polys->numverts;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	R_SetBufferIndices(GL_TRIANGLE_FAN, nv);

	for ( i = 0; i < nv; i++, v ++)
	{
		GLBUFFER_VERTEX( v->pos[0], v->pos[1], v->pos[2] )
		GLBUFFER_SINGLETEX( v->texCoord[0] + sscroll, v->texCoord[1] + tscroll )
	}
}

static void
R_DrawTriangleOutlines(void)
{
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

		for (surf = gl_lms.lightmap_surfaces[i];
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
						vtx[0+k] = p->verts[0    ].pos[k];
						vtx[3+k] = p->verts[j - 1].pos[k];
						vtx[6+k] = p->verts[j    ].pos[k];
						vtx[9+k] = p->verts[0    ].pos[k];
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
}

static void
R_DrawGLPolyChain(mpoly_t *p, float soffset, float toffset)
{
	if ((soffset == 0) && (toffset == 0))
	{
		for ( ; p != 0; p = p->chain)
		{
			mvtx_t* vert = p->verts;

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			glVertexPointer(3, GL_FLOAT, sizeof(mvtx_t), vert->pos);
			glTexCoordPointer(2, GL_FLOAT, sizeof(mvtx_t), vert->lmTexCoord);
			glDrawArrays(GL_TRIANGLE_FAN, 0, p->numverts);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
	else
	{
		// workaround for lack of VLAs (=> our workaround uses alloca() which is bad in loops)
#ifdef _MSC_VER
		int maxNumVerts = 0;
		for (mpoly_t* tmp = p; tmp; tmp = tmp->chain)
		{
			if ( tmp->numverts > maxNumVerts )
				maxNumVerts = tmp->numverts;
		}

		YQ2_VLA( GLfloat, tex, 2 * maxNumVerts );
#endif

		for ( ; p != 0; p = p->chain)
		{
			mvtx_t* vert;
			int j;

			vert = p->verts;
#ifndef _MSC_VER // we have real VLAs, so it's safe to use one in this loop
			YQ2_VLA(GLfloat, tex, 2*p->numverts);
#endif

			unsigned int index_tex = 0;

			for (j = 0; j < p->numverts; j++, vert++)
			{
				tex[index_tex++] = vert->lmTexCoord[0] - soffset;
				tex[index_tex++] = vert->lmTexCoord[1] - toffset;
			}

			vert = p->verts;

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			glVertexPointer(3, GL_FLOAT, sizeof(mvtx_t), vert->pos);
			glTexCoordPointer(2, GL_FLOAT, 0, tex);
			glDrawArrays(GL_TRIANGLE_FAN, 0, p->numverts);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		YQ2_VLAFREE(tex);
	}
}

/*
 * This routine takes all the given light mapped surfaces
 * in the world and blends them into the framebuffer.
 */
static void
R_BlendLightmaps(const model_t *currentmodel)
{
	int i;
	msurface_t *surf;

	/* don't bother if we're set to fullbright or multitexture is enabled */
	if (gl_config.multitexture || r_fullbright->value || !r_worldmodel->lightdata)
	{
		return;
	}

	/* don't bother writing Z */
	glDepthMask(GL_FALSE);

	/* set the appropriate blending mode unless
	   we're only looking at the lightmaps. */
	if (!gl_lightmap->value)
	{
		glEnable(GL_BLEND);

		if (gl1_saturatelighting->value)
		{
			glBlendFunc(GL_ONE, GL_ONE);
		}
		else
		{
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}
	}

	if (currentmodel == r_worldmodel)
	{
		c_visible_lightmaps = 0;
	}

	/* render static lightmaps first */
	for (i = 1; i < MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_surfaces[i])
		{
			if (currentmodel == r_worldmodel)
			{
				c_visible_lightmaps++;
			}

			R_Bind(gl_state.lightmap_textures + i);

			for (surf = gl_lms.lightmap_surfaces[i];
				 surf != 0;
				 surf = surf->lightmapchain)
			{
				if (surf->polys)
				{
					// Apply overbright bits to the static lightmaps
					if (gl1_overbrightbits->value)
					{
						R_TexEnv(GL_COMBINE);
						glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, gl1_overbrightbits->value);
					}

					R_DrawGLPolyChain(surf->polys, 0, 0);
				}
			}
		}
	}

	/* render dynamic lightmaps */
	if (r_dynamic->value)
	{
		msurface_t *newdrawsurf;

		LM_InitBlock();

		R_Bind(gl_state.lightmap_textures + 0);

		if (currentmodel == r_worldmodel)
		{
			c_visible_lightmaps++;
		}

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for (surf = gl_lms.lightmap_surfaces[0];
			 surf != 0;
			 surf = surf->lightmapchain)
		{
			int smax, tmax;
			byte *base;

			smax = (surf->extents[0] >> surf->lmshift) + 1;
			tmax = (surf->extents[1] >> surf->lmshift) + 1;

			if (LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
			{
				base = gl_lms.lightmap_buffer[0];
				base += (surf->dlight_t * BLOCK_WIDTH +
						surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES,
					&r_newrefdef, r_modulate->value, r_framecount, gammatable,
					gl_state.minlight_set ? minlight : NULL);
			}
			else
			{
				msurface_t *drawsurf;

				/* upload what we have so far */
				LM_UploadBlock(true);

				/* draw all surfaces that use this lightmap */
				for (drawsurf = newdrawsurf;
					 drawsurf != surf;
					 drawsurf = drawsurf->lightmapchain)
				{
					if (drawsurf->polys)
					{
						// Apply overbright bits to the dynamic lightmaps
						if (gl1_overbrightbits->value)
						{
							R_TexEnv(GL_COMBINE);
							glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, gl1_overbrightbits->value);
						}

						R_DrawGLPolyChain(drawsurf->polys,
								(drawsurf->light_s - drawsurf->dlight_s) * (float)(1.0 / BLOCK_WIDTH),
								(drawsurf->light_t - drawsurf->dlight_t) * (float)(1.0 / BLOCK_HEIGHT));
					}
				}

				newdrawsurf = drawsurf;

				/* clear the block */
				LM_InitBlock();

				/* try uploading the block now */
				if (!LM_AllocBlock(smax, tmax, &surf->dlight_s, &surf->dlight_t))
				{
					Com_Error(ERR_FATAL,
							"%s: Consecutive calls to LM_AllocBlock(%d,%d) failed (dynamic)\n",
							__func__, smax, tmax);
				}

				base = gl_lms.lightmap_buffer[0];
				base += (surf->dlight_t * BLOCK_WIDTH +
						surf->dlight_s) * LIGHTMAP_BYTES;

				R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES,
					&r_newrefdef, r_modulate->value, r_framecount, gammatable,
					gl_state.minlight_set ? minlight : NULL);
			}
		}

		/* draw remainder of dynamic lightmaps that haven't been uploaded yet */
		if (newdrawsurf)
		{
			LM_UploadBlock(true);
		}

		for (surf = newdrawsurf; surf != 0; surf = surf->lightmapchain)
		{
			if (surf->polys)
			{
				// Apply overbright bits to the remainder lightmaps
				if (gl1_overbrightbits->value)
				{
					R_TexEnv(GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, gl1_overbrightbits->value);
				}

				R_DrawGLPolyChain(surf->polys,
						(surf->light_s - surf->dlight_s) * (float)(1.0 / BLOCK_WIDTH),
						(surf->light_t - surf->dlight_t) * (float)(1.0 / BLOCK_HEIGHT));
			}
		}
	}

	/* restore state */
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}

static void
R_RenderBrushPoly(msurface_t *fa)
{
	qboolean is_dynamic = false;
	int maps;

	c_brush_polys++;

	if (fa->flags & SURF_DRAWTURB)
	{
		R_EmitWaterPolys(fa);
		return;
	}

	R_DrawGLPoly(fa);

	if (gl_config.multitexture)
	{
		return;	// lighting already done at this point for mtex
	}

	/* check for lightmap modification */
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (r_newrefdef.lightstyles[fa->styles[maps]].white !=
			fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	/* dynamic this frame or dynamic previously */
	if (fa->dlightframe == r_framecount)
	{
	dynamic:

		if (r_dynamic->value)
		{
			if (!(fa->texinfo->flags &
				  (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if (maps < MAXLIGHTMAPS &&
			((fa->styles[maps] >= 32) ||
			 (fa->styles[maps] == 0)) &&
			  (fa->dlightframe != r_framecount))
		{
			int smax, tmax, size;
			byte *temp;

			smax = (fa->extents[0] >> fa->lmshift) + 1;
			tmax = (fa->extents[1] >> fa->lmshift) + 1;

			size = smax * tmax * LIGHTMAP_BYTES;
			temp = R_GetTemporaryLMBuffer(size);

			R_BuildLightMap(fa, temp, smax * 4,
				&r_newrefdef, r_modulate->value, r_framecount, gammatable,
				gl_state.minlight_set ? minlight : NULL);
			R_SetCacheState(fa, &r_newrefdef);

			R_Bind(gl_state.lightmap_textures + fa->lightmaptexturenum);

			glTexSubImage2D(GL_TEXTURE_2D, 0, fa->light_s, fa->light_t,
					smax, tmax, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, temp);

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}

/*
 * Draw water surfaces and windows.
 * The BSP tree is waled front to back, so unwinding the chain
 * of alpha_surfaces will draw back to front, giving proper ordering.
 */
void
R_DrawAlphaSurfaces(void)
{
	msurface_t *s;
	float alpha;

	/* go back to the world matrix */
	glLoadMatrixf(r_world_matrix);

	glEnable(GL_BLEND);
	R_TexEnv(GL_MODULATE);

	for (s = r_alpha_surfaces; s; s = s->texturechain)
	{
		c_brush_polys++;

		if (s->texinfo->flags & SURF_TRANS33)
		{
			alpha = 0.33f;
		}
		else if (s->texinfo->flags & SURF_TRANS66)
		{
			alpha = 0.66f;
		}
		else
		{
			alpha = 1.0f;
		}

		R_UpdateGLBuffer(buf_alpha, s->texinfo->image->texnum, 0, 0, alpha);

		if (s->flags & SURF_DRAWTURB)
		{
			R_EmitWaterPolys(s);
		}
		else
		{
			R_DrawGLPoly(s);
		}
	}
	R_ApplyGLBuffer();	// Flush the last batched array

	R_TexEnv(GL_REPLACE);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);

	r_alpha_surfaces = NULL;
}

static void
R_RenderLightmappedPoly(msurface_t *surf)
{
	const int nv = surf->polys->numverts;
	float sscroll, tscroll;
	mvtx_t* vert;
	int i;

	c_brush_polys++;
	vert = surf->polys->verts;

	R_FlowingScroll(&r_newrefdef, surf->texinfo->flags, &sscroll, &tscroll);

	R_SetBufferIndices(GL_TRIANGLE_FAN, nv);

	for (i = 0; i < nv; i++, vert++)
	{
		GLBUFFER_VERTEX( vert->pos[0], vert->pos[1], vert->pos[2] )
		GLBUFFER_MULTITEX( vert->texCoord[0] + sscroll, vert->texCoord[1] + tscroll,
			vert->lmTexCoord[0], vert->lmTexCoord[1] )
	}
}

/* Add "adding" area to "obj" */
static void
R_JoinAreas(lmrect_t *adding, lmrect_t *obj)
{
	if (adding->top < obj->top)
	{
		obj->top = adding->top;
	}
	if (adding->bottom > obj->bottom)
	{
		obj->bottom = adding->bottom;
	}
	if (adding->left < obj->left)
	{
		obj->left = adding->left;
	}
	if (adding->right > obj->right)
	{
		obj->right = adding->right;
	}
}

/* Upload dynamic lights to each lightmap texture (multitexture path only) */
static void
R_RegenAllLightmaps()
{
	static lmrect_t lmchange[MAX_LIGHTMAPS][MAX_LIGHTMAP_COPIES];
	static qboolean altered[MAX_LIGHTMAPS][MAX_LIGHTMAP_COPIES];

	int i, lmtex;
#ifndef YQ2_GL1_GLES
	qboolean pixelstore_set = false;
#endif

	if ( !gl_config.multitexture || r_fullbright->value || !r_dynamic->value )
	{
		return;
	}

	if (gl_config.lightmapcopies)
	{
		cur_lm_copy = (cur_lm_copy + 1) % MAX_LIGHTMAP_COPIES;	// select the next lightmap copy
		lmtex = MAX_LIGHTMAPS * cur_lm_copy;	// ...and its corresponding texture
	}
	else
	{
		lmtex = 0;
	}

	for (i = 1; i < MAX_LIGHTMAPS; i++)
	{
		lmrect_t current, best;
		msurface_t *surf;
		byte *base;
		qboolean affected_lightmap;

		if (!gl_lms.lightmap_surfaces[i] || !gl_lms.lightmap_buffer[i])
		{
			continue;
		}

		affected_lightmap = false;
		best.top = BLOCK_HEIGHT;
		best.left = BLOCK_WIDTH;
		best.bottom = best.right = 0;

		for (surf = gl_lms.lightmap_surfaces[i];
			 surf != 0;
			 surf = surf->lightmapchain)
		{
			int map;

			if (surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
			{
				continue;
			}

			// Any dynamic lights on this surface?
			for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
			{
				if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
				{
					goto dynamic_surf;
				}
			}

			// Surface is considered to have dynamic lights if it had them in the previous frame
			if ( surf->dlightframe != r_framecount && !surf->dirty_lightmap )
			{
				continue;	// no dynamic lights affect this surface in this frame
			}

dynamic_surf:
			affected_lightmap = true;

			current.left = surf->light_s;
			current.right = surf->light_s + (surf->extents[0] >> surf->lmshift) + 1;	// + smax
			current.top = surf->light_t;
			current.bottom = surf->light_t + (surf->extents[1] >> surf->lmshift) + 1;	// + tmax

			base = gl_lms.lightmap_buffer[i];
			base += (current.top * BLOCK_WIDTH + current.left) * LIGHTMAP_BYTES;

			R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES,
				&r_newrefdef, r_modulate->value, r_framecount, gammatable,
				gl_state.minlight_set ? minlight : NULL);

			if (surf->dlightframe != r_framecount)
			{
				for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
				{
					if ( (surf->styles[map] >= 32) || (surf->styles[map] == 0) )
					{
						R_SetCacheState(surf, &r_newrefdef);
						break;
					}
				}
			}

			surf->dirty_lightmap = (surf->dlightframe == r_framecount);
			R_JoinAreas(&current, &best);
		}

		if (!gl_config.lightmapcopies && !affected_lightmap)
		{
			continue;
		}

		if (gl_config.lightmapcopies)
		{
			// Add all the changes that have happened in the last few frames,
			// at least just for consistency between them.
			qboolean apply_changes = affected_lightmap;
			current = best;		// save state for next frames... +

			for (int k = 0; k < MAX_LIGHTMAP_COPIES; k++)
			{
				if (altered[i][k])
				{
					apply_changes = true;
					R_JoinAreas(&lmchange[i][k], &best);
				}
			}

			altered[i][cur_lm_copy] = affected_lightmap;
			if (affected_lightmap)
			{
				lmchange[i][cur_lm_copy] = current;	// + ...here
			}

			if (!apply_changes)
			{
				continue;
			}
		}

#ifndef YQ2_GL1_GLES
		if (!pixelstore_set)
		{
			glPixelStorei(GL_UNPACK_ROW_LENGTH, BLOCK_WIDTH);
			pixelstore_set = true;
		}
#endif

		// upload changes
		base = gl_lms.lightmap_buffer[i];

#ifdef YQ2_GL1_GLES
		base += (best.top * BLOCK_WIDTH) * LIGHTMAP_BYTES;

		R_Bind(gl_state.lightmap_textures + i + lmtex);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, best.top,
			BLOCK_WIDTH, best.bottom - best.top,
			GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, base);
#else
		base += (best.top * BLOCK_WIDTH + best.left) * LIGHTMAP_BYTES;

		R_Bind(gl_state.lightmap_textures + i + lmtex);

		glTexSubImage2D(GL_TEXTURE_2D, 0, best.left, best.top,
			best.right - best.left, best.bottom - best.top,
			GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, base);
#endif
	}

#ifndef YQ2_GL1_GLES
	if (pixelstore_set)
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
#endif
}

static void
R_DrawTextureChains(void)
{
	int i;
	msurface_t *s;
	image_t *image;

	c_visible_textures = 0;

	if (!gl_config.multitexture)	// classic path
	{
		for (i = 0, image = gltextures; i < numgltextures; i++, image++)
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
				R_UpdateGLBuffer(buf_singletex, image->texnum, 0, s->flags, 1);
				R_RenderBrushPoly(s);
			}

			image->texturechain = NULL;
		}
		R_ApplyGLBuffer();	// Flush the last batched array
	}
	else	// multitexture
	{
		for (i = 0, image = gltextures; i < numgltextures; i++, image++)
		{
			if (!image->registration_sequence || !image->texturechain)
			{
				continue;
			}

			c_visible_textures++;

			for (s = image->texturechain; s; s = s->texturechain)
			{
				if (!(s->flags & SURF_DRAWTURB))
				{
					R_UpdateGLBuffer(buf_mtex, image->texnum, s->lightmaptexturenum, 0, 1);
					R_RenderLightmappedPoly(s);
				}
			}
		}
		R_ApplyGLBuffer();

		R_EnableMultitexture(false);	// force disabling, SURF_DRAWTURB surfaces may not exist

		for (i = 0, image = gltextures; i < numgltextures; i++, image++)
		{
			if (!image->registration_sequence || !image->texturechain)
			{
				continue;
			}

			for (s = image->texturechain; s; s = s->texturechain)
			{
				if (s->flags & SURF_DRAWTURB)
				{
					R_UpdateGLBuffer(buf_singletex, image->texnum, 0, s->flags, 1);
					R_RenderBrushPoly(s);
				}
			}

			image->texturechain = NULL;
		}
		R_ApplyGLBuffer();
	}
}

static void
R_DrawInlineBModel(const entity_t *currententity, const model_t *currentmodel)
{
	int i;
	msurface_t *psurf;
	image_t *image;

	/* calculate dynamic lighting for bmodel */
	if (!gl_config.multitexture && !r_flashblend->value)
	{
		R_PushDlights(&r_newrefdef, currentmodel->nodes + currentmodel->firstnode,
			r_dlightframecount, currentmodel->surfaces);
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, 0.25);
		R_TexEnv(GL_MODULATE);
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
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else
			{
				image = R_TextureAnimation(currententity, psurf->texinfo);

				if (gl_config.multitexture && !(psurf->flags & SURF_DRAWTURB))
				{
					// Dynamic lighting already generated in R_GetBrushesLighting()
					R_UpdateGLBuffer(buf_mtex, image->texnum, psurf->lightmaptexturenum, 0, 1);
					R_RenderLightmappedPoly(psurf);
				}
				else
				{
					R_UpdateGLBuffer(buf_singletex, image->texnum, 0, psurf->flags, 1);
					R_RenderBrushPoly(psurf);
				}
			}
		}
	}
	R_ApplyGLBuffer();

	if (!(currententity->flags & RF_TRANSLUCENT))
	{
		R_BlendLightmaps(currentmodel);
	}
	else
	{
		glDisable(GL_BLEND);
		glColor4f(1, 1, 1, 1);
		R_TexEnv(GL_REPLACE);
	}
}

void
R_DrawBrushModel(entity_t *currententity, const model_t *currentmodel)
{
	vec3_t mins, maxs;
	qboolean rotated;

	if (currentmodel->nummodelsurfaces == 0)
	{
		return;
	}

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		int	i;

		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = currententity->origin[i] - currentmodel->radius;
			maxs[i] = currententity->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(currententity->origin, currentmodel->mins, mins);
		VectorAdd(currententity->origin, currentmodel->maxs, maxs);
	}

	if (r_cull->value && R_CullBox(mins, maxs, frustum))
	{
		return;
	}

	if (gl_zfix->value)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	glColor4f(1, 1, 1, 1);
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract(r_newrefdef.vieworg, currententity->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(currententity->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	glPushMatrix();
	currententity->angles[0] = -currententity->angles[0];
	currententity->angles[2] = -currententity->angles[2];
	R_RotateForEntity(currententity);
	currententity->angles[0] = -currententity->angles[0];
	currententity->angles[2] = -currententity->angles[2];

	if (gl_lightmap->value)
	{
		R_TexEnv(GL_REPLACE);
	}
	else
	{
		R_TexEnv(GL_MODULATE);
	}

	R_DrawInlineBModel(currententity, currentmodel);

	glPopMatrix();

	if (gl_zfix->value)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

static void
R_RecursiveWorldNode(entity_t *currententity, mnode_t *node)
{
	int c, side, sidebit;
	cplane_t *plane;
	msurface_t *surf;
	mleaf_t *pleaf;
	float dot;
	image_t *image;

	if (node->contents == CONTENTS_SOLID)
	{
		return; /* solid */
	}

	if (node->visframe != r_visframecount)
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
		msurface_t	**mark;

		pleaf = (mleaf_t *)node;

		/* check for door connected areas */
		if (!R_AreaVisible(r_newrefdef.areabits, pleaf))
			return;	// not visible

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
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
	R_RecursiveWorldNode(currententity, node->children[side]);

	if ((node->numsurfaces + node->firstsurface) > r_worldmodel->numsurfaces)
	{
		Com_Printf("Broken node firstsurface\n");
		return;
	}

	/* draw stuff */
	for (c = node->numsurfaces,
		 surf = r_worldmodel->surfaces + node->firstsurface;
		 c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
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
			RE_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & SURF_TRANSPARENT)
		{
			/* add to the translucent chain */
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
			r_alpha_surfaces->texinfo->image = R_TextureAnimation(currententity, surf->texinfo);
		}
		else if (surf->texinfo->flags & SURF_NODRAW)
		{
			/* Surface should be skipped */
			continue;
		}
		else
		{
			/* the polygon is visible, so add it to the texture sorted chain */
			image = R_TextureAnimation(currententity, surf->texinfo);
			surf->texturechain = image->texturechain;
			image->texturechain = surf;

			if (gl_config.multitexture && !(surf->texinfo->flags & SURF_WARP))	// needed for R_RegenAllLightmaps()
			{
				surf->lightmapchain = gl_lms.lightmap_surfaces[surf->lightmaptexturenum];
				gl_lms.lightmap_surfaces[surf->lightmaptexturenum] = surf;
			}
		}
	}

	/* recurse down the back side */
	R_RecursiveWorldNode(currententity, node->children[!side]);
}

/*
 * This is for the RegenAllLightmaps() function to be able to regenerate
 * lighting not only for the world, but also for the brushes in the entity list.
 * Logic extracted from R_DrawBrushModel() & R_DrawInlineBModel().
 */
static void
R_GetBrushesLighting(void)
{
	int i, k;
	vec3_t mins, maxs;
	msurface_t *surf;
	cplane_t *pplane;
	float dot;

	if (!gl_config.multitexture || !r_drawentities->value || r_flashblend->value)
	{
		return;
	}

	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_BEAM)
		{
			continue;
		}

		const model_t *currentmodel = currententity->model;

		if (!currentmodel || currentmodel->type != mod_brush || currentmodel->nummodelsurfaces == 0)
		{
			continue;
		}

		// from R_DrawBrushModel()
		if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
		{
			for (k = 0; k < 3; k++)
			{
				mins[k] = currententity->origin[k] - currentmodel->radius;
				maxs[k] = currententity->origin[k] + currentmodel->radius;
			}
		}
		else
		{
			VectorAdd(currententity->origin, currentmodel->mins, mins);
			VectorAdd(currententity->origin, currentmodel->maxs, maxs);
		}

		if (r_cull->value && R_CullBox(mins, maxs, frustum))
		{
			continue;
		}

		// from R_DrawInlineBModel()
		R_PushDlights(&r_newrefdef, currentmodel->nodes + currentmodel->firstnode,
			r_dlightframecount, currentmodel->surfaces);

		surf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

		for (k = 0; k < currentmodel->nummodelsurfaces; k++, surf++)
		{
			if (surf->texinfo->flags & (SURF_TRANSPARENT | SURF_WARP)
				|| surf->flags & SURF_DRAWTURB || surf->lmchain_frame == r_framecount)
			{
				continue;	// either not affected by light, or already in the chain
			}

			// find which side of the node we are on
			pplane = surf->plane;
			dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

			if (((surf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
				(!(surf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
			{
				surf->lmchain_frame = r_framecount;	// don't add this twice to the chain
				surf->lightmapchain = gl_lms.lightmap_surfaces[surf->lightmaptexturenum];
				gl_lms.lightmap_surfaces[surf->lightmaptexturenum] = surf;
			}
		}
	}
}

void
R_DrawWorld(void)
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

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	glColor4f(1, 1, 1, 1);
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	RE_ClearSkyBox();
	R_RecursiveWorldNode(&ent, r_worldmodel->nodes);
	R_GetBrushesLighting();
	R_RegenAllLightmaps();
	R_DrawTextureChains();
	R_BlendLightmaps(r_worldmodel);
	R_DrawSkyBox();
	R_DrawTriangleOutlines();
}

/*
 * Mark the leaves and nodes that are
 * in the PVS for the current cluster
 */
void
R_MarkLeaves(void)
{
	const byte *vis;
	byte *fatvis = NULL;
	mnode_t *node;
	int i;
	mleaf_t *leaf;

	if ((r_oldviewcluster == r_viewcluster) &&
		(r_oldviewcluster2 == r_viewcluster2) &&
		!r_novis->value &&
		(r_viewcluster != -1))
	{
		return;
	}

	/* development aid to let you run around
	   and see exactly where the pvs ends */
	if (r_lockpvs->value)
	{
		return;
	}

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || (r_viewcluster == -1) || !r_worldmodel->vis)
	{
		/* mark everything */
		for (i = 0; i < r_worldmodel->numleafs; i++)
		{
			r_worldmodel->leafs[i].visframe = r_visframecount;
		}

		for (i = 0; i < r_worldmodel->numnodes; i++)
		{
			r_worldmodel->nodes[i].visframe = r_visframecount;
		}

		return;
	}

	vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel);

	/* may have to combine two clusters because of solid water boundaries */
	if (r_viewcluster2 != r_viewcluster)
	{
		int c;

		fatvis = malloc(((r_worldmodel->numleafs + 31) / 32) * sizeof(int));
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;

		for (i = 0; i < c; i++)
		{
			((int *)fatvis)[i] |= ((int *)vis)[i];
		}

		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs;
		 i < r_worldmodel->numleafs;
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
				if (node->visframe == r_visframecount)
				{
					break;
				}

				node->visframe = r_visframecount;
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
