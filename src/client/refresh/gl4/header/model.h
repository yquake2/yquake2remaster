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
 * Header for the model stuff.
 *
 * =======================================================================
 */

#ifndef SRC_CLIENT_REFRESH_GL4_HEADER_MODEL_H_
#define SRC_CLIENT_REFRESH_GL4_HEADER_MODEL_H_

// used for vertex array elements when drawing models
typedef struct gl4_alias_vtx_s {
	GLfloat pos[3];
	GLfloat texCoord[2];
	GLfloat color[4];
} gl4_alias_vtx_t;

/* in memory representation */

/* Whole model */

// this, must be struct model_s, not gl4model_s,
// because struct model_s* is returned by re.RegisterModel()
typedef struct model_s
{
	/* shared model definition */
	smodel_t s;

	int numsubmodels;
	struct model_s *submodels;

	/* for alias models and skins */
	struct image_s **skins;
	int numskins;
} model_t;

#endif /* SRC_CLIENT_REFRESH_GL4_HEADER_MODEL_H_ */
