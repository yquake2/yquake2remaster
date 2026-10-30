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
 * Model viewer for md2/flex models
 *
 * =======================================================================
 */

#include "../common/header/common.h"
#include "../common/header/files.h"
#include "../common/models/mesh.h"

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAME_DURATION_MS 100
#define VERTEX_NO_JOINT 0xFF
#define FLEX_MAX_JOINTS 3
#define FLEX_MESH_NODE_SIZE 516
#define SKIN_MAX_BONES 64
#define SKIN_FRAME_STRIDE 4

typedef struct {
	byte *data;
	size_t data_size;
	dstvert_t *texcoords;
	dtriangle_t *triangles;
	vec3_t *vertices;
	vec3_t *display_vertices;
	vec3_t *frame_scales;
	vec3_t *frame_translates;
	/* skeleton stored the same way as dmdx_t keeps md5 joints */
	dmdx_joint_t *joints;
	dmdx_baseframe_joint_t *frame_joints;
	byte *vertex_joints;
	/* runtime state: joint local pitch/yaw/roll and the interpolated pose */
	vec3_t *joint_angles;
	vec3_t *joint_pivots;
	quat_t *joint_orients;
	quat_t *joint_rotations;
	/* vertex frames converted to md5 style skinning, same layout as dmdx_t */
	dmdx_joint_t *skin_joints;
	dmdx_baseframe_joint_t *skin_poses;
	dmdx_weight_t *skin_weights;
	dmdx_vertex_t *skin_vertices;
	int skin_joint_count;
	int skin_weight_count;
	int use_skin;
	vec3_t skin_bone_positions[SKIN_MAX_BONES];
	size_t mesh_nodes;
	int mesh_node_count;
	int texcoord_count;
	int triangle_count;
	int vertex_count;
	int frame_count;
	int joint_count;
	float radius;
} Model;

void
Com_Printf (const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

static void model_free(Model *model)
{
	free(model->data);
	free(model->texcoords);
	free(model->triangles);
	free(model->vertices);
	free(model->display_vertices);
	free(model->frame_scales);
	free(model->frame_translates);
	free(model->joints);
	free(model->frame_joints);
	free(model->vertex_joints);
	free(model->joint_angles);
	free(model->joint_pivots);
	free(model->joint_orients);
	free(model->joint_rotations);
	free(model->skin_joints);
	free(model->skin_poses);
	free(model->skin_weights);
	free(model->skin_vertices);
	memset(model, 0, sizeof(*model));
}

static int range_valid(const Model *model, size_t offset, size_t length)
{
	return offset <= model->data_size && length <= model->data_size - offset;
}

static int read_file(Model *model, const char *filename, char *error, size_t error_size)
{
	FILE *file = fopen(filename, "rb");
	long size;
	if (!file)
	{
		snprintf(error, error_size, "cannot open model: %s", filename);
		return 0;
	}
	if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
		fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		snprintf(error, error_size, "cannot determine model size");
		return 0;
	}
	model->data_size = (size_t)size;
	model->data = (byte *)malloc(model->data_size ? model->data_size : 1);
	if (!model->data || fread(model->data, 1, model->data_size, file) != model->data_size)
	{
		fclose(file);
		snprintf(error, error_size, "cannot read model: %s", filename);
		return 0;
	}
	fclose(file);
	return 1;
}

static int allocate_geometry(Model *model, int vertex_count, int texcoord_count,
	int triangle_count, int frame_count, char *error, size_t error_size)
{
	if (vertex_count <= 0 || texcoord_count <= 0 || triangle_count <= 0 || frame_count <= 0)
	{
		snprintf(error, error_size, "model contains invalid geometry counts");
		return 0;
	}
	model->vertex_count = vertex_count;
	model->texcoord_count = texcoord_count;
	model->triangle_count = triangle_count;
	model->frame_count = frame_count;
	model->texcoords = (dstvert_t *)calloc((size_t)texcoord_count, sizeof(*model->texcoords));
	model->triangles = (dtriangle_t *)calloc((size_t)triangle_count, sizeof(*model->triangles));
	model->vertices = (vec3_t *)calloc((size_t)vertex_count * (size_t)frame_count, sizeof(*model->vertices));
	model->display_vertices = (vec3_t *)calloc((size_t)vertex_count, sizeof(*model->display_vertices));
	model->frame_scales = (vec3_t *)calloc((size_t)frame_count, sizeof(*model->frame_scales));
	model->frame_translates = (vec3_t *)calloc((size_t)frame_count, sizeof(*model->frame_translates));
	if (!model->texcoords || !model->triangles || !model->vertices ||
		!model->display_vertices || !model->frame_scales || !model->frame_translates)
	{
		snprintf(error, error_size, "out of memory loading model");
		return 0;
	}
	return 1;
}

static int load_triangles(Model *model, size_t offset, int count, char *error, size_t error_size)
{
	int i, corner;
	for (i = 0; i < count; ++i)
	{
		size_t triangle_offset = offset + (size_t)i * 12;
		for (corner = 0; corner < 3; ++corner)
		{
			model->triangles[i].index_xyz[corner] = LittleShort(*((short *)(model->data + triangle_offset + corner * 2)));
			model->triangles[i].index_st[corner] = LittleShort(*((short *)(model->data + triangle_offset + 6 + corner * 2)));
			if (model->triangles[i].index_xyz[corner] < 0 ||
				model->triangles[i].index_xyz[corner] >= model->vertex_count ||
				model->triangles[i].index_st[corner] < 0 ||
				model->triangles[i].index_st[corner] >= model->texcoord_count)
			{
				snprintf(error, error_size, "triangle references an invalid vertex");
				return 0;
			}
		}
	}
	return 1;
}

static void calculate_bounds(Model *model)
{
	size_t count = (size_t)model->vertex_count * (size_t)model->frame_count;
	size_t i;
	vec3_t min, max;
	VectorCopy(model->vertices[0], min);
	VectorCopy(model->vertices[0], max);
	for (i = 1; i < count; ++i)
	{
		const vec3_t *vertex = &model->vertices[i];
		if ((*vertex)[0] < min[0])
			min[0] = (*vertex)[0];
		if ((*vertex)[1] < min[1])
			min[1] = (*vertex)[1];
		if ((*vertex)[2] < min[2])
			min[2] = (*vertex)[2];
		if ((*vertex)[0] > max[0])
			max[0] = (*vertex)[0];
		if ((*vertex)[1] > max[1])
			max[1] = (*vertex)[1];
		if ((*vertex)[2] > max[2])
			max[2] = (*vertex)[2];
	}
	model->radius = max[0] - min[0];
	if (max[1] - min[1] > model->radius)
		model->radius = max[1] - min[1];
	if (max[2] - min[2] > model->radius)
		model->radius = max[2] - min[2];
	model->radius *= 0.5f;
}

static void decode_frame(Model *model, size_t offset, int frame, int vertex_count)
{
	int vertex;
	float scale[3] = {
		LittleFloat(*((float *)(model->data + offset))),
		LittleFloat(*((float *)(model->data + offset + 4))),
		LittleFloat(*((float *)(model->data + offset + 8)))
	};
	float translate[3] = {
		LittleFloat(*((float *)(model->data + offset + 12))),
		LittleFloat(*((float *)(model->data + offset + 16))),
		LittleFloat(*((float *)(model->data + offset + 20)))
	};
	VectorCopy(scale, model->frame_scales[frame]);
	VectorCopy(translate, model->frame_translates[frame]);
	for (vertex = 0; vertex < vertex_count; ++vertex)
	{
		size_t compressed = offset + 40 + (size_t)vertex * 4;
		float x = model->data[compressed] * scale[0] + translate[0];
		float y = model->data[compressed + 1] * scale[1] + translate[1];
		float z = model->data[compressed + 2] * scale[2] + translate[2];
		VectorSet(model->vertices[(size_t)frame * (size_t)vertex_count + (size_t)vertex], x, y, z);
	}
}

typedef struct
{
	int joint_count;
	const char *names[FLEX_MAX_JOINTS];
	int parents[FLEX_MAX_JOINTS];
} FlexSkeletonType;

/* indexed by the type stored in the flex "skeleton" block; every type is a chain */
static const FlexSkeletonType flex_skeleton_types[] = {
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
	{1, {"center"}, {-1}},
	{2, {"neck", "head"}, {-1, 0}},
	{2, {"neck", "head"}, {-1, 0}},
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
};

static int read_long(const Model *model, size_t offset)
{
	int value;
	memcpy(&value, model->data + offset, sizeof(value));
	return LittleLong(value);
}

static float read_float(const Model *model, size_t offset)
{
	float value;
	memcpy(&value, model->data + offset, sizeof(value));
	return LittleFloat(value);
}

/* same convention as Mod_Mat3x4ToQuat: matrix columns are the basis axes, xyzw */
static void basis_to_quat(const vec3_t forward, const vec3_t left, const vec3_t up, quat_t q)
{
	const float m[3][3] = {
		{forward[0], left[0], up[0]},
		{forward[1], left[1], up[1]},
		{forward[2], left[2], up[2]}
	};
	const float trace = m[0][0] + m[1][1] + m[2][2];
	float s;

	if (trace > 0.0f)
	{
		s = 0.5f / sqrtf(trace + 1.0f);
		q[3] = 0.25f / s;
		q[0] = (m[2][1] - m[1][2]) * s;
		q[1] = (m[0][2] - m[2][0]) * s;
		q[2] = (m[1][0] - m[0][1]) * s;
	}
	else if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
	{
		s = 2.0f * sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]);
		q[3] = (m[2][1] - m[1][2]) / s;
		q[0] = 0.25f * s;
		q[1] = (m[0][1] + m[1][0]) / s;
		q[2] = (m[0][2] + m[2][0]) / s;
	}
	else if (m[1][1] > m[2][2])
	{
		s = 2.0f * sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]);
		q[3] = (m[0][2] - m[2][0]) / s;
		q[0] = (m[0][1] + m[1][0]) / s;
		q[1] = 0.25f * s;
		q[2] = (m[1][2] + m[2][1]) / s;
	}
	else
	{
		s = 2.0f * sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]);
		q[3] = (m[1][0] - m[0][1]) / s;
		q[0] = (m[0][2] + m[2][0]) / s;
		q[1] = (m[1][2] + m[2][1]) / s;
		q[2] = 0.25f * s;
	}
	Quat_normalize(q);
}

static void quat_identity(quat_t q)
{
	q[0] = q[1] = q[2] = 0.0f;
	q[3] = 1.0f;
}

/* flex poses are an origin plus two far points marking the bone direction and up */
static void flex_pose_to_joint(const vec3_t origin, const vec3_t direction_point,
	const vec3_t up_point, dmdx_baseframe_joint_t *joint)
{
	vec3_t forward, up, left;

	VectorCopy(origin, joint->pos);
	VectorSubtract(direction_point, origin, forward);
	VectorSubtract(up_point, origin, up);
	if (VectorNormalize(forward) == 0.0f)
	{
		quat_identity(joint->orient);
		return;
	}
	VectorMA(up, -DotProduct(up, forward), forward, up);
	if (VectorNormalize(up) == 0.0f)
	{
		quat_identity(joint->orient);
		return;
	}
	CrossProduct(up, forward, left);
	basis_to_quat(forward, left, up, joint->orient);
}

static void angles_to_quat(const vec3_t angles, quat_t out)
{
	static const vec3_t axis_forward = {1.0f, 0.0f, 0.0f};
	static const vec3_t axis_left = {0.0f, 1.0f, 0.0f};
	static const vec3_t axis_up = {0.0f, 0.0f, 1.0f};
	quat_t pitch, yaw, roll, tmp;

	QuatAngleAxis(axis_left, angles[PITCH], pitch);
	QuatAngleAxis(axis_up, angles[YAW], yaw);
	QuatAngleAxis(axis_forward, angles[ROLL], roll);
	QuatMultiply(yaw, pitch, tmp);
	QuatMultiply(tmp, roll, out);
}

static void update_joints(Model *model, int frame, int previous_frame, float previous_weight)
{
	const float current_weight = 1.0f - previous_weight;

	for (int joint = 0; joint < model->joint_count; ++joint)
	{
		const dmdx_baseframe_joint_t *current =
			&model->frame_joints[(size_t)frame * (size_t)model->joint_count + (size_t)joint];
		const dmdx_baseframe_joint_t *previous =
			&model->frame_joints[(size_t)previous_frame * (size_t)model->joint_count + (size_t)joint];
		float *orient = model->joint_orients[joint];
		quat_t local, conjugate, tmp;
		float sign;
		int i;

		sign = (current->orient[0] * previous->orient[0] + current->orient[1] * previous->orient[1] +
			current->orient[2] * previous->orient[2] + current->orient[3] * previous->orient[3]) < 0.0f ?
			-1.0f : 1.0f;
		for (i = 0; i < 3; i++)
		{
			model->joint_pivots[joint][i] = current->pos[i] * current_weight +
				previous->pos[i] * previous_weight;
		}
		for (i = 0; i < 4; i++)
		{
			orient[i] = current->orient[i] * current_weight +
				sign * previous->orient[i] * previous_weight;
		}
		Quat_normalize(orient);

		/* joint angles are relative to the bone axes, so conjugate by the bone orientation */
		angles_to_quat(model->joint_angles[joint], local);
		QuatMultiply(orient, local, tmp);
		conjugate[0] = -orient[0];
		conjugate[1] = -orient[1];
		conjugate[2] = -orient[2];
		conjugate[3] = orient[3];
		QuatMultiply(tmp, conjugate, model->joint_rotations[joint]);
	}
}

/* deepest joint first, each around its unrotated pivot, then up to the root */
static void apply_joint_chain(const Model *model, int joint, vec3_t point)
{
	for (; joint >= 0; joint = model->joints[joint].parent)
	{
		vec3_t local, rotated;

		VectorSubtract(point, model->joint_pivots[joint], local);
		Quat_rotatePoint(model->joint_rotations[joint], local, rotated);
		VectorAdd(model->joint_pivots[joint], rotated, point);
	}
}

static int load_flex_skeleton(Model *model, size_t offset, size_t block_size, char *error, size_t error_size)
{
	const size_t end = offset + block_size;
	const FlexSkeletonType *skeleton;
	size_t cursor = offset;
	int counts[FLEX_MAX_JOINTS];
	int type, cluster_count, cluster, total = 0, attached = 0;

	if (block_size < 8 || !range_valid(model, offset, block_size))
	{
		snprintf(error, error_size, "flex skeleton block is truncated");
		return 0;
	}
	type = read_long(model, cursor);
	cluster_count = read_long(model, cursor + 4);
	cursor += 8;
	if (type < 0 || type >= (int)ARRLEN(flex_skeleton_types) ||
		cluster_count != flex_skeleton_types[type].joint_count)
	{
		snprintf(error, error_size, "flex skeleton has unknown type %d or invalid cluster count %d",
			type, cluster_count);
		return 0;
	}
	skeleton = &flex_skeleton_types[type];

	if ((end - cursor) / 4 < (size_t)cluster_count)
	{
		snprintf(error, error_size, "flex skeleton counts are truncated");
		return 0;
	}
	/* counts and vertex lists are stored from the deepest joint to the root */
	for (cluster = cluster_count - 1; cluster >= 0; --cluster)
	{
		counts[cluster] = read_long(model, cursor);
		cursor += 4;
		if (counts[cluster] < 0 || counts[cluster] > model->vertex_count - total)
		{
			snprintf(error, error_size, "flex skeleton has invalid cluster vertices");
			return 0;
		}
		total += counts[cluster];
	}
	if ((end - cursor) / 4 < (size_t)total + 1)
	{
		snprintf(error, error_size, "flex skeleton vertex lists are truncated");
		return 0;
	}

	model->vertex_joints = (byte *)malloc((size_t)model->vertex_count);
	if (!model->vertex_joints)
	{
		snprintf(error, error_size, "out of memory loading flex skeleton");
		return 0;
	}
	memset(model->vertex_joints, VERTEX_NO_JOINT, (size_t)model->vertex_count);
	for (cluster = cluster_count - 1; cluster >= 0; --cluster)
	{
		for (int i = 0; i < counts[cluster]; ++i)
		{
			const int index = read_long(model, cursor);
			cursor += 4;
			if (index < 0 || index >= model->vertex_count)
			{
				snprintf(error, error_size, "flex skeleton references an invalid vertex");
				return 0;
			}
			/* a vertex already listed for a deeper joint keeps it */
			if (model->vertex_joints[index] == VERTEX_NO_JOINT)
			{
				model->vertex_joints[index] = (byte)cluster;
				++attached;
			}
		}
	}

	if (!read_long(model, cursor))
	{
		printf("flex skeleton type %d has no joint poses, skeleton disabled\n", type);
		free(model->vertex_joints);
		model->vertex_joints = NULL;
		return 1;
	}
	cursor += 4;
	if ((end - cursor) / 36 < (size_t)model->frame_count * (size_t)cluster_count)
	{
		snprintf(error, error_size, "flex skeleton poses are truncated");
		return 0;
	}

	model->joints = (dmdx_joint_t *)calloc((size_t)cluster_count, sizeof(*model->joints));
	model->frame_joints = (dmdx_baseframe_joint_t *)calloc(
		(size_t)model->frame_count * (size_t)cluster_count, sizeof(*model->frame_joints));
	model->joint_angles = (vec3_t *)calloc((size_t)cluster_count, sizeof(*model->joint_angles));
	model->joint_pivots = (vec3_t *)calloc((size_t)cluster_count, sizeof(*model->joint_pivots));
	model->joint_orients = (quat_t *)calloc((size_t)cluster_count, sizeof(*model->joint_orients));
	model->joint_rotations = (quat_t *)calloc((size_t)cluster_count, sizeof(*model->joint_rotations));
	if (!model->joints || !model->frame_joints || !model->joint_angles ||
		!model->joint_pivots || !model->joint_orients || !model->joint_rotations)
	{
		snprintf(error, error_size, "out of memory loading flex poses");
		return 0;
	}

	for (int frame = 0; frame < model->frame_count; ++frame)
	{
		for (cluster = 0; cluster < cluster_count; ++cluster)
		{
			vec3_t points[3];

			/* poses are stored in compressed frame space */
			for (int point = 0; point < 3; ++point)
			{
				for (int axis = 0; axis < 3; ++axis)
				{
					points[point][axis] = read_float(model, cursor) * model->frame_scales[frame][axis] +
						model->frame_translates[frame][axis];
					cursor += 4;
				}
			}
			flex_pose_to_joint(points[0], points[1], points[2],
				&model->frame_joints[(size_t)frame * (size_t)cluster_count + (size_t)cluster]);
		}
	}

	for (cluster = 0; cluster < cluster_count; ++cluster)
	{
		dmdx_joint_t *joint = &model->joints[cluster];

		snprintf(joint->name, sizeof(joint->name), "%s", skeleton->names[cluster]);
		joint->parent = skeleton->parents[cluster];
		VectorCopy(model->frame_joints[cluster].pos, joint->pos);
		memcpy(joint->orient, model->frame_joints[cluster].orient, sizeof(joint->orient));
	}
	model->joint_count = cluster_count;

	printf("flex skeleton type %d: %d joints, %d of %d vertices attached\n",
		type, cluster_count, attached, model->vertex_count);
	return 1;
}
static void lerp_vec3(vec3_t out, const vec3_t current, const vec3_t previous, float previous_weight)
{
	const float current_weight = 1.0f - previous_weight;
	out[0] = current[0] * current_weight + previous[0] * previous_weight;
	out[1] = current[1] * current_weight + previous[1] * previous_weight;
	out[2] = current[2] * current_weight + previous[2] * previous_weight;
}

/* same math as R_SkeletalVerts, with nlerp in place of slerp; fm joint angles propagate to child bones */
static void skin_frame(Model *model, int frame, int previous_frame, float previous_weight,
	int apply_joints, vec3_t *out)
{
	const float frontlerp = 1.0f - previous_weight;
	float rotations[SKIN_MAX_BONES][9];
	vec3_t *positions = model->skin_bone_positions;
	quat_t deltas[SKIN_MAX_BONES];
	vec3_t offsets[SKIN_MAX_BONES];

	for (int bone = 0; bone < model->skin_joint_count; ++bone)
	{
		const dmdx_baseframe_joint_t *pose = &model->skin_poses[(size_t)frame * model->skin_joint_count + bone];
		const dmdx_baseframe_joint_t *old = &model->skin_poses[(size_t)previous_frame * model->skin_joint_count + bone];
		const int parent = model->skin_joints[bone].parent;
		const float sign = (pose->orient[0] * old->orient[0] + pose->orient[1] * old->orient[1] +
			pose->orient[2] * old->orient[2] + pose->orient[3] * old->orient[3]) < 0.0f ? -1.0f : 1.0f;
		quat_t orient, posed;
		vec3_t moved;
		int i;

		for (i = 0; i < 3; i++)
		{
			positions[bone][i] = old->pos[i] * previous_weight + pose->pos[i] * frontlerp;
		}
		for (i = 0; i < 4; i++)
		{
			orient[i] = sign * old->orient[i] * previous_weight + pose->orient[i] * frontlerp;
		}
		Quat_normalize(orient);

		/* runtime rigid offset: x -> deltas * x + offsets, inherited from the parent bone */
		if (parent >= 0)
		{
			memcpy(deltas[bone], deltas[parent], sizeof(quat_t));
			VectorCopy(offsets[parent], offsets[bone]);
		}
		else
		{
			quat_identity(deltas[bone]);
			VectorClear(offsets[bone]);
		}
		if (apply_joints && bone < model->joint_count)
		{
			const float *rotation = model->joint_rotations[bone];
			const float *pivot = model->joint_pivots[bone];
			vec3_t rotated, own;
			quat_t combined;

			Quat_rotatePoint(rotation, pivot, rotated);
			VectorSubtract(pivot, rotated, own);
			Quat_rotatePoint(deltas[bone], own, rotated);
			VectorAdd(offsets[bone], rotated, offsets[bone]);
			QuatMultiply(deltas[bone], rotation, combined);
			memcpy(deltas[bone], combined, sizeof(quat_t));
		}

		Quat_rotatePoint(deltas[bone], positions[bone], moved);
		VectorAdd(moved, offsets[bone], positions[bone]);
		QuatMultiply(deltas[bone], orient, posed);
		Quat_normalize(posed);
		Quat_toMat3(posed, rotations[bone]);
	}

	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		const dmdx_vertex_t *bind = &model->skin_vertices[vertex];
		vec3_t result = {0.0f, 0.0f, 0.0f};

		for (int k = 0; k < bind->count; ++k)
		{
			const dmdx_weight_t *weight = &model->skin_weights[bind->start + k];
			const float *j_rot = rotations[weight->joint];
			const float *j_pos = positions[weight->joint];
			const float *w_pos = weight->pos;

			result[0] += (j_pos[0] + j_rot[0] * w_pos[0] + j_rot[1] * w_pos[1] + j_rot[2] * w_pos[2]) * weight->bias;
			result[1] += (j_pos[1] + j_rot[3] * w_pos[0] + j_rot[4] * w_pos[1] + j_rot[5] * w_pos[2]) * weight->bias;
			result[2] += (j_pos[2] + j_rot[6] * w_pos[0] + j_rot[7] * w_pos[1] + j_rot[8] * w_pos[2]) * weight->bias;
		}
		VectorCopy(result, out[vertex]);
	}
}

typedef struct
{
	int bone_count;
	/* fm skeleton bones come first and keep their stored poses */
	int fixed_count;
	/* then one bone per mesh node; generated bones follow */
	int base_count;
	int *vertex_bone;
	unsigned *vertex_nodes;
	int parents[SKIN_MAX_BONES];
	int nodes[SKIN_MAX_BONES];
	/* fm joint whose runtime rotation the bone follows, -1 for none */
	int categories[SKIN_MAX_BONES];
	/* mesh section bone that generated bones hang off */
	int anchors[SKIN_MAX_BONES];
	vec3_t centers[SKIN_MAX_BONES];
	quat_t bind_orients[SKIN_MAX_BONES];
	/* frame_count * SKIN_MAX_BONES, rotation relative to the bind pose */
	vec3_t *positions;
	quat_t *orients;
	float (*matrices)[9];
} SkinFit;

/* cyclic Jacobi on a symmetric 4x4 matrix, returns the eigenvector of the largest eigenvalue */
static void max_eigenvector4(double a[4][4], double out[4])
{
	double e[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	int best = 0;

	for (int sweep = 0; sweep < 50; ++sweep)
	{
		double off = 0.0;

		for (int p = 0; p < 3; ++p)
			for (int q = p + 1; q < 4; ++q)
				off += a[p][q] * a[p][q];
		if (off < 1e-20)
			break;

		for (int p = 0; p < 3; ++p)
		{
			for (int q = p + 1; q < 4; ++q)
			{
				double theta, t, c, s;

				if (fabs(a[p][q]) < 1e-30)
					continue;
				theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
				t = (theta >= 0.0 ? 1.0 : -1.0) / (fabs(theta) + sqrt(theta * theta + 1.0));
				c = 1.0 / sqrt(t * t + 1.0);
				s = t * c;
				for (int k = 0; k < 4; ++k)
				{
					const double akp = a[k][p], akq = a[k][q];
					a[k][p] = c * akp - s * akq;
					a[k][q] = s * akp + c * akq;
				}
				for (int k = 0; k < 4; ++k)
				{
					const double apk = a[p][k], aqk = a[q][k];
					a[p][k] = c * apk - s * aqk;
					a[q][k] = s * apk + c * aqk;
				}
				for (int k = 0; k < 4; ++k)
				{
					const double ekp = e[k][p], ekq = e[k][q];
					e[k][p] = c * ekp - s * ekq;
					e[k][q] = s * ekp + c * ekq;
				}
			}
		}
	}
	for (int i = 1; i < 4; ++i)
	{
		if (a[i][i] > a[best][best])
			best = i;
	}
	for (int k = 0; k < 4; ++k)
		out[k] = e[k][best];
}

/* Horn's closed form: rotation that best maps rest offsets onto frame offsets */
static void skin_rotation_from_covariance(const double s[9], quat_t q)
{
	const double sxx = s[0], sxy = s[1], sxz = s[2];
	const double syx = s[3], syy = s[4], syz = s[5];
	const double szx = s[6], szy = s[7], szz = s[8];
	double n[4][4] = {
		{sxx + syy + szz, syz - szy, szx - sxz, sxy - syx},
		{syz - szy, sxx - syy - szz, sxy + syx, szx + sxz},
		{szx - sxz, sxy + syx, -sxx + syy - szz, syz + szy},
		{sxy - syx, szx + sxz, syz + szy, -sxx - syy + szz}
	};
	double v[4];

	max_eigenvector4(n, v);
	q[0] = (float)v[1];
	q[1] = (float)v[2];
	q[2] = (float)v[3];
	q[3] = (float)v[0];
	Quat_normalize(q);
}

static void skin_init_fixed(const Model *model, SkinFit *fit)
{
	fit->fixed_count = model->joint_count;
	for (int bone = 0; bone < fit->fixed_count; ++bone)
	{
		const dmdx_baseframe_joint_t *bind = &model->frame_joints[bone];
		quat_t inverse;

		fit->parents[bone] = model->joints[bone].parent;
		fit->nodes[bone] = -1;
		fit->categories[bone] = bone;
		fit->anchors[bone] = bone;
		VectorCopy(bind->pos, fit->centers[bone]);
		memcpy(fit->bind_orients[bone], bind->orient, sizeof(quat_t));
		inverse[0] = -bind->orient[0];
		inverse[1] = -bind->orient[1];
		inverse[2] = -bind->orient[2];
		inverse[3] = bind->orient[3];
		for (int frame = 0; frame < model->frame_count; ++frame)
		{
			const dmdx_baseframe_joint_t *pose = &model->frame_joints[(size_t)frame * model->joint_count + bone];
			const size_t index = (size_t)frame * SKIN_MAX_BONES + bone;

			VectorCopy(pose->pos, fit->positions[index]);
			QuatMultiply(pose->orient, inverse, fit->orients[index]);
			Quat_normalize(fit->orients[index]);
			Quat_toMat3(fit->orients[index], fit->matrices[index]);
		}
	}
}

static void skin_fit_bones(const Model *model, SkinFit *fit)
{
	const vec3_t *rest = model->vertices;
	int counts[SKIN_MAX_BONES] = {0};
	vec3_t sums[SKIN_MAX_BONES];
	int bone;

	memset(sums, 0, sizeof(sums));
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		bone = fit->vertex_bone[vertex];
		counts[bone]++;
		VectorAdd(sums[bone], rest[vertex], sums[bone]);
	}
	/* an empty generated bone keeps its last center */
	for (bone = fit->fixed_count; bone < fit->bone_count; ++bone)
	{
		if (counts[bone])
			VectorScale(sums[bone], 1.0f / counts[bone], fit->centers[bone]);
	}

	for (int frame = 0; frame < model->frame_count; ++frame)
	{
		const vec3_t *current = model->vertices + (size_t)frame * model->vertex_count;
		double covariance[SKIN_MAX_BONES][9];
		vec3_t targets[SKIN_MAX_BONES];

		memset(covariance, 0, sizeof(covariance));
		memset(targets, 0, sizeof(targets));
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			bone = fit->vertex_bone[vertex];
			VectorAdd(targets[bone], current[vertex], targets[bone]);
		}
		for (bone = fit->fixed_count; bone < fit->bone_count; ++bone)
		{
			if (counts[bone])
				VectorScale(targets[bone], 1.0f / counts[bone], targets[bone]);
			else
				VectorCopy(fit->centers[bone], targets[bone]);
		}
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			vec3_t a, b;

			bone = fit->vertex_bone[vertex];
			if (bone < fit->fixed_count)
				continue;
			VectorSubtract(rest[vertex], fit->centers[bone], a);
			VectorSubtract(current[vertex], targets[bone], b);
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					covariance[bone][i * 3 + j] += (double)a[i] * b[j];
		}
		for (bone = fit->fixed_count; bone < fit->bone_count; ++bone)
		{
			const size_t index = (size_t)frame * SKIN_MAX_BONES + bone;

			VectorCopy(targets[bone], fit->positions[index]);
			skin_rotation_from_covariance(covariance[bone], fit->orients[index]);
			Quat_toMat3(fit->orients[index], fit->matrices[index]);
		}
	}
}

static void skin_predict(const Model *model, const SkinFit *fit, int vertex, int bone, int frame, vec3_t out)
{
	const size_t index = (size_t)frame * SKIN_MAX_BONES + bone;
	const float *m = fit->matrices[index];
	vec3_t local;

	VectorSubtract(model->vertices[vertex], fit->centers[bone], local);
	out[0] = fit->positions[index][0] + m[0] * local[0] + m[1] * local[1] + m[2] * local[2];
	out[1] = fit->positions[index][1] + m[3] * local[0] + m[4] * local[1] + m[5] * local[2];
	out[2] = fit->positions[index][2] + m[6] * local[0] + m[7] * local[1] + m[8] * local[2];
}

static double skin_vertex_error(const Model *model, const SkinFit *fit, int vertex, int bone)
{
	double error = 0.0;

	for (int frame = 0; frame < model->frame_count; frame += SKIN_FRAME_STRIDE)
	{
		vec3_t predicted, delta;

		skin_predict(model, fit, vertex, bone, frame, predicted);
		VectorSubtract(predicted, model->vertices[(size_t)frame * model->vertex_count + vertex], delta);
		error += DotProduct(delta, delta);
	}
	return error;
}

static int skin_vertex_category(const Model *model, int vertex)
{
	return model->vertex_joints && model->vertex_joints[vertex] != VERTEX_NO_JOINT ?
		model->vertex_joints[vertex] : -1;
}

/* keep the fm rotation chain of every vertex and stay inside its mesh sections */
static int skin_bone_allowed(const Model *model, const SkinFit *fit, int vertex, int bone)
{
	if (fit->categories[bone] != skin_vertex_category(model, vertex))
		return 0;
	return bone < fit->fixed_count || !fit->vertex_nodes[vertex] ||
		((fit->vertex_nodes[vertex] >> fit->nodes[bone]) & 1);
}

/* drop empty generated bones; fm and mesh node bones stay as hierarchy anchors */
static void skin_compact(const Model *model, SkinFit *fit)
{
	int remap[SKIN_MAX_BONES], used[SKIN_MAX_BONES] = {0}, count = fit->base_count;

	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		used[fit->vertex_bone[vertex]] = 1;
	for (int bone = 0; bone < fit->base_count; ++bone)
		remap[bone] = bone;
	for (int bone = fit->base_count; bone < fit->bone_count; ++bone)
	{
		if (!used[bone])
		{
			remap[bone] = -1;
			continue;
		}
		remap[bone] = count;
		fit->parents[count] = fit->parents[bone];
		fit->nodes[count] = fit->nodes[bone];
		fit->categories[count] = fit->categories[bone];
		fit->anchors[count] = fit->anchors[bone];
		VectorCopy(fit->centers[bone], fit->centers[count]);
		memcpy(fit->bind_orients[count], fit->bind_orients[bone], sizeof(quat_t));
		count++;
	}
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		fit->vertex_bone[vertex] = remap[fit->vertex_bone[vertex]];
	fit->bone_count = count;
}

static void skin_assign(const Model *model, SkinFit *fit)
{
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		int best = fit->vertex_bone[vertex];
		double best_error = skin_vertex_error(model, fit, vertex, best);

		for (int bone = 0; bone < fit->bone_count; ++bone)
		{
			double error;

			if (bone == best || !skin_bone_allowed(model, fit, vertex, bone))
				continue;
			error = skin_vertex_error(model, fit, vertex, bone);
			if (error < best_error)
			{
				best_error = error;
				best = bone;
			}
		}
		fit->vertex_bone[vertex] = best;
	}
	skin_compact(model, fit);
}

typedef struct
{
	float distance;
	int vertex;
} SkinSortItem;

static int skin_sort_compare(const void *a, const void *b)
{
	const float da = ((const SkinSortItem *)a)->distance;
	const float db = ((const SkinSortItem *)b)->distance;
	return (da > db) - (da < db);
}

/* split the bone with the largest error around its worst fitting vertex */
static int skin_split_worst(const Model *model, SkinFit *fit, SkinSortItem *items)
{
	double bone_error[SKIN_MAX_BONES] = {0}, worst_error[SKIN_MAX_BONES] = {0};
	int worst_vertex[SKIN_MAX_BONES], counts[SKIN_MAX_BONES] = {0};
	int bone, split = -1, count = 0;

	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		const double error = skin_vertex_error(model, fit, vertex, fit->vertex_bone[vertex]);

		bone = fit->vertex_bone[vertex];
		bone_error[bone] += error;
		if (!counts[bone]++ || error > worst_error[bone])
		{
			worst_error[bone] = error;
			worst_vertex[bone] = vertex;
		}
	}
	for (bone = fit->fixed_count; bone < fit->bone_count; ++bone)
	{
		if (counts[bone] >= 4 && (split < 0 || bone_error[bone] > bone_error[split]))
			split = bone;
	}
	if (split < 0)
		return 0;

	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		vec3_t delta;

		if (fit->vertex_bone[vertex] != split)
			continue;
		VectorSubtract(model->vertices[vertex], model->vertices[worst_vertex[split]], delta);
		items[count].distance = VectorLength(delta);
		items[count].vertex = vertex;
		count++;
	}
	qsort(items, (size_t)count, sizeof(*items), skin_sort_compare);
	for (int i = 0; i < count / 2; ++i)
		fit->vertex_bone[items[i].vertex] = fit->bone_count;
	/* generated bones hang off the bone of their mesh section */
	fit->nodes[fit->bone_count] = fit->nodes[split];
	fit->categories[fit->bone_count] = fit->categories[split];
	fit->anchors[fit->bone_count] = fit->anchors[split];
	fit->parents[fit->bone_count] = fit->anchors[split];
	VectorCopy(fit->centers[split], fit->centers[fit->bone_count]);
	quat_identity(fit->bind_orients[fit->bone_count]);
	fit->bone_count++;
	return 1;
}

/* weight offsets are in the bone's bind space, as md5 weights are */
static void skin_weight_position(const Model *model, const SkinFit *fit, int vertex, int bone, vec3_t out)
{
	const float *bind = fit->bind_orients[bone];
	quat_t inverse = {-bind[0], -bind[1], -bind[2], bind[3]};
	vec3_t local;

	VectorSubtract(model->vertices[vertex], fit->centers[bone], local);
	Quat_rotatePoint(inverse, local, out);
}

static void skin_store(Model *model, const SkinFit *fit)
{
	const int bones = fit->bone_count;
	int weight = 0, blended = 0;

	model->skin_joint_count = bones;
	for (int bone = 0; bone < bones; ++bone)
	{
		dmdx_joint_t *joint = &model->skin_joints[bone];

		if (bone < fit->fixed_count)
			snprintf(joint->name, sizeof(joint->name), "%s", model->joints[bone].name);
		else
			snprintf(joint->name, sizeof(joint->name), bone < fit->base_count ? "node%02d_%s" : "node%02d_%s_%02d",
				fit->nodes[bone], fit->categories[bone] >= 0 ? model->joints[fit->categories[bone]].name : "root",
				bone);
		joint->parent = fit->parents[bone];
		VectorCopy(fit->centers[bone], joint->pos);
		memcpy(joint->orient, fit->bind_orients[bone], sizeof(joint->orient));
		for (int frame = 0; frame < model->frame_count; ++frame)
		{
			const size_t index = (size_t)frame * SKIN_MAX_BONES + bone;
			dmdx_baseframe_joint_t *pose = &model->skin_poses[(size_t)frame * bones + bone];

			VectorCopy(fit->positions[index], pose->pos);
			QuatMultiply(fit->orients[index], fit->bind_orients[bone], pose->orient);
			Quat_normalize(pose->orient);
		}
	}

	/* each vertex keeps its rigid bone, optionally blended with a second one */
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		const int primary = fit->vertex_bone[vertex];
		const double primary_error = skin_vertex_error(model, fit, vertex, primary);
		double best_error = primary_error;
		float best_blend = 0.0f;
		int secondary = -1;
		dmdx_vertex_t *bind = &model->skin_vertices[vertex];
		dmdx_weight_t *out;

		for (int bone = 0; bone < bones; ++bone)
		{
			double numerator = 0.0, denominator = 0.0, error = 0.0;
			float blend;
			int frame;

			if (bone == primary || !skin_bone_allowed(model, fit, vertex, bone))
				continue;
			for (frame = 0; frame < model->frame_count; frame += SKIN_FRAME_STRIDE)
			{
				vec3_t a, b, d, r;

				skin_predict(model, fit, vertex, primary, frame, a);
				skin_predict(model, fit, vertex, bone, frame, b);
				VectorSubtract(b, a, d);
				VectorSubtract(model->vertices[(size_t)frame * model->vertex_count + vertex], a, r);
				numerator += DotProduct(r, d);
				denominator += DotProduct(d, d);
			}
			if (denominator <= 0.0)
				continue;
			blend = (float)(numerator / denominator);
			if (blend <= 0.01f)
				continue;
			if (blend > 1.0f)
				blend = 1.0f;
			for (frame = 0; frame < model->frame_count; frame += SKIN_FRAME_STRIDE)
			{
				vec3_t a, b, p, d;

				skin_predict(model, fit, vertex, primary, frame, a);
				skin_predict(model, fit, vertex, bone, frame, b);
				for (int i = 0; i < 3; ++i)
					p[i] = a[i] + blend * (b[i] - a[i]);
				VectorSubtract(p, model->vertices[(size_t)frame * model->vertex_count + vertex], d);
				error += DotProduct(d, d);
			}
			if (error < best_error)
			{
				best_error = error;
				best_blend = blend;
				secondary = bone;
			}
		}

		bind->st[0] = bind->st[1] = 0.0f;
		bind->start = weight;
		bind->count = 0;
		if (secondary >= 0 && best_error < primary_error * 0.9)
		{
			out = &model->skin_weights[weight++];
			out->joint = secondary;
			out->bias = best_blend;
			skin_weight_position(model, fit, vertex, secondary, out->pos);
			bind->count++;
			blended++;
		}
		else
		{
			best_blend = 0.0f;
		}
		out = &model->skin_weights[weight++];
		out->joint = primary;
		out->bias = 1.0f - best_blend;
		skin_weight_position(model, fit, vertex, primary, out->pos);
		bind->count++;
	}
	model->skin_weight_count = weight;
	printf("skin: %d bones (%d fm, %d mesh nodes, %d generated), %d weights, %d vertices blended\n",
		bones, fit->fixed_count, fit->base_count - fit->fixed_count, bones - fit->base_count,
		weight, blended);
	for (int bone = 0; bone < bones; ++bone)
	{
		const int parent = model->skin_joints[bone].parent;
		int count = 0;

		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
			count += fit->vertex_bone[vertex] == bone;
		printf("  %2d %-12s parent %-12s vertices %d\n", bone, model->skin_joints[bone].name,
			parent >= 0 ? model->skin_joints[parent].name : "-", count);
	}
}

static void skin_report(Model *model)
{
	vec3_t *skinned = (vec3_t *)malloc((size_t)model->vertex_count * sizeof(*skinned));
	double sum = 0.0;
	float max_error = 0.0f;
	int max_frame = 0, max_vertex = 0;

	if (!skinned)
		return;
	for (int frame = 0; frame < model->frame_count; ++frame)
	{
		skin_frame(model, frame, frame, 0.0f, 0, skinned);
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			vec3_t delta;
			float error;

			VectorSubtract(skinned[vertex], model->vertices[(size_t)frame * model->vertex_count + vertex], delta);
			error = VectorLength(delta);
			sum += (double)error * error;
			if (error > max_error)
			{
				max_error = error;
				max_frame = frame;
				max_vertex = vertex;
			}
		}
	}
	printf("skin error vs vertex frames: rms %.4f, max %.4f (frame %d vertex %d), model size %.2f\n",
		sqrt(sum / ((double)model->frame_count * model->vertex_count)), max_error,
		max_frame, max_vertex, model->radius * 2.0f);
	free(skinned);
}

/* fm frames carry no weights, so fit rigid bones to the baked vertex animation */
static int skin_convert(Model *model)
{
	const clock_t start = clock();
	const size_t mesh_nodes = model->mesh_nodes;
	const int mesh_node_count = model->mesh_node_count;
	const size_t slots = (size_t)model->frame_count * SKIN_MAX_BONES;
	SkinFit fit = {0};
	SkinSortItem *items;
	int ok = 0, node_count;

	fit.vertex_bone = (int *)malloc((size_t)model->vertex_count * sizeof(int));
	fit.vertex_nodes = (unsigned *)calloc((size_t)model->vertex_count, sizeof(unsigned));
	fit.positions = (vec3_t *)malloc(slots * sizeof(*fit.positions));
	fit.orients = (quat_t *)malloc(slots * sizeof(*fit.orients));
	fit.matrices = (float (*)[9])malloc(slots * sizeof(*fit.matrices));
	items = (SkinSortItem *)malloc((size_t)model->vertex_count * sizeof(*items));
	if (!fit.vertex_bone || !fit.vertex_nodes || !fit.positions || !fit.orients || !fit.matrices || !items)
		goto done;

	skin_init_fixed(model, &fit);
	node_count = mesh_node_count < 32 ? mesh_node_count : 32;
	if (node_count < 1)
		node_count = 1;
	fit.bone_count = fit.fixed_count;

	/* one bone per mesh section and fm joint, parented to that fm joint or the root */
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		const int category = skin_vertex_category(model, vertex);
		int first = 0, bone;

		for (int node = (mesh_node_count < node_count ? mesh_node_count : node_count) - 1; node >= 0; --node)
		{
			const byte *bits = model->data + mesh_nodes + (size_t)node * FLEX_MESH_NODE_SIZE + 256;

			if (vertex < 256 * 8 && (bits[vertex >> 3] >> (vertex & 7)) & 1)
			{
				fit.vertex_nodes[vertex] |= 1u << node;
				first = node;
			}
		}
		for (bone = fit.fixed_count; bone < fit.bone_count; ++bone)
		{
			if (fit.nodes[bone] == first && fit.categories[bone] == category)
				break;
		}
		if (bone == fit.bone_count)
		{
			if (fit.bone_count == SKIN_MAX_BONES)
				goto done;
			fit.nodes[bone] = first;
			fit.categories[bone] = category;
			fit.anchors[bone] = bone;
			fit.parents[bone] = category;
			quat_identity(fit.bind_orients[bone]);
			fit.bone_count++;
		}
		fit.vertex_bone[vertex] = bone;
	}
	fit.base_count = fit.bone_count;

	skin_fit_bones(model, &fit);
	skin_assign(model, &fit);
	skin_fit_bones(model, &fit);
	while (fit.bone_count < SKIN_MAX_BONES && skin_split_worst(model, &fit, items))
	{
		skin_fit_bones(model, &fit);
		skin_assign(model, &fit);
		skin_fit_bones(model, &fit);
	}

	model->skin_joints = (dmdx_joint_t *)calloc((size_t)fit.bone_count, sizeof(*model->skin_joints));
	model->skin_poses = (dmdx_baseframe_joint_t *)calloc((size_t)model->frame_count * fit.bone_count,
		sizeof(*model->skin_poses));
	model->skin_weights = (dmdx_weight_t *)calloc((size_t)model->vertex_count * 2, sizeof(*model->skin_weights));
	model->skin_vertices = (dmdx_vertex_t *)calloc((size_t)model->vertex_count, sizeof(*model->skin_vertices));
	if (!model->skin_joints || !model->skin_poses || !model->skin_weights || !model->skin_vertices)
		goto done;

	skin_store(model, &fit);
	skin_report(model);
	printf("skin conversion took %.2f s\n", (double)(clock() - start) / CLOCKS_PER_SEC);
	ok = 1;

done:
	if (!ok)
	{
		free(model->skin_joints);
		free(model->skin_poses);
		free(model->skin_weights);
		free(model->skin_vertices);
		model->skin_joints = NULL;
		model->skin_poses = NULL;
		model->skin_weights = NULL;
		model->skin_vertices = NULL;
	}
	free(fit.vertex_bone);
	free(fit.vertex_nodes);
	free(fit.positions);
	free(fit.orients);
	free(fit.matrices);
	free(items);
	return ok;
}

static void interpolate_frame(Model *model, int frame, float previous_weight)
{
	const int previous_frame = frame > 0 ? frame - 1 : model->frame_count - 1;
	const vec3_t *current = model->vertices + (size_t)frame * (size_t)model->vertex_count;
	const vec3_t *previous = model->vertices + (size_t)previous_frame * (size_t)model->vertex_count;
	if (model->use_skin)
	{
		if (model->joint_count)
			update_joints(model, frame, previous_frame, previous_weight);
		skin_frame(model, frame, previous_frame, previous_weight, model->joint_count > 0,
			model->display_vertices);
		return;
	}
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		lerp_vec3(model->display_vertices[vertex], current[vertex], previous[vertex], previous_weight);
	}
	if (!model->joint_count)
	{
		return;
	}
	/* rotate in world space: compressed frame space has a non-uniform scale */
	update_joints(model, frame, previous_frame, previous_weight);
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		if (model->vertex_joints[vertex] != VERTEX_NO_JOINT)
		{
			apply_joint_chain(model, model->vertex_joints[vertex], model->display_vertices[vertex]);
		}
	}
}

static int load_md2(Model *model, const char *filename, char *error, size_t error_size)
{
	int32_t frame_size, vertex_count, texcoord_count, triangle_count, frame_count;
	int i;
	if (!read_file(model, filename, error, error_size)) return 0;
	if (model->data_size < 68 ||
		LittleLong(*((int *)(model->data))) != IDALIASHEADER ||
		LittleLong(*((int *)(model->data + 4))) != ALIAS_VERSION)
	{
		snprintf(error, error_size, "not a valid MD2 model (expected IDP2 version 8)");
		return 0;
	}
	frame_size = LittleLong(*((int *)(model->data + 16)));
	vertex_count = LittleLong(*((int *)(model->data + 24)));
	texcoord_count = LittleLong(*((int *)(model->data + 28)));
	triangle_count = LittleLong(*((int *)(model->data + 32)));
	frame_count = LittleLong(*((int *)(model->data + 40)));
	if (vertex_count <= 0 || frame_size < 40 ||
		(size_t)frame_size < 40 + (size_t)vertex_count * 4 ||
		!range_valid(model, LittleLong(*((int *)(model->data + 48))), (size_t)texcoord_count * 4) ||
		!range_valid(model, LittleLong(*((int *)(model->data + 52))), (size_t)triangle_count * 12) ||
		!range_valid(model, LittleLong(*((int *)(model->data + 56))), (size_t)frame_size * (size_t)frame_count) ||
		!allocate_geometry(model, vertex_count, texcoord_count, triangle_count, frame_count, error, error_size))
	{
		if (error[0] == '\0')
			snprintf(error, error_size, "MD2 contains invalid dimensions or offsets");
		return 0;
	}

	for (i = 0; i < texcoord_count; ++i)
	{
		size_t offset = LittleLong(*((int *)(model->data + 48))) + i * 4;
		model->texcoords[i].s = LittleShort(*((short *)(model->data + offset)));
		model->texcoords[i].t = LittleShort(*((short *)(model->data + offset + 2)));
	}

	if (!load_triangles(model, (size_t)LittleLong(*((int *)(model->data + 52))), triangle_count, error, error_size))
	{
		return 0;
	}

	for (i = 0; i < frame_count; ++i)
	{
		decode_frame(model, LittleLong(*((int *)(model->data + 56))) + (size_t)i * frame_size, i, vertex_count);
	}

	calculate_bounds(model);
	return 1;
}

static int block_name_equals(const byte *name, const char *expected)
{
	return strncmp((const char *)name, expected, 32) == 0;
}

static int load_flex(Model *model, const char *filename, char *error, size_t error_size)
{
	size_t offset = 0, header = 0, st = 0, tris = 0, frames = 0, skeleton = 0, mesh_nodes = 0;
	int32_t header_size = 0, st_size = 0, tris_size = 0, frames_size = 0, skeleton_size = 0, mesh_nodes_size = 0;
	int skin_width, skin_height, frame_size, vertex_count, texcoord_count, triangle_count, frame_count, i;
	int mesh_node_count;
	if (!read_file(model, filename, error, error_size)) return 0;
	while (offset < model->data_size)
	{
		int32_t version, size;
		if (!range_valid(model, offset, 40))
		{
			snprintf(error, error_size, "flex block header is truncated");
			return 0;
		}
		version = LittleLong(*((int *)(model->data + offset + 32)));
		size = LittleLong(*((int *)(model->data + offset + 36)));
		if (version < 0 || size < 0 ||
			!range_valid(model, offset + 40, size))
		{
			snprintf(error, error_size, "flex model contains an invalid block");
			return 0;
		}
		if (block_name_equals(model->data + offset, "header"))
		{
			header = offset + 40;
			header_size = size;
		}
		else if (block_name_equals(model->data + offset, "st coord"))
		{
			st = offset + 40;
			st_size = size;
		}
		else if (block_name_equals(model->data + offset, "tris"))
		{
			tris = offset + 40;
			tris_size = size;
		}
		else if (block_name_equals(model->data + offset, "frames"))
		{
			frames = offset + 40;
			frames_size = size;
		}
		else if (block_name_equals(model->data + offset, "skeleton"))
		{
			skeleton = offset + 40;
			skeleton_size = size;
		}
		else if (block_name_equals(model->data + offset, "mesh nodes"))
		{
			mesh_nodes = offset + 40;
			mesh_nodes_size = size;
		}
		offset += 40 + (size_t)size;
	}
	if (!header || header_size != 40 || !st || !tris || !frames)
	{
		snprintf(error, error_size, "flex model is missing required blocks");
		return 0;
	}
	skin_width = LittleLong(*((int *)(model->data + header)));
	skin_height = LittleLong(*((int *)(model->data + header + 4)));
	frame_size = LittleLong(*((int *)(model->data + header + 8)));
	vertex_count = LittleLong(*((int *)(model->data + header + 16)));
	texcoord_count = LittleLong(*((int *)(model->data + header + 20)));
	triangle_count = LittleLong(*((int *)(model->data + header + 24)));
	frame_count = LittleLong(*((int *)(model->data + header + 32)));
	mesh_node_count = LittleLong(*((int *)(model->data + header + 36)));
	if (!mesh_nodes || mesh_node_count <= 0 ||
		(size_t)mesh_nodes_size != (size_t)mesh_node_count * FLEX_MESH_NODE_SIZE)
	{
		mesh_node_count = 0;
	}
	if (skin_width <= 0 ||
		skin_height <= 0 ||
		vertex_count <= 0 || texcoord_count <= 0 ||
		triangle_count <= 0 || frame_count <= 0 ||
		frame_size < 40 ||
		(size_t)frame_size < 40 + (size_t)vertex_count * 4 ||
		(size_t)st_size != (size_t)texcoord_count * 4 ||
		(size_t)tris_size != (size_t)triangle_count * 12 ||
		(size_t)frames_size != (size_t)frame_count * (size_t)frame_size ||
		!allocate_geometry(model, vertex_count, texcoord_count, triangle_count, frame_count, error, error_size))
	{
		if (error[0] == '\0')
		{
			snprintf(error, error_size, "flex header contains invalid dimensions");
		}
		return 0;
	}
	for (i = 0; i < texcoord_count; ++i)
	{
		model->texcoords[i].s = LittleShort(*((short *)(model->data + st + i * 4)));
		model->texcoords[i].t = LittleShort(*((short *)(model->data + st + i * 4 + 2)));
	}
	if (!load_triangles(model, tris, triangle_count, error, error_size))
	{
		return 0;
	}

	for (i = 0; i < frame_count; ++i)
	{
		decode_frame(model, frames + (size_t)i * frame_size, i, vertex_count);
	}

	if (skeleton && !load_flex_skeleton(model, skeleton, (size_t)skeleton_size, error, error_size))
	{
		return 0;
	}

	calculate_bounds(model);
	model->mesh_nodes = mesh_nodes;
	model->mesh_node_count = mesh_node_count;
	return 1;
}

static void draw_model(const Model *model)
{
	int i, corner;
	const vec3_t *vertices = model->display_vertices;
	glColor3f(0.58f, 0.70f, 0.86f);
	glBegin(GL_TRIANGLES);
	for (i = 0; i < model->triangle_count; ++i)
	{
		const dtriangle_t *triangle = &model->triangles[i];
		vec3_t edge_a, edge_b, normal;
		VectorSubtract(vertices[triangle->index_xyz[1]], vertices[triangle->index_xyz[0]], edge_a);
		VectorSubtract(vertices[triangle->index_xyz[2]], vertices[triangle->index_xyz[0]], edge_b);
		CrossProduct(edge_a, edge_b, normal);
		glNormal3f(normal[0], normal[1], normal[2]);
		for (corner = 0; corner < 3; ++corner)
		{
			const dstvert_t *texcoord = &model->texcoords[triangle->index_st[corner]];
			glTexCoord2f((float)texcoord->s, (float)texcoord->t);
			glVertex3f(
				vertices[triangle->index_xyz[corner]][0],
				vertices[triangle->index_xyz[corner]][1],
				vertices[triangle->index_xyz[corner]][2]
			);
		}
	}
	glEnd();
}

static void draw_skeleton(const Model *model)
{
	static const vec3_t forward_axis = {1.0f, 0.0f, 0.0f};
	vec3_t origins[FLEX_MAX_JOINTS], ends[FLEX_MAX_JOINTS];
	const float length = model->radius * 0.2f;

	if (!model->joint_count)
		return;
	for (int joint = 0; joint < model->joint_count; ++joint)
	{
		vec3_t forward;

		VectorCopy(model->joint_pivots[joint], origins[joint]);
		apply_joint_chain(model, model->joints[joint].parent, origins[joint]);
		Quat_rotatePoint(model->joint_orients[joint], forward_axis, forward);
		VectorMA(model->joint_pivots[joint], length, forward, ends[joint]);
		apply_joint_chain(model, joint, ends[joint]);
	}
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (int joint = 0; joint < model->joint_count; ++joint)
	{
		const int parent = model->joints[joint].parent;

		glColor3f(1.0f, 0.72f, 0.12f);
		glVertex3fv(origins[joint]);
		glVertex3fv(ends[joint]);
		if (parent >= 0)
		{
			glColor3f(0.3f, 0.9f, 0.4f);
			glVertex3fv(origins[parent]);
			glVertex3fv(origins[joint]);
		}
	}
	glEnd();
	glPointSize(5.0f);
	glColor3f(1.0f, 0.72f, 0.12f);
	glBegin(GL_POINTS);
	for (int joint = 0; joint < model->joint_count; ++joint)
	{
		glVertex3fv(origins[joint]);
	}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

int main(int argc, char **argv)
{
	Model model = {0};
	char error[256] = {0};
	int is_flex, current_frame = 0, running = 1;
	int selected_joint = 0;
	int paused = 0;
	int dragging = 0;
	SDL_Window *window;
	SDL_GLContext context;
	uint64_t last_frame_tick;
	float camera_distance;
	float model_pitch = 0.0f;
	float model_yaw = 0.0f;

	Swap_Init();

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s MODEL.md2|MODEL.fm\n", argv[0]);
		return 2;
	}

	is_flex = (strlen(argv[1]) >= 3 && argv[1][strlen(argv[1]) - 3] == '.' &&
		(argv[1][strlen(argv[1]) - 2] == 'f' || argv[1][strlen(argv[1]) - 2] == 'F') &&
		(argv[1][strlen(argv[1]) - 1] == 'm' || argv[1][strlen(argv[1]) - 1] == 'M'));
	if (!(is_flex ? load_flex(&model, argv[1], error, sizeof(error)) :
		load_md2(&model, argv[1], error, sizeof(error))))
	{
		fprintf(stderr, "%s\n", error);
		model_free(&model);
		return 1;
	}
#ifdef USE_SDL3
	if (!SDL_Init(SDL_INIT_VIDEO))
#else
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
#endif
	{
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		model_free(&model);
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#ifdef USE_SDL3
	window = SDL_CreateWindow("SDL model viewer", 960, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
#else
	window = SDL_CreateWindow("SDL model viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		960, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
#endif
	context = window ? SDL_GL_CreateContext(window) : NULL;
	if (!window || !context) {
		fprintf(stderr, "OpenGL window creation failed: %s\n", SDL_GetError());
		if (context) {
#ifdef USE_SDL3
			SDL_GL_DestroyContext(context);
#else
			SDL_GL_DeleteContext(context);
#endif
		}

		if (window)
		{
			SDL_DestroyWindow(window);
		}

		SDL_Quit();
		model_free(&model);
		return 1;
	}
	{
		const GLfloat light_ambient[] = {0.18f, 0.20f, 0.24f, 1.0f};
		const GLfloat light_diffuse[] = {0.95f, 0.88f, 0.72f, 1.0f};
		const GLfloat light_position[] = {-1.0f, -2.0f, 3.0f, 0.0f};
		const GLfloat material_color[] = {0.58f, 0.70f, 0.86f, 1.0f};
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);
		glEnable(GL_COLOR_MATERIAL);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor4fv(material_color);
	}
	camera_distance = (model.radius > 1.0f ? model.radius : 1.0f) * 3.0f;
	last_frame_tick = SDL_GetTicks();
	while (running)
	{
		SDL_Event event;
		int width, height;
		float radius = model.radius > 1.0f ? model.radius : 1.0f;
		float light_angle;
		GLfloat moving_light[4];
		float previous_weight;
		while (SDL_PollEvent(&event))
		{
#ifdef USE_SDL3
			SDL_Keycode key = event.key.key;
#else
			SDL_Keycode key = event.key.keysym.sym;
#endif

#ifdef USE_SDL3
			if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_ESCAPE))
#else
			if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && key == SDLK_ESCAPE))
#endif
			{
				running = 0;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
#else
			else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
#endif
			{
				dragging = 1;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
#else
			else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
#endif
			{
				dragging = 0;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_MOUSE_MOTION && dragging)
#else
			else if (event.type == SDL_MOUSEMOTION && dragging)
#endif
			{
				model_yaw += event.motion.xrel * 0.5f;
				model_pitch += event.motion.yrel * 0.5f;
				if (model_pitch > 89.0f)
					model_pitch = 89.0f;
				if (model_pitch < -89.0f)
					model_pitch = -89.0f;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_SPACE)
#else
			else if (event.type == SDL_KEYDOWN && key == SDLK_SPACE)
#endif
			{
				paused = !paused;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_S && is_flex)
#else
			else if (event.type == SDL_KEYDOWN && key == SDLK_s && is_flex)
#endif
			{
				if (!model.skin_vertices)
				{
					printf("converting vertex frames to skeletal animation...\n");
					if (!skin_convert(&model))
					{
						printf("skin conversion failed\n");
						continue;
					}
					last_frame_tick = SDL_GetTicks();
				}
				model.use_skin = !model.use_skin;
				printf("rendering %s\n", model.use_skin ? "converted skeletal animation" : "original vertex frames");
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && key >= SDLK_1 && key <= SDLK_8 && model.joint_count > 0)
#else
			else if (event.type == SDL_KEYDOWN && key >= SDLK_1 && key <= SDLK_8 && model.joint_count > 0)
#endif
			{
				selected_joint = (int)(key - SDLK_1);
				if (selected_joint >= model.joint_count)
					selected_joint = model.joint_count - 1;
				printf("selected joint %d: %s\n", selected_joint, model.joints[selected_joint].name);
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && model.joint_count > 0 &&
				(key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN ||
				 key == SDLK_Q || key == SDLK_E))
#else
			else if (event.type == SDL_KEYDOWN && model.joint_count > 0 &&
				(key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN ||
				 key == SDLK_q || key == SDLK_e))
#endif
			{
				const float angle_step = 5.0f * M_PI / 180.0f;
				if (key == SDLK_LEFT)
					model.joint_angles[selected_joint][YAW] -= angle_step;
				else if (key == SDLK_RIGHT)
					model.joint_angles[selected_joint][YAW] += angle_step;
				else if (key == SDLK_UP)
					model.joint_angles[selected_joint][PITCH] -= angle_step;
				else if (key == SDLK_DOWN)
					model.joint_angles[selected_joint][PITCH] += angle_step;
#ifdef USE_SDL3
				else if (key == SDLK_Q)
					model.joint_angles[selected_joint][ROLL] -= angle_step;
				else if (key == SDLK_E)
					model.joint_angles[selected_joint][ROLL] += angle_step;
#else
				else if (key == SDLK_q)
					model.joint_angles[selected_joint][ROLL] -= angle_step;
				else if (key == SDLK_e)
					model.joint_angles[selected_joint][ROLL] += angle_step;
#endif
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_PLUS)
#else
			else if (event.type == SDL_KEYDOWN && key == SDLK_PLUS)
#endif
			{
				camera_distance *= 0.85f;
				if (camera_distance < radius * 1.2f)
					camera_distance = radius * 1.2f;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && key == SDLK_MINUS)
#else
			else if (event.type == SDL_KEYDOWN && key == SDLK_MINUS)
#endif
			{
				camera_distance *= 1.15f;
				if (camera_distance > radius * 20.0f)
					camera_distance = radius * 20.0f;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.y != 0.0f)
#else
			else if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0.0f)
#endif
			{
				if (event.wheel.y < 0.0f)
				{
					camera_distance *= 0.85f;
					if (camera_distance < radius * 1.2f)
						camera_distance = radius * 1.2f;
				}
				else
				{
					camera_distance *= 1.15f;
					if (camera_distance > radius * 20.0f)
						camera_distance = radius * 20.0f;
				}
			}
		}
		if (!paused)
		{
			uint64_t now = SDL_GetTicks();
			while (now - last_frame_tick >= FRAME_DURATION_MS)
			{
				current_frame = (current_frame + 1) % model.frame_count;
				last_frame_tick += FRAME_DURATION_MS;
			}
		}
		previous_weight = paused ? 0.0f :
			1.0f - (float)(SDL_GetTicks() - last_frame_tick) / (float)FRAME_DURATION_MS;
		if (previous_weight < 0.0f)
			previous_weight = 0.0f;
		if (previous_weight > 1.0f)
			previous_weight = 1.0f;
		interpolate_frame(&model, current_frame, previous_weight);
	#ifdef USE_SDL3
		SDL_GetWindowSizeInPixels(window, &width, &height);
	#else
		SDL_GetWindowSize(window, &width, &height);
	#endif
		glViewport(0, 0, width, height);
		glClearColor(0.035f, 0.045f, 0.065f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		{
			float aspect = (float)width / (float)(height > 0 ? height : 1);
			float near_plane = radius * 0.05f;
			float half_height = near_plane * tanf(30.0f * M_PI / 180.0f);
			glFrustum(-half_height * aspect, half_height * aspect, -half_height,
				half_height, near_plane, radius * 25.0f);
		}
		light_angle = (float)SDL_GetTicks() * 0.0015f;
		moving_light[0] = cosf(light_angle) * radius * 2.5f;
		moving_light[1] = sinf(light_angle) * radius * 2.5f;
		moving_light[2] = -radius * 3.0f;
		moving_light[3] = 1.0f;
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLightfv(GL_LIGHT0, GL_POSITION, moving_light);
		glTranslatef(0.0f, 0.0f, -camera_distance);
		glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
		glRotatef(model_pitch, 1.0f, 0.0f, 0.0f);
		glRotatef(model_yaw, 0.0f, 1.0f, 0.0f);
		draw_model(&model);
		draw_skeleton(&model);
		SDL_GL_SwapWindow(window);
	}
#ifdef USE_SDL3
	SDL_GL_DestroyContext(context);
#else
	SDL_GL_DeleteContext(context);
#endif
	SDL_DestroyWindow(window);
	SDL_Quit();
	model_free(&model);
	return 0;
}
