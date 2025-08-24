/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2001 Steve Yeager
 * Copyright (C) 2001-2004 Pat AfterMoon
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
 * --------------------------------------------------------------
 * The ACE Bot is a product of Steve Yeager, and is available from
 * the ACE Bot homepage, at http://www.axionfx.com/ace.
 *
 * This program is a modification of the ACE Bot, and is therefore
 * in NO WAY supported by Steve Yeager.
 */

#include "../header/local.h"
#include "ai_local.h"

//ACE

//==========================================
// AI_EnemyAdded
// Add the Player to our list
//==========================================
void AI_EnemyAdded(edict_t *ent)
{
	if (num_AIEnemies < MAX_EDICTS)
		AIEnemies[num_AIEnemies++] = ent;
}

//==========================================
// AI_EnemyRemoved
// Remove the Player from list
//==========================================
void AI_EnemyRemoved(edict_t *ent)
{
	int i;
	int pos = -1;

	/* watch for 0 players */
	if (num_AIEnemies < 1)
	{
		return;
	}

	/* special case for only one player */
	if (num_AIEnemies == 1)
	{
		num_AIEnemies = 0;
		return;
	}

	/* Find the player */
	for (i = 0; i < num_AIEnemies; i++)
	{
		if (ent == AIEnemies[i])
		{
			pos = i;
		}
	}

	if (pos < 0)
	{
		return;
	}

	/* decrement */
	for (i = pos; i < num_AIEnemies - 1; i++)
	{
		AIEnemies[i] = AIEnemies[i + 1];
	}

	num_AIEnemies--;
}

extern gitem_armor_t jacketarmor_info;
extern gitem_armor_t combatarmor_info;
extern gitem_armor_t bodyarmor_info;
//==========================================
// AI_CanUseArmor
// Check if we can use the armor
//==========================================
qboolean AI_CanUseArmor (gitem_t *item, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	if (!other->client)
		return false;

	// get info on new armor
	newinfo = (gitem_armor_t *)item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (item->tag == ARMOR_SHARD)
		return true;

	// get info on old armor
	if (old_armor_index == ITEM_INDEX(FindItem("Jacket Armor")))
		oldinfo = &jacketarmor_info;
	else if (old_armor_index == ITEM_INDEX(FindItem("Combat Armor")))
		oldinfo = &combatarmor_info;
	else
		oldinfo = &bodyarmor_info;

	if (newinfo->normal_protection <= oldinfo->normal_protection)
	{
		// calc new armor values
		salvage = newinfo->normal_protection / oldinfo->normal_protection;
		salvagecount = salvage * newinfo->base_count;
		newcount = other->client->pers.inventory[old_armor_index] + salvagecount;

		if (newcount > oldinfo->max_count)
			newcount = oldinfo->max_count;

		// if we're already maxed out then we don't need the new armor
		if (other->client->pers.inventory[old_armor_index] >= newcount)
			return false;

	}

	return true;
}

//==========================================
// AI_CanPick_Ammo
// Check if we can use the Ammo
//==========================================
qboolean AI_CanPick_Ammo (edict_t *ent, gitem_t *item)
{
	int			index;
	int			max;

	if (!ent->client)
		return false;

	if (item->tag == AMMO_BULLETS)
		max = ent->client->pers.max_bullets;
	else if (item->tag == AMMO_SHELLS)
		max = ent->client->pers.max_shells;
	else if (item->tag == AMMO_ROCKETS)
		max = ent->client->pers.max_rockets;
	else if (item->tag == AMMO_GRENADES)
		max = ent->client->pers.max_grenades;
	else if (item->tag == AMMO_CELLS)
		max = ent->client->pers.max_cells;
	else if (item->tag == AMMO_SLUGS)
		max = ent->client->pers.max_slugs;
	else
		return false;

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] >= max)
		return false;

	return true;
}

//==========================================
// AI_ItemIsReachable
// Can we get there? Jalfixme: this needs much better checks
//==========================================
qboolean AI_ItemIsReachable(edict_t *self, vec3_t goal)
{
	trace_t trace;
	vec3_t v;

	VectorCopy(self->mins,v);
	v[2] += STEPSIZE;

//	trap_Trace (&trace, self->s.origin, v, self->maxs, goal, self, MASK_NODESOLID);
	trace = gi.trace ( self->s.origin, v, self->maxs, goal, self, MASK_NODESOLID );

	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;
}

//==========================================
// AI_ItemWeight
// Find out item weight
//==========================================
float
AI_ItemWeight(edict_t *self, edict_t *it)
{
	float		weight;

	if (!self->client)
		return 0;

	if (!it->item)
		return 0;

	//IT_WEAPON
	if (it->item->flags & IT_WEAPON)
	{
		return self->ai->status.inventoryWeights[ITEM_INDEX(it->item)];
	}

	//IT_AMMO
	if (it->item->flags & IT_AMMO)
	{
		return self->ai->status.inventoryWeights[ITEM_INDEX(it->item)];
	}

	//IT_ARMOR
	if (it->item->flags & IT_ARMOR)
	{
		return self->ai->status.inventoryWeights[ITEM_INDEX(it->item)];
	}

	//IT_FLAG
	if (it->item->flags & IT_FLAG)
	{
		return self->ai->status.inventoryWeights[ITEM_INDEX(it->item)];
	}

	//IT_HEALTH
	if (it->item->flags & IT_HEALTH)
	{
		//CanPickup_Health
		if (!(it->style & 1)) {	//#define HEALTH_IGNORE_MAX	1
			if (self->health >= self->max_health)
				return 0;
		}
		if (self->health >= 250 && it->count > 25)
			return 0;

		//find the weight
		weight = 0;
		if (self->health < 100)
			weight = ((100 - self->health) + it->count)*0.01;
		else if (self->health <= 250 && it->count == 100)//mega
			weight = 8.0;

		weight += (self->health < 25);//we just NEED IT!!!

		if (weight < 0.2)
			weight = 0.2;

		return weight;
	}

	//IT_POWERUP
	if (it->item->flags & IT_POWERUP)
		return 0.7;

	//IT_TECH
	if (it->item->flags & IT_TECH)
	{
		return self->ai->status.inventoryWeights[ITEM_INDEX(it->item)];
	}

	//IT_STAY_COOP
	if (it->item->flags & IT_STAY_COOP)
		return 0;

	//item didn't have a recognizable item flag
//	if (AIDevel.debugMode)
//		gi.cprintf(NULL, PRINT_HIGH, "(AI_ItemWeight) WARNING: Item with unhandled item flag:%s\n", it->classname);

	return 0;
}
