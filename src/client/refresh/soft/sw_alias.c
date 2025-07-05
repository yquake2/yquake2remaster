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
 */

/* sw_alias.c: routines for setting up to draw alias models */

/*
** use a real variable to control lerping
*/
#include <stdint.h>
#include "header/local.h"

#define LIGHT_MIN	5	// lowest light value we'll allow, to avoid the
				//  need for inner-loop light clamping

int				r_amodels_drawn;

affinetridesc_t	r_affinetridesc;

static vec3_t	r_plightvec;
static vec3_t	r_lerp_frontv, r_lerp_backv, r_lerp_move;

static light3_t	r_ambientlight;
int		r_aliasblendcolor;
static vec3_t		r_shadelight;


static daliasxframe_t	*r_thisframe, *r_lastframe;
static dmdx_t	*s_pmdl;

static float	aliastransform[3][4];
static float	aliasworldtransform[3][4];
static float	aliasoldworldtransform[3][4];

static float	s_ziscale;
static vec3_t	s_alias_forward, s_alias_right, s_alias_up;

/*
================
R_AliasCheckBBox
================
*/

#define BBOX_TRIVIAL_ACCEPT 0
#define BBOX_MUST_CLIP_XY   1
#define BBOX_MUST_CLIP_Z    2
#define BBOX_TRIVIAL_REJECT 8

/*
================
R_AliasTransformVector
================
*/
static void
R_AliasTransformVector(const vec3_t in, vec3_t out, const float xf[3][4] )
{
	out[0] = DotProduct(in, xf[0]) + xf[0][3];
	out[1] = DotProduct(in, xf[1]) + xf[1][3];
	out[2] = DotProduct(in, xf[2]) + xf[2][3];
}

/*
 * R_AliasCheckFrameBBox
 *
 * Checks a specific alias frame bounding box
 *
 * TODO: Combine with R_CullAliasMeshModel
*/
static unsigned long
R_AliasCheckFrameBBox( daliasxframe_t *frame, float worldxf[3][4] )
{
	// FIXME: should this really be using long and not int32_t or sth?
	unsigned long aggregate_and_clipcode = ~0U,
		          aggregate_or_clipcode = 0;
	int           i;
	vec3_t        mins, maxs;
	vec3_t        transformed_min, transformed_max;
	qboolean      zfullyclipped = true;

	/*
	** get the exact frame bounding box
	*/
	for (i=0 ; i<3 ; i++)
	{
		mins[i] = frame->translate[i];
		maxs[i] = mins[i] + frame->scale[i] * 0xFFFF;
	}

	/*
	** transform the min and max values into view space
	*/
	R_AliasTransformVector( mins, transformed_min, aliastransform );
	R_AliasTransformVector( maxs, transformed_max, aliastransform );

	if ( transformed_min[2] >= ALIAS_Z_CLIP_PLANE )
	{
		zfullyclipped = false;
	}

	if ( transformed_max[2] >= ALIAS_Z_CLIP_PLANE )
	{
		zfullyclipped = false;
	}

	if ( zfullyclipped )
	{
		return BBOX_TRIVIAL_REJECT;
	}

	/*
	** build a transformed bounding box from the given min and max
	*/
	for ( i = 0; i < 8; i++ )
	{
		int      j;
		vec3_t   tmp, transformed;
		unsigned long clipcode = 0;

		if ( i & 1 )
		{
			tmp[0] = mins[0];
		}
		else
		{
			tmp[0] = maxs[0];
		}

		if ( i & 2 )
		{
			tmp[1] = mins[1];
		}
		else
		{
			tmp[1] = maxs[1];
		}

		if ( i & 4 )
		{
			tmp[2] = mins[2];
		}
		else
		{
			tmp[2] = maxs[2];
		}

		R_AliasTransformVector( tmp, transformed, worldxf );

		for ( j = 0; j < 4; j++ )
		{
			float dp = DotProduct( transformed, view_clipplanes[j].normal );

			if ( ( dp - view_clipplanes[j].dist ) < 0.0F )
			{
				clipcode |= 1 << j;
			}
		}

		aggregate_and_clipcode &= clipcode;
		aggregate_or_clipcode  |= clipcode;
	}

	if ( aggregate_and_clipcode )
	{
		return BBOX_TRIVIAL_REJECT;
	}
	if ( !aggregate_or_clipcode )
	{
		return BBOX_TRIVIAL_ACCEPT;
	}

	return BBOX_MUST_CLIP_XY;
}

static int
R_AliasCheckBBox (const entity_t *currententity)
{
	unsigned long ccodes[2] = { 0, 0 };

	ccodes[0] = R_AliasCheckFrameBBox(r_thisframe, aliasworldtransform);

	/*
	** non-lerping model
	*/
	if ( currententity->backlerp == 0 )
	{
		if (ccodes[0] == BBOX_TRIVIAL_ACCEPT)
		{
			return BBOX_TRIVIAL_ACCEPT;
		}
		else if (ccodes[0] & BBOX_TRIVIAL_REJECT)
		{
			return BBOX_TRIVIAL_REJECT;
		}
		else
		{
			return (ccodes[0] & ~BBOX_TRIVIAL_REJECT);
		}
	}

	ccodes[1] = R_AliasCheckFrameBBox(r_lastframe, aliasoldworldtransform);

	if ((ccodes[0] | ccodes[1]) == BBOX_TRIVIAL_ACCEPT)
	{
		return BBOX_TRIVIAL_ACCEPT;
	}
	else if ((ccodes[0] & ccodes[1]) & BBOX_TRIVIAL_REJECT)
	{
		return BBOX_TRIVIAL_REJECT;
	}
	else
	{
		return (ccodes[0] | ccodes[1]) & ~BBOX_TRIVIAL_REJECT;
	}
}

/*
================
R_AliasProjectAndClipTestFinalVert
================
*/
void
R_AliasProjectAndClipTestFinalVert(finalvert_t *fv)
{
	float	zi;
	float	x, y, z;

	// project points
	x = fv->xyz[0];
	y = fv->xyz[1];
	z = fv->xyz[2];
	zi = 1.0 / z;

	fv->cv.zi = zi * s_ziscale;

	fv->cv.u = (x * aliasxscale * zi) + aliasxcenter;
	fv->cv.v = (y * aliasyscale * zi) + aliasycenter;

	if (fv->cv.u < r_refdef.aliasvrect.x)
	{
		fv->flags |= ALIAS_LEFT_CLIP;
	}

	if (fv->cv.v < r_refdef.aliasvrect.y)
	{
		fv->flags |= ALIAS_TOP_CLIP;
	}

	if (fv->cv.u > r_refdef.aliasvrectright)
	{
		fv->flags |= ALIAS_RIGHT_CLIP;
	}

	if (fv->cv.v > r_refdef.aliasvrectbottom)
	{
		fv->flags |= ALIAS_BOTTOM_CLIP;
	}
}

/*
================
R_AliasTransformFinalVerts

================
*/
static void
R_AliasTransformFinalVerts(int numpoints, finalvert_t *fv, dxtrivertx_t *newv, float *lerp)
{
	int i;

	for (i = 0; i < numpoints; i++, fv++, newv++, lerp += 4)
	{
		vec3_t plightnormal;
		float	lightcos;
		int n;

		for (n = 0; n < 3; n++)
		{
			plightnormal[n] = newv->normal[n] / 127.f;
		}

		fv->xyz[0] = DotProduct(lerp, aliastransform[0]) + aliastransform[0][3];
		fv->xyz[1] = DotProduct(lerp, aliastransform[1]) + aliastransform[1][3];
		fv->xyz[2] = DotProduct(lerp, aliastransform[2]) + aliastransform[2][3];

		fv->flags = 0;

		// lighting
		lightcos = DotProduct(plightnormal, r_plightvec);

		if (lightcos < 0)
		{
			int		j;

			for(j=0; j<3; j++)
			{
				int temp;

				temp = r_ambientlight[j];

				temp += (r_shadelight[j] * lightcos);

				// clamp; because we limited the minimum ambient and shading light, we
				// don't have to clamp low light, just bright
				if (temp < 0)
				{
					temp = 0;
				}

				fv->cv.l[j] = temp;
			}
		}
		else
		{
			memcpy(fv->cv.l, r_ambientlight, sizeof(light3_t));
		}

		if (fv->xyz[2] < ALIAS_Z_CLIP_PLANE)
		{
			fv->flags |= ALIAS_Z_CLIP;
		}
		else
		{
			R_AliasProjectAndClipTestFinalVert(fv);
		}
	}
}

static void
R_AliasPrepareMeshPoints(const entity_t *currententity, const dstvert_t *pstverts, finalvert_t *verts, const dtriangle_t *ptri, const dtriangle_t *ptri_end)
{
	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value == 1.0F ) )
	{
		while(ptri < ptri_end)
		{
			finalvert_t	*pfv[3];

			pfv[0] = &verts[ptri->index_xyz[0]];
			pfv[1] = &verts[ptri->index_xyz[1]];
			pfv[2] = &verts[ptri->index_xyz[2]];

			if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags )
			{
				ptri++;

				continue;	/* completely clipped */
			}

			/* insert s/t coordinates */
			pfv[0]->cv.s = pstverts[ptri->index_st[0]].s << SHIFT16XYZ;
			pfv[0]->cv.t = pstverts[ptri->index_st[0]].t << SHIFT16XYZ;

			pfv[1]->cv.s = pstverts[ptri->index_st[1]].s << SHIFT16XYZ;
			pfv[1]->cv.t = pstverts[ptri->index_st[1]].t << SHIFT16XYZ;

			pfv[2]->cv.s = pstverts[ptri->index_st[2]].s << SHIFT16XYZ;
			pfv[2]->cv.t = pstverts[ptri->index_st[2]].t << SHIFT16XYZ;

			if ( ! (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) )
			{
				/* totally unclipped */
				R_DrawTriangle(currententity, pfv[2], pfv[1], pfv[0]);
			}
			else
			{
				R_AliasClipTriangle(currententity, pfv[2], pfv[1], pfv[0]);
			}

			ptri++;
		}
	}
	else
	{
		while(ptri < ptri_end)
		{
			finalvert_t	*pfv[3];

			pfv[0] = &verts[ptri->index_xyz[0]];
			pfv[1] = &verts[ptri->index_xyz[1]];
			pfv[2] = &verts[ptri->index_xyz[2]];

			if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags )
			{
				ptri++;

				continue;	/* completely clipped */
			}

			/* insert s/t coordinates */
			pfv[0]->cv.s = pstverts[ptri->index_st[0]].s << SHIFT16XYZ;
			pfv[0]->cv.t = pstverts[ptri->index_st[0]].t << SHIFT16XYZ;

			pfv[1]->cv.s = pstverts[ptri->index_st[1]].s << SHIFT16XYZ;
			pfv[1]->cv.t = pstverts[ptri->index_st[1]].t << SHIFT16XYZ;

			pfv[2]->cv.s = pstverts[ptri->index_st[2]].s << SHIFT16XYZ;
			pfv[2]->cv.t = pstverts[ptri->index_st[2]].t << SHIFT16XYZ;

			if ( ! (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) )
			{
				/* totally unclipped */
				R_DrawTriangle(currententity, pfv[0], pfv[1], pfv[2]);
			}
			else
			{	/* partially clipped */
				R_AliasClipTriangle(currententity, pfv[0], pfv[1], pfv[2]);
			}

			ptri++;
		}
	}
}

/*
================
R_AliasPreparePoints

General clipped case
================
*/

static void
R_AliasPreparePoints(const entity_t *currententity, finalvert_t *verts, const finalvert_t *verts_max)
{
	const dstvert_t *pstverts;
	dmdxmesh_t *mesh_nodes;
	int i, num_mesh_nodes;
	qboolean colorOnly;
	vec4_t *s_lerped;

	if ((verts + s_pmdl->num_xyz) >= verts_max)
	{
		r_outofverts = true;
		return;
	}

	/* buffer for scalled vert from frame */
	s_lerped = R_VertBufferRealloc(s_pmdl->num_xyz);
	colorOnly = 0 != (currententity->flags &
		(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE |
		 RF_SHELL_HALF_DAM));

	R_LerpVerts(colorOnly, s_pmdl->num_xyz, r_thisframe->verts, r_lastframe->verts,
		s_lerped[0], r_lerp_move, r_lerp_frontv, r_lerp_backv, currententity->scale);

	R_AliasTransformFinalVerts(s_pmdl->num_xyz,
		verts,	/* destination for transformed verts */
		r_thisframe->verts,	/* verts from this frame */
		s_lerped[0]);

	// clip and draw all triangles
	//
	pstverts = (dstvert_t *)((byte *)s_pmdl + s_pmdl->ofs_st);

	num_mesh_nodes = s_pmdl->num_meshes;
	mesh_nodes = (dmdxmesh_t *)((char*)s_pmdl + s_pmdl->ofs_meshes);

	for (i = 0; i < num_mesh_nodes; i++)
	{
		const dtriangle_t *ptri;
		int num_tris;

		if (currententity->rr_mesh & (1 << i))
		{
			continue;
		}

		num_tris = Q_min(s_pmdl->num_tris - mesh_nodes[i].ofs_tris, mesh_nodes[i].num_tris);
		ptri = (dtriangle_t *)((byte *)s_pmdl + s_pmdl->ofs_tris) + mesh_nodes[i].ofs_tris;

		R_AliasPrepareMeshPoints(currententity, pstverts, verts, ptri, ptri + num_tris);
	}
}


/*
================
R_AliasSetUpTransform
================
*/
static void
R_AliasSetUpTransform(const entity_t *currententity)
{
	int				i;
	static float	viewmatrix[3][4];

	// TODO: should really be stored with the entity instead of being reconstructed
	// TODO: should use a look-up table
	// TODO: could cache lazily, stored in the entity
	//
	// AngleVectors never change angles, we can convert from const
	AngleVectors((float *)currententity->angles, s_alias_forward, s_alias_right, s_alias_up );

	// TODO: can do this with simple matrix rearrangement
	memset( aliasworldtransform, 0, sizeof( aliasworldtransform ) );
	memset( aliasoldworldtransform, 0, sizeof( aliasworldtransform ) );

	for (i=0 ; i<3 ; i++)
	{
		aliasoldworldtransform[i][0] = aliasworldtransform[i][0] =  s_alias_forward[i];
		aliasoldworldtransform[i][1] = aliasworldtransform[i][1] = -s_alias_right[i];
		aliasoldworldtransform[i][2] = aliasworldtransform[i][2] =  s_alias_up[i];
	}

	aliasworldtransform[0][3] = currententity->origin[0]-r_origin[0];
	aliasworldtransform[1][3] = currententity->origin[1]-r_origin[1];
	aliasworldtransform[2][3] = currententity->origin[2]-r_origin[2];

	aliasoldworldtransform[0][3] = currententity->oldorigin[0]-r_origin[0];
	aliasoldworldtransform[1][3] = currententity->oldorigin[1]-r_origin[1];
	aliasoldworldtransform[2][3] = currententity->oldorigin[2]-r_origin[2];

	// FIXME: can do more efficiently than full concatenation
	// memcpy( rotationmatrix, t2matrix, sizeof( rotationmatrix ) );

	// R_ConcatTransforms (t2matrix, tmatrix, rotationmatrix);

	// TODO: should be global, set when vright, etc., set
	VectorCopy (vright, viewmatrix[0]);
	VectorCopy (vup, viewmatrix[1]);
	VectorInverse (viewmatrix[1]);
	VectorCopy (vpn, viewmatrix[2]);

	if ( currententity->flags & RF_WEAPONMODEL )
	{
		viewmatrix[0][3] = 0;
		viewmatrix[1][3] = 0;
		viewmatrix[2][3] = sw_gunzposition->value;
	}
	else
	{
		viewmatrix[0][3] = 0;
		viewmatrix[1][3] = 0;
		viewmatrix[2][3] = 0;
	}

	// memcpy( aliasworldtransform, rotationmatrix, sizeof( aliastransform ) );

	R_ConcatTransforms (viewmatrix, aliasworldtransform, aliastransform);

	aliasworldtransform[0][3] = currententity->origin[0];
	aliasworldtransform[1][3] = currententity->origin[1];
	aliasworldtransform[2][3] = currententity->origin[2];

	aliasoldworldtransform[0][3] = currententity->oldorigin[0];
	aliasoldworldtransform[1][3] = currententity->oldorigin[1];
	aliasoldworldtransform[2][3] = currententity->oldorigin[2];
}

/*
===============
R_AliasSetupSkin
===============
*/
static qboolean
R_AliasSetupSkin(const entity_t *currententity, const model_t *currentmodel)
{
	image_t *pskindesc;

	if (currententity->skin)
	{
		pskindesc = currententity->skin;
	}
	else
	{
		int skinnum;

		if (!currentmodel->numskins)
		{
			return false;
		}

		skinnum = currententity->skinnum;
		if ((skinnum >= s_pmdl->num_skins) || (skinnum < 0) ||
			(skinnum >= currentmodel->numskins))
		{
			R_Printf(PRINT_ALL, "%s %s: no such skin # %d\n",
				__func__, currentmodel->name, skinnum);
			skinnum = 0;
		}

		pskindesc = currentmodel->skins[skinnum];
	}

	if (!pskindesc)
	{
		return false;
	}

	r_affinetridesc.pskin = pskindesc->pixels[0];
	r_affinetridesc.skinwidth = pskindesc->asset_width;
	r_affinetridesc.skinheight = pskindesc->asset_height;
	r_affinetridesc.scalewidth = (float)pskindesc->asset_width / s_pmdl->skinwidth;
	r_affinetridesc.scaleheight =  (float)pskindesc->asset_height / s_pmdl->skinheight;

	return true;
}


/*
================
R_AliasSetupLighting

  FIXME: put lighting into tables
================
*/
static void
R_AliasSetupLighting(entity_t *currententity)
{
	const vec3_t lightvec = {-1, 0, 0};
	vec3_t light;
	int i;

	// all components of light should be identical in software
	if (currententity->flags & RF_FULLBRIGHT || !r_worldmodel || !r_worldmodel->lightdata)
	{
		for (i = 0; i < 3; i++)
		{
			light[i] = 1.0;
		}
	}
	else
	{
		R_LightPoint(r_worldmodel->grid, currententity, &r_newrefdef, r_worldmodel->surfaces,
			r_worldmodel->nodes, currententity->origin, light,
			r_modulate->value, lightspot);
	}

	// save off light value for server to look at (BIG HACK!)
	if (currententity->flags & RF_WEAPONMODEL)
	{
		r_lightlevel->value = 150.0 * light[0];
	}

	if ( currententity->flags & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (light[i] < 0.1)
				light[i] = 0.1;
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;

		scale = 0.1 * sin(r_newrefdef.time*7);
		for (i=0 ; i<3 ; i++)
		{
			float min;

			min = light[i] * 0.8;
			light[i] += scale;
			if (light[i] < min)
				light[i] = min;
		}
	}

	if(r_colorlight->value == 0)
	{
		float temp = (light[0] + light[1] + light[2]) / 3.0;

		light[0] = light[1] = light[2] = temp;
	}

	for(i=0; i<3; i++)
	{
		int j;

		j = light[i] * 255;

		r_ambientlight[i] = j;
		r_shadelight[i] = j;

		// clamp lighting so it doesn't overbright as much
		if (r_ambientlight[i] > 128)
			r_ambientlight[i] = 128;
		if (r_ambientlight[i] + r_shadelight[i] > 192)
			r_shadelight[i] = 192 - r_ambientlight[i];

		// guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have
		// to clamp off the bottom
		if (r_ambientlight[i] < LIGHT_MIN)
			r_ambientlight[i] = LIGHT_MIN;

		r_ambientlight[i] = (255 - r_ambientlight[i]) << VID_CBITS;

		if (r_shadelight[i] < 0)
			r_shadelight[i] = 0;

		r_shadelight[i] *= VID_GRADES;
	}

	// rotate the lighting vector into the model's frame of reference
	r_plightvec[0] =  DotProduct( lightvec, s_alias_forward );
	r_plightvec[1] = -DotProduct( lightvec, s_alias_right );
	r_plightvec[2] =  DotProduct( lightvec, s_alias_up );
}


/*
=================
R_AliasSetupFrames

=================
*/
static void
R_AliasSetupFrames(const entity_t *currententity, const model_t *currentmodel, dmdx_t *pmdl)
{
	int thisframe = currententity->frame;
	int lastframe = currententity->oldframe;

	if ( ( thisframe >= pmdl->num_frames ) || ( thisframe < 0 ) )
	{
		R_Printf(PRINT_ALL, "%s %s: no such thisframe %d\n",
			__func__, currentmodel->name, thisframe);
		thisframe = 0;
	}
	if ( ( lastframe >= pmdl->num_frames ) || ( lastframe < 0 ) )
	{
		R_Printf(PRINT_ALL, "%s %s: no such lastframe %d\n",
			__func__, currentmodel->name, lastframe);
		lastframe = 0;
	}

	r_thisframe = (daliasxframe_t *)((byte *)pmdl + pmdl->ofs_frames
		+ thisframe * pmdl->framesize);

	r_lastframe = (daliasxframe_t *)((byte *)pmdl + pmdl->ofs_frames
		+ lastframe * pmdl->framesize);
}

/*
** R_AliasSetUpLerpData
**
** Precomputes lerp coefficients used for the whole frame.
*/
static void
R_AliasSetUpLerpData(entity_t *currententity, dmdx_t *pmdl, float backlerp)
{
	float	frontlerp;
	vec3_t	translation, vectors[3];
	int		i;

	frontlerp = 1.0F - backlerp;

	/*
	** convert entity's angles into discrete vectors for R, U, and F
	*/
	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	/*
	** translation is the vector from last position to this position
	*/
	VectorSubtract (currententity->oldorigin, currententity->origin, translation);

	/*
	** move should be the delta back to the previous frame * backlerp
	*/
	r_lerp_move[0] =  DotProduct(translation, vectors[0]);	// forward
	r_lerp_move[1] = -DotProduct(translation, vectors[1]);	// left
	r_lerp_move[2] =  DotProduct(translation, vectors[2]);	// up

	VectorAdd( r_lerp_move, r_lastframe->translate, r_lerp_move );

	for (i=0 ; i<3 ; i++)
	{
		r_lerp_move[i] = backlerp*r_lerp_move[i] + frontlerp * r_thisframe->translate[i];
	}

	for (i=0 ; i<3 ; i++)
	{
		r_lerp_frontv[i] = frontlerp * r_thisframe->scale[i];
		r_lerp_backv[i]  = backlerp  * r_lastframe->scale[i];
	}
}

finalvert_t *finalverts = NULL, *finalverts_max = NULL;

extern void (*d_pdrawspans)(const entity_t *currententity, spanpackage_t *pspanpackage);
void R_PolysetDrawSpans8_Opaque(const entity_t *currententity, spanpackage_t *pspanpackage);
void R_PolysetDrawSpans8_33(const entity_t *currententity, spanpackage_t *pspanpackage);
void R_PolysetDrawSpans8_66(const entity_t *currententity, spanpackage_t *pspanpackage);
void R_PolysetDrawSpansConstant8_33(const entity_t *currententity, spanpackage_t *pspanpackage);
void R_PolysetDrawSpansConstant8_66(const entity_t *currententity, spanpackage_t *pspanpackage);

/*
================
R_DrawAliasModel
================
*/
void
R_DrawAliasModel(entity_t *currententity, const model_t *currentmodel)
{
	int i;

	s_pmdl = (dmdx_t *)currentmodel->extradata;

	if ( r_lerpmodels->value == 0 )
	{
		currententity->backlerp = 0;
	}

	float oldAliasxscale = aliasxscale;
	float oldAliasyscale = aliasyscale;

	if (currententity->flags & RF_WEAPONMODEL)
	{
		if (r_lefthand->value == 2.0F)
		{
			return;
		}

		if (r_gunfov->value >= 0)
		{
			float gunfov = 2 * tan((float)r_gunfov->value / 360 * M_PI);
			aliasxscale = ((float)r_refdef.vrect.width / gunfov) * r_aliasuvscale;
			aliasyscale = aliasxscale;
		}

		if ( r_lefthand->value == 1.0F )
			aliasxscale = -aliasxscale;
	}

	for (i = 0; i < 3; i++)
	{
		/* fix scale */
		if (!currententity->scale[i])
		{
			currententity->scale[i] = 1.0f;
		}
	}

	/*
	** we have to set our frame pointers and transformations before
	** doing any real work
	*/
	R_AliasSetupFrames(currententity, currentmodel, s_pmdl);
	R_AliasSetUpTransform(currententity);

	// see if the bounding box lets us trivially reject, also sets
	// trivial accept status
	if ( R_AliasCheckBBox(currententity) == BBOX_TRIVIAL_REJECT )
	{
		if ( currententity->flags & RF_WEAPONMODEL )
		{
			aliasxscale = oldAliasxscale;
			aliasyscale = oldAliasyscale;
		}
		return;
	}

	// set up the skin and verify it exists
	if (!R_AliasSetupSkin(currententity, currentmodel))
	{
		R_Printf(PRINT_ALL, "%s %s: NULL skin found\n",
			__func__, currentmodel->name);
		aliasxscale = oldAliasxscale;
		aliasyscale = oldAliasyscale;
		return;
	}

	r_amodels_drawn++;
	R_AliasSetupLighting(currententity);

	/*
	** select the proper span routine based on translucency
	*/
	// added double damage shell
	// reordered to handle blending
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
	{
		int		color;

		// added double
		color = currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM);

		// reordered, new shells after old shells (so they get overriden)
		if ( color == RF_SHELL_RED )
			r_aliasblendcolor = SHELL_RED_COLOR;
		else if ( color == RF_SHELL_GREEN )
			r_aliasblendcolor = SHELL_GREEN_COLOR;
		else if ( color == RF_SHELL_BLUE )
			r_aliasblendcolor = SHELL_BLUE_COLOR;
		else if ( color == (RF_SHELL_RED | RF_SHELL_GREEN) )
			r_aliasblendcolor = SHELL_RG_COLOR;
		else if ( color == (RF_SHELL_RED | RF_SHELL_BLUE) )
			r_aliasblendcolor = SHELL_RB_COLOR;
		else if ( color == (RF_SHELL_BLUE | RF_SHELL_GREEN) )
			r_aliasblendcolor = SHELL_BG_COLOR;
		// added this .. it's yellowish
		else if ( color == (RF_SHELL_DOUBLE) )
			r_aliasblendcolor = SHELL_DOUBLE_COLOR;
		else if ( color == (RF_SHELL_HALF_DAM) )
			r_aliasblendcolor = SHELL_HALF_DAM_COLOR;
		else
			r_aliasblendcolor = SHELL_WHITE_COLOR;

		if ( currententity->alpha > 0.33 )
			d_pdrawspans = R_PolysetDrawSpansConstant8_66;
		else
			d_pdrawspans = R_PolysetDrawSpansConstant8_33;
	}
	else if ( currententity->flags & RF_TRANSLUCENT )
	{
		if ( currententity->alpha > 0.66 )
			d_pdrawspans = R_PolysetDrawSpans8_Opaque;
		else if ( currententity->alpha > 0.33 )
			d_pdrawspans = R_PolysetDrawSpans8_66;
		else
			d_pdrawspans = R_PolysetDrawSpans8_33;
	}
	else
	{
		d_pdrawspans = R_PolysetDrawSpans8_Opaque;
	}

	/*
	** compute this_frame and old_frame addresses
	*/
	R_AliasSetUpLerpData(currententity, s_pmdl, currententity->backlerp);

	if (currententity->flags & RF_DEPTHHACK)
		s_ziscale = (float)0x8000 * (float)SHIFT16XYZ_MULT * 3.0;
	else
		s_ziscale = (float)0x8000 * (float)SHIFT16XYZ_MULT;

	R_AliasPreparePoints(currententity, finalverts, finalverts_max);

	if ( currententity->flags & RF_WEAPONMODEL )
	{
		aliasxscale = oldAliasxscale;
		aliasyscale = oldAliasyscale;
	}
}
