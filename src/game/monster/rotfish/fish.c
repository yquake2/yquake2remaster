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
#include "fish.h"

static int sound_search;
static int sound_death;
static int sound_melee;

static void
fish_bite_step(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, 0};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	// decino: In Q1 it's 60 units, but it will never hit anything in Q2 for some reason
	if (VectorLength(dir) > 100)
		return;
	damage = (random() + random() + random()) * 3;

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

// Melee
static mframe_t fish_frames_melee [] =
{
	{ai_run, 10, NULL},
	{ai_run, 10, NULL},
	{ai_run, 0,  fish_bite_step},
	{ai_run, 10, NULL},

	{ai_run, 10, NULL},
	{ai_run, 10, NULL},
	{ai_run, 10, NULL},
	{ai_run, 10, NULL},

	{ai_run, 0,  fish_bite_step},
	{ai_run, 10, NULL},
	{ai_run, 10, NULL},
	{ai_run, 10, NULL},

	{ai_run, 10, NULL},
	{ai_run, 10, NULL},
	{ai_run, 0,  fish_bite_step},
	{ai_run, 10, NULL},

	{ai_run, 10, NULL},
	{ai_run, 10, NULL}
};
mmove_t fish_move_melee =
{
	FRAME_attack1,
	FRAME_attack18,
	fish_frames_melee,
	monster_dynamic_run
};

void
fish_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &fish_move_melee;
}

// Search
void
fish_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void
fish_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death
static mframe_t fish_frames_death [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

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
mmove_t fish_move_death =
{
	FRAME_death1,
	FRAME_death21,
	fish_frames_death,
	fish_dead
};

void
fish_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	if (self->deadflag == DEAD_DEAD)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &fish_move_death;
}

// Pain

void
fish_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
	if (level.time < self->pain_debounce_time)
	{
		return;
	}

	self->pain_debounce_time = level.time + 3;

	monster_dynamic_pain(self, other, kick, damage);
}

/*
 * QUAKED monster_rotfish (1 .5 0) (-16, -16, -24) (16, 16, 24) Ambush Trigger_Spawn Sight
 */
void
SP_monster_rotfish(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/rotfish/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 24);
	self->health = 25 * st.health_multiplier;

	sound_search = gi.soundindex("fish/idle.wav");
	sound_death = gi.soundindex("fish/death.wav");
	sound_melee = gi.soundindex("fish/bite.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -25;
	self->mass = 25;

	monster_dynamic_setinfo(self);

	self->monsterinfo.run_dist = 12;

	self->monsterinfo.melee = fish_melee;
	self->monsterinfo.search = fish_search;

	self->pain = fish_pain;
	self->die = fish_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	swimmonster_start(self);
}
