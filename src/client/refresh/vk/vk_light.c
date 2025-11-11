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

static void
R_RenderDlight(dlight_t *light)
{
	VkDeviceSize vboOffset, dstOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData, *uboData;
	VkBuffer vbo, *buffer;
	uint32_t uboOffset;
	float rad;
	int i, j;

	rad = light->intensity * 0.35;

	struct {
		vec3_t verts;
		float color[3];
	} lightVerts[18];

	for (i = 0; i < 3; i++)
	{
		lightVerts[0].verts[i] = light->origin[i] - vpn[i] * rad;
	}

	lightVerts[0].color[0] = light->color[0] * 0.2;
	lightVerts[0].color[1] = light->color[1] * 0.2;
	lightVerts[0].color[2] = light->color[2] * 0.2;

	for (i = 16; i >= 0; i--)
	{
		float	a;

		a = i / 16.0 * M_PI * 2;
		for (j = 0; j < 3; j++)
		{
			lightVerts[i+1].verts[j] = light->origin[j] + vright[j] * cos(a)*rad
				+ vup[j] * sin(a)*rad;
			lightVerts[i+1].color[j] = 0.f;
		}
	}

	QVk_BindPipeline(&vk_drawDLightPipeline);

	vertData = QVk_GetVertexBuffer(sizeof(lightVerts), &vbo, &vboOffset);
	uboData = QVk_GetUniformBuffer(sizeof(r_viewproj_matrix), &uboOffset, &uboDescriptorSet);
	memcpy(vertData, lightVerts, sizeof(lightVerts));
	memcpy(uboData,  r_viewproj_matrix, sizeof(r_viewproj_matrix));

	Mesh_VertsRealloc(64);
	R_GenFanIndexes(vertIdxData, 0, 48);
	buffer = UpdateIndexBuffer(vertIdxData, 48 * sizeof(uint16_t), &dstOffset);

	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawDLightPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, *buffer, dstOffset, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 48, 1, 0, 0, 0);
}

void
R_RenderDlights(void)
{
	int i;
	dlight_t *l;

	if (!r_flashblend->value)
	{
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	l = r_newrefdef.dlights;

	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_RenderDlight(l);
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
