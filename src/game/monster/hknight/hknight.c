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
#include "hknight.h"

static int sound_attack;
static int sound_melee;
static int sound_death;
static int sound_proj_hit;
static int sound_sight;
static int sound_search;
static int sound_pain;

void hknight_run(edict_t *self);
void swing_sword_step(edict_t *self);

// Stand
static mframe_t hknight_frames_stand [] =
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
};
mmove_t hknight_move_stand =
{
	FRAME_stand1,
	FRAME_stand9,
	hknight_frames_stand,
	NULL
};

void
hknight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &hknight_move_stand;
}

// Charge
static mframe_t hknight_frames_charge [] =
{
	{ai_charge, 20,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 18,	NULL},
	{ai_charge, 16,	NULL},

	{ai_charge, 14,	NULL},
	{ai_charge, 20,	swing_sword_step},
	{ai_charge, 21,	swing_sword_step},
	{ai_charge, 13,	swing_sword_step},

	{ai_charge, 20,	swing_sword_step},
	{ai_charge, 20,	swing_sword_step},
	{ai_charge, 18,	swing_sword_step},
	{ai_charge, 16,	NULL},

	{ai_charge, 20,	NULL},
	{ai_charge, 14,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 21,	NULL},
};
mmove_t hknight_move_charge =
{
	FRAME_char_a1,
	FRAME_char_a16,
	hknight_frames_charge,
	hknight_run
};

static qboolean
check_for_charge(edict_t *self)
{
	if (!self->enemy)
		return false;
	if (!visible(self, self->enemy))
		return false;
	if (fabs(self->s.origin[2] - self->enemy->s.origin[2]) > 20)
		return false;
	if (realrange(self, self->enemy) > 320)
		return false;
	return true;
}

// Run
static mframe_t hknight_frames_run [] =
{
	{ai_run, 20, NULL},
	{ai_run, 18, NULL},
	{ai_run, 25, NULL},
	{ai_run, 16, NULL},

	{ai_run, 14, NULL},
	{ai_run, 25, NULL},
	{ai_run, 21, NULL},
	{ai_run, 13, NULL}
};
mmove_t hknight_move_run =
{
	FRAME_run1,
	FRAME_run8,
	hknight_frames_run,
	NULL
};

void
hknight_run(edict_t *self)
{
	self->monsterinfo.currentmove = &hknight_move_run;
}

static void
hknight_reset_magic(edict_t *self)
{
	self->radius_dmg = -2;

	if (self->enemy && realrange(self, self->enemy) < 320 && (random() < 0.75))
	{
		self->monsterinfo.attack_finished = level.time + 1.0;
	}
}

void
magic_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
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
		gi.WriteByte(66);
		gi.multicast(self->s.origin, MULTICAST_PVS);

		gi.sound (self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

static void
fire_magic(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*magic;
	trace_t	tr;

	if (!self)
	{
		return;
	}

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

	magic->s.modelindex = gi.modelindex("models/proj/fireball/tris.md2");
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

static void
fire_magic_step(edict_t *self)
{
	vec3_t dir;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	dir[1] += self->radius_dmg * 60;
	VectorNormalize(dir);

	fire_magic(self, self->s.origin, dir, 9, 300);
	self->radius_dmg++;
}

// Attack
static mframe_t hknight_frames_attack [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, hknight_reset_magic},
	{ai_charge, 0, fire_magic_step},
	{ai_charge, 0, fire_magic_step},
	{ai_charge, 0, fire_magic_step},

	{ai_charge, 0, fire_magic_step},
	{ai_charge, 0, fire_magic_step},
	{ai_charge, 0, fire_magic_step}
};
mmove_t hknight_move_attack =
{
	FRAME_magicc1,
	FRAME_magicc11,
	hknight_frames_attack,
	hknight_run
};

void
hknight_attack(edict_t *self)
{
	if (check_for_charge(self))
		self->monsterinfo.currentmove = &hknight_move_charge;
	else if (self->monsterinfo.attack_finished < level.time)
		self->monsterinfo.currentmove = &hknight_move_attack;
	else
		self->monsterinfo.currentmove = &hknight_move_charge;
}

// Slice
static mframe_t hknight_frames_slice [] =
{
	{ai_charge, 9, NULL},
	{ai_charge, 6, NULL},
	{ai_charge, 13, NULL},
	{ai_charge, 4, NULL},

	{ai_charge, 7, swing_sword_step},
	{ai_charge, 15, swing_sword_step},
	{ai_charge, 8, swing_sword_step},
	{ai_charge, 2, swing_sword_step},

	{ai_charge, 0, swing_sword_step},
	{ai_charge, 3, NULL}
};
mmove_t hknight_move_slice =
{
	FRAME_slice1,
	FRAME_slice10,
	hknight_frames_slice,
	hknight_run
};

// Smash
static mframe_t hknight_frames_smash [] =
{
	{ai_charge, 1, NULL},
	{ai_charge, 13, NULL},
	{ai_charge, 9, NULL},
	{ai_charge, 11, NULL},

	{ai_charge, 10, swing_sword_step},
	{ai_charge, 7, swing_sword_step},
	{ai_charge, 12, swing_sword_step},
	{ai_charge, 2, swing_sword_step},

	{ai_charge, 3, swing_sword_step},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t hknight_move_smash =
{
	FRAME_smash1,
	FRAME_smash11,
	hknight_frames_smash,
	hknight_run
};

// Watk
static mframe_t hknight_frames_watk [] =
{
	{ai_charge, 2, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, swing_sword_step},

	{ai_charge, 0, swing_sword_step},
	{ai_charge, 0, swing_sword_step},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},

	{ai_charge, 5, NULL},
	{ai_charge, 3, swing_sword_step},
	{ai_charge, 2, swing_sword_step},
	{ai_charge, 2, swing_sword_step},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 1, NULL},

	{ai_charge, 1, swing_sword_step},
	{ai_charge, 3, swing_sword_step},
	{ai_charge, 4, swing_sword_step},
	{ai_charge, 6, NULL},

	{ai_charge, 7, NULL},
	{ai_charge, 3, NULL},

};
mmove_t hknight_move_watk =
{
	FRAME_w_attack1,
	FRAME_w_attack22,
	hknight_frames_watk,
	hknight_run
};

// Melee
void
hknight_melee(edict_t *self)
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
static mframe_t hknight_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL}
};
mmove_t hknight_move_pain =
{
	FRAME_pain1,
	FRAME_pain5,
	hknight_frames_pain,
	hknight_run
};

void
hknight_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == SKILL_HARDPLUS)
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

void
hknight_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
static mframe_t hknight_frames_die1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 10, NULL},
	{ai_move, 8, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 10, NULL},

	{ai_move, 11, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t hknight_move_die1 =
{
	FRAME_death1,
	FRAME_death12,
	hknight_frames_die1,
	hknight_dead
};

// Death (2)
static mframe_t hknight_frames_die2 [] =
{
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
mmove_t hknight_move_die2 =
{
	FRAME_deathb1,
	FRAME_deathb9,
	hknight_frames_die2,
	hknight_dead
};

void
hknight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
void
hknight_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
void
hknight_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

/*
 * QUAKED monster_hknight (1 .5 0) (-16, -16, -24) (16, 16, 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_hknight(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/hknight/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 250 * st.health_multiplier;

	sound_attack = gi.soundindex("hknight/attack1.wav");
	sound_melee = gi.soundindex("hknight/slash1.wav");
	sound_death = gi.soundindex("hknight/death1.wav");
	sound_proj_hit = gi.soundindex("hknight/hit.wav");
	sound_sight = gi.soundindex("hknight/sight1.wav");
	sound_search = gi.soundindex("hknight/idle.wav");
	sound_pain = gi.soundindex("hknight/pain1.wav");

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

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
