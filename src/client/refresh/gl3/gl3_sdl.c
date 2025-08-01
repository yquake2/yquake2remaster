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
 * SDL backend for the GL3 renderer. Everything that needs to be on the
 * renderer side of thing. Also all glad (or whatever OpenGL loader I
 * end up using) specific things.
 *
 * =======================================================================
 */

#include "header/local.h"

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

static SDL_Window* window = NULL;
static SDL_GLContext context = NULL;
static qboolean vsyncActive = false;
qboolean IsHighDPIaware = false;

// --------

enum {
	// Not all GL.h header know about GL_DEBUG_SEVERITY_NOTIFICATION_*.
	// DG: yes, it's the same value in GLES3.2
	QGL_DEBUG_SEVERITY_NOTIFICATION = 0x826B
};

/*
 * Callback function for debug output.
 */
static void APIENTRY
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar *message, const void *userParam)
{
	const char* sourceStr = "Source: Unknown";
	const char* typeStr = "Type: Unknown";
	const char* severityStr = "Severity: Unknown";

	switch (severity)
	{
#ifdef YQ2_GL3_GLES
  #define SVRCASE(X, STR)  case GL_DEBUG_SEVERITY_ ## X ## _KHR : severityStr = STR; break;
#else // Desktop GL
  #define SVRCASE(X, STR)  case GL_DEBUG_SEVERITY_ ## X ## _ARB : severityStr = STR; break;
#endif

		case QGL_DEBUG_SEVERITY_NOTIFICATION: return;
		SVRCASE(HIGH, "Severity: High")
		SVRCASE(MEDIUM, "Severity: Medium")
		SVRCASE(LOW, "Severity: Low")
#undef SVRCASE
	}

	switch (source)
	{
#ifdef YQ2_GL3_GLES
  #define SRCCASE(X)  case GL_DEBUG_SOURCE_ ## X ## _KHR: sourceStr = "Source: " #X; break;
#else
  #define SRCCASE(X)  case GL_DEBUG_SOURCE_ ## X ## _ARB: sourceStr = "Source: " #X; break;
#endif
		SRCCASE(API);
		SRCCASE(WINDOW_SYSTEM);
		SRCCASE(SHADER_COMPILER);
		SRCCASE(THIRD_PARTY);
		SRCCASE(APPLICATION);
		SRCCASE(OTHER);
#undef SRCCASE
	}

	switch(type)
	{
#ifdef YQ2_GL3_GLES
  #define TYPECASE(X)  case GL_DEBUG_TYPE_ ## X ## _KHR: typeStr = "Type: " #X; break;
#else
  #define TYPECASE(X)  case GL_DEBUG_TYPE_ ## X ## _ARB: typeStr = "Type: " #X; break;
#endif
		TYPECASE(ERROR);
		TYPECASE(DEPRECATED_BEHAVIOR);
		TYPECASE(UNDEFINED_BEHAVIOR);
		TYPECASE(PORTABILITY);
		TYPECASE(PERFORMANCE);
		TYPECASE(OTHER);
#undef TYPECASE
	}

	// use PRINT_ALL - this is only called with gl3_debugcontext != 0 anyway.
	Com_Printf("GLDBG %s %s %s: %s\n", sourceStr, typeStr, severityStr, message);
}

// ---------

/*
 * Swaps the buffers and shows the next frame.
 */
void GL3_EndFrame(void)
{
	if(gl3config.useBigVBO)
	{
		// I think this is a good point to orphan the VBO and get a fresh one
		GL3_BindVAO(gl3state.vao3D);
		GL3_BindVBO(gl3state.vbo3D);
		glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW);
		gl3state.vbo3DcurOffset = 0;
	}

	SDL_GL_SwapWindow(window);
}

/*
 * Returns whether the vsync is enabled.
 */
qboolean GL3_IsVsyncActive(void)
{
	return vsyncActive;
}

/*
 * Enables or disables the vsync.
 */
void GL3_SetVsync(void)
{
	// Make sure that the user given
	// value is SDL compatible...
	int vsync = 0;

	if (r_vsync->value == 1)
	{
		vsync = 1;
	}
	else if (r_vsync->value == 2)
	{
		vsync = -1;
	}

#ifdef USE_SDL3
	if (!SDL_GL_SetSwapInterval(vsync))
#else
	if (SDL_GL_SetSwapInterval(vsync) == -1)
#endif
	{
		if (vsync == -1)
		{
			// Not every system supports adaptive
			// vsync, fallback to normal vsync.
			Com_Printf("Failed to set adaptive vsync, reverting to normal vsync.\n");
			SDL_GL_SetSwapInterval(1);
		}
	}

#ifdef USE_SDL3
	int vsyncState;
	if (!SDL_GL_GetSwapInterval(&vsyncState))
	{
		Com_Printf("Failed to get vsync state, assuming vsync inactive.\n");
		vsyncActive = false;
	}
	else
	{
		vsyncActive = vsyncState ? true : false;
	}
#else
	vsyncActive = SDL_GL_GetSwapInterval() != 0;
#endif
}

/*
 * This function returns the flags used at the SDL window
 * creation by GLimp_InitGraphics(). In case of error -1
 * is returned.
 */
int GL3_PrepareForWindow(void)
{
	// Mkay, let's try to load the libGL,
	const char *libgl;
	cvar_t *gl3_libgl = ri.Cvar_Get("gl3_libgl", "", CVAR_ARCHIVE);

	if (strlen(gl3_libgl->string) == 0)
	{
		libgl = NULL;
	}
	else
	{
		libgl = gl3_libgl->string;
	}

	while (1)
	{
#ifdef USE_SDL3
		if (!SDL_GL_LoadLibrary(libgl))
#else
		if (SDL_GL_LoadLibrary(libgl) < 0)
#endif
		{
			if (libgl == NULL)
			{
				Com_Error(ERR_FATAL, "%s: Couldn't load libGL: %s!",
					__func__, SDL_GetError());

				return -1;
			}
			else
			{
				Com_Printf("%s: Couldn't load libGL: %s!\n",
					__func__, SDL_GetError());
				Com_Printf("Retrying with default...\n");

				ri.Cvar_Set("gl3_libgl", "");
				libgl = NULL;
			}
		}
		else
		{
			break;
		}
	}

	// Set GL context attributs bound to the window.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifdef USE_SDL3
	if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))
#else
	if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8) == 0)
#endif
	{
		gl3config.stencil = true;
	}
	else
	{
		gl3config.stencil = false;
	}

#ifdef YQ2_GL3_GLES3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else // Desktop GL
	if (gl_version_override->value)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_version_override->value);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	// Set GL context flags.
	int contextFlags = 0;

#ifndef YQ2_GL3_GLES // Desktop GL (at least RPi4 doesn't like this for GLES3)
	contextFlags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#endif

	if (gl3_debugcontext && gl3_debugcontext->value)
	{
		contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	}

	if (contextFlags != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
	}

	// Let's see if the driver supports MSAA.
	int msaa_samples = 0;

	if (gl_msaa_samples->value)
	{
		msaa_samples = gl_msaa_samples->value;

#ifdef USE_SDL3
		if (!SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1))
#else
		if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) < 0)
#endif
		{
			Com_Printf("MSAA is unsupported: %s\n", SDL_GetError());

			ri.Cvar_SetValue ("r_msaa_samples", 0);

			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		}
#ifdef USE_SDL3
		else if (!SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa_samples))
#else
		else if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa_samples) < 0)
#endif
		{
			Com_Printf("MSAA %ix is unsupported: %s\n", msaa_samples, SDL_GetError());

			ri.Cvar_SetValue("r_msaa_samples", 0);

			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		}
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	return SDL_WINDOW_OPENGL;
}

/*
 * Initializes the OpenGL context. Returns true at
 * success and false at failure.
 */
int GL3_InitContext(void* win)
{
	// Coders are stupid.
	if (win == NULL)
	{
		Com_Error(ERR_FATAL, "%s() must not be called with NULL argument!",
			__func__);

		return false;
	}

	window = (SDL_Window *)win;

	// Initialize GL context.
	context = SDL_GL_CreateContext(window);

	if(context == NULL)
	{
		Com_Printf("%s(): Creating OpenGL Context failed: %s\n",
			__func__, SDL_GetError());

		window = NULL;

		return false;
	}

	// Check if we've got the requested MSAA.
	int msaa_samples = 0;

	if (gl_msaa_samples->value)
	{
#ifdef USE_SDL3
		if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &msaa_samples))
#else
		if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &msaa_samples) == 0)
#endif
		{
			ri.Cvar_SetValue("r_msaa_samples", msaa_samples);
		}
	}

	// Check if we've got at least 8 stencil bits
	int stencil_bits = 0;

	if (gl3config.stencil)
	{
#ifdef USE_SDL3
		if (!SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits) || stencil_bits < 8)
#else
		if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits) < 0 || stencil_bits < 8)
#endif
		{
			gl3config.stencil = false;
		}
	}

	// Enable vsync if requested.
	GL3_SetVsync();

	// Load GL pointers through GLAD and check context.
#ifdef YQ2_GL3_GLES
	if( !gladLoadGLES2Loader((void *)SDL_GL_GetProcAddress))
#else // Desktop GL
	if( !gladLoadGLLoader((void *)SDL_GL_GetProcAddress))
#endif
	{
		Com_Printf("%s(): ERROR: loading OpenGL function pointers failed!\n",
			__func__);

		return false;
	}
#ifdef YQ2_GL3_GLES3
	else if (GLVersion.major < 3)
#else // Desktop GL
	else if (GLVersion.major < 3 || (GLVersion.major == 3 && GLVersion.minor < 2))
#endif
	{
		if ((!gl_version_override->value) ||
			(GLVersion.major < gl_version_override->value))
		{
			Com_Printf("%s(): ERROR: glad only got GL version %d.%d!\n",
				__func__, GLVersion.major, GLVersion.minor);

			return false;
		}
		else
		{
			Com_Printf("%s(): Warning: glad only got GL version %d.%d.\n"
				"Some functionality could be broken.\n",
				__func__, GLVersion.major, GLVersion.minor);

		}
	}
	else
	{
		Com_Printf("Successfully loaded OpenGL function pointers using glad, got version %d.%d!\n", GLVersion.major, GLVersion.minor);
	}

#ifdef YQ2_GL3_GLES
	gl3config.debug_output = GLAD_GL_KHR_debug != 0;
#else // Desktop GL
	gl3config.debug_output = GLAD_GL_ARB_debug_output != 0;
#endif
	gl3config.anisotropic = GLAD_GL_EXT_texture_filter_anisotropic != 0;

	gl3config.major_version = GLVersion.major;
	gl3config.minor_version = GLVersion.minor;

	// Debug context setup.
	if (gl3_debugcontext && gl3_debugcontext->value && gl3config.debug_output)
	{
#ifdef YQ2_GL3_GLES
		glDebugMessageCallbackKHR(DebugCallback, NULL);

		// Call GL3_DebugCallback() synchronously, i.e. directly when and
		// where the error happens (so we can get the cause in a backtrace)
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
#else // Desktop GL
		glDebugMessageCallbackARB(DebugCallback, NULL);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
#endif
	}

	// Window title - set here so we can display renderer name in it.
	char title[64] = {0};
#ifdef YQ2_GL3_GLES3
	snprintf(title, sizeof(title), "Yamagi Quake II %s - OpenGL ES 3.0", YQ2VERSION);
#else
	snprintf(title, sizeof(title), "Yamagi Quake II %s - OpenGL 3.2", YQ2VERSION);
#endif
	SDL_SetWindowTitle(window, title);

#if SDL_VERSION_ATLEAST(2, 26, 0)
	// Figure out if we are high dpi aware.
	int flags = SDL_GetWindowFlags(win);
#ifdef USE_SDL3
	IsHighDPIaware = (flags & SDL_WINDOW_HIGH_PIXEL_DENSITY) ? true : false;
#else
	IsHighDPIaware = (flags & SDL_WINDOW_ALLOW_HIGHDPI) ? true : false;
#endif
#endif

	return true;
}

/*
 * Fills the actual size of the drawable into width and height.
 */
void GL3_GetDrawableSize(int* width, int* height)
{
#ifdef USE_SDL3
	SDL_GetWindowSizeInPixels(window, width, height);
#else
	SDL_GL_GetDrawableSize(window, width, height);
#endif
}

/*
 * Shuts the GL context down.
 */
void GL3_ShutdownContext()
{
	if (window)
	{
		if(context)
		{
#ifdef USE_SDL3
			SDL_GL_DestroyContext(context);
#else
			SDL_GL_DeleteContext(context);
#endif
			context = NULL;
		}
	}
}

/*
 * Returns the SDL major version. Implemented
 * here to not polute gl3_main.c with the SDL
 * headers.
 */
int GL3_GetSDLVersion()
{
#ifdef USE_SDL3
	int version = SDL_GetVersion();
	return SDL_VERSIONNUM_MAJOR(version);
#else
	SDL_version ver;
	SDL_VERSION(&ver);
	return ver.major;
#endif
}
