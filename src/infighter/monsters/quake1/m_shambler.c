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
// m_shambler.c

#include "../../game/g_local.h"

static int sound_attack;
static int sound_lightning;
static int sound_death;
static int sound_pain;
static int sound_idle;
static int sound_sight;
static int sound_melee1;
static int sound_melee2;
static int sound_smack;

// Stand
mframe_t shambler_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

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
mmove_t shambler_move_stand = {0, 16, shambler_frames_stand, NULL};

void shambler_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &shambler_move_stand;
}

// Run
mframe_t shambler_frames_run [] =
{
	ai_run, 20, NULL,
	ai_run, 24, NULL,
	ai_run, 20, NULL,
	ai_run, 20, NULL,

	ai_run, 24, NULL,
	ai_run, 20, NULL
};
mmove_t shambler_move_run = {29, 34, shambler_frames_run, NULL};

void shambler_run(edict_t *self)
{
	self->monsterinfo.currentmove = &shambler_move_run;
}

void CastLightning(edict_t *self)
{
	vec3_t			start, dir, end;
	trace_t			tr;

	// decino: No enemies left, so stop shooting
	if (!self->enemy || self->enemy == self)
		return;
	if (!infront(self, self->enemy))
		return;
	if (self->s.frame == 72)
		gi.sound (self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);

	// decino: Shambler has an extra lightning frame on Nightmare mode
	if (self->s.frame == 75 && skill->value < 3)
		return;
	VectorCopy(self->s.origin, start);
	VectorCopy(self->enemy->s.origin, end);

	start[2] += 40;
	end[2] += 16;

	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	VectorMA(start, 600, dir, end);

	tr = gi.trace(start, NULL, NULL, end, self, (MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WATER));

	if (!tr.ent)
		return;
	T_Damage(tr.ent, self, self, dir, tr.endpos, tr.plane.normal, 10, 1, 0, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteShort(tr.ent - g_edicts);  
	gi.WriteShort(self - g_edicts);    
	gi.WritePosition(end);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
}

void shambler_prepare(edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_lightning, 1, ATTN_NORM, 0);
}

void shambler_sparks(edict_t *self)
{
	vec3_t	end;
	VectorCopy(self->s.origin, end);

	end[2] += 80;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(15);
	gi.WritePosition(end);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(15);
	gi.multicast(end, MULTICAST_PVS);
}

// Lightning
mframe_t shambler_frames_lightning [] =
{
	ai_charge, 0, shambler_prepare,
	ai_charge, 0, shambler_sparks,
	ai_charge, 0, shambler_sparks,
	ai_charge, 0, NULL,

	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, CastLightning,

	ai_charge, 0, CastLightning,
	ai_charge, 0, CastLightning,
	ai_charge, 0, CastLightning,
	ai_charge, 0, NULL
};
mmove_t shambler_move_lightning = {65, 76, shambler_frames_lightning, shambler_run};

void shambler_lightning(edict_t *self)
{
	self->monsterinfo.currentmove = &shambler_move_lightning;
}

void shambler_prepare_smash(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee1, 1, ATTN_NORM, 0);
}

void shambler_prepare_claw(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee2, 1, ATTN_NORM, 0);
}

void ShamblerSmash(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = (random() + random() + random()) * 40;

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
}

void ShamblerClaw(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
		return;
	damage = (random() + random() + random()) * 20;

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
}

// Claw (right)
mframe_t shambler_frames_clawr [] =
{
	ai_charge, 1,	shambler_prepare_smash,
	ai_charge, 8,	NULL,
	ai_charge, 14,	NULL,
	ai_charge, 7,	NULL,

	ai_charge, 3,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 10,	ShamblerClaw,

	ai_charge, 3,	NULL
};
mmove_t shambler_move_clawr = {47, 55, shambler_frames_clawr, shambler_run};

// Claw (left)
mframe_t shambler_frames_clawl [] =
{
	ai_charge, 5,	shambler_prepare_claw,
	ai_charge, 3,	NULL,
	ai_charge, 7,	NULL,
	ai_charge, 3,	NULL,

	ai_charge, 7,	NULL,
	ai_charge, 9,	NULL,
	ai_charge, 5,	NULL,
	ai_charge, 10,	ShamblerClaw,

	ai_charge, 4,	NULL
};
mmove_t shambler_move_clawl = {56, 64, shambler_frames_clawl, shambler_run};

// Smash
mframe_t shambler_frames_smash [] =
{
	ai_charge, 6, shambler_prepare_smash,
	ai_charge, 6, NULL,
	ai_charge, 5, NULL,
	ai_charge, 4, NULL,

	ai_charge, 1, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ShamblerSmash,
	ai_charge, 0, NULL
};
mmove_t shambler_move_smash = {35, 46, shambler_frames_smash, shambler_run};

void shambler_melee(edict_t *self)
{
	float r;
	r = random();

	if (r > 0.6 || self->health == 600)
		self->monsterinfo.currentmove = &shambler_move_smash;
	else if (r > 0.3)
		self->monsterinfo.currentmove = &shambler_move_clawr;
	else
		self->monsterinfo.currentmove = &shambler_move_clawl;
}

void shambler_search(edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void shambler_sight(edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Pain
mframe_t shambler_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t shambler_move_pain = {77, 82, shambler_frames_pain, shambler_run};

void shambler_pain(edict_t *self)
{
	if (level.time < self->pain_debounce_time)
		return;
	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	self->monsterinfo.currentmove = &shambler_move_pain;
}

void shambler_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Die
mframe_t shambler_frames_death [] =
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
mmove_t shambler_move_death = {83, 93, shambler_frames_death, shambler_dead};

void shambler_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->monsterinfo.currentmove = &shambler_move_death;
}

void SP_monster_q1_shambler(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/shambler/tris.md2");
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 64);
	self->health = 600;
	self->monster_name = "Shambler";

	if (self->solid == SOLID_NOT)
		return;
	sound_attack = gi.soundindex("quake1/shambler/sboom.wav");
	sound_lightning = gi.soundindex("quake1/shambler/sattck1.wav");
	sound_death = gi.soundindex("quake1/shambler/sdeath.wav");
	sound_pain = gi.soundindex("quake1/shambler/shurt2.wav");
	sound_idle = gi.soundindex("quake1/shambler/sidle.wav");
	sound_sight = gi.soundindex("quake1/shambler/ssight.wav");
	sound_melee1 = gi.soundindex("quake1/shambler/melee1.wav");
	sound_melee2 = gi.soundindex("quake1/shambler/melee2.wav");
	sound_smack = gi.soundindex("quake1/shambler/smack.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -60;
	self->mass = 600;

	self->monsterinfo.stand = shambler_stand;
	self->monsterinfo.walk = shambler_run;
	self->monsterinfo.run = shambler_run;
	self->monsterinfo.attack = shambler_lightning;
	self->monsterinfo.sight = shambler_sight;
	self->monsterinfo.search = shambler_search;
	self->monsterinfo.melee = shambler_melee;

	self->pain = shambler_pain;
	self->die = shambler_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity (self);

	walkmonster_start(self);
}
