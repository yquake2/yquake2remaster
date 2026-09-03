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
 * Kigrax hover.
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "kigrax.h"

static vec3_t kigrax_plasma_offset = {16.0f, 0.0f, -16.0f};

static void kigrax_stand(edict_t *self);
static void kigrax_run(edict_t *self);
static void kigrax_melee(edict_t *self);
static void kigrax_dead(edict_t *self);
static void kigrax_strike1(edict_t *self);
static void kigrax_strike2(edict_t *self);
static void kigrax_fire_plasma(edict_t *self);

static mframe_t kigrax_frames_scan[] = {
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL},
	{ai_stand, 0.0f, NULL}
};

static mmove_t kigrax_move_scan = {
	FRAME_standidle1,
	FRAME_standidle21,
	kigrax_frames_scan,
	NULL
};

static mframe_t kigrax_frames_walk1[] = {
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL}
};
static mmove_t kigrax_move_walk1 = {
	FRAME_walk1,
	FRAME_walk22,
	kigrax_frames_walk1,
	NULL
};

static mframe_t kigrax_frames_walk2[] = {
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 4.0f, NULL}
};
static mmove_t kigrax_move_walk2 = {
	FRAME_walkidle1,
	FRAME_walkidle22,
	kigrax_frames_walk2,
	NULL
};

static mframe_t kigrax_frames_sight[] = {
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL},
	{ai_run, 10.0f, NULL}
};
static mmove_t kigrax_move_sight = {
	FRAME_sight1,
	FRAME_sight17,
	kigrax_frames_sight,
	NULL
};

static mframe_t kigrax_frames_run[] = {
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 15.0f, NULL}
};

static mmove_t kigrax_move_run = {
	FRAME_run1,
	FRAME_run17,
	kigrax_frames_run,
	NULL
};

static mframe_t kigrax_frames_pain[] = {
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL}
};
static mmove_t kigrax_move_pain = {
	FRAME_pain1,
	FRAME_pain11,
	kigrax_frames_pain,
	kigrax_run
};

static mframe_t kigrax_frames_death[] = {
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, kigrax_dead},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL}
};
static mmove_t kigrax_move_death = {
	FRAME_death1,
	FRAME_death19,
	kigrax_frames_death,
	kigrax_dead
};

static mframe_t kigrax_frames_melee1[] = {
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, kigrax_strike1},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, kigrax_strike1},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL}
};
static mmove_t kigrax_move_melee1 = {
	FRAME_claw1,
	FRAME_claw15,
	kigrax_frames_melee1,
	kigrax_melee
};

static mframe_t kigrax_frames_melee2[] = {
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, kigrax_strike2},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL},
	{ai_charge, 1.0f, NULL}
};
static mmove_t kigrax_move_melee2 = {
	FRAME_rake1,
	FRAME_rake11,
	kigrax_frames_melee2,
	kigrax_melee
};

static mframe_t kigrax_frames_attack[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, kigrax_fire_plasma},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL}
};

static mmove_t kigrax_move_attack = {
	FRAME_blaster1,
	FRAME_blaster10,
	kigrax_frames_attack,
	kigrax_run
};

static int sound_pain;
static int sound_death;
static int sound_sight;
static int sound_search1;
static int sound_search2;
static int sound_attack;
static int sound_idle;

static void
kigrax_stand(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		return;
	}

	if (crandk() <= 0.33)
	{
		self->monsterinfo.currentmove = &kigrax_move_scan;
	}
	else
	{
		monster_dynamic_stand(self);
	}
}

static void
kigrax_walk(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (crandk() < 0.33)
	{
		self->monsterinfo.currentmove = &kigrax_move_walk2;
	}
	else
	{
		self->monsterinfo.currentmove = &kigrax_move_walk1;
	}
}

static void
kigrax_run(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		monster_dynamic_stand(self);
		return;
	}

	self->monsterinfo.currentmove = &kigrax_move_run;
}

static void
kigrax_search(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (crandk() < 0.5)
	{
		gi.sound(self, CHAN_VOICE, sound_search1, 1.0f, ATTN_NORM, 0.0f);
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_search2, 1.0f, ATTN_NORM, 0.0f);
	}
}

static void
kigrax_sight(edict_t *self, edict_t *other)
{
	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);
	self->monsterinfo.currentmove = &kigrax_move_sight;
}

static void
kigrax_attack(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &kigrax_move_attack;
}

static void
kigrax_melee(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->enemy->health <= 0)
	{
		self->monsterinfo.currentmove = &kigrax_move_run;
		return;
	}

	if (ai_range(self, self->enemy) != RANGE_MELEE)
	{
		self->monsterinfo.currentmove = &kigrax_move_run;
		return;
	}

	if (crandk() < 0.1)
	{
		self->monsterinfo.currentmove = &kigrax_move_run;
		return;
	}

	if (crandk() < 0.9)
	{
		self->monsterinfo.currentmove = &kigrax_move_melee1;
	}
	else
	{
		self->monsterinfo.currentmove = &kigrax_move_melee2;
	}
}

static void
kigrax_strike1(edict_t *self)
{
	vec3_t	aim;

	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_attack, 1.0f, ATTN_NORM, 0.0f);
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 10.0f);
	fire_hit(self, aim, 10 + (randk() % 6), 100);
}

static void
kigrax_strike2(edict_t *self)
{
	vec3_t	aim;

	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_attack, 1.0f, ATTN_NORM, 0.0f);
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 10.0f);
	fire_hit(self, aim, 20 + (randk() % 20), 100);
}

static void
kigrax_fire_plasma(edict_t *self)
{
	vec3_t forward, right, start, target, dir;

	if (!self)
	{
		return;
	}

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, kigrax_plasma_offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;

	VectorSubtract(target, start, dir);

	fire_plasma_bolt(self, start, dir, 10, 1000,
		1);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(1);
	gi.multicast(start, MULTICAST_PVS);
}

static void
kigrax_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (!self)
	{
		return;
	}

	if (level.time < self->pain_debounce_time)
	{
		return;
	}

	self->pain_debounce_time = level.time + 3.0;

	if (skill->value == SKILL_HARDPLUS)
	{
		return;
	}

	gi.sound(self, CHAN_VOICE, sound_pain, 1.0f, ATTN_NORM, 0.0f);
	self->monsterinfo.currentmove = &kigrax_move_pain;
}

static void
kigrax_dead(edict_t *self)
{
	if (!self)
	{
		return;
	}

	VectorSet(self->mins, -16.0f, -16.0f, -16.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 0.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;
	gi.linkentity(self);
}

static void
kigrax_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, const vec3_t point)
{
	if (!self)
	{
		return;
	}

	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

	if (meansOfDeath == MOD_BRAINTENTACLE)
	{
		BecomeExplosion1(self);
		return;
	}

	if (self->health <= self->gib_health)
	{
		int n;

		gi.sound(self, CHAN_VOICE, gi.soundindex(
						"misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 2; n++)
		{
			ThrowGib(self,
					"models/objects/gibs/bone/tris.md2",
					damage,
					GIB_ORGANIC);
		}

		for (n = 0; n < 2; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		}

		ThrowHead(self,
				"models/objects/gibs/sm_meat/tris.md2",
				damage,
				GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
	{
		return;
	}

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &kigrax_move_death;
}

void
SP_monster_kigrax(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/kigrax/tris.md2");

	sound_pain = gi.soundindex("hover/hovpain1.wav");
	sound_death = gi.soundindex("hover/hovdeth1.wav");
	sound_sight = gi.soundindex("hover/hovsght1.wav");
	sound_search1 = gi.soundindex("hover/hovsrch1.wav");
	sound_search2 = gi.soundindex("hover/hovsrch2.wav");
	sound_attack = gi.soundindex("chick/chkatck3.wav");
	gi.soundindex("kigrax/hovatck1.wav");
	sound_idle = gi.soundindex("kigrax/hovidle1.wav");

	self->s.sound = sound_idle;

	VectorSet(self->mins, -20.0f, -20.0f, -32.0f);
	VectorSet(self->maxs, 20.0f, 20.0f, 12.0f);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = 200;
	self->gib_health = -100;
	self->mass = 150;
	self->yaw_speed = 20;
	self->viewheight = 90;

	self->pain = kigrax_pain;
	self->die = kigrax_die;

	self->monsterinfo.stand = kigrax_stand;
	self->monsterinfo.idle = kigrax_stand;
	self->monsterinfo.walk = kigrax_walk;
	self->monsterinfo.run = kigrax_run;
	self->monsterinfo.attack = kigrax_attack;
	self->monsterinfo.melee = kigrax_melee;
	self->monsterinfo.sight = kigrax_sight;
	self->monsterinfo.search = kigrax_search;
	self->monsterinfo.scale = MODEL_SCALE;

	gi.linkentity(self);

	monster_dynamic_stand(self);
	flymonster_start(self);
}
