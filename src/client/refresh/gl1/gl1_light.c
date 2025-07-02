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
 * Lightmaps and dynamic lighting
 *
 * =======================================================================
 */

#include "header/local.h"

int r_dlightframecount;
vec3_t lightspot;

static void
R_RenderDlight(dlight_t *light)
{
	const float rad = light->intensity * 0.35;
	int i, j;
	float vtx[3];

	R_SetBufferIndices(GL_TRIANGLE_FAN, 18);

	for ( i = 0; i < 3; i++ )
	{
		vtx [ i ] = light->origin [ i ] - vpn [ i ] * rad;
	}

	GLBUFFER_VERTEX( vtx[0], vtx[1], vtx[2] )
	GLBUFFER_COLOR( light->color[0] * 51, light->color[1] * 51,
		light->color[2] * 51, 255 )	// 255 * 0.2 = 51

	for ( i = 16; i >= 0; i-- )
	{
		float a;

		a = i / 16.0 * M_PI * 2;

		for ( j = 0; j < 3; j++ )
		{
			vtx[ j ] = light->origin [ j ] + vright [ j ] * cos( a ) * rad
				+ vup [ j ] * sin( a ) * rad;
		}

		GLBUFFER_VERTEX( vtx[0], vtx[1], vtx[2] )
		GLBUFFER_COLOR( 0, 0, 0, 255 )
	}
}

void
R_RenderDlights(void)
{
	int i;
	dlight_t *l;

	if (!r_flashblend->value)
	{
		return;
	}
	R_UpdateGLBuffer(buf_flash, 0, 0, 0, 1);

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	l = r_newrefdef.dlights;

	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_RenderDlight(l);
	}
	R_ApplyGLBuffer();

	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}

void
RI_PushDlights(void)
{
	if (r_flashblend->value || !r_worldmodel)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	R_PushDlights(&r_newrefdef, r_worldmodel->nodes,
			r_dlightframecount, r_worldmodel->surfaces);
}
