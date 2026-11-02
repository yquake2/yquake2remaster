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

#include "../../header/local.h"
#include "grunt1.h"

/*
 * QUAKED monster_grunt1v1 (1 .5 0) (-16 -16 -25) (16 16 30) Ambush Trigger_Spawn Sight
 * QUAKED monster_grunt1v2 (1 .5 0) (-16 -16 -25) (16 16 30) Ambush Trigger_Spawn Sight
 */
void
SP_monster_grunt1(edict_t *self)
{
	int version;

	version = self->classname[strlen(self->classname) - 1] - '1';

	self->health = (60 + 20 * version) * st.health_multiplier;

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	monster_dynamic_setinfo(self);

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
	self->s.skinnum = version;
}
