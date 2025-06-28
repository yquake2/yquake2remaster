/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * This file implements protocol conversion functions.
 *
 * =======================================================================
 */

#include "header/shared.h"
#include "header/common.h"

/* per-level limits Quake 2 Protocol version 26 */
#define MAX_CLIENTS_Q2DEMO 256             /* absolute limit */
#define MAX_EDICTS_Q2DEMO 1024             /* must change protocol to increase more */
#define MAX_LIGHTSTYLES_Q2DEMO 256
#define MAX_MODELS_Q2DEMO 256              /* these are sent over the net as bytes */
#define MAX_SOUNDS_Q2DEMO 256              /* so they cannot be blindly increased */
#define MAX_IMAGES_Q2DEMO 256
#define MAX_ITEMS_Q2DEMO 256
#define MAX_GENERAL_Q2DEMO (MAX_CLIENTS_Q2DEMO * 2)       /* general config strings */

/* CS structure Quake 2 Protocol version 26 */
#define CS_AIRACCEL_Q2DEMO 29              /* air acceleration control */
#define CS_MAXCLIENTS_Q2DEMO 30
#define CS_MAPCHECKSUM_Q2DEMO 31           /* for catching cheater maps */
#define CS_MODELS_Q2DEMO 32
#define CS_SOUNDS_Q2DEMO (CS_MODELS_Q2DEMO + MAX_MODELS_Q2DEMO)
#define CS_IMAGES_Q2DEMO (CS_SOUNDS_Q2DEMO + MAX_SOUNDS_Q2DEMO)
#define CS_LIGHTS_Q2DEMO (CS_IMAGES_Q2DEMO + MAX_IMAGES_Q2DEMO)
#define CS_ITEMS_Q2DEMO (CS_LIGHTS_Q2DEMO + MAX_LIGHTSTYLES_Q2DEMO)
#define CS_PLAYERSKINS_Q2DEMO (CS_ITEMS_Q2DEMO + MAX_ITEMS_Q2DEMO)
#define CS_GENERAL_Q2DEMO (CS_PLAYERSKINS_Q2DEMO + MAX_CLIENTS_Q2DEMO)
#define MAX_CONFIGSTRINGS_Q2DEMO (CS_GENERAL_Q2DEMO + MAX_GENERAL_Q2DEMO)

/* per-level limits Quake 2 ReRelease Protocol version 2022 */
#define MAX_CLIENTS_RR22DEMO 256             /* absolute limit */
#define MAX_EDICTS_RR22DEMO 8192             /* must change protocol to increase more */
#define MAX_LIGHTSTYLES_RR22DEMO 256
#define MAX_MODELS_RR22DEMO 8192             /* these are sent over the net as bytes */
#define MAX_SOUNDS_RR22DEMO 2048             /* so they cannot be blindly increased */
#define MAX_IMAGES_RR22DEMO 512
#define MAX_ITEMS_RR22DEMO 256
#define MAX_GENERAL_RR22DEMO (MAX_CLIENTS_RR22DEMO * 2)       /* general config strings */
#define MAX_WHEEL_ITEMS_RR22DEMO 32
#define MAX_SHADOW_LIGHTS_RR22DEMO 256

/* CS structure Quake 2 ReRelease Protocol version 2022 */
#define CS_AIRACCEL_RR22DEMO 59    /* air acceleration control */
#define CS_MAXCLIENTS_RR22DEMO 60
#define CS_MAPCHECKSUM_RR22DEMO 61 /* for catching cheater maps */
#define CS_MODELS_RR22DEMO 62
#define CS_SOUNDS_RR22DEMO (CS_MODELS_RR22DEMO + MAX_MODELS_RR22DEMO)
#define CS_IMAGES_RR22DEMO (CS_SOUNDS_RR22DEMO + MAX_SOUNDS_RR22DEMO)
#define CS_LIGHTS_RR22DEMO (CS_IMAGES_RR22DEMO + MAX_IMAGES_RR22DEMO)
#define CS_SHADOWLIGHTS_RR22DEMO (CS_LIGHTS_RR22DEMO + MAX_LIGHTSTYLES_RR22DEMO)
#define CS_ITEMS_RR22DEMO (CS_SHADOWLIGHTS_RR22DEMO + MAX_SHADOW_LIGHTS_RR22DEMO)
#define CS_PLAYERSKINS_RR22DEMO (CS_ITEMS_RR22DEMO + MAX_ITEMS_RR22DEMO)
#define CS_GENERAL_RR22DEMO (CS_PLAYERSKINS_RR22DEMO + MAX_CLIENTS_RR22DEMO)
/* [Paril-KEX] */
#define CS_WHEEL_WEAPONS_RR22DEMO (CS_GENERAL_RR22DEMO + MAX_GENERAL_RR22DEMO)
#define CS_WHEEL_AMMO_RR22DEMO (CS_WHEEL_WEAPONS_RR22DEMO + MAX_WHEEL_ITEMS_RR22DEMO)
#define CS_WHEEL_POWERUPS_RR22DEMO (CS_WHEEL_AMMO_RR22DEMO + MAX_WHEEL_ITEMS_RR22DEMO)
/* override default loop count */
#define CS_CD_LOOP_COUNT_RR22DEMO (CS_WHEEL_POWERUPS_RR22DEMO + MAX_WHEEL_ITEMS_RR22DEMO)
#define CS_GAME_STYLE_RR22DEMO (CS_CD_LOOP_COUNT_RR22DEMO + 1) /* see game_style_t */
#define MAX_CONFIGSTRINGS_RR22DEMO (CS_GAME_STYLE_RR22DEMO + 1)

/* Convert from current protocol to internal */
int
P_ConvertConfigStringFrom(int i, int protocol)
{
	if (IS_QII97_PROTOCOL(protocol))
	{
		if (i >= CS_AIRACCEL_Q2DEMO && i < CS_MODELS_Q2DEMO)
		{
			i += CS_AIRACCEL - CS_AIRACCEL_Q2DEMO;
		}
		else if (i >= CS_MODELS_Q2DEMO && i < CS_SOUNDS_Q2DEMO)
		{
			i += CS_MODELS - CS_MODELS_Q2DEMO;
		}
		else if (i >= CS_SOUNDS_Q2DEMO && i < CS_IMAGES_Q2DEMO)
		{
			i += CS_SOUNDS - CS_SOUNDS_Q2DEMO;
		}
		else if (i >= CS_IMAGES_Q2DEMO && i < CS_LIGHTS_Q2DEMO)
		{
			i += CS_IMAGES - CS_IMAGES_Q2DEMO;
		}
		else if (i >= CS_LIGHTS_Q2DEMO && i < CS_ITEMS_Q2DEMO)
		{
			i += CS_LIGHTS - CS_LIGHTS_Q2DEMO;
		}
		else if (i >= CS_ITEMS_Q2DEMO && i < CS_PLAYERSKINS_Q2DEMO)
		{
			i += CS_ITEMS - CS_ITEMS_Q2DEMO;
		}
		else if (i >= CS_PLAYERSKINS_Q2DEMO && i < CS_GENERAL_Q2DEMO)
		{
			i += CS_PLAYERSKINS - CS_PLAYERSKINS_Q2DEMO;
		}
		else if (i >= CS_GENERAL_Q2DEMO && i < MAX_CONFIGSTRINGS_Q2DEMO)
		{
			i += CS_GENERAL - CS_GENERAL_Q2DEMO;
		}
	}
	else if (protocol == PROTOCOL_RR22_VERSION)
	{
		if (i >= CS_AIRACCEL && i < CS_AIRACCEL_RR22DEMO)
		{
			/* Skip unknown configs */
			i = CS_SKIP;
		}
		else if (i == CS_AIRACCEL_RR22DEMO)
		{
			i = CS_AIRACCEL;
		}
		else if (i == CS_MAXCLIENTS_RR22DEMO)
		{
			i = CS_MAXCLIENTS;
		}
		else if (i == CS_MAPCHECKSUM_RR22DEMO)
		{
			i = CS_MAPCHECKSUM;
		}
		else if (i >= CS_MODELS_RR22DEMO && i < CS_SOUNDS_RR22DEMO)
		{
			i += CS_MODELS - CS_MODELS_RR22DEMO;
		}
		else if (i >= CS_SOUNDS_RR22DEMO && i < CS_IMAGES_RR22DEMO)
		{
			i += CS_SOUNDS - CS_SOUNDS_RR22DEMO;
		}
		else if (i >= CS_IMAGES_RR22DEMO && i < CS_LIGHTS_RR22DEMO)
		{
			i += CS_IMAGES - CS_IMAGES_RR22DEMO;
		}
		else if (i >= CS_LIGHTS_RR22DEMO && i < CS_SHADOWLIGHTS_RR22DEMO)
		{
			i += CS_LIGHTS - CS_LIGHTS_RR22DEMO;
		}
		else if (i >= CS_SHADOWLIGHTS_RR22DEMO && i < CS_ITEMS_RR22DEMO)
		{
			i += CS_SHADOWLIGHTS - CS_SHADOWLIGHTS_RR22DEMO;
		}
		else if (i >= CS_ITEMS_RR22DEMO && i < CS_PLAYERSKINS_RR22DEMO)
		{
			i += CS_ITEMS - CS_ITEMS_RR22DEMO;
		}
		else if (i >= CS_PLAYERSKINS_RR22DEMO && i < CS_GENERAL_RR22DEMO)
		{
			i += CS_PLAYERSKINS - CS_PLAYERSKINS_RR22DEMO;
		}
		else if (i >= CS_GENERAL_RR22DEMO && i < CS_WHEEL_WEAPONS_RR22DEMO)
		{
			i += CS_GENERAL - CS_GENERAL_RR22DEMO;
		}
		else if (i >= CS_WHEEL_WEAPONS_RR22DEMO && i < MAX_CONFIGSTRINGS_RR22DEMO)
		{
			/* Skip unknown configs */
			i = CS_SKIP;
		}
	}

	return i;
}

/* Convert from current protocol from internal */
int
P_ConvertConfigStringTo(int i, int protocol)
{
	if (IS_QII97_PROTOCOL(protocol))
	{
		if (i >= CS_MODELS && i < CS_SOUNDS)
		{
			i += CS_MODELS_Q2DEMO - CS_MODELS;

			if (i >= CS_SOUNDS_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_SOUNDS: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_SOUNDS && i < CS_IMAGES)
		{
			i += CS_SOUNDS_Q2DEMO - CS_SOUNDS;

			if (i >= CS_IMAGES_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_IMAGES: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_IMAGES && i < CS_LIGHTS)
		{
			i += CS_IMAGES_Q2DEMO - CS_IMAGES;

			if (i >= CS_LIGHTS_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_LIGHTS: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_LIGHTS && i < CS_ITEMS)
		{
			i += CS_LIGHTS_Q2DEMO - CS_LIGHTS;

			if (i >= CS_ITEMS_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_ITEMS: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_ITEMS && i < CS_PLAYERSKINS)
		{
			i += CS_ITEMS_Q2DEMO - CS_ITEMS;

			if (i >= CS_PLAYERSKINS_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_PLAYERSKINS: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_PLAYERSKINS && i < CS_GENERAL)
		{
			i += CS_PLAYERSKINS_Q2DEMO - CS_PLAYERSKINS;

			if (i >= CS_GENERAL_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: CS_GENERAL: bad index %i\n",
					__func__, i);
			}
		}
		else if (i >= CS_GENERAL && i < MAX_CONFIGSTRINGS)
		{
			i += CS_GENERAL_Q2DEMO - CS_GENERAL;

			if (i >= MAX_CONFIGSTRINGS_Q2DEMO)
			{
				Com_Error(ERR_DROP, "%s: MAX_CONFIGSTRINGS: bad index %i\n",
					__func__, i);
			}
		}
	}

	return i;
}
