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
 */

#include "header/local.h"

vec3_t lightspot;
light_t	*blocklights = NULL, *blocklight_max = NULL;
byte	*bblocklights = NULL, *bblocklight_max = NULL;

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
RI_PushDlights
=============
*/
void
RI_PushDlights(const model_t *model)
{
	if (!model)
	{
		return;
	}

	R_PushDlights(&r_newrefdef, model->nodes + model->firstnode,
		r_framecount, model->surfaces);
}

/*
 * Combine and scale multiple lightmaps into the 8.8 format in blocklights
 */
void
RI_BuildLightMap(drawsurf_t* drawsurf, const refdef_t *r_newrefdef,
	float modulate, int r_framecount)
{
	int smax, tmax;
	int size;
	const light_t *max_light;
	msurface_t *surf;

	surf = drawsurf->surf;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax * tmax;

	if ((blocklight_max <= blocklights + (size * 3)) ||
		(bblocklight_max <= bblocklights + (size * LIGHTMAP_BYTES)))
	{
		r_outoflights = true;
		return;
	}

	if (r_fullbright->value || !r_worldmodel->lightdata ||
		(surf->texinfo->flags & (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)))
	{
		/* clear to no light */
		memset(blocklights, 0, size * sizeof(light_t) * 3);
		return;
	}

	max_light = blocklights + size * 3;

	R_BuildLightMap(surf, bblocklights, smax * 4,
		r_newrefdef, modulate, r_framecount, NULL, NULL);

	/* bound, invert, and shift */
	if(r_colorlight->value == 0)
	{
		light_t *curr_light;
		byte* curr;

		curr = bblocklights;
		curr_light = blocklights;

		do
		{
			int t;

			t = (255 - curr[3]) << VID_CBITS;

			if (t < VID_GRADES)
			{
				t = VID_GRADES;
			}

			curr_light[0] = curr_light[1] = curr_light[2] = t;
			curr_light += 3;
			curr += LIGHTMAP_BYTES;
		}
		while(curr_light < max_light);
	}
	else
	{
		light_t *curr_light;
		byte* curr;

		curr = bblocklights;
		curr_light = blocklights;

		do
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				int t;

				t = (255 - curr[i]) << VID_CBITS;

				if (t < VID_GRADES)
				{
					t = VID_GRADES;
				}

				*curr_light = t;
				curr_light ++;
			}
			curr += LIGHTMAP_BYTES;
		}
		while(curr_light < max_light);
	}
}
