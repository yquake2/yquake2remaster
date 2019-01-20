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
// m_shalrath.c

#include "../../game/g_local.h"

static int sound_death;
static int sound_search;
static int sound_pain;
static int sound_attack;
static int sound_fire;
static int sound_sight;

// Stand
mframe_t shalrath_frames_stand [] =
{
	ai_stand, 0, NULL
};
mmove_t shalrath_move_stand = {0, 0, shalrath_frames_stand, NULL};

void shalrath_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &shalrath_move_stand;
}

// Run
mframe_t shalrath_frames_run [] =
{
	ai_run, 6, NULL,
	ai_run, 4, NULL,
	ai_run, 0, NULL,
	ai_run, 0, NULL,

	ai_run, 0, NULL,
	ai_run, 0, NULL,
	ai_run, 5, NULL,
	ai_run, 6, NULL,

	ai_run, 5, NULL,
	ai_run, 0, NULL,
	ai_run, 4, NULL,
	ai_run, 5, NULL
};
mmove_t shalrath_move_run = {23, 34, shalrath_frames_run, NULL};

void shalrath_run(edict_t *self)
{
	self->monsterinfo.currentmove = &shalrath_move_run;
}

void shalrath_roar(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0);
}

void shalrath_pod_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;
	if (strcmp(other->classname, "monster_q1_zombie") == 0) // decino: According to shalrath.qc
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 110, 0, 0, 0);
	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg + 40, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void shalrath_pod_home(edict_t *self)
{
	static qboolean think = false;
	vec3_t			end;
	vec3_t			dir;

	// decino: Only home every 0.2 frames
	if (think)
	{
		if (self->enemy && self->enemy->health < 1 && self->owner->health > 0)
		{
			// decino: Not a big fan of this, just wait until it hits something
			/*if (!self->owner->enemy || (self->owner->enemy == self->owner))
			{
				G_FreeEdict(self);
				return;
			}*/				
		}
		if (self->owner->enemy)
		{
			self->enemy = self->owner->enemy;
			VectorCopy(self->enemy->s.origin, end);
			end[2] += self->enemy->viewheight;
			VectorSubtract(end, self->s.origin, dir);
			VectorNormalize(dir);
			VectorScale(dir, (skill->value >= 3) ? 350 : 250, self->velocity);
		}
	}

	// decino: Draw particles each frame
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(15);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(255);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	self->nextthink = level.time + 0.1;
	self->think = shalrath_pod_home;
	think = !think;
}

void fire_shalrath_pod(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*pod;

	// decino: No enemies left, so stop shooting
	if (!self->enemy || self->enemy == self)
		return;
	// decino: Don't make impossible shots
	VectorCopy(SightEndtToDir(self, dir)[0], dir);
	VectorNormalize(dir);

	pod = G_Spawn();
	VectorCopy(start, pod->s.origin);
	VectorCopy(start, pod->s.old_origin);
	vectoangles(dir, pod->s.angles);
	VectorScale(dir, speed, pod->velocity);

	VectorSet(pod->avelocity, 300, 300, 300);
	pod->movetype = MOVETYPE_FLYMISSILE;
	pod->clipmask = MASK_SHOT;
	pod->solid = SOLID_BBOX;
	VectorClear(pod->mins);
	VectorClear(pod->maxs);
	pod->s.modelindex = gi.modelindex("models/proj/pod/tris.md2");
	pod->owner = self;
	pod->touch = shalrath_pod_touch;
	pod->nextthink = level.time + 0.1;
	pod->think = shalrath_pod_home;
	pod->dmg = damage;
	pod->classname = "shalrath_pod";
	pod->enemy = self->enemy;

	gi.linkentity(pod);
	gi.sound(self, CHAN_WEAPON, sound_fire, 1, ATTN_NORM, 0);
}

void FireShalrathPod(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t offset = {16, 0, 16};

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;

	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	fire_shalrath_pod(self, start, dir, 40, 400);
}

// Attack
mframe_t shalrath_frames_attack [] =
{
	ai_charge, 0, shalrath_roar,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,

	ai_charge, 0, FireShalrathPod,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t shalrath_move_attack = {0, 10, shalrath_frames_attack, shalrath_run};

void shalrath_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &shalrath_move_attack;
}

// Pain
mframe_t shalrath_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL
};
mmove_t shalrath_move_pain = {11, 15, shalrath_frames_pain, shalrath_run};

void shalrath_pain(edict_t *self)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	self->monsterinfo.currentmove = &shalrath_move_pain;
	self->pain_debounce_time = level.time + 3.0;

	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

void shalrath_dead(edict_t *self)
{
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death
mframe_t shalrath_frames_death [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t shalrath_move_death = {16, 22, shalrath_frames_death, shalrath_dead};

void shalrath_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->monsterinfo.currentmove = &shalrath_move_death;
}

// Sight
void shalrath_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void shalrath_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_q1_shalrath(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/quake1/shalrath/tris.md2");
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 48);
	self->health = 400;
	self->monster_name = "Vore";

	if (self->solid == SOLID_NOT)
		return;
	sound_death = gi.soundindex("quake1/shalrath/death.wav");
	sound_search = gi.soundindex("quake1/shalrath/idle.wav");
	sound_pain = gi.soundindex("quake1/shalrath/pain.wav");
	sound_attack = gi.soundindex("quake1/shalrath/attack.wav");
	sound_fire = gi.soundindex("quake1/shalrath/attack2.wav");
	sound_sight = gi.soundindex("quake1/shalrath/sight.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -90;
	self->mass = 400;

	self->monsterinfo.stand = shalrath_stand;
	self->monsterinfo.walk = shalrath_run;
	self->monsterinfo.run = shalrath_run;
	self->monsterinfo.attack = shalrath_attack;
	self->monsterinfo.sight = shalrath_sight;
	self->monsterinfo.search = shalrath_search;

	self->pain = shalrath_pain;
	self->die = shalrath_die;

	self->monsterinfo.scale = 1.000000;
	gi.linkentity(self);

	walkmonster_start(self);
}
