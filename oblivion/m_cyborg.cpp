// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_flash.h"

namespace
{
	enum
	{
		CYBORG_FRAME_WALK_FIRST = 0,
		CYBORG_FRAME_WALK_LAST = 17,
		CYBORG_FRAME_RUN_FIRST = 18,
		CYBORG_FRAME_RUN_LAST = 23,
		CYBORG_FRAME_ATTACK1_FIRST = 24,
		CYBORG_FRAME_ATTACK1_LAST = 35,
		CYBORG_FRAME_ATTACK_BACKFLIP_FIRST = 36,
		CYBORG_FRAME_ATTACK_BACKFLIP_LAST = 46,
		CYBORG_FRAME_ATTACK2_FIRST = 47,
		CYBORG_FRAME_ATTACK2_LAST = 52,
		CYBORG_FRAME_ATTACK3_FIRST = 53,
		CYBORG_FRAME_ATTACK3_LAST = 58,
		CYBORG_FRAME_MELEE1_FIRST = 59,
		CYBORG_FRAME_MELEE1_LAST = 66,
		CYBORG_FRAME_MELEE2_FIRST = 67,
		CYBORG_FRAME_MELEE2_LAST = 72,
		CYBORG_FRAME_PAIN1_FIRST = 73,
		CYBORG_FRAME_PAIN1_LAST = 78,
		CYBORG_FRAME_PAIN2_FIRST = 79,
		CYBORG_FRAME_PAIN2_LAST = 81,
		CYBORG_FRAME_PAIN2_END = 82,
		CYBORG_FRAME_MOVE_FIRST = 93,
		CYBORG_FRAME_MOVE_LAST = 104,
		CYBORG_FRAME_DEATH1_FIRST = 105,
		CYBORG_FRAME_DEATH1_LAST = 112,
		CYBORG_FRAME_DEATH2_FIRST = 113,
		CYBORG_FRAME_DEATH2_LAST = 118,
		CYBORG_FRAME_DEATH3_FIRST = 119,
		CYBORG_FRAME_DEATH3_LAST = 124,
		CYBORG_FRAME_STAND = 125
	};

	constexpr float MODEL_SCALE = 1.0f;
	constexpr gtime_t CYBORG_PAIN_COOLDOWN = 3_sec;
	constexpr gtime_t CYBORG_BACKFLIP_TIMEOUT = 3_sec;
	constexpr int CYBORG_DEATOM_DAMAGE = 50;
	constexpr int CYBORG_DEATOM_SPEED = 600;
	constexpr int CYBORG_MELEE_DAMAGE_MIN = 10;
	constexpr int CYBORG_MELEE_DAMAGE_MAX = 14;
	constexpr int CYBORG_MELEE_KICK = 100;
	constexpr int CYBORG_COLLISION_DAMAGE_MIN = 40;
	constexpr int CYBORG_COLLISION_DAMAGE_MAX = 49;
	constexpr float CYBORG_COLLISION_SPEED = 400.0f;
	constexpr float CYBORG_RANGE_MIN = 100.0f;
	constexpr float CYBORG_RANGE_SKIP_CHANCE = 0.2f;
	constexpr monster_muzzleflash_id_t CYBORG_MZ_LEFT = MZ2_CYBORG_DEATOM_LEFT;
	constexpr monster_muzzleflash_id_t CYBORG_MZ_RIGHT = MZ2_CYBORG_DEATOM_RIGHT;

	cached_soundindex sound_attack1;
	cached_soundindex sound_attack2;
	cached_soundindex sound_attack3;
	cached_soundindex sound_death;
	cached_soundindex sound_idle;
	cached_soundindex sound_pain1;
	cached_soundindex sound_pain2;
	cached_soundindex sound_sight;
	cached_soundindex sound_search;
	cached_soundindex sound_step1;
	cached_soundindex sound_step2;
	cached_soundindex sound_step3;
	cached_soundindex sound_thud;
}

static void cyborg_footstep(edict_t *self);
static void cyborg_fire_right(edict_t *self);
static void cyborg_fire_left(edict_t *self);
static void cyborg_fire_both(edict_t *self);
static void cyborg_attack_start(edict_t *self);
static void cyborg_attack_end(edict_t *self);
static void cyborg_hit_left(edict_t *self);
static void cyborg_hit_right(edict_t *self);
static void cyborg_hit_alt(edict_t *self);
static void cyborg_dead(edict_t *self);
static bool cyborg_check_range(edict_t *self);
static void cyborg_touch(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self);

void cyborg_stand(edict_t *self);
void cyborg_run(edict_t *self);

mframe_t cyborg_frames_stand[] = {
	{ ai_stand }
};
MMOVE_T(cyborg_move_stand) = { CYBORG_FRAME_STAND, CYBORG_FRAME_STAND, cyborg_frames_stand, nullptr };

mframe_t cyborg_frames_idle[] = {
	{ ai_stand }
};
MMOVE_T(cyborg_move_idle) = { CYBORG_FRAME_STAND, CYBORG_FRAME_STAND, cyborg_frames_idle, cyborg_stand };

mframe_t cyborg_frames_walk[] = {
	{ ai_walk, 12 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 8 },
	{ ai_walk, 13 },
	{ ai_walk, 12 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 8 },
	{ ai_walk, 13 }
};
MMOVE_T(cyborg_move_walk) = { CYBORG_FRAME_WALK_FIRST, CYBORG_FRAME_WALK_LAST, cyborg_frames_walk, nullptr };

mframe_t cyborg_frames_run[] = {
	{ ai_run, 6 },
	{ ai_run, 23, cyborg_footstep },
	{ ai_run, 8 },
	{ ai_run, 6, cyborg_footstep },
	{ ai_run, 23 },
	{ ai_run, 8 }
};
MMOVE_T(cyborg_move_run) = { CYBORG_FRAME_RUN_FIRST, CYBORG_FRAME_RUN_LAST, cyborg_frames_run, nullptr };

mframe_t cyborg_frames_attack1[] = {
	{ ai_charge, 4 },
	{ ai_charge, 4 },
	{ ai_charge, 5 },
	{ ai_charge, 7 },
	{ ai_charge, 7 },
	{ ai_charge, 9, cyborg_fire_right },
	{ ai_charge, 4 },
	{ ai_charge, 4 },
	{ ai_charge, 5 },
	{ ai_charge, 7 },
	{ ai_charge, 7 },
	{ ai_charge, 9, cyborg_fire_left }
};
MMOVE_T(cyborg_move_attack1) = { CYBORG_FRAME_ATTACK1_FIRST, CYBORG_FRAME_ATTACK1_LAST, cyborg_frames_attack1, cyborg_run };

mframe_t cyborg_frames_attack_backflip[] = {
	{ ai_charge, 0 },
	{ ai_charge, -17 },
	{ ai_charge, -15, cyborg_attack_start },
	{ ai_charge, -15 },
	{ ai_charge, -15 },
	{ ai_charge, -15 },
	{ ai_charge, -15 },
	{ ai_charge, -15, cyborg_attack_end },
	{ ai_charge, 0, cyborg_fire_both },
	{ ai_charge, 3 },
	{ ai_charge, 0 }
};
MMOVE_T(cyborg_move_attack_backflip) = { CYBORG_FRAME_ATTACK_BACKFLIP_FIRST, CYBORG_FRAME_ATTACK_BACKFLIP_LAST, cyborg_frames_attack_backflip, cyborg_run };

mframe_t cyborg_frames_attack2[] = {
	{ ai_charge, 0, cyborg_fire_right },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 }
};
MMOVE_T(cyborg_move_attack2) = { CYBORG_FRAME_ATTACK2_FIRST, CYBORG_FRAME_ATTACK2_LAST, cyborg_frames_attack2, cyborg_run };

mframe_t cyborg_frames_attack3[] = {
	{ ai_charge, 0, cyborg_fire_left },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 }
};
MMOVE_T(cyborg_move_attack3) = { CYBORG_FRAME_ATTACK3_FIRST, CYBORG_FRAME_ATTACK3_LAST, cyborg_frames_attack3, cyborg_run };

mframe_t cyborg_frames_melee1[] = {
	{ ai_charge, 8 },
	{ ai_charge, 10 },
	{ ai_charge, 0, cyborg_hit_right },
	{ ai_charge, 0 },
	{ ai_charge, -5, cyborg_hit_alt },
	{ ai_charge, -5 },
	{ ai_charge, -5 },
	{ ai_charge, -5 }
};
MMOVE_T(cyborg_move_melee1) = { CYBORG_FRAME_MELEE1_FIRST, CYBORG_FRAME_MELEE1_LAST, cyborg_frames_melee1, cyborg_run };

mframe_t cyborg_frames_melee2[] = {
	{ ai_charge, 6 },
	{ ai_charge, 6 },
	{ ai_charge, 6 },
	{ ai_charge, -3, cyborg_hit_left },
	{ ai_charge, -3 },
	{ ai_charge, -10 }
};
MMOVE_T(cyborg_move_melee2) = { CYBORG_FRAME_MELEE2_FIRST, CYBORG_FRAME_MELEE2_LAST, cyborg_frames_melee2, cyborg_run };

mframe_t cyborg_frames_pain1[] = {
	{ ai_move, -16 },
	{ ai_move, -4 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 }
};
MMOVE_T(cyborg_move_pain1) = { CYBORG_FRAME_PAIN1_FIRST, CYBORG_FRAME_PAIN1_LAST, cyborg_frames_pain1, cyborg_run };

mframe_t cyborg_frames_pain2[] = {
	{ ai_move, -11 },
	{ ai_move, -8 },
	{ ai_move, 4 }
};
MMOVE_T(cyborg_move_pain2) = { CYBORG_FRAME_PAIN2_FIRST, CYBORG_FRAME_PAIN2_LAST, cyborg_frames_pain2, cyborg_run };

mframe_t cyborg_frames_pain2_end[] = {
	{ ai_move, 0 }
};
MMOVE_T(cyborg_move_pain2_end) = { CYBORG_FRAME_PAIN2_END, CYBORG_FRAME_PAIN2_END, cyborg_frames_pain2_end, cyborg_run };

mframe_t cyborg_frames_move[] = {
	{ ai_move, 8 },
	{ ai_move, 7 },
	{ ai_move, 3 },
	{ ai_move, 0 },
	{ ai_move, -2 },
	{ ai_move, -3 },
	{ ai_move, 2 },
	{ ai_move, 5 },
	{ ai_move, 16 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 }
};
MMOVE_T(cyborg_move_move) = { CYBORG_FRAME_MOVE_FIRST, CYBORG_FRAME_MOVE_LAST, cyborg_frames_move, cyborg_run };

mframe_t cyborg_frames_death1[] = {
	{ ai_move, -2 },
	{ ai_move, 0 },
	{ ai_move, -3 },
	{ ai_move, 0 },
	{ ai_move, -1 },
	{ ai_move, -2 },
	{ ai_move, -3 },
	{ ai_move, -2 }
};
MMOVE_T(cyborg_move_death1) = { CYBORG_FRAME_DEATH1_FIRST, CYBORG_FRAME_DEATH1_LAST, cyborg_frames_death1, cyborg_dead };

mframe_t cyborg_frames_death2[] = {
	{ ai_move, -6 },
	{ ai_move, -4 },
	{ ai_move, -2 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 }
};
MMOVE_T(cyborg_move_death2) = { CYBORG_FRAME_DEATH2_FIRST, CYBORG_FRAME_DEATH2_LAST, cyborg_frames_death2, cyborg_dead };

mframe_t cyborg_frames_death3[] = {
	{ ai_move, 8 },
	{ ai_move, 4 },
	{ ai_move, 2 },
	{ ai_move, 1 },
	{ ai_move, 0 },
	{ ai_move, 0 }
};
MMOVE_T(cyborg_move_death3) = { CYBORG_FRAME_DEATH3_FIRST, CYBORG_FRAME_DEATH3_LAST, cyborg_frames_death3, cyborg_dead };

static void cyborg_footstep(edict_t *self)
{
	switch (irandom(0, 2))
	{
	case 0:
		gi.sound(self, CHAN_VOICE, sound_step1, 1.0f, ATTN_NORM, 0.0f);
		return;
	case 1:
		gi.sound(self, CHAN_VOICE, sound_step2, 1.0f, ATTN_NORM, 0.0f);
		return;
	default:
		gi.sound(self, CHAN_VOICE, sound_step3, 1.0f, ATTN_NORM, 0.0f);
		return;
	}
}

MONSTERINFO_IDLE(cyborg_idle) (edict_t *self) -> void
{
	M_SetAnimation(self, &cyborg_move_idle);
	gi.sound(self, CHAN_VOICE, sound_idle, 1.0f, ATTN_IDLE, 0.0f);
}

MONSTERINFO_SEARCH(cyborg_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_SIGHT(cyborg_sight) (edict_t *self, edict_t *other) -> void
{
	(void) other;
	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_STAND(cyborg_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &cyborg_move_stand);
}

MONSTERINFO_WALK(cyborg_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &cyborg_move_walk);
}

MONSTERINFO_RUN(cyborg_run) (edict_t *self) -> void
{
	if (self->monsterinfo.active_move == &cyborg_move_pain2)
	{
		M_SetAnimation(self, &cyborg_move_pain2_end);
		return;
	}

	if (self->monsterinfo.active_move == &cyborg_move_pain2_end)
	{
		if (frandom() < 0.1f)
			M_SetAnimation(self, &cyborg_move_move);
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &cyborg_move_stand);
	else
		M_SetAnimation(self, &cyborg_move_run);
}

static void cyborg_fire_right(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[CYBORG_MZ_RIGHT], forward, right);
	vec3_t target = self->enemy->s.origin;
	target[2] += self->enemy->viewheight;

	monster_fire_deatom(self, start, target - start, CYBORG_DEATOM_DAMAGE, CYBORG_DEATOM_SPEED, CYBORG_MZ_RIGHT);
}

static void cyborg_fire_left(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[CYBORG_MZ_LEFT], forward, right);
	vec3_t target = self->enemy->s.origin;
	target[2] += self->enemy->viewheight;

	monster_fire_deatom(self, start, target - start, CYBORG_DEATOM_DAMAGE, CYBORG_DEATOM_SPEED, CYBORG_MZ_LEFT);
}

static void cyborg_fire_both(edict_t *self)
{
	cyborg_fire_left(self);
	cyborg_fire_right(self);
}

static void cyborg_attack_start(edict_t *self)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	self->s.origin[2] += 1.0f;
	self->velocity = forward * -100.0f;
	self->velocity[2] = 250.0f;
	self->groundentity = nullptr;
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	self->touch = cyborg_touch;
	self->monsterinfo.attack_finished = level.time + CYBORG_BACKFLIP_TIMEOUT;
}

static void cyborg_attack_end(edict_t *self)
{
	if (!self->groundentity)
		return;

	gi.sound(self, CHAN_WEAPON, sound_thud, 1.0f, ATTN_NORM, 0.0f);
	self->monsterinfo.attack_finished = 0_ms;
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
}

TOUCH(cyborg_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	(void) tr;
	(void) other_touching_self;

	if (self->health > 0)
	{
		if (other->takedamage && self->velocity.length() > CYBORG_COLLISION_SPEED)
		{
			vec3_t dir = self->velocity;
			dir.normalize();

			vec3_t point = self->s.origin + (dir * self->maxs[0]);
			int damage = irandom(CYBORG_COLLISION_DAMAGE_MIN, CYBORG_COLLISION_DAMAGE_MAX);
			T_Damage(other, self, self, self->velocity, point, dir, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
		}

		if (!M_CheckBottom(self))
		{
			if (!self->groundentity)
				return;

			self->monsterinfo.nextframe = CYBORG_FRAME_ATTACK_BACKFLIP_FIRST + 5;
		}
	}

	self->touch = nullptr;
}

static void cyborg_hit_left(edict_t *self)
{
	if (fire_hit(self, { MELEE_DISTANCE, self->mins[0], 8.0f }, irandom(CYBORG_MELEE_DAMAGE_MIN, CYBORG_MELEE_DAMAGE_MAX), CYBORG_MELEE_KICK))
		gi.sound(self, CHAN_WEAPON, sound_attack2, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1.0f, ATTN_NORM, 0.0f);
}

static void cyborg_hit_right(edict_t *self)
{
	if (fire_hit(self, { MELEE_DISTANCE, self->mins[0], 8.0f }, irandom(CYBORG_MELEE_DAMAGE_MIN, CYBORG_MELEE_DAMAGE_MAX), CYBORG_MELEE_KICK))
		gi.sound(self, CHAN_WEAPON, sound_attack2, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1.0f, ATTN_NORM, 0.0f);
}

static void cyborg_hit_alt(edict_t *self)
{
	if (fire_hit(self, { MELEE_DISTANCE, self->maxs[0], 8.0f }, irandom(CYBORG_MELEE_DAMAGE_MIN, CYBORG_MELEE_DAMAGE_MAX), CYBORG_MELEE_KICK))
		gi.sound(self, CHAN_WEAPON, sound_attack3, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_ATTACK(cyborg_attack) (edict_t *self) -> void
{
	float r = frandom();

	if (r < 0.5f)
		M_SetAnimation(self, &cyborg_move_attack1);
	else if (r < 0.7f)
		M_SetAnimation(self, &cyborg_move_attack3);
	else
		M_SetAnimation(self, &cyborg_move_attack2);
}

MONSTERINFO_MELEE(cyborg_melee) (edict_t *self) -> void
{
	float r = frandom();

	if (r < 0.6f)
		M_SetAnimation(self, &cyborg_move_melee1);
	else if (frandom() < 0.7f)
		M_SetAnimation(self, &cyborg_move_melee2);
	else
		M_SetAnimation(self, &cyborg_move_attack_backflip);
}

static bool cyborg_check_range(edict_t *self)
{
	edict_t *enemy = self->enemy;
	if (!enemy)
		return false;

	if (self->absmin[2] > enemy->absmin[2] + (enemy->size[2] * 0.75f))
		return false;

	if (self->absmax[2] < enemy->absmin[2] + (enemy->size[2] * 0.25f))
		return false;

	vec3_t delta = self->s.origin - enemy->s.origin;
	delta[2] = 0.0f;

	float dist = delta.length();
	if (dist < CYBORG_RANGE_MIN)
		return false;

	if (dist > CYBORG_RANGE_MIN && frandom() < CYBORG_RANGE_SKIP_CHANCE)
		return false;

	return true;
}

MONSTERINFO_CHECKATTACK(cyborg_checkattack) (edict_t *self) -> bool
{
	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
		return false;

	if (range_to(self, self->enemy) <= RANGE_MELEE)
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (cyborg_check_range(self))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return false;
}

PAIN(cyborg_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	(void) other;
	(void) kick;
	(void) damage;
	(void) mod;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + CYBORG_PAIN_COOLDOWN;

	if (skill->integer == 3)
		return;

	if (frandom() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1.0f, ATTN_NORM, 0.0f);
		M_SetAnimation(self, &cyborg_move_pain1);
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1.0f, ATTN_NORM, 0.0f);
		M_SetAnimation(self, &cyborg_move_pain2);
	}
}

DIE(cyborg_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	(void) inflictor;
	(void) attacker;
	(void) point;

	if (mod.id == MOD_DISINTEGRATOR)
	{
		BecomeExplosion1(self);
		return;
	}

	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/head2/tris.md2", damage, GIB_HEAD, self->s.scale);
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	gi.sound(self, CHAN_VOICE, sound_death, 1.0f, ATTN_NORM, 0.0f);
	self->deadflag = true;
	self->takedamage = true;
	self->s.skinnum = 1;

	float r = frandom();
	if (r < 0.33f)
		M_SetAnimation(self, &cyborg_move_death1);
	else if (r < 0.66f)
		M_SetAnimation(self, &cyborg_move_death2);
	else
		M_SetAnimation(self, &cyborg_move_death3);
}

static void cyborg_dead(edict_t *self)
{
	self->mins = { -32.0f, -32.0f, -38.0f };
	self->maxs = { 32.0f, 32.0f, -20.0f };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

/*QUAKED monster_cyborg (1 .5 0) (-16 -16 -38) (16 16 27) Ambush Trigger_Spawn Sight Corpse
 */
void SP_monster_cyborg(edict_t *self)
{
	if (!M_AllowSpawn(self))
	{
		G_FreeEdict(self);
		return;
	}

	sound_attack1.assign("cyborg/mutatck1.wav");
	sound_attack2.assign("cyborg/mutatck2.wav");
	sound_attack3.assign("cyborg/mutatck3.wav");
	sound_death.assign("cyborg/mutdeth1.wav");
	sound_idle.assign("cyborg/mutidle1.wav");
	sound_pain1.assign("cyborg/mutpain1.wav");
	sound_pain2.assign("cyborg/mutpain2.wav");
	sound_sight.assign("cyborg/mutsght1.wav");
	sound_search.assign("cyborg/mutsrch1.wav");
	sound_step1.assign("cyborg/step1.wav");
	sound_step2.assign("cyborg/step2.wav");
	sound_step3.assign("cyborg/step3.wav");
	sound_thud.assign("cyborg/thud1.wav");

	self->s.modelindex = gi.modelindex("models/monsters/cyborg/tris.md2");
	self->mins = { -16.0f, -16.0f, -38.0f };
	self->maxs = { 16.0f, 16.0f, 27.0f };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 300 * st.health_multiplier;
	self->gib_health = -120;
	self->mass = 300;

	self->pain = cyborg_pain;
	self->die = cyborg_die;

	self->monsterinfo.stand = cyborg_stand;
	self->monsterinfo.walk = cyborg_walk;
	self->monsterinfo.run = cyborg_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = cyborg_attack;
	self->monsterinfo.melee = cyborg_melee;
	self->monsterinfo.sight = cyborg_sight;
	self->monsterinfo.search = cyborg_search;
	self->monsterinfo.idle = cyborg_idle;
	self->monsterinfo.checkattack = cyborg_checkattack;
	self->classname = "monster_gladiator";

	gi.linkentity(self);

	M_SetAnimation(self, &cyborg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
