#include "g_local.h"

void InitTrigger(edict_t *self);
void multi_wait(edict_t *self);
void trigger_enable(edict_t *self, edict_t *other, edict_t *activator);

constexpr float CAMERA_DEFAULT_WAIT = 3.0f;
constexpr float CAMERA_PATH_DEFAULT_SPEED = 100.0f;
constexpr float CAMERA_PATH_STOPPED = -1.0f;
constexpr int32_t CAMERA_MAX_STEPS_PER_THINK = 64;

static void Camera_MoveStep(edict_t *self);

static float Camera_ResolvePathSpeed(const edict_t *node, float fallback_speed)
{
	if (node && node->speed > 0.0f)
		return node->speed;
	if (fallback_speed > 0.0f)
		return fallback_speed;

	return CAMERA_PATH_DEFAULT_SPEED;
}

static edict_t *Camera_FindPathCorner(edict_t *self, const char *targetname, const edict_t *skip = nullptr)
{
	edict_t *node;
	bool found_path = false;

	if (!targetname || !targetname[0])
		return nullptr;

	for (node = G_FindByString<&edict_t::targetname>(nullptr, targetname); node;
		node = G_FindByString<&edict_t::targetname>(node, targetname))
	{
		if (!node->classname || Q_strcasecmp(node->classname, "path_corner"))
			continue;
		found_path = true;
		if (node == skip)
			continue;
		return node;
	}

	if (!found_path)
		gi.Com_PrintFmt("{} path {} not found\n", *self, targetname);
	return nullptr;
}

static edict_t *Camera_FindPathStart(edict_t *self, const char *targetname)
{
	return Camera_FindPathCorner(self, targetname);
}

static void Camera_ResetPathState(edict_t *self, bool clear_enemy)
{
	self->velocity = vec3_origin;
	self->avelocity = vec3_origin;
	self->delay = 0.0f;
	self->camera_path_remaining = 0.0f;
	self->camera_path_tail_speed = 0.0f;
	self->camera_path_time = 0_ms;
	self->camera_path_state = 0;
	self->camera_path_dir = vec3_origin;
	self->goalentity = nullptr;
	self->movetarget = nullptr;

	if (clear_enemy)
		self->enemy = nullptr;
}

static bool Camera_RunReadySteps(edict_t *self)
{
	for (int32_t i = 0; i < CAMERA_MAX_STEPS_PER_THINK &&
		self->camera_path_time != 0_ms &&
		self->camera_path_time <= level.time; i++)
	{
		Camera_MoveStep(self);
	}

	if (self->camera_path_time != 0_ms && self->camera_path_time <= level.time)
	{
		gi.Com_PrintFmt("{} camera path loop detected, stopping path\n", *self);
		Camera_ResetPathState(self, false);
		return false;
	}

	return true;
}

static void Camera_StartPath(edict_t *self, edict_t *start, float speed)
{
	self->movetarget = start;
	self->camera_path_speed = Camera_ResolvePathSpeed(start, speed);
	self->camera_path_state = 0;
	self->s.origin = start->s.origin;
	self->s.old_origin = self->s.origin;
	self->camera_path_time = level.time;
	gi.linkentity(self);
}

static edict_t *DummyBody_Find(edict_t *player)
{
	if (!player || !player->client)
		return nullptr;
	if (!player->client->remote_view_body)
		return nullptr;
	if (!player->client->remote_view_body->inuse)
	{
		player->client->remote_view_body = nullptr;
		return nullptr;
	}

	return player->client->remote_view_body;
}

static void DummyBody_Sync(edict_t *player, edict_t *body)
{
	if (!player || !body)
		return;

	body->s.origin = player->s.origin;
	body->s.angles = player->s.angles;
	body->velocity = player->velocity;
	body->avelocity = player->avelocity;
	body->s.modelindex = player->s.modelindex;
	body->s.modelindex2 = player->s.modelindex2;
	body->s.modelindex3 = player->s.modelindex3;
	body->s.modelindex4 = player->s.modelindex4;
	body->s.skinnum = player->s.skinnum;
	body->s.frame = player->s.frame;
	body->s.scale = player->s.scale;
	gi.linkentity(body);
}

static void DummyBody_Setup(edict_t *player)
{
	edict_t *body;

	if (!player || !player->client)
		return;

	body = DummyBody_Find(player);
	if (!body)
	{
		body = G_Spawn();
		player->client->remote_view_body = body;
	}

	body->classname = "dummy_body";
	body->enemy = player;
	body->solid = SOLID_NOT;
	body->movetype = MOVETYPE_NONE;
	body->clipmask = CONTENTS_NONE;
	body->svflags &= ~SVF_NOCLIENT;
	body->takedamage = false;
	body->mins = player->mins;
	body->maxs = player->maxs;
	DummyBody_Sync(player, body);
	player->client->remote_view_active = true;
	player->svflags |= SVF_NOCLIENT;
	gi.linkentity(player);
}

static void DummyBody_Teardown(edict_t *player)
{
	edict_t *body;

	if (!player || !player->client)
		return;

	body = DummyBody_Find(player);
	player->client->remote_view_body = nullptr;
	player->client->remote_view_active = false;
	player->svflags &= ~SVF_NOCLIENT;
	gi.linkentity(player);

	if (body)
		G_FreeEdict(body);
}

static edict_t *RemoteView_GetEntity(edict_t *ent)
{
	edict_t *viewent;

	if (!ent || !ent->client)
		return nullptr;
	if (!ent->client->remote_view_active)
		return nullptr;

	viewent = ent->client->remote_view_entity;
	if (!viewent || !viewent->inuse)
	{
		ent->client->remote_view_entity = nullptr;
		RemoteView_End(ent);
		return nullptr;
	}

	return viewent;
}

static void RemoteView_ApplyClientState(edict_t *ent, edict_t *viewent)
{
	if (!ent || !ent->client || !viewent)
		return;

	ent->client->ps.pmove.origin = viewent->s.origin;
	ent->client->ps.pmove.velocity = viewent->velocity;
	ent->client->ps.viewangles = viewent->s.angles;
	ent->client->ps.viewoffset[0] = static_cast<float>(ent->client->remote_view_state_1);
	ent->client->ps.viewoffset[1] = static_cast<float>(ent->client->remote_view_state_2);
	ent->client->ps.viewoffset[2] = ent->client->remote_view_timer;
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.pmove.viewheight = 0;
}

static bool RemoteView_ClientPreFrame(edict_t *ent)
{
	return RemoteView_GetEntity(ent) != nullptr;
}

static bool RemoteView_ClientPostFrame(edict_t *ent)
{
	edict_t *viewent;
	edict_t *body;

	viewent = RemoteView_GetEntity(ent);
	if (!viewent)
		return false;

	RemoteView_ApplyClientState(ent, viewent);

	body = DummyBody_Find(ent);
	if (body)
		DummyBody_Sync(ent, body);

	return true;
}

void RemoteView_Begin(edict_t *ent, edict_t *viewent)
{
	(void) viewent;

	if (!ent || !ent->client)
		return;

	if (ent->client->remote_view_active)
		RemoteView_End(ent);

	DummyBody_Setup(ent);
	ent->client->remote_view_cmd_hook = nullptr;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.f;
	ent->client->remote_view_saved_gunindex = ent->client->ps.gunindex;
	ent->client->ps.gunindex = 0;
	ent->svflags |= SVF_NOCLIENT;
	gi.linkentity(ent);
}

void RemoteView_End(edict_t *ent)
{
	if (!ent || !ent->client)
		return;

	DummyBody_Teardown(ent);
	ent->client->remote_view_cmd_hook = nullptr;
	ent->client->remote_view_entity = nullptr;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.f;
	ent->client->ps.gunindex = ent->client->remote_view_saved_gunindex;
	ent->client->remote_view_saved_gunindex = 0;
}

void RemoteView_AttachController(edict_t *ent, edict_t *viewent, remote_view_cmd_func_t *cmd_hook)
{
	if (!ent || !ent->client || !viewent)
		return;
	if (ent->client->remote_view_entity == viewent && ent->client->remote_view_active)
	{
		ent->client->remote_view_cmd_hook = cmd_hook;
		return;
	}

	if (ent->client->remote_view_entity || ent->client->remote_view_active)
		RemoteView_End(ent);

	ent->client->remote_view_entity = viewent;
	RemoteView_Begin(ent, viewent);
	ent->client->remote_view_cmd_hook = cmd_hook;
}

void RemoteView_DetachController(edict_t *ent, edict_t *viewent)
{
	if (!ent || !ent->client)
		return;
	if (ent->client->remote_view_entity != viewent)
		return;

	RemoteView_End(ent);
	ent->client->remote_view_entity = nullptr;
}

REMOTE_VIEW_CMD(Camera_RemoteViewCmd) (edict_t *ent, usercmd_t *ucmd) -> void
{
	edict_t *camera;

	if (!ent || !ent->client || !ucmd)
		return;

	camera = ent->client->remote_view_entity;
	if (!camera || !camera->inuse)
		return;
	if (!camera->spawnflags.has(1_spawnflag))
		return;

	ucmd->angles[PITCH] = 0;
	ucmd->angles[YAW] = 0;
	ucmd->angles[ROLL] = 0;
	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_USE | BUTTON_HOLSTER | BUTTON_JUMP | BUTTON_CROUCH);
}

static void Camera_AttachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
		return;

	RemoteView_AttachController(player, camera, Camera_RemoteViewCmd);
}

static void Camera_DetachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
		return;

	RemoteView_DetachController(player, camera);
}

static void Camera_MoveStep(edict_t *self)
{
	vec3_t delta;
	float distance;
	float move_time;
	float main_time;
	float remaining_time;
	float speed;
	float remaining_speed;
	int32_t full_frames;

	switch (self->camera_path_state)
	{
	case 0:
	case 1:
		if (!self->movetarget)
		{
			self->camera_path_time = 0_ms;
			self->delay = CAMERA_PATH_STOPPED;
			return;
		}

		delta = self->movetarget->s.origin - self->s.origin;
		self->camera_path_dir = delta;
		distance = self->camera_path_dir.normalize();
		self->camera_path_remaining = distance;

		if (distance <= 0.0f)
		{
			self->velocity = vec3_origin;
			self->avelocity = vec3_origin;
			self->camera_path_remaining = 0.0f;
			self->camera_path_tail_speed = 0.0f;
			self->camera_path_state = 3;
			self->camera_path_time = level.time;
			return;
		}

		speed = self->camera_path_speed;
		if (speed <= 0.0f)
			speed = 1.0f;

		move_time = distance / speed;
		full_frames = static_cast<int32_t>(move_time * 10.0f);
		main_time = full_frames * FRAME_TIME_S.seconds();
		remaining_time = move_time - main_time;
		self->camera_path_state = 2;
		self->camera_path_remaining = distance - (main_time * speed);
		if (self->camera_path_remaining < 0.0f)
			self->camera_path_remaining = 0.0f;

		remaining_speed = 0.0f;
		if (remaining_time > 0.0f)
			remaining_speed = self->camera_path_remaining / remaining_time;
		self->camera_path_tail_speed = remaining_speed;

		if (main_time > 0.0f)
		{
			self->velocity = self->camera_path_dir * speed;
			self->camera_path_time = level.time + gtime_t::from_sec(main_time);
			return;
		}

		if (remaining_speed > 0.0f)
			self->velocity = self->camera_path_dir * remaining_speed;
		else
			self->velocity = vec3_origin;

		self->camera_path_time = level.time + gtime_t::from_sec(remaining_time);
		return;

	case 2:
	{
		float next_path_speed;

		self->camera_path_time = 0_ms;
		next_path_speed = self->camera_path_speed;
		if (self->movetarget)
		{
			// Preserve the save layout by reusing delay as the cached path_corner wait.
			self->delay = self->movetarget->wait;
			if (self->movetarget->speed > 0.0f)
				next_path_speed = self->movetarget->speed;
		}

		if (self->camera_path_remaining > 0.0f)
		{
			speed = self->camera_path_tail_speed;
			if (speed > 0.0f)
			{
				self->velocity = self->camera_path_dir * speed;
				self->camera_path_time = level.time + gtime_t::from_sec(self->camera_path_remaining / speed);
			}
		}

		self->camera_path_speed = next_path_speed;
		self->camera_path_remaining = 0.0f;
		self->camera_path_tail_speed = 0.0f;
		self->camera_path_state = 3;
		if (self->camera_path_time == 0_ms)
			self->camera_path_time = level.time;
		return;
	}

	case 3:
	{
		edict_t *current;
		edict_t *next;

		current = self->movetarget;
		if (self->movetarget)
		{
			self->s.origin = self->movetarget->s.origin;
			self->s.old_origin = self->s.origin;
			gi.linkentity(self);
		}

		next = current ? Camera_FindPathCorner(self, current->target, current) : nullptr;

		if (self->delay != CAMERA_PATH_STOPPED && next)
		{
			self->camera_path_time = level.time + gtime_t::from_sec(self->delay);
			self->camera_path_state = 1;
			self->delay = 0.0f;
		}
		else
		{
			self->camera_path_time = 0_ms;
			self->camera_path_state = 0;
			if (self->delay != CAMERA_PATH_STOPPED)
				self->delay = 0.0f;
		}

		self->movetarget = next;
		self->goalentity = next;

		self->velocity = vec3_origin;
		self->avelocity = vec3_origin;
		return;
	}
	}
}

static void Camera_Deactivate(edict_t *self)
{
	edict_t *player;

	for (size_t i = 1; i <= game.maxclients; i++)
	{
		player = &g_edicts[i];
		if (!player->inuse || !player->client)
			continue;
		Camera_DetachClient(player, self);
	}

	Camera_ResetPathState(self, true);
	self->count = 0;
	self->timestamp = 0_ms;
	self->nextthink = 0_ms;
}

static void Camera_Activate(edict_t *self)
{
	edict_t *player;
	edict_t *path_start;

	Camera_ResetPathState(self, true);

	if (self->target)
	{
		self->enemy = G_PickTarget(self->target);
		if (!self->enemy)
			gi.Com_PrintFmt("{} target {} not found\n", self->classname, self->target);
	}

	path_start = Camera_FindPathStart(self, self->pathtarget);
	if (path_start)
	{
		Camera_StartPath(self, path_start, self->speed);
	}

	for (size_t i = 1; i <= game.maxclients; i++)
	{
		player = &g_edicts[i];
		if (!player->inuse || !player->client)
			continue;
		Camera_AttachClient(player, self);
	}

	self->count = 1;
	self->timestamp = level.time;
	self->nextthink = level.time + FRAME_TIME_S;
}

USE(Camera_Use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	(void) other;
	(void) activator;

	if (self->count == 0)
	{
		Camera_Activate(self);
		return;
	}

	Camera_Deactivate(self);
}

THINK(Camera_Think) (edict_t *self) -> void
{
	vec3_t current_forward;
	vec3_t target;
	vec3_t target_dir;

	if (self->wait != CAMERA_PATH_STOPPED && self->timestamp + gtime_t::from_sec(self->wait) < level.time)
	{
		Camera_Deactivate(self);
		return;
	}

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		Camera_Deactivate(self);
		return;
	}

	if (self->movetarget)
		Camera_RunReadySteps(self);

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		Camera_Deactivate(self);
		return;
	}

	if (self->enemy && !self->enemy->inuse)
		self->enemy = nullptr;
	else if (self->enemy)
	{
		target = (self->enemy->absmin + self->enemy->absmax) * 0.5f;
		target[2] += self->enemy->viewheight;
		target_dir = target - self->s.origin;

		if (target_dir != vec3_origin)
		{
			target_dir.normalize();
			AngleVectors(self->s.angles, current_forward, nullptr, nullptr);
			current_forward *= 0.9f;
			current_forward += target_dir * 0.1f;
			current_forward.normalize();
			self->s.angles = vectoangles(current_forward);
			gi.linkentity(self);
		}
	}

	if (self->enemy)
	{
		self->nextthink = level.time + FRAME_TIME_S;
		return;
	}

	self->nextthink = self->timestamp + gtime_t::from_sec(self->wait);
	if (self->camera_path_time != 0_ms && self->nextthink > self->camera_path_time)
		self->nextthink = self->camera_path_time;
}

void SP_misc_camera(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_FLYMISSILE;
	self->svflags |= SVF_NOCLIENT;
	self->mins = vec3_origin;
	self->maxs = vec3_origin;

	if (!self->targetname)
	{
		gi.Com_PrintFmt("{} with no targetname\n", self->classname);
		self->targetname = "unused";
	}

	if (self->wait == 0.0f)
		self->wait = CAMERA_DEFAULT_WAIT;

	self->use = Camera_Use;
	self->think = Camera_Think;
	self->s.modelindex = gi.modelindex("sprites/s_deatom1.sp2");
	gi.linkentity(self);
}

void Camera_ClientPreFrame(edict_t *ent)
{
	if (!ent->client)
		return;

	RemoteView_ClientPreFrame(ent);
}

void Camera_ClientPostFrame(edict_t *ent)
{
	if (!ent->client)
		return;

	RemoteView_ClientPostFrame(ent);
}

static void TriggerCamera_Fire(edict_t *self)
{
	edict_t *camera = nullptr;
	int32_t sound_index;

	if (self->nextthink != 0_ms)
		return;

	if (self->message && self->activator && !(self->activator->svflags & SVF_MONSTER))
	{
		gi.LocCenter_Print(self->activator, "{}", self->message);
		sound_index = self->noise_index;
		if (!sound_index)
			sound_index = gi.soundindex("misc/talk1.wav");
		gi.sound(self->activator, CHAN_AUTO, sound_index, 1.0f, ATTN_NORM, 0);
	}

	if (self->target)
	{
		for (camera = G_FindByString<&edict_t::targetname>(nullptr, self->target); camera;
			camera = G_FindByString<&edict_t::targetname>(camera, self->target))
		{
			if (!camera->classname || Q_strcasecmp(camera->classname, "misc_camera"))
				continue;

			if (self->wait != 0.0f)
				camera->wait = self->wait;

			if (self->pathtarget)
				camera->target = self->pathtarget;
			else
				camera->enemy = self->activator;

			if (camera->count == 0)
				Camera_Use(camera, self, self->activator);
			else
				camera->timestamp = level.time;
			break;
		}

		if (!camera)
			gi.Com_PrintFmt("Illegal target for trigger_misc_camera\n");
	}

	if (self->delay > 0.0f)
	{
		self->think = multi_wait;
		self->nextthink = level.time + gtime_t::from_sec(self->delay);
		return;
	}

	self->touch = nullptr;
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAME_TIME_S;
}

USE(TriggerCamera_Use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	(void) other;

	self->activator = activator;
	TriggerCamera_Fire(self);
}

TOUCH(TriggerCamera_Touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t forward;

	(void) tr;
	(void) other_touching_self;

	if (other->client)
	{
		if (self->spawnflags.has(2_spawnflag))
			return;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!self->spawnflags.has(1_spawnflag))
			return;
	}
	else
	{
		return;
	}

	if (self->movedir != vec3_origin)
	{
		AngleVectors(other->s.angles, forward, nullptr, nullptr);
		if (forward.dot(self->movedir) < 0)
			return;
	}

	self->activator = other;
	TriggerCamera_Fire(self);
}

void SP_trigger_misc_camera(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	if (self->sounds == 1)
		self->noise_index = gi.soundindex("misc/secret.wav");
	else if (self->sounds == 2)
		self->noise_index = gi.soundindex("misc/talk.wav");
	else if (self->sounds == 3)
		self->noise_index = gi.soundindex("misc/trigger1.wav");

	InitTrigger(self);
	self->touch = TriggerCamera_Touch;
	if (self->spawnflags.has(4_spawnflag))
	{
		self->solid = SOLID_NOT;
		self->use = trigger_enable;
	}
	else
	{
		self->solid = SOLID_TRIGGER;
		self->use = TriggerCamera_Use;
	}

	gi.linkentity(self);
}

USE(Camera_TargetUse) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	(void) other;
	(void) activator;

	edict_t *path_start;

	if (self->count != 0)
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	self->count = 1;
	Camera_ResetPathState(self, false);
	path_start = Camera_FindPathStart(self, self->target);
	if (!path_start)
	{
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	Camera_StartPath(self, path_start, self->speed);

	self->nextthink = level.time + FRAME_TIME_S;
}

THINK(Camera_TargetThink) (edict_t *self) -> void
{
	if (!self->movetarget)
	{
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	Camera_RunReadySteps(self);

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	if (!self->movetarget && self->camera_path_time == 0_ms)
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0_ms;
		return;
	}

	self->nextthink = self->camera_path_time;
}

void SP_misc_camera_target(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_FLYMISSILE;
	self->svflags |= SVF_NOCLIENT;
	self->mins = vec3_origin;
	self->maxs = vec3_origin;

	if (!self->targetname)
	{
		gi.Com_PrintFmt("{} with no targetname\n", self->classname);
		self->targetname = "unused";
	}

	self->count = 0;
	self->movetarget = nullptr;
	self->use = Camera_TargetUse;
	self->think = Camera_TargetThink;
	self->s.modelindex = gi.modelindex("sprites/s_deatom1.sp2");
	gi.linkentity(self);
}

void SP_misc_deatomizer_control(edict_t *self)
{
	SP_misc_camera(self);
}

void SP_misc_deatomizer_target(edict_t *self)
{
	SP_misc_camera_target(self);
}
