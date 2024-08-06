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

#include "header/local.h"

#define ON_EPSILON 0.1 /* point on plane side epsilon */
#define MAX_CLIP_VERTS 64

static float skyrotate;
static int skyautorotate;
static vec3_t skyaxis;
static image_t *sky_images[6];
static const int skytexorder[6] = {0, 2, 1, 3, 4, 5};

/* 3dstudio environment map names */
static const char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

static float skymins[2][6], skymaxs[2][6];
static float sky_min, sky_max;

/*
 * Does a water warp on the pre-fragmented mpoly_t chain
 */
void
EmitWaterPolys(msurface_t *fa, image_t *texture, const float *modelMatrix,
			  const float *color, qboolean solid_surface)
{
	mpoly_t *p, *bp;
	int i;

	struct {
		float model[16];
		float color[4];
		float time;
		float sscroll;
		float tscroll;
	} polyUbo;

	VkBuffer *buffer;
	VkDeviceSize dstOffset;

	polyUbo.color[0] = color[0];
	polyUbo.color[1] = color[1];
	polyUbo.color[2] = color[2];
	polyUbo.color[3] = color[3];
	polyUbo.time = r_newrefdef.time;

	R_FlowingScroll(&r_newrefdef, fa->texinfo->flags, &polyUbo.sscroll, &polyUbo.tscroll);
	polyUbo.sscroll /= 64.f;
	polyUbo.tscroll /= 64.f;

	if (modelMatrix)
	{
		memcpy(polyUbo.model, modelMatrix, sizeof(float) * 16);
	}
	else
	{
		Mat_Identity(polyUbo.model);
	}

	if (solid_surface)
	{
		// Solid surface
		QVk_BindPipeline(&vk_drawPolySolidWarpPipeline);
	}
	else
	{
		// Blend surface
		QVk_BindPipeline(&vk_drawPolyWarpPipeline);
	}

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(polyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &polyUbo, sizeof(polyUbo));

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	VkDescriptorSet descriptorSets[] = {
		texture->vk_texture.descriptorSet,
		uboDescriptorSet
	};
	int pos_vect = 0, index_pos = 0;

	float gamma = 2.1F - vid_gamma->value;

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(gamma), &gamma);

	if (solid_surface)
	{
		// Solid surface
		vkCmdBindDescriptorSets(
			vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_drawPolySolidWarpPipeline.layout, 0, 2,
			descriptorSets, 1, &uboOffset);
	}
	else
	{
		// Blend surface
		vkCmdBindDescriptorSets(
			vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_drawPolyWarpPipeline.layout, 0, 2,
			descriptorSets, 1, &uboOffset);
	}

	for (bp = fa->polys; bp; bp = bp->next)
	{
		p = bp;

		if (Mesh_VertsRealloc(pos_vect + p->numverts))
		{
			Com_Error(ERR_FATAL, "%s: can't allocate memory", __func__);
		}

		memcpy(verts_buffer + pos_vect, p->verts, sizeof(mvtx_t) * p->numverts);
		for (i = 0; i < p->numverts; i++)
		{
			verts_buffer[i + pos_vect].texCoord[0] /= 64.f;
			verts_buffer[i + pos_vect].texCoord[1] /= 64.f;
		}
		GenFanIndexes(vertIdxData + index_pos,
			pos_vect, p->numverts - 2 + pos_vect);
		pos_vect += p->numverts;
		index_pos += (p->numverts - 2) * 3;
	}

	uint8_t *vertData = QVk_GetVertexBuffer(sizeof(mvtx_t) * pos_vect, &vbo, &vboOffset);
	memcpy(vertData, verts_buffer, sizeof(mvtx_t) * pos_vect);

	buffer = UpdateIndexBuffer(vertIdxData, index_pos * sizeof(uint16_t), &dstOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, *buffer, dstOffset, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vk_activeCmdbuffer, index_pos, 1, 0, 0, 0);
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

void
R_DrawSkyBox(void)
{
	VkDeviceSize dstOffset;
	VkBuffer *buffer;
	qboolean farsee;
	int i;

	farsee = (r_farsee->value == 0);

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

	float model[16];
	Mat_Identity(model);
	Mat_Rotate(model, (skyautorotate ? r_newrefdef.time : 1.f) * skyrotate,
		skyaxis[0], skyaxis[1], skyaxis[2]);
	Mat_Translate(model, r_origin[0], r_origin[1], r_origin[2]);

	mvtx_t skyVerts[4];

	QVk_BindPipeline(&vk_drawSkyboxPipeline);
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(model), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, model, sizeof(model));

	Mesh_VertsRealloc(6);
	GenFanIndexes(vertIdxData, 0, 4);
	buffer = UpdateIndexBuffer(vertIdxData, 6 * sizeof(uint16_t), &dstOffset);

	for (i = 0; i < 6; i++)
	{
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

		R_MakeSkyVec(skymins[0][i], skymins[1][i], i, &skyVerts[0],
			farsee, sky_min, sky_max);
		R_MakeSkyVec(skymins[0][i], skymaxs[1][i], i, &skyVerts[1],
			farsee, sky_min, sky_max);
		R_MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i, &skyVerts[2],
			farsee, sky_min, sky_max);
		R_MakeSkyVec(skymaxs[0][i], skymins[1][i], i, &skyVerts[3],
			farsee, sky_min, sky_max);

		VkBuffer vbo;
		VkDeviceSize vboOffset;
		uint8_t *vertData = QVk_GetVertexBuffer(sizeof(mvtx_t) * 4, &vbo, &vboOffset);
		memcpy(vertData, skyVerts, sizeof(mvtx_t) * 4);

		VkDescriptorSet descriptorSets[] = {
			sky_images[skytexorder[i]]->vk_texture.descriptorSet,
			uboDescriptorSet
		};

		float gamma = 2.1F - vid_gamma->value;

		vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
			VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(gamma), &gamma);

		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vk_drawSkyboxPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);
		vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);

		vkCmdBindIndexBuffer(vk_activeCmdbuffer, *buffer, dstOffset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
	}
}

void
RE_SetSky(const char *name, float rotate, int autorotate, const vec3_t axis)
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
			r_palettedtexture->value, (findimage_t)Vk_FindImage);

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
