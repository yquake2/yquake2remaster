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
 * Oblivion Spider.
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "spider.h"

#define SPIDER_SPAWNFLAG_DEAD 8

static int sound_step;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;
static int sound_search;
static int sound_idle;
static int sound_melee1;
static int sound_melee2;
static int sound_melee3;

static void spider_idle(edict_t *self);
static void spider_melee_swing(edict_t *self);
static void spider_walk(edict_t *self);
static void spider_charge_start(edict_t *self);
static void spider_charge_end(edict_t *self);
static void spider_run(edict_t *self);
static void spider_melee_hit(edict_t *self);
static void spider_melee(edict_t *self);
static void spider_rocket_left(edict_t *self);
static void spider_rocket_right(edict_t *self);
static void spider_dead(edict_t *self);

static mframe_t spider_frames_walk[] = {
	{ai_walk, 0.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 6.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 2.0f, NULL},
	{ai_walk, 0.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 6.0f, NULL},
	{ai_walk, 4.0f, NULL},
	{ai_walk, 2.0f, NULL}
};

static mmove_t spider_move_walk = {
	FRAME_walkA1,
	FRAME_walkA10,
	spider_frames_walk,
	NULL
};

static mframe_t spider_frames_run1[] = {
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, spider_charge_start},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, spider_charge_end},
	{ai_run, 0.0f, NULL},
	{ai_run, 0.0f, NULL}
};

static mmove_t spider_move_run1 = {
	FRAME_runA1,
	FRAME_runA10,
	spider_frames_run1,
	spider_run
};

static mframe_t spider_frames_run2[] = {
	{ai_run, 16.0f, NULL},
	{ai_run, 16.0f, NULL},
	{ai_run, 16.0f, NULL},
	{ai_run, 16.0f, NULL},
	{ai_run, 16.0f, NULL},
	{ai_run, 16.0f, NULL}
};

static mmove_t spider_move_run2 = {
	FRAME_runB1,
	FRAME_runB6,
	spider_frames_run2,
	NULL
};

static mframe_t spider_frames_attack_left[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_rocket_left},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL}
};

static mmove_t spider_move_attack_left = {
	FRAME_attackL1,
	FRAME_attackL5,
	spider_frames_attack_left,
	spider_run
};

static mframe_t spider_frames_attack_right[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_rocket_right},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL}
};

static mmove_t spider_move_attack_right = {
	FRAME_attackR1,
	FRAME_attackR5,
	spider_frames_attack_right,
	spider_run
};

static mframe_t spider_frames_attack_dual[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_rocket_left},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_rocket_right},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL}
};

static mmove_t spider_move_attack_dual = {
	FRAME_attackB1,
	FRAME_attackB8,
	spider_frames_attack_dual,
	spider_run
};

static mframe_t spider_frames_melee_primary[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_melee_swing},
	{ai_charge, 0.0f, spider_melee_hit}
};

static mmove_t spider_move_melee_primary = {
	FRAME_meleeA1,
	FRAME_meleeA5,
	spider_frames_melee_primary,
	spider_run
};

static mframe_t spider_frames_melee_secondary[] = {
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, NULL},
	{ai_charge, 0.0f, spider_melee_swing},
	{ai_charge, 0.0f, spider_melee_hit},
	{ai_charge, 0.0f, spider_melee_hit}
};

static mmove_t spider_move_melee_secondary = {
	FRAME_meleeB1,
	FRAME_meleeB7,
	spider_frames_melee_secondary,
	spider_run
};

static mframe_t spider_frames_pain2[] = {
	{ai_move, -16.0f, NULL},
	{ai_move, -32.0f, NULL},
	{ai_move, -8.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL}
};

static mmove_t spider_move_pain2 = {
	FRAME_painB1,
	FRAME_painB8,
	spider_frames_pain2,
	spider_run
};

static mframe_t spider_frames_death1[] = {
	{ai_move, -8.0f, NULL},
	{ai_move, -4.0f, NULL},
	{ai_move, -2.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, -2.0f, NULL},
	{ai_move, -6.0f, NULL},
	{ai_move, -4.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 4.0f, NULL},
	{ai_move, 6.0f, NULL},
	{ai_move, 4.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL}
};

static mmove_t spider_move_death1 = {
	FRAME_deathA1,
	FRAME_deathA20,
	spider_frames_death1,
	spider_dead
};

static mframe_t spider_frames_death2[] = {
	{ai_move, -24.0f, NULL},
	{ai_move, -22.0f, NULL},
	{ai_move, -20.0f, NULL},
	{ai_move, -18.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -16.0f, NULL},
	{ai_move, -4.0f, NULL},
	{ai_move, -12.0f, NULL},
	{ai_move, -8.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL},
	{ai_move, 0.0f, NULL}
};

static mmove_t spider_move_death2 = {
	FRAME_deathB1,
	FRAME_deathB20,
	spider_frames_death2,
	spider_dead
};

static void
spider_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1.0f, ATTN_IDLE, 0.0f);
}

static void
spider_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1.0f, ATTN_NORM, 0.0f);
}

static void
spider_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);
}

static void
spider_melee_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1.0f, ATTN_NORM, 0.0f);
}

static void
spider_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &spider_move_walk;
}

static void
spider_charge_think(edict_t *self, edict_t *other, const cplane_t *plane,
	const csurface_t *surf)
{
	if (self->health > 0)
	{
		if (other->takedamage && VectorLength(self->velocity) > 400)
		{
			vec3_t dir, point;
			int damage;

			VectorCopy(self->velocity, dir);
			VectorNormalize(dir);
			VectorMA(self->s.origin, self->maxs[0], dir, point);
			damage = (randk() % 10) + 40;
			T_Damage(other, self, self, self->velocity, point, dir, damage,
				damage, 0, MOD_UNKNOWN);
		}

		if (!M_CheckBottom(self))
		{
			if (!self->groundentity)
			{
				return;
			}

			self->monsterinfo.nextframe = FRAME_runA1 + 3;
		}
	}

	self->touch = NULL;
}

static void
spider_charge_start(edict_t *self)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);

	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1.0f;
	VectorScale(forward, 500, self->velocity);
	self->velocity[2] = 250;
	self->groundentity = NULL;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->touch = spider_charge_think;
	self->monsterinfo.attack_finished = level.time + 3.0f;
}

static void
spider_charge_end(edict_t *self)
{
	if (self->groundentity)
	{
		gi.sound(self, CHAN_WEAPON, sound_step, 1.0f, ATTN_NORM, 0.0f);
		self->monsterinfo.attack_finished = 0.0f;
		self->monsterinfo.aiflags &= ~AI_DUCKED;
	}
}

static void
spider_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		monster_dynamic_stand(self);
		return;
	}

	if (crandk() < 0.2f)
	{
		self->monsterinfo.currentmove = &spider_move_run1;
		return;
	}

	self->monsterinfo.currentmove = &spider_move_run2;
}

static void
spider_melee_hit(edict_t *self)
{
	vec3_t aim;
	int damage;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], -4.0f);
	damage = (randk() % 5) + 20;

	if (fire_hit(self, aim, damage, 300))
	{
		gi.sound(self, CHAN_WEAPON, sound_melee2, 1.0f, ATTN_NORM, 0.0f);
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_melee3, 1.0f, ATTN_NORM, 0.0f);
}

static void
spider_melee(edict_t *self)
{
	if (crandk() < 0.5f)
	{
		self->monsterinfo.currentmove = &spider_move_melee_primary;
		return;
	}

	self->monsterinfo.currentmove = &spider_move_melee_secondary;
}

static void
spider_rocket_left(edict_t *self)
{
	vec3_t forward, right, start, dir;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, tv(30.0f, -18.0f, 3.0f), forward, right,
		start);

	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);

	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_MACHINEGUN_R4);
}

static void
spider_rocket_right(edict_t *self)
{
	vec3_t forward, right, start, dir;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, tv(30.0f, 18.0f, 3.0f), forward, right,
		start);

	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);

	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_MACHINEGUN_R5);
}

static void
spider_attack(edict_t *self)
{
	vec3_t delta;
	float r;

	r = crandk();

	VectorSubtract(self->s.origin, self->enemy->s.origin, delta);
	if (VectorLength(delta) <= 112)
	{
		return;
	}

	VectorCopy(self->enemy->s.origin, self->pos1);
	self->pos1[2] += self->enemy->viewheight;

	if (r < 0.33f)
	{
		self->monsterinfo.currentmove = &spider_move_attack_left;
		return;
	}

	if (r < 0.66f)
	{
		self->monsterinfo.currentmove = &spider_move_attack_right;
		return;
	}

	self->monsterinfo.currentmove = &spider_move_attack_dual;
}

static void
spider_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int sound_id;

	if (self->health < (self->max_health / 2))
	{
		self->s.skinnum = 1;
	}

	self->pain_debounce_time = level.time + 3.0f;

	sound_id = sound_pain1;
	if (crandk() >= 0.5f)
	{
		sound_id = sound_pain2;
	}

	if ((skill->value == SKILL_HARDPLUS) && crandk() < 0.1f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		monster_dynamic_action(self, "pain", 0);
		return;
	}

	if (damage < 10 && crandk() < 0.2f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		monster_dynamic_action(self, "pain", 0);
		return;
	}

	if (damage < 50 && crandk() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		if (crandk() < 0.5f)
		{
			monster_dynamic_action(self, "pain", 0);
			return;
		}
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
	}

	self->monsterinfo.currentmove = &spider_move_pain2;
}

static void
spider_dead(edict_t *self)
{
	VectorSet(self->mins, -32.0f, -32.0f, -30.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 0.0f);
	monster_dynamic_dead(self);
}

static void
spider_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, const vec3_t point)
{
	if (meansOfDeath == MOD_DISINTEGRATOR)
	{
		BecomeExplosion1(self);
		return;
	}

	if (self->health <= self->gib_health)
	{
		int n;

		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f,
			ATTN_NORM, 0.0f);

		for (n = 0; n < 2; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage,
				GIB_ORGANIC);
		}

		for (n = 0; n < 4; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage,
				GIB_METALLIC);
		}

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage,
			GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage,
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

	if (crandk() < 0.5f)
	{
		self->monsterinfo.currentmove = &spider_move_death1;
		return;
	}

	self->monsterinfo.currentmove = &spider_move_death2;
}

static qboolean
spider_checkattack(edict_t *self)
{
	int enemy_range;
	float chance;
	vec3_t temp;

	if (!self || !self->enemy || !self->enemy->inuse)
	{
		return false;
	}

	if (self->enemy->health > 0)
	{
		vec3_t spot1, spot2;
		trace_t tr;

		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace(spot1, NULL, NULL, spot2, self,
			CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME |
			CONTENTS_LAVA | CONTENTS_WINDOW);
		if (tr.ent != self->enemy)
		{
			return false;
		}
	}

	if (!infront(self, self->enemy))
	{
		return false;
	}

	enemy_range = ai_range(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
	self->ideal_yaw = vectoyaw(temp);

	if (enemy_range == RANGE_MELEE)
	{
		if (self->monsterinfo.melee)
		{
			self->monsterinfo.attack_state = AS_MELEE;
		}
		else
		{
			self->monsterinfo.attack_state = AS_MISSILE;
		}

		return true;
	}

	if (!self->monsterinfo.attack)
	{
		return false;
	}

	if (level.time < self->monsterinfo.attack_finished)
	{
		return false;
	}

	if (enemy_range == RANGE_FAR)
	{
		return false;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		chance = 0.4f;
	}
	else
	{
		if (enemy_range != RANGE_NEAR && enemy_range != RANGE_MID)
		{
			return false;
		}

		chance = 0.8f;
	}

	if (crandk() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2.0f * crandk();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (crandk() < 0.3f)
		{
			self->monsterinfo.attack_state = AS_SLIDING;
		}
		else
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
		}
	}

	return false;
}

/*
 * QUAKED monster_spider (1 .5 0) (-32 -32 -35) (32 32 32)
 *
 * Oblivion Spider
 */
void
SP_monster_spider(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_melee1 = gi.soundindex("gladiator/melee1.wav");
	sound_melee2 = gi.soundindex("gladiator/melee2.wav");
	sound_melee3 = gi.soundindex("gladiator/melee3.wav");
	sound_step = gi.soundindex("mutant/thud1.wav");
	sound_pain1 = gi.soundindex("gladiator/pain.wav");
	sound_pain2 = gi.soundindex("gladiator/gldpain2.wav");
	sound_idle = gi.soundindex("gladiator/gldidle1.wav");
	sound_search = gi.soundindex("gladiator/gldsrch1.wav");
	sound_sight = gi.soundindex("spider/sight.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	if (self->spawnflags & SPIDER_SPAWNFLAG_DEAD)
	{
		VectorSet(self->mins, -32.0f, -32.0f, -30.0f);
		VectorSet(self->maxs, 32.0f, 32.0f, 0.0f);
		self->s.skinnum |= 1;
		self->health = -1;
		monster_dynamic_dead(self);
		return;
	}

	self->pain = spider_pain;
	self->die = spider_die;

	self->monsterinfo.stand = monster_dynamic_stand;
	self->monsterinfo.idle = spider_idle;
	self->monsterinfo.search = spider_search;
	self->monsterinfo.walk = spider_walk;
	self->monsterinfo.run = spider_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = spider_attack;
	self->monsterinfo.melee = spider_melee;
	self->monsterinfo.sight = spider_sight;

	gi.linkentity(self);

	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.checkattack = spider_checkattack;

	monster_dynamic_stand(self);
	walkmonster_start(self);
}
