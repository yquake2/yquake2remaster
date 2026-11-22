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
 * The CTC models file format
 *
 * =======================================================================
 */

#include "models.h"
#include <limits.h>

typedef struct
{
	int parent;
	vec4_t base_transform[4];
} ctc_bone_runtime_t;

typedef struct
{
	int first;
	int count;
} ctc_vertex_weights_t;

typedef struct
{
	int bone;
	float weight;
} ctc_weight_t;

typedef struct
{
	dmdx_vert_t *vertices;
	ctc_bone_runtime_t *bones;
	ctc_vertex_weights_t *vertex_weights;
	ctc_weight_t *weights;
	vec4_t (*bone_poses)[4];
	vec4_t (*bone_transforms)[4];
	byte *bone_ready;
} ctc_temp_data_t;

typedef struct
{
	vec3_t translation;
	vec3_t angles;
} ctc_frame_key_t;

typedef struct
{
	char name[16];
	int first_frame;
	int last_frame;
} ctc_animation_input_t;

typedef struct
{
	int parent;						/* 1 + parent index, or 0 for root */
	int pad_0x4;
	int pad_0x8;
	float pad_0xc[6];
	vec4_t pad_0x24[4];
	vec4_t base_transform[4];		/* inverse/base pose matrix */
} ctc_bone_t;

typedef struct
{
	int num_weights;
	ctc_weight_t weights[];
} ctc_vertex_weights_input_t;

typedef struct
{
	int ident;
	byte vertex_data[];
} ctc_skeletal_data_t;

static void
Mod_CTCFreeTempData(ctc_temp_data_t *temp)
{
	free(temp->vertices);
	free(temp->bones);
	free(temp->vertex_weights);
	free(temp->weights);
	free(temp->bone_poses);
	free(temp->bone_transforms);
	free(temp->bone_ready);
	memset(temp, 0, sizeof(*temp));
}

static void
Mod_CTCMatrixMultiply(const vec4_t a[4], const vec4_t b[4], vec4_t out[4])
{
	int row, col, k;

	for (row = 0; row < 4; ++row)
	{
		for (col = 0; col < 4; ++col)
		{
			out[row][col] = 0.0f;
			for (k = 0; k < 4; ++k)
			{
				out[row][col] += a[row][k] * b[k][col];
			}
		}
	}
}

static void
Mod_CTCKeyMatrix(const ctc_frame_key_t *src, vec4_t matrix[4])
{
	vec3_t translation, angles;
	float sx, cx, sy, cy, sz, cz;
	int i;

	for (i = 0; i < 3; ++i)
	{
		translation[i] = LittleFloat(src->translation[i]);
		angles[i] = LittleFloat(src->angles[i]) * (float)(M_PI / 180.0);
	}

	sy = sinf(angles[0]);
	cy = cosf(angles[0]);
	sx = sinf(angles[1]);
	cx = cosf(angles[1]);
	sz = sinf(angles[2]);
	cz = cosf(angles[2]);

	/* CTC stores and applies its Euler angles in Y-X-Z order. */
	matrix[0][0] = cy * cz + sy * sx * sz;
	matrix[0][1] = -cy * sz + sy * sx * cz;
	matrix[0][2] = sy * cx;
	matrix[0][3] = translation[0];
	matrix[1][0] = cx * sz;
	matrix[1][1] = cx * cz;
	matrix[1][2] = -sx;
	matrix[1][3] = translation[1];
	matrix[2][0] = -sy * cz + cy * sx * sz;
	matrix[2][1] = sy * sz + cy * sx * cz;
	matrix[2][2] = cy * cx;
	matrix[2][3] = translation[2];
	matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0f;
	matrix[3][3] = 1.0f;
}

static void
Mod_CTCTransformPoint(const vec4_t matrix[4], const vec3_t point, vec3_t out)
{
	int row;

	for (row = 0; row < 3; ++row)
	{
		out[row] = DotProduct(matrix[row], point) + matrix[row][3];
	}
}

static void
Mod_CTCTransformNormal(const vec4_t matrix[3], const vec3_t normal, vec3_t out)
{
	int row;

	for (row = 0; row < 3; ++row)
	{
		out[row] = DotProduct(matrix[row], normal);
	}
}

static void
Mod_CTCConvertPoint(const vec3_t point, vec3_t out)
{
	out[0] = -point[2];
	out[1] = point[0];
	out[2] = point[1];
}

static void
Mod_CTCConvertMatrix(const vec4_t matrix[4], float out[3][4])
{
	static const float coordinate[3][3] =
	{
		{ 0.0f, 0.0f, -1.0f },
		{ 1.0f, 0.0f,  0.0f },
		{ 0.0f, 1.0f,  0.0f }
	};
	int row, col, k, l;

	for (row = 0; row < 3; ++row)
	{
		for (col = 0; col < 4; ++col)
		{
			out[row][col] = 0.0f;
			for (k = 0; k < 3; ++k)
			{
				if (col < 3)
				{
					for (l = 0; l < 3; ++l)
					{
						out[row][col] += coordinate[row][k] *
							matrix[k][l] * coordinate[col][l];
					}
				}
				else
				{
					out[row][col] += coordinate[row][k] *
						matrix[k][3];
				}
			}
		}
	}
}

static qboolean
Mod_CTCLoadFrames(const char *mod_name, const dctc_header_t *input,
	const byte *src, dmdx_t *pheader, ctc_temp_data_t *temp, int offset)
{
	dmdx_vert_t *posed_vertices;
	size_t i, framesize;

	framesize = (input->num_bones + offset) * sizeof(ctc_frame_key_t);

	posed_vertices = malloc(input->num_xyz * sizeof(*posed_vertices));
	if (!posed_vertices)
	{
		YQ2_COM_CHECK_OOM(posed_vertices, "calloc()",
			(size_t)input->num_xyz * sizeof(*posed_vertices))
		return false;
	}

	for (i = 0; i < input->num_frames; ++i)
	{
		const byte *frame_data = src + input->ofs_frames +
			(size_t)i * framesize;
		vec4_t root_transform[4];
		daliasxframe_t *frame = (daliasxframe_t *)((byte *)pheader +
			pheader->ofs_frames + (size_t)i * pheader->framesize);
		size_t j, completed;

		memset(posed_vertices, 0, input->num_xyz * sizeof(*posed_vertices));

		if (offset)
		{
			Mod_CTCKeyMatrix((const ctc_frame_key_t *)frame_data,
				root_transform);
		}
		else
		{
			ctc_frame_key_t key = {
				.translation = { 0.0f, 0.0f, 0.0f },
				.angles      = { 0.0f, -90.0f, 0.0f }
			};
			Mod_CTCKeyMatrix(&key, root_transform);
		}

		if (input->num_bones)
		{
			memset(temp->bone_ready, 0,
				(size_t)input->num_bones * sizeof(*temp->bone_ready));
			completed = 0;
			while (completed < input->num_bones)
			{
				qboolean progress = false;

				for (j = 0; j < input->num_bones; ++j)
				{
					vec4_t key_transform[4], pose_transform[4];
					vec4_t skin_transform[4];
					const byte *key = frame_data + (j + offset) * sizeof(ctc_frame_key_t);

					if (temp->bone_ready[j] ||
						(temp->bones[j].parent >= 0 &&
						 !temp->bone_ready[temp->bones[j].parent]))
					{
						continue;
					}

					Mod_CTCKeyMatrix((const ctc_frame_key_t *)key,
						key_transform);

					if (temp->bones[j].parent >= 0)
					{
						Mod_CTCMatrixMultiply(
							temp->bone_poses[temp->bones[j].parent],
							key_transform, pose_transform);
					}
					else
					{
						memcpy(pose_transform, key_transform,
							sizeof(pose_transform));
					}
					memcpy(temp->bone_poses[j], pose_transform,
						sizeof(pose_transform));
					Mod_CTCMatrixMultiply(root_transform,
						pose_transform, skin_transform);
					Mod_CTCMatrixMultiply(skin_transform,
						temp->bones[j].base_transform,
						temp->bone_transforms[j]);
					temp->bone_ready[j] = 1;
					++completed;
					progress = true;
				}

				if (!progress)
				{
					Com_Printf("%s: %s contains a cyclic CTC bone hierarchy\n",
						__func__, mod_name);
					free(posed_vertices);
					return false;
				}
			}

			if (pheader->ofs_baseframe_joints != 0)
			{
				dmdx_baseframe_joint_t *pose_row =
					(dmdx_baseframe_joint_t *)((byte *)pheader +
						pheader->ofs_baseframe_joints) + i * pheader->num_joints;

				for (j = 0; j < input->num_bones; ++j)
				{
					vec4_t world_pose[4];
					float engine_pose[3][4];

					Mod_CTCMatrixMultiply(root_transform,
						temp->bone_poses[j], world_pose);
					Mod_CTCConvertMatrix(world_pose, engine_pose);
					Mod_Mat3x4ToPose(engine_pose, pose_row[j].pos,
						pose_row[j].orient);

					if (i == 0)
					{
						dmdx_joint_t *out_joints =
							(dmdx_joint_t *)((byte *)pheader + pheader->ofs_joints);

						VectorCopy(pose_row[j].pos,
							out_joints[j].pos);
						memcpy(out_joints[j].orient,
							pose_row[j].orient,
							sizeof(out_joints[j].orient));
					}
				}
			}
		}

		for (j = 0; j < input->num_xyz; ++j)
		{
			vec3_t position, normal;

			VectorClear(position);
			VectorClear(normal);
			if (input->num_bones && temp->vertex_weights[j].count)
			{
				float total_weight = 0.0f;
				int k;

				for (k = 0; k < temp->vertex_weights[j].count; ++k)
				{
					const ctc_weight_t *weight =
						&temp->weights[temp->vertex_weights[j].first + k];
					const vec4_t *transform =
						temp->bone_transforms[weight->bone];
					vec3_t transformed;

					Mod_CTCTransformPoint(transform, temp->vertices[j].xyz,
						transformed);
					VectorMA(position, weight->weight, transformed, position);
					Mod_CTCTransformNormal(transform, temp->vertices[j].norm,
						transformed);
					VectorMA(normal, weight->weight, transformed, normal);
					total_weight += weight->weight;
				}

				if (total_weight > 0.0f)
				{
					VectorScale(position, 1.0f / total_weight, position);
					VectorScale(normal, 1.0f / total_weight, normal);
				}
				else
				{
					Mod_CTCTransformPoint(root_transform, temp->vertices[j].xyz,
						position);
					Mod_CTCTransformNormal(root_transform, temp->vertices[j].norm,
						normal);
				}
			}
			else
			{
				Mod_CTCTransformPoint(root_transform, temp->vertices[j].xyz,
					position);
				Mod_CTCTransformNormal(root_transform, temp->vertices[j].norm,
					normal);
			}

			/* Convert posed CTC coordinates to the Anachronox model basis. */
			posed_vertices[j].xyz[0] = -position[2];
			posed_vertices[j].xyz[1] = position[0];
			posed_vertices[j].xyz[2] = position[1];
			posed_vertices[j].norm[0] = -normal[2];
			posed_vertices[j].norm[1] = normal[0];
			posed_vertices[j].norm[2] = normal[1];
		}

		/* limit frame ids to 2**16 */
		snprintf(frame->name, sizeof(frame->name), "frame%d", (int)(i % 0xFFFF));
		for (j = 0; j < input->num_xyz; ++j)
		{
			VectorNormalize(posed_vertices[j].norm);
		}

		PrepareFrameVertex(posed_vertices, input->num_xyz, frame);
	}

	free(posed_vertices);

	return true;
}

static const namesconvert_t anox_ctc_names[] = {
	{"a2atak", "a2atak"},
	{"amb", "idle"}, /* ambient */
	{"atak", "attack"},
	{"bskill", "bskill"},
	{"cine", "cine"},
	{"climb", "climb"},
	{"dart", "dart"},
	{"die", "death"},
	{"gest", "gest"},
	{"grab", "grab"},
	{"hit", "hit"},
	{"lookl", "lookl"},
	{"lookr", "lookr"},
	{"run", "run"},
	{"sit", "sit"},
	{"skill", "skill"},
	{"talk", "talk"},
	{"turn_l", "turn_l"},
	{"turn_r", "turn_r"},
	{"w_skill", "w_skill"},
	{"walk", "walk"},
	{"win", "win"},
	{"wskill", "wskill"},
	{NULL, NULL}
};

static qboolean
Mod_CTCBuildGeometry(const char *mod_name, const dctc_header_t *input,
	const byte *src, dmdx_t *pheader)
{
	int i, corner;
	dstvert_t *st;
	dtriangle_t *triangles;
	dmdxmesh_t *mesh;

	if (input->num_skins)
	{
		char *skins = (char *)pheader + pheader->ofs_skins;
		memcpy(skins, src + input->ofs_skins,
			(size_t)input->num_skins * MAX_SKINNAME);
		for (i = 0; i < input->num_skins; ++i)
		{
			skins[(size_t)i * MAX_SKINNAME + MAX_SKINNAME - 1] = '\0';
		}
	}

	st = (dstvert_t *)((byte *)pheader + pheader->ofs_st);
	triangles = (dtriangle_t *)((byte *)pheader + pheader->ofs_tris);
	for (i = 0; i < input->num_tris; ++i)
	{
		const short (*in_xyz)[3] =
			(const short (*)[3])(src + input->ofs_tris) + i;
		const float (*in_st)[3][2] =
			(const float (*)[3][2])(src + input->ofs_st) + i;

		for (corner = 0; corner < 3; ++corner)
		{
			const int vertex = LittleShort((*in_xyz)[corner]);
			const float s = LittleFloat((*in_st)[corner][0]);
			const float t = LittleFloat((*in_st)[corner][1]);

			if (vertex < 0 || vertex >= input->num_xyz)
			{
				Com_Printf("%s: %s contains invalid CTC triangle data\n",
					__func__, mod_name);
				return false;
			}

			triangles[i].index_xyz[corner] = vertex;
			triangles[i].index_st[corner] = i * 3 + corner;
			st[i * 3 + corner].s = (short)(s * input->skinwidth);
			st[i * 3 + corner].t = (short)(t * input->skinheight);
		}
	}

	mesh = (dmdxmesh_t *)((byte *)pheader + pheader->ofs_meshes);
	mesh->ofs_tris = 0;
	mesh->num_tris = input->num_tris;
	mesh->ofs_glcmds = 0;
	mesh->num_glcmds = pheader->num_glcmds;

	if (input->num_animgroup)
	{
		dmdxframegroup_t *framegroups = (dmdxframegroup_t *)
			((byte *)pheader + pheader->ofs_animgroup);
		int frame_offset = 0;

		for (i = 0; i < input->num_animgroup; ++i)
		{
			const ctc_animation_input_t *animation =
				(const ctc_animation_input_t *)(src + input->ofs_animgroup) + i;
			int first_frame = LittleLong(animation->first_frame);
			int last_frame = LittleLong(animation->last_frame);

			Q_strlcpy(framegroups[i].name, animation->name, Q_min(sizeof(framegroups[i].name), sizeof(animation->name)));
			Q_strlwr(framegroups[i].name);

			framegroups[i].ofs = frame_offset;
			framegroups[i].num = last_frame - first_frame + 1;
			frame_offset += framegroups[i].num;
			Mod_UpdateMinMaxByFrames(pheader, framegroups[i].ofs,
				framegroups[i].ofs + framegroups[i].num,
				framegroups[i].mins, framegroups[i].maxs);
		}
	}
	else
	{
		Mod_LoadAnimGroupList(pheader, true);
	}

	Mod_LoadModel_AnimGroupNamesFix(pheader, anox_ctc_names);

	return true;
}

static int
Mod_CTCDetectRootBone(dctc_header_t *input)
{
	int *ofs_start, *ofs_end, ofs_frames_end;

	ofs_start = &input->ofs_skins;
	ofs_end = &input->ofs_end;
	ofs_frames_end = input->ofs_end;

	while (ofs_start < ofs_end)
	{
		if (*ofs_start > input->ofs_frames && *ofs_start < ofs_frames_end)
		{
			ofs_frames_end = *ofs_start;
		}
		ofs_start++;
	}

	ofs_frames_end -= input->ofs_frames;

	if (ofs_frames_end == input->num_bones * sizeof(ctc_frame_key_t) * input->num_frames)
	{
		return 0;
	}

	if (ofs_frames_end == (input->num_bones + 1) * sizeof(ctc_frame_key_t) * input->num_frames)
	{
		return 1;
	}

	return -1;
}

void *
Mod_LoadModel_CTC(const char *mod_name, const void *buffer, int modfilelen)
{
	dctc_header_t input;
	dmdx_t dmdxheader;
	dmdx_t *pheader = NULL;
	ctc_temp_data_t temp = { 0 };
	const byte *src = buffer;
	void *extradata = NULL;
	size_t file_size, cursor, num_weights = 0, bones_offset;
	size_t animation_frame_count = 0;
	int i;

	if (modfilelen < (int)sizeof(input))
	{
		Com_Printf("%s: %s has a truncated CTC header\n", __func__, mod_name);
		return NULL;
	}

	Mod_LittleHeader((const int *)buffer, sizeof(input) / sizeof(int),
		(int *)&input);

	if (input.ident != CTCHEADER || input.version != CTC_VERSION)
	{
		Com_Printf("%s: %s has an unsupported CTC header or version %d\n",
			__func__, mod_name, input.version);
		return NULL;
	}

	file_size = input.ofs_end;
	if (input.ofs_end < (int)sizeof(input) || input.ofs_end > modfilelen)
	{
		Com_Printf("%s: %s has an invalid CTC end offset %d\n",
			__func__, mod_name, input.ofs_end);
		return NULL;
	}

	if (input.num_skins < 0)
	{
		Com_Printf("%s: %s has an invalid CTC skin count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_xyz <= 0 || input.num_xyz > MAX_VERTS)
	{
		Com_Printf("%s: %s has an invalid CTC vertex count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_tris <= 0 || input.num_tris > MAX_TRIANGLES)
	{
		Com_Printf("%s: %s has an invalid CTC triangle count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_st <= 0 || input.num_st != input.num_tris * 3)
	{
		Com_Printf("%s: %s has an invalid CTC texture coordinate count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_frames <= 0)
	{
		Com_Printf("%s: %s has an invalid CTC frame count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_animgroup < 0 ||
		input.num_animgroup > input.num_frames)
	{
		Com_Printf("%s: %s has an invalid CTC animation group count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_bones < 0)
	{
		Com_Printf("%s: %s has an invalid CTC bone count\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.skinwidth <= 0 || input.skinheight <= 0)
	{
		Com_Printf("%s: %s has invalid CTC skin dimensions\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_skins &&
		(input.ofs_skins < 0 ||
		 input.ofs_skins > file_size ||
		 input.num_skins >
			(file_size - input.ofs_skins) / MAX_SKINNAME))
	{
		Com_Printf("%s: %s has an invalid CTC skin section range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.ofs_verts < 0 ||
		input.ofs_verts > file_size ||
		input.num_xyz >
			(file_size - input.ofs_verts) / sizeof(dmdx_vert_t))
	{
		Com_Printf("%s: %s has an invalid CTC vertex section range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.ofs_tris < 0 ||
		input.ofs_tris > file_size ||
		input.num_tris >
			(file_size - input.ofs_tris) / sizeof(short[3]))
	{
		Com_Printf("%s: %s has an invalid CTC triangle section range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.ofs_st < 0 ||
		input.ofs_st > file_size ||
		input.num_tris >
			(file_size - input.ofs_st) / sizeof(float[3][2]))
	{
		Com_Printf("%s: %s has an invalid CTC texture coordinate section range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_bones &&
		(input.ofs_bones < 0 ||
		 input.ofs_bones > file_size ||
		 input.num_bones >
			(file_size - input.ofs_bones) /
				sizeof(ctc_bone_t)))
	{
		Com_Printf("%s: %s has an invalid CTC bone section range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_bones &&
		(input.ofs_skeletal < 0 ||
		 input.ofs_skeletal > file_size ||
		 sizeof(ctc_skeletal_data_t) >
			file_size - input.ofs_skeletal))
	{
		Com_Printf("%s: %s has an invalid CTC skeletal data range\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.num_animgroup &&
		(input.ofs_animgroup < 0 ||
		 input.ofs_animgroup > file_size ||
		 input.num_animgroup >
			(file_size - input.ofs_animgroup) /
				sizeof(ctc_animation_input_t)))
	{
		Com_Printf("%s: %s has an invalid CTC animation group range\n",
			__func__, mod_name);
		return NULL;
	}

	for (i = 0; i < input.num_animgroup; ++i)
	{
		const ctc_animation_input_t *animation =
			(const ctc_animation_input_t *)(src + input.ofs_animgroup) + i;
		int first_frame = LittleLong(animation->first_frame);
		int last_frame = LittleLong(animation->last_frame);
		size_t animation_frames;

		if (!memchr(animation->name, '\0', sizeof(animation->name)) ||
			animation->name[0] == '\0' ||
			first_frame < 0 || last_frame < first_frame ||
			(size_t)(last_frame - first_frame) >=
				input.num_frames)
		{
			Com_Printf("%s: %s contains an invalid CTC animation range\n",
				__func__, mod_name);
			return NULL;
		}

		animation_frames = (size_t)(last_frame - first_frame) + 1;
		if (animation_frames > input.num_frames -
			animation_frame_count)
		{
			Com_Printf("%s: %s CTC animation ranges exceed the frame count\n",
				__func__, mod_name);
			return NULL;
		}
		animation_frame_count += animation_frames;
	}

	if (input.num_animgroup &&
		animation_frame_count != input.num_frames)
	{
		Com_Printf("%s: %s CTC animation ranges do not cover all frames\n",
			__func__, mod_name);
		return NULL;
	}

	bones_offset = Mod_CTCDetectRootBone(&input);
	if (bones_offset < 0)
	{
		Com_Printf("%s: %s CTC issues with number bones in frames\n",
			__func__, mod_name);
		return NULL;
	}

	if (input.ofs_frames < 0 ||
		input.ofs_frames > file_size ||
		(size_t)(input.num_frames * (input.num_bones + bones_offset) * sizeof(ctc_frame_key_t)) > file_size)
	{
		Com_Printf("%s: %s has truncated or overlapping CTC frame data\n",
			__func__, mod_name);
		return NULL;
	}

	temp.vertices = calloc(input.num_xyz, sizeof(*temp.vertices));
	if (!temp.vertices)
	{
		Mod_CTCFreeTempData(&temp);
		YQ2_COM_CHECK_OOM(temp.vertices, "calloc()",
			input.num_xyz * sizeof(*temp.vertices))
		return NULL;
	}

	{
		const dmdx_vert_t *input_vertices =
			(const dmdx_vert_t *)(src + input.ofs_verts);

		for (i = 0; i < input.num_xyz; ++i)
		{
			int axis;

			for (axis = 0; axis < 3; ++axis)
			{
				temp.vertices[i].xyz[axis] =
					LittleFloat(input_vertices[i].xyz[axis]);
				temp.vertices[i].norm[axis] =
					LittleFloat(input_vertices[i].norm[axis]);
			}
		}
	}

	if (input.num_bones)
	{
		temp.bones = calloc(input.num_bones, sizeof(*temp.bones));
		if (!temp.bones)
		{
			Mod_CTCFreeTempData(&temp);
			YQ2_COM_CHECK_OOM(temp.bones, "calloc()", input.num_bones)
			return NULL;
		}

		temp.bone_poses = calloc(input.num_bones, sizeof(*temp.bone_poses));
		if (!temp.bone_poses)
		{
			Mod_CTCFreeTempData(&temp);
			YQ2_COM_CHECK_OOM(temp.bone_poses, "calloc()",
				input.num_bones * sizeof(*temp.bone_poses))
			return NULL;
		}

		temp.bone_transforms = calloc(input.num_bones, sizeof(*temp.bone_transforms));
		if (!temp.bone_transforms)
		{
			Mod_CTCFreeTempData(&temp);
			YQ2_COM_CHECK_OOM(temp.bone_transforms, "calloc()",
				input.num_bones * sizeof(*temp.bone_transforms))
			return NULL;
		}

		temp.bone_ready = calloc(input.num_bones, sizeof(*temp.bone_ready));
		if (!temp.bone_ready)
		{
			Mod_CTCFreeTempData(&temp);
			YQ2_COM_CHECK_OOM(temp.bone_ready, "calloc()", input.num_bones)
			return NULL;
		}

		temp.vertex_weights = calloc(input.num_xyz, sizeof(*temp.vertex_weights));
		if (!temp.vertex_weights)
		{
			Mod_CTCFreeTempData(&temp);
			YQ2_COM_CHECK_OOM(temp.vertex_weights, "calloc()",
				input.num_xyz * sizeof(*temp.vertex_weights))
			return NULL;
		}

		for (i = 0; i < input.num_bones; ++i)
		{
			const ctc_bone_t *input_bone =
				(const ctc_bone_t *)(src + input.ofs_bones) + i;
			int parent_plus_one = LittleLong(
				input_bone->parent);
			int j;

			if (parent_plus_one < 0 || parent_plus_one > input.num_bones ||
				parent_plus_one == i + 1)
			{
				Mod_CTCFreeTempData(&temp);
				Com_Printf("%s: %s contains an invalid CTC bone parent\n",
					__func__, mod_name);
				return NULL;
			}

			temp.bones[i].parent = parent_plus_one - 1;
			for (j = 0; j < 4; ++j)
			{
				int column;

				for (column = 0; column < 4; ++column)
				{
					temp.bones[i].base_transform[j][column] = LittleFloat(
						input_bone->base_transform[column][j]);
				}
			}
		}

		cursor = input.ofs_skeletal;
		{
			const ctc_skeletal_data_t *skeletal =
				(const ctc_skeletal_data_t *)(src + cursor);

			if (LittleLong(skeletal->ident) != CTCSKELHEADER)
			{
				Mod_CTCFreeTempData(&temp);
				Com_Printf("%s: %s has invalid CTC skeletal data\n",
					__func__, mod_name);
				return NULL;
			}

			cursor += sizeof(skeletal->ident);
		}

		for (i = 0; i < input.num_xyz; ++i)
		{
			const ctc_vertex_weights_input_t *input_weights =
				(const ctc_vertex_weights_input_t *)(src + cursor);
			int count;

			if (file_size - cursor < sizeof(input_weights->num_weights))
			{
				Mod_CTCFreeTempData(&temp);
				Com_Printf("%s: %s has truncated CTC skeletal data\n",
					__func__, mod_name);
				return NULL;
			}

			count = LittleLong(input_weights->num_weights);
			if (count < 0 || count > input.num_bones ||
				(size_t)count >
					(file_size - cursor - sizeof(input_weights->num_weights)) /
						sizeof(ctc_weight_t) ||
				num_weights > (size_t)INT_MAX - (size_t)count)
			{
				Mod_CTCFreeTempData(&temp);
				Com_Printf("%s: %s has too many CTC vertex weights\n",
					__func__, mod_name);
				return NULL;
			}

			temp.vertex_weights[i].first = (int)num_weights;
			temp.vertex_weights[i].count = count;
			num_weights += (size_t)count;
			cursor += sizeof(input_weights->num_weights) +
				(size_t)count * sizeof(ctc_weight_t);
		}

		if (num_weights > (size_t)-1 / sizeof(*temp.weights) ||
			num_weights > (size_t)INT_MAX)
		{
			Mod_CTCFreeTempData(&temp);
			Com_Printf("%s: %s has too many CTC vertex weights\n",
				__func__, mod_name);
			return NULL;
		}

		if (num_weights)
		{
			temp.weights = calloc(num_weights, sizeof(*temp.weights));
			if (!temp.weights)
			{
				Mod_CTCFreeTempData(&temp);
				YQ2_COM_CHECK_OOM(temp.weights, "calloc()",
					num_weights * sizeof(*temp.weights))
				return NULL;
			}

			cursor = input.ofs_skeletal + sizeof(ctc_skeletal_data_t);
			num_weights = 0;
			for (i = 0; i < input.num_xyz; ++i)
			{
				const ctc_vertex_weights_input_t *input_weights =
					(const ctc_vertex_weights_input_t *)(src + cursor);
				int count = LittleLong(input_weights->num_weights);
				int j;
				float total_weight = 0.0f;

				for (j = 0; j < count; ++j)
				{
					const ctc_weight_t *input_weight = &input_weights->weights[j];
					float weight = LittleFloat(input_weight->weight);

					if (weight > 0.0f)
					{
						total_weight += weight;
					}
				}

				for (j = 0; j < count; ++j)
				{
					const ctc_weight_t *input_weight = &input_weights->weights[j];
					int bone = LittleLong(input_weight->bone);
					float weight = LittleFloat(input_weight->weight);

					if (bone < 0 || bone >= input.num_bones || weight < 0.0f)
					{
						Mod_CTCFreeTempData(&temp);
						Com_Printf("%s: %s contains invalid CTC vertex weights\n",
							__func__, mod_name);
						return NULL;
					}

					temp.weights[num_weights].bone = bone;
					temp.weights[num_weights].weight =
						total_weight > 0.0f ? weight / total_weight : 0.0f;
					++num_weights;
				}
				cursor += sizeof(input_weights->num_weights) +
					(size_t)count * sizeof(ctc_weight_t);
			}
		}
	}

	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.skinwidth = input.skinwidth;
	dmdxheader.skinheight = input.skinheight;
	dmdxheader.framesize = sizeof(daliasxframe_t) +
		(input.num_xyz - 1) * sizeof(dxtrivertx_t);
	dmdxheader.num_meshes = 1;
	dmdxheader.num_skins = input.num_skins;
	dmdxheader.num_xyz = input.num_xyz;
	dmdxheader.num_st = input.num_st;
	dmdxheader.num_tris = input.num_tris;
	dmdxheader.num_glcmds = input.num_tris * 10 + 1;
	dmdxheader.num_frames = input.num_frames;
	dmdxheader.num_animgroup = input.num_animgroup ?
		input.num_animgroup : 1;
	dmdxheader.num_joints = input.num_bones;
	dmdxheader.num_weights = (int)num_weights;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	if (pheader->num_joints > 0)
	{
		dmdx_joint_t *out_joints =
			(dmdx_joint_t *)((byte *)pheader + pheader->ofs_joints);
		int b;

		for (b = 0; b < pheader->num_joints; ++b)
		{
			out_joints[b].name[0] = '\0';
			out_joints[b].parent = temp.bones[b].parent;
		}
	}

	if (pheader->num_weights > 0)
	{
		dmdx_weight_t *out_weights =
			(dmdx_weight_t *)((byte *)pheader + pheader->ofs_weights);
		dmdx_vertex_t *out_mesh_vertices =
			(dmdx_vertex_t *)((byte *)pheader + pheader->ofs_mesh_verteces);
		int b;

		for (b = 0; b < pheader->num_xyz; ++b)
		{
			int k;

			out_mesh_vertices[b].start =
				temp.vertex_weights[b].first;
			out_mesh_vertices[b].count =
				temp.vertex_weights[b].count;

			for (k = 0; k < temp.vertex_weights[b].count; ++k)
			{
				dmdx_weight_t *out_weight =
					out_weights + temp.vertex_weights[b].first + k;
				const ctc_weight_t *in_weight =
					temp.weights + temp.vertex_weights[b].first + k;
				vec3_t local_position;

				out_weight->joint = in_weight->bone;
				out_weight->bias = in_weight->weight;
				Mod_CTCTransformPoint(
					temp.bones[in_weight->bone].base_transform,
					temp.vertices[b].xyz, local_position);
				Mod_CTCConvertPoint(local_position, out_weight->pos);
			}
		}
	}

	if (!Mod_CTCLoadFrames(mod_name, &input, src, pheader, &temp, bones_offset) ||
		!Mod_CTCBuildGeometry(mod_name, &input, src, pheader))
	{
		Mod_CTCFreeTempData(&temp);
		return NULL;
	}

	Mod_LoadCmdGenerate(pheader);
	Mod_LoadFixImages(mod_name, pheader, false);
	Com_DPrintf("%s: loaded %s as %d-frame CTC geometry "
		"(%d animation groups, %d vertices, %d triangles)\n",
		__func__, mod_name, input.num_frames, pheader->num_animgroup,
		input.num_xyz, input.num_tris);
	Mod_CTCFreeTempData(&temp);
	return extradata;
}

