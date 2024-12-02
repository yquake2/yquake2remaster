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
#include "zombie.h"

static int sound_sight;
static int sound_search;
static int sound_fling;
static int sound_pain1;
static int sound_pain2;
static int sound_fall;
static int sound_miss;
static int sound_hit;

static void zombie_down(edict_t *self);
static void zombie_get_up_attempt(edict_t *self);

/* Stand */
static mframe_t zombie_frames_stand [] =
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
mmove_t zombie_move_stand =
{
	FRAME_stand1,
	FRAME_stand15,
	zombie_frames_stand,
	NULL
};

void
zombie_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &zombie_move_stand;
}

/* Run */
static mframe_t zombie_frames_run[] =
{
	{ai_run, 1, NULL},
	{ai_run, 1, NULL},
	{ai_run, 0, NULL},
	{ai_run, 1, NULL},

	{ai_run, 2, NULL},
	{ai_run, 3, NULL},
	{ai_run, 4, NULL},
	{ai_run, 4, NULL},

	{ai_run, 2, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},

	{ai_run, 2, NULL},
	{ai_run, 4, NULL},
	{ai_run, 6, NULL},
	{ai_run, 7, NULL},

	{ai_run, 3, NULL},
	{ai_run, 8, NULL}
};

mmove_t zombie_move_run =
{
	FRAME_run1,
	FRAME_run18,
	zombie_frames_run,
	NULL
};

void
zombie_run(edict_t *self)
{
	self->monsterinfo.currentmove = &zombie_move_run;
}

/* Walk */
static mframe_t zombie_frames_walk[] =
{
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},

	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},

	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},

	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},

	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL}
};

mmove_t zombie_move_walk =
{
	FRAME_walk1,
	FRAME_walk19,
	zombie_frames_walk,
	NULL
};

void
zombie_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &zombie_move_walk;
}

/* Sight */
void
zombie_sight(edict_t *self, edict_t *other /* unused */)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void
zombie_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	G_FreeEdict(ent);
}

void
zombie_gib_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}
	if (other->takedamage)
	{
		gi.sound(ent, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
		T_Damage(other, ent, ent->owner, ent->s.origin, ent->s.origin, vec3_origin, ent->dmg, ent->dmg, 0, 0);
		G_FreeEdict(ent);
		return;
	}
	gi.sound(ent, CHAN_WEAPON, sound_miss, 1, ATTN_NORM, 0);
	VectorSet(ent->avelocity, 0, 0, 0);
	VectorSet(ent->velocity, 0, 0, 0);
	ent->touch = zombie_touch;
}

static void
fire_zombie_gib(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed)
{
	edict_t	*gib;
	vec3_t	dir;
	vec3_t	forward, right, up;

	if (!self)
	{
		return;
	}

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	gib = G_Spawn();
	VectorCopy(start, gib->s.origin);
	VectorScale(aimdir, speed, gib->velocity);
	VectorMA(gib->velocity, 200 + crandom() * 10.0, up, gib->velocity);
	VectorMA(gib->velocity, crandom() * 10.0, right, gib->velocity);
	VectorSet(gib->avelocity, 300, 300, 300);
	gib->movetype = MOVETYPE_BOUNCE;
	gib->clipmask = MASK_SHOT;
	gib->solid = SOLID_BBOX;
	gib->s.effects |= EF_GIB;
	VectorClear(gib->mins);
	VectorClear(gib->maxs);
	gib->s.modelindex = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gib->owner = self;
	gib->touch = zombie_gib_touch;
	gib->nextthink = level.time + 2.5;
	gib->think = G_FreeEdict;
	gib->dmg = damage;
	gib->classname = "zombie_gib";

	gi.linkentity(gib);
	gi.sound(self, CHAN_WEAPON, sound_fling, 1, ATTN_NORM, 0);
}

static void
zombie_fire_gib_step(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	aim;
	vec3_t offset = {16, 0, 8};

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(forward, aim);

	fire_zombie_gib(self, start, aim, 10, 600);
}

/* Attack (1) */
static mframe_t zombie_frames_attack1 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, zombie_fire_gib_step}
};
mmove_t zombie_move_attack1 =
{
	FRAME_atta1,
	FRAME_atta13,
	zombie_frames_attack1,
	zombie_run
};

/* Attack (2) */
static mframe_t zombie_frames_attack2 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, zombie_fire_gib_step}
};
mmove_t zombie_move_attack2 =
{
	FRAME_attb1,
	FRAME_attb14,
	zombie_frames_attack2,
	zombie_run
};

/* Attack (3) */
static mframe_t zombie_frames_attack3 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},

	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, zombie_fire_gib_step},
};
mmove_t zombie_move_attack3 =
{
	FRAME_attc1,
	FRAME_attc12,
	zombie_frames_attack3,
	zombie_run
};

/* Attack */
void
zombie_attack(edict_t *self)
{
	float r = random();

	if (r < 0.3)
	{
		self->monsterinfo.currentmove = &zombie_move_attack1;
	}
	else if (r < 0.6)
	{
		self->monsterinfo.currentmove = &zombie_move_attack2;
	}
	else
	{
		self->monsterinfo.currentmove = &zombie_move_attack3;
	}
}

static mframe_t zombie_frames_get_up [] =
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
mmove_t zombie_move_get_up =
{
	FRAME_paine12,
	FRAME_paine30,
	zombie_frames_get_up,
	zombie_run
};

static void
zombie_pain1(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

static void
zombie_pain2(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
}

static void
zombie_hit_floor(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_fall, 1, ATTN_NORM, 0);
}

static void
zombie_get_up(edict_t *self)
{
	VectorSet(self->maxs, 16, 16, 40);
	self->takedamage = DAMAGE_YES;
	self->health = 60;
	zombie_sight(self, NULL);

	if (!M_walkmove(self, 0, 0))
	{
		zombie_get_up_attempt(self);
		return;
	}
	self->monsterinfo.currentmove = &zombie_move_get_up;
}

static void
zombie_start_fall(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

/* Down */
static mframe_t zombie_frames_get_up_attempt [] =
{
	{ai_move, 0, zombie_get_up_attempt}
};

mmove_t zombie_move_get_up_attempt =
{
	FRAME_paine12,
	FRAME_paine12,
	zombie_frames_get_up_attempt,
	NULL
};

static void
zombie_get_up_attempt(edict_t *self)
{
	static int down = 0;

	zombie_down(self);

	/* Try getting up in 5 seconds */
	if (down >= 50)
	{
		down = 0;
		zombie_get_up(self);
		return;
	}
	self->s.frame = FRAME_paine11;
	self->monsterinfo.currentmove = &zombie_move_get_up_attempt;

	down++;
}

static void
zombie_down(edict_t *self)
{
	self->takedamage = DAMAGE_NO;
	self->health = 60;
	VectorSet(self->maxs, 16, 16, 0);
}

/* Pain (1) */
static mframe_t zombie_frames_pain1 [] =
{
	{ai_move, 0, zombie_pain1},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 1, NULL},

	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t zombie_move_pain1 =
{
	FRAME_paina1,
	FRAME_paina12,
	zombie_frames_pain1,
	zombie_run
};

/* Pain (2) */
static mframe_t zombie_frames_pain2 [] =
{
	{ai_move, 0, zombie_pain2},
	{ai_move, 2, NULL},
	{ai_move, 8, NULL},
	{ai_move, 6, NULL},

	{ai_move, 2, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, zombie_hit_floor},
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
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t zombie_move_pain2 =
{
	FRAME_painb1,
	FRAME_painb28,
	zombie_frames_pain2,
	zombie_run
};

/* Pain (3) */
static mframe_t zombie_frames_pain3 [] =
{
	{ai_move, 0, zombie_pain2},
	{ai_move, 0, NULL},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},

	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t zombie_move_pain3 =
{
	FRAME_painc1,
	FRAME_painc18,
	zombie_frames_pain3,
	zombie_run
};

/* Pain (4) */
static mframe_t zombie_frames_pain4 [] =
{
	{ai_move, 0, zombie_pain1},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},

	{ai_move, 0, NULL}
};
mmove_t zombie_move_pain4 =
{
	FRAME_paind1,
	FRAME_paind13,
	zombie_frames_pain4,
	zombie_run
};

/* Pain (5) */
static mframe_t zombie_frames_fall_start [] =
{
	{ai_move, 0, zombie_start_fall},
	{ai_move, -8, NULL},
	{ai_move, -5, NULL},
	{ai_move, -3, NULL},

	{ai_move, -1, NULL},
	{ai_move, -2, NULL},
	{ai_move, -1, NULL},
	{ai_move, -1, NULL},

	{ai_move, -2, NULL},
	{ai_move, 0, zombie_hit_floor},
	{ai_move, 0, zombie_down}
};
mmove_t zombie_move_fall_start =
{
	FRAME_paine1,
	FRAME_paine11,
	zombie_frames_fall_start,
	zombie_get_up_attempt
};

/* Pain */
void
zombie_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float r;
	self->health = 60;

	if (damage < 9)
	{
		return;
	}

	if (self->monsterinfo.currentmove == &zombie_move_fall_start)
	{
		return;
	}

	if (damage >= 25)
	{
		self->monsterinfo.currentmove = &zombie_move_fall_start;
		return;
	}

	if (self->pain_debounce_time > level.time)
	{
		self->monsterinfo.currentmove = &zombie_move_fall_start;
		return;
	}

	if (
		(self->monsterinfo.currentmove == &zombie_move_pain1) ||
		(self->monsterinfo.currentmove == &zombie_move_pain2) ||
		(self->monsterinfo.currentmove == &zombie_move_pain3) ||
		(self->monsterinfo.currentmove == &zombie_move_pain4)
	)
	{
		self->pain_debounce_time = level.time + 3;
		return;
	}

	/* decino: No pain animations in Nightmare mode */
	if (skill->value >= SKILL_HARDPLUS)
	{
		return;
	}

	r = random();

	if (r < 0.25)
	{
		self->monsterinfo.currentmove = &zombie_move_pain1;
	}
	else if (r <  0.5)
	{
		self->monsterinfo.currentmove = &zombie_move_pain2;
	}
	else if (r <  0.75)
	{
		self->monsterinfo.currentmove = &zombie_move_pain3;
	}
	else
	{
		self->monsterinfo.currentmove = &zombie_move_pain4;
	}
}

// Death
void
zombie_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0; n < 2; n++)
	{
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
	}

	for (n = 0; n < 4; n++)
	{
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	}

	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
	self->deadflag = DEAD_DEAD;
}

/* Search */
void
zombie_search(edict_t *self)
{
	if (random() < 0.2)
	{
		gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
	}
}

/*
 * QUAKED monster_zombie (1 .5 0) (-16, -16, -24) (16, 16, 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_zombie(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/zombie/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 60 * st.health_multiplier;

	sound_sight = gi.soundindex("zombie/z_idle.wav");
	sound_search = gi.soundindex("zombie/z_idle.wav");
	sound_fling = gi.soundindex("zombie/z_shot1.wav");
	sound_pain1 = gi.soundindex("zombie/z_pain.wav");
	sound_pain2 = gi.soundindex("zombie/z_pain1.wav");
	sound_fall = gi.soundindex("zombie/z_fall.wav");
	sound_miss = gi.soundindex("zombie/z_miss.wav");
	sound_hit = gi.soundindex("zombie/z_hit.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -5;
	self->mass = 60;

	self->monsterinfo.stand = zombie_stand;
	self->monsterinfo.walk = zombie_walk;
	self->monsterinfo.run = zombie_run;
	self->monsterinfo.attack = zombie_attack;
	self->monsterinfo.sight = zombie_sight;
	self->monsterinfo.search = zombie_search;

	self->pain = zombie_pain;
	self->die = zombie_die;

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
