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
 * Warps. Used on water surfaces und for skybox rotation.
 *
 * =======================================================================
 */

#include "header/local.h"

#define TURBSCALE (256.0 / (2 * M_PI))
#define ON_EPSILON 0.1 /* point on plane side epsilon */
#define MAX_CLIP_VERTS 64

static float skyrotate;
static int skyautorotate;
static vec3_t skyaxis;
static image_t *sky_images[6];
static const int skytexorder[6] = {0, 2, 1, 3, 4, 5};

GLfloat vtx_sky[12];
GLfloat tex_sky[8];

/* 3dstudio environment map names */
static const char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

float r_turbsin[] = {
#include "../constants/warpsin.h"
};

static const int st_to_vec[6][3] = {
	{3, -1, 2},
	{-3, 1, 2},

	{1, 3, 2},
	{-1, -3, 2},

	{-2, -1, 3}, /* 0 degrees yaw, look straight up */
	{2, -1, -3} /* look straight down */
};

static float skymins[2][6], skymaxs[2][6];
static float sky_min, sky_max;

/*
 * Does a water warp on the pre-fragmented mpoly_t chain
 */
void
R_EmitWaterPolys(msurface_t *fa)
{
	mpoly_t *p, *bp;
	mvtx_t *v;
	int i, nv;
	float s, t, os, ot;
	float sscroll, tscroll;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	for (bp = fa->polys; bp; bp = bp->next)
	{
		p = bp;
		nv = p->numverts;
		R_SetBufferIndices(GL_TRIANGLE_FAN, nv);

		for ( i = 0, v = p->verts; i < p->numverts; i++, v++)
		{
			os = v->texCoord[0];
			ot = v->texCoord[1];

			s = os + r_turbsin [ (int) ( ( ot * 0.125 + r_newrefdef.time ) * TURBSCALE ) & 255 ] + sscroll;
			t = ot + r_turbsin [ (int) ( ( os * 0.125 + r_newrefdef.time ) * TURBSCALE ) & 255 ] + tscroll;

			R_BufferVertex( v->pos[0], v->pos[1], v->pos[2] );
			R_BufferSingleTex( s * ( 1.0 / 64 ), t * ( 1.0 / 64 ) );
		}
	}
}

void
RE_AddSkySurface(msurface_t *fa)
{
	R_AddSkySurface(fa, skymins, skymaxs, r_origin);
}

void
RE_ClearSkyBox(void)
{
	R_ClearSkyBox(skymins, skymaxs);
}

static void
RE_MakeSkyVec(float s, float t, int axis, unsigned int *index_tex, unsigned int *index_vtx)
{
	vec3_t v, b;
	int j;

	float dist = (r_farsee->value == 0) ? 2300.0f : 4096.0f;

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

	tex_sky[(*index_tex)++] = s;
	tex_sky[(*index_tex)++] = t;

	vtx_sky[(*index_vtx)++] = v[ 0 ];
	vtx_sky[(*index_vtx)++] = v[ 1 ];
	vtx_sky[(*index_vtx)++] = v[ 2 ];
}

void
R_DrawSkyBox(void)
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

	glPushMatrix();
	glTranslatef(r_origin[0], r_origin[1], r_origin[2]);
	glRotatef((skyautorotate ? r_newrefdef.time : 1.f) * skyrotate,
		skyaxis[0], skyaxis[1], skyaxis[2]);

	for (i = 0; i < 6; i++)
	{
		unsigned int index_vtx = 0, index_tex = 0;

		if (skyrotate)
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

		R_Bind(sky_images[skytexorder[i]]->texnum);

		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );

		RE_MakeSkyVec(skymins[ 0 ][ i ], skymins[ 1 ] [ i ], i, &index_tex, &index_vtx);
		RE_MakeSkyVec(skymins[ 0 ][ i ], skymaxs[ 1 ] [ i ], i, &index_tex, &index_vtx);
		RE_MakeSkyVec(skymaxs[ 0 ][ i ], skymaxs[ 1 ] [ i ], i, &index_tex, &index_vtx);
		RE_MakeSkyVec(skymaxs[ 0 ][ i ], skymins[ 1 ] [ i ], i, &index_tex, &index_vtx);

		glVertexPointer( 3, GL_FLOAT, 0, vtx_sky );
		glTexCoordPointer( 2, GL_FLOAT, 0, tex_sky );
		glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

		glDisableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	glPopMatrix();
}

void
RI_SetSky(const char *name, float rotate, int autorotate, const vec3_t axis)
{
	char	skyname[MAX_QPATH];
	int		i;

	Q_strlcpy(skyname, name, sizeof(skyname));
	skyrotate = rotate;
	skyautorotate = autorotate;
	VectorCopy(axis, skyaxis);

	for (i = 0; i < 6; i++)
	{
		image_t	*image;

		image = (image_t *)GetSkyImage(skyname, suf[i],
			gl_config.palettedtexture, (findimage_t)R_FindImage);

		if (!image)
		{
			R_Printf(PRINT_ALL, "%s: can't load %s:%s sky\n",
				__func__, skyname, suf[i]);
			image = r_notexture;
		}

		sky_images[i] = image;
	}

	sky_min = 1.0 / 512;
	sky_max = 511.0 / 512;
}
