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

#define FRAME_DURATION_MS 100

typedef struct
{
	vec3_t origin, direction, up;
} JointPose;

typedef struct {
	int joint_count;
	int *parent;
	vec3_t *angles;
} RuntimeSkeleton;

typedef struct {
	byte *data;
	size_t data_size;
	dstvert_t *texcoords;
	dtriangle_t *triangles;
	vec3_t *vertices;
	vec3_t *raw_vertices;
	vec3_t *display_vertices;
	vec3_t *frame_scales;
	vec3_t *frame_translates;
	int **cluster_vertices;
	int *cluster_vertex_counts;
	JointPose *joint_poses;
	RuntimeSkeleton runtime_skeleton;
	int texcoord_count;
	int triangle_count;
	int vertex_count;
	int frame_count;
	int skeletal_type;
	int joint_count;
	int skeleton_cluster_count;
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
	free(model->raw_vertices);
	free(model->display_vertices);
	free(model->frame_scales);
	free(model->frame_translates);
	free(model->joint_poses);
	free(model->runtime_skeleton.parent);
	free(model->runtime_skeleton.angles);

	if (model->cluster_vertices)
	{
		for (int i = 0; i < model->skeleton_cluster_count; ++i)
		{
			free(model->cluster_vertices[i]);
		}
	}

	free(model->cluster_vertices);
	free(model->cluster_vertex_counts);
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
	model->raw_vertices = (vec3_t *)calloc((size_t)vertex_count * (size_t)frame_count, sizeof(*model->raw_vertices));
	model->display_vertices = (vec3_t *)calloc((size_t)vertex_count, sizeof(*model->display_vertices));
	model->frame_scales = (vec3_t *)calloc((size_t)frame_count, sizeof(*model->frame_scales));
	model->frame_translates = (vec3_t *)calloc((size_t)frame_count, sizeof(*model->frame_translates));
	if (!model->texcoords || !model->triangles || !model->vertices || !model->raw_vertices ||
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
			if (model->triangles[i].index_xyz[corner] >= model->vertex_count ||
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
		vec3_t *destination = &model->vertices[(size_t)frame * (size_t)vertex_count + (size_t)vertex];
		(*destination)[0] = x;
		(*destination)[1] = y;
		(*destination)[2] = z;
		model->raw_vertices[(size_t)frame * (size_t)vertex_count + (size_t)vertex][0] = (*destination)[0];
		model->raw_vertices[(size_t)frame * (size_t)vertex_count + (size_t)vertex][1] = (*destination)[1];
		model->raw_vertices[(size_t)frame * (size_t)vertex_count + (size_t)vertex][2] = (*destination)[2];
	}
}

static int skeleton_joint_count(int skeletal_type)
{
	static const int counts[] = {3, 1, 2, 2, 3, 3};
	return skeletal_type >= 0 && skeletal_type < ARRLEN(counts) ?
		counts[skeletal_type] : 0;
}

static int build_runtime_skeleton(Model *model)
{
	const int type = model->skeletal_type;
	const int joint_count = skeleton_joint_count(type);
	if (joint_count == 0 || model->skeleton_cluster_count != joint_count)
	{
		return 0;
	}

	model->runtime_skeleton.joint_count = joint_count;
	model->runtime_skeleton.parent = (int *)calloc((size_t)joint_count,
		sizeof(*model->runtime_skeleton.parent));
	model->runtime_skeleton.angles = (float (*)[3])calloc((size_t)joint_count,
		sizeof(*model->runtime_skeleton.angles));
	if (!model->runtime_skeleton.parent || !model->runtime_skeleton.angles)
	{
		free(model->runtime_skeleton.parent);
		free(model->runtime_skeleton.angles);
		model->runtime_skeleton.parent = NULL;
		model->runtime_skeleton.angles = NULL;
		model->runtime_skeleton.joint_count = 0;
		return 0;
	}
	for (int joint = 0; joint < joint_count; ++joint)
	{
		model->runtime_skeleton.parent[joint] = -1;
		VectorClear(model->runtime_skeleton.angles[joint]);
	}
	/* lower-back -> upper-back -> head for Raven,
	   Plague Elf, and Corvus; the other definitions are one short chain. */
	if (type == 0 || type == 4 || type == 5)
	{
		model->runtime_skeleton.parent[1] = 0;
		model->runtime_skeleton.parent[2] = 1;
	}
	else if (type == 2 || type == 3)
	{
		model->runtime_skeleton.parent[1] = 0;
	}
	return 1;
}

static void rotate_runtime_point(vec3_t out, const vec3_t point, const vec3_t origin, const float angles[3])
{
	const float cx = cosf(angles[0]), sx = sinf(angles[0]);
	const float cy = cosf(angles[1]), sy = sinf(angles[1]);
	const float cz = cosf(angles[2]), sz = sinf(angles[2]);
	vec3_t local, rotated;
	VectorSubtract(point, origin, local);
	rotated[0] = (cy * cz + sx * sy * sz) * local[0] + (cz * sx * sy - cy * sz) * local[1] + cx * sy * local[2];
	rotated[1] = cx * sz * local[0] + cx * cz * local[1] - sx * local[2];
	rotated[2] = (cy * sx * sz - cz * sy) * local[0] + (cy * cz * sx + sy * sz) * local[1] + cx * cy * local[2];
	VectorAdd(origin, rotated, out);
}

static void rotate_runtime_cluster(Model *model, int frame, int joint, vec3_t *vertices)
{
	const JointPose *pose = &model->joint_poses[(size_t)frame * (size_t)model->skeleton_cluster_count + (size_t)joint];
	for (int child = 0; child < model->runtime_skeleton.joint_count; ++child)
	{
		if (model->runtime_skeleton.parent[child] == joint)
			rotate_runtime_cluster(model, frame, child, vertices);
	}
	for (int vertex = 0; vertex < model->cluster_vertex_counts[joint]; ++vertex)
	{
		const int index = model->cluster_vertices[joint][vertex];
		const float *angles = model->runtime_skeleton.angles[joint];
		if (angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f)
		{
			vec3_t rotated;
			rotate_runtime_point(rotated, vertices[index], pose->origin, model->runtime_skeleton.angles[joint]);
			VectorCopy(rotated, vertices[index]);
		}
	}
}

static int load_flex_skeleton(Model *model, size_t offset, int32_t block_size, char *error, size_t error_size)
{
	size_t cursor = offset;
	int cluster;
	int raw_counts[8];
	int running_total = 0;
	int index_base = 0;
	if (block_size < 12 || !range_valid(model, offset, (size_t)block_size))
	{
		snprintf(error, error_size, "flex skeleton block is truncated");
		return 0;
	}
	model->skeletal_type = LittleLong(*((int *)(model->data + cursor)));
	model->joint_count = skeleton_joint_count(model->skeletal_type);
	model->skeleton_cluster_count = LittleLong(*((int *)(model->data + cursor + 4)));
	cursor += 8;
	if (model->skeleton_cluster_count != model->joint_count) {
		snprintf(error, error_size, "flex skeleton has an invalid cluster count");
		return 0;
	}
	model->cluster_vertex_counts = (int *)calloc((size_t)model->skeleton_cluster_count, sizeof(int));
	model->cluster_vertices = (int **)calloc((size_t)model->skeleton_cluster_count, sizeof(int *));
	if (!model->cluster_vertex_counts || !model->cluster_vertices) {
		snprintf(error, error_size, "out of memory loading flex skeleton");
		return 0;
	}
	for (cluster = model->skeleton_cluster_count - 1; cluster >= 0; --cluster)
	{
		if (!range_valid(model, cursor, 4))
		{
			snprintf(error, error_size, "flex skeleton counts are truncated");
			return 0;
		}
		raw_counts[cluster] = LittleLong(*((int *)(model->data + cursor)));
		cursor += 4;
		if (raw_counts[cluster] < 0 || raw_counts[cluster] > model->vertex_count)
		{
			snprintf(error, error_size, "flex skeleton has invalid cluster vertices");
			return 0;
		}
		running_total += raw_counts[cluster];
		if (running_total > model->vertex_count) {
			snprintf(error, error_size, "flex skeleton cluster vertices exceed model");
			return 0;
		}
		model->cluster_vertex_counts[cluster] = running_total;
		model->cluster_vertices[cluster] = (int *)malloc((size_t)running_total * sizeof(int));
		if (running_total > 0 && !model->cluster_vertices[cluster]) {
			snprintf(error, error_size, "out of memory loading flex cluster");
			return 0;
		}
	}
	for (cluster = model->skeleton_cluster_count - 1; cluster >= 0; --cluster)
	{
		const int count = model->cluster_vertex_counts[cluster];
		for (int vertex = index_base; vertex < count; ++vertex)
		{
			const int index = LittleLong(*((int *)(model->data + cursor)));
			cursor += 4;
			if (index < 0 || index >= model->vertex_count) {
				snprintf(error, error_size, "flex skeleton references an invalid vertex");
				return 0;
			}
			for (int parent = 0; parent <= cluster; ++parent)
				model->cluster_vertices[parent][vertex] = index;
		}
		index_base = count;
	}
	if (!range_valid(model, cursor, 4))
	{
		snprintf(error, error_size, "flex skeleton flag is truncated");
		return 0;
	}

	if (!LittleLong(*((int *)(model->data + cursor))))
	{
		return 1;
	}

	cursor += 4;

	if (!range_valid(model, cursor, (size_t)model->frame_count * (size_t)model->skeleton_cluster_count * 36)) {
		snprintf(error, error_size, "flex skeleton poses are truncated");
		return 0;
	}
	model->joint_poses = (JointPose *)calloc((size_t)model->frame_count * (size_t)model->skeleton_cluster_count, sizeof(JointPose));
	if (!model->joint_poses)
	{
		snprintf(error, error_size, "out of memory loading flex poses");
		return 0;
	}
	for (int frame = 0; frame < model->frame_count; ++frame)
	{
		for (cluster = 0; cluster < model->skeleton_cluster_count; ++cluster)
		{
			JointPose *pose = &model->joint_poses[(size_t)frame * (size_t)model->skeleton_cluster_count + (size_t)cluster];
			VectorSet(pose->origin,
					LittleFloat(*((float *)(model->data + cursor))),
					LittleFloat(*((float *)(model->data + cursor + 4))),
					LittleFloat(*((float *)(model->data + cursor + 8)))
			);
			cursor += 12;
			VectorSet(pose->direction,
					LittleFloat(*((float *)(model->data + cursor))),
					LittleFloat(*((float *)(model->data + cursor + 4))),
					LittleFloat(*((float *)(model->data + cursor + 8)))
			);
			cursor += 12;
			VectorSet(pose->up,
					LittleFloat(*((float *)(model->data + cursor))),
					LittleFloat(*((float *)(model->data + cursor + 4))),
					LittleFloat(*((float *)(model->data + cursor + 8)))
			);
			cursor += 12;
		}
	}
	return 1;
}

static void apply_flex_skeleton(Model *model)
{
	if (!model->joint_poses || !build_runtime_skeleton(model))
		return;
	for (int frame = 0; frame < model->frame_count; ++frame)
	{
		vec3_t *vertices = model->vertices + (size_t)frame * (size_t)model->vertex_count;
		const vec3_t *raw = model->raw_vertices + (size_t)frame * (size_t)model->vertex_count;
		memcpy(vertices, raw, (size_t)model->vertex_count * sizeof(*vertices));
		rotate_runtime_cluster(model, frame, 0, vertices);
	}
}

static void compare_skeletal_frames(const Model *model)
{
	if (!model->joint_poses || model->frame_count < 2)
		return;
	printf("skeleton comparison (frame 0 excluded):\n");
	for (int frame = 1; frame < model->frame_count; ++frame)
	{
		const vec3_t *raw = model->raw_vertices + (size_t)frame * (size_t)model->vertex_count;
		const vec3_t *skinned = model->vertices + (size_t)frame * (size_t)model->vertex_count;
		int changed = 0;
		float max_displacement = 0.0f;
		double total_displacement = 0.0;
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			vec3_t delta;
			VectorSubtract(skinned[vertex], raw[vertex], delta);
			const float displacement = VectorLength(delta);
			if (displacement > 0.0001f)
				++changed;
			if (displacement > max_displacement)
				max_displacement = displacement;
			total_displacement += displacement;
		}
		if (frame == 1 || frame == model->frame_count - 1)
		{
			printf("  frame %d: changed=%d/%d max=%f average=%f\n",
				frame, changed, model->vertex_count,
				max_displacement, (float)(total_displacement / model->vertex_count));
		}
	}
}

static void lerp_vec3(vec3_t out, const vec3_t current, const vec3_t previous, float previous_weight)
{
	const float current_weight = 1.0f - previous_weight;
	out[0] = current[0] * current_weight + previous[0] * previous_weight;
	out[1] = current[1] * current_weight + previous[1] * previous_weight;
	out[2] = current[2] * current_weight + previous[2] * previous_weight;
}

static void lerp_skeleton_component(vec3_t out, const Model *model, const vec3_t current,
	const vec3_t previous, int frame, int previous_frame, float previous_weight)
{
	const float current_weight = 1.0f - previous_weight;
	out[0] = current[0] * model->frame_scales[frame][0] * current_weight +
		previous[0] * model->frame_scales[previous_frame][0] * previous_weight +
		model->frame_translates[frame][0] * current_weight +
		model->frame_translates[previous_frame][0] * previous_weight;
	out[1] = current[1] * model->frame_scales[frame][1] * current_weight +
		previous[1] * model->frame_scales[previous_frame][1] * previous_weight +
		model->frame_translates[frame][1] * current_weight +
		model->frame_translates[previous_frame][1] * previous_weight;
	out[2] = current[2] * model->frame_scales[frame][2] * current_weight +
		previous[2] * model->frame_scales[previous_frame][2] * previous_weight +
		model->frame_translates[frame][2] * current_weight +
		model->frame_translates[previous_frame][2] * previous_weight;
}

static void interpolate_frame(Model *model, int frame, float previous_weight)
{
	const int previous_frame = frame > 0 ? frame - 1 : model->frame_count - 1;
	const vec3_t *current = model->raw_vertices + (size_t)frame * (size_t)model->vertex_count;
	const vec3_t *previous = model->raw_vertices + (size_t)previous_frame * (size_t)model->vertex_count;
	vec3_t scale, translate;
	lerp_vec3(scale, model->frame_scales[frame], model->frame_scales[previous_frame], previous_weight);
	lerp_vec3(translate, model->frame_translates[frame], model->frame_translates[previous_frame], previous_weight);
	for (int vertex = 0; vertex < model->vertex_count; ++vertex)
	{
		lerp_vec3(model->display_vertices[vertex], current[vertex], previous[vertex], previous_weight);
	}
	if (model->joint_poses && model->runtime_skeleton.joint_count > 0)
	{
		/* Runtime joint angles are defined before frame scale/translation,
		   matching RotateModelSegment followed by CNode::ShowFrame(). */
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			model->display_vertices[vertex][0] = (model->display_vertices[vertex][0] - translate[0]) / scale[0];
			model->display_vertices[vertex][1] = (model->display_vertices[vertex][1] - translate[1]) / scale[1];
			model->display_vertices[vertex][2] = (model->display_vertices[vertex][2] - translate[2]) / scale[2];
		}
		rotate_runtime_cluster(model, frame, 0, model->display_vertices);
		for (int vertex = 0; vertex < model->vertex_count; ++vertex)
		{
			model->display_vertices[vertex][0] = model->display_vertices[vertex][0] * scale[0] + translate[0];
			model->display_vertices[vertex][1] = model->display_vertices[vertex][1] * scale[1] + translate[1];
			model->display_vertices[vertex][2] = model->display_vertices[vertex][2] * scale[2] + translate[2];
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
	if (frame_size < 40 ||
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
	size_t offset = 0, header = 0, st = 0, tris = 0, frames = 0, skeleton = 0;
	int32_t header_size = 0, st_size = 0, tris_size = 0, frames_size = 0, skeleton_size = 0;
	int skin_width, skin_height, frame_size, vertex_count, texcoord_count, triangle_count, frame_count, i;
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
	if (skin_width <= 0 ||
		skin_height <= 0 ||
		frame_size < 40 ||
		frame_size < 40 + vertex_count * 4 ||
		st_size != texcoord_count * 4 ||
		tris_size != triangle_count * 12 ||
		frames_size != frame_count * frame_size ||
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

	if (skeleton && !load_flex_skeleton(model, skeleton, skeleton_size, error, error_size))
	{
		return 0;
	}

	apply_flex_skeleton(model);
	compare_skeletal_frames(model);
	calculate_bounds(model);
	return 1;
}

static void draw_model(const Model *model)
{
	int i, corner;
	const vec3_t *vertices = model->display_vertices;
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

static void draw_skeleton(const Model *model, int frame, float previous_weight)
{
	if (!model->joint_poses)
		return;
	glDisable(GL_LIGHTING);
	glColor3f(1.0f, 0.72f, 0.12f);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	for (int cluster = 0; cluster < model->skeleton_cluster_count; ++cluster)
	{
		const int previous_frame = frame > 0 ? frame - 1 : model->frame_count - 1;
		const JointPose *pose = &model->joint_poses[(size_t)frame * (size_t)model->skeleton_cluster_count + (size_t)cluster];
		const JointPose *previous = &model->joint_poses[(size_t)previous_frame * (size_t)model->skeleton_cluster_count + (size_t)cluster];
		vec3_t origin, direction;
		lerp_skeleton_component(origin, model, pose->origin, previous->origin, frame, previous_frame, previous_weight);
		lerp_skeleton_component(direction, model, pose->direction, previous->direction, frame, previous_frame, previous_weight);
		rotate_runtime_point(direction, direction, origin, model->runtime_skeleton.angles[cluster]);
		glVertex3f(origin[0], origin[1], origin[2]);
		glVertex3f(direction[0], direction[1], direction[2]);
	}
	glEnd();
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	for (int cluster = 0; cluster < model->skeleton_cluster_count; ++cluster)
	{
		const int previous_frame = frame > 0 ? frame - 1 : model->frame_count - 1;
		const JointPose *pose = &model->joint_poses[(size_t)frame * (size_t)model->skeleton_cluster_count + (size_t)cluster];
		const JointPose *previous = &model->joint_poses[(size_t)previous_frame * (size_t)model->skeleton_cluster_count + (size_t)cluster];
		vec3_t origin;
		lerp_skeleton_component(origin, model, pose->origin, previous->origin, frame, previous_frame, previous_weight);
		glVertex3f(origin[0], origin[1], origin[2]);
	}
	glEnd();
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
			else if (event.type == SDL_EVENT_KEY_DOWN && key >= SDLK_1 && key <= SDLK_8 && model.runtime_skeleton.joint_count > 0)
#else
			else if (event.type == SDL_KEYDOWN && key >= SDLK_1 && key <= SDLK_8 && model.runtime_skeleton.joint_count > 0)
#endif
			{
				selected_joint = (int)(key - SDLK_1);
				if (selected_joint >= model.runtime_skeleton.joint_count)
					selected_joint = model.runtime_skeleton.joint_count - 1;
			}
#ifdef USE_SDL3
			else if (event.type == SDL_EVENT_KEY_DOWN && model.runtime_skeleton.joint_count > 0 &&
				(key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN ||
				 key == SDLK_Q || key == SDLK_E))
#else
			else if (event.type == SDL_KEYDOWN && model.runtime_skeleton.joint_count > 0 &&
				(key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN ||
				 key == SDLK_q || key == SDLK_e))
#endif
			{
				const float angle_step = 5.0f * M_PI / 180.0f;
				if (key == SDLK_LEFT)
					model.runtime_skeleton.angles[selected_joint][1] -= angle_step;
				else if (key == SDLK_RIGHT)
					model.runtime_skeleton.angles[selected_joint][1] += angle_step;
				else if (key == SDLK_UP)
					model.runtime_skeleton.angles[selected_joint][0] -= angle_step;
				else if (key == SDLK_DOWN)
					model.runtime_skeleton.angles[selected_joint][0] += angle_step;
#ifdef USE_SDL3
				else if (key == SDLK_Q)
					model.runtime_skeleton.angles[selected_joint][2] -= angle_step;
				else if (key == SDLK_E)
					model.runtime_skeleton.angles[selected_joint][2] += angle_step;
#else
				else if (key == SDLK_q)
					model.runtime_skeleton.angles[selected_joint][2] -= angle_step;
				else if (key == SDLK_e)
					model.runtime_skeleton.angles[selected_joint][2] += angle_step;
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
		draw_skeleton(&model, current_frame, previous_weight);
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
