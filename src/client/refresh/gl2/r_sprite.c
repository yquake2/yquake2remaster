/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================
QUAKE2 SPRITES
==============================================================================
*/

#include "r_local.h"

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

static GLuint spritebufferhack;

void R_InitSprites()
{
	int spritebuffer[] = { 0, 1, 2, 3 };
	glGenBuffers(1, &spritebufferhack);
	glBindBuffer(GL_ARRAY_BUFFER, spritebufferhack);
	glBufferData(GL_ARRAY_BUFFER, sizeof(spritebuffer), spritebuffer, GL_STATIC_DRAW);
}

void R_DrawSprite(rentity_t* ent)
{
	//vec3_t	point;
	sp2Frame_t* frame;
	float* up, * right;
	sp2Header_t* psprite;

	psprite = (sp2Header_t*)pCurrentModel->extradata;

	// advance frame
	ent->frame %= psprite->numframes;

	frame = &psprite->frames[ent->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		vec3_t		v_forward, v_right, v_up;

		AngleVectors(currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	glColor4f(1, 1, 1, pCurrentRefEnt->alpha);

	R_BindProgram(GLPROG_SPRITE);
	R_ProgUniformVec3(LOC_SHADECOLOR, model_shadelight);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);
	//recycling uniforms.. UNDONE it's not a good idea with the common update
	//R_ProgUniform4f(LOC_DLIGHT_POS_AND_RAD, frame->origin_x, frame->origin_y, frame->width, frame->height);

	if (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
		R_ProgUniform1f(LOC_PARM0, 1);
	else
		R_ProgUniform1f(LOC_PARM0, 0);

	if (ent->renderfx & RF_TRANSLUCENT)
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, ent->alpha);
	else
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

	R_MultiTextureBind(TMU_DIFFUSE, pCurrentModel->images[ent->frame]->texnum);
	R_AlphaTest(ent->alpha == 1.0);

	//Needed only because OpenGL 2.x doesn't have gl_VertexID in shader. Cool.
	glBindBuffer(GL_ARRAY_BUFFER, spritebufferhack);
	glEnableVertexAttribArray(0);
	glBindAttribLocation(pCurrentProgram->programObject, 0, "vertexnum");
	glVertexAttribPointer(0, 1, GL_INT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(0);

	/*glBegin(GL_TRIANGLES);
	{

		glTexCoord2f(0, 1);
		VectorMA(vec3_origin, -frame->origin_y, up, point);
		VectorMA(point, -frame->origin_x, right, point);
		glVertex3fv(point);

		glTexCoord2f(0, 0);
		VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
		VectorMA(point, -frame->origin_x, right, point);
		glVertex3fv(point);

		glTexCoord2f(1, 0);
		VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
		VectorMA(point, frame->width - frame->origin_x, right, point);
		glVertex3fv(point);

		glTexCoord2f(0, 1);
		VectorMA(vec3_origin, -frame->origin_y, up, point);
		VectorMA(point, -frame->origin_x, right, point);
		glVertex3fv(point);

		glTexCoord2f(1, 0);
		VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
		VectorMA(point, frame->width - frame->origin_x, right, point);
		glVertex3fv(point);

		glTexCoord2f(1, 1);
		VectorMA(vec3_origin, -frame->origin_y, up, point);
		VectorMA(point, frame->width - frame->origin_x, right, point);
		glVertex3fv(point);
	}
	glEnd();*/

	R_AlphaTest(false);
}