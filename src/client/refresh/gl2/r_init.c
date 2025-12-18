/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_main.c
#include "r_local.h"

refimport_t	ri;

cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_lerpmodels;
cvar_t* r_lefthand;

cvar_t* gl_ext_swapinterval;

cvar_t* r_bitdepth;
cvar_t* r_drawbuffer;
cvar_t* gl_driver;
cvar_t* r_lightmap;
cvar_t* r_mode;
cvar_t* r_dynamic;
cvar_t* r_modulate;
cvar_t* r_nobind;
cvar_t* r_picmip;
cvar_t* r_showtris;
cvar_t* r_finish;
cvar_t* r_clear;
cvar_t* r_cull;
cvar_t* r_swapinterval;
cvar_t* r_texturemode;
cvar_t* r_texturealphamode;
cvar_t* r_texturesolidmode;
cvar_t* r_lockpvs;

cvar_t* r_fullscreen;
cvar_t* r_intensity;
cvar_t* r_gamma;
cvar_t* r_renderer;

cvar_t* r_postfx_blur;
cvar_t* r_postfx_contrast;
cvar_t* r_postfx_grayscale;
cvar_t* r_postfx_inverse;
cvar_t* r_postfx_noise;

float sinTable[FUNCTABLE_SIZE];

void GL_Strings_f(void);

extern vertexbuffer_t vb_gui;
extern vertexbuffer_t vb_sky;
extern vertexbuffer_t *vb_particles;
/*
==================
R_RegisterCvarsAndCommands

register all cvars and commands
==================
*/
void R_RegisterCvarsAndCommands(void)
{
	r_lefthand = ri.Cvar_Get("cl_hand", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);

	r_fullbright = ri.Cvar_Get("r_fullbright", "0", CVAR_CHEAT);

	r_drawentities = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);

	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_nocull = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);

	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", CVAR_CHEAT);

	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_CHEAT);
	r_bitdepth = ri.Cvar_Get("r_bitdepth", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "3", CVAR_ARCHIVE);
	r_lightmap = ri.Cvar_Get("r_lightmap", "0",CVAR_CHEAT);
	r_dynamic = ri.Cvar_Get("r_dynamic", "1", CVAR_CHEAT);
	r_picmip = ri.Cvar_Get("r_picmip", "0", 0);
	
	r_nobind = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);	
	r_finish = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", CVAR_CHEAT);
	
	gl_driver = ri.Cvar_Get("gl_driver", "opengl32", CVAR_ARCHIVE);
	
	r_texturemode = ri.Cvar_Get("r_texturemode", "GL_NEAREST_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_texturealphamode = ri.Cvar_Get("r_texturealphamode", "default", CVAR_ARCHIVE);
	r_texturesolidmode = ri.Cvar_Get("r_texturesolidmode", "default", CVAR_ARCHIVE);

	gl_ext_swapinterval = ri.Cvar_Get("gl_ext_swapinterval", "1", CVAR_ARCHIVE);

	r_drawbuffer = ri.Cvar_Get("r_drawbuffer", "GL_BACK", CVAR_CHEAT);
	r_swapinterval = ri.Cvar_Get("r_swapinterval", "1", CVAR_ARCHIVE);

	r_fullscreen = ri.Cvar_Get("r_fullscreen", "0", CVAR_ARCHIVE);

	r_renderer = ri.Cvar_Get("r_renderer", DEFAULT_RENDERER, CVAR_ARCHIVE);

	r_gamma = ri.Cvar_Get("r_gamma", "1.0", CVAR_ARCHIVE);

	r_intensity = ri.Cvar_Get("r_intensity", "1.5", CVAR_CHEAT);
	r_postfx_blur = ri.Cvar_Get("r_fx_blur", "0", CVAR_CHEAT);
	r_postfx_contrast = ri.Cvar_Get("r_fx_contrast", "0", CVAR_CHEAT);
	r_postfx_grayscale = ri.Cvar_Get("r_fx_grayscale", "0", CVAR_CHEAT);
	r_postfx_inverse = ri.Cvar_Get("r_fx_inverse", "0", CVAR_CHEAT);
	r_postfx_noise = ri.Cvar_Get("r_fx_noise", "0", CVAR_CHEAT);

	ri.AddCommand("shaderlist", R_ShaderList_f);
	ri.AddCommand("imagelist", R_TextureList_f);
	ri.AddCommand("screenshot", R_ScreenShot_f);
	ri.AddCommand("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode

Sets window width & height as well as fullscreen
==================
*/
qboolean R_SetMode(void)
{
	rserr_t err;
	qboolean fullscreen;

// braxi -- maybe reuse later for smth?
//	if (r_fullscreen->modified)
//	{
//		ri.Printf(PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n");
//		ri.Cvar_SetValue("r_fullscreen", !r_fullscreen->value);
//		r_fullscreen->modified = false;
//	}

	fullscreen = r_fullscreen->value;

	r_fullscreen->modified = false;
	r_mode->modified = false;

	if ((err = GLimp_SetMode(&vid.width, &vid.height, r_mode->value, fullscreen)) == rserr_ok)
	{
		gl_state.prev_mode = r_mode->value;
	}
	else
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue("r_fullscreen", 0);
			r_fullscreen->modified = false;
			ri.Printf(PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");
			if ((err = GLimp_SetMode(&vid.width, &vid.height, r_mode->value, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("gl_mode", gl_state.prev_mode);
			r_mode->modified = false;
			ri.Printf(PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok)
		{
			ri.Printf(PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}
	return true;
}

static void R_OpenGLConfig()
{
	static char renderer_buffer[1000];
	static char vendor_buffer[1000];

	gl_config.vendor_string = glGetString(GL_VENDOR);
	gl_config.renderer_string = glGetString(GL_RENDERER);
	gl_config.version_string = glGetString(GL_VERSION);
	gl_config.extensions_string = glGetString(GL_EXTENSIONS);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gl_config.max_vertex_attribs);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gl_config.max_tmu);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &gl_config.max_frag_uniforms);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &gl_config.max_vert_uniforms);

	ri.Printf(PRINT_ALL, "\n---------- OpenGL Info ---------\n", gl_config.vendor_string);
	ri.Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
	ri.Printf(PRINT_ALL, "GL_VERSION: %s\n\n", gl_config.version_string);
	//ri.Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n\n", gl_config.extensions_string);
	ri.Printf(PRINT_ALL, "MAX_VERTEX_ATTRIBS: %i\n", gl_config.max_vertex_attribs);
	ri.Printf(PRINT_ALL, "MAX_TEXTURE_IMAGE_UNITS: %i\n", gl_config.max_tmu);
	ri.Printf(PRINT_ALL, "MAX_FRAGMENT_UNIFORM_COMPONENTS: %i\n", gl_config.max_frag_uniforms);
	ri.Printf(PRINT_ALL, "MAX_VERTEX_UNIFORM_COMPONENTS: %i\n", gl_config.max_vert_uniforms);
	ri.Printf(PRINT_ALL, "---------------------------------\n", gl_config.vendor_string);

	strcpy(renderer_buffer, gl_config.renderer_string); 
	_strlwr(renderer_buffer);

	strcpy(vendor_buffer, gl_config.vendor_string);
	_strlwr(vendor_buffer);

	if (strstr(renderer_buffer, "nvidia"))
		gl_config.renderer = GL_RENDERER_NVIDIA;
	else if (strstr(renderer_buffer, "amd"))
		gl_config.renderer = GL_RENDERER_AMD;
	else if (strstr(renderer_buffer, "intel"))
		gl_config.renderer = GL_RENDERER_INTEL;
	else
		gl_config.renderer = GL_RENDERER_OTHER;


	if (gl_config.max_tmu < MIN_TEXTURE_MAPPING_UNITS)
		ri.Error(ERR_FATAL, "Your graphics card doesn't support 4 texture mapping units");
}

/*
===============
R_Init
===============
*/
int R_Init(void* hinstance, void* hWnd)
{
	int		err;
	int		j;
	extern float r_turbsin[256];

	for (j = 0; j < 256; j++)
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Printf(PRINT_ALL, "OpenGL 1.4 renderer version : "REF_VERSION"\n");

	R_RegisterCvarsAndCommands();

	// initialize our QGL dynamic bindings
	if (!QGL_Init(gl_driver->string))
	{
		QGL_Shutdown();
		ri.Printf(PRINT_ALL, "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string);
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init(hinstance, hWnd))
	{
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if (!R_SetMode())
	{
		QGL_Shutdown();
		ri.Printf(PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
		return -1;
	}

	// load opengl
	if (!gladLoadGL())
	{
		QGL_Shutdown();
		ri.Printf(PRINT_ALL, "ref_gl::R_Init() - could not resolve opengl bindings\n");
		return -1;
	}


	// get our various GL strings and consts
	R_OpenGLConfig();

	ri.Cvar_Set("scr_drawall", "1");

	/*
	** grab extensions
	*/
#ifdef WIN32
	if (strstr(gl_config.extensions_string, "WGL_EXT_swap_control"))
	{
		qwglSwapIntervalEXT = (BOOL(WINAPI*)(int)) qwglGetProcAddress("wglSwapIntervalEXT");
		ri.Printf(PRINT_ALL, "...enabling WGL_EXT_swap_control\n");
	}
	else
	{
		ri.Printf(PRINT_ALL, "...WGL_EXT_swap_control not found\n");
	}

	srand(time(NULL));
	QueryPerformanceFrequency(&qpc_freq);

#if 0 //[ISB] SSE matrix multiplication experimental benchmark. 
	mat4_t a, b, sourcea;
	for (int i = 0; i < 16; i++)
	{
		sourcea[i] = (float)rand() / RAND_MAX;
		b[i] = (float)rand() / RAND_MAX;
	}

	LARGE_INTEGER test, test2;
	QueryPerformanceCounter(&test);
	for (int run = 0; run < 100000000; run++)
	{
		memcpy(a, sourcea, sizeof(a));
		Mat4Multiply(a, b);
	}
	QueryPerformanceCounter(&test2);
	test2.QuadPart -= test.QuadPart;
	double ms = (double)test2.QuadPart / qpc_freq.QuadPart * 1000;
	ri.Printf(PRINT_ALL, "100000000 matrix multiplies in %f MS (side effect %f)\n", ms, a[rand() & 15]);
#endif

#endif
	ri.Printf(PRINT_ALL, "--- GL_ARB_multitexture forced off ---\n");
	
//	glActiveTexture = 0;
//	glMultiTexCoord2f = 0;
	R_EnableMultiTexture();

	R_InitTextures();
	R_InitPrograms();
	R_InitFrameBuffer();

	GL_SetDefaultState();
	R_InitialOGLState(); //wip


	for (int i = 0; i < FUNCTABLE_SIZE; i++)
		sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));

	R_InitModels();
	R_InitSprites();
	R_LoadFonts();

	vb_particles = R_AllocVertexBuffer((V_UV | V_COLOR | V_NOFREE), (3 * MAX_PARTICLES), 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
		ri.Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
	return 1;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(void)
{
	ri.RemoveCommand("shaderlist");
	ri.RemoveCommand("modellist");
	ri.RemoveCommand("screenshot");
	ri.RemoveCommand("imagelist");
	ri.RemoveCommand("gl_strings");

	R_FreeFrameBuffer();
	R_FreePrograms();

	R_FreeAllModels();

	R_FreeTextures();

	// remove vertex buffers
	R_DeleteVertexBuffers(&vb_gui);
	R_DeleteVertexBuffers(&vb_sky);

	R_FreeVertexBuffer(vb_particles);

	// shut down OS specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown();
	
	// shutdown our QGL subsystem
	QGL_Shutdown();
}


//===================================================================

void R_BeginRegistration(const char* map);
void R_EndRegistration();

struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
struct image_s* R_RegisterPic(char* name);

void R_SetSky(char* name, float rotate, vec3_t axis, vec3_t color);
void R_RenderFrame(refdef_t* fd, qboolean onlyortho);


void	R_DrawImage(int x, int y, char* name);
void	R_DrawSingleChar(int x, int y, int c, int charSize);
void	R_DrawTileClear(int x, int y, int w, int h, char* name);
void	R_DrawFill(int x, int y, int w, int h);

void	R_DrawString(float x, float y, int alignx, int charSize, int fontId, vec4_t color, const char* str);
void	R_DrawStringOld(char* string, float x, float y, float fontSize, int alignx, rgba_t color);
void	R_DrawStretchedImage(rect_t rect, rgba_t color, char* pic);
void	R_NewDrawFill(rect_t rect, rgba_t color);

int R_FindFont(char* name);
int R_GetFontHeight(int fontId);
int R_GetTextWidth(int fontId, char* text);
void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text);

int R_TagIndexForName(struct model_s* model, const char* tagName);
qboolean R_LerpTag(orientation_t* tag, struct model_t* model, int startFrame, int endFrame, float frac, int tagIndex);

static void	RR_SetColor(float r, float g, float b, float a)
{
	R_ProgUniform4f(LOC_COLOR4, r, g, b, a);
}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI(refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;
	re.RenderFrame = R_RenderFrame;

	re.AppActivate = GLimp_AppActivate;

	re.BeginRegistration = R_BeginRegistration;
	re.EndRegistration = R_EndRegistration;

	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = R_RegisterPic;

	re.SetSky = R_SetSky;

	re.GetImageSize = R_GetImageSize;
	re.DrawImage = R_DrawImage;
	re.DrawStretchImage = R_DrawStretchImage;
	re.DrawSingleChar = R_DrawSingleChar;
	re.DrawTileClear = R_DrawTileClear;
	re.DrawFill = R_DrawFill;
	re.SetColor = RR_SetColor;

	re.FindFont = R_FindFont;
	re.GetFontHeight = R_GetFontHeight;
	re.GetTextWidth = R_GetTextWidth;
	re.NewDrawString = R_DrawText;

	re.TagIndexForName = R_TagIndexForName;
	re.LerpTag = R_LerpTag;

	// braxi -- newer replacements
	re.DrawString = R_DrawString;
	re._DrawString = R_DrawStringOld;
	re.DrawStretchedImage = R_DrawStretchedImage;
	re.NewDrawFill = R_NewDrawFill;


	Swap_Init();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in shared.c and win_shared.c can link
void Sys_Error(char* error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	ri.Error(ERR_FATAL, "%s", text);
}

void Com_Printf(char* fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

#endif
