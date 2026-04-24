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

void
LM_UploadBlock(qboolean dynamic)
{
	const int buffer = (gl_config.multitexture)? r_lms.current_lightmap_texture : 0;
	int texture;

	texture = (dynamic)? 0 : r_lms.current_lightmap_texture;

	R_Bind(gl_state.lightmap_textures + texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (dynamic)
	{
		size_t height;

		height = LM_GetMaxHeight();

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BLOCK_WIDTH,
				height, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				r_lms.lightmap_buffer[buffer]);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LIGHTMAP_FORMAT,
				BLOCK_WIDTH, BLOCK_HEIGHT,
				0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				r_lms.lightmap_buffer[buffer]);

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
					r_lms.lightmap_buffer[buffer]);
			}
		}

		if (++r_lms.current_lightmap_texture == MAX_LIGHTMAPS)
		{
			Com_Error(ERR_DROP,
					"%s: MAX_LIGHTMAPS exceeded\n", __func__);
			return;
		}
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
		LM_InitBlock(gl_config.multitexture);

		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			Com_Error(ERR_FATAL,
				"%s: Consecutive calls to LM_AllocBlock(%d,%d) failed\n",
					__func__, smax, tmax);
			return;
		}
	}

	surf->lightmaptexturenum = r_lms.current_lightmap_texture;
	buffer = (gl_config.multitexture)? surf->lightmaptexturenum : 0;

	base = r_lms.lightmap_buffer[buffer];
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
		R_BuildLMPolygonFromSurface(currentmodel, fa, BLOCK_WIDTH, BLOCK_HEIGHT,
			fa->texinfo->image->width, fa->texinfo->image->height);
	}
}

void
LM_BeginBuildingLightmaps(model_t *m)
{
	static lightstyle_t lightstyles[MAX_LIGHTSTYLES];
	int i;

	LM_InitBlock(false);
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

	r_lms.current_lightmap_texture = 1;

	if (gl_config.multitexture)
	{
		LM_AllocLightmapBuffer(r_lms.current_lightmap_texture, false);
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
			r_lms.lightmap_buffer[0]);
}

void
LM_EndBuildingLightmaps(void)
{
	LM_UploadBlock(false);
}

