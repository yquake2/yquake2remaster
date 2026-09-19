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

#include "header/local.h"

#define CAMERA_DEFAULT_WAIT		3.0f
#define CAMERA_PATH_DEFAULT_SPEED	100.0f
#define CAMERA_PATH_STOPPED		-1.0f

static edict_t *DummyBody_Find(edict_t *player);
static void DummyBody_Sync(edict_t *player, edict_t *body);
static void DummyBody_Setup(edict_t *player);
static void DummyBody_Teardown(edict_t *player);
static edict_t *RemoteView_GetEntity(edict_t *ent);
static qboolean RemoteView_ClientPreFrame(edict_t *ent);
static qboolean RemoteView_ClientPostFrame(edict_t *ent);
static void Camera_RemoteViewCmd(edict_t *ent, usercmd_t *ucmd);
static void Camera_AttachClient(edict_t *player, edict_t *camera);
static void Camera_DetachClient(edict_t *player, edict_t *camera);
static void Camera_MoveStep(edict_t *self);
static void Camera_Deactivate(edict_t *self);
static void Camera_Activate(edict_t *self);
void Camera_Use(edict_t *self, edict_t *other, edict_t *activator);
void Camera_Think(edict_t *self);
void TriggerCamera_Fire(edict_t *self);
void TriggerCamera_Use(edict_t *self, edict_t *other, edict_t *activator);
void TriggerCamera_Touch(edict_t *self, edict_t *other, const cplane_t *plane, const csurface_t *surf);
void Camera_TargetUse(edict_t *self, edict_t *other, edict_t *activator);
void Camera_TargetThink(edict_t *self);

/*
=================
DummyBody_Find
=================
*/
static edict_t *DummyBody_Find(edict_t *player)
{
	if (!player || !player->client)
		return NULL;
	if (!player->client->remote_view_body)
		return NULL;
	if (!player->client->remote_view_body->inuse)
	{
		player->client->remote_view_body = NULL;
		return NULL;
	}

	return player->client->remote_view_body;
}

/*
=================
DummyBody_Sync
=================
*/
static void DummyBody_Sync(edict_t *player, edict_t *body)
{
	if (!player || !body)
		return;

	VectorCopy(player->s.origin, body->s.origin);
	VectorCopy(player->s.angles, body->s.angles);
	VectorCopy(player->velocity, body->velocity);
	VectorCopy(player->avelocity, body->avelocity);
	body->s.modelindex = player->s.modelindex;
	body->s.modelindex2 = player->s.modelindex2;
	body->s.frame = player->s.frame;
	gi.linkentity(body);
}

/*
=================
DummyBody_Setup
=================
*/
static void DummyBody_Setup(edict_t *player)
{
	edict_t	*body;

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
	VectorCopy(player->mins, body->mins);
	VectorCopy(player->maxs, body->maxs);
	DummyBody_Sync(player, body);
	player->client->remote_view_active = true;
	player->svflags |= SVF_NOCLIENT;
	gi.linkentity(player);
}

/*
=================
DummyBody_Teardown
=================
*/
static void DummyBody_Teardown(edict_t *player)
{
	edict_t	*body;

	if (!player || !player->client)
		return;

	body = DummyBody_Find(player);
	player->client->remote_view_body = NULL;
	player->client->remote_view_active = false;
	player->svflags &= ~SVF_NOCLIENT;
	gi.linkentity(player);

	if (body)
		G_FreeEdict(body);
}

/*
=================
RemoteView_GetEntity
=================
*/
static edict_t *RemoteView_GetEntity(edict_t *ent)
{
	edict_t	*viewent;

	if (!ent || !ent->client)
		return NULL;
	if (!ent->client->remote_view_active)
		return NULL;

	viewent = ent->client->remote_view_entity;
	if (!viewent || !viewent->inuse)
	{
		ent->client->remote_view_entity = NULL;
		RemoteView_End(ent);
		return NULL;
	}

	return viewent;
}

/*
=================
RemoteView_ApplyClientState
=================
*/
static void RemoteView_ApplyClientState(edict_t *ent, edict_t *viewent)
{
	if (!ent || !ent->client || !viewent)
		return;

	ent->client->ps.pmove.origin[0] = (short)(viewent->s.origin[0] * 8.0f);
	ent->client->ps.pmove.origin[1] = (short)(viewent->s.origin[1] * 8.0f);
	ent->client->ps.pmove.origin[2] = (short)(viewent->s.origin[2] * 8.0f);
	VectorCopy(viewent->s.angles, ent->client->ps.viewangles);
	ent->client->ps.viewoffset[0] = (float)ent->client->remote_view_state_1;
	ent->client->ps.viewoffset[1] = (float)ent->client->remote_view_state_2;
	ent->client->ps.viewoffset[2] = ent->client->remote_view_timer;
	ent->client->ps.pmove.pm_type = PM_FREEZE;
}

/*
=================
RemoteView_ClientPreFrame
=================
*/
static qboolean RemoteView_ClientPreFrame(edict_t *ent)
{
	return RemoteView_GetEntity(ent) != NULL;
}

/*
=================
RemoteView_ClientPostFrame
=================
*/
static qboolean RemoteView_ClientPostFrame(edict_t *ent)
{
	edict_t	*viewent;
	edict_t	*body;

	viewent = RemoteView_GetEntity(ent);
	if (!viewent)
		return false;

	RemoteView_ApplyClientState(ent, viewent);

	body = DummyBody_Find(ent);
	if (body)
		DummyBody_Sync(ent, body);

	return true;
}

/*
=================
RemoteView_Begin
=================
*/
void RemoteView_Begin(edict_t *ent, edict_t *viewent)
{
	(void)viewent;

	if (!ent || !ent->client)
		return;

	if (ent->client->remote_view_active)
		RemoteView_End(ent);

	DummyBody_Setup(ent);
	ent->client->remote_view_cmd_hook = NULL;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.0f;
	ent->client->remote_view_saved_gunindex = ent->client->ps.gunindex;
	ent->client->ps.gunindex = 0;
	ent->svflags |= SVF_NOCLIENT;
	gi.linkentity(ent);
}

/*
=================
RemoteView_End
=================
*/
void RemoteView_End(edict_t *ent)
{
	if (!ent || !ent->client)
		return;

	DummyBody_Teardown(ent);
	ent->client->remote_view_cmd_hook = NULL;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.0f;
	ent->client->ps.gunindex = ent->client->remote_view_saved_gunindex;
	ent->client->remote_view_saved_gunindex = 0;
}

/*
=================
RemoteView_AttachController
=================
*/
void RemoteView_AttachController(edict_t *ent, edict_t *viewent,
	remote_view_cmd_func_t *cmd_hook)
{
	if (!ent || !ent->client || !viewent)
		return;
	if (ent->client->remote_view_entity)
		return;

	ent->client->remote_view_entity = viewent;
	RemoteView_Begin(ent, viewent);
	ent->client->remote_view_cmd_hook = cmd_hook;
}

/*
=================
RemoteView_DetachController
=================
*/
void RemoteView_DetachController(edict_t *ent, edict_t *viewent)
{
	if (!ent || !ent->client)
		return;
	if (ent->client->remote_view_entity != viewent)
		return;

	RemoteView_End(ent);
	ent->client->remote_view_entity = NULL;
}

/*
=================
Camera_RemoteViewCmd
=================
*/
static void Camera_RemoteViewCmd(edict_t *ent, usercmd_t *ucmd)
{
	edict_t	*camera;

	if (!ent || !ent->client || !ucmd)
		return;

	camera = ent->client->remote_view_entity;
	if (!camera || !camera->inuse)
		return;
	if (!(camera->spawnflags & 1))
		return;

	ucmd->angles[0] = 0;
	ucmd->angles[1] = 0;
	ucmd->angles[2] = 0;
	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->upmove = 0;
	ucmd->buttons &= 0xfc;
}

/*
=================
Camera_AttachClient
=================
*/
static void Camera_AttachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
		return;

	RemoteView_AttachController(player, camera, Camera_RemoteViewCmd);
}

/*
=================
Camera_DetachClient
=================
*/
static void Camera_DetachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
		return;

	RemoteView_DetachController(player, camera);
}

/*
=================
Camera_MoveStep
=================
*/
static void Camera_MoveStep(edict_t *self)
{
	edict_t	*target;
	vec3_t	dir;
	float	dist;
	float	speed;
	float	travel_time;

	target = self->movetarget;
	if (!target)
	{
		self->delay = CAMERA_PATH_STOPPED;
		return;
	}

	VectorSubtract(target->s.origin, self->s.origin, dir);
	dist = VectorLength(dir);
	speed = self->speed ? self->speed : CAMERA_PATH_DEFAULT_SPEED;

	if (speed <= 0.0f)
		speed = CAMERA_PATH_DEFAULT_SPEED;

	travel_time = dist / speed;
	if (travel_time <= 0.0f)
		travel_time = 0.01f;

	VectorCopy(target->s.origin, self->s.origin);
	VectorCopy(target->s.angles, self->s.angles);
	gi.linkentity(self);

	if (target->pathtarget)
	{
		edict_t	*ent;

		ent = G_Find(NULL, FOFS(targetname), target->pathtarget);
		if (ent)
		{
			self->pathtarget = target->pathtarget;
			self->movetarget = ent;
		}
		else
		{
			self->movetarget = NULL;
		}
	}
	else
	{
		self->movetarget = target->target_ent;
	}

	if (target->message && target->message[0])
	{
		// Optional message broadcast
	}

	if (target->spawnflags & 1)
	{
		self->delay = CAMERA_PATH_STOPPED;
	}
}

/*
=================
Camera_Deactivate
=================
*/
static void Camera_Deactivate(edict_t *self)
{
	edict_t	*player;

	self->count = 0;
	if (self->enemy && self->enemy->client && self->enemy->client->remote_view_entity == self)
	{
		player = self->enemy;
		Camera_DetachClient(player, self);
	}

	if (self->target)
	{
		edict_t	*targ;

		for (targ = G_Find(NULL, FOFS(targetname), self->target);
			targ;
			targ = G_Find(targ, FOFS(targetname), self->target))
		{
			if (targ->use)
				targ->use(targ, self, self);
		}
	}
}

/*
=================
Camera_Activate
=================
*/
static void Camera_Activate(edict_t *self)
{
	edict_t	*player;

	self->count = 1;
	if (self->enemy && self->enemy->client)
	{
		player = self->enemy;
		if (self->target)
		{
			edict_t	*target;

			target = G_Find(NULL, FOFS(targetname), self->target);
			if (target)
			{
				self->movetarget = target;
				self->speed = target->speed ? target->speed : CAMERA_PATH_DEFAULT_SPEED;
			}
		}

		Camera_AttachClient(player, self);
	}
}

/*
=================
Camera_Use
=================
*/
void Camera_Use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->enemy = activator;
	if (self->count == 0)
	{
		Camera_Activate(self);
		return;
	}
	Camera_Deactivate(self);
}

/*
=================
Camera_Think
=================
*/
void Camera_Think(edict_t *self)
{
	if (self->wait != CAMERA_PATH_STOPPED &&
		self->timestamp && level.time - self->timestamp >= self->wait)
	{
		Camera_Deactivate(self);
		return;
	}

	if (self->movetarget && self->delay == CAMERA_PATH_STOPPED)
	{
		Camera_Deactivate(self);
		return;
	}

	if (self->movetarget)
	{
		Camera_MoveStep(self);
	}

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

/*
=================
SP_misc_camera
=================
*/
void SP_misc_camera(edict_t *self)
{
	if (!self->wait)
		self->wait = CAMERA_DEFAULT_WAIT;

	self->use = Camera_Use;
	self->think = Camera_Think;
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8.0f, -8.0f, -8.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 8.0f);
	gi.linkentity(self);
}

/*
=================
Camera_ClientPreFrame
=================
*/
void Camera_ClientPreFrame(edict_t *ent)
{
	RemoteView_ClientPreFrame(ent);
}

/*
=================
Camera_ClientPostFrame
=================
*/
void Camera_ClientPostFrame(edict_t *ent)
{
	RemoteView_ClientPostFrame(ent);
}

/*
=================
TriggerCamera_Fire
=================
*/
void TriggerCamera_Fire(edict_t *self)
{
	edict_t	*camera;

	if (!self->target)
		return;

	camera = NULL;
	for (camera = G_Find(NULL, FOFS(targetname), self->target);
		camera;
		camera = G_Find(camera, FOFS(targetname), self->target))
	{
		if (!camera->classname || strcmp(camera->classname, "misc_camera") != 0)
			continue;

		camera->wait = self->wait;
		camera->target = self->pathtarget;
		camera->enemy = self->activator;

		if (camera->count == 0)
			Camera_Use(camera, self, self->activator);
		else
			camera->timestamp = level.time;
	}

	if (!camera)
	{
		gi.bprintf(PRINT_HIGH, "Illegal target for trigger_misc_camera\n");
	}
}

/*
=================
TriggerCamera_Use
=================
*/
void TriggerCamera_Use(edict_t *self, edict_t *other, edict_t *activator)
{
	(void)other;
	self->activator = activator;
	TriggerCamera_Fire(self);
}

/*
=================
TriggerCamera_Touch
=================
*/
void TriggerCamera_Touch(edict_t *self, edict_t *other, const cplane_t *plane, const csurface_t *surf)
{
	(void)plane;
	(void)surf;

	if (!other->client)
		return;

	if (self->spawnflags & 1)
	{
		if (self->timestamp > level.time)
			return;
		self->timestamp = level.time + 1.0f;
	}

	self->activator = other;
	TriggerCamera_Fire(self);

	if (!(self->spawnflags & 1))
	{
		G_FreeEdict(self);
	}
}

/*
=================
SP_trigger_misc_camera
=================
*/
void SP_trigger_misc_camera(edict_t *self)
{
	if (!self->target)
	{
		gi.bprintf(PRINT_HIGH, "trigger_misc_camera without target\n");
		G_FreeEdict(self);
		return;
	}

	self->touch = TriggerCamera_Touch;
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_TRIGGER;
	gi.setmodel(self, self->model);
	gi.linkentity(self);
}

/*
=================
Camera_TargetUse
=================
*/
void Camera_TargetUse(edict_t *self, edict_t *other, edict_t *activator)
{
	(void)other;
	(void)activator;

	if (self->enemy)
	{
		self->enemy = NULL;
	}
}

/*
=================
Camera_TargetThink
=================
*/
void Camera_TargetThink(edict_t *self)
{
	self->nextthink = level.time + FRAMETIME;
}

/*
=================
SP_misc_camera_target
=================
*/
void SP_misc_camera_target(edict_t *self)
{
	if (!self->targetname)
	{
		gi.bprintf(PRINT_HIGH, "misc_camera_target without targetname\n");
		G_FreeEdict(self);
		return;
	}

	self->use = Camera_TargetUse;
	self->think = Camera_TargetThink;
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}
