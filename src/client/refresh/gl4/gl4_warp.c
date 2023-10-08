/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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
 * Warps. Used on water surfaces und for skybox rotation.
 *
 * =======================================================================
 */

#include "header/local.h"

/*
 * Does a water warp on the pre-fragmented mpoly_t chain
 */
void
GL4_EmitWaterPolys(msurface_t *fa)
{
	mpoly_t *bp;
	float scroll = 0.0f;

	if (fa->texinfo->flags & SURF_FLOWING)
	{
		scroll = -64.0f * ((gl4_newrefdef.time * 0.5) - (int)(gl4_newrefdef.time * 0.5));
		if (scroll == 0.0f) // this is done in GL4_DrawGLFlowingPoly() TODO: keep?
		{
			scroll = -64.0f;
		}
	}

	qboolean updateUni3D = false;
	if(gl4state.uni3DData.scroll != scroll)
	{
		gl4state.uni3DData.scroll = scroll;
		updateUni3D = true;
	}
	// these surfaces (mostly water and lava, I think?) don't have a lightmap.
	// rendering water at full brightness looks bad (esp. for water in dark environments)
	// so default use a factor of 0.5 (ontop of intensity)
	// but lava should be bright and glowing, so use full brightness there
	float lightScale = fa->texinfo->image->is_lava ? 1.0f : 0.5f;
	if(lightScale != gl4state.uni3DData.lightScaleForTurb)
	{
		gl4state.uni3DData.lightScaleForTurb = lightScale;
		updateUni3D = true;
	}

	if(updateUni3D)
	{
		GL4_UpdateUBO3D();
	}

	GL4_UseProgram(gl4state.si3Dturb.shaderProgram);

	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	for (bp = fa->polys; bp != NULL; bp = bp->next)
	{
		GL4_BufferAndDraw3D(bp->verts, bp->numverts, GL_TRIANGLE_FAN);
	}
}

// ########### below: Sky-specific stuff ##########

#define ON_EPSILON 0.1 /* point on plane side epsilon */
enum { MAX_CLIP_VERTS = 64 };


static const int skytexorder[6] = {0, 2, 1, 3, 4, 5};

static float skymins[2][6], skymaxs[2][6];
static float sky_min, sky_max;

static float skyrotate;
static int skyautorotate;
static vec3_t skyaxis;
static gl4image_t* sky_images[6];

/* 3dstudio environment map names */
static const char* suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

vec3_t skyclip[6] = {
	{1, 1, 0},
	{1, -1, 0},
	{0, -1, 1},
	{0, 1, 1},
	{1, 0, 1},
	{-1, 0, 1}
};
int c_sky;

int st_to_vec[6][3] = {
	{3, -1, 2},
	{-3, 1, 2},

	{1, 3, 2},
	{-1, -3, 2},

	{-2, -1, 3}, /* 0 degrees yaw, look straight up */
	{2, -1, -3} /* look straight down */
};

int vec_to_st[6][3] = {
	{-2, 3, 1},
	{2, 3, -1},

	{1, 3, 2},
	{-1, 3, -2},

	{-2, -1, 3},
	{-2, 1, -3}
};


void
GL4_SetSky(const char *name, float rotate, int autorotate, const vec3_t axis)
{
	char	skyname[MAX_QPATH];
	int		i;

	Q_strlcpy(skyname, name, sizeof(skyname));
	skyrotate = rotate;
	skyautorotate = autorotate;
	VectorCopy(axis, skyaxis);

	for (i = 0; i < 6; i++)
	{
		gl4image_t	*image;

		image = (gl4image_t *)GetSkyImage(skyname, suf[i],
			r_palettedtexture->value, (findimage_t)GL4_FindImage);

		if (!image)
		{
			R_Printf(PRINT_ALL, "%s: can't load %s:%s sky\n",
				__func__, skyname, suf[i]);
			image = gl4_notexture;
		}

		sky_images[i] = image;
	}

	sky_min = 1.0 / 512;
	sky_max = 511.0 / 512;
}

static void
DrawSkyPolygon(int nump, vec3_t vecs)
{
	int i, j;
	vec3_t v, av;
	float s, t, dv;
	int axis;
	float *vp;

	c_sky++;

	/* decide which face it maps to */
	VectorCopy(vec3_origin, v);

	for (i = 0, vp = vecs; i < nump; i++, vp += 3)
	{
		VectorAdd(vp, v, v);
	}

	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);

	if ((av[0] > av[1]) && (av[0] > av[2]))
	{
		if (v[0] < 0)
		{
			axis = 1;
		}
		else
		{
			axis = 0;
		}
	}
	else if ((av[1] > av[2]) && (av[1] > av[0]))
	{
		if (v[1] < 0)
		{
			axis = 3;
		}
		else
		{
			axis = 2;
		}
	}
	else
	{
		if (v[2] < 0)
		{
			axis = 5;
		}
		else
		{
			axis = 4;
		}
	}

	/* project new texture coords */
	for (i = 0; i < nump; i++, vecs += 3)
	{
		j = vec_to_st[axis][2];

		if (j > 0)
		{
			dv = vecs[j - 1];
		}
		else
		{
			dv = -vecs[-j - 1];
		}

		if (dv < 0.001)
		{
			continue; /* don't divide by zero */
		}

		j = vec_to_st[axis][0];

		if (j < 0)
		{
			s = -vecs[-j - 1] / dv;
		}
		else
		{
			s = vecs[j - 1] / dv;
		}

		j = vec_to_st[axis][1];

		if (j < 0)
		{
			t = -vecs[-j - 1] / dv;
		}
		else
		{
			t = vecs[j - 1] / dv;
		}

		if (s < skymins[0][axis])
		{
			skymins[0][axis] = s;
		}

		if (t < skymins[1][axis])
		{
			skymins[1][axis] = t;
		}

		if (s > skymaxs[0][axis])
		{
			skymaxs[0][axis] = s;
		}

		if (t > skymaxs[1][axis])
		{
			skymaxs[1][axis] = t;
		}
	}
}

static void
ClipSkyPolygon(int nump, vec3_t vecs, int stage)
{
	float *norm;
	float *v;
	qboolean front, back;
	float d, e;
	float dists[MAX_CLIP_VERTS];
	int sides[MAX_CLIP_VERTS];
	vec3_t newv[2][MAX_CLIP_VERTS];
	int newc[2];
	int i, j;

	if (nump > MAX_CLIP_VERTS - 2)
	{
		Com_Error(ERR_DROP, "R_ClipSkyPolygon: MAX_CLIP_VERTS");
	}

	if (stage == 6)
	{
		/* fully clipped, so draw it */
		DrawSkyPolygon(nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];

	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		d = DotProduct(v, norm);

		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}

		dists[i] = d;
	}

	if (!front || !back)
	{
		/* not clipped */
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	/* clip it */
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		switch (sides[i])
		{
			case SIDE_FRONT:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				break;
			case SIDE_BACK:
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			case SIDE_ON:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
		}

		if ((sides[i] == SIDE_ON) ||
			(sides[i + 1] == SIDE_ON) ||
			(sides[i + 1] == sides[i]))
		{
			continue;
		}

		d = dists[i] / (dists[i] - dists[i + 1]);

		for (j = 0; j < 3; j++)
		{
			e = v[j] + d * (v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}

		newc[0]++;
		newc[1]++;
	}

	/* continue */
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

void
GL4_AddSkySurface(msurface_t *fa)
{
	int i;
	vec3_t verts[MAX_CLIP_VERTS];
	mpoly_t *p;

	/* calculate vertex values for sky box */
	for (p = fa->polys; p; p = p->next)
	{
		for (i = 0; i < p->numverts; i++)
		{
			VectorSubtract(p->verts[i].pos, gl4_origin, verts[i]);
		}

		ClipSkyPolygon(p->numverts, verts[0], 0);
	}
}

void
GL4_ClearSkyBox(void)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}

static void
MakeSkyVec(float s, float t, int axis, mvtx_t* vert)
{
	vec3_t v, b;
	int j, k;

	float dist = (r_farsee->value == 0) ? 2300.0f : 4096.0f;

	b[0] = s * dist;
	b[1] = t * dist;
	b[2] = dist;

	for (j = 0; j < 3; j++)
	{
		k = st_to_vec[axis][j];

		if (k < 0)
		{
			v[j] = -b[-k - 1];
		}
		else
		{
			v[j] = b[k - 1];
		}
	}

	/* avoid bilerp seam */
	s = (s + 1) * 0.5;
	t = (t + 1) * 0.5;

	if (s < sky_min)
	{
		s = sky_min;
	}
	else if (s > sky_max)
	{
		s = sky_max;
	}

	if (t < sky_min)
	{
		t = sky_min;
	}
	else if (t > sky_max)
	{
		t = sky_max;
	}

	t = 1.0 - t;

	VectorCopy(v, vert->pos);

	vert->texCoord[0] = s;
	vert->texCoord[1] = t;

	vert->lmTexCoord[0] = vert->lmTexCoord[1] = 0.0f;
}

void
GL4_DrawSkyBox(void)
{
	int i;

	if (skyrotate)
	{   /* check for no sky at all */
		for (i = 0; i < 6; i++)
		{
			if ((skymins[0][i] < skymaxs[0][i]) &&
			    (skymins[1][i] < skymaxs[1][i]))
			{
				break;
			}
		}

		if (i == 6)
		{
			return; /* nothing visible */
		}
	}

	// glPushMatrix();
	hmm_mat4 origModelMat = gl4state.uni3DData.transModelMat4;

	// glTranslatef(gl4_origin[0], gl4_origin[1], gl4_origin[2]);
	hmm_vec3 transl = HMM_Vec3(gl4_origin[0], gl4_origin[1], gl4_origin[2]);
	hmm_mat4 modMVmat = HMM_MultiplyMat4(origModelMat, HMM_Translate(transl));
	if(skyrotate != 0.0f)
	{
		// glRotatef(gl4_newrefdef.time * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);
		hmm_vec3 rotAxis = HMM_Vec3(skyaxis[0], skyaxis[1], skyaxis[2]);
		modMVmat = HMM_MultiplyMat4(modMVmat, HMM_Rotate(
			(skyautorotate ? gl4_newrefdef.time : 1.f) * skyrotate, rotAxis));
	}
	gl4state.uni3DData.transModelMat4 = modMVmat;
	GL4_UpdateUBO3D();

	GL4_UseProgram(gl4state.si3Dsky.shaderProgram);
	GL4_BindVAO(gl4state.vao3D);
	GL4_BindVBO(gl4state.vbo3D);

	// TODO: this could all be done in one drawcall.. but.. whatever, it's <= 6 drawcalls/frame

	mvtx_t skyVertices[4];

	for (i = 0; i < 6; i++)
	{
		if (skyrotate != 0.0f)
		{
			skymins[0][i] = -1;
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}

		if ((skymins[0][i] >= skymaxs[0][i]) ||
		    (skymins[1][i] >= skymaxs[1][i]))
		{
			continue;
		}

		GL4_Bind(sky_images[skytexorder[i]]->texnum);

		MakeSkyVec( skymins [ 0 ] [ i ], skymins [ 1 ] [ i ], i, &skyVertices[0] );
		MakeSkyVec( skymins [ 0 ] [ i ], skymaxs [ 1 ] [ i ], i, &skyVertices[1] );
		MakeSkyVec( skymaxs [ 0 ] [ i ], skymaxs [ 1 ] [ i ], i, &skyVertices[2] );
		MakeSkyVec( skymaxs [ 0 ] [ i ], skymins [ 1 ] [ i ], i, &skyVertices[3] );

		GL4_BufferAndDraw3D(skyVertices, 4, GL_TRIANGLE_FAN);
	}

	// glPopMatrix();
	gl4state.uni3DData.transModelMat4 = origModelMat;
	GL4_UpdateUBO3D();
}