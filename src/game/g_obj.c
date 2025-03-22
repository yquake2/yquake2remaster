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
 * Objects implementations
 *
 * =======================================================================
 */

#include "header/local.h"

/*
 * QUAKED object_flame1 (1 .5 .5) (-3 -3 -6) (3 3 11)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound torch
 *    2) sound campfire
 */
void
object_flame1_think(edict_t *self)
{
	int num, i, ofs_frames = 0, num_frames = 1;
	const dmdxframegroup_t * frames;

	frames = gi.SV_GetFrameGroups(self->s.modelindex, &num);
	for (i = 0; i < num; i++)
	{
		if (!strcmp(frames[i].name, "frame"))
		{
			ofs_frames = frames[i].ofs;
			num_frames = frames[i].num;
			break;
		}
	}

	i = self->s.frame - ofs_frames;
	if (i < 0)
	{
		i = 0;
	}
	i++;

	self->s.frame = ofs_frames + i % num_frames;
	self->nextthink = level.time + FRAMETIME;
}

void
SP_object_flame1(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = object_flame1_think;
	self->nextthink = level.time + FRAMETIME;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/torchburn.wav");
			break;
		case 2:
			self->s.sound = gi.soundindex("objects/fire/campfire.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}

/*
 * QUAKED object_big_fire (1 .5 .5) (-3 -3 -6) (3 3 11)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound campfire
 */
void
object_big_fire_think(edict_t *self)
{
	self->s.frame = (self->s.frame + 1) % 60;
	self->nextthink = level.time + FRAMETIME;

	/* add particles */
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_FLAME);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void
SP_object_big_fire(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->think = object_big_fire_think;
	self->nextthink = level.time + FRAMETIME;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/torchburn.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}

/*
 * QUAKED object_campfire (1 .5 .5) (-10 -10 -5) (10 10 5)
 *
 * Dawn of Darkness:
 * "sounds"
 *    0) no sound (default)
 *    1) sound campfire
*/
void
SP_object_campfire(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;

	self->s.frame = 0;

	switch (self->sounds) {
		case 1:
			self->s.sound = gi.soundindex("objects/fire/campfire.wav");
			break;
		default:
			self->s.sound = 0;
			break;
	}

	gi.linkentity(self);
}
