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
#include "wizard.h"

static int sound_proj_hit;
static int sound_attack;
static int sound_death;
static int sound_idle1;
static int sound_idle2;
static int sound_pain;
static int sound_sight;

// Stand
static mframe_t wizard_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},

	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},

	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},

	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
};
mmove_t wizard_move_stand =
{
	FRAME_hover1,
	FRAME_hover15,
	wizard_frames_stand,
	NULL
};

void
wizard_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &wizard_move_stand;
}

// Run
static mframe_t wizard_frames_run [] =
{
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},

	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},

	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},

	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL}
};
mmove_t wizard_move_run =
{
	FRAME_fly1,
	FRAME_fly14,
	wizard_frames_run,
	NULL
};

void
wizard_run(edict_t *self)
{
	self->monsterinfo.currentmove = &wizard_move_run;
}

static void
wizard_frame(edict_t *self)
{
	static int frame = 0;

	self->s.frame = (FRAME_magatt5 - frame);
	frame++;

	if (frame > 5)
	{
		frame = 0;
	}
}

// decino: Quake plays this animation backwards, so we'll have to do some hacking
static mframe_t wizard_frames_finish [] =
{
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},

	{ai_charge, 0, wizard_frame}
};
mmove_t wizard_move_finish =
{
	FRAME_magatt1,
	FRAME_magatt5,
	wizard_frames_finish,
	wizard_run
};

void
wizard_finish_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &wizard_move_finish;
}

void
spit_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
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
		gi.WriteByte(209);
		gi.multicast(self->s.origin, MULTICAST_PVS);

		gi.sound (self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

static void
spit_fire(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*spit;
	trace_t	tr;

	if (!self)
	{
		return;
	}

	spit = G_Spawn();
	spit->svflags = SVF_DEADMONSTER;
	VectorCopy(start, spit->s.origin);
	VectorCopy(start, spit->s.old_origin);
	vectoangles(dir, spit->s.angles);
	VectorScale(dir, speed, spit->velocity);

	spit->movetype = MOVETYPE_FLYMISSILE;
	spit->clipmask = MASK_SHOT;
	spit->solid = SOLID_BBOX;
	spit->s.effects |= (EF_BLASTER|EF_TRACKER);
	VectorClear(spit->mins);
	VectorClear(spit->maxs);

	spit->s.modelindex = gi.modelindex("models/proj/spit/tris.md2");
	spit->owner = self;
	spit->touch = spit_touch;
	spit->nextthink = level.time + 2;
	spit->think = G_FreeEdict;
	spit->dmg = damage;
	spit->classname = "spit";
	gi.linkentity(spit);

	tr = gi.trace (self->s.origin, NULL, NULL, spit->s.origin, spit, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(spit->s.origin, -10, dir, spit->s.origin);
		spit->touch(spit, tr.ent, NULL, NULL);
	}
}

static void
wizard_spit(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	offset = {0, 0, 30};

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;

	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	spit_fire(self, start, dir, 9, 600);
}

static void
wizard_prespit(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

// Attack
static mframe_t wizard_frames_attack [] =
{
	{ai_charge, 0, wizard_prespit},
	{ai_charge, 0, wizard_spit},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, wizard_spit},
	{ai_charge, 0, NULL}
};
mmove_t wizard_move_attack =
{
	FRAME_magatt1,
	FRAME_magatt6,
	wizard_frames_attack,
	wizard_finish_attack
};

void
wizard_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &wizard_move_attack;
}

// Pain
static mframe_t wizard_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t wizard_move_pain =
{
	FRAME_pain1,
	FRAME_pain4,
	wizard_frames_pain,
	wizard_run
};

void
wizard_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
	if (level.time < self->pain_debounce_time)
		return;
	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	// decino: No pain animations in Nightmare mode
	if (skill->value == SKILL_HARDPLUS)
		return;
	self->monsterinfo.currentmove = &wizard_move_pain;
}

static void
wizard_fling(edict_t *self)
{
	self->velocity[0] = -200 + 400 * random();
	self->velocity[1] = -200 + 400 * random();
	self->velocity[2] = 100 + 100 * random();

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
}

void
wizard_dead(edict_t *self)
{
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death
static mframe_t wizard_frames_death [] =
{
	{ai_move, 0, wizard_fling},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t wizard_move_death =
{
	FRAME_death1,
	FRAME_death8,
	wizard_frames_death,
	wizard_dead
};

void
wizard_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->monsterinfo.currentmove = &wizard_move_death;
}

void
wizard_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void
wizard_search(edict_t *self)
{
	float r;
	r = random() * 5;

	if (r > 4.5)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0);
	if (r < 1.5)
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

/*
 * QUAKED monster_wizard (1 .5 0) (-16, -16, -24) (16, 16, 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_wizard(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/wizard/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80 * st.health_multiplier;

	sound_proj_hit = gi.soundindex("wizard/hit.wav");
	sound_attack = gi.soundindex("wizard/wattack.wav");
	sound_death = gi.soundindex("wizard/wdeath.wav");
	sound_idle1 = gi.soundindex("wizard/widle1.wav");
	sound_idle2 = gi.soundindex("wizard/widle2.wav");
	sound_pain = gi.soundindex("wizard/wpain.wav");
	sound_sight = gi.soundindex("wizard/wsight.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -40;
	self->mass = 80;

	self->monsterinfo.stand = wizard_stand;
	self->monsterinfo.walk = wizard_run;
	self->monsterinfo.run = wizard_run;
	self->monsterinfo.attack = wizard_attack;
	self->monsterinfo.sight = wizard_sight;
	self->monsterinfo.search = wizard_search;

	self->pain = wizard_pain;
	self->die = wizard_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	flymonster_start(self);
}
