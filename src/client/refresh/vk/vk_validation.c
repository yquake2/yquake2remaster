/*
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

#include "header/local.h"

static VkDebugUtilsMessengerEXT validationMessenger = VK_NULL_HANDLE;

// layer message to string
static const char* msgToString(VkDebugUtilsMessageTypeFlagsEXT type)
{
	int g = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0 ? 1 : 0;
	int p = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0 ? 1 : 0;
	int v = (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0 ? 1 : 0;

	if (g) return "";
	if (p && !v) return "(performance)";
	if (p &&  v) return "(performance and validation)";
	if (v) return "(validation)";

	return "?";
}

// validation layer callback function (VK_EXT_debug_utils)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
														 VkDebugUtilsMessageTypeFlagsEXT msgType,
														 const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
														 void* userData)
{
	switch (msgSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		Com_Printf("VK_INFO: %s %s\n", callbackData->pMessage, msgToString(msgType));
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		Com_Printf("VK_VERBOSE: %s %s\n", callbackData->pMessage, msgToString(msgType));
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		Com_Printf("VK_WARNING: %s %s\n", callbackData->pMessage, msgToString(msgType));
		break;
	default:
		Com_Printf("VK_ERROR: %s %s\n", callbackData->pMessage, msgToString(msgType));
		assert(!"Vulkan error occured!");
	}
	return VK_FALSE;
}

static VkDebugReportCallbackEXT validationLayerCallback = VK_NULL_HANDLE;

// validation layer callback function (VK_EXT_debug_report)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackReport(VkDebugReportFlagsEXT flags,
                                                          VkDebugReportObjectTypeEXT objType,
                                                          uint64_t obj, size_t location, int32_t code,
                                                          const char *layerPrefix, const char *msg,
                                                          void* userData)
{
    switch (flags)
    {
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
		Com_Printf("VK_INFO: %s %s\n", layerPrefix, msg);
        break;
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
		Com_Printf("VK_DEBUG: %s %s\n", layerPrefix, msg);
        break;
	case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
		Com_Printf("VK_PERFORMANCE: %s %s\n", layerPrefix, msg);
        break;
	case VK_DEBUG_REPORT_WARNING_BIT_EXT:
		Com_Printf("VK_WARNING: %s %s\n", layerPrefix, msg);
        break;
        default:
		Com_Printf("VK_ERROR: %s %s\n", layerPrefix, msg);
        break;
    }
    return VK_FALSE;
}

void QVk_CreateValidationLayers()
{
	VkDebugUtilsMessengerCreateInfoEXT callbackInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = 0,
		.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debugUtilsCallback,
		.pUserData = NULL
	};

	VkDebugReportCallbackCreateInfoEXT callbackReport = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
		.pfnCallback = debugCallbackReport,
		.pUserData = NULL
	};

	if (r_validation->value > 1)
	{
		callbackInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	}

	if (qvkCreateDebugUtilsMessengerEXT)
	{
		VK_VERIFY(qvkCreateDebugUtilsMessengerEXT(vk_instance, &callbackInfo, NULL, &validationMessenger));
	}

	if (qvkCreateDebugReportCallbackEXT)
	{
		VK_VERIFY(qvkCreateDebugReportCallbackEXT(vk_instance, &callbackReport, NULL, &validationLayerCallback));
		Com_Printf("...Vulkan validation layers enabled\n");
	}
}

void QVk_DestroyValidationLayers()
{
	if( validationMessenger != VK_NULL_HANDLE && qvkDestroyDebugUtilsMessengerEXT)
	{
		qvkDestroyDebugUtilsMessengerEXT( vk_instance, validationMessenger, NULL );
		validationMessenger = VK_NULL_HANDLE;
	}

	if ( validationLayerCallback != VK_NULL_HANDLE && qvkDestroyDebugReportCallbackEXT)
	{
		qvkDestroyDebugReportCallbackEXT( vk_instance, validationLayerCallback, NULL );
		validationLayerCallback = VK_NULL_HANDLE;
	}
}
