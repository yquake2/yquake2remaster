/*
 * Copyright (C) 2026
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include "../../header/local.h"
#include "grunt1.h"

#define REP2(x) x, x
#define REP4(x) x, x, x, x
#define REP8(x) x, x, x, x, x, x, x, x
#define REP16(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define REP32(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x
#define GF_STAND {ai_stand, 0, NULL}
#define GF_MOVE {grunt_move_step, 8, NULL}
#define GF_FAST {grunt_move_step, 16, NULL}
#define GF_ATTACK {ai_charge, 0, grunt_shoot}
#define GF_MELEE {ai_charge, 0, grunt_strike}
#define GF_CHARGE {ai_charge, 0, NULL}
#define GF_PAIN {ai_move, 0, NULL}
#define GF_DEATH {ai_move, 0, NULL}

static int idle_sounds[2];
static int sight_sounds[4];
static int pain_sounds[3];
static int death_sounds[2];
static int melee_sound;
static int gib_sound;
static int headshot_sound;

typedef struct
{
	int health;
	int skin;
	int melee_min;
	int bullet_damage;
	qboolean shotgun;
} grunt_variant_t;

static const grunt_variant_t grunt_types[] = {
	{60, 0, 10, 2, false},
	{80, 1, 15, 2, true}
};

static int grunt_variant(const edict_t *self)
{
	return self->style == 1 ? 1 : 0;
}

void grunt_idle(edict_t *self);
void grunt_stand(edict_t *self);
void grunt_run(edict_t *self);
void grunt_attack(edict_t *self);
void grunt_melee(edict_t *self);
void grunt_sight(edict_t *self, edict_t *other);
void grunt_pain(edict_t *self, edict_t *other, float kick, int damage);
void grunt_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);
qboolean grunt_checkattack(edict_t *self);
static void grunt_shoot(edict_t *self);
static void grunt_strike(edict_t *self);
static void grunt_move_step(edict_t *self, float dist);
static void grunt_dead(edict_t *self);

static mframe_t grunt_frames_idle[] = {
	REP16(GF_STAND)
};
mmove_t grunt_move_idle = {
	FRAME_idlea1, FRAME_idlea16, grunt_frames_idle, grunt_stand
};
static mframe_t grunt_frames_idle_b[] = {
	REP16(GF_STAND), REP8(GF_STAND), REP4(GF_STAND), GF_STAND
};
mmove_t grunt_move_idle_b = {
	FRAME_idleb1, FRAME_idleb29, grunt_frames_idle_b, grunt_stand
};
static mframe_t grunt_frames_idle_c[] = {
	REP16(GF_STAND), REP4(GF_STAND)
};
mmove_t grunt_move_idle_c = {
	FRAME_idlec1, FRAME_idlec20, grunt_frames_idle_c, grunt_stand
};
static mframe_t grunt_frames_idle_d[] = {
	REP16(GF_STAND), REP4(GF_STAND), GF_STAND
};
mmove_t grunt_move_idle_d = {
	FRAME_idled1, FRAME_idled21, grunt_frames_idle_d, grunt_stand
};
static mframe_t grunt_frames_jog[] = {
	REP8(GF_MOVE), REP4(GF_MOVE), REP2(GF_MOVE)
};
mmove_t grunt_move_jog = {
	FRAME_walk1, FRAME_walk14, grunt_frames_jog, grunt_run
};
static mframe_t grunt_frames_run[] = {
	REP8(GF_FAST), REP2(GF_FAST)
};
mmove_t grunt_move_run = {
	FRAME_run1, FRAME_run10, grunt_frames_run, grunt_run
};
static mframe_t grunt_frames_attack_run[] = {
	REP8(GF_ATTACK), REP2(GF_ATTACK)
};
mmove_t grunt_move_attack_run = {
	FRAME_runshoot1, FRAME_runshoot10, grunt_frames_attack_run, grunt_run
};
static mframe_t grunt_frames_attack_mounted[] = {
	REP16(GF_ATTACK)
};
mmove_t grunt_move_attack_mounted = {
	FRAME_attacka1, FRAME_attacka16, grunt_frames_attack_mounted, grunt_run
};
static mframe_t grunt_frames_attack_hand[] = {
	REP16(GF_ATTACK), GF_ATTACK
};
mmove_t grunt_move_attack_hand = {
	FRAME_attackb1, FRAME_attackb17, grunt_frames_attack_hand, grunt_run
};
static mframe_t grunt_frames_attack_melee[] = {
	REP8(GF_CHARGE), GF_MELEE, REP4(GF_CHARGE)
};
mmove_t grunt_move_attack_melee = {
	FRAME_attackc1, FRAME_attackc13, grunt_frames_attack_melee, grunt_run
};
static mframe_t grunt_frames_pain[] = {
	REP32(GF_PAIN), REP8(GF_PAIN), GF_PAIN
};
mmove_t grunt_move_pain = {
	FRAME_paina1, FRAME_painc12, grunt_frames_pain, grunt_run
};
static mframe_t grunt_frames_death1[] = {
	REP8(GF_DEATH), REP4(GF_DEATH)
};
mmove_t grunt_move_death1 = {
	FRAME_deatha1, FRAME_deatha12, grunt_frames_death1, grunt_dead
};
static mframe_t grunt_frames_death2[] = {
	REP16(GF_DEATH), REP2(GF_DEATH)
};
mmove_t grunt_move_death2 = {
	FRAME_deathb1, FRAME_deathb18, grunt_frames_death2, grunt_dead
};
static mframe_t grunt_frames_headshot[] = {
	REP8(GF_DEATH), REP4(GF_DEATH), GF_DEATH
};
mmove_t grunt_move_headshot = {
	FRAME_nohead1, FRAME_nohead13, grunt_frames_headshot, grunt_dead
};

void
grunt_idle(edict_t *self)
{
	if (random() < 0.08f)
	{
		gi.sound(self, CHAN_VOICE, idle_sounds[randk() & 1], 1, ATTN_IDLE, 0);
	}
}

void
grunt_stand(edict_t *self)
{
	float choice = random();
	self->monsterinfo.currentmove = choice < 0.25f ? &grunt_move_idle :
			(choice < 0.5f ? &grunt_move_idle_b :
			(choice < 0.75f ? &grunt_move_idle_c : &grunt_move_idle_d));
}

void
grunt_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		grunt_stand(self);
		return;
	}

	if (!self->enemy)
	{
		grunt_stand(self);
		return;
	}
	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	float distance = VectorLength(delta);
	self->monsterinfo.currentmove = distance <= 300 ?
			&grunt_move_jog : &grunt_move_run;
}

static void
grunt_move_step(edict_t *self, float dist)
{
	if (self->enemy)
	{
		vec3_t delta;
		VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
		if (VectorLength(delta) > 300 && random() < 0.4f)
		{
			self->ideal_yaw = anglemod(self->s.angles[YAW] +
					(random() < 0.5f ? -12.0f : 12.0f));
			M_ChangeYaw(self);
			ai_move(self, dist);
			return;
		}
	}
	ai_run(self, dist);
}

qboolean
grunt_checkattack(edict_t *self)
{
	if (!self->enemy || skill->value * random() <= 1.8f)
	{
		return false;
	}

	self->monsterinfo.attack_state = AS_MISSILE;
	return true;
}

void
grunt_attack(edict_t *self)
{
	if (!self->enemy)
	{
		grunt_run(self);
		return;
	}

	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	float distance = VectorLength(delta);
	if (distance < MELEE_DISTANCE)
	{
		self->monsterinfo.currentmove = &grunt_move_attack_melee;
		return;
	}

	if (distance > 300)
	{
		self->monsterinfo.currentmove = random() < 0.68f ?
				&grunt_move_attack_run : &grunt_move_attack_hand;
	}
	else
	{
		self->monsterinfo.currentmove = random() < 0.5f ?
				&grunt_move_attack_mounted : &grunt_move_attack_hand;
	}
}

static void
grunt_shoot(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
	{
		return;
	}

	const grunt_variant_t *variant = &grunt_types[grunt_variant(self)];
	vec3_t start, end, direction;
	VectorCopy(self->s.origin, start);
	start[2] += 24;
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, direction);
	VectorNormalize(direction);
	int damage = variant->bullet_damage +
			(int)(2 * skill->value * random());

	if (variant->shotgun)
	{
		monster_fire_shotgun(self, start, direction, damage, 1,
				DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD,
				8, MZ2_SOLDIER_SHOTGUN_1);
	}
	else
	{
		monster_fire_bullet(self, start, direction, damage, 1,
				0, 0, MZ2_INFANTRY_MACHINEGUN_1);
	}
}

static void
grunt_strike(edict_t *self)
{
	if (!self->enemy)
	{
		return;
	}
	const grunt_variant_t *variant = &grunt_types[grunt_variant(self)];
	static vec3_t aim = {MELEE_DISTANCE, 0, 0};
	gi.sound(self, CHAN_WEAPON, melee_sound, 1, ATTN_NORM, 0);
	fire_hit(self, aim, variant->melee_min + (randk() % 5), -50);
}

void
grunt_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &grunt_move_attack_melee;
}

void
grunt_sight(edict_t *self, edict_t *other)
{
	(void)other;
	gi.sound(self, CHAN_VOICE, sight_sounds[randk() & 3], 1, ATTN_NORM, 0);
}

void
grunt_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;
	if (skill->value >= SKILL_HARDPLUS ||
			level.time < self->pain_debounce_time)
	{
		return;
	}
	self->pain_debounce_time = level.time + 3;
	gi.sound(self, CHAN_VOICE, pain_sounds[randk() % 3], 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &grunt_move_pain;
	(void)damage;
}

static void
grunt_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -25);
	VectorSet(self->maxs, 16, 16, -8);
	monster_dynamic_dead(self);
}

void
grunt_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point)
{
	(void)inflictor;
	(void)attacker;
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gib_sound, 1, ATTN_NORM, 0);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, NULL, damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2",
				damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	if (self->deadflag == DEAD_DEAD)
	{
		return;
	}

	if (point[2] >= self->s.origin[2] + self->maxs[2] - 15)
	{
		gi.sound(self, CHAN_VOICE, headshot_sound, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &grunt_move_headshot;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, death_sounds[randk() & 1],
				1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = random() < 0.5f ?
				&grunt_move_death1 : &grunt_move_death2;
	}
	self->deadflag = DEAD_DYING;
	self->takedamage = DAMAGE_YES;
	self->svflags |= SVF_DEADMONSTER;
}

static void
grunt_spawn(edict_t *self, int variant)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	static qboolean sounds_loaded;
	if (!sounds_loaded)
	{
		idle_sounds[0] = gi.soundindex("Monsters/grunt1/g1idle1.wav");
		idle_sounds[1] = gi.soundindex("Monsters/grunt1/g1idle2.wav");
		sight_sounds[0] = gi.soundindex("Monsters/grunt1/g1act1.wav");
		sight_sounds[1] = gi.soundindex("Monsters/grunt1/g1act2.wav");
		sight_sounds[2] = gi.soundindex("Monsters/grunt1/g1act3.wav");
		sight_sounds[3] = gi.soundindex("Monsters/grunt1/g1act4.wav");
		pain_sounds[0] = gi.soundindex("Monsters/grunt1/g1pain1.wav");
		pain_sounds[1] = gi.soundindex("Monsters/grunt1/g1pain2.wav");
		pain_sounds[2] = gi.soundindex("Monsters/grunt1/g1pain3.wav");
		death_sounds[0] = gi.soundindex("Monsters/grunt1/g1death1.wav");
		death_sounds[1] = gi.soundindex("Monsters/grunt1/g1death2.wav");
		melee_sound = gi.soundindex("Monsters/grunt1/g1melee.wav");
		gib_sound = gi.soundindex("misc/udeath.wav");
		headshot_sound = gi.soundindex("misc/udeath3.wav");
		sounds_loaded = true;
	}

	self->style = variant;
	self->s.modelindex = gi.modelindex("models/monsters/grunt/tris.md2");
	VectorSet(self->mins, -16, -16, -25);
	VectorSet(self->maxs, 16, 16, 30);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = grunt_types[variant].health;
	self->max_health = self->health;
	self->gib_health = -40;
	self->mass = 100;
	self->svflags |= SVF_MONSTER;
	self->takedamage = DAMAGE_AIM;
	self->pain = grunt_pain;
	self->die = grunt_die;
	self->monsterinfo.stand = grunt_stand;
	self->monsterinfo.walk = grunt_run;
	self->monsterinfo.run = grunt_run;
	self->monsterinfo.attack = grunt_attack;
	self->monsterinfo.melee = grunt_melee;
	self->monsterinfo.sight = grunt_sight;
	self->monsterinfo.idle = grunt_idle;
	self->monsterinfo.checkattack = grunt_checkattack;
	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);
	walkmonster_start(self);
	self->s.skinnum = grunt_types[variant].skin;
}

void SP_monster_grunt1v1(edict_t *self) { grunt_spawn(self, 0); }
void SP_monster_grunt1v2(edict_t *self) { grunt_spawn(self, 1); }

#undef GF_STAND
#undef GF_MOVE
#undef GF_FAST
#undef GF_ATTACK
#undef GF_MELEE
#undef GF_CHARGE
#undef GF_PAIN
#undef GF_DEATH
#undef REP32
#undef REP16
#undef REP8
#undef REP4
#undef REP2
