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
 * This file implements all client side lighting
 *
 * =======================================================================
 */

#include "header/client.h"

typedef struct
{
	int length;
	float value[3];
	float map[MAX_QPATH];
} clightstyle_t;

static clightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
static int lastofs;

void
CL_ClearLightStyles(void)
{
	memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;
}

void
CL_RunLightStyles(void)
{
	int ofs;
	int i;
	clightstyle_t *ls;

	ofs = cl.time / 100;

	if (ofs == lastofs)
	{
		return;
	}

	lastofs = ofs;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}

		if (ls->length == 1)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		}

		else
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs % ls->length];
		}
	}
}

void
CL_SetLightstyle(int i)
{
	char *s;
	int j, k;

	s = cl.configstrings[i + CS_LIGHTS];

	j = (int)strlen(s);
	cl_lightstyle[i].length = j;

	for (k = 0; k < j; k++)
	{
		cl_lightstyle[i].map[k] = (float)(s[k] - 'a') / (float)('m' - 'a');
	}
}

void
CL_AddLightStyles(void)
{
	int i;
	clightstyle_t *ls;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		V_AddLightStyle(i, ls->value[0], ls->value[1], ls->value[2]);
	}
}

static cdlight_t cl_dlights[MAX_DLIGHTS];

void
CL_ClearDlights(void)
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
}

cdlight_t *
CL_AllocDlight(int key)
{
	int i;
	cdlight_t *dl;

	/* first look for an exact key match */
	if (key)
	{
		dl = cl_dlights;

		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				return dl;
			}
		}
	}

	/* then look for anything else */
	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	dl->key = key;
	return dl;
}

void
CL_RunDLights(void)
{
	int i;
	cdlight_t *dl;

	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
		{
			continue;
		}

		/* Vanilla Quake II had just cl.time. This worked in 1997
		   when computers were slow and the game reached ~30 FPS
		   on beefy hardware. Nowadays with 1000 FPS the dlights
		   are often rendered just a fraction of a frame. Work
		   around that by adding 32 ms, e.g. each dlight is shown
		   for at least 32 ms. */
		if (dl->die < cl.time - 32)
		{
			dl->radius = 0;
			continue;
		}

		dl->radius -= cls.rframetime * dl->decay;

		if (dl->radius < 0)
		{
			dl->radius = 0;
		}
	}
}

void
CL_AddDLights(void)
{
	int i;
	cdlight_t *dl;

	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
		{
			continue;
		}

		V_AddLight(dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}

