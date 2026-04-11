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
 * Lightmaps and dynamic lighting
 *
 * =======================================================================
 */

#include "header/local.h"

int r_dlightframecount;
vec3_t lightspot;

void
R_RenderDlights(void)
{
	typedef struct {
		vec3_t verts;
		float color[3];
	} lightVert_t;

	VkBuffer vbo, *ibuffer;
	VkDeviceSize vboOffset, dstOffset;
	VkDescriptorSet uboDescriptorSet;
	lightVert_t *vertData;
	uint8_t *uboData;
	uint32_t uboOffset;
	int k;

	if (!r_flashblend->value)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	if (!r_newrefdef.num_dlights)
	{
		return;
	}

	QVk_BindPipeline(&vk_drawDLightPipeline);

	vertData = (lightVert_t *)QVk_GetVertexBuffer(
		sizeof(lightVert_t) * 18 * r_newrefdef.num_dlights, &vbo, &vboOffset);
	uboData = QVk_GetUniformBuffer(sizeof(r_viewproj_matrix),
		&uboOffset, &uboDescriptorSet);
	memcpy(uboData, r_viewproj_matrix, sizeof(r_viewproj_matrix));

	for (k = 0; k < r_newrefdef.num_dlights; k++)
	{
		const dlight_t *light = &r_newrefdef.dlights[k];
		lightVert_t *lv = vertData + k * 18;
		float rad = light->intensity * 0.35;
		int i, j;

		for (i = 0; i < 3; i++)
		{
			lv[0].verts[i] = light->origin[i] - vpn[i] * rad;
		}

		lv[0].color[0] = light->color[0] * 0.2;
		lv[0].color[1] = light->color[1] * 0.2;
		lv[0].color[2] = light->color[2] * 0.2;

		for (i = 16; i >= 0; i--)
		{
			float a = i / 16.0 * M_PI * 2;

			for (j = 0; j < 3; j++)
			{
				lv[i + 1].verts[j] = light->origin[j]
					+ vright[j] * cos(a) * rad
					+ vup[j] * sin(a) * rad;
				lv[i + 1].color[j] = 0.f;
			}
		}
	}

	Mesh_VertsRealloc(64);
	R_GenFanIndexes(vertIdxData, 0, 48);
	ibuffer = UpdateIndexBuffer(vertIdxData, 48 * sizeof(uint16_t), &dstOffset);

	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_drawDLightPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, *ibuffer, dstOffset, VK_INDEX_TYPE_UINT16);

	for (k = 0; k < r_newrefdef.num_dlights; k++)
	{
		vkCmdDrawIndexed(vk_activeCmdbuffer, 48, 1, 0, k * 18, 0);
	}
}

void
RI_PushDlights(void)
{
	if (r_flashblend->value || !r_worldmodel)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	R_PushDlights(&r_newrefdef, r_worldmodel->nodes,
			r_dlightframecount, r_worldmodel->surfaces);
}
