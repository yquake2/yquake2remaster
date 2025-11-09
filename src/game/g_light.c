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
 * Custom light effects.
 *
 * =======================================================================
 */

#include "header/local.h"

void
env_fire_think(edict_t *self)
{
	self->nextthink = level.time + FRAMETIME;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_FLAME);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

/*
 * QUAKED light_chandelier1 (0.0 1.0 0.0) (-36.0 -36.0 -43.0) (36.0 36.0 43.0)
 *
 * Heretic 2: Chandelier (dirty gold) A tarnished gold chandelier hung from chains.
 */
void
SP_light_chandelier1(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->nextthink = level.time + FRAMETIME;
	self->think = object_object_think;
	self->monsterinfo.action = "poly";
	gi.linkentity(self);
}

/*
 * QUAKED light_chandelier2 (0.0 1.0 0.0) (-38.0 -38.0 -40.0) (18.0 18.0 40.0)
 *
 * Heretic 2: Chandelier (dirty metal) A tarnished metal chandelier hung from chains.
 */
void
SP_light_chandelier2(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->nextthink = level.time + FRAMETIME;
	self->think = object_object_think;
	self->monsterinfo.action = "poly";
	gi.linkentity(self);
}

/*
 * QUAKED light_chandelier3 (0.0 1.0 0.0) (-34.0 -34.0 -80.0) (34.0 34.0 0.0)
 *
 * Heretic 2: Chandelier (gold & large) A golden chandelier.
 */
void
SP_light_chandelier3(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->nextthink = level.time + FRAMETIME;
	self->think = object_object_think;
	self->monsterinfo.action = "poly";
	gi.linkentity(self);
}

/*
 * QUAKED env_fire (.3 .3 1.0) (-8 -8 -8) (8 8 8)
 * Heretic 2: Flame effect. Does not emit light.
 */
void
SP_env_fire(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = env_fire_think;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}
