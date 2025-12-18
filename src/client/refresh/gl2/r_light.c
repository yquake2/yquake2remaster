/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_light.c - dynamic lights and light sampling from lightmap

#include "r_local.h"

int	r_dlightframecount;

#define	DLIGHT_CUTOFF	64

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights(void)
{
#if 0
	int		i;
	dlight_t* l;

	l = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
	}
#endif
}



/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dlight_t* light, vec3_t lightorg, int bit, mnode_t* node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	
	if (node->contents != -1)
		return;

	splitplane = node->plane;

	dist = DotProduct(lightorg, splitplane->normal) - splitplane->dist;
	
	if (dist > light->intensity-DLIGHT_CUTOFF)
	{
		R_MarkLights(light, lightorg, bit, node->children[0]);
		return;
	}
	if (dist < -light->intensity+DLIGHT_CUTOFF)
	{
		R_MarkLights(light, lightorg, bit, node->children[1]);	
		return;
	}
		
// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}
	R_MarkLights(light, lightorg, bit, node->children[0]);
	R_MarkLights(light, lightorg, bit, node->children[1]);
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int			i;
	dlight_t	*l;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't advanced yet for this frame
	
	l = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_MarkLights(l, l->origin, 1 << i, r_worldmodel->nodes);
	}

	// braxi -- moved dlight shader updates to R_UpdateCommonProgUniforms()
}

/*
=================
R_SendDynamicLightsToCurrentProgram
=================
*/
void R_SendDynamicLightsToCurrentProgram()
{
	dlight_t* dlight;
	int			i, j;
	vec4_t		dl_pos_and_rad[MAX_DLIGHTS]; // holds xyz + radius for each light
	vec4_t		dl_dir_and_cutoff[MAX_DLIGHTS]; // holds xyz direction + spot cutoff for each light
	vec3_t		dl_colors[MAX_DLIGHTS];
	int			numDynLights;

	if (!r_worldmodel || !R_UsingProgram())
		return;

	numDynLights = !r_dynamic->value ? 0 : r_newrefdef.num_dlights; // no dlights when r_dynamic is off

	dlight = r_newrefdef.dlights;
	for (i = 0; i < numDynLights; i++, dlight++)
	{
//		if (dlight->intensity <= DLIGHT_CUTOFF)
//			continue;

		for (j = 0; j < 3; j++)
			dl_pos_and_rad[i][j] = dlight->origin[j];
		dl_pos_and_rad[i][3] = dlight->intensity;

		VectorCopy(dlight->color, dl_colors[i]);

		//Notes about spotlights:
		//xyz must be normalized. (this could be done in shader if really needed)
		//xyz should probably be valid, even if spotlights aren't being used.
		//spot cutoff is in the range -1 (infinitely small cone) to 1 (disable spotlight entirely)

#if 0 // test
		srand(i);
		dlight->type = DL_SPOTLIGHT;
		for (j = 0; j < 3; j++)
		{
			dl_dir_and_cutoff[i][j] = rand() / (float)RAND_MAX * 2 - 1;
			VectorNormalize(dl_dir_and_cutoff[i]);
		}
		float coff = -rand() / (float)RAND_MAX;
		dl_dir_and_cutoff[i][3] = -0.95;
#else
		if (dlight->type == DL_SPOTLIGHT)
		{
			for (j = 0; j < 3; j++)
			{
				dl_dir_and_cutoff[i][j] = dlight->dir[j];
				//VectorNormalize(dl_dir_and_cutoff[i]);
			}

			dl_dir_and_cutoff[i][3] = dlight->cutoff;
		}
		else
		{
			//In the absence of spotlights, set these to values that disable spotlights
			dl_dir_and_cutoff[i][0] = dl_dir_and_cutoff[i][3] = 1.f;
			dl_dir_and_cutoff[i][1] = dl_dir_and_cutoff[i][2] = 0.f;
		}
#endif

	}

	R_ProgUniform1i(LOC_DLIGHT_COUNT, numDynLights);
	if (numDynLights > 0)
	{
		R_ProgUniform3fv(LOC_DLIGHT_COLORS, numDynLights, &dl_colors[0][0]);
		R_ProgUniform4fv(LOC_DLIGHT_POS_AND_RAD, numDynLights, &dl_pos_and_rad[0][0]);
		R_ProgUniform4fv(LOC_DLIGHT_DIR_AND_CUTOFF, numDynLights, &dl_dir_and_cutoff[0][0]);
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

static vec3_t		pointcolor;
static cplane_t		*lightplane;	// used as shadow plane
static vec3_t		lightspot;

static int R_RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side, maps, r, i;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	mtexinfo_t	*tex;
	byte		*lightmap;

	if (node->contents != -1)
		return -1; // didn't hit anything
	
	//
	// calculate mid point
	//
	
	// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return R_RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	//
	// go down front side	
	//
	r = R_RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r; // hit something
		
	if ( (back < 0) == side )
		return -1; // didn't hit anything
		
	//
	// check for impact on this node
	//
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;	// no lightmaps

		tex = surf->texinfo;
		lightmap = surf->samples;

#ifdef DECOUPLED_LM
		s = DotProduct(mid, surf->lmvecs[0]) + surf->lmvecs[0][3];
		t = DotProduct(mid, surf->lmvecs[1]) + surf->lmvecs[1][3];
#else
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;
#endif

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;

//#ifdef DECOUPLED_LM
//		if (s >= surf->lm_width || t >= surf->lm_height)
//			continue; // this code will bust maps with no decoupledlm (in case you compile engine without standard bsp support)
//#endif
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= surf->lmshift;
		dt >>= surf->lmshift;
		
		VectorCopy (vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3 * (dt * ((surf->extents[0] >> surf->lmshift) + 1) + ds);

			for (maps = 0 ; maps < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[maps] != 255 ; maps++)
			{
				if (r_dynamic->value)
				{
					for (i = 0; i < 3; i++)
						scale[i] = r_modulate->value * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

					pointcolor[0] += lightmap[0] * scale[0] * (1.0 / 255);
					pointcolor[1] += lightmap[1] * scale[1] * (1.0 / 255);
					pointcolor[2] += lightmap[2] * scale[2] * (1.0 / 255);
				}
				else
				{
					pointcolor[0] += lightmap[0] * r_modulate->value * (1.0 / 255);
					pointcolor[1] += lightmap[1] * r_modulate->value * (1.0 / 255);
					pointcolor[2] += lightmap[2] * r_modulate->value * (1.0 / 255);
				}

				lightmap += 3 * ((surf->extents[0] >> surf->lmshift) + 1) * ((surf->extents[1] >> surf->lmshift) + 1);
			}
		}	
		return 1;
	}

// go down back side
	return R_RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightPoint

Returns the lightmap pixel color under p
===============
*/
void R_LightPoint(vec3_t p, vec3_t color)
{
	vec3_t		end;
	float		r;
	
	if (!r_worldmodel->lightdata || r_worldmodel->lightdatasize <= 0)
	{
		VectorSet(color, 1.0f, 1.0f, 1.0f); // fullbright
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048; // go this far down

	//
	// find lightmap pixel color underneath p
	//
	r = R_RecursiveLightPoint (r_worldmodel->nodes, p, end);
	
	if (r == -1) // nothing was found
		VectorCopy (vec3_origin, color);
	else
		VectorCopy (pointcolor, color);

	// scale the light color with r_modulate cvar
	VectorScale (color, r_modulate->value, color);
}
