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
 * Oblivion Badass.
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "badass.h"

#define BADASS_SPAWNFLAG_DEAD 8

typedef struct
{
	const char *model;
	vec3_t mins;
	vec3_t maxs;
} badass_gib_def_t;

static const badass_gib_def_t badass_gib_defs[] = {
	{
		"models/monsters/badass/gib_torso.md2",
		{-17.0f, -15.0f, -50.0f},
		{ 41.0f,  22.0f, -29.0f}
	},
	{
		"models/monsters/badass/gib_lleg.md2",
		{-35.0f,   1.0f, -46.0f},
		{ 47.0f,  53.0f, -22.0f}
	},
	{
		"models/monsters/badass/gib_rleg.md2",
		{-34.0f, -51.0f, -44.0f},
		{ 48.0f,  -1.0f, -21.0f}
	},
	{
		"models/monsters/badass/gib_larm.md2",
		{-29.0f, -12.0f, -33.0f},
		{ 31.0f,  53.0f, -12.0f}
	},
	{
		"models/monsters/badass/gib_rarm.md2",
		{-34.0f, -51.0f, -42.0f},
		{ 26.0f, -30.0f, -21.0f}
	}
};

static int sound_pain;
static int sound_death;
static int sound_step;
static int sound_sight;

static vec3_t badass_rocket_offsets[2] = {
	{18.0f,  40.0f, 0.0f},
	{18.0f, -40.0f, 0.0f}
};

static void badass_idle(edict_t *self);
static void badass_run(edict_t *self);
static void badass_attack(edict_t *self);
static void badass_attack_loop(edict_t *self);
static void badass_rocket_right(edict_t *self);
static void badass_rocket_left(edict_t *self);
static void badass_gib_explosion(edict_t *self);
static void badass_die_gibs(edict_t *self);
static void badass_dead(edict_t *self);
static void badass_step(edict_t *self);
static void badass_thud(edict_t *self);

static mframe_t badass_frames_idle_closed[] = {
	{ai_stand, 0.0f, NULL}
};
static mmove_t badass_move_idle_closed = {
	FRAME_activate1,
	FRAME_activate1,
	badass_frames_idle_closed,
	NULL
};

static mframe_t badass_frames_walk[] = {
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, badass_step},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, NULL},
	{ai_walk, 7.0f, badass_step}
};
static mmove_t badass_move_walk = {
	FRAME_walk1,
	FRAME_walk14,
	badass_frames_walk,
	NULL
};

static mframe_t badass_frames_run[] = {
	{ai_run, 14.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 21.0f, NULL},
	{ai_run, 24.0f, badass_step},
	{ai_run, 14.0f, NULL},
	{ai_run, 15.0f, NULL},
	{ai_run, 21.0f, NULL},
	{ai_run, 24.0f, badass_step}
};
static mmove_t badass_move_run = {
	FRAME_run1,
	FRAME_run8,
	badass_frames_run,
	NULL
};

static mframe_t badass_frames_attack[] = {
	{ai_charge, -5.0f, badass_rocket_right},
	{ai_charge, 0.0f, NULL},
	{ai_charge, -5.0f, badass_rocket_left},
	{ai_charge, 0.0f, NULL}
};
static mmove_t badass_move_attack = {
	FRAME_attack1,
	FRAME_attack4,
	badass_frames_attack,
	badass_attack_loop
};

static mframe_t badass_frames_pain[] = {
	{ai_move, 8.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -8.0f, NULL},
	{ai_move, 0.0f, NULL}
};
static mmove_t badass_move_pain = {
	FRAME_pain1,
	FRAME_pain10,
	badass_frames_pain,
	badass_run
};

static mframe_t badass_frames_death[] = {
	{ai_move, -8.0f, badass_gib_explosion},
	{ai_move, -8.0f, NULL},
	{ai_move, -8.0f, NULL},
	{ai_move, -7.0f, NULL},
	{ai_move, -4.0f, badass_thud},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, badass_gib_explosion},
	{ai_move, 4.0f, NULL},
	{ai_move, 2.0f, NULL},
	{ai_move, 2.0f, NULL},
	{ai_move, 2.0f, NULL},
	{ai_move, 2.0f, NULL},
	{ai_move, 2.0f, NULL},
	{ai_move, 2.0f, badass_thud},
	{ai_move, 0.0f, badass_gib_explosion},
	{ai_move, 0.0f, badass_thud},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, badass_thud}
};
static mmove_t badass_move_death = {
	FRAME_death1,
	FRAME_death20,
	badass_frames_death,
	badass_dead
};

static void badass_sight(edict_t *self, edict_t *other)
{
	if (self->monsterinfo.currentmove == &badass_move_idle_closed)
	{
		monster_dynamic_action(self, "activate", 0);
	}
	else
	{
		badass_run(self);
	}

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0);
}

static void
badass_stand(edict_t *self)
{
	if (self->monsterinfo.currentmove != &badass_move_idle_closed)
	{
		monster_dynamic_stand(self);
	}
}

static void
badass_idle(edict_t *self)
{
	if (self->monsterinfo.action &&
		!strcmp(self->monsterinfo.action, "stand"))
	{
		monster_dynamic_action(self, "deactivate", 0);
		return;
	}

	self->monsterinfo.currentmove = &badass_move_idle_closed;
}

static void
badass_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &badass_move_walk;
}

static void
badass_run(edict_t *self)
{
	self->monsterinfo.currentmove = &badass_move_run;
}

static void
badass_attack(edict_t *self)
{
	vec3_t delta;

	if (!self->enemy)
	{
		return;
	}

	VectorSubtract(self->s.origin, self->enemy->s.origin, delta);
	if ((VectorLength(delta) > 200.0f) && (crandk() < 0.5f))
	{
		return;
	}

	self->monsterinfo.currentmove = &badass_move_attack;
}

static void
badass_attack_loop(edict_t *self)
{
	if (self->enemy && visible(self, self->enemy) && (self->enemy->health > 0))
	{
		vec3_t delta;

		VectorSubtract(self->s.origin, self->enemy->s.origin, delta);
		if (VectorLength(delta) < 200.0f)
		{
			return;
		}

		if (crandk() <= 0.45f)
		{
			return;
		}
	}

	self->monsterinfo.currentmove = &badass_move_run;
}

static void
badass_rocket_right(edict_t *self)
{
	vec3_t forward, right, start, dir, target;

	if (!self->enemy)
	{
		return;
	}

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, badass_rocket_offsets[0], forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	monster_fire_rocket(self, start, dir, 50, 550, MZ2_CARRIER_ROCKET_1);
}

static void
badass_rocket_left(edict_t *self)
{
	vec3_t forward, right, start, dir, target;

	if (!self->enemy)
	{
		return;
	}

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, badass_rocket_offsets[1], forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	monster_fire_rocket(self, start, dir, 50, 550, MZ2_CARRIER_ROCKET_2);
}

static void
badass_gib_think(edict_t *self)
{
	self->nextthink = level.time + 0.1f;
	if (self->count == 4)
	{
		self->nextthink = 0;
		return;
	}

	self->count++;
}

static void
badass_gib_explosion(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	T_RadiusDamage(self, self, 100.0f, NULL, 100.0f, MOD_EXPLOSIVE);

	if (self->think == badass_gib_explosion)
	{
		self->think = badass_die_gibs;
		self->nextthink = level.time + 0.5f;
	}
}

static void
badass_die_gibs(edict_t *self)
{
	vec3_t forward, right, up;
	size_t i;

	AngleVectors(self->s.angles, forward, right, up);

	for (i = 0; i < ARRLEN(badass_gib_defs); ++i)
	{
		edict_t *gib = G_Spawn();

		VectorCopy(self->s.origin, gib->s.origin);
		VectorCopy(self->s.origin, gib->s.old_origin);
		VectorCopy(self->s.angles, gib->s.angles);
		VectorCopy(self->s.angles, gib->avelocity);
		VectorCopy(self->rrs.scale, gib->rrs.scale);
		VectorScale(gib->avelocity, 200.0f, gib->avelocity);
		gib->mass = 0;
		gib->avelocity[ROLL] = 0.0f;
		gib->avelocity[YAW] = 0.0f;
		gib->owner = self;
		gib->movetype = MOVETYPE_BOUNCE;
		gib->solid = SOLID_BBOX;
		gib->deadflag = DEAD_DEAD;
		gib->svflags |= SVF_DEADMONSTER;
		gib->think = badass_gib_think;
		gib->nextthink = level.time + 0.1f;
		gib->avelocity[YAW] = ((crandk() - 0.5f) * 2.0f) * 200.0f;

		gib->s.modelindex = gi.modelindex(badass_gib_defs[i].model);
		VectorCopy(badass_gib_defs[i].mins, gib->mins);
		VectorCopy(badass_gib_defs[i].maxs, gib->maxs);

		switch (i)
		{
		case 0:
			VectorScale(forward, crandk() * -100.0f, gib->velocity);
			VectorMA(gib->velocity, 300.0f, up, gib->velocity);
			break;

		case 1:
			VectorMA(gib->velocity, 200.0f, up, gib->velocity);
			VectorMA(gib->velocity, -200.0f, right, gib->velocity);
			break;

		case 2:
			VectorMA(gib->velocity, 200.0f, up, gib->velocity);
			VectorMA(gib->velocity, 200.0f, right, gib->velocity);
			break;

		case 3:
			VectorScale(forward, -200.0f, gib->velocity);
			VectorMA(gib->velocity, crandk() * 275.0f + 50.0f, up, gib->velocity);
			VectorMA(gib->velocity, -100.0f, right, gib->velocity);
			break;

		case 4:
			VectorScale(forward, -200.0f, gib->velocity);
			VectorMA(gib->velocity, crandk() * 300.0f, up, gib->velocity);
			VectorMA(gib->velocity, 50.0f, right, gib->velocity);
			break;
		}

		gi.linkentity(gib);
	}

	VectorAdd(self->s.origin, right, self->s.origin);
	badass_gib_explosion(self);
	VectorMA(self->s.origin, -2.0f, right, self->s.origin);
	badass_gib_explosion(self);
	G_FreeEdict(self);
}

static void
badass_dead(edict_t *self)
{
	self->deadflag = DEAD_DEAD;
	VectorSet(self->mins, -44.0f, -62.0f, -64.0f);
	VectorSet(self->maxs, 44.0f, 62.0f, -5.0f);
	self->svflags |= SVF_DEADMONSTER;
	self->think = badass_gib_explosion;
	self->nextthink = level.time + 0.5f;
	gi.linkentity(self);
}

static void
badass_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage,
	const vec3_t point)
{
	if (self->deadflag != DEAD_DEAD)
	{
		self->deadflag = DEAD_DEAD;
		self->takedamage = DAMAGE_YES;
		self->monsterinfo.currentmove = &badass_move_death;
	}
}

static void
badass_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1.0f, ATTN_NORM, 0);
}

static void
badass_thud(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_death, 1.0f, ATTN_NORM, 0);
}

static void
badass_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
	{
		self->s.skinnum |= 1;
	}

	if ((damage > 20) && (self->pain_debounce_time <= level.time))
	{
		if ((damage < 51) && (crandk() > 0.2f))
		{
			return;
		}

		self->monsterinfo.currentmove = &badass_move_pain;
		self->pain_debounce_time = level.time + 3.0f;
		gi.sound(self, CHAN_VOICE, sound_pain, 1.0f, ATTN_NORM, 0);
	}
}

/*
 * QUAKED monster_badass (1 .5 0) (-52.0 -40.0 -64.0) (38.0 40.0 32.0)
 *
 * Oblivion Badass
 */
void
SP_monster_badass(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->yaw_speed = 25.0f;
	self->gib_health = 0;

	if (self->spawnflags & BADASS_SPAWNFLAG_DEAD)
	{
		self->s.skinnum |= 1;
		self->health = -1;
		self->deadflag = DEAD_DEAD;
		VectorSet(self->mins, -31.0f, -88.0f, -64.0f);
		VectorSet(self->maxs, 38.0f, 21.0f, -13.0f);
		self->svflags |= SVF_DEADMONSTER;
		self->nextthink = 0;
		gi.linkentity(self);
		return;
	}

	sound_pain = gi.soundindex("tank/tnkpain2.wav");
	sound_death = gi.soundindex("tank/tnkdeth2.wav");
	sound_step = gi.soundindex("tank/step.wav");
	sound_sight = gi.soundindex("tank/sight1.wav");

	self->pain = badass_pain;
	self->die = badass_die;

	self->monsterinfo.stand = badass_stand;
	self->monsterinfo.idle = badass_idle;
	self->monsterinfo.walk = badass_walk;
	self->monsterinfo.run = badass_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = badass_attack;
	self->monsterinfo.melee = badass_attack;
	self->monsterinfo.sight = badass_sight;

	gi.linkentity(self);
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
	self->monsterinfo.currentmove = &badass_move_idle_closed;
}
