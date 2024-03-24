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
#define CS_MODELS_Q2DEMO 32
#define CS_SOUNDS_Q2DEMO (CS_MODELS_Q2DEMO + MAX_MODELS_Q2DEMO)
#define CS_IMAGES_Q2DEMO (CS_SOUNDS_Q2DEMO + MAX_SOUNDS_Q2DEMO)
#define CS_LIGHTS_Q2DEMO (CS_IMAGES_Q2DEMO + MAX_IMAGES_Q2DEMO)
#define CS_ITEMS_Q2DEMO (CS_LIGHTS_Q2DEMO + MAX_LIGHTSTYLES_Q2DEMO)
#define CS_PLAYERSKINS_Q2DEMO (CS_ITEMS_Q2DEMO + MAX_ITEMS_Q2DEMO)
#define CS_GENERAL_Q2DEMO (CS_PLAYERSKINS_Q2DEMO + MAX_CLIENTS_Q2DEMO)
#define MAX_CONFIGSTRINGS_Q2DEMO (CS_GENERAL_Q2DEMO + MAX_GENERAL_Q2DEMO)

/* Convert from current protocol to internal */
int
P_ConvertConfigStringFrom(int i, int protocol)
{
	if (protocol == PROTOCOL_RELEASE_VERSION ||
		protocol == PROTOCOL_DEMO_VERSION ||
		protocol == PROTOCOL_XATRIX_VERSION ||
		protocol == PROTOCOL_RR97_VERSION)
	{
		if (i >= CS_MODELS_Q2DEMO && i < CS_SOUNDS_Q2DEMO)
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

	return i;
}

/* Convert from current protocol from internal */
int
P_ConvertConfigStringTo(int i, int protocol)
{
	if (protocol == PROTOCOL_RELEASE_VERSION ||
		protocol == PROTOCOL_DEMO_VERSION ||
		protocol == PROTOCOL_XATRIX_VERSION ||
		protocol == PROTOCOL_RR97_VERSION)
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
