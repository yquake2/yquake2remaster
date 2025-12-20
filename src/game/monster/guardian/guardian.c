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
 * The Guardian.
 *
 * =======================================================================
 */

#include "../../header/local.h"
#include "guardian.h"

void BossExplode(edict_t *self);

static int sound_charge;
static int sound_spin_loop;

//
// stand
//

void
guardian_stand(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.firstframe = FRAME_idle1;
	self->monsterinfo.lastframe = FRAME_idle52;
	monster_dynamic_stand(self);
}

//
// walk
//

static int sound_step = 0;

void
guardian_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1.f, ATTN_NORM, 0.0f);
}

static mframe_t guardian_frames_walk[] = {
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, guardian_footstep},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, guardian_footstep},
	{ai_walk, 8, NULL}
};

mmove_t guardian_move_walk =
{
	FRAME_walk1,
	FRAME_walk19,
	guardian_frames_walk,
	NULL
};

void
guardian_walk(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &guardian_move_walk;
}

//
// run
//

static mframe_t guardian_frames_run[] = {
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, guardian_footstep},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, NULL},
	{ai_run, 8, guardian_footstep},
	{ai_run, 8, NULL}
};

mmove_t guardian_move_run =
{
	FRAME_walk1,
	FRAME_walk19,
	guardian_frames_run,
	NULL
};

void
guardian_run(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		guardian_stand(self);
		return;
	}

	self->monsterinfo.currentmove = &guardian_move_run;
}

//
// pain
//

void
guardian_pain(edict_t *self, edict_t *other /* other */,
		float kick /* other */, int damage)
{
	if (!self)
	{
		return;
	}

	if (damage <= 10)
	{
		return;
	}

	if (level.time < self->pain_debounce_time)
	{
		return;
	}

	if (damage <= 75)
	{
		if (random() > 0.2)
		{
			return;
		}
	}

	/* don't go into pain while attacking */
	if ((self->s.frame >= FRAME_atk1_spin1) && (self->s.frame <= FRAME_atk1_spin15))
	{
		return;
	}

	if ((self->s.frame >= FRAME_atk2_fire1) && (self->s.frame <= FRAME_atk2_fire4))
	{
		return;
	}

	if ((self->s.frame >= FRAME_kick_in1) && (self->s.frame <= FRAME_kick_in13))
	{
		return;
	}

	self->pain_debounce_time = level.time + 3;

	if (skill->value == SKILL_HARDPLUS)
	{
		return; /* no pain anims in nightmare */
	}

	self->monsterinfo.firstframe = FRAME_pain1_1;
	self->monsterinfo.lastframe = FRAME_pain1_8;
	monster_dynamic_pain(self, other, kick, damage);
}

static mframe_t guardian_frames_atk1_out[] = {
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};

mmove_t guardian_atk1_out =
{
	FRAME_atk1_out1,
	FRAME_atk1_out3,
	guardian_frames_atk1_out,
	guardian_run
};

void
guardian_atk1_finish(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &guardian_atk1_out;
}

void
guardian_atk1_charge(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_charge, 1.f, ATTN_NORM, 0.f);
}

void
guardian_fire_blaster(edict_t *self)
{
	vec3_t forward, right, target;
	vec3_t start;
	int id = MZ2_TANK_BLASTER_1;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[id], forward, right, start);
	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	for (int i = 0; i < 3; i++)
	{
		target[i] += (randk() % 10) - 5.f;
	}

	/* calc direction to where we targeted */
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	monster_fire_blaster(self, start, forward, 2, 1000, id, (self->s.frame % 4) ? 0 : EF_HYPERBLASTER);

	if (self->enemy && self->enemy->health > 0 &&
		self->s.frame == FRAME_atk1_spin12 && self->timestamp > level.time && visible(self, self->enemy))
		self->monsterinfo.nextframe = FRAME_atk1_spin5;
}

void
guardian_hyper_sound(edict_t *self)
{
	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_spin_loop, 1.f, ATTN_NORM, 0.f);
}

static mframe_t guardian_frames_atk1_spin[] = {
	{ai_charge, 0, guardian_atk1_charge},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_hyper_sound},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0, guardian_fire_blaster},
	{ai_charge, 0},
	{ai_charge, 0},
	{ai_charge, 0}
};

mmove_t guardian_move_atk1_spin =
{
	FRAME_atk1_spin1,
	FRAME_atk1_spin15,
	guardian_frames_atk1_spin,
	guardian_atk1_finish
};

void
guardian_atk1(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &guardian_move_atk1_spin;
	self->timestamp = level.time + 0.650f + (randk() % 10 ) / 10.f * 1.5f;
}

static mframe_t guardian_frames_atk1_in[] = {
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};

mmove_t guardian_move_atk1_in =
{
	FRAME_atk1_in1,
	FRAME_atk1_in3,
	guardian_frames_atk1_in,
	guardian_atk1
};

static mframe_t guardian_frames_atk2_out[] = {
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};

mmove_t guardian_move_atk2_out =
{
	FRAME_atk2_out1,
	FRAME_atk2_out7,
	guardian_frames_atk2_out,
	guardian_run
};

void
guardian_atk2_out(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &guardian_move_atk2_out;
}

static int sound_laser;

static vec3_t laser_positions[] = {
	{125.0f, -70.f, 60.f},
	{112.0f, -62.f, 60.f}
};

void
guardian_laser_fire(edict_t *self)
{
	vec3_t forward, right, up;
	vec3_t tempang, start;
	vec3_t dir, angles, end;
	vec3_t tempvec;
	edict_t *ent;

	if (!self)
	{
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_laser, 1.f, ATTN_NORM, 0.f);

	if (random() > 0.8)
	{
		gi.sound(self, CHAN_AUTO, gi.soundindex("misc/lasfly.wav"), 1, ATTN_STATIC, 0);
	}

	VectorCopy(self->s.origin, start);
	VectorCopy(self->enemy->s.origin, end);
	VectorSubtract(end, start, dir);
	vectoangles(dir, angles);
	VectorCopy(laser_positions[1 - (self->s.frame & 1)], tempvec);

	ent = G_Spawn();
	VectorCopy(self->s.origin, ent->s.origin);
	VectorCopy(angles, tempang);
	AngleVectors(tempang, forward, right, up);
	VectorCopy(tempang, ent->s.angles);
	VectorCopy(ent->s.origin, start);

	VectorMA(start, tempvec[0] + 2, right, start);
	VectorMA(start, tempvec[2] + 8, up, start);
	VectorMA(start, tempvec[1], forward, start);

	VectorCopy(start, ent->s.origin);
	ent->enemy = self->enemy;
	ent->owner = self;

	ent->dmg = 25;

	monster_dabeam(ent);
}

static mframe_t guardian_frames_atk2_fire[] = {
	{ai_charge, 0, guardian_laser_fire},
	{ai_charge, 0, guardian_laser_fire},
	{ai_charge, 0, guardian_laser_fire},
	{ai_charge, 0, guardian_laser_fire}
};

mmove_t guardian_move_atk2_fire =
{
	FRAME_atk2_fire1,
	FRAME_atk2_fire4,
	guardian_frames_atk2_fire,
	guardian_atk2_out
};

void
guardian_atk2(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = &guardian_move_atk2_fire;
}

static mframe_t guardian_frames_atk2_in[] = {
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};

mmove_t guardian_move_atk2_in =
{
	FRAME_atk2_in1,
	FRAME_atk2_in12,
	guardian_frames_atk2_in,
	guardian_atk2
};

void
guardian_kick(edict_t *self)
{
	vec3_t aim = {MELEE_DISTANCE, 0, -80};
	fire_hit(self, aim, 85, 700);
}

static mframe_t guardian_frames_kick[] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_kick},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, guardian_footstep},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};

mmove_t guardian_move_kick =
{
	FRAME_kick_in1,
	FRAME_kick_in13,
	guardian_frames_kick,
	guardian_run
};

void
guardian_attack(edict_t *self)
{
	float r;

	if (!self->enemy || !self->enemy->inuse)
	{
		return;
	}

	r = realrange(self, self->enemy);

	if (r > RANGE_NEAR)
	{
		self->monsterinfo.currentmove = &guardian_move_atk2_in;
	}
	else if (r < 120.f)
	{
		self->monsterinfo.currentmove = &guardian_move_kick;
	}
	else
	{
		self->monsterinfo.currentmove = &guardian_move_atk1_in;
	}
}

//
// death
//

void
guardian_explode(edict_t *self)
{
	vec3_t start, pos;
	int i;

	VectorAdd(self->s.origin, self->mins, start);

	VectorCopy(self->size, pos);
	for (i = 0; i < 3; i++)
	{
		pos[i] *= random();
	}

	VectorAdd(start, pos, pos);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(pos);
	gi.multicast(self->s.origin, MULTICAST_ALL);
}

static const char *gibs[] = {
	"models/monsters/guardian/gib1.md2",
	"models/monsters/guardian/gib2.md2",
	"models/monsters/guardian/gib3.md2",
	"models/monsters/guardian/gib4.md2",
	"models/monsters/guardian/gib5.md2",
	"models/monsters/guardian/gib6.md2",
	"models/monsters/guardian/gib7.md2"
};

void
guardian_dead(edict_t *self)
{
	int i, n;

	for (i = 0; i < 3; i++)
	{
		guardian_explode(self);
	}

	for (n = 0; n < 2; n++)
	{
		ThrowGib(self, NULL, 125, GIB_ORGANIC);
	}

	for (n = 0; n < 4; n++)
	{
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2",
				125, GIB_METALLIC);
	}

	for (n = 0; n < 6; n++)
	{
		for (i = 0; i < 2; i++)
		{
			ThrowGib(self, gibs[n], 125, GIB_METALLIC);
		}
	}

	ThrowHead(self, gibs[6], 125, GIB_METALLIC);
}

static mframe_t guardian_frames_death1[FRAME_death26 - FRAME_death1 + 1] = {
	{ai_move, 0, BossExplode},
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
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};

mmove_t guardian_move_death =
{
	FRAME_death1,
	FRAME_death26,
	guardian_frames_death1,
	guardian_dead
};

void
guardian_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage, vec3_t point /* unused */)
{
	if (!self)
	{
		return;
	}

	/* regular death */
	self->deadflag = true;
	self->takedamage = true;

	self->monsterinfo.currentmove = &guardian_move_death;
}

//
// monster_guardian
//

/*
 * QUAKED monster_guardian (1 .5 0) (-96 -96 -66) (96 96 62) Ambush Trigger_Spawn Sight
 */
void
SP_monster_guardian(edict_t *self)
{
	int i;

	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_step = gi.soundindex("zortemp/step.wav");
	sound_charge = gi.soundindex("weapons/hyprbu1a.wav");
	sound_spin_loop = gi.soundindex("weapons/hyprbl1a.wav");
	sound_laser = gi.soundindex("weapons/laser2.wav");

	for(i = 0; i < sizeof(gibs) / sizeof(*gibs); i++)
	{
		gi.modelindex(gibs[i]);
	}

	self->s.modelindex = gi.modelindex("models/monsters/guardian/tris.md2");
	VectorSet(self->mins, -96, -96, -66);
	VectorSet(self->maxs, 96, 96, 62);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 2500 * st.health_multiplier;
	self->gib_health = -200;

	self->monsterinfo.scale = MODEL_SCALE;

	self->mass = 850;

	self->pain = guardian_pain;
	self->die = guardian_die;
	self->monsterinfo.stand = guardian_stand;
	self->monsterinfo.walk = guardian_walk;
	self->monsterinfo.run = guardian_run;
	self->monsterinfo.attack = guardian_attack;

	gi.linkentity(self);

	guardian_stand(self);

	walkmonster_start(self);
}
