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

/*
 * misc_camera, trigger_misc_camera, misc_camera_target and the generic
 * remote view support, ported from the Oblivion mod.
 *
 * Unlike the rerelease engine, this server derives the client view origin
 * from the player entity origin instead of ps.pmove.origin (see
 * ClientEndServerFrame() in player/view.c). So while a remote view is
 * active the player entity itself is moved to the view entity - the same
 * approach as the chase cam in g_chase.c - and a "dummy body" entity is
 * left behind at the player's real position. While the remote view is
 * active the player movement (pmove) is skipped in ClientThink().
 */

#include "header/local.h"

#define CAMERA_DEFAULT_WAIT		3.0f
#define CAMERA_PATH_DEFAULT_SPEED	100.0f
#define CAMERA_PATH_STOPPED		-1.0f
#define CAMERA_MAX_STEPS_PER_FRAME	64

/* non-static helpers from g_trigger.c */
void multi_wait(edict_t *ent);
void trigger_enable(edict_t *self, edict_t *other, edict_t *activator);

void DummyBody_Think(edict_t *self);
void Camera_Use(edict_t *self, edict_t *other, edict_t *activator);
void Camera_Think(edict_t *self);
void TriggerCamera_Use(edict_t *self, edict_t *other, edict_t *activator);
void TriggerCamera_Touch(edict_t *self, edict_t *other, const cplane_t *plane,
		const csurface_t *surf);
void Camera_TargetUse(edict_t *self, edict_t *other, edict_t *activator);
void Camera_TargetThink(edict_t *self);

/*
 * Searches the next path_corner with the given targetname,
 * optionally skipping one node (to survive looped paths).
 */
static edict_t *
Camera_FindPathCorner(edict_t *self, const char *targetname, const edict_t *skip)
{
	edict_t *node = NULL;
	qboolean found_path = false;

	if (!targetname || !targetname[0])
	{
		return NULL;
	}

	while ((node = G_Find(node, FOFS(targetname), targetname)) != NULL)
	{
		if (!node->classname || Q_stricmp(node->classname, "path_corner"))
		{
			continue;
		}

		found_path = true;

		if (node == skip)
		{
			continue;
		}

		return node;
	}

	if (!found_path)
	{
		gi.dprintf("%s: path %s not found\n",
				self->classname ? self->classname : "camera", targetname);
	}

	return NULL;
}

static void
Camera_ResetPathState(edict_t *self, qboolean clear_enemy)
{
	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	self->delay = 0;
	self->moveinfo.speed = 0;	/* current path speed */
	self->moveinfo.wait = 0;	/* pause at path corner until this time */
	self->goalentity = NULL;
	self->movetarget = NULL;

	if (clear_enemy)
	{
		self->enemy = NULL;
	}
}

static void
Camera_StartPath(edict_t *self, edict_t *start, float speed)
{
	self->movetarget = start;
	self->goalentity = start;

	if (start->speed > 0)
	{
		self->moveinfo.speed = start->speed;
	}
	else if (speed > 0)
	{
		self->moveinfo.speed = speed;
	}
	else
	{
		self->moveinfo.speed = CAMERA_PATH_DEFAULT_SPEED;
	}

	VectorCopy(start->s.origin, self->s.origin);
	VectorCopy(self->s.origin, self->s.old_origin);
	gi.linkentity(self);
}

/*
 * Moves the entity along its path_corner chain. Returns
 * false when the path is finished or was stopped by a
 * path_corner with wait -1.
 */
static qboolean
Camera_PathFollow(edict_t *self)
{
	float move_dist;
	int i;

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		return false;
	}

	if (!self->movetarget)
	{
		return false;
	}

	if (self->moveinfo.wait > level.time)
	{
		/* pausing at a path corner */
		return true;
	}

	if (self->moveinfo.speed <= 0)
	{
		self->moveinfo.speed = CAMERA_PATH_DEFAULT_SPEED;
	}

	move_dist = self->moveinfo.speed * FRAMETIME;

	for (i = 0; i < CAMERA_MAX_STEPS_PER_FRAME && self->movetarget &&
			move_dist > 0; i++)
	{
		edict_t *corner = self->movetarget;
		vec3_t dir;
		float dist;

		VectorSubtract(corner->s.origin, self->s.origin, dir);
		dist = VectorNormalize(dir);

		if (dist > move_dist)
		{
			VectorMA(self->s.origin, move_dist, dir, self->s.origin);
			break;
		}

		/* reached the path corner */
		VectorCopy(corner->s.origin, self->s.origin);
		move_dist -= dist;

		if (corner->speed > 0)
		{
			self->moveinfo.speed = corner->speed;
		}

		self->movetarget = Camera_FindPathCorner(self, corner->target, corner);
		self->goalentity = self->movetarget;

		if (corner->wait == CAMERA_PATH_STOPPED)
		{
			self->delay = CAMERA_PATH_STOPPED;
			break;
		}

		if (corner->wait > 0)
		{
			self->moveinfo.wait = level.time + corner->wait;
			break;
		}
	}

	VectorCopy(self->s.origin, self->s.old_origin);
	gi.linkentity(self);

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		return false;
	}

	return self->movetarget != NULL;
}

/* ========================================================================= */

static edict_t *
DummyBody_Find(edict_t *player)
{
	if (!player || !player->client)
	{
		return NULL;
	}

	if (!player->client->remote_view_body)
	{
		return NULL;
	}

	if (!player->client->remote_view_body->inuse)
	{
		player->client->remote_view_body = NULL;
		return NULL;
	}

	return player->client->remote_view_body;
}

/*
 * Puts the player back to where the dummy body stands.
 */
static void
DummyBody_RestorePlayer(edict_t *player, edict_t *body)
{
	int i;

	VectorCopy(body->s.origin, player->s.origin);
	VectorCopy(body->s.origin, player->s.old_origin);
	VectorCopy(body->s.angles, player->s.angles);
	VectorClear(player->velocity);

	if (player->client)
	{
		VectorCopy(body->move_angles, player->client->v_angle);
		VectorCopy(body->move_angles, player->client->ps.viewangles);

		for (i = 0; i < 3; i++)
		{
			player->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
					body->move_angles[i] -
					player->client->resp.cmd_angles[i]);
		}
	}
}

/*
 * Recovers orphaned dummy bodies. This normally never runs; after a
 * savegame load the client remote view state is dropped (it contains
 * function pointers), so the body restores the player and removes itself.
 */
void
DummyBody_Think(edict_t *self)
{
	edict_t *player = self->enemy;

	if (!player || !player->inuse || !player->client ||
		player->client->remote_view_body != self ||
		!player->client->remote_view_active)
	{
		if (player && player->inuse && player->client &&
			!player->client->remote_view_active)
		{
			DummyBody_RestorePlayer(player, self);
			player->svflags &= ~SVF_NOCLIENT;
			player->solid = SOLID_BBOX;
			player->flags &= ~FL_NOTARGET;
			player->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			gi.linkentity(player);
		}

		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

static void
DummyBody_Setup(edict_t *player)
{
	edict_t *body;

	if (!player || !player->client)
	{
		return;
	}

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
	body->clipmask = 0;
	body->takedamage = DAMAGE_NO;
	body->svflags &= ~SVF_NOCLIENT;
	VectorCopy(player->mins, body->mins);
	VectorCopy(player->maxs, body->maxs);

	/* visible snapshot of the player at its real
	   position (like oldplayer in player/chase.c) */
	body->s = player->s;
	body->s.number = body - g_edicts;
	body->rrs = player->rrs;

	/* remember the real view angles for restoring them later */
	VectorCopy(player->client->v_angle, body->move_angles);

	body->think = DummyBody_Think;
	body->nextthink = level.time + FRAMETIME;
	gi.linkentity(body);

	player->client->remote_view_active = true;
	player->svflags |= SVF_NOCLIENT;
	player->solid = SOLID_NOT;

	/* The player entity is moved to the view entity, so keep the
	   AI from noticing and firing at the camera position. In the
	   original mod the player entity stayed at the body position
	   and remained a regular target there. */
	player->client->remote_view_had_notarget =
		(player->flags & FL_NOTARGET) != 0;
	player->flags |= FL_NOTARGET;

	gi.linkentity(player);
}

static void
DummyBody_Teardown(edict_t *player, qboolean restore_position)
{
	edict_t *body;

	if (!player || !player->client)
	{
		return;
	}

	body = DummyBody_Find(player);

	if (body && restore_position)
	{
		DummyBody_RestorePlayer(player, body);
	}

	player->client->remote_view_body = NULL;
	player->client->remote_view_active = false;
	player->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	player->svflags &= ~SVF_NOCLIENT;
	player->solid = SOLID_BBOX;

	/* keep the notarget cheat if it was active before */
	if (!player->client->remote_view_had_notarget)
	{
		player->flags &= ~FL_NOTARGET;
	}

	player->client->remote_view_had_notarget = false;
	gi.linkentity(player);

	if (body)
	{
		G_FreeEdict(body);
	}
}

/* ========================================================================= */

static edict_t *
RemoteView_GetEntity(edict_t *ent)
{
	edict_t *viewent;

	if (!ent || !ent->client)
	{
		return NULL;
	}

	if (!ent->client->remote_view_active)
	{
		return NULL;
	}

	viewent = ent->client->remote_view_entity;

	if (!viewent || !viewent->inuse)
	{
		ent->client->remote_view_entity = NULL;
		RemoteView_End(ent);
		return NULL;
	}

	return viewent;
}

static void
RemoteView_ApplyClientState(edict_t *ent, edict_t *viewent)
{
	gclient_t *client;
	int i;

	if (!ent || !ent->client || !viewent)
	{
		return;
	}

	client = ent->client;

	/* The server takes the client view origin from the player
	   entity, so move the player itself to the view entity
	   (the same approach as UpdateChaseCam() in g_chase.c). */
	VectorCopy(viewent->s.origin, ent->s.origin);
	VectorCopy(viewent->s.origin, ent->s.old_origin);
	VectorClear(ent->velocity);

	VectorCopy(viewent->s.angles, client->ps.viewangles);
	VectorCopy(viewent->s.angles, client->v_angle);

	for (i = 0; i < 3; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
				viewent->s.angles[i] - client->resp.cmd_angles[i]);
	}

	client->ps.viewoffset[0] = (float)client->remote_view_state_1;
	client->ps.viewoffset[1] = (float)client->remote_view_state_2;
	client->ps.viewoffset[2] = client->remote_view_timer;
	ent->viewheight = 0;

	/* don't drown or burn at the camera position; pmove
	   recalculates the real waterlevel after the view ends */
	ent->waterlevel = 0;

	client->ps.pmove.pm_type = PM_FREEZE;
	client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);
}

static qboolean
RemoteView_ClientPreFrame(edict_t *ent)
{
	return RemoteView_GetEntity(ent) != NULL;
}

static qboolean
RemoteView_ClientPostFrame(edict_t *ent)
{
	edict_t *viewent;

	viewent = RemoteView_GetEntity(ent);

	if (!viewent)
	{
		return false;
	}

	RemoteView_ApplyClientState(ent, viewent);

	return true;
}

void
RemoteView_Begin(edict_t *ent, edict_t *viewent)
{
	(void)viewent;

	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->client->remote_view_active)
	{
		RemoteView_End(ent);
	}

	DummyBody_Setup(ent);
	ent->client->remote_view_cmd_hook = NULL;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.0f;
	ent->client->remote_view_saved_gunindex = ent->client->ps.gunindex;
	ent->client->ps.gunindex = 0;
}

static void
RemoteView_EndInternal(edict_t *ent, qboolean restore_position)
{
	if (!ent || !ent->client)
	{
		return;
	}

	DummyBody_Teardown(ent, restore_position);
	ent->client->remote_view_cmd_hook = NULL;
	ent->client->remote_view_entity = NULL;
	ent->client->remote_view_state_1 = 0;
	ent->client->remote_view_state_2 = 0;
	ent->client->remote_view_timer = 0.0f;
	ent->client->ps.gunindex = ent->client->remote_view_saved_gunindex;
	ent->client->remote_view_saved_gunindex = 0;
}

void
RemoteView_End(edict_t *ent)
{
	RemoteView_EndInternal(ent, true);
}

void
RemoteView_AttachController(edict_t *ent, edict_t *viewent,
		remote_view_cmd_func_t *cmd_hook)
{
	if (!ent || !ent->client || !viewent)
	{
		return;
	}

	if ((ent->client->remote_view_entity == viewent) &&
		ent->client->remote_view_active)
	{
		ent->client->remote_view_cmd_hook = cmd_hook;
		return;
	}

	if (ent->client->remote_view_entity || ent->client->remote_view_active)
	{
		RemoteView_End(ent);
	}

	ent->client->remote_view_entity = viewent;
	RemoteView_Begin(ent, viewent);
	ent->client->remote_view_cmd_hook = cmd_hook;
}

void
RemoteView_DetachController(edict_t *ent, edict_t *viewent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->client->remote_view_entity != viewent)
	{
		return;
	}

	RemoteView_End(ent);
	ent->client->remote_view_entity = NULL;
}

/* ========================================================================= */

static void
Camera_RemoteViewCmd(edict_t *ent, usercmd_t *ucmd)
{
	edict_t *camera;

	if (!ent || !ent->client || !ucmd)
	{
		return;
	}

	camera = ent->client->remote_view_entity;

	if (!camera || !camera->inuse)
	{
		return;
	}

	if (!(camera->spawnflags & 1))
	{
		return;
	}

	/* this camera locks the player controls (movement is
	   already skipped in ClientThink() while a remote view
	   is active, but keep the player from firing) */
	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->upmove = 0;
	ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_USE);
}

static void
Camera_AttachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
	{
		return;
	}

	RemoteView_AttachController(player, camera, Camera_RemoteViewCmd);
}

static void
Camera_DetachClient(edict_t *player, edict_t *camera)
{
	if (!player || !player->client)
	{
		return;
	}

	RemoteView_DetachController(player, camera);
}

static void
Camera_AttachAllClients(edict_t *camera)
{
	int i;

	for (i = 1; i <= maxclients->value; i++)
	{
		edict_t *player = g_edicts + i;

		if (!player->inuse || !player->client)
		{
			continue;
		}

		if ((player->health <= 0) || player->deadflag ||
			player->client->resp.spectator)
		{
			continue;
		}

		Camera_AttachClient(player, camera);
	}
}

static void
Camera_DetachAllClients(edict_t *camera)
{
	int i;

	for (i = 1; i <= maxclients->value; i++)
	{
		edict_t *player = g_edicts + i;

		if (!player->inuse || !player->client)
		{
			continue;
		}

		Camera_DetachClient(player, camera);
	}
}

static void
Camera_Deactivate(edict_t *self)
{
	Camera_DetachAllClients(self);
	Camera_ResetPathState(self, true);
	self->count = 0;
	self->timestamp = 0;
	self->nextthink = 0;
}

static void
Camera_Activate(edict_t *self)
{
	edict_t *path_start;

	Camera_ResetPathState(self, true);

	if (self->target)
	{
		self->enemy = G_PickTarget(self->target);

		if (!self->enemy)
		{
			gi.dprintf("%s: target %s not found\n",
					self->classname, self->target);
		}
	}

	path_start = Camera_FindPathCorner(self, self->pathtarget, NULL);

	if (path_start)
	{
		Camera_StartPath(self, path_start, self->speed);
	}

	Camera_AttachAllClients(self);

	self->count = 1;
	self->timestamp = level.time;
	self->nextthink = level.time + FRAMETIME;
}

void
Camera_Use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator /* unused */)
{
	if (self->count == 0)
	{
		Camera_Activate(self);
		return;
	}

	Camera_Deactivate(self);
}

void
Camera_Think(edict_t *self)
{
	if ((self->wait != CAMERA_PATH_STOPPED) &&
		(level.time > self->timestamp + self->wait))
	{
		Camera_Deactivate(self);
		return;
	}

	Camera_PathFollow(self);

	if (self->delay == CAMERA_PATH_STOPPED)
	{
		/* stopped by a path_corner with wait -1 */
		Camera_Deactivate(self);
		return;
	}

	if (self->enemy && !self->enemy->inuse)
	{
		self->enemy = NULL;
	}
	else if (self->enemy)
	{
		vec3_t target, dir, forward;

		/* smoothly track the target */
		VectorAdd(self->enemy->absmin, self->enemy->absmax, target);
		VectorScale(target, 0.5f, target);
		target[2] += self->enemy->viewheight;
		VectorSubtract(target, self->s.origin, dir);

		if (VectorLength(dir) > 0)
		{
			VectorNormalize(dir);
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, 0.9f, forward);
			VectorMA(forward, 0.1f, dir, forward);
			VectorNormalize(forward);
			vectoangles(forward, self->s.angles);
			gi.linkentity(self);
		}
	}

	self->nextthink = level.time + FRAMETIME;
}

/*
 * QUAKED misc_camera (.5 .5 .5) (-8 -8 -8) (8 8 8) NO_CONTROL
 * A security camera the player can look through. Activate it to
 * take over the view of all players, activate it again to stop.
 *
 * NO_CONTROL	locks the player controls while active
 *
 * "wait"	seconds before the camera gives the view back (default 3, -1 never)
 * "target"	entity the camera tracks while active
 * "pathtarget"	first path_corner of the camera fly path
 * "speed"	path speed in units per second (default 100)
 */
void
SP_misc_camera(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	if (!self->targetname)
	{
		gi.dprintf("misc_camera with no targetname at %s\n",
				vtos(self->s.origin));
		self->targetname = "unused";
	}

	if (!self->wait)
	{
		self->wait = CAMERA_DEFAULT_WAIT;
	}

	self->use = Camera_Use;
	self->think = Camera_Think;
	gi.linkentity(self);
}

void
Camera_ClientPreFrame(edict_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (level.intermissiontime &&
		(ent->client->remote_view_active || ent->client->remote_view_entity))
	{
		/* give the view back for the intermission; the player
		   was already moved to the intermission point, so don't
		   restore the pre-camera position */
		RemoteView_EndInternal(ent, false);
		return;
	}

	RemoteView_ClientPreFrame(ent);
}

void
Camera_ClientPostFrame(edict_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	RemoteView_ClientPostFrame(ent);
}

/* ========================================================================= */

static void
TriggerCamera_Fire(edict_t *self)
{
	edict_t *camera = NULL;
	qboolean found = false;

	if (self->nextthink)
	{
		return; /* still waiting to rearm */
	}

	if (self->message && self->activator && self->activator->client)
	{
		gi.centerprintf(self->activator, "%s", self->message);
		gi.sound(self->activator, CHAN_AUTO,
				self->noise_index ? self->noise_index :
				gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	if (self->target)
	{
		while ((camera = G_Find(camera, FOFS(targetname),
						self->target)) != NULL)
		{
			if (!camera->classname ||
				Q_stricmp(camera->classname, "misc_camera"))
			{
				continue;
			}

			found = true;

			if (self->wait)
			{
				camera->wait = self->wait;
			}

			if (self->pathtarget)
			{
				camera->target = self->pathtarget;
			}
			else
			{
				camera->enemy = self->activator;
			}

			if (camera->count == 0)
			{
				Camera_Use(camera, self, self->activator);
			}
			else
			{
				/* already running, restart the timeout */
				camera->timestamp = level.time;
			}

			break;
		}

		if (!found)
		{
			gi.dprintf("Illegal target for trigger_misc_camera\n");
		}
	}

	if (self->delay > 0)
	{
		self->think = multi_wait;
		self->nextthink = level.time + self->delay;
		return;
	}

	self->touch = NULL;
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
}

void
TriggerCamera_Use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator)
{
	self->activator = activator;
	TriggerCamera_Fire(self);
}

void
TriggerCamera_Touch(edict_t *self, edict_t *other,
		const cplane_t *plane /* unused */,
		const csurface_t *surf /* unused */)
{
	if (other->client)
	{
		if (self->spawnflags & 2)
		{
			return;
		}
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!(self->spawnflags & 1))
		{
			return;
		}
	}
	else
	{
		return;
	}

	if (!VectorCompare(self->movedir, vec3_origin))
	{
		vec3_t forward;

		AngleVectors(other->s.angles, forward, NULL, NULL);

		if (DotProduct(forward, self->movedir) < 0)
		{
			return;
		}
	}

	self->activator = other;
	TriggerCamera_Fire(self);
}

/*
 * QUAKED trigger_misc_camera (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
 * Activates a misc_camera when touched.
 *
 * "target"	targetname of the misc_camera
 * "pathtarget"	entity the camera should track (overrides the camera target)
 * "wait"	overrides the camera wait time
 * "delay"	seconds until the trigger can fire again (default: fire once)
 * "message"	text printed to the activator
 * "sounds"	1 secret, 2 talk, 3 trigger1 (default talk1)
 */
void
SP_trigger_misc_camera(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (self->sounds == 1)
	{
		self->noise_index = gi.soundindex("misc/secret.wav");
	}
	else if (self->sounds == 2)
	{
		self->noise_index = gi.soundindex("misc/talk.wav");
	}
	else if (self->sounds == 3)
	{
		self->noise_index = gi.soundindex("misc/trigger1.wav");
	}

	/* matches InitTrigger() in g_trigger.c (static there) */
	if (!VectorCompare(self->s.angles, vec3_origin))
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->movetype = MOVETYPE_NONE;
	gi.setmodel(self, self->model);
	self->svflags |= SVF_NOCLIENT;
	self->touch = TriggerCamera_Touch;

	if (self->spawnflags & 4)
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

/* ========================================================================= */

void
Camera_TargetUse(edict_t *self, edict_t *other /* unused */,
		edict_t *activator /* unused */)
{
	edict_t *path_start;

	if (self->count != 0)
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0;
		return;
	}

	Camera_ResetPathState(self, false);
	path_start = Camera_FindPathCorner(self, self->target, NULL);

	if (!path_start)
	{
		self->count = 0;
		self->nextthink = 0;
		return;
	}

	self->count = 1;
	Camera_StartPath(self, path_start, self->speed);
	self->nextthink = level.time + FRAMETIME;
}

void
Camera_TargetThink(edict_t *self)
{
	if (!Camera_PathFollow(self))
	{
		Camera_ResetPathState(self, false);
		self->count = 0;
		self->nextthink = 0;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

/*
 * QUAKED misc_camera_target (.5 .5 .5) (-8 -8 -8) (8 8 8)
 * An invisible marker a misc_camera can track. Activate it to move
 * it along its path_corner chain, activate it again to stop it.
 *
 * "target"	first path_corner of the fly path
 * "speed"	path speed in units per second (default 100)
 */
void
SP_misc_camera_target(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	if (!self->targetname)
	{
		gi.dprintf("misc_camera_target with no targetname at %s\n",
				vtos(self->s.origin));
		self->targetname = "unused";
	}

	self->count = 0;
	self->movetarget = NULL;
	self->use = Camera_TargetUse;
	self->think = Camera_TargetThink;
	gi.linkentity(self);
}
