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
 */

/*
** QVK.H
*/

#ifndef __QVK_H__
#define __QVK_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#include "local.h"
#include "util.h"
#include "shaders.h"

#if defined(__APPLE__)
#undef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES 1
#include <MoltenVK/vk_mvk_moltenvk.h>
#endif

// Vulkan device
typedef struct
{
	VkPhysicalDevice physical;
	VkDevice		 logical;
	VkPhysicalDeviceMemoryProperties mem_properties;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures   features;
#if defined(__APPLE__)
	MVKPhysicalDeviceMetalFeatures metalFeatures;
#endif
	VkQueue gfxQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	int gfxFamilyIndex;
	int presentFamilyIndex;
	int transferFamilyIndex;
	qboolean screenshotSupported;
} qvkdevice_t;

// Vulkan swapchain
typedef struct
{
	VkSwapchainKHR sc;
	VkFormat format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	VkImage *images;
	int imageCount;
} qvkswapchain_t;

// available sampler types
typedef enum
{
	S_NEAREST = 0,
	S_LINEAR = 1,
	S_MIPMAP_NEAREST = 2,
	S_MIPMAP_LINEAR = 3,
	S_NEAREST_UNNORMALIZED = 4,
	S_SAMPLER_CNT = 5
} qvksampler_t;

#define NUM_SAMPLERS (S_SAMPLER_CNT * 2)

// texture object
typedef struct
{
	ImageResource_t resource;

	VkImageView   imageView;
	VkSharingMode sharingMode;
	VkSampleCountFlagBits sampleCount;
	VkFormat  format;
	VkDescriptorSet descriptorSet;
	uint32_t mipLevels;
	qboolean clampToEdge;
} qvktexture_t;

#define QVVKTEXTURE_INIT     { \
	.resource = { \
		.image = VK_NULL_HANDLE, \
		.memory = VK_NULL_HANDLE, \
		.size = 0, \
	}, \
	.imageView = VK_NULL_HANDLE, \
	.sharingMode = VK_SHARING_MODE_MAX_ENUM, \
	.sampleCount = VK_SAMPLE_COUNT_1_BIT, \
	.format = VK_FORMAT_R8G8B8A8_UNORM, \
	.descriptorSet = VK_NULL_HANDLE, \
	.mipLevels = 1, \
}

#define QVVKTEXTURE_CLEAR(i)     { \
	(i).resource.image = VK_NULL_HANDLE; \
	(i).resource.memory = VK_NULL_HANDLE; \
	(i).resource.size = 0; \
	(i).imageView = VK_NULL_HANDLE; \
	(i).sharingMode = VK_SHARING_MODE_MAX_ENUM; \
	(i).sampleCount = VK_SAMPLE_COUNT_1_BIT; \
	(i).format = VK_FORMAT_R8G8B8A8_UNORM; \
	(i).mipLevels = 1; \
}

// Vulkan renderpass
typedef struct
{
	VkRenderPass rp;
	VkAttachmentLoadOp colorLoadOp;
	VkSampleCountFlagBits sampleCount;
} qvkrenderpass_t;

// Vulkan buffer
typedef struct
{
	VkDeviceSize currentOffset;

	BufferResource_t resource;
	void *pMappedData;
} qvkbuffer_t;

// Vulkan staging buffer
typedef struct
{
	VkDeviceSize currentOffset;
	VkCommandBuffer cmdBuffer;
	VkFence fence;
	qboolean submitted;

	BufferResource_t resource;
	void *pMappedData;
} qvkstagingbuffer_t;

// Vulkan buffer options
typedef struct
{
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags reqMemFlags;
	VkMemoryPropertyFlags prefMemFlags;
} qvkbufferopts_t;

// Vulkan pipeline
typedef struct
{
	VkPipelineLayout layout;
	VkPipeline pl;
	VkPipelineCreateFlags flags;
	VkCullModeFlags cullMode;
	VkPrimitiveTopology topology;
	VkPipelineColorBlendAttachmentState blendOpts;
	VkBool32 depthTestEnable;
	VkBool32 depthWriteEnable;
} qvkpipeline_t;

// Vulkan shader
typedef struct
{
	VkPipelineShaderStageCreateInfo createInfo;
	VkShaderModule module;
} qvkshader_t;

#define QVKPIPELINE_INIT { \
	.layout = VK_NULL_HANDLE, \
	.pl = VK_NULL_HANDLE, \
	.flags = 0, \
	.cullMode = VK_CULL_MODE_BACK_BIT, \
	.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, \
	.blendOpts = { \
		.blendEnable = VK_FALSE, \
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, \
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, \
		.colorBlendOp = VK_BLEND_OP_ADD, \
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, \
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, \
		.alphaBlendOp = VK_BLEND_OP_ADD, \
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT \
	}, \
	.depthTestEnable = VK_TRUE, \
	.depthWriteEnable = VK_TRUE \
}

// renderpass type
typedef enum
{
	RP_WORLD = 0,      // renders game world to offscreen buffer
	RP_UI = 1,         // render UI elements and game console
	RP_WORLD_WARP = 2, // perform postprocessing on RP_WORLD (underwater screen warp)
	RP_COUNT = 3
} qvkrenderpasstype_t;

// Vulkan constants: command and dynamic buffer count
#define NUM_CMDBUFFERS 32
#define NUM_DYNBUFFERS 2

// Vulkan instance
extern VkInstance vk_instance;
// Vulkan surface
extern VkSurfaceKHR vk_surface;
// Vulkan device
extern qvkdevice_t vk_device;
// Vulkan swapchain
extern qvkswapchain_t vk_swapchain;
// Vulkan command buffer currently in use
extern VkCommandBuffer vk_activeCmdbuffer;
// Vulkan command pools
extern VkCommandPool vk_commandPool[NUM_CMDBUFFERS];
extern VkCommandPool vk_transferCommandPool;
// Vulkan descriptor pool
extern VkDescriptorPool vk_descriptorPool;
// viewport/scissor
extern VkViewport vk_viewport;
extern VkRect2D vk_scissor;

// Vulkan descriptor sets
extern VkDescriptorSetLayout vk_samplerDescSetLayout;

// *** pipelines ***
extern qvkpipeline_t vk_drawTexQuadPipeline[RP_COUNT];
extern qvkpipeline_t vk_drawColorQuadPipeline[RP_COUNT];
extern qvkpipeline_t vk_drawModelPipelineFan[RP_COUNT];
extern qvkpipeline_t vk_drawNoDepthModelPipelineFan;
extern qvkpipeline_t vk_drawLefthandModelPipelineFan;
extern qvkpipeline_t vk_drawNullModelPipeline;
extern qvkpipeline_t vk_drawParticlesPipeline;
extern qvkpipeline_t vk_drawPointParticlesPipeline;
extern qvkpipeline_t vk_drawSpritePipeline;
extern qvkpipeline_t vk_drawPolyPipeline;
extern qvkpipeline_t vk_drawPolyLmapPipeline;
extern qvkpipeline_t vk_drawPolyWarpPipeline;
extern qvkpipeline_t vk_drawPolySolidWarpPipeline;
extern qvkpipeline_t vk_drawBeamPipeline;
extern qvkpipeline_t vk_drawSkyboxPipeline;
extern qvkpipeline_t vk_drawDLightPipeline;
extern qvkpipeline_t vk_showTrisPipeline;
extern qvkpipeline_t vk_shadowsPipelineFan;
extern qvkpipeline_t vk_worldWarpPipeline;
extern qvkpipeline_t vk_postprocessPipeline;

// color buffer containing main game/world view
extern qvktexture_t vk_colorbuffer;
// color buffer with postprocessed game view
extern qvktexture_t vk_colorbufferWarp;
// indicator if the frame is currently being rendered
extern qboolean vk_frameStarted;
// Indicates if the swap chain needs to be rebuilt.
extern qboolean vk_recreateSwapchainNeeded;
// is QVk initialized?
extern qboolean vk_initialized;

// function pointers
extern PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;
extern PFN_vkSetDebugUtilsObjectNameEXT qvkSetDebugUtilsObjectNameEXT;
extern PFN_vkSetDebugUtilsObjectTagEXT qvkSetDebugUtilsObjectTagEXT;
extern PFN_vkCmdBeginDebugUtilsLabelEXT qvkCmdBeginDebugUtilsLabelEXT;
extern PFN_vkCmdEndDebugUtilsLabelEXT qvkCmdEndDebugUtilsLabelEXT;
extern PFN_vkCmdInsertDebugUtilsLabelEXT qvkInsertDebugUtilsLabelEXT;
extern PFN_vkCreateDebugReportCallbackEXT qvkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT qvkDestroyDebugReportCallbackEXT;
#if defined(__APPLE__)
extern PFN_vkGetPhysicalDeviceMetalFeaturesMVK qvkGetPhysicalDeviceMetalFeaturesMVK;
extern PFN_vkGetMoltenVKConfigurationMVK qvkGetMoltenVKConfigurationMVK;
extern PFN_vkSetMoltenVKConfigurationMVK qvkSetMoltenVKConfigurationMVK;
#endif

// The Interface Functions (tm)
void		QVk_SetWindow(SDL_Window*);
qboolean	QVk_Init(void);
void		QVk_PostInit(void);
void		QVk_GetDrawableSize(int *width, int *height);
void		QVk_WaitAndShutdownAll(void);
void		QVk_Shutdown(void);
void		QVk_Restart(void);
void		QVk_CreateValidationLayers(void);
void		QVk_DestroyValidationLayers(void);
qboolean	QVk_CreateDevice(int preferredDeviceIdx);
VkResult	QVk_CreateSwapchain(void);
VkFormat	QVk_FindDepthFormat(void);
VkResult	QVk_CreateCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex);
VkResult	QVk_CreateImageView(const VkImage *image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format, uint32_t mipLevels);
VkResult	QVk_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, qvktexture_t *texture);
void		QVk_CreateDepthBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *depthBuffer);
void		QVk_CreateColorBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *colorBuffer, int extraFlags);
void		QVk_CreateTexture(qvktexture_t *texture, const unsigned char *data, uint32_t width, uint32_t height, qvksampler_t samplerType, qboolean clampToEdge);
void		QVk_ReleaseTexture(qvktexture_t *texture);
void		QVk_UpdateTextureData(qvktexture_t *texture, const unsigned char *data, uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height);
VkSampler	QVk_UpdateTextureSampler(qvktexture_t *texture, qvksampler_t samplerType, qboolean clampToEdge);
void		QVk_ReadPixels(uint8_t *dstBuffer, const VkOffset2D *offset, const VkExtent2D *extent);
VkResult	QVk_BeginCommand(const VkCommandBuffer *commandBuffer);
void		QVk_SubmitCommand(const VkCommandBuffer *commandBuffer, const VkQueue *queue);
VkCommandBuffer	QVk_CreateCommandBuffer(const VkCommandPool *commandPool, VkCommandBufferLevel level);
const char*	QVk_GetError(VkResult errorCode);
VkResult	QVk_BeginFrame(const VkViewport* viewport, const VkRect2D* scissor);
VkResult	QVk_EndFrame(qboolean force);
void		QVk_BeginRenderpass(qvkrenderpasstype_t rpType);
qboolean	QVk_RecreateSwapchain();
void		QVk_FreeStagingBuffer(qvkstagingbuffer_t *buffer);
VkResult	QVk_CreateBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, const qvkbufferopts_t options);
void		QVk_FreeBuffer(qvkbuffer_t *buffer);
VkResult	QVk_CreateStagingBuffer(VkDeviceSize size, qvkstagingbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
VkResult	QVk_CreateUniformBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
void		QVk_CreateVertexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
void		QVk_CreateIndexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
qvkshader_t	QVk_CreateShader(const uint32_t *shaderSrc, size_t shaderCodeSize, VkShaderStageFlagBits shaderStage);
void		QVk_CreatePipeline(const VkDescriptorSetLayout *descriptorLayout, const uint32_t descLayoutCount, const VkPipelineVertexInputStateCreateInfo *vertexInputInfo, qvkpipeline_t *pipeline, const qvkrenderpass_t *renderpass, const qvkshader_t *shaders, uint32_t shaderCount);
void		QVk_DestroyPipeline(qvkpipeline_t *pipeline);
uint8_t*	QVk_GetVertexBuffer(VkDeviceSize size, VkBuffer *dstBuffer, VkDeviceSize *dstOffset);
uint8_t*	QVk_GetUniformBuffer(VkDeviceSize size, uint32_t *dstOffset, VkDescriptorSet *dstUboDescriptorSet);
uint8_t*	QVk_GetStagingBuffer(VkDeviceSize size, int alignment, VkCommandBuffer *cmdBuffer, VkBuffer *buffer, uint32_t *dstOffset);
void		GenFanIndexes(uint16_t *data, int from, int to);
void		GenStripIndexes(uint16_t *data, int from, int to);
VkBuffer*	UpdateIndexBuffer(uint16_t *data, VkDeviceSize bufferSize, VkDeviceSize *dstOffset);
void		QVk_DrawColorRect(float *ubo, VkDeviceSize uboSize, qvkrenderpasstype_t rpType);
void		QVk_DrawTexRect(const float *ubo, VkDeviceSize uboSize, qvktexture_t *texture);
void		QVk_BindPipeline(qvkpipeline_t *pipeline);
void		QVk_SubmitStagingBuffers(void);
void		Qvk_MemoryBarrier(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);
qboolean	QVk_CheckExtent(void);

// debug label related functions
void		QVk_DebugSetObjectName(uint64_t obj, VkObjectType objType, const char *objName);
void		QVk_DebugSetObjectTag(uint64_t obj, VkObjectType objType, uint64_t tagName, size_t tagSize, const void *tagData);
void		QVk_DebugLabelBegin(const VkCommandBuffer *cmdBuffer, const char *labelName, const float r, const float g, const float b);
void		QVk_DebugLabelEnd(const VkCommandBuffer *cmdBuffer);
void		QVk_DebugLabelInsert(const VkCommandBuffer *cmdBuffer, const char *labelName, const float r, const float g, const float b);
#endif
