/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * Objects implementations
 *
 * =======================================================================
 */

#include "header/local.h"

void
destructible_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// Play explosion or debris effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self); // Remove object
}

void
DynamicObjectSpawn(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_YES;

	self->die = destructible_die;

	gi.linkentity(self);
}

void
SP_obj_banner(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_banneronpole(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
object_flame_think(edict_t *self)
{
	M_SetAnimGroupFrame(self, "flame");
	self->nextthink = level.time + FRAMETIME;
}

/*
 * QUAKED light_torch_small_walltorch (-10 -10 -20) (10 10 20)
 * QUAKED light_flame_large_yellow (-10 -10 -40) (10 10 40)
 * QUAKED light_flame_small_white (-10 -10 -12) (12 12 18)
 * QUAKED light_flame_small_yellow (-10 -10 -12) (12 12 18)
 *
 * Quake I: Large yellow flame
 * Quake I: Small white flame
 * Quake I: Small yellow flame
 * Quake I: Small walltorch
 */
void
SP_quake_light_flame(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = object_flame_think;
	self->nextthink = level.time + FRAMETIME;

	self->s.frame = 0;
	self->s.sound = 0;

	gi.linkentity(self);
}

/*
 * QUAKED object_flame1 (1 .5 .5) (-3 -3 -6) (3 3 11)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound torch
 *    2) sound campfire
 */

void
SP_object_flame1(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = object_flame_think;
	self->nextthink = level.time + FRAMETIME;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/torchburn.wav");
			break;
		case 2:
			self->s.sound = gi.soundindex("objects/fire/campfire.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}

void
SP_obj_barrel(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_object_barrel(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_broom(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chair1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chair2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chair3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chest1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chest2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_chest3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cog1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_corpse1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_corpse2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_dying_elf(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_sign1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_sign4(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_stalagmite1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_stalagmite2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_stalagmite3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_corvus(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dolphin1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dolphin2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dolphin3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dolphin4(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_guardian(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_table1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_table2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_throne(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_kettle(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cauldron(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_firepot(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_duckbill1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_duckbill2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_seasonglobe(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_stein(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_scroll(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_fountain_fish(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_boulderfish(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pottedplant(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_plant1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_plant2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_plant3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_treetop(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_tree(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_tree2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_tree3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_treetall(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_treefallen(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_shovel(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_woodpile(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_fishtrap(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bench(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bucket(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_ropechain(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_wheelbarrow(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_wheelbarrowdamaged(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_urn(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bigcrystal(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_moss1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_moss2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_moss3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_moss4(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_moss5(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_floor_candelabrum(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dragonhead(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_dragon(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_flagonpole(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_lever1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_lever2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_lever3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bush1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bush2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cactus(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cactus3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cactus4(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_basket(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_claybowl(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_clayjar(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_gorgonbones(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_grass(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_swampflat_top(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_swampflat_bottom(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_treestump(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_jawbone(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_barrel_metal(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_barrel_exploding(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_barrel_indestructible(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_barrel_explosive(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_gascan(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pipe1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pipe2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pipewheel(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_minecart(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_minecart2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_minecart3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_andwallhanging(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pick(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_metalchunk1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_metalchunk2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_metalchunk3(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_rocks1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_rocks2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_hivepriestessssymbol(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_queenthrone(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_queenchair(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_shrine(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_larvaegg(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_larvabrokenegg(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cocoon(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_cocoonopen(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_venusflytrap(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_techeckriktomb(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_techeckrikright(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_techeckrikleft(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_spellbook(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_skullpole(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pot1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pot2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bottle1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_jug1(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_torture_table(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_torture_wallring(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_tchecktrik_bust(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_sithraguard(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_torture_ironmaiden(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_torture_rack(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_torture_bed(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_saraphbust(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_biotank(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_tapper(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_wallringplaque(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_hangingdude(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_frypan(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_eggpan(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_nest(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_choppeddude(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_lab_parts_container(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_eyeball_jar(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_lab_tray(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_hanging_ogle(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_ring_plaque2(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_statue_sariph(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_pushcart(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bookopen(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bookclosed(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_web(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_larva(edict_t *self)
{
	DynamicObjectSpawn(self);
}

void
SP_obj_bloodsplat(edict_t *self)
{
	DynamicObjectSpawn(self);
}

/*
 * QUAKED object_big_fire (1 .5 .5) (-3 -3 -6) (3 3 11)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound campfire
 */
void
object_big_fire_think(edict_t *self)
{
	M_SetAnimGroupFrame(self, "bigfire");
	self->nextthink = level.time + FRAMETIME;

	/* add particles */
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_FLAME);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void
SP_object_big_fire(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = object_big_fire_think;
	self->nextthink = level.time + FRAMETIME;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/torchburn.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}

/*
 * QUAKED object_campfire (1 .5 .5) (-10 -10 -5) (10 10 5)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound campfire
*/
void
SP_object_campfire(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/campfire.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}
