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
// m_army.c

#include "../../game/g_local.h"

static int sound_death;
static int sound_search;
static int sound_pain1;
static int sound_pain2;
static int sound_attack;
static int sound_sight;

// Stand
mframe_t army_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t army_move_stand = {0, 7, army_frames_stand, NULL};

void army_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &army_move_stand;
}

// Run
mframe_t army_frames_run [] =
{
	ai_run, 11, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,

	ai_run, 8,	NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 8,	NULL
};
mmove_t army_move_run = {73, 80, army_frames_run, NULL};

void army_run(edict_t *self)
{
	self->monsterinfo.currentmove = &army_move_run;
}

void FireArmy(edict_t *self)
{
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;

	VectorCopy(self->enemy->s.origin, end);
	VectorCopy(self->s.origin, start);
	start[2] += 30;
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	monster_fire_shotgun(self, start, dir, 4, 1, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, 4, MZ2_SOLDIER_SHOTGUN_1);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

// Attack
mframe_t army_frames_attack [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,

	ai_charge, 0,	FireArmy,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,

	ai_charge, 0,	NULL
};
mmove_t army_move_attack = {81, 89, army_frames_attack, army_run};

void army_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &army_move_attack;
}

// Sight
void army_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void army_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

// Pain (1)
mframe_t army_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0,	NULL,
	ai_move, 0, NULL
};
mmove_t army_move_pain1 = {40, 45, army_frames_pain1, army_run};

// Pain (2)
mframe_t army_frames_pain2 [] =
{
	ai_move, 0,		NULL,
	ai_move, 13,	NULL,
	ai_move, 9,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 2,		NULL,
	ai_move, 0,		NULL
};
mmove_t army_move_pain2 = {46, 59, army_frames_pain2, army_run};

// Pain (3)
mframe_t army_frames_pain3 [] =
{
	ai_move, 0,		NULL,
	ai_move, 1,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 1,		NULL,
	ai_move, 1,		NULL,
	ai_move, 0,		NULL,
	ai_move, 1,		NULL,

	ai_move, 4,		NULL,
	ai_move, 3,		NULL,
	ai_move, 6,		NULL,
	ai_move, 8,		NULL,

	ai_move, 2,		NULL
};
mmove_t army_move_pain3 = {60, 72, army_frames_pain3, army_run};

void army_pain(edict_t *self)
{
	float r;
	
	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	r = random();

	if (r < 0.2)
	{
		self->pain_debounce_time = level.time + 0.6;
		self->monsterinfo.currentmove = &army_move_pain1;
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else if (r < 0.6)
	{
		self->pain_debounce_time = level.time + 1.1;
		self->monsterinfo.currentmove = &army_move_pain2;
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
	else
	{
		self->pain_debounce_time = level.time + 1.1;
		self->monsterinfo.currentmove = &army_move_pain3;
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

void army_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
mframe_t army_frames_death1 [] =
{
	ai_move, 0,		NULL,
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
mmove_t army_move_death1 = {8, 17, army_frames_death1, army_dead};

// Death (2)
mframe_t army_frames_death2 [] =
{
	ai_move, 0,		NULL,
	ai_move, -5,	NULL,
	ai_move, -4,	NULL,
	ai_move, -13,	NULL,

	ai_move, -3,	NULL,
	ai_move, -4,	NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t army_move_death2 = {18, 28, army_frames_death2, army_dead};

void army_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

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

	if (random() < 0.5)
		self->monsterinfo.currentmove = &army_move_death1;
	else
		self->monsterinfo.currentmove = &army_move_death2;
}

void SP_monster_q1_army(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/army/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 40);
	self->health = 30;
	self->monster_name = "Grunt";

	if (self->solid == SOLID_NOT)
		return;
	sound_death = gi.soundindex("quake1/army/death1.wav");
	sound_search = gi.soundindex("quake1/army/idle.wav");
	sound_pain1 = gi.soundindex("quake1/army/pain1.wav");
	sound_pain2 = gi.soundindex("quake1/army/pain2.wav");
	sound_attack = gi.soundindex("quake1/army/sattck1.wav");
	sound_sight = gi.soundindex("quake1/army/sight1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -35;
	self->mass = 30;

	self->monsterinfo.stand = army_stand;
	self->monsterinfo.walk = army_run;
	self->monsterinfo.run = army_run;
	self->monsterinfo.attack = army_attack;
	self->monsterinfo.sight = army_sight;
	self->monsterinfo.search = army_search;

	self->pain = army_pain;
	self->die = army_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
