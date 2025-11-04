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

/* Sum by or of all official maps (1 << 7 | 1 << 6 ....) */
#define QUAKE2_ALLFLAGS 0xbff

static const int quake2_flags[32] = {
	SURF_LIGHT,     /* 0: value will hold the light strength */
	SURF_SLICK,     /* 1: effects game physics */
	SURF_SKY,       /* 2: don't draw, but add to skybox */
	SURF_WARP,      /* 3: The surface warps (like water textures do) */
	SURF_TRANS33,   /* 4: The surface is 33% transparent */
	SURF_TRANS66,   /* 5: The surface is 66% transparent */
	SURF_FLOWING,   /* 6: The texture wraps in a downward 'flowing' pattern
				     *    (warp must also be set) */
	SURF_NODRAW,    /* 7: Used for non-fixed-size brush triggers and clip brushes */
	0,              /* 8: Unused */
	0,              /* 9: Unused */
	0,              /* 10: Unused */
	0,              /* 11: Unused */
	0,              /* 12: Unused */
	0,              /* 13: Unused */
	0,              /* 14: Unused */
	0,              /* 15: Unused */
	0,              /* 16: Unused */
	0,              /* 17: Unused */
	0,              /* 18: Unused */
	0,              /* 19: Unused */
	0,              /* 20: Unused */
	0,              /* 21: Unused */
	0,              /* 22: Unused */
	0,              /* 23: Unused */
	0,              /* 24: Unused */
	0,              /* 25: Unused */
	0,              /* 26: Unused */
	0,              /* 27: Unused */
	0,              /* 28: Unused */
	0,              /* 29: Unused */
	0,              /* 30: Unused */
	0,              /* 31: Unused */
};

static const int quake2_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: Default for all brushes */
	CONTENTS_WINDOW,       /* 1: translucent, but not watery */
	CONTENTS_AUX,          /* 2: Unused by the engine */
	CONTENTS_LAVA,         /* 3: The brush is lava */
	CONTENTS_SLIME,        /* 4: The brush is slime */
	CONTENTS_WATER,        /* 5: The brush is water */
	CONTENTS_MIST,         /* 6: The brush is non-solid */
	0,                     /* 7: Unused */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	CONTENTS_AREAPORTAL,   /* 15: Area portal */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Special brush used for specifying origin of rotation for rotating brushes*/
	CONTENTS_MONSTER,      /* 25: Purpose unknown / monster */
	CONTENTS_DEADMONSTER,  /* 26: Purpose unknown / deadmonster */
	CONTENTS_DETAIL,       /* 27: Detail brush */
	CONTENTS_TRANSLUCENT,  /* 28: Use for opaque water that does not block vis */
	CONTENTS_LADDER,       /* 29: Brushes with this flag allow a player to move up and down a vertical surface */
	0,                     /* 30: Doesn't block camera */
	0,                     /* 31: Unused */
};

static const int quake3_flags[32] = {
	0,              /* 0: never give falling damage */
	SURF_SLICK,     /* 1: effects game physics */
	SURF_SKY,       /* 2: lighting from environment map */
	0,              /* 3: SURF_LADDER */
	0,              /* 4: don't make missile explosions */
	0,              /* 5: don't leave missile marks */
	0,              /* 6: make flesh sounds and effects */
	SURF_NODRAW,    /* 7: Used for non-fixed-size brush triggers and clip brushes */
	0,              /* 8: make a primary bsp splitter */
	0,              /* 9: completely ignore, allowing non-closed brushes */
	0,              /* 10: surface doesn't need a lightmap */
	0,              /* 11: generate lighting info at vertexes */
	0,              /* 12: clanking footsteps */
	0,              /* 13: no footstep sounds */
	0,              /* 14: don't collide against curves with this set */
	0,              /* 15: act as a light filter during q3map -light */
	0,              /* 16: do per-pixel light shadow casting in q3map */
	0,              /* 17: never add dynamic lights */
	0,              /* 19: Unused */
	0,              /* 20: Unused */
	0,              /* 21: Unused */
	0,              /* 22: Unused */
	0,              /* 23: Unused */
	0,              /* 24: Unused */
	0,              /* 25: Unused */
	0,              /* 26: Unused */
	0,              /* 27: Unused */
	0,              /* 28: Unused */
	0,              /* 29: Unused */
	0,              /* 30: Unused */
	0,              /* 31: Unused */
};

static const int quake3_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: Default for all brushes */
	0,                     /* 1: Unused */
	0,                     /* 2: Unused */
	CONTENTS_LAVA,         /* 3: The brush is lava */
	CONTENTS_SLIME,        /* 4: The brush is slime */
	CONTENTS_WATER,        /* 5: The brush is water */
	CONTENTS_MIST,         /* 6: The brush is fog */
	0,                     /* 7: Unused */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	CONTENTS_AREAPORTAL,   /* 15: Area portal */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	0,                     /* 18: CONTENTS_TELEPORTER */
	0,                     /* 19: CONTENTS_JUMPPAD */
	0,                     /* 20: CONTENTS_CLUSTERPORTAL */
	0,                     /* 21: CONTENTS_DONOTENTER */
	0,                     /* 22: Unused */
	0,                     /* 23: Unused */
	CONTENTS_ORIGIN,       /* 24: Removed before bsping an entity */
	CONTENTS_MONSTER,      /* 25: CONTENTS_BODY */
	CONTENTS_DEADMONSTER,  /* 26: CONTENTS_CORPSE */
	CONTENTS_DETAIL,       /* 27: brushes not used for the bsp */
	0,                     /* 28: brushes used for the bsp */
	CONTENTS_TRANSLUCENT,  /* 29: don't consume surface fragments inside */
	0,                     /* 30: CONTENTS_TRIGGER */
	0,                     /* 31: don't leave bodies or items (death fog, lava) */
};

/* Sum by or of all official maps (1 << 24 | 1 << 25 ....) */
#define HERETIC2_ALLFLAGS 0x30037FF
#define HERETIC2_FLAGS (1 << 24 | 1 << 25)

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

static const int heretic2_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: Default for all brushes */
	CONTENTS_WINDOW,       /* 1: Brush is a window (not really used) */
	0,                     /* 2: pushpull / Purpose unknown */
	CONTENTS_LAVA,         /* 3: The brush is lava */
	CONTENTS_SLIME,        /* 4: The brush is slime */
	CONTENTS_WATER,        /* 5: The brush is water */
	CONTENTS_MIST,         /* 6: The brush is non-solid */
	0,                     /* 7: Unused */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	CONTENTS_AREAPORTAL,   /* 15: Area portal */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Special brush used for specifying origin of rotation for rotating brushes*/
	CONTENTS_MONSTER,      /* 25: Purpose unknown / monster */
	CONTENTS_DEADMONSTER,  /* 26: Purpose unknown / deadmonster */
	CONTENTS_DETAIL,       /* 27: Detail brush */
	CONTENTS_TRANSLUCENT,  /* 28: Use for opaque water that does not block vis */
	CONTENTS_LADDER,       /* 29: Brushes with this flag allow a player to move up and down a vertical surface */
	0,                     /* 30: Doesn't block camera */
	0,                     /* 31: Unused */
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
	SURF_ALPHATEST, /* 19: Midtexture (Used together with Clear and Nodraw.) */
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

static const char *daikatana_material[] = {
	NULL,              /* 0: Emit light from the surface, brightness is
		                *    specified in the 'value' field" */
	NULL,              /* 1: Fullbright */
	NULL,              /* 2: The surface is sky, the texture will not be drawn,
		                *    but the background sky box is used instead */
	NULL,              /* 3: The surface warps (like water textures do) */
	NULL,              /* 4: The surface is 33% transparent */
	NULL,              /* 5: The surface is 66% transparent */
	NULL,              /* 6: The texture wraps in a downward 'flowing' pattern
		                *    (warp must also be set) */
	NULL,              /* 7: Used for non-fixed-size brush triggers and clip brushes */
	NULL,              /* 8: Hint */
	NULL,              /* 9: Skip */
	"wood",            /* 10: Wood */
	"metal",           /* 11: Metal */
	"stone",           /* 12: Stone */
	"glass",           /* 13: Glass */
	"ice",             /* 14: Ice */
	"snow",            /* 15: Snow */
	"mirror",          /* 16: Mirror */
	"holy grond",      /* 17: Holy Grond */
	NULL,              /* 18: Alphachan */
	NULL,              /* 19: Midtexture (Used together with Clear and Nodraw.) */
	"puddle",          /* 20: Puddle */
	"water surge",     /* 21: Water Surge */
	"big water surge", /* 22: Big Water Surge */
	"bullet light",    /* 23: Bullet Light */
	"fog",             /* 24: Fog */
	"sand",            /* 25: Sand */
	NULL,              /* 26: Unused */
	NULL,              /* 27: Unused */
	NULL,              /* 28: Unused */
	NULL,              /* 29: Unused */
	NULL,              /* 30: Unused */
	NULL,              /* 31: Unused */
};

static const int daikatana_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: Default for all brushes */
	CONTENTS_WINDOW,       /* 1: Brush is a window (not really used) */
	CONTENTS_AUX,          /* 2: Unused by the Dk's engine? */
	CONTENTS_LAVA,         /* 3: The brush is lava */
	CONTENTS_SLIME,        /* 4: The brush is slime */
	CONTENTS_WATER,        /* 5: The brush is water */
	CONTENTS_MIST,         /* 6: The brush is non-solid */
	CONTENTS_WINDOW,       /* 7: clear */
	0,                     /* 8: notsolid */
	0,                     /* 9: noshoot */
	CONTENTS_MIST,         /* 10: fog */
	0,                     /* 11: nitro */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	0,                     /* 15: Unused */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Special brush used for specifying origin of rotation for rotating brushes */
	CONTENTS_MONSTER,      /* 25: Purpose unknown / Monster */
	CONTENTS_DEADMONSTER,  /* 26: Purpose unknown / Corpse */
	CONTENTS_DETAIL,       /* 27: Detail */
	CONTENTS_TRANSLUCENT,  /* 28: Use for opaque water that does not block vis */
	CONTENTS_LADDER,       /* 29: Brushes with this flag allow a player to move up and down a vertical surface */
	0,                     /* 30: NPC clip */
	0,                     /* 31: Unused */
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

static const char * kingpin_material[32] = {
	NULL,          /* 0: Emit light from the surface, brightness is specified
	                *    in the 'value' field */
	NULL,          /* 1: The surface is slippery */
	NULL,          /* 2: The surface is sky, the texture will not be drawn,
	                *    but the background sky box is used instead */
	NULL,          /* 3: The surface warps (like water textures do) */
	NULL,          /* 4: The surface is 33% transparent */
	NULL,          /* 5: The surface is 66% transparent */
	NULL,          /* 6: The texture wraps in a downward 'flowing' pattern
	                *    (warp must also be set) */
	NULL,          /* 7: Used for non-fixed-size brush triggers and clip brushes */
	NULL,          /* 8: Make a primary bsp splitter */
	NULL,          /* 9: Skip, Completely ignore, allowing non-closed brushes */
	NULL,          /* 10: Specular */
	NULL,          /* 11: Diffuse */
	NULL,          /* 12: Alpha texture */
	NULL,          /* 13: Mirror */
	NULL,          /* 14: Wndw33 */
	NULL,          /* 15: Wndw66 */
	NULL,          /* 16: Unused */
	NULL,          /* 17: Unused */
	NULL,          /* 18: Unused */
	"water",       /* 19: Water sound */
	"concrete",    /* 20: Concrete sound */
	"fabric",      /* 21: Fabric sound */
	"gravel",      /* 22: Gravel sound */
	"metal",       /* 23: Metal sound */
	"metal light", /* 24: Metal light sound */
	"tin",         /* 25: Tin sound */
	"tile",        /* 26: Tile sound */
	"wood",        /* 27: Wood sound */
	NULL,          /* 28: Reflect fake */
	NULL,          /* 29: Reflect light */
	NULL,          /* 30: Unused */
	NULL,          /* 31: Unused */
};

static const int kingpin_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: An eye is never valid in a solid */
	CONTENTS_WINDOW,       /* 1: translucent, but not watery */
	0,                     /* 2: Unused by the engine */
	CONTENTS_LAVA,         /* 3: The brush is lava */
	CONTENTS_SLIME,        /* 4: The brush is slime" */
	CONTENTS_WATER,        /* 5: Water */
	CONTENTS_MIST,         /* 6: The brush is non-solid */
	0,                     /* 7: The brush is solid / fence */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	0,                     /* 15: Unused */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Removed before bsping an entity */
	CONTENTS_MONSTER,      /* 25: should never be on a brush, only in game */
	CONTENTS_DEADMONSTER,  /* 26: Corpse */
	CONTENTS_DETAIL,       /* 27: brushes to be added after vis leafs */
	CONTENTS_TRANSLUCENT,  /* 28: auto set if any surface has trans */
	CONTENTS_LADDER,       /* 29: Brushes with this flag allow a player to move up and down a vertical surface */
	0,                     /* 30: Unused */
	0,                     /* 31: Unused */
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
	0,              /* 10: Snd_Wood */
	0,              /* 11: Snd_Metal */
	0,              /* 12: Snd_Stone */
	0,              /* 13: Snd_Carpet */
	0,              /* 14: Snd_Ice */
	0,              /* 15: Snd_Snow */
	0,              /* 16: Alpha Banner */
	SURF_ALPHATEST, /* 17: Alpha Test */
	0,              /* 18: No V-turbulence */
	0,              /* 19: Snd_Hollow */
	0,              /* 20: Snd_Puddle */
	0,              /* 21: Surf_Scroll */
	0,              /* 22: Snd_Gravel */
	0,              /* 23: Snd_Leaves */
	0,              /* 24: Snd_Grass */
	0,              /* 25: Snd_Sand */
	0,              /* 26: Snd_Water */
	0,              /* 27: Surf_Particles */
	0,              /* 28: Half Scroll */
	0,              /* 29: Quarter Scroll */
	0,              /* 30: Surface Fog */
	0,              /* 31: Surface Curve */
};

static const char * anachronox_material[32] = {
	NULL,              /* 0: Light */
	NULL,              /* 1: Slick */
	NULL,              /* 2: Sky Flag */
	NULL,              /* 3: Warp */
	NULL,              /* 4: Trans33 */
	NULL,              /* 5: Trans66 */
	NULL,              /* 6: Unused */
	NULL,              /* 7: NoDraw */
	NULL,              /* 8: Hint */
	NULL,              /* 9: Skip */
	"wood",            /* 10: Snd_Wood */
	"metal",           /* 11: Snd_Metal */
	"stone",           /* 12: Snd_Stone */
	"carpet",          /* 13: Snd_Carpet */
	"ice",             /* 14: Snd_Ice */
	"snow",            /* 15: Snd_Snow */
	NULL,              /* 16: Alpha Banner */
	NULL,              /* 17: Alpha Test */
	NULL,              /* 18: No V-turbulence */
	"hollow",          /* 19: Snd_Hollow */
	"puddle",          /* 20: Snd_Puddle */
	NULL,              /* 21: Surf_Scroll */
	"gravel",          /* 22: Snd_Gravel */
	"leaves",          /* 23: Snd_Leaves */
	"grass",           /* 24: Snd_Grass */
	"sand",            /* 25: Snd_Sand */
	"water",           /* 26: Snd_Water */
	NULL,              /* 27: Surf_Particles */
	NULL,              /* 28: Half Scroll */
	NULL,              /* 29: Quarter Scroll */
	NULL,              /* 30: Surface Fog */
	NULL,              /* 31: Surface Curve */
};

static const int anachronox_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: An eye is never valid in a solid */
	CONTENTS_WINDOW,       /* 1: translucent, but not watery */
	0,                     /* 2: Unused */
	0,                     /* 3: Unused */
	0,                     /* 4: Unused */
	CONTENTS_WATER,        /* 5: Water */
	CONTENTS_MIST,         /* 6: Mist */
	0,                     /* 7: Unused */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Unused */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	CONTENTS_AREAPORTAL,   /* 15: Area portal */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Removed before bsping an entity */
	CONTENTS_MONSTER,      /* 25: should never be on a brush, only in game */
	0,                     /* 26: */
	CONTENTS_DETAIL,       /* 27: brushes to be added after vis leafs */
	CONTENTS_TRANSLUCENT,  /* 28: auto set if any surface has trans */
	0,                     /* 29: Unused */
	0,                     /* 30: Unused */
	0,                     /* 31: Unused */
};

/*
 * Based on https://github.com/NightDive-Studio/sin-ex-game/
 */
static const int sin_flags[32] = {
	SURF_LIGHT,     /* 0: Value will hold the light strength */
	SURF_SLICK,     /* 1: Effects game physics */
	SURF_SKY,       /* 2: Don't draw, but add to skybox */
	SURF_WARP,      /* 3: Turbulent water warp */
	0,              /* 4: Surface is not lit  (SiN: was SURF_TRANS33) */
	0,              /* 5: Surface is not filtered (SiN: was SURF_TRANS66) */
	0,              /* 6: SiN: was SURF_FLOWING */
	SURF_NODRAW,    /* 7: Don't bother referencing the texture */
	0,              /* 8: Unused */
	0,              /* 9: Unused */
	0,              /* 10: Surface has waves */
	0,              /* 11: Projectiles bounce literally bounce off this surface */
	0,              /* 12: Surface has intensity information for pre-lighting */
	0,              /* 13: Unused */
	0,              /* 14: Surface has a console on it */
	0,              /* 15: Unused */
	0,              /* 16: Surface should only do things in hardware */
	0,              /* 17: Surface can be damaged */
	0,              /* 18: Surface has weak hit points */
	0,              /* 19: Surface has normal hit points */
	0,              /* 20: Surface will be additive */
	0,              /* 21: Surface is envmapped */
	0,              /* 22: Surface start animating on a random frame */
	0,              /* 23: Surface animates */
	0,              /* 24: Time between animations is random */
	0,              /* 25: Surface translates */
	0,              /* 26: Surface is not merged in csg phase */
	0,              /* 27: 0 bit of surface type */
	0,              /* 28: 1 bit of surface type */
	0,              /* 29: 2 bit of surface type */
	0,              /* 30: 3 bit of surface type */
	0,              /* 31: Unused */
};

static const int sin_contents_flags[32] = {
	CONTENTS_SOLID,        /* 0: An eye is never valid in a solid */
	CONTENTS_WINDOW,       /* 1: translucent, but not watery */
	0,                     /* 2: SiN: formerly CONTENTS_AUX */
	CONTENTS_LAVA,         /* 3: Lava */
	0,                     /* 4: SiN: light volume, formerly CONTENTS_SLIME */
	CONTENTS_WATER,        /* 5: Water */
	CONTENTS_MIST,         /* 6: Mist */
	0,                     /* 7: Unused */
	0,                     /* 8: Unused */
	0,                     /* 9: Unused */
	0,                     /* 10: Unused */
	0,                     /* 11: Unused */
	0,                     /* 12: Dummy fence */
	0,                     /* 13: Unused */
	0,                     /* 14: Unused */
	CONTENTS_AREAPORTAL,   /* 15: Area portal */
	CONTENTS_PLAYERCLIP,   /* 16: Player cannot pass through the brush (other things can) */
	CONTENTS_MONSTERCLIP,  /* 17: Monster cannot pass through the brush (player and other things can) */
	CONTENTS_CURRENT_0,    /* 18: Brush has a current in direction of 0 degrees */
	CONTENTS_CURRENT_90,   /* 19: Brush has a current in direction of 90 degrees */
	CONTENTS_CURRENT_180,  /* 20: Brush has a current in direction of 180 degrees */
	CONTENTS_CURRENT_270,  /* 21: Brush has a current in direction of 270 degrees */
	CONTENTS_CURRENT_UP,   /* 22: Brush has a current in the up direction */
	CONTENTS_CURRENT_DOWN, /* 23: Brush has a current in the down direction */
	CONTENTS_ORIGIN,       /* 24: Removed before bsping an entity */
	CONTENTS_MONSTER,      /* 25: should never be on a brush, only in game */
	CONTENTS_DEADMONSTER,  /* 26: */
	CONTENTS_DETAIL,       /* 27: brushes to be added after vis leafs */
	CONTENTS_TRANSLUCENT,  /* 28: auto set if any surface has trans */
	CONTENTS_LADDER,       /* 29: */
	0,                     /* 30: Is shootable, but may not be blocking */
	0,                     /* 31: Unused */
};
