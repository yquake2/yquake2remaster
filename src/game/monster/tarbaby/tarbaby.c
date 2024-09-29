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
#include "tarbaby.h"

static int sound_death;
static int sound_hit;
static int sound_land;
static int sound_sight;

void tarbaby_rejump(edict_t *self);

static void
tarbaby_unbounce(edict_t *self)
{
	self->movetype = MOVETYPE_STEP;
}

// Stand
static mframe_t tarbaby_frames_stand [] =
{
	{ai_stand, 0, tarbaby_unbounce}
};
mmove_t tarbaby_move_stand =
{
	FRAME_walk1,
	FRAME_walk1,
	tarbaby_frames_stand,
	NULL
};

void
tarbaby_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &tarbaby_move_stand;
}

// Run
static mframe_t tarbaby_frames_run [] =
{
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},

	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},

	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},

	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},

	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},

	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},
	{ai_run, 2, NULL},

	{ai_run, 2, NULL}
};
mmove_t tarbaby_move_run =
{
	FRAME_run1,
	FRAME_run25,
	tarbaby_frames_run,
	NULL
};

void
tarbaby_run(edict_t *self)
{
	self->monsterinfo.currentmove = &tarbaby_move_run;
}

// Sight
void
tarbaby_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void
tarbaby_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->takedamage)
	{
		if (VectorLength(self->velocity) > 400)
		{
			int	damage = 10 + 10 * random();

			T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, damage, 0, 0, 0);
			gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
		}
	}
	else
		gi.sound(self, CHAN_WEAPON, sound_land, 1, ATTN_NORM, 0);
	self->touch = NULL;

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.currentmove = &tarbaby_move_run;
			self->movetype = MOVETYPE_STEP;
		}
		return;
	}
}

static void
tarbaby_jump_step(edict_t *self)
{
	vec3_t forward;

	self->movetype = MOVETYPE_BOUNCE;
	self->touch = tarbaby_touch;

	AngleVectors (self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	VectorScale(forward, 600, self->velocity);
	self->velocity[2] = 200 + (random() * 150);
	self->groundentity = NULL;
}

// Fly
static mframe_t tarbaby_frames_fly [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t tarbaby_move_fly =
{
	FRAME_fly1,
	FRAME_fly4,
	tarbaby_frames_fly,
	tarbaby_rejump
};

void
tarbaby_fly(edict_t *self)
{
	self->monsterinfo.currentmove = &tarbaby_move_fly;
}

// Jump
static mframe_t tarbaby_frames_jump [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, tarbaby_jump_step},
	{ai_charge, 0, NULL}
};
mmove_t tarbaby_move_jump =
{
	FRAME_jump1,
	FRAME_jump6,
	tarbaby_frames_jump,
	tarbaby_fly
};

void
tarbaby_rejump(edict_t *self)
{
	self->monsterinfo.currentmove = &tarbaby_move_jump;
	self->monsterinfo.nextframe = 54;
}

// Attack
void
tarbaby_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &tarbaby_move_jump;
}

void
tarbaby_explode(edict_t *self)
{
	T_RadiusDamage(self, self, 120, NULL, 160, 0);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_BIGEXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

// Death
void
tarbaby_die(edict_t *self, edict_t *inflictor /* unused */,
	edict_t *attacker /* unused */, int damage, vec3_t point /* unused */)
{
	if (self->deadflag == DEAD_DEAD)
		return;
	self->s.frame = FRAME_exp;
	self->deadflag = DEAD_DEAD;
	self->think = tarbaby_explode;
	self->nextthink = level.time + 0.1;
}

// Pain
void
tarbaby_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{

}

/*
 * QUAKED monster_tarbaby (1 .5 0) (-16, -16, -24) (16, 16, 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_tarbaby(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/tarbaby/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80;

	sound_death = gi.soundindex("tarbaby/death1.wav");
	sound_hit = gi.soundindex("tarbaby/hit1.wav");
	sound_land = gi.soundindex("tarbaby/land1.wav");
	sound_sight = gi.soundindex("tarbaby/sight1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = 0;
	self->mass = 80;

	self->monsterinfo.stand = tarbaby_stand;
	self->monsterinfo.walk = tarbaby_run;
	self->monsterinfo.run = tarbaby_run;
	self->monsterinfo.attack = tarbaby_attack;
	self->monsterinfo.sight = tarbaby_sight;

	self->die = tarbaby_die;
	self->pain = tarbaby_pain;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
