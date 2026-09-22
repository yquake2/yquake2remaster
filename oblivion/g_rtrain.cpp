// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

void train_blocked(edict_t *self, edict_t *other);

constexpr spawnflags_t SPAWNFLAG_ROTATE_TRAIN_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATE_TRAIN_BLOCK_STOPS = 4_spawnflag;

void rotate_train_next(edict_t *self);
static void rotate_train_resume(edict_t *self);

static float RotateTrain_WrapAngle(float angle)
{
	return static_cast<float>(static_cast<int>(angle) % 360);
}

THINK(RotateTrain_MoveDone) (edict_t *self) -> void
{
	self->velocity = {};
	self->avelocity = {};

	self->s.angles[0] = RotateTrain_WrapAngle(self->s.angles[0]);
	self->s.angles[1] = RotateTrain_WrapAngle(self->s.angles[1]);
	self->s.angles[2] = RotateTrain_WrapAngle(self->s.angles[2]);

	self->moveinfo.endfunc(self);
}

THINK(RotateTrain_MoveFinal) (edict_t *self) -> void
{
	if (self->moveinfo.remaining_distance == 0)
	{
		RotateTrain_MoveDone(self);
		return;
	}

	self->velocity = self->moveinfo.dir * (self->moveinfo.remaining_distance * (1.f / gi.frame_time_s));

	if (self->rotate)
	{
		vec3_t delta = self->moveinfo.end_angles - self->s.angles;

		if (!delta)
			self->avelocity = {};
		else
			self->avelocity = delta * (1.f / gi.frame_time_s);
	}

	self->think = RotateTrain_MoveDone;
	self->nextthink = level.time + FRAME_TIME_S;
}

THINK(RotateTrain_MoveBegin) (edict_t *self) -> void
{
	float travel_time;
	float frames;

	if ((self->moveinfo.speed * gi.frame_time_s) >= self->moveinfo.remaining_distance)
	{
		RotateTrain_MoveFinal(self);
		return;
	}

	self->velocity = self->moveinfo.dir * self->moveinfo.speed;

	travel_time = self->moveinfo.remaining_distance / self->moveinfo.speed;
	frames = floor(travel_time / gi.frame_time_s);
	self->moveinfo.remaining_distance -= frames * self->moveinfo.speed * gi.frame_time_s;

	if (self->rotate)
	{
		vec3_t delta = self->moveinfo.end_angles - self->moveinfo.start_angles;

		if (!delta)
		{
			self->avelocity = {};
		}
		else
		{
			if (self->duration > 0)
				travel_time = self->duration;
			else
				travel_time = delta.length() / self->moveinfo.speed;

			frames = floor(travel_time / gi.frame_time_s);
			self->avelocity = delta * (1.f / travel_time);
		}
	}

	self->think = RotateTrain_MoveFinal;
	self->nextthink = level.time + (FRAME_TIME_S * frames);
}

static void RotateTrain_MoveCalc(edict_t *self, const vec3_t &dest, void (*endfunc)(edict_t *self))
{
	self->velocity = {};
	self->moveinfo.dir = dest - self->s.origin;
	self->moveinfo.remaining_distance = self->moveinfo.dir.normalize();
	self->moveinfo.endfunc = endfunc;

	if (self->duration > 0)
		self->moveinfo.speed = self->moveinfo.remaining_distance / self->duration;

	if (self->rotate)
	{
		self->moveinfo.start_angles = self->s.angles;
		self->moveinfo.end_angles = self->s.angles + self->rotate;
	}
	else if (self->rotate_speed)
	{
		self->avelocity = self->rotate_speed;
	}
	if (level.current_entity == ((self->flags & FL_TEAMSLAVE) ? self->teammaster : self))
	{
		RotateTrain_MoveBegin(self);
	}
	else
	{
		self->think = RotateTrain_MoveBegin;
		self->nextthink = level.time + FRAME_TIME_S;
	}
}

MOVEINFO_ENDFUNC(rotate_train_wait) (edict_t *self) -> void
{
	if (self->target_ent->pathtarget)
	{
		const char *savetarget = self->target_ent->target;
		edict_t *ent = self->target_ent;

		ent->target = ent->pathtarget;
		G_UseTargets(ent, self->activator);
		ent->target = savetarget;

		if (!self->inuse)
			return;
	}

	if (self->moveinfo.wait)
	{
		if (self->moveinfo.wait > 0)
		{
			self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
			self->think = rotate_train_next;
		}
		else if (self->spawnflags.has(SPAWNFLAG_ROTATE_TRAIN_TOGGLE))
		{
			rotate_train_next(self);
			self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
			self->velocity = {};
			self->nextthink = 0_ms;
		}

		if (!(self->flags & FL_TEAMSLAVE))
		{
			if (self->moveinfo.sound_end)
				gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_end, 1, ATTN_STATIC, 0);
			self->s.sound = 0;
		}
	}
	else
	{
		rotate_train_next(self);
	}
}

THINK(rotate_train_next) (edict_t *self) -> void
{
	bool first = true;

again:
	if (!self->target)
		return;

	edict_t *ent = G_PickTarget(self->target);
	if (!ent)
	{
		gi.Com_PrintFmt("train_next: bad target {}\n", self->target);
		return;
	}

	self->target = ent->target;

	if (ent->spawnflags.has(SPAWNFLAG_PATH_CORNER_TELEPORT))
	{
		if (!first)
		{
			gi.Com_PrintFmt("connected teleport path_corners, see {} at {}\n", ent->classname ? ent->classname : "<null>", ent->s.origin);
			return;
		}

		first = false;
		self->s.origin = ent->s.origin;
		self->s.old_origin = self->s.origin;
		self->s.event = EV_OTHER_TELEPORT;
		gi.linkentity(self);
		goto again;
	}

	self->moveinfo.wait = ent->wait;
	self->target_ent = ent;

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self->s.sound = self->moveinfo.sound_middle;
	}

	self->duration = ent->duration > 0 ? ent->duration : 0.f;

	if (ent->speed > 0)
		self->moveinfo.speed = ent->speed;

	if (ent->rotate)
		self->rotate = ent->rotate;
	else
		self->rotate = {};

	if (ent->rotate_speed)
		self->rotate_speed = ent->rotate_speed;
	else
		self->rotate_speed = {};

	self->moveinfo.start_origin = self->s.origin;
	self->moveinfo.end_origin = ent->s.origin;

	RotateTrain_MoveCalc(self, ent->s.origin, rotate_train_wait);
	self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;
}

static void rotate_train_resume(edict_t *self)
{
	if (!self->target_ent)
		return;

	self->moveinfo.start_origin = self->s.origin;
	self->moveinfo.end_origin = self->target_ent->s.origin;

	RotateTrain_MoveCalc(self, self->target_ent->s.origin, rotate_train_wait);
	self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;
}

THINK(rotate_train_find) (edict_t *self) -> void
{
	if (!self->target)
	{
		gi.Com_PrintFmt("train_find: no target\n");
		return;
	}

	edict_t *ent = G_PickTarget(self->target);
	if (!ent)
	{
		gi.Com_PrintFmt("train_find: target {} not found\n", self->target);
		return;
	}

	self->target = ent->target;
	self->duration = ent->duration > 0 ? ent->duration : 0.f;

	if (ent->rotate)
		self->rotate = ent->rotate;
	else
		self->rotate = {};

	if (ent->rotate_speed)
		self->rotate_speed = ent->rotate_speed;
	else
		self->rotate_speed = {};

	self->moveinfo.speed = ent->speed;

	self->s.origin = ent->s.origin;
	self->s.old_origin = self->s.origin;
	gi.linkentity(self);

	if (!self->targetname)
		self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

	if (self->spawnflags.has(SPAWNFLAG_TRAIN_START_ON))
	{
		self->think = rotate_train_next;
		self->nextthink = level.time + FRAME_TIME_S;
		self->activator = self;
	}
}

USE(rotate_train_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->activator = activator;

	if (self->spawnflags.has(SPAWNFLAG_TRAIN_START_ON))
	{
		if (!self->spawnflags.has(SPAWNFLAG_ROTATE_TRAIN_TOGGLE))
			return;

		self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
		self->velocity = {};
		self->nextthink = 0_ms;
		return;
	}

	if (self->target_ent)
		rotate_train_resume(self);
	else
		rotate_train_next(self);
}

/*QUAKED func_rotate_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
Rotating trains are moving platforms that players can ride.
The target origin specifies the exact point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving
until activated.

"speed"		default 100
"dmg"		default 100
"noise"		looping sound to play when the train is in motion

Per-corner keys on path_corner:
"wait"		pause before advancing
"duration"	overrides travel time for the next leg
"rotate"	x y z angle delta for the next leg
"speeds"	x y z angular speeds for the next leg
*/
void SP_func_rotate_train(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	self->s.angles = {};
	self->moveinfo.blocked = train_blocked;

	if (self->spawnflags.has(SPAWNFLAG_ROTATE_TRAIN_BLOCK_STOPS))
		self->dmg = 0;
	else if (!self->dmg)
		self->dmg = 100;

	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	if (st.noise)
		self->moveinfo.sound_middle = gi.soundindex(st.noise);

	if (!self->speed)
		self->speed = 100;

	if (self->duration <= 0)
		self->duration = 0;

	self->moveinfo.speed = self->speed;
	self->use = rotate_train_use;

	gi.linkentity(self);

	if (self->target)
	{
		self->think = rotate_train_find;
		self->nextthink = level.time + FRAME_TIME_S;
	}
	else
	{
		gi.Com_PrintFmt("func_train without a target at {}\n", self->absmin);
	}
}
