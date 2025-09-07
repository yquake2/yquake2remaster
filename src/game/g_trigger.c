/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Trigger.
 *
 * =======================================================================
 */

#include "header/local.h"

#define TRIGGER_MONSTER 0x01
#define TRIGGER_NOT_PLAYER 0x02
#define TRIGGER_TRIGGERED 0x04
#define TRIGGER_TOGGLE 0x08

#define PUSH_ONCE 0x01
#define PUSH_START_OFF 0x02
#define PUSH_SILENT 0x04

static int windsound;

void trigger_push_active(edict_t *self);
void hurt_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */);

static void
InitTrigger(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!VectorCompare(self->s.angles, vec3_origin))
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel(self, self->model);
	self->svflags = SVF_NOCLIENT;
}

/*
 * The wait time has passed, so
 * set back up for another activation
 */
void
multi_wait(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->nextthink = 0;
}

/*
 * The trigger was just activated
 * ent->activator should be set to
 * the activator so it can be held
 * through a delay so wait for the
 * delay time before firing
 */
void
multi_trigger(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (ent->nextthink)
	{
		return; /* already been triggered */
	}

	G_UseTargets(ent, ent->activator);

	if (ent->wait > 0)
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + ent->wait;
	}
	else
	{
		/* we can't just remove (self) here,
		   because this is a touch function
		   called while looping through area
		   links... */
		ent->touch = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEdict;
	}
}

void
Use_Multi(edict_t *ent, edict_t *other /* unused */, edict_t *activator)
{
	if (!ent || !activator)
	{
		return;
	}

	if (ent->spawnflags & TRIGGER_TOGGLE)
	{
		if (ent->solid == SOLID_TRIGGER)
		{
			ent->solid = SOLID_NOT;
		}
		else
		{
			ent->solid = SOLID_TRIGGER;
		}

		gi.linkentity(ent);
	}
	else
	{
		ent->activator = activator;
		multi_trigger(ent);
	}
}

void
Touch_Multi(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

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

		if (_DotProduct(forward, self->movedir) < 0)
		{
			return;
		}
	}

	self->activator = other;
	multi_trigger(self);
}

/*
 * QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED TOGGLE
 * Variable sized repeatable trigger.  Must be targeted at one or more
 * entities. If "delay" is set, the trigger waits some time after
 * activating before firing.
 *
 * "wait" : Seconds between triggerings. (.2 default)
 *
 * TOGGLE - using this trigger will activate/deactivate it. trigger will begin inactive.
 *
 * sounds
 * 1)	secret
 * 2)	beep beep
 * 3)	large switch
 * 4)
 *
 * set "message" to text string
 */
void
trigger_enable(edict_t *self, edict_t *other /* unused */,
		edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	gi.linkentity(self);
}

void
SP_trigger_multiple(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (ent->sounds == 1)
	{
		ent->noise_index = gi.soundindex("misc/secret.wav");
	}
	else if (ent->sounds == 2)
	{
		ent->noise_index = gi.soundindex("misc/talk.wav");
	}
	else if (ent->sounds == 3)
	{
		ent->noise_index = gi.soundindex("misc/trigger1.wav");
	}

	if (!ent->wait)
	{
		ent->wait = 0.2;
	}

	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->svflags |= SVF_NOCLIENT;

	if (ent->spawnflags & (TRIGGER_TRIGGERED | TRIGGER_TOGGLE))
	{
		ent->solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	if (!VectorCompare(ent->s.angles, vec3_origin))
	{
		G_SetMovedir(ent->s.angles, ent->movedir);
	}

	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}

/*
 * QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
 *
 * Triggers once, then removes itself.
 *
 * You must set the key "target" to the name of another
 * object in the level that has a matching "targetname".
 *
 * If TRIGGERED, this trigger must be triggered before it is live.
 *
 * sounds
 *  1) secret
 *  2) beep beep
 *  3) large switch
 *
 * "message" string to be displayed when triggered
 */

void
SP_trigger_once(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	/* make old maps work because I
	   messed up on flag assignments here
	   triggered was on bit 1 when it
	   should have been on bit 4 */
	if (ent->spawnflags & 1)
	{
		vec3_t v;

		VectorMA(ent->mins, 0.5, ent->size, v);
		ent->spawnflags &= ~1;
		ent->spawnflags |= 4;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", ent->classname, vtos(v));
	}

	ent->wait = -1;
	SP_trigger_multiple(ent);
}

/*
 * QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
 * This fixed size trigger cannot be touched,
 * it can only be fired by other events.
 */
void
trigger_relay_use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	G_UseTargets(self, activator);
}

void
SP_trigger_relay(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->use = trigger_relay_use;
}

/*
 * ==============================================================================
 *
 * trigger_key
 *
 * ==============================================================================
 */

/*
 * QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
 * A relay trigger that only fires it's targets if player
 * has the proper key. Use "item" to specify the required key,
 * for example "key_data_cd"
 */
void
trigger_key_use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator)
{
	int index;

	if (!self || !activator)
	{
		return;
	}

	if (!self->item)
	{
		return;
	}

	if (!activator->client)
	{
		return;
	}

	index = ITEM_INDEX(self->item);

	if (!activator->client->pers.inventory[index])
	{
		if (level.time < self->touch_debounce_time)
		{
			return;
		}

		self->touch_debounce_time = level.time + 5.0;
		gi.centerprintf(activator, "You need the %s", self->item->pickup_name);
		gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"), 1, ATTN_NORM, 0);
		return;
	}

	gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NORM, 0);

	if (coop->value)
	{
		int player;
		edict_t *ent;

		if (strcmp(self->item->classname, "key_power_cube") == 0)
		{
			int cube;

			for (cube = 0; cube < 8; cube++)
			{
				if (activator->client->pers.power_cubes & (1 << cube))
				{
					break;
				}
			}

			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];

				if (!ent->inuse)
				{
					continue;
				}

				if (!ent->client)
				{
					continue;
				}

				if (ent->client->pers.power_cubes & (1 << cube))
				{
					ent->client->pers.inventory[index]--;
					ent->client->pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else
		{
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];

				if (!ent->inuse)
				{
					continue;
				}

				if (!ent->client)
				{
					continue;
				}

				ent->client->pers.inventory[index] = 0;
			}
		}
	}
	else
	{
		activator->client->pers.inventory[index]--;
	}

	G_UseTargets(self, activator);

	self->use = NULL;
}

void
SP_trigger_key(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!st.item)
	{
		gi.dprintf("no key item for trigger_key at %s\n", vtos(self->s.origin));
		return;
	}

	self->item = FindItemByClassname(st.item);

	if (!self->item)
	{
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item,
				vtos(self->s.origin));
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s at %s has no target\n", self->classname,
				vtos(self->s.origin));
		return;
	}

	gi.soundindex("misc/keytry.wav");
	gi.soundindex("misc/keyuse.wav");

	self->use = trigger_key_use;
}

/*
 * ==============================================================================
 *
 * trigger_counter
 *
 * ==============================================================================
 */

/*
 * QUAKED trigger_counter (.5 .5 .5) ? nomessage
 *
 * Acts as an intermediary for an action that takes multiple inputs.
 *
 * If nomessage is not set, it will print "1 more.. " etc when
 * triggered and "sequence complete" when finished.
 *
 * After the counter has been triggered "count" times (default 2),
 * it will fire all of it's targets and remove itself.
 */

void
trigger_counter_use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	if (self->count == 0)
	{
		return;
	}

	self->count--;

	if (self->count)
	{
		if (!(self->spawnflags & 1))
		{
			gi.centerprintf(activator, "%i more to go...", self->count);
			gi.sound(activator, CHAN_AUTO, gi.soundindex(
						"misc/talk1.wav"), 1, ATTN_NORM, 0);
		}

		return;
	}

	if (!(self->spawnflags & 1))
	{
		gi.centerprintf(activator, "Sequence completed!");
		gi.sound(activator, CHAN_AUTO, gi.soundindex(
					"misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	self->activator = activator;
	multi_trigger(self);
}

void
SP_trigger_counter(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->wait = -1;

	if (!self->count)
	{
		self->count = 2;
	}

	self->use = trigger_counter_use;
}

/*
 * ==============================================================================
 *
 * trigger_always
 *
 * ==============================================================================
 */

/*
 * QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
 *
 * This trigger will always fire. It is activated by the world.
 */
void
SP_trigger_always(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	/* we must have some delay to make
	   sure our use targets are present */
	if (ent->delay < 0.2)
	{
		ent->delay = 0.2;
	}

	G_UseTargets(ent, ent);
}

/*
 * ==============================================================================
 *
 * trigger_push
 *
 * ==============================================================================
 */

void
trigger_push_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	if (strcmp(other->classname, "grenade") == 0)
	{
		VectorScale(self->movedir, self->speed * 10, other->velocity);
	}
	else if (other->health > 0)
	{
		VectorScale(self->movedir, self->speed * 10, other->velocity);

		if (other->client)
		{
			/* don't take falling damage
			   immediately from this */
			VectorCopy(other->velocity, other->client->oldvelocity);

			if (!(self->spawnflags & PUSH_SILENT) &&
				(other->fly_sound_debounce_time < level.time))
			{
				other->fly_sound_debounce_time = level.time + 1.5;
				gi.sound(other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}

	if (self->spawnflags & PUSH_ONCE)
	{
		G_FreeEdict(self);
	}
}

/*
 * QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
 * Pushes the player
 *
 * "speed"		defaults to 1000
 */
void
trigger_effect(edict_t *self)
{
	vec3_t origin;
	vec3_t size;
	int i;

	if (!self)
	{
		return;
	}

	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);

	for (i = 0; i < 10; i++)
	{
		origin[2] += (self->speed * 0.01) * (i + random());
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_TUNNEL_SPARKS);
		gi.WriteByte(1);
		gi.WritePosition(origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(0x74 + (rand() & 7));
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}
}

void
trigger_push_inactive(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->delay > level.time)
	{
		self->nextthink = level.time + 0.1;
	}
	else
	{
		self->touch = trigger_push_touch;
		self->think = trigger_push_active;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;
	}
}

void
trigger_push_active(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->delay > level.time)
	{
		self->nextthink = level.time + 0.1;
		trigger_effect(self);
	}
	else
	{
		self->touch = NULL;
		self->think = trigger_push_inactive;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;
	}
}

void
trigger_push_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_TRIGGER;
	}
	else
	{
		self->solid = SOLID_NOT;
	}

	gi.linkentity(self);
}

/*
 * QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE START_OFF SILENT
 * Pushes the player
 * "speed"		defaults to 1000
 *
 * If targeted, it will toggle on and off when used.
 *
 * START_OFF - toggled trigger_push begins in off setting
 * SILENT - doesn't make wind noise
 */
void
SP_trigger_push(edict_t *self)
{
	if (!self)
	{
		return;
	}

	InitTrigger(self);
	windsound = gi.soundindex("misc/windfly.wav");
	self->touch = trigger_push_touch;

	if (!self->speed)
	{
		self->speed = 1000;
	}

	if (self->spawnflags & PUSH_START_OFF)
	{
		if (!self->wait)
		{
			self->wait = 10;
		}

		self->think = trigger_push_active;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;
	}

	if (self->targetname) /* toggleable */
	{
		self->use = trigger_push_use;

		if (self->spawnflags & PUSH_START_OFF)
		{
			self->solid = SOLID_NOT;
		}
	}
	else if (self->spawnflags & PUSH_START_OFF)
	{
		gi.dprintf("trigger_push is START_OFF but not targeted.\n");
		self->svflags = 0;
		self->touch = NULL;
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
	}

	gi.linkentity(self);
}

/*
 * ==============================================================================
 *
 * trigger_hurt
 *
 * ==============================================================================
 */

/*
 * QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
 *
 * Any entity that touches this will be hurt.
 *
 * It does dmg points of damage each server frame
 *
 * SILENT			supresses playing the sound
 * SLOW			changes the damage rate to once per second
 * NO_PROTECTION	*nothing* stops the damage
 *
 * "dmg"			default 5 (whole numbers only)
 *
 */
void
hurt_use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->solid == SOLID_NOT)
	{
		int	i, num;
		edict_t	*touch[MAX_EDICTS], *hurtme;

		self->solid = SOLID_TRIGGER;
		num = gi.BoxEdicts(self->absmin, self->absmax,
				touch, MAX_EDICTS, AREA_SOLID);

		/* Check for idle monsters in
		   trigger hurt */
		for (i = 0 ; i < num ; i++)
		{
			hurtme = touch[i];
			hurt_touch (self, hurtme, NULL, NULL);
		}
	}
	else
	{
		self->solid = SOLID_NOT;
	}

	gi.linkentity(self);

	if (!(self->spawnflags & 2))
	{
		self->use = NULL;
	}
}

void
hurt_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	int dflags;

	if (!self || !other)
	{
		return;
	}

	if (!other->takedamage)
	{
		return;
	}

	if (self->timestamp > level.time)
	{
		return;
	}

	if (self->spawnflags & 16)
	{
		self->timestamp = level.time + 1;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	if (!(self->spawnflags & 4))
	{
		if ((level.framenum % 10) == 0)
		{
			gi.sound(other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
		}
	}

	if (self->spawnflags & 8)
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin,
			self->dmg, self->dmg, dflags, MOD_TRIGGER_HURT);
}

void
SP_trigger_hurt(edict_t *self)
{
	if (!self)
	{
		return;
	}

	InitTrigger(self);

	self->noise_index = gi.soundindex("world/electro.wav");
	self->touch = hurt_touch;

	if (!self->dmg)
	{
		self->dmg = 5;
	}

	if (self->spawnflags & 1)
	{
		self->solid = SOLID_NOT;
	}
	else
	{
		self->solid = SOLID_TRIGGER;
	}

	if (self->spawnflags & 2)
	{
		self->use = hurt_use;
	}

	gi.linkentity(self);
}

/*
 * ==============================================================================
 *
 * trigger_gravity
 *
 * ==============================================================================
 */

void
trigger_gravity_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_TRIGGER;
	}
	else
	{
		self->solid = SOLID_NOT;
	}

	gi.linkentity(self);
}

/*
 * QUAKED trigger_gravity (.5 .5 .5) ?
 * Changes the touching entites gravity to
 * the value of "gravity".  1.0 is standard
 * gravity for the level.
 */
void
trigger_gravity_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	other->gravity = self->gravity;
}

/*
 * QUAKED trigger_gravity (.5 .5 .5) ? TOGGLE START_OFF
 * Changes the touching entites gravity to
 * the value of "gravity".  1.0 is standard
 * gravity for the level.
 *
 * TOGGLE - trigger_gravity can be turned on and off
 * START_OFF - trigger_gravity starts turned off (implies TOGGLE)
 */
void
SP_trigger_gravity(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (st.gravity == 0)
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n",
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	InitTrigger(self);
	self->gravity = (int)strtol(st.gravity, (char **)NULL, 10);

	if (self->spawnflags & 1) /* TOGGLE */
	{
		self->use = trigger_gravity_use;
	}

	if (self->spawnflags & 2) /* START_OFF */
	{
		self->use = trigger_gravity_use;
		self->solid = SOLID_NOT;
	}

	self->touch = trigger_gravity_touch;
	gi.linkentity(self);
}

/*
 * ==============================================================================
 *
 * trigger_monsterjump
 *
 * ==============================================================================
 */

/*
 * QUAKED trigger_monsterjump (.5 .5 .5) ?
 * Walking monsters that touch this will jump in the direction of the trigger's angle
 *
 * "speed"  default to 200, the speed thrown forward
 * "height" default to 200, the speed thrown upwards
 */

void
trigger_monsterjump_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	if (other->flags & (FL_FLY | FL_SWIM))
	{
		return;
	}

	if (other->svflags & SVF_DEADMONSTER)
	{
		return;
	}

	if (!(other->svflags & SVF_MONSTER))
	{
		return;
	}

	/* set XY even if not on ground, so the jump will clear lips */
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;

	if (!other->groundentity)
	{
		return;
	}

	other->groundentity = NULL;
	other->velocity[2] = self->movedir[2];
}

void
SP_trigger_monsterjump(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->speed)
	{
		self->speed = 200;
	}

	if (!st.height)
	{
		st.height = 200;
	}

	if (self->s.angles[YAW] == 0)
	{
		self->s.angles[YAW] = 360;
	}

	InitTrigger(self);
	self->touch = trigger_monsterjump_touch;
	self->movedir[2] = st.height;
}

/* QUAKED trigger_flashlight (.5 .5 .5) ?
 * Players moving against this trigger will have their flashlight turned on or off.
 * "style" default to 0, set to 1 to always turn flashlight on, 2 to always turn off,
 *      otherwise "angles" are used to control on/off state
 */

#define SPAWNFLAG_FLASHLIGHT_CLIPPED 1

void
trigger_flashlight_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!other->client)
	{
		return;
	}

	if (self->style == 1)
	{
		P_ToggleFlashlight(other, true);
	}
	else if (self->style == 2)
	{
		P_ToggleFlashlight(other, false);
	}
	else if (VectorLength(other->velocity) > 6)
	{
		vec3_t forward;

		VectorNormalize2(other->velocity, forward);

		P_ToggleFlashlight(other, _DotProduct(forward, self->movedir) > 0);
	}
}

void
SP_trigger_flashlight(edict_t *self)
{
	if (self->s.angles[YAW] == 0)
	{
		self->s.angles[YAW] = 360;
	}

	InitTrigger(self);
	self->touch = trigger_flashlight_touch;
	self->movedir[2] = (float) st.height;

	gi.linkentity(self);
}

/*
 * QUAKED trigger_fog (.5 .5 .5) ? AFFECT_FOG AFFECT_HEIGHTFOG INSTANTANEOUS FORCE BLEND
 *
 * Players moving against this trigger will have their fog settings changed.
 * Fog/heightfog will be adjusted if the spawnflags are set. Instantaneous
 * ignores any delays. Force causes it to ignore movement dir and always use
 * the "on" values. Blend causes it to change towards how far you are into the trigger
 * with respect to angles.
 * "target" can target an info_notnull to pull the keys below from.
 * "delay" default to 0.5; time in seconds a change in fog will occur over
 * "wait" default to 0.0; time in seconds before a re-trigger can be executed
 *
 * "fog_density"; density value of fog, 0-1
 * "fog_color"; color value of fog, 3d vector with values between 0-1 (r g b)
 * "fog_density_off"; transition density value of fog, 0-1
 * "fog_color_off"; transition color value of fog, 3d vector with values between 0-1 (r g b)
 * "fog_sky_factor"; sky factor value of fog, 0-1
 * "fog_sky_factor_off"; transition sky factor value of fog, 0-1
 *
 * "heightfog_falloff"; falloff value of heightfog, 0-1
 * "heightfog_density"; density value of heightfog, 0-1
 * "heightfog_start_color"; the start color for the fog (r g b, 0-1)
 * "heightfog_start_dist"; the start distance for the fog (units)
 * "heightfog_end_color"; the start color for the fog (r g b, 0-1)
 * "heightfog_end_dist"; the end distance for the fog (units)
 *
 * "heightfog_falloff_off"; transition falloff value of heightfog, 0-1
 * "heightfog_density_off"; transition density value of heightfog, 0-1
 * "heightfog_start_color_off"; transition the start color for the fog (r g b, 0-1)
 * "heightfog_start_dist_off"; transition the start distance for the fog (units)
 * "heightfog_end_color_off"; transition the start color for the fog (r g b, 0-1)
 * "heightfog_end_dist_off"; transition the end distance for the fog (units)
 */

#define SPAWNFLAG_FOG_AFFECT_FOG 1
#define SPAWNFLAG_FOG_AFFECT_HEIGHTFOG 2
#define SPAWNFLAG_FOG_INSTANTANEOUS 4
#define SPAWNFLAG_FOG_FORCE 8
#define SPAWNFLAG_FOG_BLEND 16

void
trigger_fog_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */, csurface_t *surf /* unused */)
{
	edict_t *fog_value_storage;

	if (!other->client)
	{
		return;
	}

	if (self->timestamp > level.time)
	{
		return;
	}

	self->timestamp = level.time + self->wait;

	fog_value_storage = self;

	if (self->movetarget)
	{
		fog_value_storage = self->movetarget;
	}

	if (self->spawnflags & SPAWNFLAG_FOG_INSTANTANEOUS)
	{
		other->client->pers.fog_transition_time = 0;
	}
	else
	{
		other->client->pers.fog_transition_time = fog_value_storage->delay;
	}

	if (self->spawnflags & SPAWNFLAG_FOG_BLEND)
	{
		vec3_t center, half_size, start, end, player_dist,
			player_dist_diff, start_end;
		float dist;
		int i;

		VectorAdd(self->absmin, self->absmax, center);
		VectorScale(center, 0.5, center);
		VectorAdd(self->size, other->size, half_size);
		VectorScale(half_size, 0.5, half_size);

		VectorScale(self->movedir, -1.0, start);
		VectorScale(self->movedir, 1.0, end);
		for (i = 0; i < 3; i++)
		{
			start[i] *= half_size[i];
			end[i] *= half_size[i];
		}

		VectorSubtract(other->s.origin, center, player_dist);
		for (i = 0; i < 3; i++)
		{
			player_dist[i] *= fabs(self->movedir[i]);
		}

		VectorSubtract(player_dist, start, player_dist_diff);
		dist = VectorLength(player_dist_diff);
		VectorSubtract(start, end, start_end);
		dist /= VectorLength(start_end);
		dist = Q_clamp(dist, 0.f, 1.f);

		if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_FOG)
		{
			other->client->pers.wanted_fog[0] = Q_lerp(
				fog_value_storage->fog.density_off, fog_value_storage->fog.density, dist);
			other->client->pers.wanted_fog[1] = Q_lerp(
				fog_value_storage->fog.color_off[0], fog_value_storage->fog.color[0], dist);
			other->client->pers.wanted_fog[2] = Q_lerp(
				fog_value_storage->fog.color_off[1], fog_value_storage->fog.color[1], dist);
			other->client->pers.wanted_fog[3] = Q_lerp(
				fog_value_storage->fog.color_off[2], fog_value_storage->fog.color[2], dist);
			other->client->pers.wanted_fog[4] = Q_lerp(
				fog_value_storage->fog.sky_factor_off, fog_value_storage->fog.sky_factor, dist);
		}

		if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)
		{
			VectorLerp(
				fog_value_storage->heightfog.start_color_off,
				fog_value_storage->heightfog.start_color,
				dist,
				other->client->pers.wanted_heightfog.start
			);

			VectorLerp(
				fog_value_storage->heightfog.end_color_off,
				fog_value_storage->heightfog.end_color,
				dist,
				other->client->pers.wanted_heightfog.end
			);

			other->client->pers.wanted_heightfog.start[3] = Q_lerp(
				fog_value_storage->heightfog.start_dist_off,
				fog_value_storage->heightfog.start_dist, dist);
			other->client->pers.wanted_heightfog.end[3] = Q_lerp(
				fog_value_storage->heightfog.end_dist_off,
				fog_value_storage->heightfog.end_dist, dist);
			other->client->pers.wanted_heightfog.falloff = Q_lerp(
				fog_value_storage->heightfog.falloff_off,
				fog_value_storage->heightfog.falloff, dist);
			other->client->pers.wanted_heightfog.density = Q_lerp(
				fog_value_storage->heightfog.density_off,
				fog_value_storage->heightfog.density, dist);
		}

		return;
	}

	bool use_on = true;

	if (!(self->spawnflags & SPAWNFLAG_FOG_FORCE))
	{
		float len;
		vec3_t forward;

		VectorCopy(other->velocity, forward);
		len = VectorNormalize(forward);

		// not moving enough to trip; this is so we don't trip
		// the wrong direction when on an elevator, etc.
		if (len <= 0.0001f)
		{
			return;
		}

		use_on = _DotProduct(forward, self->movedir) > 0;
	}

	if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_FOG)
	{
		if (use_on)
		{
			int i;
			other->client->pers.wanted_fog[0] = fog_value_storage->fog.density;
			other->client->pers.wanted_fog[4] = fog_value_storage->fog.sky_factor;
			for (i = 0; i < 3; i++)
			{
				other->client->pers.wanted_fog[i + 1] = fog_value_storage->fog.color[i];
			}
		}
		else
		{
			int i;

			other->client->pers.wanted_fog[0] = fog_value_storage->fog.density_off;
			other->client->pers.wanted_fog[4] = fog_value_storage->fog.sky_factor_off;
			for (i = 0; i < 3; i++)
			{
				other->client->pers.wanted_fog[i + 1] = fog_value_storage->fog.color_off[i];
			}
		}
	}

	if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)
	{
		if (use_on)
		{
			/* start */
			VectorCopy(
				fog_value_storage->heightfog.start_color,
				other->client->pers.wanted_heightfog.start
			);
			other->client->pers.wanted_heightfog.start[3] =
				fog_value_storage->heightfog.start_dist;
			/* end */
			VectorCopy(
				fog_value_storage->heightfog.end_color,
				other->client->pers.wanted_heightfog.end
			);
			other->client->pers.wanted_heightfog.end[3] =
				fog_value_storage->heightfog.end_dist;

			/* falloff, density */
			other->client->pers.wanted_heightfog.falloff =
				fog_value_storage->heightfog.falloff;
			other->client->pers.wanted_heightfog.density =
				fog_value_storage->heightfog.density;
		}
		else
		{
			/* start */
			VectorCopy(
				fog_value_storage->heightfog.start_color_off,
				other->client->pers.wanted_heightfog.start
			);
			other->client->pers.wanted_heightfog.start[3] =
				fog_value_storage->heightfog.start_dist_off;
			/* end */
			VectorCopy(
				fog_value_storage->heightfog.end_color_off,
				other->client->pers.wanted_heightfog.end
			);
			other->client->pers.wanted_heightfog.end[3] =
				fog_value_storage->heightfog.end_dist_off;

			/* falloff, density */
			other->client->pers.wanted_heightfog.falloff =
				fog_value_storage->heightfog.falloff_off;
			other->client->pers.wanted_heightfog.density =
				fog_value_storage->heightfog.density_off;
		}
	}
}

void
SP_trigger_fog(edict_t *self)
{
	if (self->s.angles[YAW] == 0)
	{
		self->s.angles[YAW] = 360;
	}

	InitTrigger(self);

	if (!(self->spawnflags & (SPAWNFLAG_FOG_AFFECT_FOG | SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)))
	{
		Com_Printf("WARNING: %s with no fog spawnflags set\n", self->classname);
	}

	if (self->target)
	{
		self->movetarget = G_PickTarget(self->target);

		if (self->movetarget)
		{
			if (!self->movetarget->delay)
			{
				self->movetarget->delay = 0.5f;
			}
		}
	}

	if (!self->delay)
	{
		self->delay = 0.5f;
	}

	self->touch = trigger_fog_touch;
}

/*
 * QUAKED trigger_fogdensity (.5 .5 .5) ?
 * Heretic 2: Sets r_fog_density
 *
 * target: Fog density (.01 - .0001)
 */
void
trigger_fogdensity_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	float density;
	int i;

	if (!self || !other || !other->client)
	{
		return;
	}

	density = (float)strtod(self->target, (char **)NULL) * 10;
	other->client->pers.wanted_fog[0] = density;
	other->client->pers.wanted_fog[4] = 1.0;
	for (i = 0; i < 3; i++)
	{
		other->client->pers.wanted_fog[i + 1] = 0.75;
	}
}

void
SP_trigger_fogdensity(edict_t *self)
{
	if (!self)
	{
		return;
	}

	InitTrigger(self);
	self->touch = trigger_fogdensity_touch;
}

/*
 * QUAKED choose_cdtrack (.5 .5 .5) ?
 * Heretic 2: Sets CD track
 *
 * style: CD Track Id
 */
void
choose_cdtrack_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self)
	{
		return;
	}

	gi.configstring(CS_CDTRACK, va("%i", self->style));
}

void
SP_choose_cdtrack(edict_t *self)
{
	if (!self)
	{
		return;
	}

	InitTrigger(self);
	self->touch = choose_cdtrack_touch;
}
