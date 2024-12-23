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
 * Local header for the OpenGL4 refresher.
 *
 * =======================================================================
 */


#ifndef SRC_CLIENT_REFRESH_GL4_HEADER_LOCAL_H_
#define SRC_CLIENT_REFRESH_GL4_HEADER_LOCAL_H_

#ifdef IN_IDE_PARSER
  // this is just a hack to get proper auto-completion in IDEs:
  // using system headers for their parsers/indexers but glad for real build
  // (in glad glFoo is just a #define to glad_glFoo or sth, which screws up autocompletion)
  // (you may have to configure your IDE to #define IN_IDE_PARSER, but not for building!)
  #define GL_GLEXT_PROTOTYPES 1
  #include <GL/gl.h>
  #include <GL/glext.h>

#else

#include "../glad/include/glad/glad.h"

#endif

#include "../../ref_shared.h"

#include "../../files/HandmadeMath.h"

#if 0 // only use this for development ..
#define STUB_ONCE(msg) do { \
		static int show=1; \
		if(show) { \
			show = 0; \
			R_Printf(PRINT_ALL, "STUB: %s() %s\n", __FUNCTION__, msg); \
		} \
	} while(0);
#else // .. so make this a no-op in released code
#define STUB_ONCE(msg)
#endif

// a wrapper around glVertexAttribPointer() to stay sane
// (caller doesn't have to cast to GLintptr and then void*)
static inline void
qglVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset)
{
	glVertexAttribPointer(index, size, type, normalized, stride, (const void*)offset);
}

static inline void
qglVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset)
{
	glVertexAttribIPointer(index, size, type, stride, (void*)offset);
}

// attribute locations for vertex shaders
enum {
	GL4_ATTRIB_POSITION   = 0,
	GL4_ATTRIB_TEXCOORD   = 1, // for normal texture
	GL4_ATTRIB_LMTEXCOORD = 2, // for lightmap
	GL4_ATTRIB_COLOR      = 3, // per-vertex color
	GL4_ATTRIB_NORMAL     = 4, // vertex normal
	GL4_ATTRIB_LIGHTFLAGS = 5  // uint, each set bit means "dyn light i affects this surface"
};

// always using RGBA now, GLES3 on RPi4 doesn't work otherwise
// and I think all modern GPUs prefer 4byte pixels over 3bytes
static const int gl4_solid_format = GL_RGBA;
static const int gl4_alpha_format = GL_RGBA;
static const int gl4_tex_solid_format = GL_RGBA;
static const int gl4_tex_alpha_format = GL_RGBA;

extern unsigned gl4_rawpalette[256];
extern unsigned d_8to24table[256];

typedef struct
{
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *glsl_version_string;

	int major_version;
	int minor_version;

	// ----

	qboolean anisotropic; // is GL_EXT_texture_filter_anisotropic supported?
	qboolean debug_output; // is GL_ARB_debug_output supported?
	qboolean stencil; // Do we have a stencil buffer?

	qboolean useBigVBO; // workaround for AMDs windows driver for fewer calls to glBufferData()

	// ----

	float max_anisotropy;
} gl4config_t;

typedef struct
{
	GLuint shaderProgram;
	GLint uniVblend;
	GLint uniLmScalesOrTime; // for 3D it's lmScales, for 2D underwater PP it's time
	hmm_vec4 lmScales[4];
} gl4ShaderInfo_t;

typedef struct
{
	GLfloat gamma;
	GLfloat intensity;
	GLfloat intensity2D; // for HUD, menus etc

		// entries of std430 UBOs are aligned to multiples of their own size
		// so we'll need to pad accordingly for following vec4
		GLfloat _padding;

	hmm_vec4 color;
} gl4UniCommon_t;

typedef struct
{
	hmm_mat4 transMat4;
} gl4Uni2D_t;

typedef struct
{
	hmm_mat4 transProjViewMat4; // gl4state.projMat3D * gl4state.viewMat3D - so we don't have to do this in the shader
	hmm_mat4 transModelMat4;

	GLfloat sscroll; // for SURF_FLOWING
	GLfloat tscroll; // for SURF_FLOWING
	GLfloat time; // for warping surfaces like water & possibly other things
	GLfloat alpha; // for translucent surfaces (water, glass, ..)
	GLfloat overbrightbits; // gl4_overbrightbits, applied to lightmaps (and elsewhere to models)
	GLfloat particleFadeFactor; // gl4_particle_fade_factor, higher => less fading out towards edges

	GLfloat lightScaleForTurb; // surfaces with SURF_DRAWTURB (water, lava) don't have lightmaps, use this instead
	GLfloat _padding; // again, some padding to ensure this has right size, round up to 16 bytes?
} gl4Uni3D_t;

extern const hmm_mat4 gl4_identityMat4;

typedef struct
{
	vec3_t origin;
	GLfloat _padding;
	vec3_t color;
	GLfloat intensity;
} gl4UniDynLight;

typedef struct
{
	gl4UniDynLight dynLights[MAX_DLIGHTS];
	GLuint numDynLights;
	GLfloat _padding[3];
} gl4UniLights_t;

enum {
	// width and height used to be 128, so now we should be able to get the same lightmap data
	// that used 32 lightmaps before into one, so 4 lightmaps should be enough
	BLOCK_WIDTH = 1024,
	BLOCK_HEIGHT = 1024,
	MAX_LIGHTMAPS = 16,
	MAX_LIGHTMAPS_PER_SURFACE = MAXLIGHTMAPS // 4
};

typedef struct
{
	// TODO: what of this do we need?
	qboolean fullscreen;

	int prev_mode;

	// each lightmap consists of 4 sub-lightmaps allowing changing shadows on the same surface
	// used for switching on/off light and stuff like that.
	// most surfaces only have one really and the remaining for are filled with dummy data
	GLuint lightmap_textureIDs[MAX_LIGHTMAPS][MAX_LIGHTMAPS_PER_SURFACE]; // instead of lightmap_textures+i use lightmap_textureIDs[i]

	GLuint currenttexture; // bound to GL_TEXTURE0
	int currentlightmap; // lightmap_textureIDs[currentlightmap] bound to GL_TEXTURE1
	GLuint currenttmu; // GL_TEXTURE0 or GL_TEXTURE1

	// FBO for postprocess effects (like under-water-warping)
	GLuint ppFBO;
	GLuint ppFBtex; // ppFBO's texture for color buffer
	int ppFBtexWidth, ppFBtexHeight;
	GLuint ppFBrbo; // ppFBO's renderbuffer object for depth and stencil buffer
	qboolean ppFBObound; // is it currently bound (rendered to)?

	//float camera_separation;
	//enum stereo_modes stereo_mode;

	GLuint currentVAO;
	GLuint currentVBO;
	GLuint currentEBO;
	GLuint currentShaderProgram;
	GLuint currentUBO;

	// NOTE: make sure si2D is always the first shaderInfo (or adapt GL4_ShutdownShaders())
	gl4ShaderInfo_t si2D;      // shader for rendering 2D with textures
	gl4ShaderInfo_t si2Dcolor; // shader for rendering 2D with flat colors
	gl4ShaderInfo_t si2DpostProcess; // shader to render postprocess FBO, when *not* underwater
	gl4ShaderInfo_t si2DpostProcessWater; // shader to apply water-warp postprocess effect

	gl4ShaderInfo_t si3Dlm;        // a regular opaque face (e.g. from brush) with lightmap
	// TODO: lm-only variants for gl_lightmap 1
	gl4ShaderInfo_t si3Dtrans;     // transparent is always w/o lightmap
	gl4ShaderInfo_t si3DcolorOnly; // used for beams - no lightmaps
	gl4ShaderInfo_t si3Dturb;      // for water etc - always without lightmap
	gl4ShaderInfo_t si3DlmFlow;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl4ShaderInfo_t si3DtransFlow; // for transparent flowing/scrolling things (=> no lightmap)
	gl4ShaderInfo_t si3Dsky;       // guess what..
	gl4ShaderInfo_t si3Dsprite;    // for sprites
	gl4ShaderInfo_t si3DspriteAlpha; // for sprites with alpha-testing

	gl4ShaderInfo_t si3Dalias;      // for models
	gl4ShaderInfo_t si3DaliasColor; // for models w/ flat colors

	// NOTE: make sure siParticle is always the last shaderInfo (or adapt GL4_ShutdownShaders())
	gl4ShaderInfo_t siParticle; // for particles. surprising, right?

	GLuint vao3D, vbo3D; // for brushes etc, using 10 floats and one uint as vertex input (x,y,z, s,t, lms,lmt, normX,normY,normZ ; lightFlags)

	// the next two are for gl4config.useBigVBO == true
	int vbo3Dsize;
	int vbo3DcurOffset;

	GLuint vaoAlias, vboAlias, eboAlias; // for models, using 9 floats as (x,y,z, s,t, r,g,b,a)
	GLuint vaoParticle, vboParticle; // for particles, using 9 floats (x,y,z, size,distance, r,g,b,a)

	// UBOs and their data
	gl4UniCommon_t uniCommonData;
	gl4Uni2D_t uni2DData;
	gl4Uni3D_t uni3DData;
	gl4UniLights_t uniLightsData;
	GLuint uniCommonUBO;
	GLuint uni2DUBO;
	GLuint uni3DUBO;
	GLuint uniLightsUBO;

	hmm_mat4 projMat3D;
	hmm_mat4 viewMat3D;
} gl4state_t;

extern gl4config_t gl4config;
extern gl4state_t gl4state;

extern viddef_t vid;

extern refdef_t gl4_newrefdef;

extern int gl4_visframecount; /* bumped when going to a new PVS */
extern int gl4_framecount; /* used for dlight push checking */

extern int gl4_viewcluster, gl4_viewcluster2, gl4_oldviewcluster, gl4_oldviewcluster2;

extern int c_brush_polys, c_alias_polys;

extern qboolean IsHighDPIaware;

extern vec3_t lightspot;

/* NOTE: struct image_s* is what re.RegisterSkin() etc return so no gl4image_s!
 *       (I think the client only passes the pointer around and doesn't know the
 *        definition of this struct, so this being different from struct image_s
 *        in ref_gl should be ok)
 */
typedef struct image_s
{
	char name[MAX_QPATH];               /* game path, including extension */
	imagetype_t type;
	int width, height;                  /* source image */
	//int upload_width, upload_height;    /* after power of two and picmip */
	int registration_sequence;          /* 0 = free */
	struct msurface_s *texturechain;    /* for sort-by-texture world drawing */
	GLuint texnum;                      /* gl texture binding */
	float sl, tl, sh, th;               /* 0,0 - 1,1 unless part of the scrap */
	// qboolean scrap; // currently unused
	qboolean has_alpha;
	qboolean is_lava; // DG: added for lava brightness hack

} gl4image_t;

// include this down here so it can use gl4image_t
#include "model.h"

typedef struct
{
	int current_lightmap_texture; // index into gl4state.lightmap_textureIDs[]

	//msurface_t *lightmap_surfaces[MAX_LIGHTMAPS]; - no more lightmap chains, lightmaps are rendered multitextured

	int allocated[BLOCK_WIDTH];

	/* the lightmap texture data needs to be kept in
	   main memory so texsubimage can update properly */
	byte lightmap_buffers[MAX_LIGHTMAPS_PER_SURFACE][4 * BLOCK_WIDTH * BLOCK_HEIGHT];
} gl4lightmapstate_t;

extern gl4model_t *gl4_worldmodel;

extern float gl4depthmin, gl4depthmax;

extern cplane_t frustum[4];

extern vec3_t gl4_origin;

extern gl4image_t *gl4_notexture; /* use for bad textures */
extern gl4image_t *gl4_particletexture; /* little dot for particles */

extern int gl_filter_min;
extern int gl_filter_max;

static inline void
GL4_UseProgram(GLuint shaderProgram)
{
	if(shaderProgram != gl4state.currentShaderProgram)
	{
		gl4state.currentShaderProgram = shaderProgram;
		glUseProgram(shaderProgram);
	}
}

static inline void
GL4_BindVAO(GLuint vao)
{
	if(vao != gl4state.currentVAO)
	{
		gl4state.currentVAO = vao;
		glBindVertexArray(vao);
	}
}

static inline void
GL4_BindVBO(GLuint vbo)
{
	if(vbo != gl4state.currentVBO)
	{
		gl4state.currentVBO = vbo;
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
}

static inline void
GL4_BindEBO(GLuint ebo)
{
	if(ebo != gl4state.currentEBO)
	{
		gl4state.currentEBO = ebo;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	}
}

extern void GL4_BufferAndDraw3D(const mvtx_t* verts, int numVerts, GLenum drawMode);

extern void GL4_RotateForEntity(entity_t *e);

// gl4_sdl.c
extern int GL4_InitContext(void* win);
extern void GL4_GetDrawableSize(int* width, int* height);
extern int GL4_PrepareForWindow(void);
extern qboolean GL4_IsVsyncActive(void);
extern void GL4_EndFrame(void);
extern void GL4_SetVsync(void);
extern void GL4_ShutdownContext(void);
extern int GL4_GetSDLVersion(void);

// gl4_misc.c
extern void GL4_InitParticleTexture(void);
extern void GL4_ScreenShot(void);
extern void GL4_SetDefaultState(void);

// gl4_model.c
extern int registration_sequence;
extern void GL4_Mod_Init(void);
extern void GL4_Mod_FreeAll(void);
extern void GL4_BeginRegistration(const char *model);
extern struct model_s * GL4_RegisterModel(const char *name);
extern void GL4_EndRegistration(void);
extern void GL4_Mod_Modellist_f(void);
extern const byte* GL4_Mod_ClusterPVS(int cluster, const gl4model_t *model);

// gl4_draw.c
extern void GL4_Draw_InitLocal(void);
extern void GL4_Draw_ShutdownLocal(void);
extern gl4image_t * GL4_Draw_FindPic(const char *name);
extern void GL4_Draw_GetPicSize(int *w, int *h, const char *pic);

extern void GL4_Draw_PicScaled(int x, int y, const char *pic, float factor, const char *alttext);
extern void GL4_Draw_StretchPic(int x, int y, int w, int h, const char *pic);
extern void GL4_Draw_CharScaled(int x, int y, int num, float scale);
extern void GL4_Draw_TileClear(int x, int y, int w, int h, const char *pic);
extern void GL4_DrawFrameBufferObject(int x, int y, int w, int h, GLuint fboTexture, const float v_blend[4]);
extern void GL4_Draw_Fill(int x, int y, int w, int h, int c);
extern void GL4_Draw_FadeScreen(void);
extern void GL4_Draw_Flash(const float color[4], float x, float y, float w, float h);
extern void GL4_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits);

// gl4_image.c

static inline void
GL4_SelectTMU(GLenum tmu)
{
	if(gl4state.currenttmu != tmu)
	{
		glActiveTexture(tmu);
		gl4state.currenttmu = tmu;
	}
}

extern void GL4_TextureMode(char *string);
extern void GL4_Bind(GLuint texnum);
extern void GL4_BindLightmap(int lightmapnum);
extern gl4image_t *GL4_LoadPic(char *name, byte *pic, int width, int realwidth,
                               int height, int realheight, size_t data_size,
                               imagetype_t type, int bits);
extern gl4image_t *GL4_FindImage(const char *name, imagetype_t type);
extern gl4image_t *GL4_RegisterSkin(const char *name);
extern void GL4_ShutdownImages(void);
extern void GL4_FreeUnusedImages(void);
extern qboolean GL4_ImageHasFreeSpace(void);
extern void GL4_ImageList_f(void);

// gl4_light.c
extern int r_dlightframecount;
extern void GL4_PushDlights(void);
extern void GL4_BuildLightMap(msurface_t *surf, int offsetInLMbuf, int stride);

// gl4_lightmap.c
#define GL_LIGHTMAP_FORMAT GL_RGBA

extern void LM_InitBlock(void);
extern void LM_UploadBlock(void);
extern qboolean LM_AllocBlock(int w, int h, int *x, int *y);
extern void LM_CreateLightmapsPoligon(gl4model_t *currentmodel, msurface_t *fa);
extern void LM_BeginBuildingLightmaps(gl4model_t *m);
extern void LM_EndBuildingLightmaps(void);

// gl4_warp.c
extern void GL4_EmitWaterPolys(msurface_t *fa);

extern void GL4_SetSky(const char *name, float rotate, int autorotate, const vec3_t axis);
extern void GL4_DrawSkyBox(void);
extern void RE_ClearSkyBox(void);
extern void GL4_AddSkySurface(msurface_t *fa);


// gl4_surf.c
extern void GL4_SurfInit(void);
extern void GL4_SurfShutdown(void);
extern void GL4_DrawGLPoly(msurface_t *fa);
extern void GL4_DrawGLFlowingPoly(msurface_t *fa);
extern void GL4_DrawTriangleOutlines(void);
extern void GL4_DrawAlphaSurfaces(void);
extern void GL4_DrawBrushModel(entity_t *e, gl4model_t *currentmodel);
extern void GL4_DrawWorld(void);
extern void GL4_MarkLeaves(void);

// gl4_mesh.c
extern void GL4_DrawAliasModel(entity_t *e);
extern void GL4_ResetShadowAliasModels(void);
extern void GL4_DrawAliasShadows(void);
extern void GL4_ShutdownMeshes(void);

// gl4_shaders.c

extern qboolean GL4_RecreateShaders(void);
extern qboolean GL4_InitShaders(void);
extern void GL4_ShutdownShaders(void);
extern void GL4_UpdateUBOCommon(void);
extern void GL4_UpdateUBO2D(void);
extern void GL4_UpdateUBO3D(void);
extern void GL4_UpdateUBOLights(void);

// ############ Cvars ###########

extern cvar_t *gl_msaa_samples;
extern cvar_t *gl_version_override;
extern cvar_t *r_vsync;
extern cvar_t *r_retexturing;
extern cvar_t *r_scale8bittextures;
extern cvar_t *vid_fullscreen;
extern cvar_t *r_mode;
extern cvar_t *r_customwidth;
extern cvar_t *r_customheight;

extern cvar_t *r_2D_unfiltered;
extern cvar_t *r_videos_unfiltered;
extern cvar_t *r_nolerp_list;
extern cvar_t *r_lerp_list;
extern cvar_t *gl_nobind;
extern cvar_t *r_lockpvs;
extern cvar_t *r_novis;

extern cvar_t *r_cull;
extern cvar_t *gl_zfix;
extern cvar_t *r_fullbright;

extern cvar_t *r_norefresh;
extern cvar_t *gl_lefthand;
extern cvar_t *r_gunfov;
extern cvar_t *r_farsee;
extern cvar_t *r_drawworld;

extern cvar_t *vid_gamma;
extern cvar_t *gl4_intensity;
extern cvar_t *gl4_intensity_2D;
extern cvar_t *gl_anisotropic;
extern cvar_t *gl_texturemode;

extern cvar_t *r_lightlevel;
extern cvar_t *gl4_overbrightbits;
extern cvar_t *gl4_particle_fade_factor;
extern cvar_t *gl4_particle_square;
extern cvar_t *gl4_colorlight;
extern cvar_t *gl_polyblend;

extern cvar_t *r_modulate;
extern cvar_t *gl_lightmap;
extern cvar_t *gl_shadows;
extern cvar_t *r_fixsurfsky;
extern cvar_t *r_palettedtexture;
extern cvar_t *r_validation;

extern cvar_t *gl4_debugcontext;

#endif /* SRC_CLIENT_REFRESH_GL4_HEADER_LOCAL_H_ */
