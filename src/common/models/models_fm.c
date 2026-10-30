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
 * Heretic 2 flex model skeleton, converted to skinned joints and weights
 * by fitting rigid bones to the vertex frame animation.
 *
 * Flex models store every frame as baked vertex positions, the skeleton
 * block only has 3 joints for procedural head/torso turning. To get linear
 * blend skinning data (https://en.wikipedia.org/wiki/Skeletal_animation)
 * the vertex animation is decomposed into rigid bones, close to:
 *   D. L. James, C. D. Twigg, "Skinning Mesh Animations", SIGGRAPH 2005;
 *   B. H. Le, Z. Deng, "Smooth Skinning Decomposition with Rigid Bones",
 *   SIGGRAPH Asia 2012.
 *
 * Steps:
 *  1. decode frames, fm joint poses, joint vertex groups and mesh nodes;
 *  2. seed bones: fm joints keep their stored poses, plus one bone per
 *     (mesh node, fm joint) group of vertices;
 *  3. per frame fit a rigid transform for every bone (Horn / Kabsch);
 *  4. reassign each vertex to the bone that predicts it best (Lloyd /
 *     k-means style);
 *  5. split the worst bone in two and repeat 3-4 up to 64 bones
 *     (divisive clustering);
 *  6. store poses and weights, optionally blending a second bone with a
 *     least squares weight.
 *
 * =======================================================================
 */

#include "models.h"
#include "mesh.h"

#define FLEX_MAX_JOINTS 3
#define FLEX_MESH_NODE_SIZE 516
#define FLEX_MAX_MESH_NODES 32
#define FLEX_SKIN_MAX_BONES 64
#define FLEX_SKIN_FRAME_STRIDE 4

typedef struct
{
	int num_joints;
	const char *names[FLEX_MAX_JOINTS];
	int parents[FLEX_MAX_JOINTS];
} flex_skeleton_type_t;

/* indexed by the type stored in the flex "skeleton" block, every type is a chain */
static const flex_skeleton_type_t flex_skeleton_types[] = {
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
	{1, {"center"}, {-1}},
	{2, {"neck", "head"}, {-1, 0}},
	{2, {"neck", "head"}, {-1, 0}},
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
	{3, {"lowerback", "upperback", "head"}, {-1, 0, 1}},
};

/* decoded flex data used as fit input */
typedef struct
{
	int num_xyz;
	int num_frames;
	int num_joints;
	const flex_skeleton_type_t *type;
	vec3_t *verts; /* num_frames * num_xyz */
	dmdx_baseframe_joint_t *poses; /* num_frames * num_joints, world space */
	int *vertex_joint; /* deepest fm joint, -1 for none */
	unsigned *vertex_nodes; /* mesh node bits */
} flex_model_t;

typedef struct
{
	int bone_count;
	/* fm skeleton bones come first and keep their stored poses */
	int fixed_count;
	/* then one bone per mesh node and fm joint, generated bones follow */
	int base_count;
	int *vertex_bone;
	int parents[FLEX_SKIN_MAX_BONES];
	int nodes[FLEX_SKIN_MAX_BONES];
	/* fm joint the bone follows, -1 for none */
	int categories[FLEX_SKIN_MAX_BONES];
	/* mesh node bone that generated bones hang off */
	int anchors[FLEX_SKIN_MAX_BONES];
	vec3_t centers[FLEX_SKIN_MAX_BONES];
	quat_t bind_orients[FLEX_SKIN_MAX_BONES];
	/* num_frames * FLEX_SKIN_MAX_BONES, rotation relative to the bind pose */
	vec3_t *positions;
	quat_t *orients;
	float (*matrices)[9];
} flex_fit_t;

typedef struct
{
	float distance;
	int vertex;
} flex_sort_t;

static void
Mod_FlexQuatIdentity(quat_t q)
{
	q[0] = q[1] = q[2] = 0.0f;
	q[3] = 1.0f;
}

/*
 * flex poses are an origin plus two far points marking the bone direction and up,
 * orthonormalize them with Gram-Schmidt
 * (https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process) and convert the
 * basis to a quaternion (https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion)
 */
static void
Mod_FlexPoseToJoint(const vec3_t origin, const vec3_t direction_point,
	const vec3_t up_point, dmdx_baseframe_joint_t *joint)
{
	vec3_t forward, up, left;
	float m[3][4];
	int i;

	VectorCopy(origin, joint->pos);
	VectorSubtract(direction_point, origin, forward);
	VectorSubtract(up_point, origin, up);

	/* degenerate pose, keep the world axes */
	if (VectorNormalize(forward) == 0.0f)
	{
		Mod_FlexQuatIdentity(joint->orient);
		return;
	}

	VectorMA(up, -DotProduct(up, forward), forward, up);
	if (VectorNormalize(up) == 0.0f)
	{
		Mod_FlexQuatIdentity(joint->orient);
		return;
	}

	/* right handed basis: forward x left = up */
	CrossProduct(up, forward, left);

	/* basis vectors are the matrix columns */
	for (i = 0; i < 3; i++)
	{
		m[i][0] = forward[i];
		m[i][1] = left[i];
		m[i][2] = up[i];
		m[i][3] = 0.0f;
	}

	Mod_Mat3x4ToQuat(m, joint->orient);
	Quat_normalize(joint->orient);
}

static void
Mod_FlexFreeModel(flex_model_t *model)
{
	free(model->verts);
	free(model->poses);
	free(model->vertex_joint);
	free(model->vertex_nodes);
	memset(model, 0, sizeof(*model));
}

/*
 * Decode skeleton, frames and mesh nodes, returns false only on malformed
 * data, num_joints is zero if there is no usable skeleton.
 */
static qboolean
Mod_FlexLoadSkeleton(const char *mod_name, const void *buffer, int modfilelen,
	size_t inframesize, int num_mesh_nodes, flex_model_t *model)
{
	int version, joints, type, counts[FLEX_MAX_JOINTS], i, j, frame, vertex;
	size_t size, count, pos, total = 0;
	const char *src, *frames;
	const int *data, *lists;
	const float *poses;

	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "skeleton", &version, &size);
	if (!src || !size)
	{
		return true;
	}

	if (version != 1)
	{
		Com_Printf("%s: Invalid skeleton version %d\n",
			__func__, version);
		return false;
	}

	count = size / sizeof(int);
	if (count < 2)
	{
		Com_Printf("%s: Invalid skeleton size\n",
			__func__);
		return false;
	}

	data = (const int *)src;
	type = LittleLong(data[0]);
	joints = LittleLong(data[1]);
	Com_DPrintf("%s: %s in skeleton has 0x%02x type and %d bones\n",
		__func__, mod_name, type, joints);

	if (type < 0 || type >= (int)ARRLEN(flex_skeleton_types) ||
		joints != flex_skeleton_types[type].num_joints)
	{
		Com_Printf("%s: %s has unsupported skeleton type %d with %d bones\n",
			__func__, mod_name, type, joints);
		return true;
	}

	if (count < (size_t)(2 + joints))
	{
		Com_Printf("%s: Invalid skeleton size\n",
			__func__);
		return false;
	}

	/* counts and vertex lists are stored from the deepest joint to the root */
	for (i = 0; i < joints; i++)
	{
		int verts;

		verts = LittleLong(data[2 + i]);
		if (verts < 0 || verts > model->num_xyz)
		{
			Com_Printf("%s: Invalid skeleton cluster size\n",
				__func__);
			return false;
		}

		counts[joints - 1 - i] = verts;
		total += verts;
	}

	lists = data + 2 + joints;
	pos = 2 + joints + total;
	if (count < pos + 1)
	{
		Com_Printf("%s: Invalid skeleton size\n",
			__func__);
		return false;
	}

	if (!LittleLong(data[pos]))
	{
		Com_DPrintf("%s: %s skeleton has no joint poses\n",
			__func__, mod_name);
		return true;
	}
	pos++;

	/* origin, direction and up points per frame and joint */
	if ((count - pos) / 9 < (size_t)model->num_frames * joints)
	{
		Com_Printf("%s: Invalid skeleton poses size\n",
			__func__);
		return false;
	}
	poses = (const float *)(data + pos);

	frames = Mod_LoadModel_FlexSection(buffer, modfilelen, "frames", &version, &size);
	if (!frames || version != 1 || size < (size_t)model->num_frames * inframesize)
	{
		Com_Printf("%s: %s has no frames for skeleton\n",
			__func__, mod_name);
		return true;
	}

	model->verts = malloc((size_t)model->num_frames * model->num_xyz * sizeof(*model->verts));
	model->poses = malloc((size_t)model->num_frames * joints * sizeof(*model->poses));
	model->vertex_joint = malloc(model->num_xyz * sizeof(*model->vertex_joint));
	model->vertex_nodes = calloc(model->num_xyz, sizeof(*model->vertex_nodes));
	if (!model->verts || !model->poses || !model->vertex_joint || !model->vertex_nodes)
	{
		Com_Printf("%s: %s out of memory for skeleton\n",
			__func__, mod_name);
		return true;
	}

	/* step 1: decode the vertex animation and the fm joint poses */
	for (frame = 0; frame < model->num_frames; frame++)
	{
		const daliasframe_t *in;
		vec3_t scale, translate;

		in = (const daliasframe_t *)(frames + frame * inframesize);
		for (j = 0; j < 3; j++)
		{
			scale[j] = LittleFloat(in->scale[j]);
			translate[j] = LittleFloat(in->translate[j]);
		}

		for (vertex = 0; vertex < model->num_xyz; vertex++)
		{
			float *out = model->verts[(size_t)frame * model->num_xyz + vertex];

			for (j = 0; j < 3; j++)
			{
				out[j] = in->verts[vertex].v[j] * scale[j] + translate[j];
			}
		}

		/* poses are stored in compressed frame space */
		for (i = 0; i < joints; i++)
		{
			vec3_t points[3];
			int p;

			for (p = 0; p < 3; p++)
			{
				for (j = 0; j < 3; j++)
				{
					points[p][j] = LittleFloat(*poses) * scale[j] + translate[j];
					poses++;
				}
			}

			Mod_FlexPoseToJoint(points[0], points[1], points[2],
				&model->poses[(size_t)frame * joints + i]);
		}
	}

	/* step 1: vertex groups are nested, lowerback contains upperback contains head */
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		model->vertex_joint[vertex] = -1;
	}

	for (i = joints - 1; i >= 0; i--)
	{
		for (j = 0; j < counts[i]; j++)
		{
			vertex = LittleLong(*lists);
			lists++;

			if (vertex < 0 || vertex >= model->num_xyz)
			{
				Com_Printf("%s: %s skeleton references invalid vertex %d\n",
					__func__, mod_name, vertex);
				return true;
			}

			/* a vertex already listed for a deeper joint keeps it */
			if (model->vertex_joint[vertex] < 0)
			{
				model->vertex_joint[vertex] = i;
			}
		}
	}

	/* step 1: mesh nodes are separately drawable parts, bones must not cross them */
	src = Mod_LoadModel_FlexSection(buffer, modfilelen, "mesh nodes", &version, &size);
	if (src && version == 3 && num_mesh_nodes > 0 &&
		size == (size_t)num_mesh_nodes * FLEX_MESH_NODE_SIZE)
	{
		int nodes, max_verts;

		nodes = Q_min(num_mesh_nodes, FLEX_MAX_MESH_NODES);
		max_verts = Q_min(model->num_xyz, 256 * 8);

		for (i = 0; i < nodes; i++)
		{
			/* 256 bytes of tri bits, then 256 bytes of vertex bits */
			const byte *bits = (const byte *)src + i * FLEX_MESH_NODE_SIZE + 256;

			for (vertex = 0; vertex < max_verts; vertex++)
			{
				if ((bits[vertex >> 3] >> (vertex & 7)) & 1)
				{
					model->vertex_nodes[vertex] |= 1u << i;
				}
			}
		}
	}

	model->type = &flex_skeleton_types[type];
	model->num_joints = joints;
	return true;
}

/*
 * cyclic Jacobi eigenvalue algorithm on a symmetric 4x4 matrix, returns the
 * eigenvector of the largest eigenvalue
 * (https://en.wikipedia.org/wiki/Jacobi_eigenvalue_algorithm)
 */
static void
Mod_FlexMaxEigenvector4(double a[4][4], double out[4])
{
	double e[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	int sweep, best = 0, p, q, k, i;

	for (sweep = 0; sweep < 50; sweep++)
	{
		double off = 0.0;

		/* converged when the off-diagonal part is zero */
		for (p = 0; p < 3; p++)
		{
			for (q = p + 1; q < 4; q++)
			{
				off += a[p][q] * a[p][q];
			}
		}

		if (off < 1e-20)
		{
			break;
		}

		for (p = 0; p < 3; p++)
		{
			for (q = p + 1; q < 4; q++)
			{
				double theta, t, c, s;

				if (fabs(a[p][q]) < 1e-30)
				{
					continue;
				}

				/* Givens rotation angle that zeroes a[p][q]
				 * (https://en.wikipedia.org/wiki/Jacobi_rotation) */
				theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
				t = (theta >= 0.0 ? 1.0 : -1.0) / (fabs(theta) + sqrt(theta * theta + 1.0));
				c = 1.0 / sqrt(t * t + 1.0);
				s = t * c;

				/* A' = J^T A J, columns then rows */
				for (k = 0; k < 4; k++)
				{
					const double akp = a[k][p], akq = a[k][q];

					a[k][p] = c * akp - s * akq;
					a[k][q] = s * akp + c * akq;
				}

				for (k = 0; k < 4; k++)
				{
					const double apk = a[p][k], aqk = a[q][k];

					a[p][k] = c * apk - s * aqk;
					a[q][k] = s * apk + c * aqk;
				}

				/* accumulate eigenvectors E' = E J */
				for (k = 0; k < 4; k++)
				{
					const double ekp = e[k][p], ekq = e[k][q];

					e[k][p] = c * ekp - s * ekq;
					e[k][q] = s * ekp + c * ekq;
				}
			}
		}
	}

	/* diagonal now holds the eigenvalues */
	for (i = 1; i < 4; i++)
	{
		if (a[i][i] > a[best][best])
		{
			best = i;
		}
	}

	for (k = 0; k < 4; k++)
	{
		out[k] = e[k][best];
	}
}

/*
 * step 3: rotation that best maps rest offsets onto frame offsets, Horn's
 * closed form with unit quaternions: B. K. P. Horn, "Closed-form solution of
 * absolute orientation using unit quaternions", JOSA A 4(4), 1987.
 * Same problem as https://en.wikipedia.org/wiki/Kabsch_algorithm and
 * https://en.wikipedia.org/wiki/Wahba%27s_problem, s is the 3x3 cross-covariance
 * matrix, the answer is the top eigenvector of the 4x4 matrix N built from it.
 */
static void
Mod_FlexRotationFromCovariance(const double s[9], quat_t q)
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

	Mod_FlexMaxEigenvector4(n, v);
	/* Horn's quaternion is wxyz, ours is xyzw */
	q[0] = (float)v[1];
	q[1] = (float)v[2];
	q[2] = (float)v[3];
	q[3] = (float)v[0];
	Quat_normalize(q);
}

/*
 * step 2: fm joints are fixed bones, their per frame rotation relative to the
 * bind pose is pose * conjugate(bind)
 * (https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation)
 */
static void
Mod_FlexFitInitFixed(const flex_model_t *model, flex_fit_t *fit)
{
	int bone, frame;

	fit->fixed_count = model->num_joints;
	for (bone = 0; bone < fit->fixed_count; bone++)
	{
		const dmdx_baseframe_joint_t *bind = &model->poses[bone];
		quat_t inverse;

		fit->parents[bone] = model->type->parents[bone];
		fit->nodes[bone] = -1;
		fit->categories[bone] = bone;
		fit->anchors[bone] = bone;
		VectorCopy(bind->pos, fit->centers[bone]);
		memcpy(fit->bind_orients[bone], bind->orient, sizeof(quat_t));
		inverse[0] = -bind->orient[0];
		inverse[1] = -bind->orient[1];
		inverse[2] = -bind->orient[2];
		inverse[3] = bind->orient[3];

		for (frame = 0; frame < model->num_frames; frame++)
		{
			const dmdx_baseframe_joint_t *pose =
				&model->poses[(size_t)frame * model->num_joints + bone];
			const size_t index = (size_t)frame * FLEX_SKIN_MAX_BONES + bone;

			VectorCopy(pose->pos, fit->positions[index]);
			QuatMultiply(pose->orient, inverse, fit->orients[index]);
			Quat_normalize(fit->orients[index]);
			Quat_toMat3(fit->orients[index], fit->matrices[index]);
		}
	}
}

/*
 * step 3: per frame rigid fit of every generated bone to its vertices,
 * the best translation maps the rest centroid to the frame centroid
 * (https://en.wikipedia.org/wiki/Centroid), rotation comes from the
 * cross-covariance (https://en.wikipedia.org/wiki/Cross-covariance_matrix)
 */
static void
Mod_FlexFitBones(const flex_model_t *model, flex_fit_t *fit)
{
	const vec3_t *rest = model->verts;
	int counts[FLEX_SKIN_MAX_BONES];
	vec3_t sums[FLEX_SKIN_MAX_BONES];
	int bone, vertex, frame, i, j;

	/* rest centroid of each bone */
	memset(counts, 0, sizeof(counts));
	memset(sums, 0, sizeof(sums));
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		bone = fit->vertex_bone[vertex];
		counts[bone]++;
		VectorAdd(sums[bone], rest[vertex], sums[bone]);
	}

	/* an empty generated bone keeps its last center */
	for (bone = fit->fixed_count; bone < fit->bone_count; bone++)
	{
		if (counts[bone])
		{
			VectorScale(sums[bone], 1.0f / counts[bone], fit->centers[bone]);
		}
	}

	for (frame = 0; frame < model->num_frames; frame++)
	{
		const vec3_t *current = model->verts + (size_t)frame * model->num_xyz;
		double covariance[FLEX_SKIN_MAX_BONES][9];
		vec3_t targets[FLEX_SKIN_MAX_BONES];

		memset(covariance, 0, sizeof(covariance));
		memset(targets, 0, sizeof(targets));
		/* frame centroid of each bone */
		for (vertex = 0; vertex < model->num_xyz; vertex++)
		{
			bone = fit->vertex_bone[vertex];
			VectorAdd(targets[bone], current[vertex], targets[bone]);
		}

		for (bone = fit->fixed_count; bone < fit->bone_count; bone++)
		{
			if (counts[bone])
			{
				VectorScale(targets[bone], 1.0f / counts[bone], targets[bone]);
			}
			else
			{
				VectorCopy(fit->centers[bone], targets[bone]);
			}
		}

		/* S = sum (rest - rest centroid) * (frame - frame centroid)^T */
		for (vertex = 0; vertex < model->num_xyz; vertex++)
		{
			vec3_t a, b;

			bone = fit->vertex_bone[vertex];
			if (bone < fit->fixed_count)
			{
				continue;
			}

			VectorSubtract(rest[vertex], fit->centers[bone], a);
			VectorSubtract(current[vertex], targets[bone], b);
			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					covariance[bone][i * 3 + j] += (double)a[i] * b[j];
				}
			}
		}

		for (bone = fit->fixed_count; bone < fit->bone_count; bone++)
		{
			const size_t index = (size_t)frame * FLEX_SKIN_MAX_BONES + bone;

			VectorCopy(targets[bone], fit->positions[index]);
			Mod_FlexRotationFromCovariance(covariance[bone], fit->orients[index]);
			Quat_toMat3(fit->orients[index], fit->matrices[index]);
		}
	}
}

/* rigid bone transform of a rest vertex: frame position + R * (rest - center) */
static void
Mod_FlexPredict(const flex_model_t *model, const flex_fit_t *fit,
	int vertex, int bone, int frame, vec3_t out)
{
	const size_t index = (size_t)frame * FLEX_SKIN_MAX_BONES + bone;
	const float *m = fit->matrices[index];
	const float *pos = fit->positions[index];
	vec3_t local;

	VectorSubtract(model->verts[vertex], fit->centers[bone], local);
	out[0] = pos[0] + m[0] * local[0] + m[1] * local[1] + m[2] * local[2];
	out[1] = pos[1] + m[3] * local[0] + m[4] * local[1] + m[5] * local[2];
	out[2] = pos[2] + m[6] * local[0] + m[7] * local[1] + m[8] * local[2];
}

/* sum of squared distances to the real frames, every 4th frame to save time */
static double
Mod_FlexVertexError(const flex_model_t *model, const flex_fit_t *fit, int vertex, int bone)
{
	double error = 0.0;
	int frame;

	for (frame = 0; frame < model->num_frames; frame += FLEX_SKIN_FRAME_STRIDE)
	{
		vec3_t predicted, delta;

		Mod_FlexPredict(model, fit, vertex, bone, frame, predicted);
		VectorSubtract(predicted,
			model->verts[(size_t)frame * model->num_xyz + vertex], delta);
		error += DotProduct(delta, delta);
	}

	return error;
}

/* keep the fm rotation chain of every vertex and stay inside its mesh nodes */
static qboolean
Mod_FlexBoneAllowed(const flex_model_t *model, const flex_fit_t *fit, int vertex, int bone)
{
	if (fit->categories[bone] != model->vertex_joint[vertex])
	{
		return false;
	}

	return bone < fit->fixed_count || !model->vertex_nodes[vertex] ||
		((model->vertex_nodes[vertex] >> fit->nodes[bone]) & 1);
}

/* drop empty generated bones, fm and mesh node bones stay as hierarchy anchors */
static void
Mod_FlexCompact(const flex_model_t *model, flex_fit_t *fit)
{
	int remap[FLEX_SKIN_MAX_BONES], used[FLEX_SKIN_MAX_BONES];
	int count = fit->base_count, bone, vertex;

	memset(used, 0, sizeof(used));
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		used[fit->vertex_bone[vertex]] = 1;
	}

	for (bone = 0; bone < fit->base_count; bone++)
	{
		remap[bone] = bone;
	}

	for (bone = fit->base_count; bone < fit->bone_count; bone++)
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

	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		fit->vertex_bone[vertex] = remap[fit->vertex_bone[vertex]];
	}

	fit->bone_count = count;
}

/*
 * step 4: assignment step of Lloyd's algorithm
 * (https://en.wikipedia.org/wiki/K-means_clustering#Standard_algorithm_(naive_k-means)),
 * the "distance" to a bone is how badly its rigid motion predicts the vertex
 */
static void
Mod_FlexAssign(const flex_model_t *model, flex_fit_t *fit)
{
	int vertex, bone;

	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		int best = fit->vertex_bone[vertex];
		double best_error = Mod_FlexVertexError(model, fit, vertex, best);

		for (bone = 0; bone < fit->bone_count; bone++)
		{
			double error;

			if (bone == best || !Mod_FlexBoneAllowed(model, fit, vertex, bone))
			{
				continue;
			}

			error = Mod_FlexVertexError(model, fit, vertex, bone);
			if (error < best_error)
			{
				best_error = error;
				best = bone;
			}
		}

		fit->vertex_bone[vertex] = best;
	}

	Mod_FlexCompact(model, fit);
}

static int
Mod_FlexSortCompare(const void *a, const void *b)
{
	const float da = ((const flex_sort_t *)a)->distance;
	const float db = ((const flex_sort_t *)b)->distance;

	return (da > db) - (da < db);
}

/*
 * step 5: split the bone with the largest error around its worst fitting vertex,
 * the half nearest to it gets a new bone (divisive clustering,
 * https://en.wikipedia.org/wiki/Hierarchical_clustering)
 */
static qboolean
Mod_FlexSplitWorst(const flex_model_t *model, flex_fit_t *fit, flex_sort_t *items)
{
	double bone_error[FLEX_SKIN_MAX_BONES], worst_error[FLEX_SKIN_MAX_BONES];
	int worst_vertex[FLEX_SKIN_MAX_BONES], counts[FLEX_SKIN_MAX_BONES];
	int bone, vertex, i, split = -1, count = 0;

	memset(bone_error, 0, sizeof(bone_error));
	memset(worst_error, 0, sizeof(worst_error));
	memset(worst_vertex, 0, sizeof(worst_vertex));
	memset(counts, 0, sizeof(counts));

	/* total error and worst vertex of every bone */
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		double error;

		bone = fit->vertex_bone[vertex];
		error = Mod_FlexVertexError(model, fit, vertex, bone);
		bone_error[bone] += error;
		if (!counts[bone]++ || error > worst_error[bone])
		{
			worst_error[bone] = error;
			worst_vertex[bone] = vertex;
		}
	}

	/* fixed fm bones are never split, tiny bones are not worth it */
	for (bone = fit->fixed_count; bone < fit->bone_count; bone++)
	{
		if (counts[bone] >= 4 && (split < 0 || bone_error[bone] > bone_error[split]))
		{
			split = bone;
		}
	}

	if (split < 0)
	{
		return false;
	}

	/* sort by rest distance to the worst vertex */
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		vec3_t delta;

		if (fit->vertex_bone[vertex] != split)
		{
			continue;
		}

		VectorSubtract(model->verts[vertex], model->verts[worst_vertex[split]], delta);
		items[count].distance = VectorLength(delta);
		items[count].vertex = vertex;
		count++;
	}

	qsort(items, count, sizeof(*items), Mod_FlexSortCompare);
	for (i = 0; i < count / 2; i++)
	{
		fit->vertex_bone[items[i].vertex] = fit->bone_count;
	}

	/* generated bones hang off the bone of their mesh node */
	bone = fit->bone_count;
	fit->nodes[bone] = fit->nodes[split];
	fit->categories[bone] = fit->categories[split];
	fit->anchors[bone] = fit->anchors[split];
	fit->parents[bone] = fit->anchors[split];
	VectorCopy(fit->centers[split], fit->centers[bone]);
	Mod_FlexQuatIdentity(fit->bind_orients[bone]);
	fit->bone_count++;

	return true;
}

/* weight offsets are in the bone's bind space, as md5 weights are */
static void
Mod_FlexWeightPosition(const flex_model_t *model, const flex_fit_t *fit,
	int vertex, int bone, vec3_t out)
{
	const float *bind = fit->bind_orients[bone];
	quat_t inverse;
	vec3_t local;

	inverse[0] = -bind[0];
	inverse[1] = -bind[1];
	inverse[2] = -bind[2];
	inverse[3] = bind[3];
	VectorSubtract(model->verts[vertex], fit->centers[bone], local);
	Quat_rotatePoint(inverse, local, out);
}

/*
 * step 6: best blend factor and error of a second bone on top of the primary
 * one, minimizes sum |a + w (b - a) - v|^2 over frames, so w is the projection
 * of (v - a) onto (b - a) (https://en.wikipedia.org/wiki/Linear_least_squares)
 */
static double
Mod_FlexBlendError(const flex_model_t *model, const flex_fit_t *fit,
	int vertex, int primary, int bone, float *blend)
{
	double numerator = 0.0, denominator = 0.0, error = 0.0;
	int frame, i;

	for (frame = 0; frame < model->num_frames; frame += FLEX_SKIN_FRAME_STRIDE)
	{
		vec3_t a, b, d, r;

		Mod_FlexPredict(model, fit, vertex, primary, frame, a);
		Mod_FlexPredict(model, fit, vertex, bone, frame, b);
		VectorSubtract(b, a, d);
		VectorSubtract(model->verts[(size_t)frame * model->num_xyz + vertex], a, r);
		numerator += DotProduct(r, d);
		denominator += DotProduct(d, d);
	}

	if (denominator <= 0.0)
	{
		return -1.0;
	}

	*blend = (float)(numerator / denominator);
	if (*blend <= 0.01f)
	{
		return -1.0;
	}

	/* weights are convex, a blend of 1 is just the other bone */
	if (*blend > 1.0f)
	{
		*blend = 1.0f;
	}

	/* error of the blended prediction */
	for (frame = 0; frame < model->num_frames; frame += FLEX_SKIN_FRAME_STRIDE)
	{
		vec3_t a, b, p, d;

		Mod_FlexPredict(model, fit, vertex, primary, frame, a);
		Mod_FlexPredict(model, fit, vertex, bone, frame, b);
		for (i = 0; i < 3; i++)
		{
			p[i] = a[i] + *blend * (b[i] - a[i]);
		}

		VectorSubtract(p, model->verts[(size_t)frame * model->num_xyz + vertex], d);
		error += DotProduct(d, d);
	}

	return error;
}

/* step 6: write joints, per frame world poses and weights in dmdx layout */
static void
Mod_FlexStore(const flex_model_t *model, const flex_fit_t *fit, flex_skin_t *skin)
{
	const int bones = fit->bone_count;
	int bone, frame, vertex, weight = 0, blended = 0;

	for (bone = 0; bone < bones; bone++)
	{
		dmdx_joint_t *joint = &skin->joints[bone];
		const int category = fit->categories[bone];
		const char *category_name;

		category_name = category >= 0 ? model->type->names[category] : "root";
		if (bone < fit->fixed_count)
		{
			Q_strlcpy(joint->name, model->type->names[bone], sizeof(joint->name));
		}
		else if (bone < fit->base_count)
		{
			snprintf(joint->name, sizeof(joint->name), "node%02d_%s",
				fit->nodes[bone], category_name);
		}
		else
		{
			snprintf(joint->name, sizeof(joint->name), "node%02d_%s_%02d",
				fit->nodes[bone], category_name, bone);
		}

		joint->parent = fit->parents[bone];
		VectorCopy(fit->centers[bone], joint->pos);
		memcpy(joint->orient, fit->bind_orients[bone], sizeof(joint->orient));

		/* world orientation = fitted rotation * bind orientation */
		for (frame = 0; frame < model->num_frames; frame++)
		{
			const size_t index = (size_t)frame * FLEX_SKIN_MAX_BONES + bone;
			dmdx_baseframe_joint_t *pose = &skin->poses[(size_t)frame * bones + bone];

			VectorCopy(fit->positions[index], pose->pos);
			QuatMultiply(fit->orients[index], fit->bind_orients[bone], pose->orient);
			Quat_normalize(pose->orient);
		}
	}

	/* each vertex keeps its rigid bone, optionally blended with a second one */
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		const int primary = fit->vertex_bone[vertex];
		const double primary_error = Mod_FlexVertexError(model, fit, vertex, primary);
		dmdx_vertex_t *bind = &skin->verts[vertex];
		double best_error = primary_error;
		float best_blend = 0.0f;
		int secondary = -1;
		dmdx_weight_t *out;

		for (bone = 0; bone < bones; bone++)
		{
			double error;
			float blend;

			if (bone == primary || !Mod_FlexBoneAllowed(model, fit, vertex, bone))
			{
				continue;
			}

			error = Mod_FlexBlendError(model, fit, vertex, primary, bone, &blend);
			if (error >= 0.0 && error < best_error)
			{
				best_error = error;
				best_blend = blend;
				secondary = bone;
			}
		}

		bind->st[0] = bind->st[1] = 0.0f;
		bind->start = weight;
		bind->count = 0;
		/* keep the blend only if it cuts the error by at least 10% */
		if (secondary >= 0 && best_error < primary_error * 0.9)
		{
			out = &skin->weights[weight++];
			out->joint = secondary;
			out->bias = best_blend;
			Mod_FlexWeightPosition(model, fit, vertex, secondary, out->pos);
			bind->count++;
			blended++;
		}
		else
		{
			best_blend = 0.0f;
		}

		out = &skin->weights[weight++];
		out->joint = primary;
		out->bias = 1.0f - best_blend;
		Mod_FlexWeightPosition(model, fit, vertex, primary, out->pos);
		bind->count++;
	}

	skin->num_joints = bones;
	skin->num_weights = weight;

	Com_DPrintf("%s: %d bones (%d fm, %d mesh nodes, %d generated), "
		"%d weights, %d vertices blended\n",
		__func__, bones, fit->fixed_count, fit->base_count - fit->fixed_count,
		bones - fit->base_count, weight, blended);
}

/*
 * same math as the renderer skeletal path, prints root mean square
 * (https://en.wikipedia.org/wiki/Root_mean_square) and max error
 */
static void
Mod_FlexReport(const char *mod_name, const flex_model_t *model, const flex_skin_t *skin)
{
	float rotations[FLEX_SKIN_MAX_BONES][9];
	double sum = 0.0;
	float max_error = 0.0f;
	int frame, vertex, bone, k;

	for (frame = 0; frame < model->num_frames; frame++)
	{
		const dmdx_baseframe_joint_t *poses = skin->poses + (size_t)frame * skin->num_joints;

		for (bone = 0; bone < skin->num_joints; bone++)
		{
			Quat_toMat3(poses[bone].orient, rotations[bone]);
		}

		for (vertex = 0; vertex < model->num_xyz; vertex++)
		{
			const dmdx_vertex_t *bind = &skin->verts[vertex];
			vec3_t result = {0.0f, 0.0f, 0.0f}, delta;
			float error;

			for (k = 0; k < bind->count; k++)
			{
				const dmdx_weight_t *weight = &skin->weights[bind->start + k];
				const float *r = rotations[weight->joint];
				const float *p = poses[weight->joint].pos;
				const float *w = weight->pos;

				result[0] += (p[0] + r[0] * w[0] + r[1] * w[1] + r[2] * w[2]) * weight->bias;
				result[1] += (p[1] + r[3] * w[0] + r[4] * w[1] + r[5] * w[2]) * weight->bias;
				result[2] += (p[2] + r[6] * w[0] + r[7] * w[1] + r[8] * w[2]) * weight->bias;
			}

			VectorSubtract(result,
				model->verts[(size_t)frame * model->num_xyz + vertex], delta);
			error = VectorLength(delta);
			sum += (double)error * error;
			if (error > max_error)
			{
				max_error = error;
			}
		}
	}

	Com_DPrintf("%s: %s skin error vs vertex frames: rms %.4f, max %.4f\n",
		__func__, mod_name,
		sqrt(sum / ((double)model->num_frames * model->num_xyz)), max_error);
}

/* fm frames carry no weights, so fit rigid bones to the baked vertex animation */
static qboolean
Mod_FlexConvert(const char *mod_name, const flex_model_t *model, flex_skin_t *skin)
{
	const size_t slots = (size_t)model->num_frames * FLEX_SKIN_MAX_BONES;
	flex_sort_t *items;
	qboolean ok = false;
	flex_fit_t fit;
	int vertex, bone;

	memset(&fit, 0, sizeof(fit));
	fit.vertex_bone = malloc(model->num_xyz * sizeof(*fit.vertex_bone));
	fit.positions = malloc(slots * sizeof(*fit.positions));
	fit.orients = malloc(slots * sizeof(*fit.orients));
	fit.matrices = malloc(slots * sizeof(*fit.matrices));
	items = malloc(model->num_xyz * sizeof(*items));
	if (!fit.vertex_bone || !fit.positions || !fit.orients || !fit.matrices || !items)
	{
		Com_Printf("%s: %s out of memory\n", __func__, mod_name);
		goto done;
	}

	Mod_FlexFitInitFixed(model, &fit);
	fit.bone_count = fit.fixed_count;

	/* step 2: one bone per mesh node and fm joint, parented to that fm joint or the root */
	for (vertex = 0; vertex < model->num_xyz; vertex++)
	{
		const int category = model->vertex_joint[vertex];
		const unsigned nodes = model->vertex_nodes[vertex];
		int first = 0;

		/* lowest mesh node the vertex belongs to */
		while (nodes && !((nodes >> first) & 1))
		{
			first++;
		}

		for (bone = fit.fixed_count; bone < fit.bone_count; bone++)
		{
			if (fit.nodes[bone] == first && fit.categories[bone] == category)
			{
				break;
			}
		}

		if (bone == fit.bone_count)
		{
			if (fit.bone_count == FLEX_SKIN_MAX_BONES)
			{
				Com_Printf("%s: %s needs too many bones\n", __func__, mod_name);
				goto done;
			}

			fit.nodes[bone] = first;
			fit.categories[bone] = category;
			fit.anchors[bone] = bone;
			fit.parents[bone] = category;
			Mod_FlexQuatIdentity(fit.bind_orients[bone]);
			fit.bone_count++;
		}

		fit.vertex_bone[vertex] = bone;
	}
	fit.base_count = fit.bone_count;

	/* steps 3-4 on the seed bones, fit again after vertices moved */
	Mod_FlexFitBones(model, &fit);
	Mod_FlexAssign(model, &fit);
	Mod_FlexFitBones(model, &fit);
	/* step 5: grow bones until the limit or nothing is left to split */
	while (fit.bone_count < FLEX_SKIN_MAX_BONES && Mod_FlexSplitWorst(model, &fit, items))
	{
		Mod_FlexFitBones(model, &fit);
		Mod_FlexAssign(model, &fit);
		Mod_FlexFitBones(model, &fit);
	}

	skin->joints = calloc(fit.bone_count, sizeof(*skin->joints));
	skin->poses = calloc((size_t)model->num_frames * fit.bone_count, sizeof(*skin->poses));
	skin->weights = calloc((size_t)model->num_xyz * 2, sizeof(*skin->weights));
	skin->verts = calloc(model->num_xyz, sizeof(*skin->verts));
	if (!skin->joints || !skin->poses || !skin->weights || !skin->verts)
	{
		Com_Printf("%s: %s out of memory\n", __func__, mod_name);
		goto done;
	}

	Mod_FlexStore(model, &fit, skin);
	Mod_FlexReport(mod_name, model, skin);
	ok = true;

done:
	free(fit.vertex_bone);
	free(fit.positions);
	free(fit.orients);
	free(fit.matrices);
	free(items);

	return ok;
}

/* fm joints only, without weights */
static qboolean
Mod_FlexStoreSkeleton(const flex_model_t *model, flex_skin_t *skin)
{
	int i;

	skin->joints = calloc(model->num_joints, sizeof(*skin->joints));
	skin->poses = malloc((size_t)model->num_frames * model->num_joints * sizeof(*skin->poses));
	if (!skin->joints || !skin->poses)
	{
		return false;
	}

	memcpy(skin->poses, model->poses,
		(size_t)model->num_frames * model->num_joints * sizeof(*skin->poses));

	/* bind pose is the first frame */
	for (i = 0; i < model->num_joints; i++)
	{
		Q_strlcpy(skin->joints[i].name, model->type->names[i], sizeof(skin->joints[i].name));
		skin->joints[i].parent = model->type->parents[i];
		VectorCopy(model->poses[i].pos, skin->joints[i].pos);
		memcpy(skin->joints[i].orient, model->poses[i].orient, sizeof(skin->joints[i].orient));
	}

	skin->num_joints = model->num_joints;
	return true;
}

void
Mod_FreeFlexSkin(flex_skin_t *skin)
{
	free(skin->joints);
	free(skin->poses);
	free(skin->weights);
	free(skin->verts);
	memset(skin, 0, sizeof(*skin));
}

qboolean
Mod_LoadFlexSkin(const char *mod_name, const void *buffer, int modfilelen,
	int num_xyz, int num_frames, int num_mesh_nodes, size_t inframesize,
	flex_skin_t *skin)
{
	flex_model_t model;

	memset(skin, 0, sizeof(*skin));
	memset(&model, 0, sizeof(model));
	model.num_xyz = num_xyz;
	model.num_frames = num_frames;

	if (!Mod_FlexLoadSkeleton(mod_name, buffer, modfilelen, inframesize,
		num_mesh_nodes, &model))
	{
		Mod_FlexFreeModel(&model);
		return false;
	}

	if (model.num_joints > 0 && !Mod_FlexConvert(mod_name, &model, skin))
	{
		Mod_FreeFlexSkin(skin);
		if (!Mod_FlexStoreSkeleton(&model, skin))
		{
			Mod_FreeFlexSkin(skin);
		}
	}

	Mod_FlexFreeModel(&model);
	return true;
}

void
Mod_StoreFlexSkin(dmdx_t *pheader, const flex_skin_t *skin)
{
	if (pheader->num_joints != skin->num_joints ||
		pheader->num_weights != skin->num_weights ||
		skin->num_joints <= 0)
	{
		return;
	}

	memcpy((byte *)pheader + pheader->ofs_joints, skin->joints,
		skin->num_joints * sizeof(*skin->joints));
	memcpy((byte *)pheader + pheader->ofs_baseframe_joints, skin->poses,
		(size_t)pheader->num_frames * skin->num_joints * sizeof(*skin->poses));

	if (skin->num_weights > 0)
	{
		memcpy((byte *)pheader + pheader->ofs_weights, skin->weights,
			skin->num_weights * sizeof(*skin->weights));
		memcpy((byte *)pheader + pheader->ofs_mesh_verteces, skin->verts,
			pheader->num_xyz * sizeof(*skin->verts));
	}
}
