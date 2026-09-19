// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_flash.h"

namespace
{
	constexpr float MODEL_SCALE = 1.0f;

	constexpr int SPIDER_FRAME_STAND_START = 0;
	constexpr int SPIDER_FRAME_STAND_END = 54;
	constexpr int SPIDER_FRAME_WALK_START = 55;
	constexpr int SPIDER_FRAME_WALK_END = 64;
	constexpr int SPIDER_FRAME_RUN1_START = 65;
	constexpr int SPIDER_FRAME_RUN1_END = 74;
	constexpr int SPIDER_FRAME_RUN2_START = 75;
	constexpr int SPIDER_FRAME_RUN2_END = 80;
	constexpr int SPIDER_FRAME_ATTACK_LEFT_START = 81;
	constexpr int SPIDER_FRAME_ATTACK_LEFT_END = 85;
	constexpr int SPIDER_FRAME_ATTACK_RIGHT_START = 86;
	constexpr int SPIDER_FRAME_ATTACK_RIGHT_END = 90;
	constexpr int SPIDER_FRAME_ATTACK_DUAL_START = 91;
	constexpr int SPIDER_FRAME_ATTACK_DUAL_END = 98;
	constexpr int SPIDER_FRAME_MELEE_PRIMARY_START = 99;
	constexpr int SPIDER_FRAME_MELEE_PRIMARY_END = 103;
	constexpr int SPIDER_FRAME_MELEE_SECONDARY_START = 104;
	constexpr int SPIDER_FRAME_MELEE_SECONDARY_END = 110;
	constexpr int SPIDER_FRAME_PAIN1_START = 111;
	constexpr int SPIDER_FRAME_PAIN1_END = 116;
	constexpr int SPIDER_FRAME_PAIN2_START = 117;
	constexpr int SPIDER_FRAME_PAIN2_END = 124;
	constexpr int SPIDER_FRAME_DEATH1_START = 125;
	constexpr int SPIDER_FRAME_DEATH1_END = 144;
	constexpr int SPIDER_FRAME_DEATH2_START = 145;
	constexpr int SPIDER_FRAME_DEATH2_END = 164;

	constexpr int SPIDER_ROCKET_DAMAGE = 50;
	constexpr int SPIDER_ROCKET_SPEED = 500;
	constexpr int SPIDER_MELEE_KICK = 300;
	constexpr int SPIDER_CHARGE_DAMAGE_BASE = 40;
	constexpr int SPIDER_CHARGE_DAMAGE_MAX = 49;
	constexpr float SPIDER_CHARGE_IMPACT_SPEED = 400.0f;
	constexpr float SPIDER_CHARGE_SPEED = 500.0f;
	constexpr float SPIDER_CHARGE_Z_SPEED = 250.0f;
	constexpr float SPIDER_ATTACK_MIN_RANGE = 112.0f;
	constexpr spawnflags_t SPAWNFLAG_SPIDER_CORPSE = SPAWNFLAG_MONSTER_CORPSE;

	constexpr monster_muzzleflash_id_t SPIDER_MZ_LEFT = MZ2_SPIDER_ROCKET_LEFT;
	constexpr monster_muzzleflash_id_t SPIDER_MZ_RIGHT = MZ2_SPIDER_ROCKET_RIGHT;

	cached_soundindex sound_step;
	cached_soundindex sound_pain1;
	cached_soundindex sound_pain2;
	cached_soundindex sound_sight;
	cached_soundindex sound_search;
	cached_soundindex sound_idle;
	cached_soundindex sound_melee1;
	cached_soundindex sound_melee2;
	cached_soundindex sound_melee3;
}

void spider_idle(edict_t *self);
void spider_search(edict_t *self);
void spider_sight(edict_t *self, edict_t *other);
void spider_melee_swing(edict_t *self);
void spider_stand(edict_t *self);
void spider_walk(edict_t *self);
void spider_charge_start(edict_t *self);
void spider_charge_end(edict_t *self);
void spider_run(edict_t *self);
void spider_melee_hit(edict_t *self);
void spider_melee(edict_t *self);
void spider_rocket_left(edict_t *self);
void spider_rocket_right(edict_t *self);
void spider_attack(edict_t *self);
void spider_dead(edict_t *self);
bool spider_checkattack(edict_t *self);
void spider_charge_touch(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self);

mframe_t spider_frames_stand[] = {
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(spider_move_stand) = { SPIDER_FRAME_STAND_START, SPIDER_FRAME_STAND_END, spider_frames_stand, nullptr };

mframe_t spider_frames_walk[] = {
	{ ai_walk, 0.0f }, { ai_walk, 4.0f }, { ai_walk, 6.0f }, { ai_walk, 4.0f }, { ai_walk, 2.0f },
	{ ai_walk, 0.0f }, { ai_walk, 4.0f }, { ai_walk, 6.0f }, { ai_walk, 4.0f }, { ai_walk, 2.0f }
};
MMOVE_T(spider_move_walk) = { SPIDER_FRAME_WALK_START, SPIDER_FRAME_WALK_END, spider_frames_walk, nullptr };

mframe_t spider_frames_run1[] = {
	{ ai_run, 0.0f }, { ai_run, 0.0f }, { ai_run, 0.0f, spider_charge_start }, { ai_run, 0.0f }, { ai_run, 0.0f },
	{ ai_run, 0.0f }, { ai_run, 0.0f }, { ai_run, 0.0f, spider_charge_end }, { ai_run, 0.0f }, { ai_run, 0.0f }
};
MMOVE_T(spider_move_run1) = { SPIDER_FRAME_RUN1_START, SPIDER_FRAME_RUN1_END, spider_frames_run1, spider_run };

mframe_t spider_frames_run2[] = {
	{ ai_run, 16.0f }, { ai_run, 16.0f }, { ai_run, 16.0f },
	{ ai_run, 16.0f }, { ai_run, 16.0f }, { ai_run, 16.0f }
};
MMOVE_T(spider_move_run2) = { SPIDER_FRAME_RUN2_START, SPIDER_FRAME_RUN2_END, spider_frames_run2, nullptr };

mframe_t spider_frames_attack_left[] = {
	{ ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f, spider_rocket_left }, { ai_charge, 0.0f }, { ai_charge, 0.0f }
};
MMOVE_T(spider_move_attack_left) = { SPIDER_FRAME_ATTACK_LEFT_START, SPIDER_FRAME_ATTACK_LEFT_END, spider_frames_attack_left, spider_run };

mframe_t spider_frames_attack_right[] = {
	{ ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f, spider_rocket_right }, { ai_charge, 0.0f }, { ai_charge, 0.0f }
};
MMOVE_T(spider_move_attack_right) = { SPIDER_FRAME_ATTACK_RIGHT_START, SPIDER_FRAME_ATTACK_RIGHT_END, spider_frames_attack_right, spider_run };

mframe_t spider_frames_attack_dual[] = {
	{ ai_charge, 0.0f }, { ai_charge, 0.0f, spider_rocket_left }, { ai_charge, 0.0f }, { ai_charge, 0.0f },
	{ ai_charge, 0.0f }, { ai_charge, 0.0f, spider_rocket_right }, { ai_charge, 0.0f }, { ai_charge, 0.0f }
};
MMOVE_T(spider_move_attack_dual) = { SPIDER_FRAME_ATTACK_DUAL_START, SPIDER_FRAME_ATTACK_DUAL_END, spider_frames_attack_dual, spider_run };

mframe_t spider_frames_melee_primary[] = {
	{ ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f, spider_melee_swing }, { ai_charge, 0.0f, spider_melee_hit }
};
MMOVE_T(spider_move_melee_primary) = { SPIDER_FRAME_MELEE_PRIMARY_START, SPIDER_FRAME_MELEE_PRIMARY_END, spider_frames_melee_primary, spider_run };

mframe_t spider_frames_melee_secondary[] = {
	{ ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f }, { ai_charge, 0.0f },
	{ ai_charge, 0.0f, spider_melee_swing }, { ai_charge, 0.0f, spider_melee_hit }, { ai_charge, 0.0f, spider_melee_hit }
};
MMOVE_T(spider_move_melee_secondary) = { SPIDER_FRAME_MELEE_SECONDARY_START, SPIDER_FRAME_MELEE_SECONDARY_END, spider_frames_melee_secondary, spider_run };

mframe_t spider_frames_pain1[] = {
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(spider_move_pain1) = { SPIDER_FRAME_PAIN1_START, SPIDER_FRAME_PAIN1_END, spider_frames_pain1, spider_run };

mframe_t spider_frames_pain2[] = {
	{ ai_move, -16.0f }, { ai_move, -32.0f }, { ai_move, -8.0f }, { ai_move, 0.0f },
	{ ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }
};
MMOVE_T(spider_move_pain2) = { SPIDER_FRAME_PAIN2_START, SPIDER_FRAME_PAIN2_END, spider_frames_pain2, spider_run };

mframe_t spider_frames_death1[] = {
	{ ai_move, -8.0f }, { ai_move, -4.0f }, { ai_move, -2.0f }, { ai_move, 0.0f }, { ai_move, 0.0f },
	{ ai_move, -2.0f }, { ai_move, -6.0f }, { ai_move, -4.0f }, { ai_move, 0.0f }, { ai_move, 4.0f },
	{ ai_move, 6.0f }, { ai_move, 4.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f },
	{ ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }
};
MMOVE_T(spider_move_death1) = { SPIDER_FRAME_DEATH1_START, SPIDER_FRAME_DEATH1_END, spider_frames_death1, spider_dead };

mframe_t spider_frames_death2[] = {
	{ ai_move, -24.0f }, { ai_move, -22.0f }, { ai_move, -20.0f }, { ai_move, -18.0f }, { ai_move, -16.0f },
	{ ai_move, -16.0f }, { ai_move, -16.0f }, { ai_move, -16.0f }, { ai_move, -16.0f }, { ai_move, -4.0f },
	{ ai_move, -12.0f }, { ai_move, -8.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f },
	{ ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }, { ai_move, 0.0f }
};
MMOVE_T(spider_move_death2) = { SPIDER_FRAME_DEATH2_START, SPIDER_FRAME_DEATH2_END, spider_frames_death2, spider_dead };

MONSTERINFO_IDLE(spider_idle) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1.0f, ATTN_IDLE, 0.0f);
}

MONSTERINFO_SEARCH(spider_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_SIGHT(spider_sight) (edict_t *self, edict_t *other) -> void
{
	(void) other;

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);
}

void spider_melee_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_STAND(spider_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &spider_move_stand);
}

MONSTERINFO_WALK(spider_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &spider_move_walk);
}

TOUCH(spider_charge_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	(void) tr;
	(void) other_touching_self;

	if (self->health > 0)
	{
		if (other->takedamage && self->velocity.length() > SPIDER_CHARGE_IMPACT_SPEED)
		{
			vec3_t dir = self->velocity;
			dir.normalize();

			vec3_t point = self->s.origin + (dir * self->maxs[0]);
			int damage = irandom(SPIDER_CHARGE_DAMAGE_BASE, SPIDER_CHARGE_DAMAGE_MAX);
			T_Damage(other, self, self, self->velocity, point, dir, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
		}

		if (!M_CheckBottom(self))
		{
			if (!self->groundentity)
				return;

			self->monsterinfo.nextframe = SPIDER_FRAME_RUN1_START + 3;
		}
	}

	self->touch = nullptr;
}

void spider_charge_start(edict_t *self)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight, 1.0f, ATTN_NORM, 0.0f);

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	self->s.origin[2] += 1.0f;
	self->velocity = forward * SPIDER_CHARGE_SPEED;
	self->velocity[2] = SPIDER_CHARGE_Z_SPEED;
	self->groundentity = nullptr;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->touch = spider_charge_touch;
	self->monsterinfo.attack_finished = level.time + 3_sec;
}

void spider_charge_end(edict_t *self)
{
	if (self->groundentity)
	{
		gi.sound(self, CHAN_WEAPON, sound_step, 1.0f, ATTN_NORM, 0.0f);
		self->monsterinfo.attack_finished = 0_ms;
		self->monsterinfo.aiflags &= ~AI_DUCKED;
	}
}

MONSTERINFO_RUN(spider_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &spider_move_stand);
		return;
	}

	if (frandom() < 0.2f)
		M_SetAnimation(self, &spider_move_run1);
	else
		M_SetAnimation(self, &spider_move_run2);
}

void spider_melee_hit(edict_t *self)
{
	int damage = irandom(20, 24);

	if (fire_hit(self, { MELEE_DISTANCE, self->mins[0], -4.0f }, damage, SPIDER_MELEE_KICK))
	{
		gi.sound(self, CHAN_WEAPON, sound_melee2, 1.0f, ATTN_NORM, 0.0f);
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_melee3, 1.0f, ATTN_NORM, 0.0f);
}

MONSTERINFO_MELEE(spider_melee) (edict_t *self) -> void
{
	if (g_debug_monster_paths->integer)
		gi.Com_Print("spider_melee\n");

	if (frandom() < 0.5f)
		M_SetAnimation(self, &spider_move_melee_primary);
	else
		M_SetAnimation(self, &spider_move_melee_secondary);
}

void spider_rocket_left(edict_t *self)
{
	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[SPIDER_MZ_LEFT], forward, right);
	vec3_t dir = self->pos1 - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, SPIDER_ROCKET_DAMAGE, SPIDER_ROCKET_SPEED, SPIDER_MZ_LEFT);
}

void spider_rocket_right(edict_t *self)
{
	vec3_t forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	vec3_t start = M_ProjectFlashSource(self, monster_flash_offset[SPIDER_MZ_RIGHT], forward, right);
	vec3_t dir = self->pos1 - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, SPIDER_ROCKET_DAMAGE, SPIDER_ROCKET_SPEED, SPIDER_MZ_RIGHT);
}

MONSTERINFO_ATTACK(spider_attack) (edict_t *self) -> void
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	if ((self->s.origin - self->enemy->s.origin).length() <= SPIDER_ATTACK_MIN_RANGE)
		return;

	self->pos1 = self->enemy->s.origin;
	self->pos1[2] += self->enemy->viewheight;

	float r = frandom();

	if (r < 0.33f)
		M_SetAnimation(self, &spider_move_attack_left);
	else if (r < 0.66f)
		M_SetAnimation(self, &spider_move_attack_right);
	else
		M_SetAnimation(self, &spider_move_attack_dual);
}

PAIN(spider_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	(void) other;
	(void) kick;
	(void) mod;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	self->pain_debounce_time = level.time + 3_sec;

	int32_t sound_id = (frandom() >= 0.5f) ? (int32_t) sound_pain2 : (int32_t) sound_pain1;

	if (skill->value == 3 && frandom() < 0.1f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		M_SetAnimation(self, &spider_move_pain1);
		return;
	}

	if (damage < 10 && frandom() < 0.2f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		M_SetAnimation(self, &spider_move_pain1);
		return;
	}

	if (damage < 50 && frandom() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
		if (frandom() < 0.5f)
		{
			M_SetAnimation(self, &spider_move_pain1);
			return;
		}
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_id, 1.0f, ATTN_NORM, 0.0f);
	}

	M_SetAnimation(self, &spider_move_pain2);
}

void spider_dead(edict_t *self)
{
	self->mins = { -32.0f, -32.0f, -30.0f };
	self->maxs = { 32.0f, 32.0f, 0.0f };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

DIE(spider_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
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
		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 4, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
			{ "models/objects/gibs/chest/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	self->deadflag = true;
	self->takedamage = true;

	if (frandom() < 0.5f)
		M_SetAnimation(self, &spider_move_death1);
	else
		M_SetAnimation(self, &spider_move_death2);
}

MONSTERINFO_CHECKATTACK(spider_checkattack) (edict_t *self) -> bool
{
	if (!self->enemy || !self->enemy->inuse)
		return false;

	if (self->enemy->health > 0)
	{
		vec3_t spot1 = self->s.origin;
		spot1[2] += self->viewheight;

		vec3_t spot2 = self->enemy->s.origin;
		spot2[2] += self->enemy->viewheight;

		trace_t tr = gi.traceline(spot1, spot2, self, MASK_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW);
		if (tr.ent != self->enemy)
			return false;
	}

	(void) infront(self, self->enemy);

	float enemy_range = range_to(self, self->enemy);
	vec3_t temp = self->enemy->s.origin - self->s.origin;
	self->ideal_yaw = vectoyaw(temp);

	if (enemy_range <= RANGE_MELEE)
	{
		self->monsterinfo.attack_state = self->monsterinfo.melee ? AS_MELEE : AS_MISSILE;
		return true;
	}

	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (enemy_range > RANGE_MID)
		return false;

	float chance = (self->monsterinfo.aiflags & AI_STAND_GROUND) ? 0.4f : 0.8f;
	if (frandom() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + random_time(2_sec);
		return true;
	}

	if (self->flags & FL_FLY)
		self->monsterinfo.attack_state = (frandom() < 0.3f) ? AS_SLIDING : AS_STRAIGHT;

	return false;
}

/*QUAKED monster_spider (1 .5 0) (-32 -32 -35) (32 32 32) Ambush Trigger_Spawn Sight Corpse
 */
void SP_monster_spider(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_melee1.assign("gladiator/melee1.wav");
	sound_melee2.assign("gladiator/melee2.wav");
	sound_melee3.assign("gladiator/melee3.wav");
	sound_step.assign("mutant/thud1.wav");
	gi.soundindex("mutant/mutsght1.wav");
	sound_pain1.assign("gladiator/pain.wav");
	sound_pain2.assign("gladiator/gldpain2.wav");
	sound_idle.assign("gladiator/gldidle1.wav");
	sound_search.assign("gladiator/gldsrch1.wav");
	sound_sight.assign("spider/sight.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/spider/tris.md2");
	self->mins = { -32.0f, -32.0f, -35.0f };
	self->maxs = { 32.0f, 32.0f, 32.0f };
	self->classname = "monster_chick";

	self->health = 400 * st.health_multiplier;
	self->gib_health = -175;
	self->mass = 300;

	if (self->spawnflags.has(SPAWNFLAG_SPIDER_CORPSE))
	{
		self->s.skinnum |= 1;
		self->health = -1;
		self->deadflag = true;
		self->takedamage = true;
		self->svflags |= SVF_DEADMONSTER;
		self->movetype = MOVETYPE_TOSS;
		self->mins = { -32.0f, -32.0f, -30.0f };
		self->maxs = { 32.0f, 32.0f, 0.0f };
		self->nextthink = 0_ms;
		gi.linkentity(self);
		return;
	}

	self->pain = spider_pain;
	self->die = spider_die;

	self->monsterinfo.stand = spider_stand;
	self->monsterinfo.idle = spider_idle;
	self->monsterinfo.search = spider_search;
	self->monsterinfo.walk = spider_walk;
	self->monsterinfo.run = spider_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = spider_attack;
	self->monsterinfo.melee = spider_melee;
	self->monsterinfo.sight = spider_sight;
	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.checkattack = spider_checkattack;

	gi.linkentity(self);

	M_SetAnimation(self, &spider_move_stand);
	walkmonster_start(self);
}
