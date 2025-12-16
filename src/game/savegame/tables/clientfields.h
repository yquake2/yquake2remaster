/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2011 Yamagi Burmeister
 * Copyright (c) ZeniMax Media Inc.
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
 * Fields of the client to be saved.
 *
 * =======================================================================
 */

{"newweapon", CLOFS(newweapon), F_ITEM},
{"owned_sphere", CLOFS(owned_sphere), F_EDICT},
{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
{"resp.coop_respawn.lastweapon", CLOFS(resp.coop_respawn.lastweapon), F_ITEM, 0, 3},
{"resp.coop_respawn.weapon", CLOFS(resp.coop_respawn.weapon), F_ITEM, 0, 3},
/* store fog */
{"pers.wanted_fog_density", CLOFS(pers.wanted_fog[0]), F_FLOAT},
{"pers.wanted_fog_r", CLOFS(pers.wanted_fog[1]), F_FLOAT},
{"pers.wanted_fog_g", CLOFS(pers.wanted_fog[2]), F_FLOAT},
{"pers.wanted_fog_b", CLOFS(pers.wanted_fog[3]), F_FLOAT},
{"pers.wanted_fog_skyfactor", CLOFS(pers.wanted_fog[4]), F_FLOAT},
{"pers.wanted_heightfog_falloff", CLOFS(pers.wanted_heightfog.falloff), F_FLOAT},
{"pers.wanted_heightfog_density", CLOFS(pers.wanted_heightfog.density), F_FLOAT},
{"pers.wanted_heightfog_start_color", CLOFS(pers.wanted_heightfog.start), F_VECTOR},
{"pers.wanted_heightfog_start_dist", CLOFS(pers.wanted_heightfog.start[3]), F_FLOAT},
{"pers.wanted_heightfog_end_color", CLOFS(pers.wanted_heightfog.end), F_VECTOR},
{"pers.wanted_heightfog_end_dist", CLOFS(pers.wanted_heightfog.end[3]), F_FLOAT},
{"pers.fog_transition_time", CLOFS(pers.fog_transition_time), F_FLOAT},
{NULL, 0, F_INT, 0}
