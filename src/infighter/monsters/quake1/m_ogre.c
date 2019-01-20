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
// m_ogre.c

#include "../../game/g_local.h"

static int sound_death;
static int sound_attack;
static int sound_melee;
static int sound_sight;
static int sound_search;
static int sound_idle;
static int sound_pain;

void ogre_idle(edict_t *self)
{
	if (random() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

// Stand
mframe_t ogre_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, ogre_idle,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL
};
mmove_t ogre_move_stand = {0, 8, ogre_frames_stand, NULL};

void ogre_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_stand;
}

// Run
mframe_t ogre_frames_run [] =
{
	ai_run, 9,	NULL,
	ai_run, 12,	NULL,
	ai_run, 8,	NULL,
	ai_run, 22,	NULL,
	ai_run, 16,	NULL,

	ai_run, 4,	NULL,
	ai_run, 13,	NULL,
	ai_run, 24,	NULL
};
mmove_t ogre_move_run = {25, 32, ogre_frames_run, NULL};

void ogre_run(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_run;
}

void OgreChainsaw(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = (random() + random() + random()) * 4;

	fire_hit(self, aim, damage, damage);
}

// Smash
mframe_t ogre_frames_smash [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, 4,	NULL,

	ai_charge, 14,	OgreChainsaw,
	ai_charge, 14,	OgreChainsaw,
	ai_charge, 20,	OgreChainsaw,
	ai_charge, 23,	OgreChainsaw,

	ai_charge, 10,	OgreChainsaw,
	ai_charge, 12,	OgreChainsaw,
	ai_charge, 1,	NULL,
	ai_charge, 4,	NULL,

	ai_charge, 12,	NULL,
	ai_charge, 0,	NULL
};
mmove_t ogre_move_smash = {47, 60, ogre_frames_smash, ogre_run};

// Swing
mframe_t ogre_frames_swing [] =
{
	ai_charge, 11,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, 4,	NULL,
	ai_charge, 19,	OgreChainsaw,

	ai_charge, 13,	OgreChainsaw,
	ai_charge, 10,	OgreChainsaw,
	ai_charge, 10,	OgreChainsaw,
	ai_charge, 10,	OgreChainsaw,

	ai_charge, 10,	OgreChainsaw,
	ai_charge, 10,	OgreChainsaw,
	ai_charge, 3,	NULL,
	ai_charge, 8,	NULL,

	ai_charge, 9,	NULL,
	ai_charge, 0,	NULL
};
mmove_t ogre_move_swing = {33, 46, ogre_frames_swing, ogre_run};

// Melee
void ogre_melee(edict_t *self)
{
	if (random() > 0.5)
		self->monsterinfo.currentmove = &ogre_move_smash;
	else
		self->monsterinfo.currentmove = &ogre_move_swing;
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

void FireOgreGrenade(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	aim;
	vec3_t offset = {0, 0, 16};

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(forward, aim);

	monster_fire_grenade(self, start, aim, 40, 600, MZ2_GUNNER_GRENADE_1);
}

// Grenade
mframe_t ogre_frames_attack [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	FireOgreGrenade,

	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL
};
mmove_t ogre_move_attack = {61, 66, ogre_frames_attack, ogre_run};

void ogre_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_move_attack;
}

// Pain (1)
mframe_t ogre_frames_pain1 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL
};
mmove_t ogre_move_pain1 = {67, 71, ogre_frames_pain1, ogre_run};

// Pain (2)
mframe_t ogre_frames_pain2 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t ogre_move_pain2 = {72, 74, ogre_frames_pain2, ogre_run};

// Pain (3)
mframe_t ogre_frames_pain3 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t ogre_move_pain3 = {75, 80, ogre_frames_pain3, ogre_run};

// Pain (4)
mframe_t ogre_frames_pain4 [] =
{
	ai_move, 0,		NULL,
	ai_move, 10,	NULL,
	ai_move, 9,		NULL,
	ai_move, 4,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
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
mmove_t ogre_move_pain4 = {81, 96, ogre_frames_pain4, ogre_run};

// Pain (5)
mframe_t ogre_frames_pain5 [] =
{
	ai_move, 0,		NULL,
	ai_move, 10,	NULL,
	ai_move, 9,		NULL,
	ai_move, 4,		NULL,

	ai_move, 0,		NULL,
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
mmove_t ogre_move_pain5 = {97, 111, ogre_frames_pain5, ogre_run};

// Pain
void ogre_pain(edict_t *self)
{
	float r;

	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = random();
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	
	if (r < 0.25)
	{
		self->monsterinfo.currentmove = &ogre_move_pain1;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.5)
	{
		self->monsterinfo.currentmove = &ogre_move_pain2;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.75)
	{
		self->monsterinfo.currentmove = &ogre_move_pain3;
		self->pain_debounce_time = level.time + 1.0;
	}
	else if (r < 0.88)
	{
		self->monsterinfo.currentmove = &ogre_move_pain4;
		self->pain_debounce_time = level.time + 2.0;
	}
	else
	{
		self->monsterinfo.currentmove = &ogre_move_pain5;
		self->pain_debounce_time = level.time + 2.0;
	}
}

void ogre_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
mframe_t ogre_frames_death1 [] =
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
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t ogre_move_death1 = {112, 125, ogre_frames_death1, ogre_dead};

// Death (2)
mframe_t ogre_frames_death2 [] =
{
	ai_move, 0,		NULL,
	ai_move, 5,		NULL,
	ai_move, 0,		NULL,
	ai_move, 1,		NULL,

	ai_move, 3,		NULL,
	ai_move, 7,		NULL,
	ai_move, 25,	NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t ogre_move_death2 = {126, 135, ogre_frames_death2, ogre_dead};

// Death
void ogre_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
		self->monsterinfo.currentmove = &ogre_move_death1;
	else
		self->monsterinfo.currentmove = &ogre_move_death2;
}

// Sight
void ogre_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void ogre_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_q1_ogre(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/ogre/tris.md2");
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 64);
	self->health = 200;
	self->monster_name = "Ogre";

	if (self->solid == SOLID_NOT)
		return;
	sound_death = gi.soundindex("quake1/ogre/ogdth.wav");
	sound_attack = gi.soundindex("quake1/ogre/grenade.wav");
	sound_melee = gi.soundindex("quake1/ogre/ogsawatk.wav");
	sound_sight = gi.soundindex("quake1/ogre/ogwake.wav");
	sound_search = gi.soundindex("quake1/ogre/ogidle2.wav");
	sound_idle = gi.soundindex("quake1/ogre/ogidle.wav");
	sound_pain = gi.soundindex("quake1/ogre/ogpain1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -80;
	self->mass = 200;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_run;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = ogre_melee;
	self->monsterinfo.sight = ogre_sight;
	self->monsterinfo.search = ogre_search;

	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
