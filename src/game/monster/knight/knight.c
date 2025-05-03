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
#include "knight.h"

static int sound_death;
static int sound_pain;
static int sound_sight;
static int sound_search;
static int sound_melee1;
static int sound_melee2;

void knight_attack(edict_t *self);

// Stand
static mframe_t knight_frames_stand [] =
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
mmove_t knight_move_stand =
{
	FRAME_stand1,
	FRAME_stand9,
	knight_frames_stand,
	NULL
};

void
knight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_move_stand;
}

// Run
static mframe_t knight_frames_run [] =
{
	{ai_run, 16, NULL},
	{ai_run, 20, NULL},
	{ai_run, 13, NULL},
	{ai_run, 7, NULL},

	{ai_run, 16, NULL},
	{ai_run, 20, NULL},
	{ai_run, 14, NULL},
	{ai_run, 6, knight_attack}
};
mmove_t knight_move_run =
{
	FRAME_runb1,
	FRAME_runb8,
	knight_frames_run,
	NULL
};

void
knight_run(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_move_run;
}

static void
knight_attack_swing(edict_t *self)
{
	if (random() > 0.5)
	{
		gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
	}
}

void
swing_sword_step(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
	{
		return;
	}

	VectorSubtract(self->s.origin, self->enemy->s.origin, dir);

	if (VectorLength(dir) > 100.0)
	{
		return;
	}

	damage = (random() + random() + random()) * 3;

	fire_hit(self, aim, damage, damage);
}

// Attack
static mframe_t knight_frames_attack [] =
{
	{ai_charge, 16, knight_attack_swing},
	{ai_charge, 20, NULL},
	{ai_charge, 13, NULL},
	{ai_charge, 7, NULL},

	{ai_charge, 16, swing_sword_step},
	{ai_charge, 20, swing_sword_step},
	{ai_charge, 14, swing_sword_step},
	{ai_charge, 6, swing_sword_step},

	{ai_charge, 14, swing_sword_step},
	{ai_charge, 10, NULL},
	{ai_charge, 7, NULL}
};
mmove_t knight_move_attack =
{
	FRAME_runattack1,
	FRAME_runattack11,
	knight_frames_attack,
	knight_run
};

void
knight_attack(edict_t *self)
{
	if (self->enemy && realrange(self, self->enemy) < (MELEE_DISTANCE * 4))
	{
		self->monsterinfo.currentmove = &knight_move_attack;
	}
	else
	{
		self->monsterinfo.currentmove = &knight_move_run;
	}
}

static void
knight_melee_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

// Melee
static mframe_t knight_frames_melee [] =
{
	{ai_charge, 0, knight_melee_swing},
	{ai_charge, 7, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 4, swing_sword_step},
	{ai_charge, 4, swing_sword_step},
	{ai_charge, 1, swing_sword_step},
	{ai_charge, 3, NULL},

	{ai_charge, 1, NULL},
	{ai_charge, 5, NULL}
};
mmove_t knight_move_melee =
{
	FRAME_attackb1,
	FRAME_attackb11,
	knight_frames_melee,
	knight_run
};

void
knight_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_move_melee;
}

// Pain (1)
static mframe_t knight_frames_pain1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t knight_move_pain1 =
{
	FRAME_pain1,
	FRAME_pain3,
	knight_frames_pain1,
	knight_run
};

// Pain (2)
static mframe_t knight_frames_pain2 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 3, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 2, NULL},
	{ai_move, 4, NULL},
	{ai_move, 2, NULL},
	{ai_move, 5, NULL},

	{ai_move, 5, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t knight_move_pain2 =
{
	FRAME_painb1,
	FRAME_painb11,
	knight_frames_pain2,
	knight_run
};

void
knight_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
	// decino: No pain animations in Nightmare mode
	if (skill->value == SKILL_HARDPLUS)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	if (random() < 0.85)
		self->monsterinfo.currentmove = &knight_move_pain1;
	else
		self->monsterinfo.currentmove = &knight_move_pain2;
	self->pain_debounce_time = level.time + 1;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

void
knight_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

// Death (1)
static mframe_t knight_frames_die1 [] =
{
	{ai_move, 0, NULL},
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
mmove_t knight_move_die1 =
{
	FRAME_death1,
	FRAME_death10,
	knight_frames_die1,
	knight_dead
};

// Death (2)
static mframe_t knight_frames_die2 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
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
mmove_t knight_move_die2 =
{
	FRAME_deathb1,
	FRAME_deathb11,
	knight_frames_die2,
	knight_dead
};

void
knight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
		self->monsterinfo.currentmove = &knight_move_die1;
	else
		self->monsterinfo.currentmove = &knight_move_die2;
}

// Search
void
knight_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

// Sight
void
knight_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

/*
 * QUAKED monster_knight (1 .5 0) (-16, -16, -24) (16, 16, 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_knight(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/knight/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 75 * st.health_multiplier;

	sound_melee1 = gi.soundindex("knight/sword1.wav");
	sound_melee2 = gi.soundindex("knight/sword2.wav");
	sound_death = gi.soundindex("knight/kdeath.wav");
	sound_pain = gi.soundindex("knight/khurt.wav");
	sound_sight = gi.soundindex("knight/ksight.wav");
	sound_search = gi.soundindex("knight/idle.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -40;
	self->mass = 75;

	self->monsterinfo.stand = knight_stand;
	self->monsterinfo.walk = knight_run;
	self->monsterinfo.run = knight_run;
	self->monsterinfo.attack = knight_attack;
	self->monsterinfo.melee = knight_melee;
	self->monsterinfo.sight = knight_sight;
	self->monsterinfo.search = knight_search;

	self->pain = knight_pain;
	self->die = knight_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
