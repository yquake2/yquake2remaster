/*
 * Copyright (C) 2026
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include "../../header/local.h"
#include "alienship.h"

#define AFRAME {alienship_move, 10, NULL}
#define AFRAME_BIRTH {ai_move, 0, NULL}

static int sound_sight;
static int sound_idle1;
static int sound_idle2;
static int sound_pain1;
static int sound_pain2;
static int sound_death;
static int sound_attack;

void alienship_stand(edict_t *self);
void alienship_birth(edict_t *self);
void alienship_run(edict_t *self);
void alienship_attack(edict_t *self);
void alienship_idle(edict_t *self);
qboolean alienship_checkattack(edict_t *self);
static void alienship_move(edict_t *self, float dist);
static void alienship_fire(edict_t *self);
void alienship_pain(edict_t *self, edict_t *other, float kick, int damage);
void alienship_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);

static mframe_t alienship_frames_birth[] = {
	AFRAME_BIRTH, AFRAME_BIRTH, AFRAME_BIRTH, AFRAME_BIRTH,
	AFRAME_BIRTH, AFRAME_BIRTH, AFRAME_BIRTH, AFRAME_BIRTH
};
mmove_t alienship_move_birth = {
	FRAME_birth1, FRAME_birth8, alienship_frames_birth, alienship_run
};
static mframe_t alienship_frames_flight[] = {
	AFRAME, AFRAME, AFRAME, AFRAME, AFRAME
};
mmove_t alienship_move_flight = {
	FRAME_wiggle1, FRAME_wiggle5, alienship_frames_flight, NULL
};

static mframe_t alienship_frames_attack[] = {
	{ai_charge, 0, alienship_fire},
	{ai_charge, 0, alienship_fire},
	{ai_charge, 0, alienship_fire},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t alienship_move_attack = {
	FRAME_fire1, FRAME_fire5, alienship_frames_attack, alienship_run
};

void
alienship_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &alienship_move_flight;
}

void
alienship_birth(edict_t *self)
{
	self->monsterinfo.currentmove = &alienship_move_birth;
}

void
alienship_run(edict_t *self)
{
	self->monsterinfo.stand = alienship_stand;
	self->monsterinfo.currentmove = &alienship_move_flight;
}

void
alienship_idle(edict_t *self)
{
	int snd = random() < 0.5f ? sound_idle1 : sound_idle2;
	gi.sound(self, CHAN_VOICE, snd, 1, ATTN_IDLE, 0);
}

qboolean
alienship_checkattack(edict_t *self)
{
	if (!self->enemy || skill->value * random() <= 1.8f)
	{
		return false;
	}

	self->monsterinfo.attack_state = AS_MISSILE;
	return true;
}

static void
alienship_move(edict_t *self, float dist)
{
	if (self->enemy)
	{
		vec3_t delta;
		VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
		if (VectorLength(delta) < 180)
		{
			ai_run(self, -18);
			return;
		}
	}

	ai_run(self, dist);
}

static mframe_t alienship_frames_pain[] = {
	{ai_move, 0, NULL}, {ai_move, 0, NULL},
	{ai_move, 0, NULL}, {ai_move, 0, NULL}
};
mmove_t alienship_move_pain = {
	FRAME_wiggle1, FRAME_wiggle4, alienship_frames_pain, alienship_run
};

void
alienship_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;
	(void)damage;
	gi.sound(self, CHAN_VOICE,
		 random() < 0.5f ? sound_pain1 : sound_pain2, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &alienship_move_pain;
}

static void
alienship_fire(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
	{
		return;
	}

	vec3_t start, aim, dir;
	VectorCopy(self->s.origin, start);
	start[2] += 12;
	VectorCopy(self->enemy->s.origin, aim);
	aim[2] += self->enemy->viewheight;
	VectorSubtract(aim, start, dir);
	VectorNormalize(dir);
	monster_fire_blaster(self, start, dir,
			3 + (int)(8 * skill->value * random()), 880,
			MZ2_TANK_BLASTER_1, EF_BLASTER);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

void
alienship_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point)
{
	(void)inflictor;
	(void)attacker;
	(void)point;
	if (self->deadflag == DEAD_DEAD)
	{
		return;
	}
	static const char *gibs[] = {
		"models/monsters/shipgib/shipgib1.md2",
		"models/monsters/shipgib/shipgib2.md2",
		"models/monsters/shipgib/shipgib3.md2",
		"models/monsters/shipgib/shipgib4.md2",
		"models/monsters/shipgib/shipgib5.md2"
	};
	for (size_t i = 0; i < sizeof(gibs) / sizeof(gibs[0]); ++i)
	{
		ThrowGib(self, gibs[i], damage, GIB_METALLIC);
	}
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->solid = SOLID_BBOX;
	self->svflags |= SVF_DEADMONSTER;
	self->monsterinfo.run = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.currentmove = &alienship_move_pain;
}

void
alienship_sight(edict_t *self, edict_t *other)
{
	(void)other;
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void
SP_monster_alienship1(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_sight = gi.soundindex("monsters/aliens1/asact1.wav");
	sound_idle1 = gi.soundindex("monsters/aliens1/asidle1.wav");
	sound_idle2 = gi.soundindex("monsters/aliens1/asidle2.wav");
	sound_pain1 = gi.soundindex("monsters/aliens1/aspain1.wav");
	sound_pain2 = gi.soundindex("monsters/aliens1/aspain2.wav");
	sound_death = gi.soundindex("monsters/aliens1/asdeath.wav");
	sound_attack = gi.soundindex("weapons/aswhip1.wav");

	self->s.modelindex = gi.modelindex(
			(self->spawnflags & 8) ?
			"models/monsters/aliens1/tris2.md2" :
			"models/monsters/aliens1/tris.md2");
	VectorSet(self->mins, -34, -25, -13);
	VectorSet(self->maxs, 45, 43, 41);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = 35;
	self->max_health = 35;
	self->gib_health = -40;
	self->mass = 50;
	self->flags |= FL_FLY;
	self->svflags |= SVF_MONSTER;
	self->takedamage = DAMAGE_AIM;
	self->pain = alienship_pain;
	self->die = alienship_die;
	self->monsterinfo.stand = alienship_birth;
	self->monsterinfo.walk = alienship_run;
	self->monsterinfo.run = alienship_run;
	self->monsterinfo.attack = alienship_attack;
	self->monsterinfo.sight = alienship_sight;
	self->monsterinfo.idle = alienship_idle;
	self->monsterinfo.checkattack = alienship_checkattack;
	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);
	flymonster_start(self);
}

void
alienship_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &alienship_move_attack;
}

#undef AFRAME
#undef AFRAME_BIRTH
