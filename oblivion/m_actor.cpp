// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_actor.c

#include "g_local.h"
#include "m_flash.h"

constexpr const char *actor_names[] = {
	"Hellrot",
	"Tokay",
	"Killme",
	"Disruptor",
	"Adrianator",
	"Rambear",
	"Titus",
	"Bitterman"
};

static const char *Actor_FallbackName(edict_t *self)
{
	if (!self || self < g_edicts)
		return actor_names[0];

	return actor_names[(self - g_edicts) % q_countof(actor_names)];
}

constexpr gtime_t ACTOR_IDLE_PAUSE = gtime_t::from_sec(100000000.0);
constexpr float MODEL_SCALE = 1.0f;
constexpr int32_t ACTOR_PLAYER_SKIN_SLOT = MAX_CLIENTS - 4;
constexpr const char *ACTOR_PLAYER_SKIN = "actor\\male/grunt";
constexpr const char *ACTOR_PLAYER_WEAPON_MODEL = "players/male/w_machinegun.md2";

enum
{
	ACTOR_FRAME_STAND_FIRST = 0,
	ACTOR_FRAME_STAND_LAST = 39,
	ACTOR_FRAME_WALK_FIRST = 40,
	ACTOR_FRAME_WALK_LAST = 45,
	ACTOR_FRAME_RUN_FIRST = 40,
	ACTOR_FRAME_RUN_LAST = 45,
	ACTOR_FRAME_ATTACK_FIRST = 46,
	ACTOR_FRAME_ATTACK_LAST = 53,
	ACTOR_FRAME_PAIN1_FIRST = 54,
	ACTOR_FRAME_PAIN1_LAST = 57,
	ACTOR_FRAME_PAIN2_FIRST = 58,
	ACTOR_FRAME_PAIN2_LAST = 61,
	ACTOR_FRAME_PAIN3_FIRST = 62,
	ACTOR_FRAME_PAIN3_LAST = 65,
	ACTOR_FRAME_FLIPOFF_FIRST = 72,
	ACTOR_FRAME_FLIPOFF_LAST = 83,
	ACTOR_FRAME_TAUNT_FIRST = 95,
	ACTOR_FRAME_TAUNT_LAST = 95,
	ACTOR_FRAME_DEATH1_FIRST = 178,
	ACTOR_FRAME_DEATH1_LAST = 183,
	ACTOR_FRAME_DEATH2_FIRST = 184,
	ACTOR_FRAME_DEATH2_LAST = 189,
	ACTOR_FRAME_DEATH3_FIRST = 190,
	ACTOR_FRAME_DEATH3_LAST = 197
};

mframe_t actor_frames_stand[] = {
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
MMOVE_T(actor_move_stand) = { ACTOR_FRAME_STAND_FIRST, ACTOR_FRAME_STAND_LAST, actor_frames_stand, nullptr };

MONSTERINFO_STAND(actor_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_stand);

	// randomize on startup
	if (level.time < 1_sec)
		self->s.frame = irandom(self->monsterinfo.active_move->firstframe, self->monsterinfo.active_move->lastframe + 1);
}

mframe_t actor_frames_walk[] = {
	{ ai_walk, 8 },
	{ ai_walk, 30 },
	{ ai_walk, 30 },
	{ ai_walk, 16 },
	{ ai_walk, 40 },
	{ ai_walk, 30 }
};
MMOVE_T(actor_move_walk) = { ACTOR_FRAME_WALK_FIRST, ACTOR_FRAME_WALK_LAST, actor_frames_walk, nullptr };

MONSTERINFO_WALK(actor_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_walk);
}

mframe_t actor_frames_run[] = {
	{ ai_run, 8 },
	{ ai_run, 30 },
	{ ai_run, 30 },
	{ ai_run, 16 },
	{ ai_run, 40 },
	{ ai_run, 30 }
};
MMOVE_T(actor_move_run) = { ACTOR_FRAME_RUN_FIRST, ACTOR_FRAME_RUN_LAST, actor_frames_run, nullptr };

MONSTERINFO_RUN(actor_run) (edict_t *self) -> void
{
	if ((level.time < self->pain_debounce_time) || self->enemy)
	{
		if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		{
			M_SetAnimation(self, &actor_move_run);
			return;
		}
	}
	else if (self->movetarget)
	{
		actor_walk(self);
		return;
	}

	actor_stand(self);
}

mframe_t actor_frames_pain1[] = {
	{ ai_move, -5 },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move, 1 }
};
MMOVE_T(actor_move_pain1) = { ACTOR_FRAME_PAIN1_FIRST, ACTOR_FRAME_PAIN1_LAST, actor_frames_pain1, actor_run };

mframe_t actor_frames_pain2[] = {
	{ ai_move, -4 },
	{ ai_move },
	{ ai_move, 4 },
	{ ai_move }
};
MMOVE_T(actor_move_pain2) = { ACTOR_FRAME_PAIN2_FIRST, ACTOR_FRAME_PAIN2_LAST, actor_frames_pain2, actor_run };

mframe_t actor_frames_pain3[] = {
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, 1 },
	{ ai_move, 0 }
};
MMOVE_T(actor_move_pain3) = { ACTOR_FRAME_PAIN3_FIRST, ACTOR_FRAME_PAIN3_LAST, actor_frames_pain3, actor_run };

mframe_t actor_frames_flipoff[] = {
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn }
};
MMOVE_T(actor_move_flipoff) = { ACTOR_FRAME_FLIPOFF_FIRST, ACTOR_FRAME_FLIPOFF_LAST, actor_frames_flipoff, actor_run };

mframe_t actor_frames_taunt[] = {
	{ ai_turn }
};
MMOVE_T(actor_move_taunt) = { ACTOR_FRAME_TAUNT_FIRST, ACTOR_FRAME_TAUNT_LAST, actor_frames_taunt, actor_run };

const char *messages[] = {
	"Watch it",
	"#$@*&",
	"Idiot"
};

PAIN(actor_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	int n;

	Actor_SanitizeEnemy(self);

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;
	//	gi.sound (self, CHAN_VOICE, actor.sound_pain, 1, ATTN_NORM, 0);

	if ((other->client) && !(self->monsterinfo.aiflags & AI_ACTOR_FRIENDLY) && (frandom() < 0.4f))
	{
		vec3_t		v;
		const char *name;

		v = other->s.origin - self->s.origin;
		self->ideal_yaw = vectoyaw(v);
		if (frandom() < 0.5f)
			M_SetAnimation(self, &actor_move_flipoff);
		else
			M_SetAnimation(self, &actor_move_taunt);
		name = Actor_FallbackName(self);
		gi.Client_Print(other, PRINT_CHAT, G_Fmt("{}: {}!\n", name, random_element(messages)).data());
		return;
	}

	n = irandom(3);
	if (n == 0)
		M_SetAnimation(self, &actor_move_pain1);
	else if (n == 1)
		M_SetAnimation(self, &actor_move_pain2);
	else
		M_SetAnimation(self, &actor_move_pain3);
}

void actorMachineGun(edict_t *self)
{
	vec3_t start, target;
	vec3_t forward, right;

	if (Actor_SanitizeEnemy(self) && !self->enemy)
		return;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1], forward, right);
	if (self->enemy)
	{
		if (self->enemy->health > 0)
		{
			target = self->enemy->s.origin + (self->enemy->velocity * -0.2f);
			target[2] += self->enemy->viewheight;
		}
		else
		{
			target = self->enemy->absmin;
			target[2] += (self->enemy->size[2] / 2) + 1;
		}
		forward = target - start;
		forward.normalize();
	}
	else
	{
		AngleVectors(self->s.angles, forward, nullptr, nullptr);
	}
	monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_ACTOR_MACHINEGUN_1);
}

static void Actor_SetupRenderModel(edict_t *self)
{
	// The retail 0xff player-model path assumes vanilla skinnum semantics. In rerelease,
	// route actors through a dedicated fake player-skin configstring instead.
	self->s.modelindex = MODELINDEX_PLAYER;
	self->s.modelindex2 = gi.modelindex(ACTOR_PLAYER_WEAPON_MODEL);
	self->s.skinnum = ACTOR_PLAYER_SKIN_SLOT;
	gi.configstring(CS_PLAYERSKINS + ACTOR_PLAYER_SKIN_SLOT, ACTOR_PLAYER_SKIN);
}

static void Actor_EnterIdlePath(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_ACTOR_PATH_IDLE;
	self->monsterinfo.aiflags &= ~(AI_ACTOR_FOLLOW | AI_ACTOR_DEFENSIVE_FIRE);
}

static void Actor_LeaveIdlePath(edict_t *self)
{
	self->monsterinfo.aiflags &= ~(AI_ACTOR_PATH_IDLE | AI_ACTOR_FOLLOW | AI_ACTOR_DEFENSIVE_FIRE);
	self->target_ent = nullptr;
}

static void Actor_QueueNodePause(edict_t *self, float wait)
{
	self->timestamp = wait > 0.0f ? level.time + gtime_t::from_sec(wait) : 0_ms;
}

bool Actor_ResumeScriptedPath(edict_t *self)
{
	if (!self || !self->inuse || !self->classname || strcmp(self->classname, "misc_actor") != 0)
		return false;

	self->monsterinfo.aiflags &= ~(AI_ACTOR_SHOOT_ONCE | AI_ACTOR_TEMP_HOLD | AI_BRUTAL | AI_ACTOR_DEFENSIVE_FIRE);
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;

	if (!self->movetarget || !self->movetarget->inuse)
	{
		self->timestamp = 0_ms;
		return false;
	}

	Actor_LeaveIdlePath(self);
	self->goalentity = self->movetarget;

	if (self->timestamp > level.time)
	{
		self->monsterinfo.pausetime = self->timestamp;
		self->monsterinfo.stand(self);
	}
	else
	{
		self->timestamp = 0_ms;
		self->monsterinfo.pausetime = 0_ms;
		self->monsterinfo.walk(self);
	}

	return true;
}

void actor_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

mframe_t actor_frames_death1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, -13 },
	{ ai_move, 1 },
	{ ai_move, 14 },
	{ ai_move, 1 }
};
MMOVE_T(actor_move_death1) = { ACTOR_FRAME_DEATH1_FIRST, ACTOR_FRAME_DEATH1_LAST, actor_frames_death1, actor_dead };

mframe_t actor_frames_death2[] = {
	{ ai_move },
	{ ai_move, -6 },
	{ ai_move, -5 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move }
};
MMOVE_T(actor_move_death2) = { ACTOR_FRAME_DEATH2_FIRST, ACTOR_FRAME_DEATH2_LAST, actor_frames_death2, actor_dead };

mframe_t actor_frames_death3[] = {
	{ ai_move },
	{ ai_move, -6 },
	{ ai_move, -5 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move }
};
MMOVE_T(actor_move_death3) = { ACTOR_FRAME_DEATH3_FIRST, ACTOR_FRAME_DEATH3_LAST, actor_frames_death3, actor_dead };

DIE(actor_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (mod.id == MOD_DISINTEGRATOR)
	{
		BecomeExplosion1(self);
		return;
	}

	// check for gib
	if (self->health <= -80)
	{
		//		gi.sound (self, CHAN_VOICE, actor.sound_gib, 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 4, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	//	gi.sound (self, CHAN_VOICE, actor.sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;
	self->s.modelindex2 = 0;

	int n = irandom(3);
	if (n == 0)
		M_SetAnimation(self, &actor_move_death1);
	else if (n == 1)
		M_SetAnimation(self, &actor_move_death2);
	else
		M_SetAnimation(self, &actor_move_death3);
}

void actor_fire(edict_t *self)
{
	actorMachineGun(self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void actor_attack_finished(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_ACTOR_SHOOT_ONCE)
	{
		self->enemy = nullptr;
		self->oldenemy = nullptr;
		self->monsterinfo.last_player_enemy = nullptr;

		if (Actor_ResumeScriptedPath(self))
			return;
	}

	if ((self->monsterinfo.aiflags & AI_ACTOR_DEFENSIVE_FIRE) && self->enemy && self->enemy->inuse)
	{
		actor_run(self);
		return;
	}

	actor_stand(self);
}

mframe_t actor_frames_attack[] = {
	{ ai_charge, -2, actor_fire },
	{ ai_charge, -2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 3 },
	{ ai_charge, 2 }
};
MMOVE_T(actor_move_attack) = { ACTOR_FRAME_ATTACK_FIRST, ACTOR_FRAME_ATTACK_LAST, actor_frames_attack, actor_attack_finished };

MONSTERINFO_ATTACK(actor_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_attack);
	self->monsterinfo.pausetime = level.time + gtime_t::from_ms(100 * (10 + irandom(16)));
}

MONSTERINFO_CHECKATTACK(actor_checkattack) (edict_t *self) -> bool
{
	if (Actor_SanitizeEnemy(self) && !self->enemy)
		return false;

	if (!self->enemy)
		return false;

	if ((self->monsterinfo.aiflags & AI_STAND_GROUND) &&
		!M_CheckClearShot(self, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1]))
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		return false;
	}

	if (self->monsterinfo.aiflags & AI_ACTOR_SHOOT_ONCE)
	{
		if (level.time < self->monsterinfo.attack_finished)
			return false;

		if (!M_CheckClearShot(self, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1]))
			return false;

		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time;
		return true;
	}

	return M_CheckAttack_Base(self, 0.65f, 0.35f, 0.2f, 0.04f, 0.0f, 1.0f);
}

USE(actor_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	vec3_t v;

	self->timestamp = 0_ms;
	self->monsterinfo.aiflags &= ~(AI_ACTOR_SHOOT_ONCE | AI_ACTOR_TEMP_HOLD | AI_BRUTAL | AI_ACTOR_DEFENSIVE_FIRE);
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	self->target_ent = nullptr;

	if (!self->target || !*self->target)
	{
		self->target = nullptr;
		Actor_EnterIdlePath(self);
		self->monsterinfo.pausetime = ACTOR_IDLE_PAUSE;
		self->monsterinfo.stand(self);
		return;
	}

	self->goalentity = self->movetarget = G_PickTarget(self->target);
	if ((!self->movetarget) || (strcmp(self->movetarget->classname, "target_actor") != 0))
	{
		self->target = nullptr;
		Actor_EnterIdlePath(self);
		self->monsterinfo.pausetime = ACTOR_IDLE_PAUSE;
		self->monsterinfo.stand(self);
		return;
	}

	v = self->goalentity->s.origin - self->s.origin;
	self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
	Actor_LeaveIdlePath(self);
	self->monsterinfo.walk(self);

	// START_ON actors are pre-used before monster_start_go() runs; keep the route target
	// long enough for the startup think to seed the initial path cleanly.
	if (self->monsterinfo.aiflags & AI_SPAWNED_ALIVE)
		self->target = nullptr;
}

constexpr spawnflags_t SPAWNFLAG_ACTOR_CORPSE = SPAWNFLAG_MONSTER_CORPSE;
constexpr spawnflags_t SPAWNFLAG_ACTOR_START_ON = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ACTOR_WIMPY = 64_spawnflag;

/*QUAKED misc_actor (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Corpse x START_ON WIMPY
START_ON		actor immediately begins its scripted path
WIMPY		clear the good-guy pathing behavior used by retail Oblivion actors
 */

void SP_misc_actor(edict_t *self)
{
	static const int corpse_frames[] = {
		ACTOR_FRAME_DEATH1_LAST,
		ACTOR_FRAME_DEATH2_LAST,
		ACTOR_FRAME_DEATH3_LAST
	};

	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict(self);
		return;
	}

	if (!self->targetname)
	{
		self->targetname = "Yo Mama";
		self->spawnflags |= SPAWNFLAG_ACTOR_START_ON;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->model = "players/male/tris.md2";
	Actor_SetupRenderModel(self);
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };

	if (!self->health)
		self->health = 100;
	self->mass = 200;

	if (self->spawnflags.has(SPAWNFLAG_ACTOR_CORPSE))
	{
		self->s.frame = corpse_frames[irandom(q_countof(corpse_frames))];
		self->health = -1;
		self->deadflag = true;
		self->takedamage = true;
		self->mins = { -16, -16, -24 };
		self->maxs = { 16, 16, -8 };
		self->svflags |= SVF_DEADMONSTER;
		self->nextthink = 0_ms;
		gi.linkentity(self);
		return;
	}

	self->pain = actor_pain;
	self->die = actor_die;
	self->use = actor_use;

	self->monsterinfo.stand = actor_stand;
	self->monsterinfo.walk = actor_walk;
	self->monsterinfo.run = actor_run;
	self->monsterinfo.attack = actor_attack;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = nullptr;
	self->monsterinfo.checkattack = actor_checkattack;
	self->monsterinfo.combat_style = COMBAT_RANGED;
	self->monsterinfo.setskin = nullptr;

	if (!self->target)
		Actor_EnterIdlePath(self);

	if (!self->spawnflags.has(SPAWNFLAG_ACTOR_WIMPY))
		self->monsterinfo.aiflags |= AI_ACTOR_FRIENDLY;

	self->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(self);

	M_SetAnimation(self, &actor_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);

	// walkmonster_start rewires use to monster_use; scripted actors keep actor_use.
	self->use = actor_use;

	if (self->spawnflags.has(SPAWNFLAG_ACTOR_START_ON) && !self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		self->use(self, world, world);
}

/*QUAKED target_actor (.5 .3 0) (-8 -8 -8) (8 8 8) JUMP SHOOT ATTACK x HOLD BRUTAL
JUMP			jump in set direction upon reaching this target
SHOOT			take a single shot at the pathtarget
ATTACK			attack pathtarget until it or actor is dead

"target"		next target_actor
"pathtarget"	target of any action to be taken at this point
"wait"			amount of time actor should pause at this point
"message"		actor will "say" this to the player

for JUMP only:
"speed"			speed thrown forward (default 200)
"height"		speed thrown upwards (default 200)
*/

constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_JUMP = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_SHOOT = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_ATTACK = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_HOLD = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_BRUTAL = 32_spawnflag;

TOUCH(target_actor_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t v;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	other->goalentity = other->movetarget = nullptr;
	Actor_QueueNodePause(other, self->wait);

	if (self->message)
	{
		edict_t *ent;

		for (uint32_t n = 1; n <= game.maxclients; n++)
		{
			ent = &g_edicts[n];
			if (!ent->inuse || !ent->client)
				continue;
			gi.Client_Print(ent, PRINT_CHAT, G_Fmt("{}: {}\n", Actor_FallbackName(other), self->message).data());
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_JUMP)) // jump
	{
		other->velocity[0] = self->movedir[0] * self->speed;
		other->velocity[1] = self->movedir[1] * self->speed;

		if (other->groundentity)
		{
			other->groundentity = nullptr;
			other->velocity[2] = self->movedir[2];
			gi.sound(other, CHAN_VOICE, gi.soundindex("player/male/jump1.wav"), 1, ATTN_NORM, 0);
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_SHOOT) || self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_ATTACK))
	{
		other->enemy = G_PickTarget(self->pathtarget);
		if (other->enemy)
		{
			other->goalentity = other->enemy;
			other->monsterinfo.aiflags &= ~(AI_ACTOR_SHOOT_ONCE | AI_ACTOR_TEMP_HOLD | AI_BRUTAL | AI_ACTOR_DEFENSIVE_FIRE);
			if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_SHOOT))
				other->monsterinfo.aiflags |= AI_ACTOR_SHOOT_ONCE;
			if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_BRUTAL))
				other->monsterinfo.aiflags |= AI_BRUTAL;

			if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_HOLD) || self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_SHOOT))
			{
				other->monsterinfo.aiflags |= AI_ACTOR_TEMP_HOLD;
				other->monsterinfo.aiflags |= AI_STAND_GROUND;
				actor_stand(other);
			}
			else
			{
				actor_run(other);
			}
		}
	}

	if (self->pathtarget)
	{
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	other->movetarget = G_PickTarget(self->target);

	if (!other->goalentity)
		other->goalentity = other->movetarget;

	if (!other->movetarget && !other->enemy)
	{
		other->timestamp = 0_ms;
		other->monsterinfo.pausetime = level.time + ACTOR_IDLE_PAUSE;
		other->monsterinfo.stand(other);
		Actor_EnterIdlePath(other);
		return;
	}

	if (other->movetarget)
	{
		Actor_LeaveIdlePath(other);

		if (other->movetarget == other->goalentity)
		{
			v = other->movetarget->s.origin - other->s.origin;
			other->ideal_yaw = vectoyaw(v);
		}
	}

	if (!other->enemy && other->timestamp > level.time)
	{
		other->monsterinfo.pausetime = other->timestamp;
		other->monsterinfo.stand(other);
	}
}

void SP_target_actor(edict_t *self)
{
	if (!self->targetname)
		gi.Com_PrintFmt("{}: no targetname\n", *self);

	self->solid = SOLID_TRIGGER;
	self->touch = target_actor_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svflags = SVF_NOCLIENT;

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_JUMP))
	{
		if (!self->speed)
			self->speed = 200;
		if (!st.height)
			st.height = 200;
		if (self->s.angles[YAW] == 0)
			self->s.angles[YAW] = 360;
		G_SetMovedir(self->s.angles, self->movedir);
		self->movedir[2] = (float) st.height;
	}

	gi.linkentity(self);
}
