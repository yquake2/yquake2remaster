/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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

#include "../ref_shared.h"

#define ON_EPSILON 0.1 /* point on plane side epsilon */
#define MAX_CLIP_VERTS 64

static const vec3_t skyclip[6] = {
	{1, 1, 0},
	{1, -1, 0},
	{0, -1, 1},
	{0, 1, 1},
	{1, 0, 1},
	{-1, 0, 1}
};

static const int st_to_vec[6][3] = {
	{3, -1, 2},
	{-3, 1, 2},

	{1, 3, 2},
	{-1, -3, 2},

	{-2, -1, 3}, /* 0 degrees yaw, look straight up */
	{2, -1, -3} /* look straight down */
};

static const int vec_to_st[6][3] = {
	{-2, 3, 1},
	{2, 3, -1},

	{1, 3, 2},
	{-1, 3, -2},

	{-2, -1, 3},
	{-2, 1, -3}
};

static void
R_DrawSkyPolygon(int nump, vec3_t vecs, float skymins[2][6], float skymaxs[2][6])
{
	int i;
	vec3_t v, av;
	float s, t, dv;
	int axis;
	float *vp;

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
		int j;

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

void
R_ClipSkyPolygon(int nump, vec3_t vecs, int stage, float skymins[2][6], float skymaxs[2][6])
{
	const float *norm;
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
		Com_Error(ERR_DROP, "%s: MAX_CLIP_VERTS", __func__);
		return;
	}

	if (stage == 6)
	{
		/* fully clipped, so draw it */
		R_DrawSkyPolygon(nump, vecs, skymins, skymaxs);
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
		R_ClipSkyPolygon(nump, vecs, stage + 1, skymins, skymaxs);
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
	R_ClipSkyPolygon(newc[0], newv[0][0], stage + 1, skymins, skymaxs);
	R_ClipSkyPolygon(newc[1], newv[1][0], stage + 1, skymins, skymaxs);
}

void
R_AddSkySurface(msurface_t *fa, float skymins[2][6], float skymaxs[2][6], vec3_t r_origin)
{
	int i;
	vec3_t verts[MAX_CLIP_VERTS];
	mpoly_t *p;

	/* calculate vertex values for sky box */
	for (p = fa->polys; p; p = p->next)
	{
		for (i = 0; i < p->numverts; i++)
		{
			VectorSubtract(p->verts[i].pos, r_origin, verts[i]);
		}

		R_ClipSkyPolygon(p->numverts, verts[0], 0, skymins, skymaxs);
	}
}

void
R_ClearSkyBox(float skymins[2][6], float skymaxs[2][6])
{
	int i;

	for (i = 0; i < 6; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}

void
R_MakeSkyVec(float s, float t, int axis, mvtx_t* vert, qboolean farsee,
	float sky_min, float sky_max)
{
	vec3_t v, b;
	int j;

	float dist = (farsee) ? 2300.0f : 4096.0f;

	b[0] = s * dist;
	b[1] = t * dist;
	b[2] = dist;

	for (j = 0; j < 3; j++)
	{
		int k;

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
R_FlowingScroll(const refdef_t *r_newrefdef, int flags, float *sscroll, float *tscroll)
{
	float multiply = 0.0;

	*sscroll = 0;
	*tscroll = 0;

	if (flags & SURF_DRAWTURB)
	{
		if (flags & SURF_FLOWING)
		{
			multiply = 0.5; // mod 2
		}
	}
	else if (flags & SURF_FLOWING)
	{
		if (flags & SURF_WARP)
		{
			multiply = 0.25; // mod 4
		}
		else
		{
			multiply = 0.025; // mod 40
		}
	}
	else if (flags & (SURF_N64_SCROLL_X | SURF_N64_SCROLL_Y))
	{
		multiply = 0.0125; // mod 80
	}

	if (flags & (SURF_DRAWTURB | SURF_FLOWING | SURF_N64_SCROLL_X))
	{
		*sscroll = -64.0f * ((r_newrefdef->time * multiply) - (int)(r_newrefdef->time * multiply));
	}

	if (flags & SURF_N64_SCROLL_Y)
	{
		*tscroll = -64.0f * ((r_newrefdef->time * multiply) - (int)(r_newrefdef->time * multiply));
	}

	/* Opposite direction with SCROLL_X|Y ?, check at -1714 -824 238 q64/outpost */
	if (!(flags & SURF_N64_SCROLL_FLIP) && (flags & (SURF_N64_SCROLL_Y | SURF_N64_SCROLL_X)))
	{
		*sscroll = -64 - *sscroll;
		*tscroll = -64 - *tscroll;
	}

	if (*sscroll >= 0.0)
	{
		*sscroll = -64.0;
	}

	if (*tscroll >= 0.0)
	{
		*tscroll = -64.0;
	}
}
