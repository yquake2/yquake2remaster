#include "g_local.h"

constexpr int32_t RTDU_PICKUP_COUNT = 200;
constexpr int32_t RTDU_HEALTH = 60;
constexpr int32_t RTDU_AMMO = 200;
constexpr int32_t RTDU_ATTACK_DAMAGE = 10;
constexpr int32_t RTDU_ATTACK_KICK = 0;
constexpr int32_t RTDU_ATTACK_HSPREAD = 300;
constexpr int32_t RTDU_ATTACK_VSPREAD = 500;
constexpr int32_t RTDU_EXPLOSION_DAMAGE = 100;
constexpr float RTDU_EXPLOSION_RADIUS = 100.0f;
constexpr float RTDU_REMOTE_VIEW_Z = 15.0f;
constexpr float RTDU_TRIPOD_Z_OFFSET = 40.0f;
constexpr int32_t RTDU_FIRE_FRAME_FIRST = 7;
constexpr int32_t RTDU_FIRE_FRAME_LAST = 15;

static const vec3_t rtdu_muzzle_offsets[] = {
	{ 20.0f, 0.0f, 8.0f },
	{ 20.0f, -5.0f, 15.0f },
	{ 20.0f, 5.0f, 15.0f },
	{ 20.0f, 0.0f, 8.0f },
	{ 20.0f, -5.0f, 15.0f },
	{ 20.0f, 5.0f, 15.0f },
	{ 20.0f, 0.0f, 8.0f },
	{ 20.0f, -5.0f, 15.0f },
	{ 20.0f, 5.0f, 15.0f }
};

static void RTDU_TurretAttack(edict_t *self);

THINK(RTDU_RemoveTurretThink) (edict_t *self) -> void
{
	edict_t *owner;
	vec3_t origin;

	if (!self)
		return;

	owner = self->owner;
	self->takedamage = false;

	if (owner)
		PlayerNoise(owner, self->s.origin, PNOISE_IMPACT);

	T_RadiusDamage(self, owner ? owner : self, RTDU_EXPLOSION_DAMAGE, self, RTDU_EXPLOSION_RADIUS, DAMAGE_NONE, MOD_REMOTE_CANNON);

	origin = self->s.origin + (self->velocity * -0.02f);

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
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	if (self->target_ent && self->target_ent->inuse)
		G_FreeEdict(self->target_ent);

	G_FreeEdict(self);
}

REMOTE_VIEW_CMD(RTDU_RemoteViewCmd) (edict_t *ent, usercmd_t *ucmd) -> void
{
	edict_t *turret;

	if (!ent || !ent->client || !ucmd)
		return;

	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->buttons &= ~(BUTTON_JUMP | BUTTON_CROUCH);

	turret = ent->client->rtdu_turret;
	if (!turret || !turret->inuse)
	{
		ent->client->rtdu_turret = nullptr;
		return;
	}

	// Retail used SHORT2ANGLE(cmd->angles[]). In rerelease the command angles
	// are already float view angles, so applying delta_angles again over-rotates
	// and effectively locks turret aim. Use the incoming command angles directly.
	if (ucmd->angles[PITCH] > 180.0f)
		turret->s.angles[PITCH] = (-360.0f + ucmd->angles[PITCH]) / 3.0f;
	else
		turret->s.angles[PITCH] = ucmd->angles[PITCH] / 3.0f;

	turret->s.angles[YAW] = anglemod(ucmd->angles[YAW]);
	turret->s.angles[ROLL] = 0.0f;
	ucmd->angles = {};

	if (ucmd->buttons & BUTTON_ATTACK)
	{
		RTDU_TurretAttack(turret);
		ucmd->buttons &= ~BUTTON_ATTACK;
	}
}

static edict_t *RTDU_SpawnTripod(edict_t *turret)
{
	edict_t *tripod;

	tripod = G_Spawn();
	tripod->solid = SOLID_NOT;
	tripod->movetype = MOVETYPE_FLY;
	tripod->clipmask = MASK_SHOT;
	tripod->classname = "RTDUTripod";
	tripod->s.modelindex = gi.modelindex("models/objects/rtdu/tripod.md2");
	tripod->mins = { -100.0f, -100.0f, -100.0f };
	tripod->maxs = { 100.0f, 100.0f, 100.0f };
	tripod->s.origin = turret->s.origin;
	tripod->s.origin[2] += RTDU_TRIPOD_Z_OFFSET;
	gi.linkentity(tripod);

	turret->target_ent = tripod;

	return tripod;
}

static void RTDU_BeginRemoteView(edict_t *turret)
{
	edict_t *owner;

	if (!turret || !turret->owner || !turret->owner->client)
		return;

	owner = turret->owner;
	if (owner->client->remote_view_entity)
		RemoteView_DetachController(owner, owner->client->remote_view_entity);

	RemoteView_AttachController(owner, turret, RTDU_RemoteViewCmd);
	owner->client->remote_view_state_1 = 0;
	owner->client->remote_view_state_2 = 0;
	owner->client->remote_view_timer = RTDU_REMOTE_VIEW_Z;
	turret->timestamp = level.time;
}

static void RTDU_EndRemoteView(edict_t *turret)
{
	edict_t *owner;

	if (!turret || !turret->owner || !turret->owner->client)
		return;

	owner = turret->owner;
	turret->timestamp = 0_ms;
	RemoteView_DetachController(owner, turret);
}

DIE(RTDU_RemoveTurret) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	edict_t *owner;

	(void) inflictor;
	(void) attacker;
	(void) damage;
	(void) point;
	(void) mod;

	if (!self)
		return;

	owner = self->owner;
	if (owner && owner->client && self->item)
		owner->client->pers.inventory[self->item->id] = 0;

	if (owner && owner->client)
	{
		RTDU_EndRemoteView(self);
		if (owner->client->rtdu_turret == self)
			owner->client->rtdu_turret = nullptr;
	}

	self->think = RTDU_RemoveTurretThink;
	self->nextthink = level.time + FRAME_TIME_S;
}

THINK(RTDU_TurretThink) (edict_t *self) -> void
{
	if (!self || !self->owner || !self->owner->inuse || !self->owner->client || self->owner->client->rtdu_turret != self)
	{
		RTDU_RemoveTurret(self, self, self, 0, self ? self->s.origin : vec3_origin, MOD_UNKNOWN);
		return;
	}

	if (self->timestamp > level.time)
		self->timestamp = level.time;

	if (self->target_ent)
	{
		self->target_ent->s.origin = self->s.origin;
		self->target_ent->s.angles = self->s.angles;
		gi.linkentity(self->target_ent);
	}

	self->nextthink = level.time + FRAME_TIME_S;
}

static void RTDU_TurretAttack(edict_t *self)
{
	edict_t *owner;
	vec3_t forward;
	vec3_t right;
	vec3_t start;
	int32_t offset_index;

	if (!self || !self->owner || !self->owner->client)
		return;

	owner = self->owner;
	if (self->count < 1)
	{
		RTDU_RemoveTurret(self, owner, owner, 0, self->s.origin, MOD_UNKNOWN);
		return;
	}

	--self->count;
	if (self->item)
		--owner->client->pers.inventory[self->item->id];

	if (self->s.frame < RTDU_FIRE_FRAME_FIRST || self->s.frame > RTDU_FIRE_FRAME_LAST)
		self->s.frame = RTDU_FIRE_FRAME_FIRST;

	offset_index = self->s.frame - RTDU_FIRE_FRAME_FIRST;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = G_ProjectSource(self->s.origin, rtdu_muzzle_offsets[offset_index], forward, right);

	fire_bullet(self, start, forward, RTDU_ATTACK_DAMAGE, RTDU_ATTACK_KICK, RTDU_ATTACK_HSPREAD, RTDU_ATTACK_VSPREAD, MOD_REMOTE_CANNON);

	++self->s.frame;
	if (self->s.frame > RTDU_FIRE_FRAME_LAST)
		self->s.frame = RTDU_FIRE_FRAME_FIRST;

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(self);
	gi.WriteByte(MZ_MACHINEGUN);
	gi.multicast(start, MULTICAST_PVS, false);

	PlayerNoise(owner, start, PNOISE_WEAPON);
}

static void RTDU_InitTurret(edict_t *self)
{
	self->mins = { -12.0f, -12.0f, -14.0f };
	self->maxs = { 12.0f, 12.0f, 13.0f };
	self->solid = SOLID_BBOX;
	self->clipmask = MASK_SHOT;
	self->movetype = MOVETYPE_TOSS;
	self->classname = "RTDU";
	self->s.modelindex = gi.modelindex("models/objects/rtdu/rtdu.md2");
	self->s.frame = 0;
	self->item = FindItem("RTDU");
	self->health = RTDU_HEALTH;
	self->count = RTDU_AMMO;
	self->timestamp = 0_ms;
	self->takedamage = true;
	self->think = RTDU_TurretThink;
	self->die = RTDU_RemoveTurret;
	self->nextthink = level.time + FRAME_TIME_S;
	gi.linkentity(self);

	RTDU_SpawnTripod(self);
}

static void RTDU_SpawnTurret(edict_t *owner)
{
	edict_t *turret;

	turret = G_Spawn();
	turret->owner = owner;
	turret->s.origin = owner->s.origin;
	turret->s.angles[YAW] = owner->s.angles[YAW];
	RTDU_InitTurret(turret);

	owner->client->rtdu_turret = turret;
}

bool Pickup_RTDU(edict_t *ent, edict_t *other)
{
	int32_t index;

	if (!other->client)
		return false;

	index = ent->item->id;
	if (other->client->pers.inventory[index] > 0)
		return false;

	other->client->pers.inventory[index] = RTDU_PICKUP_COUNT;
	return true;
}

void rtdu_use(edict_t *ent, gitem_t *item)
{
	edict_t *turret;

	(void) item;

	if (!ent || !ent->client)
		return;

	turret = ent->client->rtdu_turret;
	if (turret && !turret->inuse)
	{
		ent->client->rtdu_turret = nullptr;
		turret = nullptr;
	}

	if (turret && turret->classname && !Q_strcasecmp(turret->classname, "RTDU"))
	{
		if (ent->client->remote_view_active && ent->client->remote_view_entity == turret)
			RTDU_EndRemoteView(turret);
		else
			RTDU_BeginRemoteView(turret);
		return;
	}

	RTDU_SpawnTurret(ent);
}

void Drop_RTDU(edict_t *ent, gitem_t *item)
{
	(void) ent;
	(void) item;
}
