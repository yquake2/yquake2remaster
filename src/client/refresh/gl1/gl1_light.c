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
	int i, j;
	float a;
	float rad;

	rad = light->intensity * 0.35;

	GLfloat vtx[3*18];
	GLfloat clr[4*18];

	unsigned int index_vtx = 3;
	unsigned int index_clr = 0;

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	clr[index_clr++] = light->color [ 0 ] * 0.2;
	clr[index_clr++] = light->color [ 1 ] * 0.2;
	clr[index_clr++] = light->color [ 2 ] * 0.2;
	clr[index_clr++] = 1;

	for ( i = 0; i < 3; i++ )
	{
		vtx [ i ] = light->origin [ i ] - vpn [ i ] * rad;
	}

	for ( i = 16; i >= 0; i-- )
	{
		clr[index_clr++] = 0;
		clr[index_clr++] = 0;
		clr[index_clr++] = 0;
		clr[index_clr++] = 1;

		a = i / 16.0 * M_PI * 2;

		for ( j = 0; j < 3; j++ )
		{
			vtx[index_vtx++] = light->origin [ j ] + vright [ j ] * cos( a ) * rad
				+ vup [ j ] * sin( a ) * rad;
		}
	}

	glVertexPointer( 3, GL_FLOAT, 0, vtx );
	glColorPointer( 4, GL_FLOAT, 0, clr );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 18 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
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

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	glDepthMask(0);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	l = r_newrefdef.dlights;

	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_RenderDlight(l);
	}

	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(1);
}

void
RI_PushDlights(void)
{
	if (r_flashblend->value)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	R_PushDlights(&r_newrefdef, r_worldmodel->nodes, r_dlightframecount,
		r_worldmodel->surfaces);
}

float *s_blocklights = NULL, *s_blocklights_max = NULL;

/*
 * Combine and scale multiple lightmaps into the floating format in blocklights
 */
void
RI_BuildLightMap(msurface_t *surf, byte *dest, int stride)
{
	int size, smax, tmax;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax * tmax;

	if (!s_blocklights || (s_blocklights + (size * 3) >= s_blocklights_max))
	{
		int new_size = ROUNDUP(size * 3, 1024);

		if (new_size < 4096)
		{
			new_size = 4096;
		}

		if (s_blocklights)
		{
			free(s_blocklights);
		}

		s_blocklights = malloc(new_size * sizeof(float));
		s_blocklights_max = s_blocklights + new_size;

		if (!s_blocklights)
		{
			Com_Error(ERR_DROP, "Can't alloc s_blocklights");
		}
	}

	R_BuildLightMap(surf, dest, stride, &r_newrefdef, s_blocklights, s_blocklights_max,
		r_modulate->value, r_framecount);
}
