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
 * Surface generation and drawing
 *
 * =======================================================================
 */

#include <assert.h>

#include "header/local.h"

int c_visible_lightmaps;
int c_visible_textures;
static vec3_t modelorg; /* relative to viewpoint */
msurface_t *r_alpha_surfaces;

vklightmapstate_t vk_lms;

static void
DrawVkPoly(msurface_t *fa, image_t *texture, const float *color)
{
	float sscroll, tscroll;
	mpoly_t *p;
	int i;

	p = fa->polys;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &sscroll, &tscroll);

	if (Mesh_VertsRealloc(p->numverts))
	{
		Com_Error(ERR_FATAL, "%s: can't allocate memory", __func__);
	}

	memcpy(verts_buffer, p->verts, sizeof(mvtx_t) * p->numverts);
	for (i = 0; i < p->numverts; i++)
	{
		verts_buffer[i].texCoord[0] += sscroll;
		verts_buffer[i].texCoord[1] += tscroll;
	}

	QVk_BindPipeline(&vk_drawPolyPipeline);

	VkDeviceSize vboOffset, dstOffset;
	VkBuffer vbo, *buffer;
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData = QVk_GetVertexBuffer(sizeof(mvtx_t) * p->numverts, &vbo, &vboOffset);
	uint8_t *uboData  = QVk_GetUniformBuffer(sizeof(float) * 4, &uboOffset, &uboDescriptorSet);
	memcpy(vertData, verts_buffer, sizeof(mvtx_t) * p->numverts);
	memcpy(uboData,  color, sizeof(float) * 4);

	VkDescriptorSet descriptorSets[] = {
		texture->vk_texture.descriptorSet,
		uboDescriptorSet
	};

	float gamma = 2.1F - vid_gamma->value;

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, PUSH_CONSTANT_VERTEX_SIZE * sizeof(float), sizeof(gamma), &gamma);

	Mesh_VertsRealloc((p->numverts - 2) * 3);
	GenFanIndexes(vertIdxData, 0, p->numverts - 2);
	buffer = UpdateIndexBuffer(vertIdxData, (p->numverts - 2) * 3 * sizeof(uint16_t), &dstOffset);

	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_drawPolyPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, *buffer, dstOffset, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vk_activeCmdbuffer, (p->numverts - 2) * 3, 1, 0, 0, 0);
}

static void
R_DrawTriangleOutlines(void)
{
	int			i, j, k;
	mpoly_t	*p;

	if (!r_showtris->value)
	{
		return;
	}

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	float color[3] = { 1.f, 1.f, 1.f };
	struct {
		vec3_t v;
		float color[3];
	} triVert[4];

	QVk_BindPipeline(&vk_showTrisPipeline);
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(r_viewproj_matrix), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, r_viewproj_matrix, sizeof(r_viewproj_matrix));
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_showTrisPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		msurface_t *surf;

		for (surf = vk_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
		{
			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2, k = 0; j < p->numverts; j++, k++)
				{
					VectorCopy(p->verts[0].pos, triVert[0].v);
					memcpy(triVert[0].color, color, sizeof(color));

					VectorCopy(p->verts[j - 1].pos, triVert[1].v);
					memcpy(triVert[1].color, color, sizeof(color));

					VectorCopy(p->verts[j].pos, triVert[2].v);
					memcpy(triVert[2].color, color, sizeof(color));

					VectorCopy(p->verts[0].pos, triVert[3].v);
					memcpy(triVert[3].color, color, sizeof(color));

					uint8_t *vertData = QVk_GetVertexBuffer(sizeof(triVert), &vbo, &vboOffset);
					memcpy(vertData, triVert, sizeof(triVert));

					vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
					vkCmdDraw(vk_activeCmdbuffer, 4, 1, 0, 0);
				}
			}
		}
	}
}

static void
R_RenderBrushPoly(msurface_t *fa, const float *modelMatrix, float alpha,
	const entity_t *currententity)
{
	int			maps;
	image_t		*image;
	qboolean is_dynamic = false;
	float		color[4] = { 1.f, 1.f, 1.f, alpha };
	c_brush_polys++;

	image = R_TextureAnimation(currententity, fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		color[0] = color[1] = color[2] = vk_state.inverse_intensity;
		color[3] = 1.f;
		// warp texture, no lightmaps
		EmitWaterPolys(fa, image, modelMatrix, color, alpha == 1.f);
		return;
	}

	//======
	//PGM
	DrawVkPoly(fa, image, color);
	//PGM
	//======

	/* check for lightmap modification */
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (r_newrefdef.lightstyles[fa->styles[maps]].white !=
			fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	/* dynamic this frame or dynamic previously */
	if (fa->dlightframe == r_framecount)
	{
	dynamic:

		if (r_dynamic->value)
		{
			if (!(fa->texinfo->flags &
				  (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if (maps < MAXLIGHTMAPS &&
			((fa->styles[maps] >= 32) ||
			 (fa->styles[maps] == 0)) &&
			  (fa->dlightframe != r_framecount))
		{
			int smax, tmax, size;
			byte *temp;

			smax = (fa->extents[0] >> fa->lmshift) + 1;
			tmax = (fa->extents[1] >> fa->lmshift) + 1;

			size = smax * tmax * LIGHTMAP_BYTES;
			temp = R_GetTemporaryLMBuffer(size);

			R_BuildLightMap(fa, temp, smax * 4,
				&r_newrefdef, r_modulate->value, r_framecount, NULL, NULL);
			R_SetCacheState(fa, &r_newrefdef);

			QVk_UpdateTextureData(&vk_state.lightmap_textures[fa->lightmaptexturenum],
				(byte*)temp, fa->light_s, fa->light_t, smax, tmax);

			fa->lightmapchain = vk_lms.lightmap_surfaces[fa->lightmaptexturenum];
			vk_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = vk_lms.lightmap_surfaces[0];
			vk_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = vk_lms.lightmap_surfaces[fa->lightmaptexturenum];
		vk_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}

/*
 * Draw water surfaces and windows.
 * The BSP tree is waled front to back, so unwinding the chain
 * of alpha_surfaces will draw back to front, giving proper ordering.
 */
void
R_DrawAlphaSurfaces(void)
{
	msurface_t *s;
	float intens;

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = vk_state.inverse_intensity;
	float color[4] = { intens, intens, intens, 1.f };

	for (s = r_alpha_surfaces; s; s = s->texturechain)
	{
		c_brush_polys++;

		if (s->texinfo->flags & SURF_TRANS33)
		{
			color[3] = 0.33f;
		}
		else if (s->texinfo->flags & SURF_TRANS66)
		{
			color[3] = 0.66f;
		}

		if (s->flags & SURF_DRAWTURB)
		{
			EmitWaterPolys(s, s->texinfo->image, NULL, color, false);
		}
		else
		{
			DrawVkPoly(s, s->texinfo->image, color);
		}
	}

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains

Draw world surfaces (mostly solid with alpha == 1.f)
================
*/
static void
DrawTextureChains(entity_t *currententity)
{
	int		i;
	msurface_t	*s;
	image_t		*image;

	c_visible_textures = 0;

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (!image->registration_sequence)
		{
			continue;
		}

		if (!image->texturechain)
		{
			continue;
		}

		c_visible_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
			{
				R_RenderBrushPoly(s, NULL, 1.f, currententity);
			}
		}
	}

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (!image->registration_sequence)
		{
			continue;
		}

		s = image->texturechain;
		if (!s)
		{
			continue;
		}

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
			{
				R_RenderBrushPoly(s, NULL, 1.f, currententity);
			}
		}

		image->texturechain = NULL;
	}
}


static void
Vk_RenderLightmappedPoly(msurface_t *surf, float alpha,
	const entity_t *currententity, VkDescriptorSet *uboDescriptorSet,
	uint32_t *uboOffset)
{
	int		i, nv = surf->polys->numverts;
	int		map;
	image_t *image = R_TextureAnimation(currententity, surf->texinfo);
	qboolean is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	mpoly_t *p;

	for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
	{
		if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	/* dynamic this frame or dynamic previously */
	if (surf->dlightframe == r_framecount)
	{
	dynamic:
		if (r_dynamic->value)
		{
			if (!(surf->texinfo->flags & (SURF_SKY | SURF_TRANSPARENT | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (Mesh_VertsRealloc(nv))
	{
		Com_Error(ERR_FATAL, "%s: can't allocate memory", __func__);
	}

	if (is_dynamic)
	{
		int smax, tmax, size;
		byte *temp;

		smax = (surf->extents[0] >> surf->lmshift) + 1;
		tmax = (surf->extents[1] >> surf->lmshift) + 1;

		size = smax * tmax * LIGHTMAP_BYTES;
		temp = R_GetTemporaryLMBuffer(size);

		R_BuildLightMap(surf, temp, smax * 4,
			&r_newrefdef, r_modulate->value, r_framecount, NULL, NULL);

		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) &&
			(surf->dlightframe != r_framecount))
		{
			R_SetCacheState(surf, &r_newrefdef);

			lmtex = surf->lightmaptexturenum;
		}
		else
		{
			lmtex = surf->lightmaptexturenum + DYNLIGHTMAP_OFFSET;
		}

		QVk_UpdateTextureData(&vk_state.lightmap_textures[lmtex],
			(byte*)temp, surf->light_s, surf->light_t, smax, tmax);
	}

	VkDeviceSize vboOffset, dstOffset;
	VkBuffer vbo, *buffer;
	uint8_t *vertData;
	float sscroll, tscroll;
	int pos_vect = 0, index_pos = 0;

	c_brush_polys++;

	//==========
	//PGM
	R_FlowingScroll(&r_newrefdef, surf->texinfo->flags, &sscroll, &tscroll);

	for (p = surf->polys; p; p = p->chain)
	{
		if (Mesh_VertsRealloc(pos_vect + nv))
		{
			Com_Error(ERR_FATAL, "%s: can't allocate memory", __func__);
		}

		memcpy(verts_buffer + pos_vect, p->verts, sizeof(mvtx_t) * nv);
		for (i = 0; i < nv; i++)
		{
			verts_buffer[pos_vect + i].texCoord[0] += sscroll;
			verts_buffer[pos_vect + i].texCoord[1] += tscroll;
		}

		GenFanIndexes(vertIdxData + index_pos,
			pos_vect, nv - 2 + pos_vect);
		pos_vect += nv;
		index_pos += (nv - 2) * 3;
	}

	QVk_BindPipeline(&vk_drawPolyLmapPipeline);

	VkDescriptorSet descriptorSets[] = {
		image->vk_texture.descriptorSet,
		*uboDescriptorSet,
		vk_state.lightmap_textures[lmtex].descriptorSet
	};

	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_drawPolyLmapPipeline.layout, 0, 3, descriptorSets, 1, uboOffset);

	vertData = QVk_GetVertexBuffer(sizeof(mvtx_t) * pos_vect, &vbo, &vboOffset);
	memcpy(vertData, verts_buffer, sizeof(mvtx_t) * pos_vect);

	buffer = UpdateIndexBuffer(vertIdxData, index_pos * sizeof(uint16_t), &dstOffset);

	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, *buffer, dstOffset, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vk_activeCmdbuffer, index_pos, 1, 0, 0, 0);
	//PGM
	//==========
}

static void
R_DrawInlineBModel(entity_t *currententity, const model_t *currentmodel, float *modelMatrix)
{
	int i;
	msurface_t *psurf;
	float alpha = 1.f;

	/* calculate dynamic lighting for bmodel */
	if (!r_flashblend->value)
	{
		R_PushDlights(&r_newrefdef, currentmodel->nodes + currentmodel->firstnode,
			r_dlightframecount, currentmodel->surfaces);
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		alpha = .25f;
	}

	struct {
		float model[16];
		float viewLightmaps;
	} lmapPolyUbo;

	lmapPolyUbo.viewLightmaps = r_lightmap->value ? 1.f : 0.f;

	if (modelMatrix)
	{
		memcpy(lmapPolyUbo.model, modelMatrix, sizeof(float) * 16);
	}
	else
	{
		Mat_Identity(lmapPolyUbo.model);
	}

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(lmapPolyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &lmapPolyUbo, sizeof(lmapPolyUbo));

	/* draw texture */
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++)
	{
		cplane_t *pplane;
		float dot;

		/* find which side of the node we are on */
		pplane = psurf->plane;

		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		/* draw the polygon */
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & SURF_TRANSPARENT)
			{
				/* add to the translucent chain */
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (!(psurf->flags & SURF_DRAWTURB) && !r_showtris->value)
			{
				Vk_RenderLightmappedPoly(psurf, alpha, currententity,
					&uboDescriptorSet, &uboOffset);
			}
			else
			{
				R_RenderBrushPoly(psurf, modelMatrix, alpha, currententity);
			}
		}
	}
}

void
R_DrawBrushModel(entity_t *currententity, const model_t *currentmodel)
{
	vec3_t mins, maxs;
	qboolean rotated;
	float model[16];

	if (currentmodel->nummodelsurfaces == 0)
	{
		return;
	}

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		int	i;

		rotated = true;

		for (i = 0; i < 3; i++)
		{
			mins[i] = currententity->origin[i] - currentmodel->radius;
			maxs[i] = currententity->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(currententity->origin, currentmodel->mins, mins);
		VectorAdd(currententity->origin, currentmodel->maxs, maxs);
	}

	if (r_cull->value && R_CullBox(mins, maxs, frustum))
	{
		return;
	}

	memset(vk_lms.lightmap_surfaces, 0, sizeof(vk_lms.lightmap_surfaces));

	VectorSubtract(r_newrefdef.vieworg, currententity->origin, modelorg);

	if (rotated)
	{
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(currententity->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	Mat_Identity(model);

	currententity->angles[0] = -currententity->angles[0];	// stupid quake bug
	currententity->angles[2] = -currententity->angles[2];	// stupid quake bug
	R_RotateForEntity(currententity, model);
	currententity->angles[0] = -currententity->angles[0];	// stupid quake bug
	currententity->angles[2] = -currententity->angles[2];	// stupid quake bug

	R_DrawInlineBModel(currententity, currentmodel, model);
}

static void
R_RecursiveWorldNode(entity_t *currententity, mnode_t *node)
{
	int c, side, sidebit;
	cplane_t *plane;
	msurface_t *surf;
	mleaf_t *pleaf;
	float dot;
	image_t *image;

	if (node->contents == CONTENTS_SOLID)
	{
		return; /* solid */
	}

	if (node->visframe != r_visframecount)
	{
		return;
	}

	if (r_cull->value && R_CullBox(node->minmaxs, node->minmaxs + 3, frustum))
	{
		return;
	}

	/* if a leaf node, draw stuff */
	if (node->contents != CONTENTS_NODE)
	{
		msurface_t	**mark;

		pleaf = (mleaf_t *)node;

		/* check for door connected areas */
		if (!R_AreaVisible(r_newrefdef.areabits, pleaf))
			return;	// not visible

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			}
			while (--c);
		}

		return;
	}

	/* node is just a decision point, so go down the apropriate
	   sides find which side of the node we are on */
	plane = node->plane;

	switch (plane->type)
	{
		case PLANE_X:
			dot = modelorg[0] - plane->dist;
			break;
		case PLANE_Y:
			dot = modelorg[1] - plane->dist;
			break;
		case PLANE_Z:
			dot = modelorg[2] - plane->dist;
			break;
		default:
			dot = DotProduct(modelorg, plane->normal) - plane->dist;
			break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	/* recurse down the children, front side first */
	R_RecursiveWorldNode(currententity, node->children[side]);

	if ((node->numsurfaces + node->firstsurface) > r_worldmodel->numsurfaces)
	{
		Com_Printf("Broken node firstsurface\n");
		return;
	}

	struct {
		float model[16];
		float viewLightmaps;
	} lmapPolyUbo;

	lmapPolyUbo.viewLightmaps = r_lightmap->value ? 1.f : 0.f;
	Mat_Identity(lmapPolyUbo.model);

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(lmapPolyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &lmapPolyUbo, sizeof(lmapPolyUbo));

	/* draw stuff */
	for (c = node->numsurfaces,
		 surf = r_worldmodel->surfaces + node->firstsurface;
		 c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
		{
			continue;
		}

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
		{
			continue; /* wrong side */
		}

		if (surf->texinfo->flags & SURF_SKY)
		{
			/* just adds to visible sky bounds */
			RE_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & SURF_TRANSPARENT)
		{
			/* add to the translucent chain */
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else if (surf->texinfo->flags & SURF_NODRAW)
		{
			/* Surface should be skipped */
			continue;
		}
		else
		{
			if (!(surf->flags & SURF_DRAWTURB) && !r_showtris->value)
			{
				Vk_RenderLightmappedPoly(surf, 1.f, currententity,
					&uboDescriptorSet, &uboOffset);
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation(currententity, surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	/* recurse down the back side */
	R_RecursiveWorldNode(currententity, node->children[!side]);
}

void
R_DrawWorld(void)
{
	entity_t ent;

	if (!r_drawworld->value)
	{
		return;
	}

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	VectorCopy(r_newrefdef.vieworg, modelorg);

	/* auto cycle the world frame for texture animation */
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time * 2);

	memset(vk_lms.lightmap_surfaces, 0, sizeof(vk_lms.lightmap_surfaces));

	RE_ClearSkyBox();
	R_RecursiveWorldNode(&ent, r_worldmodel->nodes);

	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled - in practice, this code renders non-transparent liquids!
	*/
	DrawTextureChains(&ent);
	R_DrawSkyBox();
	R_DrawTriangleOutlines();
}

/*
 * Mark the leaves and nodes that are
 * in the PVS for the current cluster
 */
void
R_MarkLeaves(void)
{
	const byte *vis;
	byte *fatvis = NULL;
	mnode_t *node;
	int i;
	mleaf_t *leaf;

	if ((r_oldviewcluster == r_viewcluster) &&
		(r_oldviewcluster2 == r_viewcluster2) &&
		!r_novis->value &&
		(r_viewcluster != -1))
	{
		return;
	}

	/* development aid to let you run around
	   and see exactly where the pvs ends */
	if (r_lockpvs->value)
	{
		return;
	}

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || (r_viewcluster == -1) || !r_worldmodel->vis)
	{
		/* mark everything */
		for (i = 0; i < r_worldmodel->numleafs; i++)
		{
			r_worldmodel->leafs[i].visframe = r_visframecount;
		}

		for (i = 0; i < r_worldmodel->numnodes; i++)
		{
			r_worldmodel->nodes[i].visframe = r_visframecount;
		}

		return;
	}

	vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel);

	/* may have to combine two clusters because of solid water boundaries */
	if (r_viewcluster2 != r_viewcluster)
	{
		int c;

		fatvis = malloc(((r_worldmodel->numleafs + 31) / 32) * sizeof(int));
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;

		for (i = 0; i < c; i++)
		{
			((int *)fatvis)[i] |= ((int *)vis)[i];
		}

		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs;
		 i < r_worldmodel->numleafs;
		 i++, leaf++)
	{
		int cluster;

		cluster = leaf->cluster;

		if (cluster == -1)
		{
			continue;
		}

		if (vis[cluster >> 3] & (1 << (cluster & 7)))
		{
			node = (mnode_t *)leaf;

			do
			{
				if (node->visframe == r_visframecount)
				{
					break;
				}

				node->visframe = r_visframecount;
				node = node->parent;
			}
			while (node);
		}
	}

	/* clean combined buffer */
	if (fatvis)
	{
		free(fatvis);
	}
}
