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
 * The maps file format
 *
 * =======================================================================
 */

/*
 * Based on https://github.com/TrenchBroom/
*/
static const int heretic2_flags[32] = {
	SURF_LIGHT,     /* 0: Emit light from the surface, brightness is specified in the 'value' field */
	SURF_SLICK,     /* 1: The surface is slippery */
	SURF_SKY,       /* 2: The surface is sky, the texture will not be drawn,
				     *    but the background sky box is used instead */
	SURF_WARP,      /* 3: The surface warps (like water textures do) */
	SURF_TRANS33,   /* 4: The surface is 33% transparent */
	SURF_TRANS66,   /* 5: The surface is 66% transparent */
	SURF_FLOWING,   /* 6: The texture wraps in a downward 'flowing' pattern
				     *    (warp must also be set) */
	SURF_NODRAW,    /* 7: Used for non-fixed-size brush triggers and clip brushes */
	0,              /* 8: Make a primary bsp splitter */
	0,              /* 9: Completely ignore, allowing non-closed brushes */
	0,              /* 10: Tall wall. Purpose unknown */
	0,              /* 11: Alpha_tex. Purpose unknown */
	0,              /* 12: Animspeed. Purpose unknown */
	0,              /* 13: Undulate. Purpose unknown */
	0,              /* 14: Skyreflect. Purpose unknown */
	0,              /* 15: Unused */
	0,              /* 16: Unused */
	0,              /* 17: Unused */
	0,              /* 18: Unused */
	0,              /* 19: Unused */
	0,              /* 20: Unused */
	0,              /* 21: Unused */
	0,              /* 22: Unused */
	0,              /* 23: Unused */
	0,              /* 24: Metal. Sound when walked on */
	0,              /* 25: Stone. Sound when walked on */
	0,              /* 26: Unused */
	0,              /* 27: Unused */
	0,              /* 28: Unused */
	0,              /* 29: Unused */
	0,              /* 30: Unused */
	0,              /* 31: Unused */
};

static const int daikatana_flags[32] = {
	SURF_LIGHT,     /* 0: Emit light from the surface, brightness is
				     *    specified in the 'value' field" */
	SURF_LIGHT,     /* 1: Fullbright */
	SURF_SKY,       /* 2: The surface is sky, the texture will not be drawn,
				     *    but the background sky box is used instead */
	SURF_WARP,      /* 3: The surface warps (like water textures do) */
	SURF_TRANS33,   /* 4: The surface is 33% transparent */
	SURF_TRANS66,   /* 5: The surface is 66% transparent */
	SURF_FLOWING,   /* 6: The texture wraps in a downward 'flowing' pattern
				     *    (warp must also be set) */
	SURF_NODRAW,    /* 7: Used for non-fixed-size brush triggers and clip brushes */
	0,              /* 8: Hint */
	0,              /* 9: Skip */
	0,              /* 10: Wood */
	0,              /* 11: Metal */
	0,              /* 12: Stone */
	0,              /* 13: Glass */
	0,              /* 14: Ice */
	0,              /* 15: Snow */
	0,              /* 16: Mirror */
	0,              /* 17: Holy Grond */
	SURF_ALPHATEST, /* 18: Alphachan */
	0,              /* 19: Midtexture (Used together with Clear and Nodraw.) */
	0,              /* 20: Puddle */
	0,              /* 21: Water Surge */
	0,              /* 22: Big Water Surge */
	0,              /* 23: Bullet Light */
	0,              /* 24: Fog */
	0,              /* 25: Sand */
	0,              /* 26: Unused */
	0,              /* 27: Unused */
	0,              /* 28: Unused */
	0,              /* 29: Unused */
	0,              /* 30: Unused */
	0,              /* 31: Unused */
};

static const int kingpin_flags[32] = {
	SURF_LIGHT,     /* 0: Emit light from the surface, brightness is specified
				     *    in the 'value' field */
	SURF_SLICK,     /* 1: The surface is slippery */
	SURF_SKY,       /* 2: The surface is sky, the texture will not be drawn,
				     *    but the background sky box is used instead */
	SURF_WARP,      /* 3: The surface warps (like water textures do) */
	SURF_TRANS33,   /* 4: The surface is 33% transparent */
	SURF_TRANS66,   /* 5: The surface is 66% transparent */
	SURF_FLOWING,   /* 6: The texture wraps in a downward 'flowing' pattern
				     *    (warp must also be set) */
	SURF_NODRAW,    /* 7: Used for non-fixed-size brush triggers and clip brushes */
	0,              /* 8: Make a primary bsp splitter */
	0,              /* 9: Skip, Completely ignore, allowing non-closed brushes */
	0,              /* 10: Specular */
	0,              /* 11: Diffuse */
	SURF_ALPHATEST, /* 12: Alpha texture */
	0,              /* 13: Mirror */
	0,              /* 14: Wndw33 */
	0,              /* 15: Wndw66 */
	0,              /* 16: Unused */
	0,              /* 17: Unused */
	0,              /* 18: Unused */
	0,              /* 19: Water sound */
	0,              /* 20: Concrete sound */
	0,              /* 21: Fabric sound */
	0,              /* 22: Gravel sound */
	0,              /* 23: Metal sound */
	0,              /* 24: Metal light sound */
	0,              /* 25: Tin sound */
	0,              /* 26: Tile sound */
	0,              /* 27: Wood sound */
	0,              /* 28: Reflect fake */
	0,              /* 29: Reflect light */
	0,              /* 30: Unused */
	0,              /* 31: Unused */
};

/*
 * Based on https://anachrodox.talonbrave.info/
 */
static const int anachronox_flags[32] = {
	SURF_LIGHT,     /* 0: Light */
	SURF_SLICK,     /* 1: Slick */
	SURF_SKY,       /* 2: Sky Flag */
	SURF_WARP,      /* 3: Warp */
	SURF_TRANS33,   /* 4: Trans33 */
	SURF_TRANS66,   /* 5: Trans66 */
	0,              /* 6: Unused */
	SURF_NODRAW,    /* 7: NoDraw */
	0,              /* 8: Hint */
	0,              /* 9: Skip */
	0,              /* 10: Unused */
	0,              /* 11: Unused */
	0,              /* 12: Unused */
	0,              /* 13: Unused */
	0,              /* 14: Unused */
	0,              /* 15: Unused */
	0,              /* 16: Alpha Banner */
	SURF_ALPHATEST, /* 17: Alpha Test */
	0,              /* 18: No V-turbulence */
	0,              /* 19: Unused */
	0,              /* 20: Unused */
	0,              /* 21: Unused */
	0,              /* 22: Unused */
	0,              /* 23: Unused */
	0,              /* 24: Unused */
	0,              /* 25: Unused */
	0,              /* 26: Unused */
	0,              /* 27: Unused */
	0,              /* 28: Half Scroll */
	0,              /* 29: Quarter Scroll */
	0,              /* 30: Surface Fog */
	0,              /* 31: Surface Curve */
};
