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

/* common r_* settings */
cvar_t *r_2D_unfiltered;
cvar_t *r_anisotropic;
cvar_t *r_clear;
cvar_t *r_cull;
cvar_t *r_customheight;
cvar_t *r_customwidth;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_dynamic;
cvar_t *r_farsee;
cvar_t *r_fixsurfsky;
cvar_t *r_flashblend;
cvar_t *r_fullbright;
cvar_t *r_gunfov;
cvar_t *r_lefthand;
cvar_t *r_lerp_list;
cvar_t *r_lerpmodels;
cvar_t *r_lightlevel; // FIXME: This is a HACK to get the client's light level
cvar_t *r_lightmap;
cvar_t *r_lockpvs;
cvar_t *r_mode;
cvar_t *r_modulate;
cvar_t *r_msaa_samples;
cvar_t *r_nolerp_list;
cvar_t *r_norefresh;
cvar_t *r_novis;
cvar_t *r_palettedtextures;
cvar_t *r_polyblend;
cvar_t *r_retexturing;
cvar_t *r_scale8bittextures;
cvar_t *r_shadows;
cvar_t *r_showtris;
cvar_t *r_speeds;
cvar_t *r_ttffont;
cvar_t *r_validation;
cvar_t *r_videos_unfiltered;
cvar_t *r_vsync;
cvar_t *vid_fullscreen;
cvar_t *vid_gamma;
cvar_t *viewsize;

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
R_CombineBlendWithFog(float *v_blend, qboolean native_fog)
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

		fog_density = r_newrefdef.fog.density * 5;
		if (!native_fog)
		{
			fog_density *= 2;
		}

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

void
R_InitCvar(void)
{
	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "11", CVAR_ARCHIVE);
	r_vsync = ri.Cvar_Get("r_vsync", "0", CVAR_ARCHIVE);
	r_gunfov = ri.Cvar_Get("r_gunfov", "80", CVAR_ARCHIVE);
	r_farsee = ri.Cvar_Get("r_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	r_validation = ri.Cvar_Get("r_validation", "0", CVAR_ARCHIVE);
	r_palettedtextures = ri.Cvar_Get("r_palettedtextures", "0", CVAR_ARCHIVE);
	r_flashblend = ri.Cvar_Get("r_flashblend", "0", 0);
	r_clear = ri.Cvar_Get("r_clear", "0", CVAR_ARCHIVE);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);
	r_polyblend = ri.Cvar_Get("r_polyblend", "1", 0);
	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	r_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	r_dynamic = ri.Cvar_Get("r_dynamic", "1", 0);
	r_msaa_samples = ri.Cvar_Get("r_msaa_samples", "0", CVAR_ARCHIVE);
	r_showtris = ri.Cvar_Get("r_showtris", "0", 0);
	r_lightmap = ri.Cvar_Get("r_lightmap", "0", 0);
	r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);
	r_scale8bittextures = ri.Cvar_Get("r_scale8bittextures", "0", CVAR_ARCHIVE);
	r_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
	/* don't bilerp characters and crosshairs */
	r_nolerp_list = ri.Cvar_Get("r_nolerp_list", DEFAULT_NOLERP_LIST, CVAR_ARCHIVE);
	/* textures that should always be filtered, even if r_2D_unfiltered or an unfiltered gl mode is used */
	r_lerp_list = ri.Cvar_Get("r_lerp_list", "", CVAR_ARCHIVE);
	/* don't bilerp any 2D elements */
	r_2D_unfiltered = ri.Cvar_Get("r_2D_unfiltered", "0", CVAR_ARCHIVE);
	/* don't bilerp videos */
	r_videos_unfiltered = ri.Cvar_Get("r_videos_unfiltered", "0", CVAR_ARCHIVE);
	r_fixsurfsky = ri.Cvar_Get("r_fixsurfsky", "0", CVAR_ARCHIVE);
	/* font should looks good with 8 pixels size */
	r_ttffont = ri.Cvar_Get("r_ttffont", "RussoOne-Regular", CVAR_ARCHIVE);
	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	viewsize = ri.Cvar_Get("viewsize", "100", CVAR_ARCHIVE);

	/* clamp r_msaa_samples to accepted range so that video menu doesn't crash on us */
	if (r_msaa_samples->value < 0)
	{
		ri.Cvar_Set("r_msaa_samples", "0");
	}

}
