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

static vec4_t *lerpbuff = NULL;
static int lerpbuffnum = 0;
float r_byteNormalScale[256];

vec4_t *
R_VertBufferRealloc(int num)
{
	void *ptr;

	if (num < lerpbuffnum)
	{
		return lerpbuff;
	}

	lerpbuffnum = num * 2;
	ptr = realloc(lerpbuff, lerpbuffnum * sizeof(vec4_t));
	YQ2_COM_CHECK_OOM(ptr, "realloc()", lerpbuffnum * sizeof(vec4_t))
	if (!ptr)
	{
		return NULL;
	}

	lerpbuff = ptr;

	return lerpbuff;
}

void
R_VertBufferInit(void)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		r_byteNormalScale[i] = (float)((signed char)i) / 127.f;
	}

	lerpbuff = NULL;
	lerpbuffnum = 0;
	R_VertBufferRealloc(MAX_VERTS);
}

void
R_VertBufferFree(void)
{
	if (lerpbuff)
	{
		free(lerpbuff);
		lerpbuff = NULL;
	}
	lerpbuffnum = 0;
}

void
R_LerpVerts(qboolean powerUpEffect, int nverts,
		const dxtrivertx_t *v, const dxtrivertx_t *ov,
		float *lerp, const float move[3],
		const float frontv[3], const float backv[3], const float *scale)
{
	if (powerUpEffect)
	{
		int i;

		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			int n;

			for (n = 0; n < 3; n ++)
			{
				float normal;

				normal = r_byteNormalScale[(unsigned char)v->normal[n]];

				lerp[n] = scale[n] * (move[n] + ov->v[n] * backv[n] + v->v[n] * frontv[n]) +
						  normal * POWERSUIT_SCALE;
			}
		}
	}
	else
	{
		int i;

		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			int n;

			for (n = 0; n < 3; n++)
			{
				lerp[n] = scale[n] * (move[n] + ov->v[n] * backv[n] + v->v[n] * frontv[n]);
			}
		}
	}
}

void
R_GenFanIndexes(unsigned short *data, unsigned from, unsigned to)
{
	int i;

	/* fill the index buffer so that we can emulate triangle fans via triangle lists */
	for (i = from; i < to; i++)
	{
		*data++ = from;
		*data++ = i + 1;
		*data++ = i + 2;
	}
}

void
R_GenStripIndexes(unsigned short *data, unsigned from, unsigned to)
{
	size_t i;

	/* fill the index buffer so that we can emulate triangle strips via triangle lists */
	for (i = from + 2; i < to + 1; i += 2)
	{
		/* add two triangles at once, because the vertex order is different
		 * for odd vs even triangles */
		*data =  i - 2;
		data ++;
		*data =  i - 1;
		data ++;
		*data =  i;
		data ++;
		*data = i + 1;
		data ++;
		*data =  i;
		data ++;
		*data =  i - 1;
		data ++;
	}

	if (i < to + 2)
	{
		/* add remaining triangle, if any */
		*data =  i - 2;
		data ++;
		*data =  i - 1;
		data ++;
		*data =  i;
	}
}
