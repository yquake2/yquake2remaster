/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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

extern gl4lightmapstate_t gl4_lms;

int r_dlightframecount;
vec3_t lightspot;

void
GL4_PushDlights(void)
{
	dlight_t *l;
	int i;

	if (!gl4_worldmodel)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = gl4_framecount + 1;

	R_PushDlights(&r_newrefdef, gl4_worldmodel->nodes, r_dlightframecount,
			gl4_worldmodel->surfaces);

	l = r_newrefdef.dlights;

	gl4state.uniLightsData.numDynLights = r_newrefdef.num_dlights;

	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		gl4UniDynLight* udl = &gl4state.uniLightsData.dynLights[i];
		VectorCopy(l->origin, udl->origin);
		VectorCopy(l->color, udl->color);
		udl->intensity = l->intensity;
	}

	assert(MAX_DLIGHTS == 32 && "If MAX_DLIGHTS changes, remember to adjust the uniform buffer definition in the shader!");

	if(i < MAX_DLIGHTS)
	{
		memset(&gl4state.uniLightsData.dynLights[i], 0, (MAX_DLIGHTS-i)*sizeof(gl4state.uniLightsData.dynLights[0]));
	}

	GL4_UpdateUBOLights();
}

/*
 * Combine and scale multiple lightmaps into the floating format in blocklights
 */
void
GL4_BuildLightMap(msurface_t *surf, int offsetInLMbuf, int stride)
{
	int smax, tmax;
	int r, g, b, a, max;
	int i, j, size, map, nummaps;
	byte *lightmap;

	if (surf->texinfo->flags &
		(SURF_SKY | SURF_TRANSPARENT | SURF_WARP))
	{
		Com_Error(ERR_DROP, "%s called for non-lit surface", __func__);
	}

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax * tmax;

	stride -= (smax << 2);

	if (size > BLOCK_WIDTH * BLOCK_HEIGHT * 3)
	{
		Com_Error(ERR_DROP, "Bad s_blocklights size");
	}

	// count number of lightmaps surf actually has
	for (nummaps = 0; nummaps < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[nummaps] != 255; ++nummaps)
	{}

	if (!surf->samples)
	{
		// no lightmap samples? set at least one lightmap to fullbright, rest to 0 as normal

		if (nummaps == 0)  nummaps = 1; // make sure at least one lightmap is set to fullbright

		for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE; ++map)
		{
			// we always create 4 (MAX_LIGHTMAPS_PER_SURFACE) lightmaps.
			// if surf has less (nummaps < 4), the remaining ones are zeroed out.
			// this makes sure that all 4 lightmap textures in gl4state.lightmap_textureIDs[i] have the same layout
			// and the shader can use the same texture coordinates for all of them

			int c = (map < nummaps) ? 255 : 0;
			byte* dest = gl4_lms.lightmap_buffers[map] + offsetInLMbuf;

			for (i = 0; i < tmax; i++, dest += stride)
			{
				memset(dest, c, 4 * smax);
				dest += 4 * smax;
			}
		}

		return;
	}

	/* add all the lightmaps */

	// Note: dynamic lights aren't handled here anymore, they're handled in the shader

	// as we don't apply scale here anymore, nor blend the nummaps lightmaps together,
	// the code has gotten a lot easier and we can copy directly from surf->samples to dest
	// without converting to float first etc

	lightmap = surf->samples;

	for(map=0; map<nummaps; ++map)
	{
		byte* dest = gl4_lms.lightmap_buffers[map] + offsetInLMbuf;
		int idxInLightmap = 0;
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				r = lightmap[idxInLightmap * 3 + 0];
				g = lightmap[idxInLightmap * 3 + 1];
				b = lightmap[idxInLightmap * 3 + 2];

				/* determine the brightest of the three color components */
				if (r > g)  max = r;
				else  max = g;

				if (b > max)  max = b;

				/* alpha is ONLY used for the mono lightmap case. For this
				   reason we set it to the brightest of the color components
				   so that things don't get too dim. */
				a = max;

				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
				dest[3] = a;

				dest += 4;
				++idxInLightmap;
			}
		}

		lightmap += size * 3; /* skip to next lightmap */
	}

	for ( ; map < MAX_LIGHTMAPS_PER_SURFACE; ++map)
	{
		// like above, fill up remaining lightmaps with 0

		byte* dest = gl4_lms.lightmap_buffers[map] + offsetInLMbuf;

		for (i = 0; i < tmax; i++, dest += stride)
		{
			memset(dest, 0, 4*smax);
			dest += 4*smax;
		}
	}
}

