/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2011 Yamagi Burmeister
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
 * Spawntemp fields used for entity spawning.
 *
 * =======================================================================
 */

{"effects", STOFS(effects), F_INT},
{"fade_end_dist", STOFS(fade_end_dist), F_FLOAT},
{"fade_start_dist", STOFS(fade_start_dist), F_FLOAT},
{"goals", STOFS(goals), F_LRAWSTRING},
{"health_multiplier", STOFS(health_multiplier), F_FLOAT},
{"image", STOFS(image), F_LRAWSTRING},
{"radius", STOFS(radius), F_FLOAT},
{"shadowlightradius", STOFS(sl_radius), F_FLOAT},
{"shadowlightresolution", STOFS(sl_resolution), F_INT},
{"shadowlightintensity", STOFS(sl_intensity), F_FLOAT},
{"shadowlightstartfadedistance", STOFS(sl_fade_start), F_FLOAT},
{"shadowlightendfadedistance", STOFS(sl_fade_end), F_FLOAT},
{"shadowlightstyle", STOFS(sl_lightstyle), F_INT},
{"shadowlightconeangle", STOFS(sl_coneangle), F_FLOAT},
{"shadowlightstyletarget", STOFS(sl_lightstyletarget), F_LRAWSTRING},
{"renderfx", STOFS(renderfx), F_INT},
{"rgba", STOFS(rgba), F_RGBA},
{"rgb", STOFS(rgba), F_RGBA},
{"scale", STOFS(scale), F_SCALE},
{"lip", STOFS(lip), F_INT},
{"distance", STOFS(distance), F_INT},
{"height", STOFS(height), F_INT},
{"noise", STOFS(noise), F_LRAWSTRING},
{"pausetime", STOFS(pausetime), F_FLOAT},
{"item", STOFS(item), F_LRAWSTRING},
{"gravity", STOFS(gravity), F_LRAWSTRING},
{"start_items", STOFS(start_items), F_LRAWSTRING},
{"sky", STOFS(sky), F_LRAWSTRING},
{"skyrotate", STOFS(skyrotate), F_FLOAT},
{"skyautorotate", STOFS(skyautorotate), F_INT},
{"skyaxis", STOFS(skyaxis), F_VECTOR},
{"minyaw", STOFS(minyaw), F_FLOAT},
{"maxyaw", STOFS(maxyaw), F_FLOAT},
{"minpitch", STOFS(minpitch), F_FLOAT},
{"maxpitch", STOFS(maxpitch), F_FLOAT},
{"music", STOFS(music), F_LRAWSTRING},
{"nextmap", STOFS(nextmap), F_LRAWSTRING},
{"weight", STOFS(weight), F_INT},
