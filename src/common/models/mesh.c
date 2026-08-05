/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 2005-2015 David HENRY
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

#include "mesh.h"

void
Quat_normalize(quat_t q)
{
	/* compute magnitude of the quaternion */
	float mag = sqrt ((q[0] * q[0]) + (q[1] * q[1])
		+ (q[2] * q[2]) + (q[3] * q[3]));

	/* check for bogus length, to protect against divide by zero */
	if (mag > 0.0f)
	{
		/* normalize it */
		float oneOverMag = 1.0f / mag;

		q[0] *= oneOverMag;
		q[1] *= oneOverMag;
		q[2] *= oneOverMag;
		q[3] *= oneOverMag;
	}
}

static void
Quat_multVec(const quat_t q, const vec3_t v, quat_t out)
{
	out[3] = - (q[0] * v[0]) - (q[1] * v[1]) - (q[2] * v[2]);
	out[0] =   (q[3] * v[0]) + (q[1] * v[2]) - (q[2] * v[1]);
	out[1] =   (q[3] * v[1]) + (q[2] * v[0]) - (q[0] * v[2]);
	out[2] =   (q[3] * v[2]) + (q[0] * v[1]) - (q[1] * v[0]);
}

void
Quat_rotatePoint(const quat_t q, const vec3_t in, vec3_t out)
{
	quat_t tmp, inv, final;

	inv[0] = -q[0]; inv[1] = -q[1];
	inv[2] = -q[2]; inv[3] =  q[3];

	Quat_normalize(inv);

	Quat_multVec(q, in, tmp);
	QuatMultiply(tmp, inv, final);

	out[0] = final[0];
	out[1] = final[1];
	out[2] = final[2];
}
