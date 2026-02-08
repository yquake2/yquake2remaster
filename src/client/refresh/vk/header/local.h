/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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
 * Local header for the refresher.
 *
 * =======================================================================
 */

#ifndef __VK_LOCAL_H__
#define __VK_LOCAL_H__

#include <stdio.h>
#include <math.h>

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#else
#if defined(__APPLE__)
#include <SDL.h>
#include <SDL_vulkan.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif
#endif


#include "../../ref_shared.h"
#include "../volk/volk.h"
#include "qvk.h"

#if defined(__APPLE__)
#include <MoltenVK/vk_mvk_moltenvk.h>
#include <dlfcn.h>
#endif

// verify if VkResult is VK_SUCCESS
#define VK_VERIFY(x) { \
	VkResult res = (x); \
	if(res != VK_SUCCESS) { \
		Com_Printf("%s:%d: VkResult verification failed: %s\n", \
			 __func__, __LINE__, QVk_GetError(res)); \
	} \
}

typedef struct image_s
{
	char name[MAX_QPATH];               /* game path, including extension */
	imagetype_t type;
	int width, height;                  /* source image */
	int upload_width, upload_height;    /* after power of two and picmip */
	int registration_sequence;          /* 0 = free */
	struct msurface_s *texturechain;    /* for sort-by-texture world drawing */
	qvktexture_t vk_texture;            /* Vulkan texture handle */
} image_t;

//===================================================================

#include "model.h"

//====================================================

extern	image_t		vktextures[MAX_TEXTURES];
extern	int			numvktextures;

extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
extern	image_t		*r_squaretexture;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;

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
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*vk_znear;
extern	cvar_t	*vk_overbrightbits;
extern	cvar_t	*vk_picmip;
extern	cvar_t	*vk_finish;
extern	cvar_t	*vk_texturemode;
extern	cvar_t	*vk_lmaptexturemode;
extern	cvar_t	*vk_sampleshading;
extern	cvar_t	*vk_device_idx;
extern	cvar_t	*vk_mip_nearfilter;
#if defined(__APPLE__)
extern  cvar_t  *vk_molten_fastmath;
extern  cvar_t  *vk_molten_metalbuffers;
#endif
extern	cvar_t	*vk_pixel_size;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_viewproj_matrix[16];

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	unsigned	r_rawpalette[256];
extern	qvktexture_t	vk_rawTexture;
extern	int	vk_activeBufferIdx;
extern	float	r_view_matrix[16];
extern	float	r_projection_matrix[16];
extern	float	r_viewproj_matrix[16];
extern	vec3_t	lightspot;

extern	int		registration_sequence;
extern	int		r_dlightframecount;
extern	qvksampler_t vk_current_sampler;
extern	qvksampler_t vk_current_lmap_sampler;

void	 RE_Shutdown( void );

void Vk_ScreenShot_f (void);
void Vk_Strings_f(void);
void Vk_Mem_f(void);

void RI_PushDlights(void);

void R_DrawAliasModel(entity_t *currententity, const model_t *currentmodel);
void R_DrawBrushModel(entity_t *currententity, const model_t *currentmodel);
void R_DrawBeam(entity_t *currententity);
void R_DrawWorld(void);
void R_RenderDlights(void);
void R_DrawAlphaSurfaces(void);
void RE_InitParticleTexture(void);
void Draw_InitLocal(void);
void Draw_FreeLocal(void);
void R_RotateForEntity(entity_t *e, float *mvMatrix);
void R_MarkLeaves(void);

void EmitWaterPolys(msurface_t *fa, image_t *texture,
				   const float *modelMatrix, const float *color,
				   qboolean solid_surface);
void RE_AddSkySurface(msurface_t *fa);
void RE_ClearSkyBox(void);
void R_DrawSkyBox(void);

struct image_s	*RE_Draw_FindPic (const char *name);

void	RE_Draw_GetPicSize (int *w, int *h, const char *name);
void	RE_Draw_PicScaled (int x, int y, const char *name, float scale, const char *alttext);
void	RE_Draw_StretchPic (int x, int y, int w, int h, const char *name);
void	RE_Draw_CharScaled (int x, int y, int num, float scale);
void	RE_Draw_StringScaled(int x, int y, float scale, qboolean alt, const char *message);
void	RE_Draw_TileClear (int x, int y, int w, int h, const char *name);
void	RE_Draw_Fill (int x, int y, int w, int h, int c);
void	RE_Draw_FadeScreen (void);
void	RE_Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int bits);

qboolean	RE_EndWorldRenderpass( void );

struct image_s *RE_RegisterSkin (const char *name);

image_t *Vk_LoadPic(const char *name, byte *pic, int width, int realwidth,
		    int height, int realheight, size_t data_size, imagetype_t type,
		    int bits);
image_t	*Vk_FindImage (const char *name, imagetype_t type);
void	Vk_TextureMode(const char *string);
void	Vk_LmapTextureMode(const char *string);
void	Vk_ImageList_f (void);

void LM_CreateLightmapsPoligon(model_t *currentmodel, msurface_t *fa);
void LM_EndBuildingLightmaps(void);
void LM_BeginBuildingLightmaps(model_t *m);

void	Vk_InitImages (void);
void	Vk_ShutdownImages (void);
void	Vk_FreeUnusedImages (void);
qboolean Vk_ImageHasFreeSpace(void);

void LM_InitBlock(void);
qboolean LM_AllocBlock(int w, int h, int *x, int *y);

void	RE_BeginRegistration (const char *model);
struct model_s	*RE_RegisterModel (const char *name);
struct image_s	*RE_RegisterSkin (const char *name);
void	RE_SetSky (const char *name, float rotate, int autorotate, const vec3_t axis);
void	RE_EndRegistration (void);

void Mat_Identity(float *matrix);
void Mat_Mul(float *m1, float *m2, float *res);
void Mat_Translate(float *matrix, float x, float y, float z);
void Mat_Rotate(float *matrix, float deg, float x, float y, float z);
void Mat_Scale(float *matrix, float x, float y, float z);
void Mat_Perspective(float *matrix, float *correction_matrix, float fovy, float aspect, float zNear, float zFar);
void Mat_Ortho(float *matrix, float left, float right, float bottom, float top, float zNear, float zFar);

typedef struct
{
	uint32_t    vk_version;
	const char *vendor_name;
	const char *device_type;
	const char *present_mode;
	const char *supported_present_modes[256];
	const char *extensions[256];
	const char *layers[256];
	uint32_t    vertex_buffer_usage;
	uint32_t    vertex_buffer_max_usage;
	uint32_t    vertex_buffer_size;
	uint32_t    index_buffer_usage;
	uint32_t    index_buffer_max_usage;
	uint32_t    index_buffer_size;
	uint32_t    uniform_buffer_usage;
	uint32_t    uniform_buffer_max_usage;
	uint32_t    uniform_buffer_size;
} vkconfig_t;

#define MAX_LIGHTMAPS 256
#define DYNLIGHTMAP_OFFSET MAX_LIGHTMAPS

#define BLOCK_WIDTH 1024
#define BLOCK_HEIGHT 1024

typedef struct
{
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} vklightmapstate_t;

extern vklightmapstate_t vk_lms;

typedef struct
{
	float inverse_intensity;
	qboolean fullscreen;

	int prev_mode;

	unsigned char *d_16to8table;

	qvktexture_t lightmap_textures[MAX_LIGHTMAPS*2];

	int currenttextures[2];
	int currenttmu;

	float camera_separation;
	qboolean stereo_enabled;

	VkPipeline current_pipeline;
	qvkrenderpasstype_t current_renderpass;
} vkstate_t;

extern vkconfig_t  vk_config;
extern vkstate_t   vk_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

qboolean Vkimp_CreateSurface(SDL_Window *window);

extern mvtx_t	*verts_buffer;
extern uint16_t	*vertIdxData;
extern int drawCalls;

void	Mesh_Init (void);
void	Mesh_Free (void);
int Mesh_VertsRealloc(int count);

// All renders should export such function
Q2_DLL_EXPORTED refexport_t GetRefAPI(refimport_t imp);

#endif
