/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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
 * Refresher setup and main part of the frame generation, for OpenGL3
 *
 * =======================================================================
 */

#include "../ref_shared.h"
#include "header/local.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "../files/HandmadeMath.h"

#define DG_DYNARR_IMPLEMENTATION
#include "../files/DG_dynarr.h"

#ifdef YQ2_GL3_GLES3
  #define REF_VERSION "Yamagi Quake II OpenGL ES3 Refresher"
#else
  #define REF_VERSION "Yamagi Quake II OpenGL3 Refresher"
#endif

refimport_t ri;

gl3config_t gl3config;
gl3state_t gl3state;

unsigned gl3_rawpalette[256];

gl3model_t *gl3_worldmodel;

float gl3depthmin=0.0f, gl3depthmax=1.0f;

cplane_t frustum[4];

/* view origin */
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t gl3_origin;

int gl3_visframecount; /* bumped when going to a new PVS */
int gl3_framecount; /* used for dlight push checking */

int c_brush_polys, c_alias_polys;

static float v_blend[4]; /* final blending color */

int gl3_viewcluster, gl3_viewcluster2, gl3_oldviewcluster, gl3_oldviewcluster2;

const hmm_mat4 gl3_identityMat4 = {{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1},
}};

cvar_t *gl_msaa_samples;
cvar_t *gl_version_override;
cvar_t *r_vsync;
cvar_t *r_retexturing;
cvar_t *r_scale8bittextures;
cvar_t *vid_fullscreen;
cvar_t *r_mode;
cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *vid_gamma;
cvar_t *gl_anisotropic;
cvar_t *gl_texturemode;
cvar_t *gl_drawbuffer;
cvar_t *r_clear;
cvar_t *gl3_particle_size;
cvar_t *gl3_particle_fade_factor;
cvar_t *gl3_particle_square;
cvar_t *gl3_colorlight;
cvar_t *gl_polyblend;

cvar_t *gl_lefthand;
cvar_t *r_gunfov;
cvar_t *r_farsee;

cvar_t *gl3_intensity;
cvar_t *gl3_intensity_2D;
cvar_t *r_lightlevel;
cvar_t *gl3_overbrightbits;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_nolerp_list;
cvar_t *r_lerp_list;
cvar_t *r_2D_unfiltered;
cvar_t *r_videos_unfiltered;
cvar_t *gl_nobind;
cvar_t *r_lockpvs;
cvar_t *r_novis;
cvar_t *r_speeds;
cvar_t *gl_finish;

cvar_t *r_cull;
cvar_t *gl_zfix;
cvar_t *r_fullbright;
cvar_t *r_modulate;
cvar_t *gl_lightmap;
cvar_t *gl_shadows;
cvar_t *gl3_debugcontext;
cvar_t *gl3_usebigvbo;
cvar_t *r_fixsurfsky;
cvar_t *r_ttffont;
cvar_t *r_palettedtexture;
cvar_t *r_validation;
cvar_t *gl3_usefbo;

static cvar_t *gl_znear;

// Yaw-Pitch-Roll
// equivalent to R_z * R_y * R_x where R_x is the trans matrix for rotating around X axis for aroundXdeg
static hmm_mat4 rotAroundAxisZYX(float aroundZdeg, float aroundYdeg, float aroundXdeg)
{
	// Naming of variables is consistent with http://planning.cs.uiuc.edu/node102.html
	// and https://de.wikipedia.org/wiki/Roll-Nick-Gier-Winkel#.E2.80.9EZY.E2.80.B2X.E2.80.B3-Konvention.E2.80.9C
	float alpha = HMM_ToRadians(aroundZdeg);
	float beta = HMM_ToRadians(aroundYdeg);
	float gamma = HMM_ToRadians(aroundXdeg);

	float sinA = HMM_SinF(alpha);
	float cosA = HMM_CosF(alpha);
	// TODO: or sincosf(alpha, &sinA, &cosA); ?? (not a standard function)
	float sinB = HMM_SinF(beta);
	float cosB = HMM_CosF(beta);
	float sinG = HMM_SinF(gamma);
	float cosG = HMM_CosF(gamma);

	hmm_mat4 ret = {{
		{ cosA*cosB,                  sinA*cosB,                   -sinB,    0 }, // first *column*
		{ cosA*sinB*sinG - sinA*cosG, sinA*sinB*sinG + cosA*cosG, cosB*sinG, 0 },
		{ cosA*sinB*cosG + sinA*sinG, sinA*sinB*cosG - cosA*sinG, cosB*cosG, 0 },
		{  0,                          0,                          0,        1 }
	}};

	return ret;
}

void
GL3_RotateForEntity(entity_t *e)
{
	// angles: pitch (around y), yaw (around z), roll (around x)
	// rot matrices to be multiplied in order Z, Y, X (yaw, pitch, roll)
	hmm_mat4 transMat = rotAroundAxisZYX(e->angles[1], -e->angles[0], -e->angles[2]);

	for(int i=0; i<3; ++i)
	{
		transMat.Elements[3][i] = e->origin[i]; // set translation
	}

	gl3state.uni3DData.transModelMat4 = HMM_MultiplyMat4(gl3state.uni3DData.transModelMat4, transMat);

	GL3_UpdateUBO3D();
}


static void
GL3_Strings(void)
{
	GLint i, numExtensions;
	Com_Printf("GL_VENDOR: %s\n", gl3config.vendor_string);
	Com_Printf("GL_RENDERER: %s\n", gl3config.renderer_string);
	Com_Printf("GL_VERSION: %s\n", gl3config.version_string);
	Com_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", gl3config.glsl_version_string);

	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	Com_Printf("GL_EXTENSIONS:");
	for(i = 0; i < numExtensions; i++)
	{
		Com_Printf(" %s", (const char*)glGetStringi(GL_EXTENSIONS, i));
	}
	Com_Printf("\n");
}

static void
GL3_Register(void)
{
	gl_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_gunfov = ri.Cvar_Get("r_gunfov", "80", CVAR_ARCHIVE);
	r_farsee = ri.Cvar_Get("r_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);

	gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	r_vsync = ri.Cvar_Get("r_vsync", "1", CVAR_ARCHIVE);
	gl_msaa_samples = ri.Cvar_Get ( "r_msaa_samples", "0", CVAR_ARCHIVE );
	gl_version_override = ri.Cvar_Get ( "gl_version_override", "0", CVAR_ARCHIVE );
	r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);
	r_scale8bittextures = ri.Cvar_Get("r_scale8bittextures", "0", CVAR_ARCHIVE);
	gl3_debugcontext = ri.Cvar_Get("gl3_debugcontext", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE);
	r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	gl3_particle_size = ri.Cvar_Get("gl3_particle_size", "40", CVAR_ARCHIVE);
	gl3_particle_fade_factor = ri.Cvar_Get("gl3_particle_fade_factor", "1.2", CVAR_ARCHIVE);
	gl3_particle_square = ri.Cvar_Get("gl3_particle_square", "0", CVAR_ARCHIVE);
	// if set to 0, lights (from lightmaps, dynamic lights and on models) are white instead of colored
	gl3_colorlight = ri.Cvar_Get("gl3_colorlight", "1", CVAR_ARCHIVE);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", CVAR_ARCHIVE);

	//  0: use lots of calls to glBufferData()
	//  1: reduce calls to glBufferData() with one big VBO (see GL3_BufferAndDraw3D())
	// -1: auto (let yq2 choose to enable/disable this based on detected driver)
	gl3_usebigvbo = ri.Cvar_Get("gl3_usebigvbo", "-1", CVAR_ARCHIVE);

	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	/* font should looks good with 8 pixels size */
	r_ttffont = ri.Cvar_Get("r_ttffont", "RussoOne-Regular", CVAR_ARCHIVE);
	r_fixsurfsky = ri.Cvar_Get("r_fixsurfsky", "0", CVAR_ARCHIVE);
	r_palettedtexture = ri.Cvar_Get("r_palettedtexture", "0", 0);
	r_validation = ri.Cvar_Get("r_validation", "0", CVAR_ARCHIVE);

	/* don't bilerp characters and crosshairs */
	r_nolerp_list = ri.Cvar_Get("r_nolerp_list", DEFAULT_NOLERP_LIST, CVAR_ARCHIVE);
	/* textures that should always be filtered, even if r_2D_unfiltered or an unfiltered gl mode is used */
	r_lerp_list = ri.Cvar_Get("r_lerp_list", "", CVAR_ARCHIVE);
	/* don't bilerp any 2D elements */
	r_2D_unfiltered = ri.Cvar_Get("r_2D_unfiltered", "0", CVAR_ARCHIVE);
	/* don't bilerp videos */
	r_videos_unfiltered = ri.Cvar_Get("r_videos_unfiltered", "0", CVAR_ARCHIVE);
	gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);

	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.2", CVAR_ARCHIVE);
	gl3_intensity = ri.Cvar_Get("gl3_intensity", "1.5", CVAR_ARCHIVE);
	gl3_intensity_2D = ri.Cvar_Get("gl3_intensity_2D", "1.5", CVAR_ARCHIVE);

	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	gl3_overbrightbits = ri.Cvar_Get("gl3_overbrightbits", "1.3", CVAR_ARCHIVE);

	gl_lightmap = ri.Cvar_Get("r_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);

	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", 0);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	gl_znear = ri.Cvar_Get("gl_znear", "4", CVAR_ARCHIVE);

	gl3_usefbo = ri.Cvar_Get("gl3_usefbo", "1", CVAR_ARCHIVE); // use framebuffer object for postprocess effects (water)

#if 0 // TODO!
	//gl_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	//gl_farsee = ri.Cvar_Get("gl_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	//r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	//r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	//r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	//r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	//r_novis = ri.Cvar_Get("r_novis", "0", 0);
	//r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0); NOTE: screw this, it looks horrible without
	//r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	//r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	//gl_overbrightbits = ri.Cvar_Get("gl_overbrightbits", "0", CVAR_ARCHIVE);

	gl1_particle_min_size = ri.Cvar_Get("gl1_particle_min_size", "2", CVAR_ARCHIVE);
	gl1_particle_max_size = ri.Cvar_Get("gl1_particle_max_size", "40", CVAR_ARCHIVE);
	//gl1_particle_size = ri.Cvar_Get("gl1_particle_size", "40", CVAR_ARCHIVE);
	gl1_particle_att_a = ri.Cvar_Get("gl1_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl1_particle_att_b = ri.Cvar_Get("gl1_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl1_particle_att_c = ri.Cvar_Get("gl1_particle_att_c", "0.01", CVAR_ARCHIVE);

	//gl_modulate = ri.Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
	//r_mode = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE);
	//gl_lightmap = ri.Cvar_Get("r_lightmap", "0", 0);
	//gl_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	//gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
	r_showtris = ri.Cvar_Get("r_showtris", "0", 0);
	gl_showbbox = Cvar_Get("gl_showbbox", "0", 0);
	//gl1_ztrick = ri.Cvar_Get("gl1_ztrick", "0", 0); NOTE: dump this.
	//gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	//gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	//r_flashblend = ri.Cvar_Get("r_flashblend", "0", 0);

	//gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl1_texturealphamode = ri.Cvar_Get("gl1_texturealphamode", "default", CVAR_ARCHIVE);
	gl1_texturesolidmode = ri.Cvar_Get("gl1_texturesolidmode", "default", CVAR_ARCHIVE);
	//gl_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
	//r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);

	//gl1_palettedtexture = ri.Cvar_Get("gl1_palettedtexture", "0", CVAR_ARCHIVE); NOPE.
	gl1_pointparameters = ri.Cvar_Get("gl1_pointparameters", "1", CVAR_ARCHIVE);

	//gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	//r_vsync = ri.Cvar_Get("r_vsync", "1", CVAR_ARCHIVE);


	//vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	//vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);

	//r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	//r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	//gl_msaa_samples = ri.Cvar_Get ( "r_msaa_samples", "0", CVAR_ARCHIVE );

	gl1_stereo = ri.Cvar_Get( "gl1_stereo", "0", CVAR_ARCHIVE );
	gl1_stereo_separation = ri.Cvar_Get( "gl1_stereo_separation", "-0.4", CVAR_ARCHIVE );
	gl1_stereo_anaglyph_colors = ri.Cvar_Get( "gl1_stereo_anaglyph_colors", "rc", CVAR_ARCHIVE );
	gl1_stereo_convergence = ri.Cvar_Get( "gl1_stereo_convergence", "1", CVAR_ARCHIVE );
#endif // 0

	ri.Cmd_AddCommand("imagelist", GL3_ImageList_f);
	ri.Cmd_AddCommand("screenshot", GL3_ScreenShot);
	ri.Cmd_AddCommand("modellist", GL3_Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", GL3_Strings);
}

/*
 * Changes the video mode
 */

static int
SetMode_impl(int *pwidth, int *pheight, int mode, int fullscreen)
{
	Com_Printf("Setting mode %d:", mode);

	/* mode -1 is not in the vid mode table - so we keep the values in pwidth
	   and pheight and don't even try to look up the mode info */
	if ((mode >= 0) && !ri.Vid_GetModeInfo(pwidth, pheight, mode))
	{
		Com_Printf(" invalid mode\n");
		return rserr_invalid_mode;
	}

	/* We trying to get resolution from desktop */
	if (mode == -2)
	{
		if(!ri.GLimp_GetDesktopMode(pwidth, pheight))
		{
			Com_Printf(" can't detect mode\n" );
			return rserr_invalid_mode;
		}
	}

	Com_Printf(" %dx%d (vid_fullscreen %i)\n", *pwidth, *pheight, fullscreen);

	if (!ri.GLimp_InitGraphics(fullscreen, pwidth, pheight))
	{
		return rserr_invalid_mode;
	}

	if (mode == -2 || fullscreen)
	{
		GL3_BindVBO(0);
	}

	/* This is totaly obscure: For some strange reasons the renderer
	   maintains two(!) repesentations of the resolution. One comes
	   from the client and is saved in r_newrefdef. The other one
	   is determined here and saved in vid. Several calculations take
	   both representations into account.

	   The values will always be the same. The GLimp_InitGraphics()
	   call above communicates the requested resolution to the client
	   where it ends up in the vid subsystem and the vid system writes
	   it into r_newrefdef.

	   We can't avoid the client roundtrip, because we can get the
	   real size of the drawable (which can differ from the resolution
	   due to high dpi awareness) only after the render context was
	   created by GLimp_InitGraphics() and need to communicate it
	   somehow to the client. So we just overwrite the values saved
	   in vid with a call to GL3_GetDrawableSize(), just like the
	   client does. This makes sure that both values are the same
	   and everything is okay.

	   We also need to take the special case fullscreen window into
	   account. With the fullscreen windows we cannot use the
	   drawable size, it would scale all cases to the size of the
	   window. Instead use the drawable size when the user wants
	   native resolution (the fullscreen window fills the screen)
	   and use the requested resolution in all other cases. */
	if (IsHighDPIaware)
	{
		if (vid_fullscreen->value != 2)
		{
			GL3_GetDrawableSize(pwidth, pheight);
		}
		else
		{
			if (r_mode->value == -2)
			{
				/* User requested native resolution. */
				GL3_GetDrawableSize(pwidth, pheight);
			}
		}
	}

	return rserr_ok;
}

static qboolean
GL3_SetMode(void)
{
	int err;
	int fullscreen;

	fullscreen = (int)vid_fullscreen->value;

	/* a bit hackish approach to enable custom resolutions:
	   Glimp_SetMode needs these values set for mode -1 */
	vid.width = r_customwidth->value;
	vid.height = r_customheight->value;

	if ((err = SetMode_impl(&vid.width, &vid.height, r_mode->value, fullscreen)) == rserr_ok)
	{
		if (r_mode->value == -1)
		{
			gl3state.prev_mode = 4; /* safe default for custom mode */
		}
		else
		{
			gl3state.prev_mode = r_mode->value;
		}
	}
	else
	{
		if (err == rserr_invalid_mode)
		{
			Com_Printf("ref_gl3::GL3_SetMode() - invalid mode\n");

			if (gl_msaa_samples->value != 0.0f)
			{
				Com_Printf("gl_msaa_samples was %d - will try again with gl_msaa_samples = 0\n", (int)gl_msaa_samples->value);
				ri.Cvar_SetValue("r_msaa_samples", 0.0f);
				gl_msaa_samples->modified = false;

				if ((err = SetMode_impl(&vid.width, &vid.height, r_mode->value, 0)) == rserr_ok)
				{
					return true;
				}
			}
			if(r_mode->value == gl3state.prev_mode)
			{
				// trying again would result in a crash anyway, give up already
				// (this would happen if your initing fails at all and your resolution already was 640x480)
				return false;
			}

			ri.Cvar_SetValue("r_mode", gl3state.prev_mode);
			r_mode->modified = false;
		}

		/* try setting it back to something safe */
		if ((err = SetMode_impl(&vid.width, &vid.height, gl3state.prev_mode, 0)) != rserr_ok)
		{
			Com_Printf("ref_gl3::GL3_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}

	return true;
}

// only needed (and allowed!) if using OpenGL compatibility profile, it's not in 3.2 core
enum { QGL_POINT_SPRITE = 0x8861 };

static qboolean
GL3_Init(void)
{
	Swap_Init(); // FIXME: for fucks sake, this doesn't have to be done at runtime!

	Com_Printf("Refresh: " REF_VERSION "\n");
	Com_Printf("Client: " YQ2VERSION "\n\n");

	if(sizeof(float) != sizeof(GLfloat))
	{
		// if this ever happens, things would explode because we feed vertex arrays and UBO data
		// using floats to OpenGL, which expects GLfloat (can't easily change, those floats are from HMM etc)
		// (but to be honest I very much doubt this will ever happen.)
		Com_Printf("ref_gl3: sizeof(float) != sizeof(GLfloat) - we're in real trouble here.\n");
		return false;
	}

	ri.VID_GetPalette(NULL, d_8to24table);

	GL3_Register();

	/* set our "safe" mode */
	gl3state.prev_mode = 4;
	//gl_state.stereo_mode = gl1_stereo->value;

	/* create the window and set up the context */
	if (!GL3_SetMode())
	{
		Com_Printf("ref_gl3::R_Init() - could not R_SetMode()\n");
		return false;
	}

	ri.Vid_MenuInit();

	/* get our various GL strings */
	gl3config.vendor_string = (const char*)glGetString(GL_VENDOR);
	gl3config.renderer_string = (const char*)glGetString(GL_RENDERER);
	gl3config.version_string = (const char*)glGetString(GL_VERSION);
	gl3config.glsl_version_string = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	Com_Printf("\nOpenGL setting:\n");
	GL3_Strings();

	Com_Printf("\n\nProbing for OpenGL extensions:\n");


	/* Anisotropic */
	Com_Printf(" - Anisotropic Filtering: ");

	if(gl3config.anisotropic)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl3config.max_anisotropy);

		Com_Printf("Max level: %ux\n", (int)gl3config.max_anisotropy);
	}
	else
	{
		gl3config.max_anisotropy = 0.0;

		Com_Printf("Not supported\n");
	}

	if(gl3config.debug_output)
	{
		Com_Printf(" - OpenGL Debug Output: Supported ");
		if(gl3_debugcontext->value == 0.0f)
		{
			Com_Printf("(but disabled with gl3_debugcontext = 0)\n");
		}
		else
		{
			Com_Printf("and enabled with gl3_debugcontext = %i\n", (int)gl3_debugcontext->value);
		}
	}
	else
	{
		Com_Printf(" - OpenGL Debug Output: Not Supported\n");
	}

	gl3config.useBigVBO = false;
	if(gl3_usebigvbo->value == 1.0f)
	{
		Com_Printf("Enabling useBigVBO workaround because gl3_usebigvbo = 1\n");
		gl3config.useBigVBO = true;
	}
	else if(gl3_usebigvbo->value == -1.0f)
	{
		// enable for AMDs proprietary Windows and Linux drivers
#ifdef _WIN32
		if(gl3config.version_string != NULL && gl3config.vendor_string != NULL
		   && strstr(gl3config.vendor_string, "ATI Technologies Inc") != NULL)
		{
			int a, b, ver;
			if(sscanf(gl3config.version_string, " %d.%d.%d ", &a, &b, &ver) >= 3 && ver >= 13431)
			{
				// turns out the legacy driver is a lot faster *without* the workaround :-/
				// GL_VERSION for legacy 16.2.1 Beta driver: 3.2.13399 Core Profile Forward-Compatible Context 15.200.1062.1004
				//            (this is the last version that supports the Radeon HD 6950)
				// GL_VERSION for (non-legacy) 16.3.1 driver on Radeon R9 200: 4.5.13431 Compatibility Profile Context 16.150.2111.0
				// GL_VERSION for non-legacy 17.7.2 WHQL driver: 4.5.13491 Compatibility Profile/Debug Context 22.19.662.4
				// GL_VERSION for 18.10.1 driver: 4.6.13541 Compatibility Profile/Debug Context 25.20.14003.1010
				// GL_VERSION for (current) 19.3.2 driver: 4.6.13547 Compatibility Profile/Debug Context 25.20.15027.5007
				// (the 3.2/4.5/4.6 can probably be ignored, might depend on the card and what kind of context was requested
				//  but AFAIK the number behind that can be used to roughly match the driver version)
				// => let's try matching for x.y.z with z >= 13431
				// (no, I don't feel like testing which release since 16.2.1 has introduced the slowdown.)
				Com_Printf("Detected AMD Windows GPU driver, enabling useBigVBO workaround\n");
				gl3config.useBigVBO = true;
			}
		}
#elif defined(__linux__)
		if(gl3config.vendor_string != NULL && strstr(gl3config.vendor_string, "Advanced Micro Devices, Inc.") != NULL)
		{
			Com_Printf("Detected proprietary AMD GPU driver, enabling useBigVBO workaround\n");
			Com_Printf("(consider using the open source RadeonSI drivers, they tend to work better overall)\n");
			gl3config.useBigVBO = true;
		}
#endif
	}

	// generate texture handles for all possible lightmaps
	glGenTextures(MAX_LIGHTMAPS*MAX_LIGHTMAPS_PER_SURFACE, gl3state.lightmap_textureIDs[0]);

	GL3_SetDefaultState();

	if(GL3_InitShaders())
	{
		Com_Printf("Loading shaders succeeded.\n");
	}
	else
	{
		Com_Printf("Loading shaders failed.\n");
		return false;
	}

	registration_sequence = 1; // from R_InitImages() (everything else from there shouldn't be needed anymore)

	R_VertBufferInit();

	GL3_Mod_Init();

	GL3_InitParticleTexture();

	GL3_Draw_InitLocal();

	GL3_SurfInit();

	glGenFramebuffers(1, &gl3state.ppFBO);
	// the rest for the FBO is done dynamically in GL3_RenderView() so it can
	// take the viewsize into account (enforce that by setting invalid size)
	gl3state.ppFBtexWidth = gl3state.ppFBtexHeight = -1;

	Com_Printf("\n");
	return true;
}

void
GL3_Shutdown(void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	// only call all these if we have an OpenGL context and the gl function pointers
	// randomly chose one function that should always be there to test..
	if(glDeleteBuffers != NULL)
	{
		GL3_Mod_FreeAll();
		GL3_ShutdownMeshes();
		GL3_ShutdownImages();
		R_VertBufferFree();
		GL3_SurfShutdown();
		GL3_Draw_ShutdownLocal();
		GL3_ShutdownShaders();

		// free the postprocessing FBO and its renderbuffer and texture
		if(gl3state.ppFBrbo != 0)
			glDeleteRenderbuffers(1, &gl3state.ppFBrbo);
		if(gl3state.ppFBtex != 0)
			glDeleteTextures(1, &gl3state.ppFBtex);
		if(gl3state.ppFBO != 0)
			glDeleteFramebuffers(1, &gl3state.ppFBO);
		gl3state.ppFBrbo = gl3state.ppFBtex = gl3state.ppFBO = 0;
		gl3state.ppFBObound = false;
		gl3state.ppFBtexWidth = gl3state.ppFBtexHeight = -1;
	}

	/* shutdown OS specific OpenGL stuff like contexts, etc.  */
	GL3_ShutdownContext();
}

// assumes gl3state.v[ab]o3D are bound
// buffers and draws mvtx_t vertices
// drawMode is something like GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN or whatever
void
GL3_BufferAndDraw3D(const mvtx_t* verts, int numVerts, GLenum drawMode)
{
	if(!gl3config.useBigVBO)
	{
		glBufferData( GL_ARRAY_BUFFER, sizeof(mvtx_t)*numVerts, verts, GL_STREAM_DRAW );
		glDrawArrays( drawMode, 0, numVerts );
	}
	else // gl3config.useBigVBO == true
	{
		/*
		 * For some reason, AMD's Windows driver doesn't seem to like lots of
		 * calls to glBufferData() (some of them seem to take very long then).
		 * GL3_BufferAndDraw3D() is called a lot when drawing world geometry
		 * (once for each visible face I think?).
		 * The simple code above caused noticeable slowdowns - even a fast
		 * quadcore CPU and a Radeon RX580 weren't able to maintain 60fps..
		 * The workaround is to not call glBufferData() with small data all the time,
		 * but to allocate a big buffer and on each call to GL3_BufferAndDraw3D()
		 * to use a different region of that buffer, resulting in a lot less calls
		 * to glBufferData() (=> a lot less buffer allocations in the driver).
		 * Only when the buffer is full and at the end of a frame (=> GL3_EndFrame())
		 * we get a fresh buffer.
		 *
		 * BTW, we couldn't observe this kind of problem with any other driver:
		 * Neither nvidias driver, nor AMDs or Intels Open Source Linux drivers,
		 * not even Intels Windows driver seem to care that much about the
		 * glBufferData() calls.. However, at least nvidias driver doesn't like
		 * this workaround (with glMapBufferRange()), the framerate dropped
		 * significantly - that's why both methods are available and
		 * selectable at runtime.
		 */
#if 0
		// I /think/ doing it with glBufferSubData() didn't really help
		const int bufSize = gl3state.vbo3Dsize;
		int neededSize = numVerts*sizeof(mvtx_t);
		int curOffset = gl3state.vbo3DcurOffset;
		if(curOffset + neededSize > gl3state.vbo3Dsize)
			curOffset = 0;
		int curIdx = curOffset / sizeof(mvtx_t);

		gl3state.vbo3DcurOffset = curOffset + neededSize;

		glBufferSubData( GL_ARRAY_BUFFER, curOffset, neededSize, verts );
		glDrawArrays( drawMode, curIdx, numVerts );
#else
		int curOffset = gl3state.vbo3DcurOffset;
		int neededSize = numVerts*sizeof(mvtx_t);
		if(curOffset+neededSize > gl3state.vbo3Dsize)
		{
			// buffer is full, need to start again from the beginning
			// => need to sync or get fresh buffer
			// (getting fresh buffer seems easier)
			glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW);
			curOffset = 0;
		}

		// as we make sure to use a previously unused part of the buffer,
		// doing it unsynchronized should be safe..
		GLbitfield accessBits = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
		void* data = glMapBufferRange(GL_ARRAY_BUFFER, curOffset, neededSize, accessBits);
		memcpy(data, verts, neededSize);
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glDrawArrays(drawMode, curOffset/sizeof(mvtx_t), numVerts);

		gl3state.vbo3DcurOffset = curOffset + neededSize; // TODO: padding or sth needed?
#endif
	}
}

static void
GL3_DrawBeam(entity_t *e)
{
	int i;
	float r, g, b;

	enum { NUM_BEAM_SEGS = 6 };

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	mvtx_t verts[NUM_BEAM_SEGS*4];
	unsigned int pointb;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
	{
		return;
	}

	PerpendicularVector(perpvec, normalized_direction);
	VectorScale(perpvec, e->frame / 2, perpvec);

	for (i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec,
		                        (360.0 / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], origin, start_points[i]);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	GL3_UseProgram(gl3state.si3DcolorOnly.shaderProgram);

	r = (LittleLong(d_8to24table[e->skinnum & 0xFF])) & 0xFF;
	g = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 8) & 0xFF;
	b = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 16) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	gl3state.uniCommonData.color = HMM_Vec4(r, g, b, e->alpha);
	GL3_UpdateUBOCommon();

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		VectorCopy(start_points[i], verts[4*i+0].pos);
		VectorCopy(end_points[i], verts[4*i+1].pos);

		pointb = ( i + 1 ) % NUM_BEAM_SEGS;

		VectorCopy(start_points[pointb], verts[4*i+2].pos);
		VectorCopy(end_points[pointb], verts[4*i+3].pos);
	}

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	GL3_BufferAndDraw3D(verts, NUM_BEAM_SEGS*4, GL_TRIANGLE_STRIP);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

static void
GL3_DrawSpriteModel(entity_t *e, const gl3model_t *currentmodel)
{
	float alpha = 1.0F;
	mvtx_t verts[4];
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;
	gl3image_t *skin = NULL;
	vec3_t scale;

	VectorCopy(e->scale, scale);

	/* don't even bother culling, because it's just
	   a single polygon without a surface cache */
	psprite = (dsprite_t *)currentmodel->extradata;

	e->frame %= psprite->numframes;
	frame = &psprite->frames[e->frame];

	/* normal sprite */
	up = vup;
	right = vright;

	if (e->flags & RF_TRANSLUCENT)
	{
		alpha = e->alpha;
	}

	if (alpha != gl3state.uni3DData.alpha)
	{
		gl3state.uni3DData.alpha = alpha;
		GL3_UpdateUBO3D();
	}

	skin = currentmodel->skins[e->frame];
	if (!skin)
	{
		skin = gl3_notexture; /* fallback... */
	}

	GL3_Bind(skin->texnum);

	if (e->flags & RF_FLARE)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		GL3_UseProgram(gl3state.si3Dsprite.shaderProgram);
	}
	else
	{
		if (alpha == 1.0)
		{
			// use shader with alpha test
			GL3_UseProgram(gl3state.si3DspriteAlpha.shaderProgram);
		}
		else
		{
			glEnable(GL_BLEND);

			GL3_UseProgram(gl3state.si3Dsprite.shaderProgram);
		}
	}

	verts[0].texCoord[0] = 0;
	verts[0].texCoord[1] = 1;
	verts[1].texCoord[0] = 0;
	verts[1].texCoord[1] = 0;
	verts[2].texCoord[0] = 1;
	verts[2].texCoord[1] = 0;
	verts[3].texCoord[0] = 1;
	verts[3].texCoord[1] = 1;

	VectorMA( e->origin, -frame->origin_y * scale[0], up, verts[0].pos );
	VectorMA( verts[0].pos, -frame->origin_x * scale[1], right, verts[0].pos );

	VectorMA( e->origin, (frame->height - frame->origin_y) * scale[0], up, verts[1].pos );
	VectorMA( verts[1].pos, -frame->origin_x * scale[1], right, verts[1].pos );

	VectorMA( e->origin, (frame->height - frame->origin_y) * scale[0], up, verts[2].pos );
	VectorMA( verts[2].pos, (frame->width - frame->origin_x) * scale[1], right, verts[2].pos );

	VectorMA( e->origin, -frame->origin_y * scale[0], up, verts[3].pos );
	VectorMA( verts[3].pos, (frame->width - frame->origin_x) * scale[1], right, verts[3].pos );

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	GL3_BufferAndDraw3D(verts, 4, GL_TRIANGLE_FAN);

	if (e->flags & RF_FLARE)
	{
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (alpha != 1.0F)
		{
			gl3state.uni3DData.alpha = 1.0f;
			GL3_UpdateUBO3D();
		}
	}
	else
	{
		if (alpha != 1.0F)
		{
			glDisable(GL_BLEND);
			gl3state.uni3DData.alpha = 1.0f;
			GL3_UpdateUBO3D();
		}
	}
}

static void
GL3_DrawNullModel(entity_t *currententity)
{
	vec3_t shadelight;

	if (currententity->flags & RF_FULLBRIGHT || !gl3_worldmodel || !gl3_worldmodel->lightdata)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	}
	else
	{
		R_LightPoint(gl3_worldmodel->grid, currententity,
			gl3_worldmodel->surfaces, gl3_worldmodel->nodes, currententity->origin,
			shadelight, r_modulate->value, lightspot);
	}

	hmm_mat4 origModelMat = gl3state.uni3DData.transModelMat4;
	GL3_RotateForEntity(currententity);

	gl3state.uniCommonData.color = HMM_Vec4( shadelight[0], shadelight[1], shadelight[2], 1 );
	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si3DcolorOnly.shaderProgram);

	GL3_BindVAO(gl3state.vao3D);
	GL3_BindVBO(gl3state.vbo3D);

	mvtx_t vtxA[6] = {
		{{0, 0, -16}, {0,0}, {0,0}},
		{{16 * cos( 0 * M_PI / 2 ), 16 * sin( 0 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 1 * M_PI / 2 ), 16 * sin( 1 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 2 * M_PI / 2 ), 16 * sin( 2 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 3 * M_PI / 2 ), 16 * sin( 3 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 4 * M_PI / 2 ), 16 * sin( 4 * M_PI / 2 ), 0}, {0,0}, {0,0}}
	};

	GL3_BufferAndDraw3D(vtxA, 6, GL_TRIANGLE_FAN);

	mvtx_t vtxB[6] = {
		{{0, 0, 16}, {0,0}, {0,0}},
		vtxA[5], vtxA[4], vtxA[3], vtxA[2], vtxA[1]
	};

	GL3_BufferAndDraw3D(vtxB, 6, GL_TRIANGLE_FAN);

	gl3state.uni3DData.transModelMat4 = origModelMat;
	GL3_UpdateUBO3D();
}

static void
GL3_DrawParticles(void)
{
	// TODO: stereo
	//qboolean stereo_split_tb = ((gl_state.stereo_mode == STEREO_SPLIT_VERTICAL) && gl_state.camera_separation);
	//qboolean stereo_split_lr = ((gl_state.stereo_mode == STEREO_SPLIT_HORIZONTAL) && gl_state.camera_separation);

	//if (!(stereo_split_tb || stereo_split_lr))
	{
		int i;
		int numParticles = r_newrefdef.num_particles;
		YQ2_ALIGNAS_TYPE(unsigned) byte color[4];
		const particle_t *p;
		// assume the size looks good with window height 480px and scale according to real resolution
		float pointSize = gl3_particle_size->value * (float)r_newrefdef.height/480.0f;

		typedef struct part_vtx {
			GLfloat pos[3];
			GLfloat size;
			GLfloat dist;
			GLfloat color[4];
		} part_vtx;
		YQ2_STATIC_ASSERT(sizeof(part_vtx)==9*sizeof(float), "invalid part_vtx size"); // remember to update GL3_SurfInit() if this changes!

		// Don't try to draw particles if there aren't any.
		if (numParticles == 0)
		{
			return;
		}

		YQ2_VLA(part_vtx, buf, numParticles);

		// TODO: viewOrg could be in UBO
		vec3_t viewOrg;
		VectorCopy(r_newrefdef.vieworg, viewOrg);

		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);

#ifdef YQ2_GL3_GLES
		// the RPi4 GLES3 implementation doesn't draw particles if culling is
		// enabled (at least with GL_FRONT which seems to be default in q2?)
		glDisable(GL_CULL_FACE);
#else
		// GLES doesn't have this, maybe it's always enabled? (https://gamedev.stackexchange.com/a/15528 says it works)
		// luckily we don't use glPointSize() but set gl_PointSize in shader anyway
		glEnable(GL_PROGRAM_POINT_SIZE);
#endif

		GL3_UseProgram(gl3state.siParticle.shaderProgram);

		for ( i = 0, p = r_newrefdef.particles; i < numParticles; i++, p++ )
		{
			*(int *) color = p->color;
			part_vtx* cur = &buf[i];
			vec3_t offset; // between viewOrg and particle position
			VectorSubtract(viewOrg, p->origin, offset);

			VectorCopy(p->origin, cur->pos);
			cur->size = pointSize;
			cur->dist = VectorLength(offset);

			for(int j=0; j<3; ++j)  cur->color[j] = color[j]*(1.0f/255.0f);

			cur->color[3] = p->alpha;
		}

		GL3_BindVAO(gl3state.vaoParticle);
		GL3_BindVBO(gl3state.vboParticle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(part_vtx)*numParticles, buf, GL_STREAM_DRAW);
		glDrawArrays(GL_POINTS, 0, numParticles);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
#ifdef YQ2_GL3_GLES
		if(r_cull->value != 0.0f)
			glEnable(GL_CULL_FACE);
#else
		glDisable(GL_PROGRAM_POINT_SIZE);
#endif

		YQ2_VLAFREE(buf);
	}
}

static void
GL3_DrawEntitiesOnList(void)
{
	qboolean translucent_entities = false;
	int i;

	if (!r_drawentities->value)
	{
		return;
	}

	GL3_ResetShadowAliasModels();

	/* draw non-transparent first */
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &r_newrefdef.entities[i];

		if (currententity->flags & (RF_TRANSLUCENT | RF_FLARE))
		{
			translucent_entities = true;
			continue; /* not solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			GL3_DrawBeam(currententity);
		}
		else
		{
			gl3model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				GL3_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					GL3_DrawAliasModel(currententity);
					break;
				case mod_brush:
					GL3_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					GL3_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					Com_Printf("%s: Bad modeltype %d\n",
						__func__, currentmodel->type);
					break;
			}
		}
	}

	if (!translucent_entities)
	{
		return;
	}

	/* draw transparent entities
	   we could sort these if it ever
	   becomes a problem... */
	glDepthMask(GL_FALSE);

	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &r_newrefdef.entities[i];

		if (!(currententity->flags & (RF_TRANSLUCENT | RF_FLARE)))
		{
			continue; /* solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			GL3_DrawBeam(currententity);
		}
		else
		{
			gl3model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				GL3_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					GL3_DrawAliasModel(currententity);
					break;
				case mod_brush:
					GL3_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					GL3_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					Com_Printf("%s: Bad modeltype %d\n",
						__func__, currentmodel->type);
					break;
			}
		}
	}

	GL3_DrawAliasShadows();

	glDepthMask(GL_TRUE); /* back to writing */
}

static void
SetupFrame(void)
{
	int i;
	mleaf_t *leaf;

	gl3_framecount++;

	/* build the transformation matrix for the given view angles */
	VectorCopy(r_newrefdef.vieworg, gl3_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	/* current viewcluster */
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		if (!gl3_worldmodel)
		{
			Com_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		gl3_oldviewcluster = gl3_viewcluster;
		gl3_oldviewcluster2 = gl3_viewcluster2;
		leaf = Mod_PointInLeaf(gl3_origin, gl3_worldmodel->nodes);
		gl3_viewcluster = gl3_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(gl3_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, gl3_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl3_viewcluster2))
			{
				gl3_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(gl3_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, gl3_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl3_viewcluster2))
			{
				gl3_viewcluster2 = leaf->cluster;
			}
		}
	}

	for (i = 0; i < 4; i++)
	{
		v_blend[i] = r_newrefdef.blend[i];
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

	/* clear out the portion of the screen that the NOWORLDMODEL defines */
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.3, 0.3, 0.3, 1);
		glScissor(r_newrefdef.x,
				vid.height - r_newrefdef.height - r_newrefdef.y,
				r_newrefdef.width, r_newrefdef.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(1, 0, 0.5, 0.5);
		glDisable(GL_SCISSOR_TEST);
	}
}

static void
GL3_SetGL2D(void)
{
	int x = 0;
	int w = vid.width;
	int y = 0;
	int h = vid.height;

#if 0 // TODO: stereo
	/* set 2D virtual screen size */
	qboolean drawing_left_eye = gl_state.camera_separation < 0;
	qboolean stereo_split_tb = ((gl_state.stereo_mode == STEREO_SPLIT_VERTICAL) && gl_state.camera_separation);
	qboolean stereo_split_lr = ((gl_state.stereo_mode == STEREO_SPLIT_HORIZONTAL) && gl_state.camera_separation);

	if(stereo_split_lr) {
		w =  w / 2;
		x = drawing_left_eye ? 0 : w;
	}

	if(stereo_split_tb) {
		h =  h / 2;
		y = drawing_left_eye ? h : 0;
	}
#endif // 0

	glViewport(x, y, w, h);

	hmm_mat4 transMatr = HMM_Orthographic(0, vid.width, vid.height, 0, -99999, 99999);

	gl3state.uni2DData.transMat4 = transMatr;

	GL3_UpdateUBO2D();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

// equivalent to R_x * R_y * R_z where R_x is the trans matrix for rotating around X axis for aroundXdeg
static hmm_mat4 rotAroundAxisXYZ(float aroundXdeg, float aroundYdeg, float aroundZdeg)
{
	float alpha = HMM_ToRadians(aroundXdeg);
	float beta = HMM_ToRadians(aroundYdeg);
	float gamma = HMM_ToRadians(aroundZdeg);

	float sinA = HMM_SinF(alpha);
	float cosA = HMM_CosF(alpha);
	float sinB = HMM_SinF(beta);
	float cosB = HMM_CosF(beta);
	float sinG = HMM_SinF(gamma);
	float cosG = HMM_CosF(gamma);

	hmm_mat4 ret = {{
		{  cosB*cosG,  sinA*sinB*cosG + cosA*sinG, -cosA*sinB*cosG + sinA*sinG, 0 }, // first *column*
		{ -cosB*sinG, -sinA*sinB*sinG + cosA*cosG,  cosA*sinB*sinG + sinA*cosG, 0 },
		{  sinB,      -sinA*cosB,                   cosA*cosB,                  0 },
		{  0,          0,                           0,                          1 }
	}};

	return ret;
}

// equivalent to R_SetPerspective() but returning a matrix instead of setting internal OpenGL state
hmm_mat4
GL3_SetPerspective(GLdouble fovy)
{
	// gluPerspective() / R_MYgluPerspective() style parameters
	const GLdouble zNear = Q_max(gl_znear->value, 0.1f);
	const GLdouble zFar = (r_farsee->value) ? 8192.0f : 4096.0f;
	const GLdouble aspect = (GLdouble)r_newrefdef.width / r_newrefdef.height;

	// calculation of left, right, bottom, top is from R_MYgluPerspective() of old gl backend
	// which seems to be slightly different from the real gluPerspective()
	// and thus also from HMM_Perspective()
	GLdouble left, right, bottom, top;
	float A, B, C, D;

	top = zNear * tan(fovy * M_PI / 360.0);
	right = top * aspect;

	bottom = -top;
	left = -right;

	// TODO:  stereo stuff
	// left += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;
	// right += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;

	// the following emulates glFrustum(left, right, bottom, top, zNear, zFar)
	// see https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
	//  or http://docs.gl/gl2/glFrustum#description (looks better in non-Firefox browsers)
	A = (right+left)/(right-left);
	B = (top+bottom)/(top-bottom);
	C = -(zFar+zNear)/(zFar-zNear);
	D = -(2.0*zFar*zNear)/(zFar-zNear);

	hmm_mat4 ret = {{
		{ (2.0*zNear)/(right-left), 0, 0, 0 }, // first *column*
		{ 0, (2.0*zNear)/(top-bottom), 0, 0 },
		{ A, B, C, -1.0 },
		{ 0, 0, D, 0 }
	}};

	return ret;
}

static void GL3_Clear(void);

static void
SetupGL(void)
{
	int x, x2, y2, y, w, h;

	/* set up viewport */
	x = floor(r_newrefdef.x * vid.width / (float)vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / (float)vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / (float)vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / (float)vid.height);

	w = x2 - x;
	h = y - y2;

#if 0 // TODO: stereo stuff
	qboolean drawing_left_eye = gl_state.camera_separation < 0;
	qboolean stereo_split_tb = ((gl_state.stereo_mode == STEREO_SPLIT_VERTICAL) && gl_state.camera_separation);
	qboolean stereo_split_lr = ((gl_state.stereo_mode == STEREO_SPLIT_HORIZONTAL) && gl_state.camera_separation);

	if(stereo_split_lr) {
		w = w / 2;
		x = drawing_left_eye ? (x / 2) : (x + vid.width) / 2;
	}

	if(stereo_split_tb) {
		h = h / 2;
		y2 = drawing_left_eye ? (y2 + vid.height) / 2 : (y2 / 2);
	}
#endif // 0

	// set up the FBO accordingly, but only if actually rendering the world
	// (=> don't use FBO when rendering the playermodel in the player menu)
	// also, only do this when under water, because this has a noticeable overhead on some systems
	if (gl3_usefbo->value && gl3state.ppFBO != 0
		&& (r_newrefdef.rdflags & (RDF_NOWORLDMODEL|RDF_UNDERWATER)) == RDF_UNDERWATER)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gl3state.ppFBO);
		gl3state.ppFBObound = true;
		if(gl3state.ppFBtex == 0)
		{
			gl3state.ppFBtexWidth = -1; // make sure we generate the texture storage below
			glGenTextures(1, &gl3state.ppFBtex);
		}

		if(gl3state.ppFBrbo == 0)
		{
			gl3state.ppFBtexWidth = -1; // make sure we generate the RBO storage below
			glGenRenderbuffers(1, &gl3state.ppFBrbo);
		}

		// even if the FBO already has a texture and RBO, the viewport size
		// might have changed so they need to be regenerated with the correct sizes
		if(gl3state.ppFBtexWidth != w || gl3state.ppFBtexHeight != h)
		{
			gl3state.ppFBtexWidth = w;
			gl3state.ppFBtexHeight = h;
			GL3_Bind(gl3state.ppFBtex);
			// create texture for FBO with size of the viewport
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GL3_Bind(0);
			// attach it to currently bound FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl3state.ppFBtex, 0);

			// also create a renderbuffer object so the FBO has a stencil- and depth-buffer
			glBindRenderbuffer(GL_RENDERBUFFER, gl3state.ppFBrbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			// attach it to the FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			                          GL_RENDERBUFFER, gl3state.ppFBrbo);

			GLenum fbState = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if(fbState != GL_FRAMEBUFFER_COMPLETE)
			{
				Com_Printf("GL3 SetupGL(): WARNING: FBO is not complete, status = 0x%x\n", fbState);
				gl3state.ppFBtexWidth = -1; // to try again next frame; TODO: maybe give up?
				gl3state.ppFBObound = false;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		GL3_Clear(); // clear the FBO that's bound now

		glViewport(0, 0, w, h); // this will be moved to the center later, so no x/y offset
	}
	else // rendering directly (not to FBO for postprocessing)
	{
		glViewport(x, y2, w, h);
	}

	/* set up projection matrix (eye coordinates -> clip coordinates) */
	gl3state.projMat3D = GL3_SetPerspective(r_newrefdef.fov_y);

	glCullFace(GL_FRONT);

	/* set up view matrix (world coordinates -> eye coordinates) */
	{
		// first put Z axis going up
		hmm_mat4 viewMat = {{
			{  0, 0, -1, 0 }, // first *column* (the matrix is column-major)
			{ -1, 0,  0, 0 },
			{  0, 1,  0, 0 },
			{  0, 0,  0, 1 }
		}};

		// now rotate by view angles
		hmm_mat4 rotMat = rotAroundAxisXYZ(-r_newrefdef.viewangles[2], -r_newrefdef.viewangles[0], -r_newrefdef.viewangles[1]);

		viewMat = HMM_MultiplyMat4( viewMat, rotMat );

		// .. and apply translation for current position
		hmm_vec3 trans = HMM_Vec3(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);
		viewMat = HMM_MultiplyMat4( viewMat, HMM_Translate(trans) );

		gl3state.viewMat3D = viewMat;
	}

	// just use one projection-view-matrix (premultiplied here)
	// so we have one less mat4 multiplication in the 3D shaders
	gl3state.uni3DData.transProjViewMat4 = HMM_MultiplyMat4(gl3state.projMat3D, gl3state.viewMat3D);

	gl3state.uni3DData.transModelMat4 = gl3_identityMat4;

	gl3state.uni3DData.time = r_newrefdef.time;

	GL3_UpdateUBO3D();

	/* set drawing parms */
	if (r_cull->value)
	{
		glEnable(GL_CULL_FACE);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	glEnable(GL_DEPTH_TEST);
}

extern int c_visible_lightmaps, c_visible_textures;

/*
 * r_newrefdef must be set before the first call
 */
static void
GL3_RenderView(refdef_t *fd)
{
#if 0 // TODO: keep stereo stuff?
	if ((gl_state.stereo_mode != STEREO_MODE_NONE) && gl_state.camera_separation) {

		qboolean drawing_left_eye = gl_state.camera_separation < 0;
		switch (gl_state.stereo_mode) {
			case STEREO_MODE_ANAGLYPH:
				{

					// Work out the colour for each eye.
					int anaglyph_colours[] = { 0x4, 0x3 }; // Left = red, right = cyan.

					if (strlen(gl1_stereo_anaglyph_colors->string) == 2) {
						int eye, colour, missing_bits;
						// Decode the colour name from its character.
						for (eye = 0; eye < 2; ++eye) {
							colour = 0;
							switch (toupper(gl1_stereo_anaglyph_colors->string[eye])) {
								case 'B': ++colour; // 001 Blue
								case 'G': ++colour; // 010 Green
								case 'C': ++colour; // 011 Cyan
								case 'R': ++colour; // 100 Red
								case 'M': ++colour; // 101 Magenta
								case 'Y': ++colour; // 110 Yellow
									anaglyph_colours[eye] = colour;
									break;
							}
						}
						// Fill in any missing bits.
						missing_bits = ~(anaglyph_colours[0] | anaglyph_colours[1]) & 0x3;
						for (eye = 0; eye < 2; ++eye) {
							anaglyph_colours[eye] |= missing_bits;
						}
					}

					// Set the current colour.
					glColorMask(
						!!(anaglyph_colours[drawing_left_eye] & 0x4),
						!!(anaglyph_colours[drawing_left_eye] & 0x2),
						!!(anaglyph_colours[drawing_left_eye] & 0x1),
						GL_TRUE
					);
				}
				break;
			case STEREO_MODE_ROW_INTERLEAVED:
			case STEREO_MODE_COLUMN_INTERLEAVED:
			case STEREO_MODE_PIXEL_INTERLEAVED:
				{
					qboolean flip_eyes = true;
					int client_x, client_y;

					//GLimp_GetClientAreaOffset(&client_x, &client_y);
					client_x = 0;
					client_y = 0;

					GL3_SetGL2D();

					glEnable(GL_STENCIL_TEST);
					glStencilMask(GL_TRUE);
					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

					glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
					glStencilFunc(GL_NEVER, 0, 1);

					glBegin(GL_QUADS);
					{
						glVertex2i(0, 0);
						glVertex2i(vid.width, 0);
						glVertex2i(vid.width, vid.height);
						glVertex2i(0, vid.height);
					}
					glEnd();

					glStencilOp(GL_INVERT, GL_KEEP, GL_KEEP);
					glStencilFunc(GL_NEVER, 1, 1);

					glBegin(GL_LINES);
					{
						if (gl_state.stereo_mode == STEREO_MODE_ROW_INTERLEAVED || gl_state.stereo_mode == STEREO_MODE_PIXEL_INTERLEAVED) {
							int y;
							for (y = 0; y <= vid.height; y += 2) {
								glVertex2f(0, y - 0.5f);
								glVertex2f(vid.width, y - 0.5f);
							}
							flip_eyes ^= (client_y & 1);
						}

						if (gl_state.stereo_mode == STEREO_MODE_COLUMN_INTERLEAVED || gl_state.stereo_mode == STEREO_MODE_PIXEL_INTERLEAVED) {
							int x;
							for (x = 0; x <= vid.width; x += 2) {
								glVertex2f(x - 0.5f, 0);
								glVertex2f(x - 0.5f, vid.height);
							}
							flip_eyes ^= (client_x & 1);
						}
					}
					glEnd();

					glStencilMask(GL_FALSE);
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					glStencilFunc(GL_EQUAL, drawing_left_eye ^ flip_eyes, 1);
					glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}
				break;
			default:
				break;
		}
	}
#endif // 0 (stereo stuff)

	if (r_norefresh->value)
	{
		return;
	}

	r_newrefdef = *fd;

	if (!gl3_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		Com_Error(ERR_DROP, "R_RenderView: NULL worldmodel");
	}

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	GL3_PushDlights();

	if (gl_finish->value)
	{
		glFinish();
	}

	SetupFrame();

	R_SetFrustum(vup, vpn, vright, gl3_origin,
		r_newrefdef.fov_x, r_newrefdef.fov_y, frustum);

	SetupGL();

	GL3_MarkLeaves(); /* done here so we know if we're in water */

	GL3_DrawWorld();

	GL3_DrawEntitiesOnList();

	// kick the silly r_flashblend poly lights
	// GL3_RenderDlights();

	GL3_DrawParticles();

	GL3_DrawAlphaSurfaces();

	// Note: R_Flash() is now GL3_Draw_Flash() and called from GL3_RenderFrame()

	if (r_speeds->value)
	{
		Com_Printf("%4i wpoly %4i epoly %i tex %i lmaps\n",
				c_brush_polys, c_alias_polys, c_visible_textures,
				c_visible_lightmaps);
	}

#if 0 // TODO: stereo stuff
	switch (gl_state.stereo_mode) {
		case STEREO_MODE_NONE:
			break;
		case STEREO_MODE_ANAGLYPH:
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			break;
		case STEREO_MODE_ROW_INTERLEAVED:
		case STEREO_MODE_COLUMN_INTERLEAVED:
		case STEREO_MODE_PIXEL_INTERLEAVED:
			glDisable(GL_STENCIL_TEST);
			break;
		default:
			break;
	}
#endif // 0
}

#if 0 // TODO: stereo
enum opengl_special_buffer_modes
GL3_GetSpecialBufferModeForStereoMode(enum stereo_modes stereo_mode) {
	switch (stereo_mode) {
		case STEREO_MODE_NONE:
		case STEREO_SPLIT_HORIZONTAL:
		case STEREO_SPLIT_VERTICAL:
		case STEREO_MODE_ANAGLYPH:
			return OPENGL_SPECIAL_BUFFER_MODE_NONE;
		case STEREO_MODE_OPENGL:
			return OPENGL_SPECIAL_BUFFER_MODE_STEREO;
		case STEREO_MODE_ROW_INTERLEAVED:
		case STEREO_MODE_COLUMN_INTERLEAVED:
		case STEREO_MODE_PIXEL_INTERLEAVED:
			return OPENGL_SPECIAL_BUFFER_MODE_STENCIL;
	}
	return OPENGL_SPECIAL_BUFFER_MODE_NONE;
}
#endif // 0

static void
GL3_SetLightLevel(entity_t *currententity)
{
	vec3_t shadelight = {0};

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	/* save off light value for server to look at */
	R_LightPoint(gl3_worldmodel->grid, currententity,
		gl3_worldmodel->surfaces, gl3_worldmodel->nodes, r_newrefdef.vieworg,
		shadelight, r_modulate->value, lightspot);

	/* pick the greatest component, which should be the
	 * same as the mono value returned by software */
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[0];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
	else
	{
		if (shadelight[1] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[1];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
}

static void
GL3_RenderFrame(refdef_t *fd)
{
	GL3_RenderView(fd);
	GL3_SetLightLevel(NULL);
	qboolean usedFBO = gl3state.ppFBObound; // if it was/is used this frame
	if(usedFBO)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // now render to default framebuffer
		gl3state.ppFBObound = false;
	}
	GL3_SetGL2D();

	int x = (vid.width - r_newrefdef.width)/2;
	int y = (vid.height - r_newrefdef.height)/2;
	if (usedFBO)
	{
		// if we're actually drawing the world and using an FBO, render the FBO's texture
		GL3_DrawFrameBufferObject(x, y, r_newrefdef.width, r_newrefdef.height, gl3state.ppFBtex, v_blend);
	}
	else if(v_blend[3] != 0.0f)
	{
		GL3_Draw_Flash(v_blend, x, y, r_newrefdef.width, r_newrefdef.height);
	}
}


static void
GL3_Clear(void)
{
	// Check whether the stencil buffer needs clearing, and do so if need be.
	GLbitfield stencilFlags = 0;
#if 0 // TODO: stereo stuff
	if (gl3state.stereo_mode >= STEREO_MODE_ROW_INTERLEAVED && gl_state.stereo_mode <= STEREO_MODE_PIXEL_INTERLEAVED) {
		glClearStencil(GL_FALSE);
		stencilFlags |= GL_STENCIL_BUFFER_BIT;
	}
#endif // 0


	if (r_clear->value)
	{
		glClear(GL_COLOR_BUFFER_BIT | stencilFlags | GL_DEPTH_BUFFER_BIT);
	}
	else
	{
		glClear(GL_DEPTH_BUFFER_BIT | stencilFlags);
	}

	gl3depthmin = 0;
	gl3depthmax = 1;
	glDepthFunc(GL_LEQUAL);

	glDepthRange(gl3depthmin, gl3depthmax);

	if (gl_zfix->value)
	{
		if (gl3depthmax > gl3depthmin)
		{
			glPolygonOffset(0.05, 1);
		}
		else
		{
			glPolygonOffset(-0.05, -1);
		}
	}

	/* stencilbuffer shadows */
	if (gl_shadows->value && gl3config.stencil)
	{
		glClearStencil(GL_TRUE);
		glClear(GL_STENCIL_BUFFER_BIT);
	}
}

void
GL3_BeginFrame(float camera_separation)
{
#if 0 // TODO: stereo stuff
	gl_state.camera_separation = camera_separation;

	// force a vid_restart if gl1_stereo has been modified.
	if ( gl_state.stereo_mode != gl1_stereo->value ) {
		// If we've gone from one mode to another with the same special buffer requirements there's no need to restart.
		if ( GL_GetSpecialBufferModeForStereoMode( gl_state.stereo_mode ) == GL_GetSpecialBufferModeForStereoMode( gl1_stereo->value )  ) {
			gl_state.stereo_mode = gl1_stereo->value;
		}
		else
		{
			Com_Printf("stereo supermode changed, restarting video!\n");
			vid_fullscreen->modified = true;
		}
	}
#endif // 0

	if (vid_gamma->modified || gl3_intensity->modified || gl3_intensity_2D->modified)
	{
		vid_gamma->modified = false;
		gl3_intensity->modified = false;
		gl3_intensity_2D->modified = false;

		gl3state.uniCommonData.gamma = 1.0f/vid_gamma->value;
		gl3state.uniCommonData.intensity = gl3_intensity->value;
		gl3state.uniCommonData.intensity2D = gl3_intensity_2D->value;
		GL3_UpdateUBOCommon();
	}

	// in GL3, overbrightbits can have any positive value
	if (gl3_overbrightbits->modified)
	{
		gl3_overbrightbits->modified = false;

		if(gl3_overbrightbits->value < 0.0f)
		{
			ri.Cvar_Set("gl3_overbrightbits", "0");
		}

		gl3state.uni3DData.overbrightbits = (gl3_overbrightbits->value <= 0.0f) ? 1.0f : gl3_overbrightbits->value;
		GL3_UpdateUBO3D();
	}

	if(gl3_particle_fade_factor->modified)
	{
		gl3_particle_fade_factor->modified = false;
		gl3state.uni3DData.particleFadeFactor = gl3_particle_fade_factor->value;
		GL3_UpdateUBO3D();
	}

	if(gl3_particle_square->modified || gl3_colorlight->modified)
	{
		gl3_particle_square->modified = false;
		gl3_colorlight->modified = false;
		GL3_RecreateShaders();
	}


	/* go into 2D mode */

	GL3_SetGL2D();

	/* draw buffer stuff */
	if (gl_drawbuffer->modified)
	{
		gl_drawbuffer->modified = false;


#ifdef YQ2_GL3_GLES
		// OpenGL ES3 only supports GL_NONE, GL_BACK and GL_COLOR_ATTACHMENT*
		// so this doesn't make sense here, see https://docs.gl/es3/glDrawBuffers
		Com_Printf("NOTE: gl_drawbuffer not supported by OpenGL ES!\n");
#else // Desktop GL
		// TODO: stereo stuff
		//if ((gl3state.camera_separation == 0) || gl3state.stereo_mode != STEREO_MODE_OPENGL)
		{
			GLenum drawBuffer = GL_BACK;
			if (Q_stricmp(gl_drawbuffer->string, "GL_FRONT") == 0)
			{
				drawBuffer = GL_FRONT;
			}
			glDrawBuffer(drawBuffer);
		}
#endif
	}

	/* texturemode stuff */
	if (gl_texturemode->modified || (gl3config.anisotropic && gl_anisotropic->modified)
	    || r_nolerp_list->modified || r_lerp_list->modified
		|| r_2D_unfiltered->modified || r_videos_unfiltered->modified)
	{
		GL3_TextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
		gl_anisotropic->modified = false;
		r_nolerp_list->modified = false;
		r_lerp_list->modified = false;
		r_2D_unfiltered->modified = false;
		r_videos_unfiltered->modified = false;
	}

	if (r_vsync->modified)
	{
		r_vsync->modified = false;
		GL3_SetVsync();
	}

	/* clear screen if desired */
	GL3_Clear();
}

static void
GL3_SetPalette(const unsigned char *palette)
{
	int i;
	byte *rp = (byte *)gl3_rawpalette;

	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = LittleLong(d_8to24table[i]) & 0xff;
			rp[i * 4 + 1] = (LittleLong(d_8to24table[i]) >> 8) & 0xff;
			rp[i * 4 + 2] = (LittleLong(d_8to24table[i]) >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1, 0, 0.5, 0.5);
}

/*
=====================
GL3_EndWorldRenderpass
=====================
*/
static qboolean
GL3_EndWorldRenderpass( void )
{
	return true;
}

Q2_DLL_EXPORTED refexport_t
GetRefAPI(refimport_t imp)
{
	refexport_t re = {0};

	ri = imp;

	re.api_version = API_VERSION;
	re.framework_version = GL3_GetSDLVersion();

	re.Init = GL3_Init;
	re.Shutdown = GL3_Shutdown;
	re.PrepareForWindow = GL3_PrepareForWindow;
	re.InitContext = GL3_InitContext;
	re.GetDrawableSize = GL3_GetDrawableSize;
	re.ShutdownContext = GL3_ShutdownContext;
	re.IsVSyncActive = GL3_IsVsyncActive;

	re.BeginRegistration = GL3_BeginRegistration;
	re.RegisterModel = GL3_RegisterModel;
	re.RegisterSkin = GL3_RegisterSkin;

	re.SetSky = GL3_SetSky;
	re.EndRegistration = GL3_EndRegistration;

	re.RenderFrame = GL3_RenderFrame;

	re.DrawFindPic = GL3_Draw_FindPic;
	re.DrawGetPicSize = GL3_Draw_GetPicSize;

	re.DrawPicScaled = GL3_Draw_PicScaled;
	re.DrawStretchPic = GL3_Draw_StretchPic;

	re.DrawCharScaled = GL3_Draw_CharScaled;
	re.DrawStringScaled = GL3_Draw_StringScaled;
	re.DrawTileClear = GL3_Draw_TileClear;
	re.DrawFill = GL3_Draw_Fill;
	re.DrawFadeScreen = GL3_Draw_FadeScreen;

	re.DrawStretchRaw = GL3_Draw_StretchRaw;
	re.SetPalette = GL3_SetPalette;

	re.BeginFrame = GL3_BeginFrame;
	re.EndWorldRenderpass = GL3_EndWorldRenderpass;
	re.EndFrame = GL3_EndFrame;

	// Tell the client that we're unsing the
	// new renderer restart API.
	ri.Vid_RequestRestart(RESTART_NO);

	return re;
}
