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
// m_enforcer.c

#include "../../game/g_local.h"

static int sound_death;
static int sound_hit;
static int sound_attack;
static int sound_search;
static int sound_pain1;
static int sound_pain2;
static int sound_sight1;
static int sound_sight2;
static int sound_sight3;
static int sound_sight4;

// Stand
mframe_t enforcer_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t enforcer_move_stand = {0, 6, enforcer_frames_stand, NULL};

void enforcer_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &enforcer_move_stand;
}

// Run
mframe_t enforcer_frames_run [] =
{
	ai_run, 18, NULL,
	ai_run, 14, NULL,
	ai_run, 7,	NULL,
	ai_run, 12, NULL,

	ai_run, 14,	NULL,
	ai_run, 14, NULL,
	ai_run, 7,	NULL,
	ai_run, 11,	NULL
};
mmove_t enforcer_move_run = {23, 30, enforcer_frames_run, NULL};

void enforcer_run(edict_t *self)
{
	self->monsterinfo.currentmove = &enforcer_move_run;
}

void enfbolt_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int	mod;

	if (other == self->owner)
		return;
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, 0);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(15);
		gi.WritePosition(self->s.origin);
		gi.WriteDir((!plane) ? vec3_origin : plane->normal);
		gi.WriteByte(226);
		gi.multicast(self->s.origin, MULTICAST_PVS);

		gi.sound (self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_enfbolt(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*bolt;
	trace_t	tr;

	// decino: No enemies left, so stop shooting
	if (!self->enemy || self->enemy == self)
		return;
	// decino: Don't make impossible shots
	VectorCopy(SightEndtToDir(self, dir)[0], dir);
	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);

	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_HYPERBLASTER;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);

	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->owner = self;
	bolt->touch = enfbolt_touch;
	bolt->nextthink = level.time + 5;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "enfbolt";
	gi.linkentity(bolt);

	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}	

void FireEnforcerBolt(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	offset = {30, 8.5, 16};

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;

	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	fire_enfbolt(self, start, dir, 15, 600);
}

// Attack (second half)
mframe_t enforcer_frames_attack2 [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	FireEnforcerBolt,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,

	ai_charge, 0,	NULL
};
mmove_t enforcer_move_attack2 = {35, 40, enforcer_frames_attack2, enforcer_run};

void enforcer_attack_again(edict_t *self)
{
	self->s.frame = 34;
	self->monsterinfo.currentmove = &enforcer_move_attack2;
}

// Attack (first half)
mframe_t enforcer_frames_attack1 [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,

	ai_charge, 0,	NULL,
	ai_charge, 0,	FireEnforcerBolt,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL
};
mmove_t enforcer_move_attack1 = {31, 38, enforcer_frames_attack1, enforcer_attack_again};

void enforcer_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &enforcer_move_attack1;
}

// Sight
void enforcer_sight(edict_t *self)
{
	int r = (int)(random() * 4);

	switch (r)
	{
		case 0: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
		case 1: gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0); break;
		case 2: gi.sound(self, CHAN_VOICE, sound_sight3, 1, ATTN_NORM, 0); break;
		case 3: gi.sound(self, CHAN_VOICE, sound_sight4, 1, ATTN_NORM, 0); break;
		default: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
	}
}

// Search
void enforcer_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

// Pain (1)
mframe_t enforcer_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t enforcer_move_pain1 = {66, 69, enforcer_frames_pain1, enforcer_run};

// Pain (2)
mframe_t enforcer_frames_pain2 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL
};
mmove_t enforcer_move_pain2 = {70, 74, enforcer_frames_pain2, enforcer_run};

// Pain (3)
mframe_t enforcer_frames_pain3 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t enforcer_move_pain3 = {75, 82, enforcer_frames_pain3, enforcer_run};

// Pain (4)
mframe_t enforcer_frames_pain4 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 2, NULL,

	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL,
	ai_move, 1, NULL,

	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL,

	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t enforcer_move_pain4 = {83, 101, enforcer_frames_pain4, enforcer_run};

// Pain
void enforcer_pain(edict_t *self)
{
	float r;

	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	r = random();
	
	if (r < 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	if (r < 0.2)
	{
		self->pain_debounce_time = level.time + 1.0;
		self->monsterinfo.currentmove = &enforcer_move_pain1;
	}
	else if (r < 0.4)
	{
		self->pain_debounce_time = level.time + 1.0;
		self->monsterinfo.currentmove = &enforcer_move_pain2;
	}
	else if (r < 0.7)
	{
		self->pain_debounce_time = level.time + 1.0;
		self->monsterinfo.currentmove = &enforcer_move_pain3;
	}
	else
	{
		self->pain_debounce_time = level.time + 2.0;
		self->monsterinfo.currentmove = &enforcer_move_pain4;
	}
}

void enforcer_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
mframe_t enforcer_frames_death1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 14, NULL,

	ai_move, 2, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 3, NULL,
	ai_move, 5, NULL,
	ai_move, 5, NULL,
	ai_move, 5, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t enforcer_move_death1 = {41, 54, enforcer_frames_death1, enforcer_dead};

// Death (2)
mframe_t enforcer_frames_death2 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t enforcer_move_death2 = {55, 65, enforcer_frames_death2, enforcer_dead};

// Death
void enforcer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
		self->monsterinfo.currentmove = &enforcer_move_death1;
	else
		self->monsterinfo.currentmove = &enforcer_move_death2;
}

void SP_monster_q1_enforcer(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/enforcer/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 40);
	self->health = 80;
	self->monster_name = "Enforcer";

	if (self->solid == SOLID_NOT)
		return;
	sound_death = gi.soundindex("quake1/enforcer/death1.wav");
	sound_hit = gi.soundindex("quake1/enforcer/enfstop.wav");
	sound_attack = gi.soundindex("quake1/enforcer/enfire.wav");
	sound_search = gi.soundindex("quake1/enforcer/idle1.wav");
	sound_pain1 = gi.soundindex("quake1/enforcer/pain1.wav");
	sound_pain2 = gi.soundindex("quake1/enforcer/pain2.wav");
	sound_sight1 = gi.soundindex("quake1/enforcer/sight1.wav");
	sound_sight2 = gi.soundindex("quake1/enforcer/sight2.wav");
	sound_sight3 = gi.soundindex("quake1/enforcer/sight3.wav");
	sound_sight4 = gi.soundindex("quake1/enforcer/sight4.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -35;
	self->mass = 80;

	self->monsterinfo.stand = enforcer_stand;
	self->monsterinfo.walk = enforcer_run;
	self->monsterinfo.run = enforcer_run;
	self->monsterinfo.attack = enforcer_attack;
	self->monsterinfo.sight = enforcer_sight;
	self->monsterinfo.search = enforcer_search;

	self->pain = enforcer_pain;
	self->die = enforcer_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
