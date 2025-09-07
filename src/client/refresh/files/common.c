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
 * Refresh common initialization code
 *
 * =======================================================================
 */

#include "../ref_shared.h"

#define MAXPRINTMSG 4096

/*
 * this is only here so the functions in shared source files
 * (shared.c, rand.c, flash.c, mem.c/hunk.c) can link
 */
void
R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(level, msg, argptr);
	va_end(argptr);
}

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[MAXPRINTMSG];

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(PRINT_ALL, msg, argptr);
	va_end(argptr);
}

void
Com_DPrintf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(PRINT_DEVELOPER, msg, argptr);
	va_end(argptr);
}

void
Com_Error(int code, const char *fmt, ...)
{
	va_list argptr;
	char text[MAXPRINTMSG];

	va_start(argptr, fmt);
	vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	ri.Sys_Error(code, "%s", text);
}

/* shared variables */
refdef_t r_newrefdef;
viddef_t vid;

static void
R_GetHeightFog(float *color)
{
	float factor;

	color[3] = 0;
	if (r_newrefdef.fog.hf_start_dist < r_newrefdef.fog.hf_end_dist)
	{
		if (r_newrefdef.vieworg[2] >= r_newrefdef.fog.hf_end_dist ||
			r_newrefdef.vieworg[2] <= r_newrefdef.fog.hf_start_dist)
		{
			return;
		}
	}
	else
	{
		if (r_newrefdef.vieworg[2] <= r_newrefdef.fog.hf_end_dist ||
			r_newrefdef.vieworg[2] >= r_newrefdef.fog.hf_start_dist)
		{
			color[3] = 0;
			return;
		}
	}

	factor = (r_newrefdef.vieworg[2] - r_newrefdef.fog.hf_start_dist) /
		(r_newrefdef.fog.hf_end_dist - r_newrefdef.fog.hf_start_dist);

	factor = Q_clamp(factor, 0.0f, 1.0f);
	factor = pow(factor, r_newrefdef.fog.hf_falloff);
	color[0] = Q_lerp(r_newrefdef.fog.hf_start_r / 255.0f,
		r_newrefdef.fog.hf_end_r / 255.0f, factor);
	color[1] = Q_lerp(r_newrefdef.fog.hf_start_g / 255.0f,
		r_newrefdef.fog.hf_end_g / 255.0f, factor);
	color[2] = Q_lerp(r_newrefdef.fog.hf_start_b / 255.0f,
		r_newrefdef.fog.hf_end_b / 255.0f, factor);
	color[3] = r_newrefdef.fog.hf_density * factor;
}

static void
R_CombineBlends(const float *add, float *v_blend)
{
	if (add[3])
	{
		int i;

		v_blend[3] += add[3];
		for (i = 0; i < 3; i++)
		{
			v_blend[i] += add[i] * add[3];
		}
	}
}

void
R_CombineBlendWithFog(float *v_blend)
{
	float hf_color[4];
	int i;

	for (i = 0; i < 4; i++)
	{
		v_blend[i] = 0;
	}

	/* blend */
	R_CombineBlends(r_newrefdef.blend, v_blend);

	/* fog */
	if (r_newrefdef.fog.density)
	{
		float fog_density;

		fog_density = r_newrefdef.fog.density * 10;
		v_blend[3]  += fog_density;

		v_blend[0] += (r_newrefdef.fog.red / 255.0) * fog_density;
		v_blend[1] += (r_newrefdef.fog.green / 255.0) * fog_density;
		v_blend[2] += (r_newrefdef.fog.blue / 255.0) * fog_density;
	}

	/* height fog */
	R_GetHeightFog(hf_color);
	R_CombineBlends(hf_color, v_blend);

	if (v_blend[3])
	{
		for (i = 0; i < 3; i++)
		{
			v_blend[i] /= v_blend[3];
		}
	}

	for (i = 0; i < 4; i++)
	{
		if (v_blend[i] > 1.0)
		{
			v_blend[i] = 1.0;
		}
	}
}
