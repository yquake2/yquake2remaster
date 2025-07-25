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
 * Lightmap handling
 *
 * =======================================================================
 */

#include "header/local.h"

extern gllightmapstate_t gl_lms;

void
LM_FreeLightmapBuffers(void)
{
	for (int i=0; i<MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_buffer[i])
		{
			free(gl_lms.lightmap_buffer[i]);
		}
		gl_lms.lightmap_buffer[i] = NULL;
	}
}

static void
LM_AllocLightmapBuffer(int buffer, qboolean clean)
{
	static const unsigned int lightmap_size =
		BLOCK_WIDTH * BLOCK_HEIGHT * LIGHTMAP_BYTES;

	if (!gl_lms.lightmap_buffer[buffer])
	{
		gl_lms.lightmap_buffer[buffer] = malloc (lightmap_size);
	}
	if (!gl_lms.lightmap_buffer[buffer])
	{
		Com_Error(ERR_FATAL, "%s: Could not allocate lightmap buffer %d\n",
			__func__, buffer);
	}
	if (clean)
	{
		memset (gl_lms.lightmap_buffer[buffer], 0, lightmap_size);
	}
}

void
LM_InitBlock(void)
{
	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

	if (gl_config.multitexture)
	{
		LM_AllocLightmapBuffer(gl_lms.current_lightmap_texture, false);
	}
}

void
LM_UploadBlock(qboolean dynamic)
{
	const int texture = (dynamic)? 0 : gl_lms.current_lightmap_texture;
	const int buffer = (gl_config.multitexture)? gl_lms.current_lightmap_texture : 0;

	R_Bind(gl_state.lightmap_textures + texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (dynamic)
	{
		int i, height = 0;

		for (i = 0; i < BLOCK_WIDTH; i++)
		{
			if (gl_lms.allocated[i] > height)
			{
				height = gl_lms.allocated[i];
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BLOCK_WIDTH,
				height, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				gl_lms.lightmap_buffer[buffer]);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LIGHTMAP_FORMAT,
				BLOCK_WIDTH, BLOCK_HEIGHT,
				0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				gl_lms.lightmap_buffer[buffer]);

		if (gl_config.lightmapcopies && buffer != 0)
		{
			int i;

			// Upload to all lightmap copies
			for (i = 1; i < MAX_LIGHTMAP_COPIES; i++)
			{
				R_Bind(gl_state.lightmap_textures + (MAX_LIGHTMAPS * i) + texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LIGHTMAP_FORMAT,
					BLOCK_WIDTH, BLOCK_HEIGHT,
					0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
					gl_lms.lightmap_buffer[buffer]);
			}
		}

		if (++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS)
		{
			Com_Error(ERR_DROP,
					"%s: MAX_LIGHTMAPS exceeded\n", __func__);
		}
	}
}

/*
 * returns a texture number and the position inside it
 */
qboolean
LM_AllocBlock(int w, int h, int *x, int *y)
{
	int i, best;

	best = BLOCK_HEIGHT;

	for (i = 0; i < BLOCK_WIDTH - w; i++)
	{
		int		j, best2;

		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (gl_lms.allocated[i + j] >= best)
			{
				break;
			}

			if (gl_lms.allocated[i + j] > best2)
			{
				best2 = gl_lms.allocated[i + j];
			}
		}

		if (j == w)
		{
			/* this is a valid spot */
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
	{
		return false;
	}

	for (i = 0; i < w; i++)
	{
		gl_lms.allocated[*x + i] = best + h;
	}

	return true;
}

static void
LM_BuildPolygonFromSurface(model_t *currentmodel, msurface_t *fa)
{
	medge_t *pedges, *r_pedge;
	int i, lnumverts;
	const float *vec;
	mpoly_t *poly;
	vec3_t total;
	vec3_t normal;

	/* reconstruct the polygon */
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;

	VectorClear(total);

	/* draw texture */
	poly = Hunk_Alloc(sizeof(mpoly_t) +
		   (lnumverts - 4) * sizeof(mvtx_t));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	VectorCopy(fa->plane->normal, normal);

	if(fa->flags & SURF_PLANEBACK)
	{
		// if for some reason the normal sticks to the back of the plane, invert it
		// so it's usable for the shader
		for (i=0; i<3; ++i)
		{
			normal[i] = -normal[i];
		}
	}

	for (i = 0; i < lnumverts; i++)
	{
		mvtx_t* vert;
		float s, t;
		int lindex;

		vert = &poly->verts[i];

		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		if (fa->texinfo->flags & SURF_N64_UV)
		{
			s *= 0.5;
			t *= 0.5;
		}

		VectorAdd(total, vec, total);
		VectorCopy(vec, vert->pos);
		vert->texCoord[0] = s;
		vert->texCoord[1] = t;

		/* lightmap texture coordinates */
		s = DotProduct(vec, fa->lmvecs[0]) + fa->lmvecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * (1 << fa->lmshift);
		s += (1 << fa->lmshift) * 0.5;
		s /= BLOCK_WIDTH * (1 << fa->lmshift);

		t = DotProduct(vec, fa->lmvecs[1]) + fa->lmvecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * (1 << fa->lmshift);
		t += (1 << fa->lmshift) * 0.5;
		t /= BLOCK_HEIGHT * (1 << fa->lmshift);

		vert->lmTexCoord[0] = s;
		vert->lmTexCoord[1] = t;

		VectorCopy(normal, vert->normal);
		vert->lightFlags = 0;
	}
}

static void
LM_CreateSurfaceLightmap(msurface_t *surf)
{
	int smax, tmax, buffer;
	byte *base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
	{
		return;
	}

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;

	if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		LM_UploadBlock(false);
		LM_InitBlock();

		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			Com_Error(ERR_FATAL,
				"%s: Consecutive calls to LM_AllocBlock(%d,%d) failed\n",
					__func__, smax, tmax);
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;
	buffer = (gl_config.multitexture)? surf->lightmaptexturenum : 0;

	base = gl_lms.lightmap_buffer[buffer];
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState(surf, &r_newrefdef);
	R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES,
		&r_newrefdef, r_modulate->value, r_framecount, gammatable,
		gl_state.minlight_set ? minlight : NULL);
}

void
LM_CreateLightmapsPoligon(model_t *currentmodel, msurface_t *fa)
{
	/* create lightmaps and polygons */
	if (!(fa->texinfo->flags & (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)))
	{
		LM_CreateSurfaceLightmap(fa);
	}

	if (!(fa->texinfo->flags & SURF_WARP))
	{
		LM_BuildPolygonFromSurface(currentmodel, fa);
	}
}

void
LM_BeginBuildingLightmaps(model_t *m)
{
	static lightstyle_t lightstyles[MAX_LIGHTSTYLES];
	int i;

	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));
	LM_FreeLightmapBuffers();

	r_framecount = 1; /* no dlightcache */

	/* setup the base lightstyles so the lightmaps
	   won't have to be regenerated the first time
	   they're seen */
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}

	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	if (gl_config.multitexture)
	{
		LM_AllocLightmapBuffer(gl_lms.current_lightmap_texture, false);
		return;
	}

	// dynamic lightmap for classic rendering path (no multitexture)
	LM_AllocLightmapBuffer(0, true);

	/* initialize the dynamic lightmap texture */
	R_Bind(gl_state.lightmap_textures + 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LIGHTMAP_FORMAT,
			BLOCK_WIDTH, BLOCK_HEIGHT,
			0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer[0]);
}

void
LM_EndBuildingLightmaps(void)
{
	LM_UploadBlock(false);
}

