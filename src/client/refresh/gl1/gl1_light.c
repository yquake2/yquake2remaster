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
R_MarkSurfaceLights(dlight_t *light, int bit, mnode_t *node, int r_dlightframecount)
{
	msurface_t	*surf;
	int			i;

	/* mark the polygons */
	surf = r_worldmodel->surfaces + node->firstsurface;

	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		int sidebit;
		float dist;

		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}

		dist = DotProduct(light->origin, surf->plane->normal) - surf->plane->dist;

		if (dist >= 0)
		{
			sidebit = 0;
		}
		else
		{
			sidebit = SURF_PLANEBACK;
		}

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
		{
			continue;
		}

		surf->dlightbits |= bit;
	}
}

void
R_PushDlights(void)
{
	dlight_t *l;
	int i;

	if (r_flashblend->value)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	l = r_newrefdef.dlights;

	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_MarkLights(l, 1 << i, r_worldmodel->nodes, r_dlightframecount,
			R_MarkSurfaceLights);
	}
}

float *s_blocklights = NULL, *s_blocklights_max = NULL;

/*
 * Combine and scale multiple lightmaps into the floating format in blocklights
 */
void
R_BuildLightMap(msurface_t *surf, byte *dest, int stride)
{
	int smax, tmax;
	int r, g, b, a, max;
	int i, j, size;
	byte *lightmap;
	float scale[4];
	int nummaps;
	float *bl;

	if (surf->texinfo->flags &
		(SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
	{
		Com_Error(ERR_DROP, "R_BuildLightMap called for non-lit surface");
	}

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

	/* set to full bright if no light data */
	if (!surf->samples)
	{
		for (i = 0; i < size * 3; i++)
		{
			s_blocklights[i] = 255;
		}

		goto store;
	}

	/* count the # of maps */
	for (nummaps = 0; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255;
		 nummaps++)
	{
	}

	lightmap = surf->samples;

	/* add all the lightmaps */
	if (nummaps == 1)
	{
		int maps;

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
			{
				scale[i] = r_modulate->value *
						   r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];
			}

			if ((scale[0] == 1.0F) &&
				(scale[1] == 1.0F) &&
				(scale[2] == 1.0F))
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] = lightmap[i * 3 + 0];
					bl[1] = lightmap[i * 3 + 1];
					bl[2] = lightmap[i * 3 + 2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] = lightmap[i * 3 + 0] * scale[0];
					bl[1] = lightmap[i * 3 + 1] * scale[1];
					bl[2] = lightmap[i * 3 + 2] * scale[2];
				}
			}

			lightmap += size * 3; /* skip to next lightmap */
		}
	}
	else
	{
		int maps;

		memset(s_blocklights, 0, sizeof(s_blocklights[0]) * size * 3);

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
			{
				scale[i] = r_modulate->value *
						   r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];
			}

			if ((scale[0] == 1.0F) &&
				(scale[1] == 1.0F) &&
				(scale[2] == 1.0F))
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] += lightmap[i * 3 + 0];
					bl[1] += lightmap[i * 3 + 1];
					bl[2] += lightmap[i * 3 + 2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] += lightmap[i * 3 + 0] * scale[0];
					bl[1] += lightmap[i * 3 + 1] * scale[1];
					bl[2] += lightmap[i * 3 + 2] * scale[2];
				}
			}

			lightmap += size * 3; /* skip to next lightmap */
		}
	}

	/* add all the dynamic lights */
	if (surf->dlightframe == r_framecount)
	{
		R_AddDynamicLights(surf, &r_newrefdef, s_blocklights, s_blocklights_max);
	}

store:
	/* put into texture format */
	stride -= (smax << 2);
	bl = s_blocklights;

	for (i = 0; i < tmax; i++, dest += stride)
	{
		for (j = 0; j < smax; j++)
		{
			r = Q_ftol(bl[0]);
			g = Q_ftol(bl[1]);
			b = Q_ftol(bl[2]);

			/* catch negative lights */
			if (r < 0)
			{
				r = 0;
			}

			if (g < 0)
			{
				g = 0;
			}

			if (b < 0)
			{
				b = 0;
			}

			/* determine the brightest of the three color components */
			if (r > g)
			{
				max = r;
			}
			else
			{
				max = g;
			}

			if (b > max)
			{
				max = b;
			}

			/* alpha is ONLY used for the mono lightmap case. For this
			   reason we set it to the brightest of the color components
			   so that things don't get too dim. */
			a = max;

			/* rescale all the color components if the
			   intensity of the greatest channel exceeds
			   1.0 */
			if (max > 255)
			{
				float t = 255.0F / max;

				r = r * t;
				g = g * t;
				b = b * t;
				a = a * t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}
}
