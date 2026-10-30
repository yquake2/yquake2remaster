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
 * The Zaero visor item and the misc_securitycamera entity.
 *
 * Clean-room implementation written from the functional specification
 * in .agents/docs/zaero_visor_SPEC.md on top of the generic remote
 * view support in g_camera.c.
 */

#include "header/local.h"

#define VISOR_FULL_TIME			300	/* server frames (30 seconds) */
#define VISOR_STATIC_TIME		0.2f	/* white noise flash length */
#define VISOR_HUD_INTERVAL		1.0f
#define VISOR_RESPAWN_DELAY		30.0f
#define VISOR_BODY_HEALTH		100000

#define SECURITYCAMERA_FRAME_LAST	59
#define SECURITYCAMERA_VIEW_FORWARD	8.0f
#define SECURITYCAMERA_VIEW_DOWN	32.0f
#define SECURITYCAMERA_SWAY_DEGREES	15.0f
#define SECURITYCAMERA_SWAY_PERIOD	64
#define SECURITYCAMERA_FLASH_TIME	0.2f

void SecurityCamera_Think(edict_t *self);
void SecurityCamera_Use(edict_t *self, edict_t *other, edict_t *activator);
void SecurityCamera_Pain(edict_t *self, edict_t *other, float kick, int damage);
void SecurityCamera_Die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);
void VisorBody_Pain(edict_t *self, edict_t *other, float kick, int damage);
void VisorBody_Die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point);

/* ========================================================================= */
/* misc_securitycamera                                                       */

static qboolean
SecurityCamera_IsPowered(const edict_t *camera)
{
	return camera && camera->inuse && camera->count;
}

void
SecurityCamera_Use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator /* unused */)
{
	self->count = !self->count;
}

void
SecurityCamera_Think(edict_t *self)
{
	if (self->count)
	{
		/* powered cameras pan around */
		self->s.frame++;

		if (self->s.frame > SECURITYCAMERA_FRAME_LAST)
		{
			self->s.frame = 0;
		}
	}

	if (self->timestamp > level.time)
	{
		/* recently shot */
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_GREEN;
	}
	else
	{
		self->s.effects &= ~EF_COLOR_SHELL;
		self->s.renderfx &= ~RF_SHELL_GREEN;
	}

	self->nextthink = level.time + FRAMETIME;
}

void
SecurityCamera_Pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage /* unused */)
{
	self->timestamp = level.time + SECURITYCAMERA_FLASH_TIME;
	self->health = VISOR_BODY_HEALTH;
}

void
SecurityCamera_Die(edict_t *self, edict_t *inflictor /* unused */,
		edict_t *attacker /* unused */, int damage /* unused */,
		const vec3_t point /* unused */)
{
	/* cameras can not be destroyed */
	self->timestamp = level.time + SECURITYCAMERA_FLASH_TIME;
	self->health = VISOR_BODY_HEALTH;
	self->deadflag = DEAD_NO;
}

/*
 * QUAKED misc_securitycamera (.5 .5 .5) (-16 -16 -32) (16 16 0)
 * A security camera the visor can connect to.
 *
 * "message"	display name of the camera (required)
 * "mangle"	pitch yaw roll the camera view faces
 *
 * With a targetname the camera spawns powered off and
 * each trigger toggles its power.
 */
void
SP_misc_securitycamera(edict_t *self)
{
	vec3_t forward, up;

	if (!self->message)
	{
		gi.dprintf("misc_securitycamera without message at %s\n",
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->s.modelindex = gi.modelindex("models/objects/camera/tris.md2");
	VectorSet(self->mins, -16, -16, -32);
	VectorSet(self->maxs, 16, 16, 0);

	/* view direction comes from the "mangle" key
	   (stored in move_angles by the field parser) */
	if (VectorCompare(self->move_angles, vec3_origin))
	{
		VectorCopy(self->s.angles, self->move_angles);
	}

	/* the model only yaws */
	VectorSet(self->s.angles, 0, self->move_angles[YAW], 0);

	/* the view point sits in front of and below the model origin */
	AngleVectors(self->s.angles, forward, NULL, up);
	VectorMA(self->s.origin, SECURITYCAMERA_VIEW_FORWARD, forward,
			self->move_origin);
	VectorMA(self->move_origin, -SECURITYCAMERA_VIEW_DOWN, up,
			self->move_origin);

	if (self->targetname)
	{
		self->use = SecurityCamera_Use;
		self->count = 0; /* starts powered off */
	}
	else
	{
		self->count = 1; /* always powered */
	}

	self->health = VISOR_BODY_HEALTH;
	self->max_health = self->health;
	self->takedamage = DAMAGE_YES;
	self->pain = SecurityCamera_Pain;
	self->die = SecurityCamera_Die;
	self->think = SecurityCamera_Think;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}

/* ========================================================================= */
/* visor                                                                     */

qboolean
Visor_IsViewing(const edict_t *ent)
{
	return ent && ent->client && ent->client->visor_active &&
		ent->client->remote_view_active;
}

/*
 * Searches the next powered camera in cyclic map
 * order. With current == NULL returns the first one.
 */
static edict_t *
Visor_NextCamera(edict_t *current)
{
	edict_t *e;

	for (e = G_Find(current, FOFS(classname), "misc_securitycamera");
		e; e = G_Find(e, FOFS(classname), "misc_securitycamera"))
	{
		if (SecurityCamera_IsPowered(e))
		{
			return e;
		}
	}

	/* wrap around */
	for (e = G_Find(NULL, FOFS(classname), "misc_securitycamera");
		e; e = G_Find(e, FOFS(classname), "misc_securitycamera"))
	{
		if (SecurityCamera_IsPowered(e))
		{
			return e;
		}

		if (e == current)
		{
			break;
		}
	}

	return NULL;
}

static void
Visor_RemoteViewCmd(edict_t *ent, usercmd_t *ucmd)
{
	if (!ent || !ent->client || !ucmd)
	{
		return;
	}

	/* the player is frozen and can not fire */
	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->upmove = 0;
	ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_USE);
}

static void
Visor_ViewTransform(edict_t *ent /* unused */, edict_t *viewent,
		vec3_t view_origin, vec3_t view_angles)
{
	float sway;

	if (!viewent)
	{
		return;
	}

	/* view from the camera eye point, swaying from side to side */
	sway = sin(((float)(level.framenum % SECURITYCAMERA_SWAY_PERIOD) /
			(float)SECURITYCAMERA_SWAY_PERIOD) * 2 * M_PI);

	VectorCopy(viewent->move_origin, view_origin);
	VectorCopy(viewent->move_angles, view_angles);
	view_angles[YAW] += sway * SECURITYCAMERA_SWAY_DEGREES;
}

void
VisorBody_Pain(edict_t *self, edict_t *other, float kick /* unused */,
		int damage)
{
	edict_t *player = self->enemy;

	self->health = VISOR_BODY_HEALTH;

	if (!player || !player->inuse || !player->client)
	{
		return;
	}

	/* pass the damage on to the viewing player */
	T_Damage(player, other ? other : self, other ? other : self,
			vec3_origin, player->s.origin, vec3_origin, damage, 0,
			DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
}

void
VisorBody_Die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, const vec3_t point /* unused */)
{
	(void)inflictor;

	self->deadflag = DEAD_NO;
	VisorBody_Pain(self, attacker, 0, damage);
}

static void
Visor_SendHud(edict_t *ent, qboolean reliable)
{
	char layout[192];
	const edict_t *camera = ent->client->remote_view_entity;

	if (!camera || !camera->message)
	{
		return;
	}

	Com_sprintf(layout, sizeof(layout),
			"xv 26 yb -75 string2 \"Tracking %s\" "
			"xv 200 yb -75 string \"Time: %d sec\"",
			camera->message,
			(ent->client->pers.visor_time + 9) / 10);

	gi.WriteByte(svc_layout);
	gi.WriteString(layout);
	gi.unicast(ent, reliable);

	ent->client->visor_hud_update = level.time + VISOR_HUD_INTERVAL;
}

/*
 * Connects the player to a camera (initial connect and switching).
 */
static void
Visor_SetCamera(edict_t *ent, edict_t *camera)
{
	edict_t *body;

	RemoteView_AttachController(ent, camera, Visor_RemoteViewCmd);

	if (!ent->client->remote_view_active)
	{
		return;
	}

	ent->client->remote_view_view_hook = Visor_ViewTransform;

	/* the body left behind stays tangible: the player
	   remains vulnerable while using the visor */
	body = ent->client->remote_view_body;

	if (body && body->inuse)
	{
		body->solid = SOLID_BBOX;
		body->clipmask = MASK_PLAYERSOLID;
		body->takedamage = DAMAGE_AIM;
		body->health = VISOR_BODY_HEALTH;
		body->max_health = body->health;
		body->pain = VisorBody_Pain;
		body->die = VisorBody_Die;
		gi.linkentity(body);
	}

	ent->client->visor_active = true;
	ent->client->visor_prev_health = ent->health;
	ent->client->visor_static_until = level.time + VISOR_STATIC_TIME;

	gi.sound(ent, CHAN_AUTO, gi.soundindex("items/visor/act.wav"),
			1, ATTN_NORM, 0);
	Visor_SendHud(ent, true);
}

/*
 * Restores the default field of view after the visor forced it to 90.
 */
static void
Visor_RestoreFov(edict_t *ent)
{
	gclient_t *client = ent->client;

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
		return;
	}

	client->ps.fov = (int)strtol(Info_ValueForKey(client->pers.userinfo,
				"fov"), (char **)NULL, 10);

	if (client->ps.fov < 1)
	{
		client->ps.fov = 90;
	}
	else if (client->ps.fov > 160)
	{
		client->ps.fov = 160;
	}
}

void
Visor_Stop(edict_t *ent)
{
	if (!ent || !ent->client || !ent->client->visor_active)
	{
		return;
	}

	ent->client->visor_active = false;
	ent->client->visor_static_until = 0;
	ent->client->visor_hud_update = 0;

	if (ent->client->remote_view_entity)
	{
		RemoteView_DetachController(ent, ent->client->remote_view_entity);
	}
	else if (ent->client->remote_view_active)
	{
		RemoteView_End(ent);
	}

	Visor_RestoreFov(ent);
	gi.sound(ent, CHAN_AUTO, gi.soundindex("items/visor/deact.wav"),
			1, ATTN_NORM, 0);
}

/*
 * Per-frame visor logic, called at the end of ClientEndServerFrame().
 */
void
Visor_ClientFrame(edict_t *ent)
{
	gclient_t *client;

	if (!ent || !ent->client)
	{
		return;
	}

	client = ent->client;

	if (!client->visor_active)
	{
		return;
	}

	if (!client->remote_view_active)
	{
		/* the remote view ended behind our back
		   (death, camera removed, ...) */
		Visor_Stop(ent);
		return;
	}

	/* taking damage interrupts the visor */
	if (ent->health < client->visor_prev_health)
	{
		Visor_Stop(ent);
		return;
	}

	client->visor_prev_health = ent->health;

	/* consume view time; an empty budget consumes the visor */
	client->pers.visor_time--;

	if (client->pers.visor_time <= 0)
	{
		client->pers.visor_time = 0;
		Visor_Stop(ent);
		client->pers.inventory[ITEM_INDEX(FindItem("Visor"))] = 0;
		ValidateSelectedItem(client);
		return;
	}

	client->ps.fov = 90;

	if (client->visor_static_until > level.time)
	{
		/* white noise flash right after connecting */
		client->ps.blend[0] = 1;
		client->ps.blend[1] = 1;
		client->ps.blend[2] = 1;
		client->ps.blend[3] = 1;
	}

	if (level.time >= client->visor_hud_update)
	{
		Visor_SendHud(ent, false);
	}
}

/* ========================================================================= */
/* item callbacks                                                            */

qboolean
Pickup_Visor(edict_t *ent, edict_t *other)
{
	int index;

	if (!ent || !other || !other->client)
	{
		return false;
	}

	index = ITEM_INDEX(ent->item);

	if ((other->client->pers.inventory[index] > 0) &&
		(other->client->pers.visor_time >= VISOR_FULL_TIME))
	{
		return false;
	}

	other->client->pers.inventory[index] = 1;

	if (ent->spawnflags & DROPPED_ITEM)
	{
		/* a dropped visor carries its remaining view time */
		other->client->pers.visor_time += ent->count;
	}
	else
	{
		other->client->pers.visor_time = VISOR_FULL_TIME;

		if (deathmatch->value)
		{
			SetRespawn(ent, VISOR_RESPAWN_DELAY);
		}
	}

	return true;
}

void
Use_Visor(edict_t *ent, const gitem_t *item /* unused */)
{
	edict_t *camera;

	if (!ent || !ent->client)
	{
		return;
	}

	if (!Visor_IsViewing(ent))
	{
		if (ent->client->pers.visor_time <= 0)
		{
			gi.cprintf(ent, PRINT_HIGH, "No time left for visor\n");
			return;
		}

		camera = Visor_NextCamera(NULL);

		if (!camera)
		{
			gi.cprintf(ent, PRINT_HIGH, "No cameras are available\n");
			return;
		}

		Visor_SetCamera(ent, camera);
		return;
	}

	/* already viewing: switch to the next camera */
	camera = Visor_NextCamera(ent->client->remote_view_entity);

	if (camera && (camera != ent->client->remote_view_entity))
	{
		Visor_SetCamera(ent, camera);
	}
}

void
Drop_Visor(edict_t *ent, const gitem_t *item)
{
	edict_t *dropped;

	if (!ent || !ent->client || !item)
	{
		return;
	}

	if (Visor_IsViewing(ent))
	{
		Visor_Stop(ent);
	}

	dropped = Drop_Item(ent, item);

	if (dropped)
	{
		/* the dropped visor carries the remaining view time */
		dropped->count = ent->client->pers.visor_time;
	}

	ent->client->pers.visor_time = 0;
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
	ValidateSelectedItem(ent->client);
}
