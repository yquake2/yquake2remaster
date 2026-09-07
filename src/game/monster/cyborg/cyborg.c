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
 * Oblivion Cyborg
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "cyborg.h"

static int sound_attack1;
static int sound_attack2;
static int sound_attack3;
static int sound_death;
static int sound_idle;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;
static int sound_search;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_thud;

static void cyborg_stand(edict_t *self);
static void cyborg_run(edict_t *self);
static void cyborg_footstep(edict_t *self);
static void cyborg_fire_right(edict_t *self);
static void cyborg_fire_left(edict_t *self);
static void cyborg_fire_both(edict_t *self);
static void cyborg_attack_start(edict_t *self);
static void cyborg_attack_end(edict_t *self);
static void cyborg_touch(edict_t *self, edict_t *other, const cplane_t *plane,
	const csurface_t *surf);
static void cyborg_hit_left(edict_t *self);
static void cyborg_hit_right(edict_t *self);
static void cyborg_hit_alt(edict_t *self);
static void cyborg_dead(edict_t *self);

static mframe_t cyborg_frames_stand[] = {
	{ai_stand, 0, NULL}
};

mmove_t cyborg_move_stand = {
	FRAME_scene,
	FRAME_scene,
	cyborg_frames_stand,
	NULL};

static mframe_t cyborg_frames_idle[] = {
	{ai_stand, 0, NULL}
};
mmove_t cyborg_move_idle = {
	FRAME_scene,
	FRAME_scene,
	cyborg_frames_idle,
	cyborg_stand
};

static mframe_t cyborg_frames_walk[] = {
	{ai_walk, 12, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 13, NULL},
	{ai_walk, 12, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 13, NULL}
};

mmove_t cyborg_move_walk = {
	FRAME_walk1,
	FRAME_walk18,
	cyborg_frames_walk,
	NULL};

static mframe_t cyborg_frames_run[] = {
	{ai_run, 6, NULL},
	{ai_run, 23, cyborg_footstep},
	{ai_run, 8, NULL},
	{ai_run, 6, cyborg_footstep},
	{ai_run, 23, NULL},
	{ai_run, 8, NULL}
};
mmove_t cyborg_move_run = {
	FRAME_run1,
	FRAME_run6,
	cyborg_frames_run,
	NULL
};

static mframe_t cyborg_frames_attack1[] = {
	{ai_charge, 4, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 5, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 9, cyborg_fire_right},
	{ai_charge, 4, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 5, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 9, cyborg_fire_left}
};
mmove_t cyborg_move_attack1 = {
	FRAME_attackA1,
	FRAME_attackA12,
	cyborg_frames_attack1,
	cyborg_run
};

static mframe_t cyborg_frames_attack_backflip[] = {
	{ai_charge, 0, NULL},
	{ai_charge, -17, NULL},
	{ai_charge, -15, cyborg_attack_start},
	{ai_charge, -15, NULL},
	{ai_charge, -15, NULL},
	{ai_charge, -15, NULL},
	{ai_charge, -15, NULL},
	{ai_charge, -15, cyborg_attack_end},
	{ai_charge, 0, cyborg_fire_both},
	{ai_charge, 3, NULL},
	{ai_charge, 0, NULL}
};
mmove_t cyborg_move_attack_backflip = {
	FRAME_attackB1,
	FRAME_attackB11,
	cyborg_frames_attack_backflip,
	cyborg_run
};

static mframe_t cyborg_frames_attack2[] = {
	{ai_charge, 0, cyborg_fire_right},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t cyborg_move_attack2 = {
	FRAME_attackC1,
	FRAME_attackC6,
	cyborg_frames_attack2,
	cyborg_run
};

static mframe_t cyborg_frames_attack3[] = {
	{ai_charge, 0, cyborg_fire_left},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t cyborg_move_attack3 = {
	FRAME_attackD1,
	FRAME_attackD6,
	cyborg_frames_attack3,
	cyborg_run
};

static mframe_t cyborg_frames_melee1[] = {
	{ai_charge, 8, NULL},
	{ai_charge, 10, NULL},
	{ai_charge, 0, cyborg_hit_right},
	{ai_charge, 0, NULL},
	{ai_charge, -5, cyborg_hit_alt},
	{ai_charge, -5, NULL},
	{ai_charge, -5, NULL},
	{ai_charge, -5, NULL}
};
mmove_t cyborg_move_melee1 = {
	FRAME_meleeA1,
	FRAME_meleeA8,
	cyborg_frames_melee1,
	cyborg_run
};

static mframe_t cyborg_frames_melee2[] = {
	{ai_charge, 6, NULL},
	{ai_charge, 6, NULL},
	{ai_charge, 6, NULL},
	{ai_charge, -3, cyborg_hit_left},
	{ai_charge, -3, NULL},
	{ai_charge, -10, NULL}
};
mmove_t cyborg_move_melee2 = {
	FRAME_meleeB1,
	FRAME_meleeB6,
	cyborg_frames_melee2,
	cyborg_run
};

static mframe_t cyborg_frames_pain1[] = {
	{ai_move, -16, NULL},
	{ai_move, -4, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t cyborg_move_pain1 = {
	FRAME_painA1,
	FRAME_painA6,
	cyborg_frames_pain1,
	cyborg_run
};

static mframe_t cyborg_frames_pain2[] = {
	{ai_move, -11, NULL},
	{ai_move, -8, NULL},
	{ai_move, 4, NULL}
};
mmove_t cyborg_move_pain2 = {
	FRAME_painB1,
	FRAME_painB3,
	cyborg_frames_pain2,
	cyborg_run
};

static mframe_t cyborg_frames_pain2_end[] = {
	{ai_move, 0, NULL}
};
mmove_t cyborg_move_pain2_end = {
	FRAME_painB4,
	FRAME_painB4,
	cyborg_frames_pain2_end,
	cyborg_run
};

static mframe_t cyborg_frames_move[] = {
	{ai_move, 8, NULL},
	{ai_move, 7, NULL},
	{ai_move, 3, NULL},
	{ai_move, 0, NULL},
	{ai_move, -2, NULL},
	{ai_move, -3, NULL},
	{ai_move, 2, NULL},
	{ai_move, 5, NULL},
	{ai_move, 16, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t cyborg_move_move = {
	FRAME_painB15,
	FRAME_painB26,
	cyborg_frames_move,
	cyborg_run
};

static mframe_t cyborg_frames_death1[] = {
	{ai_move, -2, NULL},
	{ai_move, 0, NULL},
	{ai_move, -3, NULL},
	{ai_move, 0, NULL},
	{ai_move, -1, NULL},
	{ai_move, -2, NULL},
	{ai_move, -3, NULL},
	{ai_move, -2, NULL}
};
mmove_t cyborg_move_death1 = {
	FRAME_deathA1,
	FRAME_deathA8,
	cyborg_frames_death1,
	cyborg_dead
};

static mframe_t cyborg_frames_death2[] = {
	{ai_move, -6, NULL},
	{ai_move, -4, NULL},
	{ai_move, -2, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t cyborg_move_death2 = {
	FRAME_deathB1,
	FRAME_deathB6,
	cyborg_frames_death2,
	cyborg_dead
};

static mframe_t cyborg_frames_death3[] = {
	{ai_move, 8, NULL},
	{ai_move, 4, NULL},
	{ai_move, 2, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t cyborg_move_death3 = {
	FRAME_deathC1,
	FRAME_deathC6,
	cyborg_frames_death3,
	cyborg_dead
};

static void
cyborg_footstep(edict_t *self)
{
	int n;

	n = (randk() + 1) % 3;
	if (n == 0)
	{
		gi.sound(self, CHAN_VOICE, sound_step1, 1, ATTN_NORM, 0);
	}
	else if (n == 1)
	{
		gi.sound(self, CHAN_VOICE, sound_step2, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_step3, 1, ATTN_NORM, 0);
	}
}

static void
cyborg_idle(edict_t *self)
{
	self->monsterinfo.currentmove = &cyborg_move_idle;
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void
cyborg_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void
cyborg_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void
cyborg_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &cyborg_move_stand;
}

static void
cyborg_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &cyborg_move_walk;
}

static void
cyborg_run(edict_t *self)
{
	if (self->monsterinfo.currentmove == &cyborg_move_pain2)
	{
		self->monsterinfo.currentmove = &cyborg_move_pain2_end;
		return;
	}

	if (self->monsterinfo.currentmove == &cyborg_move_pain2_end)
	{
		if (crandk() < 0.1f)
		{
			self->monsterinfo.currentmove = &cyborg_move_move;
		}
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &cyborg_move_stand;
	}
	else
	{
		self->monsterinfo.currentmove = &cyborg_move_run;
	}
}

static void
cyborg_fire_right(edict_t *self)
{
	vec3_t forward;
	vec3_t right;
	vec3_t start;
	vec3_t target;
	vec3_t dir;
	vec3_t offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 15, 12, 12);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	fire_deatom(self, start, dir, 50, 600);
}

static void
cyborg_fire_left(edict_t *self)
{
	vec3_t forward, right, start, target, dir, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 15, -12, 12);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	fire_deatom(self, start, dir, 50, 600);
}

static void
cyborg_fire_both(edict_t *self)
{
	vec3_t forward, right, start, target, dir, offset;

	AngleVectors(self->s.angles, forward, right, NULL);

	VectorSet(offset, 15, -12, 12);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	fire_deatom(self, start, dir, 50, 600);

	VectorSet(offset, 15, 12, 12);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	fire_deatom(self, start, dir, 50, 600);
}

static void
cyborg_attack_start(edict_t *self)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);

	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1.0f;
	VectorScale(forward, -100.0f, self->velocity);
	self->velocity[2] = 250.0f;
	self->groundentity = NULL;
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	self->touch = cyborg_touch;
	self->monsterinfo.attack_finished = level.time + 3.0f;
}

static void
cyborg_attack_end(edict_t *self)
{
	if (self->groundentity)
	{
		gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
		self->monsterinfo.attack_finished = 0;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
	}
}

static void
cyborg_touch(edict_t *self, edict_t *other, const cplane_t *plane,
	const csurface_t *surf)
{
	vec3_t dir, point;
	int damage;

	if (self->health > 0)
	{
		if (other->takedamage && VectorLength(self->velocity) > 400.0f)
		{
			VectorNormalize2(self->velocity, dir);
			VectorMA(self->s.origin, self->maxs[0], dir, point);
			damage = (int)(crandk() * 10.0f) + 40;
			T_Damage(other, self, self, self->velocity, point, dir, damage,
				damage, 0, MOD_UNKNOWN);
		}

		if (!M_CheckBottom(self))
		{
			if (!self->groundentity)
			{
				return;
			}

			self->monsterinfo.nextframe = FRAME_attackB1 + 5;
		}
	}

	self->touch = NULL;
}

static void
cyborg_hit_left(edict_t *self)
{
	vec3_t aim;
	int damage;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8);
	damage = (randk() % 5) + 10;

	if (fire_hit(self, aim, damage, 100))
	{
		gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
	}
}

static void
cyborg_hit_right(edict_t *self)
{
	vec3_t aim;
	int damage;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8);
	damage = (randk() % 5) + 10;

	if (fire_hit(self, aim, damage, 100))
	{
		gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
	}
}

static void
cyborg_hit_alt(edict_t *self)
{
	vec3_t aim;
	int damage;

	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8);
	damage = (randk() % 5) + 10;

	if (fire_hit(self, aim, damage, 100))
	{
		gi.sound(self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
	}
}

static void
cyborg_attack(edict_t *self)
{
	float r;

	r = crandk();
	if (r < 0.5f)
	{
		self->monsterinfo.currentmove = &cyborg_move_attack1;
	}
	else if (r < 0.7f)
	{
		self->monsterinfo.currentmove = &cyborg_move_attack3;
	}
	else
	{
		self->monsterinfo.currentmove = &cyborg_move_attack2;
	}
}

static void
cyborg_melee(edict_t *self)
{
	float r;

	r = crandk();
	if (r < 0.6f)
	{
		self->monsterinfo.currentmove = &cyborg_move_melee1;
	}
	else if (crandk() < 0.7f)
	{
		self->monsterinfo.currentmove = &cyborg_move_melee2;
	}
	else
	{
		self->monsterinfo.currentmove = &cyborg_move_attack_backflip;
	}
}

static qboolean
cyborg_check_range(edict_t *self)
{
	edict_t	*enemy;
	vec3_t delta;
	float dist;

	enemy = self->enemy;
	if (self->absmin[2] > enemy->absmin[2] + enemy->size[2] * 0.75f)
	{
		return false;
	}

	if (self->absmax[2] < enemy->absmin[2] + enemy->size[2] * 0.25f)
	{
		return false;
	}

	delta[0] = self->s.origin[0] - enemy->s.origin[0];
	delta[1] = self->s.origin[1] - enemy->s.origin[1];
	delta[2] = 0;

	dist = VectorLength(delta);
	if (dist < 100.0f)
	{
		return false;
	}

	if (dist > 100.0f && crandk() < 0.2f)
	{
		return false;
	}

	return true;
}

static qboolean
cyborg_checkattack(edict_t *self)
{
	if (self->enemy && self->enemy->health > 0)
	{
		if (ai_range(self, self->enemy) == RANGE_MELEE)
		{
			self->monsterinfo.attack_state = AS_MELEE;
			return true;
		}

		if (cyborg_check_range(self))
		{
			self->monsterinfo.attack_state = AS_MISSILE;
			return true;
		}
	}

	return false;
}

static void
cyborg_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
	{
		self->s.skinnum = 1;
	}

	if (level.time < self->pain_debounce_time)
	{
		return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == SKILL_HARDPLUS)
	{
		return;
	}

	if (crandk() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &cyborg_move_pain1;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &cyborg_move_pain2;
	}
}

static void
cyborg_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, const vec3_t point)
{
	int n;
	float r;

	if (meansOfDeath == MOD_DISINTEGRATOR)
	{
		BecomeExplosion1(self);
		return;
	}

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1,
			ATTN_NORM, 0);

		for (n = 0; n < 2; n++)
		{
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage,
				GIB_ORGANIC);
		}
		for (n = 0; n < 4; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage,
				GIB_ORGANIC);
		}

		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage,
			GIB_ORGANIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum = 1;

	r = crandk();
	if (r < 0.33f)
	{
		self->monsterinfo.currentmove = &cyborg_move_death1;
	}
	else if (r < 0.66f)
	{
		self->monsterinfo.currentmove = &cyborg_move_death2;
	}
	else
	{
		self->monsterinfo.currentmove = &cyborg_move_death3;
	}
}

static void
cyborg_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -38);
	VectorSet(self->maxs, 32, 32, -20);
	monster_dynamic_dead(self);
}

/*
 * QUAKED monster_cyborg (1 .5 0) (-16 -16 -38) (16 16 27) Ambush Trigger_Spawn
 *
 * Oblivion: Cyborg
 */
void
SP_monster_cyborg(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_attack1 = gi.soundindex("cyborg/mutatck1.wav");
	sound_attack2 = gi.soundindex("cyborg/mutatck2.wav");
	sound_attack3 = gi.soundindex("cyborg/mutatck3.wav");
	sound_death = gi.soundindex("cyborg/mutdeth1.wav");
	sound_idle = gi.soundindex("cyborg/mutidle1.wav");
	sound_pain1 = gi.soundindex("cyborg/mutpain1.wav");
	sound_pain2 = gi.soundindex("cyborg/mutpain2.wav");
	sound_sight = gi.soundindex("cyborg/mutsght1.wav");
	sound_search = gi.soundindex("cyborg/mutsrch1.wav");
	sound_step1 = gi.soundindex("cyborg/step1.wav");
	sound_step2 = gi.soundindex("cyborg/step2.wav");
	sound_step3 = gi.soundindex("cyborg/step3.wav");
	sound_thud = gi.soundindex("cyborg/thud1.wav");

	self->s.modelindex = gi.modelindex("models/monsters/cyborg/tris.md2");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->pain = cyborg_pain;
	self->die = cyborg_die;

	self->monsterinfo.stand = cyborg_stand;
	self->monsterinfo.walk = cyborg_walk;
	self->monsterinfo.run = cyborg_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = cyborg_attack;
	self->monsterinfo.melee = cyborg_melee;
	self->monsterinfo.sight = cyborg_sight;
	self->monsterinfo.search = cyborg_search;
	self->monsterinfo.idle = cyborg_idle;
	self->monsterinfo.checkattack = cyborg_checkattack;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &cyborg_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
