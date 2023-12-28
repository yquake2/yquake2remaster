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
 * Mesh logic
 *
 * =======================================================================
 */

#include "../ref_shared.h"

static float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "../constants/anorms.h"
};

void
R_LerpVerts(qboolean powerUpEffect, int nverts, dxtrivertx_t *v, dxtrivertx_t *ov,
		dxtrivertx_t *verts, float *lerp, const float move[3],
		const float frontv[3], const float backv[3])
{
	int i;

	if (powerUpEffect)
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			float *normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0] +
					  normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1] +
					  normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2] +
					  normal[2] * POWERSUIT_SCALE;
		}
	}
	else
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0];
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1];
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2];
		}
	}
}
