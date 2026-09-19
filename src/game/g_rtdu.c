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

#define RTDU_PICKUP_COUNT		200
#define RTDU_HEALTH			60
#define RTDU_AMMO			200
#define RTDU_ATTACK_DAMAGE		10
#define RTDU_ATTACK_KICK		0
#define RTDU_ATTACK_HSPREAD		300
#define RTDU_ATTACK_VSPREAD		500
#define RTDU_EXPLOSION_DAMAGE		100
#define RTDU_EXPLOSION_RADIUS		100.0f
#define RTDU_REMOTE_VIEW_Z		15.0f
#define RTDU_TRIPOD_Z_OFFSET		40.0f
#define RTDU_FIRE_FRAME_FIRST		7
#define RTDU_FIRE_FRAME_LAST		15

static vec3_t rtdu_muzzle_offsets[] = {
	{20.0f, 0.0f, 8.0f},
	{20.0f, -5.0f, 15.0f},
	{20.0f, 5.0f, 15.0f},
	{20.0f, 0.0f, 8.0f},
	{20.0f, -5.0f, 15.0f},
	{20.0f, 5.0f, 15.0f},
	{20.0f, 0.0f, 8.0f},
	{20.0f, -5.0f, 15.0f},
	{20.0f, 5.0f, 15.0f}
};

static void RTDU_TurretAttack(edict_t *self);

/*
=================
RTDU_RemoveTurretThink
=================
*/
void RTDU_RemoveTurretThink(edict_t *self)
{
	edict_t	*owner;
	vec3_t	origin;

	if (!self)
		return;

	owner = self->owner;
	self->takedamage = DAMAGE_NO;

	if (owner)
		PlayerNoise(owner, self->s.origin, PNOISE_IMPACT);

	T_RadiusDamage(self, owner ? owner : self, RTDU_EXPLOSION_DAMAGE, self,
		RTDU_EXPLOSION_RADIUS, MOD_REMOTE_CANNON);

	VectorMA(self->s.origin, -0.02f, self->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	if (self->waterlevel)
	{
		if (self->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (self->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition(origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	if (self->target_ent && self->target_ent->inuse)
		G_FreeEdict(self->target_ent);

	G_FreeEdict(self);
}

/*
=================
RTDU_RemoteViewCmd
=================
*/
static void RTDU_RemoteViewCmd(edict_t *ent, usercmd_t *ucmd)
{
	int		i;

	if (!ent || !ent->client || !ucmd)
		return;

	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->upmove = 0;

	if (!ent->client->rtdu.turret)
		return;

	for (i = 0; i < 3; i++)
	{
		float angle;

		angle = SHORT2ANGLE(ucmd->angles[i]);
		if (i == PITCH)
			angle *= (1.0f / 3.0f);

		ent->client->rtdu.turret->s.angles[i] = angle;
		ucmd->angles[i] = 0;
	}

	if (ucmd->buttons & BUTTON_ATTACK)
	{
		RTDU_TurretAttack(ent->client->rtdu.turret);
		ucmd->buttons &= ~BUTTON_ATTACK;
	}
}

/*
=================
RTDU_SpawnTripod
=================
*/
static edict_t *RTDU_SpawnTripod(edict_t *turret)
{
	edict_t	*tripod;

	tripod = G_Spawn();
	tripod->solid = SOLID_NOT;
	tripod->movetype = MOVETYPE_FLY;
	tripod->clipmask = MASK_SHOT;
	tripod->classname = "RTDUTripod";
	tripod->s.modelindex = gi.modelindex("models/objects/rtdu/tripod.md2");
	VectorSet(tripod->mins, -100.0f, -100.0f, -100.0f);
	VectorSet(tripod->maxs, 100.0f, 100.0f, 100.0f);
	VectorCopy(turret->s.origin, tripod->s.origin);
	tripod->s.origin[2] += RTDU_TRIPOD_Z_OFFSET;
	gi.linkentity(tripod);

	turret->target_ent = tripod;

	return tripod;
}

/*
=================
RTDU_BeginRemoteView
=================
*/
static void RTDU_BeginRemoteView(edict_t *turret)
{
	edict_t	*owner;

	if (!turret || !turret->owner || !turret->owner->client)
		return;

	owner = turret->owner;
	RemoteView_Begin(owner, turret);
	owner->client->remote_view_cmd_hook = RTDU_RemoteViewCmd;
	owner->client->remote_view_state_1 = 0;
	owner->client->remote_view_state_2 = 0;
	owner->client->remote_view_timer = RTDU_REMOTE_VIEW_Z;
	turret->timestamp = level.time;
}

/*
=================
RTDU_EndRemoteView
=================
*/
static void RTDU_EndRemoteView(edict_t *turret)
{
	edict_t	*owner;

	if (!turret || !turret->owner || !turret->owner->client)
		return;

	owner = turret->owner;
	turret->timestamp = 0.0f;
	owner->client->remote_view_cmd_hook = NULL;
	RemoteView_End(owner);
}

/*
=================
RTDU_RemoveTurret
=================
*/
void RTDU_RemoveTurret(edict_t *self, edict_t *inflictor,
	edict_t *attacker, int damage, const vec3_t point)
{
	edict_t	*owner;

	(void)inflictor;
	(void)attacker;
	(void)damage;
	(void)point;

	if (!self)
		return;

	owner = self->owner;
	if (owner && owner->client && self->item)
		owner->client->pers.inventory[ITEM_INDEX(self->item)] = 0;

	if (owner && owner->client)
		RTDU_EndRemoteView(self);

	if (owner && owner->client)
		owner->client->rtdu.turret = NULL;

	self->think = RTDU_RemoveTurretThink;
	self->nextthink = level.time + FRAMETIME;
}

/*
=================
RTDU_TurretThink
=================
*/
void RTDU_TurretThink(edict_t *self)
{
	if (!self || !self->owner || !self->owner->inuse || !self->owner->client ||
		self->owner->client->rtdu.turret != self)
	{
		RTDU_RemoveTurret(self, self, self, 0, self->s.origin);
		return;
	}

	if (self->timestamp > level.time)
		self->timestamp = level.time;

	if (self->target_ent)
	{
		VectorCopy(self->s.origin, self->target_ent->s.origin);
		VectorCopy(self->s.angles, self->target_ent->s.angles);
		gi.linkentity(self->target_ent);
	}

	self->nextthink = level.time + FRAMETIME;
}

/*
=================
RTDU_TurretAttack
=================
*/
static void RTDU_TurretAttack(edict_t *self)
{
	edict_t	*owner;
	vec3_t	forward;
	vec3_t	right;
	vec3_t	start;
	int		offset_index;

	if (!self || !self->owner || !self->owner->client)
		return;

	owner = self->owner;
	if (self->count < 1)
	{
		RTDU_RemoveTurret(self, owner, owner, 0, self->s.origin);
		return;
	}

	self->count--;
	owner->client->pers.inventory[ITEM_INDEX(self->item)]--;

	if (self->s.frame < RTDU_FIRE_FRAME_FIRST || self->s.frame > RTDU_FIRE_FRAME_LAST)
		self->s.frame = RTDU_FIRE_FRAME_FIRST;

	offset_index = self->s.frame - RTDU_FIRE_FRAME_FIRST;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, rtdu_muzzle_offsets[offset_index], forward, right, start);
	fire_bullet(self, start, forward, RTDU_ATTACK_DAMAGE, RTDU_ATTACK_KICK,
		RTDU_ATTACK_HSPREAD, RTDU_ATTACK_VSPREAD, MOD_REMOTE_CANNON);

	self->s.frame++;
	if (self->s.frame > RTDU_FIRE_FRAME_LAST)
		self->s.frame = RTDU_FIRE_FRAME_FIRST;

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((short)(self - g_edicts));
	gi.WriteByte(MZ_MACHINEGUN);
	gi.multicast(start, MULTICAST_PVS);

	PlayerNoise(owner, start, PNOISE_WEAPON);
}

/*
=================
RTDU_InitTurret
=================
*/
static void RTDU_InitTurret(edict_t *self)
{
	VectorSet(self->mins, -12.0f, -12.0f, -14.0f);
	VectorSet(self->maxs, 12.0f, 12.0f, 13.0f);
	self->solid = SOLID_BBOX;
	self->clipmask = MASK_SHOT;
	self->movetype = MOVETYPE_TOSS;
	self->classname = "RTDU";
	self->s.modelindex = gi.modelindex("models/objects/rtdu/rtdu.md2");
	self->s.frame = 0;
	self->item = FindItem("RTDU");
	self->health = RTDU_HEALTH;
	self->count = RTDU_AMMO;
	self->timestamp = 0.0f;
	self->takedamage = DAMAGE_YES;
	self->think = RTDU_TurretThink;
	self->die = RTDU_RemoveTurret;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);

	RTDU_SpawnTripod(self);
}

/*
=================
RTDU_SpawnTurret
=================
*/
static void RTDU_SpawnTurret(edict_t *owner)
{
	edict_t	*turret;

	turret = G_Spawn();
	RTDU_InitTurret(turret);
	turret->owner = owner;
	VectorCopy(owner->s.origin, turret->s.origin);
	turret->s.angles[YAW] = owner->s.angles[YAW];

	owner->client->rtdu.turret = turret;
}

/*
=================
Pickup_RTDU
=================
*/
qboolean Pickup_RTDU(edict_t *ent, edict_t *other)
{
	int	index;

	if (!other->client)
		return false;

	index = ITEM_INDEX(ent->item);
	if (other->client->pers.inventory[index] > 0)
		return false;

	other->client->pers.inventory[index] = RTDU_PICKUP_COUNT;
	return true;
}

/*
=================
rtdu_use
=================
*/
void rtdu_use(edict_t *ent, const gitem_t *item)
{
	edict_t	*turret;

	(void)item;

	if (!ent || !ent->client)
		return;

	turret = ent->client->rtdu.turret;
	if (turret && Q_stricmp(turret->classname, "RTDU") == 0)
	{
		if (ent->client->remote_view_active)
			RTDU_EndRemoteView(turret);
		else
			RTDU_BeginRemoteView(turret);
		return;
	}

	RTDU_SpawnTurret(ent);
}

/*
=================
Drop_RTDU
=================
*/
void Drop_RTDU(edict_t *ent, const gitem_t *item)
{
	(void)ent;
	(void)item;
}
