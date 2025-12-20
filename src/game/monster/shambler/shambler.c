/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * SHAMBLER
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "shambler.h"

static int sound_pain;
static int sound_idle;
static int sound_die;
static int sound_sight;
static int sound_windup;
static int sound_melee1;
static int sound_melee2;
static int sound_smack;
static int sound_boom;

//
// misc
//

void
shambler_sight(edict_t* self, edict_t* other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static vec3_t lightning_left_hand[] = {
	{ 44, 36, 25},
	{ 10, 44, 57},
	{ -1, 40, 70},
	{ -10, 34, 75},
	{ 7.4f, 24, 89 }
};

static vec3_t lightning_right_hand[] = {
	{ 28, -38, 25},
	{ 31, -7, 70},
	{ 20, 0, 80},
	{ 16, 1.2f, 81},
	{ 27, -11, 83 }
};

void
shambler_lightning_update(edict_t *self)
{
	edict_t *lightning;
	vec3_t f, r;

	if (self->s.frame >= FRAME_magic01 + sizeof(lightning_left_hand) / sizeof(*lightning_left_hand))
	{
		return;
	}

	lightning = G_Spawn();
	lightning->s.modelindex = gi.modelindex("models/proj/lightning/tris.md2");
	lightning->s.renderfx |= RF_BEAM;
	lightning->owner = self;

	AngleVectors(self->s.angles, f, r, NULL);
	G_ProjectSource(self->s.origin, lightning_left_hand[self->s.frame - FRAME_magic01], f, r, lightning->s.origin);
	G_ProjectSource(self->s.origin, lightning_right_hand[self->s.frame - FRAME_magic01], f, r, lightning->s.old_origin);
	gi.linkentity(lightning);
}

void
shambler_windup(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);

	shambler_lightning_update(self);
}

void
shambler_idle(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void
shambler_maybe_idle(edict_t* self)
{
	if (random() > 0.8)
	{
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
	}
}

//
// stand
//

static mframe_t shambler_frames_stand[] =
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
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};

mmove_t shambler_move_stand =
{
	FRAME_stand01,
	FRAME_stand17,
	shambler_frames_stand,
	NULL
};

void
shambler_stand(edict_t* self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &shambler_move_stand;
}

//
// walk
//

void shambler_walk(edict_t* self);

static mframe_t shambler_frames_walk[] =
{
	{ai_walk, 10, NULL}, /* FIXME: add footsteps? */
	{ai_walk, 9, NULL},
	{ai_walk, 9, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 12, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 13, NULL},
	{ai_walk, 9, NULL},
	{ai_walk, 7, shambler_maybe_idle},
	{ai_walk, 5, NULL},
};

mmove_t shambler_move_walk =
{
	FRAME_walk01,
	FRAME_walk12,
	shambler_frames_walk,
	NULL
};

void
shambler_walk(edict_t* self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &shambler_move_walk;
}

//
// run
//

void shambler_run(edict_t* self);

static mframe_t shambler_frames_run[] =
{
	{ai_run, 20, NULL}, /* FIXME: add footsteps? */
	{ai_run, 24, NULL},
	{ai_run, 20, NULL},
	{ai_run, 20, NULL},
	{ai_run, 24, NULL},
	{ai_run, 20, shambler_maybe_idle},
};

mmove_t shambler_move_run =
{
	FRAME_run01,
	FRAME_run06,
	shambler_frames_run,
	NULL
};

void
shambler_run(edict_t* self)
{
	if (!self)
	{
		return;
	}

	if (self->enemy && self->enemy->client)
	{
		self->monsterinfo.aiflags |= AI_BRUTAL;
	}
	else
	{
		self->monsterinfo.aiflags &= ~AI_BRUTAL;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &shambler_move_stand;
		return;
	}

	self->monsterinfo.currentmove = &shambler_move_run;
}

//
// pain
//

// FIXME: needs halved explosion damage

static mframe_t shambler_frames_pain[] = {
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
};

mmove_t shambler_move_pain =
{
	FRAME_pain01,
	FRAME_pain06,
	shambler_frames_pain,
	shambler_run
};

void
shambler_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage /* unused */)
{
	if (!self)
	{
		return;
	}

	if (level.time < self->timestamp)
	{
		return;
	}

	self->timestamp = level.time + (1.0f / 1000.0f);
	gi.sound(self, CHAN_AUTO, sound_pain, 1, ATTN_NORM, 0);

	if (damage <= 30 && random() > 0.2f)
	{
		return;
	}

	/* If hard or nightmare, don't go into pain while attacking */
	if (skill->value >= SKILL_HARDPLUS)
	{
		if ((self->s.frame >= FRAME_smash01) && (self->s.frame <= FRAME_smash12))
			return;

		if ((self->s.frame >= FRAME_swingl01) && (self->s.frame <= FRAME_swingl09))
			return;

		if ((self->s.frame >= FRAME_swingr01) && (self->s.frame <= FRAME_swingr09))
			return;
	}

	if (skill->value == SKILL_HARDPLUS)
	{
		return; /* no pain anims in nightmare */
	}

	if (level.time < self->pain_debounce_time)
	{
		return;
	}

	self->pain_debounce_time = level.time + 2;
	self->monsterinfo.currentmove = &shambler_move_pain;
}

/*
 * attacks
 */

static void
ShamblerSaveLoc(edict_t* self)
{
	/* save for aiming the shot */
	VectorCopy(self->enemy->s.origin, self->pos1);
	self->pos1[2] += self->enemy->viewheight;
	self->monsterinfo.nextframe = FRAME_magic09;

	gi.sound(self, CHAN_WEAPON, sound_boom, 1, ATTN_NORM, 0);
	shambler_lightning_update(self);
}

static void
ShamblerCastLightning(edict_t* self)
{
	vec3_t			start, dir, end;
	trace_t			tr;

	if (!self->enemy)
	{
		return;
	}

	if (!infront(self, self->enemy))
	{
		return;
	}

	if (self->s.frame == FRAME_magic07)
	{
		gi.sound(self, CHAN_WEAPON, sound_boom, 1, ATTN_NORM, 0);
	}

	/* decino: Shambler has an extra lightning frame on Nightmare mode */
	if ((self->s.frame == FRAME_magic10) && (skill->value < SKILL_HARDPLUS))
	{
		return;
	}

	VectorCopy(self->s.origin, start);
	VectorCopy(self->enemy->s.origin, end);

	start[2] += 40;
	end[2] += 16;

	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	VectorMA(start, 600, dir, end);

	tr = gi.trace(start, NULL, NULL, end, self,
		(MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WATER));

	if (!tr.ent)
	{
		return;
	}

	T_Damage(tr.ent, self, self, dir, tr.endpos, tr.plane.normal, 10, 1, 0, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteShort(tr.ent - g_edicts);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(end);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
}

static mframe_t shambler_frames_magic[] = {
	{ai_charge, 0, shambler_windup},
	{ai_charge, 0, shambler_lightning_update},
	{ai_charge, 0, shambler_lightning_update},
	{ai_move, 0, shambler_lightning_update},
	{ai_move, 0, shambler_lightning_update},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, NULL},
};

mmove_t shambler_attack_magic =
{
	FRAME_magic01,
	FRAME_magic12,
	shambler_frames_magic,
	shambler_run
};

void
shambler_attack(edict_t* self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &shambler_attack_magic;
}

//
// melee
//

void
shambler_melee1(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

void
shambler_melee2(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

static void sham_swingl9_step(edict_t* self);
static void sham_swingr9_step(edict_t* self);

static void
sham_smash10_step(edict_t* self)
{
	if (!self->enemy)
		return;

	ai_charge(self, 0);

	if (!CanDamage(self->enemy, self))
		return;

	vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
	if (fire_hit(self, aim, (110 + (randk() % 10)), 120))
	{
		/* Slower attack */
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
	}
};

static void
ShamClaw(edict_t* self)
{
	if (!self->enemy)
		return;

	ai_charge(self, 10);

	if (!CanDamage(self->enemy, self))
		return;

	vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
	if (fire_hit(self, aim, (70 + (randk() % 10)), 80))
	{
		/* Slower attack */
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
	}
};

static mframe_t shambler_frames_smash[] = {
	{ai_charge, 2, shambler_melee1},
	{ai_charge, 6},
	{ai_charge, 6},
	{ai_charge, 5},
	{ai_charge, 4},
	{ai_charge, 1},
	{ai_charge, 0},
	{ai_charge, 0},
	{ai_charge, 0},
	{ai_charge, 0, sham_smash10_step},
	{ai_charge, 5},
	{ai_charge, 4},
};

mmove_t shambler_attack_smash =
{
	FRAME_smash01,
	FRAME_smash12,
	shambler_frames_smash,
	shambler_run
};

static mframe_t shambler_frames_swingl[] = {
	{ai_charge, 5, shambler_melee1},
	{ai_charge, 3},
	{ai_charge, 7},
	{ai_charge, 3},
	{ai_charge, 7},
	{ai_charge, 9},
	{ai_charge, 5, ShamClaw},
	{ai_charge, 4},
	{ai_charge, 8, sham_swingl9_step},
};

mmove_t shambler_attack_swingl =
{
	FRAME_swingl01,
	FRAME_swingl09,
	shambler_frames_swingl,
	shambler_run
};

static mframe_t shambler_frames_swingr[] = {
	{ai_charge, 1, shambler_melee2},
	{ai_charge, 8},
	{ai_charge, 14},
	{ai_charge, 7},
	{ai_charge, 3},
	{ai_charge, 6},
	{ai_charge, 6, ShamClaw},
	{ai_charge, 3},
	{ai_charge, 8, sham_swingr9_step},
};

mmove_t shambler_attack_swingr =
{
	FRAME_swingr01,
	FRAME_swingr09,
	shambler_frames_swingr,
	shambler_run
};

static void
sham_swingl9_step(edict_t* self)
{
	if (!self)
	{
		return;
	}

	ai_charge(self, 8);

	if (self->enemy)
	{
		float real_enemy_range;

		real_enemy_range = realrange(self, self->enemy);

		if ((randk() % 2) && self->enemy && real_enemy_range < MELEE_DISTANCE)
		{
			self->monsterinfo.currentmove = &shambler_attack_swingr;
		}
	}
}

static void
sham_swingr9_step(edict_t* self)
{
	if (!self)
	{
		return;
	}

	ai_charge(self, 1);
	ai_charge(self, 10);

	if (self->enemy)
	{
		float real_enemy_range;

		real_enemy_range = realrange(self, self->enemy);

		if ((randk() % 2) && self->enemy && real_enemy_range < MELEE_DISTANCE)
		{
			self->monsterinfo.currentmove = &shambler_attack_swingl;
		}
	}
}

void
shambler_melee(edict_t* self)
{
	float chance = random();
	if (chance > 0.6 || self->health == 600)
	{
		self->monsterinfo.currentmove = &shambler_attack_smash;
	}
	else if (chance > 0.3)
	{
		self->monsterinfo.currentmove = &shambler_attack_swingl;
	}
	else
	{
		self->monsterinfo.currentmove = &shambler_attack_swingr;
	}
}

//
// death
//

void
shambler_dead(edict_t* self)
{
	if (!self)
	{
		return;
	}

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	monster_dynamic_dead(self);
}

static void
shambler_shrink(edict_t* self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static mframe_t shambler_frames_death[] = {
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, shambler_shrink},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}, /* FIXME: thud? */
};

mmove_t shambler_move_death =
{
	FRAME_death01,
	FRAME_death11,
	shambler_frames_death,
	shambler_dead
};

void
shambler_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage, vec3_t point /* unused */)
{
	/* check for gib */
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		/* FIXME: better gibs for shambler, shambler head */
		ThrowGib(self, NULL, damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/chest/tris.md2",
				damage, GIB_ORGANIC);

		ThrowHead(self, NULL, damage, GIB_ORGANIC);

		self->deadflag = true;
		return;
	}

	if (self->deadflag)
	{
		return;
	}

	/* regular death */
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;

	self->monsterinfo.currentmove = &shambler_move_death;
}

/*
 * QUAKED monster_shambler (1 .5 0) (-32, -32, -24) (32, 32, 64) Ambush Trigger_Spawn Sight
 */
void
SP_monster_shambler(edict_t* self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/shambler/tris.md2");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	gi.modelindex("models/proj/lightning/tris.md2");
	sound_pain = gi.soundindex("shambler/shurt2.wav");
	sound_idle = gi.soundindex("shambler/sidle.wav");
	sound_die = gi.soundindex("shambler/sdeath.wav");
	sound_windup = gi.soundindex("shambler/sattck1.wav");
	sound_melee1 = gi.soundindex("shambler/melee1.wav");
	sound_melee2 = gi.soundindex("shambler/melee2.wav");
	sound_sight = gi.soundindex("shambler/ssight.wav");
	sound_smack = gi.soundindex("shambler/smack.wav");
	sound_boom = gi.soundindex("shambler/sboom.wav");

	self->health = 600 * st.health_multiplier;
	self->gib_health = -60;

	self->mass = 500;

	self->pain = shambler_pain;
	self->die = shambler_die;
	self->monsterinfo.stand = shambler_stand;
	self->monsterinfo.walk = shambler_walk;
	self->monsterinfo.run = shambler_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = shambler_attack;
	self->monsterinfo.melee = shambler_melee;
	self->monsterinfo.sight = shambler_sight;
	self->monsterinfo.idle = shambler_idle;
	self->monsterinfo.blocked = NULL;

	gi.linkentity(self);

	if (self->spawnflags & 1)
	{
		self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	}

	self->monsterinfo.currentmove = &shambler_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
