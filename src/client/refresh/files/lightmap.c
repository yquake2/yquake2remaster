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

#include "lightmap.h"

r_lightmapstate_t r_lms = {0};

void
LM_FreeLightmapBuffers(void)
{
	size_t i;

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (r_lms.lightmap_buffer[i])
		{
			free(r_lms.lightmap_buffer[i]);
		}

		r_lms.lightmap_buffer[i] = NULL;
	}
}

void
LM_AllocLightmapBuffer(int buffer, qboolean clean)
{
	static const size_t lightmap_size =
		BLOCK_WIDTH * BLOCK_HEIGHT * LIGHTMAP_BYTES;

	if (!r_lms.lightmap_buffer[buffer])
	{
		r_lms.lightmap_buffer[buffer] = malloc(lightmap_size);
	}

	if (!r_lms.lightmap_buffer[buffer])
	{
		Com_Error(ERR_FATAL, "%s: Could not allocate lightmap buffer %d\n",
			__func__, buffer);
		return;
	}

	if (clean)
	{
		memset (r_lms.lightmap_buffer[buffer], 0, lightmap_size);
	}
}

void
LM_InitBlock(qboolean multitexture)
{
	memset(r_lms.allocated, 0, sizeof(r_lms.allocated));

	if (multitexture)
	{
		LM_AllocLightmapBuffer(r_lms.current_lightmap_texture, false);
	}
}

size_t
LM_GetMaxHeight(void)
{
	size_t i, height = 0;

	for (i = 0; i < BLOCK_WIDTH; i++)
	{
		if (r_lms.allocated[i] > height)
		{
			height = r_lms.allocated[i];
		}
	}

	return height;
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
		int j, best2;

		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (r_lms.allocated[i + j] >= best)
			{
				break;
			}

			if (r_lms.allocated[i + j] > best2)
			{
				best2 = r_lms.allocated[i + j];
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
		r_lms.allocated[*x + i] = best + h;
	}

	return true;
}
