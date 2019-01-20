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
// m_dog.c

#include "../../game/g_local.h"

static int sound_melee;
static int sound_death;
static int sound_pain;
static int sound_sight;
static int sound_search;

void dog_leap(edict_t *self);

// Stand
mframe_t dog_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL
};
mmove_t dog_move_stand = {69, 77, dog_frames_stand, NULL};

void dog_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &dog_move_stand;
}

// Run
mframe_t dog_frames_run [] =
{
	ai_run, 16, NULL,
	ai_run, 32, NULL,
	ai_run, 32, NULL,
	ai_run, 20, NULL,
	ai_run, 64, NULL,

	ai_run, 32, NULL,
	ai_run, 16, NULL,
	ai_run, 32, NULL,
	ai_run, 32, NULL,

	ai_run, 20, NULL,
	ai_run, 64, NULL,
	ai_run, 32, dog_leap
};
mmove_t dog_move_run = {48, 59, dog_frames_run, NULL};

void dog_run(edict_t *self)
{
	self->monsterinfo.currentmove = &dog_move_run;
}	

void DogLeapTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->health < 1)
		return;
	if (other->takedamage && other->monster_team != self->monster_team)
	{
		if (VectorLength(self->velocity) > 300)
		{
			int	damage = 10 + 10 * random();

			T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, damage, 0, 0, 0);
		}
	}
	self->touch = NULL;

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.currentmove = &dog_move_run;
			self->movetype = MOVETYPE_STEP;
		}
		return;
	}
}

void DogLeap(edict_t *self)
{
	vec3_t forward;

	AngleVectors (self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	VectorScale(forward, 300, self->velocity);
	self->velocity[2] = 200;

	self->groundentity = NULL;
	self->touch = DogLeapTouch;
}

// Leap
mframe_t dog_frames_leap [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	DogLeap,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,

	ai_move,	0,	NULL
};
mmove_t dog_move_leap = {60, 68, dog_frames_leap, dog_run};

void dog_leap(edict_t *self)
{
	//if (self->enemy && CheckDistance(self, self->enemy) < (MELEE_DISTANCE * 4))
	self->monsterinfo.currentmove = &dog_move_leap;
}

void DogBite(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	gi.sound(self, CHAN_VOICE, sound_melee, 1, ATTN_NORM, 0);

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = (random() + random() + random()) * 8;

	fire_hit(self, aim, damage, damage);
}

// Melee
mframe_t dog_frames_melee [] =
{
	ai_charge, 10,	NULL,
	ai_charge, 10,	NULL,
	ai_charge, 10,	NULL,
	ai_charge, 10,	DogBite,

	ai_charge, 10,	NULL,
	ai_charge, 10,	NULL,
	ai_charge, 10,	NULL,
	ai_charge, 10,	NULL
};
mmove_t dog_move_melee = {0, 7, dog_frames_melee, dog_run};

void dog_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &dog_move_melee;
}

// Sight
void dog_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void dog_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

// Pain (1)
mframe_t dog_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t dog_move_pain1 = {26, 31, dog_frames_pain1, dog_run};

// Pain (2)
mframe_t dog_frames_pain2 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, -4,	NULL,
	ai_move, -12,	NULL,

	ai_move, -12,	NULL,
	ai_move, -2,	NULL,
	ai_move, 0,		NULL,
	ai_move, -4,	NULL,

	ai_move, 0,		NULL,
	ai_move, -10,	NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t dog_move_pain2 = {32, 47, dog_frames_pain2, dog_run};

void dog_pain(edict_t *self)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (random() > 0.5)
		self->monsterinfo.currentmove = &dog_move_pain1;
	else
		self->monsterinfo.currentmove = &dog_move_pain2;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

void dog_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
mframe_t dog_frames_die1 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL
};
mmove_t dog_move_die1 = {8, 16, dog_frames_die1, dog_dead};

// Death (2)
mframe_t dog_frames_die2 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL
};
mmove_t dog_move_die2 = {17, 25, dog_frames_die2, dog_dead};

void dog_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

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

	if (random() > 0.5)
		self->monsterinfo.currentmove = &dog_move_die1;
	else
		self->monsterinfo.currentmove = &dog_move_die2;
}

void SP_monster_q1_dog(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/dog/tris.md2");
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 40);
	self->health = 25;
	self->monster_name = "Rottweiler";

	if (self->solid == SOLID_NOT)
		return;
	sound_melee = gi.soundindex("quake1/dog/dattack1.wav");
	sound_death = gi.soundindex("quake1/dog/ddeath.wav");
	sound_pain = gi.soundindex("quake1/dog/dpain1.wav");
	sound_sight = gi.soundindex("quake1/dog/dsight.wav");
	sound_search = gi.soundindex("quake1/dog/idle.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -35;
	self->mass = 30;

	self->monsterinfo.stand = dog_stand;
	self->monsterinfo.walk = dog_run;
	self->monsterinfo.run = dog_run;
	//self->monsterinfo.attack = dog_leap;
	self->monsterinfo.melee = dog_melee;
	self->monsterinfo.sight = dog_sight;
	self->monsterinfo.search = dog_search;

	self->pain = dog_pain;
	self->die = dog_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
