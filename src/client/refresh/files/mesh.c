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

static void
R_StaticVerts(qboolean powerUpEffect, int nverts,
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

/* quaternion slerp for bone interpolation; assumes unit quaternions */
static void
BoneSlerp(const vec4_t qa, const vec4_t qb, float t, vec4_t out)
{
	float cosom, sclp, sclq;
	vec4_t q;
	int i;

	cosom = qa[0]*qb[0] + qa[1]*qb[1] + qa[2]*qb[2] + qa[3]*qb[3];

	if (cosom < 0.0f)
	{
		cosom = -cosom;
		for (i = 0; i < 4; i++)
		{
			q[i] = -qb[i];
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
		{
			q[i] = qb[i];
		}
	}

	if ((1.0f - cosom) > 0.00001f)
	{
		float omega = acosf(cosom);
		float sinom = sinf(omega);
		sclp = sinf((1.0f - t) * omega) / sinom;
		sclq = sinf(t * omega) / sinom;
	}
	else
	{
		sclp = 1.0f - t;
		sclq = t;
	}

	for (i = 0; i < 4; i++)
	{
		out[i] = sclp * qa[i] + sclq * q[i];
	}
}

/* build a 3x4 transform matrix from position and unit quaternion */
static void
BonePoseToMatrix(const vec3_t pos, const vec4_t q, float m[3][4])
{
	float x2 = q[0] + q[0], y2 = q[1] + q[1], z2 = q[2] + q[2];
	float xx = q[0] * x2, yy = q[1] * y2, zz = q[2] * z2;
	float xy = q[0] * y2, xz = q[0] * z2, yz = q[1] * z2;
	float wx = q[3] * x2, wy = q[3] * y2, wz = q[3] * z2;

	m[0][0] = 1.0f - (yy + zz);
	m[0][1] = xy - wz;
	m[0][2] = xz + wy;
	m[0][3] = pos[0];
	m[1][0] = xy + wz;
	m[1][1] = 1.0f - (xx + zz);
	m[1][2] = yz - wx;
	m[1][3] = pos[1];
	m[2][0] = xz - wy;
	m[2][1] = yz + wx;
	m[2][2] = 1.0f - (xx + yy);
	m[2][3] = pos[2];
}

static void
R_SkeletalVerts(const dmdx_t *pheader, int frame, int oldframe, float frontlerp,
	float backlerp, float *lerp, const float move[3], const float *scale)
{
	const dmdx_baseframe_joint_t *poses, *old_poses;
	const dmdx_weight_t *weights;
	const dmdx_vertex_t *mesh_verteces;
	int num_joints, num_verts, i;

	YQ2_VLA(float, bonematrix, pheader->num_joints * 12);

	poses = (const dmdx_baseframe_joint_t *)((const byte *)pheader + pheader->ofs_baseframe_joints)
	        + frame * pheader->num_joints;
	old_poses = (const dmdx_baseframe_joint_t *)((const byte *)pheader + pheader->ofs_baseframe_joints)
	           + oldframe * pheader->num_joints;
	weights = (const dmdx_weight_t *)((const byte *)pheader + pheader->ofs_weights);
	mesh_verteces = (const dmdx_vertex_t *)((const byte *)pheader + pheader->ofs_mesh_verteces);
	num_joints = pheader->num_joints;
	num_verts = pheader->num_xyz;

	/* lerp/slerp each bone and build its world-space matrix */
	for (i = 0; i < num_joints; i++)
	{
		vec3_t lpos;
		vec4_t lorient;
		float (*m)[4] = (float (*)[4])(bonematrix + i * 12);
		int n;

		for (n = 0; n < 3; n++)
		{
			lpos[n] = old_poses[i].pos[n] * backlerp + poses[i].pos[n] * frontlerp;
		}
		BoneSlerp(old_poses[i].orient, poses[i].orient, frontlerp, lorient);
		BonePoseToMatrix(lpos, lorient, m);
	}

	/* skin each vertex */
	for (i = 0; i < num_verts; i++, lerp += 4)
	{
		const dmdx_vertex_t *bind = &mesh_verteces[i];
		vec3_t result = { 0.0f, 0.0f, 0.0f };
		int k;

		for (k = 0; k < bind->count; k++)
		{
			const dmdx_weight_t *inf = &weights[bind->start + k];
			const vec4_t *m = (const vec4_t*)(bonematrix + inf->joint * 12);
			int n;

			for (n = 0; n < 3; n++)
			{
				result[n] += inf->bias * (
					m[n][0] * inf->pos[0] +
					m[n][1] * inf->pos[1] +
					m[n][2] * inf->pos[2] +
					m[n][3]);
			}
		}

		lerp[0] = scale[0] * (result[0] + move[0]);
		lerp[1] = scale[1] * (result[1] + move[1]);
		lerp[2] = scale[2] * (result[2] + move[2]);
	}

	YQ2_VLAFREE(bonematrix);
}

void
R_LerpVerts(const dmdx_t *paliashdr, int frame, int oldframe, float frontlerp,
	float backlerp, float *lerp, const float base_move[3], const float *scale,
	qboolean colorOnly)
{
	if (r_skeletalanimation->value && !colorOnly &&
		paliashdr->ofs_baseframe_joints != 0 && paliashdr->num_weights > 0)
	{
		R_SkeletalVerts(paliashdr, frame, oldframe, frontlerp, backlerp, lerp, base_move, scale);
	}
	else
	{
		const daliasxframe_t *oldframep, *framep;
		vec3_t frontv, backv, move;
		size_t i;

		VectorCopy(base_move, move);

		framep = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
								+ frame * paliashdr->framesize);

		oldframep = (daliasxframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
								+ oldframe * paliashdr->framesize);

		for (i = 0; i < 3; i++)
		{
			move[i] += backlerp * oldframep->translate[i] + frontlerp * framep->translate[i];

			frontv[i] = frontlerp * framep->scale[i];
			backv[i] = backlerp * oldframep->scale[i];
		}

		R_StaticVerts(colorOnly, paliashdr->num_xyz, framep->verts, oldframep->verts, lerp,
			move, frontv, backv, scale);
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
