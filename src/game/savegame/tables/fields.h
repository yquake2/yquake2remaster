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
 * Game fields to be saved.
 *
 * =======================================================================
 */

{"classname", FOFS(classname), F_LRAWSTRING},
{"model", FOFS(model), F_LRAWSTRING},
{"spawnflags", FOFS(spawnflags), F_INT},
{"speed", FOFS(speed), F_FLOAT},
{"accel", FOFS(accel), F_FLOAT},
{"decel", FOFS(decel), F_FLOAT},
{"target", FOFS(target), F_LRAWSTRING},
{"targetname", FOFS(targetname), F_LRAWSTRING},
{"pathtarget", FOFS(pathtarget), F_LRAWSTRING},
{"deathtarget", FOFS(deathtarget), F_LRAWSTRING},
{"killtarget", FOFS(killtarget), F_LRAWSTRING},
{"combattarget", FOFS(combattarget), F_LRAWSTRING},
{"message", FOFS(message), F_LSTRING},
{"team", FOFS(team), F_LSTRING},
{"wait", FOFS(wait), F_FLOAT},
{"delay", FOFS(delay), F_FLOAT},
{"random", FOFS(random), F_FLOAT},
{"move_origin", FOFS(move_origin), F_VECTOR},
{"move_angles", FOFS(move_angles), F_VECTOR},
{"style", FOFS(style), F_INT},
{"count", FOFS(count), F_INT},
{"health", FOFS(health), F_INT},
{"sounds", FOFS(sounds), F_INT},
{"light", 0, F_IGNORE},
{"dmg", FOFS(dmg), F_INT},
{"mass", FOFS(mass), F_INT},
{"volume", FOFS(volume), F_FLOAT},
{"attenuation", FOFS(attenuation), F_FLOAT},
{"map", FOFS(map), F_LRAWSTRING},
{"origin", FOFS(s.origin), F_VECTOR},
{"angles", FOFS(s.angles), F_VECTOR},
{"angle", FOFS(s.angles), F_ANGLEHACK},
{"health_multiplier", STOFS(health_multiplier), F_FLOAT, FFL_SPAWNTEMP},
{"rgb", STOFS(rgba), F_RGBA, FFL_SPAWNTEMP},
{"rgba", STOFS(rgba), F_RGBA, FFL_SPAWNTEMP},
{"scale", STOFS(scale), F_VECTOR, FFL_SPAWNTEMP},
{"radius", STOFS(radius), F_FLOAT, FFL_SPAWNTEMP},
{"fade_start_dist", STOFS(fade_start_dist), F_FLOAT, FFL_SPAWNTEMP},
{"fade_end_dist", STOFS(fade_end_dist), F_FLOAT, FFL_SPAWNTEMP},
{"image", STOFS(image), F_LRAWSTRING, FFL_SPAWNTEMP},
{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},
{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},
{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},
{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},
{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
{"noise", STOFS(noise), F_LRAWSTRING, FFL_SPAWNTEMP},
{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
{"item", STOFS(item), F_LRAWSTRING, FFL_SPAWNTEMP},
{"item", FOFS(item), F_ITEM},
{"gravity", STOFS(gravity), F_LRAWSTRING, FFL_SPAWNTEMP},
{"sky", STOFS(sky), F_LRAWSTRING, FFL_SPAWNTEMP},
{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
{"skyautorotate", STOFS(skyautorotate), F_INT, FFL_SPAWNTEMP},
{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
{"music", STOFS(music), F_LRAWSTRING, FFL_SPAWNTEMP},
{"nextmap", STOFS(nextmap), F_LRAWSTRING, FFL_SPAWNTEMP},
{"bad_area", FOFS(bad_area), F_EDICT},
{"hint_chain", FOFS(hint_chain), F_EDICT},
{"monster_hint_chain", FOFS(monster_hint_chain), F_EDICT},
{"target_hint_chain", FOFS(target_hint_chain), F_EDICT},
{"goal_hint", FOFS(monsterinfo.goal_hint), F_EDICT},
{"badMedic1", FOFS(monsterinfo.badMedic1), F_EDICT},
{"badMedic2", FOFS(monsterinfo.badMedic2), F_EDICT},
{"last_player_enemy", FOFS(monsterinfo.last_player_enemy), F_EDICT},
{"commander", FOFS(monsterinfo.commander), F_EDICT},
{"blocked", FOFS(monsterinfo.blocked), F_FUNCTION, FFL_NOSPAWN},
{"duck", FOFS(monsterinfo.duck), F_FUNCTION, FFL_NOSPAWN},
{"unduck", FOFS(monsterinfo.unduck), F_FUNCTION, FFL_NOSPAWN},
{"sidestep", FOFS(monsterinfo.sidestep), F_FUNCTION, FFL_NOSPAWN},
{0, 0, 0, 0}
