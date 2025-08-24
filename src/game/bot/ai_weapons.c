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

ai_weapon_t		AIWeapons[WEAP_TOTAL];

//WEAP_NONE,
//WEAP_BLASTER
//WEAP_SHOTGUN
//WEAP_SUPERSHOTGUN
//WEAP_MACHINEGUN
//WEAP_CHAINGUN
//WEAP_GRENADES
//WEAP_GRENADELAUNCHER
//WEAP_ROCKETLAUNCHER
//WEAP_HYPERBLASTER
//WEAP_RAILGUN
//WEAP_BFG
//WEAP_GRAPPLE

//==========================================
// AI_InitAIWeapons
//
// AIWeapons are the way the AI uses to analize
// weapon types, for choosing and fire them
//==========================================
void AI_InitAIWeapons (void)
{
	//clear all
	memset( &AIWeapons, 0, sizeof(ai_weapon_t)*WEAP_TOTAL);

	//BLASTER
	AIWeapons[WEAP_BLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.05; //blaster must always have some value
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.05;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.1;
	AIWeapons[WEAP_BLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2;
	AIWeapons[WEAP_BLASTER].weaponItem = FindItemByClassname("weapon_blaster");
	AIWeapons[WEAP_BLASTER].ammoItem = NULL;		//doesn't use ammo

	//SHOTGUN
	AIWeapons[WEAP_SHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.1;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAP_SHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_SHOTGUN].weaponItem = FindItemByClassname("weapon_shotgun");
	AIWeapons[WEAP_SHOTGUN].ammoItem = FindItemByClassname("ammo_shells");

	//SUPERSHOTGUN
	AIWeapons[WEAP_SUPERSHOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.2;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.2;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.6;
	AIWeapons[WEAP_SUPERSHOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7;
	AIWeapons[WEAP_SUPERSHOTGUN].weaponItem = FindItemByClassname("weapon_supershotgun");
	AIWeapons[WEAP_SUPERSHOTGUN].ammoItem = FindItemByClassname("ammo_shells");

	//MACHINEGUN
	AIWeapons[WEAP_MACHINEGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;
	AIWeapons[WEAP_MACHINEGUN].weaponItem = FindItemByClassname("weapon_machinegun");
	AIWeapons[WEAP_MACHINEGUN].ammoItem = FindItemByClassname("ammo_bullets");

	//CHAINGUN
	AIWeapons[WEAP_CHAINGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAP_CHAINGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.7;
	AIWeapons[WEAP_CHAINGUN].weaponItem = FindItemByClassname("weapon_chaingun");
	AIWeapons[WEAP_CHAINGUN].ammoItem = FindItemByClassname("ammo_bullets");

	//GRENADES
	AIWeapons[WEAP_GRENADES].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_LONG_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAP_GRENADES].RangeWeight[AIWEAP_MELEE_RANGE] = 0.2;
	AIWeapons[WEAP_GRENADES].weaponItem = FindItemByClassname("ammo_grenades");
	AIWeapons[WEAP_GRENADES].ammoItem = FindItemByClassname("ammo_grenades");

	//GRENADELAUNCHER
	AIWeapons[WEAP_GRENADELAUNCHER].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5;
	AIWeapons[WEAP_GRENADELAUNCHER].weaponItem = FindItemByClassname("weapon_grenadelauncher");
	AIWeapons[WEAP_GRENADELAUNCHER].ammoItem = FindItemByClassname("ammo_grenades");

	//ROCKETLAUNCHER
	AIWeapons[WEAP_ROCKETLAUNCHER].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.2;	//machinegun is better
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.9;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.6;
	AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem = FindItemByClassname("weapon_rocketlauncher");
	AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem = FindItemByClassname("ammo_rockets");

	//WEAP_HYPERBLASTER
	AIWeapons[WEAP_HYPERBLASTER].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_LONG_RANGE] = 0.1;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2;
	AIWeapons[WEAP_HYPERBLASTER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_HYPERBLASTER].weaponItem = FindItemByClassname("weapon_hyperblaster");
	AIWeapons[WEAP_HYPERBLASTER].ammoItem = FindItemByClassname("ammo_cells");

	//WEAP_RAILGUN
	AIWeapons[WEAP_RAILGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.9;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4;
	AIWeapons[WEAP_RAILGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3;
	AIWeapons[WEAP_RAILGUN].weaponItem = FindItemByClassname("weapon_railgun");
	AIWeapons[WEAP_RAILGUN].ammoItem = FindItemByClassname("ammo_slugs");

	//WEAP_BFG
	AIWeapons[WEAP_BFG].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7;
	AIWeapons[WEAP_BFG].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0;
	AIWeapons[WEAP_BFG].weaponItem = FindItemByClassname("weapon_bfg");
	AIWeapons[WEAP_BFG].ammoItem = FindItemByClassname("ammo_cells");

	//WEAP_GRAPPLE
	AIWeapons[WEAP_GRAPPLE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.0; //grapple is not used for attacks
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.0;
	AIWeapons[WEAP_GRAPPLE].weaponItem = FindItemByClassname("weapon_grapplinghook");
	AIWeapons[WEAP_GRAPPLE].ammoItem = NULL;		//doesn't use ammo

}

