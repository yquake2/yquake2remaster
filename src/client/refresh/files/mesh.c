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
#include "../../../common/models/mesh.h"

#define MAX_BONES 64

typedef struct
{
	float rot[9];
	vec3_t pos;
} skeletal_bone_t;

static vec4_t *lerpbuff = NULL;
static size_t lerpbuffnum = 0;
static skeletal_bone_t *bonesbuff = NULL;
static size_t bonesbuffnum = 0;
float r_byteNormalScale[256];

vec4_t *
R_VertBufferRealloc(size_t num)
{
	void *ptr;

	if (num < lerpbuffnum)
	{
		return lerpbuff;
	}

	lerpbuffnum = ROUNDUP(num * 2, 32);
	ptr = realloc(lerpbuff, lerpbuffnum * sizeof(vec4_t));
	YQ2_COM_CHECK_OOM(ptr, "realloc()", lerpbuffnum * sizeof(vec4_t))
	if (!ptr)
	{
		return NULL;
	}

	lerpbuff = ptr;

	return lerpbuff;
}

static skeletal_bone_t *
R_BonesBufferRealloc(size_t num)
{
	void *ptr;

	if (num < bonesbuffnum)
	{
		return bonesbuff;
	}

	bonesbuffnum = ROUNDUP(num * 2, 32);
	ptr = realloc(bonesbuff, bonesbuffnum * sizeof(skeletal_bone_t));
	YQ2_COM_CHECK_OOM(ptr, "realloc()", bonesbuffnum * sizeof(skeletal_bone_t))
	if (!ptr)
	{
		return NULL;
	}

	bonesbuff = ptr;

	return bonesbuff;
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

	bonesbuff = NULL;
	bonesbuffnum = 0;
	R_BonesBufferRealloc(MAX_BONES);
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

	if (bonesbuff)
	{
		free(bonesbuff);
		bonesbuff = NULL;
	}
	bonesbuffnum = 0;
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

static void
R_SkeletalVerts(const dmdx_t *pheader, int frame, int oldframe, float frontlerp,
	float backlerp, float *lerp, const float move[3], const float *scale)
{
	const dmdx_baseframe_joint_t *poses, *old_poses;
	int num_joints, num_verts, num_weights, i;
	const dmdx_vertex_t *mesh_verteces;
	const dmdx_weight_t *weights;
	skeletal_bone_t *bonematrix;

	bonematrix = R_BonesBufferRealloc(pheader->num_joints);

	poses = (const dmdx_baseframe_joint_t *)((const byte *)pheader + pheader->ofs_baseframe_joints)
	        + frame * pheader->num_joints;
	old_poses = (const dmdx_baseframe_joint_t *)((const byte *)pheader + pheader->ofs_baseframe_joints)
	           + oldframe * pheader->num_joints;
	weights = (const dmdx_weight_t *)((const byte *)pheader + pheader->ofs_weights);
	mesh_verteces = (const dmdx_vertex_t *)((const byte *)pheader + pheader->ofs_mesh_verteces);
	num_joints = pheader->num_joints;
	num_weights = pheader->num_weights;
	num_verts = pheader->num_xyz;

	/* lerp/slerp each bone and build its world-space matrix */
	for (i = 0; i < num_joints; i++)
	{
		vec4_t lorient;
		int n;

		for (n = 0; n < 3; n++)
		{
			bonematrix[i].pos[n] = old_poses[i].pos[n] * backlerp +
				poses[i].pos[n] * frontlerp;
		}

		BoneSlerp(old_poses[i].orient, poses[i].orient, frontlerp, lorient);
		Quat_normalize(lorient);
		Quat_toMat3(lorient, bonematrix[i].rot);
	}

	/* skin each vertex */
	for (i = 0; i < num_verts; i++, lerp += 4)
	{
		const dmdx_vertex_t *bind = &mesh_verteces[i];
		vec3_t result = { 0.0f, 0.0f, 0.0f };
		int k, count = bind->count;

		if (bind->start < 0 || count < 0 || bind->start > num_weights - count)
		{
			count = 0;
		}

		for (k = 0; k < count; k++)
		{
			const dmdx_weight_t *weight;
			const skeletal_bone_t *joint;
			const float *j_rot, *w_pos;

			weight = &weights[bind->start + k];

			if (weight->joint < 0 || weight->joint >= num_joints)
			{
				break;
			}

			joint = bonematrix + weight->joint;
			j_rot = joint->rot;
			w_pos = weight->pos;

			/* The sum of all weight->bias should be 1.0 */
			result[0] += (joint->pos[0] + j_rot[0] * w_pos[0] + j_rot[1] * w_pos[1] + j_rot[2] * w_pos[2]) * weight->bias;
			result[1] += (joint->pos[1] + j_rot[3] * w_pos[0] + j_rot[4] * w_pos[1] + j_rot[5] * w_pos[2]) * weight->bias;
			result[2] += (joint->pos[2] + j_rot[6] * w_pos[0] + j_rot[7] * w_pos[1] + j_rot[8] * w_pos[2]) * weight->bias;
		}

		lerp[0] = scale[0] * (result[0] + move[0]);
		lerp[1] = scale[1] * (result[1] + move[1]);
		lerp[2] = scale[2] * (result[2] + move[2]);
	}
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
