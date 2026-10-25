/*
 * Copyright (C) 2026
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include "../../header/local.h"
#include "grunt2.h"

#define REP2(x) x, x
#define REP4(x) x, x, x, x
#define REP6(x) x, x, x, x, x, x
#define REP8(x) x, x, x, x, x, x, x, x
#define REP10(x) x, x, x, x, x, x, x, x, x, x
#define REP12(x) x, x, x, x, x, x, x, x, x, x, x, x
#define REP14(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define REP16(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define REP20(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define REP22(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define G2_STAND {ai_stand, 0, NULL}
#define G2_JOG {ai_run, 8, NULL}
#define G2_RUN {ai_run, 16, NULL}
#define G2_CHARGE {ai_charge, 0, NULL}
#define G2_SHOT {ai_charge, 0, grunt2_fire}
#define G2_PAIN {ai_move, 0, NULL}
#define G2_DEATH {ai_move, 0, NULL}
#define G2_DUCK {ai_move, 0, NULL}

static int sound_idle[3];
static int sound_pain[3];
static int sound_death[2];
static int sound_sight[3];

static void grunt2_fire(edict_t *self);
static void grunt2_dead(edict_t *self);
static void grunt2_duck_down(edict_t *self);
static void grunt2_duck_up(edict_t *self);
static void grunt2_idle_select(edict_t *self);

void grunt2_stand(edict_t *self);
void grunt2_run(edict_t *self);
void grunt2_attack(edict_t *self);
void grunt2_dodge(edict_t *self, edict_t *other, float eta, trace_t *tr);
void grunt2_pain(edict_t *self, edict_t *other, float kick, int damage);
void grunt2_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);
void grunt2_idle(edict_t *self);
void grunt2_sight(edict_t *self, edict_t *other);
qboolean grunt2_checkattack(edict_t *self);

static mframe_t grunt2_frames_stand[] = {
	REP16(G2_STAND), REP4(G2_STAND), G2_STAND
};
mmove_t grunt2_move_stand = {
	FRAME_stand1, FRAME_stand21, grunt2_frames_stand, grunt2_idle_select
};
static mframe_t grunt2_frames_idle_a[] = {
	REP16(G2_STAND), REP4(G2_STAND), REP2(G2_STAND), G2_STAND
};
mmove_t grunt2_move_idle_a = {
	FRAME_idlea1, FRAME_idlea23, grunt2_frames_idle_a, grunt2_stand
};
static mframe_t grunt2_frames_idle_b[] = {
	REP16(G2_STAND), REP8(G2_STAND)
};
mmove_t grunt2_move_idle_b = {
	FRAME_idleb1, FRAME_idleb24, grunt2_frames_idle_b, grunt2_stand
};
static mframe_t grunt2_frames_earpiece[] = {
	REP16(G2_STAND), REP8(G2_STAND)
};
mmove_t grunt2_move_earpiece = {
	FRAME_earpiece1, FRAME_earpiece24, grunt2_frames_earpiece, grunt2_stand
};
static mframe_t grunt2_frames_walk[] = { REP8(G2_JOG), REP2(G2_JOG) };
mmove_t grunt2_move_walk = {
	FRAME_walk1, FRAME_walk10, grunt2_frames_walk, grunt2_run
};
static mframe_t grunt2_frames_walkfire[] = {
	G2_CHARGE, G2_CHARGE, G2_CHARGE, G2_SHOT, REP6(G2_CHARGE)
};
mmove_t grunt2_move_walkfire = {
	FRAME_walkshoot1, FRAME_walkshoot10, grunt2_frames_walkfire, grunt2_run
};
static mframe_t grunt2_frames_run[] = { REP6(G2_RUN) };
mmove_t grunt2_move_run = {
	FRAME_run1, FRAME_run6, grunt2_frames_run, grunt2_run
};
static mframe_t grunt2_frames_runfire[] = {
	G2_CHARGE, G2_CHARGE, G2_SHOT, REP2(G2_CHARGE), G2_CHARGE
};
mmove_t grunt2_move_runfire = {
	FRAME_runshoot1, FRAME_runshoot6, grunt2_frames_runfire, grunt2_run
};
static mframe_t grunt2_frames_duck[] = {
	{ai_move, 0, grunt2_duck_down}, REP8(G2_DUCK),
	{ai_move, 0, grunt2_duck_up}, G2_DUCK
};
mmove_t grunt2_move_duck = {
	FRAME_duck1, FRAME_duck11, grunt2_frames_duck, grunt2_run
};
static mframe_t grunt2_frames_jump[] = { REP10(G2_RUN) };
mmove_t grunt2_move_jump = {
	FRAME_jumpingjack1, FRAME_jumpingjack10, grunt2_frames_jump, grunt2_run
};
static mframe_t grunt2_frames_attack_a[] = {
	G2_CHARGE, G2_CHARGE, G2_CHARGE, G2_SHOT, REP10(G2_CHARGE)
};
mmove_t grunt2_move_attack_a = {
	FRAME_shoota1, FRAME_shoota14, grunt2_frames_attack_a, grunt2_run
};
static mframe_t grunt2_frames_attack_b[] = {
	G2_CHARGE, G2_CHARGE, G2_CHARGE, G2_SHOT, REP10(G2_CHARGE)
};
mmove_t grunt2_move_attack_b = {
	FRAME_shootb1, FRAME_shootb14, grunt2_frames_attack_b, grunt2_run
};
static mframe_t grunt2_frames_pain_a[] = { REP12(G2_PAIN) };
mmove_t grunt2_move_pain_a = {
	FRAME_paina1, FRAME_paina12, grunt2_frames_pain_a, grunt2_run
};
static mframe_t grunt2_frames_pain_b[] = { REP22(G2_PAIN) };
mmove_t grunt2_move_pain_b = {
	FRAME_painb1, FRAME_painb22, grunt2_frames_pain_b, grunt2_run
};
static mframe_t grunt2_frames_death_a[] = { REP4(G2_DEATH), G2_DEATH };
mmove_t grunt2_move_death_a = {
	FRAME_deatha1, FRAME_deatha5, grunt2_frames_death_a, grunt2_dead
};
static mframe_t grunt2_frames_death_b[] = { REP6(G2_DEATH) };
mmove_t grunt2_move_death_b = {
	FRAME_deathb1, FRAME_deathb6, grunt2_frames_death_b, grunt2_dead
};
static mframe_t grunt2_frames_death_heavy[] = { REP8(G2_DEATH), REP2(G2_DEATH) };
mmove_t grunt2_move_death_heavy = {
	FRAME_raildeath1, FRAME_raildeath10, grunt2_frames_death_heavy, grunt2_dead
};

static qboolean
grunt2_is_attacking(const edict_t *self)
{
	const mmove_t *move = self->monsterinfo.currentmove;
	return move == &grunt2_move_walkfire || move == &grunt2_move_runfire ||
			move == &grunt2_move_attack_a || move == &grunt2_move_attack_b;
}

static void
grunt2_fire(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
	{
		return;
	}
	vec3_t start, end, direction;
	VectorCopy(self->s.origin, start);
	start[2] += 32;
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, direction);
	VectorNormalize(direction);
	monster_fire_blaster(self, start, direction,
			28 + (int)(8 * skill->value * random()), 1200,
			MZ2_TANK_BLASTER_1, EF_TRACKER);
}

static void
grunt2_dead(edict_t *self)
{
	VectorSet(self->mins, -10, -23, -25);
	VectorSet(self->maxs, 12, 22, -8);
	monster_dynamic_dead(self);
}

static void
grunt2_idle_select(edict_t *self)
{
	float choice = random();
	self->monsterinfo.currentmove = choice < 0.34f ? &grunt2_move_idle_a :
			(choice < 0.67f ? &grunt2_move_idle_b : &grunt2_move_earpiece);
}

void
grunt2_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &grunt2_move_stand;
}

void
grunt2_run(edict_t *self)
{
	if (!self->enemy || (self->monsterinfo.aiflags & AI_STAND_GROUND))
	{
		grunt2_stand(self);
		return;
	}
	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	self->monsterinfo.currentmove = VectorLength(delta) > 300 ?
			&grunt2_move_run : &grunt2_move_walk;
}

qboolean
grunt2_checkattack(edict_t *self)
{
	if (!self->enemy || skill->value * random() <= 1.8f)
	{
		return false;
	}
	self->monsterinfo.attack_state = AS_MISSILE;
	return true;
}

void
grunt2_attack(edict_t *self)
{
	if (!self->enemy)
	{
		grunt2_run(self);
		return;
	}
	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	float distance = VectorLength(delta);
	if (!infront(self, self->enemy))
	{
		self->ideal_yaw = vectoyaw(delta);
		M_ChangeYaw(self);
		self->monsterinfo.currentmove = random() < 0.5f ?
				&grunt2_move_attack_a : &grunt2_move_attack_b;
	}
	else if (distance > 300)
	{
		self->monsterinfo.currentmove = &grunt2_move_runfire;
	}
	else if (distance <= 150)
	{
		self->monsterinfo.currentmove = random() < 0.5f ?
				&grunt2_move_attack_a : &grunt2_move_attack_b;
	}
	else
	{
		self->monsterinfo.currentmove = random() < 0.5f ?
				&grunt2_move_walkfire :
				(random() < 0.5f ? &grunt2_move_attack_a :
				&grunt2_move_attack_b);
	}
}

static void
grunt2_duck_down(edict_t *self)
{
	self->maxs[2] = 24;
	gi.linkentity(self);
}

static void
grunt2_duck_up(edict_t *self)
{
	self->maxs[2] = 52;
	gi.linkentity(self);
}

void
grunt2_dodge(edict_t *self, edict_t *other, float eta, trace_t *tr)
{
	(void)other;
	(void)eta;
	(void)tr;
	if (random() >= 0.25f)
	{
		return;
	}
	if (skill->value <= SKILL_EASY)
	{
		self->monsterinfo.currentmove = &grunt2_move_duck;
	}
	else if (random() < 0.35f * (skill->value - SKILL_EASY))
	{
		grunt2_attack(self);
	}
	else
	{
		self->monsterinfo.currentmove = &grunt2_move_duck;
	}
}

void
grunt2_idle(edict_t *self)
{
	if (random() < 0.08f)
	{
		gi.sound(self, CHAN_VOICE, sound_idle[randk() % 3], 1, ATTN_IDLE, 0);
	}
}

void
grunt2_sight(edict_t *self, edict_t *other)
{
	(void)other;
	gi.sound(self, CHAN_VOICE, sound_sight[randk() % 3], 1, ATTN_NORM, 0);
}

void
grunt2_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;
	if (grunt2_is_attacking(self) || level.time < self->pain_debounce_time)
	{
		return;
	}
	self->pain_debounce_time = level.time + 3;
	gi.sound(self, CHAN_VOICE, sound_pain[randk() % 3], 1, ATTN_NORM, 0);
	if (skill->value >= SKILL_HARDPLUS)
	{
		return;
	}
	self->monsterinfo.currentmove = damage > 50 ?
			&grunt2_move_pain_b : &grunt2_move_pain_a;
}

void
grunt2_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point)
{
	(void)inflictor;
	(void)attacker;
	(void)point;
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"),
				1, ATTN_NORM, 0);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	if (self->deadflag == DEAD_DEAD)
	{
		return;
	}
	gi.sound(self, CHAN_VOICE, sound_death[randk() & 1], 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = damage > 50 ?
			&grunt2_move_death_heavy :
			(random() < 0.5f ? &grunt2_move_death_a : &grunt2_move_death_b);
	self->deadflag = DEAD_DYING;
	self->svflags |= SVF_DEADMONSTER;
}

void
SP_monster_grunt2(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}
	sound_idle[0] = gi.soundindex("Monsters/grunt2/g2idlng1.wav");
	sound_idle[1] = gi.soundindex("Monsters/grunt2/g2idlng2.wav");
	sound_idle[2] = gi.soundindex("Monsters/grunt2/g2idlng3.wav");
	sound_pain[0] = gi.soundindex("Monsters/grunt2/g2pain1.wav");
	sound_pain[1] = gi.soundindex("Monsters/grunt2/g2pain2.wav");
	sound_pain[2] = gi.soundindex("Monsters/grunt2/g2pain3.wav");
	sound_death[0] = gi.soundindex("Monsters/grunt2/g2death1.wav");
	sound_death[1] = gi.soundindex("Monsters/grunt2/g2death2.wav");
	sound_sight[0] = gi.soundindex("Monsters/grunt2/g2act1.wav");
	sound_sight[1] = gi.soundindex("Monsters/grunt2/g2act2.wav");
	sound_sight[2] = gi.soundindex("Monsters/grunt2/g2act3.wav");

	self->s.modelindex = gi.modelindex("models/monsters/grunt2/tris.md2");
	VectorSet(self->mins, -10, -23, -25);
	VectorSet(self->maxs, 12, 22, 52);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = self->max_health = 180;
	self->gib_health = -60;
	self->mass = 100;
	self->svflags |= SVF_MONSTER;
	self->takedamage = DAMAGE_AIM;
	self->pain = grunt2_pain;
	self->die = grunt2_die;
	self->monsterinfo.stand = grunt2_stand;
	self->monsterinfo.walk = grunt2_run;
	self->monsterinfo.run = grunt2_run;
	self->monsterinfo.attack = grunt2_attack;
	self->monsterinfo.dodge = grunt2_dodge;
	self->monsterinfo.sight = grunt2_sight;
	self->monsterinfo.idle = grunt2_idle;
	self->monsterinfo.checkattack = grunt2_checkattack;
	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);
	walkmonster_start(self);
}

#undef G2_STAND
#undef G2_JOG
#undef G2_RUN
#undef G2_CHARGE
#undef G2_SHOT
#undef G2_PAIN
#undef G2_DEATH
#undef G2_DUCK
#undef REP22
#undef REP20
#undef REP16
#undef REP14
#undef REP12
#undef REP10
#undef REP8
#undef REP6
#undef REP4
#undef REP2
