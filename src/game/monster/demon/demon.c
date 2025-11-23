/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../../header/local.h"
#include "demon.h"

static int sound_death;
static int sound_hit;
static int sound_jump;
static int sound_land;
static int sound_pain;
static int sound_sight;
static int sound_search;

// Run
static mframe_t demon_frames_run [] =
{
	{ai_run, 20, NULL},
	{ai_run, 15, NULL},
	{ai_run, 36, NULL},
	{ai_run, 20, NULL},

	{ai_run, 15, NULL},
	{ai_run, 36, NULL}
};
mmove_t demon_move_run =
{
	FRAME_run1,
	FRAME_run6,
	demon_frames_run,
	NULL
};

void
demon_run(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_move_run;
}

static qboolean
check_demon_jump(edict_t *self)
{
	vec3_t	dir;
	float	distance;

	if (!self->enemy)
		return false;
	if (self->s.origin[2] + self->mins[2] > self->enemy->s.origin[2] + self->enemy->mins[2] + 0.75 * self->enemy->size[2])
		return false;
	if (self->s.origin[2] + self->mins[2] < self->enemy->s.origin[2] + self->enemy->mins[2] + 0.25 * self->enemy->size[2])
		return false;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	dir[2] = 0;
	distance = VectorLength(dir);

	if (distance < 100)
		return false;

	if (distance > 200)
	{
		if (random() < 0.9)
			return false;
	}
	return true;
};

void
demon_jump_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->health < 1)
		return;
	if (other->takedamage)
	{
		if (VectorLength(self->velocity) > 400)
		{
			int	damage = 40 + 10 * random();

			T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, damage, 0, 0, 0);
		}
	}
	else
		gi.sound(self, CHAN_WEAPON, sound_land, 1, ATTN_NORM, 0);
	self->touch = NULL;

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.currentmove = &demon_move_run;
			self->movetype = MOVETYPE_STEP;
		}
		return;
	}
}

static void
demon_jump(edict_t *self)
{
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	VectorScale(forward, 600, self->velocity);
	self->velocity[2] = 250;

	self->groundentity = NULL;
	self->touch = demon_jump_touch;
}

static void
demon_roar(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_jump, 1, ATTN_NORM, 0);
}

// Attack
static mframe_t demon_frames_jump [] =
{
	{ai_charge, 0, demon_roar},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, demon_jump},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t demon_move_jump =
{
	FRAME_leap1,
	FRAME_leap12,
	demon_frames_jump,
	demon_run
};

void
demon_attack(edict_t *self)
{
	if (check_demon_jump(self))
	{
		self->monsterinfo.currentmove = &demon_move_jump;
	}
}

static void
demon_melee_step(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = 10 + 5 * random();

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
}

// Melee
static mframe_t demon_frames_melee [] =
{
	{ai_charge, 4, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 1, NULL},

	{ai_charge, 14, demon_melee_step},
	{ai_charge, 1, NULL},
	{ai_charge, 6, NULL},
	{ai_charge, 8, NULL},

	{ai_charge, 4, NULL},
	{ai_charge, 2, NULL},
	{ai_charge, 12, demon_melee_step},
	{ai_charge, 5, NULL},

	{ai_charge, 8, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 4, NULL}
};
mmove_t demon_move_melee =
{
	FRAME_attacka1,
	FRAME_attacka15,
	demon_frames_melee,
	demon_run
};

void
demon_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_move_melee;
}

// Pain

void
demon_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == SKILL_HARDPLUS)
	{
		return;
	}

	if (self->touch == demon_jump_touch)
	{
		return;
	}

	if (self->pain_debounce_time > level.time)
	{
		return;
	}

	self->pain_debounce_time = level.time + 1.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (random() * 200 > damage)
	{
		return;
	}

	monster_dynamic_pain(self, other, kick, damage);
}

void
demon_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	monster_dynamic_dead(self);
}

// Death
static mframe_t demon_frames_die [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL}
};
mmove_t demon_move_die =
{
	FRAME_death1,
	FRAME_death9,
	demon_frames_die,
	demon_dead
};

void
demon_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	if (self->deadflag == DEAD_DEAD)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &demon_move_die;
}

// Sight
void
demon_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void
demon_search(edict_t *self)
{
	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

/*
 * QUAKED monster_demon (1 .5 0) (-32, -32, -24) (32, 32, 64) Ambush Trigger_Spawn Sight
 */
void
SP_monster_demon(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/demon/tris.md2");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 300 * st.health_multiplier;

	sound_death = gi.soundindex("demon/ddeath.wav");
	sound_hit = gi.soundindex("demon/dhit2.wav");
	sound_jump = gi.soundindex("demon/djump.wav");
	sound_land = gi.soundindex("demon/dland2.wav");
	sound_pain = gi.soundindex("demon/dpain1.wav");
	sound_search = gi.soundindex("demon/idle1.wav");
	sound_sight = gi.soundindex("demon/sight2.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -80;
	self->mass = 300;

	monster_dynamic_setinfo(self);

	self->monsterinfo.walk = demon_run;
	self->monsterinfo.run = demon_run;
	self->monsterinfo.attack = demon_attack;
	self->monsterinfo.melee = demon_melee;
	self->monsterinfo.sight = demon_sight;
	self->monsterinfo.search = demon_search;

	self->pain = demon_pain;
	self->die = demon_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
