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
 * Refresher setup and main part of the frame generation, for OpenGL4
 *
 * =======================================================================
 */

#include "../ref_shared.h"
#include "header/local.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "../files/HandmadeMath.h"

#define DG_DYNARR_IMPLEMENTATION
#include "../files/DG_dynarr.h"

#define REF_VERSION "Yamagi Quake II OpenGL4 Refresher"

refimport_t ri;

gl4config_t gl4config;
gl4state_t gl4state;

unsigned gl4_rawpalette[256];

gl4model_t *gl4_worldmodel;

float gl4depthmin=0.0f, gl4depthmax=1.0f;

cplane_t frustum[4];

/* view origin */
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t gl4_origin;

int gl4_visframecount; /* bumped when going to a new PVS */
int gl4_framecount; /* used for dlight push checking */

int c_brush_polys, c_alias_polys;

static float v_blend[4]; /* final blending color */

int gl4_viewcluster, gl4_viewcluster2, gl4_oldviewcluster, gl4_oldviewcluster2;

const hmm_mat4 gl4_identityMat4 = {{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1},
}};

cvar_t *gl_version_override;
cvar_t *gl_texturemode;
cvar_t *gl_drawbuffer;
cvar_t *gl4_particle_size;
cvar_t *gl4_particle_fade_factor;
cvar_t *gl4_particle_square;
cvar_t *gl4_colorlight;
cvar_t *gl_polyblend;
cvar_t *gl4_intensity;
cvar_t *gl4_intensity_2D;
cvar_t *gl4_overbrightbits;
cvar_t *gl_nobind;
cvar_t *gl_finish;
cvar_t *gl_zfix;
cvar_t *gl4_debugcontext;
cvar_t *gl4_usebigvbo;
cvar_t *gl4_usefbo;

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
GL4_RotateForEntity(entity_t *e)
{
	// angles: pitch (around y), yaw (around z), roll (around x)
	// rot matrices to be multiplied in order Z, Y, X (yaw, pitch, roll)
	hmm_mat4 transMat = rotAroundAxisZYX(e->angles[1], -e->angles[0], -e->angles[2]);

	for(int i=0; i<3; ++i)
	{
		transMat.Elements[3][i] = e->origin[i]; // set translation
	}

	gl4state.uni3DData.transModelMat4 = HMM_MultiplyMat4(gl4state.uni3DData.transModelMat4, transMat);

	GL4_UpdateUBO3D();
}


static void
GL4_Strings(void)
{
	GLint i, numExtensions;
	Com_Printf("GL_VENDOR: %s\n", gl4config.vendor_string);
	Com_Printf("GL_RENDERER: %s\n", gl4config.renderer_string);
	Com_Printf("GL_VERSION: %s\n", gl4config.version_string);
	Com_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", gl4config.glsl_version_string);

	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	Com_Printf("GL_EXTENSIONS:");
	for(i = 0; i < numExtensions; i++)
	{
		Com_Printf(" %s", (const char*)glGetStringi(GL_EXTENSIONS, i));
	}
	Com_Printf("\n");
}

static void
GL4_Register(void)
{
	R_InitCvar();

	gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	gl_version_override = ri.Cvar_Get("gl_version_override", "0", CVAR_ARCHIVE );
	gl4_debugcontext = ri.Cvar_Get("gl4_debugcontext", "0", 0);
	gl4_particle_size = ri.Cvar_Get("gl4_particle_size", "40", CVAR_ARCHIVE);
	gl4_particle_fade_factor = ri.Cvar_Get("gl4_particle_fade_factor", "1.2", CVAR_ARCHIVE);
	gl4_particle_square = ri.Cvar_Get("gl4_particle_square", "0", CVAR_ARCHIVE);
	// if set to 0, lights (from lightmaps, dynamic lights and on models) are white instead of colored
	gl4_colorlight = ri.Cvar_Get("gl4_colorlight", "1", CVAR_ARCHIVE);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", CVAR_ARCHIVE);

	//  0: use lots of calls to glBufferData()
	//  1: reduce calls to glBufferData() with one big VBO (see GL4_BufferAndDraw3D())
	// -1: auto (let yq2 choose to enable/disable this based on detected driver)
	gl4_usebigvbo = ri.Cvar_Get("gl4_usebigvbo", "-1", CVAR_ARCHIVE);
	gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);

	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);

	gl4_intensity = ri.Cvar_Get("gl4_intensity", "1.5", CVAR_ARCHIVE);
	gl4_intensity_2D = ri.Cvar_Get("gl4_intensity_2D", "1.5", CVAR_ARCHIVE);

	gl4_overbrightbits = ri.Cvar_Get("gl4_overbrightbits", "1.3", CVAR_ARCHIVE);

	gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);
	gl_znear = ri.Cvar_Get("gl_znear", "4", CVAR_ARCHIVE);

	gl4_usefbo = ri.Cvar_Get("gl4_usefbo", "1", CVAR_ARCHIVE); // use framebuffer object for postprocess effects (water)

#if 0 // TODO!
	//gl_farsee = ri.Cvar_Get("gl_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	//gl_overbrightbits = ri.Cvar_Get("gl_overbrightbits", "0", CVAR_ARCHIVE);

	gl1_particle_min_size = ri.Cvar_Get("gl1_particle_min_size", "2", CVAR_ARCHIVE);
	gl1_particle_max_size = ri.Cvar_Get("gl1_particle_max_size", "40", CVAR_ARCHIVE);
	//gl1_particle_size = ri.Cvar_Get("gl1_particle_size", "40", CVAR_ARCHIVE);
	gl1_particle_att_a = ri.Cvar_Get("gl1_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl1_particle_att_b = ri.Cvar_Get("gl1_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl1_particle_att_c = ri.Cvar_Get("gl1_particle_att_c", "0.01", CVAR_ARCHIVE);

	//gl_modulate = ri.Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
	//gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
	gl_showbbox = Cvar_Get("gl_showbbox", "0", 0);
	//gl1_ztrick = ri.Cvar_Get("gl1_ztrick", "0", 0); NOTE: dump this.
	//gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	//gl_finish = ri.Cvar_Get("gl_finish", "0", CVAR_ARCHIVE);

	//gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl1_texturealphamode = ri.Cvar_Get("gl1_texturealphamode", "default", CVAR_ARCHIVE);
	gl1_texturesolidmode = ri.Cvar_Get("gl1_texturesolidmode", "default", CVAR_ARCHIVE);

	gl1_pointparameters = ri.Cvar_Get("gl1_pointparameters", "1", CVAR_ARCHIVE);

	//gl_drawbuffer = ri.Cvar_Get("gl_drawbuffer", "GL_BACK", 0);

	gl1_stereo = ri.Cvar_Get("gl1_stereo", "0", CVAR_ARCHIVE );
	gl1_stereo_separation = ri.Cvar_Get("gl1_stereo_separation", "-0.4", CVAR_ARCHIVE );
	gl1_stereo_anaglyph_colors = ri.Cvar_Get("gl1_stereo_anaglyph_colors", "rc", CVAR_ARCHIVE );
	gl1_stereo_convergence = ri.Cvar_Get("gl1_stereo_convergence", "1", CVAR_ARCHIVE );
#endif // 0

	ri.Cmd_AddCommand("imagelist", GL4_ImageList_f);
	ri.Cmd_AddCommand("screenshot", GL4_ScreenShot);
	ri.Cmd_AddCommand("modellist", GL4_Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", GL4_Strings);
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
		GL4_BindVBO(0);
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
	   in vid with a call to GL4_GetDrawableSize(), just like the
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
			GL4_GetDrawableSize(pwidth, pheight);
		}
		else
		{
			if (r_mode->value == -2)
			{
				/* User requested native resolution. */
				GL4_GetDrawableSize(pwidth, pheight);
			}
		}
	}

	return rserr_ok;
}

static qboolean
GL4_SetMode(void)
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
			gl4state.prev_mode = 4; /* safe default for custom mode */
		}
		else
		{
			gl4state.prev_mode = r_mode->value;
		}
	}
	else
	{
		if (err == rserr_invalid_mode)
		{
			Com_Printf("ref_gl4::GL4_SetMode() - invalid mode\n");

			if (r_msaa_samples->value != 0.0f)
			{
				Com_Printf("r_msaa_samples was %d - will try again with r_msaa_samples = 0\n", (int)r_msaa_samples->value);
				ri.Cvar_SetValue("r_msaa_samples", 0.0f);
				r_msaa_samples->modified = false;

				if ((err = SetMode_impl(&vid.width, &vid.height, r_mode->value, 0)) == rserr_ok)
				{
					return true;
				}
			}
			if(r_mode->value == gl4state.prev_mode)
			{
				// trying again would result in a crash anyway, give up already
				// (this would happen if your initing fails at all and your resolution already was 640x480)
				return false;
			}

			ri.Cvar_SetValue("r_mode", gl4state.prev_mode);
			r_mode->modified = false;
		}

		/* try setting it back to something safe */
		if ((err = SetMode_impl(&vid.width, &vid.height, gl4state.prev_mode, 0)) != rserr_ok)
		{
			Com_Printf("ref_gl4::GL4_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}

	return true;
}

// only needed (and allowed!) if using OpenGL compatibility profile, it's not in 3.2 core
enum { QGL_POINT_SPRITE = 0x8861 };

static qboolean
GL4_Init(void)
{
	Swap_Init(); // FIXME: for fucks sake, this doesn't have to be done at runtime!

	Com_Printf("Refresh: " REF_VERSION "\n");
	Com_Printf("Client: " YQ2VERSION "\n\n");

	if(sizeof(float) != sizeof(GLfloat))
	{
		// if this ever happens, things would explode because we feed vertex arrays and UBO data
		// using floats to OpenGL, which expects GLfloat (can't easily change, those floats are from HMM etc)
		// (but to be honest I very much doubt this will ever happen.)
		Com_Printf("ref_gl4: sizeof(float) != sizeof(GLfloat) - we're in real trouble here.\n");
		return false;
	}

	ri.VID_GetPalette(NULL, d_8to24table);

	GL4_Register();

	/* set our "safe" mode */
	gl4state.prev_mode = 4;
	//gl_state.stereo_mode = gl1_stereo->value;

	/* create the window and set up the context */
	if (!GL4_SetMode())
	{
		Com_Printf("ref_gl4::R_Init() - could not R_SetMode()\n");
		return false;
	}

	ri.Vid_MenuInit();

	/* get our various GL strings */
	gl4config.vendor_string = (const char*)glGetString(GL_VENDOR);
	gl4config.renderer_string = (const char*)glGetString(GL_RENDERER);
	gl4config.version_string = (const char*)glGetString(GL_VERSION);
	gl4config.glsl_version_string = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	Com_Printf("\nOpenGL setting:\n");
	GL4_Strings();

	Com_Printf("\n\nProbing for OpenGL extensions:\n");


	/* Anisotropic */
	Com_Printf(" - Anisotropic Filtering: ");

	if(gl4config.anisotropic)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &gl4config.max_anisotropy);

		Com_Printf("Max level: %ux\n", (int)gl4config.max_anisotropy);
	}
	else
	{
		gl4config.max_anisotropy = 0.0;

		Com_Printf("Not supported\n");
	}

	if(gl4config.debug_output)
	{
		Com_Printf(" - OpenGL Debug Output: Supported ");
		if(gl4_debugcontext->value == 0.0f)
		{
			Com_Printf("(but disabled with gl4_debugcontext = 0)\n");
		}
		else
		{
			Com_Printf("and enabled with gl4_debugcontext = %i\n", (int)gl4_debugcontext->value);
		}
	}
	else
	{
		Com_Printf(" - OpenGL Debug Output: Not Supported\n");
	}

	gl4config.useBigVBO = false;
	if(gl4_usebigvbo->value == 1.0f)
	{
		Com_Printf("Enabling useBigVBO workaround because gl4_usebigvbo = 1\n");
		gl4config.useBigVBO = true;
	}
	else if(gl4_usebigvbo->value == -1.0f)
	{
		// enable for AMDs proprietary Windows and Linux drivers
#ifdef _WIN32
		if(gl4config.version_string != NULL && gl4config.vendor_string != NULL
		   && strstr(gl4config.vendor_string, "ATI Technologies Inc") != NULL)
		{
			int a, b, ver;
			if(sscanf(gl4config.version_string, " %d.%d.%d ", &a, &b, &ver) >= 3 && ver >= 13431)
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
				gl4config.useBigVBO = true;
			}
		}
#elif defined(__linux__)
		if(gl4config.vendor_string != NULL && strstr(gl4config.vendor_string, "Advanced Micro Devices, Inc.") != NULL)
		{
			Com_Printf("Detected proprietary AMD GPU driver, enabling useBigVBO workaround\n");
			Com_Printf("(consider using the open source RadeonSI drivers, they tend to work better overall)\n");
			gl4config.useBigVBO = true;
		}
#endif
	}

	// generate texture handles for all possible lightmaps
	glGenTextures(MAX_LIGHTMAPS*MAX_LIGHTMAPS_PER_SURFACE, gl4state.lightmap_textureIDs[0]);

	GL4_SetDefaultState();

	if(GL4_InitShaders())
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

	GL4_Mod_Init();

	GL4_InitParticleTexture();

	GL4_Draw_InitLocal();

	GL4_SurfInit();

	glGenFramebuffers(1, &gl4state.ppFBO);
	// the rest for the FBO is done dynamically in GL4_RenderView() so it can
	// take the viewsize into account (enforce that by setting invalid size)
	gl4state.ppFBtexWidth = gl4state.ppFBtexHeight = -1;

	Com_Printf("\n");
	return true;
}

void
GL4_Shutdown(void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	// only call all these if we have an OpenGL context and the gl function pointers
	// randomly chose one function that should always be there to test..
	if(glDeleteBuffers != NULL)
	{
		GL4_Mod_FreeAll();
		GL4_ShutdownMeshes();
		GL4_ShutdownImages();
		R_VertBufferFree();
		GL4_SurfShutdown();
		GL4_Draw_ShutdownLocal();
		GL4_ShutdownShaders();

		// free the postprocessing FBO and its renderbuffer and texture
		if(gl4state.ppFBrbo != 0)
			glDeleteRenderbuffers(1, &gl4state.ppFBrbo);
		if(gl4state.ppFBtex != 0)
			glDeleteTextures(1, &gl4state.ppFBtex);
		if(gl4state.ppFBO != 0)
			glDeleteFramebuffers(1, &gl4state.ppFBO);
		gl4state.ppFBrbo = gl4state.ppFBtex = gl4state.ppFBO = 0;
		gl4state.ppFBObound = false;
		gl4state.ppFBtexWidth = gl4state.ppFBtexHeight = -1;
	}

	/* shutdown OS specific OpenGL stuff like contexts, etc.  */
	GL4_ShutdownContext();
}

// assumes gl4state.v[ab]o3D are bound
// buffers and draws mvtx_t vertices
// drawMode is something like GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN or whatever
void
GL4_BufferAndDraw3D(const mvtx_t* verts, int numVerts, GLenum drawMode)
{
	if(!gl4config.useBigVBO)
	{
		glBufferData( GL_ARRAY_BUFFER, sizeof(mvtx_t)*numVerts, verts, GL_STREAM_DRAW );
		glDrawArrays( drawMode, 0, numVerts );
	}
	else // gl4config.useBigVBO == true
	{
		int curOffset = gl4state.vbo3DcurOffset;
		int neededSize = numVerts * sizeof(mvtx_t);

		if (curOffset + neededSize > gl4state.vbo3Dsize) {
			curOffset = 0;
		}

		glBindBuffer(GL_ARRAY_BUFFER, gl4state.vbo3D);
		void* data = glMapBufferRange(GL_ARRAY_BUFFER, curOffset, gl4state.vbo3Dsize - curOffset, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		memcpy((char*)data + curOffset, verts, neededSize);

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glDrawArrays(drawMode, curOffset / sizeof(mvtx_t), numVerts);
		gl4state.vbo3DcurOffset = (curOffset + neededSize) % gl4state.vbo3Dsize;
	}
}

static void
GL4_DrawBeam(entity_t *e)
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

	GL4_UseProgram(gl4state.si3DcolorOnly.shaderProgram);

	r = (LittleLong(d_8to24table[e->skinnum & 0xFF])) & 0xFF;
	g = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 8) & 0xFF;
	b = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 16) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	gl4state.uniCommonData.color = HMM_Vec4(r, g, b, e->alpha);
	GL4_UpdateUBOCommon();

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		VectorCopy(start_points[i], verts[4*i+0].pos);
		VectorCopy(end_points[i], verts[4*i+1].pos);

		pointb = ( i + 1 ) % NUM_BEAM_SEGS;

		VectorCopy(start_points[pointb], verts[4*i+2].pos);
		VectorCopy(end_points[pointb], verts[4*i+3].pos);
	}

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	GL4_BufferAndDraw3D(verts, NUM_BEAM_SEGS*4, GL_TRIANGLE_STRIP);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

static void
GL4_DrawSpriteModel(entity_t *e, const gl4model_t *currentmodel)
{
	float alpha = 1.0F;
	mvtx_t verts[4];
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;
	gl4image_t *skin = NULL;
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

	if (alpha != gl4state.uni3DData.alpha)
	{
		gl4state.uni3DData.alpha = alpha;
		GL4_UpdateUBO3D();
	}

	skin = currentmodel->skins[e->frame];
	if (!skin)
	{
		skin = gl4_notexture; /* fallback... */
	}

	GL4_Bind(skin->texnum);

	if (e->flags & RF_FLARE)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		GL4_UseProgram(gl4state.si3Dsprite.shaderProgram);
	}
	else
	{
		if (alpha == 1.0)
		{
			// use shader with alpha test
			GL4_UseProgram(gl4state.si3DspriteAlpha.shaderProgram);
		}
		else
		{
			glEnable(GL_BLEND);

			GL4_UseProgram(gl4state.si3Dsprite.shaderProgram);
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

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	GL4_BufferAndDraw3D(verts, 4, GL_TRIANGLE_FAN);

	if (e->flags & RF_FLARE)
	{
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (alpha != 1.0F)
		{
			gl4state.uni3DData.alpha = 1.0f;
			GL4_UpdateUBO3D();
		}
	}
	else
	{
		if (alpha != 1.0F)
		{
			glDisable(GL_BLEND);
			gl4state.uni3DData.alpha = 1.0f;
			GL4_UpdateUBO3D();
		}
	}
}

static void
GL4_DrawNullModel(entity_t *currententity)
{
	vec3_t shadelight;

	if (currententity->flags & RF_FULLBRIGHT || !gl4_worldmodel || !gl4_worldmodel->lightdata)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	}
	else
	{
		R_LightPoint(gl4_worldmodel->grid, currententity,
			gl4_worldmodel->surfaces, gl4_worldmodel->nodes, currententity->origin,
			shadelight, r_modulate->value, lightspot);
	}

	hmm_mat4 origModelMat = gl4state.uni3DData.transModelMat4;
	GL4_RotateForEntity(currententity);

	gl4state.uniCommonData.color = HMM_Vec4( shadelight[0], shadelight[1], shadelight[2], 1 );
	GL4_UpdateUBOCommon();

	GL4_UseProgram(gl4state.si3DcolorOnly.shaderProgram);

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	mvtx_t vtxA[6] = {
		{{0, 0, -16}, {0,0}, {0,0}},
		{{16 * cos( 0 * M_PI / 2 ), 16 * sin( 0 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 1 * M_PI / 2 ), 16 * sin( 1 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 2 * M_PI / 2 ), 16 * sin( 2 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 3 * M_PI / 2 ), 16 * sin( 3 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 4 * M_PI / 2 ), 16 * sin( 4 * M_PI / 2 ), 0}, {0,0}, {0,0}}
	};

	GL4_BufferAndDraw3D(vtxA, 6, GL_TRIANGLE_FAN);

	mvtx_t vtxB[6] = {
		{{0, 0, 16}, {0,0}, {0,0}},
		vtxA[5], vtxA[4], vtxA[3], vtxA[2], vtxA[1]
	};

	GL4_BufferAndDraw3D(vtxB, 6, GL_TRIANGLE_FAN);

	gl4state.uni3DData.transModelMat4 = origModelMat;
	GL4_UpdateUBO3D();
}

static void
GL4_DrawParticles(void)
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
		float pointSize = gl4_particle_size->value * (float)r_newrefdef.height/480.0f;

		typedef struct part_vtx {
			GLfloat pos[3];
			GLfloat size;
			GLfloat dist;
			GLfloat color[4];
		} part_vtx;
		YQ2_STATIC_ASSERT(sizeof(part_vtx)==9*sizeof(float), "invalid part_vtx size"); // remember to update GL4_SurfInit() if this changes!

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

		glEnable(GL_PROGRAM_POINT_SIZE);

		GL4_UseProgram(gl4state.siParticle.shaderProgram);

		for ( i = 0, p = r_newrefdef.particles; i < numParticles; i++, p++ )
		{
			*(int *) color = p->color;
			part_vtx* cur = &buf[i];
			vec3_t offset; // between viewOrg and particle position
			VectorSubtract(viewOrg, p->origin, offset);

			VectorCopy(p->origin, cur->pos);
			cur->size = pointSize;
			cur->dist = VectorLength(offset);

			for(int j=0; j<3; ++j)
			{
				cur->color[j] = color[j] * (1.0f / 255.0f);
			}

			cur->color[3] = p->alpha;
		}

		GL4_BindVAO(gl4state.vaoParticle);
		GL4_BindVBO(gl4state.vboParticle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(part_vtx)*numParticles, buf, GL_STREAM_DRAW);
		glDrawArrays(GL_POINTS, 0, numParticles);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		glDisable(GL_PROGRAM_POINT_SIZE);

		YQ2_VLAFREE(buf);
	}
}

static void
GL4_DrawEntitiesOnList(void)
{
	qboolean translucent_entities = false;
	int i;

	if (!r_drawentities->value)
	{
		return;
	}

	GL4_ResetShadowAliasModels();

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
			GL4_DrawBeam(currententity);
		}
		else
		{
			gl4model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				GL4_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					GL4_DrawAliasModel(currententity);
					break;
				case mod_brush:
					GL4_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					GL4_DrawSpriteModel(currententity, currentmodel);
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
			GL4_DrawBeam(currententity);
		}
		else
		{
			gl4model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				GL4_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					GL4_DrawAliasModel(currententity);
					break;
				case mod_brush:
					GL4_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					GL4_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					Com_Printf("%s: Bad modeltype %d\n",
						__func__, currentmodel->type);
					break;
			}
		}
	}

	GL4_DrawAliasShadows();

	glDepthMask(GL_TRUE); /* back to writing */
}

static void
SetupFrame(void)
{
	mleaf_t *leaf;

	gl4_framecount++;

	/* build the transformation matrix for the given view angles */
	VectorCopy(r_newrefdef.vieworg, gl4_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	/* current viewcluster */
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		if (!gl4_worldmodel)
		{
			Com_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		gl4_oldviewcluster = gl4_viewcluster;
		gl4_oldviewcluster2 = gl4_viewcluster2;
		leaf = Mod_PointInLeaf(gl4_origin, gl4_worldmodel->nodes);
		gl4_viewcluster = gl4_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(gl4_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, gl4_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl4_viewcluster2))
			{
				gl4_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(gl4_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, gl4_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl4_viewcluster2))
			{
				gl4_viewcluster2 = leaf->cluster;
			}
		}
	}

	R_CombineBlendWithFog(v_blend);

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
GL4_SetGL2D(void)
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

	gl4state.uni2DData.transMat4 = transMatr;

	GL4_UpdateUBO2D();

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
GL4_SetPerspective(GLdouble fovy)
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

static void GL4_Clear(void);

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
	if (gl4_usefbo->value && gl4state.ppFBO != 0
		&& (r_newrefdef.rdflags & (RDF_NOWORLDMODEL|RDF_UNDERWATER)) == RDF_UNDERWATER)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gl4state.ppFBO);
		gl4state.ppFBObound = true;
		if(gl4state.ppFBtex == 0)
		{
			gl4state.ppFBtexWidth = -1; // make sure we generate the texture storage below
			glGenTextures(1, &gl4state.ppFBtex);
		}

		if(gl4state.ppFBrbo == 0)
		{
			gl4state.ppFBtexWidth = -1; // make sure we generate the RBO storage below
			glGenRenderbuffers(1, &gl4state.ppFBrbo);
		}

		// even if the FBO already has a texture and RBO, the viewport size
		// might have changed so they need to be regenerated with the correct sizes
		if(gl4state.ppFBtexWidth != w || gl4state.ppFBtexHeight != h)
		{
			gl4state.ppFBtexWidth = w;
			gl4state.ppFBtexHeight = h;
			GL4_Bind(gl4state.ppFBtex);
			// create texture for FBO with size of the viewport
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GL4_Bind(0);
			// attach it to currently bound FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl4state.ppFBtex, 0);

			// also create a renderbuffer object so the FBO has a stencil- and depth-buffer
			glBindRenderbuffer(GL_RENDERBUFFER, gl4state.ppFBrbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			// attach it to the FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			                          GL_RENDERBUFFER, gl4state.ppFBrbo);

			GLenum fbState = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if(fbState != GL_FRAMEBUFFER_COMPLETE)
			{
				Com_Printf("GL4 SetupGL(): WARNING: FBO is not complete, status = 0x%x\n", fbState);
				gl4state.ppFBtexWidth = -1; // to try again next frame; TODO: maybe give up?
				gl4state.ppFBObound = false;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		GL4_Clear(); // clear the FBO that's bound now

		glViewport(0, 0, w, h); // this will be moved to the center later, so no x/y offset
	}
	else // rendering directly (not to FBO for postprocessing)
	{
		glViewport(x, y2, w, h);
	}

	/* set up projection matrix (eye coordinates -> clip coordinates) */
	gl4state.projMat3D = GL4_SetPerspective(r_newrefdef.fov_y);

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

		gl4state.viewMat3D = viewMat;
	}

	// just use one projection-view-matrix (premultiplied here)
	// so we have one less mat4 multiplication in the 3D shaders
	gl4state.uni3DData.transProjViewMat4 = HMM_MultiplyMat4(gl4state.projMat3D, gl4state.viewMat3D);

	gl4state.uni3DData.transModelMat4 = gl4_identityMat4;

	gl4state.uni3DData.time = r_newrefdef.time;

	GL4_UpdateUBO3D();

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
GL4_RenderView(refdef_t *fd)
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

					GL4_SetGL2D();

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

	if (!gl4_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		Com_Error(ERR_DROP, "R_RenderView: NULL worldmodel");
	}

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	GL4_PushDlights();

	if (gl_finish->value)
	{
		glFinish();
	}

	SetupFrame();

	R_SetFrustum(vup, vpn, vright, gl4_origin,
		r_newrefdef.fov_x, r_newrefdef.fov_y, frustum);

	SetupGL();

	GL4_MarkLeaves(); /* done here so we know if we're in water */

	GL4_DrawWorld();

	GL4_DrawEntitiesOnList();

	GL4_DrawParticles();

	GL4_DrawAlphaSurfaces();

	// Note: R_Flash() is now GL4_Draw_Flash() and called from GL4_RenderFrame()

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
GL4_GetSpecialBufferModeForStereoMode(enum stereo_modes stereo_mode) {
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
GL4_SetLightLevel(entity_t *currententity)
{
	vec3_t shadelight = {0};

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	/* save off light value for server to look at */
	R_LightPoint(gl4_worldmodel->grid, currententity,
		gl4_worldmodel->surfaces, gl4_worldmodel->nodes, r_newrefdef.vieworg,
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
GL4_RenderFrame(refdef_t *fd)
{
	GL4_RenderView(fd);
	GL4_SetLightLevel(NULL);
	qboolean usedFBO = gl4state.ppFBObound; // if it was/is used this frame
	if(usedFBO)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // now render to default framebuffer
		gl4state.ppFBObound = false;
	}
	GL4_SetGL2D();

	int x = (vid.width - r_newrefdef.width)/2;
	int y = (vid.height - r_newrefdef.height)/2;
	if (usedFBO)
	{
		// if we're actually drawing the world and using an FBO, render the FBO's texture
		GL4_DrawFrameBufferObject(x, y, r_newrefdef.width, r_newrefdef.height, gl4state.ppFBtex, v_blend);
	}
	else if(v_blend[3] != 0.0f)
	{
		GL4_Draw_Flash(v_blend, x, y, r_newrefdef.width, r_newrefdef.height);
	}
}


static void
GL4_Clear(void)
{
	// Check whether the stencil buffer needs clearing, and do so if need be.
	GLbitfield stencilFlags = 0;
#if 0 // TODO: stereo stuff
	if (gl4state.stereo_mode >= STEREO_MODE_ROW_INTERLEAVED && gl_state.stereo_mode <= STEREO_MODE_PIXEL_INTERLEAVED) {
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

	gl4depthmin = 0;
	gl4depthmax = 1;
	glDepthFunc(GL_LEQUAL);

	glDepthRange(gl4depthmin, gl4depthmax);

	if (gl_zfix->value)
	{
		if (gl4depthmax > gl4depthmin)
		{
			glPolygonOffset(0.05, 1);
		}
		else
		{
			glPolygonOffset(-0.05, -1);
		}
	}

	/* stencilbuffer shadows */
	if (r_shadows->value && gl4config.stencil)
	{
		glClearStencil(GL_TRUE);
		glClear(GL_STENCIL_BUFFER_BIT);
	}
}

void
GL4_BeginFrame(float camera_separation)
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

	if (vid_gamma->modified || gl4_intensity->modified || gl4_intensity_2D->modified)
	{
		vid_gamma->modified = false;
		gl4_intensity->modified = false;
		gl4_intensity_2D->modified = false;

		gl4state.uniCommonData.gamma = 1.0f/vid_gamma->value;
		gl4state.uniCommonData.intensity = gl4_intensity->value;
		gl4state.uniCommonData.intensity2D = gl4_intensity_2D->value;
		GL4_UpdateUBOCommon();
	}

	// in GL4, overbrightbits can have any positive value
	if (gl4_overbrightbits->modified)
	{
		gl4_overbrightbits->modified = false;

		if(gl4_overbrightbits->value < 0.0f)
		{
			ri.Cvar_Set("gl4_overbrightbits", "0");
		}

		gl4state.uni3DData.overbrightbits = (gl4_overbrightbits->value <= 0.0f) ? 1.0f : gl4_overbrightbits->value;
		GL4_UpdateUBO3D();
	}

	if(gl4_particle_fade_factor->modified)
	{
		gl4_particle_fade_factor->modified = false;
		gl4state.uni3DData.particleFadeFactor = gl4_particle_fade_factor->value;
		GL4_UpdateUBO3D();
	}

	if(gl4_particle_square->modified || gl4_colorlight->modified)
	{
		gl4_particle_square->modified = false;
		gl4_colorlight->modified = false;
		GL4_RecreateShaders();
	}


	/* go into 2D mode */

	GL4_SetGL2D();

	/* draw buffer stuff */
	if (gl_drawbuffer->modified)
	{
		gl_drawbuffer->modified = false;

		// TODO: stereo stuff
		//if ((gl4state.camera_separation == 0) || gl4state.stereo_mode != STEREO_MODE_OPENGL)
		{
			GLenum drawBuffer = GL_BACK;
			if (Q_stricmp(gl_drawbuffer->string, "GL_FRONT") == 0)
			{
				drawBuffer = GL_FRONT;
			}
			glDrawBuffer(drawBuffer);
		}
	}

	/* texturemode stuff */
	if (gl_texturemode->modified || (gl4config.anisotropic && r_anisotropic->modified)
	    || r_nolerp_list->modified || r_lerp_list->modified
		|| r_2D_unfiltered->modified || r_videos_unfiltered->modified)
	{
		GL4_TextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
		r_anisotropic->modified = false;
		r_nolerp_list->modified = false;
		r_lerp_list->modified = false;
		r_2D_unfiltered->modified = false;
		r_videos_unfiltered->modified = false;
	}

	if (r_vsync->modified)
	{
		r_vsync->modified = false;
		GL4_SetVsync();
	}

	/* clear screen if desired */
	GL4_Clear();
}

static void
GL4_SetPalette(const unsigned char *palette)
{
	int i;
	byte *rp = (byte *)gl4_rawpalette;

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
GL4_EndWorldRenderpass
=====================
*/
static qboolean
GL4_EndWorldRenderpass( void )
{
	return true;
}

Q2_DLL_EXPORTED refexport_t
GetRefAPI(refimport_t imp)
{
	refexport_t re = {0};

	ri = imp;

	re.api_version = API_VERSION;
	re.framework_version = GL4_GetSDLVersion();

	re.Init = GL4_Init;
	re.Shutdown = GL4_Shutdown;
	re.PrepareForWindow = GL4_PrepareForWindow;
	re.InitContext = GL4_InitContext;
	re.GetDrawableSize = GL4_GetDrawableSize;
	re.ShutdownContext = GL4_ShutdownContext;
	re.IsVSyncActive = GL4_IsVsyncActive;

	re.BeginRegistration = GL4_BeginRegistration;
	re.RegisterModel = GL4_RegisterModel;
	re.RegisterSkin = GL4_RegisterSkin;

	re.SetSky = GL4_SetSky;
	re.EndRegistration = GL4_EndRegistration;

	re.RenderFrame = GL4_RenderFrame;

	re.DrawFindPic = GL4_Draw_FindPic;
	re.DrawGetPicSize = GL4_Draw_GetPicSize;

	re.DrawPicScaled = GL4_Draw_PicScaled;
	re.DrawStretchPic = GL4_Draw_StretchPic;

	re.DrawCharScaled = GL4_Draw_CharScaled;
	re.DrawStringScaled = GL4_Draw_StringScaled;
	re.DrawTileClear = GL4_Draw_TileClear;
	re.DrawFill = GL4_Draw_Fill;
	re.DrawFadeScreen = GL4_Draw_FadeScreen;

	re.DrawStretchRaw = GL4_Draw_StretchRaw;
	re.SetPalette = GL4_SetPalette;

	re.BeginFrame = GL4_BeginFrame;
	re.EndWorldRenderpass = GL4_EndWorldRenderpass;
	re.EndFrame = GL4_EndFrame;

	// Tell the client that we're unsing the
	// new renderer restart API.
	ri.Vid_RequestRestart(RESTART_NO);

	return re;
}
