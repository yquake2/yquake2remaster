// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_flash.h"

namespace
{
	enum
	{
		KIGRAX_FRAME_STAND_FIRST = 0,
		KIGRAX_FRAME_STAND_LAST = 27,
		KIGRAX_FRAME_SCAN_FIRST = 28,
		KIGRAX_FRAME_SCAN_LAST = 48,
		KIGRAX_FRAME_WALK1_FIRST = 61,
		KIGRAX_FRAME_WALK1_LAST = 82,
		KIGRAX_FRAME_WALK2_FIRST = 83,
		KIGRAX_FRAME_WALK2_LAST = 104,
		KIGRAX_FRAME_SIGHT_FIRST = 105,
		KIGRAX_FRAME_SIGHT_LAST = 121,
		KIGRAX_FRAME_RUN_FIRST = 122,
		KIGRAX_FRAME_RUN_LAST = 138,
		KIGRAX_FRAME_PAIN_FIRST = 139,
		KIGRAX_FRAME_PAIN_LAST = 149,
		KIGRAX_FRAME_DEATH_FIRST = 150,
		KIGRAX_FRAME_DEATH_LAST = 168,
		KIGRAX_FRAME_MELEE1_FIRST = 169,
		KIGRAX_FRAME_MELEE1_LAST = 183,
		KIGRAX_FRAME_MELEE2_FIRST = 184,
		KIGRAX_FRAME_MELEE2_LAST = 194,
		KIGRAX_FRAME_ATTACK_FIRST = 195,
		KIGRAX_FRAME_ATTACK_LAST = 204
	};

	constexpr float MODEL_SCALE = 1.0f;
	constexpr float KIGRAX_STAND_SELECT_CHANCE = 1.0f / 3.0f;
	constexpr float KIGRAX_SEARCH_SOUND_CHANCE = 0.5f;
	constexpr float KIGRAX_MELEE_SKIP_CHANCE = 0.1f;
	constexpr float KIGRAX_MELEE_PRIMARY_CHANCE = 0.9f;
	constexpr gtime_t KIGRAX_PAIN_COOLDOWN = 3_sec;
	constexpr int KIGRAX_YAW_SPEED = 20;
	constexpr int KIGRAX_MELEE_KICK = 100;
	constexpr int KIGRAX_PLASMA_DAMAGE = 10;
	constexpr int KIGRAX_PLASMA_SPEED = 1000;
	constexpr monster_muzzleflash_id_t KIGRAX_PLASMA_FLASH = MZ2_KIGRAX_PLASMA;

	cached_soundindex sound_pain;
	cached_soundindex sound_death;
	cached_soundindex sound_sight;
	cached_soundindex sound_search1;
	cached_soundindex sound_search2;
	cached_soundindex sound_attack;
	cached_soundindex sound_idle;
}

static void kigrax_dead(edict_t *self);
static void kigrax_explode(edict_t *self);
static void kigrax_strike1(edict_t *self);
static void kigrax_strike2(edict_t *self);
static void kigrax_fire_plasma(edict_t *self);
void kigrax_idle(edict_t *self);
void kigrax_run(edict_t *self);
void kigrax_melee(edict_t *self);

mframe_t kigrax_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(kigrax_move_stand) = { KIGRAX_FRAME_STAND_FIRST, KIGRAX_FRAME_STAND_LAST, kigrax_frames_stand, nullptr };

mframe_t kigrax_frames_scan[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(kigrax_move_scan) = { KIGRAX_FRAME_SCAN_FIRST, KIGRAX_FRAME_SCAN_LAST, kigrax_frames_scan, nullptr };

mframe_t kigrax_frames_walk1[] = {
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 }
};
MMOVE_T(kigrax_move_walk1) = { KIGRAX_FRAME_WALK1_FIRST, KIGRAX_FRAME_WALK1_LAST, kigrax_frames_walk1, nullptr };

mframe_t kigrax_frames_walk2[] = {
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 }
};
MMOVE_T(kigrax_move_walk2) = { KIGRAX_FRAME_WALK2_FIRST, KIGRAX_FRAME_WALK2_LAST, kigrax_frames_walk2, nullptr };

mframe_t kigrax_frames_sight[] = {
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 }
};
MMOVE_T(kigrax_move_sight) = { KIGRAX_FRAME_SIGHT_FIRST, KIGRAX_FRAME_SIGHT_LAST, kigrax_frames_sight, nullptr };

mframe_t kigrax_frames_run[] = {
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 15 }
};
MMOVE_T(kigrax_move_run) = { KIGRAX_FRAME_RUN_FIRST, KIGRAX_FRAME_RUN_LAST, kigrax_frames_run, nullptr };

mframe_t kigrax_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(kigrax_move_pain) = { KIGRAX_FRAME_PAIN_FIRST, KIGRAX_FRAME_PAIN_LAST, kigrax_frames_pain, kigrax_run };

mframe_t kigrax_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, kigrax_dead },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(kigrax_move_death) = { KIGRAX_FRAME_DEATH_FIRST, KIGRAX_FRAME_DEATH_LAST, kigrax_frames_death, kigrax_explode };

mframe_t kigrax_frames_melee1[] = {
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1, kigrax_strike1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1, kigrax_strike1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 }
};
MMOVE_T(kigrax_move_melee1) = { KIGRAX_FRAME_MELEE1_FIRST, KIGRAX_FRAME_MELEE1_LAST, kigrax_frames_melee1, kigrax_melee };

mframe_t kigrax_frames_melee2[] = {
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1, kigrax_strike2 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 }
};
MMOVE_T(kigrax_move_melee2) = { KIGRAX_FRAME_MELEE2_FIRST, KIGRAX_FRAME_MELEE2_LAST, kigrax_frames_melee2, kigrax_melee };

mframe_t kigrax_frames_attack[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, kigrax_fire_plasma },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(kigrax_move_attack) = { KIGRAX_FRAME_ATTACK_FIRST, KIGRAX_FRAME_ATTACK_LAST, kigrax_frames_attack, kigrax_run };

MONSTERINFO_STAND(kigrax_stand) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	M_SetAnimation(self, frandom() <= KIGRAX_STAND_SELECT_CHANCE ? &kigrax_move_scan : &kigrax_move_stand);
}

MONSTERINFO_IDLE(kigrax_idle) (edict_t *self) -> void
{
	kigrax_stand(self);
}

MONSTERINFO_WALK(kigrax_walk) (edict_t *self) -> void
{
	if (g_debug_monster_paths->integer)
		gi.Com_Print("walking...\n");

	M_SetAnimation(self, frandom() < KIGRAX_STAND_SELECT_CHANCE ? &kigrax_move_walk2 : &kigrax_move_walk1);
}

MONSTERINFO_RUN(kigrax_run) (edict_t *self) -> void
{
	if (g_debug_monster_paths->integer)
		gi.Com_Print("running...\n");

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &kigrax_move_stand);
		return;
	}

	M_SetAnimation(self, &kigrax_move_run);
}

MONSTERINFO_SEARCH(kigrax_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, frandom() < KIGRAX_SEARCH_SOUND_CHANCE ? sound_search1 : sound_search2, 1, ATTN_NORM, 0);
}

MONSTERINFO_SIGHT(kigrax_sight) (edict_t *self, edict_t *other) -> void
{
	(void) other;

	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &kigrax_move_sight);
}

MONSTERINFO_ATTACK(kigrax_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &kigrax_move_attack);
}

MONSTERINFO_MELEE(kigrax_melee) (edict_t *self) -> void
{
	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
	{
		M_SetAnimation(self, &kigrax_move_run);
		return;
	}

	if (range_to(self, self->enemy) > RANGE_MELEE || frandom() < KIGRAX_MELEE_SKIP_CHANCE)
	{
		M_SetAnimation(self, &kigrax_move_run);
		return;
	}

	M_SetAnimation(self, frandom() < KIGRAX_MELEE_PRIMARY_CHANCE ? &kigrax_move_melee1 : &kigrax_move_melee2);
}

static void kigrax_strike1(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
	fire_hit(self, { MELEE_DISTANCE, self->mins[0], 10.f }, irandom(10, 15), KIGRAX_MELEE_KICK);
}

static void kigrax_strike2(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
	fire_hit(self, { MELEE_DISTANCE, self->mins[0], 10.f }, irandom(20, 39), KIGRAX_MELEE_KICK);
}

static void kigrax_fire_plasma(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[KIGRAX_PLASMA_FLASH], forward, right);
	vec3_t target = self->enemy->s.origin;
	target[2] += self->enemy->viewheight;

	vec3_t dir = target - start;

	fire_plasma_bolt(self, start, dir, KIGRAX_PLASMA_DAMAGE, KIGRAX_PLASMA_SPEED, 1);
	monster_muzzleflash(self, start, KIGRAX_PLASMA_FLASH);
}

PAIN(kigrax_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	(void) other;
	(void) kick;
	(void) damage;
	(void) mod;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + KIGRAX_PAIN_COOLDOWN;

	if (skill->integer == 3)
		return;

	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &kigrax_move_pain);
}

static void kigrax_dead(edict_t *self)
{
	self->mins = { -16, -16, -16 };
	self->maxs = { 16, 16, 0 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

static void kigrax_explode(edict_t *self)
{
	BecomeExplosion1(self);
}

DIE(kigrax_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
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
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->s.scale);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_HEAD, self->s.scale);
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;
	M_SetAnimation(self, &kigrax_move_death);
}

/*QUAKED monster_kigrax (1 .5 0) (-20 -20 -32) (20 20 12) Ambush Trigger_Spawn Sight Corpse
 */
void SP_monster_kigrax(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain.assign("hover/hovpain1.wav");
	sound_death.assign("hover/hovdeth1.wav");
	sound_sight.assign("hover/hovsght1.wav");
	sound_search1.assign("hover/hovsrch1.wav");
	sound_search2.assign("hover/hovsrch2.wav");
	sound_attack.assign("chick/chkatck3.wav");
	sound_idle.assign("kigrax/hovidle1.wav");
	gi.soundindex("kigrax/hovatck1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/kigrax/tris.md2");
	self->mins = { -20, -20, -32 };
	self->maxs = { 20, 20, 12 };

	self->health = 200 * st.health_multiplier;
	self->gib_health = -100;
	self->mass = 150;
	self->yaw_speed = KIGRAX_YAW_SPEED;
	// Companion visibility/attack traces aim at enemy->origin + viewheight.
	// Keep this within the Kigrax bbox so allies can acquire and shoot it.
	self->viewheight = 12;

	self->pain = kigrax_pain;
	self->die = kigrax_die;

	self->monsterinfo.stand = kigrax_stand;
	self->monsterinfo.idle = kigrax_idle;
	self->monsterinfo.walk = kigrax_walk;
	self->monsterinfo.run = kigrax_run;
	self->monsterinfo.attack = kigrax_attack;
	self->monsterinfo.melee = kigrax_melee;
	self->monsterinfo.sight = kigrax_sight;
	self->monsterinfo.search = kigrax_search;
	self->monsterinfo.scale = MODEL_SCALE;
	self->s.sound = sound_idle;
	self->classname = "monster_hover";

	gi.linkentity(self);

	M_SetAnimation(self, &kigrax_move_stand);
	flymonster_start(self);
}
