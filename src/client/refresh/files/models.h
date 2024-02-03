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
 * The shared model structures file format
 *
 * =======================================================================
 */

#ifndef SRC_CLIENT_REFRESH_FILES_MODELS_H_
#define SRC_CLIENT_REFRESH_FILES_MODELS_H_

#include "../ref_shared.h"

/* Unpacked vertex info model convert */
typedef struct dmdx_vert_s
{
	vec3_t xyz;
	vec3_t norm;
} dmdx_vert_t;

#endif /* SRC_CLIENT_REFRESH_FILES_MODELS_H_ */
