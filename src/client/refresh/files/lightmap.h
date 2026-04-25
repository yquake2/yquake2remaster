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
 * Vulcan/GL1: Lightmap shared code
 *
 * =======================================================================
 */

#ifndef __LIGHTMAP_H__
#define __LIGHTMAP_H__

#include "../ref_shared.h"

/* Shared lightmaps logic */
#define MAX_LIGHTMAPS 256

typedef struct
{
	int current_lightmap_texture;

	msurface_t *lightmap_surfaces[MAX_LIGHTMAPS];

	int allocated[BLOCK_WIDTH];

	/* the lightmap texture data needs to be kept in
	   main memory so texsubimage can update properly,
	   used only first except OpenGL 1 Multitexture mode */
	byte *lightmap_buffer[MAX_LIGHTMAPS];
} r_lightmapstate_t;

extern r_lightmapstate_t r_lms;

void LM_InitBlock(qboolean multitexture);
void LM_FreeLightmapBuffers(void);
void LM_AllocLightmapBuffer(int buffer, qboolean clean);
qboolean LM_AllocBlock(int w, int h, int *x, int *y);
size_t LM_GetMaxHeight(void);

#endif
