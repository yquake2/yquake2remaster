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

extern polydesc_t r_polydesc;

extern vec5_t	r_clip_verts[2][MAXWORKINGVERTS+2];

/*
** R_DrawSprite
**
** Draw currententity / currentmodel as a single texture
** mapped polygon
*/
void
R_DrawSprite(entity_t *currententity, const model_t *currentmodel)
{
	vec5_t		*pverts;
	vec3_t		left, up, right, down, scale;
	dsprite_t	*s_psprite;
	dsprframe_t	*s_psprframe;
	image_t		*skin = NULL;

	VectorCopy(currententity->scale, scale);

	s_psprite = (dsprite_t *)currentmodel->extradata;
	currententity->frame %= s_psprite->numframes;

	s_psprframe = &s_psprite->frames[currententity->frame];

	if (currententity->frame < currentmodel->numskins)
	{
		skin = currentmodel->skins[currententity->frame];
	}

	if (!skin)
	{
		skin = r_notexture_mip;
	}

	r_polydesc.pixels       = skin->pixels[0];
	r_polydesc.pixel_width  = Q_min(s_psprframe->width, skin->width);
	r_polydesc.pixel_height = Q_min(s_psprframe->height, skin->height);
	r_polydesc.dist         = 0;

	// generate the sprite's axes, completely parallel to the viewplane.
	VectorCopy (vup, r_polydesc.vup);
	VectorCopy (vright, r_polydesc.vright);
	VectorCopy (vpn, r_polydesc.vpn);

	// build the sprite poster in worldspace
	VectorScale (r_polydesc.vright,
		(s_psprframe->width - s_psprframe->origin_x) * scale[0], right);
	VectorScale (r_polydesc.vup,
		(s_psprframe->height - s_psprframe->origin_y) * scale[1], up);
	VectorScale (r_polydesc.vright,
		-s_psprframe->origin_x * scale[0], left);
	VectorScale (r_polydesc.vup,
		-s_psprframe->origin_y * scale[1], down);

	// invert UP vector for sprites
	VectorInverse( r_polydesc.vup );

	pverts = r_clip_verts[0];

	pverts[0][0] = r_entorigin[0] + up[0] + left[0];
	pverts[0][1] = r_entorigin[1] + up[1] + left[1];
	pverts[0][2] = r_entorigin[2] + up[2] + left[2];
	pverts[0][3] = 0;
	pverts[0][4] = 0;

	pverts[1][0] = r_entorigin[0] + up[0] + right[0];
	pverts[1][1] = r_entorigin[1] + up[1] + right[1];
	pverts[1][2] = r_entorigin[2] + up[2] + right[2];
	pverts[1][3] = s_psprframe->width;
	pverts[1][4] = 0;

	pverts[2][0] = r_entorigin[0] + down[0] + right[0];
	pverts[2][1] = r_entorigin[1] + down[1] + right[1];
	pverts[2][2] = r_entorigin[2] + down[2] + right[2];
	pverts[2][3] = s_psprframe->width;
	pverts[2][4] = s_psprframe->height;

	pverts[3][0] = r_entorigin[0] + down[0] + left[0];
	pverts[3][1] = r_entorigin[1] + down[1] + left[1];
	pverts[3][2] = r_entorigin[2] + down[2] + left[2];
	pverts[3][3] = 0;
	pverts[3][4] = s_psprframe->height;

	r_polydesc.nump = 4;
	r_polydesc.s_offset = ( r_polydesc.pixel_width  >> 1);
	r_polydesc.t_offset = ( r_polydesc.pixel_height >> 1);
	VectorCopy( modelorg, r_polydesc.viewer_position );

	r_polydesc.stipple_parity = 1;
	if ( currententity->flags & RF_TRANSLUCENT )
	{
		R_ClipAndDrawPoly ( currententity->alpha, false, true );
	}
	else
	{
		R_ClipAndDrawPoly ( 1.0F, false, true );
	}

	r_polydesc.stipple_parity = 0;
}

void
R_DrawFlare(const entity_t *currententity)
{
	vec5_t		*pverts;
	vec3_t		left, up, right, down, scale;
	image_t		*skin = NULL;

	VectorCopy(currententity->scale, scale);

	skin = currententity->skin;

	if (!skin)
	{
		skin = r_notexture_mip;
	}

	r_polydesc.pixels       = skin->pixels[0];
	r_polydesc.pixel_width  = skin->width;
	r_polydesc.pixel_height = skin->height;
	r_polydesc.dist         = 0;

	// generate the sprite's axes, completely parallel to the viewplane.
	VectorCopy (vup, r_polydesc.vup);
	VectorCopy (vright, r_polydesc.vright);
	VectorCopy (vpn, r_polydesc.vpn);

	// build the sprite poster in worldspace
	VectorScale (r_polydesc.vright,
		skin->width * scale[0] / 2, right);
	VectorScale (r_polydesc.vup,
		skin->height * scale[1] / 2, up);
	VectorScale (r_polydesc.vright,
		-skin->width * scale[0] / 2, left);
	VectorScale (r_polydesc.vup,
		-skin->height * scale[1] / 2, down);

	// invert UP vector for sprites
	VectorInverse( r_polydesc.vup );

	pverts = r_clip_verts[0];

	pverts[0][0] = r_entorigin[0] + up[0] + left[0];
	pverts[0][1] = r_entorigin[1] + up[1] + left[1];
	pverts[0][2] = r_entorigin[2] + up[2] + left[2];
	pverts[0][3] = 0;
	pverts[0][4] = 0;

	pverts[1][0] = r_entorigin[0] + up[0] + right[0];
	pverts[1][1] = r_entorigin[1] + up[1] + right[1];
	pverts[1][2] = r_entorigin[2] + up[2] + right[2];
	pverts[1][3] = skin->width;
	pverts[1][4] = 0;

	pverts[2][0] = r_entorigin[0] + down[0] + right[0];
	pverts[2][1] = r_entorigin[1] + down[1] + right[1];
	pverts[2][2] = r_entorigin[2] + down[2] + right[2];
	pverts[2][3] = skin->width;
	pverts[2][4] = skin->height;

	pverts[3][0] = r_entorigin[0] + down[0] + left[0];
	pverts[3][1] = r_entorigin[1] + down[1] + left[1];
	pverts[3][2] = r_entorigin[2] + down[2] + left[2];
	pverts[3][3] = 0;
	pverts[3][4] = skin->height;

	r_polydesc.nump = 4;
	r_polydesc.s_offset = ( r_polydesc.pixel_width  >> 1);
	r_polydesc.t_offset = ( r_polydesc.pixel_height >> 1);
	VectorCopy( modelorg, r_polydesc.viewer_position );

	r_polydesc.stipple_parity = 1;
	if ( currententity->flags & RF_TRANSLUCENT )
		R_ClipAndDrawPoly ( currententity->alpha, false, true );
	else
		R_ClipAndDrawPoly ( 1.0F, false, true );
	r_polydesc.stipple_parity = 0;
}
