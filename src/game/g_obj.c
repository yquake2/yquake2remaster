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

/*
 * NODAMAGE: can't be damaged
 * EXPLODE: could explode
 * STOPMOVE: can't be moved
 */
#define OBJ_NODAMAGE   1
#define OBJ_WITHEFFECT 2
#define OBJ_EXPLODE    4
#define OBJ_STOPMOVE   8

void
destructible_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t org;

	if (!self)
	{
		return;
	}

	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", 5, org);

	if (self->dmg > 0)
	{
		BecomeExplosion1(self);
	}
	else
	{
		G_FreeEdict(self); /* Remove object */
	}
}

/*
 * QUAKED obj_andwallhanging (0.3 0.3 1.0) (0.0 -19.0 -24.0) (4.0 19.0 24.0)
 *
 * Heretic 2: Circular Andorian wall hanging
 */
/*
 * QUAKED monster_bee (1 .5 0) (-2.0 -2.0 -25.0) (2.0 2.0 25.0)
 *
 * The bee
 */
/*
 * QUAKED obj_basket (0.3 0.3 1.0) (-13.0 -13.0 -21.0) (13.0 13.0 21.0)
 *
 * Heretic 2: Basket
 */
/*
 * QUAKED obj_bookclosed (0.3 0.3 1.0) (-8.0 -8.0 -2.0) (8.0 8.0 2.0)
 *
 * Heretic 2: Closed book standing up
 */
/*
 * QUAKED obj_bookopen (0.3 0.3 1.0) (-8.0 -16.0 -2.0) (8.0 16.0 2.0)
 *
 * Heretic 2: Open Book
 */
/*
 * QUAKED obj_bottle1 (0.3 0.3 1.0) (-3.0 -3.0 -7.0) (3.0 3.0 7.0)
 *
 * Heretic 2: Bottle
 */
/*
 * QUAKED obj_chair1 (0.3 0.3 1.0) (-12.0 -8.0 -26.0) (12.0 8.0 26.0)
 *
 * Heretic 2: Chair (wood)
 */
/*
 * QUAKED obj_claybowl (0.3 0.3 1.0) (-6.0 -6.0 -2.0) (6.0 6.0 2.0)
 *
 * Heretic 2: Clay bowl
 */
/*
 * QUAKED obj_clayjar (0.3 0.3 1.0) (-15.0 -15.0 -24.0) (15.0 15.0 24.0)
 *
 * Heretic 2: Clay jar
 */
/*
 * QUAKED obj_fishtrap (0.3 0.3 1.0) (-14.0 -28.0 -13.0) (14.0 28.0 13.0)
 *
 * Heretic 2: Fish trap
 */
/*
 * QUAKED obj_jug1 (0.3 0.3 1.0) (-6.0 -6.0 -6.0) (6.0 6.0 6.0)
 *
 * Heretic 2: Jug
 */
/*
 * QUAKED obj_kettle (0.3 0.3 1.0) (-8.0 -8.0 0.0) (8.0 8.0 10.0)
 *
 * Heretic 2: Kettle
 */
/*
 * QUAKED obj_lab_parts_container (0.3 0.3 1.0) (-8.0 -8.0 -11.0) (8.0 8.0 11.0)
 *
 * Heretic 2: Body parts container
 */
/*
 * QUAKED obj_pick (0.3 0.3 1.0) (-12.0 -13.0 -2.0) (12.0 13.0 2.0)
 *
 * Heretic 2: Pick
 */
/*
 * QUAKED obj_pot2 (0.3 0.3 1.0) (-7.0 -7.0 -3.0) (7.0 7.0 3.0)
 *
 * Heretic 2: Flat cooking pot
 */
/*
 * QUAKED obj_pottedplant (0.3 0.3 1.0) (-20.0 -20.0 -30.0) (20.0 20.0 30.0)
 *
 * Heretic 2: Potted plant
 */
/*
 * QUAKED obj_pushcart (0.3 0.3 1.0) (-13.0 -16.0 -41.0) (13.0 16.0 41.0)
 *
 * Heretic 2: Push cart
 */
/*
 * QUAKED obj_scroll (0.3 0.3 1.0) (-2.0 -18.0 -3.0) (2.0 18.0 3.0)
 *
 * Heretic 2: Scroll
 */
/*
 * QUAKED obj_stein (0.3 0.3 1.0) (-2.0 -2.0 -3.0) (2.0 2.0 3.0)
 *
 * Heretic 2: Stein
 */
/*
 * QUAKED obj_table1 (0.3 0.3 1.0) (-28.0 -54.0 -18.0) (28.0 54.0 18.0)
 *
 * Heretic 2: Table (wood)
 */
/*
 * QUAKED obj_urn (0.3 0.3 1.0) (-8.0 -8.0 -27.0) (8.0 8.0 30.0)
 *
 * Heretic 2: Urn (Andorian)
 */
/*
 * QUAKED obj_wheelbarrow (0.3 0.3 1.0) (-37.0 -20.0 -21.0) (37.0 20.0 21.0)
 *
 * Heretic 2: Wheelbarrow
 */
void
SP_obj_material(edict_t *self)
{
	if (self->spawnflags & OBJ_NODAMAGE)
	{
		self->takedamage = DAMAGE_NO;
	}
	else
	{
		self->takedamage = DAMAGE_YES;
	}

	if (!(self->spawnflags & OBJ_STOPMOVE))
	{
		self->movetype = MOVETYPE_STOP;
	}
	else
	{
		self->movetype = MOVETYPE_NONE;
	}

	self->die = destructible_die;
	gi.linkentity(self);
}

/*
 * QUAKED obj_banner (0.3 0.3 1.0) (-8.0 -44.0 -296.0) (8.0 44.0 0.0)
 *
 * Heretic 2: Banner animated, can't be moved and damaged
 */
void
SP_obj_banner(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.sound = gi.soundindex("ambient/bannerflap.wav");
	object_spawn(self);
}

/*
 * QUAKED obj_banneronpole (0.3 0.3 1.0) (-8.0 -28.0 -30.0) (8.0 28.0 30.0)
 *
 * Heretic 2: Banner on pole, animated, can't be moved and damaged
 */
void
SP_obj_banneronpole(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.sound = gi.soundindex("ambient/bannerflap.wav");
	object_spawn(self);
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
	object_spawn(self);
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

	object_spawn(self);
}

/*
 * QUAKED obj_barrel (0.3 0.3 1.0) (-12.0 -12.0 -19.0) (12.0 12.0 19.0) NODAMAGE EXPLODE STOPMOVE
 *
 * Heretic 2: Barrel
 */
void
SP_obj_barrel(edict_t *self)
{
	if (self->spawnflags & OBJ_EXPLODE)
	{
		self->dmg = 10;
		self->s.skinnum = 1;
	}

	SP_obj_material(self);
}

/*
 * QUAKED object_barrel (0.3 0.3 1.0) (-10.0 -10.0 -14.0) (10.0 10.0 14.0)
 *
 * Dawn of Darkness: Barrel
 */
void
SP_object_barrel(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_broom (0.3 0.3 1.0) (-2.0 -2.0 -25.0) (2.0 2.0 25.0) NODAMAGE
 *
 * Heretic 2: Broom, can't be moved
 */
/*
 * QUAKED obj_chair2 (0.3 0.3 1.0) (-18.0 -29.0 -30.0) (18.0 29.0 30.0) NODAMAGE
 *
 * Heretic 2: Chair (wood, slanted), can't be moved
 */
/*
 * QUAKED obj_table2 (0.3 0.3 1.0) (-28.0 -54.0 -18.0) (28.0 54.0 18.0) NODAMAGE
 *
 * Heretic 2: Table (stone), can't be moved
 */
/*
 * QUAKED obj_throne (0.3 0.3 1.0) (-20.0 -22.0 -44.0) (20.0 22.0 44.0) NODAMAGE
 *
 * Heretic 2: Throne, can't be moved
 */
/*
 * QUAKED obj_statue_boulderfish (0.3 0.3 1.0) (-26.0 -16.0 -27.0) (26.0 16.0 27.0) NODAMAGE
 *
 * Heretic 2: Fish statue, can't be moved
 */
/*
 * QUAKED obj_shovel (0.3 0.3 1.0) (-8.0 -8.0 -20.0) (8.0 8.0 20.0) NODAMAGE
 *
 * Heretic 2: Shovel, can't be moved
 */
/*
 * QUAKED obj_woodpile (0.3 0.3 1.0) (-12.0 -20.0 -7.0) (12.0 20.0 7.0) NODAMAGE
 *
 * Heretic 2: Wood Pile, can't be moved
 */
/*
 * QUAKED obj_bench (0.3 0.3 1.0) (-10.0 -21.0 -10.0) (10.0 21.0 10.0) NODAMAGE
 *
 * Heretic 2: Bench, can't be moved
 */
/*
 * QUAKED obj_bucket (0.3 0.3 1.0) (-8.0 -8.0 -9.0) (8.0 8.0 10.0) NODAMAGE
 *
 * Heretic 2: Bucket, can't be moved
 */
/*
 * QUAKED obj_gorgonbones (0.3 0.3 1.0) (-18.0 -38.0 -9.0) (18.0 38.0 1.0) NODAMAGE
 *
 * Heretic 2: Gorgon bones, can't be moved
 */
/*
 * QUAKED obj_grass (0.3 0.3 1.0) (-8.0 -8.0 -10.0) (8.0 8.0 10.0) NODAMAGE
 *
 * Heretic 2: Grass clump, can't be moved
 */
/*
 * QUAKED obj_queenchair (0.3 0.3 1.0) (-30.0 -28.0 -31.0) (30.0 28.0 31.0) NODAMAGE
 *
 * Heretic 2: Hive queen chair, can't be moved
 */
/*
 * QUAKED obj_larvaegg (0.3 0.3 1.0) (-6.0 -14.0 -6.0) (6.0 14.0 6.0) NODAMAGE
 *
 * Heretic 2: Hive egg, can't be moved
 */
/*
 * QUAKED obj_larvabrokenegg (0.3 0.3 1.0) (-6.0 -7.0 -5.0) (6.0 7.0 5.0) NODAMAGE
 *
 * Heretic 2: Hive egg, can't be moved
 */
/*
 * QUAKED obj_cocoon (0.3 0.3 1.0) (-8.0 -8.0 -8.0) (8.0 8.0 8.0) NODAMAGE
 *
 * Heretic 2: Hanging cocoon, can't be moved
 */
/*
 * QUAKED obj_pot1 (0.3 0.3 1.0) (-3.0 -8.0 -8.0) (3.0 8.0 8.0) NODAMAGE
 *
 * Heretic 2: Hanging cooking pot, can't be moved
 */
/*
 * QUAKED obj_torture_wallring (0.3 0.3 1.0) (-2.0 -4.0 -6.0) (2.0 4.0 6.0) NODAMAGE
 *
 * Heretic 2: Hanging ring, can't be moved
 */
/*
 * QUAKED obj_statue_saraphbust (0.3 0.3 1.0) (-10.0 -20.0 -24.0) (10.0 20.0 24.0) NODAMAGE
 *
 * Heretic 2: Seraph bust, can't be moved
 */
/*
 * QUAKED obj_tapper (0.3 0.3 1.0) (-2.0 -5.0 -2.0) (2.0 5.0 2.0) NODAMAGE
 *
 * Heretic 2: Keg tapper, can't be moved
 */
/*
 * QUAKED obj_frypan (0.3 0.3 1.0) (-1.0 -3.0 -10.0) (1.0 3.0 10.0) NODAMAGE
 *
 * Heretic 2: Hanging pan, can't be moved
 */
/*
 * QUAKED obj_eggpan (0.3 0.3 1.0) (-4.0 -10.0 -1.0) (4.0 10.0 1.0) NODAMAGE
 *
 * Heretic 2: Flat pan, can't be moved
 */
/*
 * QUAKED obj_nest (0.3 0.3 1.0) (-25.0 -25.0 -4.0) (25.0 25.0 4.0) NODAMAGE
 *
 * Heretic 2: Nest, can't be moved
 */
/*
 * QUAKED obj_choppeddude (0.3 0.3 1.0) (-15.0 -40.0 -8.0) (15.0 40.0 8.0) NODAMAGE
 *
 * Heretic 2: Lying chopped corpse, can't be moved
 */
/*
 * QUAKED obj_eyeball_jar (0.3 0.3 1.0) (-13.0 -13.0 -18.0) (13.0 13.0 18.0) NODAMAGE
 *
 * Heretic 2: Jar of joy, can't be moved
 */
/*
 * QUAKED obj_statue_sariph (0.3 0.3 1.0) (-13.0 -16.0 -41.0) (13.0 16.0 41.0) NODAMAGE
 *
 * Heretic 2: Seraph statue, can't be moved
 */
/*
 * QUAKED obj_rocks1 (0.3 0.3 1.0) (-12.0 -13.0 -4.0) (12.0 13.0 4.0) NODAMAGE
 *
 * Heretic 2: Rock cluster, can't be moved
 */
/*
 * QUAKED obj_rocks2 (0.3 0.3 1.0) (-9.0 -30.0 -4.0) (9.0 30.0 4.0) NODAMAGE
 *
 * Heretic 2: Big rock, can't be moved, can't be moved
 */
/*
 * QUAKED obj_lab_tray (0.3 0.3 1.0) (-8.0 -8.0 -5.0) (8.0 8.0 5.0) NODAMAGE
 *
 * Heretic 2: Tray with heart and tools, can't be moved
 */
void
SP_obj_material_stopmove(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_chair3 (0.3 0.3 1.0) (-14.0 -21.0 -28.0) (14.0 21.0 28.0)
 *
 * Heretic 2: Chair (stone), can't be moved and damaged.
 */
/*
 * QUAKED obj_statue_corvus (0.3 0.3 1.0) (-16.0 -16.0 0.0) (16.0 16.0 32.0)
 *
 * Heretic 2: Corvus statue, can't be moved and damaged.
 */
/*
 * QUAKED obj_statue_guardian (0.3 0.3 1.0) (-100.0 -64.0 0.0) (64.0 64.0 128.0)
 *
 * Heretic 2: Guardian statue, can't be moved and damaged.
 */
/*
 * QUAKED obj_fountain_fish (0.3 0.3 1.0) (-52.0 -34.0 -48.0) (52.0 34.0 48.0)
 *
 * Heretic 2: Two-headed fish fountain. Water FX not included, can't be moved and damaged.
 */
/*
 * QUAKED obj_treetop (0.3 0.3 1.0) (-176.0 -176.0 -125.0) (176.0 176.0 125.0)
 *
 * Heretic 2: Tree canopy, can't be moved and damaged.
 */
/*
 * QUAKED obj_tree (0.3 0.3 1.0) (-100.0 -100.0 -120.0) (100.0 100.0 120.0)
 *
 * Heretic 2: Tree, can't be moved and damaged.
 */
/*
 * QUAKED obj_statue_dragonhead (0.3 0.3 1.0) (-76.0 -28.0 -46.0) (76.0 28.0 46.0)
 *
 * Heretic 2: Dragon head statue, can't be moved and damaged.
 */
/*
 * QUAKED obj_swampflat_top (0.3 0.3 1.0) (0.0 -100.0 -48.0) (2.0 100.0 48.0)
 *
 * Heretic 2: A flat poly to be used on the outer edge of swamp levels. Vegetation
 * growing up, can't be moved and damaged.
 */
/*
 * QUAKED obj_queenthrone (0.3 0.3 1.0) (-40.0 -56.0 -49.0) (40.0 56.0 49.0)
 *
 * Heretic 2: Hive queen throne, can't be moved and damaged.
 */
/*
 * QUAKED obj_skullpole (0.3 0.3 1.0) (-10.0 -10.0 -47.0) (10.0 10.0 47.0)
 *
 * Heretic 2: Skull pole, can't be moved and damaged.
 */
/*
 * QUAKED obj_torture_rack (0.3 0.3 1.0) (-22.0 -46.0 -19.0) (22.0 46.0 19.0)
 *
 * Heretic 2: Torture rack, can't be moved and damaged.
 */
/*
 * QUAKED obj_torture_bed (0.3 0.3 1.0) (-21.0 -43.0 -94.0) (21.0 43.0 94.0)
 *
 * Heretic 2: Bed of spikes, can't be moved and damaged.
 */
/*
 * QUAKED obj_wallringplaque (0.3 0.3 1.0) (-3.0 -20.0 -55.0) (3.0 20.0 55.0)
 *
 * Heretic 2: Wall ring plaque, can't be moved and damaged.
 */
/*
 * QUAKED obj_ring_plaque2 (0.3 0.3 1.0) (-2.0 -24.0 -20.0) (2.0 24.0 20.0)
 *
 * Heretic 2: Rings mounted on wall plate, can't be moved and damaged.
 */
void
SP_obj_material_nomoveanddamage(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_chest1 (0.3 0.3 1.0) (-10.0 -18.0 -19.0) (10.0 18.0 19.0)
 *
 * Heretic 2: Chest (opens when triggered)
 */
void
SP_obj_chest1(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_chest2 (0.3 0.3 1.0) (-14.0 -17.0 -9.0) (14.0 17.0 9.0)
 *
 * Heretic 2: Chest (mines)
 */
void
SP_obj_chest2(edict_t *self)
{
	self->spawnflags &= ~OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_chest3 (0.3 0.3 1.0) (-10.0 -17.0 -6.0) (10.0 17.0 6.0)
 *
 * Heretic 2: Chest (mines, closed)
 */
void
SP_obj_chest3(edict_t *self)
{
	self->spawnflags &= ~OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cog1 (0.3 0.3 1.0) (-8.0 -4.0 0.0) (8.0 4.0 20.0)
 *
 * Heretic 2: Cog (triggerable)
 */
void
SP_obj_cog1(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_corpse1 (0.3 0.3 1.0) (-30.0 -12.0 0.0) (30.0 12.0 5.0)
 *
 * Heretic 2: Plague Elf corpse
 */
/*
 * QUAKED obj_corpse2 (0.3 0.3 1.0) (-30.0 -12.0 0.0) (30.0 12.0 5.0)
 *
 * Heretic 2: Plague Elf corpse (alternate skin)
 */
void
SP_obj_corpse(edict_t *self)
{
	int frame = 0, count = 0;

	if (!strcmp(self->classname, "obj_corpse2"))
	{
		self->s.skinnum = 1;
	}
	else
	{
		self->s.skinnum = 0;
	}

	self->rrs.mesh = 0x1e; /* disable weapons */

	if (self->style >= 0 && self->style <= 3)
	{
		M_SetAnimGroupFrameValues(self, "death", &frame, &count, self->style);
	}
	else if (self->style == 4)
	{
		M_SetAnimGroupFrameValues(self, "skewered", &frame, &count, self->style);
	}

	if (count)
	{
		frame += count - 1;
	}

	self->s.frame = frame;

	SP_obj_material(self);
}

/*
 * QUAKED obj_dying_elf (0.3 0.3 1.0) (-30.0 -12.0 0.0) (30.0 12.0 5.0)
 *
 * Heretic 2: Dying elf
 */
void
SP_obj_dying_elf(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->monsterinfo.action = "fetal";
	self->rrs.mesh = 0x1e; /* disable weapons */
	self->think = object_think;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

/*
 * QUAKED obj_sign1 (0.3 0.3 1.0) (-29.0 -4.0 -16.0) (29.0 4.0 16.0)
 *
 * Heretic 2: Sign 1
 */
void
SP_obj_sign1(edict_t *self)
{
	if (self->style >= 0 && self->style <= 2)
	{
		self->s.skinnum = self->style;
	}

	self->s.frame = 3;
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_sign4 (0.3 0.3 1.0) (-8.0 -18.0 -29.0) (8.0 18.0 29.0)
 *
 * Heretic 2: Sign 4
 */
void
SP_obj_sign4(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;

	if (!self->style)
	{
		self->s.skinnum = 0;
	}
	else
	{
		self->s.skinnum = 1;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_dolphin1 (0.3 0.3 1.0) (-68.0 -22.0 -30.0) (68.0 22.0 30.0)
 *
 * Heretic 2: Dolphin statue on all fours
 */
void
SP_obj_statue_dolphin1(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;

	/* Toggle destructibility */
	self->spawnflags ^= OBJ_NODAMAGE;

	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_dolphin2 (0.3 0.3 1.0) (-17.0 -20.0 -70.0) (17.0 20.0 70.0)
 *
 * Heretic 2: Dolphin statue on wall turned right
 */
void
SP_obj_statue_dolphin2(edict_t *self)
{
	self->s.frame = 1;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_dolphin3 (0.3 0.3 1.0) (-17.0 -20.0 -70.0) (17.0 20.0 70.0)
 *
 * Heretic 2: Dolphin statue on wall turned left
 */
void
SP_obj_statue_dolphin3(edict_t *self)
{
	self->s.frame = 3;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_dolphin4 (0.3 0.3 1.0) (-63.0 -22.0 -37.0) (63.0 22.0 37.0)
 *
 * Heretic 2: Dolphin statue on two legs
 */
void
SP_obj_statue_dolphin4(edict_t *self)
{
	self->s.frame = 2;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cauldron (0.3 0.3 1.0) (-22.0 -22.0 -10.0) (22.0 22.0 10.0)
 *
 * Heretic 2: Cauldron
 */
void
SP_obj_cauldron(edict_t *self)
{
	if (self->spawnflags & OBJ_WITHEFFECT)
	{
		self->s.sound = gi.soundindex("ambient/cauldronbubble.wav");
	}

	self->spawnflags |= OBJ_STOPMOVE;

	SP_obj_material(self);
}

/*
 * QUAKED obj_firepot (0.3 0.3 1.0) (-18.0 -18.0 -12.0) (18.0 18.0 12.0)
 *
 * Heretic 2: Firepot
 */
void
SP_obj_firepot(edict_t *self)
{
	self->s.sound = gi.soundindex("ambient/fireplace.wav");

	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_duckbill1 (0.3 0.3 1.0) (-63.0 -22.0 -37.0) (63.0 22.0 37.0)
 *
 * Heretic 2: The duckbilled thing - tail to the right
 */
void
SP_obj_statue_duckbill1(edict_t *self)
{
	self->s.frame = 0;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_duckbill2 (0.3 0.3 1.0) (-67.0 -24.0 -50.0) (67.0 24.0 50.0)
 *
 * Heretic 2: The duckbilled thing - tail to the left
 */
void
SP_obj_statue_duckbill2(edict_t *self)
{
	self->s.frame = 1;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_seasonglobe (0.3 0.3 1.0) (-80.0 -80.0 0.0) (80.0 80.0 100.0)
 *
 * Heretic 2: Globe (triggerable)
 */
void
SP_obj_seasonglobe(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_plant1 (0.3 0.3 1.0) (-8.0 -8.0 -24.0) (8.0 8.0 24.0)
 *
 * Heretic 2: Plant
 */
/*
 * QUAKED obj_plant2 (0.3 0.3 1.0) (-20.0 -20.0 -10.0) (20.0 20.0 20.0)
 *
 * Heretic 2: Plant
 */
/*
 * QUAKED obj_plant3 (0.3 0.3 1.0) (-8.0 -8.0 -12.0) (8.0 8.0 12.0)
 *
 * Heretic 2: Plant
 */
void
SP_obj_plant(edict_t *self)
{
	if (!strcmp(self->classname, "obj_plant3"))
	{
		self->s.skinnum = self->style;
	}

	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_tree2 (0.3 0.3 1.0) (-50.0 -50.0 -286.0) (50.0 50.0 286.0)
 *
 * Heretic 2: Tree (swamp)
 */
void
SP_obj_tree2(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_tree3 (0.3 0.3 1.0) (-50.0 -50.0 -286.0) (50.0 50.0 286.0)
 *
 * Heretic 2: Tree with roots
 */
void
SP_obj_tree3(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_treetall (0.3 0.3 1.0) (-46.0 -46.0 -340.0) (46.0 46.0 340.0)
 *
 * Heretic 2: Tree (tall)
 */
void
SP_obj_treetall(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_treefallen (0.3 0.3 1.0) (-24.0 -62.0 -35.0) (24.0 62.0 35.0)
 *
 * Heretic 2: Fallen tree
 */
void
SP_obj_treefallen(edict_t *self)
{
	self->s.frame = 1;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_ropechain (0.3 0.3 1.0) (-20.0 -20.0 -14.0) (20.0 20.0 14.0)
 *
 * Heretic 2: Rope chain
 */
void
SP_obj_ropechain(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;

	if (self->s.skinnum == 0)
	{
		self->gib = GIB_WOOD;
	}
	else
	{
		self->gib = GIB_METALLIC;
		self->health *= 2;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_wheelbarrowdamaged (0.3 0.3 1.0) (-38.0 -26.0 -20.0) (38.0 26.0 20.0)
 *
 * Heretic 2: Wheelbarrow on side
 */
void
SP_obj_wheelbarrowdamaged(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	self->s.frame = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_bigcrystal (0.3 0.3 1.0) (-35.0 -35.0 -50.0) (35.0 35.0 50.0)
 *
 * Heretic 2: Rotating crystal
 */
void
SP_obj_bigcrystal(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_moss1 (0.3 0.3 1.0) (-4.0 -10.0 -40.0) (4.0 10.0 40.0)
 *
 * Heretic 2: Moss 1
 */
/*
 * QUAKED obj_moss2 (0.3 0.3 1.0) (-4.0 -10.0 -40.0) (4.0 10.0 40.0)
 *
 * Heretic 2: Moss 2
 */
/*
 * QUAKED obj_moss3 (0.3 0.3 1.0) (-4.0 -10.0 -40.0) (4.0 10.0 40.0)
 *
 * Heretic 2: Moss 3
 */
/*
 * QUAKED obj_moss4 (0.3 0.3 1.0) (-4.0 -10.0 -40.0) (4.0 10.0 40.0)
 *
 * Heretic 2: Moss 4
 */
/*
 * QUAKED obj_moss5 (0.3 0.3 1.0) (-4.0 -10.0 -40.0) (4.0 10.0 40.0)
 *
 * Heretic 2: Moss 5
 */
void
SP_obj_moss(edict_t *self)
{
	if (!strcmp(self->classname, "obj_moss2"))
	{
		self->s.skinnum = 1;
	}
	else if (!strcmp(self->classname, "obj_moss3"))
	{
		self->s.skinnum = 2;
	}
	else if (!strcmp(self->classname, "obj_moss4"))
	{
		self->s.skinnum = 3;
	}
	else if (!strcmp(self->classname, "obj_moss5"))
	{
		self->s.skinnum = 4;
	}
	else
	{
		self->s.skinnum = 0;
	}

	self->s.renderfx |= RF_TRANSLUCENT;
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_floor_candelabrum (0.3 0.3 1.0) (-8.0 -8.0 -35.0) (8.0 8.0 35.0)
 *
 * Heretic 2: Floor candelbrum. Does not emit light
 */
void
SP_obj_floor_candelabrum(edict_t *self)
{
	object_spawn(self);
}

/*
 * QUAKED obj_statue_dragon (0.3 0.3 1.0) (-53.0 -33.0 -72.0) (53.0 33.0 72.0)
 *
 * Heretic 2: Dragon statue
 */
void
SP_obj_statue_dragon(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;

	if (!self->style)
	{
		self->s.frame = 0;
	}
	else
	{
		self->s.frame = 1;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_flagonpole (0.3 0.3 1.0) (-8.0 -8.0 0.0) (8.0 8.0 60.0)
 *
 * Heretic 2: Flag on pole
 */
void
SP_obj_flagonpole(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.sound = gi.soundindex("ambient/bannerflap.wav");
	object_spawn(self);
}

/*
 * QUAKED obj_lever1 (0.3 0.3 1.0) (-6.0 -14.0 -17.0) (6.0 14.0 17.0)
 *
 * Heretic 2: Floor lever
 */
void
SP_obj_lever1(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_lever2 (0.3 0.3 1.0) (-14.0 -14.0 -9.0) (14.0 14.0 9.0)
 *
 * Heretic 2: Wooden wheel lever
 */
void
SP_obj_lever2(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_lever3 (0.3 0.3 1.0) (-4.0 -6.0 -16.0) (4.0 6.0 16.0)
 *
 * Heretic 2: Wall lever
 */
void
SP_obj_lever3(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_bush1 (0.3 0.3 1.0) (-34.0 -34.0 -19.0) (34.0 34.0 19.0)
 *
 * Heretic 2: Bush
 */
/*
 * QUAKED obj_bush2 (0.3 0.3 1.0) (-56.0 -56.0 -40.0) (56.0 56.0 40.0)
 *
 * Heretic 2: Bush 2
 */
void
SP_obj_bush(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cactus (0.3 0.3 1.0) (-18.0 -18.0 -44.0) (18.0 18.0 44.0)
 *
 * Heretic 2: Cactus
 */
void
SP_obj_cactus(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cactus3 (0.3 0.3 1.0) (-14.0 -14.0 -32.0) (18.0 18.0 32.0)
 *
 * Heretic 2: Cactus 3
 */
void
SP_obj_cactus3(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cactus4 (0.3 0.3 1.0) (-11.0 -11.0 -11.0) (11.0 11.0 11.0)
 *
 * Heretic 2: Cactus 4 (triggerable)
 */
void
SP_obj_cactus4(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_swampflat_bottom (0.3 0.3 1.0) (0.0 -100.0 -48.0) (2.0 100.0 48.0)
 *
 * Heretic 2: A flat poly to be used on the outer edge of swamp levels. Vegetation
 * growing down, can't be moved and damaged.
 */
void
SP_obj_swampflat_bottom(edict_t *self)
{
	self->s.skinnum = 1;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_treestump (0.3 0.3 1.0) (-18.0 -18.0 -16.0) (18.0 18.0 16.0)
 *
 * Heretic 2: Short tree stump, can't be moved and damaged.
 */
void
SP_obj_treestump(edict_t *self)
{
	self->s.skinnum = 1;
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_jawbone (0.3 0.3 1.0) (-8.0 -8.0 -8.0) (8.0 8.0 8.0)
 *
 * Heretic 2: Fish jawbone
 */
void
SP_obj_jawbone(edict_t *self)
{
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_barrel_metal (0.3 0.3 1.0) (-11.0 -12.0 -18.0) (11.0 12.0 18.0)
 *
 * Heretic 2: Metal barrel
 */
void
SP_obj_barrel_metal(edict_t *self)
{
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_barrel_exploding (.5 .5 .5) (-13.0 -13.0 0.0) (13.0 13.0 36.0)
 *
 * Hexen 2: Barrel exploding
 */
void
SP_obj_barrel_exploding(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_barrel_indestructible (.5 .5 .5) (-13.0 -13.0 0.0) (13.0 13.0 36.0)
 *
 * Hexen 2: Barrel indestructible
 */
void
SP_obj_barrel_indestructible(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_barrel_explosive (0.3 0.3 1.0) (-11.0 -12.0 -18.0) (11.0 12.0 18.0)
 *
 * Heretic 2: Explosive barrel. Use barrel with spawnflags 4 instead
 */
void
SP_obj_barrel_explosive(edict_t *self)
{
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_gascan (0.3 0.3 1.0) (-8.0 -9.0 -13.0) (8.0 9.0 13.0)
 *
 * Heretic 2: Gas can
 */
void
SP_obj_gascan(edict_t *self)
{
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_pipe1 (0.3 0.3 1.0) (-11.0 -24.0 -7.0) (11.0 24.0 7.0)
 *
 * Heretic 2: Pipe (90  turn)
 */
/*
 * QUAKED obj_pipe2 (0.3 0.3 1.0) (-6.0 -25.0 -4.0) (6.0 25.0 4.0)
 *
 * Heretic 2: Pipe (straight)
 */
void
SP_obj_pipe(edict_t *self)
{
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_pipewheel (0.3 0.3 1.0) (-14.0 -14.0 -12.0) (14.0 14.0 12.0)
 *
 * Heretic 2: Pipe shutoff valve
 */
void
SP_obj_pipewheel(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	self->s.skinnum = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_minecart (0.3 0.3 1.0) (-18.0 -29.0 -20.0) (18.0 29.0 20.0)
 *
 * Heretic 2: Full minecart
 */
/*
 * QUAKED obj_minecart2 (0.3 0.3 1.0) (-18.0 -29.0 -20.0) (18.0 29.0 20.0)
 *
 * Heretic 2: Empty minecart
 */
/*
 * QUAKED obj_minecart3 (0.3 0.3 1.0) (-18.0 -29.0 -20.0) (18.0 29.0 20.0)
 *
 * Heretic 2: Busted minecart
 */
void
SP_obj_minecart(edict_t *self)
{
	if (!strcmp(self->classname, "obj_minecart2"))
	{
		/* end of "polympty" */;
		self->s.frame = 20;
	}
	else if (!strcmp(self->classname, "obj_minecart3"))
	{
		/* end of "wrecked" */;
		self->s.frame = 40;
	}
	else
	{
		/* end of "polyfull" */;
		self->s.frame = 0;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_metalchunk1 (0.3 0.3 1.0) (-10.0 -26.0 -4.0) (10.0 26.0 4.0)
 *
 * Heretic 2: Metal chunk
 */
/*
 * QUAKED obj_metalchunk2 (0.3 0.3 1.0) (-10.0 -26.0 -4.0) (10.0 26.0 4.0)
 *
 * Heretic 2: Metal chunk
 */
/*
 * QUAKED obj_metalchunk3 (0.3 0.3 1.0) (-10.0 -26.0 -4.0) (10.0 26.0 4.0)
 *
 * Heretic 2: Metal chunk
 */
void
SP_obj_metalchunk(edict_t *self)
{
	if (!strcmp(self->classname, "obj_metalchunk2"))
	{
		self->s.frame = 1;
	}
	else if (!strcmp(self->classname, "obj_metalchunk3"))
	{
		self->s.frame = 2;
	}
	else
	{
		self->s.frame = 0;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_hivepriestessssymbol (0.3 1.0 0) (-4.0 -13.0 4.0) (4.0 13.0 shadow)
 *
 * Heretic 2: Hive Priestess symbol (triggered)
 */
void
SP_obj_hivepriestessssymbol(edict_t *self)
{
	self->spawnflags |= OBJ_NODAMAGE; // can't be destroyed
	SP_obj_material(self);
}

/*
 * QUAKED obj_shrine (0.3 0.3 1.0) (-26.0 -38.0 -38.0) (26.0 38.0 38.0)
 *
 * Heretic 2: Shrine model. Target with a matching shrine_(style) trigger brush.
 */
void
SP_obj_shrine(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_cocoonopen (0.3 0.3 1.0) (-8.0 -8.0 -8.0) (8.0 8.0 8.0)
 *
 * Heretic 2: Hanging cocoon
 */
void
SP_obj_cocoonopen(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	self->s.frame = 20;
	SP_obj_material(self);
}

/*
 * QUAKED obj_venusflytrap (0.3 0.3 1.0) (-20.0 -20.0 -24.0) (20.0 20.0 24.0)
 *
 * Heretic 2: Venus flytrap
 */
void
SP_obj_venusflytrap(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_techeckriktomb (0.3 0.3 1.0) (-41.0 -11.0 -14.0) (41.0 11.0 14.0)
 *
 * Heretic 2: Tchekrik laying down (triggerable)
 */
void
SP_obj_statue_techeckriktomb(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 1;
	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_techeckrikright (0.3 0.3 1.0) (-26.0 -40.0 -50.0) (26.0 40.0 50.0)
 *
 * Heretic 2: Tcheckrik statue right (triggerable)
 */
void
SP_obj_statue_techeckrikright(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 2;

	if (self->spawnflags & 16)
	{
		self->s.frame = 3;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_techeckrikleft (0.3 0.3 1.0) (-26.0 -40.0 -50.0) (26.0 40.0 50.0)
 *
 * Heretic 2: Tcheckrik statue left (triggerable)
 */
void
SP_obj_statue_techeckrikleft(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 0;

	if (self->spawnflags & 16)
	{
		self->s.frame = 1;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_spellbook (0.3 0.3 1.0) (-14.0 -14.0 -35.0) (14.0 14.0 40.0)
 *
 * Heretic 2: Spell book that closes when triggered
 */
void
SP_obj_spellbook(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 20;
	SP_obj_material(self);
}

/*
 * QUAKED obj_torture_table (0.3 0.3 1.0) (-46.0 -14.0 -14.0) (46.0 14.0 14.0)
 *
 * Heretic 2: Torture table
 */
void
SP_obj_torture_table(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	if (self->style < 2)
	{
		self->s.frame = self->style;
	}
	else
	{
		self->s.frame = 0;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_tchecktrik_bust (0.3 0.3 1.0) (-8.0 -12.0 -15.0) (8.0 12.0 15.0)
 *
 * Heretic 2: A bust of a tchecktrik.  When used a necklace appears around it's neck.
 */
void
SP_obj_statue_tchecktrik_bust(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	if (!self->style)
	{
		self->s.frame = 1;
	}
	else
	{
		self->s.frame = 0;
	}

	SP_obj_material(self);
}

/*
 * QUAKED obj_statue_sithraguard (0.3 0.3 1.0) (-22.0 -20.0 -57.0) (22.0 20.0 57.0)
 *
 * Heretic 2: A statue of a sithra guard with spear extended.
 * When used he gains a shield and pulls his arm back. Used with: item_puzzle_shield
 */
void
SP_obj_statue_sithraguard(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 0;
	SP_obj_material(self);
}

/*
 * QUAKED obj_torture_ironmaiden (0.3 0.3 1.0) (-18.0 -18.0 -49.0) (18.0 18.0 49.0)
 *
 * Heretic 2: Iron maiden (closes when used), can't be moved and damaged.
 */
void
SP_obj_torture_ironmaiden(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	self->s.frame = 0;

	SP_obj_material(self);
}

/*
 * QUAKED obj_biotank (0.3 0.3 1.0) (-20.0 -33.0 -52.0) (20.0 33.0 52.0)
 *
 * Heretic 2: Bio Tank, can't be moved and damaged.
 */
void
SP_obj_biotank(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_hangingdude (0.3 0.3 1.0) (-3.0 -20.0 -55.0) (3.0 20.0 55.0)
 *
 * Heretic 2: Hanging dude, can't be moved and damaged.
 */
void
SP_obj_hangingdude(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_hanging_ogle (0.3 0.3 1.0) (-8.0 -16.0 -34.0) (8.0 16.0 34.0)
 *
 * Heretic 2: Hanging Ogle, animated, can't be moved and damaged.
 */
void
SP_obj_hanging_ogle(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_web (0.3 0.3 1.0) (-2.0 -18.0 -20.0) (2.0 18.0 20.0)
 *
 * Heretic 2: Cobweb
 */
void
SP_obj_web(edict_t *self)
{
	self->s.renderfx |= RF_TRANSLUCENT;
	SP_obj_material(self);
}

/*
 * QUAKED obj_larva (0.3 0.3 1.0) (-8.0 -8.0 -2.0) (8.0 8.0 2.0)
 *
 * Heretic 2: Squirming larva
 */
void
SP_obj_larva(edict_t *self)
{
	SP_obj_material(self);
}

/*
 * QUAKED obj_bloodsplat (0.3 0.3 1.0) (-8.0 -8.0 -8.0) (8.0 8.0 8.0)
 *
 * Heretic 2: Blood splat
 */
void
SP_obj_bloodsplat(edict_t *self)
{
	SP_obj_material(self);
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
	M_SetAnimGroupFrame(self, "bigfire", false);
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

/*
 * QUAKED obj_fishhead1 (0.3 0.3 1.0) (0 -76 -86) (136 76 86)
 *
 * Heretic2: Fish head fountain, can't be moved and damaged.
 */
void
SP_obj_fishhead1(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE | OBJ_NODAMAGE;
	SP_obj_material(self);
}

/*
 * QUAKED obj_fishhead2 (0.3 0.3 1.0) (0 -110 -118) (136 110 118)
 *
 * Heretic2: Fish head fountain 2
 */
void
SP_obj_fishhead2(edict_t *self)
{
	self->spawnflags |= OBJ_STOPMOVE;
	self->takedamage = DAMAGE_NO;
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->clipmask = MASK_MONSTERSOLID;
	gi.linkentity(self);
}
