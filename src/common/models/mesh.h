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
 * The MESH common functions
 *
 * =======================================================================
 */

#ifndef SRC_COMMON_MODELS_MESH_H_
#define SRC_COMMON_MODELS_MESH_H_

#include "../header/common.h"

void Quat_normalize(quat_t q);
void Quat_rotatePoint(const quat_t q, const vec3_t in, vec3_t out);
void Quat_toMat3(const quat_t q, float m[9]);

#endif /* SRC_COMMON_MODELS_MESH_H_ */
