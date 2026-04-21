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
 * Allocate all the little status bar objects into a single texture
 * to crutch up inefficient hardware / drivers.
 *
 * =======================================================================
 */

#include "header/local.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "../files/stb_rect_pack.h"

static stbrp_context scrap_packer[MAX_SCRAPS];
static stbrp_node scrap_nodes[MAX_SCRAPS][SCRAP_WIDTH * 2];

byte scrap_texels[MAX_SCRAPS][SCRAP_WIDTH * SCRAP_HEIGHT];
qboolean scrap_dirty;

/* returns a texture number and the position inside it */
int
Scrap_AllocBlock(int w, int h, int *x, int *y)
{
	int texnum;

	/* add an empty border to all sides */
	w += 2;
	h += 2;

	for (texnum = 0; texnum < MAX_SCRAPS; texnum++)
	{
		stbrp_rect rect;
		rect.w = w;
		rect.h = h;
		rect.x = 0;
		rect.y = 0;
		rect.was_packed = 0;

		if (stbrp_pack_rects(&scrap_packer[texnum], &rect, 1))
		{
			*x = rect.x + 1; /* skip border */
			*y = rect.y + 1;
			scrap_dirty = true;
			return texnum;
		}
	}

	return -1;
}

void
Scrap_Upload(void)
{
	R_Bind(TEXNUM_SCRAPS);
	R_Upload8(scrap_texels[0], SCRAP_WIDTH, SCRAP_HEIGHT, false, false);
	scrap_dirty = false;
}

void
Scrap_Init(void)
{
	size_t i;

	for (i = 0; i < MAX_SCRAPS; i ++)
	{
		stbrp_init_target(&scrap_packer[i], SCRAP_WIDTH, SCRAP_HEIGHT,
			scrap_nodes[i], SCRAP_WIDTH * 2);
	}

	/* transparent */
	memset(scrap_texels, 255, sizeof(scrap_texels));
}

