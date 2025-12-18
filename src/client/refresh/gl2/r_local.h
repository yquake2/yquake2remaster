/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdio.h>

#if 0
#include "include/glad/glad_gl33core.h"
#else
#include "include/glad/glad_21.h"
#endif

#include <math.h>
#include "../qcommon/renderer.h"
#include "win_qgl.h"

#define	REF_VERSION	"0.5-next"

#define DECOUPLED_LM 1

//===================================================================
// used to generate sin tables
//===================================================================

//#ifndef RAD2DEG
//#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI ) // unused
//#endif

#ifndef DEG2RAD
#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#endif

#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)

//===================================================================


#define	PITCH	0	// up / down
#define	YAW		1	// left / right
#define	ROLL	2	// fall over

typedef struct
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;

extern	viddef_t	vid;

typedef enum 
{
	it_model,
	//it_font,
	it_sprite,
	it_texture,
	it_gui,
	it_font,
	it_sky
} texType_t;

typedef struct image_s
{
	char			name[MAX_QPATH];			// game path, including extension
	texType_t		type;
	int				width, height;				// source image
	int				upload_width, upload_height;// after power of two and picmip
	int				registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;			// for sort-by-texture world drawing
	int				texnum;						// gl texture binding
	float			sl, tl, sh, th;				// 0,0 - 1,1
	qboolean		has_alpha;
} image_t;


//===================================================================
// r_fbo.c
//===================================================================
qboolean R_InitFrameBuffer();
void R_FreeFrameBuffer();
void R_DrawFBO(int x, int y, int w, int h, qboolean diffuse);

//===================================================================
// r_progs.c
//===================================================================
enum
{
	GLPROG_WORLD,
	GLPROG_SKY,
	GLPROG_ALIAS,
	GLPROG_SPRITE,
	GLPROG_PARTICLE,
	GLPROG_GUI,
	GLPROG_POSTFX,
	GLPROG_DEBUGSTRING,
	GLPROG_DEBUGLINE,
	MAX_GLPROGS
};
typedef enum
{
	LOC_COLORMAP,
	LOC_LIGHTMAP,
	LOC_LIGHTSTYLES,
	LOC_SCALE,
	LOC_COLOR4,
	LOC_TIME,
	LOC_SHADEVECTOR,
	LOC_SHADECOLOR,
	LOC_LERPFRAC,
	LOC_WARPSTRENGTH,
	LOC_FLOWSTRENGTH,

	LOC_PARM0,
	LOC_PARM1,
	LOC_PARM2,

	LOC_SCREENSIZE,
	LOC_INTENSITY,
	LOC_GAMMA,
	LOC_BLUR,
	LOC_CONTRAST,
	LOC_GRAYSCALE,
	LOC_INVERSE,
	LOC_NOISE,

	LOC_DLIGHT_COUNT,
	LOC_DLIGHT_COLORS,
	LOC_DLIGHT_POS_AND_RAD,
	LOC_DLIGHT_DIR_AND_CUTOFF,

	LOC_PROJECTION,
	LOC_MODELVIEW,
	LOC_LOCALMODELVIEW, //Used to properly light models

	NUM_LOCS,
} glprogLoc_t;

typedef enum
{
	VALOC_POS,
	VALOC_NORMAL,
	VALOC_TEXCOORD,
	VALOC_LMTEXCOORD, // BSP ONLY
	VALOC_ALPHA,		// BSP ONLY?
	VALOC_COLOR,
	VALOC_OLD_POS,
	VALOC_OLD_NORMAL,
	NUM_VALOCS
} glprogLoc_t;

typedef struct glprog_s
{
	char			name[MAX_QPATH];
	int				index;
	int				locs[NUM_LOCS];
	int				valocs[NUM_VALOCS];
	unsigned int	programObject; /*GLuint*/ 
	unsigned int	vertexShader, fragmentShader; /*GLuint*/ 
	qboolean		isValid;
} glprog_t;

extern glprog_t glprogs[MAX_GLPROGS];
extern glprog_t* pCurrentProgram;
extern int numProgs;

glprog_t* R_ProgramIndex(int progindex);
void R_BindProgram(int progindex);
void R_UnbindProgram();
void R_FreePrograms();
void R_InitPrograms();

void R_ProgUniform1i(int uniform, int val);
void R_ProgUniform1f(int uniform, float val);
void R_ProgUniform2i(int uniform, int val, int val2);
void R_ProgUniform2f(int uniform, float val, float val2);
void R_ProgUniformVec2(int uniform, vec2_t v);
void R_ProgUniform3i(int uniform, int val, int val2, int val3);
void R_ProgUniform3f(int uniform, float val, float val2, float val3);
void R_ProgUniformVec3(int uniform, vec3_t v);
void R_ProgUniform4i(int uniform, int val, int val2, int val3, int val4);
void R_ProgUniform4f(int uniform, float val, float val2, float val3, float val4);
void R_ProgUniformVec4(int uniform, vec4_t v);
void R_ProgUniform3fv(int uniform, int count, float* val);
void R_ProgUniform4fv(int uniform, int count, float* val);
void R_ProgUniformMatrix4fv(int uniform, int count, float* val);
int R_GetProgAttribLoc(glprogLoc_t attrib);
char* R_GetProgAttribName(glprogLoc_t attrib);
char* R_GetCurrentProgramName();
qboolean R_UsingProgram();
void R_UpdateCommonProgUniforms(qboolean orthoonly);
void R_ShaderList_f(void);

//===================================================================
// r_vertexbuffer.c
//===================================================================
typedef struct
{
	float	xyz[3];
	float	normal[3];
	float	st[2];
	float	rgba[4];
} glvert_t;

typedef enum
{
	V_UV = 1,
	V_NORMAL = 2,
	V_COLOR = 4,
	V_INDICES = 8,
	V_NOFREE = 16	// don't free verts allocated by R_AllocVertexBuffer after upload to gpu
} vboFlags_t;

typedef struct
{
	unsigned int	vboBuf; //GLuint
	unsigned int	indexBuf;

	vboFlags_t		flags;
	unsigned int	numVerts;
	unsigned int	numIndices;

	glvert_t		*verts;		// pointer to first verticle, NULL if not allocated by VBO!
	unsigned short	*indices;	// pointer to indices, NULL if not allocated by VBO!
} vertexbuffer_t;

vertexbuffer_t* R_AllocVertexBuffer(vboFlags_t flags, unsigned int numVerts, unsigned int numIndices);
void R_UpdateVertexBuffer(vertexbuffer_t* vbo, glvert_t* verts, unsigned int numVerts, vboFlags_t flags);
void R_UpdateVertexBufferIndices(vertexbuffer_t* vbo, unsigned short* indices, unsigned int numIndices);
void R_DrawVertexBuffer(vertexbuffer_t* vbo, unsigned int startVert, unsigned int numVerts);
void R_DeleteVertexBuffers(vertexbuffer_t* vbo); // this is handy for stack allocated vbos
void R_FreeVertexBuffer(vertexbuffer_t* vbo);

//===================================================================


#include "r_model.h"


#define BACKFACE_EPSILON	0.01


//====================================================

extern	rentity_t	*pCurrentRefEnt;
extern	model_t		*pCurrentModel;

//====================================================

extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];

extern	float		gldepthmin, gldepthmax;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int			r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;
extern cvar_t	*gl_ext_swapinterval;

extern	cvar_t	*r_bitdepth;
extern	cvar_t	*r_mode;
extern	cvar_t	*r_lightmap;
extern	cvar_t	*r_dynamic;
extern	cvar_t	*r_nobind;
extern	cvar_t	*r_picmip;
extern	cvar_t	*r_showtris;
extern	cvar_t	*r_finish;
extern	cvar_t	*r_clear;
extern	cvar_t	*r_cull;
extern	cvar_t	*r_modulate;
extern	cvar_t	*r_drawbuffer;
extern  cvar_t  *gl_driver;
extern	cvar_t	*r_swapinterval;
extern	cvar_t	*r_texturemode;
extern	cvar_t	*r_texturealphamode;
extern	cvar_t	*r_texturesolidmode;
extern  cvar_t  *r_lockpvs;

extern	cvar_t	*r_fullscreen;
extern	cvar_t	*r_gamma;
extern	cvar_t	*r_intensity;

extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern mat4_t r_world_matrix;
extern mat4_t r_local_matrix; //Transforms a vertex into its final position in the world
extern mat4_t r_projection_matrix;
extern mat4_t r_ortho_matrix;


//===================================================================
// r_model.c
//===================================================================
extern model_t *r_worldmodel;
extern model_t* r_defaultmodel;

extern int registration_sequence;
void R_InitSprites();

//===================================================================
// r_init.c
//===================================================================
int R_Init( void *hinstance, void *hWnd );
void R_Shutdown( void );

//===================================================================
// r_misc.c
//===================================================================
void R_InitCodeTextures();
void R_ScreenShot_f(void);
void GL_SetDefaultState(void);
void GL_UpdateSwapInterval(void);

//===================================================================
// r_world.c
//===================================================================
void R_DrawWorld();
void R_World_MarkLeaves();
void R_World_DrawAlphaSurfaces(); //old rendering path
void R_DrawBrushModel(rentity_t* e);

//===================================================================
// r_warp.c
//===================================================================
void R_SubdivideSurface(msurface_t* fa);
void R_World_DrawUnlitWaterSurf (msurface_t *fa); //old rendering path
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox();
void R_DrawSkyBox();

//===================================================================
// r_main.c 
//===================================================================
void R_RenderView(refdef_t* fd);
void R_BeginFrame(float camera_separation);
void R_RotateForEntity(rentity_t* e);
qboolean R_CullBox(vec3_t mins, vec3_t maxs);
void R_DrawBeam(rentity_t* e);

//===================================================================
// r_light.c
//===================================================================
void R_MarkLights(dlight_t* light, vec3_t lightorg, int bit, mnode_t* node);
void R_LightPoint(vec3_t p, vec3_t color);
void R_PushDlights(void);
void R_SendDynamicLightsToCurrentProgram();
void R_RenderDlights(void); // development aid

//===================================================================
// shared.c
//===================================================================
// void COM_StripExtension (char *in, char *out); //unused in render


//===================================================================
// r_draw.c
//===================================================================
void R_LoadFonts();
void R_GetImageSize (int *w, int *h, char *name);
void R_DrawImage (int x, int y, char *name);
void R_DrawStretchImage (int x, int y, int w, int h, char *name);
void R_DrawSingleChar (int x, int y, int c, int charSize);
void R_DrawTileClear (int x, int y, int w, int h, char *name);
void R_DrawFill (int x, int y, int w, int h);

//===================================================================
// r_image.c
//===================================================================
struct image_s* R_RegisterSkin(char* name);
void R_EnableMultiTexture();
void R_DisableMultiTexture();
void R_SelectTextureUnit(unsigned int textureMappingUnit);
void R_BindTexture(int texnum);
void R_MultiTextureBind(unsigned int tmu, int texnum);
image_t *R_LoadTexture(char *name, byte *pixels, int width, int height, texType_t type, int bits);
image_t	*R_FindTexture(char *name, texType_t type, qboolean load);
void R_SetTextureMode(char *string);
void R_TextureList_f(void);
void R_InitTextures();
void R_FreeTextures();
void R_FreeUnusedTextures();
void R_SetTextureAlphaMode(char *string);
void R_TextureSolidMode(char *string);

#define	TEXNUM_IMAGES		1153
#define	MAX_GLTEXTURES		1024

#define MIN_TEXTURE_MAPPING_UNITS 4

enum
{
	TMU_DIFFUSE,
	TMU_LIGHTMAP,
};

extern int gl_filter_min, gl_filter_max;

extern	image_t	r_textures[MAX_GLTEXTURES];
extern	int		r_textures_count;

extern	image_t* r_texture_white;
extern	image_t* r_texture_missing;
extern	image_t* r_texture_particle;
extern	image_t* r_texture_view;

//===================================================================
// GL config stuff
//===================================================================
#define GL_RENDERER_OTHER		0
#define GL_RENDERER_NVIDIA		1
#define GL_RENDERER_AMD			2
#define GL_RENDERER_INTEL		3

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;

	int			max_vertex_attribs;
	int			max_tmu;
	int			max_frag_uniforms;
	int			max_vert_uniforms;
} glconfig_t;

typedef struct
{
	qboolean fullscreen;
	int     prev_mode;			// previous r_mode->value

	int lightmap_textures;		// TEXNUM_LIGHTMAPS + NUM_LIGHTMAPS

	int current_lightmap;		// currently bound lightmap textureset
	int	current_texture[5];		// currently bound textures [TEX0-TEX4]
	int current_tmu;			// GL_TEXTURE0 to GL_TEXTURE4

	float camera_separation;
	qboolean stereo_enabled;
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

/*
====================================================================
a better gl state tracker
====================================================================
*/

extern inline void R_AlphaTest(qboolean enable);
extern inline void R_Blend(qboolean enable);
extern inline void R_DepthTest(qboolean enable);
extern inline void R_CullFace(qboolean enable);

extern inline void R_SetCullFace(GLenum newstate);
extern inline void R_WriteToDepthBuffer(GLboolean newstate);
extern inline void R_BlendFunc(GLenum newstateA, GLenum newstateB);

extern inline void R_SetClearColor(float r, float g, float b, float a);

extern void R_InitialOGLState();

/*
====================================================================
Live profiling (currently windows only) + counters
====================================================================
*/

typedef struct
{
	int	brush_polys;
	int brush_tris;
	int brush_drawcalls;
	int	brush_textures;

	int alias_tris;
	int alias_drawcalls;

	int	texture_binds[MIN_TEXTURE_MAPPING_UNITS];
} rperfcounters_t;

extern rperfcounters_t rperf;

#define NUM_TIMESAMPLES 16
typedef enum
{
	STAGE_START,
	STAGE_SETUP,
	STAGE_DRAWWORLD,
	STAGE_ENTITIES,
	STAGE_DEBUG,
	STAGE_ALPHASURFS,
	STAGE_PARTICLES,
	STAGE_TOTAL,
	NUM_PROFILES
} profiletype_e;

#ifdef _WIN32
extern LARGE_INTEGER qpc_freq;
extern LARGE_INTEGER qpc_samples[NUM_PROFILES];
#endif

extern double lastsamples[NUM_PROFILES][NUM_TIMESAMPLES];

//Starts profiling. Will record the time it was called at.
void R_StartProfiling();
//Called at the end of a stage, compares the stage's time to the previous,
//unless that stage is STAGE_TOTAL, at which it compares to the time at the call of R_StartProfiling.
void R_ProfileAtStage(profiletype_e stage);
//Finishes profiling by converting all the relevant numbers into milliseconds and incrementing the sample number.
void R_FinishProfiling();

//Draws the profiling report to screen.
void R_DrawProfilingReport();

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS - win_opengl.c

====================================================================
*/

void GLimp_BeginFrame( float camera_separation );
void GLimp_EndFrame( void );
int GLimp_Init( void *hinstance, void *hWnd );
void GLimp_Shutdown( void );
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void GLimp_AppActivate( qboolean active );

typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;