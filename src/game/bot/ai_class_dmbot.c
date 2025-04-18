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
 */

#include "../header/local.h"
#include "ai_local.h"

//ACE



//==========================================
// Some CTF stuff
//==========================================
static gitem_t *redflag;
static gitem_t *blueflag;


//==========================================
// BOT_DMclass_Move
// DMClass is generic bot class
//==========================================
static void
BOT_DMclass_Move(edict_t *self, usercmd_t *ucmd)
{
	int next_node_flags = 0;
	int	current_link_type = 0;
	int i;

	if (self->ai->next_node < 0)
	{
		return;
	}

	next_node_flags = nodes[self->ai->next_node].flags;
	if( AI_PlinkExists( self->ai->current_node, self->ai->next_node ))
	{
		current_link_type = AI_PlinkMoveType( self->ai->current_node, self->ai->next_node );
		//Com_Printf("%s\n", AI_LinkString( current_link_type ));
	}

	// Platforms
	if( current_link_type == LINK_PLATFORM )
	{
		// Move to the center
		self->ai->move_vector[2] = 0; // kill z movement
		if(VectorLength(self->ai->move_vector) > 10)
		{
			ucmd->forwardmove = 200; // walk to center
		}

		AI_ChangeAngle(self);

		return; // No move, riding elevator
	}
	else if( next_node_flags & NODEFLAGS_PLATFORM )
	{
		// is lift down?
		for(i=0;i<nav.num_ents;i++){
			if( nav.ents[i].node == self->ai->next_node )
			{
				//testing line
				//vec3_t	tPoint;
				//int		j;
				//for(j=0; j<3; j++)//center of the ent
				//	tPoint[j] = nav.ents[i].ent->s.origin[j] + 0.5*(nav.ents[i].ent->mins[j] + nav.ents[i].ent->maxs[j]);
				//tPoint[2] = nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2];
				//tPoint[2] += 8;
				//AITools_DrawLine( self->s.origin, tPoint );

				//if not reachable, wait for it (only height matters)
				if( ((nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2])
					> (self->s.origin[2] + self->mins[2] + AI_JUMPABLE_HEIGHT) ) &&
					nav.ents[i].ent->moveinfo.state != STATE_BOTTOM) //jabot092(2)
				{
					return; //wait for elevator
				}
			}
		}
	}

	// Ladder movement
	if( self->is_ladder )
	{
		ucmd->forwardmove = 70;
		ucmd->upmove = 200;
		ucmd->sidemove = 0;
		return;
	}

	// Falling off ledge
	if(!self->groundentity && !self->is_step && !self->is_swim )
	{
		AI_ChangeAngle(self);
		if (current_link_type == LINK_JUMPPAD)
		{
			ucmd->forwardmove = 100;
		}
		else if( current_link_type == LINK_JUMP )
		{
			self->velocity[0] = self->ai->move_vector[0] * 280;
			self->velocity[1] = self->ai->move_vector[1] * 280;
		}
		else
		{
			self->velocity[0] = self->ai->move_vector[0] * 160;
			self->velocity[1] = self->ai->move_vector[1] * 160;
		}
		return;
	}

	// jumping over (keep fall before this)
	if( current_link_type == LINK_JUMP && self->groundentity)
	{
		trace_t trace;
		vec3_t  v1, v2;
		//check floor in front, if there's none... Jump!
		VectorCopy( self->s.origin, v1 );
		VectorCopy( self->ai->move_vector, v2 );
		VectorNormalize( v2 );
		VectorMA( v1, 12, v2, v1 );
		v1[2] += self->mins[2];
		trace = gi.trace( v1, tv(-2, -2, -AI_JUMPABLE_HEIGHT), tv(2, 2, 0), v1, self, MASK_AISOLID );
		if( !trace.startsolid && trace.fraction == 1.0 )
		{
			//jump!
			ucmd->forwardmove = 400;
			//prevent double jumping on crates
			VectorCopy( self->s.origin, v1 );
			v1[2] += self->mins[2];
			trace = gi.trace( v1, tv(-12, -12, -8), tv(12, 12, 0), v1, self, MASK_AISOLID );
			if( trace.startsolid )
			{
				ucmd->upmove = 400;
			}
			return;
		}
	}

	// Move To Short Range goal (not following paths)
	// plats, grapple, etc have higher priority than SR Goals, cause the bot will
	// drop from them and have to repeat the process from the beginning
	if (AI_MoveToGoalEntity(self,ucmd))
	{
		return;
	}

	// swimming
	if( self->is_swim )
	{
		// We need to be pointed up/down
		AI_ChangeAngle(self);

		if( !(gi.pointcontents(nodes[self->ai->next_node].origin) & MASK_WATER) ) // Exit water
		{
			ucmd->upmove = 400;
		}

		ucmd->forwardmove = 300;
		return;
	}

	// Check to see if stuck, and if so try to free us
 	if(VectorLength(self->velocity) < 37)
	{
		// Keep a random factor just in case....
		if( random() > 0.1 && AI_SpecialMove(self, ucmd) ) //jumps, crouches, turns...
		{
			return;
		}

		self->s.angles[YAW] += random() * 180 - 90;

		AI_ChangeAngle(self);

		ucmd->forwardmove = 400;

		return;
	}


	AI_ChangeAngle(self);

	// Otherwise move as fast as we can...
	ucmd->forwardmove = 400;
}


//==========================================
// BOT_DMclass_Wander
// movement when couldn't find a goal
// Wandering code (based on old ACE movement code)
//==========================================
void
BOT_DMclass_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;

	// Do not move
	if(self->ai->next_move_time > level.time)
	{
		return;
	}

	if (self->deadflag)
	{
		return;
	}

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
	{
		if(self->groundentity->moveinfo.state == STATE_UP ||
		   self->groundentity->moveinfo.state == STATE_DOWN)
		{
			self->velocity[0] = 0;
			self->velocity[1] = 0;
			self->velocity[2] = 0;
			self->ai->next_move_time = level.time + 0.5;
			return;
		}
	}

	// Move To Goal (Short Range Goal, not following paths)
	if (AI_MoveToGoalEntity(self, ucmd))
	{
		return;
	}

	// Swimming?
	VectorCopy(self->s.origin,temp);
	temp[2] += 24;

	if( gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if( self->client && self->client->next_drown_time > 0 )	//jalfixme: client references must pass into botStatus
		{
			ucmd->upmove = 100;
			self->s.angles[PITCH] = -45;
		}
		else
		{
			ucmd->upmove = 15;
		}

		ucmd->forwardmove = 300;
	}
	// else self->client->next_drown_time = 0; // probably shound not be messing with this, but

	// Lava?
	temp[2] -= 48;

	if( gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME) )
	{
		self->s.angles[YAW] += random() * 360 - 180;
		ucmd->forwardmove = 400;
		if(self->groundentity)
		{
			ucmd->upmove = 400;
		}
		else
		{
			ucmd->upmove = 0;
		}

		return;
	}

	// Check for special movement
 	if(VectorLength(self->velocity) < 37)
	{
		if(random() > 0.1 && AI_SpecialMove(self,ucmd))	//jumps, crouches, turns...
		{
			return;
		}

		self->s.angles[YAW] += random() * 180 - 90;

		if (!self->is_step)// if there is ground continue otherwise wait for next move
		{
			ucmd->forwardmove = 0; //0
		}
		else if( AI_CanMove( self, BOT_MOVE_FORWARD))
		{
			ucmd->forwardmove = 100;
		}

		return;
	}


	// Otherwise move slowly, walking wondering what's going on
	if( AI_CanMove( self, BOT_MOVE_FORWARD))
	{
		ucmd->forwardmove = 100;
	}
	else
	{
		ucmd->forwardmove = -100;
	}
}


//==========================================
// BOT_DMclass_CombatMovement
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
//==========================================
void
BOT_DMclass_CombatMovement(edict_t *self, usercmd_t *ucmd)
{
	float	c;
	vec3_t	attackvector;
	float	dist;

	//jalToDo: Convert CombatMovement to a BOT_STATE, allowing
	//it to dodge, but still follow paths, chasing enemy or
	//running away... hmmm... maybe it will need 2 different BOT_STATEs

	if(!self->enemy)
	{

		// do whatever (tmp move wander)
		if (AI_FollowPath(self))
		{
			BOT_DMclass_Move(self, ucmd);
		}

		return;
	}

	// Randomly choose a movement direction
	c = random();

	if (c < 0.2 && AI_CanMove(self, BOT_MOVE_LEFT))
	{
		ucmd->sidemove -= 400;
	}
	else if (c < 0.4 && AI_CanMove(self, BOT_MOVE_RIGHT))
	{
		ucmd->sidemove += 400;
	}
	else if (c < 0.6 && AI_CanMove(self, BOT_MOVE_FORWARD))
	{
		ucmd->forwardmove += 400;
	}
	else if(c < 0.8 && AI_CanMove(self,BOT_MOVE_BACK))
	{
		ucmd->forwardmove -= 400;
	}

	VectorSubtract( self->s.origin, self->enemy->s.origin, attackvector);
	dist = VectorLength( attackvector);

	if(dist < 75)
	{
		ucmd->forwardmove -= 200;
	}
}


//==========================================
// BOT_DMclass_CheckShot
// Checks if shot is blocked or if too far to shoot
//==========================================
static qboolean
BOT_DMclass_CheckShot(edict_t *ent, vec3_t	point)
{
	trace_t tr;
	vec3_t	start, forward, right, offset;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource(ent, offset, forward, right, start);

	//bloqued, don't shoot
	tr = gi.trace( start, vec3_origin, vec3_origin, point, ent, MASK_AISOLID);
//	trap_Trace( &tr, self->s.origin, vec3_origin, vec3_origin, point, self, MASK_AISOLID);
	if (tr.fraction < 0.3) //just enough to prevent self damage (by now)
		return false;

	return true;
}


//==========================================
// BOT_DMclass_FindEnemy
// Scan for enemy (simplifed for now to just pick any visible enemy)
//==========================================
qboolean
BOT_DMclass_FindEnemy(edict_t *self)
{
	int i;

	edict_t		*bestenemy = NULL;
	float		bestweight = 99999;
	float		weight;
	vec3_t		dist;

	// we already set up an enemy this frame (reacting to attacks)
	if(self->enemy != NULL)
		return true;

	// Find Enemy
	for(i=0;i<num_AIEnemies;i++)
	{
		if( AIEnemies[i] == NULL || AIEnemies[i] == self
			|| AIEnemies[i]->solid == SOLID_NOT)
			continue;

		//Ignore players with 0 weight (was set at botstatus)
		if(self->ai->status.playersWeights[i] == 0)
			continue;

		if( !AIEnemies[i]->deadflag && visible(self, AIEnemies[i]) &&
			//trap_inPVS (self->s.origin, players[i]->s.origin))
			gi.inPVS(self->s.origin, AIEnemies[i]->s.origin))
		{
			//(weight enemies from fusionbot) Is enemy visible, or is it too close to ignore
			VectorSubtract(self->s.origin, AIEnemies[i]->s.origin, dist);
			weight = VectorLength( dist );

			//modify weight based on precomputed player weights
			weight *= (1.0 - self->ai->status.playersWeights[i]);

			if( infront( self, AIEnemies[i] ) ||
				(weight < 300 ) )
			{
				// Check if best target, or better than current target
				if (weight < bestweight)
				{
					bestweight = weight;
					bestenemy = AIEnemies[i];
				}
			}
		}
	}

	// If best enemy, set up
	if(bestenemy)
	{
//		if (AIDevel.debugChased && bot_showcombat->value && bestenemy->ai->is_bot)
//			gi.cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
//			self->ai->pers.netname,
//			bestenemy->ai->pers.netname);

		self->enemy = bestenemy;
		return true;
	}

	return false;	// NO enemy
}


//==========================================
// BOT_DMClass_ChangeWeapon
//==========================================
qboolean BOT_DMClass_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	// see if we're already using it
	if (!item || item == ent->client->pers.weapon)
		return true;

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		if ( !ent->client->pers.inventory[ammo_index] && !g_select_empty->value )
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;
	ent->ai->changeweapon_timeout = level.time + 6.0;

	return true;
}


//==========================================
// BOT_DMclass_ChooseWeapon
// Choose weapon based on range & weights
//==========================================
void BOT_DMclass_ChooseWeapon(edict_t *self)
{
	float	dist;
	vec3_t	v;
	int		i;
	float	best_weight = 0.0;
	gitem_t	*best_weapon = NULL;
	int		weapon_range = 0;

	// if no enemy, then what are we doing here?
	if(!self->enemy)
		return;

	if( self->ai->changeweapon_timeout > level.time )
		return;

	// Base weapon selection on distance:
	VectorSubtract (self->s.origin, self->enemy->s.origin, v);
	dist = VectorLength(v);

	if(dist < 150)
		weapon_range = AIWEAP_MELEE_RANGE;

	else if(dist < 500)	//Medium range limit is Grenade Laucher range
		weapon_range = AIWEAP_SHORT_RANGE;

	else if(dist < 900)
		weapon_range = AIWEAP_MEDIUM_RANGE;

	else
		weapon_range = AIWEAP_LONG_RANGE;


	for(i=0; i<WEAP_TOTAL; i++)
	{
		if (!AIWeapons[i].weaponItem)
			continue;

		//ignore those we don't have
		if (!self->client->pers.inventory[ITEM_INDEX(AIWeapons[i].weaponItem)] )
			continue;

		//ignore those we don't have ammo for
		if (AIWeapons[i].ammoItem != NULL	//excepting for those not using ammo
			&& !self->client->pers.inventory[ITEM_INDEX(AIWeapons[i].ammoItem)] )
			continue;

		//compare range weights
		if (AIWeapons[i].RangeWeight[weapon_range] > best_weight) {
			best_weight = AIWeapons[i].RangeWeight[weapon_range];
			best_weapon = AIWeapons[i].weaponItem;
		}
		//jal: enable randomnes later
		//else if (AIWeapons[i].RangeWeight[weapon_range] == best_weight && random() > 0.2) {	//allow some random for equal weights
		//	best_weight = AIWeapons[i].RangeWeight[weapon_range];
		//	best_weapon = AIWeapons[i].weaponItem;
		//}
	}

	//do the change (same weapon, or null best_weapon is covered at ChangeWeapon)
	BOT_DMClass_ChangeWeapon( self, best_weapon );

	return;
}


//==========================================
// BOT_DMclass_FireWeapon
// Fire if needed
//==========================================
void BOT_DMclass_FireWeapon (edict_t *self, usercmd_t *ucmd)
{
	//float	c;
	float	firedelay;
	vec3_t  target;
	vec3_t  angles;
	int		weapon;
	//vec3_t	attackvector;
	//float	dist;

	if (!self->enemy)
		return;

	//weapon = self->s.skinnum & 0xff;
	if (self->client->pers.weapon)
			weapon = (self->client->pers.weapon->weapmodel & 0xff);
	else
		weapon = 0;

	//jalToDo: Add different aiming types (explosive aim to legs, hitscan aim to body)

	//was find range. I might use it later
	//VectorSubtract( self->s.origin, self->enemy->s.origin, attackvector);
	//dist = VectorLength( attackvector);


	// Aim
	VectorCopy(self->enemy->s.origin,target);

	// find out our weapon AIM style
	if( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION_EXPLOSIVE )
	{
		//aim to the feets when enemy isn't higher
		if( self->s.origin[2] + self->viewheight > target[2] + (self->enemy->mins[2] * 0.8) )
			target[2] += self->enemy->mins[2];
	}
	else if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION )
	{
		//jalToDo

	}
	else if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_DROP )
	{
		//jalToDo

	} else { //AI_AIMSTYLE_INSTANTHIT

	}

	// modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
	target[0] += (random()-0.5) * ((MAX_BOT_SKILL - self->ai->pers.skillLevel) *2);
	target[1] += (random()-0.5) * ((MAX_BOT_SKILL - self->ai->pers.skillLevel) *2);

	// Set direction
	VectorSubtract (target, self->s.origin, self->ai->move_vector);
	vectoangles (self->ai->move_vector, angles);
	VectorCopy(angles,self->s.angles);


	// Set the attack
	firedelay = random()*(MAX_BOT_SKILL*1.8);
	if (firedelay > (MAX_BOT_SKILL - self->ai->pers.skillLevel) && BOT_DMclass_CheckShot(self, target))
		ucmd->buttons = BUTTON_ATTACK;

	//if(AIDevel.debugChased && bot_showcombat->integer)
	//	gi.cprintf(AIDevel.devguy, PRINT_HIGH, "%s: attacking %s\n",self->bot.pers.netname ,self->enemy->r.client->pers.netname);
}


//==========================================
// BOT_DMclass_WeightPlayers
// weight players based on game state
//==========================================
void BOT_DMclass_WeightPlayers(edict_t *self)
{
	int i;

	//clear
	memset(self->ai->status.playersWeights, 0, sizeof (self->ai->status.playersWeights));

	for( i=0; i<num_AIEnemies; i++ )
	{
		if( AIEnemies[i] == NULL )
			continue;

		if( AIEnemies[i] == self )
			continue;

		//ignore spectators and dead players
		if( AIEnemies[i]->svflags & SVF_NOCLIENT || AIEnemies[i]->deadflag ) {
			self->ai->status.playersWeights[i] = 0.0f;
			continue;
		}

		if( ctf->value )
		{
			if( AIEnemies[i]->client->resp.ctf_team != self->client->resp.ctf_team )
			{
				//being at enemy team gives a small weight, but weight afterall
				self->ai->status.playersWeights[i] = 0.2;

				//enemy has redflag
				if( redflag && AIEnemies[i]->client->pers.inventory[ITEM_INDEX(redflag)]
					&& (self->client->resp.ctf_team == CTF_TEAM1) )
				{
					if( !self->client->pers.inventory[ITEM_INDEX(blueflag)] ) //don't hunt if you have the other flag, let others do
						self->ai->status.playersWeights[i] = 0.9;
				}

				//enemy has blueflag
				if( blueflag && AIEnemies[i]->client->pers.inventory[ITEM_INDEX(blueflag)]
					&& (self->client->resp.ctf_team == CTF_TEAM2) )
				{
					if( !self->client->pers.inventory[ITEM_INDEX(redflag)] ) //don't hunt if you have the other flag, let others do
						self->ai->status.playersWeights[i] = 0.9;
				}
			}
		}
		else	//if not at ctf every player has some value
			self->ai->status.playersWeights[i] = 0.3;

	}
}


//==========================================
// BOT_DMclass_WantedFlag
// find needed flag
//==========================================
gitem_t	*BOT_DMclass_WantedFlag (edict_t *self)
{
	qboolean	hasflag;

	if (!ctf->value)
		return NULL;

	if (!self->client || !self->client->resp.ctf_team)
		return NULL;

	//find out if the player has a flag, and what flag is it
	if (redflag && self->client->pers.inventory[ITEM_INDEX(redflag)])
		hasflag = true;
	else if (blueflag && self->client->pers.inventory[ITEM_INDEX(blueflag)])
		hasflag = true;
	else
		hasflag = false;

	//jalToDo: see if our flag is at base

	if (!hasflag)//if we don't have a flag we want other's team flag
	{
		if (self->client->resp.ctf_team == CTF_TEAM1)
			return blueflag;
		else
			return redflag;
	}
	else	//we have a flag
	{
		if (self->client->resp.ctf_team == CTF_TEAM1)
			return redflag;
		else
			return blueflag;
	}

	return NULL;
}


//==========================================
// BOT_DMclass_WeightInventory
// weight items up or down based on bot needs
//==========================================
void BOT_DMclass_WeightInventory(edict_t *self)
{
	float		LowNeedFactor = 0.5;
	gclient_t	*client;
	int			i;

	client = self->client;

	//reset with persistant values
	memcpy(self->ai->status.inventoryWeights, self->ai->pers.inventoryWeights, sizeof(self->ai->pers.inventoryWeights));


	//weight ammo down if bot doesn't have the weapon for it,
	//or denny weight for it, if bot is packed up.
	//------------------------------------------------------

	//AMMO_BULLETS

	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_MACHINEGUN].ammoItem) )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_CHAINGUN].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].weaponItem)] )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].ammoItem)] *= LowNeedFactor;

	//AMMO_SHELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_SHOTGUN].ammoItem) )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_SUPERSHOTGUN].weaponItem)] )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].ammoItem)] *= LowNeedFactor;

	//AMMO_ROCKETS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem))
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem)] )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem)] *= LowNeedFactor;

	//AMMO_GRENADES:

	//find if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_GRENADES].ammoItem))
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADES].ammoItem)] = 0.0;
	//grenades are also weapons, and are weighted down by LowNeedFactor in weapons group

	//AMMO_CELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_HYPERBLASTER].ammoItem))
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_BFG].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(FindItemByClassname("item_power_shield"))])
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].ammoItem)] *= LowNeedFactor;

	//AMMO_SLUGS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_RAILGUN].ammoItem))
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].weaponItem)] )
		self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].ammoItem)] *= LowNeedFactor;


	//WEAPONS
	//-----------------------------------------------------

	//weight weapon down if bot already has it
	for (i=0; i<WEAP_TOTAL; i++) {
		if ( AIWeapons[i].weaponItem && client->pers.inventory[ITEM_INDEX(AIWeapons[i].weaponItem)])
			self->ai->status.inventoryWeights[ITEM_INDEX(AIWeapons[i].weaponItem)] *= LowNeedFactor;
	}

	//ARMOR
	//-----------------------------------------------------
	//shards are ALWAYS accepted but still...
	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_shard"), self ))
		self->ai->status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_shard"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_jacket"), self ))
		self->ai->status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_jacket"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_combat"), self ))
		self->ai->status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_combat"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_body"), self ))
		self->ai->status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_body"))] = 0.0;


	//TECH :
	//-----------------------------------------------------
	if ( self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("item_tech1"))]
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("item_tech2"))]
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("item_tech3"))]
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("item_tech4"))] )
	{
		self->ai->status.inventoryWeights[ITEM_INDEX( FindItemByClassname("item_tech1"))] = 0.0;
		self->ai->status.inventoryWeights[ITEM_INDEX( FindItemByClassname("item_tech2"))] = 0.0;
		self->ai->status.inventoryWeights[ITEM_INDEX( FindItemByClassname("item_tech3"))] = 0.0;
		self->ai->status.inventoryWeights[ITEM_INDEX( FindItemByClassname("item_tech4"))] = 0.0;
	}

	//CTF:
	//-----------------------------------------------------
	if( ctf->value )
	{
		gitem_t		*wantedFlag;

		wantedFlag = BOT_DMclass_WantedFlag( self ); //Returns the flag gitem_t

		//flags have weights defined inside persistant inventory. Remove weight from the unwanted one/s.
		if (blueflag && blueflag != wantedFlag)
			self->ai->status.inventoryWeights[ITEM_INDEX(blueflag)] = 0.0;
		if (redflag && redflag != wantedFlag)
			self->ai->status.inventoryWeights[ITEM_INDEX(redflag)] = 0.0;
	}
}


//==========================================
// BOT_DMclass_UpdateStatus
// update ai->status values based on bot state,
// so ai can decide based on these settings
//==========================================
void BOT_DMclass_UpdateStatus( edict_t *self )
{
	self->enemy = NULL;
	self->movetarget = NULL;

	// Set up for new client movement: jalfixme
	VectorCopy(self->client->ps.viewangles,self->s.angles);
	VectorSet (self->client->ps.pmove.delta_angles, 0, 0, 0);

	//JALFIXMEQ2
/*
	if (self->client->jumppad_time)
		self->ai->status.jumpadReached = true;	//jumpad time from client to botStatus
	else
		self->ai->status.jumpadReached = false;
*/
	if (self->client->ps.pmove.pm_flags & PMF_TIME_TELEPORT)
		self->ai->status.TeleportReached = true;
	else
		self->ai->status.TeleportReached = false;

	//set up AI status for the upcoming AI_frame
	BOT_DMclass_WeightInventory( self );	//weight items
	BOT_DMclass_WeightPlayers( self );		//weight players
}


//==========================================
// BOT_DMClass_BloquedTimeout
// the bot has been bloqued for too long
//==========================================
void BOT_DMClass_BloquedTimeout( edict_t *self )
{
	self->health = 0;
	self->ai->bloqued_timeout = level.time + 15.0;
	self->die(self, self, self, 100000, vec3_origin);
	self->nextthink = level.time + FRAMETIME;
}


//==========================================
// BOT_DMclass_DeadFrame
// ent is dead = run this think func
//==========================================
void BOT_DMclass_DeadFrame( edict_t *self )
{
	usercmd_t	ucmd;

	// ask for respawn
	self->client->buttons = 0;
	ucmd.buttons = BUTTON_ATTACK;
	ClientThink (self, &ucmd);
	self->nextthink = level.time + FRAMETIME;
}


//==========================================
// BOT_DMclass_RunFrame
// States Machine & call client movement
//==========================================
void BOT_DMclass_RunFrame( edict_t *self )
{
	usercmd_t ucmd = {0};

	// Look for enemies
	if( BOT_DMclass_FindEnemy(self) )
	{
		BOT_DMclass_ChooseWeapon( self );
		BOT_DMclass_FireWeapon( self, &ucmd );
		self->ai->state = BOT_STATE_ATTACK;
		self->ai->state_combat_timeout = level.time + 1.0;
	}
	else if( self->ai->state == BOT_STATE_ATTACK &&
		level.time > self->ai->state_combat_timeout)
	{
		//Jalfixme: change to: AI_SetUpStateMove(self);
		self->ai->state = BOT_STATE_MOVE;
	}
	// Execute the move, or wander
	if (self->ai->state == BOT_STATE_MOVE)
	{
		BOT_DMclass_Move( self, &ucmd );
	}
	else if(self->ai->state == BOT_STATE_ATTACK)
	{
		BOT_DMclass_CombatMovement( self, &ucmd );
	}
	else if ( self->ai->state == BOT_STATE_WANDER )
	{
		BOT_DMclass_Wander( self, &ucmd );
	}

	//set up for pmove
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);

	// set approximate ping and show values
	ucmd.msec = 75 + floor (random () * 25) + 1;
	self->client->ping = ucmd.msec;

	// send command through id's code
	ClientThink( self, &ucmd );
	self->nextthink = level.time + FRAMETIME;
}


//==========================================
// BOT_DMclass_InitPersistant
// Persistant after respawns.
//==========================================
void BOT_DMclass_InitPersistant(edict_t *self)
{
	self->classname = "dmbot";

	/* copy name */
	if (self->client->pers.netname[0])
	{
		self->ai->pers.netname = self->client->pers.netname;
	}
	else
	{
		self->ai->pers.netname = "dmBot";
	}

	/* set 'class' functions */
	self->ai->pers.RunFrame = BOT_DMclass_RunFrame;
	self->ai->pers.UpdateStatus = BOT_DMclass_UpdateStatus;
	self->ai->pers.bloquedTimeout = BOT_DMClass_BloquedTimeout;
	self->ai->pers.deadFrame = BOT_DMclass_DeadFrame;

	/* available moveTypes for this class */
	self->ai->pers.moveTypesMask = (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_JUMPPAD|LINK_PLATFORM|LINK_TELEPORT|LINK_LADDER|LINK_JUMP|LINK_CROUCH);

	//Persistant Inventory Weights (0 = can not pick)
	memset(self->ai->pers.inventoryWeights, 0, sizeof (self->ai->pers.inventoryWeights));

	//weapons
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_BLASTER].weaponItem)] = 0.0;
	//self->bot.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("weapon_blaster"))] = 0.0; //it's the same thing
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].weaponItem)] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SUPERSHOTGUN].weaponItem)] = 0.7;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].weaponItem)] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_CHAINGUN].weaponItem)] = 0.7;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADES].weaponItem)] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADELAUNCHER].weaponItem)] = 0.6;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem)] = 0.8;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].weaponItem)] = 0.7;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].weaponItem)] = 0.8;
	self->ai->pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_BFG].weaponItem)] = 0.5;

	//ammo
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_shells"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_bullets"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_cells"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_rockets"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_slugs"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("ammo_grenades"))] = 0.5;

	//armor
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_body"))] = 0.9;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_combat"))] = 0.8;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_jacket"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_shard"))] = 0.2;

	//techs
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_tech1"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_tech2"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_tech3"))] = 0.5;
	self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_tech4"))] = 0.5;

	if( ctf->value )
	{
		redflag = FindItemByClassname("item_flag_team1");	// store pointers to flags gitem_t, for
		blueflag = FindItemByClassname("item_flag_team2");// simpler comparisons inside this archive
		self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_flag_team1"))] = 3.0;
		self->ai->pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_flag_team2"))] = 3.0;
	}
}
