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
// m_hknight.c

#include "../../game/g_local.h"

static int sound_attack;
static int sound_melee;
static int sound_death;
static int sound_proj_hit;
static int sound_sight;
static int sound_search;
static int sound_pain;

void hknight_run(edict_t *self);
void SwingSword(edict_t *self);

// Stand
mframe_t hknight_frames_stand [] =
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
mmove_t hknight_move_stand = {0, 8, hknight_frames_stand, NULL};

void hknight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &hknight_move_stand;
}

// Charge
mframe_t hknight_frames_charge [] =
{
	ai_charge, 20,	NULL,
	ai_charge, 25,	NULL,
	ai_charge, 18,	NULL,
	ai_charge, 16,	NULL,

	ai_charge, 14,	NULL,
	ai_charge, 20,	SwingSword,
	ai_charge, 21,	SwingSword,
	ai_charge, 13,	SwingSword,

	ai_charge, 20,	SwingSword,
	ai_charge, 20,	SwingSword,
	ai_charge, 18,	SwingSword,
	ai_charge, 16,	NULL,

	ai_charge, 20,	NULL,
	ai_charge, 14,	NULL,
	ai_charge, 25,	NULL,
	ai_charge, 21,	NULL,
};
mmove_t hknight_move_charge = {63, 78, hknight_frames_charge, hknight_run};

qboolean CheckForCharge(edict_t *self)
{
	if (!self->enemy)
		return false;
	if (!visible(self, self->enemy))
		return false;
	if (fabs(self->s.origin[2] - self->enemy->s.origin[2]) > 20)
		return false;
	if (CheckDistance(self, self->enemy) > 320)
		return false;
	return true;
}

// Run
mframe_t hknight_frames_run [] =
{
	ai_run, 20, NULL,
	ai_run, 18, NULL,
	ai_run, 25, NULL,
	ai_run, 16, NULL,

	ai_run, 14,	NULL,
	ai_run, 25, NULL,
	ai_run, 21, NULL,
	ai_run, 13, NULL
};
mmove_t hknight_move_run = {29, 36, hknight_frames_run, NULL};

void hknight_run(edict_t *self)
{
	self->monsterinfo.currentmove = &hknight_move_run;
}

void hknight_reset_magic(edict_t *self)
{
	self->radius_dmg = -2;

	if (self->enemy && CheckDistance(self, self->enemy) < 320 && (random() < 0.75))
		self->monsterinfo.attack_finished = level.time + 1.0;
}

void magic_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
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
		gi.WriteByte(66);
		gi.multicast(self->s.origin, MULTICAST_PVS);

		gi.sound (self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_magic(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*magic;
	trace_t	tr;

	// decino: No enemies left, so stop shooting
	if (!self->enemy || self->enemy == self)
		return;
	// decino: Don't make impossible shots
	VectorCopy(SightEndtToDir(self, dir)[0], dir);
	VectorNormalize(dir);

	magic = G_Spawn();
	magic->svflags = SVF_DEADMONSTER;
	VectorCopy(start, magic->s.origin);
	VectorCopy(start, magic->s.old_origin);
	vectoangles(dir, magic->s.angles);
	VectorScale(dir, speed, magic->velocity);

	magic->movetype = MOVETYPE_FLYMISSILE;
	magic->clipmask = MASK_SHOT;
	magic->solid = SOLID_BBOX;
	magic->s.effects |= EF_IONRIPPER;
	VectorClear(magic->mins);
	VectorClear(magic->maxs);

	magic->s.modelindex = gi.modelindex ("models/proj/fireball/tris.md2");
	magic->owner = self;
	magic->touch = magic_touch;
	magic->nextthink = level.time + 10;
	magic->think = G_FreeEdict;
	magic->dmg = damage;
	magic->classname = "fireball";
	gi.linkentity(magic);

	tr = gi.trace (magic->s.origin, NULL, NULL, magic->s.origin, magic, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(magic->s.origin, -10, dir, magic->s.origin);
		magic->touch(magic, tr.ent, NULL, NULL);
	}
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}	

void FireMagic(edict_t *self)
{
	vec3_t		dir;
	vec3_t		test;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	dir[1] += self->radius_dmg * 60;
	VectorNormalize(dir);

	fire_magic(self, self->s.origin, dir, 9, 300);
	self->radius_dmg++;
}

// Attack
mframe_t hknight_frames_attack [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, hknight_reset_magic,
	ai_charge, 0, FireMagic,
	ai_charge, 0, FireMagic,
	ai_charge, 0, FireMagic,

	ai_charge, 0, FireMagic,
	ai_charge, 0, FireMagic,
	ai_charge, 0, FireMagic
};
mmove_t hknight_move_attack = {155, 165, hknight_frames_attack, hknight_run};

void hknight_attack(edict_t *self)
{
	if (CheckForCharge(self))
		self->monsterinfo.currentmove = &hknight_move_charge;
	else if (self->monsterinfo.attack_finished < level.time)
		self->monsterinfo.currentmove = &hknight_move_attack;
	else
		self->monsterinfo.currentmove = &hknight_move_charge;
}

// Slice
mframe_t hknight_frames_slice [] =
{
	ai_charge, 9,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 13,	NULL,
	ai_charge, 4,	NULL,

	ai_charge, 7,	SwingSword,
	ai_charge, 15,	SwingSword,
	ai_charge, 8,	SwingSword,
	ai_charge, 2,	SwingSword,

	ai_charge, 0,	SwingSword,
	ai_charge, 3,	NULL
};
mmove_t hknight_move_slice = {112, 121, hknight_frames_slice, hknight_run};

// Smash
mframe_t hknight_frames_smash [] =
{
	ai_charge, 1,	NULL,
	ai_charge, 13,	NULL,
	ai_charge, 9,	NULL,
	ai_charge, 11,	NULL,

	ai_charge, 10,	SwingSword,
	ai_charge, 7,	SwingSword,
	ai_charge, 12,	SwingSword,
	ai_charge, 2,	SwingSword,

	ai_charge, 3,	SwingSword,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL
};
mmove_t hknight_move_smash = {122, 132, hknight_frames_smash, hknight_run};

// Watk
mframe_t hknight_frames_watk [] =
{
	ai_charge, 2, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, SwingSword,

	ai_charge, 0, SwingSword,
	ai_charge, 0, SwingSword,
	ai_charge, 1, NULL,
	ai_charge, 4, NULL,

	ai_charge, 5, NULL,
	ai_charge, 3, SwingSword,
	ai_charge, 2, SwingSword,
	ai_charge, 2, SwingSword,

	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 1, NULL,

	ai_charge, 1, SwingSword,
	ai_charge, 3, SwingSword,
	ai_charge, 4, SwingSword,
	ai_charge, 6, NULL,

	ai_charge, 7, NULL,
	ai_charge, 3, NULL,

};
mmove_t hknight_move_watk = {133, 154, hknight_frames_watk, hknight_run};

// Melee
void hknight_melee(edict_t *self)
{
	self->dmg_radius++;
	
	if (self->dmg_radius == 1)
		self->monsterinfo.currentmove = &hknight_move_slice;
	else if (self->dmg_radius == 2)
		self->monsterinfo.currentmove = &hknight_move_smash;
	else if (self->dmg_radius == 3)
	{
		self->monsterinfo.currentmove = &hknight_move_watk;
		self->dmg_radius = 0;
	}
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

// Pain
mframe_t hknight_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL
};
mmove_t hknight_move_pain = {37, 41, hknight_frames_pain, hknight_run};

void hknight_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;

	if (level.time - self->pain_debounce_time > 5)
	{
		self->monsterinfo.currentmove = &hknight_move_pain;
		self->pain_debounce_time = level.time + 1;
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
		return;
	}
	if ((random() * 30 > damage) )
		return;
	self->monsterinfo.currentmove = &hknight_move_pain;
	self->pain_debounce_time = level.time + 1;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

void hknight_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
mframe_t hknight_frames_die1 [] =
{
	ai_move, 0,		NULL,
	ai_move, 10,	NULL,
	ai_move, 8,		NULL,
	ai_move, 0,		NULL,

	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 10,	NULL,

	ai_move, 11,	NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL
};
mmove_t hknight_move_die1 = {42, 53, hknight_frames_die1, hknight_dead};

// Death (2)
mframe_t hknight_frames_die2 [] =
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
mmove_t hknight_move_die2 = {54, 62, hknight_frames_die2, hknight_dead};

void hknight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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

	if (random() < 0.5)
		self->monsterinfo.currentmove = &hknight_move_die1;
	else
		self->monsterinfo.currentmove = &hknight_move_die2;
}

// Sight
void hknight_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void hknight_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_q1_hknight(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/hknight/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 40);
	self->health = 250;
	self->monster_name = "Death Knight";

	if (self->solid == SOLID_NOT)
		return;
	sound_attack = gi.soundindex("quake1/hknight/attack1.wav");
	sound_melee = gi.soundindex("quake1/hknight/slash1.wav");
	sound_death = gi.soundindex("quake1/hknight/death1.wav");
	sound_proj_hit = gi.soundindex("quake1/hknight/hit.wav");
	sound_sight = gi.soundindex("quake1/hknight/sight1.wav");
	sound_search = gi.soundindex("quake1/hknight/idle.wav");
	sound_pain = gi.soundindex("quake1/hknight/pain1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -40;
	self->mass = 250;

	self->monsterinfo.stand = hknight_stand;
	self->monsterinfo.walk = hknight_run;
	self->monsterinfo.run = hknight_run;
	self->monsterinfo.attack = hknight_attack;
	self->monsterinfo.melee = hknight_melee;
	self->monsterinfo.sight = hknight_sight;
	self->monsterinfo.search = hknight_search;

	self->pain = hknight_pain;
	self->die = hknight_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
