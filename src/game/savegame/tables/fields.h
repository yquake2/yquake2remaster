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

{"accel", FOFS(accel), F_FLOAT},
{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
{"angle", FOFS(s.angles), F_ANGLEHACK},
{"angles", FOFS(s.angles), F_VECTOR},
{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
{"attenuation", FOFS(attenuation), F_FLOAT},
{"badMedic1", FOFS(monsterinfo.badMedic1), F_EDICT},
{"badMedic2", FOFS(monsterinfo.badMedic2), F_EDICT},
{"bad_area", FOFS(bad_area), F_EDICT},
{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
{"blocked", FOFS(monsterinfo.blocked), F_FUNCTION, FFL_NOSPAWN},
{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},
{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
{"classname", FOFS(classname), F_LSTRING},
{"combattarget", FOFS(combattarget), F_LSTRING},
{"commander", FOFS(monsterinfo.commander), F_EDICT},
{"count", FOFS(count), F_INT},
{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},
{"deathtarget", FOFS(deathtarget), F_LSTRING},
{"decel", FOFS(decel), F_FLOAT},
{"delay", FOFS(delay), F_FLOAT},
{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},
{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
{"dmg", FOFS(dmg), F_INT},
{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
{"duck", FOFS(monsterinfo.duck), F_FUNCTION, FFL_NOSPAWN},
{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},
{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
{"goal_hint", FOFS(monsterinfo.goal_hint), F_EDICT},
{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
{"health", FOFS(health), F_INT},
{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
{"hint_chain", FOFS(hint_chain), F_EDICT},
{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
{"item", FOFS(item), F_ITEM},
{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},
{"killtarget", FOFS(killtarget), F_LSTRING},
{"last_player_enemy", FOFS(monsterinfo.last_player_enemy), F_EDICT},
{"light", 0, F_IGNORE},
{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
{"map", FOFS(map), F_LSTRING},
{"mass", FOFS(mass), F_INT},
{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
{"message", FOFS(message), F_LSTRING},
{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
{"model", FOFS(model), F_LSTRING},
{"monster_hint_chain", FOFS(monster_hint_chain), F_EDICT},
{"move_angles", FOFS(move_angles), F_VECTOR},
{"move_origin", FOFS(move_origin), F_VECTOR},
{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},
{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
{"origin", FOFS(s.origin), F_VECTOR},
{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
{"pathtarget", FOFS(pathtarget), F_LSTRING},
{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
{"random", FOFS(random), F_FLOAT},
{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
{"sidestep", FOFS(monsterinfo.sidestep), F_FUNCTION, FFL_NOSPAWN},
{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
{"skyautorotate", STOFS(skyautorotate), F_INT, FFL_SPAWNTEMP},
{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
{"sounds", FOFS(sounds), F_INT},
{"spawnflags", FOFS(spawnflags), F_INT},
{"speed", FOFS(speed), F_FLOAT},
{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
{"style", FOFS(style), F_INT},
{"target", FOFS(target), F_LSTRING},
{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
{"target_hint_chain", FOFS(target_hint_chain), F_EDICT},
{"targetname", FOFS(targetname), F_LSTRING},
{"team", FOFS(team), F_LSTRING},
{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
{"unduck", FOFS(monsterinfo.unduck), F_FUNCTION, FFL_NOSPAWN},
{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
{"volume", FOFS(volume), F_FLOAT},
{"wait", FOFS(wait), F_FLOAT},
{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
{0, 0, 0, 0}
