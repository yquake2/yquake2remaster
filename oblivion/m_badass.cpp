// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_flash.h"

namespace
{
	enum
	{
		BADASS_FRAME_ACTIVATE_FIRST = 0,
		BADASS_FRAME_ACTIVATE_LAST = 6,
		BADASS_FRAME_DEACTIVATE_FIRST = 7,
		BADASS_FRAME_DEACTIVATE_LAST = 21,
		BADASS_FRAME_ATTACK_FIRST = 22,
		BADASS_FRAME_ATTACK_LAST = 25,
		BADASS_FRAME_PAIN_FIRST = 26,
		BADASS_FRAME_PAIN_LAST = 35,
		BADASS_FRAME_STAND_FIRST = 36,
		BADASS_FRAME_STAND_LAST = 55,
		BADASS_FRAME_WALK_FIRST = 56,
		BADASS_FRAME_WALK_LAST = 69,
		BADASS_FRAME_RUN_FIRST = 70,
		BADASS_FRAME_RUN_LAST = 77,
		BADASS_FRAME_DEATH_FIRST = 78,
		BADASS_FRAME_DEATH_LAST = 97
	};

	constexpr float MODEL_SCALE = 1.0f;
	constexpr float BADASS_ATTACK_RANGE = 200.0f;
	constexpr gtime_t BADASS_PAIN_COOLDOWN = 3_sec;
	constexpr gtime_t BADASS_GIB_DELAY = 500_ms;
	constexpr int BADASS_HEALTH = 1500;
	constexpr int BADASS_GIB_HEALTH = 0;
	constexpr int BADASS_MASS = 1000;
	constexpr int BADASS_YAW_SPEED = 25;
	constexpr int BADASS_ROCKET_DAMAGE = 50;
	constexpr int BADASS_ROCKET_SPEED = 550;
	constexpr int BADASS_GIB_EXPLOSION_DAMAGE = 100;
	constexpr float BADASS_GIB_EXPLOSION_RADIUS = 100.0f;
	constexpr int BADASS_GIB_THINK_FRAMES = 4;
	constexpr int BADASS_GIB_SENTINEL = 0x7a69;
	constexpr spawnflags_t SPAWNFLAG_BADASS_DEAD = SPAWNFLAG_MONSTER_CORPSE;
	constexpr monster_muzzleflash_id_t BADASS_MZ_LEFT = MZ2_BADASS_ROCKET_LEFT;
	constexpr monster_muzzleflash_id_t BADASS_MZ_RIGHT = MZ2_BADASS_ROCKET_RIGHT;

	struct badass_gib_def_t
	{
		const char *model;
		vec3_t mins;
		vec3_t maxs;
	};

	constexpr badass_gib_def_t badass_gib_defs[] = {
		{ "models/monsters/badass/gib_torso.md2", { -17.0f, -15.0f, -50.0f }, { 41.0f, 22.0f, -29.0f } },
		{ "models/monsters/badass/gib_lleg.md2", { -35.0f, 1.0f, -46.0f }, { 47.0f, 53.0f, -22.0f } },
		{ "models/monsters/badass/gib_rleg.md2", { -34.0f, -51.0f, -44.0f }, { 48.0f, -1.0f, -21.0f } },
		{ "models/monsters/badass/gib_larm.md2", { -29.0f, -12.0f, -33.0f }, { 31.0f, 53.0f, -12.0f } },
		{ "models/monsters/badass/gib_rarm.md2", { -34.0f, -51.0f, -42.0f }, { 26.0f, -30.0f, -21.0f } }
	};

	cached_soundindex sound_pain;
	cached_soundindex sound_thud;
	cached_soundindex sound_step;
	cached_soundindex sound_sight;
}

static void badass_step(edict_t *self);
static void badass_thud(edict_t *self);
static void badass_rocket(edict_t *self, const vec3_t &offset);
static void badass_rocket_right(edict_t *self);
static void badass_rocket_left(edict_t *self);
void badass_sight(edict_t *self, edict_t *other);
void badass_stand(edict_t *self);
void badass_idle(edict_t *self);
void badass_walk(edict_t *self);
void badass_run(edict_t *self);
void badass_attack(edict_t *self);
static void badass_attack_loop(edict_t *self);
void badass_gib_think(edict_t *self);
void badass_die_gibs(edict_t *self);
void badass_gib_explosion(edict_t *self);
static void badass_dead(edict_t *self);
void badass_pain(edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod);
void badass_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod);

mframe_t badass_frames_idle_closed[] = {
	{ ai_stand }
};
MMOVE_T(badass_move_idle_closed) = { BADASS_FRAME_ACTIVATE_FIRST, BADASS_FRAME_ACTIVATE_FIRST, badass_frames_idle_closed, nullptr };

// Retail stores this move in an overlapped blob with the mmove header living
// where the seventh frame would land. Keep the intended 0..6 frame band here
// as a normal runtime-safe move.
mframe_t badass_frames_activate[] = {
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(badass_move_activate) = { BADASS_FRAME_ACTIVATE_FIRST, BADASS_FRAME_ACTIVATE_LAST, badass_frames_activate, badass_run };

mframe_t badass_frames_deactivate[] = {
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(badass_move_deactivate) = { BADASS_FRAME_DEACTIVATE_FIRST, BADASS_FRAME_DEACTIVATE_LAST, badass_frames_deactivate, badass_idle };

mframe_t badass_frames_stand[] = {
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(badass_move_stand) = { BADASS_FRAME_STAND_FIRST, BADASS_FRAME_STAND_LAST, badass_frames_stand, nullptr };

mframe_t badass_frames_walk[] = {
	{ ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f },
	{ ai_walk, 7.0f }, { ai_walk, 7.0f, badass_step }, { ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f },
	{ ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f }, { ai_walk, 7.0f, badass_step }
};
MMOVE_T(badass_move_walk) = { BADASS_FRAME_WALK_FIRST, BADASS_FRAME_WALK_LAST, badass_frames_walk, nullptr };

mframe_t badass_frames_run[] = {
	{ ai_run, 14.0f }, { ai_run, 15.0f }, { ai_run, 21.0f }, { ai_run, 24.0f, badass_step },
	{ ai_run, 14.0f }, { ai_run, 15.0f }, { ai_run, 21.0f }, { ai_run, 24.0f, badass_step }
};
MMOVE_T(badass_move_run) = { BADASS_FRAME_RUN_FIRST, BADASS_FRAME_RUN_LAST, badass_frames_run, nullptr };

mframe_t badass_frames_attack[] = {
	{ ai_charge, -5.0f, badass_rocket_right },
	{ ai_charge, 0.0f },
	{ ai_charge, -5.0f, badass_rocket_left },
	{ ai_charge, 0.0f }
};
MMOVE_T(badass_move_attack) = { BADASS_FRAME_ATTACK_FIRST, BADASS_FRAME_ATTACK_LAST, badass_frames_attack, badass_attack_loop };

mframe_t badass_frames_pain[] = {
	{ ai_move, 8.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f },
	{ ai_move, 0.0f }, { ai_move, -16.0f }, { ai_move, -16.0f }, { ai_move, -8.0f }, { ai_move, 0.0f }
};
MMOVE_T(badass_move_pain) = { BADASS_FRAME_PAIN_FIRST, BADASS_FRAME_PAIN_LAST, badass_frames_pain, badass_run };

mframe_t badass_frames_death[] = {
	{ ai_move, -8.0f, badass_gib_explosion },
	{ ai_move, -8.0f },
	{ ai_move, -8.0f },
	{ ai_move, -7.0f },
	{ ai_move, -4.0f, badass_thud },
	{ ai_move, 0.0f },
	{ ai_move, 0.0f },
	{ ai_move, 0.0f },
	{ ai_move, 0.0f, badass_gib_explosion },
	{ ai_move, 4.0f },
	{ ai_move, 2.0f },
	{ ai_move, 2.0f },
	{ ai_move, 2.0f },
	{ ai_move, 2.0f },
	{ ai_move, 2.0f },
	{ ai_move, 2.0f, badass_thud },
	{ ai_move, 0.0f, badass_gib_explosion },
	{ ai_move, 0.0f, badass_thud },
	{ ai_move, 0.0f },
	{ ai_move, 0.0f, badass_thud }
};
MMOVE_T(badass_move_death) = { BADASS_FRAME_DEATH_FIRST, BADASS_FRAME_DEATH_LAST, badass_frames_death, badass_dead };

MONSTERINFO_SIGHT(badass_sight) (edict_t *self, edict_t *other) -> void
{
	(void) other;

	if (self->monsterinfo.active_move == &badass_move_idle_closed)
		M_SetAnimation(self, &badass_move_activate);
	else
		badass_run(self);

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_STAND(badass_stand) (edict_t *self) -> void
{
	if (self->monsterinfo.active_move != &badass_move_idle_closed)
		M_SetAnimation(self, &badass_move_stand);
}

MONSTERINFO_IDLE(badass_idle) (edict_t *self) -> void
{
	if (self->monsterinfo.active_move == &badass_move_stand)
	{
		M_SetAnimation(self, &badass_move_deactivate);
		return;
	}

	M_SetAnimation(self, &badass_move_idle_closed);
}

MONSTERINFO_WALK(badass_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &badass_move_walk);
}

MONSTERINFO_RUN(badass_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &badass_move_run);
}

MONSTERINFO_ATTACK(badass_attack) (edict_t *self) -> void
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	if ((self->s.origin - self->enemy->s.origin).length() > BADASS_ATTACK_RANGE && frandom() < 0.5f)
		return;

	M_SetAnimation(self, &badass_move_attack);
}

MONSTERINFO_MELEE(badass_melee) (edict_t *self) -> void
{
	badass_attack(self);
}

static void badass_attack_loop(edict_t *self)
{
	if (self->enemy && self->enemy->inuse && visible(self, self->enemy) && self->enemy->health > 0)
	{
		if ((self->s.origin - self->enemy->s.origin).length() < BADASS_ATTACK_RANGE)
			return;

		if (frandom() <= 0.45f)
			return;
	}

	M_SetAnimation(self, &badass_move_run);
}

static void badass_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1.0f, ATTN_NORM, 0.0f);
}

static void badass_thud(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_thud, 1.0f, ATTN_NORM, 0.0f);
}

static void badass_rocket(edict_t *self, monster_muzzleflash_id_t flash)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[flash], forward, right);
	vec3_t target = self->enemy->s.origin;
	target[2] += self->enemy->viewheight;

	vec3_t dir = target - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, BADASS_ROCKET_DAMAGE, BADASS_ROCKET_SPEED, flash);
}

static void badass_rocket_right(edict_t *self)
{
	badass_rocket(self, BADASS_MZ_RIGHT);
}

static void badass_rocket_left(edict_t *self)
{
	badass_rocket(self, BADASS_MZ_LEFT);
}

THINK(badass_gib_think) (edict_t *self) -> void
{
	if (self->count == BADASS_GIB_THINK_FRAMES)
	{
		self->think = nullptr;
		self->nextthink = 0_ms;
		return;
	}

	self->count++;
	self->nextthink = level.time + 10_hz;
}

THINK(badass_die_gibs) (edict_t *self) -> void
{
	auto [forward, right, up] = AngleVectors(self->s.angles);

	for (size_t i = 0; i < q_countof(badass_gib_defs); i++)
	{
		const auto &def = badass_gib_defs[i];

		edict_t *gib = G_Spawn();
		gib->s.origin = self->s.origin;
		gib->s.old_origin = self->s.origin;
		gib->s.angles = self->s.angles;
		gib->avelocity = self->s.angles * 200.0f;
		gib->avelocity[ROLL] = 0.0f;
		gib->avelocity[YAW] = crandom() * 200.0f;
		gib->mass = 0;
		gib->owner = self;
		gib->movetype = MOVETYPE_BOUNCE;
		gib->solid = SOLID_BBOX;
		gib->deadflag = true;
		gib->svflags |= SVF_DEADMONSTER;
		gib->classname = "gib";
		gib->think = badass_gib_think;
		gib->nextthink = level.time + 10_hz;
		gib->s.modelindex = gi.modelindex(def.model);
		gib->mins = def.mins;
		gib->maxs = def.maxs;

		switch (i)
		{
		case 0:
			gib->velocity = (forward * (-100.0f * frandom())) + (up * 300.0f);
			break;
		case 1:
			gib->velocity = (up * 200.0f) + (right * -200.0f);
			break;
		case 2:
			gib->velocity = (up * 200.0f) + (right * 200.0f);
			break;
		case 3:
			gib->velocity = (forward * -200.0f) + (up * (50.0f + (275.0f * frandom()))) + (right * -100.0f);
			break;
		case 4:
			gib->velocity = (forward * -200.0f) + (up * (300.0f * frandom())) + (right * 50.0f);
			break;
		}

		gi.linkentity(gib);
	}

	const vec3_t original_origin = self->s.origin;
	self->s.origin = original_origin + right;
	badass_gib_explosion(self);
	self->s.origin = original_origin - right;
	badass_gib_explosion(self);
	G_FreeEdict(self);
}

THINK(badass_gib_explosion) (edict_t *self) -> void
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	T_RadiusDamage(self, self, BADASS_GIB_EXPLOSION_DAMAGE, nullptr, BADASS_GIB_EXPLOSION_RADIUS, DAMAGE_NONE, MOD_EXPLOSIVE);

	if (self->think == badass_gib_explosion)
	{
		self->think = badass_die_gibs;
		self->nextthink = level.time + BADASS_GIB_DELAY;
	}
}

static void badass_dead(edict_t *self)
{
	self->deadflag = true;
	self->mins = { -44.0f, -62.0f, -64.0f };
	self->maxs = { 44.0f, 62.0f, -5.0f };
	self->svflags |= SVF_DEADMONSTER;
	self->think = badass_gib_explosion;
	self->count = BADASS_GIB_SENTINEL;
	self->nextthink = level.time + BADASS_GIB_DELAY;
	gi.linkentity(self);
}

PAIN(badass_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	(void) other;
	(void) kick;
	(void) mod;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (damage <= 20 || level.time < self->pain_debounce_time)
		return;

	if (damage < 51 && frandom() > 0.2f)
		return;

	self->pain_debounce_time = level.time + BADASS_PAIN_COOLDOWN;
	gi.sound(self, CHAN_VOICE, sound_pain, 1.0f, ATTN_NORM, 0.0f);
	M_SetAnimation(self, &badass_move_pain);
}

DIE(badass_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	(void) inflictor;
	(void) attacker;
	(void) damage;
	(void) point;
	(void) mod;

	if (self->deadflag)
		return;

	self->deadflag = true;
	self->takedamage = true;
	M_SetAnimation(self, &badass_move_death);
}

/*QUAKED monster_badass (1 .5 0) (-52 -40 -64) (38 40 32) Ambush Trigger_Spawn Sight Corpse
 */
void SP_monster_badass(edict_t *self)
{
	if (!M_AllowSpawn(self))
	{
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->yaw_speed = BADASS_YAW_SPEED;
	self->s.modelindex = gi.modelindex("models/monsters/badass/tris.md2");
	self->mins = { -52.0f, -40.0f, -64.0f };
	self->maxs = { 38.0f, 40.0f, 32.0f };
	self->classname = "monster_tank";
	self->health = BADASS_HEALTH * st.health_multiplier;
	self->gib_health = BADASS_GIB_HEALTH;
	self->mass = BADASS_MASS;

	gi.modelindex("models/monsters/badass/gib_larm.md2");
	gi.modelindex("models/monsters/badass/gib_rarm.md2");
	gi.modelindex("models/monsters/badass/gib_lleg.md2");
	gi.modelindex("models/monsters/badass/gib_rleg.md2");
	gi.modelindex("models/monsters/badass/gib_torso.md2");

	if (self->spawnflags.has(SPAWNFLAG_BADASS_DEAD))
	{
		self->s.skinnum |= 1;
		self->health = -1;
		self->deadflag = true;
		self->mins = { -31.0f, -88.0f, -64.0f };
		self->maxs = { 38.0f, 21.0f, -13.0f };
		self->svflags |= SVF_DEADMONSTER;
		self->nextthink = 0_ms;
		gi.linkentity(self);
		return;
	}

	sound_pain.assign("tank/tnkpain2.wav");
	sound_thud.assign("tank/tnkdeth2.wav");
	gi.soundindex("tank/tnkidle1.wav");
	gi.soundindex("tank/death.wav");
	sound_step.assign("tank/step.wav");
	gi.soundindex("tank/tnkatck4.wav");
	gi.soundindex("tank/tnkatck5.wav");
	sound_sight.assign("tank/sight1.wav");
	gi.soundindex("tank/tnkatck1.wav");
	gi.soundindex("tank/tnkatk2a.wav");
	gi.soundindex("tank/tnkatk2b.wav");
	gi.soundindex("tank/tnkatk2c.wav");
	gi.soundindex("tank/tnkatk2d.wav");
	gi.soundindex("tank/tnkatk2e.wav");
	gi.soundindex("tank/tnkatck3.wav");

	self->pain = badass_pain;
	self->die = badass_die;

	self->monsterinfo.stand = badass_stand;
	self->monsterinfo.idle = badass_idle;
	self->monsterinfo.walk = badass_walk;
	self->monsterinfo.run = badass_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = badass_attack;
	self->monsterinfo.melee = badass_melee;
	self->monsterinfo.sight = badass_sight;
	self->monsterinfo.scale = MODEL_SCALE;

	gi.linkentity(self);

	M_SetAnimation(self, &badass_move_idle_closed);
	walkmonster_start(self);
}
