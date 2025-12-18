/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

#define GL_TMUS 2 // texture mapping units, 0 = colormap, 1 = lightmap
static const GLenum textureTargets[GL_TMUS] = { GL_TEXTURE0, GL_TEXTURE1 };

// boring file for boring opengl states to avoid unnecessary calls to driver
typedef struct oglstate_s
{
	qboolean	alphaTestEnabled;
	qboolean	blendEnabled;
	qboolean	depthTestEnabled;
	qboolean	cullFaceEnabled;

	GLenum		cullface;
	GLboolean	depthmask;
	GLenum		blendfunc[2];

	// texturing
	qboolean	textureEnabled[GL_TMUS];
	qboolean	textureBind[GL_TMUS];
	GLuint		textureMappingUnit;

	float		color[4];
	float		clearcolor[4];
} oglstate_t;

static oglstate_t glstate;

// toggle on or off
#define OGL_TOGGLE_STATE_FUNC(funcName, currentState, stateVar) \
inline void funcName(qboolean newstate) { \
		if (currentState != newstate) \
		{ \
				if (newstate) \
					glEnable(stateVar); \
				else \
					glDisable(stateVar); \
					currentState = newstate; \
		}\
}

// use stateFunc to set newstate
#define OGL_STATE_FUNC(funcName, currentState, stateFunc, stateValType) \
inline void funcName(stateValType newstate) { \
		if (currentState != newstate) \
		{ \
					stateFunc(newstate); \
					currentState = newstate; \
		}\
}

// use stateFunc to set newstate (two values)
#define OGL_STATE_FUNC2(funcName, currentState, stateFunc, stateValType) \
inline void funcName(stateValType newstateA, stateValType newstateB) { \
		if (currentState[0] != newstateA || currentState[1] != newstateB) \
		{ \
					stateFunc(newstateA, newstateB); \
					currentState[0] = newstateA; \
					currentState[1] = newstateB; \
		}\
}

OGL_TOGGLE_STATE_FUNC(R_AlphaTest, glstate.alphaTestEnabled, GL_ALPHA_TEST)
OGL_TOGGLE_STATE_FUNC(R_Blend, glstate.blendEnabled, GL_BLEND)
OGL_TOGGLE_STATE_FUNC(R_DepthTest, glstate.depthTestEnabled, GL_DEPTH_TEST)
OGL_TOGGLE_STATE_FUNC(R_CullFace, glstate.cullFaceEnabled, GL_CULL_FACE)

OGL_STATE_FUNC(R_SetCullFace, glstate.cullface, glCullFace, GLenum)
OGL_STATE_FUNC(R_WriteToDepthBuffer, glstate.depthmask, glDepthMask, GLboolean)

OGL_STATE_FUNC2(R_BlendFunc, glstate.blendfunc, glBlendFunc, GLenum)


inline void R_SetColor(float r, float g, float b, float a)
{
//	TODO: uncomment whem models stop using R_Color for shading
//	if (glstate.color[0] == r && glstate.color[1] == g && glstate.color[2] == b && glstate.color[3] == a)
//		return;

	glstate.color[0] = r;
	glstate.color[1] = g;
	glstate.color[2] = b;
	glstate.color[3] = a;

	glColor4fv(glstate.color);
}

inline void R_SetClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void R_InitialOGLState()
{
	memset(&glstate, 0, sizeof(glstate_t));

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glstate.cullface = GL_NONE;
	glCullFace(glstate.cullface);

	glstate.depthmask = GL_FALSE;
	glDepthMask(glstate.depthmask);

	R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	R_SetColor(1, 1, 1, 1);
	R_SetClearColor(0, 0, 0, 1);

	// clear and disable all textures but 0
	for (int tmu = GL_TMUS-1; tmu > -1; tmu--)
	{	
		glstate.textureMappingUnit = tmu;
		glstate.textureEnabled[tmu] = (tmu == 0);
		glstate.textureBind[tmu] = 0;

		if(glActiveTexture)
			glActiveTexture(textureTargets[tmu]);

		glBindTexture(GL_TEXTURE_2D, 0);

		if (tmu == 0)
			glEnable(GL_TEXTURE_2D);
		else
			glDisable(GL_TEXTURE_2D);	
	}
}