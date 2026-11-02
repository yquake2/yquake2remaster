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
#include "grunt2.h"

/*
 * QUAKED monster_grunt2 (1 .5 0) (-9 -18 -24) (8 18 40) Ambush Trigger_Spawn Sight
 */
void
SP_monster_grunt2(edict_t *self)
{
	self->health = 180 * st.health_multiplier;

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	monster_dynamic_setinfo(self);

	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);

	walkmonster_start(self);
}
