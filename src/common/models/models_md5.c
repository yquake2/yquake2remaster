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

#include "models.h"

/* Joint */
typedef struct md5_joint_s
{
	char name[64];
	int parent;

	vec3_t pos;
	quat_t orient;
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

typedef struct md5_frame_s
{
	md5_bbox_t bbox;
	md5_joint_t *skelJoints;
	dmdx_vert_t *vertexArray;
} md5_frame_t;

/* MD5 model structure */
typedef struct md5_model_s
{
	md5_joint_t *baseSkel;
	md5_mesh_t *meshes;
	md5_frame_t *skelFrames;
	vec2_t *st;
	char *skins;
	char *framenames;

	int num_frames;
	int num_joints;
	int num_meshes;
	int num_verts;
	int num_tris;
	int num_skins;
	int num_framenames;
	float frameRate;
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
	quat_t orient;
} md5_baseframe_joint_t;

/**
 * Basic quaternion operations.
 */
static void
Quat_computeW(quat_t q)
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

static void
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
		quat_t animatedOrient;
		int j = 0;

		memcpy(animatedPos, baseJoint->pos, sizeof(vec3_t));
		memcpy(animatedOrient, baseJoint->orient, sizeof(quat_t));

		if (jointInfos[i].flags & 1) /* Tx */
		{
			animatedPos[0] = animFrameData[jointInfos[i].startIndex + j];
			j++;
		}

		if (jointInfos[i].flags & 2) /* Ty */
		{
			animatedPos[1] = animFrameData[jointInfos[i].startIndex + j];
			j++;
		}

		if (jointInfos[i].flags & 4) /* Tz */
		{
			animatedPos[2] = animFrameData[jointInfos[i].startIndex + j];
			j++;
		}

		if (jointInfos[i].flags & 8) /* Qx */
		{
			animatedOrient[0] = animFrameData[jointInfos[i].startIndex + j];
			j++;
		}

		if (jointInfos[i].flags & 16) /* Qy */
		{
			animatedOrient[1] = animFrameData[jointInfos[i].startIndex + j];
			j++;
		}

		if (jointInfos[i].flags & 32) /* Qz */
		{
			animatedOrient[2] = animFrameData[jointInfos[i].startIndex + j];
			j++;
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
			memcpy(thisJoint->orient, animatedOrient, sizeof(quat_t));
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
			QuatMultiply(parentJoint->orient, animatedOrient, thisJoint->orient);
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

	anim->st = (vec2_t *)
		malloc(sizeof(vec2_t) * anim->num_tris * 3);
	if (!anim->st)
	{
		YQ2_COM_CHECK_OOM(anim->st, "malloc()",
			sizeof(vec2_t) * anim->num_tris * 3)
		return;
	}

	for (i = 0; i < anim->num_frames; ++i)
	{
		memset(&(anim->skelFrames[i].bbox), 0, sizeof(md5_bbox_t));

		/* Allocate memory for joints of each frame */
		anim->skelFrames[i].skelJoints = (md5_joint_t *)
			malloc(sizeof(md5_joint_t) * anim->num_joints);
		if (!anim->skelFrames[i].skelJoints)
		{
			YQ2_COM_CHECK_OOM(anim->skelFrames[i].skelJoints, "malloc()",
				sizeof(md5_joint_t) * anim->num_joints)
			return;
		}

		anim->skelFrames[i].vertexArray = (dmdx_vert_t *)
			malloc(sizeof(dmdx_vert_t) * anim->num_verts);
		if (!anim->skelFrames[i].vertexArray)
		{
			YQ2_COM_CHECK_OOM(anim->skelFrames[i].vertexArray, "malloc()",
				sizeof(dmdx_vert_t) * anim->num_verts)
			return;
		}
		memcpy(anim->skelFrames[i].skelJoints, anim->baseSkel,
			sizeof(md5_joint_t) * anim->num_joints);
	}
}

/**
 * Free frames allocated for the model.
 */
static void
FreeModelMd5Frames(md5_model_t *mdl)
{
	if (mdl->skelFrames)
	{
		int i;

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
		mdl->num_frames = 0;
	}
}

/*
 * Parse float block
 */
static qboolean
ParseFloatBlock(char **curr_buff, int count, float *v)
{
	const char *token;
	int j;

	token = COM_Parse(curr_buff);
	if (strcmp(token, "("))
	{
		Com_Printf("Error: expected float block open\n");
		return false;
	}

	for (j = 0; j < count; j ++)
	{
		token = COM_Parse(curr_buff);
		v[j] = (float)strtod(token, (char **)NULL);
	}

	token = COM_Parse(curr_buff);
	if (strcmp(token, ")"))
	{
		Com_Printf("Error: expected float block close\n");
		return false;
	}

	return true;
}


/**
 * Load an MD5 animation from file.
 */
static void
ReadMD5Anim(md5_model_t *anim, const char *buffer, size_t size)
{
	md5_baseframe_joint_t *baseFrame = NULL;
	md5_joint_info_t *jointInfos = NULL;
	int numAnimatedComponents = 0;
	char *curr_buff,*safe_buffer;
	float *animFrameData = NULL;
	const char *end_buff;

	/* buffer has not always had final zero */
	safe_buffer = malloc(size + 1);
	if (!safe_buffer)
	{
		FreeModelMd5Frames(anim);
		YQ2_COM_CHECK_OOM(safe_buffer, "malloc()", size + 1)
		return;
	}

	memcpy(safe_buffer, buffer, size);
	safe_buffer[size] = 0;

	curr_buff = safe_buffer;
	end_buff = safe_buffer + size;

	while (curr_buff && (curr_buff < end_buff))
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!curr_buff)
		{
			/* end of buffer */
			break;
		}

		if (!strcmp(token, "MD5Version"))
		{
			token = COM_Parse(&curr_buff);
			if (strcmp(token, "10"))
			{
				/* Bad version */
				Com_Printf("Error: bad animation version\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
		}
		else if (!strcmp(token, "numFrames"))
		{
			token = COM_Parse(&curr_buff);
			if (anim->skelFrames)
			{
				/* To insure analysers as we are in a loop */
				Com_Printf("Error: several numFrames sections");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
			anim->num_frames = (int)strtol(token, (char **)NULL, 10);

			/* Allocate memory for skeleton frames and bounding boxes */
			if (anim->num_frames > 0)
			{
				anim->skelFrames = (md5_frame_t *)
					malloc(sizeof(md5_frame_t) * anim->num_frames);
				if (!anim->skelFrames)
				{
					FreeModelMd5Frames(anim);
					YQ2_COM_CHECK_OOM(anim->skelFrames, "malloc()",
						sizeof(md5_frame_t) * anim->num_frames)
					return;
				}
			}
		}
		else if (!strcmp(token, "numJoints"))
		{
			token = COM_Parse(&curr_buff);
			if (jointInfos || baseFrame)
			{
				/* To insure analysers as we are in a loop */
				Com_Printf("Error: several numJoints sections");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
			anim->num_joints = (int)strtol(token, (char **)NULL, 10);

			if (anim->num_joints > 0)
			{
				AllocateFrames(anim);

				/* Allocate temporary memory for building skeleton frames */
				jointInfos = (md5_joint_info_t *)
					malloc(sizeof(md5_joint_info_t) * anim->num_joints);
				if (!jointInfos)
				{
					FreeModelMd5Frames(anim);
					YQ2_COM_CHECK_OOM(jointInfos, "malloc()",
						sizeof(md5_joint_info_t) * anim->num_joints)
					return;
				}

				baseFrame = (md5_baseframe_joint_t *)
					malloc(sizeof(md5_baseframe_joint_t) * anim->num_joints);
				if (!baseFrame)
				{
					FreeModelMd5Frames(anim);
					YQ2_COM_CHECK_OOM(baseFrame, "malloc()",
						sizeof(md5_baseframe_joint_t) * anim->num_joints)
					return;
				}
			}
		}
		else if (!strcmp(token, "frameRate"))
		{
			token = COM_Parse(&curr_buff);
			anim->frameRate = (float)strtod(token, (char **)NULL);

			Com_DPrintf("%s: animation's frame rate is %.2f\n",
				__func__, anim->frameRate);
		}
		else if (!strcmp(token, "numAnimatedComponents"))
		{
			token = COM_Parse(&curr_buff);
			if (animFrameData)
			{
				/* To insure analysers as we are in a loop */
				Com_Printf("Error: several numAnimatedComponents sections");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
			numAnimatedComponents = (int)strtol(token, (char **)NULL, 10);

			if (numAnimatedComponents > 0)
			{
				/* Allocate memory for animation frame data */
				animFrameData = (float *)malloc(sizeof(float) * numAnimatedComponents);
				if (!animFrameData)
				{
					FreeModelMd5Frames(anim);
					YQ2_COM_CHECK_OOM(animFrameData, "malloc()",
						sizeof(float) * numAnimatedComponents)
					return;
				}
			}
		}
		else if (!strcmp(token, "hierarchy"))
		{
			int i;

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("%s: expected hierarchy block open\n",
					__func__);
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			if (!jointInfos)
			{
				Com_Printf("%s: unexpected block\n", __func__);
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			for (i = 0; i < anim->num_joints; ++i)
			{
				/* Read joint info */
				token = COM_Parse(&curr_buff);
				Q_strlcpy(jointInfos[i].name, token, sizeof(jointInfos[i].name));

				token = COM_Parse(&curr_buff);
				jointInfos[i].parent = (int)strtol(token, (char **)NULL, 10);

				token = COM_Parse(&curr_buff);
				jointInfos[i].flags = (int)strtol(token, (char **)NULL, 10);

				token = COM_Parse(&curr_buff);
				jointInfos[i].startIndex = (int)strtol(token, (char **)NULL, 10);
			}

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "}"))
			{
				Com_Printf("Error: expected hierarchy block close\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
		}
		else if (!strcmp(token, "bounds"))
		{
			int i;

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("Error: expected hierarchy bounds open\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			for (i = 0; i < anim->num_frames; ++i)
			{
				if (!ParseFloatBlock(&curr_buff, 3, anim->skelFrames[i].bbox.min) ||
					!ParseFloatBlock(&curr_buff, 3, anim->skelFrames[i].bbox.max))
				{
					Com_Printf("Error: unexpected bounds format\n");
					break;
				}
			}

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "}"))
			{
				Com_Printf("Error: expected block close\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
		}
		else if (!strcmp(token, "baseframe"))
		{
			int i;

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("Error: expected baseframe block open\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			for (i = 0; i < anim->num_joints; ++i)
			{
				/* Read base frame joint */
				if (!baseFrame ||
					!ParseFloatBlock(&curr_buff, 3, baseFrame[i].pos) ||
					!ParseFloatBlock(&curr_buff, 3, baseFrame[i].orient))
				{
					Com_Printf("Error: unexpected baseframe format\n");
					break;
				}

				/* Compute the w component */
				Quat_computeW(baseFrame[i].orient);
			}

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "}"))
			{
				Com_Printf("Error: expected baseframe block close\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
		}
		else if (!strcmp(token, "frame"))
		{
			int i, frame_index;

			token = COM_Parse(&curr_buff);
			frame_index = (int)strtol(token, (char **)NULL, 10);

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("Error: expected frame bounds open\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			/* Read frame data */
			for (i = 0; i < numAnimatedComponents; ++i)
			{
				token = COM_Parse(&curr_buff);
				animFrameData[i] = (float)strtod(token, (char **)NULL);
			}

			if (frame_index < 0 || frame_index >= anim->num_frames)
			{
				Com_Printf("Error: unknown frame number\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			if (!jointInfos || !baseFrame || !animFrameData)
			{
				Com_Printf("Error: unknown size of frame\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}

			/* Build frame skeleton from the collected data */
			BuildFrameSkeleton(jointInfos, baseFrame, animFrameData,
				anim->skelFrames[frame_index].skelJoints, anim->num_joints);

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "}"))
			{
				Com_Printf("Error: expected frame block close\n");
				/* broken file */
				FreeModelMd5Frames(anim);
				break;
			}
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

/**
 * Free resources allocated for the model.
 */
static void
FreeModelMd5(md5_model_t *mdl)
{
	FreeModelMd5Frames(mdl);

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

	if (mdl->skins)
	{
		free(mdl->skins);
		mdl->skins = NULL;
	}

	if (mdl->framenames)
	{
		free(mdl->framenames);
		mdl->framenames = NULL;
	}

	if (mdl->meshes)
	{
		int i;

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
	free(mdl);
}

/**
 * Load an MD5 model from file.
 */
static md5_model_t *
ReadMD5Model(const char *buffer, size_t size)
{
	char *curr_buff, *safe_buffer;
	const char *end_buff;
	int curr_mesh = 0;

	md5_model_t *mdl = calloc(1, sizeof(*mdl));
	if (!mdl)
	{
		YQ2_COM_CHECK_OOM(mdl, "malloc()", sizeof(*mdl))
		return NULL;
	}

	/* buffer has not always had final zero */
	safe_buffer = malloc(size + 1);
	if (!safe_buffer)
	{
		FreeModelMd5(mdl);
		YQ2_COM_CHECK_OOM(safe_buffer, "malloc()", size + 1)
		/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
		return NULL;
	}

	memcpy(safe_buffer, buffer, size);
	safe_buffer[size] = 0;

	curr_buff = safe_buffer;
	end_buff = safe_buffer + size;

	while (curr_buff && (curr_buff < end_buff))
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!curr_buff)
		{
			/* end of buffer */
			break;
		}

		if (!strcmp(token, "MD5Version"))
		{
			token = COM_Parse(&curr_buff);
			if (strcmp(token, "10"))
			{
				/* Bad version */
				Com_Printf("Error: bad model version\n");
				FreeModelMd5(mdl);
				free(safe_buffer);

				return NULL;
			}
		}
		else if (!strcmp(token, "numSkins"))
		{
			token = COM_Parse(&curr_buff);
			mdl->num_skins = (int)strtol(token, (char **)NULL, 10);

			if (mdl->num_skins > 0)
			{
				int size;

				size = mdl->num_skins * MAX_SKINNAME;
				mdl->skins = malloc(size);
				if (!mdl->skins)
				{
					FreeModelMd5(mdl);
					free(safe_buffer);
					Com_Error(ERR_FATAL, "%s: can't allocate %d bytes\n",
						__func__, size);
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					return NULL;
				}

				memset(mdl->skins, 0, mdl->num_skins * MAX_SKINNAME);
			}
		}
		else if (!strcmp(token, "numFramenames"))
		{
			token = COM_Parse(&curr_buff);
			mdl->num_framenames = (int)strtol(token, (char **)NULL, 10);

			if (mdl->num_framenames > 0)
			{
				int size;

				size = mdl->num_framenames * 16;
				mdl->framenames = malloc(size);
				if (!mdl->framenames)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					FreeModelMd5(mdl);
					free(safe_buffer);
					Com_Error(ERR_FATAL, "%s: can't allocate %d bytes\n",
						__func__, size);
					return NULL;
				}

				memset(mdl->framenames, 0, mdl->num_framenames * 16);
			}
		}
		else if (!strcmp(token, "numJoints"))
		{
			token = COM_Parse(&curr_buff);
			mdl->num_joints = (int)strtol(token, (char **)NULL, 10);

			if (mdl->num_joints > 0)
			{
				/* Allocate memory for base skeleton joints */
				mdl->baseSkel = (md5_joint_t *)
					calloc (mdl->num_joints, sizeof(md5_joint_t));
				if (!mdl->baseSkel)
				{
					FreeModelMd5(mdl);
					free(safe_buffer);
					Com_Error(ERR_FATAL, "%s: can't allocate memory\n", __func__);
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					return NULL;
				}
			}
		}
		else if (!strcmp(token, "numMeshes"))
		{
			token = COM_Parse(&curr_buff);
			mdl->num_meshes = (int)strtol(token, (char **)NULL, 10);

			if (mdl->num_meshes > 0)
			{
				/* Allocate memory for meshes */
				mdl->meshes = (md5_mesh_t *)
					calloc(mdl->num_meshes, sizeof(md5_mesh_t));
				if (!mdl->meshes)
				{
					FreeModelMd5(mdl);
					free(safe_buffer);
					Com_Error(ERR_FATAL, "%s: can't allocate memory\n", __func__);
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					return NULL;
				}
			}
		}
		else if (!strcmp(token, "skin"))
		{
			int pos;

			token = COM_Parse(&curr_buff);
			pos = (int)strtol(token, (char **)NULL, 10);
			token = COM_Parse(&curr_buff);

			if (pos >= 0 && pos < mdl->num_skins)
			{
				char *skinname;

				skinname = mdl->skins + pos * MAX_SKINNAME;
				Q_strlcpy(skinname, token, MAX_SKINNAME);
			}
		}
		else if (!strcmp(token, "framename"))
		{
			int pos;

			token = COM_Parse(&curr_buff);
			pos = (int)strtol(token, (char **)NULL, 10);
			token = COM_Parse(&curr_buff);

			if (pos >= 0 && pos < mdl->num_framenames)
			{
				char *framename;

				framename = mdl->framenames + pos * 16;
				Q_strlcpy(framename, token, 16);
			}
		}
		else if (!strcmp(token, "joints"))
		{
			int i;

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("Error: expected joints block open\n");
				FreeModelMd5(mdl);
				free(safe_buffer);

				return NULL;
			}

			/* Read each joint */
			for (i = 0; i < mdl->num_joints; ++i)
			{
				md5_joint_t *joint = &mdl->baseSkel[i];

				token = COM_Parse(&curr_buff);
				Q_strlcpy(joint->name, token, sizeof(joint->name));

				token = COM_Parse(&curr_buff);
				joint->parent = (int)strtol(token, (char **)NULL, 10);

				if (!ParseFloatBlock(&curr_buff, 3, joint->pos) ||
					!ParseFloatBlock(&curr_buff, 3, joint->orient))
				{
					FreeModelMd5(mdl);
					free(safe_buffer);

					return NULL;
				}

				/* Compute the w component */
				Quat_computeW(joint->orient);
			}

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "}"))
			{
				Com_Printf("Error: expected joints block close\n");
				FreeModelMd5(mdl);
				free(safe_buffer);

				return NULL;
			}
		}
		else if (!strcmp(token, "mesh"))
		{
			md5_mesh_t *mesh;

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "{"))
			{
				Com_Printf("Error: expected mesh block open\n");
				FreeModelMd5(mdl);
				free(safe_buffer);

				return NULL;
			}

			/* more meshes than originally provided */
			if (curr_mesh >= mdl->num_meshes)
			{
				md5_mesh_t *tmp;

				mdl->num_meshes = curr_mesh + 1;

				/* Allocate memory for meshes */
				tmp = (md5_mesh_t *)
					realloc(mdl->meshes, mdl->num_meshes * sizeof(md5_mesh_t));
				YQ2_COM_CHECK_OOM(tmp, "realloc()",
					mdl->num_meshes * sizeof(md5_mesh_t))
				if (!tmp)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					FreeModelMd5(mdl);
					free(safe_buffer);

					return NULL;
				}

				mdl->meshes = tmp;
				memset(mdl->meshes + curr_mesh, 0, sizeof(md5_mesh_t));
			}

			mesh = &mdl->meshes[curr_mesh];

			while (curr_buff)
			{
				token = COM_Parse(&curr_buff);

				if (!strcmp(token, "}"))
				{
					break;
				}
				else if (!strcmp(token, "shader"))
				{
					token = COM_Parse(&curr_buff);
					Q_strlcpy(mesh->shader, token, sizeof(mesh->shader));
				}
				else if (!strcmp(token, "numverts"))
				{
					token = COM_Parse(&curr_buff);
					mesh->num_verts = (int)strtol(token, (char **)NULL, 10);

					if (mesh->num_verts > 0)
					{
						/* Allocate memory for vertices */
						mesh->vertices = (md5_vertex_t *)
							malloc(sizeof(md5_vertex_t) * mesh->num_verts);
						YQ2_COM_CHECK_OOM(mesh->vertices, "realloc()",
							sizeof(md5_vertex_t) * mesh->num_verts)
						if (!mesh->vertices)
						{
							/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
							FreeModelMd5(mdl);
							free(safe_buffer);

							return NULL;
						}
					}
				}
				else if (!strcmp(token, "numtris"))
				{
					token = COM_Parse(&curr_buff);
					mesh->num_tris = (int)strtol(token, (char **)NULL, 10);

					if (mesh->num_tris > 0)
					{
						/* Allocate memory for triangles */
						mesh->triangles = (md5_triangle_t *)
							malloc(sizeof(md5_triangle_t) * mesh->num_tris);
						YQ2_COM_CHECK_OOM(mesh->triangles, "realloc()",
							sizeof(md5_triangle_t) * mesh->num_tris)
						if (!mesh->triangles)
						{
							/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
							FreeModelMd5(mdl);
							free(safe_buffer);

							return NULL;
						}
					}
				}
				else if (!strcmp(token, "numweights"))
				{
					token = COM_Parse(&curr_buff);
					mesh->num_weights = (int)strtol(token, (char **)NULL, 10);

					if (mesh->num_weights > 0)
					{
						/* Allocate memory for vertex weights */
						mesh->weights = (md5_weight_t *)
							malloc(sizeof(md5_weight_t) * mesh->num_weights);
						YQ2_COM_CHECK_OOM(mesh->weights, "realloc()",
							sizeof(md5_weight_t) * mesh->num_weights)
						if (!mesh->weights)
						{
							/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
							FreeModelMd5(mdl);
							free(safe_buffer);

							return NULL;
						}
					}
				}
				else if (!strcmp(token, "vert"))
				{
					int index;

					token = COM_Parse(&curr_buff);
					index = (int)strtol(token, (char **)NULL, 10);

					if (index >= mesh->num_verts)
					{
						Com_Printf("Error: incorrect vert index\n");
						FreeModelMd5(mdl);
						free(safe_buffer);

						return NULL;
					}

					/* Copy vertex data */
					if (!ParseFloatBlock(&curr_buff, 2, mesh->vertices[index].st))
					{
						FreeModelMd5(mdl);
						free(safe_buffer);

						return NULL;
					}

					token = COM_Parse(&curr_buff);
					mesh->vertices[index].start = (int)strtol(token, (char **)NULL, 10);

					token = COM_Parse(&curr_buff);
					mesh->vertices[index].count = (int)strtol(token, (char **)NULL, 10);
				}
				else if (!strcmp(token, "tri"))
				{
					int j, index;

					token = COM_Parse(&curr_buff);
					index = (int)strtol(token, (char **)NULL, 10);

					if (index >= mesh->num_tris)
					{
						Com_Printf("Error: incorrect tri index\n");
						FreeModelMd5(mdl);
						free(safe_buffer);

						return NULL;
					}

					/* Copy triangle data */
					for (j = 0; j < 3; j++)
					{
						token = COM_Parse(&curr_buff);
						mesh->triangles[index].index[j] = (int)strtol(token, (char **)NULL, 10);
					}
				}
				else if (!strcmp(token, "weight"))
				{
					int index;

					token = COM_Parse(&curr_buff);
					index = (int)strtol(token, (char **)NULL, 10);

					if (index >= mesh->num_weights)
					{
						Com_Printf("Error: incorrect weight index\n");
						FreeModelMd5(mdl);
						free(safe_buffer);

						return NULL;
					}

					token = COM_Parse(&curr_buff);
					mesh->weights[index].joint = (int)strtol(token, (char **)NULL, 10);

					token = COM_Parse(&curr_buff);
					mesh->weights[index].bias = (float)strtod(token, (char **)NULL);

					/* Copy vertex data */
					if (!ParseFloatBlock(&curr_buff, 3, mesh->weights[index].pos))
					{
						FreeModelMd5(mdl);
						free(safe_buffer);

						return NULL;
					}
				}
			}

			curr_mesh++;
		}
	}

	free(safe_buffer);

	return mdl;
}

/**
 * Prepare a mesh for drawing.
 * Compute mesh's final vertex positions given a skeleton.
 * Put the vertices in vertex arrays.
 */
static void
PrepareMeshIndices(const md5_mesh_t *mesh, vec2_t *st, int vertex_index, int tris_index)
{
	int i, k;

	/* Setup vertex indices */
	for (k = tris_index, i = 0; i < mesh->num_tris; ++i)
	{
		int j;

		for (j = 0; j < 3; ++j, ++k)
		{
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

void
PrepareFrameVertex(dmdx_vert_t *vertexArray, int num_verts, daliasxframe_t *frame_out)
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
			if (mins[j] > vertexArray[i].xyz[j])
			{
				mins[j] = vertexArray[i].xyz[j];
			}

			if (maxs[j] < vertexArray[i].xyz[j])
			{
				maxs[j] = vertexArray[i].xyz[j];
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
				vertexArray[i].xyz[j] - frame_out->translate[j]
			) / frame_out->scale[j];

			frame_out->verts[i].normal[j] = vertexArray[i].norm[j] * 127.f;
		}
	}
}

static void
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
		const md5_frame_t *frame_in;
		int k, vert_step = 0;

		frame_in = md5file->skelFrames + i;

		for (k = 0; k < md5file->num_meshes; ++k)
		{
			int j;

			for (j = 0; j < md5file->meshes[k].num_tris; ++j)
			{
				dmdx_vert_t *v0, *v1, *v2;
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
Mod_LoadModel_MD5(const char *mod_name, const void *buffer, int modfilelen)
{
	int framesize, i, num_verts = 0, num_tris = 0, num_glcmds = 0;
	dmdx_t dmdxheader, *pheader;
	int mesh_size, anim_size;
	void *extradata = NULL;
	dmdxmesh_t *mesh_nodes;
	const byte *endbuffer;
	md5_model_t *md5file;
	byte *startbuffer;
	dtriangle_t *tris;
	dstvert_t *st;

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

	if (anim_size > 0)
	{
		ReadMD5Anim(md5file, (char*)buffer + mesh_size + 1, anim_size);
	}

	if (!md5file->num_frames)
	{
		size_t size;

		Com_Printf("%s: no animation loaded.\n", __func__);
		md5file->num_frames = 1;

		size = sizeof(md5_frame_t) * md5file->num_frames;
		md5file->skelFrames = (md5_frame_t *)malloc(size);
		if (!md5file->skelFrames)
		{
			FreeModelMd5(md5file);
			Com_Error(ERR_FATAL, "%s: can't allocate\n", __func__);
			/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
			return NULL;
		}

		memset(md5file->skelFrames, 0, size);

		AllocateFrames(md5file);

		memcpy(md5file->skelFrames[0].skelJoints, md5file->baseSkel,
			sizeof(md5_joint_t) * md5file->num_joints);
	}

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
			md5file->st,
			num_verts, num_tris);

		num_verts += md5file->meshes[i].num_verts;
		num_tris += md5file->meshes[i].num_tris * 3;
	}

	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	num_glcmds = (10 * md5file->num_tris) + 1 * md5file->num_meshes;

	MD5_ComputeNormals(md5file);

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * num_verts;

	/* copy back all values */
	memset(&dmdxheader, 0, sizeof(dmdxheader));
	dmdxheader.framesize = framesize;
	dmdxheader.skinheight = 256;
	dmdxheader.skinwidth = 256;
	dmdxheader.num_skins = md5file->num_skins;
	dmdxheader.num_glcmds = num_glcmds;
	dmdxheader.num_frames = md5file->num_frames;
	dmdxheader.num_xyz = num_verts;
	dmdxheader.num_meshes = md5file->num_meshes;
	dmdxheader.num_st = md5file->num_tris * 3;
	dmdxheader.num_tris = md5file->num_tris;
	dmdxheader.num_imgbit = 0;
	dmdxheader.num_animgroup = md5file->num_frames;

	pheader = Mod_LoadAllocate(mod_name, &dmdxheader, &extradata);

	for(i = 0; i < md5file->num_frames; i ++)
	{
		daliasxframe_t *frame = (daliasxframe_t *)(
			(byte *)pheader + pheader->ofs_frames + i * pheader->framesize);

		if (md5file->framenames && (i < md5file->num_framenames))
		{
			Q_strlcpy(frame->name, md5file->framenames + i * 16,
				sizeof(frame->name));
		}
		else
		{
			/* limit frame ids to 2**16 */
			snprintf(frame->name, sizeof(frame->name), "frame%d", i % 0xFFFF);
		}

		PrepareFrameVertex((md5file->skelFrames + i)->vertexArray,
			num_verts, frame);
	}

	mesh_nodes = (dmdxmesh_t *)((byte *)pheader + pheader->ofs_meshes);
	tris = (dtriangle_t*)((byte *)pheader + pheader->ofs_tris);
	st = (dstvert_t*)((byte *)pheader + pheader->ofs_st);

	for(i = 0; i < pheader->num_st; i ++)
	{
		st[i].s = md5file->st[i][0] * pheader->skinwidth;
		st[i].t = md5file->st[i][1] * pheader->skinheight;
	}

	num_tris = 0;
	num_verts = 0;

	for (i = 0; i < md5file->num_meshes; ++i)
	{
		int j;

		mesh_nodes[i].ofs_tris = num_tris;
		mesh_nodes[i].num_tris = md5file->meshes[i].num_tris;

		for (j = 0; j < md5file->meshes[i].num_tris; j++)
		{
			int k;

			for (k = 0; k < 3; k++)
			{
				int vert_id;

				vert_id = num_verts + md5file->meshes[i].triangles[j].index[k];
				tris[num_tris + j].index_xyz[k] = vert_id;
				tris[num_tris + j].index_st[k] = (num_tris + j) * 3 + k;
			}
		}
		num_verts += md5file->meshes[i].num_verts;
		num_tris += md5file->meshes[i].num_tris;
	}

	Mod_LoadAnimGroupList(pheader);
	Mod_LoadCmdGenerate(pheader);

	/* register all skins */
	memcpy((char *)pheader + pheader->ofs_skins, md5file->skins,
		pheader->num_skins * MAX_SKINNAME);

	FreeModelMd5(md5file);

	Mod_LoadFixImages(mod_name, pheader, false);

	return extradata;
}
