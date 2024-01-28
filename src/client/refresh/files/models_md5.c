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
 * The MD5 models file format
 *
 * =======================================================================
 */

#include "../ref_shared.h"

/* Joint */
typedef struct md5_joint_s
{
	char name[64];
	int parent;

	vec3_t pos;
	vec4_t orient;
} md5_joint_t;

/* Vertex */
typedef struct md5_vertex_s
{
	vec2_t st;

	int start; /* start weight */
	int count; /* weight count */
} md5_vertex_t;

/* Triangle */
typedef struct md5_triangle_s
{
	int index[3];
} md5_triangle_t;

/* Weight */
typedef struct md5_weight_s
{
	int joint;
	float bias;

	vec3_t pos;
} md5_weight_t;

/* Bounding box */
typedef struct md5_bbox_s
{
	vec3_t min;
	vec3_t max;
} md5_bbox_t;

/* MD5 mesh */
typedef struct md5_mesh_s
{
	md5_vertex_t *vertices;
	md5_triangle_t *triangles;
	md5_weight_t *weights;

	int num_verts;
	int num_tris;
	int num_weights;

	char shader[256];
} md5_mesh_t;

typedef struct md5_fvert_s
{
	vec3_t xyz;
	vec3_t norm;
} md5_fvert_t;

typedef struct md5_frame_s
{
	md5_bbox_t bbox;
	md5_joint_t *skelJoints;
	md5_fvert_t *vertexArray;
} md5_frame_t;

/* MD5 model structure */
typedef struct md5_model_s
{
	md5_joint_t *baseSkel;
	md5_mesh_t *meshes;
	md5_frame_t *skelFrames;
	int *vertexIndices;
	vec2_t *st;

	int num_frames;
	int num_joints;
	int num_meshes;
	int num_verts;
	int num_tris;
	int frameRate;
} md5_model_t;

/* Joint info */
typedef struct md5_joint_info_s
{
	char name[64];
	int parent;
	int flags;
	int startIndex;
} md5_joint_info_t;

/* Base frame joint */
typedef struct md5_baseframe_joint_s
{
	vec3_t pos;
	vec4_t orient;
} md5_baseframe_joint_t;

/**
 * Basic quaternion operations.
 */
static void
Quat_computeW(vec4_t q)
{
	float t = 1.0f - (q[0] * q[0]) - (q[1] * q[1]) - (q[2] * q[2]);

	if (t < 0.0f)
	{
		q[3] = 0.0f;
	}
	else
	{
		q[3] = -sqrt (t);
	}
}

static void
Quat_normalize(vec4_t q)
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
Quat_multQuat(const vec4_t qa, const vec4_t qb, vec4_t out)
{
	out[3] = (qa[3] * qb[3]) - (qa[0] * qb[0]) - (qa[1] * qb[1]) - (qa[2] * qb[2]);
	out[0] = (qa[0] * qb[3]) + (qa[3] * qb[0]) + (qa[1] * qb[2]) - (qa[2] * qb[1]);
	out[1] = (qa[1] * qb[3]) + (qa[3] * qb[1]) + (qa[2] * qb[0]) - (qa[0] * qb[2]);
	out[2] = (qa[2] * qb[3]) + (qa[3] * qb[2]) + (qa[0] * qb[1]) - (qa[1] * qb[0]);
}

static void
Quat_multVec(const vec4_t q, const vec3_t v, vec4_t out)
{
	out[3] = - (q[0] * v[0]) - (q[1] * v[1]) - (q[2] * v[2]);
	out[0] =   (q[3] * v[0]) + (q[1] * v[2]) - (q[2] * v[1]);
	out[1] =   (q[3] * v[1]) + (q[2] * v[0]) - (q[0] * v[2]);
	out[2] =   (q[3] * v[2]) + (q[0] * v[1]) - (q[1] * v[0]);
}

static void
Quat_rotatePoint(const vec4_t q, const vec3_t in, vec3_t out)
{
	vec4_t tmp, inv, final;

	inv[0] = -q[0]; inv[1] = -q[1];
	inv[2] = -q[2]; inv[3] =  q[3];

	Quat_normalize(inv);

	Quat_multVec(q, in, tmp);
	Quat_multQuat(tmp, inv, final);

	out[0] = final[0];
	out[1] = final[1];
	out[2] = final[2];
}

/**
 * Build skeleton for a given frame data.
 */
static void
BuildFrameSkeleton(const md5_joint_info_t *jointInfos,
	const md5_baseframe_joint_t *baseFrame,
	const float *animFrameData,
	md5_joint_t *skelFrame,
	int num_joints)
{
	int i;

	for (i = 0; i < num_joints; ++i)
	{
		const md5_baseframe_joint_t *baseJoint = &baseFrame[i];
		vec3_t animatedPos;
		vec4_t animatedOrient;
		int j = 0;

		memcpy(animatedPos, baseJoint->pos, sizeof(vec3_t));
		memcpy(animatedOrient, baseJoint->orient, sizeof(vec4_t));

		if (jointInfos[i].flags & 1) /* Tx */
		{
			animatedPos[0] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 2) /* Ty */
		{
			animatedPos[1] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 4) /* Tz */
		{
			animatedPos[2] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 8) /* Qx */
		{
			animatedOrient[0] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 16) /* Qy */
		{
			animatedOrient[1] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 32) /* Qz */
		{
			animatedOrient[2] = animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		/* Compute orient quaternion's w value */
		Quat_computeW(animatedOrient);

		/* NOTE: we assume that this joint's parent has
		 * already been calculated, i.e. joint's ID should
		 * never be smaller than its parent ID. */
		md5_joint_t *thisJoint = &skelFrame[i];

		int parent = jointInfos[i].parent;
		thisJoint->parent = parent;
		strcpy(thisJoint->name, jointInfos[i].name);

		/* Has parent? */
		if (thisJoint->parent < 0)
		{
			memcpy(thisJoint->pos, animatedPos, sizeof(vec3_t));
			memcpy(thisJoint->orient, animatedOrient, sizeof(vec4_t));
		}
		else
		{
			md5_joint_t *parentJoint = &skelFrame[parent];
			vec3_t rpos; /* Rotated position */

			/* Add positions */
			Quat_rotatePoint (parentJoint->orient, animatedPos, rpos);
			thisJoint->pos[0] = rpos[0] + parentJoint->pos[0];
			thisJoint->pos[1] = rpos[1] + parentJoint->pos[1];
			thisJoint->pos[2] = rpos[2] + parentJoint->pos[2];

			/* Concatenate rotations */
			Quat_multQuat(parentJoint->orient, animatedOrient, thisJoint->orient);
			Quat_normalize(thisJoint->orient);
		}
	}
}

static void
AllocateFrames(md5_model_t *anim)
{
	int i;

	anim->num_verts = 0;
	anim->num_tris = 0;

	for (i = 0; i < anim->num_meshes; ++i)
	{
		anim->num_verts += anim->meshes[i].num_verts;
		anim->num_tris += anim->meshes[i].num_tris;
	}

	R_Printf(PRINT_DEVELOPER, "mesh num tris %d / num vert %d\n",
		anim->num_tris, anim->num_verts);

	anim->vertexIndices = (int *)
		malloc(sizeof(int) * anim->num_tris * 3);
	anim->st = (vec2_t *)
		malloc(sizeof(vec2_t) * anim->num_tris * 3);

	for (i = 0; i < anim->num_frames; ++i)
	{
		memset(&(anim->skelFrames[i].bbox), 0, sizeof(md5_bbox_t));

		/* Allocate memory for joints of each frame */
		anim->skelFrames[i].skelJoints = (md5_joint_t *)
			malloc(sizeof(md5_joint_t) * anim->num_joints);
		anim->skelFrames[i].vertexArray = (md5_fvert_t *)
			malloc(sizeof(md5_fvert_t) * anim->num_verts);
		memcpy(anim->skelFrames[i].skelJoints, anim->baseSkel,
			sizeof(md5_joint_t) * anim->num_joints);
	}
}

static const char *
get_line(char *buff, const char *curr_buff, qboolean newline)
{
	const char *startline, *endline;

	curr_buff += strspn(curr_buff, " \t\n");

	startline = curr_buff;
	if (newline)
	{
		endline = strchr(curr_buff, '\n');
	}
	else
	{
		endline = curr_buff + strcspn(curr_buff, " \t\n");
	}

	if (endline)
	{
		curr_buff = endline + 1;
	}
	else
	{
		int len;

		len = strlen(curr_buff);
		endline = curr_buff + len;

		curr_buff += len;
	}

	memcpy(buff, startline, endline - startline);
	buff[endline - startline] = 0;

	return curr_buff;
}

/**
 * Load an MD5 animation from file.
 */
static void
ReadMD5Anim(md5_model_t *anim, const char *buffer, size_t size)
{
	const char *curr_buff, *end_buff;
	char *safe_buffer;
	char buff[512];
	md5_joint_info_t *jointInfos = NULL;
	md5_baseframe_joint_t *baseFrame = NULL;
	float *animFrameData = NULL;
	int version;
	int numAnimatedComponents;
	int frame_index;
	int i;

	/* buffer has not always had final zero */
	safe_buffer = malloc(size + 1);
	memcpy(safe_buffer, buffer, size);
	safe_buffer[size] = 0;

	curr_buff = safe_buffer;
	end_buff = safe_buffer + size;

	while (curr_buff < end_buff)
	{
		curr_buff = get_line(buff, curr_buff, true);

		if (sscanf(buff, "MD5Version %d", &version) == 1)
		{
			if (version != 10)
				{
					/* Bad version */
					R_Printf(PRINT_ALL, "Error: bad animation version\n");
					free(safe_buffer);

					return;
				}
		}
		else if (sscanf(buff, "numFrames %d", &anim->num_frames) == 1)
		{
			/* Allocate memory for skeleton frames and bounding boxes */
			if (anim->num_frames > 0)
			{
				anim->skelFrames = (md5_frame_t *)
					malloc(sizeof(md5_frame_t) * anim->num_frames);
			}
		}
		else if (sscanf(buff, "numJoints %d", &anim->num_joints) == 1)
		{
			if (anim->num_joints > 0)
			{
				AllocateFrames(anim);

				/* Allocate temporary memory for building skeleton frames */
				jointInfos = (md5_joint_info_t *)
					malloc(sizeof(md5_joint_info_t) * anim->num_joints);

				baseFrame = (md5_baseframe_joint_t *)
					malloc(sizeof(md5_baseframe_joint_t) * anim->num_joints);
			}
		}
		else if (sscanf(buff, "frameRate %d", &anim->frameRate) == 1)
		{
			R_Printf(PRINT_DEVELOPER, "md5anim: animation's frame rate is %d\n", anim->frameRate);
		}
		else if (sscanf(buff, "numAnimatedComponents %d", &numAnimatedComponents) == 1)
		{
			if (numAnimatedComponents > 0)
			{
				/* Allocate memory for animation frame data */
				animFrameData = (float *)malloc(sizeof(float) * numAnimatedComponents);
			}
		}
		else if (strncmp (buff, "hierarchy {", 11) == 0)
		{
			for (i = 0; i < anim->num_joints; ++i)
			{
				curr_buff = get_line(buff, curr_buff, true);

				/* Read joint info */
				sscanf(buff, "%s %d %d %d", jointInfos[i].name, &jointInfos[i].parent,
					&jointInfos[i].flags, &jointInfos[i].startIndex);
			}
		}
		else if (strncmp (buff, "bounds {", 8) == 0)
		{
			for (i = 0; i < anim->num_frames; ++i)
			{
				curr_buff = get_line(buff, curr_buff, true);

				/* Read bounding box */
				sscanf(buff, "( %f %f %f ) ( %f %f %f )",
					&anim->skelFrames[i].bbox.min[0],
					&anim->skelFrames[i].bbox.min[1],
					&anim->skelFrames[i].bbox.min[2],
					&anim->skelFrames[i].bbox.max[0],
					&anim->skelFrames[i].bbox.max[1],
					&anim->skelFrames[i].bbox.max[2]);
			}
		}
		else if (strncmp (buff, "baseframe {", 10) == 0)
		{
			for (i = 0; i < anim->num_joints; ++i)
			{
				curr_buff = get_line(buff, curr_buff, true);

				/* Read base frame joint */
				if (sscanf(buff, "( %f %f %f ) ( %f %f %f )",
					&baseFrame[i].pos[0], &baseFrame[i].pos[1],
					&baseFrame[i].pos[2], &baseFrame[i].orient[0],
					&baseFrame[i].orient[1], &baseFrame[i].orient[2]) == 6)
				{
					/* Compute the w component */
					Quat_computeW(baseFrame[i].orient);
				}
			}
		}
		else if (sscanf(buff, "frame %d", &frame_index) == 1)
		{
			/* Read frame data */
			for (i = 0; i < numAnimatedComponents; ++i)
			{
				curr_buff = get_line(buff, curr_buff, false);
				sscanf(buff, "%f", &animFrameData[i]);
			}

			/* Build frame skeleton from the collected data */
			BuildFrameSkeleton(jointInfos, baseFrame, animFrameData,
							anim->skelFrames[frame_index].skelJoints, anim->num_joints);
		}
	}

	/* Free temporary data allocated */
	if (animFrameData)
	{
		free(animFrameData);
	}

	if (baseFrame)
	{
		free(baseFrame);
	}

	if (jointInfos)
	{
		free(jointInfos);
	}

	free(safe_buffer);
}

static md5_model_t *
AllocModel(void)
{
	md5_model_t *mdl;

	mdl = (md5_model_t *)malloc(sizeof(*mdl));
	memset(mdl, 0, sizeof(*mdl));
	return mdl;
}

/**
 * Load an MD5 model from file.
 */
static md5_model_t *
ReadMD5Model(const char *buffer, size_t size)
{
	const char *curr_buff, *end_buff;
	char *safe_buffer;
	char buff[512];
	int version;
	int curr_mesh = 0;

	md5_model_t *mdl = AllocModel();

	/* buffer has not always had final zero */
	safe_buffer = malloc(size + 1);
	memcpy(safe_buffer, buffer, size);
	safe_buffer[size] = 0;

	curr_buff = safe_buffer;
	end_buff = safe_buffer + size;

	while (curr_buff < end_buff)
	{
		curr_buff = get_line(buff, curr_buff, true);

		if (sscanf(buff, "MD5Version %d", &version) == 1)
		{
			if (version != 10)
			{
				/* Bad version */
				R_Printf(PRINT_ALL, "Error: bad model version\n");
				free(safe_buffer);

				return NULL;
			}
		}
		else if (sscanf(buff, "numJoints %d", &mdl->num_joints) == 1)
		{
			if (mdl->num_joints > 0)
			{
				/* Allocate memory for base skeleton joints */
				mdl->baseSkel = (md5_joint_t *)
					calloc (mdl->num_joints, sizeof(md5_joint_t));
			}
		}
		else if (sscanf(buff, "numMeshes %d", &mdl->num_meshes) == 1)
		{
			if (mdl->num_meshes > 0)
			{
				/* Allocate memory for meshes */
				mdl->meshes = (md5_mesh_t *)
					calloc (mdl->num_meshes, sizeof(md5_mesh_t));
			}
		}
		else if (strncmp (buff, "joints {", 8) == 0)
		{
			int i;

			/* Read each joint */
			for (i = 0; i < mdl->num_joints; ++i)
			{
				md5_joint_t *joint = &mdl->baseSkel[i];

				curr_buff = get_line(buff, curr_buff, true);

				if (sscanf(buff, "%s %d ( %f %f %f ) ( %f %f %f )",
					joint->name, &joint->parent, &joint->pos[0],
					&joint->pos[1], &joint->pos[2], &joint->orient[0],
					&joint->orient[1], &joint->orient[2]) == 8)
				{
					/* Compute the w component */
					Quat_computeW(joint->orient);
				}
			}
		}
		else if (strncmp (buff, "mesh {", 6) == 0)
		{
			md5_mesh_t *mesh = &mdl->meshes[curr_mesh];
			int vert_index = 0;
			int tri_index = 0;
			int weight_index = 0;
			float fdata[4];
			int idata[3];

			while ((buff[0] != '}') && (curr_buff < end_buff))
			{
				curr_buff = get_line(buff, curr_buff, true);

				if (strstr(buff, "shader "))
				{
					int quote = 0, j = 0, i;

					/* Copy the shader name whithout the quote marks */
					for (i = 0; i < sizeof(buff) && (quote < 2); ++i)
					{
						if (buff[i] == '\"')
						{
							quote++;
						}

						if ((quote == 1) && (buff[i] != '\"'))
						{
							mesh->shader[j] = buff[i];
							j++;
						}
					}
				}
				else if (sscanf(buff, "numverts %d", &mesh->num_verts) == 1)
				{
					if (mesh->num_verts > 0)
					{
						/* Allocate memory for vertices */
						mesh->vertices = (md5_vertex_t *)
							malloc(sizeof(md5_vertex_t) * mesh->num_verts);
					}
				}
				else if (sscanf(buff, "numtris %d", &mesh->num_tris) == 1)
				{
					if (mesh->num_tris > 0)
					{
						/* Allocate memory for triangles */
						mesh->triangles = (md5_triangle_t *)
							malloc(sizeof(md5_triangle_t) * mesh->num_tris);
					}
				}
				else if (sscanf(buff, "numweights %d", &mesh->num_weights) == 1)
				{
					if (mesh->num_weights > 0)
					{
						/* Allocate memory for vertex weights */
						mesh->weights = (md5_weight_t *)
							malloc(sizeof(md5_weight_t) * mesh->num_weights);
					}
				}
				else if (sscanf(buff, "vert %d ( %f %f ) %d %d", &vert_index,
						 &fdata[0], &fdata[1], &idata[0], &idata[1]) == 5)
				{
					/* Copy vertex data */
					mesh->vertices[vert_index].st[0] = fdata[0];
					mesh->vertices[vert_index].st[1] = fdata[1];
					mesh->vertices[vert_index].start = idata[0];
					mesh->vertices[vert_index].count = idata[1];
				}
				else if (sscanf(buff, "tri %d %d %d %d", &tri_index,
						 &idata[0], &idata[1], &idata[2]) == 4)
				{
					/* Copy triangle data */
					mesh->triangles[tri_index ].index[0] = idata[0];
					mesh->triangles[tri_index ].index[1] = idata[1];
					mesh->triangles[tri_index ].index[2] = idata[2];
				}
				else if (sscanf(buff, "weight %d %d %f ( %f %f %f )",
						 &weight_index, &idata[0], &fdata[3],
						 &fdata[0], &fdata[1], &fdata[2]) == 6)
				{
					/* Copy vertex data */
					mesh->weights[weight_index].joint = idata[0];
					mesh->weights[weight_index].bias = fdata[3];
					mesh->weights[weight_index].pos[0] = fdata[0];
					mesh->weights[weight_index].pos[1] = fdata[1];
					mesh->weights[weight_index].pos[2] = fdata[2];
				}
			}

			curr_mesh++;
		}
	}

	free(safe_buffer);

	return mdl;
}

/**
 * Free resources allocated for the model.
 */
static void
FreeModelMd5(md5_model_t *mdl)
{
	int i;

	if (mdl->skelFrames)
	{
		for (i = 0; i < mdl->num_frames; ++i)
		{
			if (mdl->skelFrames[i].vertexArray)
			{
				free(mdl->skelFrames[i].vertexArray);
				mdl->skelFrames[i].vertexArray = NULL;
			}

			if (mdl->skelFrames[i].skelJoints)
			{
				free(mdl->skelFrames[i].skelJoints);
				mdl->skelFrames[i].skelJoints = NULL;
			}
		}

		free(mdl->skelFrames);
		mdl->skelFrames = NULL;
	}

	if (mdl->vertexIndices)
	{
		free(mdl->vertexIndices);
		mdl->vertexIndices = NULL;
	}

	if (mdl->st)
	{
		free(mdl->st);
		mdl->st = NULL;
	}

	if (mdl->baseSkel)
	{
		free(mdl->baseSkel);
		mdl->baseSkel = NULL;
	}

	if (mdl->meshes)
	{
		/* Free mesh data */
		for (i = 0; i < mdl->num_meshes; ++i)
		{
			if (mdl->meshes[i].vertices)
			{
				free(mdl->meshes[i].vertices);
				mdl->meshes[i].vertices = NULL;
			}

			if (mdl->meshes[i].triangles)
			{
				free(mdl->meshes[i].triangles);
				mdl->meshes[i].triangles = NULL;
			}

			if (mdl->meshes[i].weights)
			{
				free(mdl->meshes[i].weights);
				mdl->meshes[i].weights = NULL;
			}
		}

		free(mdl->meshes);
		mdl->meshes = NULL;
	}
}

/**
 * Prepare a mesh for drawing.
 * Compute mesh's final vertex positions given a skeleton.
 * Put the vertices in vertex arrays.
 */
static void
PrepareMeshIndices(const md5_mesh_t *mesh, int *vertexIndices, vec2_t *st, int vertex_index, int tris_index)
{
	int i, k;

	/* Setup vertex indices */
	for (k = tris_index, i = 0; i < mesh->num_tris; ++i)
	{
		int j;

		for (j = 0; j < 3; ++j, ++k)
		{
			vertexIndices[k] = mesh->triangles[i].index[j] + vertex_index;
			st[k][0] = mesh->vertices[mesh->triangles[i].index[j]].st[0];
			st[k][1] = mesh->vertices[mesh->triangles[i].index[j]].st[1];
		}
	}
}

static void
PrepareMeshVertex(const md5_mesh_t *mesh, const md5_joint_t *skeleton,
	md5_frame_t *frame, int vert_index)
{
	int i, j;

	/* Setup vertices */
	for (i = 0; i < mesh->num_verts; ++i)
	{
		vec3_t finalVertex = { 0.0f, 0.0f, 0.0f };

		/* Calculate final vertex to draw with weights */
		for (j = 0; j < mesh->vertices[i].count; ++j)
		{
			const md5_weight_t *weight
				= &mesh->weights[mesh->vertices[i].start + j];
			const md5_joint_t *joint
				= &skeleton[weight->joint];

			/* Calculate transformed vertex for this weight */
			vec3_t wv;
			Quat_rotatePoint(joint->orient, weight->pos, wv);

			/* The sum of all weight->bias should be 1.0 */
			finalVertex[0] += (joint->pos[0] + wv[0]) * weight->bias;
			finalVertex[1] += (joint->pos[1] + wv[1]) * weight->bias;
			finalVertex[2] += (joint->pos[2] + wv[2]) * weight->bias;
		}

		frame->vertexArray[i + vert_index].xyz[0] = finalVertex[0];
		frame->vertexArray[i + vert_index].xyz[1] = finalVertex[1];
		frame->vertexArray[i + vert_index].xyz[2] = finalVertex[2];
	}
}

static void
PrepareFrameVertex(md5_frame_t *frame_in, int num_verts, daliasxframe_t *frame_out)
{
	int i;
	vec3_t mins, maxs;

	mins[0] = 9999;
	mins[1] = 9999;
	mins[2] = 9999;
	maxs[0] = -9999;
	maxs[1] = -9999;
	maxs[2] = -9999;

	/* get min/max in frame */
	for (i = 0; i < num_verts; ++i)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			if (mins[j] > frame_in->vertexArray[i].xyz[j])
			{
				mins[j] = frame_in->vertexArray[i].xyz[j];
			}

			if (maxs[j] < frame_in->vertexArray[i].xyz[j])
			{
				maxs[j] = frame_in->vertexArray[i].xyz[j];
			}
		}
	}

	for (i = 0; i < 3; i++)
	{
		frame_out->translate[i] = mins[i];
		frame_out->scale[i] = (maxs[i] - mins[i]) / 0xFFFF;
	}

	for (i = 0; i < num_verts; ++i)
	{
		int j;

		for (j = 0; j < 3; j++)
		{
			frame_out->verts[i].v[j] = (
				frame_in->vertexArray[i].xyz[j] - frame_out->translate[j]
			) / frame_out->scale[j];

			frame_out->verts[i].lightnormalindex =
				R_CompressNormalMDL(frame_in->vertexArray[i].norm);
		}
	}
}

void
MD5_ComputeNormals(md5_model_t *md5file)
{
	int i;

	/* based on MD5_ComputeNormals:
	 * https://github.com/Shpoike/Quakespasm/blob/qsrebase/Quake/gl_mesh.c */
	for(i = 0; i < md5file->num_frames; ++i)
	{
		md5_frame_t *frame_in;
		int j;

		frame_in = md5file->skelFrames + i;

		for (j = 0; j < md5file->num_verts; ++j)
		{
			int k;

			for (k = 0; k < 3; ++k)
			{
				frame_in->vertexArray[j].norm[k] = 0;
			}
		}
	}

	for(i = 0; i < md5file->num_frames; ++i)
	{
		md5_frame_t *frame_in;
		int k, vert_step = 0;

		frame_in = md5file->skelFrames + i;

		for (k = 0; k < md5file->num_meshes; ++k)
		{
			int j;

			for (j = 0; j < md5file->meshes[k].num_tris; ++j)
			{
				md5_fvert_t *v0, *v1, *v2;
				vec3_t d1, d2, norm;

				v0 = frame_in->vertexArray + md5file->meshes[k].triangles[j].index[0] + vert_step;
				v1 = frame_in->vertexArray + md5file->meshes[k].triangles[j].index[1] + vert_step;
				v2 = frame_in->vertexArray + md5file->meshes[k].triangles[j].index[2] + vert_step;

				VectorSubtract(v1->xyz, v0->xyz, d1);
				VectorSubtract(v2->xyz, v0->xyz, d2);
				CrossProduct(d1, d2, norm);
				VectorNormalize(norm);

				// FIXME: this should be weighted by each vertex angle.
				VectorAdd(v0->norm, norm, v0->norm);
				VectorAdd(v1->norm, norm, v1->norm);
				VectorAdd(v2->norm, norm, v2->norm);
			}

			vert_step += md5file->meshes[k].num_verts;
		}
	}

	for(i = 0; i < md5file->num_frames; ++i)
	{
		md5_frame_t *frame_in;
		int j;

		frame_in = md5file->skelFrames + i;

		for (j = 0; j < md5file->num_verts; ++j)
		{
			VectorNormalize(frame_in->vertexArray[j].norm);
			/* FIXME: QSS does not have such invert */
			VectorInverse(frame_in->vertexArray[j].norm);
		}
	}
}

/* mesh and anim should be in same buffer with zero as separator */
void *
Mod_LoadModel_MD5(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	int mesh_size, anim_size;
	md5_model_t *md5file;
	void *extradata = NULL;
	byte *startbuffer, *endbuffer;

	startbuffer = (byte*) buffer;
	endbuffer = startbuffer + modfilelen;

	while (startbuffer < endbuffer)
	{
		if (!(*startbuffer))
		{
			break;
		}

		startbuffer ++;
	}

	mesh_size = startbuffer - (byte*)buffer;
	anim_size = modfilelen - mesh_size - 1;

	md5file = ReadMD5Model(buffer, mesh_size);
	if (!md5file)
	{
		return NULL;
	}

	if (md5file && anim_size > 0)
	{
		ReadMD5Anim(md5file, buffer + mesh_size + 1, anim_size);
	}

	if (!md5file->num_frames)
	{
		R_Printf(PRINT_ALL, "init: no animation loaded.\n");
		md5file->num_frames = 1;

		md5file->skelFrames = (md5_frame_t *)
			malloc(sizeof(md5_frame_t) * md5file->num_frames);

		AllocateFrames(md5file);

		memcpy(md5file->skelFrames[0].skelJoints, md5file->baseSkel,
			sizeof(md5_joint_t) * md5file->num_joints);
	}

	int i, num_verts = 0, num_tris = 0, num_glcmds = 0;

	for (i = 0; i < md5file->num_meshes; ++i)
	{
		int j;

		for(j = 0; j < md5file->num_frames; j ++)
		{
			PrepareMeshVertex(md5file->meshes + i,
				md5file->skelFrames[j].skelJoints,
				md5file->skelFrames + j, num_verts);
		}

		PrepareMeshIndices(md5file->meshes + i,
			md5file->vertexIndices,
			md5file->st,
			num_verts, num_tris);

		num_verts += md5file->meshes[i].num_verts;
		num_tris += md5file->meshes[i].num_tris * 3;
		/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
		num_glcmds += (10 * md5file->meshes[i].num_tris) + 1;
	}

	MD5_ComputeNormals(md5file);

	int framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * num_verts;
	int ofs_skins = sizeof(dmdx_t);
	int ofs_frames = ofs_skins + md5file->num_meshes * MAX_SKINNAME;
	int ofs_glcmds = ofs_frames + framesize * md5file->num_frames;
	int ofs_meshes = ofs_glcmds + num_glcmds * sizeof(int);
	int ofs_tris = ofs_meshes + md5file->num_tris * sizeof(dtriangle_t);
	int ofs_st = ofs_tris + md5file->num_tris * 3 * sizeof(dmdxmesh_t);
	int ofs_end = ofs_st + md5file->num_tris * 3 * sizeof(dstvert_t);

	dmdx_t *pheader = NULL;

	*numskins = md5file->num_meshes;
	extradata = Hunk_Begin(ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	pheader->framesize = framesize;
	pheader->skinheight = 256;
	pheader->skinwidth = 256;
	pheader->num_skins = *numskins;
	pheader->num_glcmds = num_glcmds;
	pheader->num_frames = md5file->num_frames;
	pheader->num_xyz = num_verts;
	pheader->num_meshes = md5file->num_meshes;
	pheader->num_st = md5file->num_tris * 3;
	pheader->num_tris = md5file->num_tris;
	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_end = ofs_end;

	for(i = 0; i < md5file->num_frames; i ++)
	{
		daliasxframe_t *frame = (daliasxframe_t *)(
			(byte *)pheader + pheader->ofs_frames + i * pheader->framesize);
		snprintf(frame->name, 15, "%d", i);
		PrepareFrameVertex(md5file->skelFrames + i, num_verts, frame);
	}

	num_tris = 0;
	int *pglcmds, *baseglcmds;

	pglcmds = baseglcmds = (int *)((byte *)pheader + pheader->ofs_glcmds);
	dmdxmesh_t *mesh_nodes = (dmdxmesh_t *)((byte *)pheader + pheader->ofs_meshes);
	dtriangle_t *tris = (dtriangle_t*)((byte *)pheader + pheader->ofs_tris);
	dstvert_t *st = (dstvert_t*)((byte *)pheader + pheader->ofs_st);

	for(i = 0; i < pheader->num_st; i ++)
	{
		st[i].s = md5file->st[i][0] * pheader->skinwidth;
		st[i].t = md5file->st[i][1] * pheader->skinheight;
	}

	for (i = 0; i < md5file->num_meshes; ++i)
	{
		int j;

		mesh_nodes[i].start = pglcmds - baseglcmds;

		for (j = 0; j < md5file->meshes[i].num_tris * 3; j++)
		{
			/* count */
			if ((j % 3) == 0)
			{
				*pglcmds = 3;
				pglcmds++;
			}

			/* st */
			memcpy(pglcmds, &md5file->st[num_tris * 3 + j], sizeof(vec2_t));
			pglcmds += 2;
			/* index */
			*pglcmds = md5file->vertexIndices[num_tris * 3 + j];
			tris[num_tris + j / 3].index_xyz[j % 3] = *pglcmds;
			tris[num_tris + j / 3].index_st[j % 3] = num_tris * 3 + j;

			pglcmds++;
		}

		/* final zero */
		*pglcmds = 0;
		pglcmds++;

		mesh_nodes[i].num = pglcmds - baseglcmds - mesh_nodes[i].start;

		num_tris += md5file->meshes[i].num_tris; // vertexIndices
	}

	memset((char *)pheader + pheader->ofs_skins, 0,
		pheader->num_skins * MAX_SKINNAME);

	/* use meshes names as replacement for skins */
	for (i = 0; i < pheader->num_skins; i++)
	{
		strncpy((char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME,
			md5file->meshes[i].shader, MAX_SKINNAME - 1);

		R_Printf(PRINT_DEVELOPER, "%s: %s #%d: Should load external '%s'\n",
			__func__, mod_name, i,
			(char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME);
	}

	if (md5file)
	{
		FreeModelMd5(md5file);
		free(md5file);
	}

	*type = mod_alias;

	return extradata;
}
