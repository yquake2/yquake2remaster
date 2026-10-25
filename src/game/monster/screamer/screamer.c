/*
 * Copyright (C) 2026
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include "../../header/local.h"
#include "screamer.h"

#define REP2(x) x, x
#define REP4(x) x, x, x, x
#define REP8(x) x, x, x, x, x, x, x, x
#define REP16(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define REP32(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define SF_STAND {ai_stand, 0, NULL}
#define SF_MOVE {ai_run, 8, NULL}
#define SF_CHARGE {ai_charge, 0, NULL}
#define SF_ATTACK {ai_charge, 0, screamer_scream}
#define SF_MELEE {ai_charge, 0, screamer_strike}
#define SF_WALL_IDLE {ai_move, 0, NULL}
#define SF_PAIN {ai_move, 0, NULL}
#define SF_DEATH {ai_move, 0, NULL}

static int idle_sounds[3];
static int pain_sounds[2];
static int death_sound;
static int sight_sounds[2];
static int melee_sound;
static int scream_sound;

void screamer_stand(edict_t *self);
void screamer_run(edict_t *self);
void screamer_attack(edict_t *self);
static void screamer_scream(edict_t *self);
static void screamer_strike(edict_t *self);
static void screamer_jump(edict_t *self);
void screamer_touch(edict_t *self, edict_t *other,
		const cplane_t *plane, const csurface_t *surf);
void screamer_pain(edict_t *self, edict_t *other, float kick, int damage);
void screamer_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);
static void screamer_dead(edict_t *self);
static void screamer_wall_next(edict_t *self);
void screamer_idle(edict_t *self);
void screamer_sight(edict_t *self, edict_t *other);
qboolean screamer_checkattack(edict_t *self);

static mframe_t screamer_frames_stand[] = { REP16(SF_STAND), REP16(SF_STAND), REP4(SF_STAND), SF_STAND, SF_STAND };
mmove_t screamer_move_stand = {
	FRAME_standa1, FRAME_standc16, screamer_frames_stand, NULL
};
static mframe_t screamer_frames_idle[] = { REP16(SF_STAND), REP4(SF_STAND), REP2(SF_STAND) };
mmove_t screamer_move_idle = {
	FRAME_idlea1, FRAME_idleb10, screamer_frames_idle, NULL
};
static mframe_t screamer_frames_run[] = { REP4(SF_MOVE), REP2(SF_MOVE) };
mmove_t screamer_move_run = {
	FRAME_run1, FRAME_run6, screamer_frames_run, NULL
};
static mframe_t screamer_frames_swim_a[] = { REP4(SF_MOVE), REP2(SF_MOVE) };
mmove_t screamer_move_swim_a = {
	FRAME_swim1, FRAME_swim6, screamer_frames_swim_a, NULL
};
static mframe_t screamer_frames_swim_b[] = { REP4(SF_MOVE), REP2(SF_MOVE) };
mmove_t screamer_move_swim_b = {
	FRAME_swimb1, FRAME_swimb6, screamer_frames_swim_b, NULL
};
static mframe_t screamer_frames_scream[] = {
	SF_ATTACK, REP8(SF_CHARGE), REP4(SF_CHARGE), SF_CHARGE
};
mmove_t screamer_move_scream = {
	FRAME_scream1, FRAME_scream14, screamer_frames_scream, screamer_run
};
static mframe_t screamer_frames_runattack[] = {
	SF_CHARGE, SF_CHARGE, SF_MELEE, SF_CHARGE, SF_CHARGE, SF_CHARGE
};
mmove_t screamer_move_runattack = {
	FRAME_runattack1, FRAME_runattack6, screamer_frames_runattack, screamer_run
};
static mframe_t screamer_frames_jump[] = {
	REP8(SF_MOVE), REP4(SF_MOVE), REP2(SF_MOVE), SF_MOVE
};
mmove_t screamer_move_jump = {
	FRAME_jump1, FRAME_jump15, screamer_frames_jump, screamer_run
};
static mframe_t screamer_frames_jumpattack[] = {
	REP8(SF_CHARGE), REP4(SF_CHARGE), REP2(SF_CHARGE), SF_MELEE
};
mmove_t screamer_move_jumpattack = {
	FRAME_jumpattack1, FRAME_jumpattack15, screamer_frames_jumpattack, screamer_run
};
static mframe_t screamer_frames_wall_crawl[] = { REP4(SF_MOVE), REP2(SF_MOVE) };
mmove_t screamer_move_wall_crawl = {
	FRAME_wallcrawl1, FRAME_wallcrawl6, screamer_frames_wall_crawl, screamer_wall_next
};
static mframe_t screamer_frames_wall_pause[] = {
	REP8(SF_WALL_IDLE), REP2(SF_WALL_IDLE)
};
mmove_t screamer_move_wall_pause = {
	FRAME_wallidle1, FRAME_wallidle10, screamer_frames_wall_pause, screamer_wall_next
};
static mframe_t screamer_frames_underidle[] = {
	REP16(SF_STAND), REP4(SF_STAND), REP2(SF_STAND)
};
mmove_t screamer_move_underidle = {
	FRAME_subidle1, FRAME_subidle14, screamer_frames_underidle, NULL
};
static mframe_t screamer_frames_underattack[] = {
	SF_ATTACK, REP8(SF_CHARGE), REP4(SF_CHARGE)
};
mmove_t screamer_move_underattack = {
	FRAME_subscream1, FRAME_subscream13, screamer_frames_underattack, screamer_run
};
static mframe_t screamer_frames_undermelee[] = {
	REP4(SF_CHARGE), SF_MELEE, REP2(SF_CHARGE), SF_CHARGE
};
mmove_t screamer_move_undermelee = {
	FRAME_subattack1, FRAME_subattack8, screamer_frames_undermelee, screamer_run
};
static mframe_t screamer_frames_pain[] = {
	REP16(SF_PAIN), REP8(SF_PAIN), REP4(SF_PAIN), REP2(SF_PAIN), REP2(SF_PAIN)
};
mmove_t screamer_move_pain = {
	FRAME_paina1, FRAME_painb20, screamer_frames_pain, screamer_run
};
static mframe_t screamer_frames_underpain[] = {
	REP16(SF_PAIN), REP8(SF_PAIN), REP2(SF_PAIN)
};
mmove_t screamer_move_underpain = {
	FRAME_subpaina1, FRAME_subpainb13, screamer_frames_underpain, screamer_run
};
static mframe_t screamer_frames_pain_heavy[] = {
	REP16(SF_PAIN), REP4(SF_PAIN)
};
mmove_t screamer_move_pain_heavy = {
	FRAME_painb1, FRAME_painb20, screamer_frames_pain_heavy, screamer_run
};
static mframe_t screamer_frames_death1[] = {
	REP4(SF_DEATH), REP4(SF_DEATH)
};
mmove_t screamer_move_death1 = {
	FRAME_deatha1, FRAME_deatha8, screamer_frames_death1, screamer_dead
};
static mframe_t screamer_frames_death2[] = {
	REP8(SF_DEATH), REP4(SF_DEATH), REP2(SF_DEATH), SF_DEATH
};
mmove_t screamer_move_death2 = {
	FRAME_deathb1, FRAME_deathb13, screamer_frames_death2, screamer_dead
};
static mframe_t screamer_frames_death3[] = {
	REP4(SF_DEATH), REP2(SF_DEATH)
};
mmove_t screamer_move_death3 = {
	FRAME_deathc1, FRAME_deathc6, screamer_frames_death3, screamer_dead
};

void
screamer_idle(edict_t *self)
{
	self->monsterinfo.currentmove = self->waterlevel > 1 ?
			&screamer_move_underidle : &screamer_move_idle;
	if (random() < 0.08f)
	{
		gi.sound(self, CHAN_VOICE, idle_sounds[randk() % 3], 1, ATTN_IDLE, 0);
	}
}

void
screamer_stand(edict_t *self)
{
	self->monsterinfo.currentmove = self->waterlevel > 1 ?
			&screamer_move_underidle : &screamer_move_stand;
}

void
screamer_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = self->waterlevel > 1 ?
				&screamer_move_underidle : &screamer_move_stand;
		return;
	}
	self->monsterinfo.currentmove = self->waterlevel > 1 ?
			(random() < 0.5f ? &screamer_move_swim_a : &screamer_move_swim_b) :
			&screamer_move_run;
}

qboolean
screamer_checkattack(edict_t *self)
{
	if (!self->enemy || skill->value * random() <= 1.8f)
	{
		return false;
	}
	self->monsterinfo.attack_state = AS_MISSILE;
	return true;
}

void
screamer_attack(edict_t *self)
{
	if (!self->enemy)
	{
		return;
	}
	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	float distance = VectorLength(delta);

	if (self->waterlevel > 1)
	{
		if (distance > 120 && self->enemy->waterlevel > 1 &&
				random() < 0.68f)
		{
			self->monsterinfo.currentmove = &screamer_move_underattack;
		}
		else if (self->enemy->waterlevel <= 1)
		{
			self->monsterinfo.currentmove = &screamer_move_jump;
			screamer_jump(self);
		}
		else
		{
			self->monsterinfo.currentmove = &screamer_move_undermelee;
		}
		return;
	}

	if (distance > 180 && random() < 0.68f)
	{
		self->monsterinfo.currentmove = &screamer_move_scream;
	}
	else if (distance < 100)
	{
		self->monsterinfo.currentmove = &screamer_move_runattack;
	}
	else
	{
		self->monsterinfo.currentmove = &screamer_move_jump;
		screamer_jump(self);
	}
}

static void
screamer_scream(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
	{
		return;
	}
	vec3_t start, end, direction;
	VectorCopy(self->s.origin, start);
	start[2] += 24;
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, direction);
	VectorNormalize(direction);
	monster_fire_blaster(self, start, direction,
			3 + (int)(8 * skill->value * random()), 450,
			MZ2_TANK_BLASTER_1, EF_TRACKER);
	gi.sound(self, CHAN_WEAPON, scream_sound, 1, ATTN_NORM, 0);
}

static void
screamer_strike(edict_t *self)
{
	if (!self->enemy)
	{
		return;
	}
	static vec3_t aim_left = {MELEE_DISTANCE, -10, 0};
	static vec3_t aim_right = {MELEE_DISTANCE, 10, 0};
	gi.sound(self, CHAN_WEAPON, melee_sound, 1, ATTN_NORM, 0);
	int damage = 12 + randk() % 5;
	fire_hit(self, aim_left, damage, -50);
	fire_hit(self, aim_right, damage, -50);
}

static void
screamer_jump(edict_t *self)
{
	if (!self->enemy)
	{
		return;
	}
	vec3_t forward;
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorScale(forward, 600, self->velocity);
	self->velocity[2] = 250;
	self->flags &= ~FL_FLY;
	self->touch = screamer_touch;
}

void
screamer_touch(edict_t *self, edict_t *other, const cplane_t *plane,
		const csurface_t *surf)
{
	(void)surf;
	if (other && other->takedamage && VectorLength(self->velocity) > 400)
	{
		vec3_t direction;
		VectorCopy(self->velocity, direction);
		VectorNormalize(direction);
		T_Damage(other, self, self, direction, other->s.origin,
				vec3_origin, 40 + (int)(10 * random()), 0, 0, MOD_UNKNOWN);
		self->velocity[0] *= -0.4f;
		self->velocity[1] *= -0.4f;
		self->velocity[2] = 100;
		self->flags &= ~FL_FLY;
		self->touch = NULL;
		return;
	}

	if (plane && plane->normal[2] < 0.7f && self->style != 1 &&
			self->count <= 3)
	{
		self->style = 1;
		self->flags |= FL_FLY;
		self->velocity[0] = self->velocity[1] = self->velocity[2] = 0;
		self->monsterinfo.currentmove = &screamer_move_wall_crawl;
	}
	else if (plane && plane->normal[2] > 0.7f)
	{
		self->touch = NULL;
		self->flags &= ~FL_FLY;
		self->monsterinfo.currentmove = &screamer_move_jumpattack;
	}
}

static void
screamer_wall_next(edict_t *self)
{
	if (self->count > 3)
	{
		self->style = 0;
		self->flags &= ~FL_FLY;
		self->touch = NULL;
		screamer_run(self);
		return;
	}

	float choice = random();
	if (choice < 0.34f)
	{
		self->velocity[2] = 0;
		self->monsterinfo.currentmove = &screamer_move_wall_pause;
	}
	else if (choice < 0.67f)
	{
		vec3_t forward;
		AngleVectors(self->s.angles, forward, NULL, NULL);
		VectorScale(forward, 120, self->velocity);
		self->monsterinfo.currentmove = &screamer_move_wall_crawl;
	}
	else
	{
		if (++self->count > 3)
		{
			self->style = 0;
			self->flags &= ~FL_FLY;
			self->touch = NULL;
			screamer_run(self);
			return;
		}
		self->style = 0;
		self->monsterinfo.currentmove = &screamer_move_jump;
		screamer_jump(self);
	}
}

void
screamer_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;
	if (self->style == 1 || self->monsterinfo.currentmove == &screamer_move_scream ||
			self->monsterinfo.currentmove == &screamer_move_runattack ||
			self->monsterinfo.currentmove == &screamer_move_jumpattack ||
			self->monsterinfo.currentmove == &screamer_move_underattack ||
			self->monsterinfo.currentmove == &screamer_move_undermelee ||
			self->monsterinfo.currentmove == &screamer_move_jump ||
			self->monsterinfo.currentmove == &screamer_move_pain ||
			self->monsterinfo.currentmove == &screamer_move_underpain ||
			level.time < self->pain_debounce_time || skill->value >= SKILL_HARDPLUS)
	{
		return;
	}
	self->pain_debounce_time = level.time + 3;
	gi.sound(self, CHAN_VOICE, pain_sounds[randk() & 1], 1, ATTN_NORM, 0);
	if (self->waterlevel > 1)
	{
		self->monsterinfo.currentmove = &screamer_move_underpain;
	}
	else if (damage > 50 && random() < 0.5f)
	{
		self->monsterinfo.currentmove = &screamer_move_pain_heavy;
	}
	else
	{
		self->monsterinfo.currentmove = &screamer_move_pain;
	}
}

static void
screamer_dead(edict_t *self)
{
	VectorSet(self->mins, -20, -20, -25);
	VectorSet(self->maxs, 20, 20, -8);
	monster_dynamic_dead(self);
}

void
screamer_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
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
	gi.sound(self, CHAN_VOICE, death_sound, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = random() < 0.33f ?
			&screamer_move_death1 : (random() < 0.5f ?
			&screamer_move_death2 : &screamer_move_death3);
	self->deadflag = DEAD_DYING;
	self->svflags |= SVF_DEADMONSTER;
}

void
screamer_sight(edict_t *self, edict_t *other)
{
	(void)other;
	gi.sound(self, CHAN_VOICE, sight_sounds[randk() & 1], 1, ATTN_NORM, 0);
}

void
SP_monster_screamer(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}
	idle_sounds[0] = gi.soundindex("Monsters/Screamer/sidling1.wav");
	idle_sounds[1] = gi.soundindex("Monsters/Screamer/sidling2.wav");
	idle_sounds[2] = gi.soundindex("Monsters/Screamer/sidling3.wav");
	pain_sounds[0] = gi.soundindex("Monsters/Screamer/spain1.wav");
	pain_sounds[1] = gi.soundindex("Monsters/Screamer/spain2.wav");
	death_sound = gi.soundindex("Monsters/Screamer/sdeath1.wav");
	sight_sounds[0] = gi.soundindex("Monsters/Screamer/sact1.wav");
	sight_sounds[1] = gi.soundindex("Monsters/Screamer/sact2.wav");
	melee_sound = gi.soundindex("Monsters/Screamer/smelee.wav");
	scream_sound = gi.soundindex("Monsters/Screamer/ssscream.wav");

	self->s.modelindex = gi.modelindex("models/monsters/screamer/tris.md2");
	VectorSet(self->mins, -20, -20, -25);
	VectorSet(self->maxs, 20, 20, 38);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = self->max_health = 180;
	self->gib_health = -50;
	self->mass = 200;
	self->flags |= FL_SWIM;
	self->svflags |= SVF_MONSTER;
	self->takedamage = DAMAGE_AIM;
	self->pain = screamer_pain;
	self->die = screamer_die;
	self->touch = screamer_touch;
	self->monsterinfo.stand = screamer_stand;
	self->monsterinfo.walk = screamer_run;
	self->monsterinfo.run = screamer_run;
	self->monsterinfo.attack = screamer_attack;
	self->monsterinfo.melee = screamer_attack;
	self->monsterinfo.sight = screamer_sight;
	self->monsterinfo.idle = screamer_idle;
	self->monsterinfo.checkattack = screamer_checkattack;
	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);
	swimmonster_start(self);
}

#undef SF_STAND
#undef SF_MOVE
#undef SF_ATTACK
#undef SF_MELEE
#undef SF_WALL_IDLE
#undef SF_JUMP
#undef SF_PAIN
#undef SF_DEATH
#undef REP32
#undef REP16
#undef REP8
#undef REP4
#undef REP2
