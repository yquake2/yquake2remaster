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
 * Mesh handling
 *
 * =======================================================================
 */

#include "header/local.h"

static void
R_DrawAliasDrawCommands(const entity_t *currententity, int *order, const int *order_end,
	float alpha, dxtrivertx_t *verts, vec4_t *s_lerped, const float *shadelight,
	const float *shadevector)
{
#ifdef _MSC_VER // workaround for lack of VLAs (=> our workaround uses alloca() which is bad in loops)
	int maxCount = 0;
	const int* tmpOrder = order;
	while (1)
	{
		int c = *tmpOrder++;
		if (!c)
			break;
		if ( c < 0 )
			c = -c;
		if ( c > maxCount )
			maxCount = c;

		tmpOrder += 3 * c;
	}

	YQ2_VLA( GLfloat, vtx, 3 * maxCount );
	YQ2_VLA( GLfloat, tex, 2 * maxCount );
	YQ2_VLA( GLfloat, clr, 4 * maxCount );
#endif

	while (1)
	{
		unsigned short total;
		GLenum type;
		int count;

		/* get the vertex count and primitive type */
		count = *order++;

		if (!count || order >= order_end)
		{
			break; /* done */
		}

		if (count < 0)
		{
			count = -count;

			type = GL_TRIANGLE_FAN;
		}
		else
		{
			type = GL_TRIANGLE_STRIP;
		}

		total = count;

#ifndef _MSC_VER // we have real VLAs, so it's safe to use one in this loop
		YQ2_VLA(GLfloat, vtx, 3*total);
		YQ2_VLA(GLfloat, tex, 2*total);
		YQ2_VLA(GLfloat, clr, 4*total);
#endif

		if (currententity->flags &
			(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
		{
			unsigned int index_vtx = 0;
			unsigned int index_clr = 0;

			do
			{
				int index_xyz;

				index_xyz = order[2];
				order += 3;

				clr[index_clr++] = shadelight[0];
				clr[index_clr++] = shadelight[1];
				clr[index_clr++] = shadelight[2];
				clr[index_clr++] = alpha;

				vtx[index_vtx++] = s_lerped[index_xyz][0];
				vtx[index_vtx++] = s_lerped[index_xyz][1];
				vtx[index_vtx++] = s_lerped[index_xyz][2];
			}
			while (--count);
		}
		else
		{
			unsigned int index_vtx = 0;
			unsigned int index_tex = 0;
			unsigned int index_clr = 0;

			do
			{
				int i, index_xyz;
				vec3_t normal;
				float l;

				/* texture coordinates come from the draw list */
				tex[index_tex++] = ((float *) order)[0];
				tex[index_tex++] = ((float *) order)[1];

				index_xyz = order[2];
				order += 3;

				/* unpack normal */
				for(i = 0; i < 3; i++)
				{
					normal[i] = verts[index_xyz].normal[i] / 127.f;
				}

				/* normals and vertexes come from the frame list */
				/* shadevector is set above according to rotation (around Z axis I think) */
				l = DotProduct(normal, shadevector) + 1;

				clr[index_clr++] = l * shadelight[0];
				clr[index_clr++] = l * shadelight[1];
				clr[index_clr++] = l * shadelight[2];
				clr[index_clr++] = alpha;

				vtx[index_vtx++] = s_lerped[index_xyz][0];
				vtx[index_vtx++] = s_lerped[index_xyz][1];
				vtx[index_vtx++] = s_lerped[index_xyz][2];
			}
			while (--count);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, vtx);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glColorPointer(4, GL_FLOAT, 0, clr);
		glDrawArrays(type, 0, total);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	YQ2_VLAFREE( vtx );
	YQ2_VLAFREE( tex );
	YQ2_VLAFREE( clr );
}

/*
 * Interpolates between two frames and origins
 */
static void
R_DrawAliasFrameLerp(entity_t *currententity, dmdx_t *paliashdr, float backlerp,
	vec4_t *s_lerped, const float *shadelight, const float *shadevector)
{
	daliasxframe_t *frame, *oldframe;
	const dxtrivertx_t *ov;
	dxtrivertx_t *verts;
	int *order;
	float frontlerp;
	float alpha;
	vec3_t move, delta, vectors[3];
	vec3_t frontv, backv;
	int i;
	float *lerp;
	int num_mesh_nodes;
	dmdxmesh_t *mesh_nodes;
	qboolean colorOnly = 0 != (currententity->flags &
			(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE |
			 RF_SHELL_HALF_DAM));

	frame = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
							  + currententity->frame * paliashdr->framesize);
	verts = frame->verts;

	oldframe = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
				+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	if (currententity->flags & RF_TRANSLUCENT)
	{
		alpha = currententity->alpha;
	}
	else
	{
		alpha = 1.0;
	}

	if (colorOnly)
	{
		glDisable(GL_TEXTURE_2D);
	}

	frontlerp = 1.0 - backlerp;

	/* move should be the delta back to the previous frame * backlerp */
	VectorSubtract(currententity->oldorigin, currententity->origin, delta);
	AngleVectors(currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct(delta, vectors[0]); /* forward */
	move[1] = -DotProduct(delta, vectors[1]); /* left */
	move[2] = DotProduct(delta, vectors[2]); /* up */

	VectorAdd(move, oldframe->translate, move);

	for (i = 0; i < 3; i++)
	{
		move[i] = backlerp * move[i] + frontlerp * frame->translate[i];

		frontv[i] = frontlerp * frame->scale[i];
		backv[i] = backlerp * oldframe->scale[i];
	}

	lerp = s_lerped[0];

	R_LerpVerts(colorOnly, paliashdr->num_xyz, verts, ov, lerp,
		move, frontv, backv);

	num_mesh_nodes = paliashdr->num_meshes;
	mesh_nodes = (dmdxmesh_t *)((char*)paliashdr + paliashdr->ofs_meshes);

	for (i = 0; i < num_mesh_nodes; i++)
	{
		R_DrawAliasDrawCommands(currententity,
			order + mesh_nodes[i].ofs_glcmds,
			order + Q_min(paliashdr->num_glcmds,
				mesh_nodes[i].ofs_glcmds + mesh_nodes[i].num_glcmds),
			alpha, verts, s_lerped, shadelight, shadevector);
	}

	if (colorOnly)
	{
		glEnable(GL_TEXTURE_2D);
	}
}

static void
R_DrawAliasShadowCommand(const entity_t *currententity, int *order, const int *order_end,
	float height, float lheight, vec4_t *s_lerped, const float *shadevector)
{
	unsigned short total;
	vec3_t point;
	GLenum type;
	int count;

	#ifdef _MSC_VER // workaround for lack of VLAs (=> our workaround uses alloca() which is bad in loops)
	int maxCount = 0;
	const int* tmpOrder = order;
	while (1)
	{
		int c = *tmpOrder++;
		if (!c)
			break;
		if (c < 0)
			c = -c;
		if (c > maxCount)
			maxCount = c;

		tmpOrder += 3 * c;
	}

	YQ2_VLA(GLfloat, vtx, 3 * maxCount);
#endif

	while (1)
	{
		/* get the vertex count and primitive type */
		count = *order++;

		if (!count || order >= order_end)
		{
			break; /* done */
		}

		if (count < 0)
		{
			count = -count;

			type = GL_TRIANGLE_FAN;
		}
		else
		{
			type = GL_TRIANGLE_STRIP;
		}

		total = count;

#ifndef _MSC_VER // we have real VLAs, so it's safe to use one in this loop
		YQ2_VLA(GLfloat, vtx, 3*total);
#endif
		unsigned int index_vtx = 0;

		do
		{
			/* normals and vertexes come from the frame list */
			memcpy(point, s_lerped[order[2]], sizeof(point));

			point[0] -= shadevector[0] * (point[2] + lheight);
			point[1] -= shadevector[1] * (point[2] + lheight);
			point[2] = height;

			vtx[index_vtx++] = point[0];
			vtx[index_vtx++] = point[1];
			vtx[index_vtx++] = point[2];

			order += 3;
		}
		while (--count);

		glEnableClientState( GL_VERTEX_ARRAY );

		glVertexPointer( 3, GL_FLOAT, 0, vtx );
		glDrawArrays( type, 0, total );

		glDisableClientState( GL_VERTEX_ARRAY );
	}
	YQ2_VLAFREE(vtx);
}

static void
R_DrawAliasShadow(entity_t *currententity, dmdx_t *paliashdr, int posenum,
	vec4_t *s_lerped, vec3_t shadevector)
{
	int *order, i, num_mesh_nodes;
	float height = 0, lheight;
	dmdxmesh_t *mesh_nodes;

	lheight = currententity->origin[2] - lightspot[2];
	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);
	height = -lheight + 0.1f;

	/* stencilbuffer shadows */
	if (gl_state.stencil && gl1_stencilshadow->value)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_EQUAL, 1, 2);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	}

	num_mesh_nodes = paliashdr->num_meshes;
	mesh_nodes = (dmdxmesh_t *)((char*)paliashdr + paliashdr->ofs_meshes);

	for (i = 0; i < num_mesh_nodes; i++)
	{
		R_DrawAliasShadowCommand(currententity,
			order + mesh_nodes[i].ofs_glcmds,
			order + Q_min(paliashdr->num_glcmds,
				mesh_nodes[i].ofs_glcmds + mesh_nodes[i].num_glcmds),
			height, lheight, s_lerped, shadevector);
	}

	/* stencilbuffer shadows */
	if (gl_state.stencil && gl1_stencilshadow->value)
	{
		glDisable(GL_STENCIL_TEST);
	}
}

static qboolean
R_CullAliasModel(const model_t *currentmodel, vec3_t bbox[8], entity_t *e)
{
	dmdx_t *paliashdr;

	paliashdr = (dmdx_t *)currentmodel->extradata;
	if (!paliashdr)
	{
		R_Printf(PRINT_ALL, "%s %s: Model is not fully loaded\n",
				__func__, currentmodel->name);
		return true;
	}

	if ((e->frame >= paliashdr->num_frames) || (e->frame < 0))
	{
		R_Printf(PRINT_DEVELOPER, "%s %s: no such frame %d\n",
				__func__, currentmodel->name, e->frame);
		e->frame = 0;
	}

	if ((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0))
	{
		R_Printf(PRINT_DEVELOPER, "%s %s: no such oldframe %d\n",
				__func__, currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	return R_CullAliasMeshModel(paliashdr, frustum, e->frame, e->oldframe,
		e->angles, e->origin, bbox);
}

void
R_DrawAliasModel(entity_t *currententity, const model_t *currentmodel)
{
	int i;
	float an;
	const image_t *skin = NULL;
	vec3_t bbox[8];
	vec3_t shadevector, shadelight;
	dmdx_t *paliashdr;
	vec4_t *s_lerped;

	if (!(currententity->flags & RF_WEAPONMODEL))
	{
		if (R_CullAliasModel(currentmodel, bbox, currententity))
		{
			return;
		}
	}

	if (currententity->flags & RF_WEAPONMODEL)
	{
		if (gl_lefthand->value == 2)
		{
			return;
		}
	}

	R_EnableMultitexture(false);
	paliashdr = (dmdx_t *)currentmodel->extradata;

	/* get lighting information */
	if (currententity->flags &
		(RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED |
		 RF_SHELL_BLUE | RF_SHELL_DOUBLE))
	{
		VectorClear(shadelight);

		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
			shadelight[0] = 0.56;
			shadelight[1] = 0.59;
			shadelight[2] = 0.45;
		}

		if (currententity->flags & RF_SHELL_DOUBLE)
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}

		if (currententity->flags & RF_SHELL_RED)
		{
			shadelight[0] = 1.0;
		}

		if (currententity->flags & RF_SHELL_GREEN)
		{
			shadelight[1] = 1.0;
		}

		if (currententity->flags & RF_SHELL_BLUE)
		{
			shadelight[2] = 1.0;
		}
	}
	else if (currententity->flags & RF_FULLBRIGHT)
	{
		for (i = 0; i < 3; i++)
		{
			shadelight[i] = 1.0;
		}
	}
	else
	{
		if (!r_worldmodel || !r_worldmodel->lightdata)
		{
			shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
		}
		else
		{
			R_LightPoint(r_worldmodel->grid, currententity, &r_newrefdef, r_worldmodel->surfaces,
				r_worldmodel->nodes, currententity->origin, shadelight,
				r_modulate->value, lightspot);
		}

		/* player lighting hack for communication back to server */
		if (currententity->flags & RF_WEAPONMODEL)
		{
			/* pick the greatest component, which should be
			   the same as the mono value returned by software */
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
	}

	if (currententity->flags & RF_MINLIGHT)
	{
		for (i = 0; i < 3; i++)
		{
			if (shadelight[i] > 0.1)
			{
				break;
			}
		}

		if (i == 3)
		{
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if (currententity->flags & RF_GLOW)
	{
		/* bonus items will pulse with time */
		float scale;

		scale = 0.1 * sin(r_newrefdef.time * 7);

		for (i = 0; i < 3; i++)
		{
			float	min;

			min = shadelight[i] * 0.8;
			shadelight[i] += scale;

			if (shadelight[i] < min)
			{
				shadelight[i] = min;
			}
		}
	}

	// Apply gl1_overbrightbits to the mesh. If we don't do this they will appear slightly dimmer relative to walls.
	if (gl1_overbrightbits->value)
	{
		for (i = 0; i < 3; ++i)
		{
			shadelight[i] *= gl1_overbrightbits->value;
		}
	}

	/* ir goggles color override */
	if (r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags &
		RF_IR_VISIBLE)
	{
		shadelight[0] = 1.0;
		shadelight[1] = 0.0;
		shadelight[2] = 0.0;
	}

	an = currententity->angles[1] / 180 * M_PI;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize(shadevector);

	/* locate the proper data */
	c_alias_polys += paliashdr->num_tris;

	/* draw all the triangles */
	if (currententity->flags & RF_DEPTHHACK)
	{
		/* hack the depth range to prevent view model from poking into walls */
		glDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));
	}

	if (currententity->flags & RF_WEAPONMODEL)
	{
		extern void R_MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		if (gl_lefthand->value == 1.0F)
		{
			glScalef(-1, 1, 1);
		}

		float dist = (r_farsee->value == 0) ? 4096.0f : 8192.0f;

		if (r_gunfov->value < 0)
		{
			R_MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, dist);
		}
		else
		{
			R_MYgluPerspective(r_gunfov->value, (float)r_newrefdef.width / r_newrefdef.height, 4, dist);
		}

		glMatrixMode(GL_MODELVIEW);

		if (gl_lefthand->value == 1.0F)
		{
			glCullFace(GL_BACK);
		}
	}

	glPushMatrix();
	currententity->angles[PITCH] = -currententity->angles[PITCH];
	R_RotateForEntity(currententity);
	currententity->angles[PITCH] = -currententity->angles[PITCH];

	/* select skin */
	if (currententity->skin)
	{
		skin = currententity->skin; /* custom player skin */
	}
	else
	{
		if (currententity->skinnum < currentmodel->numskins)
		{
			skin = currentmodel->skins[currententity->skinnum];
		}

		if (!skin)
		{
			skin = currentmodel->skins[0];
		}
	}

	if (!skin)
	{
		skin = r_notexture; /* fallback... */
	}

	R_Bind(skin->texnum);

	/* draw it */
	glShadeModel(GL_SMOOTH);

	R_TexEnv(GL_MODULATE);

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glEnable(GL_BLEND);
	}

	if ((currententity->frame >= paliashdr->num_frames) ||
		(currententity->frame < 0))
	{
		R_Printf(PRINT_DEVELOPER, "R_DrawAliasModel %s: no such frame %d\n",
				currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ((currententity->oldframe >= paliashdr->num_frames) ||
		(currententity->oldframe < 0))
	{
		R_Printf(PRINT_DEVELOPER, "R_DrawAliasModel %s: no such oldframe %d\n",
				currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if (!r_lerpmodels->value)
	{
		currententity->backlerp = 0;
	}

	/* buffer for scalled vert from frame */
	s_lerped = R_VertBufferRealloc(paliashdr->num_xyz);

	R_DrawAliasFrameLerp(currententity, paliashdr, currententity->backlerp,
		s_lerped, shadelight, shadevector);

	R_TexEnv(GL_REPLACE);
	glShadeModel(GL_FLAT);

	glPopMatrix();

	if (gl_showbbox->value)
	{
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLE_STRIP);

		for (i = 0; i < 8; i++)
		{
			glVertex3fv(bbox[i]);
		}

		glEnd();
		glEnable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}

	if (currententity->flags & RF_WEAPONMODEL)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		if (gl_lefthand->value == 1.0F)
			glCullFace(GL_FRONT);
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		glDisable(GL_BLEND);
	}

	if (currententity->flags & RF_DEPTHHACK)
	{
		glDepthRange(gldepthmin, gldepthmax);
	}

	if (gl_shadows->value &&
		!(currententity->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL | RF_NOSHADOW)))
	{
		glPushMatrix();

		/* don't rotate shadows on ungodly axes */
		glTranslatef(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
		glRotatef(currententity->angles[1], 0, 0, 1);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glColor4f(0, 0, 0, 0.5f);
		R_DrawAliasShadow(currententity, paliashdr, currententity->frame,
			s_lerped, shadevector);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	glColor4f(1, 1, 1, 1);
}
